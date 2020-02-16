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
// This task is mandatory and must always be placed in first position
TASK_DEF(TASK_TIMER,    TASK_PRIORITY_MED, 128)

// Other possible tasks in the process

//MME SCENARIO PLAYER TEST TASK
TASK_DEF(TASK_MME_SCENARIO_PLAYER, TASK_PRIORITY_MED, 256)
/// FW_IP task
TASK_DEF(TASK_FW_IP,    TASK_PRIORITY_MED, 256)
/// MCE Applicative task
TASK_DEF(TASK_MCE_APP,  TASK_PRIORITY_MED, 256)
/// SM task
TASK_DEF(TASK_SM,       TASK_PRIORITY_MED, 256)
/// M2AP task
TASK_DEF(TASK_M2AP, 	TASK_PRIORITY_MED, 256)
/// M3AP task
TASK_DEF(TASK_M3AP, 	TASK_PRIORITY_MED, 256)
/// SCTP task
TASK_DEF(TASK_SCTP,     TASK_PRIORITY_MED, 256)
/// UDP task
TASK_DEF(TASK_UDP,      TASK_PRIORITY_MED, 256)
//LOGGING TXT TASK
TASK_DEF(TASK_LOG,      TASK_PRIORITY_MED, 1024)
//GENERAL PURPOSE SHARED LOGGING TASK
TASK_DEF(TASK_SHARED_TS_LOG, TASK_PRIORITY_MED, 1024)
//UTILITY TASK FOR SYSTEM() CALLS
TASK_DEF(TASK_ASYNC_SYSTEM, TASK_PRIORITY_MED, 256)


