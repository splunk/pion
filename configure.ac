# -----------------------------------
# libpion autoconf configuration file
# -----------------------------------

AC_INIT([libpion], [0.1.0], [support@atomiclabs.com])

AC_CONFIG_AUX_DIR(build)
AC_CONFIG_SRCDIR(src/lib/PionEngine.cpp)
AM_INIT_AUTOMAKE()

# Define CXXFLAGS before AC_PROG_CXX to suppress the default autoconf
# compiler options
CXXFLAGS="-D_REENTRANT -Wall"

# Check for programs
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Use C++ language for tests
AC_LANG_CPLUSPLUS


# Debug setting
AC_ARG_WITH([debug],
    AC_HELP_STRING([--with-debug],[build with debugging information]),
    [with_debug=$withval],
    [with_debug=no])
if test "$with_debug" = "no"
then
	CXXFLAGS="$CXXFLAGS -O2 -DNDEBUG"
	#BOOST_LIB_SUFFIX="-1_33_1"
else
	AC_MSG_NOTICE(Building with debugging information)
	CXXFLAGS="$CXXFLAGS -ggdb -DDEBUG"
	#BOOST_LIB_SUFFIX="-d-1_33_1"
fi


# Check for unordered container support
AC_CHECK_HEADERS([unordered_map hash_map ext/hash_map],,)
#AC_MSG_ERROR([C++ compiler does not seem to support unordered containers]))


# Check for Boost
AX_BOOST_BASE([1.33])
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
BOOST_LIBRARIES="-lboost_thread$BOOST_LIB_SUFFIX"
LDFLAGS="$LDFLAGS $BOOST_LDFLAGS -lpthread $BOOST_LIBRARIES"


# Check for Boost asio headers
AC_CHECK_HEADERS([boost/asio.hpp],,AC_MSG_ERROR(Unable to find the boost::asio headers))


# Check for Boost threads headers
AC_CHECK_HEADERS([boost/thread/thread.hpp],,AC_MSG_ERROR(Unable to find the boost::thread headers))


# Check for Boost threads library
AC_TRY_LINK([#include <boost/thread/thread.hpp>],[ boost::thread_group threads; return 0; ],
	AC_MSG_NOTICE(Linking with boost::thread works),
	AC_MSG_ERROR(Unable to link with the boost::thread library))


# Check for log4cxx library
AC_ARG_WITH([log4cxx],
    AC_HELP_STRING([--with-log4cxx\[=DIR\]],[location of log4cxx library (enables logging)]),
    [ log4cxx_location=$withval ], [ without_log4cxx=true ])
if test "$without_log4cxx" = "true"
then
	AC_MSG_NOTICE([Logging is disabled (use --with-log4cxx) to enable])
else
	# Check if log4cxx location is specified
	if test "$log4cxx_location" != ""
	then
		CPPFLAGS="$CPPFLAGS -I$log4cxx_location/include"
		LDFLAGS="$LDFLAGS -L$log4cxx_location/lib"
	fi

	# Check for log4cxx headers
	AC_CHECK_HEADERS([log4cxx/logger.h],,AC_MSG_ERROR([Unable to find the log4cxx headers]))
	
	# Check for log4cxx library
	LDFLAGS="$LDFLAGS -llog4cxx"
	AC_TRY_LINK([#include <log4cxx/logger.h>],[ log4cxx::Logger::getRootLogger(); return 0; ],
		AC_MSG_NOTICE(Linking with log4cxx works),
		AC_MSG_ERROR(Unable to link with the log4cxx library))

	# Found the log4cxx library
	AC_MSG_NOTICE(Using the log4cxx library for logging)
	AC_DEFINE([HAVE_LOG4CXX],[1],[Define to 1 if you have the `log4cxx' library (-llog4cxx).])
fi


# Output Makefiles
AC_OUTPUT(Makefile src/Makefile \
	src/lib/Makefile src/modules/Makefile src/tests/Makefile)
