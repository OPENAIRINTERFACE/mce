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


/*! \file m3ap_mme_retransmission.h
  \brief
  \author Dincer BEKEN
  \company Blackned GmbH
*/

#ifndef FILE_M3AP_MME_RETRANSMISSION_SEEN
#define FILE_M3AP_MME_RETRANSMISSION_SEEN

#include "tree.h"

typedef struct m3ap_timer_map_s {
  long               timer_id;
  mme_mbms_m3ap_id_t mme_mbms_m3ap_id;

  RB_ENTRY(m3ap_timer_map_s) entries;
} m3ap_timer_map_t;

int m3ap_mme_timer_map_compare_id(
  const struct m3ap_timer_map_s * const p1, const struct m3ap_timer_map_s * const p2);

int m3ap_handle_timer_expiry(timer_has_expired_t *timer_has_expired);

// TODO: (amar) unused functions check with OAI.
int m3ap_timer_insert(const mme_mbms_m3ap_id_t mme_mbms_m3ap_id, const long timer_id);

int m3ap_timer_remove_mbms(const mme_mbms_m3ap_id_t mme_mbms_m3ap_id);

#endif /* FILE_M3AP_MME_RETRANSMISSION_SEEN */
