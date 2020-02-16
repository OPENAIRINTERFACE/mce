/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file m3ap_mme_procedures.c
  \brief
  \author Dincer Beken
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "assertions.h"
#include "hashtable.h"
#include "log.h"
#include "msc.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "asn1_conversions.h"
#include "timer.h"

#include "mce_config.h"
#include "common_types_mbms.h"
#include "m3ap_common.h"
#include "mxap_main.h"
#include "m3ap_mme_encoder.h"
#include "m3ap_mme.h"
#include "m3ap_mme_itti_messaging.h"
#include "m3ap_mme_procedures.h"

/* Every time a new MBMS service is associated, increment this variable.
   But care if it wraps to increment also the mme_mbms_m3ap_id_has_wrapped
   variable. Limit: UINT32_MAX (in stdint.h).
*/
extern const char              *m3ap_direction2String[];
extern hash_table_ts_t 					g_m3ap_mbms_coll; 	// MME MBMS M3AP ID association to MBMS Reference;
extern hash_table_ts_t 					g_m3ap_mce_coll; 		// SCTP Association ID association to M3AP MCE Reference;

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/
static void m3ap_update_mbms_service_context(const mme_mbms_m3ap_id_t mme_mbms_m3ap_id);
static int m3ap_generate_mbms_session_start_request(mme_mbms_m3ap_id_t mbms_m3ap_id, const uint8_t num_m3ap_mces, m3ap_mce_description_t ** m3ap_mce_descriptions);
static int m3ap_generate_mbms_session_update_request(mme_mbms_m3ap_id_t mme_mbms_m3ap_id, sctp_assoc_id_t sctp_assoc_id);

//------------------------------------------------------------------------------
void
m3ap_handle_mbms_session_start_request (
  const itti_m3ap_mbms_session_start_req_t * const mbms_session_start_req_pP)
{
  /*
   * MBMS-GW triggers a new MBMS Service. No MCEs are specified but only the MBMS service area.
   * MCE_APP will not be MCE specific. MCE specific messages will be handled in the M3AP_APP.
   * That a single MCE fails to broadcast, is not of importance to the MME_APP.
   * This message initiates for all MCEs in the given MBMS Service are a new MBMS Bearer Service.
   */
  uint8_t                              *buffer_p = NULL;
  uint32_t                              length = 0;
  uint8_t								  							num_m3ap_mces = 0;
  mbms_description_t               		 *mbms_ref = NULL;
  mme_mbms_m3ap_id_t 					  				mme_mbms_m3ap_id 	= INVALID_MME_MBMS_M3AP_ID;
  OAILOG_FUNC_IN (LOG_M3AP);
  DevAssert (mbms_session_start_req_pP != NULL);

  /*
   * We need to check the MBMS Service via TMGI and MBMS Service Index.
   * Currently, only their combination must be unique and only 1 SA can be activated at a time.
   *
   */
  mbms_ref = m3ap_is_mbms_tmgi_in_list(&mbms_session_start_req_pP->tmgi, mbms_session_start_req_pP->mbms_service_area_id); /**< Nothing MCE specific. */
  if (mbms_ref) {
    OAILOG_ERROR (LOG_M3AP, "An MBMS Service Description with for TMGI " TMGI_FMT " and MBMS_Service_Area ID " MBMS_SERVICE_AREA_ID_FMT "already exists. Removing implicitly. \n",
        TMGI_ARG(&mbms_session_start_req_pP->tmgi), mbms_session_start_req_pP->mbms_service_area_id);
    /**
     * Trigger a session stop and inform the MCEs.
     * MBMS Services should only be removed over this function.
     * We will update the MCE statistics also.
     */
    m3ap_handle_mbms_session_stop_request(&mbms_ref->tmgi, mbms_ref->mbms_service_area_id, true);
  }
  /** Check that there exists at least a single MCE with the MBMS Service Area (we don't start MBMS sessions for MCEs which later on connected). */
  mce_config_read_lock (&mce_config);
  m3ap_mce_description_t *			         m3ap_mce_p_elements[mce_config.max_m3_mces];
  memset(m3ap_mce_p_elements, 0, (sizeof(m3ap_mce_description_t*) * mce_config.max_m3_mces));
  mce_config_unlock (&mce_config);
  m3ap_is_mbms_sai_in_list(mbms_session_start_req_pP->mbms_service_area_id, &num_m3ap_mces, (m3ap_mce_description_t **)m3ap_mce_p_elements);
  if(!num_m3ap_mces){
    OAILOG_ERROR (LOG_M3AP, "No M3AP MCEs could be found for the MBMS SA " MBMS_SERVICE_AREA_ID_FMT" for the MBMS Service with TMGI " TMGI_FMT". \n",
    	mbms_session_start_req_pP->mbms_service_area_id, TMGI_ARG(&mbms_session_start_req_pP->tmgi));
    OAILOG_FUNC_OUT (LOG_M3AP);
  }

  /**
   * We don't care to inform the MME_APP layer.
   * Create a new MBMS Service Description.
   * Allocate an MME M3AP MBMS ID (16) inside it. Will be used for all MCE associations.
   */
  if((mbms_ref = m3ap_new_mbms (&mbms_session_start_req_pP->tmgi, mbms_session_start_req_pP->mbms_service_area_id)) == NULL) {
    // If we failed to allocate a new MBMS Service Description return -1
    OAILOG_ERROR (LOG_M3AP, "M3AP:MBMS Session Start Request- Failed to allocate M3AP Service Description for TMGI " TMGI_FMT " and MBMS Service Area Id "MBMS_SERVICE_AREA_ID_FMT". \n",
    	TMGI_ARG(&mbms_session_start_req_pP->tmgi), mbms_session_start_req_pP->mbms_service_area_id);
    OAILOG_FUNC_OUT (LOG_M3AP);
  }
  /**
   * Update the created MBMS Service Description.
   * Directly set the values and don't wait for a response, we will set these values into the MCE, once the timer runs out.
   * We don't need the MBMS Session Duration. It will be handled in the MME_APP layer.
   */
  memcpy((void*)&mbms_ref->mbms_bc.mbms_ip_mc_distribution,  (void*)&mbms_session_start_req_pP->mbms_bearer_tbc.mbms_ip_mc_dist, sizeof(mbms_ip_multicast_distribution_t));
  memcpy((void*)&mbms_ref->mbms_bc.eps_bearer_context.bearer_level_qos, (void*)&mbms_session_start_req_pP->mbms_bearer_tbc.bc_tbc.bearer_level_qos, sizeof(bearer_qos_t));
  /**
   * Check if a timer has been given, if so start the timer.
   * If not send immediately. MBMS Session start will be based on the absolute time and not on the MCCH modification/repetition periods.
   * They will be handled separately/independently.
   */
  if(mbms_session_start_req_pP->abs_start_time_sec){
  	uint32_t delta_to_start_sec = mbms_session_start_req_pP->abs_start_time_sec - time(NULL);
  	OAILOG_INFO (LOG_M3AP, "M3AP:MBMS Session Start Request- Received a timer of (%d)s for start of TMGI " TMGI_FMT " and MBMS Service Area ID "MBMS_SERVICE_AREA_ID_FMT". \n",
  			delta_to_start_sec, TMGI_ARG(&mbms_session_start_req_pP->tmgi), mbms_session_start_req_pP->mbms_service_area_id);
  	// todo: calculate delta for usec
  	if (timer_setup (delta_to_start_sec, mbms_session_start_req_pP->abs_start_time_usec,
  			TASK_M3AP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void*)mme_mbms_m3ap_id, &(mbms_ref->mxap_action_timer.id)) < 0) {
      OAILOG_ERROR (LOG_M3AP, "Failed to start MBMS Session Start timer for MBMS with MBMS M3AP MME ID " MME_MBMS_M3AP_ID_FMT". \n", mme_mbms_m3ap_id);
      mbms_ref->mxap_action_timer.id = MxAP_TIMER_INACTIVE_ID;
      /** Send immediately. */
    } else {
      OAILOG_DEBUG (LOG_M3AP, "Started M3AP MBMS Session start timer (timer id 0x%lx) for MBMS Session MBMS M3AP MME ID " MME_MBMS_M3AP_ID_FMT". "
    		  "Waiting for timeout to trigger M3AP Session Start to M3AP MMEs.\n", mbms_ref->mxap_action_timer.id, mme_mbms_m3ap_id);
      /** Leave the method. */
      OAILOG_FUNC_OUT(LOG_M3AP);
    }
  }
  /**
   * Try to send the MBMS Session Start requests.
   * If it cannot be sent, we still keep the MBMS Service Description in the map.
   * The SCTP associations will only be removed with a successful response.
   */
  m3ap_generate_mbms_session_start_request(mme_mbms_m3ap_id, num_m3ap_mces, &m3ap_mce_p_elements);
  OAILOG_FUNC_OUT(LOG_M3AP);
}

