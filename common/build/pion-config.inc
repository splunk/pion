# --------------------------------
# Pion autoconf configuration file
# --------------------------------

# Set Pion version information
PION_LIBRARY_VERSION=`echo $PACKAGE_VERSION | sed 's;^\([[0-9]][[0-9]]*\.[[0-9]][[0-9]]*\).*$;\1;'`
# AC_MSG_NOTICE("Pion Library Version: $PION_LIBRARY_VERSION")
AC_SUBST(PION_LIBRARY_VERSION)
AC_DEFINE_UNQUOTED([PION_VERSION],["$PACKAGE_VERSION"],[Define to the version number of Pion.])

# Note: AM_CONFIG_HEADER is deprecated
AC_CONFIG_HEADERS([common/include/pion/PionConfig.hpp])


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


# Check for malloc_trim support
AC_MSG_CHECKING(for malloc_trim() support)
AC_TRY_LINK([#include <malloc.h>],
	[
	malloc_trim(0);
	],
	[ AC_MSG_RESULT(yes)
	  AC_DEFINE([PION_HAVE_MALLOC_TRIM],[1],[Define to 1 if C library supports malloc_trim()])
	],
	[ AC_MSG_RESULT(no) ])

     
# Check for unordered container support
AC_CHECK_HEADERS([tr1/unordered_map],[unordered_map_type=tr1_unordered_map],[])
if test "x$unordered_map_type" = "xtr1_unordered_map"; then
	# test to make sure it's good enough
	AC_MSG_CHECKING(if unordered_map is ok)
	AC_TRY_LINK([#include <tr1/unordered_map>],[
		#if defined(__APPLE__)
			#error completely broken on OSX, even for gcc 4.2
		#elif !defined(__GNUC__) || (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
			return(0);
		#else
			#error only use gcc versions 4.1 or greater
		#endif
		],
		[ AC_MSG_RESULT(ok) ],
		[ AC_MSG_RESULT(failed)
		  unordered_map_type=""
		])
fi
if test "x$unordered_map_type" = "x"; then
	AC_CHECK_HEADERS([ext/hash_map],[unordered_map_type=ext_hash_map],[])
fi
if test "x$unordered_map_type" = "x"; then
	AC_CHECK_HEADERS([hash_map],[unordered_map_type=hash_map],[])
fi
if test "$unordered_map_type" = "tr1_unordered_map"; then
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
	LIBS="$LIBS_SAVED"
	PION_ZLIB="-lz"
	# Found the zlib library
	AC_DEFINE([PION_HAVE_ZLIB],[1],[Define to 1 if you have the `zlib' library.])
else
	# zlib is disabled
	AC_MSG_RESULT(no)
fi
AC_SUBST(PION_ZLIB)


# Check for bzlib
AC_MSG_CHECKING([for bzip2 compression support (bzlib)])
AC_ARG_WITH([bzlib], AC_HELP_STRING([--with-bzlib@<:@=DIR@:>@],[location of bzlib library (for bzip2 compression); use --without-bzlib to disable]),
    [ bzlib_location=$withval ], [ bzlib_location=yes ])
# Check if bzlib location is specified
if test "x_$bzlib_location" != "x_no"; then
	AC_MSG_RESULT(yes)
	if test "x_$bzlib_location" != "x_yes"; then
		# alternative location provided
		CPPFLAGS="$CPPFLAGS -I$bzlib_location/include"
		LDFLAGS="$LDFLAGS -L$bzlib_location/lib"
	fi
	# Check for bzlib headers
	AC_CHECK_HEADERS([bzlib.h],,AC_MSG_ERROR([Unable to find the bzlib headers]))
	# Check for bzlib library
	LIBS="$LIBS_SAVED -lbz2"
	AC_MSG_CHECKING(linking with bzlib)
	AC_TRY_LINK([#include <bzlib.h>],[ BZ2_bzlibVersion(); return(0); ],
		[ AC_MSG_RESULT(ok) ],
		[ AC_MSG_RESULT(failed)
		  AC_MSG_ERROR(Unable to link with the bzlib library; use --without-bzlib to disable)
		])
	LIBS="$LIBS_SAVED"
	PION_BZLIB="-lbz2"
	# Found the bzlib library
	AC_DEFINE([PION_HAVE_BZLIB],[1],[Define to 1 if you have the `bzlib' library.])
else
	# bzlib is disabled
	AC_MSG_RESULT(no)
fi
AC_SUBST(PION_BZLIB)


# Check for OpenSSL
AC_MSG_CHECKING([for SSL support (openssl)])
AC_ARG_WITH([openssl], AC_HELP_STRING([--with-openssl@<:@=DIR@:>@],[location of OpenSSL library (enables SSL support); use --without-openssl to disable]),
    [ openssl_location=$withval ], [ openssl_location=yes ])
# Check if openssl location is specified
if test "x_$openssl_location" != "x_no"; then
	AC_MSG_RESULT(yes)
	if test "x_$openssl_location" != "x_yes"; then
		# alternative location provided
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
		  AC_MSG_ERROR(Unable to link with the openssl library; use --without-openssl to disable)
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


# Check for boost::lockfree library
AC_ARG_ENABLE([lockfree],
    AC_HELP_STRING([--disable-lockfree],[disable lock-free support]),
    [enable_lockfree=$enableval], [enable_lockfree=no])
AC_MSG_CHECKING([for boost::lockfree library])
if test "x$enable_lockfree" == "xno"; then
	# Display notice if lock-free is disabled
	AC_MSG_RESULT(disabled)
else
	CPPFLAGS_SAVED="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS -I$PION_COMMON_HOME/include"
	AC_TRY_LINK([#include <boost/lockfree/detail/tagged_ptr.hpp>],
		[
		BOOST_STATIC_ASSERT(boost::lockfree::tagged_ptr<int>::is_lockfree);
		boost::lockfree::tagged_ptr<int> ptr_a(NULL);
		boost::lockfree::tagged_ptr<int> ptr_b = ptr_a;
		int * ptr_c = NULL;
		ptr_a.cas(ptr_b, ptr_c);
		return 0;
		],[ AC_MSG_RESULT(yes)
			AC_DEFINE([PION_HAVE_LOCKFREE],[1],[Define to 1 if you have the `boost.lockfree' library.]) ],
		  [ AC_MSG_RESULT(no) ])
	CPPFLAGS="$CPPFLAGS_SAVED"
fi


# Check for logging support
AC_ARG_ENABLE([logging],
    AC_HELP_STRING([--disable-logging],[disable all logging support (including std::ostream)]),
    [enable_logging=$enableval], [enable_logging=yes])
AC_ARG_WITH([ostream-logging],
    AC_HELP_STRING([--with-ostream-logging],[use std::ostream logging instead of library]),
    [ ostream_logging=yes ], [ ostream_logging=no ])
AC_ARG_WITH([log4cplus],
    AC_HELP_STRING([--with-log4cplus@<:@=DIR@:>@],[location of log4cplus library (recommended)]),
    [ log4cplus_location=$withval without_log4cxx=true without_log4cpp=true],
    [])
AC_ARG_WITH([log4cxx],
    AC_HELP_STRING([--with-log4cxx@<:@=DIR@:>@],[location of log4cxx library]),
    [ log4cxx_location=$withval without_log4cplus=true without_log4cpp=true],
    [])
AC_ARG_WITH([log4cpp],
    AC_HELP_STRING([--with-log4cpp@<:@=DIR@:>@],[location of log4cpp library]),
    [ log4cpp_location=$withval without_log4cplus=true without_log4cxx=true],
    [])

if test "x$enable_logging" == "xno"; then
	# Display notice if no logging found
	AC_MSG_NOTICE(Logging has been disabled)
	AC_DEFINE([PION_DISABLE_LOGGING],[1],[Define to 1 to disable logging.])
elif test "x$ostream_logging" == "xyes"; then
	AC_MSG_NOTICE(Using std::ostream for logging)
	AC_DEFINE([PION_USE_OSTREAM_LOGGING],[1],[Define to 1 to use std::cout and std::cerr for logging.])
else

	# check for an available logging library (log4cplus, then log4cxx, then log4cpp)

	# log4cplus
	if test "$without_log4cplus" != "true"; then
		# Check if log4cplus location is specified
		if test "x$log4cplus_location" != "xyes"
		then
			CPPFLAGS="$CPPFLAGS -I$log4cplus_location/include"
			LDFLAGS="$LDFLAGS -L$log4cplus_location/lib"
		fi
		# Check for log4cplus library
		LIBS_SAVED="$LIBS"
		LIBS="$LIBS_SAVED -llog4cplus"
		AC_MSG_CHECKING(log4cplus library)
		AC_TRY_LINK([#include <log4cplus/logger.h>],[ log4cplus::Logger::getRoot(); return 0; ],
			[ 
			  # Found the log4cplus library
			  AC_MSG_RESULT(ok) 
			  AC_DEFINE([PION_USE_LOG4CPLUS],[1],[Define to 1 if you have the `log4cplus' library (-llog4cplus).])
			  AC_MSG_NOTICE(Using log4cplus for logging)
			  without_log4cxx=true without_log4cpp=true
			  PION_LOG_LIB="-llog4cplus"
			],
			[ AC_MSG_RESULT(no) ])
		LIBS="$LIBS_SAVED"
	fi

	# log4cxx
	if test "$without_log4cxx" != "true"; then
		# Check if log4cxx location is specified
		if test "x$log4cxx_location" != "xyes"
		then
			CPPFLAGS="$CPPFLAGS -I$log4cxx_location/include"
			LDFLAGS="$LDFLAGS -L$log4cxx_location/lib"
		fi
		# Check for log4cxx library
		LIBS_SAVED="$LIBS"
		LIBS="$LIBS_SAVED -llog4cxx"
		AC_MSG_CHECKING(log4cxx library)
		AC_TRY_LINK([#include <log4cxx/logger.h>],[ log4cxx::Logger::getRootLogger(); return 0; ],
			[ 
			  # Found the log4cxx library
			  AC_MSG_RESULT(ok) 
			  AC_DEFINE([PION_USE_LOG4CXX],[1],[Define to 1 if you have the `log4cxx' library (-llog4cxx).])
			  AC_MSG_NOTICE(Using log4cxx for logging)
			  without_log4cplus=true without_log4cpp=true
			  PION_LOG_LIB="-llog4cxx"
			],
			[ AC_MSG_RESULT(no) ])
		LIBS="$LIBS_SAVED"
	fi
	
	# log4cpp
	if test "$without_log4cpp" != "true"; then
		# Check if log4cpp location is specified
		if test "x$log4cpp_location" != "xyes"
		then
			CPPFLAGS="$CPPFLAGS -I$log4cpp_location/include"
			LDFLAGS="$LDFLAGS -L$log4cpp_location/lib"
		fi
		# Check for log4cpp library
		LIBS_SAVED="$LIBS"
		LIBS="$LIBS_SAVED -llog4cpp"
		AC_MSG_CHECKING(log4cpp library)
		AC_TRY_LINK([#include <log4cpp/Category.hh>],[ log4cpp::Category::getRoot(); return 0; ],
			[ 
			  # Found the log4cpp library
			  AC_MSG_RESULT(ok) 
			  AC_DEFINE([PION_USE_LOG4CPP],[1],[Define to 1 if you have the `log4cpp' library (-llog4cpp).])
			  AC_MSG_NOTICE(Using log4cpp for logging)
			  without_log4cplus=true without_log4cxx=true
			  PION_LOG_LIB="-llog4cpp"
			],
			[ AC_MSG_RESULT(no) ])
		LIBS="$LIBS_SAVED"
	fi

	# no log library found
	if test "x$PION_LOG_LIB" == "x"; then
		if test "x$ostream_logging" == "xno"; then
			AC_MSG_WARN(No logging library found - disabling logging)
			AC_DEFINE([PION_DISABLE_LOGGING],[1],[Define to 1 to disable logging.])
		else
			AC_MSG_WARN(No logging library found - using std::ostream for logging)
			AC_DEFINE([PION_USE_OSTREAM_LOGGING],[1],[Define to 1 to use std::cout and std::cerr for logging.])
		fi
	fi
fi
AC_SUBST(PION_LOG_LIB)


# Set external library dependencies
PION_EXTERNAL_LIBS="$PION_EXTERNAL_LIBS $BOOST_THREAD_LIB $BOOST_SYSTEM_LIB $BOOST_FILESYSTEM_LIB $BOOST_REGEX_LIB $BOOST_DATE_TIME_LIB $BOOST_SIGNALS_LIB $BOOST_IOSTREAMS_LIB $PION_SSL_LIB $PION_ZLIB $PION_BZLIB $PION_LOG_LIB"
AC_SUBST(PION_EXTERNAL_LIBS)
