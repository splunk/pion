# -------------------------------------
# with-pion autoconf configuration file
# -------------------------------------

#
# Determine the location of Pion libraries
#

AC_MSG_CHECKING([for Pion libraries])
AC_ARG_WITH([pion],
    AC_HELP_STRING([--with-pion@<:@=DIR@:>@],[location of Pion install or source]),
    [ pion_location=$withval ], [ pion_location="" ] )

# Determine which directory to look in
if test "x$pion_location" == "xyes"; then
	pion_location="`pwd`/../pion-platform"
elif test "x$pion_location" == "x"; then
	pion_location="`pwd`/../pion-platform"
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
					-I$pion_location/net/include -I$pion_location/platform/include \
					-I$pion_location/platform/server"
				PION_LIBS="$pion_location/common/src/libpion-common.la \
					$pion_location/net/src/libpion-net.la \
					$pion_location/platform/src/libpion-platform.la"
				PION_SERVER_LIB="$pion_location/platform/server/libpion-server.la"
			fi
		fi
	fi
fi

# If not found yet, check if directory is Pion installation directory
if test "x$found_pion_source" == "x"; then
	# Not found yet.. keep looking
	if test -r "$pion_location/lib/pkgconfig/pion-platform.pc"; then
		if test -r "$pion_location/lib/libpion-common.la"; then
			if test -r "$pion_location/lib/libpion-net.la"; then
				if test -r "$pion_location/lib/libpion-platform.la"; then
					if test -r "$pion_location/include/pion"; then
						# Found installation directory
						AC_MSG_RESULT(found install in $pion_location)
						found_pion_install="true"
						PION_LIBS="$pion_location/lib/libpion-common.la \
							$pion_location/lib/libpion-net.la \
							$pion_location/lib/libpion-platform.la"
						PION_SERVER_LIB="$pion_location/lib/libpion-server.la"
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
PION_LDFLAGS="`PKG_CONFIG_PATH=$pion_location pkg-config --libs pion-platform`"

# Substitute variables in Makefiles
AC_SUBST(PION_CPPFLAGS)
AC_SUBST(PION_LDFLAGS)
AC_SUBST(PION_LIBS)
AC_SUBST(PION_SERVER_LIB)
