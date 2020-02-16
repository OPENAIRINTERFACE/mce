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

/*! \file mce_config.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_MCE_CONFIG_SEEN
#define FILE_MCE_CONFIG_SEEN
#include <arpa/inet.h>

#include "mce_default_values.h"
#include "3gpp_23.003.h"
#include "3gpp_29.274.h"
#include "common_dim.h"
#include "common_types_mbms.h"
#include "bstrlib.h"
#include "log.h"

#define MAX_GUMCEI          1
#define MAX_MBMS_SA				  8

#define MCE_CONFIG_STRING_MCE_CONFIG                     "MCE"
#define MCE_CONFIG_STRING_PID_DIRECTORY                  "PID_DIRECTORY"
#define MCE_CONFIG_STRING_INSTANCE                       "MCE_INSTANCE"

// todo: what is run mode?
#define MCE_CONFIG_STRING_RUN_MODE                       "RUN_MODE"
#define MCE_CONFIG_STRING_RUN_MODE_TEST                  "TEST"

#define MCE_CONFIG_STRING_RELATIVE_CAPACITY              "RELATIVE_CAPACITY"
#define MCE_CONFIG_STRING_STATISTIC_TIMER                "MCE_STATISTIC_TIMER"

#define MCE_CONFIG_STRING_INTERTASK_INTERFACE_CONFIG     "INTERTASK_INTERFACE"
#define MCE_CONFIG_STRING_INTERTASK_INTERFACE_QUEUE_SIZE "ITTI_QUEUE_SIZE"

#define MCE_CONFIG_STRING_SCTP_CONFIG                    "SCTP"
#define MCE_CONFIG_STRING_SCTP_INSTREAMS                 "SCTP_INSTREAMS"
#define MCE_CONFIG_STRING_SCTP_OUTSTREAMS                "SCTP_OUTSTREAMS"

#define MCE_CONFIG_STRING_M2AP_CONFIG                    "M2AP"
#define MCE_CONFIG_STRING_M2AP_OUTCOME_TIMER             "M2AP_OUTCOME_TIMER"
#define MCE_CONFIG_STRING_M2AP_PORT                      "M2AP_PORT"

/**
 * MBMS Configuration definitions
 */
#define MCE_CONFIG_STRING_MBMS							 						 "MBMS"
#define MCE_CONFIG_MAX_MBMS_SERVICES               	 	   "MAX_MBMS_SERVICES"
#define MCE_CONFIG_MBMS_SHORT_IDLE_SESSION_DUR_IN_SEC    "MBMS_SHORT_IDLE_SESSION_DUR_IN_SEC"

#define MCE_CONFIG_STRING_GUMCEI_LIST										 "GUMCEI_LIST"
#define MCE_CONFIG_STRING_MCC                            "MCC"
#define MCE_CONFIG_STRING_MNC                            "MNC"
#define MCE_CONFIG_STRING_GLOBAL_MCE_ID				 			  	 "GLOBAL_MCE_ID"

#define MCE_CONFIG_MBMS_MCCH_MSI_MCS														"MCE_CONFIG_MBMS_MCCH_MSI_MCS"
#define MCE_CONFIG_MBMS_MCCH_MODIFICATION_PERIOD_RF					  	"MCE_CONFIG_MBMS_MCCH_MODIFICATION_PERIOD_RF"
#define MCE_CONFIG_MBMS_MCCH_REPETITION_PERIOD_RF		 						"MCE_CONFIG_MBMS_MCCH_REPETITION_PERIOD_RF"
#define MCE_CONFIG_MCH_MCS_ENB_FACTOR													  "MCE_CONFIG_MCH_MCS_ENB_FACTOR"
#define MCE_CONFIG_MBSFN_CSA_4_RF_THRESHOLD											"MCE_CONFIG_MBSFN_CSA_4_RF_THRESHOLD"

#define MCE_CONFIG_MBMS_GLOBAL_SERVICE_AREA_TYPES		 						"MBMS_GLOBAL_SERVICE_AREAS"
#define MCE_CONFIG_MBMS_LOCAL_SERVICE_AREAS			 	 							"MBMS_LOCAL_SERVICE_AREAS"
#define MCE_CONFIG_MBMS_LOCAL_SERVICE_AREA_TYPES	 	 						"MBMS_LOCAL_SERVICE_AREA_TYPES"
#define MCE_CONFIG_MBMS_LOCAL_SERVICE_AREA_SFD_DISTANCES_IN_M 	"MBMS_LOCAL_SERVICE_AREA_SFD_DISTANCES_IN_M"
#define MCE_CONFIG_MBSFN_SYNCH_AREA_ID												  "MBSFN_SYNCH_AREA_ID"
#define MCE_CONFIG_MAX_MBSFN_AREA_PER_M2_ENB										"MBMS_MAX_MBSFN_AREA_PER_M2_ENB"

#define MCE_CONFIG_STRING_M2_MAX_ENB                     				"MAX_M2_ENB"
#define MCE_CONFIG_MBMS_M2_ENB_BAND						 									"MBMS_M2_ENB_BAND"
#define MCE_CONFIG_MBMS_M2_ENB_BW																"M2_ENB_BW"
#define MCE_CONFIG_MBMS_M2_ENB_TDD_UL_DL_SF_CONF 								"MBMS_M2_ENB_TDD_UL_DL_SF_CONF"

#define MCE_CONFIG_MBMS_ENB_SCPTM						 							"MBMS_ENB_SCPTM"
#define MCE_CONFIG_MBMS_RESOURCE_ALLOCATION_FULL		 			"MBMS_RESOURCE_ALLOCATION_FULL"
#define MCE_CONFIG_MBMS_GLOBAL_MBSFN_AREA_PER_LOCAL_GROUP	"MBMS_GLOBAL_MBSFN_AREA_PER_LOCAL_GROUP"
#define MCE_CONFIG_MBMS_SUBFRAME_SLOT_HALF				 				"MCE_CONFIG_MBMS_SUBFRAME_SLOT_HALF"

