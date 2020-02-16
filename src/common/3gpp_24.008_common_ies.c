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

/*! \file 3gpp_24.008_common_ies.c
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

#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "common_defs.h"
#include "assertions.h"
#include "TLVDecoder.h"
#include "TLVEncoder.h"
#include "log.h"

//******************************************************************************
// 10.5.1 Common information elements
//******************************************************************************
//------------------------------------------------------------------------------
// 10.5.1.3 Location Area Identification
//------------------------------------------------------------------------------

int decode_location_area_identification_ie (
  location_area_identification_t * locationareaidentification,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     decoded = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, LOCATION_AREA_IDENTIFICATION_IE_MAX_LENGTH, len);
    CHECK_IEI_DECODER (C_LOCATION_AREA_IDENTIFICATION_IEI, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (LOCATION_AREA_IDENTIFICATION_IE_MAX_LENGTH - 1), len);
  }

  locationareaidentification->mccdigit2 = (*(buffer + decoded) >> 4) & 0xf;
  locationareaidentification->mccdigit1 = *(buffer + decoded) & 0xf;
  decoded++;
  locationareaidentification->mncdigit3 = (*(buffer + decoded) >> 4) & 0xf;
  locationareaidentification->mccdigit3 = *(buffer + decoded) & 0xf;
  decoded++;
  locationareaidentification->mncdigit2 = (*(buffer + decoded) >> 4) & 0xf;
  locationareaidentification->mncdigit1 = *(buffer + decoded) & 0xf;
  decoded++;
  //IES_DECODE_U16(locationareaidentification->lac, *(buffer + decoded));
  IES_DECODE_U16 (buffer, decoded, locationareaidentification->lac);
  return decoded;
}

//------------------------------------------------------------------------------
int encode_location_area_identification_ie (
  location_area_identification_t * locationareaidentification,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  uint32_t                                encoded = 0;

  // Checking IEI and pointer
  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, LOCATION_AREA_IDENTIFICATION_IE_MAX_LENGTH, len);
    *buffer = C_LOCATION_AREA_IDENTIFICATION_IEI;
    encoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (LOCATION_AREA_IDENTIFICATION_IE_MAX_LENGTH - 1), len);
  }

  *(buffer + encoded) = 0x00 | ((locationareaidentification->mccdigit2 & 0xf) << 4) | (locationareaidentification->mccdigit1 & 0xf);
  encoded++;
  *(buffer + encoded) = 0x00 | ((locationareaidentification->mncdigit3 & 0xf) << 4) | (locationareaidentification->mccdigit3 & 0xf);
  encoded++;
  *(buffer + encoded) = 0x00 | ((locationareaidentification->mncdigit2 & 0xf) << 4) | (locationareaidentification->mncdigit1 & 0xf);
  encoded++;
  IES_ENCODE_U16 (buffer, encoded, locationareaidentification->lac);
  return encoded;
}

//------------------------------------------------------------------------------
// 10.5.1.4 TMGI
//------------------------------------------------------------------------------
static int decode_tmgi_mobile_identity   (tmgi_mobile_identity_t   * tmgi,   uint8_t * buffer, const uint32_t len);
static int encode_tmgi_mobile_identity   (tmgi_mobile_identity_t   * tmgi,   uint8_t * buffer, const uint32_t len);

//------------------------------------------------------------------------------
static int decode_tmgi_mobile_identity (
  tmgi_mobile_identity_t * tmgi,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     decoded = 0;

  CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, MOBILE_IDENTITY_IE_TMGI_LENGTH, len);
  tmgi->spare = (*(buffer + decoded) >> 6) & 0x2;

  /*
   * Spare bits are coded with 0s
   */
  if (tmgi->spare != 0) {
    return (TLV_VALUE_DOESNT_MATCH);
  }

  tmgi->mbmssessionidindication = (*(buffer + decoded) >> 5) & 0x1;
  tmgi->mccmncindication = (*(buffer + decoded) >> 4) & 0x1;
  tmgi->oddeven = (*(buffer + decoded) >> 3) & 0x1;
  tmgi->typeofidentity = *(buffer + decoded) & 0x7;

  if (tmgi->typeofidentity != MOBILE_IDENTITY_TMGI) {
    return (TLV_VALUE_DOESNT_MATCH);
  }

  decoded++;
  //IES_DECODE_U24(tmgi->mbmsserviceid, *(buffer + decoded));
  IES_DECODE_U24 (buffer, decoded, tmgi->mbmsserviceid);
  tmgi->mccdigit2 = (*(buffer + decoded) >> 4) & 0xf;
  tmgi->mccdigit1 = *(buffer + decoded) & 0xf;
  decoded++;
  tmgi->mncdigit3 = (*(buffer + decoded) >> 4) & 0xf;
  tmgi->mccdigit3 = *(buffer + decoded) & 0xf;
  decoded++;
  tmgi->mncdigit2 = (*(buffer + decoded) >> 4) & 0xf;
  tmgi->mncdigit1 = *(buffer + decoded) & 0xf;
  decoded++;
  tmgi->mbmssessionid = *(buffer + decoded);
  decoded++;
  return decoded;
}

