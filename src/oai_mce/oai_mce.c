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

/*! \file oai_mce.c
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <stdio.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>

#if HAVE_CONFIG_H
#  include "config.h"
#endif

#include "bstrlib.h"


#include "log.h"
#include "dynamic_memory_check.h"
#include "assertions.h"
#include "msc.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "security_types.h"
#include "common_types.h"
#include "common_defs.h"
#include "pid_file.h"

#include "intertask_interface_init.h"

#include "oai_mce.h"

#include "sctp_primitives_server.h"
#include "udp_primitives_server.h"
#include "timer.h"
#include "mce_config.h"
#include "mce_app_extern.h"
#include "sm_mce.h"

#include "mce_app_extern.h"

int
main (
  int argc,
  char *argv[])
{
  char *pid_dir;
  char *pid_file_name;

  CHECK_INIT_RETURN (shared_log_init (MAX_LOG_PROTOS));
  CHECK_INIT_RETURN (OAILOG_INIT (LOG_SPGW_ENV, OAILOG_LEVEL_DEBUG, MAX_LOG_PROTOS));
  /*
   * Parse the command line for options and set the mce_config accordingly.
   */
  CHECK_INIT_RETURN (mce_config_parse_opt_line (argc, argv, &mce_config));

  pid_dir = bstr2cstr(mce_config.pid_dir, 1);
  pid_dir = pid_dir ? pid_dir : "/var/run";
  pid_file_name = get_exe_absolute_path(pid_dir, mce_config.instance);
  bcstrfree(pid_dir);

#if DAEMONIZE
  pid_t pid, sid; // Our process ID and Session ID

  // Fork off the parent process
  pid = fork();
  if (pid < 0) {
    exit(EXIT_FAILURE);
  }
  // If we got a good PID, then we can exit the parent process.
  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }
  // Change the file mode mask
  umask(0);

  // Create a new SID for the child process
  sid = setsid();
  if (sid < 0) {
    exit(EXIT_FAILURE); // Log the failure
  }

  // Change the current working directory
  if ((chdir("/")) < 0) {
    // Log the failure
    exit(EXIT_FAILURE);
  }

  /* Close out the standard file descriptors */
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);


  if (! is_pid_file_lock_success(pid_file_name)) {
    closelog();
    free_wrapper((void**)&pid_file_name);
    exit (-EDEADLK);
  }
#else
  if (! is_pid_file_lock_success(pid_file_name)) {
    free_wrapper((void**)&pid_file_name);
    exit (-EDEADLK);
  }
#endif

  /*
   * Calling each layer init function
   */
  //CHECK_INIT_RETURN (log_init (&mce_config, oai_mme_log_specific));
  CHECK_INIT_RETURN (itti_init (TASK_MAX, THREAD_MAX, MESSAGES_ID_MAX, tasks_info, messages_info,
#if ENABLE_ITTI_ANALYZER
          messages_definition_xml,
#else
          NULL,
#endif
          NULL));
  MSC_INIT (MSC_MME, THREAD_MAX + TASK_MAX);
  CHECK_INIT_RETURN (sctp_init (&mce_config));
  CHECK_INIT_RETURN (udp_init ());
  OAILOG_DEBUG(LOG_MCE_APP, "MCE app initialization of mandatory interfaces complete.\n");

  /** Initialize M2AP.*/
  CHECK_INIT_RETURN (m2ap_mce_init ());
  CHECK_INIT_RETURN (m3ap_mme_init ());
  /** Initialize SM. */
  CHECK_INIT_RETURN (sm_mce_init (&mce_config));
  /** Initialize MCE_APP. */
  CHECK_INIT_RETURN (mce_app_init (&mce_config));
  /** Activate M2AP layer right away. */
  MessageDef                             *message_p;
  OAILOG_NOTICE(LOG_MCE_APP, "Activating M2AP layer..\n");
  message_p = itti_alloc_new_message (TASK_M2AP, ACTIVATE_MESSAGE);
  itti_send_msg_to_task (TASK_M2AP, INSTANCE_DEFAULT, message_p);
  /** Activate M3AP layer right away. */
  OAILOG_NOTICE(LOG_MCE_APP, "Activating M3AP layer..\n");
  message_p = itti_alloc_new_message (TASK_M3AP, ACTIVATE_MESSAGE);
  itti_send_msg_to_task (TASK_M3AP, INSTANCE_DEFAULT, message_p);

  OAILOG_DEBUG(LOG_MCE_APP, "MCE app initialization of optional interfaces complete. \n");

  /*
   * Handle signals here
   */
  itti_wait_tasks_end ();
  pid_file_unlock();
  free_wrapper((void**)&pid_file_name);
  return 0;
}
