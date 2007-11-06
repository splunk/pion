# --------------------------------
# Pion autoconf configuration file
# --------------------------------

# Set Pion version information
PION_LIBRARY_VERSION=$PACKAGE_VERSION
AC_SUBST(PION_LIBRARY_VERSION)
AC_DEFINE_UNQUOTED([PION_VERSION],["$PACKAGE_VERSION"],[Define to the version number of pion-net.])

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
		PION_OPT_FLAGS="-O2"
		PION_DEBUG_FLAGS="-g"
	else
		# Solaris: GCC compiler
		CPPFLAGS="$CPPFLAGS -pthreads -D_REENTRANT"
		LDFLAGS="$LDFLAGS -pthreads"
		PION_OPT_FLAGS="-O2 -Wall"
		PION_DEBUG_FLAGS="-ggdb -Wall -fno-inline"
	fi
	LIBS="$LIBS -lsocket -lnsl -ldl -lposix4"
	;;
*darwin*)
	# Mac OS X: GCC compiler
	# -pthread is implied (automatically set by compiler)
	CPPFLAGS="$CPPFLAGS -D_REENTRANT"
	#LDFLAGS="$LDFLAGS"
	PION_OPT_FLAGS="-O2 -Wall"
	PION_DEBUG_FLAGS="-ggdb -Wall -fno-inline"
	LIBS="$LIBS -ldl"
	;;
*cygwin*)
	# Cygwin GCC compiler (Windows)
	CPPFLAGS="$CPPFLAGS -mthreads -D_REENTRANT -DPION_WIN32 -D__USE_W32_SOCKETS"
	LDFLAGS="$LDFLAGS -mthreads"
	PION_OPT_FLAGS="-O2 -Wall"
	PION_DEBUG_FLAGS="-ggdb -Wall -fno-inline"
	LIBS="$LIBS -lws2_32 -lmswsock"
	;;
*freebsd*)
	# FreeBSD: GCC compiler
	CPPFLAGS="$CPPFLAGS -pthread -D_REENTRANT"
	LDFLAGS="$LDFLAGS -pthread"
	PION_OPT_FLAGS="-O2 -Wall"
	PION_DEBUG_FLAGS="-ggdb -Wall -fno-inline"
	;;
*)
	# Other (Linux): GCC compiler
	CPPFLAGS="$CPPFLAGS -pthread -D_REENTRANT"
	LDFLAGS="$LDFLAGS -pthread"
	PION_OPT_FLAGS="-O2 -Wall"
	PION_DEBUG_FLAGS="-ggdb -Wall -fno-inline"
	LIBS="$LIBS -ldl"
	;;
esac


# Check for debug
AC_ARG_WITH([debug],
    AC_HELP_STRING([--with-debug],[build with debugging information]),
    [with_debug=$withval],
    [with_debug=no])
if test "$with_debug" = "no"; then
	CXXFLAGS="$CXXFLAGS $PION_OPT_FLAGS"
else
	AC_MSG_NOTICE(Building with debugging information)
	CXXFLAGS="$CXXFLAGS $PION_DEBUG_FLAGS"
fi


# Check for --with-plugins
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
AC_MSG_NOTICE([Pion Plug-ins directory: $PION_PLUGINS_DIRECTORY])
AC_DEFINE_UNQUOTED([PION_PLUGINS_DIRECTORY],["$PION_PLUGINS_DIRECTORY"],[Define to the directory where Pion Plug-ins are installed.])
AC_SUBST(PION_PLUGINS_DIRECTORY)
     

# Check for --with-cygwin
AC_ARG_WITH([cygwin],
    AC_HELP_STRING([--with-cygwin@<:@=DIR@:>@],[directory where cygwin is installed (Windows only)]),
    [with_cygwin=$withval],
    [with_cygwin=maybe])
if test "$with_cygwin" = "maybe"; then
	case "$build_os" in
	*cygwin*)
		PION_CYGWIN_DIRECTORY="c:/cygwin"
		AC_MSG_NOTICE([Cygwin root directory: $PION_CYGWIN_DIRECTORY])
		;;
	*)
		;;
	esac
