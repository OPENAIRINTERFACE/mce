add_list1_option(M2AP_VERSION R15 "M2AP Asn.1 grammar version" R15)
add_definitions(-DASN1_MINIMUM_VERSION=923)

# Common headers
include_directories("${SRC_TOP_DIR}/common")
include_directories("${SRC_TOP_DIR}/common/itti")
include_directories("${SRC_TOP_DIR}/utils")
include_directories("${SRC_TOP_DIR}/mme")

# This is a fork of https://github.com/websnarf/bstrlib
# Evaluate if we want to pull the src directly from github.
include_directories("${SRC_TOP_DIR}/utils/bstr")
include_directories("${SRC_TOP_DIR}/utils/hashtable")

# Possible header leak
include_directories("${SRC_TOP_DIR}/mce_app")

###############################################################################
# A difficulty: asn1c generates C code of a un-predictable list of files
# so, generate the c from asn1c once at cmake run time
# So, if someone modify the asn.1 source file in such as way that it will create
# (so creating new asn.1 objects instead of modifying the object attributes)
# New C code source file, cmake must be re-run (instead of re-running make only)
###############################################################################

set(M3AP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
include_directories("${M3AP_DIR}")

## M3AP

execute_process(COMMAND ${asn1c_call} ${M3AP_C_DIR} ${M3AP_ASN_FILES})
file(GLOB M3AP_source ${M3AP_C_DIR}/*.c)

set(M3AP_OAI_generated
    ${M3AP_C_DIR}/m2ap_decoder.c
    ${M3AP_C_DIR}/m2ap_encoder.c
    ${M3AP_C_DIR}/m3ap_xer_print.c
    ${M3AP_C_DIR}/m3ap_ies_defs.h
    )

file(GLOB m3ap_h ${M3AP_C_DIR}/*.h)
set(m3ap_h ${m3ap_h})

add_custom_command(
    OUTPUT ${M3AP_OAI_generated}
    COMMAND ${asn1c_call} ${M3AP_C_DIR} ${M3AP_ASN_FILES}
    COMMAND python ${M3AP_DIR}/messages/asn1/asn1tostruct.py -f${M3AP_ASN_DIR}/M3AP-PDU-Contents.asn -o${M3AP_C_DIR}
    DEPENDS ${M3AP_ASN_FILES}
)

add_library(M3AP_LIB
    ${M3AP_OAI_generated}
    ${M3AP_source}
    m3ap_common.c
    )
    
add_library(M3AP_EPC
    ${M3AP_DIR}/m3ap_mme_encoder.c
    ${M3AP_DIR}/m3ap_mme_decoder.c
    ${M3AP_DIR}/m3ap_mme_handlers.c
    ${M3AP_DIR}/m3ap_mme_procedures.c
    ${M3AP_DIR}/m3ap_mme_procedures.c
    ${M3AP_DIR}/m3ap_mme.c
    ${M3AP_DIR}/m3ap_mme.c
    ${M3AP_DIR}/m3ap_mme_itti_messaging.c
    ${M3AP_DIR}/m3ap_mme_itti_messaging.c
    ${M3AP_DIR}/m3ap_mme_retransmission.c
    ${M3AP_DIR}/m3ap_mme_retransmission.c
    )
#    ${M3AP_DIR}/m3ap_mce_mbms_sa.c
