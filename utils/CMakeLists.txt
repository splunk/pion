# ---------------------------------------------------------------------
# pion:  a Boost C++ framework for building lightweight HTTP interfaces
# ---------------------------------------------------------------------
# Copyright (C) 2013 Splunk Inc.  (https://github.com/splunk/pion)
#
# Distributed under the Boost Software License, Version 1.0.
# See http://www.boost.org/LICENSE_1_0.txt
# ---------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)

set(PROJECT_NAME utils)
project(${PROJECT_NAME} CXX)

message("BUILD_PIOND = ${BUILD_PIOND}")
if (BUILD_PIOND)
    set(PIOND_SRC_FILES piond.cpp)
    add_executable(piond ${PIOND_SRC_FILES})
    target_link_libraries(piond pion ${CMAKE_THREAD_LIBS_INIT})

    if(NOT BUILD_SHARED_LIBS)
        if(BUILD_ALLOWNOTHINGSERVICE)
            target_link_libraries(piond AllowNothingService)
        endif()
        if(BUILD_COOKIESERVICE)
            target_link_libraries(piond CookieService)
        endif()
        if(BUILD_ECHOSERVICE)
            target_link_libraries(piond EchoService)
        endif()
        if(BUILD_FILESERVICE)
            target_link_libraries(piond FileService)
        endif()
        if(BUILD_HELLOSERVICE)
            target_link_libraries(piond HelloService)
        endif()
        if(BUILD_LOGSERVICE)
            target_link_libraries(piond LogService)
        endif()
    endif()

    install(TARGETS piond
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endif()

message("BUILD_HELLOSERVER = ${BUILD_HELLOSERVER}")
if (BUILD_HELLOSERVER)
    set(HELLOSRVR_SRC_FILES helloserver.cpp)
    add_executable(helloserver ${HELLOSRVR_SRC_FILES})
    target_link_libraries(helloserver pion ${CMAKE_THREAD_LIBS_INIT})
    install(TARGETS helloserver
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endif()
