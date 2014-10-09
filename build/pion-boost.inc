# --------------------------------
# Pion autoconf configuration file
# --------------------------------

# Check for Boost
AX_BOOST_BASE([1.35])
# AC_MSG_NOTICE(Boost home directory: $BOOST_HOME_DIR)
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"

# Check for Boost library extension
AC_MSG_CHECKING([boost library extension])
AC_ARG_WITH([boost-extension],
    AC_HELP_STRING([--with-boost-extension@<:@=EXT@:>@],[extension used for boost library files]),
    [ boost_ext=$withval ], [ detect_boost_ext=true ])
# Check if openssl location is specified
if test "$detect_boost_ext" != "true"; then
    # use provided extension for boost library files
    BOOST_LIB_EXTENSION="$boost_ext"
else
    # Attempt to Autodetect Boost library extension by looking for thread library (i.e. "-gcc41-mt")
    for boost_thread_library in `ls $BOOST_HOME_DIR/lib*/libboost_thread*-mt*{dylib,dll,so,a}* 2>/dev/null` ;
    do
        if test -r $boost_thread_library ; then
            BOOST_LIB_EXTENSION=`echo $boost_thread_library | sed 's,.*/,,' | sed -e 's;^libboost_thread\(.*\)\.dylib.*$;\1;' -e 's;^libboost_thread\(.*\)\.dll*$;\1;' -e 's;^libboost_thread\(.*\)\.so.*$;\1;' -e 's;^libboost_thread\(.*\)\.a*$;\1;'`
            break
        fi
    done
fi
if test "x$BOOST_LIB_EXTENSION" = "x"; then
	AC_MSG_RESULT("(none)")
else
	AC_MSG_RESULT($BOOST_LIB_EXTENSION)
fi
AC_SUBST(BOOST_LIB_EXTENSION)


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


# Check for Boost Thread library
BOOST_TRY_LIB=thread
BOOST_TRY_LINK="boost_${BOOST_TRY_LIB}${BOOST_LIB_EXTENSION}"
LIBS_SAVED="$LIBS"
LIBS="$LIBS_SAVED -l${BOOST_TRY_LINK} ${BOOST_SYSTEM_LIB} ${BOOST_DATE_TIME_LIB}"
AC_MSG_CHECKING([for boost::${BOOST_TRY_LIB} library])
AC_TRY_LINK([#include <boost/thread/thread.hpp>
	#include <boost/thread/condition.hpp>],
	[ boost::thread current_thread; return 0; ],
	[ AC_MSG_RESULT(ok)
	  BOOST_THREAD_LIB="-l${BOOST_TRY_LINK}"
	],
	[ AC_MSG_RESULT(not found)
	  AC_MSG_ERROR(Unable to link with the boost::${BOOST_TRY_LIB} library)
	])
LIBS="$LIBS_SAVED"
AC_SUBST(BOOST_THREAD_LIB)


# Check for Boost Filesystem library
BOOST_TRY_LIB=filesystem
BOOST_TRY_LINK="boost_${BOOST_TRY_LIB}${BOOST_LIB_EXTENSION}"
LIBS_SAVED="$LIBS"
LIBS="$LIBS_SAVED -l${BOOST_TRY_LINK} $BOOST_SYSTEM_LIB"
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


# Check for Boost Unit Test Framework
AC_ARG_ENABLE([tests],
    AC_HELP_STRING([--disable-tests],[do not build and run the unit tests]),
    [enable_tests=$enableval], [enable_tests=yes])
if test "x$enable_tests" == "xno"; then
	# Display notice if unit tests are disabled
	AC_MSG_NOTICE([Unit tests are disabled])
else
	# check if static version of lib is available
	BOOST_TRY_LIB=unit_test_framework
	BOOST_TRY_LINK="boost_${BOOST_TRY_LIB}${BOOST_LIB_EXTENSION}"
	PION_TESTS_CPPFLAGS="-DBOOST_TEST_DYN_LINK"
	LIBS_SAVED="$LIBS"
	LIBS="$LIBS_SAVED -l${BOOST_TRY_LINK}"
	CPPFLAGS_SAVED="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS ${PION_TESTS_CPPFLAGS}"
	AC_MSG_CHECKING([for boost::test library])
	AC_TRY_LINK([#include <boost/test/unit_test.hpp>
		using namespace boost::unit_test;
		test_suite* init_unit_test_suite( int, char** )
		{ return BOOST_TEST_SUITE("Master test suite"); }],
		[],
		[ AC_MSG_RESULT(ok)
		  BOOST_TEST_LIB="-l${BOOST_TRY_LINK}"
		],
		[ AC_MSG_RESULT(not found)
		  AC_MSG_ERROR(Unable to link with the boost::${BOOST_TRY_LIB} library)
		])
	LIBS="$LIBS_SAVED"
	CPPFLAGS="$CPPFLAGS_SAVED"
	PION_TESTS_MAKEDIRS="tests"
fi
AC_SUBST(BOOST_TEST_LIB)
AC_SUBST(PION_TESTS_MAKEDIRS)
AC_SUBST(PION_TESTS_CPPFLAGS)
