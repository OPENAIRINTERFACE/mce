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

/*! \file s11_ie_formatter.c
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include "gtpv2c_ie_formatter.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <pthread.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "common_defs.h"
#include "gcc_diag.h"
#include "log.h"
#include "assertions.h"
#include "conversions.h"
#include "3gpp_33.401.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_24.007.h"
#include "3gpp_29.274.h"
#include "3gpp_36.443.h"
#include "3gpp_36.444.h"
#include "NwGtpv2c.h"
#include "NwGtpv2cIe.h"
#include "NwGtpv2cMsg.h"
#include "NwGtpv2cMsgParser.h"
#include "security_types.h"
#include "common_types.h"

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_cause_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  gtpv2c_cause_t                             *cause = (gtpv2c_cause_t *) arg;

  DevAssert (cause );
  cause->cause_value = ieValue[0];
  cause->cs          = ieValue[1] & 0x01;
  cause->bce         = (ieValue[1] & 0x02) >> 1;
  cause->pce         = (ieValue[1] & 0x04) >> 2;
  if (6 == ieLength) {
    cause->offending_ie_type     = ieValue[2];
    cause->offending_ie_length   = ((uint16_t)ieValue[3]) << 8;
    cause->offending_ie_length  |= ((uint16_t)ieValue[4]);
    cause->offending_ie_instance = ieValue[5] & 0x0F;
  }
  OAILOG_DEBUG (LOG_GTPV2C, "\t- Cause value %u\n", cause->cause_value);
  return NW_OK;
}

//------------------------------------------------------------------------------
int
gtpv2c_cause_ie_set (
  nw_gtpv2c_msg_handle_t * msg,
  const gtpv2c_cause_t * cause)
{
  nw_rc_t                                   rc;
  uint8_t                                 value[6];

  DevAssert (msg );
  DevAssert (cause );
  value[0] = cause->cause_value;
  value[1] = ((cause->pce & 0x1) << 2) | ((cause->bce & 0x1) << 1) | (cause->cs & 0x1);

  if (cause->offending_ie_type ) {
    value[2] = cause->offending_ie_type;
    value[3] = (cause->offending_ie_length & 0xFF00) >> 8;
    value[4] = cause->offending_ie_length & 0x00FF;
    value[5] = cause->offending_ie_instance & 0x0F;
    rc = nwGtpv2cMsgAddIe (*msg, NW_GTPV2C_IE_CAUSE, 6, 0, value);
  } else {
    rc = nwGtpv2cMsgAddIe (*msg, NW_GTPV2C_IE_CAUSE, 2, 0, value);
  }

  DevAssert (NW_OK == rc);
  return rc == NW_OK ? 0 : -1;
}


//------------------------------------------------------------------------------
int
gtpv2c_fteid_ie_set (
  nw_gtpv2c_msg_handle_t * msg,
  const fteid_t * fteid,
  const uint8_t   instance)
{
  nw_rc_t                                   rc;
  uint8_t                                 value[25];

  DevAssert (msg );
  DevAssert (fteid );
  /*
   * MCC Decimal | MCC Hundreds
   */
  value[0] = (fteid->ipv4 << 7) | (fteid->ipv6 << 6) | (fteid->interface_type & 0x3F);
  value[1] = (fteid->teid >> 24 );
  value[2] = (fteid->teid >> 16 ) & 0xFF;
  value[3] = (fteid->teid >>  8 ) & 0xFF;
  value[4] = (fteid->teid >>  0 ) & 0xFF;

  int offset = 5;
  if (fteid->ipv4 == 1) {
    uint32_t hbo = ntohl(fteid->ipv4_address.s_addr);
    value[offset++] = (uint8_t)(hbo >> 24);
    value[offset++] = (uint8_t)(hbo >> 16);
    value[offset++] = (uint8_t)(hbo >> 8);
    value[offset++] = (uint8_t)hbo;
  }
  if (fteid->ipv6 == 1) {
    /*
     * IPv6 present: copy the 16 bytes
     */
    memcpy (&value[offset], fteid->ipv6_address.__in6_u.__u6_addr8, 16);
    offset += 16;
  }

  rc = nwGtpv2cMsgAddIe (*msg, NW_GTPV2C_IE_FTEID, offset, instance, value);
  DevAssert (NW_OK == rc);
  return RETURNok;
}


