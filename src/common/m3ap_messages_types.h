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

/*! \file m3ap_message_types.h
* \brief
* \author Dincer Beken
* \company Blackned GmbH
* \email: dbeken@blackned.de
*
*/

#ifndef FILE_M3AP_MESSAGES_TYPES_SEEN
#define FILE_M3AP_MESSAGES_TYPES_SEEN

#include "mxap_messages_types.h"

#define M3AP_MBMS_SESSION_START_REQUEST(mSGpTR)                  (mSGpTR)->ittiMsg.m3ap_mbms_session_start_req
#define M3AP_MBMS_SESSION_UPDATE_REQUEST(mSGpTR)                 (mSGpTR)->ittiMsg.m3ap_mbms_session_update_req
#define M3AP_MBMS_SESSION_STOP_REQUEST(mSGpTR)                   (mSGpTR)->ittiMsg.m3ap_mbms_session_stop_req

#define M3AP_MCE_SETUP_REQUEST(mSGpTR)		                   		 (mSGpTR)->ittiMsg.m3ap_mce_setup_req
#define M3AP_MCE_SETUP_RESPONSE(mSGpTR)   	                		 (mSGpTR)->ittiMsg.m3ap_mce_setup_res

#define M3AP_MCE_INITIATED_RESET_REQ(mSGpTR)                		 (mSGpTR)->ittiMsg.m3ap_mce_initiated_reset_req
#define M3AP_MCE_INITIATED_RESET_ACK(mSGpTR)                		 (mSGpTR)->ittiMsg.m3ap_mce_initiated_reset_ack

#define M3AP_MCE_CONFIG_UPDATE(mSGpTR)                		 			 (mSGpTR)->ittiMsg.m3ap_mce_config_update

// List of possible causes for MME generated UE context release command towards eNB
enum m3cause {
  M3AP_INVALID_CAUSE = 0,
  M3AP_NAS_NORMAL_RELEASE,
  M3AP_NAS_DETACH,
  M3AP_RADIO_EUTRAN_GENERATED_REASON,
  M3AP_IMPLICIT_CONTEXT_RELEASE,
  M3AP_INITIAL_CONTEXT_SETUP_FAILED,
  M3AP_SCTP_SHUTDOWN_OR_RESET,

  M3AP_HANDOVER_CANCELLED,
  M3AP_HANDOVER_FAILED,
  M3AP_NETWORK_ERROR,
  M3AP_SYSTEM_FAILURE,

  M3AP_INVALIDATE_NAS,  /**< Removing the NAS layer only. */

  M3AP_SUCCESS
};

typedef struct m3_sig_conn_id_s {
  mme_mbms_m3ap_id_t   mme_mbms_m3ap_id;
  mce_mbms_m3ap_id_t   mce_mbms_m3ap_id;
} m3_sig_conn_id_t;

//------------------------------------------------------------------------------
typedef struct itti_m3ap_mbms_session_start_req_s {
  tmgi_t									tmgi;
  mbms_service_area_id_t					mbms_service_area_id;
  mbms_bearer_context_to_be_created_t	    mbms_bearer_tbc;
  // todo: handle timer better --> Removal of current time should be done in M3AP layer
  uint32_t									abs_start_time_sec;  /**< Will be handled in the MCE_APP layer. */
  uint32_t									abs_start_time_usec;  /**< Will be handled in the MCE_APP layer. */
} itti_m3ap_mbms_session_start_req_t;

//------------------------------------------------------------------------------
typedef struct itti_m3ap_mbms_session_update_req_s {
  tmgi_t									tmgi;
  mbms_service_area_id_t					new_mbms_service_area_id;
  mbms_service_area_id_t					old_mbms_service_area_id;
  mbms_bearer_context_to_be_created_t	    mbms_bearer_tbc;
  uint32_t									abs_update_time_sec;  /**< Will be handled in the MCE_APP layer. */
  uint32_t									abs_update_time_usec;  /**< Will be handled in the MCE_APP layer. */
} itti_m3ap_mbms_session_update_req_t;

//------------------------------------------------------------------------------
typedef struct itti_m3ap_mbms_session_stop_req_s {
  tmgi_t 				    				tmgi;           //< MBMS Service TMGI.
  mbms_service_area_id_t		mbms_service_area_id;
  bool											inform_mces;
} itti_m3ap_mbms_session_stop_req_t;

//------------------------------------------------------------------------------
typedef struct itti_m3ap_mce_setup_req_s {
  sctp_assoc_id_t		    		sctp_assoc;
  mbms_service_area_t				mbms_service_areas;
  uint32_t									m3ap_mce_global_id;
} itti_m3ap_mce_setup_req_t;

//------------------------------------------------------------------------------
typedef struct itti_m3ap_mce_setup_res_s {
	uint32_t								  m3_mce_global_id;
	uint8_t										local_mbms_area;
  sctp_assoc_id_t		    		sctp_assoc;
  mbsfn_areas_t		  				mbsfn_areas;
} itti_m3ap_mce_setup_res_t;

//------------------------------------------------------------------------------
typedef struct itti_m3ap_mce_initiated_reset_req_s {
  uint32_t          sctp_assoc_id;
  uint16_t          sctp_stream_id;
  uint32_t          mce_id;
  mxap_reset_type_t  m3ap_reset_type;
  uint32_t          num_mbms;
  m3_sig_conn_id_t  *mbms_to_reset_list;
} itti_m3ap_mce_initiated_reset_req_t;

//------------------------------------------------------------------------------
typedef struct itti_m3ap_mce_initiated_reset_ack_s {
  uint32_t          sctp_assoc_id;
  uint16_t          sctp_stream_id;
  mxap_reset_type_t m3ap_reset_type;
  uint32_t          num_mbms;
  m3_sig_conn_id_t  *mbsm_to_reset_list;
} itti_m3ap_mce_initiated_reset_ack_t;

//------------------------------------------------------------------------------
/** M3AP MCE Configuration Update. */
typedef struct itti_m3ap_mce_config_update_s {
  mme_mbms_m3ap_id_t      mme_mbms_m3ap_id;
  mce_mbms_m3ap_id_t      mce_mbms_m3ap_id;
  sctp_assoc_id_t         assoc_id;
  uint32_t                global_mce_id;
  enum m3cause            cause;
}itti_m3ap_mce_config_update_t;

#endif /* FILE_M3AP_MESSAGES_TYPES_SEEN */
