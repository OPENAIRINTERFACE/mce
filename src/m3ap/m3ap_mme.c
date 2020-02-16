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

/*! \file m3ap_mce.c
  \brief
  \author Dincer BEKEN
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#if HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>


#include "bstrlib.h"
#include "queue.h"
#include "tree.h"

#include "hashtable.h"
#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "timer.h"
#include "itti_free_defined_msg.h"
#include "m3ap_mme.h"
#include "m3ap_mme_decoder.h"
#include "m3ap_mme_handlers.h"
#include "m3ap_mme_procedures.h"
#include "m3ap_mme_retransmission.h"
#include "m3ap_mme_itti_messaging.h"
#include "dynamic_memory_check.h"
#include "3gpp_23.003.h"
#include "mce_config.h"


#if M3AP_DEBUG_LIST
#  define m3AP_mCE_LIST_OUT(x, args...) OAILOG_DEBUG (LOG_M3AP, "[MCE]%*s"x"\n", 4*indent, "", ##args)
#  define MBMS_LIST_OUT(x, args...)  OAILOG_DEBUG (LOG_M3AP, "[MBMS] %*s"x"\n", 4*indent, "", ##args)
#else
#  define m3AP_mCE_LIST_OUT(x, args...)
#  define MBMS_LIST_OUT(x, args...)
#endif


uint32_t                nb_m3ap_mce_associated 			= 0;
static mme_mbms_m3ap_id_t mme_mbms_m3ap_id_generator 	= 1;

hash_table_ts_t g_m3ap_mce_coll 	= {.mutex = PTHREAD_MUTEX_INITIALIZER, 0}; // contains mce_description_s, key is mce_description_s.mce_d (uint16_t);
hash_table_ts_t g_m3ap_mbms_coll 	= {.mutex = PTHREAD_MUTEX_INITIALIZER, 0}; // contains MBMS_description_s, key is MBMS_description_s.mbms_m3ap_id (uint16_t);
/** An MBMS Service can be associated to multiple SCTP IDs. */

static int                              indent = 0;
extern struct mce_config_s              mce_config;
void *m3ap_mce_thread (void *args);

//------------------------------------------------------------------------------
static int m3ap_send_init_sctp (void)
{
  // Create and alloc new message
  MessageDef                             *message_p = NULL;

  message_p = itti_alloc_new_message (TASK_M3AP, SCTP_INIT_MSG);
  if (message_p) {
    message_p->ittiMsg.sctpInit.port = M3AP_PORT_NUMBER;
    message_p->ittiMsg.sctpInit.ppid = M3AP_SCTP_PPID;
    message_p->ittiMsg.sctpInit.ipv4 = 1;
    message_p->ittiMsg.sctpInit.ipv6 = 1;
    message_p->ittiMsg.sctpInit.nb_ipv4_addr = 0;
    message_p->ittiMsg.sctpInit.nb_ipv6_addr = 0;
    message_p->ittiMsg.sctpInit.ipv4_address[0].s_addr = mce_config.ip.mc_mce_v4.s_addr;

    if (mce_config.ip.mc_mce_v4.s_addr != INADDR_ANY) {
      message_p->ittiMsg.sctpInit.nb_ipv4_addr = 1;
    }
    if(memcmp(&mce_config.ip.mc_mce_v6.s6_addr, (void*)&in6addr_any, sizeof(mce_config.ip.mc_mce_v6.s6_addr)) != 0) {
      message_p->ittiMsg.sctpInit.nb_ipv6_addr = 1;
      memcpy(message_p->ittiMsg.sctpInit.ipv6_address[0].s6_addr, mce_config.ip.mc_mce_v6.s6_addr, 16);
    }
    /*
     * SR WARNING: ipv6 multi-homing fails sometimes for localhost.
     * Disable it for now.
     * todo : ?!?!?
     */
//    message_p->ittiMsg.sctpInit.nb_ipv6_addr = 0;
    return itti_send_msg_to_task (TASK_SCTP, INSTANCE_DEFAULT, message_p);
  }
  return RETURNerror;
}

//------------------------------------------------------------------------------
static bool m3ap_mce_mbms_remove_sctp_assoc_id_cb (__attribute__((unused)) const hash_key_t key,
                                      void * const elementP, void * parameterP, void **resultP)
{
  mbms_description_t                       *mbms_ref           = (mbms_description_t*)elementP;
  /**
   * Just remove the SCTP association from each MBMS service.
   * This method will be triggered by the eNodeB only, does not trigger any additional MCE removals or other methods.
   * Returns false, to iterate through all MBMS services for the given SCTP key.
   */
  hashtable_uint64_ts_free(&mbms_ref->g_m3ap_assoc_id2mme_mce_id_coll, *((sctp_assoc_id_t*)parameterP));
  return false;
}