//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_fteid_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  uint8_t                                 offset = 0;
   fteid_t                                *fteid = (fteid_t *) arg;

   DevAssert (fteid );
   fteid->ipv4 = (ieValue[0] & 0x80) >> 7;
   fteid->ipv6 = (ieValue[0] & 0x40) >> 6;
   fteid->interface_type = ieValue[0] & 0x1F;
   OAILOG_DEBUG (LOG_GTPV2C, "\t- F-TEID type %d\n", fteid->interface_type);
   /*
    * Copy the TEID or GRE key
    */
   fteid->teid = ntoh_int32_buf (&ieValue[1]);
   OAILOG_DEBUG (LOG_GTPV2C, "\t- TEID/GRE    %08x\n", fteid->teid);

   if (fteid->ipv4 == 1) {
     /*
      * IPv4 present: copy the 4 bytes
      */
     uint32_t hbo = (((uint32_t)ieValue[5]) << 24) |
                    (((uint32_t)ieValue[6]) << 16) |
                    (((uint32_t)ieValue[7]) << 8) |
                    (uint32_t)ieValue[8];
     fteid->ipv4_address.s_addr = htonl(hbo);
     offset = 4;
     OAILOG_DEBUG (LOG_GTPV2C, "\t- IPv4 addr   " IN_ADDR_FMT "\n", PRI_IN_ADDR (fteid->ipv4_address));
   }

   if (fteid->ipv6 == 1) {
     char                                    ipv6_ascii[INET6_ADDRSTRLEN];

     /*
      * IPv6 present: copy the 16 bytes
      * * * * WARNING: if Ipv4 is present, 4 bytes of offset should be applied
      */
     memcpy (fteid->ipv6_address.__in6_u.__u6_addr8, &ieValue[5 + offset], 16);
     inet_ntop (AF_INET6, (void*)&fteid->ipv6_address, ipv6_ascii, INET6_ADDRSTRLEN);
     OAILOG_DEBUG (LOG_GTPV2C, "\t- IPv6 addr   %s\n", ipv6_ascii);
   }

   return NW_OK;
}

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_paa_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  uint8_t                                 offset = 0;
  paa_t                                  *paa = (paa_t *) arg;

  DevAssert (paa );
  paa->pdn_type = ieValue[0] & 0x07;
  OAILOG_DEBUG (LOG_SM, "\t- PAA type  %d\n", paa->pdn_type);

  if (paa->pdn_type & 0x2) {
    char                                    ipv6_ascii[INET6_ADDRSTRLEN];

    /*
     * IPv6 present: copy the 16 bytes
     * * * * WARNING: if both ipv4 and ipv6 are present,
     * * * *          17 bytes of offset should be applied for ipv4
     * * * * NOTE: an ipv6 prefix length is prepend
     * * * * NOTE: in Rel.8 the prefix length has a default value of /64
     */
    paa->ipv6_prefix_length = ieValue[1];

    memcpy (paa->ipv6_address.__in6_u.__u6_addr8, &ieValue[2], 16);
    inet_ntop (AF_INET6, &paa->ipv6_address, ipv6_ascii, INET6_ADDRSTRLEN);
    OAILOG_DEBUG (LOG_SM, "\t- IPv6 addr %s/%u\n", ipv6_ascii, paa->ipv6_prefix_length);
  }

  if (paa->pdn_type == 3) {
    offset = 17;
  }

  if (paa->pdn_type & 0x1) {
    uint32_t ip = (((uint32_t)ieValue[1 + offset]) << 24) |
                  (((uint32_t)ieValue[2 + offset]) << 16) |
                  (((uint32_t)ieValue[3 + offset]) << 8) |
                   ((uint32_t)ieValue[4 + offset]);

    paa->ipv4_address.s_addr = htonl(ip);
    char ipv4[INET_ADDRSTRLEN];
    inet_ntop (AF_INET, (void*)&paa->ipv4_address, ipv4, INET_ADDRSTRLEN);
    OAILOG_DEBUG (LOG_SM, "\t- IPv4 addr %s\n", ipv4);
  }

  paa->pdn_type -= 1;
  return NW_OK;
}