//------------------------------------------------------------------------------
void
m3ap_handle_mbms_session_update_request (
  const itti_m3ap_mbms_session_update_req_t * const mbms_session_update_req_pP)
{
  /*
   * MBMS-GW triggers the stop of an MBMS Service on all the MCEs which are receiving it.
   */
  uint8_t                *buffer_p 					 = NULL;
  uint32_t                length 					 = 0;
  mbms_description_t     *mbms_ref 					 = NULL;
  M3AP_M3AP_PDU_t         pdu 						 = {0};
  int 									  num_m3ap_mces_new_mbms_sai = 0;

  OAILOG_FUNC_IN (LOG_M3AP);
  DevAssert (mbms_session_update_req_pP != NULL);
  /*
   * We need to check the MBMS Service via TMGI and MBMS Service Index.
   */
  mbms_ref = m3ap_is_mbms_tmgi_in_list(&mbms_session_update_req_pP->tmgi, mbms_session_update_req_pP->old_mbms_service_area_id); /**< Nothing MCE specific. */
  if (!mbms_ref) {
    /** No MBMS Reference found, just ignore the message and return. */
  	OAILOG_ERROR (LOG_M3AP, "No MBMS Service Description with for TMGI " TMGI_FMT " and MBMS_Service_Area ID " MBMS_SERVICE_AREA_ID_FMT " already exists. \n",
  			TMGI_ARG(&mbms_session_update_req_pP->tmgi), mbms_session_update_req_pP->old_mbms_service_area_id);
  	OAILOG_FUNC_OUT(LOG_M3AP);
  }

  /**
   * Cleared the MBMS service from MCEs with unmatching MBMS Service Area Id.
   * We can continue with the update of the MBMS service.
   */
  memcpy((void*)&mbms_ref->mbms_bc.mbms_ip_mc_distribution,  (void*)&mbms_session_update_req_pP->mbms_bearer_tbc.mbms_ip_mc_dist, sizeof(mbms_ip_multicast_distribution_t));
  memcpy((void*)&mbms_ref->mbms_bc.eps_bearer_context.bearer_level_qos, (void*)&mbms_session_update_req_pP->mbms_bearer_tbc.bc_tbc.bearer_level_qos, sizeof(bearer_qos_t));

  /**
   * Before the timer, check if the new MBMS Service Area Id is served by any MCE.
   * If not, directly remove the MBMS description.
   */
  mce_config_read_lock (&mce_config);
  m3ap_mce_description_t *			         m3ap_mce_p_elements[mce_config.max_m3_mces];
  memset(m3ap_mce_p_elements, 0, (sizeof(m3ap_mce_description_t*) * mce_config.max_m3_mces));
  mce_config_unlock (&mce_config);
  m3ap_is_mbms_sai_in_list(mbms_session_update_req_pP->new_mbms_service_area_id, &num_m3ap_mces_new_mbms_sai,
		  (m3ap_mce_description_t **)m3ap_mce_p_elements);
  if(!num_m3ap_mces_new_mbms_sai){
    OAILOG_ERROR (LOG_M3AP, "No M3AP MCEs could be found for the MBMS SAI " MBMS_SERVICE_AREA_ID_FMT" for the MBMS Service with TMGI " TMGI_FMT". "
    		"Stopping the MBMS service. \n", mbms_session_update_req_pP->new_mbms_service_area_id, TMGI_ARG(&mbms_session_update_req_pP->tmgi));
    m3ap_handle_mbms_session_stop_request(&mbms_ref->tmgi, mbms_ref->mbms_service_area_id, true); /**< Should also remove the MBMS service. */
    OAILOG_FUNC_OUT (LOG_M3AP);
  }

  /**
   * Iterate through the current MBMS services. Check an current M3 MCEs, where the new MBMS service area id is NOT supported.
   * Send a session stop to all of them without removing the MBMS service.
   */
  mce_config_read_lock (&mce_config);
  memset(m3ap_mce_p_elements, 0, (sizeof(m3ap_mce_description_t*) * mce_config.max_m3_mces));
  mce_config_unlock (&mce_config);
  int num_m3ap_mces_missing_new_mbms_sai = 0;
  m3ap_is_mbms_sai_not_in_list(mbms_session_update_req_pP->new_mbms_service_area_id, &num_m3ap_mces_missing_new_mbms_sai, (m3ap_mce_description_t **)&m3ap_mce_p_elements);
  if(num_m3ap_mces_missing_new_mbms_sai){
	  OAILOG_ERROR (LOG_M3AP, "(%d) M3AP MCEs not supporting new MBMS SAI " MBMS_SERVICE_AREA_ID_FMT" for the MBMS Service with TMGI " TMGI_FMT". "
		"Stopping the MBMS session in the M3AP MCEs. \n", mbms_session_update_req_pP->new_mbms_service_area_id, TMGI_ARG(&mbms_session_update_req_pP->tmgi));
     /** Send an MBMS session stop and remove the association. */
     for(int i = 0; i < num_m3ap_mces_missing_new_mbms_sai; i++){
       m3ap_generate_mbms_session_stop_request(mbms_ref->mme_mbms_m3ap_id, m3ap_mce_p_elements[i]->sctp_assoc_id);
       /** Remove the association and decrement the count. */
       m3ap_mce_p_elements[i]->nb_mbms_associated--; /**< We don't check for restart, since it is trigger due update. */
       hashtable_uint64_ts_free(&mbms_ref->g_m3ap_assoc_id2mme_mce_id_coll, m3ap_mce_p_elements[i]->sctp_assoc_id);
     }
  } else {
	  OAILOG_INFO(LOG_M3AP, "All existing MCEs of the MBMS Service " MME_MBMS_M3AP_ID_FMT " support the new MBMS SA " MBMS_SERVICE_AREA_ID_FMT". \n",
    	mbms_ref->mme_mbms_m3ap_id, mbms_session_update_req_pP->new_mbms_service_area_id);
  }
  /** Done cleaning up old MBMS Service Area Id artifacts. Continue with the new MBMS Service Area Id. */
  mbms_ref->mbms_service_area_id = mbms_session_update_req_pP->new_mbms_service_area_id;

  /**
   * Check if there is a timer set.
   * If so postpone the update.
   */
  if(mbms_session_update_req_pP->abs_update_time_sec){
    uint32_t delta_to_update_sec = mbms_session_update_req_pP->abs_update_time_sec- time(NULL);
    OAILOG_INFO (LOG_M3AP, "M3AP:MBMS Session Update Request- Received a timer of (%d)s for start of TMGI " TMGI_FMT " and MBMS Service Area ID "MBMS_SERVICE_AREA_ID_FMT". \n",
    	delta_to_update_sec, TMGI_ARG(&mbms_session_update_req_pP->tmgi), mbms_session_update_req_pP->new_mbms_service_area_id);
    if (timer_setup (delta_to_update_sec, mbms_session_update_req_pP->abs_update_time_usec,
           TASK_M3AP, INSTANCE_DEFAULT, TIMER_ONE_SHOT, (void*)mbms_ref->mme_mbms_m3ap_id, &(mbms_ref->mxap_action_timer.id)) < 0) {
         OAILOG_ERROR (LOG_M3AP, "Failed to start MBMS Session Update timer for MBMS with MBMS M3AP MME ID " MME_MBMS_M3AP_ID_FMT". \n", mbms_ref->mme_mbms_m3ap_id);
         mbms_ref->mxap_action_timer.id = MxAP_TIMER_INACTIVE_ID;
         /** Send immediately. */
       } else {
         OAILOG_DEBUG (LOG_M3AP, "Started M3AP MBMS Session Update timer (timer id 0x%lx) for MBMS Session MME MBMS M3AP ID " MME_MBMS_M3AP_ID_FMT". "
        		 "Waiting for timeout to trigger M3AP Session Update to M3AP MCEs.\n", mbms_ref->mxap_action_timer.id, mbms_ref->mme_mbms_m3ap_id);
         /** Leave the method. */
         OAILOG_FUNC_OUT(LOG_M3AP);
       }
  }
  /** Update the MBMS Service Context. */
  m3ap_update_mbms_service_context(mbms_ref->mme_mbms_m3ap_id);
  OAILOG_FUNC_OUT(LOG_M3AP);
}