//------------------------------------------------------------------------------
static bool m3ap_clear_mce_mbms_stats_cb (__attribute__((unused)) const hash_key_t key,
                                      void * const elementP, void * parameterP, void __attribute__((unused)) **resultP)
{
  mce_mbms_m3ap_id_t			*mce_mbms_m3ap_id_p = (mce_mbms_m3ap_id_t*)elementP;
  sctp_assoc_id_t				 	 sctp_assoc_id 		  = (sctp_assoc_id_t)key;
  m3ap_mce_description_t	*m3ap_mce_ref 		  = NULL;

  /**
   * Get the MCE with the given SCTP_ASSOC ID.
   * Clear the stats of the MCE.
   */
  hashtable_ts_get(&g_m3ap_mce_coll, (const hash_key_t)sctp_assoc_id, (void**)&m3ap_mce_ref);
  if(m3ap_mce_ref) {
    /**
     * Found an M3AP MCE description element.
     * Updating number of MBMS Services of the MBMS MCE.
     */
  	if(m3ap_mce_ref->nb_mbms_associated > 0)
  		m3ap_mce_ref->nb_mbms_associated--;
  	/**
  	 * No conflicts should occur, since it checks above the existence of MBMS service count.
  	 */
  	if (!m3ap_mce_ref->nb_mbms_associated) {
  		if (m3ap_mce_ref->m3_state == M3AP_RESETING) {
  			OAILOG_INFO(LOG_M3AP, "Moving M3AP MCE state to M3AP_INIT");
  			m3ap_mce_ref->m3_state = M3AP_INIT;
  			update_mme_app_stats_connected_m3ap_mce_sub();
  		} else if (m3ap_mce_ref->m3_state == M3AP_SHUTDOWN) {
  			OAILOG_INFO(LOG_M3AP, "Deleting MCE");
  			hashtable_ts_free (&g_m3ap_mce_coll, m3ap_mce_ref->sctp_assoc_id);
  		}
  	}
  }
  /**
   * Don't remove anything from the hash set here.
   * Always return false, such that all SCTP keys of the MBMS service are iterated.
   */
  return false;
}

//------------------------------------------------------------------------------
static void
m3ap_remove_mce (
  void ** mce_ref)
{
  m3ap_mce_description_t               *m3ap_mce_description = NULL;
  mbms_description_t                   *mbms_ref 			 = NULL;
  if (*mce_ref ) {
    m3ap_mce_description = (m3ap_mce_description_t*)(*mce_ref);
    /**
     * Go through the MBMS Services, and remove the SCTP association.
     * This should not trigger anything, only key removal.
     * No need to check the # of MBMS services of the MCE. If the number of MBMS services,
     * which we receive from the counter is not equal to the current one, use it as a response.
     * Also use that value for anything received from the MCE_APP layer.
     */
    hashtable_ts_apply_callback_on_elements((hash_table_ts_t * const)&g_m3ap_mbms_coll,
    	m3ap_mce_mbms_remove_sctp_assoc_id_cb, (void*)&m3ap_mce_description->sctp_assoc_id, (void**)&mbms_ref);
    free_wrapper(mce_ref);
    nb_m3ap_mce_associated--;
  }
  return;
}

//------------------------------------------------------------------------------
static void
m3ap_remove_mbms (
  void ** mbms_ref_pp)
{
  m3ap_mce_description_t          *m3ap_mce_ref 			= NULL;
  mbms_description_t						  *mbms_ref     			= NULL;
  mme_mbms_m3ap_id_t 						   mme_mbms_m3ap_id 	= INVALID_MME_MBMS_M3AP_ID;
  /*
   * NULL reference...
   */
  if (*mbms_ref_pp == NULL)
    return;
  mbms_ref = (mbms_description_t*)(*mbms_ref_pp);
  mme_mbms_m3ap_id = mbms_ref->mme_mbms_m3ap_id;
  /** Stop MBMS Action timer,if running. */
  if (mbms_ref->mxap_action_timer.id != MxAP_TIMER_INACTIVE_ID) {
    if (timer_remove (mbms_ref->mxap_action_timer.id, NULL)) {
      OAILOG_ERROR (LOG_M3AP, "Failed to stop m3ap mbms context action timer for MBMS id  " MME_MBMS_M3AP_ID_FMT " \n",
      		mbms_ref->mme_mbms_m3ap_id);
    }
    mbms_ref->mxap_action_timer.id = MxAP_TIMER_INACTIVE_ID;
  }
  /**
   * Decrement from the MCEs the number of the MBMS services. Eventually trigger an M3AP MCE removal, depending on the MCE state.
   * The map of the MBMS Service itself will not be altered. Destroyed afterwards.
   */
  void * no_return = NULL;
  hashtable_uint64_ts_apply_callback_on_elements((hash_table_uint64_ts_t * const)&mbms_ref->g_m3ap_assoc_id2mme_mce_id_coll,
		  m3ap_clear_mce_mbms_stats_cb, (void*)NULL, (void**)&no_return);
  if (hashtable_uint64_ts_destroy(&mbms_ref->g_m3ap_assoc_id2mme_mce_id_coll) != HASH_TABLE_OK) {
    OAILOG_ERROR(LOG_M3AP, "An error occurred while destroying sctp_assoc/mce_mbms_id hash table. \n");
    DevAssert(0);
  }
  /** Remove the MBMS service. */
  free_wrapper(mbms_ref_pp);
}