//------------------------------------------------------------------------------
int
gtpv2c_paa_ie_set (
  nw_gtpv2c_msg_handle_t * msg,
  const paa_t * paa)
{
  /*
   * ipv4 address = 4 + ipv6 address = 16 + ipv6 prefix length = 1
   * * * * + pdn_type = 1
   * * * * = maximum of 22 bytes
   */
  uint8_t                                 temp[22];
  uint8_t                                 pdn_type;
  uint8_t                                 offset = 0;
  nw_rc_t                                   rc;

  DevAssert (paa );
  pdn_type = paa->pdn_type + 1;
  temp[offset] = pdn_type;
  offset++;

  if (pdn_type & 0x2) {
    /*
     * If ipv6 or ipv4v6 present
     */
    temp[1] = paa->ipv6_prefix_length;
    memcpy (&temp[2], paa->ipv6_address.__in6_u.__u6_addr8, 16);
    offset += 17;
  }

  if (pdn_type & 0x1) {
    uint32_t hbo = ntohl(paa->ipv4_address.s_addr);
    temp[offset++] = (uint8_t)(hbo >> 24);
    temp[offset++] = (uint8_t)(hbo >> 16);
    temp[offset++] = (uint8_t)(hbo >> 8);
    temp[offset++] = (uint8_t)hbo;
  }

  rc = nwGtpv2cMsgAddIe (*msg, NW_GTPV2C_IE_PAA, offset, 0, temp);
  DevAssert (NW_OK == rc);
  return RETURNok;
}

//------------------------------------------------------------------------------
int
gtpv2c_ebi_ie_set (
  nw_gtpv2c_msg_handle_t * msg,
  const unsigned ebi,
  const uint8_t   instance)
{
  nw_rc_t                                   rc;
  uint8_t                                 value = 0;

  value = ebi & 0x0F;
  rc = nwGtpv2cMsgAddIe (*msg, NW_GTPV2C_IE_EBI, 1, instance, &value);
  DevAssert (NW_OK == rc);
  return RETURNok;
}

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_ebi_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  uint8_t                                *ebi = (uint8_t *) arg;

  DevAssert (ebi );
  *ebi = ieValue[0] & 0x0F;
  OAILOG_DEBUG (LOG_SM, "\t- EBI %u\n", *ebi);
  return NW_OK;
}

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_ambr_ie_set (
    nw_gtpv2c_msg_handle_t * msg, ambr_t * ambr)
{
  nw_rc_t                                   rc;

  DevAssert (msg );
  DevAssert (ambr );
  /*
   * MCC Decimal | MCC Hundreds
   */

  uint8_t                                 ambr_br[8];
  uint8_t                                 *p_ambr;
  p_ambr = ambr_br;

  memset(ambr_br, 0, 8);

  INT32_TO_BUFFER((ambr->br_ul/1000), p_ambr);
  p_ambr+=4;

  INT32_TO_BUFFER(((ambr->br_dl/1000)), p_ambr);
  // todo: byte order?

  rc = nwGtpv2cMsgAddIe (*msg, NW_GTPV2C_IE_AMBR, 8, 0, ambr_br);
  DevAssert (NW_OK == rc);
  return RETURNok;
}

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_ambr_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  ambr_t                                 *ambr = (ambr_t *) arg;

  DevAssert (ambr );

  BUFFER_TO_INT32(ieValue, ambr->br_ul);
  BUFFER_TO_INT32((ieValue+4), ambr->br_dl);
  ambr->br_dl *=1000;
  ambr->br_ul *=1000;
  OAILOG_DEBUG (LOG_SM, "\t- AMBR UL %" PRIu64 "\n", ambr->br_ul);
  OAILOG_DEBUG (LOG_SM, "\t- AMBR DL %" PRIu64 "\n", ambr->br_dl);
  return NW_OK;
}

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_bearer_qos_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  bearer_qos_t                       *bearer_qos = (bearer_qos_t *) arg;

  DevAssert (bearer_qos );

  if (22 <= ieLength) {
    int index = 0;
    bearer_qos->pci = (ieValue[index] >> 6) & 0x01;
    bearer_qos->pci = (ieValue[index] >> 6) & 0x01;
    bearer_qos->pl  = (ieValue[index] >> 2) & 0x0F;
    bearer_qos->pvi = ieValue[index++] & 0x01;
    bearer_qos->qci = ieValue[index++];

    bearer_qos->mbr.br_ul = ((bit_rate_t)ieValue[index++]) << 32;
    bearer_qos->mbr.br_ul |= (((bit_rate_t)ieValue[index++]) << 24);
    bearer_qos->mbr.br_ul |= (((bit_rate_t)ieValue[index++]) << 16);
    bearer_qos->mbr.br_ul |= (((bit_rate_t)ieValue[index++]) << 8);
    bearer_qos->mbr.br_ul |= (bit_rate_t)ieValue[index++];
    bearer_qos->mbr.br_ul *=1000;

    bearer_qos->mbr.br_dl = ((bit_rate_t)ieValue[index++]) << 32;
    bearer_qos->mbr.br_dl |= (((bit_rate_t)ieValue[index++]) << 24);
    bearer_qos->mbr.br_dl |= (((bit_rate_t)ieValue[index++]) << 16);
    bearer_qos->mbr.br_dl |= (((bit_rate_t)ieValue[index++]) << 8);
    bearer_qos->mbr.br_dl |= (bit_rate_t)ieValue[index++];
    bearer_qos->mbr.br_dl *=1000;

    bearer_qos->gbr.br_ul = ((bit_rate_t)ieValue[index++]) << 32;
    bearer_qos->gbr.br_ul |= (((bit_rate_t)ieValue[index++]) << 24);
    bearer_qos->gbr.br_ul |= (((bit_rate_t)ieValue[index++]) << 16);
    bearer_qos->gbr.br_ul |= (((bit_rate_t)ieValue[index++]) << 8);
    bearer_qos->gbr.br_ul |= (bit_rate_t)ieValue[index++];
    bearer_qos->gbr.br_ul *=1000;

    bearer_qos->gbr.br_dl = ((bit_rate_t)ieValue[index++]) << 32;
    bearer_qos->gbr.br_dl |= (((bit_rate_t)ieValue[index++]) << 24);
    bearer_qos->gbr.br_dl |= (((bit_rate_t)ieValue[index++]) << 16);
    bearer_qos->gbr.br_dl |= (((bit_rate_t)ieValue[index++]) << 8);
    bearer_qos->gbr.br_dl |= (bit_rate_t)ieValue[index++];
    bearer_qos->gbr.br_dl *=1000;

    if (22 < ieLength) {
      OAILOG_ERROR (LOG_SM, "TODO gtpv2c_bearer_qos_ie_get() BearerQOS_t\n");
      return NW_GTPV2C_IE_INCORRECT;
    }
    return NW_OK;
  } else {
    OAILOG_ERROR (LOG_SM, "Bad IE length %"PRIu8"\n", ieLength);
    return NW_GTPV2C_IE_INCORRECT;
  }
}

