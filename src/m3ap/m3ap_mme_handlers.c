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

/*! \file m3ap_mme_handlers.c
  \brief
  \author Dincer BEKEN
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/


#include "m3ap_mme_handlers.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "3gpp_requirements_36.444.h"
#include "bstrlib.h"

#include "hashtable.h"
#include "log.h"

#include "assertions.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "mce_config.h"
#include "mce_app_statistics.h"
#include "timer.h"
#include "dynamic_memory_check.h"
#include "m3ap_common.h"
#include "m3ap_mme.h"
#include "m3ap_mme_itti_messaging.h"
#include "m3ap_mme_procedures.h"
#include "m3ap_mme_encoder.h"
// #include "m3ap_mme_mbms_sa.h"

extern hash_table_ts_t 					g_m3ap_mce_coll; 	  // SCTP Association ID association to M3AP MCE Reference;
extern hash_table_ts_t 					g_m3ap_mbms_coll; 	// MME MBMS M3AP ID association to MBMS Reference;

static const char * const m3_mce_state_str [] = {"M3AP_INIT", "M3AP_RESETTING", "M3AP_READY", "M3AP_SHUTDOWN"};

static int m3ap_generate_m3_setup_response (m3ap_mce_description_t * m3ap_mce_association, mbsfn_areas_t * mbsfn_areas);

//static int                              m3ap_mme_generate_ue_context_release_command (
//    mbms_description_t * mbms_ref_p, mce_mbms_m3ap_id_t mce_mbms_m3ap_id, enum s1cause, m3ap_mce_description_t* mce_ref_p);

/* Handlers matrix. Only mme related procedures present here.
*/
m3ap_message_decoded_callback           m3ap_messages_callback[][3] = {
  {0, m3ap_mme_handle_mbms_session_start_response,				 /* sessionStart */
		  m3ap_mme_handle_mbms_session_start_failure},
  {0, m3ap_mme_handle_mbms_session_stop_response, 0},      /* sessionStop */
  {m3ap_mme_handle_error_indication, 0, 0},     				   /* errorIndication */
  {m3ap_mme_handle_reset_request, 0, 0},                   /* reset */
  {m3ap_mme_handle_m3_setup_request, 0, 0},     				   /* m3Setup */
  {0, 0, 0},                    								   /* mceConfigurationUpdate */
  {0, 0, 0},                   									   /* mmeConfigurationUpdate */
  {0, 0, 0},                    								   /* privateMessage */
  {0, m3ap_mme_handle_mbms_session_update_response,				   /* sessionUpdate */
		  m3ap_mme_handle_mbms_session_update_failure},
};

const char                             *m3ap_direction2String[] = {
  "",                           /* Nothing */
  "Originating message",        /* originating message */
  "Successful outcome",         /* successful outcome */
  "UnSuccessfull outcome",      /* unsuccessful outcome */
};

//------------------------------------------------------------------------------
int
m3ap_mme_handle_message (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    M3AP_M3AP_PDU_t *pdu)
{

  /* Checking procedure Code and direction of message */
  if (pdu->choice.initiatingMessage.procedureCode >= sizeof(m3ap_messages_callback) / (3 * sizeof(m3ap_message_decoded_callback))
      || (pdu->present > M3AP_M3AP_PDU_PR_unsuccessfulOutcome)) {
    OAILOG_DEBUG (LOG_M3AP, "[SCTP %d] Either procedureCode %ld or direction %d exceed expected\n",
               assoc_id, pdu->choice.initiatingMessage.procedureCode, pdu->present);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_M3AP_M3AP_PDU, pdu);
    return -1;
  }

  /* No handler present.
   * This can mean not implemented or no procedure for MCE (wrong direction).
   */
  if (m3ap_messages_callback[pdu->choice.initiatingMessage.procedureCode][pdu->present - 1] == NULL) {
    OAILOG_DEBUG (LOG_M3AP, "[SCTP %d] No handler for procedureCode %ld in %s\n",
               assoc_id, pdu->choice.initiatingMessage.procedureCode,
               m3ap_direction2String[pdu->present]);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_M3AP_M3AP_PDU, pdu);
    return -1;
  }

  /* Calling the right handler */
  int ret = (*m3ap_messages_callback[pdu->choice.initiatingMessage.procedureCode][pdu->present - 1])(assoc_id, stream, pdu);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_M3AP_M3AP_PDU, pdu);
  return ret;
}

//------------------------------------------------------------------------------
int
m3ap_mme_set_cause (
  M3AP_Cause_t * cause_p,
  const M3AP_Cause_PR cause_type,
  const long cause_value)
{
  DevAssert (cause_p != NULL);
  cause_p->present = cause_type;

  switch (cause_type) {
  case M3AP_Cause_PR_radioNetwork:
    cause_p->choice.misc = cause_value;
    break;

  case M3AP_Cause_PR_transport:
    cause_p->choice.transport = cause_value;
    break;

  case M3AP_Cause_PR_nAS:
    cause_p->choice.nAS = cause_value;
    break;

  case M3AP_Cause_PR_protocol:
    cause_p->choice.protocol = cause_value;
    break;

  case M3AP_Cause_PR_misc:
    cause_p->choice.misc = cause_value;
    break;

  default:
    return -1;
  }

  return 0;
}
//------------------------------------------------------------------------------
int
m3ap_mme_generate_m3_setup_failure (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
	const M3AP_Cause_PR cause_type,
    const long cause_value,
    const long time_to_wait)
{
  uint8_t                                *buffer_p = 0;
  uint32_t                                length = 0;
  M3AP_M3AP_PDU_t                         pdu;
  M3AP_M3SetupFailure_t                  *out;
  M3AP_M3SetupFailureIEs_t              *ie = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_M3AP);

  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M3AP_M3AP_PDU_PR_unsuccessfulOutcome;
  pdu.choice.unsuccessfulOutcome.procedureCode = M3AP_ProcedureCode_id_m3Setup;
  pdu.choice.unsuccessfulOutcome.criticality = M3AP_Criticality_reject;
  pdu.choice.unsuccessfulOutcome.value.present = M3AP_UnsuccessfulOutcome__value_PR_M3SetupFailure;
  out = &pdu.choice.unsuccessfulOutcome.value.choice.M3SetupFailure;

  ie = (M3AP_M3SetupFailureIEs_t *)calloc(1, sizeof(M3AP_M3SetupFailureIEs_t));
  ie->id = M3AP_ProtocolIE_ID_id_Cause;
  ie->criticality = M3AP_Criticality_ignore;
  ie->value.present = M3AP_M3SetupFailureIEs__value_PR_Cause;
  m3ap_mme_set_cause (&ie->value.choice.Cause, cause_type, cause_value);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /*
   * Include the optional field time to wait only if the value is > -1
   */
  if (time_to_wait > -1) {
    ie = (M3AP_M3SetupFailureIEs_t *)calloc(1, sizeof(M3AP_M3SetupFailureIEs_t));
    ie->id = M3AP_ProtocolIE_ID_id_TimeToWait;
    ie->criticality = M3AP_Criticality_ignore;
    ie->value.present = M3AP_M3SetupFailureIEs__value_PR_TimeToWait;
    ie->value.choice.TimeToWait = time_to_wait;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  if (m3ap_mme_encode_pdu (&pdu, &buffer_p, &length) < 0) {
    OAILOG_ERROR (LOG_M3AP, "Failed to encode m3 setup failure\n");
    OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }

  bstring b = blk2bstr(buffer_p, length);
  free(buffer_p);
  rc =  m3ap_mme_itti_send_sctp_request (&b, assoc_id, stream, INVALID_MCE_MBMS_M3AP_ID);
  OAILOG_FUNC_RETURN (LOG_M3AP, rc);
}

