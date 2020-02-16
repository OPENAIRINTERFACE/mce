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

/*! \file common_types.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "security_types.h"
#include "common_types.h"
#include "common_defs.h"
#include "3gpp_29.274.h"

//------------------------------------------------------------------------------
bstring fteid_ip_address_to_bstring(const struct fteid_s * const fteid)
{
  bstring bstr = NULL;
  if (fteid->ipv4) {
    bstr = blk2bstr(&fteid->ipv4_address.s_addr, 4);
  } else if (fteid->ipv6) {
    bstr = blk2bstr(&fteid->ipv6_address, 16);
  } else {
    char avoid_seg_fault[4] = {0,0,0,0};
    bstr = blk2bstr(avoid_seg_fault, 4);
  }
  return bstr;
}

//------------------------------------------------------------------------------
bstring ip_address_to_bstring(ip_address_t *ip_address)
{
  bstring bstr = NULL;
  switch (ip_address->pdn_type) {
  case IPv4:
    bstr = blk2bstr(&ip_address->address.ipv4_address.s_addr, 4);
    break;
  case IPv6:
    bstr = blk2bstr(&ip_address->address.ipv6_address, 16);
    break;
  case IPv4_AND_v6:
    bstr = blk2bstr(&ip_address->address.ipv4_address.s_addr, 4);
    bcatblk(bstr, &ip_address->address.ipv6_address, 16);
    break;
  case IPv4_OR_v6:
    // do it like that now, TODO
    bstr = blk2bstr(&ip_address->address.ipv4_address.s_addr, 4);
    break;
  default:
    ;
  }
  return bstr;
}

//------------------------------------------------------------------------------
void bstring_to_ip_address(bstring const bstr, ip_address_t * const ip_address)
{
  if (bstr) {
    switch (blength(bstr)) {
    case 4:
      ip_address->pdn_type = IPv4;
      memcpy(&ip_address->address.ipv4_address, bstr->data, blength(bstr));
      break;
    case 16:
      ip_address->pdn_type = IPv6;
      memcpy(&ip_address->address.ipv6_address, bstr->data, blength(bstr));
      break;
      break;
    case 20:
      ip_address->pdn_type = IPv4_AND_v6;
      memcpy(&ip_address->address.ipv4_address, bstr->data, 4);
      memcpy(&ip_address->address.ipv6_address, &bstr->data[4], 16);
      break;
    default:
      ;
    }
  }
}

