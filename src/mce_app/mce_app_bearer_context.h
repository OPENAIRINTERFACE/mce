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

/*! \file mce_app_bearer_context.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_MCE_APP_BEARER_CONTEXT_SEEN
#define FILE_MCE_APP_BEARER_CONTEXT_SEEN

//#include "esm_data.h"
#include "queue.h"

typedef uint8_t mce_app_bearer_state_t;

/*
 * @struct bearer_context_new_t
 * @brief Parameters that should be kept for an eps bearer. Used for stacked memory
 *
 * Structure of an EPS bearer
 * --------------------------
 * An EPS bearer is a logical concept which applies to the connection
 * between two endpoints (UE and PDN Gateway) with specific QoS attri-
 * butes. An EPS bearer corresponds to one Quality of Service policy
 * applied within the EPC and E-UTRAN.
 */
typedef struct bearer_context_new_s {
  // EPS Bearer ID: An EPS bearer identity uniquely identifies an EP S bearer for one UE accessing via E-UTRAN
  ebi_t                       				ebi;
  ebi_t                       				linked_ebi;

  // S-GW IP address for S1-u: IP address of the S-GW for the S1-u interfaces.
  // S-GW TEID for S1u: Tunnel Endpoint Identifier of the S-GW for the S1-u interface.
  fteid_t                      				s_gw_fteid_s1u;            // set by S11 CREATE_SESSION_RESPONSE

  // PDN GW TEID for S5/S8 (user plane): P-GW Tunnel Endpoint Identifier for the S5/S8 interface for the user plane. (Used for S-GW change only).
  // NOTE:
  // The PDN GW TEID is needed in MCE context as S-GW relocation is triggered without interaction with the source S-GW, e.g. when a TAU
  // occurs. The Target S-GW requires this Information Element, so it must be stored by the MCE.
  // PDN GW IP address for S5/S8 (user plane): P GW IP address for user plane for the S5/S8 interface for the user plane. (Used for S-GW change only).
  // NOTE:
  // The PDN GW IP address for user plane is needed in MCE context as S-GW relocation is triggered without interaction with the source S-GW,
  // e.g. when a TAU occurs. The Target S GW requires this Information Element, so it must be stored by the MCE.
  fteid_t                      				p_gw_fteid_s5_s8_up;

  // EPS bearer QoS: QCI and ARP, optionally: GBR and MBR for GBR bearer

  // extra 23.401 spec members
  pdn_cid_t                         	pdn_cx_id;

  /*
   * Two bearer states, one mce_app_bearer_state (towards SAE-GW) and one towards eNodeB (if activated in RAN).
   * todo: setting one, based on the other is possible?
   */
  mce_app_bearer_state_t            	bearer_state;     /**< Need bearer state to establish them. */
  fteid_t                           	enb_fteid_s1u;

  /* QoS for this bearer */
  bearer_qos_t                				bearer_level_qos;

  /** Add an entry field to make it part of a list (session or UE, no need to save more lists). */
  // LIST_ENTRY(bearer_context_new_s) 	entries;
  STAILQ_ENTRY (bearer_context_new_s)	entries;
}__attribute__((__packed__)) bearer_context_new_t;

/*
 * @struct bearer_context_mbms_t
 * @brief Parameters that should be kept for an eps bearer. Used for stacked memory
 *
 * Structure of an MBMS bearer for EPS
 * --------------------------
 * Has additional IP Multicast and CTEID fields. Using inheritance instead of union or creating a new field.
 */
typedef struct mbms_bearer_context_s {
  bearer_context_new_t   						eps_bearer_context;
  mbms_ip_multicast_distribution_t  mbms_ip_mc_distribution;
}__attribute__((__packed__)) mbms_bearer_context_t;

#endif
