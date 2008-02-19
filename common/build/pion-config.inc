# --------------------------------
# Pion autoconf configuration file
# --------------------------------

# Set Pion version information
PION_LIBRARY_VERSION=$PACKAGE_VERSION
AC_SUBST(PION_LIBRARY_VERSION)
AC_DEFINE_UNQUOTED([PION_VERSION],["$PACKAGE_VERSION"],[Define to the version number of Pion.])

# Note: AM_CONFIG_HEADER is deprecated
AC_CONFIG_HEADERS([common/include/pion/PionConfig.hpp])

# DO NOT USE autoheader (the .hpp.in file is not automanaged)
AUTOHEADER="echo autoheader ignored"

# Check for programs
AC_PROG_CXX
AC_PROG_INSTALL

# Use C++ language
AC_LANG_CPLUSPLUS
AX_COMPILER_VENDOR

# Set basic compiler options
case "$build_os" in
*solaris*)
	if test "$ax_cv_cxx_compiler_vendor" = "sun"; then
		# Solaris: Sun Studio C++ compiler
		CPPFLAGS="$CPPFLAGS -mt -library=stlport4"
		LDFLAGS="$LDFLAGS -mt -library=stlport4"
		PION_OPT_FLAGS="-O2 -DNDEBUG"
		PION_DEBUG_FLAGS="-g"
	else
		# Solaris: GCC compiler
		CPPFLAGS="$CPPFLAGS -pthreads -D_REENTRANT"
		LDFLAGS="$LDFLAGS -pthreads"
		PION_OPT_FLAGS="-O2 -Wall -Wno-strict-aliasing -DNDEBUG"
		PION_DEBUG_FLAGS="-ggdb -Wall -Wno-strict-aliasing -fkeep-inline-functions"
	fi
	PION_EXTERNAL_LIBS="-lsocket -lnsl -ldl -lposix4"
	;;
*darwin*)
	# Mac OS X: GCC compiler
	# -pthread is implied (automatically set by compiler)
	CPPFLAGS="$CPPFLAGS -D_REENTRANT"
	#LDFLAGS="$LDFLAGS"
	PION_OPT_FLAGS="-O2 -Wall -Wno-strict-aliasing -DNDEBUG"
	PION_DEBUG_FLAGS="-ggdb -Wall -Wno-strict-aliasing -fkeep-inline-functions"
	PION_EXTERNAL_LIBS="-ldl"
	;;
*cygwin*)
	# Cygwin GCC compiler (Windows)
	CPPFLAGS="$CPPFLAGS -mthreads -D_REENTRANT -DPION_WIN32 -D__USE_W32_SOCKETS"
	LDFLAGS="$LDFLAGS -mthreads -Wl,-E"
	PION_OPT_FLAGS="-O2 -Wall -Wno-strict-aliasing -DNDEBUG"
	PION_DEBUG_FLAGS="-ggdb -Wall -Wno-strict-aliasing -fkeep-inline-functions"
	PION_EXTERNAL_LIBS="-lws2_32 -lmswsock"
	;;
*freebsd*)
	# FreeBSD: GCC compiler
	CPPFLAGS="$CPPFLAGS -pthread -D_REENTRANT"
	LDFLAGS="$LDFLAGS -pthread -Wl,-E"
	PION_OPT_FLAGS="-O2 -Wall -Wno-strict-aliasing -DNDEBUG"
	PION_DEBUG_FLAGS="-ggdb -Wall -Wno-strict-aliasing -fkeep-inline-functions"
	;;
*)
	# Other (Linux): GCC compiler
	CPPFLAGS="$CPPFLAGS -pthread -D_REENTRANT"
	LDFLAGS="$LDFLAGS -pthread -Wl,-E"
	PION_OPT_FLAGS="-O2 -Wall -Wno-strict-aliasing -DNDEBUG"
	PION_DEBUG_FLAGS="-ggdb -Wall -Wno-strict-aliasing -fkeep-inline-functions"
	PION_EXTERNAL_LIBS="-ldl"
	;;
esac


# Check for debug
AC_MSG_CHECKING([for debugging])
AC_ARG_WITH([debug],
    AC_HELP_STRING([--with-debug],[build with debugging information]),
    [with_debug=$withval],
    [with_debug=no])
