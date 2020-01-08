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

/*! \file 3gpp_24.008_sm_ies.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>

#include "bstrlib.h"

#include "log.h"
#include "dynamic_memory_check.h"
#include "common_defs.h"
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_24.301.h"
#include "TLVDecoder.h"
#include "TLVEncoder.h"


//******************************************************************************
// 10.5.6 Session management information elements
//******************************************************************************
//------------------------------------------------------------------------------
// 10.5.6.1 Access Point Name
//------------------------------------------------------------------------------
int decode_access_point_name_ie (
  access_point_name_t * access_point_name,
  bool is_ie_present,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     decoded = 0;
  uint8_t                                 ielen = 0;

  *access_point_name = NULL;

  if (is_ie_present > 0) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, ACCESS_POINT_NAME_IE_MIN_LENGTH, len);
    CHECK_IEI_DECODER (SM_ACCESS_POINT_NAME_IEI, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (ACCESS_POINT_NAME_IE_MIN_LENGTH - 1), len);
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER (len - decoded, ielen);

  if (1 < ielen) {
    int length_apn = *(buffer + decoded);
    decoded++;
    *access_point_name = blk2bstr((void *)(buffer + decoded), length_apn);
    decoded += length_apn;
    ielen = ielen - 1 - length_apn;
    while (1 <= ielen) {
      bconchar(*access_point_name, '.');
      length_apn = *(buffer + decoded);
      decoded++;
      ielen = ielen - 1;

      // apn terminated by '.' ?
      if (length_apn > 0) {
        AssertFatal(ielen >= length_apn, "Mismatch in lengths remaining ielen %d apn length %d", ielen, length_apn);
        bcatblk(*access_point_name, (void *)(buffer + decoded), length_apn);
        decoded += length_apn;
        ielen = ielen - length_apn;
      }
    }
  }
  return decoded;
}

//------------------------------------------------------------------------------
int encode_access_point_name_ie (
  access_point_name_t access_point_name,
  bool is_ie_present,
  uint8_t * buffer,
  const uint32_t len)
{
  uint8_t                                *lenPtr = NULL;
  uint32_t                                encoded = 0;
  int                                     encode_result = 0;
  uint32_t                                length_index = 0;
  uint32_t                                index = 0;
  uint32_t                                index_copy = 0;
  uint8_t                                 apn_encoded[ACCESS_POINT_NAME_IE_MAX_LENGTH] = {0};


  if (is_ie_present > 0) {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, ACCESS_POINT_NAME_IE_MAX_LENGTH, len);
    *buffer = SM_ACCESS_POINT_NAME_IEI;
    encoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (ACCESS_POINT_NAME_IE_MIN_LENGTH - 1), len);
  }

  lenPtr = (buffer + encoded);
  encoded++;
  index = 0;                    // index on original APN string
  length_index = 0;             // marker where to write partial length
  index_copy = 1;

  while ((access_point_name->data[index] != 0) && (index < access_point_name->slen)) {
    if (access_point_name->data[index] == '.') {
      apn_encoded[length_index] = index_copy - length_index - 1;
      length_index = index_copy;
      index_copy = length_index + 1;
    } else {
      apn_encoded[index_copy] = access_point_name->data[index];
      index_copy++;
    }

    index++;
  }

  apn_encoded[length_index] = index_copy - length_index - 1;
  bstring bapn = blk2bstr(apn_encoded, index_copy);

  if ((encode_result = encode_bstring (bapn, buffer + encoded, len - encoded)) < 0) {
    bdestroy_wrapper (&bapn);
    return encode_result;
  } else {
    encoded += encode_result;
  }
  bdestroy_wrapper (&bapn);
  *lenPtr = encoded - 1 - ((is_ie_present) ? 1 : 0);
  return encoded;
}

//------------------------------------------------------------------------------
// 10.5.6.3 Protocol configuration options
//------------------------------------------------------------------------------
void copy_protocol_configuration_options (protocol_configuration_options_t * const pco_dst, const protocol_configuration_options_t * const pco_src)
{
  if ((pco_dst) && (pco_src)) {
    pco_dst->ext = pco_src->ext;
    pco_dst->spare = pco_src->spare;
    pco_dst->configuration_protocol = pco_src->configuration_protocol;
    pco_dst->num_protocol_or_container_id = pco_src->num_protocol_or_container_id;
    AssertFatal(PCO_UNSPEC_MAXIMUM_PROTOCOL_ID_OR_CONTAINER_ID >= pco_dst->num_protocol_or_container_id,
        "Invalid number of protocol_or_container_id %d", pco_dst->num_protocol_or_container_id);
    for (int i = 0; i < pco_src->num_protocol_or_container_id; i++) {
      pco_dst->protocol_or_container_ids[i].id     = pco_src->protocol_or_container_ids[i].id;
      pco_dst->protocol_or_container_ids[i].length = pco_src->protocol_or_container_ids[i].length;
      pco_dst->protocol_or_container_ids[i].contents = bstrcpy(pco_src->protocol_or_container_ids[i].contents);
    }
  }
}

//------------------------------------------------------------------------------
void clear_protocol_configuration_options (protocol_configuration_options_t * const pco)
{
  if (pco) {
    for (int i = 0; i < PCO_UNSPEC_MAXIMUM_PROTOCOL_ID_OR_CONTAINER_ID; i++) {
      if (pco->protocol_or_container_ids[i].contents) {
        bdestroy_wrapper (&pco->protocol_or_container_ids[i].contents);
      }
    }
    memset(pco, 0, sizeof(protocol_configuration_options_t));
  }
}

//------------------------------------------------------------------------------
void free_protocol_configuration_options (protocol_configuration_options_t ** const protocol_configuration_options)
{
  protocol_configuration_options_t *pco = *protocol_configuration_options;
  if (pco) {
    for (int i = 0; i < PCO_UNSPEC_MAXIMUM_PROTOCOL_ID_OR_CONTAINER_ID; i++) {
      if (pco->protocol_or_container_ids[i].contents) {
        bdestroy_wrapper (&pco->protocol_or_container_ids[i].contents);
      }
    }
    free_wrapper((void**)protocol_configuration_options);
  }
}

//------------------------------------------------------------------------------
int
decode_protocol_configuration_options (
    protocol_configuration_options_t * protocolconfigurationoptions,
    const uint8_t * const buffer,
    const uint32_t len)
{
  int                                     decoded = 0;
  int                                     decode_result = 0;


  if (((*(buffer + decoded) >> 7) & 0x1) != 1) {
    return TLV_VALUE_DOESNT_MATCH;
  }

  /*
   * Bits 7 to 4 of octet 3 are spare, read as 0
   */
  if (((*(buffer + decoded) & 0x78) >> 3) != 0) {
    return TLV_VALUE_DOESNT_MATCH;
  }

  protocolconfigurationoptions->configuration_protocol = (*(buffer + decoded) >> 1) & 0x7;
  decoded++;
  protocolconfigurationoptions->num_protocol_or_container_id = 0;

  while (3 <= ((int32_t)len - (int32_t)decoded)) {
    DECODE_U16 (buffer + decoded, protocolconfigurationoptions->protocol_or_container_ids[protocolconfigurationoptions->num_protocol_or_container_id].id, decoded);
    DECODE_U8 (buffer + decoded, protocolconfigurationoptions->protocol_or_container_ids[protocolconfigurationoptions->num_protocol_or_container_id].length, decoded);

    if (0 < protocolconfigurationoptions->protocol_or_container_ids[protocolconfigurationoptions->num_protocol_or_container_id].length) {
      if ((decode_result = decode_bstring (&protocolconfigurationoptions->protocol_or_container_ids[protocolconfigurationoptions->num_protocol_or_container_id].contents,
          protocolconfigurationoptions->protocol_or_container_ids[protocolconfigurationoptions->num_protocol_or_container_id].length,
          buffer + decoded,
          len - decoded)) < 0) {
        return decode_result;
      } else {
        decoded += decode_result;
      }
    } else {
      protocolconfigurationoptions->protocol_or_container_ids[protocolconfigurationoptions->num_protocol_or_container_id].contents = NULL;
    }
    protocolconfigurationoptions->num_protocol_or_container_id += 1;
  }

  return decoded;
}
//------------------------------------------------------------------------------
int
decode_protocol_configuration_options_ie (
    protocol_configuration_options_t * protocolconfigurationoptions,
    const bool iei_present,
    const uint8_t * const buffer,
    const uint32_t len)
{
  int                                     decoded = 0;
  int                                     decoded2 = 0;
  uint8_t                                 ielen = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, PROTOCOL_CONFIGURATION_OPTIONS_IE_MIN_LENGTH, len);
    CHECK_IEI_DECODER (SM_PROTOCOL_CONFIGURATION_OPTIONS_IEI, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (PROTOCOL_CONFIGURATION_OPTIONS_IE_MIN_LENGTH - 1), len);
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER (len - decoded, ielen);

  decoded2 = decode_protocol_configuration_options(protocolconfigurationoptions, buffer + decoded, len - decoded);
  if (decoded2 < 0) return decoded2;
  return decoded+decoded2;
}
//------------------------------------------------------------------------------
int
encode_protocol_configuration_options (
    const protocol_configuration_options_t * const protocolconfigurationoptions,
    uint8_t * buffer,
    const uint32_t len)
{
  uint8_t                                 num_protocol_or_container_id = 0;
  uint32_t                                encoded = 0;
  int                                     encode_result = 0;

  *(buffer + encoded) = 0x00 | (1 << 7) | (protocolconfigurationoptions->configuration_protocol & 0x7);
  encoded++;

  while (num_protocol_or_container_id < protocolconfigurationoptions->num_protocol_or_container_id) {
    ENCODE_U16 (buffer + encoded, protocolconfigurationoptions->protocol_or_container_ids[num_protocol_or_container_id].id, encoded);
    *(buffer + encoded) = protocolconfigurationoptions->protocol_or_container_ids[num_protocol_or_container_id].length;
    encoded++;

    if ((encode_result = encode_bstring (protocolconfigurationoptions->protocol_or_container_ids[num_protocol_or_container_id].contents,
        buffer + encoded, len - encoded)) < 0)
      return encode_result;
    else
      encoded += encode_result;

    num_protocol_or_container_id += 1;
  }
  return encoded;
}

