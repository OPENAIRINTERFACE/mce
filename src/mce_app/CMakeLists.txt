add_definitions("-DPACKAGE_NAME=\"MCE\"")
#TODO: TBD
# set in parent CMakeLists.txt: add_definitions("-DPACKAGE_VERSION=\"0.1\"")
# set in parent CMakeLists.txt: add_definitions("-DPACKAGE_BUGREPORT=\"TBD\"")

set(ASN1RELDIR r10.5)
include_directories(${SRC_TOP_DIR}/common)
include_directories(${SRC_TOP_DIR}/common/itti)
include_directories(${SRC_TOP_DIR}/common/message_utils)
include_directories(${CMAKE_BINARY_DIR}/m2ap/${ASN1RELDIR})
include_directories(${CMAKE_BINARY_DIR}/m3ap/${ASN1RELDIR})
include_directories(${SRC_TOP_DIR}/gtpv2-c/gtpv2c_ie_formatter/shared)
include_directories(${SRC_TOP_DIR}/secu)
include_directories(${SRC_TOP_DIR}/m2ap)
include_directories(${SRC_TOP_DIR}/m3ap)
include_directories(${SRC_TOP_DIR}/utils)
include_directories(${SRC_TOP_DIR}/utils/bstr)
include_directories(${SRC_TOP_DIR}/utils/hashtable)
include_directories(${SRC_TOP_DIR}/utils/msc)
include_directories(${SRC_TOP_DIR}/mce)

# TODO (amar) fix include leak
include_directories("${SRC_TOP_DIR}/mce_app")
include_directories("${SRC_TOP_DIR}/m2ap/messages/asn1/${ASN1RELDIR}")
include_directories("${SRC_TOP_DIR}/m2ap")
include_directories("${SRC_TOP_DIR}/m3ap/messages/asn1/${ASN1RELDIR}")
include_directories("${SRC_TOP_DIR}/m3ap")
include_directories("${SRC_TOP_DIR}/sm") 
add_library(MCE_APP
    mce_app_mbms_service.c
    mce_app_itti_messaging.c
    mce_app_main.c
    mce_app_statistics.c
		mce_app_mbms_service_context.c
  	mce_app_mbsfn_context.c
  	mce_app_mbsfn_scheduling.c
  	EpsQualityOfService.c
    mce_config.c
    )
    

include_directories(${SRC_TOP_DIR}/sctp)
include_directories(${SRC_TOP_DIR}/udp)
include_directories(${SRC_TOP_DIR}/sm)

# MCE executable
################################
add_executable(mce
    ../oai_mce/oai_mce_log.c
    ../oai_mce/oai_mce.c
    ../common/common_types.c
    ../common/itti_free_defined_msg.c
    )

target_link_libraries (mce
    -Wl,--start-group
    M2AP_LIB M2AP_EPC M3AP_LIB M3AP_EPC M2AP M3AP Sm GTPV2C SCTP_SERVER UDP_SERVER SECU_CN MCE_APP
            ${MSC_LIB} ITTI  3GPP_TYPES CN_UTILS
            HASHTABLE BSTR
    -Wl,--end-group
    pthread m sctp  rt crypt ${LFDS} ${NETTLE_LIBRARIES} ${CONFIG_LIBRARIES} gnutls 
    )