////////////////////////////////////////////////////////////////////////////////
//************************** Management procedures ***************************//
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
int
m3ap_mme_handle_m3_setup_request (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    M3AP_M3AP_PDU_t *pdu)
{

  int                                     rc = RETURNok;
  OAILOG_FUNC_IN (LOG_M3AP);
  M3AP_M3SetupRequest_t                  *container = NULL;
  M3AP_M3SetupRequestIEs_t               *ie = NULL;
  M3AP_M3SetupRequestIEs_t               *ie_mce_name = NULL;
  M3AP_M3SetupRequestIEs_t               *ie_mce_mbms_configuration_per_cell = NULL;
  ecgi_t                                  ecgi = {.plmn = {0}, .cell_identity = {0}};

  m3ap_mce_description_t                 *m3ap_mce_association = NULL;
  uint32_t                                m3ap_mce_id = 0;
  char                                   *m3ap_mce_name = NULL;
  int                                     mbms_sa_ret = 0;
  uint16_t                                max_m3_mce_connected = 0;

  DevAssert (pdu != NULL);
  container = &pdu->choice.initiatingMessage.value.choice.M3SetupRequest;
  /*
   * We received a new valid M3 Setup Request on a stream != 0.
   * * * * This should not happen -> reject M3AP MCE m3 setup request.
   */
  if (stream != 0) {
    OAILOG_ERROR (LOG_M3AP, "Received new m3 setup request on stream != 0\n");
    /*
     * Send a m3 setup failure with protocol cause unspecified
     */
    rc = m3ap_mme_generate_m3_setup_failure (assoc_id, stream, M3AP_Cause_PR_protocol, M3AP_CauseProtocol_unspecified, -1);
    OAILOG_FUNC_RETURN (LOG_M3AP, rc);
  }

  mce_config_read_lock (&mce_config);
  max_m3_mce_connected = mce_config.max_m3_mces;
  mce_config_unlock (&mce_config);

  if (nb_m3ap_mce_associated >= max_m3_mce_connected) {
    OAILOG_ERROR (LOG_M3AP, "There is too much M3 MCEs connected to MME, rejecting the association\n");
    OAILOG_ERROR (LOG_M3AP, "Connected = %d, maximum allowed = %d\n", nb_m3ap_mce_associated, max_m3_mce_connected);
    /*
     * Send an overload cause...
     */
    rc = m3ap_mme_generate_m3_setup_failure (assoc_id, stream, M3AP_Cause_PR_misc, M3AP_CauseMisc_control_processing_overload, M3AP_TimeToWait_v20s);
    OAILOG_FUNC_RETURN (LOG_M3AP, rc);
  }

  shared_log_queue_item_t *  context = NULL;
  OAILOG_MESSAGE_START (OAILOG_LEVEL_DEBUG, LOG_M3AP, (&context), "New m3 setup request incoming from ");

//  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_M3SetupRequest_IEs_t, ie_mce_name, container,
//		  M3AP_ProtocolIE_ID_id_MCEname, false);
//  if (ie) {
//    OAILOG_MESSAGE_ADD (context, "%*s ", (int)ie_mce_name->value.choice.MCEname.size, ie_mce_name->value.choice.MCEname.buf);
//    m3ap_mce_name = (char *) ie_mce_name->value.choice.MCEname.buf;
//  }
//
//  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_M3SetupRequest_IEs_t, ie, container, M3AP_ProtocolIE_ID_id_GlobalENB_ID, true);
//  // TODO make sure ie != NULL
//
//  if (ie->value.choice.GlobalENB_ID.eNB_ID.present == M3AP_ENB_ID_PR_short_Macro_eNB_ID) {
//    uint8_t *enb_id_buf = ie->value.choice.GlobalENB_ID.eNB_ID.choice.short_Macro_eNB_ID.buf;
//    if (ie->value.choice.GlobalENB_ID.eNB_ID.choice.short_Macro_eNB_ID.size != 18) {
//      //TODO: handle case were size != 18 -> notify ? reject ?
//      OAILOG_DEBUG (LOG_M3AP, "M3-Setup-Request shortENB_ID.size %lu (should be 18)\n", ie->value.choice.GlobalENB_ID.eNB_ID.choice.short_Macro_eNB_ID.size);
//    }
//    m3ap_enb_id = (enb_id_buf[0] << 10) + (enb_id_buf[1] << 2) + ((enb_id_buf[2] & 0xc0) >> 6);
//    OAILOG_MESSAGE_ADD (context, "short eNB id: %07x", m3ap_enb_id);
//  } else if (ie->value.choice.GlobalENB_ID.eNB_ID.present == M3AP_ENB_ID_PR_long_Macro_eNB_ID) {
//  	uint8_t *enb_id_buf = ie->value.choice.GlobalENB_ID.eNB_ID.choice.long_Macro_eNB_ID.buf;
//  	if (ie->value.choice.GlobalENB_ID.eNB_ID.choice.long_Macro_eNB_ID.size != 22) {
//  		//TODO: handle case were size != 21 -> notify ? reject ?
//  		OAILOG_DEBUG (LOG_M3AP, "M3-Setup-Request longENB_ID.size %lu (should be 21)\n", ie->value.choice.GlobalENB_ID.eNB_ID.choice.long_Macro_eNB_ID.size);
//  	}
//  	m3ap_enb_id = (enb_id_buf[0] << 13) + (enb_id_buf[1] << 5) + ((enb_id_buf[2] & 0xf8) >> 3);
//  	OAILOG_MESSAGE_ADD (context, "long eNB id: %07x", m3ap_enb_id);
//  } else {
//  	// Macro eNB = 20 bits
//  	uint8_t                                *enb_id_buf = ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf;
//  	if (ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.size != 20) {
//  		//TODO: handle case were size != 20 -> notify ? reject ?
//  		OAILOG_DEBUG (LOG_M3AP, "M3-Setup-Request macroENB_ID.size %lu (should be 20)\n", ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.size);
//  	}
//  	m3ap_enb_id = (enb_id_buf[0] << 12) + (enb_id_buf[1] << 4) + ((enb_id_buf[2] & 0xf0) >> 4);
//  	OAILOG_MESSAGE_ADD (context, "macro eNB id: %05x", m3ap_enb_id);
//  }
//  OAILOG_MESSAGE_FINISH(context);
//
//  /**
//   * Check the MCE PLMN.
//   */
//  if(m3ap_mce_combare_mbms_plmn(&ie->value.choice.GlobalENB_ID.pLMN_Identity) != MBMS_SA_LIST_RET_OK){
//  	OAILOG_ERROR (LOG_M3AP, "No Common PLMN with M3AP eNB, generate_m2_setup_failure\n");
//  	rc =  m3ap_mce_generate_m2_setup_failure (assoc_id, stream, M3AP_Cause_PR_misc, M3AP_CauseMisc_unspecified, M3AP_TimeToWait_v20s);
//  	OAILOG_FUNC_RETURN (LOG_M3AP, rc);
//  }
//
//  /* Requirement MME36.413R10_8.7.3.4 Abnormal Conditions
//   * If the eNB initiates the procedure by sending a M2 SETUP REQUEST message including the PLMN Identity IEs and
//   * none of the PLMNs provided by the eNB is identified by the MME, then the MME shall reject the eNB M2 Setup
//   * Request procedure with the appropriate cause value, e.g, “Unknown PLMN”.
//   */
//  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_M2SetupRequest_IEs_t, ie_enb_mbms_configuration_per_cell, container,
//		  M3AP_ProtocolIE_ID_id_ENB_MBMS_Configuration_data_Item, true);
//
//  M3AP_ENB_MBMS_Configuration_data_List_t	* enb_mbms_cfg_data_list = &ie_enb_mbms_configuration_per_cell->value.choice.ENB_MBMS_Configuration_data_List;
//  /** Take the first cell. */
//  if(enb_mbms_cfg_data_list->list.count > 1){
//    OAILOG_ERROR(LOG_M3AP, "More than 1 cell is not supported for the eNB MBMS. \n");
//    rc =  m3ap_mce_generate_m2_setup_failure (assoc_id, stream, M3AP_Cause_PR_misc, M3AP_CauseMisc_unspecified, M3AP_TimeToWait_v20s);
//    OAILOG_FUNC_RETURN (LOG_M3AP, rc);
//  }
//  M3AP_ENB_MBMS_Configuration_data_Item_t 	* m3ap_enb_mbms_cfg_item = enb_mbms_cfg_data_list->list.array[0];
//  DevAssert(m3ap_enb_mbms_cfg_item);
//  mbms_sa_ret = m3ap_mce_compare_mbms_enb_configuration_item(m3ap_enb_mbms_cfg_item);
//
//  /*
//   * eNB (first cell) and MCE have no common MBMS SA
//   */
//  if (mbms_sa_ret != MBMS_SA_LIST_RET_OK) {
//    OAILOG_ERROR (LOG_M3AP, "No Common PLMN/MBMS_SA with M3AP eNB, generate_m2_setup_failure\n");
//    rc = m3ap_mce_generate_m2_setup_failure(assoc_id, stream,
//    		M3AP_Cause_PR_misc,
//			M3AP_CauseMisc_unspecified,
//			M3AP_TimeToWait_v20s);
//    // todo: test leaks..
//    OAILOG_FUNC_RETURN (LOG_M3AP, rc);
//  }
//
//  mce_config_read_lock (&mce_config);
//  int mbsfn_synch_area_id = mce_config.mbsfn_synch_area_id;
//  mce_config_unlock (&mce_config);
//
//  if(m3ap_enb_mbms_cfg_item->mbsfnSynchronisationArea != mbsfn_synch_area_id){
//  	OAILOG_ERROR (LOG_M3AP, "MBSFN Synch Area Id (%d) not equals to one received from M2 eNB (%d).\n",
//  			mbsfn_synch_area_id, m3ap_enb_mbms_cfg_item->mbsfnSynchronisationArea);
//  	rc = m3ap_mce_generate_m2_setup_failure(assoc_id, stream,
//  			M3AP_Cause_PR_misc,
//				M3AP_CauseMisc_unspecified,
//				M3AP_TimeToWait_v20s);
//  	// todo: test leaks..
//  	OAILOG_FUNC_RETURN (LOG_M3AP, rc);
//  }
//
//  OAILOG_DEBUG (LOG_M3AP, "Adding M3AP eNB with eNB-ID %d to the list of served MBMS eNBs. \n", m3ap_enb_id);
//  if ((m3ap_enb_association = m3ap_is_enb_id_in_list (m3ap_enb_id)) == NULL) {
//    /*
//     * eNB has not been fount in list of associated eNB,
//     * * * * Add it to the tail of list and initialize data
//     */
//    if ((m3ap_enb_association = m3ap_is_enb_assoc_id_in_list (assoc_id)) == NULL) {
//      OAILOG_ERROR(LOG_M3AP, "Ignoring m2 setup from unknown assoc %u and enbId %u", assoc_id, m3ap_enb_id);
//      OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
//    } else {
//      m3ap_enb_association->m2_state = M3AP_RESETING;
//      OAILOG_DEBUG (LOG_M3AP, "Adding M3AP eNB id %u to the list of served eNBs\n", m3ap_enb_id);
//      m3ap_enb_association->m3ap_enb_id = m3ap_enb_id;
//      /**
//       * Set the configuration item of the first cell, which is a single MBSFN area.
//       * Set all MBMS Service Area ids. */
//      m3ap_set_embms_cfg_item(m3ap_enb_association, &m3ap_enb_mbms_cfg_item->mbmsServiceAreaList);
//      if (m3ap_enb_name != NULL) {
//    	  memcpy (m3ap_enb_association->m3ap_enb_name, ie_enb_name->value.choice.ENBname.buf, ie_enb_name->value.choice.ENBname.size);
//    	  m3ap_enb_association->m3ap_enb_name[ie_enb_name->value.choice.ENBname.size] = '\0';
//      }
//    }
//  }	else {
//		m3ap_enb_association->m2_state = M3AP_RESETING;
//
//		/*
//		 * eNB has been fount in list, consider the m2 setup request as a reset connection,
//		 * * * * reseting any previous MBMS state if sctp association is != than the previous one
//		 */
//		if (m3ap_enb_association->sctp_assoc_id != assoc_id) {
//			OAILOG_ERROR (LOG_M3AP, "Rejecting m2 setup request as eNB id %d is already associated to an active sctp association" "Previous known: %d, new one: %d\n",
//			m3ap_enb_id, m3ap_enb_association->sctp_assoc_id, assoc_id);
//			rc = m3ap_mce_generate_m2_setup_failure (assoc_id, stream, M3AP_Cause_PR_misc, M3AP_CauseMisc_unspecified, -1); /**< eNB should attach again. */
//			/** Also remove the old eNB. */
//			OAILOG_INFO(LOG_M3AP, "Rejecting the old M3AP eNB connection for eNB id %d and old assoc_id: %d\n", m3ap_enb_id, assoc_id);
//			m3ap_dump_enb_list();
//			m3ap_handle_sctp_disconnection(m3ap_enb_association->sctp_assoc_id);
//			OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
//		}
//
//		OAILOG_INFO(LOG_M3AP, "We found the M3AP eNB id %d in the current list of enbs with matching sctp associations:" "assoc: %d\n", m3ap_enb_id, m3ap_enb_association->sctp_assoc_id);
//		/*
//		 * TODO: call the reset procedure
//		 * todo: recheck how it affects the MBSFN areas..
//		 */
//		/** Forward this to the MCE_APP layer, too. MBSFN info..). */
//  }
  /**
   * For the case of a new MCE, forward the M3 Setup Request to MCE_APP.
   * It needs the information to create and manage MBSFN areas.
   */
  m3ap_mme_itti_m3ap_mce_setup_request(assoc_id, m3ap_mce_id, &m3ap_mce_association->mbms_sa_list);
  OAILOG_FUNC_RETURN (LOG_M3AP, rc);
}

