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

/*! \file m3ap_mme.h
  \brief
  \author Dincer BEKEN
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#ifndef FILE_M3AP_MME_SEEN
#define FILE_M3AP_MME_SEEN

#if MME_CLIENT_TEST == 0
# include "intertask_interface.h"
#endif

#include "hashtable.h"
#include "m3ap_common.h"
#include "mce_app_bearer_context.h"
#include "mxap_main.h"

// Forward declarations
struct m3ap_mce_description_s;

#define M3AP_MCE_SERVICE_SCTP_STREAM_ID         0
#define MBMS_SERVICE_SCTP_STREAM_ID          	1

// The current m2 state of the MCE relating to the specific eNB.
enum mme_m3_mce_state_s {
  M3AP_INIT,          /// The sctp association has been established but m3 hasn't been setup.
  M3AP_RESETING,      /// The m3state is resetting due to an SCTP reset on the bound association.
  M3AP_READY,         ///< MME and MCE are M3 associated, MBMS contexts can be added
  M3AP_SHUTDOWN       /// The M3 state is being torn down due to sctp shutdown.
};

/* Main structure representing eNB association over m3ap
 * Generated (or updated) every time a new M3SetupRequest is received.
 */
typedef struct m3ap_mce_description_s {

  enum mme_m3_mce_state_s m3_state;         ///< State of the MCE specific M3AP association

  /** MCE related parameters **/
  /*@{*/
  char     				m3ap_mce_name[150];      ///< Printable MCE Name
  uint32_t 				m3ap_mce_id;             ///< Unique MCE ID
  /** Received MBMS SA list. */
  mbms_service_area_t   mbms_sa_list;      ///< Tracking Area Identifiers signaled by the MCE.
  /** Configured MBSFN Area Id . */
  mbsfn_area_ids_t		  mbsfn_area_ids;

  uint8_t								local_mbms_area;
  /*@}*/

  /** MBMS Services for this MCE **/
  /*@{*/
  uint32_t nb_mbms_associated;
  /*@}*/

  /** SCTP stuff **/
  /*@{*/
  sctp_assoc_id_t  sctp_assoc_id;    ///< SCTP association id on this machine
  sctp_stream_id_t instreams;        ///< Number of streams avalaible on MCE -> MME
  sctp_stream_id_t outstreams;       ///< Number of streams avalaible on MME -> MCE
  /*@}*/
} m3ap_mce_description_t;

extern uint32_t         nb_m3ap_mce_associated;

/** \brief M3AP layer top init
 * @returns -1 in case of failure
 **/
int m3ap_mme_init(void);

/** \brief M3AP layer top exit
 **/
void m3ap_mme_exit (void);

/** \brief Look for given local MBMS area in the list.
 * \param local MBMS area is not unique and used for the search in the list.
 * @returns All matched M3AP MMEs in the m3ap_mce_list.
 **/
void m3ap_is_mbms_area_list (
  const uint8_t local_mbms_area,
  int *num_m3ap_mces,
  m3ap_mce_description_t ** m3ap_mces);

/** \brief Look for given MBMS Service Area Id in the list.
 * \param MBMS Service Area Id is not unique and used for the search in the list.
 * @returns All matched M3AP MCEs in the m3ap_mce_list.
 **/
void m3ap_is_mbms_sai_in_list (
  const mbms_service_area_id_t mbms_sai,
  int *num_m3ap_mces,
	const m3ap_mce_description_t ** m3ap_mces);

/** \brief Look for given MBMS Service Area Id in the list.
 * \param tac MBMS Service Area Id is not unique and used for the search in the list.
 * @returns All M3AP MCEs in the m3ap_mce_list, which don't support the given MBMS SAI.
 **/
void m3ap_is_mbms_sai_not_in_list (
  const mbms_service_area_id_t mbms_sai,
  int *num_m3ap_mces,
  m3ap_mce_description_t ** m3ap_mces);

/** \brief Look for given MCE SCTP assoc id in the list
 * \param mce_id The unique sctp assoc id to search in list
 * @returns NULL if no MCE matchs the sctp assoc id, or reference to the MCE element in list if matches
 **/
m3ap_mce_description_t* m3ap_is_mce_assoc_id_in_list(const sctp_assoc_id_t sctp_assoc_id);

/** \brief Look for given mbms mme id in the list
 * \param mbms_mme_id The unique mbms_mme_id to search in list
 * @returns NULL if no MBMS matchs the mbms_mme_id, or reference to the mbms element in list if matches
 **/
mbms_description_t* m3ap_is_mbms_mme_m3ap_id_in_list(const mme_mbms_m3ap_id_t mbms_mme_id);

/** \brief Perform an MCE reset (cleaning SCTP associations in the MBMS Service References) without removing the M3AP MCE.
 * \param m3ap_mce_ref : MCE to reset fully
 **/
void m3ap_mce_full_reset (const void *m3ap_mce_ref_p);

/** \brief Look for given mbms via TMGI is in the list
 * \param tmgi
 * \param mbms_sai
 * @returns NULL if no MBMS matchs the mbms_mme_id, or reference to the mbms element in list if matches
 **/
mbms_description_t* m3ap_is_mbms_tmgi_in_list (
  const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_sai);

/** \brief Allocate and add to the list a new MCE descriptor
 * @returns Reference to the new MCE element in list
 **/
m3ap_mce_description_t* m3ap_new_mce(void);

/** \brief Allocate a new MBMS Service Description. Will allocate a new MME MBMS M3AP Id (16).
 * \param tmgi_t
 * \param mbms_service_are_id
 * @returns Reference to the new MBMS Service element in list
 **/
mbms_description_t* m3ap_new_mbms(const tmgi_t * const tmgi, const mbms_service_area_id_t mbms_sai);

////------------------------------------------------------------------------------
//void
//m3ap_set_embms_cfg_item (m3ap_mce_description_t * const m3ap_mce_ref,
//	M3AP_MBMS_Service_Area1_ID_List_t * mbms_service_areas);

/** \brief Dump the MCE related information.
 * hashtable callback. It is called by hashtable_ts_apply_funct_on_elements()
 * Calls m3ap_dump_mce
 **/
bool m3ap_dump_mce_hash_cb (const hash_key_t keyP,
               void * const enb_void,
               void *unused_parameterP,
               void **unused_resultP);

/** \brief Dump the MCE list
 * Calls dump_mce for each MCE in list
 **/
void m3ap_dump_mce_list(void);

/** \brief Dump MCE related information.
 * Calls dump_mce for each MBMS in list
 * \param mce_ref MCE structure reference to dump
 **/
void m3ap_dump_mce(const m3ap_mce_description_t * const m3ap_mce_ref);

/** \brief Dump the MBMS related information.
 * hashtable callback. It is called by hashtable_ts_apply_funct_on_elements()
 * Calls m3ap_dump_mbms
 **/
bool m3ap_dump_mbms_hash_cb (const hash_key_t keyP,
               void * const mbms_void,
               void *unused_parameterP,
               void **unused_resultP);

 /** \brief Dump MBMS related information.
 * \param mbms_ref MBMS structure reference to dump
 **/
void m3ap_dump_mbms(const mbms_description_t * const mbms_ref);

bool m3ap_mme_compare_by_m3ap_mce_global_id_cb (const hash_key_t keyP,
      void * const elementP, void * parameterP, void __attribute__((unused)) **unused_resultP);

m3ap_mce_description_t                      *
m3ap_is_mce_id_in_list ( const uint16_t m3ap_mce_id);

#endif /* FILE_M3AP_MME_SEEN */