//------------------------------------------------------------------------------
static
bool m3ap_mce_compare_by_mbms_sai_cb (__attribute__((unused)) const hash_key_t keyP,
                                    void * const elementP,
                                    void * parameterP, void **resultP)
{
  const mbms_service_area_id_t * const mbms_sai_p  	= (const mbms_service_area_id_t *const)parameterP;
  m3ap_mce_description_t       * m3ap_mce_ref  	    = (m3ap_mce_description_t*)elementP;
  hashtable_element_array_t    * ea_p								= (hashtable_element_array_t*)*resultP;
  for(int i = 0; i < m3ap_mce_ref->mbms_sa_list.num_service_area; i++) {
    if (*mbms_sai_p == m3ap_mce_ref->mbms_sa_list.serviceArea[i]) {
    	ea_p->elements[ea_p->num_elements++] = (void*)m3ap_mce_ref;
      return false;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
static
bool m3ap_mce_compare_by_local_mbms_area_cb (__attribute__((unused)) const hash_key_t keyP,
                                    void * const elementP,
                                    void * parameterP, void **resultP)
{
  const uint8_t 							 * const local_mbms_area_p  = (const uint8_t *const)parameterP;
  m3ap_mce_description_t       * m3ap_mce_ref  	    			= (m3ap_mce_description_t*)elementP;
  hashtable_element_array_t    * ea_p											= (hashtable_element_array_t*)*resultP;
  if (*local_mbms_area_p == m3ap_mce_ref->local_mbms_area) {
  		/** Just return true. */
    	ea_p->elements[ea_p->num_elements++] = (void*)m3ap_mce_ref;
    	return false;
  }
  return false;
}

//------------------------------------------------------------------------------
static
bool m3ap_mce_compare_by_mbms_sai_NACK_cb (__attribute__((unused)) const hash_key_t keyP,
                                    void * const elementP,
                                    void * parameterP, void **resultP)
{
  const mbms_service_area_id_t * const mbms_sai_p  = (const mbms_service_area_id_t *const)parameterP;
  m3ap_mce_description_t       * m3ap_mce_ref  	    = (m3ap_mce_description_t*)elementP;
  hashtable_element_array_t    * ea_p								= (hashtable_element_array_t*)*resultP;
  for(int i = 0; i < m3ap_mce_ref->mbms_sa_list.num_service_area; i++) {
    if (*mbms_sai_p == m3ap_mce_ref->mbms_sa_list.serviceArea[i]) {
      return false;
    }
  }
  /** Found an M3AP MCE, where none of the MBMS Service Area Ids match the given key. */
	ea_p->elements[ea_p->num_elements++] = (void*)m3ap_mce_ref;
  return false;
}

//------------------------------------------------------------------------------
static bool m3ap_mbms_compare_by_tmgi_cb (__attribute__((unused)) const hash_key_t keyP,
                                      void * const elementP, void * parameterP, void **resultP)
{
  mme_mbms_m3ap_id_t                      * mme_mbms_m3ap_id_p = (mme_mbms_m3ap_id_t*)parameterP;
  mbms_description_t                       *mbms_ref           = (mbms_description_t*)elementP;
  if ( *mme_mbms_m3ap_id_p == mbms_ref->mme_mbms_m3ap_id ) {
    *resultP = elementP;
    OAILOG_TRACE(LOG_M3AP, "Found mbms_ref %p mme_mbms_m3ap_id " MME_MBMS_M3AP_ID_FMT "\n", mbms_ref, mbms_ref->mme_mbms_m3ap_id);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
static bool m3ap_mbms_compare_by_mme_mbms_id_cb (__attribute__((unused)) const hash_key_t keyP,
                                      void * const elementP, void * parameterP, void **resultP)
{
  mme_mbms_m3ap_id_t                      * mme_mbms_m3ap_id_p = (mme_mbms_m3ap_id_t*)parameterP;
  mbms_description_t                       *mbms_ref           = (mbms_description_t*)elementP;
  if ( *mme_mbms_m3ap_id_p == mbms_ref->mme_mbms_m3ap_id ) {
    *resultP = elementP;
    OAILOG_TRACE(LOG_M3AP, "Found mbms_ref %p mme_mbms_m3ap_id " MME_MBMS_M3AP_ID_FMT "\n", mbms_ref, mbms_ref->mme_mbms_m3ap_id);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void                                   *
m3ap_mce_thread (
  __attribute__((unused)) void *args)
{
  itti_mark_task_ready (TASK_M3AP);
//  OAILOG_START_USE ();
//  MSC_START_USE ();

  while (1) {
    MessageDef                             *received_message_p = NULL;
    MessagesIds                             message_id = MESSAGES_ID_MAX;
    /*
     * Trying to fetch a message from the message queue.
     * * * * If the queue is empty, this function will block till a
     * * * * message is sent to the task.
     */
    itti_receive_msg (TASK_M3AP, &received_message_p);
    DevAssert(received_message_p != NULL);

    switch (ITTI_MSG_ID (received_message_p)) {
    case ACTIVATE_MESSAGE:{
    	if (m3ap_send_init_sctp () < 0) {
    		OAILOG_CRITICAL (LOG_M3AP, "Error while sending SCTP_INIT_MSG to SCTP\n");
    	}
    }
    break;

    case MESSAGE_TEST:{
    	OAI_FPRINTF_INFO("TASK_M3AP received MESSAGE_TEST\n");
    }
    break;

    // MBMS session messages from MCE_APP task (M3 --> should trigger MBMS service for all MCE in the service area - not MCE specific).
    case M3AP_MBMS_SESSION_START_REQUEST:{
    	m3ap_handle_mbms_session_start_request (&M3AP_MBMS_SESSION_START_REQUEST (received_message_p));
    }
    break;

    case M3AP_MBMS_SESSION_STOP_REQUEST:{
    	m3ap_handle_mbms_session_stop_request (&M3AP_MBMS_SESSION_STOP_REQUEST (received_message_p).tmgi,
    			&M3AP_MBMS_SESSION_STOP_REQUEST (received_message_p).mbms_service_area_id,
				&M3AP_MBMS_SESSION_STOP_REQUEST (received_message_p).inform_mces);
    }
    break;

    case M3AP_MBMS_SESSION_UPDATE_REQUEST:{
    	m3ap_handle_mbms_session_update_request (&M3AP_MBMS_SESSION_UPDATE_REQUEST (received_message_p));
    }
    break;
    // ToDo: Leave the stats collection in the M3AP layer for now.

    case M3AP_MCE_SETUP_RESPONSE:{
    	/*
    	 * New message received from MCE_APP task.
    	 */
    	m3ap_handle_m3ap_mce_setup_res(&M3AP_MCE_SETUP_RESPONSE(received_message_p));
    }
    break;

    // From SCTP layer, notifies M3AP of disconnection of a peer (MCE).
    case SCTP_CLOSE_ASSOCIATION:{
    	m3ap_handle_sctp_disconnection(SCTP_CLOSE_ASSOCIATION (received_message_p).assoc_id);
    }
    break;

    // From SCTP
    case SCTP_DATA_IND:{
    	/*
    	 * New message received from SCTP layer.
    	 * Decode and handle it.
    	 */
    	M3AP_M3AP_PDU_t                            pdu = {0};

    	/*
    	 * Invoke M3AP message decoder
    	 */
    	if (m3ap_mme_decode_pdu (&pdu, SCTP_DATA_IND (received_message_p).payload) < 0) {
    		// TODO: Notify MCE of failure with right cause
    		OAILOG_ERROR (LOG_M3AP, "Failed to decode new buffer\n");
    	} else {
    		m3ap_mme_handle_message (SCTP_DATA_IND (received_message_p).assoc_id, SCTP_DATA_IND (received_message_p).stream, &pdu);
    	}

    	/*
    	 * Free received PDU array
    	 */
    	bdestroy_wrapper (&SCTP_DATA_IND (received_message_p).payload);
    }
    break;

    case M3AP_MCE_INITIATED_RESET_ACK:{
    	m3ap_handle_mce_initiated_reset_ack (&M3AP_MCE_INITIATED_RESET_ACK (received_message_p));
    }
    break;

    case TIMER_HAS_EXPIRED:{
    	mbms_description_t                       *mbms_ref_p = NULL;
    	if (received_message_p->ittiMsg.timer_has_expired.arg != NULL) {
    	  mme_mbms_m3ap_id_t mme_mbms_m3ap_id = (mme_mbms_m3ap_id_t) received_message_p->ittiMsg.timer_has_expired.arg;
    	  /** Check if the MBMS still exists. */
    	  mbms_ref_p = m3ap_is_mbms_mme_m3ap_id_in_list(mme_mbms_m3ap_id);
    	  if (!mbms_ref_p) {
    	    OAILOG_WARNING (LOG_M3AP, "Timer with id 0x%lx expired but no associated MBMS context!\n", received_message_p->ittiMsg.timer_has_expired.timer_id);
    	    break;
    	  }
    	  OAILOG_WARNING (LOG_M3AP, "Processing expired timer with id 0x%lx for MBMS M3AP Id "MME_MBMS_M3AP_ID_FMT " with m3ap_mbms_action_timer_id 0x%lx !\n",
   			  received_message_p->ittiMsg.timer_has_expired.timer_id, mbms_ref_p->mme_mbms_m3ap_id, mbms_ref_p->mxap_action_timer.id);
    	  m3ap_mme_handle_mbms_action_timer_expiry (mbms_ref_p);
    	}
    	/* TODO - Commenting out below function as it is not used as of now.
    	 * Need to handle it when we support other timers in M3AP
    	 */
    }
    break;

    // From SCTP layer, notifies M3AP of connection of a peer (MCE).
    case SCTP_NEW_ASSOCIATION:{
    	m3ap_handle_new_association (&received_message_p->ittiMsg.sctp_new_peer);
    }
    break;

    case TERMINATE_MESSAGE:{
    	m3ap_mme_exit();
    	itti_free_msg_content(received_message_p);
    	itti_free (ITTI_MSG_ORIGIN_ID (received_message_p), received_message_p);
    	OAI_FPRINTF_INFO("TASK_M3AP terminated\n");
    	itti_exit_task ();
    }
    break;

    default:{
    	OAILOG_ERROR (LOG_M3AP, "Unknown message ID %d:%s\n", ITTI_MSG_ID (received_message_p), ITTI_MSG_NAME (received_message_p));
    }
          break;
    }

    itti_free_msg_content(received_message_p);
    itti_free (ITTI_MSG_ORIGIN_ID (received_message_p), received_message_p);
    received_message_p = NULL;
  }

  return NULL;
}

//------------------------------------------------------------------------------
int
m3ap_mme_init(void)
{
  OAILOG_DEBUG (LOG_M3AP, "Initializing M3AP interface\n");

  if (get_asn1c_environment_version () < ASN1_MINIMUM_VERSION) {
    OAILOG_ERROR (LOG_M3AP, "ASN1C version %d found, expecting at least %d\n", get_asn1c_environment_version (), ASN1_MINIMUM_VERSION);
    return RETURNerror;
  } else {
    OAILOG_DEBUG (LOG_M3AP, "ASN1C version %d\n", get_asn1c_environment_version ());
  }

  OAILOG_DEBUG (LOG_M3AP, "M3AP Release v%s\n", M3AP_VERSION);

  /** Free the M3AP MCE list. */
  bstring bs1 = bfromcstr("m3ap_mce_coll");
  hash_table_ts_t* h = hashtable_ts_init (&g_m3ap_mce_coll, mce_config.max_m3_mces, NULL, m3ap_remove_mce, bs1); /**< Use a better removal handler. */
  bdestroy_wrapper (&bs1);
  if (!h) return RETURNerror;

  /** Free all MBMS Services and clear the association list. */
  bs1 = bfromcstr("m3ap_MBMS_coll");
  h = hashtable_ts_init (&g_m3ap_mbms_coll, mce_config.max_mbms_services, NULL, m3ap_remove_mbms, bs1); /**< Use a better removal handler. */
  bdestroy_wrapper (&bs1);
  if (!h) return RETURNerror;

  if (itti_create_task (TASK_M3AP, &m3ap_mce_thread, NULL) < 0) {
    OAILOG_ERROR (LOG_M3AP, "Error while creating M3AP task\n");
    return RETURNerror;
  }
  return RETURNok;
}

//------------------------------------------------------------------------------
void m3ap_mme_exit (void)
{
  OAILOG_DEBUG (LOG_M3AP, "Cleaning M3AP\n");
  if (hashtable_ts_destroy(&g_m3ap_mbms_coll) != HASH_TABLE_OK) {
    OAILOG_ERROR(LOG_M3AP, "An error occurred while destroying MBMS Service hash table. \n");
  }

  if (hashtable_ts_destroy(&g_m3ap_mce_coll) != HASH_TABLE_OK) {
    OAILOG_ERROR(LOG_M3AP, "An error occurred while destroying MCE hash table. \n");
  }
  OAILOG_DEBUG (LOG_M3AP, "Cleaning M3AP: DONE\n");
}

//------------------------------------------------------------------------------
void
m3ap_dump_mce_list (
  void)
{
  hashtable_ts_apply_callback_on_elements(&g_m3ap_mce_coll, m3ap_dump_mce_hash_cb, NULL, NULL);
}

//------------------------------------------------------------------------------
bool m3ap_dump_mce_hash_cb (__attribute__((unused))const hash_key_t keyP,
               void * const mce_void,
               void __attribute__((unused)) *unused_parameterP,
               void __attribute__((unused)) **unused_resultP)
{
  const m3ap_mce_description_t * const mce_ref = (const m3ap_mce_description_t *)mce_void;
  if (mce_ref == NULL) {
    return false;
  }
  m3ap_dump_mce(mce_ref);
  return false;
}

//------------------------------------------------------------------------------
void
m3ap_dump_mce(
  const m3ap_mce_description_t * const m3ap_mce_ref)
{
#  ifdef M3AP_DEBUG_LIST
  //Reset indentation
  indent = 0;

  if (m3ap_mce_ref == NULL) {
    return;
  }

  m3AP_mCE_LIST_OUT ("");
  m3AP_mCE_LIST_OUT ("MCE name:           %s", m3ap_mce_ref->m3ap_mce_name == NULL ? "not present" : m3ap_mce_ref->m3ap_mce_name);
  m3AP_mCE_LIST_OUT ("MCE Global ID:      %07x", m3ap_mce_ref->m3ap_mce_global_id);
  m3AP_mCE_LIST_OUT ("SCTP assoc id:      %d", m3ap_mce_ref->sctp_assoc_id);
  m3AP_mCE_LIST_OUT ("SCTP instreams:     %d", m3ap_mce_ref->instreams);
  m3AP_mCE_LIST_OUT ("SCTP outstreams:    %d", m3ap_mce_ref->outstreams);
  m3AP_mCE_LIST_OUT ("MBMS active on MCE: %d", m3ap_mce_ref->nb_mbms_associated);
  m3AP_mCE_LIST_OUT ("");
#  else
  m3ap_dump_mbms (NULL);
#  endif
}

//------------------------------------------------------------------------------
bool m3ap_dump_mbms_hash_cb (__attribute__((unused)) const hash_key_t keyP,
               void * const mbms_void,
               void __attribute__((unused)) *unused_parameterP,
               void __attribute__((unused)) **unused_resultP)
{
  mbms_description_t * mbms_ref = (mbms_description_t *)mbms_void;
  if (mbms_ref == NULL) {
    return false;
  }
  m3ap_dump_mbms(mbms_ref);
  return false;
}

//------------------------------------------------------------------------------
void
m3ap_dump_mbms (const mbms_description_t * const mbms_ref)
{
#  ifdef M3AP_DEBUG_LIST

  if (mbms_ref == NULL)
    return;

  MBMS_LIST_OUT ("MCE MBMS m3ap id:   0x%06x", mbms_ref->mce_mbms_m3ap_id);
  MBMS_LIST_OUT ("MME MBMS m3ap id:   0x%08x", mbms_ref->mme_mbms_m3ap_id);
  MBMS_LIST_OUT ("SCTP stream recv: 0x%04x", mbms_ref->sctp_stream_recv);
  MBMS_LIST_OUT ("SCTP stream send: 0x%04x", mbms_ref->sctp_stream_send);
# endif
}

//------------------------------------------------------------------------------
bool m3ap_mme_compare_by_m3ap_mce_global_id_cb (__attribute__((unused)) const hash_key_t keyP,
                                    void * const elementP,
                                    void * parameterP, void **resultP)
{
  const uint32_t                  * const m3ap_mce_id_p = (const uint32_t*const)parameterP;
  m3ap_mce_description_t          * m3ap_mce_ref  = (m3ap_mce_description_t*)elementP;
  if (*m3ap_mce_id_p == m3ap_mce_ref->m3ap_mce_id) {
    *resultP = elementP;
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
m3ap_mce_description_t                      *
m3ap_is_mce_id_in_list (
  const uint16_t m3ap_mce_id)
{
  m3ap_mce_description_t                 *m3ap_mce_ref = NULL;
  uint32_t                               *m3ap_mce_id_p  = (uint32_t*)&m3ap_mce_id;
  hashtable_ts_apply_callback_on_elements((hash_table_ts_t * const)&g_m3ap_mce_coll, m3ap_mme_compare_by_m3ap_mce_global_id_cb, (void *)m3ap_mce_id_p, (void**)&m3ap_mce_ref);
  return m3ap_mce_ref;
}

//------------------------------------------------------------------------------
void m3ap_is_mbms_area_list (
  const uint8_t local_mbms_area,
  int *num_m3ap_mces,
  m3ap_mce_description_t ** m3ap_mces)
{
  m3ap_mce_description_t    *m3ap_mce_ref 			 = NULL;
  /** Collect all M3AP MCEs for the given MBSFN Area Id. */
  hashtable_element_array_t  ea, *ea_p = &ea;
  memset(&ea, 0, sizeof(hashtable_element_array_t));
  ea.elements = m3ap_mces;
  hashtable_ts_apply_list_callback_on_elements((hash_table_ts_t * const)&g_m3ap_mce_coll,
  		m3ap_mce_compare_by_local_mbms_area_cb, (void *)&local_mbms_area, &ea_p);
  OAILOG_DEBUG(LOG_M3AP, "Found (%d) matching m3ap_MCE references based on the received local MBMS area (%d). \n", ea.num_elements, local_mbms_area);
  *num_m3ap_mces = ea.num_elements;
}

//------------------------------------------------------------------------------
void m3ap_is_mbms_sai_in_list (
  const mbms_service_area_id_t mbms_sai,
  int *num_m3ap_mces,
	const m3ap_mce_description_t ** m3ap_mces)
{
  m3ap_mce_description_t                 *m3ap_mce_ref 	= NULL;
  mbms_service_area_id_t                 *mbms_sai_p  	= (mbms_service_area_id_t*)&mbms_sai;

  /** Collect all M3AP MCEs for the given MBMS Service Area Id. */
  hashtable_element_array_t              ea, *ea_p = NULL;
  memset(&ea, 0, sizeof(hashtable_element_array_t));
  ea.elements = m3ap_mces;
  ea_p = &ea;
  hashtable_ts_apply_list_callback_on_elements((hash_table_ts_t * const)&g_m3ap_mce_coll, m3ap_mce_compare_by_mbms_sai_cb, (void *)mbms_sai_p, &ea_p);
  OAILOG_DEBUG(LOG_M3AP, "Found %d matching m3ap_mce references based on the received MBMS SAI" MBMS_SERVICE_AREA_ID_FMT". \n", ea.num_elements, mbms_sai);
  *num_m3ap_mces = ea.num_elements;
}

//------------------------------------------------------------------------------
void m3ap_is_mbms_sai_not_in_list (
  const mbms_service_area_id_t mbms_sai,
  int *num_m3ap_mces,
  m3ap_mce_description_t ** m3ap_mces)
{
  m3ap_mce_description_t                 *m3ap_mce_ref 	= NULL;
  mbms_service_area_id_t                 *mbms_sai_p  	= (mbms_service_area_id_t*)&mbms_sai;

  /** Collect all M3AP MCEs for the given MBMS Service Area Id. */
  hashtable_element_array_t              ea, *ea_p = NULL;
  memset(&ea, 0, sizeof(hashtable_element_array_t));
  ea.elements = m3ap_mces;
  ea_p = &ea;
  hashtable_ts_apply_list_callback_on_elements((hash_table_ts_t * const)&g_m3ap_mce_coll, m3ap_mce_compare_by_mbms_sai_NACK_cb, (void *)mbms_sai_p, &ea_p);
  OAILOG_DEBUG(LOG_M3AP, "Found %d unmatching m3ap_mce references based on the received MBMS SAI" MBMS_SERVICE_AREA_ID_FMT". \n", ea.num_elements, mbms_sai);
  *num_m3ap_mces = ea.num_elements;
}

//------------------------------------------------------------------------------
m3ap_mce_description_t                      *
m3ap_is_mce_assoc_id_in_list (
  const sctp_assoc_id_t sctp_assoc_id)
{
  m3ap_mce_description_t                      *mce_ref = NULL;
  hashtable_ts_get(&g_m3ap_mce_coll, (const hash_key_t)sctp_assoc_id, (void**)&mce_ref);
  return mce_ref;
}

//------------------------------------------------------------------------------
mbms_description_t                       *
m3ap_is_mbms_mme_m3ap_id_in_list (
  const mme_mbms_m3ap_id_t mme_mbms_m3ap_id)
{
  mbms_description_t                       *mbms_ref = NULL;
  mme_mbms_m3ap_id_t                       *mme_mbms_m3ap_id_p = (mme_mbms_m3ap_id_t*)&mme_mbms_m3ap_id;

  hashtable_ts_apply_callback_on_elements((hash_table_ts_t * const)&g_m3ap_mbms_coll, m3ap_mbms_compare_by_mme_mbms_id_cb, (void*)mme_mbms_m3ap_id_p, (void**)&mbms_ref);
  if (mbms_ref) {
    OAILOG_TRACE(LOG_M3AP, "Found mbms_ref %p mme_mbms_m3ap_id " MME_MBMS_M3AP_ID_FMT "\n", mbms_ref, mbms_ref->mme_mbms_m3ap_id);
  }
  return mbms_ref;
}

//------------------------------------------------------------------------------
void
m3ap_mce_full_reset (
  const void *m3ap_mce_ref_p)
{
  DevAssert(m3ap_mce_ref_p);

  m3ap_mce_description_t                   *m3ap_mce_ref = (m3ap_mce_description_t*)m3ap_mce_ref_p;
  void * mbms_ref							= NULL;

  hashtable_ts_apply_callback_on_elements((hash_table_ts_t * const)&g_m3ap_mbms_coll,
	m3ap_mce_mbms_remove_sctp_assoc_id_cb, (void*)&m3ap_mce_ref->sctp_assoc_id, (void**)&mbms_ref);
  m3ap_mce_ref->nb_mbms_associated = 0;
}

//------------------------------------------------------------------------------
mbms_description_t                       *
m3ap_is_mbms_tmgi_in_list (
  const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_sai)
{
  mbms_description_t                     *mbms_ref = NULL;
  mxap_tmgi_t						   	  m3ap_tmgi = {.tmgi = *tmgi, .mbms_service_area_id_t = mbms_sai};
  mxap_tmgi_t				 	 		 	 *m3ap_tmgi_p = &m3ap_tmgi;

  hashtable_ts_apply_callback_on_elements((hash_table_ts_t * const)&g_m3ap_mbms_coll, m3ap_mbms_compare_by_tmgi_cb, (void*)m3ap_tmgi_p, (void**)&mbms_ref);
  if (mbms_ref) {
    OAILOG_TRACE(LOG_M3AP, "Found mbms_ref %p for TMGI " TMGI_FMT " and MBMS-SAI " MBMS_SERVICE_AREA_ID_FMT ". \n", mbms_ref, TMGI_ARG(tmgi), mbms_sai);
  }
  return mbms_ref;
}

//------------------------------------------------------------------------------
m3ap_mce_description_t *m3ap_new_mce (void)
{
  m3ap_mce_description_t                      *m3ap_mce_ref = NULL;

  m3ap_mce_ref = calloc (1, sizeof (m3ap_mce_description_t));
  /*
   * Something bad happened during malloc...
   * * * * May be we are running out of memory.
   * * * * TODO: Notify MCE with a cause like Hardware Failure.
   */
  DevAssert (m3ap_mce_ref != NULL);
  nb_m3ap_mce_associated++;
  /** No table for MBMS. */
  return m3ap_mce_ref;
}

//------------------------------------------------------------------------------
static mme_mbms_m3ap_id_t generate_new_mme_mbms_m3ap_id(void)
{
  mme_mbms_m3ap_id_t tmp = INVALID_MME_MBMS_M3AP_ID;
  tmp = __sync_fetch_and_add (&mme_mbms_m3ap_id_generator, 1);
  return tmp;
}

//------------------------------------------------------------------------------
mbms_description_t                       *
m3ap_new_mbms (
  const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_sai)
{
  m3ap_mce_description_t                   *m3ap_mce_ref = NULL;
  mbms_description_t                       *mbms_ref = NULL;
  mme_mbms_m3ap_id_t												mme_mbms_m3ap_id = INVALID_MME_MBMS_M3AP_ID;

  /** Try to generate a new MBMS M3AP Id. */
  mme_mbms_m3ap_id = generate_new_mme_mbms_m3ap_id();
  if(mme_mbms_m3ap_id == INVALID_MME_MBMS_M3AP_ID)
  	return NULL;

  /** Assert that it is not reused. */
  DevAssert(!m3ap_is_mbms_mme_m3ap_id_in_list(mme_mbms_m3ap_id));

  /** No MCE reference needed at this stage. */
  mbms_ref = calloc (1, sizeof (mbms_description_t));
  /*
   * Something bad happened during malloc...
   * * * * May be we are running out of memory.
   * * * * TODO: Notify MCE with a cause like Hardware Failure.
   */
  DevAssert (mbms_ref != NULL);
  mbms_ref->mme_mbms_m3ap_id = mme_mbms_m3ap_id;
  memcpy((void*)&mbms_ref->tmgi, (void*)tmgi, sizeof(tmgi_t));
  mbms_ref->mbms_service_area_id = mbms_sai;  /**< Only supporting a single MBMS Service Area ID. */
  /**
   * Create and initialize the SCTP MCE MBMS M3AP Id map.
   */
  bstring bs2 = bfromcstr("m3ap_assoc_id2mme_mbms_id_coll");
  hash_table_uint64_ts_t* h = hashtable_uint64_ts_init (&mbms_ref->g_m3ap_assoc_id2mme_mce_id_coll, mce_config.max_m3_mces, NULL, bs2);
  bdestroy_wrapper (&bs2);
  if(!h){
  	free_wrapper(&mbms_ref);
  	return NULL;
  }
  /** No Deadlocks should occur, since we check the nb_mbms services in the MCEs above,
   * when removing keys from the list. */
  DevAssert(pthread_mutex_init(&mbms_ref->g_m3ap_assoc_id2mme_mce_id_coll.mutex, NULL) == 0);
  hashtable_rc_t  hash_rc = hashtable_ts_insert (&g_m3ap_mce_coll, (const hash_key_t)mme_mbms_m3ap_id, (void *)mbms_ref);
  DevAssert (HASH_TABLE_OK == hash_rc); /**< Else we need an extra method to avoid a leak. This should not happens, since we check above. */
  return mbms_ref;
}
//
////------------------------------------------------------------------------------
//void
//m3ap_set_embms_cfg_item (m3ap_mce_description_t * const m3ap_mce_ref,
//	M3AP_MBMS_Service_Area_ID_List_t * mbms_service_areas)
//{
//  uint16_t                                mbms_sa_value = 0;
//  /**
//   * Create a new MBMS Service Area item with the MBMS Service Areas matching the MME configuration.
//   */
//  mce_config_read_lock (&mce_config);
//  for (int j = 0; j < mbms_service_areas->list.count; j++) {
//		M3AP_MBMS_Service_Area_t * mbms_sa = &mbms_service_areas->list.array[j];
//		OCTET_STRING_TO_MBMS_SA (mbms_sa, mbms_sa_value);
//		 /** Check if it is a global MBMS SAI. */
//		if(mbms_sa_value <= mce_config.mbms_global_service_area_types) {
//			/** Global MBMS Service Area Id received. */
//			OAILOG_INFO(LOG_MCE_APP, "Found a matching global MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT ". \n", mbms_sa_value);
//			m3ap_mce_ref->mbms_sa_list.serviceArea[m3ap_mce_ref->mbms_sa_list.num_service_area] = mbms_sa_value;
//			m3ap_mce_ref->mbms_sa_list.num_service_area++;
//			/** No need to check other values, directly check new config value. */
//			break;
//		}
//		/** Check if it is in bounds for the local service areas. */
//		int val = mbms_sa_value - mce_config.mbms_global_service_area_types + 1;
//		int local_area = val / mce_config.mbms_local_service_area_types;
//		int local_area_type = val % mce_config.mbms_local_service_area_types;
//		if(local_area < mce_config.mbms_local_service_areas){
//			OAILOG_INFO(LOG_MCE_APP, "Found a valid MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT ". \n", mbms_sa_value);
//			m3ap_mce_ref->mbms_sa_list.serviceArea[m3ap_mce_ref->mbms_sa_list.num_service_area] = mbms_sa_value;
//			m3ap_mce_ref->mbms_sa_list.num_service_area++;
//			break;
//		}
//  }
//  mce_config_unlock (&mce_config);
//}
