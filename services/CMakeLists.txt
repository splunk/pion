#--------------------------------------------
# C++ language project template
#
#--------------------------------------------
cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)

set(PROJECT_NAME services)
project(${PROJECT_NAME} CXX)

# get all *.cpp files from src to build target
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp)

# build targets from each file
foreach(_file ${SRC_FILES})
    get_filename_component(_prj_name ${_file} NAME_WE)
    add_library(${_prj_name} MODULE ${_file})
    target_link_libraries(${_prj_name} pion)
    install(TARGETS ${_prj_name}
        RUNTIME DESTINATION services
        LIBRARY DESTINATION services
        ARCHIVE DESTINATION services
        )

endforeach()
