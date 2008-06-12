# -------------------------------------
# with-pion autoconf configuration file
# -------------------------------------

#
# Determine the location of Pion libraries
#

AC_MSG_CHECKING([for Pion libraries])
AC_ARG_WITH([pion],
    AC_HELP_STRING([--with-pion@<:@=DIR@:>@],[location of Pion install or source]),
    [ pion_location=$withval ], [ pion_location="/usr/local" ] )

# Determine which directory to look in
if test "x$pion_location" == "xyes"; then
	pion_location="/usr/local"
elif test "x$pion_location" == "x"; then
	pion_location="/usr/local"
fi

# Check if the directory is a source code distribution (assumes that it is pre-built)
if test -r "$pion_location/pion-platform.pc"; then
	if test -r "$pion_location/common"; then
		if test -r "$pion_location/net"; then
			if test -r "$pion_location/platform"; then
				# Found source repository
				AC_MSG_RESULT(found source in $pion_location)
				found_pion_source="true"
				PION_CPPFLAGS="-I$pion_location/common/include \
					-I$pion_location/net/include -I$pion_location/platform/include"
				PION_LDFLAGS="-L$pion_location/common/src/.libs \
					-L$pion_location/net/src/.libs -L$pion_location/platform/src/.libs"
			fi
		fi
	fi
fi

# If not found yet, check if directory is Pion installation directory
if test "x$found_pion_source" == "x"; then
	# Not found yet.. keep looking
	if test -r "$pion_location/lib/pkgconfig/pion-platform.pc"; then
		if test -r "$pion_location/lib/libpion-common.dylib"; then
			if test -r "$pion_location/lib/libpion-net.dylib"; then
				if test -r "$pion_location/lib/libpion-platform.dylib"; then
					if test -r "$pion_location/include/pion"; then
						# Found installation directory
						AC_MSG_RESULT(found install in $pion_location)
						found_pion_install="true"
					fi
				fi
			fi
		fi
	fi
	
	# Error if still not found
	if test "x$found_pion_install" == "x"; then
		AC_MSG_RESULT(no)
		AC_MSG_ERROR([Unable to find the Pion libraries in $pion_location])
	fi
fi

# Setup compiler flags for Pion libraries
PION_CPPFLAGS="$PION_CPPFLAGS `PKG_CONFIG_PATH=$pion_location pkg-config --cflags pion-platform`"
PION_LDFLAGS="$PION_LDFLAGS `PKG_CONFIG_PATH=$pion_location pkg-config --libs pion-platform`"
PION_LIBS="-lpion-common -lpion-net -lpion-platform"

# Prepare flags for link tests
CPPFLAGS_SAVED="$CPPFLAGS"
LDFLAGS_SAVED="$LDFLAGS"
LIBS_SAVED="$LIBS"
CPPFLAGS="$CPPFLAGS $PION_CPPFLAGS"
LDFLAGS="$LDFLAGS $PION_LDFLAGS"
LIBS="$LIBS $PION_LIBS"

# Try linking with the pion-common library
AC_MSG_CHECKING(linking with pion-common library)
AC_TRY_LINK([#include <pion/PionScheduler.hpp>],[ pion::PionSingleServiceScheduler pion_sched; return(0); ],
	[ AC_MSG_RESULT(ok) ],
	[ AC_MSG_RESULT(failed)
	  AC_MSG_ERROR(Unable to link with the pion-common library)
	])

# Try linking with the pion-net library
AC_MSG_CHECKING(linking with pion-net library)
AC_TRY_LINK([#include <pion/net/HTTPTypes.hpp>],[ std::string hello("hello+world"); pion::net::HTTPTypes::url_decode(hello); return(0); ],
	[ AC_MSG_RESULT(ok) ],
	[ AC_MSG_RESULT(failed)
	  AC_MSG_ERROR(Unable to link with the pion-net library)
	])

# Try linking with the pion-platform library
AC_MSG_CHECKING(linking with pion-platform library)
AC_TRY_LINK([#include <pion/platform/Vocabulary.hpp>],[ pion::platform::Vocabulary v; return(0); ],
	[ AC_MSG_RESULT(ok) ],
	[ AC_MSG_RESULT(failed)
	  AC_MSG_ERROR(Unable to link with the pion-platform library)
	])

# Revert flags following link tests
CPPFLAGS="$CPPFLAGS_SAVED"
LDFLAGS="$LDFLAGS_SAVED"
LIBS="$LIBS_SAVED"
AC_SUBST(PION_CPPFLAGS)
AC_SUBST(PION_LDFLAGS)
AC_SUBST(PION_LIBS)
