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
#ifndef FILE_MCE_APP_PROCEDURES_SEEN
#define FILE_MCE_APP_PROCEDURES_SEEN

#include "queue.h"

typedef void (*time_out_t)(void *arg);
#define MCE_APP_TIMER_INACTIVE_ID   (-1)

// todo: #define MCE_APP_INITIAL_CONTEXT_SETUP_RSP_TIMER_VALUE 2 // In seconds
/* Timer structure */
struct mce_app_timer_s {
  long id;         /* The timer identifier                 */
  long sec;       /* The timer interval value in seconds  */
};

/*! \file mme_app_procedures.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

//typedef int (*mme_app_pdu_in_resp_t)(void *arg);
//typedef int (*mme_app_pdu_in_rej_t)(void *arg);
//typedef int (*mme_app_time_out_t)(void *arg);
//typedef int (*mme_app_sdu_out_not_delivered_t)(void *arg);

typedef enum {
  MCE_APP_BASE_PROC_TYPE_NONE = 0,
  MCE_APP_BASE_PROC_TYPE_MBMS
} mce_app_base_proc_type_t;


typedef struct mce_app_base_proc_s {
  // PDU interface
  //pdu_in_resp_t              resp_in;
  //pdu_in_rej_t               fail_in;
  time_out_t                 time_out;
  mce_app_base_proc_type_t   type;
  bool						 in_progress;
  int						 in_progress_count;
} mce_app_base_proc_t;

typedef struct mce_app_mbms_proc_s {
  mce_app_base_proc_t           		proc;
  bool							 								trigger_mbms_session_stop;

  uintptr_t                      		sm_trxn;
  struct mce_app_timer_s         		timer;
  LIST_ENTRY(mce_app_mbms_proc_s)  	entries;      /* List. */
} mce_app_mbms_proc_t;

#endif /** FILE_MCE_APP_PROCEDURES_SEEN **/
