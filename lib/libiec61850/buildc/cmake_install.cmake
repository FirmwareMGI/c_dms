# Install script for directory: /home/pi/c_dms/lib/libiec61850

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libiec61850" TYPE FILE FILES
    "/home/pi/c_dms/lib/libiec61850/hal/inc/hal_base.h"
    "/home/pi/c_dms/lib/libiec61850/hal/inc/hal_time.h"
    "/home/pi/c_dms/lib/libiec61850/hal/inc/hal_thread.h"
    "/home/pi/c_dms/lib/libiec61850/hal/inc/hal_filesystem.h"
    "/home/pi/c_dms/lib/libiec61850/hal/inc/hal_ethernet.h"
    "/home/pi/c_dms/lib/libiec61850/hal/inc/hal_socket.h"
    "/home/pi/c_dms/lib/libiec61850/hal/inc/tls_config.h"
    "/home/pi/c_dms/lib/libiec61850/src/common/inc/libiec61850_common_api.h"
    "/home/pi/c_dms/lib/libiec61850/src/common/inc/linked_list.h"
    "/home/pi/c_dms/lib/libiec61850/src/common/inc/sntp_client.h"
    "/home/pi/c_dms/lib/libiec61850/src/iec61850/inc/iec61850_client.h"
    "/home/pi/c_dms/lib/libiec61850/src/iec61850/inc/iec61850_common.h"
    "/home/pi/c_dms/lib/libiec61850/src/iec61850/inc/iec61850_server.h"
    "/home/pi/c_dms/lib/libiec61850/src/iec61850/inc/iec61850_model.h"
    "/home/pi/c_dms/lib/libiec61850/src/iec61850/inc/iec61850_cdc.h"
    "/home/pi/c_dms/lib/libiec61850/src/iec61850/inc/iec61850_dynamic_model.h"
    "/home/pi/c_dms/lib/libiec61850/src/iec61850/inc/iec61850_config_file_parser.h"
    "/home/pi/c_dms/lib/libiec61850/src/mms/inc/mms_value.h"
    "/home/pi/c_dms/lib/libiec61850/src/mms/inc/mms_common.h"
    "/home/pi/c_dms/lib/libiec61850/src/mms/inc/mms_types.h"
    "/home/pi/c_dms/lib/libiec61850/src/mms/inc/mms_type_spec.h"
    "/home/pi/c_dms/lib/libiec61850/src/mms/inc/mms_client_connection.h"
    "/home/pi/c_dms/lib/libiec61850/src/mms/inc/mms_server.h"
    "/home/pi/c_dms/lib/libiec61850/src/mms/inc/iso_connection_parameters.h"
    "/home/pi/c_dms/lib/libiec61850/src/goose/goose_subscriber.h"
    "/home/pi/c_dms/lib/libiec61850/src/goose/goose_receiver.h"
    "/home/pi/c_dms/lib/libiec61850/src/goose/goose_publisher.h"
    "/home/pi/c_dms/lib/libiec61850/src/sampled_values/sv_subscriber.h"
    "/home/pi/c_dms/lib/libiec61850/src/sampled_values/sv_publisher.h"
    "/home/pi/c_dms/lib/libiec61850/src/r_session/r_session.h"
    "/home/pi/c_dms/lib/libiec61850/src/logging/logging_api.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/pi/c_dms/lib/libiec61850/buildc/hal/cmake_install.cmake")
  include("/home/pi/c_dms/lib/libiec61850/buildc/src/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/pi/c_dms/lib/libiec61850/buildc/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
