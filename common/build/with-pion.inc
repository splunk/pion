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
if (test "x$pion_location" == "xyes") || (test "x$pion_location" == "x"); then
	pion_location=""
	if test `pwd | grep 'pion-.*-\(.*\)'`; then
		pion_location=`pwd | sed 's,/$,,' | sed 's;pion-.*-\(.*\);pion-platform-\1;'`;
		if ! test -r "$pion_location"; then
			pion_location=""
		fi
	fi
	if test "x$pion_location" == "x"; then
		pion_location="`pwd | sed 's,/$,,' | sed 's,\(.*/\).*,\1pion-platform,'`"
	fi
fi
PION_PLATFORM_PATH="$pion_location"

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
					-I$pion_location/platform/server -I$pion_location/sqlite"
				PION_LIBS="$pion_location/common/src/libpion-common.la \
					$pion_location/net/src/libpion-net.la \
					$pion_location/platform/src/libpion-platform.la \
					$pion_location/sqlite/libpion-sqlite.la"
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
							$pion_location/lib/libpion-platform.la \
							$pion_location/lib/libpion-sqlite.la"
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
PION_LDFLAGS="$PION_LDFLAGS `PKG_CONFIG_PATH=$pion_location pkg-config --libs-only-L pion-platform`"
PION_LDLIBS="$PION_LDLIBS `PKG_CONFIG_PATH=$pion_location pkg-config --libs-only-l pion-platform`"

# Find static versions of Pion library dependencies
PION_STATIC_LIBS=""
for param in $PION_LDLIBS
do
	if [[[ "$param" =~ ^-l ]]]; then
		lib=`echo $param | sed 's;-l;lib;'`
		if test -r "/usr/local/lib/$lib.a"; then
			PION_STATIC_LIBS="$PION_STATIC_LIBS /usr/local/lib/$lib.a"
		elif test -r "/usr/lib/$lib.a"; then
			PION_STATIC_LIBS="$PION_STATIC_LIBS /usr/lib/$lib.a"
		else
			PION_STATIC_LIBS="$PION_STATIC_LIBS $param"
			AC_MSG_WARN([Unable to find static archive for $param])
		fi
	else
		PION_STATIC_LIBS="$PION_STATIC_LIBS $param"
	fi
done
# Append direct references to the Pion library archives
for lib in $PION_LIBS
do
	pionlib=`echo $lib | sed 's;\([[^ ]]*\)/\([[^/]]*\).la;\1/.libs/\2.a;'`
	PION_STATIC_LIBS="$PION_STATIC_LIBS $pionlib"
done

# Substitute variables in Makefiles
AC_SUBST(PION_CPPFLAGS)
AC_SUBST(PION_LDFLAGS)
AC_SUBST(PION_LDLIBS)
AC_SUBST(PION_STATIC_LIBS)
AC_SUBST(PION_LIBS)
AC_SUBST(PION_SERVER_LIB)
AC_SUBST(PION_PLATFORM_PATH)