//------------------------------------------------------------------------------
static
  int
m3ap_generate_m3_setup_response (
  m3ap_mce_description_t * m3ap_mce_association, mbsfn_areas_t * mbsfn_areas)
{
  int                                     			  i,j;
  M3AP_M3AP_PDU_t                         			  pdu;
  M3AP_M3SetupResponse_t                 			 	 *out;
  M3AP_M3SetupResponseIEs_t             			 	 *ie = NULL;
//  M3AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item_t     *mbsfnCfgItem = NULL;
  uint8_t                                			 *buffer = NULL;
  uint32_t                                			  length = 0;
  int                                     			  rc = RETURNok;

//  OAILOG_FUNC_IN (LOG_M3AP);
//  DevAssert (m3ap_mce_association != NULL);
//  memset(&pdu, 0, sizeof(pdu));
//  pdu.present = M3AP_M3AP_PDU_PR_successfulOutcome;
//  pdu.choice.successfulOutcome.procedureCode = M3AP_ProcedureCode_id_m3Setup;
//  pdu.choice.successfulOutcome.criticality = M3AP_Criticality_reject;
//  pdu.choice.successfulOutcome.value.present = M3AP_SuccessfulOutcome__value_PR_M3SetupResponse;
//  out = &pdu.choice.successfulOutcome.value.choice.M3SetupResponse;
//
//  // Generating response
//  mce_config_read_lock (&mce_config);
//  /** mandatory. */
//  ie = (M3AP_M3SetupResponseIEs_t *)calloc(1, sizeof(M3AP_M3SetupResponseIEs_t));
//  ie->id = M3AP_ProtocolIE_ID_id_GlobalMME_ID;
//  ie->criticality = M3AP_Criticality_ignore;
//  ie->value.present = M3AP_M3SetupResponseIEs__value_PR_GlobalMME_ID;
//  INT16_TO_OCTET_STRING (mce_config.gummei.gummei[0].mme_gid, &ie->value.choice.GlobalMME_ID.mME_ID);
//  mce_config_unlock (&mme_config);
//
////  M3AP_PLMN_Identity_t                    *plmn = NULL;
//////  plmn = calloc (1, sizeof (*plmn));
////  ie->value.choice.GlobalMCE_ID.pLMN_Identity.buf = calloc(1, 3);
////  uint16_t                                mcc = 0;
////  uint16_t                                mnc = 0;
////  uint16_t                                mnc_len = 0;
////  /** Get the integer values from the PLMN. */
////  PLMN_T_TO_MCC_MNC ((mce_config.gumcei.gumcei[0].plmn), mcc, mnc, mnc_len);
////  MCC_MNC_TO_PLMNID (mcc, mnc, mnc_len,	&ie->value.choice.GlobalMCE_ID.pLMN_Identity);
////  /** Use same MME code for MCE ID for all eNBs. */
////  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
////
////  /** Skip the MCC name. */
////
////  /** mandatory. **/
////  ie = (M3AP_M2SetupResponse_IEs_t *)calloc(1, sizeof(M3AP_M2SetupResponse_IEs_t));
////  ie->id = M3AP_ProtocolIE_ID_id_MCCHrelatedBCCH_ConfigPerMBSFNArea;
////  ie->criticality = M3AP_Criticality_reject;
////  ie->value.present = M3AP_M2SetupResponse_IEs__value_PR_MCCHrelatedBCCH_ConfigPerMBSFNArea;
////  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
////
////  //  MCCH related BCCH Configuration data per MBSFN area
////  for(int num_mbsfn = 0; num_mbsfn < mbsfn_areas->num_mbsfn_areas; num_mbsfn++) {
////  	/** Only supporting a single MBSFN Area currently. */
////  	// memset for gcc 4.8.4 instead of {0}, servedGUMMEI.servedPLMNs
////	  mbsfnCfgItem = calloc(1, sizeof (M3AP_MCCHrelatedBCCH_ConfigPerMBSFNArea_Item_t));
////  	/** MBSFN Area Id. */
////  	mbsfnCfgItem->mbsfnArea = mbsfn_areas->mbsfn_area_cfg[num_mbsfn].mbsfnArea.mbsfn_area_id;
////  	mbsfnCfgItem->pdcchLength = 2;
////  	mbsfnCfgItem->repetitionPeriod = log2(mbsfn_areas->mbsfn_area_cfg[num_mbsfn].mbsfnArea.mcch_repetition_period_rf / get_csa_period_rf(CSA_PERIOD_RF32));
////  	mbsfnCfgItem->modificationPeriod = log2(mbsfn_areas->mbsfn_area_cfg[num_mbsfn].mbsfnArea.mcch_modif_period_rf / 512);
////  	mbsfnCfgItem->modulationAndCodingScheme = mbsfn_areas->mbsfn_area_cfg[num_mbsfn].mbsfnArea.mbms_mcch_msi_mcs;
////  	ONE_FRAME_ITEM_SF_TO_BIT_STRING(mbsfn_areas->mbsfn_area_cfg[num_mbsfn].mbsfnArea.mbms_mcch_csa_pattern_1rf, &mbsfnCfgItem->subframeAllocationInfo);
////  	mbsfnCfgItem->offset = COMMON_CSA_PATTERN;
////  	/**
////  	 * Subframes, where MCCH could exist for this MBSFN areas.
////  	 * Set the 6 leftmost bits.
////  	 */
////  	ASN_SEQUENCE_ADD(&ie->value.choice.MCCHrelatedBCCH_ConfigPerMBSFNArea.list, mbsfnCfgItem);
////  }
//
//  if (m3ap_mme_encode_pdu (&pdu, &buffer, &length) < 0) {
//    OAILOG_DEBUG (LOG_M3AP, "Removed M3 MCE %d\n", m3ap_mce_association->sctp_assoc_id);
//    hashtable_ts_free (&g_m3ap_mce_coll, m3ap_mce_association->sctp_assoc_id);
//  } else {
//  	/*
//  	 * Consider the response as sent.
//  	 */
//  	m3ap_mce_association->m3_state = M3AP_READY;
//  }

  /*
   * Non-MBMS signalling -> stream 0
   */
  bstring b = blk2bstr(buffer, length);
  free(buffer);
  rc = m3ap_mme_itti_send_sctp_request (&b, m3ap_mce_association->sctp_assoc_id, M3AP_MCE_SERVICE_SCTP_STREAM_ID, INVALID_MME_MBMS_M3AP_ID);
  OAILOG_FUNC_RETURN (LOG_M3AP, rc);
}

