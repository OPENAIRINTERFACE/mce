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

/*! \file 3gpp_24.008_mm_ies.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"

#include "common_defs.h"
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "TLVDecoder.h"
#include "TLVEncoder.h"
#include "log.h"

//------------------------------------------------------------------------------
// 10.5.3.5a Network Name
//------------------------------------------------------------------------------
int decode_network_name_ie (
  network_name_t * networkname,
  const uint8_t iei,
  uint8_t * buffer,
  const uint32_t len)
{
  OAILOG_FUNC_IN (LOG_MCE_APP);
  int                                     decoded = 0;
  uint8_t                                 ielen = 0;
  int                                     decode_result;

  if (iei > 0) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, NETWORK_NAME_IE_MIN_LENGTH, len);
    CHECK_IEI_DECODER (iei, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (NETWORK_NAME_IE_MIN_LENGTH - 1), len);
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER (len - decoded, ielen);

  if (((*buffer >> 7) & 0x1) != 1) {
    errorCodeDecoder = TLV_VALUE_DOESNT_MATCH;
    return TLV_VALUE_DOESNT_MATCH;
  }

  networkname->codingscheme = (*(buffer + decoded) >> 5) & 0x7;
  networkname->addci = (*(buffer + decoded) >> 4) & 0x1;
  networkname->numberofsparebitsinlastoctet = (*(buffer + decoded) >> 1) & 0x7;

  if ((decode_result = decode_bstring (&networkname->textstring, ielen, buffer + decoded, len - decoded)) < 0) {
    OAILOG_FUNC_RETURN (LOG_MCE_APP, decode_result);
  } else
    decoded += decode_result;
  OAILOG_FUNC_RETURN (LOG_MCE_APP, decoded);
}

//------------------------------------------------------------------------------
int encode_network_name_ie (
  network_name_t * networkname,
  const uint8_t iei,
  uint8_t * buffer,
  uint32_t len)
{
  uint8_t                                *lenPtr;
  uint32_t                                encoded = 0;
  int                                     encode_result;


  if (iei > 0) {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, NETWORK_NAME_IE_MIN_LENGTH +blength( networkname->textstring) - 1, len);
    *buffer = iei;
    encoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, NETWORK_NAME_IE_MIN_LENGTH + blength(networkname->textstring) - 2, len);
  }

  lenPtr = (buffer + encoded);
  encoded++;
  *(buffer + encoded) = 0x00 | (1 << 7) | ((networkname->codingscheme & 0x7) << 4) | ((networkname->addci & 0x1) << 3) | (networkname->numberofsparebitsinlastoctet & 0x7);
  encoded++;

  if ((encode_result = encode_bstring (networkname->textstring, buffer + encoded, len - encoded)) < 0)
    return encode_result;
  else
    encoded += encode_result;

  *lenPtr = encoded - 1 - ((iei > 0) ? 1 : 0);
  return encoded;
}

//------------------------------------------------------------------------------
// 10.5.3.8 Time Zone
//------------------------------------------------------------------------------
int encode_time_zone(time_zone_t *timezone, const bool iei_present, uint8_t *buffer, const uint32_t len)
{
  uint32_t                                encoded = 0;

  /*
   * Checking IEI and pointer
   */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, TIME_ZONE_IE_MAX_LENGTH, len);

  if (iei_present) {
    *buffer = MM_TIME_ZONE_IEI;
    encoded++;
  }

  *(buffer + encoded) = *timezone;
  encoded++;
  return encoded;
}

//------------------------------------------------------------------------------
int decode_time_zone(time_zone_t *timezone, const bool iei_present, uint8_t *buffer, const uint32_t len)
{
  int                                     decoded = 0;

  if (iei_present) {
    CHECK_IEI_DECODER (MM_TIME_ZONE_IEI, *buffer);
    decoded++;
  }

  *timezone = *(buffer + decoded);
  decoded++;
  return decoded;

}

//------------------------------------------------------------------------------
// 10.5.3.9 Time Zone and Time
//------------------------------------------------------------------------------
int encode_time_zone_and_time(time_zone_and_time_t *timezoneandtime, const bool iei_present, uint8_t *buffer, const uint32_t len)
{
  uint32_t                                encoded = 0;

  /*
   * Checking IEI and pointer
   */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, TIME_ZONE_AND_TIME_MAX_LENGTH, len);

  if (iei_present) {
    *buffer = MM_TIME_ZONE_AND_TIME_IEI;
    encoded++;
  }

  *(buffer + encoded) = timezoneandtime->year;
  encoded++;
  *(buffer + encoded) = timezoneandtime->month;
  encoded++;
  *(buffer + encoded) = timezoneandtime->day;
  encoded++;
  *(buffer + encoded) = timezoneandtime->hour;
  encoded++;
  *(buffer + encoded) = timezoneandtime->minute;
  encoded++;
  *(buffer + encoded) = timezoneandtime->second;
  encoded++;
  *(buffer + encoded) = timezoneandtime->timezone;
  encoded++;
  return encoded;
}

