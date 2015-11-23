// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_PIONCONFIG_HEADER__
#define __PION_PIONCONFIG_HEADER__

#cmakedefine PION_HAVE_SSL    ${PION_HAVE_SSL} 
#cmakedefine PION_HAVE_JSON   ${PION_HAVE_JSON}
#cmakedefine PION_HAVE_PYTHON ${PION_HAVE_PYTHON}

/* Define to the version number of pion. */
#cmakedefine PION_VERSION "${PION_VERSION}"

/* Define to the directory where Pion plug-ins are installed. */
//#undef PION_PLUGINS_DIRECTORY
#define PION_PLUGINS_DIRECTORY "."

/* Define to 1 if C library supports malloc_trim() */
#cmakedefine PION_HAVE_MALLOC_TRIM ${PION_HAVE_MALLOC_TRIM}

// -----------------------------------------------------------------------
// hash_map support
//
// At least one of the following options should be defined.

/* Define to 1 if you have the <ext/hash_map> header file. */
#cmakedefine PION_HAVE_EXT_HASH_MAP ${PION_HAVE_EXT_HASH_MAP}

/* Define to 1 if you have the <hash_map> header file. */
#cmakedefine PION_HAVE_HASH_MAP ${PION_HAVE_HASH_MAP}

/* Define to 1 if you have the <unordered_map> header file. */
#cmakedefine PION_HAVE_UNORDERED_MAP ${PION_HAVE_UNORDERED_MAP}

// -----------------------------------------------------------------------
// Logging Options
//
// At most one of the logging options below should be defined.  If none of 
// them are defined, std::cout and std::cerr will be used for logging.

/* To use the `log4cplus' library for logging, include PION_USE_LOG4CPLUS or PION_FULL
   in Preprocessor Definitions, or uncomment the following line. */
#cmakedefine PION_USE_LOG4CPLUS ${PION_USE_LOG4CPLUS}

/* To use the `log4cxx' library for logging, include PION_USE_LOG4CXX
   in Preprocessor Definitions, or uncomment the following line. */
#cmakedefine PION_USE_LOG4CXX ${PION_USE_LOG4CXX}

/* To use the `log4cpp' library for logging, include PION_USE_LOG4CPP
   in Preprocessor Definitions, or uncomment the following line. */
#cmakedefine PION_USE_LOG4CPP ${PION_USE_LOG4CPP}

/* To disable logging, include PION_DISABLE_LOGGING in Preprocessor Definitions, 
   or uncomment the following line. */
#cmakedefine PION_DISABLE_LOGGING ${PION_DISABLE_LOGGING}

/* Define to 1 to use C++11 types rather than boost types */
#cmakedefine PION_HAVE_CXX11 ${PION_HAVE_CXX11}

/* Define to 1 to use the standalone ASIO library */
#cmakedefine PION_HAVE_STANDALONE_ASIO ${PION_HAVE_STANDALONE_ASIO}

// -----------------------------------------------------------------------

/* Define to 1 if you have the `zlib' library. */
#cmakedefine PION_HAVE_ZLIB ${PION_HAVE_ZLIB}

/* Define to 1 if you have the `bzlib' library. */
#cmakedefine PION_HAVE_BZLIB ${PION_HAVE_BZLIB}

#if defined(_WIN32) || defined(_WINDOWS)
    #define PION_WIN32  1
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT    0x0501
    #endif
#endif // _WIN32

#include <boost/config.hpp>

#ifdef _MSC_VER
    #ifdef PION_EXPORTS
        #define PION_API __declspec(dllexport)
    #elif defined PION_STATIC_LINKING
        #define PION_API
    #else
        #define PION_API __declspec(dllimport)
    #endif

    #ifdef PION_STATIC_LINKING
        #define PION_PLUGIN 
    #else
        #define PION_PLUGIN __declspec(dllexport)
    #endif

    /*
    Verify correctness of the PION_STATIC_LINKING setup
    */
    #ifdef PION_STATIC_LINKING
        #ifdef _USRDLL
            #error Need to be compiled as a static library for PION_STATIC_LINKING
        #endif
    #endif

#else

    /* This is used by Windows projects to flag exported symbols */
    #define PION_API
    #define PION_PLUGIN

#endif // _MSC_VER

#endif //__PION_PIONCONFIG_HEADER__
