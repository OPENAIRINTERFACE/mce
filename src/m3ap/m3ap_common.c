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


/*! \file m3ap_common.c
   \brief m3ap procedures for both MCE and MME
   \author Dincer BEKEN <dbeken@blackned.de>
   \date 2019
   \version 0.1
*/

#include "m3ap_common.h"

#include <stdint.h>

#include "dynamic_memory_check.h"
#include "log.h"

int                                     asn_debug = 0;
int                                     asn1_xer_print = 0;
void
m3ap_handle_criticality (
  M3AP_Criticality_t criticality)
{
}
