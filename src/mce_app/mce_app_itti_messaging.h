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

/*! \file mce_app_itti_messaging.h
  \brief
  \author Dincer Beken
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#include "mce_app_mbms_service_context.h"

#ifndef FILE_MCE_APP_ITTI_MESSAGING_SEEN
#define FILE_MCE_APP_ITTI_MESSAGING_SEEN

int mce_app_remove_sm_tunnel_endpoint(teid_t local_teid, struct sockaddr *peer_ip);

/** MBMS Session Start Request. */
void mce_app_itti_sm_mbms_session_start_response(teid_t mme_sm_teid, teid_t mbms_sm_teid, struct sockaddr *mbms_ip_address, void *trxn,  gtpv2c_cause_value_t gtpv2cCause);

/** MBMS Session Update Request. */
void mce_app_itti_sm_mbms_session_update_response(teid_t mme_sm_teid, teid_t mbms_sm_teid, struct sockaddr *mbms_ip_address, void *trxn,  gtpv2c_cause_value_t gtpv2cCause);

/** MBMS Session Stop Request. */
void mce_app_itti_sm_mbms_session_stop_response(teid_t mme_sm_teid, teid_t mbms_sm_teid, struct sockaddr *mbms_ip_address, void *trxn,  gtpv2c_cause_value_t gtpv2cCause);

/** M2AP Session Start Request. */
void mce_app_itti_m2ap_mbms_session_start_request(tmgi_t * tmgi, mbms_service_area_id_t mbms_service_area_id,
  bearer_qos_t * mbms_bearer_qos, mbms_ip_multicast_distribution_t * mbms_ip_mc_dist, const uint32_t abs_start_time_sec, const uint32_t abs_start_time_usec);

/** M2AP Session Update Request. */
void mce_app_itti_m2ap_mbms_session_update_request(tmgi_t * tmgi, const mbms_service_area_id_t new_mbms_service_area_id, const mbms_service_area_id_t old_mbms_service_area_id,
  bearer_qos_t * mbms_bearer_qos, mbms_ip_multicast_distribution_t * mbms_ip_mc_dist, const uint32_t abs_update_time_sec, const uint32_t abs_update_time_usec);

/** M2AP Session Stop Request. */
void mce_app_itti_m2ap_mbms_session_stop_request(tmgi_t * tmgi, mbms_service_area_id_t mbms_sa_id, const bool inform_enbs);

/** M2AP eNB Response. */
void mce_app_itti_m2ap_enb_setup_response(mbsfn_areas_t * mbsfn_areas_p, uint8_t local_mbms_area, uint32_t m2ap_enb_id, sctp_assoc_id_t assoc_id);

/** M3AP MBMS Scheduling Information */
void mce_app_itti_m3ap_send_mbms_scheduling_info(const mbsfn_areas_t* const mbsfn_areas_p, const uint8_t max_mbms_areas, const long mcch_rep_abs_rf);

/**
 * M3 PART (playing MME)
 */
/** M3AP Session Start Request. */
void mce_app_itti_m3ap_mbms_session_start_request(tmgi_t * tmgi, mbms_service_area_id_t mbms_service_area_id,
  bearer_qos_t * mbms_bearer_qos, mbms_ip_multicast_distribution_t * mbms_ip_mc_dist, const uint32_t abs_start_time_sec, const uint32_t abs_start_time_usec);

/** M3AP Session Update Request. */
void mce_app_itti_m3ap_mbms_session_update_request(tmgi_t * tmgi, const mbms_service_area_id_t new_mbms_service_area_id, const mbms_service_area_id_t old_mbms_service_area_id,
  bearer_qos_t * mbms_bearer_qos, mbms_ip_multicast_distribution_t * mbms_ip_mc_dist, const uint32_t abs_update_time_sec, const uint32_t abs_update_time_usec);

/** M3AP Session Stop Request. */
void mce_app_itti_m3ap_mbms_session_stop_request(tmgi_t * tmgi, mbms_service_area_id_t mbms_sa_id, const bool inform_mces);

/** M3AP MCE Response. */
void mce_app_itti_m3ap_mce_setup_response(mbsfn_areas_t * mbsfn_areas_p, uint8_t local_mbms_area, uint32_t m3ap_global_mce_id, sctp_assoc_id_t assoc_id);

#endif /* FILE_MCE_APP_ITTI_MESSAGING_SEEN */