elif test "$with_cygwin" != "no"; then
	PION_CYGWIN_DIRECTOR="$with_cygwin"
	AC_MSG_NOTICE([Cygwin root directory: $PION_CYGWIN_DIRECTORY])
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


# Check for Boost
AX_BOOST_BASE([1.34])
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"


# Check for Boost ASIO library
AC_CHECK_HEADERS([boost/asio.hpp],,AC_MSG_ERROR(Unable to find the boost::asio headers))


# Check for Boost Thread library
AC_CHECK_HEADERS([boost/thread/thread.hpp],,AC_MSG_ERROR(Unable to find the boost::thread headers))
LIBS_SAVED="$LIBS"
BOOST_LIB=boost_thread
for ax_lib in $BOOST_LIB-$CC-mt $BOOST_LIB-mt lib$BOOST_LIB-$CC-mt \
	lib$BOOST_LIB-mt $BOOST_LIB-$CC $BOOST_LIB lib$BOOST_LIB-$CC \
	lib$BOOST_LIB \
	$BOOST_LIB-${CC}34-mt lib$BOOST_LIB-${CC}34-mt \
	$BOOST_LIB-${CC}34 lib$BOOST_LIB-${CC}34 \
	$BOOST_LIB-${CC}41-mt lib$BOOST_LIB-${CC}41-mt \
	$BOOST_LIB-${CC}41 lib$BOOST_LIB-${CC}41;
