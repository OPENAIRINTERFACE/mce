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

/*! \file m3ap_mme_handlers.h
  \brief
  \author Dincer Beken
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#ifndef FILE_M3AP_MME_HANDLERS_SEEN
#define FILE_M3AP_MME_HANDLERS_SEEN
#include "m3ap_mme.h"
#include "intertask_interface.h"
#define MAX_NUM_PARTIAL_M3_CONN_RESET 256

/** \brief Handle decoded incoming messages from SCTP
 * \param assoc_id SCTP association ID
 * \param stream Stream number
 * \param pdu The message decoded by the ASN1C decoder
 * @returns int
 **/
int m3ap_mme_handle_message(const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream,
                            M3AP_M3AP_PDU_t *pdu);

/** \brief Handle an M3 Setup request message.
 * Typically add the MCE in the list of served MEC if not present, simply reset
 * MCE association otherwise. M3SetupResponse message is sent in case of success or
 * M3SetupFailure if the MME cannot accept the configuration received.
 * \param assoc_id SCTP association ID
 * \param stream Stream number
 * \param pdu The message decoded by the ASN1C decoder
 * @returns int
 **/
int m3ap_mme_handle_m3_setup_request(const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream,
                                     M3AP_M3AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int m3ap_handle_m3ap_mce_setup_res(itti_m3ap_mce_setup_res_t * m3ap_mce_setup_res);

//------------------------------------------------------------------------------
int
m3ap_mme_handle_mbms_session_start_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M3AP_M3AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m3ap_mme_handle_mbms_session_start_failure (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M3AP_M3AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m3ap_mme_handle_mbms_session_stop_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M3AP_M3AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m3ap_mme_handle_mbms_session_update_response (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M3AP_M3AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m3ap_mme_handle_mbms_session_update_failure (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M3AP_M3AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m3ap_mme_handle_reset_request (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M3AP_M3AP_PDU_t *pdu);

//------------------------------------------------------------------------------
int
m3ap_mme_handle_error_indication (
    __attribute__((unused)) const sctp_assoc_id_t assoc_id,
    __attribute__((unused)) const sctp_stream_id_t stream,
    M3AP_M3AP_PDU_t *pdu) ;

//------------------------------------------------------------------------------
int m3ap_handle_sctp_disconnection(const sctp_assoc_id_t assoc_id);

int m3ap_handle_new_association(sctp_new_peer_t *sctp_new_peer_p);

int m3ap_mme_set_cause(M3AP_Cause_t *cause_p, const M3AP_Cause_PR cause_type, const long cause_value);

int m3ap_mme_generate_m3_setup_failure(
    const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream,
	const M3AP_Cause_PR cause_type, const long cause_value,
    const long time_to_wait);

/*** HANDLING EXPIRED TIMERS. */
//------------------------------------------------------------------------------

int m3ap_handle_mce_initiated_reset_ack (const itti_m3ap_mce_initiated_reset_ack_t * const m3ap_mce_initiated_reset_ack_p);

int m3ap_mme_handle_mce_reset (const sctp_assoc_id_t assoc_id,
                               const sctp_stream_id_t stream, M3AP_M3AP_PDU_t *message);

#endif /* FILE_M3AP_MME_HANDLERS_SEEN */