//------------------------------------------------------------------------------
int m3ap_handle_m3ap_mce_setup_res(itti_m3ap_mce_setup_res_t * m3ap_mce_setup_res){
	int														rc = RETURNerror;
	m3ap_mce_description_t 			 *m3ap_mce_association = NULL;

	OAILOG_FUNC_IN(LOG_M3AP);

	m3ap_mce_association = m3ap_is_mce_assoc_id_in_list(m3ap_mce_setup_res->sctp_assoc);
  if(!m3ap_mce_association) {
    OAILOG_ERROR (LOG_M3AP, "No MCE association found for %d. Cannot trigger M3 setup response. \n", m3ap_mce_setup_res->sctp_assoc);
    OAILOG_FUNC_RETURN(LOG_M3AP, rc);
  }

  /** Nothing extra is to be done for the MBSFNs. MCE_APP takes care of it. */
  if(!m3ap_mce_setup_res->mbsfn_areas.num_mbsfn_areas){
  	OAILOG_ERROR (LOG_M3AP, "No MBSFN area could be associated for MCE %d. M3 Setup failed. \n", m3ap_mce_setup_res->sctp_assoc);
		rc = m3ap_mme_generate_m3_setup_failure (m3ap_mce_association->sctp_assoc_id, M3AP_MCE_SERVICE_SCTP_STREAM_ID,
				M3AP_Cause_PR_misc, M3AP_CauseMisc_unspecified, -1); /**< MCE should attach again. */
		OAILOG_FUNC_RETURN(LOG_M3AP, rc);
  }
	m3ap_dump_mce(m3ap_mce_association);
  OAILOG_INFO(LOG_M3AP, "MBSFN area could be associated for MCE %d. M3 Setup succeeded. \n", m3ap_mce_setup_res->sctp_assoc);
  /**
   * Update the MBSFN and MBMS areas.
   */
  m3ap_mce_association->local_mbms_area = m3ap_mce_setup_res->local_mbms_area;
  for(int num_mbsfn = 0; num_mbsfn < m3ap_mce_setup_res->mbsfn_areas.num_mbsfn_areas; num_mbsfn++) {
  	/** Set it in the M3 MCE Description element. */
  	m3ap_mce_association->mbsfn_area_ids.mbsfn_area_id[m3ap_mce_association->mbsfn_area_ids.num_mbsfn_area_ids] =
  			m3ap_mce_setup_res->mbsfn_areas.mbsfn_area_cfg[num_mbsfn].mbsfnArea.mbsfn_area_id;
  	m3ap_mce_association->mbsfn_area_ids.mbms_service_area_id[m3ap_mce_association->mbsfn_area_ids.num_mbsfn_area_ids] =
  			m3ap_mce_setup_res->mbsfn_areas.mbsfn_area_cfg[num_mbsfn].mbsfnArea.mbms_service_area_id;
  	m3ap_mce_association->mbsfn_area_ids.num_mbsfn_area_ids++;
  }

  rc =  m3ap_generate_m3_setup_response (m3ap_mce_association, &m3ap_mce_setup_res->mbsfn_areas);
  if (rc == RETURNok) {
  	update_mme_app_stats_connected_m3ap_mce_add();
  }
	OAILOG_FUNC_RETURN(LOG_M3AP, rc);
}

////////////////////////////////////////////////////////////////////////////////
//**************** MBMS Service Context Management procedures ****************//
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
int
m3ap_mme_handle_mbms_session_start_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M3AP_M3AP_PDU_t *pdu)
{
  M3AP_MBMSSessionStartResponse_t     		 *container;
  M3AP_MBMSSessionStartResponse_IEs_t  		 *ie = NULL;
  mbms_description_t              	       *mbms_ref_p = NULL;
  MessageDef                        	     *message_p = NULL;
  mme_mbms_m3ap_id_t                  	    mme_mbms_m3ap_id = 0;
  mce_mbms_m3ap_id_t                    	  mce_mbms_m3ap_id = 0;
  m3ap_mce_description_t				 				   *m3ap_mce_desc = NULL;

  OAILOG_FUNC_IN (LOG_M3AP);
  /**
   * Check that the MCE is not in the SCTP Association Hashmap.
   * Add it into the list. No need to inform the MCE_APP layer.
   */
  container = &pdu->choice.successfulOutcome.value.choice.MBMSSessionStartResponse;
  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_MBMSSessionStartResponse_IEs_t, ie, container,
		  M3AP_ProtocolIE_ID_id_MME_MBMS_M3AP_ID, true);
  mce_mbms_m3ap_id = ie->value.choice.MCE_MBMS_M3AP_ID;
  mce_mbms_m3ap_id &= MCE_MBMS_M3AP_ID_MASK;

  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_MBMSSessionStartResponse_IEs_t, ie, container,
                             M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID, true);
  mce_mbms_m3ap_id = (mce_mbms_m3ap_id_t) (ie->value.choice.MCE_MBMS_M3AP_ID & MCE_MBMS_M3AP_ID_MASK);
  if ((mbms_ref_p = m3ap_is_mbms_mme_m3ap_id_in_list(mme_mbms_m3ap_id)) == NULL) {
    OAILOG_ERROR(LOG_M3AP, "No MBMS is attached to this MCE MBMS M3AP id: " MCE_MBMS_M3AP_ID_FMT "\n", mce_mbms_m3ap_id);
    OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }

  /** Check that the MCE description also exists. */
  DevAssert(m3ap_mce_desc = m3ap_is_mce_assoc_id_in_list(assoc_id));

  /**
   * Try to insert the received MCE MBMS M3AP ID into the MBMS service.
   */
  hashtable_rc_t  h_rc = hashtable_uint64_ts_insert (&mbms_ref_p->g_m3ap_assoc_id2mme_mce_id_coll, (const hash_key_t) assoc_id, (uint64_t)mce_mbms_m3ap_id); /**< Add the value as pointer. No need to free. */
  if (h_rc != HASH_TABLE_OK) {
    OAILOG_ERROR(LOG_M3AP, "Error inserting MBMS description with MME MBMS M3AP ID " MME_MBMS_M3AP_ID_FMT" MCE MBMS M3AP ID " MCE_MBMS_M3AP_ID_FMT". Hash-Rc (%d). Leaving the MBMS reference. \n",
    	mme_mbms_m3ap_id, mce_mbms_m3ap_id, h_rc);
    OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }
  m3ap_mce_desc->nb_mbms_associated++;
  OAILOG_INFO(LOG_M3AP, "Successfully started MBMS Service on MCE with sctp assoc id (%d) MBMS description with MME MBMS M3AP ID " MME_MBMS_M3AP_ID_FMT " MCE MBMS M3AP ID " MCE_MBMS_M3AP_ID_FMT ". "
		  "# of MBMS services on MCE (%d). \n", assoc_id, mme_mbms_m3ap_id, mce_mbms_m3ap_id, m3ap_mce_desc->nb_mbms_associated);
  OAILOG_FUNC_RETURN (LOG_M3AP, RETURNok);
}

