macro(pion_get_version _config_ac_PATH vmajor vminor vpatch)
    file(STRINGS "${_config_ac_PATH}/configure.ac" _pion_VER_STRING_AUX REGEX "^AC_INIT")
#    message("_pion_VER_STRING_AUX = ${_pion_VER_STRING_AUX}")
    string(REGEX MATCH "[0-9]+[.][0-9]+[.][0-9]+" _pion_VER_STR ${_pion_VER_STRING_AUX})
#    message("_pion_VER_STR = ${_pion_VER_STR}")
    string(REGEX MATCHALL "[0-9]+" _pion_VER_LIST ${_pion_VER_STR})
#    message("_pion_VER_LIST = ${_pion_VER_LIST}")
    list(GET _pion_VER_LIST 0 ${vmajor})
    list(GET _pion_VER_LIST 1 ${vminor})
    list(GET _pion_VER_LIST 2 ${vpatch})
endmacro()

macro(pion_update_compilation_opts)
# Make VC compiler more verbose
if(MSVC)
    foreach(flag_var
       CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
       CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
       if(${flag_var} MATCHES "/W[0-3]")
           string(REGEX REPLACE "/W[0-3]" "/W4" ${flag_var} "${${flag_var}}")
       endif()
    endforeach(flag_var)

    add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_SCL_SECURE_NO_DEPRECATE)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.7.0")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -W")
    endif()
endif()

endmacro()