//------------------------------------------------------------------------------
static int encode_tmgi_mobile_identity (
  tmgi_mobile_identity_t * tmgi,
  uint8_t * buffer,
  const uint32_t len)
{
  uint32_t                                encoded = 0;

  CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, MOBILE_IDENTITY_IE_TMGI_LENGTH, len);
  *(buffer + encoded) = ((tmgi->mbmssessionidindication & 0x1) << 5) | ((tmgi->mccmncindication & 0x1) << 4) | ((tmgi->oddeven & 0x1) << 3) | (tmgi->typeofidentity & 0x7);
  encoded++;
  IES_ENCODE_U24 (buffer, encoded, tmgi->mbmsserviceid);
  *(buffer + encoded) = ((tmgi->mccdigit2 & 0xf) << 4) | (tmgi->mccdigit1 & 0xf);
  encoded++;
  *(buffer + encoded) = ((tmgi->mncdigit3 & 0xf) << 4) | (tmgi->mccdigit3 & 0xf);
  encoded++;
  *(buffer + encoded) = ((tmgi->mncdigit2 & 0xf) << 4) | (tmgi->mncdigit1 & 0xf);
  encoded++;
  *(buffer + encoded) = tmgi->mbmssessionid;
  encoded++;
  return encoded;
}

//------------------------------------------------------------------------------
// 10.5.1.6 Mobile Station Classmark 2
//------------------------------------------------------------------------------
int decode_mobile_station_classmark_2_ie (
  mobile_station_classmark2_t * mobilestationclassmark2,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     decoded = 0;
  uint8_t                                 ielen = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, MOBILE_STATION_CLASSMARK_2_IE_MAX_LENGTH, len);
    CHECK_IEI_DECODER (C_MOBILE_STATION_CLASSMARK_2_IEI, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (MOBILE_STATION_CLASSMARK_2_IE_MAX_LENGTH - 1), len);
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER (len - decoded, ielen);
  mobilestationclassmark2->revisionlevel = (*(buffer + decoded) >> 5) & 0x3;
  mobilestationclassmark2->esind = (*(buffer + decoded) >> 4) & 0x1;
  mobilestationclassmark2->a51 = (*(buffer + decoded) >> 3) & 0x1;
  mobilestationclassmark2->rfpowercapability = *(buffer + decoded) & 0x7;
  decoded++;
  mobilestationclassmark2->pscapability = (*(buffer + decoded) >> 6) & 0x1;
  mobilestationclassmark2->ssscreenindicator = (*(buffer + decoded) >> 4) & 0x3;
  mobilestationclassmark2->smcapability = (*(buffer + decoded) >> 3) & 0x1;
  mobilestationclassmark2->vbs = (*(buffer + decoded) >> 2) & 0x1;
  mobilestationclassmark2->vgcs = (*(buffer + decoded) >> 1) & 0x1;
  mobilestationclassmark2->fc = *(buffer + decoded) & 0x1;
  decoded++;
  mobilestationclassmark2->cm3 = (*(buffer + decoded) >> 7) & 0x1;
  mobilestationclassmark2->lcsvacap = (*(buffer + decoded) >> 5) & 0x1;
  mobilestationclassmark2->ucs2 = (*(buffer + decoded) >> 4) & 0x1;
  mobilestationclassmark2->solsa = (*(buffer + decoded) >> 3) & 0x1;
  mobilestationclassmark2->cmsp = (*(buffer + decoded) >> 2) & 0x1;
  mobilestationclassmark2->a53 = (*(buffer + decoded) >> 1) & 0x1;
  mobilestationclassmark2->a52 = *(buffer + decoded) & 0x1;
  decoded++;
  return decoded;
}