//------------------------------------------------------------------------------
int
encode_protocol_configuration_options_ie (
    const protocol_configuration_options_t * const protocolconfigurationoptions,
    const bool iei_present,
    uint8_t * buffer,
    const uint32_t len)
{
  uint8_t                                *lenPtr = NULL;
  uint32_t                                encoded = 0;

 if (iei_present) {
   CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, PROTOCOL_CONFIGURATION_OPTIONS_IE_MIN_LENGTH, len);
   *buffer = SM_PROTOCOL_CONFIGURATION_OPTIONS_IEI;
   encoded++;
 } else {
   CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (PROTOCOL_CONFIGURATION_OPTIONS_IE_MIN_LENGTH - 1), len);
 }

 lenPtr = (buffer + encoded);
 encoded++;

 encoded += encode_protocol_configuration_options(protocolconfigurationoptions, buffer + encoded, len - encoded);

  *lenPtr = encoded - 1 - ((iei_present) ? 1 : 0);
  return encoded;
}

//------------------------------------------------------------------------------
// 10.5.6.5 Quality of service
//------------------------------------------------------------------------------
int decode_quality_of_service_ie (
  quality_of_service_t * qualityofservice,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     decoded = 0;
  uint8_t                                 ielen = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, QUALITY_OF_SERVICE_IE_MIN_LENGTH, len);
    CHECK_IEI_DECODER (SM_QUALITY_OF_SERVICE_IEI, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (QUALITY_OF_SERVICE_IE_MIN_LENGTH - 1), len);
  }

  ielen = *(buffer + decoded);
  decoded++;
  CHECK_LENGTH_DECODER (len - decoded, ielen);
  qualityofservice->delayclass = (*(buffer + decoded) >> 3) & 0x7;
  qualityofservice->reliabilityclass = *(buffer + decoded) & 0x7;
  decoded++;
  qualityofservice->peakthroughput = (*(buffer + decoded) >> 4) & 0xf;
  qualityofservice->precedenceclass = *(buffer + decoded) & 0x7;
  decoded++;
  qualityofservice->meanthroughput = *(buffer + decoded) & 0x1f;
  decoded++;
  qualityofservice->trafficclass = (*(buffer + decoded) >> 5) & 0x7;
  qualityofservice->deliveryorder = (*(buffer + decoded) >> 3) & 0x3;
  qualityofservice->deliveryoferroneoussdu = *(buffer + decoded) & 0x7;
  decoded++;
  qualityofservice->maximumsdusize = *(buffer + decoded);
  decoded++;
  qualityofservice->maximumbitrateuplink = *(buffer + decoded);
  decoded++;
  qualityofservice->maximumbitratedownlink = *(buffer + decoded);
  decoded++;
  qualityofservice->residualber = (*(buffer + decoded) >> 4) & 0xf;
  qualityofservice->sduratioerror = *(buffer + decoded) & 0xf;
  decoded++;
  qualityofservice->transferdelay = (*(buffer + decoded) >> 2) & 0x3f;
  qualityofservice->traffichandlingpriority = *(buffer + decoded) & 0x3;
  decoded++;
  qualityofservice->guaranteedbitrateuplink = *(buffer + decoded);
  decoded++;
  qualityofservice->guaranteedbitratedownlink = *(buffer + decoded);
  decoded++;
  qualityofservice->signalingindication = (*(buffer + decoded) >> 4) & 0x1;
  qualityofservice->sourcestatisticsdescriptor = *(buffer + decoded) & 0xf;
  decoded++;
  return decoded;
}

