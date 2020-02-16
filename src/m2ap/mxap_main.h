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

/*! \file mxap_main.h
  \brief
  \author Dincer BEKEN
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#ifndef FILE_MxAP_MAIN_SEEN
#define FILE_MxAP_MAIN_SEEN

#if MME_CLIENT_TEST == 0
# include "intertask_interface.h"
#endif

#include "hashtable.h"
// #include "m2ap_common.h"
// #include "m3ap_common.h"
#include "mce_app_bearer_context.h"

// Forward declarations
struct m2ap_enb_description_s;
struct m3ap_mce_description_s;

#define MxAP_TIMER_INACTIVE_ID   (-1)
#define MxAP_MBMS_CONTEXT_REL_COMP_TIMER 1 // in seconds

#define M2AP_ENB_SERVICE_SCTP_STREAM_ID         0
#define M2AP_MCE_SERVICE_SCTP_STREAM_ID         0
#define MBMS_SERVICE_SCTP_STREAM_ID          	1

/* Timer structure */
struct mxap_timer_t {
  long id;           /* The timer identifier                 */
  long sec;          /* The timer interval value in seconds  */
};

/*
 * Used to search by TMGI and MBMS SAI.
 */
typedef struct mxap_tmgi_s{
  tmgi_t					tmgi;
  mbms_service_area_id_t 	mbms_service_area_id_t;
}mxap_tmgi_t;

/** Main structure representing MBMS Bearer service association over m3ap per MCE.
 * Created upon a successful
 */
typedef struct mbms_description_s {
  /** For M2 */
  mce_mbms_m2ap_id_t 			mce_mbms_m2ap_id;    ///< Unique MBMS id over MCE (24 bits wide)
  enb_mbms_m2ap_id_t 			enb_mbms_m2ap_id;    ///< Unique MBMS id over eNB (16 bits wide)
  /** List of SCTP associations pointing to the eNBs. */
  hash_table_uint64_ts_t	g_m2ap_assoc_id2mce_enb_id_coll; // key is enb_mbms_m2ap_id, key is sctp association id;

	/** For M3 */
  mme_mbms_m3ap_id_t 			mme_mbms_m3ap_id;    ///< Unique MBMS id over MME (16 bits wide)
  mce_mbms_m3ap_id_t 			mce_mbms_m3ap_id;    ///< Unique MBMS id over MCE (16 bits wide)
  /** List of SCTP associations pointing to the eNBs. */
  hash_table_uint64_ts_t	g_m3ap_assoc_id2mme_mce_id_coll; // key is mce_mbms_m3ap_id, key is sctp association id;

  /** MBMS Parameters. */
  tmgi_t					    tmgi;
  mbms_service_area_id_t		mbms_service_area_id;

  /** SCTP stream on which M3 message will be sent/received.
   *  During an MBMS M3 connection, a pair of streams is
   *  allocated and is used during all the connection.
   *  Stream 0 is reserved for non MBMS signalling.
   *  @name sctp stream identifier
   **/

  /*@{*/
//  sctp_stream_id_t sctp_stream_recv; ///< MCE -> MME stream
//  sctp_stream_id_t sctp_stream_send; ///< MME -> MCE stream
//  /*@}*/
//

  /** MBMS Bearer. */
  struct mbms_bearer_context_s		mbms_bc;

  // MBMS Action timer
  struct mxap_timer_t       			mxap_action_timer;
} mbms_description_t;

extern struct mce_config_s    *global_mce_config_p;

//------------------------------------------------------------------------------
/** \brief Dump the MBMS related information.
 * hashtable callback. It is called by hashtable_ts_apply_funct_on_elements()
 * Calls mxap_dump_mbms
 **/
bool mxap_dump_mbms_hash_cb (const hash_key_t keyP,
               void * const mbms_void,
               void *unused_parameterP,
               void **unused_resultP);

 /** \brief Dump MBMS related information.
 * \param mbms_ref MBMS structure reference to dump
 **/
void mxap_dump_mbms(const mbms_description_t * const mbms_ref);

#endif /* FILE_MxAP_MAIN_SEEN */