//------------------------------------------------------------------------------
int
m3ap_mme_handle_mbms_session_start_failure (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M3AP_M3AP_PDU_t *pdu)
{
  M3AP_MBMSSessionStartFailure_t     	   *container;
  M3AP_MBMSSessionStartFailure_IEs_t     *ie = NULL;
  mbms_description_t                     *mbms_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;
  mme_mbms_m3ap_id_t                      mme_mbms_m3ap_id = 0;

  OAILOG_FUNC_IN (LOG_M3AP);

  /**
   * Check that the MCE is not in the SCTP Association Hashmap.
   * Add it into the list. No need to inform the MCE_APP layer.
   */
  container = &pdu->choice.unsuccessfulOutcome.value.choice.MBMSSessionStartFailure;
  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_MBMSSessionStartFailure_IEs_t, ie, container,
		  M3AP_ProtocolIE_ID_id_MME_MBMS_M3AP_ID, true);
  mme_mbms_m3ap_id = ie->value.choice.MME_MBMS_M3AP_ID;
  mme_mbms_m3ap_id &= MME_MBMS_M3AP_ID_MASK;

  if ((mbms_ref_p = m3ap_is_mbms_mme_m3ap_id_in_list((uint32_t) mme_mbms_m3ap_id)) == NULL) {
    OAILOG_ERROR(LOG_M3AP, "No MBMS is attached to this MME MBMS M3AP id: " MME_MBMS_M3AP_ID_FMT "\n", mme_mbms_m3ap_id);
    OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }
  OAILOG_ERROR(LOG_M3AP, "Error starting MBMS Service on MME with sctp assoc id (%d) MBMS description with MME MBMS M3AP ID " MME_MBMS_M3AP_ID_FMT ". \n", assoc_id, mme_mbms_m3ap_id);
  /**
   * Check that there is no SCTP association.
   */
  hashtable_rc_t h_rc = hashtable_uint64_ts_is_key_exists(&mbms_ref_p->g_m3ap_assoc_id2mme_mce_id_coll, assoc_id);
  DevAssert(h_rc == HASH_TABLE_KEY_NOT_EXISTS);
  OAILOG_FUNC_RETURN (LOG_M3AP, rc);
}

//------------------------------------------------------------------------------
int
m3ap_mme_handle_mbms_session_stop_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M3AP_M3AP_PDU_t *pdu)
{
  M3AP_MBMSSessionStopResponse_t     		 *container;
  M3AP_MBMSSessionStopResponse_IEs_t 	 	 *ie = NULL;
  mbms_description_t                     *mbms_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  mme_mbms_m3ap_id_t                      mme_mbms_m3ap_id = 0;
  mce_mbms_m3ap_id_t                      mce_mbms_m3ap_id = 0;

  OAILOG_FUNC_IN (LOG_M3AP);

  /**
   * Check that the MCE is not in the SCTP Association Hashmap.
   * Add it into the list. No need to inform the MCE_APP layer.
   */
  container = &pdu->choice.successfulOutcome.value.choice.MBMSSessionStopResponse;
  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_MBMSSessionStopResponse_IEs_t, ie, container,
	M3AP_ProtocolIE_ID_id_MME_MBMS_M3AP_ID, true);
  mme_mbms_m3ap_id = ie->value.choice.MME_MBMS_M3AP_ID;
  mme_mbms_m3ap_id &= MME_MBMS_M3AP_ID_MASK;

  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_MBMSSessionStopResponse_IEs_t, ie, container,
	M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID, true);
  mce_mbms_m3ap_id = (mce_mbms_m3ap_id_t) (ie->value.choice.MCE_MBMS_M3AP_ID & MCE_MBMS_M3AP_ID_MASK);

  /**
   * The MBMS Service may or may not exist (removal only in some MCEs).
   * We perform all changes when transmitting the request.
   */
  OAILOG_FUNC_RETURN (LOG_M3AP, RETURNok);
}

//------------------------------------------------------------------------------
int
m3ap_mme_handle_mbms_session_update_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M3AP_M3AP_PDU_t *pdu)
{
  M3AP_MBMSSessionUpdateResponse_t     	 *container;
  M3AP_MBMSSessionUpdateResponse_IEs_t   *ie = NULL;
  mbms_description_t                	   *mbms_ref_p = NULL;
  MessageDef                             *message_p = NULL;
  int                                     rc = RETURNok;
  mme_mbms_m3ap_id_t                      mme_mbms_m3ap_id = 0;
  mce_mbms_m3ap_id_t                      mce_mbms_m3ap_id = 0;

  OAILOG_FUNC_IN (LOG_M3AP);

  /**
   * Check that the MCE is not in the SCTP Association Hashmap.
   * Add it into the list. No need to inform the MCE_APP layer.
   */
  container = &pdu->choice.successfulOutcome.value.choice.MBMSSessionUpdateResponse;
  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_MBMSSessionUpdateResponse_IEs_t, ie, container,
	M3AP_ProtocolIE_ID_id_MME_MBMS_M3AP_ID, true);
  mme_mbms_m3ap_id = ie->value.choice.MME_MBMS_M3AP_ID;
  mme_mbms_m3ap_id &= MME_MBMS_M3AP_ID_MASK;

  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_MBMSSessionUpdateResponse_IEs_t, ie, container,
	M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID, true);
  mce_mbms_m3ap_id = (mce_mbms_m3ap_id_t) (ie->value.choice.MCE_MBMS_M3AP_ID & MCE_MBMS_M3AP_ID_MASK);

  if ((mbms_ref_p = m3ap_is_mbms_mme_m3ap_id_in_list ((uint32_t) mme_mbms_m3ap_id)) == NULL) {
    OAILOG_ERROR(LOG_M3AP, "No MBMS is attached to this MME MBMS M3AP id: " MME_MBMS_M3AP_ID_FMT "\n", mme_mbms_m3ap_id);
    OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }
  /**
   * For any case, just remove the SCTP association.
   */
  OAILOG_INFO(LOG_M3AP, "Successfully updated MBMS Service on MME with sctp assoc id (%d) MBMS description with MME MBMS M3AP ID " MME_MBMS_M3AP_ID_FMT " MCE MBMS M3AP ID " MCE_MBMS_M3AP_ID_FMT ". \n",
	  assoc_id, mme_mbms_m3ap_id, mce_mbms_m3ap_id);
  OAILOG_FUNC_RETURN (LOG_M3AP, RETURNok);
}

