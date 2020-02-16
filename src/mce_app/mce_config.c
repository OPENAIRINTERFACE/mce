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

/*! \file mce_config.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#if HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <libconfig.h>

#include <arpa/inet.h>          /* To provide inet_addr */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "assertions.h"
#include "dynamic_memory_check.h"
#include "log.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "common_defs.h"
#include "mce_config.h"
#include "m2ap_common.h"
#include "m2ap_mce_mbms_sa.h"
#include "m3ap_common.h"

struct mce_config_s                       mce_config = {.rw_lock = PTHREAD_RWLOCK_INITIALIZER, 0};

//------------------------------------------------------------------------------
static
int mce_app_compare_plmn (
  const plmn_t * const plmn)
{
  int                                     i = 0;
  uint16_t                                mcc = 0;
  uint16_t                                mnc = 0;
  uint16_t                                mnc_len = 0;

  DevAssert (plmn != NULL);
  /** Get the integer values from the PLMN. */
  PLMN_T_TO_MCC_MNC ((*plmn), mcc, mnc, mnc_len);

  mce_config_read_lock (&mce_config);

  for (i = 0; i < mce_config.gumcei.nb; i++) {
  	 if(memcmp((void*)plmn, (void*)&mce_config.gumcei.gumcei[i].plmn, sizeof(plmn_t)) == 0){
  	  	mce_config_unlock (&mce_config);
  	  	return MBMS_SA_LIST_AT_LEAST_ONE_MATCH;
  	 }
  }
	mce_config_unlock (&mce_config);
  return MBMS_SA_LIST_NO_MATCH;
}

