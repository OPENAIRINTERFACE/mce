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


/*! \file 3gpp_24.008_gmm_ies.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>
#include <pthread.h>

#include "bstrlib.h"

#include "log.h"
#include "common_defs.h"
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "TLVDecoder.h"
#include "TLVEncoder.h"

//******************************************************************************
// 10.5.5 GPRS mobility management information elements
//******************************************************************************

//------------------------------------------------------------------------------
// 10.5.5.6 DRX parameter
//------------------------------------------------------------------------------
int
decode_drx_parameter_ie (
  drx_parameter_t * drxparameter,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     decoded = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, DRX_PARAMETER_IE_MAX_LENGTH, len);
    CHECK_IEI_DECODER (GMM_DRX_PARAMETER_IEI, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (DRX_PARAMETER_IE_MAX_LENGTH - 1), len);
  }

  drxparameter->splitpgcyclecode = *(buffer + decoded);
  decoded++;
  drxparameter->cnspecificdrxcyclelengthcoefficientanddrxvaluefors1mode = (*(buffer + decoded) >> 4) & 0xf;
  drxparameter->splitonccch = (*(buffer + decoded) >> 3) & 0x1;
  drxparameter->nondrxtimer = *(buffer + decoded) & 0x7;
  decoded++;
  return decoded;
}

//------------------------------------------------------------------------------
int encode_drx_parameter_ie (
  drx_parameter_t * drxparameter,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  uint32_t                                encoded = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, DRX_PARAMETER_IE_MAX_LENGTH, len);
    *buffer = GMM_DRX_PARAMETER_IEI;
    encoded++;
  }else {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (DRX_PARAMETER_IE_MAX_LENGTH - 1), len);
  }

  *(buffer + encoded) = drxparameter->splitpgcyclecode;
  encoded++;
  *(buffer + encoded) = 0x00 | ((drxparameter->cnspecificdrxcyclelengthcoefficientanddrxvaluefors1mode & 0xf) << 4) | ((drxparameter->splitonccch & 0x1) << 3) | (drxparameter->nondrxtimer & 0x7);
  encoded++;
  return encoded;
}

//------------------------------------------------------------------------------
// 10.5.5.9 Identity type 2
//------------------------------------------------------------------------------
int decode_identity_type_2_ie (
    identity_type2_t * identitytype2,
  bool is_ie_present,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     decoded = 0;

  CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, IDENTITY_TYPE_2_IE_MAX_LENGTH, len);

  if (is_ie_present) {
    AssertFatal(0,"No IEI for Identity type 2");
    CHECK_IEI_DECODER ((*buffer & 0xf0), 0);
  }

  *identitytype2 = *buffer & 0x7;
  decoded++;
  return decoded;
}

//------------------------------------------------------------------------------
int encode_identity_type_2_ie (
  identity_type2_t * identitytype2,
  bool is_ie_present,
  uint8_t * buffer,
  const uint32_t len)
{
  uint8_t                                 encoded = 0;

  /*
   * Checking length and pointer
   */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, IDENTITY_TYPE_2_IE_MAX_LENGTH, len);
  *(buffer + encoded) = 0x00;
  if (is_ie_present) {
    AssertFatal(0,"No IEI for Identity type 2");
    *(buffer + encoded) |=  (0 & 0xf0);
  }
  *(buffer + encoded) |= (*identitytype2 & 0x7);
  encoded++;
  return encoded;
}