//------------------------------------------------------------------------------
int
gtpv2c_bearer_qos_ie_set (
  nw_gtpv2c_msg_handle_t * msg,
  const bearer_qos_t * bearer_qos)
{
  nw_rc_t                                   rc;
  uint8_t                                 value[22];
  int                                     index = 0;

  DevAssert (msg );
  DevAssert (bearer_qos );
  value[index++] = (bearer_qos->pci << 6) | (bearer_qos->pl << 2) | (bearer_qos->pvi);
  value[index++] = bearer_qos->qci;
  /*
   * TODO: check endianness
   */
  value[index++] = ((bearer_qos->mbr.br_ul/1000) & 0xFF00000000) >> 32;
  value[index++] = ((bearer_qos->mbr.br_ul/1000) & 0x00FF000000) >> 24;
  value[index++] = ((bearer_qos->mbr.br_ul/1000) & 0x0000FF0000) >> 16;
  value[index++] = ((bearer_qos->mbr.br_ul/1000) & 0x000000FF00) >> 8;
  value[index++] = ((bearer_qos->mbr.br_ul/1000) & 0x00000000FF);

  value[index++] = ((bearer_qos->mbr.br_dl/1000) & 0xFF00000000) >> 32;
  value[index++] = ((bearer_qos->mbr.br_dl/1000) & 0x00FF000000) >> 24;
  value[index++] = ((bearer_qos->mbr.br_dl/1000) & 0x0000FF0000) >> 16;
  value[index++] = ((bearer_qos->mbr.br_dl/1000) & 0x000000FF00) >> 8;
  value[index++] = ((bearer_qos->mbr.br_dl/1000) & 0x00000000FF);

  value[index++] = ((bearer_qos->gbr.br_ul/1000) & 0xFF00000000) >> 32;
  value[index++] = ((bearer_qos->gbr.br_ul/1000) & 0x00FF000000) >> 24;
  value[index++] = ((bearer_qos->gbr.br_ul/1000) & 0x0000FF0000) >> 16;
  value[index++] = ((bearer_qos->gbr.br_ul/1000) & 0x000000FF00) >> 8;
  value[index++] = ((bearer_qos->gbr.br_ul/1000) & 0x00000000FF);

  value[index++] = ((bearer_qos->gbr.br_dl/1000) & 0xFF00000000) >> 32;
  value[index++] = ((bearer_qos->gbr.br_dl/1000) & 0x00FF000000) >> 24;
  value[index++] = ((bearer_qos->gbr.br_dl/1000) & 0x0000FF0000) >> 16;
  value[index++] = ((bearer_qos->gbr.br_dl/1000) & 0x000000FF00) >> 8;
  value[index++] = ((bearer_qos->gbr.br_dl/1000) & 0x00000000FF);

  rc = nwGtpv2cMsgAddIe (*msg, NW_GTPV2C_IE_BEARER_LEVEL_QOS, 22, 0, value);
  DevAssert (NW_OK == rc);
  return RETURNok;
}

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_bearer_context_created_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  bearer_contexts_created_t              *bearer_contexts = (bearer_contexts_created_t *) arg;
  DevAssert (bearer_contexts);
  DevAssert (0 <= bearer_contexts->num_bearer_context);
  DevAssert (MSG_CREATE_SESSION_REQUEST_MAX_BEARER_CONTEXTS >= bearer_contexts->num_bearer_context);
  bearer_context_created_t               *bearer_context = &bearer_contexts->bearer_context[bearer_contexts->num_bearer_context];
  uint16_t                                 read = 0;
  nw_rc_t                                   rc;

  while (ieLength > read) {
    nw_gtpv2c_ie_tlv_t                         *ie_p;

    ie_p = (nw_gtpv2c_ie_tlv_t *) & ieValue[read];

    switch (ie_p->t) {
    case NW_GTPV2C_IE_EBI:
      rc = gtpv2c_ebi_ie_get (ie_p->t, ntohs (ie_p->l), ie_p->i, &ieValue[read + sizeof (nw_gtpv2c_ie_tlv_t)], &bearer_context->eps_bearer_id);
      DevAssert (NW_OK == rc);
      break;

//    case NW_GTPV2C_IE_FTEID:
//      rc = gtpv2c_fteid_ie_get (ie_p->t, ntohs (ie_p->l), ie_p->i, &ieValue[read + sizeof (nw_gtpv2c_ie_tlv_t)], &bearer_context->s1u_sgw_fteid);
//      break;
    case NW_GTPV2C_IE_FTEID:
      switch (ie_p->i) {
      case 0:
        rc = gtpv2c_fteid_ie_get (ie_p->t, ntohs (ie_p->l), ie_p->i, &ieValue[read + sizeof (nw_gtpv2c_ie_tlv_t)], &bearer_context->s1u_sgw_fteid);
        break;
      case 1:
        rc = gtpv2c_fteid_ie_get (ie_p->t, ntohs (ie_p->l), ie_p->i, &ieValue[read + sizeof (nw_gtpv2c_ie_tlv_t)], &bearer_context->s4u_sgw_fteid);
        break;
      case 2:
        rc = gtpv2c_fteid_ie_get (ie_p->t, ntohs (ie_p->l), ie_p->i, &ieValue[read + sizeof (nw_gtpv2c_ie_tlv_t)], &bearer_context->s5_s8_u_pgw_fteid);
        break;
      case 3:
        rc = gtpv2c_fteid_ie_get (ie_p->t, ntohs (ie_p->l), ie_p->i, &ieValue[read + sizeof (nw_gtpv2c_ie_tlv_t)], &bearer_context->s12_sgw_fteid);
        break;
      default:
        OAILOG_ERROR (LOG_SM, "Received unexpected IE %u instance %u\n", ie_p->t, ie_p->i);
        return NW_GTPV2C_IE_INCORRECT;

      }
      DevAssert (NW_OK == rc);
      break;

    case NW_GTPV2C_IE_CAUSE:
      rc = gtpv2c_cause_ie_get (ie_p->t, ntohs (ie_p->l), ie_p->i, &ieValue[read + sizeof (nw_gtpv2c_ie_tlv_t)], &bearer_context->cause);
      break;

    case NW_GTPV2C_IE_BEARER_LEVEL_QOS:
       rc = gtpv2c_bearer_qos_ie_get (ie_p->t, ntohs (ie_p->l), ie_p->i, &ieValue[read + sizeof (nw_gtpv2c_ie_tlv_t)], &bearer_context->bearer_level_qos);
       break;

    case NW_GTPV2C_IE_BEARER_FLAGS:
       OAILOG_WARNING (LOG_SM, "Received IE BEARER_FLAGS %u to implement\n", ie_p->t);
      break;

    default:
      OAILOG_ERROR (LOG_SM, "Received unexpected IE %u\n", ie_p->t);
      return NW_GTPV2C_IE_INCORRECT;
    }

    read += (ntohs (ie_p->l) + sizeof (nw_gtpv2c_ie_tlv_t));
  }
  bearer_contexts->num_bearer_context += 1;
  return NW_OK;
}