//------------------------------------------------------------------------------
int
m3ap_mme_handle_mbms_session_update_failure (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M3AP_M3AP_PDU_t *pdu)
{
  M3AP_MBMSSessionUpdateFailure_t       *container;
  M3AP_MBMSSessionUpdateFailure_IEs_t   *ie = NULL;
  mbms_description_t                		*mbms_ref_p = NULL;
  MessageDef                        		*message_p = NULL;
  m3ap_mce_description_t					  		*m3ap_mce_desc = NULL;
  int                                		 rc = RETURNok;
  mme_mbms_m3ap_id_t                 		 mme_mbms_m3ap_id = 0;
  mce_mbms_m3ap_id_t                 		 mce_mbms_m3ap_id = 0;

  OAILOG_FUNC_IN (LOG_M3AP);

  /**
   * Check that the MCE is not in the SCTP Association Hashmap.
   * Add it into the list. No need to inform the MCE_APP layer.
   */
  container = &pdu->choice.unsuccessfulOutcome.value.choice.MBMSSessionUpdateFailure;
  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_MBMSSessionUpdateFailure_IEs_t, ie, container,
		  M3AP_ProtocolIE_ID_id_MME_MBMS_M3AP_ID, true);
  mme_mbms_m3ap_id = ie->value.choice.MME_MBMS_M3AP_ID;
  mme_mbms_m3ap_id &= MME_MBMS_M3AP_ID_MASK;

  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_MBMSSessionUpdateFailure_IEs_t, ie, container,
	M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID, true);
  mce_mbms_m3ap_id = (mce_mbms_m3ap_id_t) (ie->value.choice.MCE_MBMS_M3AP_ID & MCE_MBMS_M3AP_ID_MASK);

  if ((mbms_ref_p = m3ap_is_mbms_mme_m3ap_id_in_list ((uint32_t) mme_mbms_m3ap_id)) == NULL) {
    OAILOG_ERROR(LOG_M3AP, "No MBMS is attached to this MME MBMS M3AP id: " MME_MBMS_M3AP_ID_FMT "\n", mme_mbms_m3ap_id);
    OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }
  m3ap_mce_desc = m3ap_is_mce_assoc_id_in_list(assoc_id);
  if(m3ap_mce_desc) {
    OAILOG_ERROR(LOG_M3AP, "Failed updating MBMS Service on MCE with sctp assoc id (%d) MBMS description with MME MBMS M3AP ID " MME_MBMS_M3AP_ID_FMT ". \n",
    	assoc_id, mme_mbms_m3ap_id);
    m3ap_generate_mbms_session_stop_request(mme_mbms_m3ap_id, assoc_id);
    /** Remove the hash key, which should also update the MCE. */
    hashtable_uint64_ts_free(&mbms_ref_p->g_m3ap_assoc_id2mme_mce_id_coll, (const hash_key_t)assoc_id);
    if(m3ap_mce_desc->nb_mbms_associated)
    	m3ap_mce_desc->nb_mbms_associated--;
    OAILOG_ERROR(LOG_M3AP, "Removed association after failed update.\n");
  }
  OAILOG_FUNC_RETURN (LOG_M3AP, rc);
}

//------------------------------------------------------------------------------
typedef struct arg_m3ap_construct_mce_reset_req_s {
  uint8_t      current_mbms_index;
  MessageDef  *message_p;
}arg_m3ap_construct_mce_reset_req_t;

//------------------------------------------------------------------------------
int
m3ap_mme_handle_reset_request (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M3AP_M3AP_PDU_t *pdu) {
  M3AP_Reset_t					     					*container;
  M3AP_ResetIEs_t 				     				*ie = NULL;
  mbms_description_t              		*mbms_ref_p = NULL;
  m3ap_mce_description_t			 				*m3ap_mce_association = NULL;
  itti_m3ap_mce_initiated_reset_ack_t  m3ap_mce_initiated_reset_ack = {0};

  OAILOG_FUNC_IN (LOG_M3AP);
  /*
   * Checking that the assoc id has a valid MCE attached to.
   */
  m3ap_mce_association = m3ap_is_mce_assoc_id_in_list (assoc_id);
  if (m3ap_mce_association == NULL) {
    OAILOG_ERROR (LOG_M3AP, "No M3AP MCE attached to this assoc_id: %d\n", assoc_id);
    OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }
  // Check the state of M3AP
  if (m3ap_mce_association->m3_state != M3AP_READY) {
    // Ignore the message
    OAILOG_INFO (LOG_M3AP, "M3 setup is not done.Invalid state. Ignoring MCE Initiated Reset.MCE Id = %d , M3AP state = %d \n", m3ap_mce_association->m3ap_mce_id, m3ap_mce_association->m3_state);
    OAILOG_FUNC_RETURN (LOG_M3AP, RETURNok);
  }
  // Check the reset type - partial_reset OR M3AP_RESET_ALL
  container = &pdu->choice.initiatingMessage.value.choice.Reset;
  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_ResetIEs_t, ie, container, M3AP_ProtocolIE_ID_id_ResetType, true);
//  M3AP_ResetType_t * m3ap_reset_type = &ie->value.choice.ResetType;
//  switch (m3ap_reset_type->present){
//    case M3AP_ResetType_PR_m3_Interface:
//      m3ap_mce_initiated_reset_ack.m3ap_reset_type = M3AP_RESET_ALL;
//	  break;
//    case M3AP_ResetType_PR_partOfM2_Interface:
//    	m3ap_mce_initiated_reset_ack.m3ap_reset_type = M3AP_RESET_PARTIAL;
//  	  break;
//    default:
//	  OAILOG_ERROR (LOG_M3AP, "Reset Request from M3AP MCE  with Invalid reset_type = %d\n", m3ap_reset_type->present);
//	  // TBD - Here MME should send Error Indication as it is abnormal scenario.
//	  OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
//  }
//  m3ap_mce_initiated_reset_ack.sctp_assoc_id 	= assoc_id;
//  m3ap_mce_initiated_reset_ack.sctp_stream_id 	= stream;
//  /** Create a cleared array of identifiers. */
//  if (m3ap_mce_initiated_reset_ack.m3ap_reset_type != M3AP_RESET_ALL) {
//	// Check if there are no MBMSs connected
//	if (!m3ap_mce_association->nb_mbms_associated) {
//	  // Ignore the message
//	  OAILOG_INFO (LOG_M3AP, "No MBMSs is active for MCE. Still continuing with MCE Initiated Reset.MCE Id = %d\n",
//		  m3ap_mce_association->m3ap_mce_id);
//	}
//	m3ap_mce_initiated_reset_ack.num_mbms = m3ap_reset_type->choice.partOfM2_Interface.list.count;
//	m2_sig_conn_id_t mbms_to_reset_list[m3ap_mce_initiated_reset_ack.num_mbms]; /**< Create a stacked array. */
//	memset(mbms_to_reset_list, 0, m3ap_enb_initiated_reset_ack.num_mbms * sizeof(m2_sig_conn_id_t));
//	for (int item = 0; item < m3ap_enb_initiated_reset_ack.num_mbms; item++) {
//	  M3AP_MBMS_Service_associatedLogicalM2_ConnectionItemResAck_t *m2_sig_conn_p = (M3AP_MBMS_Service_associatedLogicalM2_ConnectionItemResAck_t*)
//		ie->value.choice.ResetType.choice.partOfM2_Interface.list.array[item];
//	  /** Initialize each element with invalid values. */
//	  mbms_to_reset_list[item].mce_mbms_m3ap_id = INVALID_MCE_MBMS_M3AP_ID;
//	  mbms_to_reset_list[item].mce_mbms_m3ap_id = INVALID_ENB_MBMS_M3AP_ID;
//	  if(!m2_sig_conn_p) {
//	    OAILOG_ERROR (LOG_M3AP, "No logical M2 connection item could be found for the partial connection. "
//	    		"Ignoring the received partial reset request. \n");
//	    /** No need to answer this one, continue to the next. */
//	    continue;
//	  }
//	  /** We will only implicitly remove the MBMS Description, based on the MCE MBMS M3AP ID or the ENB MBMS M3AP ID. */
//	  M3AP_MBMS_Service_associatedLogicalM2_ConnectionItem_t * m2_sig_conn_item = &m2_sig_conn_p->value.choice.MBMS_Service_associatedLogicalM2_ConnectionItem;
//	  if (m2_sig_conn_item->mCE_MBMS_M3AP_ID != NULL) {
//		mbms_to_reset_list[item].mce_mbms_m3ap_id = ((mce_mbms_m3ap_id_t) *(m2_sig_conn_item->mCE_MBMS_M3AP_ID) & MCE_MBMS_M3AP_ID_MASK);
//	  }
//	  if (m2_sig_conn_item->eNB_MBMS_M3AP_ID != NULL) {
//	    mbms_to_reset_list[item].mce_mbms_m3ap_id = (mce_mbms_m3ap_id_t) *(m2_sig_conn_item->eNB_MBMS_M3AP_ID) & ENB_MBMS_M3AP_ID_MASK;
//	  }
//	  if(mbms_to_reset_list[item].mce_mbms_m3ap_id == INVALID_MCE_MBMS_M3AP_ID && mbms_to_reset_list[item].mce_mbms_m3ap_id != INVALID_ENB_MBMS_M3AP_ID){
//		OAILOG_ERROR (LOG_M3AP, "Received invalid MBMS identifiers. Continuing... \n");
//		continue;
//	  }
//	  /**
//	   * Only evaluate the MCE MBMS M3AP Id, should exist.
//	   */
//	  if(mbms_to_reset_list[item].mce_mbms_m3ap_id == INVALID_MCE_MBMS_M3AP_ID){
//		OAILOG_ERROR (LOG_M3AP, "Received invalid MCE MBMS M3AP ID in partial reset request. Skipping. \n");
//		continue;
//	  }
//	  if((mbms_ref_p = m3ap_is_mbms_mce_m3ap_id_in_list(mbms_to_reset_list[item].mce_mbms_m3ap_id)) == NULL) {
//        /** No MCE MBMS reference is found, skip the partial reset request. */
//		OAILOG_ERROR (LOG_M3AP, "No MBMS Reference was found for MCE MBMS M3AP ID "MCE_MBMS_M3AP_ID_FMT" in partial reset request. Skipping (will add response). \n",
//		  mbms_to_reset_list[item].mce_mbms_m3ap_id);
//		continue;
//	  }
//	  /** Check the received eNB MBMS Id, if it is valid, check it.*/
//	  mce_mbms_m3ap_id_t current_mce_mbms_m3ap_id = INVALID_ENB_MBMS_M3AP_ID;
//	  hashtable_uint64_ts_get(&mbms_ref_p->g_m3ap_assoc_id2mce_enb_id_coll, assoc_id, (void*)&current_mce_mbms_m3ap_id);
//	  if(current_mce_mbms_m3ap_id != INVALID_ENB_MBMS_M3AP_ID){
//	    if(current_mce_mbms_m3ap_id != mbms_to_reset_list[item].mce_mbms_m3ap_id) {
//	    	OAILOG_ERROR (LOG_M3AP, "MBMS Service for MCE MBMS M3AP ID "MCE_MBMS_M3AP_ID_FMT" has ENB MBMS M3AP ID " ENB_MBMS_M3AP_ID_FMT ", "
//	      		"whereas partial reset received for ENB MBMS M3AP ID " ENB_MBMS_M3AP_ID_FMT ". Skipping. \n",
//						mbms_to_reset_list[item].mce_mbms_m3ap_id, current_mce_mbms_m3ap_id, mbms_to_reset_list[item].mce_mbms_m3ap_id);
//	    	continue;
//	    }
//	  }
//	  OAILOG_WARNING(LOG_M3AP, "Removing SCTP association of MBMS Service for MCE MBMS M3AP ID "MCE_MBMS_M3AP_ID_FMT" with ENB MBMS M3AP ID " ENB_MBMS_M3AP_ID_FMT " due"
//		"partial reset from eNB with sctp-assoc-id (%d). \n", mbms_to_reset_list[item].mce_mbms_m3ap_id,
//		mbms_to_reset_list[item].mce_mbms_m3ap_id, assoc_id);
//	  /** Remove the hash key, which should also update the eNB. */
//	  hashtable_uint64_ts_free(&mbms_ref_p->g_m3ap_assoc_id2mce_enb_id_coll, (const hash_key_t)assoc_id);
//	  if(m3ap_enb_association->nb_mbms_associated)
//		m3ap_enb_association->nb_mbms_associated--;
//	}
//	OAILOG_INFO(LOG_M3AP, "Successfully performed partial reset for M3AP eNB with sctp-assoc-id (%d). Sending back M3AP Reset ACK. \n", assoc_id);
//	m3ap_enb_initiated_reset_ack.mbsm_to_reset_list = mbms_to_reset_list;
//	m3ap_handle_enb_initiated_reset_ack(&m3ap_enb_initiated_reset_ack);	  /**< Array scope. */
// } else {
//	  /** Generating full reset request. */
//	  m3ap_enb_full_reset((void*)m3ap_enb_association);
//	  m3ap_handle_enb_initiated_reset_ack(&m3ap_enb_initiated_reset_ack); /**< Array scope. */
//	  OAILOG_INFO (LOG_M3AP, "Received a full reset request from eNB with SCTP-ASSOC (%d). Removing all SCTP associations. Sending RESET ACK.\n", assoc_id);
//  }
  OAILOG_FUNC_RETURN (LOG_M3AP, RETURNok);
}