do
	LIBS="$LIBS_SAVED -l$ax_lib"
	AC_MSG_NOTICE(Checking for -l$ax_lib)
	AC_TRY_LINK([#include <boost/thread/thread.hpp>],
		[ boost::thread current_thread; return 0; ],
		[BOOST_THREAD_LIB="-l$ax_lib"; break],
		[BOOST_THREAD_LIB=""])
done
if test "x$BOOST_THREAD_LIB" = "x"; then
	AC_MSG_ERROR(Unable to link with the boost::thread library)
else
	AC_MSG_NOTICE(Linking with boost::thread works)
fi
LIBS="$LIBS_SAVED"
AC_SUBST(BOOST_THREAD_LIB)


# Check for Boost System library
AC_CHECK_HEADERS([boost/system/error_code.hpp],,AC_MSG_ERROR(Unable to find the boost::system headers))
LIBS_SAVED="$LIBS"
BOOST_LIB=boost_system
for ax_lib in $BOOST_LIB-$CC-mt $BOOST_LIB-mt lib$BOOST_LIB-$CC-mt \
	lib$BOOST_LIB-mt $BOOST_LIB-$CC $BOOST_LIB lib$BOOST_LIB-$CC \
	lib$BOOST_LIB \
	$BOOST_LIB-${CC}34-mt lib$BOOST_LIB-${CC}34-mt \
	$BOOST_LIB-${CC}34 lib$BOOST_LIB-${CC}34 \
	$BOOST_LIB-${CC}41-mt lib$BOOST_LIB-${CC}41-mt \
	$BOOST_LIB-${CC}41 lib$BOOST_LIB-${CC}41;
do
	LIBS="$LIBS_SAVED -l$ax_lib"
	AC_MSG_NOTICE(Checking for -l$ax_lib)
	AC_TRY_LINK([#include <boost/system/error_code.hpp>],
		[ boost::system::error_code error_code; std::string message(error_code.message()); return 0; ],
		[BOOST_SYSTEM_LIB="-l$ax_lib"; break],
		[BOOST_SYSTEM_LIB=""])
done
if test "x$BOOST_SYSTEM_LIB" = "x"; then
	AC_MSG_ERROR(Unable to link with the boost::system library)
else
	AC_MSG_NOTICE(Linking with boost::system works)
fi
LIBS="$LIBS_SAVED"
AC_SUBST(BOOST_SYSTEM_LIB)


# Check for Boost Filesystem library
AC_CHECK_HEADERS([boost/filesystem/path.hpp],,AC_MSG_ERROR(Unable to find the boost::filesystem headers))
LIBS_SAVED="$LIBS"
BOOST_LIB=boost_filesystem
for ax_lib in $BOOST_LIB-$CC-mt $BOOST_LIB-mt lib$BOOST_LIB-$CC-mt \
	lib$BOOST_LIB-mt $BOOST_LIB-$CC $BOOST_LIB lib$BOOST_LIB-$CC \
	lib$BOOST_LIB \
	$BOOST_LIB-${CC}34-mt lib$BOOST_LIB-${CC}34-mt \
	$BOOST_LIB-${CC}34 lib$BOOST_LIB-${CC}34 \
	$BOOST_LIB-${CC}41-mt lib$BOOST_LIB-${CC}41-mt \
	$BOOST_LIB-${CC}41 lib$BOOST_LIB-${CC}41;
do
	LIBS="$LIBS_SAVED -l$ax_lib"
	AC_MSG_NOTICE(Checking for -l$ax_lib)
	AC_TRY_LINK([#include <boost/filesystem/path.hpp>],
		[ boost::filesystem::path a_path; return 0; ],
		[BOOST_FILESYSTEM_LIB="-l$ax_lib"; break],
		[BOOST_FILESYSTEM_LIB=""])
done
if test "x$BOOST_FILESYSTEM_LIB" = "x"; then
	AC_MSG_ERROR(Unable to link with the boost::filesystem library)
else
	AC_MSG_NOTICE(Linking with boost::filesystem works)
fi
LIBS="$LIBS_SAVED"
AC_SUBST(BOOST_FILESYSTEM_LIB)


# Check for Boost Regex library
AC_CHECK_HEADERS([boost/regex.hpp],,AC_MSG_ERROR(Unable to find the boost::regex headers))
LIBS_SAVED="$LIBS"
BOOST_LIB=boost_regex
for ax_lib in $BOOST_LIB-$CC-mt $BOOST_LIB-mt lib$BOOST_LIB-$CC-mt \
	lib$BOOST_LIB-mt $BOOST_LIB-$CC $BOOST_LIB lib$BOOST_LIB-$CC \
	lib$BOOST_LIB \
	$BOOST_LIB-${CC}34-mt lib$BOOST_LIB-${CC}34-mt \
	$BOOST_LIB-${CC}34 lib$BOOST_LIB-${CC}34 \
	$BOOST_LIB-${CC}41-mt lib$BOOST_LIB-${CC}41-mt \
	$BOOST_LIB-${CC}41 lib$BOOST_LIB-${CC}41;
do
	LIBS="$LIBS_SAVED -l$ax_lib"
	AC_MSG_NOTICE(Checking for -l$ax_lib)
	AC_TRY_LINK([#include <boost/regex.hpp>],
		[ boost::regex exp(".*"); return 0; ],
		[BOOST_REGEX_LIB="-l$ax_lib"; break],
		[BOOST_REGEX_LIB=""])
done
if test "x$BOOST_REGEX_LIB" = "x"; then
	AC_MSG_ERROR(Unable to link with the boost::regex library)
else
	AC_MSG_NOTICE(Linking with boost::regex works)
fi
LIBS="$LIBS_SAVED"
AC_SUBST(BOOST_REGEX_LIB)


# Check for Boost Unit Test Framework
AC_ARG_ENABLE([tests],
    AC_HELP_STRING([--disable-tests],[do not build and run the unit tests]),
    [enable_tests=$enableval], [enable_tests=yes])
if test "x$enable_tests" == "xno"; then
	# Display notice if unit tests are disabled
	AC_MSG_NOTICE([Unit tests are disabled])
else
	AC_CHECK_HEADERS([boost/test/unit_test.hpp],,AC_MSG_ERROR(Unable to find the boost::unit_test headers))
	LIBS_SAVED="$LIBS"
	BOOST_LIB=boost_unit_test_framework
	for ax_lib in $BOOST_LIB-$CC-mt $BOOST_LIB-mt lib$BOOST_LIB-$CC-mt \
		lib$BOOST_LIB-mt $BOOST_LIB-$CC $BOOST_LIB lib$BOOST_LIB-$CC \
		lib$BOOST_LIB \
		$BOOST_LIB-${CC}34-mt lib$BOOST_LIB-${CC}34-mt \
		$BOOST_LIB-${CC}34 lib$BOOST_LIB-${CC}34 \
		$BOOST_LIB-${CC}41-mt lib$BOOST_LIB-${CC}41-mt \
		$BOOST_LIB-${CC}41 lib$BOOST_LIB-${CC}41;
	do
		LIBS="$LIBS_SAVED -l$ax_lib"
		AC_MSG_NOTICE(Checking for -l$ax_lib)
		AC_TRY_LINK([#include <boost/test/unit_test.hpp>
			using namespace boost::unit_test;
			test_suite* init_unit_test_suite( int argc, char* argv[] )
			{ return BOOST_TEST_SUITE("Master test suite"); }],
			[],
			[BOOST_TEST_LIB="-l$ax_lib"; break],
			[BOOST_TEST_LIB=""])
	done
	if test "x$BOOST_TEST_LIB" == "x"; then
		AC_MSG_ERROR(Unable to link with the boost::unit_test library)
	else
                
		# Check for dynamic library & set cppflags for unit tests
		CPPFLAGS_SAVED=$CPPFLAGS
		CPPFLAGS="$CPPFLAGS -DBOOST_TEST_DYN_LINK"
		AC_TRY_LINK([#define BOOST_TEST_MODULE test-if-dynamic
			#include <boost/test/unit_test.hpp>],
			[],
			[BOOST_TEST_CPPFLAGS="-DBOOST_TEST_DYN_LINK"],
			[BOOST_TEST_CPPFLAGS=""])
		CPPFLAGS=$CPPFLAGS_SAVED
		AC_MSG_NOTICE(Linking with boost::unit_test works)
	fi
	LIBS="$LIBS_SAVED"
	PION_TESTS_MAKEDIRS="tests"
fi
AC_SUBST(BOOST_TEST_LIB)
AC_SUBST(BOOST_TEST_CPPFLAGS)
AC_SUBST(PION_TESTS_MAKEDIRS)


# Check for OpenSSL
AC_ARG_WITH([openssl],
    AC_HELP_STRING([--with-openssl@<:@=DIR@:>@],[location of OpenSSL library (enables SSL support)]),
    [ openssl_location=$withval ], [ without_openssl=true ])
# Check if openssl location is specified
if test "$without_openssl" != "true"; then
	if test "x$openssl_location" != "xyes"; then
		CPPFLAGS="$CPPFLAGS -I$openssl_location/include"
		LDFLAGS="$LDFLAGS -L$openssl_location/lib"
	fi
	# Check for OpenSSL headers
	AC_CHECK_HEADERS([openssl/ssl.h],,AC_MSG_ERROR([Unable to find the OpenSSL headers]))
	# Check for OpenSSL library
	LIBS_SAVED="$LIBS"
	LIBS="$LIBS_SAVED -lssl -lcrypto"
	AC_TRY_LINK([#include <openssl/ssl.h>],[ SSL_library_init(); return(0); ],
		AC_MSG_NOTICE(Linking with OpenSSL works),
		AC_MSG_ERROR(Unable to link with the OpenSSL library))
	LIBS="$LIBS_SAVED"
	PION_SSL_LIB="-lssl -lcrypto"
	# Found the OpenSSL library
	AC_MSG_NOTICE(SSL encryption support enabled (found OpenSSL))
	AC_DEFINE([PION_HAVE_SSL],[1],[Define to 1 if you have the `OpenSSL' library.])
else
	# Display notice if SSL is disabled
	AC_MSG_NOTICE([SSL support is disabled (try --with-openssl)])
fi
AC_SUBST(PION_SSL_LIB)


# Check for Apache Portable Runtime (APR) library
AC_ARG_WITH([apr],
    AC_HELP_STRING([--with-apr@<:@=DIR@:>@],[location of Apache Portable Runtime library]),
    [ apr_location=$withval ], [ without_apr=true ])
# Check if APR location is specified
if test "$without_apr" != "true"; then
	# Check for apr headers
	if test "x$apr_location" != "xyes"; then
		CPPFLAGS="$CPPFLAGS -I$apr_location/include"
		LDFLAGS="$LDFLAGS -L$apr_location/lib"
	fi
	AC_CHECK_HEADERS([apr-1/apr_atomic.h],,AC_MSG_ERROR([Unable to find the APR headers]))
	# Check for APR library
	LIBS_SAVED="$LIBS"
	LIBS="$LIBS_SAVED -lapr-1"
	AC_TRY_LINK([#include <apr-1/apr_atomic.h>],[ apr_uint32_t n; apr_atomic_set32(&n, 0); return(0); ],
		AC_MSG_NOTICE(Linking with APR works),
		AC_MSG_ERROR(Unable to link with the APR library))
	LIBS="$LIBS_SAVED"
	PION_APR_LIB="-lapr-1"
	# Found the APR library
	AC_MSG_NOTICE([Building Pion with support for the Apache Portable Runtime (APR)])
	AC_DEFINE([PION_HAVE_APR],[1],[Define to 1 if you have the `Apache Portable Runtime' library.])
else
	# Display notice if APR is disabled
	AC_MSG_NOTICE([Building Pion without the Apache Portable Runtime (APR)])
fi
AC_SUBST(PION_APR_LIB)


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
if test "x$enable_logging" == "xno"; then
	# Display notice if no logging found
	AC_MSG_NOTICE([Logging is disabled])
	AC_DEFINE([PION_DISABLE_LOGGING],[1],[Define to 1 to disable logging.])
elif test "$without_log4cxx" != "true"; then
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
	AC_TRY_LINK([#include <log4cxx/logger.h>],[ log4cxx::Logger::getRootLogger(); return 0; ],
		AC_MSG_NOTICE(Linking with log4cxx works),
		AC_MSG_ERROR(Unable to link with the log4cxx library))
	LIBS="$LIBS_SAVED"
	PION_LOG_LIB="-llog4cxx"

	# Found the log4cxx library
	AC_MSG_NOTICE(Using the log4cxx library for logging)
	AC_DEFINE([PION_USE_LOG4CXX],[1],[Define to 1 if you have the `log4cxx' library (-llog4cxx).])
elif test "$without_log4cplus" != "true"; then
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
	AC_TRY_LINK([#include <log4cplus/logger.h>],[ log4cplus::Logger::getRoot(); return 0; ],
		AC_MSG_NOTICE(Linking with log4cplus works),
		AC_MSG_ERROR(Unable to link with the log4cplus library))
	LIBS="$LIBS_SAVED"
	PION_LOG_LIB="-llog4cplus"

	# Found the log4cplus library
	AC_MSG_NOTICE(Using the log4cplus library for logging)
	AC_DEFINE([PION_USE_LOG4CPLUS],[1],[Define to 1 if you have the `log4cplus' library (-llog4cplus).])
elif test "$without_log4cpp" != "true"; then
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
	AC_TRY_LINK([#include <log4cpp/Category.hh>],[ log4cpp::Category::getRoot(); return 0; ],
		AC_MSG_NOTICE(Linking with log4cpp works),
		AC_MSG_ERROR(Unable to link with the log4cpp library))
	LIBS="$LIBS_SAVED"
	PION_LOG_LIB="-llog4cpp"

	# Found the log4cpp library
	AC_MSG_NOTICE(Using the log4cpp library for logging)
	AC_DEFINE([PION_USE_LOG4CPP],[1],[Define to 1 if you have the `log4cpp' library (-llog4cpp).])
else
	AC_MSG_NOTICE(Using std::cout and std::cerr for logging)
	AC_DEFINE([PION_USE_OSTREAM_LOGGING],[1],[Define to 1 to use std::cout and std::cerr for logging.])
fi
AC_SUBST(PION_LOG_LIB)


# Set external library dependencies
PION_EXTERNAL_LIBS="$BOOST_THREAD_LIB $BOOST_SYSTEM_LIB $BOOST_FILESYSTEM_LIB $BOOST_REGEX_LIB $PION_SSL_LIB $PION_APR_LIB $PION_LOG_LIB"
AC_SUBST(PION_EXTERNAL_LIBS)
