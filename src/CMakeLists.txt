#--------------------------------------------
# C++ language project template
#
#--------------------------------------------
cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)

set(PROJECT_NAME pion)
project(${PROJECT_NAME} CXX)

# get all *.cpp files from src to build target
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE HDR_FILES ${PROJECT_WIDE_INCLUDE}/*.hpp)

if(MSVC)
    if(BUILD_SHARED_LIBS)
        add_definitions(-DPION_EXPORTS -D_USRDLL)
    else()
        add_definitions(-D_LIB)
    endif()
endif()
# build target
add_library(${PROJECT_NAME} ${SRC_FILES} ${HDR_FILES})
target_link_libraries(${PROJECT_NAME} 
    ${Boost_LIBRARIES} ${ZLIB_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    )

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

