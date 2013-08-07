#--------------------------------------------
# C++ language project template
#
#--------------------------------------------
cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)

set(PROJECT_NAME plugins)
project(${PROJECT_NAME} CXX)

include_directories(${PROJECT_SOURCE_DIR})

# get all *.cpp files from src to build target
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp)

# build targets from each file
foreach(_file ${SRC_FILES})
    get_filename_component(_prj_name ${_file} NAME_WE)
    set(_hdr_file "${PROJECT_SOURCE_DIR}/${_prj_name}.hpp")
    add_library(${_prj_name} MODULE ${_file} ${_hdr_file})
    target_link_libraries(${_prj_name} pion)
    install(TARGETS ${_prj_name}
        RUNTIME DESTINATION tests
        LIBRARY DESTINATION tests
        ARCHIVE DESTINATION tests
        )

endforeach()
