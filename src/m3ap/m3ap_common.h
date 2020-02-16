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


/** @defgroup _m3ap_impl_ M3AP Layer Reference Implementation
 * @ingroup _ref_implementation_
 * @{
 */

#if HAVE_CONFIG_H_
# include "config.h"
#endif

#ifndef FILE_M3AP_COMMON_SEEN
#define FILE_M3AP_COMMON_SEEN

#include "bstrlib.h"

/* Defined in asn_internal.h */
// extern int asn_debug_indent;
extern int asn_debug;

#if defined(EMIT_ASN_DEBUG_EXTERN)
inline void ASN_DEBUG(const char *fmt, ...);
#endif

#include "M3AP_ProtocolIE-Field.h"
#include "M3AP_M3AP-PDU.h"
#include "M3AP_InitiatingMessage.h"
#include "M3AP_SuccessfulOutcome.h"
#include "M3AP_UnsuccessfulOutcome.h"
#include "M3AP_ProtocolIE-Field.h"
#include "M3AP_ProtocolIE-FieldPair.h"
#include "M3AP_ProtocolIE-ContainerPair.h"
#include "M3AP_ProtocolExtensionField.h"
#include "M3AP_ProtocolExtensionContainer.h"
#include "M3AP_asn_constant.h"
#include "M3AP_ProcedureCode.h"
#include "M3AP_ProtocolIE-Container.h"
#include "M3AP_ProtocolIE-ContainerList.h"
#include "M3AP_ProtocolIE-ContainerPairList.h"
#include "M3AP_ProtocolIE-ID.h"
#include "M3AP_ProtocolIE-Single-Container.h"

#include "M3AP_Absolute-Time-ofMBMS-Data.h"
#include "M3AP_AllocationAndRetentionPriority.h"
#include "M3AP_BitRate.h"
#include "M3AP_Cause.h"
#include "M3AP_CauseMisc.h"
#include "M3AP_CauseNAS.h"
#include "M3AP_CauseProtocol.h"
#include "M3AP_CauseRadioNetwork.h"
#include "M3AP_CauseTransport.h"
#include "M3AP_CriticalityDiagnosticsM3.h"
#include "M3AP_CriticalityDiagnosticsM3-IE-List.h"
#include "M3AP_Criticality.h"
#include "M3AP_ECGI.h"
#include "M3AP_ErrorIndication.h"
#include "M3AP_EUTRANCellIdentifier.h"
#include "M3AP_ExtendedMCE-ID.h"
#include "M3AP_GBR-QosInformation.h"
#include "M3AP_Global-MCE-ID.h"
#include "M3AP_GTP-TEID.h"
#include "M3AP_IPAddress.h"
#include "M3AP_M3SetupFailure.h"
#include "M3AP_M3SetupRequest.h"
#include "M3AP_M3SetupResponse.h"
#include "M3AP_MBMS-Cell-List.h"
#include "M3AP_MBMS-E-RAB-QoS-Parameters.h"
#include "M3AP_MBMSServiceArea1.h"
#include "M3AP_MBMS-Service-Area.h"
#include "M3AP_MBMSServiceAreaListItem.h"
#include "M3AP_MBMS-Service-associatedLogicalM3-ConnectionItem.h"
#include "M3AP_MBMS-Service-associatedLogicalM3-ConnectionListResAck.h"
#include "M3AP_MBMS-Service-associatedLogicalM3-ConnectionListRes.h"
#include "M3AP_MBMS-Session-Duration.h"
#include "M3AP_MBMS-Session-ID.h"
#include "M3AP_MBMSSessionStartFailure.h"
#include "M3AP_MBMSSessionStartRequest.h"
#include "M3AP_MBMSSessionStartResponse.h"
#include "M3AP_MBMSSessionStopRequest.h"
#include "M3AP_MBMSSessionStopResponse.h"
#include "M3AP_MBMSSessionUpdateFailure.h"
#include "M3AP_MBMSSessionUpdateRequest.h"
#include "M3AP_MBMSSessionUpdateResponse.h"
#include "M3AP_MCEConfigurationUpdateAcknowledge.h"
#include "M3AP_MCEConfigurationUpdateFailure.h"
#include "M3AP_MCEConfigurationUpdate.h"
#include "M3AP_MCE-ID.h"
#include "M3AP_MCE-MBMS-M3AP-ID.h"
#include "M3AP_MCEname.h"
#include "M3AP_MinimumTimeToMBMSDataTransfer.h"
#include "M3AP_MME-MBMS-M3AP-ID.h"
#include "M3AP_PLMN-Identity.h"
#include "M3AP_Pre-emptionCapability.h"
#include "M3AP_Pre-emptionVulnerability.h"
#include "M3AP_Presence.h"
#include "M3AP_PriorityLevel.h"
#include "M3AP_PrivateIE-Container.h"
#include "M3AP_PrivateIE-Field.h"
#include "M3AP_PrivateIE-ID.h"
#include "M3AP_PrivateMessage.h"
#include "M3AP_QCI.h"
#include "M3AP_Reestablishment.h"
#include "M3AP_ResetAcknowledge.h"
#include "M3AP_ResetAll.h"
#include "M3AP_Reset.h"
#include "M3AP_ResetType.h"
#include "M3AP_TimeToWait.h"
#include "M3AP_TMGI.h"
#include "M3AP_TNL-Information.h"
#include "M3AP_TriggeringMessage.h"
#include "M3AP_TypeOfError.h"
#include "M3AP_UnsuccessfulOutcome.h"

/* Checking version of ASN1C compiler */
#if (ASN1C_ENVIRONMENT_VERSION < ASN1C_MINIMUM_VERSION)
# error "You are compiling M3AP with the wrong version of ASN1C"
#endif


extern int asn_debug;
extern int asn1_xer_print;

# include <stdbool.h>
# include "mce_default_values.h"
# include "3gpp_23.003.h"
# include "3gpp_24.008.h"
# include "3gpp_33.401.h"
# include "3gpp_36.444.h"
# include "security_types.h"
# include "common_types_mbms.h"

#define M3AP_FIND_PROTOCOLIE_BY_ID(IE_TYPE, ie, container, IE_ID, mandatory) \
  do {\
    IE_TYPE **ptr; \
    ie = NULL; \
    for (ptr = container->protocolIEs.list.array; \
         ptr < &container->protocolIEs.list.array[container->protocolIEs.list.count]; \
         ptr++) { \
      if((*ptr)->id == IE_ID) { \
        ie = *ptr; \
        break; \
      } \
    } \
    if (ie == NULL ) { \
      if (mandatory)  OAILOG_ERROR (LOG_M3AP, "M3AP_FIND_PROTOCOLIE_BY_ID: %s %d: Mandatory ie is NULL\n",__FILE__,__LINE__);\
      else OAILOG_DEBUG (LOG_M3AP, "M3AP_FIND_PROTOCOLIE_BY_ID: %s %d: Optional ie is NULL\n",__FILE__,__LINE__);\
    } \
    if (mandatory)  DevAssert(ie != NULL); \
  } while(0)
/** \brief Function callback prototype.
 **/
typedef int (*m3ap_message_decoded_callback)(
    const sctp_assoc_id_t         assoc_id,
    const sctp_stream_id_t        stream,
    M3AP_M3AP_PDU_t *pdu
);

/** \brief Handle criticality
 \param criticality Criticality of the IE
 @returns void
 **/
void m3ap_handle_criticality(M3AP_Criticality_t criticality);

#endif /* FILE_M3AP_COMMON_SEEN */