//------------------------------------------------------------------------------
int encode_quality_of_service_ie (
  quality_of_service_t * qualityofservice,
  const bool iei_present,
  uint8_t * buffer,
  const uint32_t len)
{
  uint8_t                                *lenPtr;
  uint32_t                                encoded = 0;

  if (iei_present) {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, QUALITY_OF_SERVICE_IE_MIN_LENGTH, len);
    *buffer = SM_QUALITY_OF_SERVICE_IEI;
    encoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (QUALITY_OF_SERVICE_IE_MIN_LENGTH - 1), len);
  }

  lenPtr = (buffer + encoded);
  encoded++;
  *(buffer + encoded) = 0x00 | ((qualityofservice->delayclass & 0x7) << 3) | (qualityofservice->reliabilityclass & 0x7);
  encoded++;
  *(buffer + encoded) = 0x00 | ((qualityofservice->peakthroughput & 0xf) << 4) | (qualityofservice->precedenceclass & 0x7);
  encoded++;
  *(buffer + encoded) = 0x00 | (qualityofservice->meanthroughput & 0x1f);
  encoded++;
  *(buffer + encoded) = 0x00 | ((qualityofservice->trafficclass & 0x7) << 5) | ((qualityofservice->deliveryorder & 0x3) << 3) | (qualityofservice->deliveryoferroneoussdu & 0x7);
  encoded++;
  *(buffer + encoded) = qualityofservice->maximumsdusize;
  encoded++;
  *(buffer + encoded) = qualityofservice->maximumbitrateuplink;
  encoded++;
  *(buffer + encoded) = qualityofservice->maximumbitratedownlink;
  encoded++;
  *(buffer + encoded) = 0x00 | ((qualityofservice->residualber & 0xf) << 4) | (qualityofservice->sduratioerror & 0xf);
  encoded++;
  *(buffer + encoded) = 0x00 | ((qualityofservice->transferdelay & 0x3f) << 2) | (qualityofservice->traffichandlingpriority & 0x3);
  encoded++;
  *(buffer + encoded) = qualityofservice->guaranteedbitrateuplink;
  encoded++;
  *(buffer + encoded) = qualityofservice->guaranteedbitratedownlink;
  encoded++;
  *(buffer + encoded) = 0x00 | ((qualityofservice->signalingindication & 0x1) << 4) | (qualityofservice->sourcestatisticsdescriptor & 0xf);
  encoded++;
  *lenPtr = encoded - 1 - ((iei_present) ? 1 : 0);
  return encoded;
}