//------------------------------------------------------------------------------
void
m3ap_handle_mbms_session_stop_request (
  const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_service_area_id, const bool inform_mces)
{
  OAILOG_FUNC_IN (LOG_M3AP);

  /*
   * MBMS-GW triggers the stop of an MBMS Service on all the MCEs which are receiving it.
   */
  uint8_t                                *buffer_p = NULL;
  uint32_t                                length = 0;
  mbms_description_t               		 	 *mbms_ref = NULL;
  m3ap_mce_description_t                 *target_mce_ref = NULL;
  M3AP_M3AP_PDU_t                         pdu = {0};

  /*
   * We need to check the MBMS Service via TMGI and MBMS Service Index.
   */
  mbms_ref = m3ap_is_mbms_tmgi_in_list(tmgi, mbms_service_area_id); /**< Nothing MCE specific. */
  if (!mbms_ref) {
    /**
     * No MBMS Reference found, just ignore the message and return.
     */
  	OAILOG_ERROR (LOG_M3AP, "No MBMS Service Description with for TMGI " TMGI_FMT " and MBMS_Service_Area ID " MBMS_SERVICE_AREA_ID_FMT "already exists. \n",
  			TMGI_ARG(tmgi), mbms_service_area_id);
  	OAILOG_FUNC_OUT(LOG_M3AP);
  }

  if(inform_mces) {
    /** Check that there exists at least a single MCE with the MBMS Service Area (we don't start MBMS sessions for MCEs which later on connected). */
  	mce_config_read_lock (&mce_config);
  	m3ap_mce_description_t *			         m3ap_mce_p_elements[mce_config.max_m3_mces];
  	memset(m3ap_mce_p_elements, 0, (sizeof(m3ap_mce_description_t*) * mce_config.max_m3_mces));
  	mce_config_unlock (&mce_config);
  	int num_m3ap_mces = 0;
  	m3ap_is_mbms_sai_in_list(mbms_service_area_id, &num_m3ap_mces, (m3ap_mce_description_t **)m3ap_mce_p_elements);
  	if(num_m3ap_mces){
  		for(int i = 0; i < num_m3ap_mces; i++){
  			m3ap_generate_mbms_session_stop_request(mbms_ref->mme_mbms_m3ap_id, m3ap_mce_p_elements[i]->sctp_assoc_id);
  		}
  	} else {
  		OAILOG_ERROR(LOG_M3AP, "No M3AP MCEs could be found for the MBMS SA " MBMS_SERVICE_AREA_ID_FMT" for the MBMS Service with TMGI " TMGI_FMT" to be removed. Removing implicitly. \n",
  				mbms_service_area_id, TMGI_ARG(tmgi));
  	}
  }

  /**
   * Remove the MBMS Service.
   * Should also remove all the sctp associations from the MCEs and decrement the MCE MBMS numbers.
   */
  hashtable_ts_free (&g_m3ap_mbms_coll, mbms_ref->mme_mbms_m3ap_id);
  OAILOG_FUNC_OUT(LOG_M3AP);
}