//------------------------------------------------------------------------------
int
gtpv2c_bearer_context_created_ie_set (
  nw_gtpv2c_msg_handle_t * msg,
  const bearer_context_created_t * const bearer)
{
  nw_rc_t                                   rc;

  DevAssert (msg );
  DevAssert (bearer);
  /*
   * Start section for grouped IE: bearer context created
   */
  rc = nwGtpv2cMsgGroupedIeStart (*msg, NW_GTPV2C_IE_BEARER_CONTEXT, NW_GTPV2C_IE_INSTANCE_ZERO);
  DevAssert (NW_OK == rc);
  gtpv2c_ebi_ie_set (msg, bearer->eps_bearer_id, NW_GTPV2C_IE_INSTANCE_ZERO);
  rc = gtpv2c_cause_ie_set (msg, &bearer->cause);
  DevAssert (NW_OK == rc);
  // todo: data forwarding..
//  rc = nwGtpv2cMsgAddIeFteid (*msg, NW_GTPV2C_IE_INSTANCE_,
//                              bearer->s1u_sgw_fteid.interface_type,
//                              bearer->s1u_sgw_fteid.teid,
//                              bearer->s1u_sgw_fteid.ipv4 ? &bearer->s1u_sgw_fteid.ipv4_address : 0,
//                              bearer->s1u_sgw_fteid.ipv6 ? &bearer->s1u_sgw_fteid.ipv6_address : NULL);
//  DevAssert (NW_OK == rc);
  /*
   * End section for grouped IE: bearer context created
   */
  rc = nwGtpv2cMsgGroupedIeEnd (*msg);
  DevAssert (NW_OK == rc);
  return RETURNok;
}

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_bearer_context_to_be_created_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  bearer_contexts_to_be_created_t         *bearer_contexts = (bearer_contexts_to_be_created_t *) arg;
  DevAssert (bearer_contexts );
  DevAssert (0 <= bearer_contexts->num_bearer_context);
  DevAssert (MSG_CREATE_SESSION_REQUEST_MAX_BEARER_CONTEXTS >= bearer_contexts->num_bearer_context);
  bearer_context_to_be_created_t          *bearer_context  = &bearer_contexts->bearer_context[bearer_contexts->num_bearer_context];
  uint16_t                                 read = 0;
  nw_rc_t                                   rc;

  while (ieLength > read) {
    nw_gtpv2c_ie_tlv_t                         *ie_p;

    ie_p = (nw_gtpv2c_ie_tlv_t *) & ieValue[read];

    switch (ie_p->t) {
    case NW_GTPV2C_IE_EBI:
      rc = gtpv2c_ebi_ie_get (ie_p->t, ntohs (ie_p->l), ie_p->i, &ieValue[read + sizeof (nw_gtpv2c_ie_tlv_t)], &bearer_context->eps_bearer_id);
      DevAssert (NW_OK == rc);
      break;

    case NW_GTPV2C_IE_BEARER_LEVEL_QOS:
      rc = gtpv2c_bearer_qos_ie_get (ie_p->t, ntohs (ie_p->l), ie_p->i, &ieValue[read + sizeof (nw_gtpv2c_ie_tlv_t)], &bearer_context->bearer_level_qos);
      break;

    case NW_GTPV2C_IE_BEARER_TFT:
      OAILOG_ERROR (LOG_SM, "Received IE %u to implement\n", ie_p->t);
      return NW_GTPV2C_IE_INCORRECT;
      break;

    case NW_GTPV2C_IE_FTEID:
      switch (ie_p->i) {
        case 0:
          rc = gtpv2c_fteid_ie_get (ie_p->t, ntohs (ie_p->l), ie_p->i, &ieValue[read + sizeof (nw_gtpv2c_ie_tlv_t)], &bearer_context->s1u_enb_fteid);
          break;
        case 1:
          rc = gtpv2c_fteid_ie_get (ie_p->t, ntohs (ie_p->l), ie_p->i, &ieValue[read + sizeof (nw_gtpv2c_ie_tlv_t)], &bearer_context->s4u_sgsn_fteid);
          break;
        case 2:
          rc = gtpv2c_fteid_ie_get (ie_p->t, ntohs (ie_p->l), ie_p->i, &ieValue[read + sizeof (nw_gtpv2c_ie_tlv_t)], &bearer_context->s5_s8_u_sgw_fteid);
          break;
        case 3:
          rc = gtpv2c_fteid_ie_get (ie_p->t, ntohs (ie_p->l), ie_p->i, &ieValue[read + sizeof (nw_gtpv2c_ie_tlv_t)], &bearer_context->s5_s8_u_pgw_fteid);
          break;
        case 4:
          rc = gtpv2c_fteid_ie_get (ie_p->t, ntohs (ie_p->l), ie_p->i, &ieValue[read + sizeof (nw_gtpv2c_ie_tlv_t)], &bearer_context->s12_rnc_fteid);
          break;
        case 5:
          rc = gtpv2c_fteid_ie_get (ie_p->t, ntohs (ie_p->l), ie_p->i, &ieValue[read + sizeof (nw_gtpv2c_ie_tlv_t)], &bearer_context->s2b_u_epdg_fteid);
          break;
        default:
          OAILOG_ERROR (LOG_SM, "Received unexpected IE %u instance %u\n", ie_p->t, ie_p->i);
          return NW_GTPV2C_IE_INCORRECT;

      }
      DevAssert (NW_OK == rc);
      break;

    default:
      OAILOG_ERROR (LOG_SM, "Received unexpected IE %u\n", ie_p->t);
      return NW_GTPV2C_IE_INCORRECT;
    }

    read += (ntohs (ie_p->l) + sizeof (nw_gtpv2c_ie_tlv_t));
  }
  bearer_contexts->num_bearer_context += 1;
  return NW_OK;
}