if test "$with_debug" = "no"; then
	AC_MSG_RESULT(no)
	CXXFLAGS="$CXXFLAGS $PION_OPT_FLAGS"
else
	AC_MSG_RESULT(yes)
	CXXFLAGS="$CXXFLAGS $PION_DEBUG_FLAGS"
fi


# Check for --with-plugins
AC_MSG_CHECKING([for plug-ins directory])
AC_ARG_WITH([plugins],
    AC_HELP_STRING([--with-plugins@<:@=DIR@:>@],[directory where Pion Plug-ins are installed]),
    [with_plugins=$withval],
    [with_plugins=no])
if test "$with_plugins" = "no"; then
	if test "x$prefix" = "xNONE"; then
		PION_PLUGINS_DIRECTORY=/usr/local/lib/pion/plugins
	else
		PION_PLUGINS_DIRECTORY=${prefix}/lib/pion/plugins
	fi
else
	PION_PLUGINS_DIRECTORY=$with_plugins
fi
AC_MSG_RESULT($PION_PLUGINS_DIRECTORY)
AC_DEFINE_UNQUOTED([PION_PLUGINS_DIRECTORY],["$PION_PLUGINS_DIRECTORY"],[Define to the directory where Pion Plug-ins are installed.])
AC_SUBST(PION_PLUGINS_DIRECTORY)
     

# Check for --with-cygwin
AC_MSG_CHECKING([for cygwin directory])
AC_ARG_WITH([cygwin],
    AC_HELP_STRING([--with-cygwin@<:@=DIR@:>@],[directory where cygwin is installed (Windows only)]),
    [with_cygwin=$withval],
    [with_cygwin=maybe])
if test "$with_cygwin" = "maybe"; then
	case "$build_os" in
	*cygwin*)
		PION_CYGWIN_DIRECTORY="c:/cygwin"
		AC_MSG_RESULT($PION_CYGWIN_DIRECTORY)
		;;
	*)
		AC_MSG_RESULT(no)
		;;
	esac
elif test "$with_cygwin" != "no"; then
	PION_CYGWIN_DIRECTOR="$with_cygwin"
	AC_MSG_RESULT($PION_CYGWIN_DIRECTORY)
else
	AC_MSG_RESULT(no)
fi
AC_DEFINE_UNQUOTED([PION_CYGWIN_DIRECTORY],["$PION_CYGWIN_DIRECTORY"],[Define to the directory where cygwin is installed.])
AC_SUBST(PION_CYGWIN_DIRECTORY)

     
# Check for unordered container support
AC_CHECK_HEADERS([unordered_map],[unordered_map_type=unordered_map],[])
AC_CHECK_HEADERS([ext/hash_map],[unordered_map_type=ext_hash_map],[])
AC_CHECK_HEADERS([hash_map],[unordered_map_type=hash_map],[])
if test "$unordered_map_type" = "unordered_map"; then
	AC_DEFINE([PION_HAVE_UNORDERED_MAP],[1],[Define to 1 if you have the <unordered_map> header file.])
elif test "$unordered_map_type" = "ext_hash_map"; then
	AC_DEFINE([PION_HAVE_EXT_HASH_MAP],[1],[Define to 1 if you have the <ext/hash_map> header file.])
elif test "$unordered_map_type" = "hash_map"; then
	AC_DEFINE([PION_HAVE_HASH_MAP],[1],[Define to 1 if you have the <hash_map> header file.])
else
	AC_MSG_ERROR([C++ compiler does not seem to support unordered containers])
fi


# Check for zlib
AC_MSG_CHECKING([for gzip compression support (zlib)])
AC_ARG_WITH([zlib], AC_HELP_STRING([--with-zlib@<:@=DIR@:>@],[location of zlib library (for gzip compression); use --without-zlib to disable]),
    [ zlib_location=$withval ], [ zlib_location=yes ])
