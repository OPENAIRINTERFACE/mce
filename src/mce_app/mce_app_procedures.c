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

/*! \file mce_app_procedures.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "common_types.h"
#include "intertask_interface.h"
#include "timer.h"
#include "mce_config.h"
#include "mce_app_extern.h"
#include "mce_app_mbms_service_context.h"
#include "mce_app_defs.h"
#include "mce_app_defs.h"
#include "common_defs.h"

/**
 * MBMS Procedures.
 */
//------------------------------------------------------------------------------
mce_app_mbms_proc_t * mce_app_create_mbms_procedure(mbms_service_t * const mbms_service,
  long abs_start_time_in_sec, long abs_start_time_usec, const mbms_session_duration_t * const mbms_session_duration)
{
  OAILOG_FUNC_IN (LOG_MME_APP);

  mce_app_mbms_proc_t 				*mbms_proc    = NULL;
  /** Check if the list of Sm procedures is empty. */
  if(mbms_service->mbms_procedure){
  	OAILOG_ERROR (LOG_MME_APP, "MBMS Service with TMGI " TMGI_FMT " has already a MBMS procedure ongoing. Cannot create new MBMS procedure. \n",
  			TMGI_ARG(&mbms_service->privates.fields.tmgi));
  	OAILOG_FUNC_RETURN(LOG_MME_APP, NULL);
  }

  /**
   * Times of MBMS session start and stop.
   */
  mbms_proc = calloc(1, sizeof(mce_app_mbms_proc_t));
  mbms_proc->proc.type = MCE_APP_BASE_PROC_TYPE_MBMS;
  /** Get the MBMS Service Index. */
  mbms_service_index_t mbms_service_idx = mce_get_mbms_service_index(&mbms_service->privates.fields.tmgi, mbms_service->privates.fields.mbms_service_area_id);
  DevAssert(mbms_service_idx != INVALID_MBMS_SERVICE_INDEX);
  mce_config_read_lock (&mce_config);
  /**
   * Set the timer, depending on the duration of the MBMS Session Duration and the configuration,
   * the MBMS Session may be terminated, or just checked for existence (clear-up).
   * Mark the MME procedure, depending on the timeout (we won't know after wakeup).
   * We don't include the absolute start time here.
   */
  if(!mbms_session_duration) {	/**< If this is a stop procedure. */
  	mbms_proc->trigger_mbms_session_stop = true;
  	DevAssert(abs_start_time_in_sec);
  }
  else if(mce_config.mbms.mbms_short_idle_session_duration_in_sec >= mbms_session_duration->seconds){
  	OAILOG_INFO(LOG_MME_APP, "MBMS Session procedure for MBMS Service-Index " MBMS_SERVICE_INDEX_FMT " has session duration (%ds) is shorter/equal than the minimum (%ds). \n",
  			mbms_service_idx, mbms_session_duration->seconds, mce_config.mbms.mbms_short_idle_session_duration_in_sec);
  	mbms_proc->trigger_mbms_session_stop = true;
  }
  mce_config_unlock (&mce_config);

//  /**
//   * If the ABS time is not to be considered, too short, we can also check, if we can have a single session duration.
//   */
//  if(!*mbms_abs_start_time) {
//    /**
//	 * If the MBMS Session Duration is not 1 second longer than the current new Sm process timeout,
//	 * and the absolute start time is not to be considered, set the duration as null and also set the stop_trigger.
//	 */
//	if(sm_check_timeout > mbms_session_duration->seconds) {
//	  /** Set the MBMS Stop Trigger and no additional timer for the MBMS Session Duration. */
//	  sm_proc_mbms_session_start->proc.trigger_mbms_session_stop = true;
//	} else {
//	  /** We will trigger another timer for the duration afterwards. It may also have an implicit stop trigger. */
//	  sm_proc_mbms_session_start->proc.mbms_session_duration.seconds = mbms_session_duration->seconds;
//	}
//  } else {
//	/**
//	 * Set the duration as the combination of both.
//	 * Immediately stop the the MBMS session afterwards.
//	 * The M2AP layer, does not need to get notified about the MBMS Session duration or the absolute start time.
//	 * eNB does not need to know.
//	 */
//	sm_proc_mbms_session_start->proc.mbms_session_duration.seconds = mbms_session_duration->seconds + delta_to_start;
//  }

  /**
   * Set the implicit removal later after the first timeout.
   * The first timeout should always check bearers.
   * If the
   */
  long delta_to_start_in_sec = abs_start_time_in_sec - time(NULL); /**< Time since EPOCH. */
  if (timer_setup (mbms_session_duration->seconds + delta_to_start_in_sec, abs_start_time_usec,
    TASK_MCE_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,  (void *) (mbms_service_idx), &(mbms_proc->timer.id)) < 0)
  {
	  OAILOG_ERROR (LOG_MME_APP, "Failed to start the MME MBMS Session timer for MBMS Service Idx " MBMS_SERVICE_INDEX_FMT " for duration %ds \n",
	  		mbms_service_idx, mbms_session_duration->seconds);
	  mbms_proc->timer.id = MCE_APP_TIMER_INACTIVE_ID;
	  mbms_proc->trigger_mbms_session_stop = false;
	  free_wrapper(&mbms_proc);
	  OAILOG_FUNC_RETURN(LOG_MME_APP, NULL);
  }
  mbms_service->mbms_procedure = mbms_proc;
  /** Initialize the of the procedure. */
  OAILOG_FUNC_RETURN(LOG_MME_APP, mbms_proc);
}

//------------------------------------------------------------------------------
void mce_app_delete_mbms_procedure(const mbms_service_t * mbms_service)
{
  if(!mbms_service)
    return;
  if(!mbms_service->mbms_procedure)
	return;

  if(mbms_service->mbms_procedure->timer.id != MCE_APP_TIMER_INACTIVE_ID){
    if (timer_remove(mbms_service->mbms_procedure->timer.id, NULL)) {
      OAILOG_ERROR (LOG_MME_APP, "Failed to stop the procedure timer for -MMME MBMS Service Start with TMGI " TMGI_FMT ". \n", TMGI_ARG(&mbms_service->privates.fields.tmgi));
      mbms_service->mbms_procedure->timer.id = MCE_APP_TIMER_INACTIVE_ID;
    }else{
      OAILOG_DEBUG(LOG_MME_APP, "Successfully removed timer for -MMME MBMS Service Start for TMGI " TMGI_FMT ". \n", TMGI_ARG(&mbms_service->privates.fields.tmgi));
    }
  }
  free_wrapper(&mbms_service->mbms_procedure);
  return;
}
