# ---------------------------------------------------------------------
# pion:  a Boost C++ framework for building lightweight HTTP interfaces
# ---------------------------------------------------------------------
# Copyright (C) 2013 Splunk Inc.  (https://github.com/splunk/pion)
#
# Distributed under the Boost Software License, Version 1.0.
# See http://www.boost.org/LICENSE_1_0.txt
# ---------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)

set(PROJECT_NAME pion)
project(${PROJECT_NAME} CXX)

set(HTTP_HDR_FILES
    ${PROJECT_WIDE_INCLUDE}/pion/http/auth.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/http/basic_auth.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/http/cookie_auth.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/http/message.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/http/parser.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/http/plugin_server.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/http/plugin_service.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/http/reader.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/http/request.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/http/request_reader.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/http/request_writer.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/http/response.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/http/response_reader.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/http/response_writer.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/http/server.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/http/types.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/http/writer.hpp
    )
source_group("include\\pion\\http" FILES ${HTTP_HDR_FILES})

set(TCP_HDR_FILES
    ${PROJECT_WIDE_INCLUDE}/pion/tcp/connection.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/tcp/server.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/tcp/stream.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/tcp/timer.hpp
    )
source_group("include\\pion\\tcp" FILES ${TCP_HDR_FILES})

set(TEST_HDR_FILES
    ${PROJECT_WIDE_INCLUDE}/pion/test/unit_test.hpp
    )
source_group("include\\pion\\test" FILES ${TEST_HDR_FILES})

set(COMMON_HDR_FILES
    ${PROJECT_WIDE_INCLUDE}/pion/admin_rights.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/algorithm.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/error.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/hash_map.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/logger.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/plugin.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/plugin_manager.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/process.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/scheduler.hpp
    ${PROJECT_WIDE_INCLUDE}/pion/user.hpp
    )
source_group("include\\pion" FILES ${COMMON_HDR_FILES})


set(SRC_FILES
    ${PROJECT_SOURCE_DIR}/admin_rights.cpp
    ${PROJECT_SOURCE_DIR}/algorithm.cpp
    ${PROJECT_SOURCE_DIR}/http_auth.cpp
    ${PROJECT_SOURCE_DIR}/http_basic_auth.cpp
    ${PROJECT_SOURCE_DIR}/http_cookie_auth.cpp
    ${PROJECT_SOURCE_DIR}/http_message.cpp
    ${PROJECT_SOURCE_DIR}/http_parser.cpp
    ${PROJECT_SOURCE_DIR}/http_plugin_server.cpp
    ${PROJECT_SOURCE_DIR}/http_reader.cpp
    ${PROJECT_SOURCE_DIR}/http_server.cpp
    ${PROJECT_SOURCE_DIR}/http_types.cpp
    ${PROJECT_SOURCE_DIR}/http_writer.cpp
    ${PROJECT_SOURCE_DIR}/logger.cpp
    ${PROJECT_SOURCE_DIR}/plugin.cpp
    ${PROJECT_SOURCE_DIR}/process.cpp
    ${PROJECT_SOURCE_DIR}/scheduler.cpp
    ${PROJECT_SOURCE_DIR}/tcp_server.cpp
    ${PROJECT_SOURCE_DIR}/tcp_timer.cpp
    )

if (BUILD_SPDY)
    set(SRC_FILES
        ${SRC_FILES}
        ${PROJECT_SOURCE_DIR}/spdy_decompressor.cpp
        ${PROJECT_SOURCE_DIR}/spdy_parser.cpp
        )
    set(SPDY_HEADERS
        ${PROJECT_WIDE_INCLUDE}/pion/spdy/decompressor.hpp
        ${PROJECT_WIDE_INCLUDE}/pion/spdy/parser.hpp
        ${PROJECT_WIDE_INCLUDE}/pion/spdy/types.hpp
        )
    source_group("include\\pion\\spdy" FILES ${SPDY_HEADERS})
endif()
source_group("src" FILES ${SRC_FILES})

set(HDR_FILES
    ${HTTP_HDR_FILES}
    ${TCP_HDR_FILES}
    ${COMMON_HDR_FILES}
    ${TEST_HDR_FILES}
    ${SPDY_HEADERS}
    )

if(MSVC)
    if(BUILD_SHARED_LIBS)
        add_definitions(-DPION_EXPORTS -D_USRDLL)
    else()
        add_definitions(-D_LIB)
    endif()
endif()
# build target
add_library(${PROJECT_NAME} ${SRC_FILES} ${HDR_FILES})

target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

if(UNIX)
    target_link_libraries(${PROJECT_NAME} dl)
endif()

if (ZLIB_FOUND)
    target_link_libraries(${PROJECT_NAME} ${ZLIB_LIBRARIES})
endif()
if (BZIP2_FOUND )
    target_link_libraries(${PROJECT_NAME} ${BZIP2_LIBRARIES})
endif()
if (OPENSSL_FOUND )
    target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
endif()
if (LOG4CPLUS_FOUND )
    target_link_libraries(${PROJECT_NAME} ${LOG4CPLUS_LIBRARIES})
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