# Check if zlib location is specified
if test "x_$zlib_location" != "x_no"; then
	AC_MSG_RESULT(yes)
	if test "x_$zlib_location" != "x_yes"; then
		# alternative location provided
		CPPFLAGS="$CPPFLAGS -I$zlib_location/include"
		LDFLAGS="$LDFLAGS -L$zlib_location/lib"
	fi
	# Check for zlib headers
	AC_CHECK_HEADERS([zlib.h],,AC_MSG_ERROR([Unable to find the zlib headers]))
	# Check for zlib library
	LIBS="$LIBS_SAVED -lz"
	AC_MSG_CHECKING(linking with zlib)
	AC_TRY_LINK([#include <zlib.h>],[ zlibVersion(); return(0); ],
		[ AC_MSG_RESULT(ok) ],
		[ AC_MSG_RESULT(failed)
		  AC_MSG_ERROR(Unable to link with the zlib library)
		])
	# Found the zlib library
	AC_DEFINE([PION_HAVE_ZLIB],[1],[Define to 1 if you have the `zlib' library.])
else
	# zlib is disabled
	AC_MSG_RESULT(no)
fi


# Check for OpenSSL
AC_MSG_CHECKING([for SSL support (openssl)])
AC_ARG_WITH([openssl],
    AC_HELP_STRING([--with-openssl@<:@=DIR@:>@],[location of OpenSSL library (enables SSL support)]),
    [ openssl_location=$withval ], [ without_openssl=true ])
# Check if openssl location is specified
if test "$without_openssl" != "true"; then
	AC_MSG_RESULT(yes)
	if test "x$openssl_location" != "xyes"; then
		CPPFLAGS="$CPPFLAGS -I$openssl_location/include"
		LDFLAGS="$LDFLAGS -L$openssl_location/lib"
	fi
	# Check for OpenSSL headers
	AC_CHECK_HEADERS([openssl/ssl.h],,AC_MSG_ERROR([Unable to find the OpenSSL headers]))
	# Check for OpenSSL library
	LIBS_SAVED="$LIBS"
	LIBS="$LIBS_SAVED -lssl -lcrypto"
	AC_MSG_CHECKING(linking with openssl)
	AC_TRY_LINK([#include <openssl/ssl.h>],[ SSL_library_init(); return(0); ],
		[ AC_MSG_RESULT(ok) ],
		[ AC_MSG_RESULT(failed)
		  AC_MSG_ERROR(Unable to link with the openssl library)
		])
	LIBS="$LIBS_SAVED"
	PION_SSL_LIB="-lssl -lcrypto"
	# Found the OpenSSL library
	AC_MSG_NOTICE(Building Pion with support for SSL (using OpenSSL))
	AC_DEFINE([PION_HAVE_SSL],[1],[Define to 1 if you have the `OpenSSL' library.])
else
	# SSL is disabled
	AC_MSG_RESULT(no)
fi
AC_SUBST(PION_SSL_LIB)


# Check for Boost
AX_BOOST_BASE([1.34])
# AC_MSG_NOTICE(Boost home directory: $BOOST_HOME_DIR)
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"

# Determine Boost library extension by looking for thread library (i.e. "-gcc41-mt")
AC_MSG_CHECKING([boost library extension])
for boost_thread_library in `ls $BOOST_HOME_DIR/lib/libboost_thread*-mt.{dll,so,a}* 2>/dev/null` ;
do
	if test -r $boost_thread_library ; then
		BOOST_LIB_EXTENSION=`echo $boost_thread_library | sed 's,.*/,,' | sed -e 's;^libboost_thread\(.*\)\.dll*$;\1;' -e 's;^libboost_thread\(.*\)\.so.*$;\1;' -e 's;^libboost_thread\(.*\)\.a*$;\1;'`
		break
	fi
done
if test "x$BOOST_LIB_EXTENSION" = "x"; then
	AC_MSG_ERROR(Unable to determine Boost library extension)
fi
AC_MSG_RESULT($BOOST_LIB_EXTENSION)
AC_SUBST(BOOST_LIB_EXTENSION)


# Check for Boost ASIO library
AC_CHECK_HEADERS([boost/asio.hpp],,AC_MSG_ERROR(Unable to find the boost::asio headers))


# Check for Boost Thread library
BOOST_TRY_LIB=thread
BOOST_TRY_LINK="boost_${BOOST_TRY_LIB}${BOOST_LIB_EXTENSION}"
LIBS_SAVED="$LIBS"
LIBS="$LIBS_SAVED -l${BOOST_TRY_LINK}"
AC_MSG_CHECKING([for boost::${BOOST_TRY_LIB} library])
AC_TRY_LINK([#include <boost/thread/thread.hpp>],
	[ boost::thread current_thread; return 0; ],
	[ AC_MSG_RESULT(ok)
	  BOOST_THREAD_LIB="-l${BOOST_TRY_LINK}"
	],
	[ AC_MSG_RESULT(not found)
	  AC_MSG_ERROR(Unable to link with the boost::${BOOST_TRY_LIB} library)
	])
LIBS="$LIBS_SAVED"
AC_SUBST(BOOST_THREAD_LIB)


# Check for Boost System library
BOOST_TRY_LIB=system
BOOST_TRY_LINK="boost_${BOOST_TRY_LIB}${BOOST_LIB_EXTENSION}"
LIBS_SAVED="$LIBS"
LIBS="$LIBS_SAVED -l${BOOST_TRY_LINK}"
AC_MSG_CHECKING([for boost::${BOOST_TRY_LIB} library])
AC_TRY_LINK([#include <boost/system/error_code.hpp>],
	[ boost::system::error_code error_code; std::string message(error_code.message()); return 0; ],
	[ AC_MSG_RESULT(ok)
	  BOOST_SYSTEM_LIB="-l${BOOST_TRY_LINK}"
	],
	[ AC_MSG_RESULT(not found)
	  AC_MSG_ERROR(Unable to link with the boost::${BOOST_TRY_LIB} library)
	])
LIBS="$LIBS_SAVED"
AC_SUBST(BOOST_SYSTEM_LIB)


# Check for Boost Filesystem library
BOOST_TRY_LIB=filesystem
BOOST_TRY_LINK="boost_${BOOST_TRY_LIB}${BOOST_LIB_EXTENSION}"
LIBS_SAVED="$LIBS"
LIBS="$LIBS_SAVED -l${BOOST_TRY_LINK}"
AC_MSG_CHECKING([for boost::${BOOST_TRY_LIB} library])
AC_TRY_LINK([#include <boost/filesystem/path.hpp>],
	[ boost::filesystem::path a_path; return 0; ],
	[ AC_MSG_RESULT(ok)
	  BOOST_FILESYSTEM_LIB="-l${BOOST_TRY_LINK}"
	],
	[ AC_MSG_RESULT(not found)
	  AC_MSG_ERROR(Unable to link with the boost::${BOOST_TRY_LIB} library)
	])
LIBS="$LIBS_SAVED"
AC_SUBST(BOOST_FILESYSTEM_LIB)


# Check for Boost Regex library
BOOST_TRY_LIB=regex
BOOST_TRY_LINK="boost_${BOOST_TRY_LIB}${BOOST_LIB_EXTENSION}"
LIBS_SAVED="$LIBS"
LIBS="$LIBS_SAVED -l${BOOST_TRY_LINK}"
AC_MSG_CHECKING([for boost::${BOOST_TRY_LIB} library])
AC_TRY_LINK([#include <boost/regex.hpp>],
	[ boost::regex exp(".*"); return 0; ],
	[ AC_MSG_RESULT(ok)
	  BOOST_REGEX_LIB="-l${BOOST_TRY_LINK}"
	],
	[ AC_MSG_RESULT(not found)
	  AC_MSG_ERROR(Unable to link with the boost::${BOOST_TRY_LIB} library)
	])
LIBS="$LIBS_SAVED"
AC_SUBST(BOOST_REGEX_LIB)


# Check for Boost Date Time library
BOOST_TRY_LIB=date_time
BOOST_TRY_LINK="boost_${BOOST_TRY_LIB}${BOOST_LIB_EXTENSION}"
LIBS_SAVED="$LIBS"
LIBS="$LIBS_SAVED -l${BOOST_TRY_LINK}"
AC_MSG_CHECKING([for boost::${BOOST_TRY_LIB} library])
AC_TRY_LINK([#include <boost/date_time/gregorian/greg_date.hpp>],
	[ boost::gregorian::date d(2007, 11, 11); return 0; ],
	[ AC_MSG_RESULT(ok)
	  BOOST_DATE_TIME_LIB="-l${BOOST_TRY_LINK}"
	],
	[ AC_MSG_RESULT(not found)
	  AC_MSG_ERROR(Unable to link with the boost::${BOOST_TRY_LIB} library)
	])
LIBS="$LIBS_SAVED"
AC_SUBST(BOOST_DATE_TIME_LIB)


# Check for Boost Signals library
BOOST_TRY_LIB=signals
BOOST_TRY_LINK="boost_${BOOST_TRY_LIB}${BOOST_LIB_EXTENSION}"
LIBS_SAVED="$LIBS"
LIBS="$LIBS_SAVED -l${BOOST_TRY_LINK}"
AC_MSG_CHECKING([for boost::${BOOST_TRY_LIB} library])
AC_TRY_LINK([#include <boost/signal.hpp>],
	[ boost::signal0<void> sig; sig(); return 0; ],
	[ AC_MSG_RESULT(ok)
	  BOOST_SIGNALS_LIB="-l${BOOST_TRY_LINK}"
	],
	[ AC_MSG_RESULT(not found)
	  AC_MSG_ERROR(Unable to link with the boost::${BOOST_TRY_LIB} library)
	])
LIBS="$LIBS_SAVED"
AC_SUBST(BOOST_SIGNALS_LIB)


# Check for Boost Unit Test Framework
AC_ARG_ENABLE([tests],
    AC_HELP_STRING([--disable-tests],[do not build and run the unit tests]),
    [enable_tests=$enableval], [enable_tests=yes])
if test "x$enable_tests" == "xno"; then
	# Display notice if unit tests are disabled
	AC_MSG_NOTICE([Unit tests are disabled])
else
	BOOST_TRY_LIB=unit_test_framework
	BOOST_TRY_LINK="$BOOST_HOME_DIR/lib/libboost_${BOOST_TRY_LIB}${BOOST_LIB_EXTENSION}.a"
	LIBS_SAVED="$LIBS"
	LIBS="$LIBS_SAVED ${BOOST_TRY_LINK}"
	AC_MSG_CHECKING([for boost::test library])
	AC_TRY_LINK([#include <boost/test/unit_test.hpp>
		using namespace boost::unit_test;
		test_suite* init_unit_test_suite( int argc, char* argv[] )
		{ return BOOST_TEST_SUITE("Master test suite"); }],
		[],
		[ AC_MSG_RESULT(ok)
		  BOOST_TEST_LIB="${BOOST_TRY_LINK}"
		],
		[ AC_MSG_RESULT(not found)
		  AC_MSG_ERROR(Unable to link with the boost::${BOOST_TRY_LIB} library)
		])
	LIBS="$LIBS_SAVED"
	PION_TESTS_MAKEDIRS="tests"
fi
AC_SUBST(BOOST_TEST_LIB)
AC_SUBST(PION_TESTS_MAKEDIRS)


# Check for logging support
AC_ARG_ENABLE([logging],
    AC_HELP_STRING([--disable-logging],[disable all logging support (including ostream)]),
    [enable_logging=$enableval], [enable_logging=yes])
AC_ARG_WITH([log4cxx],
    AC_HELP_STRING([--with-log4cxx@<:@=DIR@:>@],[location of log4cxx library (recommended)]),
    [ log4cxx_location=$withval ], [ without_log4cxx=true ])
AC_ARG_WITH([log4cplus],
    AC_HELP_STRING([--with-log4cplus@<:@=DIR@:>@],[location of log4cplus library]),
    [ log4cplus_location=$withval ], [ without_log4cplus=true ])
AC_ARG_WITH([log4cpp],
    AC_HELP_STRING([--with-log4cpp@<:@=DIR@:>@],[location of log4cpp library]),
    [ log4cpp_location=$withval ], [ without_log4cpp=true ])
AC_MSG_CHECKING([for logging support])
if test "x$enable_logging" == "xno"; then
	# Display notice if no logging found
	AC_MSG_RESULT(no)
	AC_DEFINE([PION_DISABLE_LOGGING],[1],[Define to 1 to disable logging.])
elif test "$without_log4cxx" != "true"; then
	AC_MSG_RESULT(log4cxx)

	# Check if log4cxx location is specified
	if test "x$log4cxx_location" != "xyes"
	then
		CPPFLAGS="$CPPFLAGS -I$log4cxx_location/include"
		LDFLAGS="$LDFLAGS -L$log4cxx_location/lib"
	fi

	# Check for log4cxx headers
	AC_CHECK_HEADERS([log4cxx/logger.h],,AC_MSG_ERROR([Unable to find the log4cxx headers]))
	
	# Check for log4cxx library
	LIBS_SAVED="$LIBS"
	LIBS="$LIBS_SAVED -llog4cxx"
	AC_MSG_CHECKING(linking with log4cxx)
	AC_TRY_LINK([#include <log4cxx/logger.h>],[ log4cxx::Logger::getRootLogger(); return 0; ],
		[ AC_MSG_RESULT(ok) ],
		[ AC_MSG_RESULT(failed)
		  AC_MSG_ERROR(Unable to link with the log4cxx library)
		])
	LIBS="$LIBS_SAVED"
	PION_LOG_LIB="-llog4cxx"

	# Found the log4cxx library
	AC_DEFINE([PION_USE_LOG4CXX],[1],[Define to 1 if you have the `log4cxx' library (-llog4cxx).])
elif test "$without_log4cplus" != "true"; then
	AC_MSG_RESULT(log4cplus)

	# Check if log4cplus location is specified
	if test "x$log4cplus_location" != "xyes"
	then
		CPPFLAGS="$CPPFLAGS -I$log4cplus_location/include"
		LDFLAGS="$LDFLAGS -L$log4cplus_location/lib"
	fi

	# Check for log4cplus headers
	AC_CHECK_HEADERS([log4cplus/logger.h],,AC_MSG_ERROR([Unable to find the log4cplus headers]))
	
	# Check for log4cplus library
	LIBS_SAVED="$LIBS"
	LIBS="$LIBS_SAVED -llog4cplus"
	AC_MSG_CHECKING(linking with log4cplus)
	AC_TRY_LINK([#include <log4cplus/logger.h>],[ log4cplus::Logger::getRoot(); return 0; ],
		[ AC_MSG_RESULT(ok) ],
		[ AC_MSG_RESULT(failed)
		  AC_MSG_ERROR(Unable to link with the log4cplus library)
		])
	LIBS="$LIBS_SAVED"
	PION_LOG_LIB="-llog4cplus"

	# Found the log4cplus library
	AC_DEFINE([PION_USE_LOG4CPLUS],[1],[Define to 1 if you have the `log4cplus' library (-llog4cplus).])
elif test "$without_log4cpp" != "true"; then
	AC_MSG_RESULT(log4cpp)

	# Check if log4cpp location is specified
	if test "x$log4cpp_location" != "xyes"
	then
		CPPFLAGS="$CPPFLAGS -I$log4cpp_location/include"
		LDFLAGS="$LDFLAGS -L$log4cpp_location/lib"
	fi

	# Check for log4cpp headers
	AC_CHECK_HEADERS([log4cpp/Category.hh],,AC_MSG_ERROR([Unable to find the log4cpp headers]))
	
	# Check for log4cpp library
	LIBS_SAVED="$LIBS"
	LIBS="$LIBS_SAVED -llog4cpp"
	AC_MSG_CHECKING(linking with log4cpp)
	AC_TRY_LINK([#include <log4cpp/Category.hh>],[ log4cpp::Category::getRoot(); return 0; ],
		[ AC_MSG_RESULT(ok) ],
		[ AC_MSG_RESULT(failed)
		  AC_MSG_ERROR(Unable to link with the log4cpp library)
		])
	LIBS="$LIBS_SAVED"
	PION_LOG_LIB="-llog4cpp"

	# Found the log4cpp library
	AC_DEFINE([PION_USE_LOG4CPP],[1],[Define to 1 if you have the `log4cpp' library (-llog4cpp).])
else
	AC_MSG_RESULT(using std::ostream)
	AC_DEFINE([PION_USE_OSTREAM_LOGGING],[1],[Define to 1 to use std::cout and std::cerr for logging.])
fi
AC_SUBST(PION_LOG_LIB)


# Set external library dependencies
PION_EXTERNAL_LIBS="$PION_EXTERNAL_LIBS $BOOST_THREAD_LIB $BOOST_SYSTEM_LIB $BOOST_FILESYSTEM_LIB $BOOST_REGEX_LIB $BOOST_DATE_TIME_LIB $BOOST_SIGNALS_LIB $PION_SSL_LIB $PION_LOG_LIB"
AC_SUBST(PION_EXTERNAL_LIBS)
