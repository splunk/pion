# ---------------------------------------------------------------------
# pion:  a Boost C++ framework for building lightweight HTTP interfaces
# ---------------------------------------------------------------------
# Copyright (C) 2013 Cloudmeter, Inc.  (http://www.cloudmeter.com)
#
# Distributed under the Boost Software License, Version 1.0.
# See http://www.boost.org/LICENSE_1_0.txt
# ---------------------------------------------------------------------

include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckTypeSize)
include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

# check for required includes
CHECK_INCLUDE_FILE_CXX("unordered_map" PION_HAVE_UNORDERED_MAP)
if(NOT PION_HAVE_UNORDERED_MAP)
    CHECK_INCLUDE_FILE_CXX("ext/hash_map" PION_HAVE_EXT_HASH_MAP)
    if(NOT PION_HAVE_EXT_HASH_MAP)
        CHECK_INCLUDE_FILE_CXX("hash_map" PION_HAVE_HASH_MAP)
    endif()
endif()

# check for required functions
check_function_exists(malloc_trim PION_HAVE_MALLOC_TRIM)