//------------------------------------------------------------------------------
int
gtpv2c_bearer_context_to_be_created_ie_set (
  nw_gtpv2c_msg_handle_t * msg,
  const bearer_context_to_be_created_t * bearer_context)
{
  nw_rc_t                                   rc;

  DevAssert (msg );
  DevAssert (bearer_context );
  /*
   * Start section for grouped IE: bearer context to create
   */
  rc = nwGtpv2cMsgGroupedIeStart (*msg, NW_GTPV2C_IE_BEARER_CONTEXT, NW_GTPV2C_IE_INSTANCE_ZERO);
  DevAssert (NW_OK == rc);
  gtpv2c_ebi_ie_set (msg, bearer_context->eps_bearer_id, NW_GTPV2C_IE_INSTANCE_ZERO);
  gtpv2c_bearer_qos_ie_set(msg, &bearer_context->bearer_level_qos);

  /*
   * End section for grouped IE: bearer context to create
   */
  rc = nwGtpv2cMsgGroupedIeEnd (*msg);
  DevAssert (NW_OK == rc);
  return RETURNok;
}

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_selection_mode_ie_set (
    nw_gtpv2c_msg_handle_t * msg, SelectionMode_t * sm)
{
  DevAssert (msg );
  DevAssert (sm );
  uint8_t val8 = *sm;
  nw_rc_t rc = nwGtpv2cMsgAddIe (*msg, NW_GTPV2C_IE_SELECTION_MODE, 1, 0, &val8);
  DevAssert (NW_OK == rc);
  return NW_OK;
}

//------------------------------------------------------------------------------
nw_rc_t
gtpv2c_selection_mode_ie_get (
  uint8_t ieType,
  uint16_t ieLength,
  uint8_t ieInstance,
  uint8_t * ieValue,
  void *arg)
{
  SelectionMode_t                  *sm = (SelectionMode_t *) arg;

  DevAssert (sm);
  *sm = (ieValue[0] & 0x03);
  OAILOG_DEBUG (LOG_GTPV2C, "\t- Selection Mode  %d\n", *sm);
  return NW_OK;
}