//------------------------------------------------------------------------------
int decode_time_zone_and_time(time_zone_and_time_t *timezoneandtime, const bool iei_present, uint8_t *buffer, const uint32_t len)
{
  int                                     decoded = 0;

  if (iei_present) {
    CHECK_IEI_DECODER (MM_TIME_ZONE_AND_TIME_IEI, *buffer);
    decoded++;
  }

  timezoneandtime->year = *(buffer + decoded);
  decoded++;
  timezoneandtime->month = *(buffer + decoded);
  decoded++;
  timezoneandtime->day = *(buffer + decoded);
  decoded++;
  timezoneandtime->hour = *(buffer + decoded);
  decoded++;
  timezoneandtime->minute = *(buffer + decoded);
  decoded++;
  timezoneandtime->second = *(buffer + decoded);
  decoded++;
  timezoneandtime->timezone = *(buffer + decoded);
  decoded++;
  return decoded;
}

//------------------------------------------------------------------------------
// 10.5.3.12 Daylight Saving Time
//------------------------------------------------------------------------------
int decode_daylight_saving_time_ie (
  daylight_saving_time_t * daylightsavingtime,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     decoded = 0;
  uint8_t                                 ielen = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, DAYLIGHT_SAVING_TIME_IE_MAX_LENGTH, len);
    CHECK_IEI_DECODER (MM_DAYLIGHT_SAVING_TIME_IEI, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (DAYLIGHT_SAVING_TIME_IE_MAX_LENGTH - 1), len);
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER (len - decoded, ielen);
  *daylightsavingtime = *buffer & 0x3;
  decoded++;
  return decoded;
}

//------------------------------------------------------------------------------
int encode_daylight_saving_time_ie (
  daylight_saving_time_t * daylightsavingtime,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  uint8_t                                *lenPtr;
  uint32_t                                encoded = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, DAYLIGHT_SAVING_TIME_IE_MAX_LENGTH, len);
    *buffer = MM_DAYLIGHT_SAVING_TIME_IEI;
    encoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (DAYLIGHT_SAVING_TIME_IE_MAX_LENGTH - 1), len);
  }

  lenPtr = (buffer + encoded);
  encoded++;
  *(buffer + encoded) = 0x00 | (*daylightsavingtime & 0x3);
  encoded++;
  *lenPtr = encoded - 1 - ((iei_present) ? 1 : 0);
  return encoded;
}

//------------------------------------------------------------------------------
// 10.5.3.13 Emergency Number List
//------------------------------------------------------------------------------
int decode_emergency_number_list_ie (
  emergency_number_list_t * emergencynumberlist,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     decoded = 0;
  uint8_t                                 ielen = 0;
  emergency_number_list_t                *e = emergencynumberlist;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, EMERGENCY_NUMBER_LIST_IE_MIN_LENGTH, len);
    CHECK_IEI_DECODER (MM_EMERGENCY_NUMBER_LIST_IEI, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (EMERGENCY_NUMBER_LIST_IE_MIN_LENGTH - 1), len);
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER (len - decoded, ielen);

  e->lengthofemergencynumberinformation = *(buffer + decoded);
  decoded++;
  emergencynumberlist->emergencyservicecategoryvalue = *(buffer + decoded) & 0x1f;
  decoded++;
  for (int i=0; i < e->lengthofemergencynumberinformation - 1; i++) {
    e->number_digit[i] = *(buffer + decoded);
    decoded++;
  }
  for (int i=e->lengthofemergencynumberinformation - 1; i < EMERGENCY_NUMBER_MAX_DIGITS; i++) {
    e->number_digit[i] = 0xFF;
  }
  AssertFatal(0, "TODO emergency_number_list_t->next");

  return decoded;
}

//------------------------------------------------------------------------------
int encode_emergency_number_list_ie (
  emergency_number_list_t * emergencynumberlist,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  uint8_t                                *lenPtr;
  uint32_t                                encoded = 0;
  emergency_number_list_t                *e = emergencynumberlist;

  AssertFatal(0, "TODO");
  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, EMERGENCY_NUMBER_LIST_IE_MIN_LENGTH, len);
    *buffer = MM_EMERGENCY_NUMBER_LIST_IEI;
    encoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (EMERGENCY_NUMBER_LIST_IE_MIN_LENGTH - 1), len);
  }

  lenPtr = (buffer + encoded);
  encoded++;
  while (e) {
    *(buffer + encoded) = emergencynumberlist->lengthofemergencynumberinformation;
    encoded++;
    *(buffer + encoded) = 0x00 | (emergencynumberlist->emergencyservicecategoryvalue & 0x1f);
    encoded++;
    for (int i=0; i < EMERGENCY_NUMBER_MAX_DIGITS; i++) {
      if (e->number_digit[i] < 10) {
        *(buffer + encoded) = e->number_digit[i] ;
      } else {
        break;
      }
      if (e->number_digit[i] < 10) {
        *(buffer + encoded) |= (e->number_digit[i] << 4);
      } else {
        *(buffer + encoded) |= 0xF0;
        encoded++;
        break;
      }
      encoded++;
    }
    e = e->next;
  }
  *lenPtr = encoded - 1 - ((iei_present) ? 1 : 0);
  return encoded;
}