//------------------------------------------------------------------------------
int encode_mobile_station_classmark_2_ie (
  mobile_station_classmark2_t * mobilestationclassmark2,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  uint8_t                                *lenPtr;
  uint32_t                                encoded = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, MOBILE_STATION_CLASSMARK_2_IE_MAX_LENGTH, len);
    *buffer = C_MOBILE_STATION_CLASSMARK_2_IEI;
    encoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (MOBILE_STATION_CLASSMARK_2_IE_MAX_LENGTH - 1), len);
  }


  lenPtr = (buffer + encoded);
  encoded++;
  *(buffer + encoded) = 0x00 | ((mobilestationclassmark2->revisionlevel & 0x3) << 5) | ((mobilestationclassmark2->esind & 0x1) << 4) | ((mobilestationclassmark2->a51 & 0x1) << 3) | (mobilestationclassmark2->rfpowercapability & 0x7);
  encoded++;
  *(buffer + encoded) = 0x00 |
    ((mobilestationclassmark2->pscapability & 0x1) << 6) |
    ((mobilestationclassmark2->ssscreenindicator & 0x3) << 4) |
    ((mobilestationclassmark2->smcapability & 0x1) << 3) | ((mobilestationclassmark2->vbs & 0x1) << 2) | ((mobilestationclassmark2->vgcs & 0x1) << 1) | (mobilestationclassmark2->fc & 0x1);
  encoded++;
  *(buffer + encoded) = 0x00 | ((mobilestationclassmark2->cm3 & 0x1) << 7) |
    ((mobilestationclassmark2->lcsvacap & 0x1) << 5) |
    ((mobilestationclassmark2->ucs2 & 0x1) << 4) | ((mobilestationclassmark2->solsa & 0x1) << 3) | ((mobilestationclassmark2->cmsp & 0x1) << 2) | ((mobilestationclassmark2->a53 & 0x1) << 1) | (mobilestationclassmark2->a52 & 0x1);
  encoded++;
  *lenPtr = encoded - 1 - ((iei_present) ? 1 : 0);
  return encoded;
}

//------------------------------------------------------------------------------
// 10.5.1.7 Mobile Station Classmark 3
//------------------------------------------------------------------------------

int decode_mobile_station_classmark_3_ie (
  mobile_station_classmark3_t * mobilestationclassmark3,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  //AssertFatal(false, "TODO");
  return 0;
}

//------------------------------------------------------------------------------
int encode_mobile_station_classmark_3_ie(
  mobile_station_classmark3_t * mobilestationclassmark3,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  //AssertFatal(false, "TODO");
  return 0;
}