//------------------------------------------------------------------------------
int m3ap_generate_mbms_session_stop_request(mme_mbms_m3ap_id_t mme_mbms_m3ap_id, sctp_assoc_id_t sctp_assoc_id)
{
  OAILOG_FUNC_IN (LOG_M3AP);
  mbms_description_t             *mbms_ref = NULL;
  m3ap_mce_description_t				 *m3ap_mce_description = NULL;
  uint8_t                        *buffer_p = NULL;
  uint32_t                        length = 0;
  mce_mbms_m3ap_id_t					  	mce_mbms_m3ap_id = INVALID_MCE_MBMS_M3AP_ID;
  MessagesIds                     message_id = MESSAGES_ID_MAX;
  void                           *id = NULL;
  M3AP_M3AP_PDU_t                 pdu = {0};
  M3AP_MBMSSessionStopRequest_t			 *out;
  M3AP_MBMSSessionStopRequest_IEs_t	 *ie = NULL;

  mbms_ref = m3ap_is_mbms_mme_m3ap_id_in_list(mme_mbms_m3ap_id);
  if (!mbms_ref) {
  	OAILOG_ERROR (LOG_M3AP, "No MME MBMS M3AP ID " MME_MBMS_M3AP_ID_FMT". Cannot generate MBMS Session Stop Request. \n", mme_mbms_m3ap_id);
    OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }

  m3ap_mce_description = m3ap_is_mce_assoc_id_in_list(sctp_assoc_id);
  if (!m3ap_mce_description) {
  	OAILOG_ERROR (LOG_M3AP, "No M3AP MCE description for SCTP Assoc Id (%d). Cannot trigger MBMS Session Stop Request for MBMS Service with MME MBMS M3AP ID " MME_MBMS_M3AP_ID_FMT". \n",
			sctp_assoc_id, mme_mbms_m3ap_id);
    OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }

  /** Check that an MCE-MBMS-ID exists. */
  hashtable_uint64_ts_get (&mbms_ref->g_m3ap_assoc_id2mme_mce_id_coll, (const hash_key_t)sctp_assoc_id, (void *)&mce_mbms_m3ap_id);
  if(mce_mbms_m3ap_id == INVALID_MCE_MBMS_M3AP_ID){
  	OAILOG_ERROR (LOG_M3AP, "No MCE MBMS M3AP ID could be retrieved. Cannot generate MBMS Session Stop Request. \n", mce_mbms_m3ap_id);
  	OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }

  /*
   * We have found the UE in the list.
   * Create new IE list message and encode it.
   */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M3AP_M3AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = M3AP_ProcedureCode_id_mBMSsessionStop;
  pdu.choice.initiatingMessage.criticality = M3AP_Criticality_ignore;
  pdu.choice.initiatingMessage.value.present = M3AP_InitiatingMessage__value_PR_MBMSSessionStopRequest;
  out = &pdu.choice.initiatingMessage.value.choice.MBMSSessionStopRequest;

  /*
   * Setting MBMS informations with the ones found in ue_ref
   */
  /* mandatory */
  ie = (M3AP_MBMSSessionStopRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStopRequest_IEs_t));
  ie->id = M3AP_ProtocolIE_ID_id_MME_MBMS_M3AP_ID;
  ie->criticality = M3AP_Criticality_reject;
  ie->value.present = M3AP_MBMSSessionStopRequest_IEs__value_PR_MME_MBMS_M3AP_ID;
  ie->value.choice.MME_MBMS_M3AP_ID = mme_mbms_m3ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (M3AP_MBMSSessionStopRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStopRequest_IEs_t));
  ie->id = M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID;
  ie->criticality = M3AP_Criticality_reject;
  ie->value.present = M3AP_MBMSSessionStopRequest_IEs__value_PR_MCE_MBMS_M3AP_ID;
  ie->value.choice.MCE_MBMS_M3AP_ID = mce_mbms_m3ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // todo: qos, sctpm, scheduling..
  if (m3ap_mme_encode_pdu (&pdu, &buffer_p, &length) < 0) {
  	// TODO: handle something
  	OAILOG_ERROR (LOG_M3AP, "Failed to encode MBMS Session Stop Request. \n");
  	OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }

  bstring b = blk2bstr(buffer_p, length);
  free(buffer_p);
  OAILOG_NOTICE (LOG_M3AP, "Send M3AP_MBMS_SESSION_STOP_REQUEST message MME_MBMS_M3AP_ID = " MME_MBMS_M3AP_ID_FMT "\n", mme_mbms_m3ap_id);
  m3ap_mme_itti_send_sctp_request(&b, m3ap_mce_description->sctp_assoc_id, MBMS_SERVICE_SCTP_STREAM_ID, mme_mbms_m3ap_id);
  OAILOG_FUNC_RETURN (LOG_M3AP, RETURNok);
}

