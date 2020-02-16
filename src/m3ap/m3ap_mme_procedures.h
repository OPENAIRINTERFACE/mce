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

/*! \file m3ap_mme_procedures.h
  \brief
  \author Dincer BEKEN
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#ifndef FILE_M3AP_MME_PROCEDURES_SEEN
#define FILE_M3AP_MME_PROCEDURES_SEEN

#include "common_defs.h"

/** \brief Handle MBMS Session Start Request from the MCE_APP.
 **/
void
m3ap_handle_mbms_session_start_request (
  const itti_m3ap_mbms_session_start_req_t * const mbms_session_start_req_pP);

/** \brief Handle MBMS Session Stop Request from the MCE_APP.
 **/
void
m3ap_handle_mbms_session_stop_request (
  const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_service_area_id, const bool inform_mces);

/** \brief Handle MBMS Session Update Request from the MCE_APP.
 **/
void
m3ap_handle_mbms_session_update_request (
  const itti_m3ap_mbms_session_update_req_t * const mbms_session_update_req_pP);

/** \brief Trigger an MBMS Session Stop Request from the MCE_APP.
 **/
int m3ap_generate_mbms_session_stop_request(mme_mbms_m3ap_id_t mme_mbms_m3ap_id, sctp_assoc_id_t sctp_assoc_id);

/** \brief Handles M3AP Timeouts.
 **/
void m3ap_mme_handle_mbms_action_timer_expiry (void *arg);

#endif /* FILE_M3AP_MME_PROCEDURES_SEEN */
