set(ITTI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/itti)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/itti)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ITTI_ANALYZER)
include_directories(${SRC_TOP_DIR}/utils)
include_directories(${SRC_TOP_DIR}/utils/msc)
include_directories(${SRC_TOP_DIR}/utils/bstr)
include_directories(${SRC_TOP_DIR}/utils/hashtable)

# TODO (amar) fix include leak
include_directories("${SRC_TOP_DIR}/mce_app")
include_directories("${SRC_TOP_DIR}/m2ap")

set(ITTI_FILES
      # add .h files if depend on (this one is generated)
      ${ITTI_DIR}/intertask_interface.h
      ${ITTI_DIR}/intertask_interface.c
      ${ITTI_DIR}/backtrace.c
      ${ITTI_DIR}/memory_pools.c
      ${ITTI_DIR}/signals.c
      ${ITTI_DIR}/timer.c
      )
add_library(ITTI ${ITTI_FILES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(3GPP_TYPES
  ${CMAKE_CURRENT_SOURCE_DIR}/3gpp_24.008_cc_ies.c
  ${CMAKE_CURRENT_SOURCE_DIR}/3gpp_24.008_gmm_ies.c
  ${CMAKE_CURRENT_SOURCE_DIR}/3gpp_24.008_mm_ies.c
  ${CMAKE_CURRENT_SOURCE_DIR}/3gpp_24.008_sm_ies.c
  ${CMAKE_CURRENT_SOURCE_DIR}/3gpp_29.274_sm_ies.c
  )


