#--------------------------------------------
# C++ language project template
#
#--------------------------------------------
cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)

set(PROJECT_NAME utils)
project(${PROJECT_NAME} CXX)

message("BUILD_PIOND = ${BUILD_PIOND}")
if (BUILD_PIOND)
    set(PIOND_SRC_FILES piond.cpp)
    add_executable(piond ${PIOND_SRC_FILES})
    target_link_libraries(piond pion ${CMAKE_THREAD_LIBS_INIT})
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
