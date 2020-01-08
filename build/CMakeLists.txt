################################################################################
# Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The OpenAirInterface Software Alliance licenses this file to You under
# the Apache License, Version 2.0  (the "License"); you may not use this file
# except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#-------------------------------------------------------------------------------
# For more information about the OpenAirInterface (OAI) Software Alliance:
#      contact@openairinterface.org
################################################################################
# Author: laurent THOMAS, Lionel GAUTHIER
###############################################################################

cmake_minimum_required (VERSION 2.8)

#############################################
# Base directories, compatible with legacy OAI building
################################################
set (OPENAIRCN_DIR   $ENV{OPENAIRCN_DIR})
set (BUILD_TOP_DIR   ${OPENAIRCN_DIR}/build)
set (OPENAIRCN_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY})

project (OpenAirInterface)

###########################################
# macros to define options as there is numerous options in oai
################################################
macro(add_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  add_definitions("-D${name}=${value}")
endmacro(add_option)

macro(add_boolean_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  set_property(CACHE ${name} PROPERTY TYPE BOOL)
  if (${value})
    add_definitions("-D${name}=1")
  else (${value})
    add_definitions("-D${name}=0")
  endif (${value})
endmacro(add_boolean_option)

macro(add_integer_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  add_definitions("-D${name}=${value}")
endmacro(add_integer_option)

macro(add_list1_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  set_property(CACHE ${name} PROPERTY STRINGS ${ARGN})
  if(NOT "${value}" STREQUAL "False")
    add_definitions("-D${name}=${value}")
  endif()
endmacro(add_list1_option)

macro(add_list2_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  set_property(CACHE ${name} PROPERTY STRINGS ${ARGN})
  if(NOT "${value}" STREQUAL "False")
    add_definitions("-D${value}=1")
  endif()
endmacro(add_list2_option)

macro(add_list_string_option name val helpstr)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  set_property(CACHE ${name} PROPERTY STRINGS ${ARGN})
  if(NOT "${value}" STREQUAL "False")
    add_definitions("-D${name}=\"${value}\"")
  endif()
endmacro(add_list_string_option)

################################################################
# Compilation flags
################################################################
# Build type
################################################################
if (CMAKE_BUILD_TYPE STREQUAL "")
   set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()
add_list_string_option(CMAKE_BUILD_TYPE "RelWithDebInfo" "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." Debug Release RelWithDebInfo MinSizeRel)
Message("Build type is ${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  add_boolean_option(LOG_OAI True         "Thread safe logging api")
  add_boolean_option(LOG_OAI_MINIMAL True "Thread safe logging api, log only levels above NOTICE")
endif()
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_boolean_option(LOG_OAI True "Thread safe logging api")
endif()

################################################################
# Processor architecture
################################################################

Message("Architecture is ${CMAKE_SYSTEM_PROCESSOR}")
if (CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l")
  set(C_FLAGS_PROCESSOR "-gdwarf-2 -mfloat-abi=hard -mfpu=neon -lgcc -lrt")
else (CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l")
  set(C_FLAGS_PROCESSOR "-msse4.2")
endif()
#
set(CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} ${C_FLAGS_PROCESSOR} -std=gnu99 -Wall -Wstrict-prototypes -fno-strict-aliasing -rdynamic -funroll-loops -Wno-packed-bitfield-compat -fPIC ")
# add autoTOOLS definitions that were maybe used!
set(CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_FCNTL_H=1 -DHAVE_ARPA_INET_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_SYS_SOCKET_H=1 -DHAVE_STRERROR=1 -DHAVE_SOCKET=1 -DHAVE_MEMSET=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_STDLIB_H=1 -DHAVE_MALLOC=1 -DHAVE_LIBSCTP"
)
# set a flag for changes in the source code
# these changes are related to hardcoded path to include .h files
add_definitions(-DCMAKER)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -fstack-protector-all -g -DMALLOC_CHECK_=3 -DDEBUG_IS_ON=1 -DTRACE_IS_ON=1 -O0")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS} -g -DMALLOC_CHECK_=3  -DDEBUG_IS_ON -O1")

################################################################
# Git Version
################################################################
set(GIT_BRANCH        "UNKNOWN")
set(GIT_COMMIT_HASH   "UNKNOWN")
set(GIT_COMMIT_DATE   "UNKNOWN")

find_package(Git)
if(GIT_FOUND)
  message("git found: ${GIT_EXECUTABLE}")
  # Get the current working branch
  execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # Get the latest abbreviated commit hash of the working branch
  execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # Get the latest commit date of the working branch
  execute_process(
    COMMAND git log -1 --format=%cd
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_DATE
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()


add_list_string_option(PACKAGE_NAME "NotDefined" "As per attribute name")
add_definitions("-DPACKAGE_VERSION=\"Branch: ${GIT_BRANCH} Abrev. Hash: ${GIT_COMMIT_HASH} Date: ${GIT_COMMIT_DATE}\"")
add_definitions("-DPACKAGE_BUGREPORT=\"openaircn-user@lists.eurecom.fr\"")

###############################################################################
# Include CMake modules to find other library
###############################################################################

set(CMAKE_MODULE_PATH "${BUILD_TOP_DIR}/cmake_modules/")

###############################################################################
# Include Check library for unit tests
###############################################################################

include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckTypeSize)


###############################################################################
# Debug and build related options
###############################################################################
add_boolean_option( DAEMONIZE                       False    "Fork executable if true")

add_boolean_option( DISABLE_ITTI_DETECT_SUB_TASK_ID False    " ")
add_boolean_option( DISPLAY_LICENCE_INFO            False    "If a module has a licence banner to show")
add_boolean_option( ENABLE_ITTI                     True     "ITTI is internal messaging, should remain enabled for most targets")
add_boolean_option( ENABLE_ITTI_ANALYZER            False    "ITTI Analyzer is a GUI based on GTK that displays the ITTI messages exchanged between tasks")
add_integer_option( ITTI_TASK_STACK_SIZE            0        "pthread allocated stack size in bytes of an ITTI task, if 0, use default stack size ")
add_boolean_option( ITTI_LITE                       False    "Do not use ITTI systematically for each message exchanged between layer modules")
add_boolean_option( MESSAGE_CHART_GENERATOR         False    "For generating sequence diagrams")
add_boolean_option( ENABLE_LIBGTPNL                 False    "Use libgtpnl (patched for dealing with packets marked) for setting GTPV1U tunnels")
add_boolean_option( ENABLE_OPENFLOW                 False    "Use OpenFlow for setting GTPV1U tunnels, use candidate version in dir src/openflow/controller")
add_boolean_option( ENABLE_OPENFLOW_MOSAIC          False    "Use OpenFlow for setting GTPV1U tunnels, use candidate version in dir src/openflow/eps")
# NAS LAYER OPTIONS
##########################
add_boolean_option( EPC_BUILD                       False    "BUILD MCE-xGW executable")
add_boolean_option( GTPV1U_LINEAR_TEID_ALLOCATION   False    "Teid allocation id mode versus pseudo random")
# S1AP LAYER OPTIONS
##########################
add_boolean_option(M2AP_DEBUG_LIST                  False    "Traces, option to be removed soon")
# SCTP LAYER OPTIONS
##########################
add_boolean_option(SCTP_DUMP_LIST                   False    "Traces, option to be removed soon")

add_boolean_option( TRACE_HASHTABLE                 False    "Trace hashtables operations ")
add_boolean_option( LOG_OAI                         False    "Thread safe logging utility")
add_boolean_option( LOG_OAI_CLEAN_HARD              False    "Thread safe logging utility option for cleaning inner structs")
add_boolean_option( SECU_DEBUG                      False    "Traces, option to be removed soon")

add_boolean_option( TRACE_3GPP_SPEC                 True     "Log hits of 3GPP specifications requirements")
add_boolean_option( TRACE_XML                       False     "Log some messages in XML (messages necessary for MCE scenario player)")


set (ITTI_DIR ${OPENAIRCN_DIR}/src/common/itti)
if (${ENABLE_ITTI})

  set(ITTI_FILES
    # add .h files if depend on (this one is generated)
    ${ITTI_DIR}/intertask_interface.h
    ${ITTI_DIR}/intertask_interface.c
    ${ITTI_DIR}/backtrace.c
    ${ITTI_DIR}/memory_pools.c
    ${ITTI_DIR}/signals.c
    ${ITTI_DIR}/timer.c
    )
  if (${ENABLE_ITTI_ANALYZER})
    set(ITTI_FILES
    ${ITTI_FILES}
    ${ITTI_DIR}/intertask_interface_dump.c
    ${OPENAIRCN_BIN_DIR}/messages_xml.h )

    include_directories ("${OPENAIRCN_BIN_DIR}")
  endif (${ENABLE_ITTI_ANALYZER})

  add_library(ITTI ${ITTI_FILES})

  set(ITTI_LIB ITTI)
endif (${ENABLE_ITTI})

add_library(3GPP_TYPES
  ${OPENAIRCN_DIR}/src/common/3gpp_24.008_cc_ies.c
  ${OPENAIRCN_DIR}/src/common/3gpp_24.008_common_ies.c
  ${OPENAIRCN_DIR}/src/common/3gpp_24.008_mm_ies.c
  ${OPENAIRCN_DIR}/src/common/3gpp_24.008_gmm_ies.c
  ${OPENAIRCN_DIR}/src/common/3gpp_24.008_gprs_common_ies.c
  ${OPENAIRCN_DIR}/src/common/3gpp_24.008_sm_ies.c
  ${OPENAIRCN_DIR}/src/common/3gpp_29.274_sm_ies.c)
set(3GPP_TYPES_LIB 3GPP_TYPES)




###############################################################################
# ASN.1 grammar C code generation & dependancies
###############################################################################
# A difficulty: asn1c generates C code of a un-predictable list of files
# so, generate the c from asn1c once at cmake run time
# So, if someone modify the asn.1 source file in such as way that it will create
# (so creating new asn.1 objects instead of modifying the object attributes)
# New C code source file, cmake must be re-run (instead of re-running make only)
###############################################################################
#### S1AP PART ####

#### M2AP PART ####
add_list1_option(M2AP_RELEASE r15 "M2AP ASN.1 grammar version" r15)
set(M2AP_DIR ${OPENAIRCN_DIR}/src/m2ap)
set(asn1_generated_dir ${OPENAIRCN_BIN_DIR})
if (${M2AP_RELEASE} STREQUAL "r15")
  set(M2AP_VERSION 15.0.0)
  set(M2AP_ASN_FILES "m2ap-15.0.0.asn1")
endif(${M2AP_RELEASE} STREQUAL "r15")
add_definitions("-DM2AP_VERSION=\"${M2AP_VERSION}\"")
set(M2AP_ASN_DIR ${M2AP_DIR}/messages/asn1/${M2AP_RELEASE})
set(M2AP_C_DIR ${asn1_generated_dir}/M2AP_${M2AP_RELEASE})

set(GENERATED_FULL_DIR ${M2AP_C_DIR})
set(ASN1_SOURCE_DIR ${M2AP_ASN_DIR}/${M2AP_ASN_FILES})
set(ENV{ASN1C_PREFIX} "M2AP_" )
set(m2ap_generate_code_done_flag ${GENERATED_FULL_DIR}/M2AP_GENERATE_CODE_DONE)

if( ${ASN1_SOURCE_DIR} IS_NEWER_THAN ${m2ap_generate_code_done_flag})
   file(REMOVE ${GENERATED_FULL_DIR}/${ASN1C_PREFIX}*.c ${GENERATED_FULL_DIR}/${ASN1C_PREFIX}*.h)
   file(MAKE_DIRECTORY ${GENERATED_FULL_DIR})
   #execute_process(COMMAND  asn1c -pdu=all -fcompound-names -gen-PER -no-gen-OER -no-gen-example -fno-include-deps -D ${GENERATED_FULL_DIR} ${ASN1_SOURCE_DIR} RESULT_VARIABLE ret)
   execute_process(COMMAND  asn1c -pdu=all -fcompound-names -gen-PER -no-gen-example -fno-include-deps -D ${GENERATED_FULL_DIR} ${ASN1_SOURCE_DIR} RESULT_VARIABLE ret)
   if (NOT ${ret} STREQUAL 0)
      message(FATAL_ERROR "${ret}")
   endif (NOT ${ret} STREQUAL 0)
endif()
# TOUCH not in cmake 3.10
file(WRITE ${m2ap_generate_code_done_flag})

# Warning: if you modify ASN.1 source file to generate new C files, cmake should be re-run instead of make
#execute_process(COMMAND ${OPENAIR_CMAKE}/tools/make_asn1c_includes.sh "${M2AP_C_DIR}" "${M2AP_ASN_DIR}/${M2AP_ASN_FILES}" "M2AP_" -fno-include-deps
#                RESULT_VARIABLE ret)
file(GLOB M2AP_source ${M2AP_C_DIR}/*.c)

add_library(M2AP_LIB
  ${M2AP_source}
  ${M2AP_DIR}/m2ap_common.c
  )
################################

include_directories ("${M2AP_C_DIR}")
include_directories ("${M2AP_DIR}")

add_library(M2AP_EPC
  ${M2AP_DIR}/m2ap_mce.c
  ${M2AP_DIR}/m2ap_mce_encoder.c
  ${M2AP_DIR}/m2ap_mce_decoder.c
  ${M2AP_DIR}/m2ap_mce_handlers.c
  ${M2AP_DIR}/m2ap_mce_itti_messaging.c
  ${M2AP_DIR}/m2ap_mce_procedures.c
  ${M2AP_DIR}/m2ap_mce_retransmission.c
  ${M2AP_DIR}/m2ap_mce_mbms_sa.c
  )

add_dependencies(M2AP_EPC M2AP_LIB)


###############################################################################
# add the binary tree to the search path for include files
###############################################################################
# We will find ConfigOAI.h after generation in target directory
###############################################################################

include_directories("${OPENAIRCN_BIN_DIR}")

###############################################################################
# add directories to find all include files
# the internal rule is to use generic names such as defs.h
# but to make it uniq name as adding the relative path in the include directive
# example: #include "RRC/LITE/defs.h"
#find_path (include_dirs_all *.h ${OPENAIRCN_DIR}/src)
#find_path (include_dirs_all *.h PATHS /usr/include NO_CMAKE_PATH)
#include_directories("${include_dirs_all}")
###############################################################################

include_directories("${OPENAIRCN_DIR}/src/common")
include_directories("${OPENAIRCN_DIR}/src/utils")

include_directories("${OPENAIRCN_DIR}/src/common/itti")
include_directories("${OPENAIRCN_DIR}/src/sctp")
include_directories("${OPENAIRCN_DIR}/src/m2ap")
include_directories("${OPENAIRCN_DIR}/src/udp")
include_directories("${OPENAIRCN_DIR}/src/utils/hashtable")
include_directories("${OPENAIRCN_DIR}/src/utils/msc")
include_directories("${OPENAIRCN_DIR}/src/utils/bstr")
include_directories("${OPENAIRCN_DIR}/src/mce")
include_directories("${OPENAIRCN_DIR}/src/mce_app")
include_directories("${OPENAIRCN_DIR}/src")


###############################################################################
# Utilities Library
###############################################################################
add_library(BSTR
  ${OPENAIRCN_DIR}/src/utils/bstr/bstraux.c
  ${OPENAIRCN_DIR}/src/utils/bstr/bstrlib.c
  ${OPENAIRCN_DIR}/src/utils/bstr/buniutil.c
  ${OPENAIRCN_DIR}/src/utils/bstr/utf8util.c
)
include_directories(${OPENAIRCN_DIR}/src/utils/bstr)

add_library(HASHTABLE
  ${OPENAIRCN_DIR}/src/utils/hashtable/hashtable.c
  ${OPENAIRCN_DIR}/src/utils/hashtable/hashtable_uint64.c
  ${OPENAIRCN_DIR}/src/utils/hashtable/obj_hashtable.c
  ${OPENAIRCN_DIR}/src/utils/hashtable/obj_hashtable_uint64.c
)
include_directories(${OPENAIRCN_DIR}/src/utils/hashtable)

if (MESSAGE_CHART_GENERATOR)
  add_library(MSC
    ${OPENAIRCN_DIR}/src/utils/msc/msc.c
  )
  set(MSC_LIB MSC)
endif()
include_directories(${OPENAIRCN_DIR}/src/utils/msc/msc)


if (TRACE_XML)
  add_library(XML_MSG_DUMP
    ${OPENAIRCN_DIR}/src/utils/xml_msg_dump.c
    ${OPENAIRCN_DIR}/src/utils/xml_msg_dump_itti.c
    ${OPENAIRCN_DIR}/src/nas/api/network/nas_message_xml.c
  )
  set(XML_MSG_DUMP_LIB XML_MSG_DUMP)

 add_library(3GPP_TYPES_XML
    ${OPENAIRCN_DIR}/src/common/3gpp_23.003_xml.c
    ${OPENAIRCN_DIR}/src/common/3gpp_24.007_xml.c
    ${OPENAIRCN_DIR}/src/common/3gpp_24.008_cc_ies_xml.c
    ${OPENAIRCN_DIR}/src/common/3gpp_24.008_common_ies_xml.c
    ${OPENAIRCN_DIR}/src/common/3gpp_24.008_mm_ies_xml.c
    ${OPENAIRCN_DIR}/src/common/3gpp_24.008_gmm_ies_xml.c
    ${OPENAIRCN_DIR}/src/common/3gpp_24.008_gprs_common_ies_xml.c
    ${OPENAIRCN_DIR}/src/common/3gpp_24.008_sm_ies_xml.c
    ${OPENAIRCN_DIR}/src/common/3gpp_29.274_xml.c
    ${OPENAIRCN_DIR}/src/nas/3gpp_24.301_common_ies_xml.c
    ${OPENAIRCN_DIR}/src/nas/3gpp_24.301_ies_xml.c
    ${OPENAIRCN_DIR}/src/nas/3gpp_24.301_emm_ies_xml.c
    ${OPENAIRCN_DIR}/src/nas/3gpp_24.301_esm_ies_xml.c
    ${OPENAIRCN_DIR}/src/nas/3gpp_24.301_emm_msg_xml.c
    ${OPENAIRCN_DIR}/src/nas/3gpp_24.301_esm_msg_xml.c
    ${OPENAIRCN_DIR}/src/common/3gpp_36.331_xml.c
    ${OPENAIRCN_DIR}/src/common/3gpp_36.401_xml.c
    ${OPENAIRCN_DIR}/src/common/3gpp_36.413_xml.c)
  set(3GPP_TYPES_XML_LIB 3GPP_TYPES_XML)

  set(SCENARIO_PLAYER_LIB SCENARIO_PLAYER)
endif (TRACE_XML)

include_directories(${OPENAIRCN_DIR}/src/test/scenario_player/)

###############################################################################
# Core Network Utils
###############################################################################

set(CN_UTILS_SRC
  ${OPENAIRCN_DIR}/src/utils/async_system.c
  ${OPENAIRCN_DIR}/src/utils/conversions.c
  ${OPENAIRCN_DIR}/src/utils/dynamic_memory_check.c
  ${OPENAIRCN_DIR}/src/utils/enum_string.c
  ${OPENAIRCN_DIR}/src/utils/mcc_mnc_itu.c
  ${OPENAIRCN_DIR}/src/utils/pid_file.c
  ${OPENAIRCN_DIR}/src/utils/shared_ts_log.c
  ${OPENAIRCN_DIR}/src/utils/TLVEncoder.c
  ${OPENAIRCN_DIR}/src/utils/TLVDecoder.c
  ${OPENAIRCN_DIR}/src/utils/xml2_wrapper.c
  )

if (LOG_OAI)
  set(CN_UTILS_SRC   ${CN_UTILS_SRC}   ${OPENAIRCN_DIR}/src/utils/log.c )
endif(LOG_OAI)

add_library(CN_UTILS ${CN_UTILS_SRC})

##########################
# Core network libs
##########################

set(GTPV2C_DIR  ${OPENAIRCN_DIR}/src/gtpv2-c/nwgtpv2c-0.11/src)
set(GTPV2C_FORMATTER_DIR  ${OPENAIRCN_DIR}/src/gtpv2-c/gtpv2c_ie_formatter/src)
add_library(GTPV2C
  ${GTPV2C_DIR}/NwGtpv2cTrxn.c
  ${GTPV2C_DIR}/NwGtpv2cTunnel.c
  ${GTPV2C_DIR}/NwGtpv2cMsg.c
  ${GTPV2C_DIR}/NwGtpv2cMsgIeParseInfo.c
  ${GTPV2C_DIR}/NwGtpv2cMsgParser.c
  ${GTPV2C_DIR}/NwGtpv2c.c
  ${GTPV2C_FORMATTER_DIR}/gtpv2c_ie_formatter.c
  )

include_directories(${OPENAIRCN_DIR}/src/gtpv2-c/nwgtpv2c-0.11/include/)
include_directories(${OPENAIRCN_DIR}/src/gtpv2-c/nwgtpv2c-0.11/shared/)
include_directories(${OPENAIRCN_DIR}/src/gtpv2-c/gtpv2c_ie_formatter/shared/)

add_library(SCTP_SERVER
  ${OPENAIRCN_DIR}/src/sctp/sctp_common.c
  ${OPENAIRCN_DIR}/src/sctp/sctp_itti_messaging.c
  ${OPENAIRCN_DIR}/src/sctp/sctp_primitives_server.c
  )


add_library(UDP_SERVER ${OPENAIRCN_DIR}/src/udp/udp_primitives_server.c)

set(Sm_DIR ${OPENAIRCN_DIR}/src/sm)
add_library(Sm
  ${Sm_DIR}/sm_common.c
  ${Sm_DIR}/sm_ie_formatter.c
  ${Sm_DIR}/sm_mce_task.c
  ${Sm_DIR}/sm_mce_session_manager.c
)
include_directories(${Sm_DIR})

set(MCE_DIR ${OPENAIRCN_DIR}/src/mce_app)
add_library(MCE_APP
  ${MCE_DIR}/mce_app_mbms_service.c
  ${MCE_DIR}/mce_app_itti_messaging.c
  ${MCE_DIR}/mce_app_main.c
  ${MCE_DIR}/mce_app_statistics.c
  ${MCE_DIR}/mce_app_mbms_service_context.c
  ${MCE_DIR}/mce_app_mbsfn_context.c 
  ${MCE_DIR}/mce_app_mbsfn_scheduling.c 
  ${MCE_DIR}/EpsQualityOfService.c
  ${MCE_DIR}/mce_app_procedures.c
  ${MCE_DIR}/mce_app_statistics.c
  ${MCE_DIR}/mce_config.c
  )

# System packages that are required
# We use either the cmake buildin, in ubuntu are in: /usr/share/cmake*/Modules/
# or cmake provide a generic interface to pkg-config that widely used
###################################
set(CMAKE_MODULE_PATH "${OPENAIRCN_DIR}/build/cmake_modules" "${CMAKE_MODULE_PATH}")

include(FindPkgConfig)

pkg_search_module(LIBXML2 libxml-2.0 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIRS})

pkg_search_module(OPENSSL openssl REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIRS})

pkg_search_module(CONFIG libconfig REQUIRED)
include_directories(${CONFIG_INCLUDE_DIRS})

pkg_search_module(CRYPTO libcrypto REQUIRED)
include_directories(${CRYPTO_INCLUDE_DIRS})

pkg_search_module(NETTLE nettle)
if(NOT ${NETTLE_FOUND})
  message("PACKAGE nettle not found: some targets will fail")
else()
  include_directories(${NETTLE_INCLUDE_DIRS})
endif()

pkg_search_module(NETTLE nettle)
if(NOT ${NETTLE_FOUND})
  message( FATAL_ERROR "PACKAGE nettle not found: some targets will fail. Run scripts/build_mce -i or scripts/build_hss -i  again!")
else()
  include_directories(${NETTLE_INCLUDE_DIRS})
endif()

message ("NETTLE VERSION_INSTALLED  = ${NETTLE_VERSION}")

string(REGEX REPLACE "([0-9]+).*" "\\1" NETTLE_VERSION_MAJOR ${NETTLE_VERSION})
string(REGEX REPLACE "[0-9]+\\.([0-9]+).*" "\\1" NETTLE_VERSION_MINOR ${NETTLE_VERSION})
message ("NETTLE_VERSION_MAJOR = ${NETTLE_VERSION_MAJOR}")
message ("NETTLE_VERSION_MINOR = ${NETTLE_VERSION_MINOR}")

if ("${NETTLE_VERSION_MAJOR}" STREQUAL "" OR "${NETTLE_VERSION_MINOR}" STREQUAL "")
  message( FATAL_ERROR "The nettle version not detected properly. Try to run SCRIPTS/build_mce -i or scripts/build_hss -i again" )
endif()

add_definitions("-DNETTLE_VERSION_MAJOR=${NETTLE_VERSION_MAJOR}")
add_definitions("-DNETTLE_VERSION_MINOR=${NETTLE_VERSION_MINOR}")

find_library(LFDS lfds710 PATHS /usr/local/lib /usr/lib )

if ("${LFDS}" STREQUAL "LFDS-NOTFOUND")
  message(FATAL_ERROR "LIB LFDS not found, please install it" )
endif()

pkg_search_module(XPM xpm)
if(NOT ${XPM_FOUND})
  message("PACKAGE xpm not found: some targets will fail")
else()
  include_directories(${XPM_INCLUDE_DIRS})
endif()



# Hack on a test of asn1c version (already dirty)
add_definitions(-DASN1_MINIMUM_VERSION=923)

#################################
# add executables for operation
#################################


# MCE standalone
################################
add_executable(mce
  ${OPENAIRCN_DIR}/src/oai_mce/oai_mce_log.c
  ${OPENAIRCN_DIR}/src/oai_mce/oai_mce.c
  ${OPENAIRCN_DIR}/src/common/common_types.c
  ${OPENAIRCN_DIR}/src/common/itti_free_defined_msg.c
#  ${OPENAIRCN_DIR}/src/common/itti_comp.c
  )

if( ITTI_ANALYZER )
  add_executable(mce ${OPENAIRCN_BIN_DIR}/messages_xml.h )
endif( ITTI_ANALYZER )
target_link_libraries (mce
  -Wl,--start-group
    M2AP_LIB M2AP_EPC Sm GTPV2C SCTP_SERVER UDP_SERVER 
   MCE_APP ${MSC_LIB} ${ITTI_LIB} ${XML_MSG_DUMP_LIB} ${3GPP_TYPES_LIB}
   ${3GPP_TYPES_XML_LIB} CN_UTILS ${SCENARIO_PLAYER_LIB} HASHTABLE BSTR
  -Wl,--end-group
  pthread m sctp  rt crypt ${LFDS} ${CRYPTO_LIBRARIES} ${OPENSSL_LIBRARIES} ${NETTLE_LIBRARIES} ${CONFIG_LIBRARIES} ${LIBXML2_LIBRARIES} gnutls fdproto fdcore
  )