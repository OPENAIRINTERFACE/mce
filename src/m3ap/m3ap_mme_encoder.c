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

/*! \file m3ap_mce_encoder.c
   \brief m3ap encode procedures for MCE
   \author Dincer BEKEN <dbeken@blackned.de>
   \date 2019
*/

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>

#include "bstrlib.h"

#include "intertask_interface.h"
#include "assertions.h"
#include "log.h"
#include "m3ap_common.h"
#include "m3ap_mme_encoder.h"
#include "mce_app_bearer_context.h"

static inline int                       m3ap_mme_encode_initiating (
  M3AP_M3AP_PDU_t * pdu,
  uint8_t ** buffer,
  uint32_t * length);
static inline int                       m3ap_mme_encode_successfull_outcome (
  M3AP_M3AP_PDU_t * pdu,
  uint8_t ** buffer,
  uint32_t * len);
static inline int                       m3ap_mme_encode_unsuccessfull_outcome (
  M3AP_M3AP_PDU_t * pdu,
  uint8_t ** buffer,
  uint32_t * len);

//------------------------------------------------------------------------------
int
m3ap_mme_encode_pdu (M3AP_M3AP_PDU_t *pdu, uint8_t **buffer, uint32_t *length)
{
  int ret = -1;
  DevAssert (pdu != NULL);
  DevAssert (buffer != NULL);
  DevAssert (length != NULL);


  switch (pdu->present) {
  case M3AP_M3AP_PDU_PR_initiatingMessage:
    ret = m3ap_mme_encode_initiating (pdu, buffer, length);
    break;

  case M3AP_M3AP_PDU_PR_successfulOutcome:
    ret = m3ap_mme_encode_successfull_outcome (pdu, buffer, length);
    break;

  case M3AP_M3AP_PDU_PR_unsuccessfulOutcome:
    ret = m3ap_mme_encode_unsuccessfull_outcome (pdu, buffer, length);
    break;

  default:
    OAILOG_NOTICE (LOG_M3AP, "Unknown message outcome (%d) or not implemented", (int)pdu->present);
    break;
  }
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_M3AP_M3AP_PDU, pdu);
  return ret;
}

//------------------------------------------------------------------------------
static inline int
m3ap_mme_encode_initiating (
  M3AP_M3AP_PDU_t *pdu,
  uint8_t ** buffer,
  uint32_t * length)
{
  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  DevAssert(pdu != NULL);

  switch(pdu->choice.initiatingMessage.procedureCode) {
  case M3AP_ProcedureCode_id_mBMSsessionStart:
  case M3AP_ProcedureCode_id_mBMSsessionStop:
  case M3AP_ProcedureCode_id_mBMSsessionUpdate:
  case M3AP_ProcedureCode_id_mCEConfigurationUpdate:
	  break;

  default:
    OAILOG_NOTICE (LOG_M3AP, "Unknown procedure ID (%d) for initiating message_p\n", (int)pdu->choice.initiatingMessage.procedureCode);
    *buffer = NULL;
    *length = 0;
    return -1;
  }

  memset(&res, 0, sizeof(res));
  res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_M3AP_M3AP_PDU, pdu);
  *buffer = res.buffer;
  *length = res.result.encoded;
  return 0;
}

//------------------------------------------------------------------------------
static inline int
m3ap_mme_encode_successfull_outcome (
  M3AP_M3AP_PDU_t * pdu,
  uint8_t ** buffer,
  uint32_t * length)
{
  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  DevAssert(pdu != NULL);

  switch (pdu->choice.successfulOutcome.procedureCode) {
  case M3AP_ProcedureCode_id_m3Setup:
  case M3AP_ProcedureCode_id_Reset:
     break;

  default:
    OAILOG_DEBUG (LOG_M3AP, "Unknown procedure ID (%d) for successfull outcome message\n", (int)pdu->choice.successfulOutcome.procedureCode);
    *buffer = NULL;
    *length = 0;
    return -1;
  }

  memset(&res, 0, sizeof(res));
  res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_M3AP_M3AP_PDU, pdu);
  *buffer = res.buffer;
  *length = res.result.encoded;
  return 0;
}

//------------------------------------------------------------------------------
static inline int
m3ap_mme_encode_unsuccessfull_outcome (
  M3AP_M3AP_PDU_t * pdu,
  uint8_t ** buffer,
  uint32_t * length)
{

  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  DevAssert(pdu != NULL);

  switch(pdu->choice.unsuccessfulOutcome.procedureCode) {
  case M3AP_ProcedureCode_id_m3Setup:
    break;

  default:
    OAILOG_DEBUG (LOG_M3AP, "Unknown procedure ID (%d) for unsuccessfull outcome message\n", (int)pdu->choice.unsuccessfulOutcome.procedureCode);
    *buffer = NULL;
    *length = 0;
    return -1;
  }

  memset(&res, 0, sizeof(res));
  res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_M3AP_M3AP_PDU, pdu);
  *buffer = res.buffer;
  *length = res.result.encoded;
  return 0;
}
