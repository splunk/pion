# ---------------------------------------------------------------------
# pion:  a Boost C++ framework for building lightweight HTTP interfaces
# ---------------------------------------------------------------------
# Copyright (C) 2013 Cloudmeter, Inc.  (http://www.cloudmeter.com)
#
# Distributed under the Boost Software License, Version 1.0.
# See http://www.boost.org/LICENSE_1_0.txt
# ---------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)

set(PROJECT_NAME piond)
project(${PROJECT_NAME} CXX)

set(PIOND_SRC_FILES piond.cpp)
set(HELLOSRVR_SRC_FILES helloserver.cpp)

# build targets
add_executable(${PROJECT_NAME} ${PIOND_SRC_FILES})
target_link_libraries(${PROJECT_NAME} pion ${CMAKE_THREAD_LIBS_INIT})

add_executable(helloserver ${HELLOSRVR_SRC_FILES})
target_link_libraries(helloserver pion ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS ${PROJECT_NAME} helloserver
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