//------------------------------------------------------------------------------
void
m3ap_mme_handle_mbms_action_timer_expiry (void *arg)
{
  MessageDef                             *message_p = NULL;
  OAILOG_FUNC_IN (LOG_M3AP);
  DevAssert (arg != NULL);

  mbms_description_t *mbms_ref_p  =        (mbms_description_t *)arg;

  mbms_ref_p->mxap_action_timer.id = MxAP_TIMER_INACTIVE_ID;
  OAILOG_DEBUG (LOG_M3AP, "Expired- MBMS Action timer MCE MBMS M3AP " MME_MBMS_M3AP_ID_FMT" . \n", mbms_ref_p->mme_mbms_m3ap_id);

  /**
   * If there are no associated MCEs, we need to start the MBMS service.
   * If there are some associated MCEs, we need to update the MBMS service.
   * No timer for MBMS Service stop.
   */
  if(!mbms_ref_p->g_m3ap_assoc_id2mme_mce_id_coll.num_elements) {
    OAILOG_DEBUG (LOG_M3AP, "Starting MBMS service with MME MBMS M3AP " MME_MBMS_M3AP_ID_FMT" . \n", mbms_ref_p->mme_mbms_m3ap_id);
    uint8_t								  num_m3ap_mces = 0;
    /** Get the list of MCEs of the matching service area. */
    mce_config_read_lock (&mce_config);
    m3ap_mce_description_t *			         m3ap_mce_p_elements[mce_config.max_m3_mces];
    memset(m3ap_mce_p_elements, 0, (sizeof(m3ap_mce_description_t*) * mce_config.max_m3_mces));
    mce_config_unlock (&mce_config);

    m3ap_is_mbms_sai_in_list(mbms_ref_p->mbms_service_area_id, &num_m3ap_mces, (m3ap_mce_description_t **)m3ap_mce_p_elements);
    if(!num_m3ap_mces){
      OAILOG_ERROR (LOG_M3AP, "No M3AP MCEs could be found for the MBMS SA " MBMS_SERVICE_AREA_ID_FMT" for the MBMS Service with TMGI " TMGI_FMT" after timeout. Removing implicitly. \n",
    		  mbms_ref_p->mbms_service_area_id, TMGI_ARG(&mbms_ref_p->tmgi));
      hashtable_ts_free (&g_m3ap_mbms_coll, mbms_ref_p->mme_mbms_m3ap_id);
      OAILOG_FUNC_OUT (LOG_M3AP);
    }
    m3ap_generate_mbms_session_start_request(mbms_ref_p->mme_mbms_m3ap_id, num_m3ap_mces, m3ap_mce_p_elements);
  } else {
    OAILOG_DEBUG (LOG_M3AP, "MBMS service with MME MBMS M3AP " MME_MBMS_M3AP_ID_FMT" has already some MCEs, updating it. \n", mbms_ref_p->mme_mbms_m3ap_id);
    m3ap_update_mbms_service_context(mbms_ref_p->mme_mbms_m3ap_id);
  }
  OAILOG_FUNC_OUT (LOG_M3AP);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

//------------------------------------------------------------------------------
static
void m3ap_update_mbms_service_context(const mme_mbms_m3ap_id_t mme_mbms_m3ap_id) {
  mbms_description_t               		 *mbms_ref 					 = NULL;
  int 							  								  num_m3ap_mce_new_mbms_sai = 0;

  OAILOG_FUNC_IN (LOG_M3AP);

  /** Check that there exists at least a single MCE with the MBMS Service Area (we don't start MBMS sessions for MCEs which later on connected). */
  mce_config_read_lock (&mce_config);
  m3ap_mce_description_t *			         new_mbms_sai_m3ap_me_p_elements[mce_config.max_m3_mces];
  memset(new_mbms_sai_m3ap_me_p_elements, 0, (sizeof(m3ap_mce_description_t*) * mce_config.max_m3_mces));
  mce_config_unlock (&mce_config);

  mbms_ref = m3ap_is_mbms_mme_m3ap_id_in_list(mme_mbms_m3ap_id); /**< Nothing MCE specific. */
  if (!mbms_ref) {
    /** No MBMS Reference found, just ignore the message and return. */
	OAILOG_ERROR (LOG_M3AP, "No MBMS Service Description with for MME MBMS M3AP ID " MME_MBMS_M3AP_ID_FMT " existing. Cannot update. \n", mme_mbms_m3ap_id);
	OAILOG_FUNC_OUT(LOG_M3AP);
  }

  /**
   * Get the list of all MCEs.
   * If the MCE is not in the current list, send an update, else send a session start.
   * We should have removed (stopped the MBMS Session) in the MCEs, which don't support the MBMS Service Area ID before.
   */
  mce_config_read_lock (&mce_config);
  m3ap_mce_description_t *			         m3ap_me_p_elements[mce_config.max_m3_mces];
  memset(m3ap_me_p_elements, 0, (sizeof(m3ap_mce_description_t*) * mce_config.max_m3_mces));
  mce_config_unlock (&mce_config);
  m3ap_is_mbms_sai_in_list(mbms_ref->mbms_service_area_id, &num_m3ap_mce_new_mbms_sai, (m3ap_mce_description_t **)m3ap_me_p_elements);
  if(!num_m3ap_mce_new_mbms_sai){
	OAILOG_ERROR (LOG_M3AP, "No M3AP MCEs could be found for the MBMS SAI " MBMS_SERVICE_AREA_ID_FMT" for the MBMS Service with TMGI " TMGI_FMT". "
		"Stopping the MBMS service. \n", mbms_ref->mbms_service_area_id, TMGI_ARG(&mbms_ref->tmgi));
	/** Change of the MBMS SAI before, does not affect this. */
	m3ap_handle_mbms_session_stop_request(&mbms_ref->tmgi, mbms_ref->mbms_service_area_id, true); /**< Should also remove the MBMS service. */
	OAILOG_FUNC_OUT (LOG_M3AP);
  }

  /** Update all MCEs, which are already in the MBMS Service. */
  for(int i = 0; i < num_m3ap_mce_new_mbms_sai; i++) {
    /** Get the MCE from the SCTP association. */
	m3ap_mce_description_t * m3ap_mce_ref = m3ap_me_p_elements[i];
	hashtable_rc_t h_rc = hashtable_uint64_ts_is_key_exists(&mbms_ref->g_m3ap_assoc_id2mme_mce_id_coll, m3ap_mce_ref->sctp_assoc_id);
	if(HASH_TABLE_OK == h_rc) {
	  /** MCE is already in the list, update it. */
	  int rc = m3ap_generate_mbms_session_update_request(mbms_ref->mme_mbms_m3ap_id, m3ap_mce_ref->sctp_assoc_id);
	  if(rc != RETURNok) {
	    OAILOG_ERROR(LOG_M3AP, "Error updating M3AP MCE with SCTP Assoc ID (%d) for the updated MBMS Service with TMGI " TMGI_FMT". "
	    "Removing the association.\n", m3ap_mce_ref->sctp_assoc_id, TMGI_ARG(&mbms_ref->tmgi));
	    m3ap_generate_mbms_session_stop_request(mbms_ref->mme_mbms_m3ap_id, m3ap_mce_ref->sctp_assoc_id);
	    /** Remove the hash key, which should also update the MCE. */
	    hashtable_uint64_ts_free(&mbms_ref->g_m3ap_assoc_id2mme_mce_id_coll, (const hash_key_t)m3ap_mce_ref->sctp_assoc_id);
	    if(m3ap_mce_ref->nb_mbms_associated)
	      m3ap_mce_ref->nb_mbms_associated--;
	    OAILOG_ERROR(LOG_M3AP, "Removed association after erroneous update.\n");
	  }
	  OAILOG_INFO(LOG_M3AP, "Successfully updated MCE with SCTP-Assoc (%d) for MBMS Service " MME_MBMS_M3AP_ID_FMT " with updated MBMS SAI (%d).\n",
			  m3ap_mce_ref->sctp_assoc_id, mbms_ref->mme_mbms_m3ap_id, mbms_ref->mbms_service_area_id);
	  /** continue. */
	} else {
		int rc = m3ap_generate_mbms_session_start_request(mbms_ref->mme_mbms_m3ap_id, 1, &m3ap_mce_ref); // todo: test if this works..
		/** continue. */
		if(rc == RETURNok) {
		  OAILOG_INFO(LOG_M3AP, "Successfully adding new MCE with SCTP-Assoc (%d) for MBMS Service " MME_MBMS_M3AP_ID_FMT " with updated MBMS SAI (%d).\n",
				  m3ap_mce_ref->sctp_assoc_id, mbms_ref->mme_mbms_m3ap_id, mbms_ref->mbms_service_area_id);
		} else {
		  OAILOG_ERROR(LOG_M3AP, "Error adding new NB with SCTP-Assoc (%d) for MBMS Service " MME_MBMS_M3AP_ID_FMT " with updated MBMS SAI (%d).\n",
				  m3ap_mce_ref->sctp_assoc_id, mbms_ref->mme_mbms_m3ap_id, mbms_ref->mbms_service_area_id);
		}
	}
  }
  OAILOG_FUNC_OUT(LOG_M3AP);
}

//------------------------------------------------------------------------------
static
int m3ap_generate_mbms_session_start_request(mme_mbms_m3ap_id_t mme_mbms_m3ap_id, const uint8_t num_m3ap_mce, m3ap_mce_description_t ** m3ap_mce_descriptions)
{
  OAILOG_FUNC_IN (LOG_M3AP);
  mbms_description_t                     *mbms_ref = NULL;
  uint8_t                                *buffer_p = NULL;
  uint32_t                                length = 0;
  MessagesIds                             message_id = MESSAGES_ID_MAX;
  void                                   *id = NULL;
  M3AP_M3AP_PDU_t                         pdu = {0};
  M3AP_MBMSSessionStartRequest_t			 			 *out;
  M3AP_MBMSSessionStartRequest_IEs_t		  	 *ie = NULL;

  mbms_ref = m3ap_is_mbms_mme_m3ap_id_in_list(mme_mbms_m3ap_id);
  if (!mbms_ref) {
  	OAILOG_ERROR (LOG_M3AP, "No MBMS MME M3AP ID " MME_MBMS_M3AP_ID_FMT". Cannot generate MBMS Session Start Request. \n", mme_mbms_m3ap_id);
    OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }

  /**
   * Get all M3AP MCEs which match the MBMS Service Area ID and transmit to all of them.
   * We don't encode MCE MBMS M3AP ID, so we can reuse the encoded message.
   */
  if(!num_m3ap_mce || !*m3ap_mce_descriptions){
  	OAILOG_ERROR (LOG_M3AP, "No M3AP MCEs given for the received MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT " for MBMS Service " MME_MBMS_M3AP_ID_FMT". \n",
  			mbms_ref->mbms_service_area_id, mme_mbms_m3ap_id);
  	OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }

  /*
   * We have found the UE in the list.
   * Create new IE list message and encode it.
   */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M3AP_M3AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = M3AP_ProcedureCode_id_mBMSsessionStart;
  pdu.choice.initiatingMessage.criticality = M3AP_Criticality_ignore;
  pdu.choice.initiatingMessage.value.present = M3AP_InitiatingMessage__value_PR_MBMSSessionStartRequest;
  out = &pdu.choice.initiatingMessage.value.choice.MBMSSessionStartRequest;

  /*
   * mandatory
   * Setting MBMS informations with the ones found in ue_ref
   */
  ie = (M3AP_MBMSSessionStartRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStartRequest_IEs_t));
  ie->id = M3AP_ProtocolIE_ID_id_MME_MBMS_M3AP_ID;
  ie->criticality = M3AP_Criticality_reject;
  ie->value.present = M3AP_MBMSSessionStartRequest_IEs__value_PR_MME_MBMS_M3AP_ID;
  ie->value.choice.MME_MBMS_M3AP_ID = mme_mbms_m3ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (M3AP_MBMSSessionStartRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStartRequest_IEs_t));
  ie->id = M3AP_ProtocolIE_ID_id_TMGI;
  ie->criticality = M3AP_Criticality_reject;
  ie->value.present = M3AP_MBMSSessionStartRequest_IEs__value_PR_TMGI;
  INT24_TO_OCTET_STRING(mbms_ref->tmgi.mbms_service_id, &ie->value.choice.TMGI.serviceID);
  TBCD_TO_PLMN_T(&ie->value.choice.TMGI.pLMNidentity, &mbms_ref->tmgi.plmn);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /** No MBMS Session Id since we only support GCS-AS (public safety). */

  /* mandatory
   * Only a single MBMS Service Area Id per MBMS Service is supported right now.
   */
  ie = (M3AP_MBMSSessionStartRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStartRequest_IEs_t));
  ie->id = M3AP_ProtocolIE_ID_id_TMGI;
  ie->criticality = M3AP_Criticality_reject;
  ie->value.present = M3AP_MBMSSessionStartRequest_IEs__value_PR_MBMS_Service_Area;
  uint32_t mbms_sai = mbms_ref->mbms_service_area_id | (0x01 << 16); /**< Add the length into the encoded value. */
  INT24_TO_OCTET_STRING(mbms_sai, &ie->value.choice.MBMS_Service_Area);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory
   * Add the Downlink Tunnel Information
   */
  ie = (M3AP_MBMSSessionStartRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionStartRequest_IEs_t));
  ie->id = M3AP_ProtocolIE_ID_id_TNL_Information;
  ie->criticality = M3AP_Criticality_reject;
  ie->value.present = M3AP_MBMSSessionStartRequest_IEs__value_PR_TNL_Information;
  INT32_TO_OCTET_STRING (mbms_ref->mbms_bc.mbms_ip_mc_distribution.cteid, &ie->value.choice.TNL_Information.gTP_DLTEID);
  /** Distribution Address. */
  if(mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.pdn_type == IPv4 ) {
    ie->value.choice.TNL_Information.iPMCAddress.buf = calloc(4, sizeof(uint8_t));
    IN_ADDR_TO_BUFFER(mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.address.ipv4_address, ie->value.choice.TNL_Information.iPMCAddress.buf);
    ie->value.choice.TNL_Information.iPMCAddress.size = 4;
  } else  if(mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.pdn_type == IPv6 )  {
  	ie->value.choice.TNL_Information.iPMCAddress.buf = calloc(16, sizeof(uint8_t));
    IN6_ADDR_TO_BUFFER(mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.address.ipv6_address, ie->value.choice.TNL_Information.iPMCAddress.buf);
    ie->value.choice.TNL_Information.iPMCAddress.size = 16;
  } else {
  	DevMessage("INVALID PDN TYPE FOR IP MC DISTRIBUTION " + mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.pdn_type);
  }
  /** Source Address. */
  if(mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.pdn_type == IPv4 ) {
    ie->value.choice.TNL_Information.iPSourceAddress.buf = calloc(4, sizeof(uint8_t));
    IN_ADDR_TO_BUFFER(mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.address.ipv4_address, ie->value.choice.TNL_Information.iPSourceAddress.buf);
    ie->value.choice.TNL_Information.iPSourceAddress.size = 4;
  } else  if(mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.pdn_type == IPv6 )  {
    ie->value.choice.TNL_Information.iPSourceAddress.buf = calloc(16, sizeof(uint8_t));
    IN6_ADDR_TO_BUFFER(mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.address.ipv6_address, ie->value.choice.TNL_Information.iPSourceAddress.buf);
    ie->value.choice.TNL_Information.iPSourceAddress.size = 16;
  } else {
  	DevMessage("INVALID PDN TYPE FOR IP MC SOURCE " + mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.pdn_type);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (m3ap_mme_encode_pdu (&pdu, &buffer_p, &length) < 0) {
  	OAILOG_ERROR (LOG_M3AP, "Failed to encode MBMS Session Start Request. \n");
  	OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }

  // todo: the next_sctp_stream is the one without incrementation?
  for(int i = 0; i < num_m3ap_mce; i++){
	  m3ap_mce_description_t * target_mce_ref = m3ap_mce_descriptions[i];
	  if(target_mce_ref){
	  	/** Check if it is in the list of the MBMS Service. */
	  	bstring b = blk2bstr(buffer_p, length);
	  	free(buffer_p);
	  	OAILOG_NOTICE (LOG_M3AP, "Send M3AP_MBMS_SESSION_START_REQUEST message MME_MBMS_M3AP_ID = " MME_MBMS_M3AP_ID_FMT "\n", mme_mbms_m3ap_id);
	  	/** For the sake of complexity (we wan't to keep it simple, and the same ), we are using the same SCTP Stream Id for all MBMS Service Index. */
	  	m3ap_mme_itti_send_sctp_request(&b, target_mce_ref->sctp_assoc_id, MBMS_SERVICE_SCTP_STREAM_ID, mme_mbms_m3ap_id);
	  }
  }

  OAILOG_FUNC_RETURN (LOG_M3AP, RETURNok);
}