//------------------------------------------------------------------------------
// 10.5.1.13 PLMN list
//------------------------------------------------------------------------------
int decode_plmn_list_ie (
  plmn_list_t * plmnlist,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     decoded = 0;
  uint8_t                                 ielen = 0;
  uint8_t                                 i = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (plmnlist->num_plmn*3+2), len);
    CHECK_IEI_DECODER (C_PLMN_LIST_IEI, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (plmnlist->num_plmn*3+1), len);
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER (len - decoded, ielen);
  plmnlist->num_plmn = 0;
  while (decoded < len) {
    plmnlist->plmn[i].mcc_digit2 = (*(buffer + decoded) >> 4) & 0xf;
    plmnlist->plmn[i].mcc_digit1 = *(buffer + decoded) & 0xf;
    decoded++;
    plmnlist->plmn[i].mnc_digit3 = (*(buffer + decoded) >> 4) & 0xf;
    plmnlist->plmn[i].mcc_digit3 = *(buffer + decoded) & 0xf;
    decoded++;
    plmnlist->plmn[i].mnc_digit2 = (*(buffer + decoded) >> 4) & 0xf;
    plmnlist->plmn[i].mnc_digit1 = *(buffer + decoded) & 0xf;
    decoded++;
    plmnlist->num_plmn += 1;
    i += 1;
  }
  return decoded;
}

//------------------------------------------------------------------------------
int encode_plmn_list_ie (
  plmn_list_t * plmnlist,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  uint8_t                                *lenPtr;
  uint32_t                                encoded = 0;

  /*
   * Checking IEI and pointer
   */

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (2 + (plmnlist->num_plmn*3)), len);
    *buffer = C_PLMN_LIST_IEI;
    encoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (1 + (plmnlist->num_plmn*3)), len);
  }

  lenPtr = (buffer + encoded);
  encoded++;
  for (int i = 0; i < plmnlist->num_plmn; i++) {
    *(buffer + encoded) = 0x00 | ((plmnlist->plmn[i].mcc_digit2 & 0xf) << 4) | (plmnlist->plmn[i].mcc_digit1 & 0xf);
    encoded++;
    *(buffer + encoded) = 0x00 | ((plmnlist->plmn[i].mnc_digit3 & 0xf) << 4) | (plmnlist->plmn[i].mcc_digit3 & 0xf);
    encoded++;
    *(buffer + encoded) = 0x00 | ((plmnlist->plmn[i].mnc_digit2 & 0xf) << 4) | (plmnlist->plmn[i].mnc_digit1 & 0xf);
    encoded++;
  }
  *lenPtr = encoded - 1 - ((iei_present) ? 1 : 0);
  return encoded;
}

//------------------------------------------------------------------------------
// 10.5.1.15 MS network feature support
//------------------------------------------------------------------------------
int decode_ms_network_feature_support_ie(ms_network_feature_support_t *msnetworkfeaturesupport, const bool iei_present, uint8_t *buffer, const uint32_t len)
{
  int decoded = 0;


  CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, MS_NETWORK_FEATURE_SUPPORT_IE_MAX_LENGTH, len);
  if (iei_present) {
    CHECK_IEI_DECODER(C_MS_NETWORK_FEATURE_SUPPORT_IEI, (*buffer & 0xc0));
  }
  msnetworkfeaturesupport->spare_bits= (*(buffer + decoded) >> 3) & 0x7;
  msnetworkfeaturesupport->extended_periodic_timers= *(buffer + decoded) & 0x1;
  decoded++;

  return decoded;
}

//------------------------------------------------------------------------------
int encode_ms_network_feature_support_ie(ms_network_feature_support_t *msnetworkfeaturesupport, const bool iei_present, uint8_t *buffer, const uint32_t len)
{
  uint32_t encoded = 0;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, MS_NETWORK_FEATURE_SUPPORT_IE_MAX_LENGTH, len);

  *(buffer + encoded) = 0x00 | ((msnetworkfeaturesupport->spare_bits & 0x7) << 3)
                             | (msnetworkfeaturesupport->extended_periodic_timers & 0x1);
  encoded++;
  return encoded;
}


