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

/*! \file 3gpp_requirements_36.444.h
  \brief
  \author Dincer Beken
  \company blackned GmbH
  \email: dbeken@blackned.de
*/

#ifndef FILE_3GPP_REQUIREMENTS_36_444_SEEN
#define FILE_3GPP_REQUIREMENTS_36_444_SEEN

#include "3gpp_requirements.h"
#include "log.h"

#define REQUIREMENT_3GPP_36_444(rElEaSe_sEcTiOn__OaImark) REQUIREMENT_3GPP_SPEC(LOG_M2AP, "Hit 3GPP TS 36_444"#rElEaSe_sEcTiOn__OaImark" : "rElEaSe_sEcTiOn__OaImark##_BRIEF"\n")
#define NO_REQUIREMENT_3GPP_36_444(rElEaSe_sEcTiOn__OaImark) REQUIREMENT_3GPP_SPEC(LOG_M2AP, "#NOT IMPLEMENTED 3GPP TS 36_444"#rElEaSe_sEcTiOn__OaImark" : "rElEaSe_sEcTiOn__OaImark##_BRIEF"\n")
#define NOT_REQUIREMENT_3GPP_36_444(rElEaSe_sEcTiOn__OaImark) REQUIREMENT_3GPP_SPEC(LOG_M2AP, "#NOT ASSERTED 3GPP TS 36_444"#rElEaSe_sEcTiOn__OaImark" : "rElEaSe_sEcTiOn__OaImark##_BRIEF"\n")

////-----------------------------------------------------------------------------------------------------------------------
//#define R10_8_3_3_2__2 "MME36.444R10_8.3.3.2_2: Successful Operation\
//                                                                                                                        \
//    The UE CONTEXT RELEASE COMMAND message shall contain the UE S1AP ID pair IE if available, otherwise the             \
//    message shall contain the MME UE S1AP ID IE."                                                                                \
//
//#define R10_8_3_3_2__2_BRIEF "UE CONTEXT RELEASE COMMAND contains UE S1AP ID pair IE or at least MME UE S1AP ID IE"


#endif /* FILE_3GPP_REQUIREMENTS_36_444_SEEN */