//------------------------------------------------------------------------------
/* @brief compare a MBMS-SA
 * @return the first matching MBMS Service Area value.
*/
static
mbms_service_area_id_t mme_app_compare_mbms_sa (const mbms_service_area_t * mbms_service_area)
{
  int                                     i = 0, j = 0;
  mce_config_read_lock (&mce_config);
  for (j = 0; j < mbms_service_area->num_service_area; j++) {
    if(mbms_service_area->serviceArea[j] == INVALID_MBMS_SERVICE_AREA_ID){
      OAILOG_ERROR(LOG_MCE_APP, "Skipping invalid MBMS Service Area ID (0). \n");
      continue;
    }
    /** Check if it is a global MBMS SAI. */
    if(mbms_service_area->serviceArea[j] <= mce_config.mbms_global_service_area_types) {
      /** Global MBMS Service Area Id received. */
      OAILOG_INFO(LOG_MCE_APP, "Found a matching global MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT ". \n", mbms_service_area->serviceArea[j]);
      return mbms_service_area->serviceArea[j];
    }
    /** Check if it is in bounds for the local service areas. */
    int val = mbms_service_area->serviceArea[j] - (mce_config.mbms_global_service_area_types +1);
    int local_area = val / mce_config.mbms_local_service_area_types;
    int local_area_type = val % mce_config.mbms_local_service_area_types;
    if(local_area < mce_config.mbms_local_service_area_types){
      OAILOG_INFO(LOG_MCE_APP, "Found a valid MBMS Service Area ID " MBMS_SERVICE_AREA_ID_FMT ". \n", mbms_service_area->serviceArea[j]);
      return mbms_service_area->serviceArea[j];
    }
  }
  mce_config_unlock (&mce_config);
  return INVALID_MBMS_SERVICE_AREA_ID;
}

//------------------------------------------------------------------------------
mbms_service_area_id_t mce_app_check_mbms_sa_exists(const plmn_t * target_plmn, const mbms_service_area_t * mbms_service_area){
  if(MBMS_SA_LIST_AT_LEAST_ONE_MATCH == mce_app_compare_plmn(target_plmn)){
  	mbms_service_area_id_t mbms_service_area_id;
    if((mbms_service_area_id = mme_app_compare_mbms_sa(mbms_service_area)) != INVALID_MBMS_SERVICE_AREA_ID){
      OAILOG_DEBUG (LOG_MCE_APP, "MBMS-SA " MBMS_SERVICE_AREA_ID_FMT " and PLMN " PLMN_FMT " are matching. \n", mbms_service_area_id, PLMN_ARG(target_plmn));
      return mbms_service_area_id;
    }
  }
  OAILOG_DEBUG (LOG_MCE_APP, "MBMS-SA or PLMN " PLMN_FMT " are not matching. \n", PLMN_ARG(target_plmn));
  return INVALID_MBMS_SERVICE_AREA_ID;
}

//------------------------------------------------------------------------------
static void mce_config_init (mce_config_t * config_pP)
{
  memset(&mce_config, 0, sizeof(mce_config));
  pthread_rwlock_init (&config_pP->rw_lock, NULL);
  config_pP->log_config.output             = NULL;
  config_pP->log_config.is_output_thread_safe = false;
  config_pP->log_config.color              = false;
  config_pP->log_config.udp_log_level      = MAX_LOG_LEVEL; // Means invalid
  config_pP->log_config.gtpv2c_log_level   = MAX_LOG_LEVEL;
  config_pP->log_config.sctp_log_level     = MAX_LOG_LEVEL;

  config_pP->log_config.util_log_level     = MAX_LOG_LEVEL;
  /** MBMS Logs. */
  config_pP->log_config.m2ap_log_level   	 = MAX_LOG_LEVEL;
  config_pP->log_config.m3ap_log_level   	 = MAX_LOG_LEVEL;
  config_pP->log_config.mce_app_log_level  = MAX_LOG_LEVEL;
  config_pP->log_config.sm_log_level       = MAX_LOG_LEVEL;
  config_pP->log_config.sm_log_level       = MAX_LOG_LEVEL;

  config_pP->log_config.msc_log_level      = MAX_LOG_LEVEL;
  config_pP->log_config.itti_log_level     = MAX_LOG_LEVEL;

  config_pP->log_config.asn1_verbosity_level = 0;
  config_pP->config_file = NULL;
  config_pP->max_m2_enbs 				= 1;
  config_pP->max_m3_mces 				= 1;
  config_pP->max_mbms_services  = 2;

  config_pP->m2ap_config.port_number = M2AP_PORT_NUMBER;
  config_pP->m2ap_config.outcome_drop_timer_sec = M2AP_OUTCOME_TIMER_DEFAULT;
  config_pP->m3ap_config.port_number = M3AP_PORT_NUMBER;
  config_pP->m3ap_config.outcome_drop_timer_sec = M3AP_OUTCOME_TIMER_DEFAULT;

  /*
   * IP configuration
   */
  config_pP->ip.if_name_mc = NULL;
  config_pP->ip.mc_mce_v4.s_addr = INADDR_ANY;
  config_pP->ip.mc_mce_v6 = in6addr_any;
  config_pP->ip.if_name_mc = NULL;
  config_pP->ip.port_sm = 2123;

  config_pP->itti_config.queue_size = ITTI_QUEUE_MAX_ELEMENTS;
  config_pP->itti_config.log_file = NULL;
  config_pP->sctp_config.in_streams = SCTP_IN_STREAMS;
  config_pP->sctp_config.out_streams = SCTP_OUT_STREAMS;
  config_pP->relative_capacity = RELATIVE_CAPACITY;
  config_pP->mce_statistic_timer = MCE_STATISTIC_TIMER_S;

  config_pP->gumcei.gumcei[0].mce_gid = GLOBAL_MCE_ID;
  config_pP->gumcei.gumcei[0].plmn.mcc_digit1 = 0;
  config_pP->gumcei.gumcei[0].plmn.mcc_digit1 = 0;
  config_pP->gumcei.gumcei[0].plmn.mcc_digit2 = 0;
  config_pP->gumcei.gumcei[0].plmn.mcc_digit3 = 1;
  config_pP->gumcei.gumcei[0].plmn.mnc_digit1 = 0;
  config_pP->gumcei.gumcei[0].plmn.mnc_digit2 = 1;
  config_pP->gumcei.gumcei[0].plmn.mnc_digit3 = 0x0F;

  /*
   * Set the MBMS Area Parameters
   */
  config_pP->mbms_global_service_area_types = GLOBAL_MBMS_AREAS;
  config_pP->mbms_local_service_areas				= LOCAL_MBMS_AREAS;
  config_pP->mbms_local_service_area_types	= LOCAL_MBMS_AREA_TYPES;

  /*
   * Set default values for remaining parameter.
   */
  config_pP->max_mbsfn_area_per_m2_enb 			= MAX_MBSFN_AREA_PER_M2_ENB;
}

//------------------------------------------------------------------------------
void mce_config_exit (void)
{
  pthread_rwlock_destroy (&mce_config.rw_lock);
  bdestroy_wrapper(&mce_config.log_config.output);
  bdestroy_wrapper(&mce_config.config_file);

  /*
   * IP configuration
   */
  bdestroy_wrapper(&mce_config.ip.if_name_mc);
  bdestroy_wrapper(&mce_config.itti_config.log_file);

  for (int i = 0; i < mce_config.e_dns_emulation.nb_service_entries; i++) {
    bdestroy_wrapper(&mce_config.e_dns_emulation.service_id[i]);
  }

}
//------------------------------------------------------------------------------
static int mce_config_parse_file (mce_config_t * config_pP)
{
  config_t                                cfg = {0};
  config_setting_t                       *setting_mce = NULL;
  config_setting_t                       *setting = NULL;
  config_setting_t                       *subsetting = NULL;
  config_setting_t                       *sub2setting = NULL;
  int                                     aint = 0;
  double                                  adouble  = 0;
  int                                     aint_mc = 0;

  int                                     i = 0,n = 0,
                                          stop_index = 0,
                                          num = 0;
  const char                             *astring = NULL;
  const char                             *tac = NULL;
  const char                             *mcc = NULL;
  const char                             *mnc = NULL;
  const char                             *mbms_sa = NULL;

  char                                   *if_name_mc = NULL;
  char                                   *mc_mce_v4 = NULL;
  char                                   *mc_mce_v6 = NULL;

  bool                                    swap = false;
  bstring                                 address = NULL;
  bstring                                 cidr = NULL;
  bstring                                 mask = NULL;
  struct in_addr                          in_addr_var = {0};
  struct in6_addr                         in6_addr_var = {0};

  config_init (&cfg);

  if (config_pP->config_file != NULL) {
    /*
     * Read the file. If there is an error, report it and exit.
     */
    if (!config_read_file (&cfg, bdata(config_pP->config_file))) {
      OAILOG_ERROR (LOG_CONFIG, ": %s:%d - %s\n", bdata(config_pP->config_file), config_error_line (&cfg), config_error_text (&cfg));
      config_destroy (&cfg);
      AssertFatal (1 == 0, "Failed to parse MCE configuration file %s!\n", bdata(config_pP->config_file));
    }
  } else {
    OAILOG_ERROR (LOG_CONFIG, " No MCE configuration file provided!\n");
    config_destroy (&cfg);
    AssertFatal (0, "No MCE configuration file provided!\n");
  }

  setting_mce = config_lookup (&cfg, MCE_CONFIG_STRING_MCE_CONFIG);

  if (setting_mce != NULL) {
    // LOGGING setting
    setting = config_setting_get_member (setting_mce, LOG_CONFIG_STRING_LOGGING);
    if (setting != NULL) {
      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_OUTPUT, (const char **)&astring)) {
        if (astring != NULL) {
          if (config_pP->log_config.output) {
            bassigncstr(config_pP->log_config.output , astring);
          } else {
            config_pP->log_config.output = bfromcstr(astring);
          }
        }
      }

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_OUTPUT_THREAD_SAFE, (const char **)&astring)) {
        if (astring != NULL) {
          if (strcasecmp (astring, "yes") == 0) {
            config_pP->log_config.is_output_thread_safe = true;
          } else {
            config_pP->log_config.is_output_thread_safe = false;
          }
        }
      }

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_COLOR, (const char **)&astring)) {
        if (0 == strcasecmp("yes", astring)) config_pP->log_config.color = true;
        else config_pP->log_config.color = false;
      }

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_SCTP_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.sctp_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_M2AP_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.m2ap_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_M3AP_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.m3ap_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_MCE_APP_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.mce_app_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_GTPV2C_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.gtpv2c_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_UDP_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.udp_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_SM_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.sm_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_UTIL_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.util_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_MSC_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.msc_log_level = OAILOG_LEVEL_STR2INT (astring);

      if (config_setting_lookup_string (setting, LOG_CONFIG_STRING_ITTI_LOG_LEVEL, (const char **)&astring))
        config_pP->log_config.itti_log_level = OAILOG_LEVEL_STR2INT (astring);

      if ((config_setting_lookup_string (setting_mce, MCE_CONFIG_STRING_ASN1_VERBOSITY, (const char **)&astring))) {
        if (strcasecmp (astring, MCE_CONFIG_STRING_ASN1_VERBOSITY_NONE) == 0)
          config_pP->log_config.asn1_verbosity_level = 0;
        else if (strcasecmp (astring, MCE_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING) == 0)
          config_pP->log_config.asn1_verbosity_level = 2;
        else if (strcasecmp (astring, MCE_CONFIG_STRING_ASN1_VERBOSITY_INFO) == 0)
          config_pP->log_config.asn1_verbosity_level = 1;
        else
          config_pP->log_config.asn1_verbosity_level = 0;
      }
    }

    // GENERAL MCE SETTINGS
    if ((config_setting_lookup_string (setting_mce,
                                       MCE_CONFIG_STRING_PID_DIRECTORY,
                                       (const char **)&astring))) {
      config_pP->pid_dir = bfromcstr (astring);
    }

    if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_STRING_INSTANCE, &aint))) {
      config_pP->instance = (uint32_t) aint;
    }

    if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_STRING_RELATIVE_CAPACITY, &aint))) {
      config_pP->relative_capacity = (uint8_t) aint;
    }

    if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_STRING_STATISTIC_TIMER, &aint))) {
      config_pP->mce_statistic_timer = (uint32_t) aint;
    }

    /*
     * Other configurations..
     */

    // ITTI SETTING
    setting = config_setting_get_member (setting_mce, MCE_CONFIG_STRING_INTERTASK_INTERFACE_CONFIG);

    if (setting != NULL) {
      if ((config_setting_lookup_int (setting, MCE_CONFIG_STRING_INTERTASK_INTERFACE_QUEUE_SIZE, &aint))) {
        config_pP->itti_config.queue_size = (uint32_t) aint;
      }
    }
    // SCTP SETTING
    setting = config_setting_get_member (setting_mce, MCE_CONFIG_STRING_SCTP_CONFIG);

    if (setting != NULL) {
      if ((config_setting_lookup_int (setting, MCE_CONFIG_STRING_SCTP_INSTREAMS, &aint))) {
        config_pP->sctp_config.in_streams = (uint16_t) aint;
      }

      if ((config_setting_lookup_int (setting, MCE_CONFIG_STRING_SCTP_OUTSTREAMS, &aint))) {
        config_pP->sctp_config.out_streams = (uint16_t) aint;
      }
    }

  // GUMCEI SETTING
  setting = config_setting_get_member (setting_mce, MCE_CONFIG_STRING_GUMCEI_LIST);
  config_pP->gumcei.nb = 0;
  if (setting != NULL) {
  	num = config_setting_length (setting);
  	AssertFatal(num == 1, "Only one GUMCEI supported for this version of MME");
  	for (i = 0; i < num; i++) {
  		sub2setting = config_setting_get_elem (setting, i);
  		if (sub2setting != NULL) {
  			if ((config_setting_lookup_string (sub2setting, MCE_CONFIG_STRING_MCC, &mcc))) {
  				AssertFatal( 3 == strlen(mcc), "Bad MCC length, it must be 3 digit ex: 001");
  				char c[2] = { mcc[0], 0};
  				config_pP->gumcei.gumcei[i].plmn.mcc_digit1 = (uint8_t) atoi (c);
  				c[0] = mcc[1];
  				config_pP->gumcei.gumcei[i].plmn.mcc_digit2 = (uint8_t) atoi (c);
  				c[0] = mcc[2];
  				config_pP->gumcei.gumcei[i].plmn.mcc_digit3 = (uint8_t) atoi (c);
  			}
  			if ((config_setting_lookup_string (sub2setting, MCE_CONFIG_STRING_MNC, &mnc))) {
  				AssertFatal( (3 == strlen(mnc)) || (2 == strlen(mnc)) , "Bad MNC length, it must be 3 digit ex: 001");
  				char c[2] = { mnc[0], 0};
  				config_pP->gumcei.gumcei[i].plmn.mnc_digit1 = (uint8_t) atoi (c);
  				c[0] = mnc[1];
  				config_pP->gumcei.gumcei[i].plmn.mnc_digit2 = (uint8_t) atoi (c);
  				if (3 == strlen(mnc)) {
  					c[0] = mnc[2];
  					config_pP->gumcei.gumcei[i].plmn.mnc_digit3 = (uint8_t) atoi (c);
  				} else {
  					config_pP->gumcei.gumcei[i].plmn.mnc_digit3 = 0x0F;
  				}
  			}
  			if ((config_setting_lookup_string (sub2setting, MCE_CONFIG_STRING_GLOBAL_MCE_ID, &mnc))) {
  				config_pP->gumcei.gumcei[i].mce_gid = (uint16_t) atoi (mnc);
  			}
  			config_pP->gumcei.nb += 1;
  		}
  	}
  }

  // NETWORK INTERFACE SETTING
  setting = config_setting_get_member (setting_mce, MCE_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);

  if (setting != NULL) {
  	/** Process Mandatory Interfaces. */
  	config_setting_lookup_string (setting, MCE_CONFIG_STRING_IPV4_ADDRESS_FOR_MC, (const char **)&mc_mce_v4);
  	config_setting_lookup_string (setting, MCE_CONFIG_STRING_IPV6_ADDRESS_FOR_MC, (const char **)&mc_mce_v6);

  	if (
  		(
  		/** MC. */
  		config_setting_lookup_string (setting, MCE_CONFIG_STRING_INTERFACE_NAME_FOR_MC, (const char **)&if_name_mc) && (mc_mce_v4 || mc_mce_v6)
  		&& config_setting_lookup_int (setting, MCE_CONFIG_STRING_MCE_PORT_FOR_SM, &aint_mc)
  		)
  	) {
  		config_pP->ip.port_sm = (uint16_t)aint_mc;
  		struct bstrList *list = NULL;

  		/** MC: IPv4. */
  		config_pP->ip.if_name_mc = bfromcstr(if_name_mc);
  		if(mc_mce_v4) {
  			cidr = bfromcstr (mc_mce_v4);
  			list = bsplit (cidr, '/');
  			AssertFatal(2 == list->qty, "Bad CIDR address %s", bdata(cidr));
  			address = list->entry[0];
  			mask    = list->entry[1];
  			IPV4_STR_ADDR_TO_INADDR (bdata(address), config_pP->ip.mc_mce_v4, "BAD IP ADDRESS FORMAT FOR MC-MCE !\n");
  			config_pP->ip.mc_mce_cidrv4 = atoi ((const char*)mask->data);
  			bstrListDestroy(list);
  			in_addr_var.s_addr = config_pP->ip.mc_mce_v4.s_addr;
  			OAILOG_INFO (LOG_MCE_APP, "Parsing configuration file found MC-MME: %s/%d on %s\n",
  					inet_ntoa (in_addr_var), config_pP->ip.mc_mce_cidrv4, bdata(config_pP->ip.if_name_mc));
  			bdestroy_wrapper(&cidr);
  		}
  		/** MC: IPv6. */
  		if(mc_mce_v6) {
  			cidr = bfromcstr (mc_mce_v6);
  			list = bsplit (cidr, '/');
  			AssertFatal(2 == list->qty, "Bad CIDR address %s", bdata(cidr));
  			address = list->entry[0];
  			mask    = list->entry[1];
  			IPV6_STR_ADDR_TO_INADDR (bdata(address), config_pP->ip.mc_mce_v6, "BAD IPV6 ADDRESS FORMAT FOR MC-MCE !\n");
  			//    	config_pP->ipv6.netmask_s1_mme = atoi ((const char*)mask->data);
  			bstrListDestroy(list);
  			//    	memcpy(in6_addr_var.s6_addr, config_pP->ipv6.s1_mme.s6_addr, 16);
  			char strv6[16];
  			OAILOG_INFO (LOG_MCE_APP, "Parsing configuration file found MC-MME: %s/%d on %s\n",
  					inet_ntop(AF_INET6, &in6_addr_var, strv6, 16), config_pP->ip.mc_mce_cidrv6, bdata(config_pP->ip.if_name_mc));
  			bdestroy_wrapper(&cidr);
  		}
  	}
  }

  // M2AP SETTING
  setting = config_setting_get_member (setting_mce, MCE_CONFIG_STRING_M2AP_CONFIG);

  if (setting != NULL) {
  	if ((config_setting_lookup_int (setting, MCE_CONFIG_STRING_M2AP_OUTCOME_TIMER, &aint))) {
  		config_pP->m2ap_config.outcome_drop_timer_sec = (uint8_t) aint;
  	}

  	if ((config_setting_lookup_int (setting, MCE_CONFIG_STRING_M2AP_PORT, &aint))) {
  		config_pP->m2ap_config.port_number = (uint16_t) aint;
  	}
  }

	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MAX_MBMS_SERVICES, &aint))) {
		config_pP->max_mbms_services = (uint32_t) aint;
	}

	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MAX_MBSFN_AREA_PER_M2_ENB, &aint))) {
		config_pP->max_mbsfn_area_per_m2_enb = (uint32_t) aint;
	}

	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MBMS_SHORT_IDLE_SESSION_DUR_IN_SEC, &aint))) {
		config_pP->mbms_short_idle_session_duration_in_sec = (uint8_t) aint;
	}

	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MBMS_MCCH_MSI_MCS, &aint))) {
		config_pP->mbms_mcch_msi_mcs = (uint8_t) aint;
		AssertFatal(config_pP->mbms_mcch_msi_mcs == 2 || config_pP->mbms_mcch_msi_mcs == 7
				|| config_pP->mbms_mcch_msi_mcs == 9 || config_pP->mbms_mcch_msi_mcs == 13,
				"Bad MCCH MSI %d", config_pP->mbms_mcch_msi_mcs);
	}

	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MBMS_MCCH_MODIFICATION_PERIOD_RF, &aint))) {
		config_pP->mbms_mcch_modification_period_rf = (uint16_t) aint;
	}

	AssertFatal(log2(config_pP->mbms_mcch_modification_period_rf) == 9
			|| log2(config_pP->mbms_mcch_modification_period_rf) == 10,
			"Only Pre-Release 14 MBMS MCCH Modification Period Rfs supported 512/1024. Current (%d).",
			config_pP->mbms_mcch_modification_period_rf);

	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MBMS_MCCH_REPETITION_PERIOD_RF, &aint))) {
		config_pP->mbms_mcch_repetition_period_rf = (uint16_t) aint;
	}

	AssertFatal(log2(config_pP->mbms_mcch_repetition_period_rf) >= 5
			&& log2(config_pP->mbms_mcch_repetition_period_rf) <= 8,
			"Only Pre-Release 14 MBMS MCCH Repetition Period Rfs supported 32..256. Current (%d).",
			config_pP->mbms_mcch_repetition_period_rf);

	/** Check the conditions on the MCCH modification & repetition periods. */
	AssertFatal(!(config_pP->mbms_mcch_modification_period_rf % config_pP->mbms_mcch_repetition_period_rf),
			"MBMS MCCH Modification Period Rf (%d) should be an intiger multiple of MCCH Repetition Period RF (%d).",
			config_pP->mbms_mcch_modification_period_rf, config_pP->mbms_mcch_repetition_period_rf);

	AssertFatal((config_pP->mbms_mcch_modification_period_rf / config_pP->mbms_mcch_repetition_period_rf) > 1,
			"MBMS MCCH Modification Period Rf (%d) should be larger than MCCH Repetition Period RF (%d).",
			config_pP->mbms_mcch_modification_period_rf, config_pP->mbms_mcch_repetition_period_rf);

	if ((config_setting_lookup_float(setting_mce, MCE_CONFIG_MCH_MCS_ENB_FACTOR, &adouble))) {
		config_pP->mch_mcs_enb_factor = (double) adouble;
	}
	AssertFatal(config_pP->mch_mcs_enb_factor > 1, "MCH MCS eNB factor (%d) should >1.", config_pP->mch_mcs_enb_factor);

	if ((config_setting_lookup_int(setting_mce, MCE_CONFIG_MBSFN_CSA_4_RF_THRESHOLD, &aint))) {
		config_pP->mbsfn_csa_4_rf_threshold = aint;
	}
	AssertFatal(config_pP->mbsfn_csa_4_rf_threshold > 1, "MBSFN 4RF Allocation Threshold (%d) should >1.", config_pP->mbsfn_csa_4_rf_threshold);

	/** MBMS SA configurations. */
	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MBMS_GLOBAL_SERVICE_AREA_TYPES, &aint))) {
		config_pP->mbms_global_service_area_types = (uint8_t) aint;
	}

	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MBMS_LOCAL_SERVICE_AREAS, &aint))) {
		config_pP->mbms_local_service_areas = (uint8_t) aint;
	}
	DevAssert(config_pP->mbms_local_service_areas < MCE_CONFIG_MAX_LOCAL_MBMS_SERVICE_AREAS);

	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MBMS_LOCAL_SERVICE_AREA_TYPES, &aint))) {
		config_pP->mbms_local_service_area_types = (uint8_t) aint;
	}

	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MBMS_LOCAL_SERVICE_AREA_SFD_DISTANCES_IN_M, &aint))) {
		config_pP->mbms_local_service_area_sfd_distance_in_m = (uint16_t) aint;
	}

	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MBSFN_SYNCH_AREA_ID, &aint))) {
		config_pP->mbsfn_synch_area_id = (uint8_t) aint;
	}

	/** MBMS M2 eNB configurations. */
	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_STRING_M2_MAX_ENB, &aint))) {
		config_pP->max_m2_enbs = (uint32_t) aint;
	}

	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MBMS_M2_ENB_BAND, &aint))) {
		config_pP->mbms_m2_enb_band = (enb_band_e) aint;
	}
	AssertFatal(get_enb_type(config_pP->mbms_m2_enb_band) != ENB_TYPE_NULL, "MBSFN band is invalid!");

	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MBMS_M2_ENB_TDD_UL_DL_SF_CONF, &aint))) {
		config_pP->mbms_m2_enb_tdd_ul_dl_sf_conf = (uint8_t) aint;
	}
	AssertFatal(config_pP->mbms_m2_enb_tdd_ul_dl_sf_conf >= 0 && config_pP->mbms_m2_enb_tdd_ul_dl_sf_conf <=6,
			"MBMS TDD UL/DL Configuration (%d) is not in bounds [0,6].", config_pP->mbms_m2_enb_tdd_ul_dl_sf_conf);

	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MBMS_M2_ENB_BW, &aint))) {
		config_pP->mbms_m2_enb_bw = (enb_bw_e) aint;
	}
	AssertFatal(config_pP->mbms_m2_enb_tdd_ul_dl_sf_conf >= 0 && config_pP->mbms_m2_enb_tdd_ul_dl_sf_conf <=6,
			"MBMS TDD UL/DL Configuration (%d) is not in bounds [0,6].", config_pP->mbms_m2_enb_tdd_ul_dl_sf_conf);

	/** MBMS Flags. */
	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MBMS_ENB_SCPTM, &aint))) {
		config_pP->mbms_enb_scptm = ((uint8_t) aint & 0x01);
	}
	AssertFatal(!config_pP->mbms_enb_scptm, "SC-PTM not supported right now.");

	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MBMS_RESOURCE_ALLOCATION_FULL, &aint))) {
		config_pP->mbms_resource_allocation_full = ((uint8_t) aint & 0x01);
	}
	AssertFatal(!config_pP->mbms_resource_allocation_full, "MBSFN Full Radio Frame allocation not supported currently.");

	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MBMS_GLOBAL_MBSFN_AREA_PER_LOCAL_GROUP, &aint))) {
		config_pP->mbms_global_mbsfn_area_per_local_group = ((uint8_t) aint & 0x01);
	}

	/** Check the MBMS Local and Global Service Area types. */
	int max_mbsfn_area_id = 0;
	if(config_pP->mbms_global_mbsfn_area_per_local_group) {
		max_mbsfn_area_id = mce_config.mbms_global_service_area_types + (mce_config.mbms_local_service_areas) * (mce_config.mbms_local_service_area_types + mce_config.mbms_global_service_area_types);
	} else {
		max_mbsfn_area_id = mce_config.mbms_global_service_area_types + (mce_config.mbms_local_service_areas * mce_config.mbms_local_service_area_types);
	}
	AssertFatal(max_mbsfn_area_id <= MAX_MBMSFN_AREAS, "MBMS Area Configuration exceeds bounds.");

	if ((config_setting_lookup_int (setting_mce, MCE_CONFIG_MBMS_SUBFRAME_SLOT_HALF, &aint))) {
		config_pP->mbms_subframe_slot_half = ((uint8_t) aint & 0x01);
	}

	/**
	 * Check for the case of local-global flag inactive, if, given the eNB configuration, any local/global MBMS areas are allowed.
	 */
	uint8_t mbsfn_mcch_size = get_enb_subframe_size(get_enb_type(config_pP->mbms_m2_enb_band), config_pP->mbms_m2_enb_tdd_ul_dl_sf_conf);
	/**
	 * If the local-global flag is set, we can configure arbitrarily local MBMS areas,
	 * The MCCH subframes will be assigned sequentially.
	 * If not, we need to check that we can reserve the given number of global MBMS areas.
	 * We don't check for local.
	 */
	if(!config_pP->mbms_global_mbsfn_area_per_local_group){
		if(mbsfn_mcch_size < config_pP->mbms_global_service_area_types){
			OAILOG_ERROR(LOG_MCE_APP, "Total (%d) MCCH MBSFN subframes cannot be lower than #global_mbms_areas (%d).\n",
					mbsfn_mcch_size, config_pP->mbms_global_service_area_types);
			DevAssert(0);
		}
		if(mbsfn_mcch_size == config_pP->mbms_global_service_area_types){
			/** No local MBMS areas may exist. */
			if(config_pP->mbms_local_service_areas){
				OAILOG_ERROR(LOG_MCE_APP, "Total of (%d) MCCH MBSFN subframes are available. We cannot assign any local MBMS areas.\n", mbsfn_mcch_size);
				DevAssert(0);
			}
		}
	}
  }

	OAILOG_SET_CONFIG(&config_pP->log_config);
  config_destroy (&cfg);
  return 0;
}

