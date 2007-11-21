# -----------------------------------------------
# Pion Common Library autoconf configuration file
# -----------------------------------------------

# Check for pion-common library
AC_ARG_WITH([common],
    AC_HELP_STRING([--with-common@<:@=DIR@:>@],[location of pion-common library]),
    [ pion_common_location=$withval ], [ use_included_pion_common=true ])

# Check if common location is specified
if test "$use_included_pion_common" == "true"; then
	PION_COMMON_HOME=`pwd`/common
	PION_COMMON_MAKEDIRS=common
	AC_SUBST(PION_COMMON_MAKEDIRS)
	AC_MSG_NOTICE([Using the embedded pion-common library])
else
	PION_COMMON_HOME="$pion_common_location"
	for pion_common_test in $pion_common_location/lib $pion_common_location/src
	do
		if test -f "$pion_common_test/libpion-common.la"; then
			PION_COMMON_LIB="$pion_common_test/libpion-common.la"
			break
		else
			PION_COMMON_LIB=""
		fi
	done
	if test "x$PION_COMMON_LIB" = "x"; then
		AC_MSG_ERROR(Unable to find the pion-common library in $pion_common_location)
	fi
	AC_MSG_NOTICE(Using the pion-common library in $pion_common_location)
fi

PION_COMMON_LIB="$PION_COMMON_HOME/src/libpion-common.la"
AC_SUBST(PION_COMMON_LIB)
AC_SUBST(PION_COMMON_HOME)
