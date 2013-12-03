# ---------------------------------------------------------------------
# pion:  a Boost C++ framework for building lightweight HTTP interfaces
# ---------------------------------------------------------------------
# Copyright (C) 2013 Cloudmeter, Inc.  (http://www.cloudmeter.com)
#
# Distributed under the Boost Software License, Version 1.0.
# See http://www.boost.org/LICENSE_1_0.txt
# ---------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)

set(PROJECT_NAME piontests)
project(${PROJECT_NAME} CXX)

# add test plugins
add_subdirectory(plugins)

if (MSVC AND NOT CMAKE_CFG_INTDIR STREQUAL ".")
    # hack for msvc path substitution
    set(PLUGIN_PATH ${COMMON_BIN_PATH}/${CMAKE_CFG_INTDIR})
else()
    set(PLUGIN_PATH ${EXECUTABLE_OUTPUT_PATH})
endif()

message("PLUGIN_PATH = ${PLUGIN_PATH}; EXECUTABLE_OUTPUT_PATH = ${EXECUTABLE_OUTPUT_PATH}")

add_custom_target(prepare_path
    COMMAND cmake -DPION_PLUGIN_PATH=${PLUGIN_PATH} -DPION_SRC_ROOT=${PION_SRC_ROOT} -P ${PION_SRC_ROOT}/cmake/GenTestInclude.cmake
)

include_directories(${PLUGIN_PATH})
include_directories(${PROJECT_SOURCE_DIR})

# get all *.cpp files from src to build target
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp)

add_definitions(-DBOOST_TEST_DYN_LINK)

# build target
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} pion ${CMAKE_THREAD_LIBS_INIT})
add_dependencies(${PROJECT_NAME} prepare_path)

add_test(NAME pion_test WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION tests
    ARCHIVE DESTINATION tests
    )

