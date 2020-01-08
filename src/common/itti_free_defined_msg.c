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

/*! \file itti_free_defined_msg.c
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

#include "dynamic_memory_check.h"
#include "assertions.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_29.274.h"
#include "3gpp_36.331.h"
#include "security_types.h"
#include "common_types.h"
#include "common_defs.h"
#include "intertask_interface.h"
#include "itti_free_defined_msg.h"

//------------------------------------------------------------------------------
void itti_free_msg_content (MessageDef * const message_p)
{
  switch (ITTI_MSG_ID (message_p)) {
  case ASYNC_SYSTEM_COMMAND:{
      if (ASYNC_SYSTEM_COMMAND (message_p).system_command) {
        bdestroy_wrapper(&ASYNC_SYSTEM_COMMAND (message_p).system_command);
      }
    }
    break;

  case SCTP_INIT_MSG:
    // DO nothing (ipv6_address statically allocated)
    break;

  case SCTP_DATA_REQ:
    bdestroy_wrapper (&message_p->ittiMsg.sctp_data_req.payload);
    AssertFatal(NULL == message_p->ittiMsg.sctp_data_req.payload, "TODO clean pointer");
    break;

  case SCTP_DATA_IND:
    bdestroy_wrapper (&message_p->ittiMsg.sctp_data_ind.payload);
    AssertFatal(NULL == message_p->ittiMsg.sctp_data_ind.payload, "TODO clean pointer");
    break;

  case SCTP_DATA_CNF:
  case SCTP_NEW_ASSOCIATION:
  case SCTP_CLOSE_ASSOCIATION:
    // DO nothing
    break;

  case UDP_INIT:
  case UDP_DATA_REQ:
  case UDP_DATA_IND:
    /** Changed to stacked buffer. */
   break;

   /**
     * Sm Messages
     */
  case SM_MBMS_SESSION_START_REQUEST:
  case SM_MBMS_SESSION_STOP_REQUEST:
  case SM_MBMS_SESSION_UPDATE_REQUEST:
  	break;
  case SM_MBMS_SESSION_START_RESPONSE:
  case SM_MBMS_SESSION_UPDATE_RESPONSE:
  case SM_MBMS_SESSION_STOP_RESPONSE:
    break;
  default:
    ;
  }
}
