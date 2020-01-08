set(ASN1RELDIR r10.5)
include_directories(${SRC_TOP_DIR}/common)
include_directories(${SRC_TOP_DIR}/common/itti)
include_directories(${SRC_TOP_DIR}/utils)
include_directories(${SRC_TOP_DIR}/utils/bstr)
include_directories(${SRC_TOP_DIR}/utils/hashtable)
include_directories(${SRC_TOP_DIR}/utils/msc)
include_directories(${SRC_TOP_DIR}/gtpv2-c/gtpv2c_ie_formatter/shared)
include_directories(${SRC_TOP_DIR}/gtpv2-c/nwgtpv2c-0.11/include)
include_directories(${SRC_TOP_DIR}/gtpv2-c/nwgtpv2c-0.11/shared)
include_directories(${SRC_TOP_DIR}/mce)

# TODO (amar) fix include leak
include_directories("${SRC_TOP_DIR}/mce_app")

add_library(Sm
    sm_common.c
    sm_ie_formatter.c
    sm_mce_task.c
    sm_mce_session_manager.c
    )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