//------------------------------------------------------------------------------
int
m3ap_mme_handle_error_indication (
    const sctp_assoc_id_t assoc_id,
    const sctp_stream_id_t stream,
    M3AP_M3AP_PDU_t *pdu)
{
  mce_mbms_m3ap_id_t                        mce_mbms_m3ap_id = INVALID_MCE_MBMS_M3AP_ID;
  mme_mbms_m3ap_id_t                        mme_mbms_m3ap_id = INVALID_MME_MBMS_M3AP_ID;
  M3AP_ErrorIndication_t             	     *container = NULL;
  M3AP_ErrorIndication_IEs_t 			   			 *ie = NULL;
  MessageDef                               *message_p = NULL;
  long                                      cause_value;

  OAILOG_FUNC_IN (LOG_M3AP);

  container = &pdu->choice.initiatingMessage.value.choice.ErrorIndication;
  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_ErrorIndication_IEs_t, ie, container, M3AP_ProtocolIE_ID_id_MME_MBMS_M3AP_ID, false);
  if(ie) {
	mme_mbms_m3ap_id = ie->value.choice.MME_MBMS_M3AP_ID;
	mme_mbms_m3ap_id &= MME_MBMS_M3AP_ID_MASK;
  }

  M3AP_FIND_PROTOCOLIE_BY_ID(M3AP_ErrorIndication_IEs_t, ie, container, M3AP_ProtocolIE_ID_id_MCE_MBMS_M3AP_ID, false);
  // MCE MBMS M3AP ID is limited to 24 bits
  if(ie)
    mce_mbms_m3ap_id = (mce_mbms_m3ap_id_t) (ie->value.choice.MCE_MBMS_M3AP_ID & MCE_MBMS_M3AP_ID_MASK);

  m3ap_mce_description_t * m3ap_mce_ref = m3ap_is_mce_assoc_id_in_list (assoc_id);
  DevAssert(m3ap_mce_ref);

  if(mme_mbms_m3ap_id == INVALID_MME_MBMS_M3AP_ID){
    OAILOG_ERROR(LOG_M3AP, "Received invalid mme_mbms_m3ap_id " MME_MBMS_M3AP_ID_FMT". Dropping error indication. \n", mme_mbms_m3ap_id);
    OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }

  mbms_description_t * mbms_ref = m3ap_is_mbms_mme_m3ap_id_in_list(mme_mbms_m3ap_id);
  /** If no MBMS reference exists, drop the message. */
  if(!mbms_ref){
    /** Check that the MBMS reference exists. */
    OAILOG_ERROR(LOG_M3AP, "No MBMS reference exists for MME MBMS Id "MME_MBMS_M3AP_ID_FMT ". Dropping error indication. \n",
    	mme_mbms_m3ap_id);
    OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }

  /**
   * No need to inform the MCE App layer, implicitly remove the MBMS Service session.
   */
  OAILOG_ERROR(LOG_M3AP, "Received Error indication for MBMS Service MME MBMS M3AP ID " MME_MBMS_M3AP_ID_FMT " with MCE MBMS M3AP Id " MCE_MBMS_M3AP_ID_FMT " on MCE with sctp assoc id (%d). \n",
		  mme_mbms_m3ap_id, mce_mbms_m3ap_id, assoc_id);
  m3ap_generate_mbms_session_stop_request(mme_mbms_m3ap_id, assoc_id);
  /** Remove the hash key, which should also update the MCE. */
  hashtable_uint64_ts_free(&mbms_ref->g_m3ap_assoc_id2mme_mce_id_coll, (const hash_key_t)assoc_id);
  if(m3ap_mce_ref->nb_mbms_associated)
	m3ap_mce_ref->nb_mbms_associated--;
  OAILOG_ERROR(LOG_M3AP, "Removed association after error indication.\n");
  OAILOG_FUNC_RETURN (LOG_M3AP, RETURNok);
}

//------------------------------------------------------------------------------
int
m3ap_handle_sctp_disconnection (
    const sctp_assoc_id_t assoc_id)
{
  int                                     i = 0;
  m3ap_mce_description_t                 *m3ap_mce_association = NULL;

  OAILOG_FUNC_IN (LOG_M3AP);
  /*
   * Checking that the assoc id has a valid MCE attached to.
   */
  m3ap_mce_association = m3ap_is_mce_assoc_id_in_list (assoc_id);

  if (m3ap_mce_association == NULL) {
    OAILOG_ERROR (LOG_M3AP, "No M3AP MCE attached to this assoc_id: %d\n", assoc_id);
    OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
  }
  /**
   * Remove the MCE association, which will also remove all SCTP associations of the MBMS Services.
   */
  hashtable_ts_free (&g_m3ap_mce_coll, m3ap_mce_association->sctp_assoc_id);
  update_mme_app_stats_connected_m3ap_mce_sub();
  OAILOG_INFO(LOG_M3AP, "Removing M3AP MCE with association id %u \n", assoc_id);
  m3ap_dump_mce_list ();
  OAILOG_DEBUG (LOG_M3AP, "Removed M3AP MCE attached to assoc_id: %d\n", assoc_id);
  OAILOG_FUNC_RETURN (LOG_M3AP, RETURNok);
}