//------------------------------------------------------------------------------
static
int m3ap_generate_mbms_session_update_request(mme_mbms_m3ap_id_t mme_mbms_m3ap_id, sctp_assoc_id_t sctp_assoc_id)
{
  OAILOG_FUNC_IN (LOG_M3AP);
  mbms_description_t                     *mbms_ref = NULL;
  m3ap_mce_description_t			    			 *m3ap_mce_description = NULL;
  uint8_t                                *buffer_p = NULL;
  uint32_t                                length = 0;
  mce_mbms_m3ap_id_t					  			    mce_mbms_m3ap_id = INVALID_MCE_MBMS_M3AP_ID;
  MessagesIds                             message_id = MESSAGES_ID_MAX;
  void                                   *id = NULL;
  M3AP_M3AP_PDU_t                         pdu = {0};
  M3AP_MBMSSessionUpdateRequest_t		 	   *out;
  M3AP_MBMSSessionUpdateRequest_IEs_t		 *ie = NULL;

  mbms_ref = m3ap_is_mbms_mme_m3ap_id_in_list(mme_mbms_m3ap_id);
  if (!mbms_ref) {
  	OAILOG_ERROR (LOG_M3AP, "No MME MBMS M3AP ID " MME_MBMS_M3AP_ID_FMT". Cannot generate MBMS Session Update Request. \n", mme_mbms_m3ap_id);
    OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }

  m3ap_mce_description = m3ap_is_mce_assoc_id_in_list(sctp_assoc_id);
  if (!m3ap_mce_description) {
  	OAILOG_ERROR (LOG_M3AP, "No M3AP MCE description for SCTP Assoc Id (%d). Cannot trigger MBMS Session Update Request for MBMS Service with MME MBMS M3AP ID " MME_MBMS_M3AP_ID_FMT". \n", sctp_assoc_id, mme_mbms_m3ap_id);
    OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }

  /** Check that an MCE-MBMS-ID exists. */
  hashtable_uint64_ts_get (&mbms_ref->g_m3ap_assoc_id2mme_mce_id_coll, (const hash_key_t)sctp_assoc_id, (void *)&mce_mbms_m3ap_id);
  if(mce_mbms_m3ap_id == INVALID_MCE_MBMS_M3AP_ID){
  	OAILOG_ERROR (LOG_M3AP, "No MCE MBMS M3AP ID could be retrieved. Cannot generate MBMS Session Update Request. \n");
  	OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }
  /*
   * We have found the MBMS in the list.
   * Create new IE list message and encode it.
   */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M3AP_M3AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = M3AP_ProcedureCode_id_mBMSsessionUpdate;
  pdu.choice.initiatingMessage.criticality = M3AP_Criticality_ignore;
  pdu.choice.initiatingMessage.value.present = M3AP_InitiatingMessage__value_PR_MBMSSessionUpdateRequest;
  out = &pdu.choice.initiatingMessage.value.choice.MBMSSessionUpdateRequest;

  /*
   * Setting MBMS informations with the ones found in ue_ref
   */
  /* mandatory */
  ie = (M3AP_MBMSSessionUpdateRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateRequest_IEs_t));
  ie->id = M3AP_ProtocolIE_ID_id_MME_MBMS_M3AP_ID;
  ie->criticality = M3AP_Criticality_reject;
  ie->value.present = M3AP_MBMSSessionUpdateRequest_IEs__value_PR_MME_MBMS_M3AP_ID;
  ie->value.choice.MME_MBMS_M3AP_ID = mme_mbms_m3ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (M3AP_MBMSSessionUpdateRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateRequest_IEs_t));
  ie->id = M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID;
  ie->criticality = M3AP_Criticality_reject;
  ie->value.present = M3AP_MBMSSessionUpdateRequest_IEs__value_PR_MCE_MBMS_M3AP_ID;
  ie->value.choice.MCE_MBMS_M3AP_ID = mce_mbms_m3ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (M3AP_MBMSSessionUpdateRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateRequest_IEs_t));
  ie->id = M3AP_ProtocolIE_ID_id_TMGI;
  ie->criticality = M3AP_Criticality_reject;
  ie->value.present = M3AP_MBMSSessionUpdateRequest_IEs__value_PR_TMGI;
  INT24_TO_OCTET_STRING(mbms_ref->tmgi.mbms_service_id, &ie->value.choice.TMGI.serviceID);
  TBCD_TO_PLMN_T(&ie->value.choice.TMGI.pLMNidentity, &mbms_ref->tmgi.plmn);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /** No MBMS Session Id since we only support GCS-AS (public safety). */

  /* mandatory
   * Only a single MBMS Service Area Id per MBMS Service is supported right now.
   */
  ie = (M3AP_MBMSSessionUpdateRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateRequest_IEs_t));
  ie->id = M3AP_ProtocolIE_ID_id_TMGI;
  ie->criticality = M3AP_Criticality_reject;
  ie->value.present = M3AP_MBMSSessionUpdateRequest_IEs__value_PR_MBMS_Service_Area;
  uint32_t mbms_sai = mbms_ref->mbms_service_area_id | (1 << 16); /**< Add the length into the encoded value. */
  INT24_TO_OCTET_STRING(mbms_sai, &ie->value.choice.MBMS_Service_Area);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory
   * Add the Downlink Tunnel Information
   */
  ie = (M3AP_MBMSSessionUpdateRequest_IEs_t *)calloc(1, sizeof(M3AP_MBMSSessionUpdateRequest_IEs_t));
  ie->id = M3AP_ProtocolIE_ID_id_TNL_Information;
  ie->criticality = M3AP_Criticality_reject;
  ie->value.present = M3AP_MBMSSessionUpdateRequest_IEs__value_PR_TNL_Information;
  INT32_TO_OCTET_STRING (mbms_ref->mbms_bc.mbms_ip_mc_distribution.cteid, &ie->value.choice.TNL_Information.gTP_DLTEID);
  /** Distribution Address. */
  if(mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.pdn_type == IPv4 ) {
    ie->value.choice.TNL_Information.iPMCAddress.buf = calloc(4, sizeof(uint8_t));
	IN_ADDR_TO_BUFFER(mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.address.ipv4_address, ie->value.choice.TNL_Information.iPMCAddress.buf);
	ie->value.choice.TNL_Information.iPMCAddress.size = 4;
  } else  if(mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.pdn_type == IPv6 )  {
    ie->value.choice.TNL_Information.iPMCAddress.buf = calloc(16, sizeof(uint8_t));
    IN6_ADDR_TO_BUFFER(mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.address.ipv6_address, ie->value.choice.TNL_Information.iPMCAddress.buf);
    ie->value.choice.TNL_Information.iPMCAddress.size = 16;
  } else {
	DevMessage("INVALID PDN TYPE FOR IP MC DISTRIBUTION " + mbms_ref->mbms_bc.mbms_ip_mc_distribution.distribution_address.pdn_type);
  }
  /** Source Address. */
  if(mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.pdn_type == IPv4 ) {
    ie->value.choice.TNL_Information.iPSourceAddress.buf = calloc(4, sizeof(uint8_t));
    IN_ADDR_TO_BUFFER(mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.address.ipv4_address, ie->value.choice.TNL_Information.iPSourceAddress.buf);
    ie->value.choice.TNL_Information.iPSourceAddress.size = 4;
  } else  if(mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.pdn_type == IPv6 )  {
    ie->value.choice.TNL_Information.iPSourceAddress.buf = calloc(16, sizeof(uint8_t));
    IN6_ADDR_TO_BUFFER(mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.address.ipv6_address, ie->value.choice.TNL_Information.iPSourceAddress.buf);
    ie->value.choice.TNL_Information.iPSourceAddress.size = 16;
  } else {
  	DevMessage("INVALID PDN TYPE FOR IP MC SOURCE " + mbms_ref->mbms_bc.mbms_ip_mc_distribution.source_address.pdn_type);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  // todo: qos, sctpm, scheduling..
  if (m3ap_mme_encode_pdu (&pdu, &buffer_p, &length) < 0) {
  	// TODO: handle something
  	OAILOG_ERROR (LOG_M3AP, "Failed to encode MBMS Session Update Request. \n");
  	OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }

  bstring b = blk2bstr(buffer_p, length);
  free(buffer_p);
  OAILOG_NOTICE (LOG_M3AP, "Send M3AP_MBMS_SESSION_UPDATE_REQUEST message MME_MBMS_M3AP_ID = " MME_MBMS_M3AP_ID_FMT "\n", mme_mbms_m3ap_id);
  m3ap_mme_itti_send_sctp_request(&b, m3ap_mce_description->sctp_assoc_id, MBMS_SERVICE_SCTP_STREAM_ID, mme_mbms_m3ap_id);
  OAILOG_FUNC_RETURN (LOG_M3AP, RETURNok);
}