//------------------------------------------------------------------------------
static void mce_config_display (mce_config_t * config_pP)
{
  int                                     j;

  OAILOG_INFO (LOG_CONFIG, "==== EURECOM %s v%s ====\n", PACKAGE_NAME, PACKAGE_VERSION);
#if DEBUG_IS_ON
  OAILOG_DEBUG (LOG_CONFIG, "Built with CMAKE_BUILD_TYPE ................: %s\n", CMAKE_BUILD_TYPE);
  OAILOG_DEBUG (LOG_CONFIG, "Built with DISABLE_ITTI_DETECT_SUB_TASK_ID .: %d\n", DISABLE_ITTI_DETECT_SUB_TASK_ID);
  OAILOG_DEBUG (LOG_CONFIG, "Built with ITTI_TASK_STACK_SIZE ............: %d\n", ITTI_TASK_STACK_SIZE);
  OAILOG_DEBUG (LOG_CONFIG, "Built with ITTI_LITE .......................: %d\n", ITTI_LITE);
  OAILOG_DEBUG (LOG_CONFIG, "Built with LOG_OAI .........................: %d\n", LOG_OAI);
  OAILOG_DEBUG (LOG_CONFIG, "Built with LOG_OAI_CLEAN_HARD ..............: %d\n", LOG_OAI_CLEAN_HARD);
  OAILOG_DEBUG (LOG_CONFIG, "Built with MESSAGE_CHART_GENERATOR .........: %d\n", MESSAGE_CHART_GENERATOR);
  OAILOG_DEBUG (LOG_CONFIG, "Built with PACKAGE_NAME ....................: %s\n", PACKAGE_NAME);
  OAILOG_DEBUG (LOG_CONFIG, "Built with M2AP_DEBUG_LIST .................: %d\n", M2AP_DEBUG_LIST);
  OAILOG_DEBUG (LOG_CONFIG, "Built with SECU_DEBUG ......................: %d\n", SECU_DEBUG);
  OAILOG_DEBUG (LOG_CONFIG, "Built with SCTP_DUMP_LIST ..................: %d\n", SCTP_DUMP_LIST);
  OAILOG_DEBUG (LOG_CONFIG, "Built with TRACE_HASHTABLE .................: %d\n", TRACE_HASHTABLE);
  OAILOG_DEBUG (LOG_CONFIG, "Built with TRACE_3GPP_SPEC .................: %d\n", TRACE_3GPP_SPEC);
#endif
  OAILOG_INFO (LOG_CONFIG, "Configuration:\n");
  OAILOG_INFO (LOG_CONFIG, "- File .................................: %s\n", bdata(config_pP->config_file));
  OAILOG_INFO (LOG_CONFIG, "- Max M2 eNBs ..........................: %u\n", config_pP->max_m2_enbs);
  OAILOG_INFO (LOG_CONFIG, "- Max MBMS-Services ....................: %u\n", config_pP->max_mbms_services);
  OAILOG_INFO (LOG_CONFIG, "- Max MBMS-Global-Areas.................: %u\n", config_pP->mbms_global_service_area_types);
  OAILOG_INFO (LOG_CONFIG, "- Max MBMS-Local-Areas..................: %u\n", config_pP->mbms_local_service_areas);
  OAILOG_INFO (LOG_CONFIG, "- Max MBMS-Local-Area-Types.............: %u\n", config_pP->mbms_local_service_area_types);
  OAILOG_INFO (LOG_CONFIG, "- Max MBSFN Area per M2 eNB ............: %u\n", config_pP->max_mbsfn_area_per_m2_enb);
  OAILOG_INFO (LOG_CONFIG, "- Relative capa ........................: %u\n", config_pP->relative_capacity);
  OAILOG_INFO (LOG_CONFIG, "- Statistics timer .....................: %u (seconds)\n\n", config_pP->mce_statistic_timer);
  OAILOG_INFO (LOG_CONFIG, "- M2-MME:\n");
  OAILOG_INFO (LOG_CONFIG, "    port number ......: %d\n", config_pP->m2ap_config.port_number);
  OAILOG_INFO (LOG_CONFIG, "- IP:\n");
  // todo: print ipv6 addresses
  OAILOG_INFO (LOG_CONFIG, "    MC MME iface .....: %s\n", bdata(config_pP->ip.if_name_mc));
  OAILOG_INFO (LOG_CONFIG, "    MC MME Sm port ...: %d\n", config_pP->ip.port_sm);
  OAILOG_INFO (LOG_CONFIG, "    MC MME ip ........: %s\n", inet_ntoa (*((struct in_addr *)&config_pP->ip.mc_mce_v4)));
  OAILOG_INFO (LOG_CONFIG, "- ITTI:\n");
  OAILOG_INFO (LOG_CONFIG, "    queue size .......: %u (bytes)\n", config_pP->itti_config.queue_size);
  OAILOG_INFO (LOG_CONFIG, "    log file .........: %s\n", bdata(config_pP->itti_config.log_file));
  OAILOG_INFO (LOG_CONFIG, "- SCTP:\n");
  OAILOG_INFO (LOG_CONFIG, "    in streams .......: %u\n", config_pP->sctp_config.in_streams);
  OAILOG_INFO (LOG_CONFIG, "    out streams ......: %u\n", config_pP->sctp_config.out_streams);
  OAILOG_INFO (LOG_CONFIG, "- PLMN|GLOBAL_MCE_ID):\n");
  OAILOG_INFO (LOG_CONFIG, "            " PLMN_FMT "|%u \n", PLMN_ARG(&config_pP->gumcei.gumcei[0].plmn), config_pP->gumcei.gumcei[0].mce_gid);
  OAILOG_INFO (LOG_CONFIG, "- Logging:\n");
  OAILOG_INFO (LOG_CONFIG, "    Output ..............: %s\n", bdata(config_pP->log_config.output));
  OAILOG_INFO (LOG_CONFIG, "    Output thread safe ..: %s\n", (config_pP->log_config.is_output_thread_safe) ? "true":"false");
  OAILOG_INFO (LOG_CONFIG, "    Output with color ...: %s\n", (config_pP->log_config.color) ? "true":"false");
  OAILOG_INFO (LOG_CONFIG, "    UDP log level........: %s\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.udp_log_level));
  OAILOG_INFO (LOG_CONFIG, "    GTPV2-C log level....: %s\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.gtpv2c_log_level));
  OAILOG_INFO (LOG_CONFIG, "    SCTP log level.......: %s\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.sctp_log_level));
  OAILOG_INFO (LOG_CONFIG, "    ASN1 Verbosity level : %d\n", config_pP->log_config.asn1_verbosity_level);
  OAILOG_INFO (LOG_CONFIG, "    MCE_APP log level....: %s\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.mce_app_log_level));
  OAILOG_INFO (LOG_CONFIG, "    SM log level.........: %s\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.sm_log_level));
  OAILOG_INFO (LOG_CONFIG, "    UTIL log level.......: %s\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.util_log_level));
  OAILOG_INFO (LOG_CONFIG, "    MSC log level........: %s (MeSsage Chart)\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.msc_log_level));
  OAILOG_INFO (LOG_CONFIG, "    ITTI log level.......: %s (InTer-Task Interface)\n", OAILOG_LEVEL_INT2STR(config_pP->log_config.itti_log_level));
}

//------------------------------------------------------------------------------
static void usage (char *target)
{
  OAI_FPRINTF_INFO( "==== EURECOM %s version: %s ====\n", PACKAGE_NAME, PACKAGE_VERSION);
  OAI_FPRINTF_INFO( "Please report any bug to: %s\n", PACKAGE_BUGREPORT);
  OAI_FPRINTF_INFO( "Usage: %s [options]\n", target);
  OAI_FPRINTF_INFO( "Available options:\n");
  OAI_FPRINTF_INFO( "-h      Print this help and return\n");
  OAI_FPRINTF_INFO( "-c<path>\n");
  OAI_FPRINTF_INFO( "        Set the configuration file for mme\n");
  OAI_FPRINTF_INFO( "        See template in UTILS/CONF\n");
#if TRACE_XML
  OAI_FPRINTF_INFO( "-s<path>\n");
  OAI_FPRINTF_INFO( "        Set the scenario file for mme scenario player\n");
#endif
  OAI_FPRINTF_INFO( "-V      Print %s version and return\n", PACKAGE_NAME);
  OAI_FPRINTF_INFO( "-v[1-2] Debug level:\n");
  OAI_FPRINTF_INFO( "            1 -> ASN1 XER printf on and ASN1 debug off\n");
  OAI_FPRINTF_INFO( "            2 -> ASN1 XER printf on and ASN1 debug on\n");
}

//------------------------------------------------------------------------------
int
mce_config_parse_opt_line (
  int argc,
  char *argv[],
  mce_config_t * config_pP)
{
  int                                     c;
  char                                   *config_file = NULL;

  mce_config_init (config_pP);

  /*
   * Parsing command line
   */
  while ((c = getopt (argc, argv, "c:hs:S:v:V")) != -1) {
    switch (c) {
    case 'c':{
        /*
         * Store the given configuration file. If no file is given,
         * * * * then the default values will be used.
         */
        config_pP->config_file = blk2bstr(optarg, strlen(optarg));
        OAI_FPRINTF_INFO ("%s mce_config.config_file %s\n", __FUNCTION__, bdata(config_pP->config_file));
      }
      break;

    case 'v':{
        config_pP->log_config.asn1_verbosity_level = atoi (optarg);
      }
      break;

    case 'V':{
      OAI_FPRINTF_INFO ("==== EURECOM %s v%s ====" "Please report any bug to: %s\n", PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_BUGREPORT);
      }
      break;

#if TRACE_XML
    case 'S':
      config_pP->run_mode = RUN_MODE_SCENARIO_PLAYER;
      config_pP->scenario_player_config.scenario_file = blk2bstr(optarg, strlen(optarg));
      config_pP->scenario_player_config.stop_on_error = true;
      OAI_FPRINTF_INFO ("%s mce_config.scenario_player_config.scenario_file %s\n", __FUNCTION__, bdata(config_pP->scenario_player_config.scenario_file));
      break;
    case 's':
      config_pP->run_mode = RUN_MODE_SCENARIO_PLAYER;
      config_pP->scenario_player_config.scenario_file = blk2bstr(optarg, strlen(optarg));
      config_pP->scenario_player_config.stop_on_error = false;
      OAI_FPRINTF_INFO ("%s mce_config.scenario_player_config.scenario_file %s\n", __FUNCTION__, bdata(config_pP->scenario_player_config.scenario_file));
      break;
#else
    case 's':
    case 'S':
      OAI_FPRINTF_ERR ("Should have compiled mme executable with TRACE_XML set in CMakeLists.template\n");
      exit (0);
      break;
#endif

    case 'h':                  /* Fall through */
    default:
      OAI_FPRINTF_ERR ("Unknown command line option %c\n", c);
      usage (argv[0]);
      exit (0);
    }
  }

  if (!config_pP->config_file) {
    config_file = getenv("CONFIG_FILE");
    if (config_file) {
      config_pP->config_file            = bfromcstr(config_file);
    } else {
      OAILOG_ERROR (LOG_CONFIG, "No config file provided through arg -c, or env variable CONFIG_FILE, exiting\n");
      return RETURNerror;
    }
  }
  OAILOG_DEBUG (LOG_CONFIG, "Config file is %s\n", config_file);
  /*
   * Parse the configuration file using libconfig
   */
  if (mce_config_parse_file (config_pP) != 0) {
    return -1;
  }

  /*
   * Display the configuration
   */
  mce_config_display (config_pP);
  return 0;
}