//------------------------------------------------------------------------------
// 10.5.6.7 Linked TI
//------------------------------------------------------------------------------
int encode_linked_ti_ie(linked_ti_t *linkedti, const bool iei_present, uint8_t *buffer, const uint32_t len)
{
  AssertFatal(0, "TODO");
  return 0;
}

int decode_linked_ti_ie(linked_ti_t *linkedti, const bool iei_present, uint8_t *buffer, const uint32_t len)
{
  AssertFatal(0, "TODO");
  return 0;
}

//------------------------------------------------------------------------------
// 10.5.6.9 LLC service access point identifier
//------------------------------------------------------------------------------
int decode_llc_service_access_point_identifier_ie (
  llc_service_access_point_identifier_t * llc_sap_id,
  bool is_ie_present,
  uint8_t * buffer,
  const uint32_t len)
{
  int                                     decoded = 0;

  if (is_ie_present) {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, LLC_SERVICE_ACCESS_POINT_IDENTIFIER_IE_MAX_LENGTH, len);
    CHECK_IEI_DECODER (SM_LLC_SERVICE_ACCESS_POINT_IDENTIFIER_IEI, *buffer);
    decoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_DECODER (buffer, (LLC_SERVICE_ACCESS_POINT_IDENTIFIER_IE_MAX_LENGTH - 1), len);
  }

  *llc_sap_id = *buffer & 0xf;
  decoded++;
  return decoded;
}

//------------------------------------------------------------------------------
int encode_llc_service_access_point_identifier_ie (
  llc_service_access_point_identifier_t * llc_sap_id,
  bool is_ie_present,
  uint8_t * buffer,
  const uint32_t len)
{
  uint32_t                                encoded = 0;

  if (is_ie_present) {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, LLC_SERVICE_ACCESS_POINT_IDENTIFIER_IE_MIN_LENGTH, len);
    *buffer = SM_LLC_SERVICE_ACCESS_POINT_IDENTIFIER_IEI;
    encoded++;
  } else {
    CHECK_PDU_POINTER_AND_LENGTH_ENCODER (buffer, (LLC_SERVICE_ACCESS_POINT_IDENTIFIER_IE_MIN_LENGTH - 1), len);
  }

  *(buffer + encoded) = 0x00 | (*llc_sap_id & 0xf);
  encoded++;
  return encoded;
}