//------------------------------------------------------------------------------
int
m3ap_handle_new_association (
  sctp_new_peer_t * sctp_new_peer_p)
{
  m3ap_mce_description_t                      *m3ap_mce_association = NULL;

  OAILOG_FUNC_IN (LOG_M3AP);
  DevAssert (sctp_new_peer_p != NULL);

  /*
   * Checking that the assoc id has a valid MCE attached to.
   */
  if ((m3ap_mce_association = m3ap_is_mce_assoc_id_in_list (sctp_new_peer_p->assoc_id)) == NULL) {
    OAILOG_DEBUG (LOG_M3AP, "Create MCE context for assoc_id: %d\n", sctp_new_peer_p->assoc_id);
    /*
     * Create new context
     */
    m3ap_mce_association = m3ap_new_mce ();

    if (m3ap_mce_association == NULL) {
      /*
       * We failed to allocate memory
       */
      /*
       * TODO: send reject there
       */
      OAILOG_ERROR (LOG_M3AP, "Failed to allocate MCE context for assoc_id: %d\n", sctp_new_peer_p->assoc_id);
    }
    m3ap_mce_association->sctp_assoc_id = sctp_new_peer_p->assoc_id;
    hashtable_rc_t  hash_rc = hashtable_ts_insert (&g_m3ap_mce_coll, (const hash_key_t)m3ap_mce_association->sctp_assoc_id, (void *)m3ap_mce_association);
    if (HASH_TABLE_OK != hash_rc) {
      OAILOG_FUNC_RETURN (LOG_M3AP, RETURNerror);
    }
  } else if ((m3ap_mce_association->m3_state == M3AP_SHUTDOWN) || (m3ap_mce_association->m3_state == M3AP_RESETING)) {
    OAILOG_WARNING(LOG_M3AP, "Received new association request on an association that is being %s, ignoring",
                   m3_mce_state_str[m3ap_mce_association->m3_state]);
    OAILOG_FUNC_RETURN(LOG_M3AP, RETURNerror);
  } else {
    OAILOG_DEBUG (LOG_M3AP, "MCE context already exists for assoc_id: %d, update it\n", sctp_new_peer_p->assoc_id);
  }

  m3ap_mce_association->sctp_assoc_id = sctp_new_peer_p->assoc_id;
  /*
   * Fill in in and out number of streams available on SCTP connection.
   */
  m3ap_mce_association->instreams = sctp_new_peer_p->instreams;
  m3ap_mce_association->outstreams = sctp_new_peer_p->outstreams;
  /*
   * initialize the next sctp stream to 1 as 0 is reserved for non
   * * * * ue associated signalling.
   */
//  m3ap_mce_association->next_sctp_stream = 1;
  m3ap_mce_association->m3_state = M3AP_INIT;
  OAILOG_FUNC_RETURN (LOG_M3AP, RETURNok);
}

//------------------------------------------------------------------------------
int
m3ap_handle_mce_initiated_reset_ack (
  const itti_m3ap_mce_initiated_reset_ack_t * const m3ap_mce_initiated_reset_ack_p)
{
  uint8_t                                *buffer = NULL;
  uint32_t                                length = 0;
  M3AP_M3AP_PDU_t                         pdu;
  /** Reset Acknowledgment. */
  M3AP_ResetAcknowledge_t				 				 *out;
  M3AP_ResetAcknowledgeIEs_t	    	 		 *reset_ack_ie = NULL;
  M3AP_ResetAcknowledgeIEs_t	   	       *reset_ack_ie_list = NULL;
  int                                     rc = RETURNok;

  OAILOG_FUNC_IN (LOG_M3AP);

  memset(&pdu, 0, sizeof(pdu));
  pdu.present = M3AP_M3AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = M3AP_ProcedureCode_id_Reset;
  pdu.choice.successfulOutcome.criticality = M3AP_Criticality_ignore;
  pdu.choice.successfulOutcome.value.present = M3AP_SuccessfulOutcome__value_PR_ResetAcknowledge;
  out = &pdu.choice.successfulOutcome.value.choice.ResetAcknowledge;

  if (m3ap_mce_initiated_reset_ack_p->m3ap_reset_type == MxAP_RESET_PARTIAL) {
  	/** Add an Item for each counter. */
  	if(m3ap_mce_initiated_reset_ack_p->mbsm_to_reset_list){
  		reset_ack_ie_list = (M3AP_ResetAcknowledgeIEs_t *)calloc(1, sizeof(M3AP_ResetAcknowledgeIEs_t));
  		reset_ack_ie_list->id = M3AP_ProtocolIE_ID_id_MBMS_Service_associatedLogicalM3_ConnectionListResAck;
  		reset_ack_ie_list->criticality = M3AP_Criticality_ignore;
  		reset_ack_ie_list->value.present = M3AP_ResetAcknowledgeIEs__value_PR_MBMS_Service_associatedLogicalM3_ConnectionListResAck;
  		ASN_SEQUENCE_ADD(&out->protocolIEs.list, reset_ack_ie_list);
  		/** MME MBMS M3AP ID. */
  		M3AP_MBMS_Service_associatedLogicalM3_ConnectionListResAck_t * ie_p = &reset_ack_ie_list->value.choice.MBMS_Service_associatedLogicalM3_ConnectionListResAck;
  		/** Allocate an item for each. */
  		for (uint32_t i = 0; i < m3ap_mce_initiated_reset_ack_p->num_mbms; i++) {
  			M3AP_MBMS_Service_associatedLogicalM3_ConnectionItemResAck_t * sig_conn_item = calloc (1, sizeof (M3AP_MBMS_Service_associatedLogicalM3_ConnectionItemResAck_t));
  			sig_conn_item->id = M3AP_ProtocolIE_ID_id_MBMS_Service_associatedLogicalM3_ConnectionItem;
  			sig_conn_item->criticality = M3AP_Criticality_ignore;
  			sig_conn_item->value.present = M3AP_MBMS_Service_associatedLogicalM3_ConnectionItemResAck__value_PR_MBMS_Service_associatedLogicalM3_ConnectionItem;
  			M3AP_MBMS_Service_associatedLogicalM3_ConnectionItem_t * item = &sig_conn_item->value.choice.MBMS_Service_associatedLogicalM3_ConnectionItem;
  			if (m3ap_mce_initiated_reset_ack_p->mbsm_to_reset_list[i].mme_mbms_m3ap_id != INVALID_MME_MBMS_M3AP_ID) {
  				item->mME_MBMS_M3AP_ID = calloc(1, sizeof(M3AP_MME_MBMS_M3AP_ID_t));
  				*item->mME_MBMS_M3AP_ID = m3ap_mce_initiated_reset_ack_p->mbsm_to_reset_list[i].mme_mbms_m3ap_id;
  			}
  			else {
  				item->mME_MBMS_M3AP_ID = NULL;
  			}
  			/** MCE MBMS M3AP ID. */
  			if (m3ap_mce_initiated_reset_ack_p->mbsm_to_reset_list[i].mce_mbms_m3ap_id != INVALID_MCE_MBMS_M3AP_ID) {
  				item->mME_MBMS_M3AP_ID = calloc(1, sizeof(M3AP_MCE_MBMS_M3AP_ID_t));
  				*item->mME_MBMS_M3AP_ID = m3ap_mce_initiated_reset_ack_p->mbsm_to_reset_list[i].mce_mbms_m3ap_id;
  			}
  			else {
  				item->mME_MBMS_M3AP_ID = NULL;
  			}
  			ASN_SEQUENCE_ADD(&ie_p->list, sig_conn_item);
  		}
  	}
  }

  if (m3ap_mme_encode_pdu (&pdu, &buffer, &length) < 0) {
  	OAILOG_ERROR (LOG_M3AP, "Failed to M3 encode MCE initiated Reset ack. \n");
  	/** We rely on the handover_notify timeout to remove the MBMS context. */
  	DevAssert(!buffer);
  	OAILOG_FUNC_OUT (LOG_M3AP);
  }

  if(buffer) {
  	bstring b = blk2bstr(buffer, length);
  	free(buffer);
  	rc = m3ap_mme_itti_send_sctp_request (&b, m3ap_mce_initiated_reset_ack_p->sctp_assoc_id, m3ap_mce_initiated_reset_ack_p->sctp_stream_id, INVALID_MME_MBMS_M3AP_ID);
  }
  OAILOG_FUNC_RETURN (LOG_M3AP, rc);
}