#define MCE_CONFIG_STRING_NETWORK_INTERFACES_CONFIG      "NETWORK_INTERFACES"

#define MCE_CONFIG_STRING_INTERFACE_NAME_FOR_MC          "MCE_INTERFACE_NAME_FOR_MC"
#define MCE_CONFIG_STRING_IPV4_ADDRESS_FOR_MC            "MCE_IPV4_ADDRESS_FOR_MC"
#define MCE_CONFIG_STRING_IPV6_ADDRESS_FOR_MC            "MCE_IPV6_ADDRESS_FOR_MC"
#define MCE_CONFIG_STRING_MCE_PORT_FOR_SM                "MCE_PORT_FOR_SM"

#define MCE_CONFIG_STRING_ASN1_VERBOSITY                 "ASN1_VERBOSITY"
#define MCE_CONFIG_STRING_ASN1_VERBOSITY_NONE            "none"
#define MCE_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING        "annoying"
#define MCE_CONFIG_STRING_ASN1_VERBOSITY_INFO            "info"

#define MCE_CONFIG_STRING_ID                             "ID"

typedef struct mce_config_s {
  /* Reader/writer lock for this configuration */
  pthread_rwlock_t rw_lock;

  unsigned int instance;
  bstring 		 config_file;
  bstring 		 pid_dir;
	uint32_t 		 max_mbms_services;

  uint8_t 	   relative_capacity;
  uint32_t 		 mce_statistic_timer;

  /** MBMS Parameters. */
  uint8_t 		mbms_short_idle_session_duration_in_sec;
  /** MCCH values. */
  uint8_t 		mbms_mcch_msi_mcs;
  uint16_t 		mbms_mcch_modification_period_rf;
  uint16_t 		mbms_mcch_repetition_period_rf;
  /** MBMS Service Area configurations. */
  uint8_t  		mbms_global_service_area_types; 	/**< Offset 0. */
  uint8_t  		mbms_local_service_areas; 	/**< Mod. */
  uint8_t  		mbms_local_service_area_types; 	/**< Offset 1. */
  uint16_t 		mbms_local_service_area_sfd_distance_in_m;
  double	 		mch_mcs_enb_factor;
  uint8_t  		mbsfn_synch_area_id;
  int      		mbsfn_csa_4_rf_threshold;

  /** Possible eNB configurations. */
  uint32_t 		max_m2_enbs;
  enb_band_e  mbms_m2_enb_band;
  enb_bw_e	  mbms_m2_enb_bw;
  uint8_t  		mbms_m2_enb_tdd_ul_dl_sf_conf;
  uint32_t 		max_mbsfn_area_per_m2_enb;

  /** Possible MCE configurations. */
  uint32_t 		max_m3_mces;

  /** Flags. */
  uint8_t  	mbms_enb_scptm:1;
  uint8_t  	mbms_resource_allocation_full:1;
  uint8_t  	mbms_global_mbsfn_area_per_local_group:1;
  uint8_t  	mbms_subframe_slot_half:1;

  struct {
    int      nb;
    gumcei_t gumcei[MAX_GUMCEI];
  } gumcei;

  /* Interface Configuration.
   * Sm/M2AP/M3AP
   */
  struct {
    uint16_t port_number;
    uint8_t  outcome_drop_timer_sec;
  } m2ap_config;

  struct {
    uint16_t port_number;
    uint8_t  outcome_drop_timer_sec;
  } m3ap_config;

  struct {
  	bstring    if_name_mc;
  	struct in_addr  mc_mce_v4;
  	struct in6_addr mc_mce_v6;
  	int        mc_mce_cidrv4;
  	int        mc_mce_cidrv6;
  	uint16_t   port_sm;
  }ip;

  struct {
    uint16_t in_streams;
    uint16_t out_streams;
  } sctp_config;

  struct {
    uint32_t  queue_size;
    bstring   log_file;
  } itti_config;

  struct {
    int nb_service_entries;
#define MCE_CONFIG_MAX_SERVICE 128
    bstring        			 service_id[MCE_CONFIG_MAX_SERVICE];
    interface_type_t 		 interface_type[MCE_CONFIG_MAX_SERVICE];
    union{
    	struct sockaddr_in   v4; //   service_ip_addr[MCE_CONFIG_MAX_SERVICE]; /**< Just allocating sockaddr was not enough. */
    	struct sockaddr_in6  v6; //; /**< Just allocating sockaddr was not enough. */
    }sockaddr[MCE_CONFIG_MAX_SERVICE];
    /** MCE entries. */
  } e_dns_emulation;

  log_config_t log_config;
} mce_config_t;

extern mce_config_t mce_config;

mbms_service_area_id_t mce_app_check_mbms_sa_exists(const plmn_t * target_plmn, const mbms_service_area_t * mbms_service_area);

int mce_config_parse_opt_line(int argc, char *argv[], mce_config_t *mce_config);

void mce_config_exit (void);

#define mce_config_read_lock(mCEcONFIG)  pthread_rwlock_rdlock(&(mCEcONFIG)->rw_lock)
#define mce_config_write_lock(mCEcONFIG) pthread_rwlock_wrlock(&(mCEcONFIG)->rw_lock)
#define mce_config_unlock(mCEcONFIG)     pthread_rwlock_unlock(&(mCEcONFIG)->rw_lock)

#endif /* FILE_MCE_CONFIG_SEEN */
