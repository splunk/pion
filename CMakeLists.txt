# ---------------------------------------------------------------------
# pion:  a Boost C++ framework for building lightweight HTTP interfaces
# ---------------------------------------------------------------------
# Copyright (C) 2013 Cloudmeter, Inc.  (http://www.cloudmeter.com)
#
# Distributed under the Boost Software License, Version 1.0.
# See http://www.boost.org/LICENSE_1_0.txt
# ---------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)
set(PROJECT_NAME pion_solution)
project(${PROJECT_NAME} CXX C)

#-------------------------------------------------
# prepare folders var's
#-------------------------------------------------
# src root path
set(PION_SRC_ROOT ${PROJECT_SOURCE_DIR} CACHE PATH "Pion source root")
# binary output by default
set(COMMON_BIN_PATH ${CMAKE_BINARY_DIR}/Bin)
set(LIBRARY_OUTPUT_PATH ${COMMON_BIN_PATH}/${CMAKE_BUILD_TYPE})
set(EXECUTABLE_OUTPUT_PATH ${COMMON_BIN_PATH}/${CMAKE_BUILD_TYPE})
# set where to find additional cmake modules if any
set(CMAKE_MODULE_PATH ${PION_SRC_ROOT}/cmake ${CMAKE_MODULE_PATH})

message("EXECUTABLE_OUTPUT_PATH = ${EXECUTABLE_OUTPUT_PATH}")

include(PionUtils)
pion_get_version("${PION_SRC_ROOT}" PION_V_MAJOR PION_V_MINOR PION_V_PATCH)
pion_update_compilation_opts()

set(PION_VERSION "${PION_V_MAJOR}.${PION_V_MINOR}.${PION_V_PATCH}" CACHE STRING "Pion version" FORCE)
message(">>> Building Pion version: ${PION_VERSION}")

#-------------------------------------------------
# Configuration options block
#----------------------------------------------------------
option(BUILD_SHARED_LIBS "Build Pion as shared library" ON)
option(PION_DISABLE_LOGGING "Disable logging" OFF)
option(PION_BUILD_UT "Enable Unit test" ON)

# cleanup prefix lib for Unix-like OSes
set(CMAKE_SHARED_MODULE_PREFIX)

# install target to this folder by default
set(PION_BINARY_DIR ${PION_SRC_ROOT}/bin)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PION_BINARY_DIR}" CACHE PATH "default install path" FORCE)
endif()

# make include globaly visible
set(PROJECT_WIDE_INCLUDE ${PION_SRC_ROOT}/include)
include_directories(${PROJECT_WIDE_INCLUDE})

find_package(Threads REQUIRED)

# TODO: need to handle this with option
set(Boost_USE_MULTITHREADED ON)
#set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost COMPONENTS thread system filesystem regex unit_test_framework REQUIRED)
if (Boost_FOUND)
    # disable autolinking feature
    add_definitions(-DBOOST_ALL_NO_LIB)
    include_directories(${Boost_INCLUDE_DIR})
else()
    message(FATAL_ERROR "Boost >= 1.35 required to build Pion")
endif()

include(PionConfigure)

find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    set(PION_HAVE_ZLIB 1)
endif()

find_package(BZip2)
if(BZIP2_FOUND)
    include_directories(${BZIP2_INCLUDE_DIR})
    set(PION_HAVE_BZLIB 1)
endif()

find_package(OpenSSL)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    set(PION_HAVE_SSL 1)
endif()

if(NOT PION_DISABLE_LOGGING)
    set(Log4Cplus_USE_STATIC_LIBS 1)
    find_package(Log4cplus)
    if (LOG4CPLUS_FOUND)
        include_directories(${LOG4CPLUS_INCLUDE_DIR})
        set(PION_USE_LOG4CPLUS 1)
    endif()
endif()

find_package(Doxygen)
if (DOXYGEN_FOUND)
    add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/doc/Doxyfile
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endif()

configure_file(${CMAKE_MODULE_PATH}/config.hpp.cmake ${COMMON_BIN_PATH}/pion/config.hpp)
include_directories(${COMMON_BIN_PATH})

# TODO: handle #pragma comment, which is hardcoded in config.hpp
# need to generate config.hpp by ourselfs
add_definitions(-DPION_CMAKE_BUILD)

#-------------------------------------------------
# enable TDD
#-------------------------------------------------
if(PION_BUILD_UT)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

# here we add submodules
add_subdirectory(src)
add_subdirectory(services)
add_subdirectory(utils)

install(DIRECTORY ${PION_SRC_ROOT}/include/ DESTINATION include FILES_MATCHING PATTERN "*.hpp")
install(FILES ${COMMON_BIN_PATH}/pion/config.hpp DESTINATION include/pion/)
