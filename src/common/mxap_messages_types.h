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

/*! \file mxap_message_types.h
* \brief
* \author Dincer Beken
* \company Blackned GmbH
* \email: dbeken@blackned.de
*
*/

#ifndef FILE_MXAP_MESSAGES_TYPES_SEEN
#define FILE_MXAP_MESSAGES_TYPES_SEEN

//-----------------
typedef struct mbms_bearer_context_to_be_created_s {
  struct bearer_context_to_be_created_s 		bc_tbc;
  struct mbms_ip_multicast_distribution_s 		mbms_ip_mc_dist;
} mbms_bearer_context_to_be_created_t;

typedef enum mxap_reset_type_e {
  MxAP_RESET_ALL = 0,
  MxAP_RESET_PARTIAL
} mxap_reset_type_t;

#endif /* FILE_MXAP_MESSAGES_TYPES_SEEN */
