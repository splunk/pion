# --------------------------------------
# pion-setup autoconf configuration file
# --------------------------------------

# DO NOT USE autoheader (the .hpp.in file is not automanaged)
AUTOHEADER="echo autoheader ignored"

# Check for programs
AC_PROG_CXX
AC_PROG_INSTALL

# Use C++ language
AC_LANG_CPLUSPLUS
AX_COMPILER_VENDOR

# library to link with for dlopen, etc.
PION_DLOPEN_LIBS=$lt_cv_dlopen_libs
AC_SUBST(PION_DLOPEN_LIBS)

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
		PION_OPT_FLAGS="-O2 -ggdb -Wall -Wno-strict-aliasing -DNDEBUG"
		PION_DEBUG_FLAGS="-O0 -ggdb -Wall -Wno-strict-aliasing -fkeep-inline-functions"
	fi
	PION_EXTERNAL_LIBS="-lsocket -lnsl -lposix4"
	;;
*darwin*)
	# Mac OS X: GCC compiler
	# -pthread is implied (automatically set by compiler)
	CPPFLAGS="$CPPFLAGS -D_REENTRANT"
	#LDFLAGS="$LDFLAGS"
	#PION_OPT_FLAGS="-O3 -ggdb -fomit-frame-pointer -fstrict-aliasing -fno-tree-pre -falign-loops -Wall -Wno-strict-aliasing -DNDEBUG"
	PION_OPT_FLAGS="-O3 -ggdb -fomit-frame-pointer -fstrict-aliasing -Wall -Wno-strict-aliasing -DNDEBUG"
	PION_DEBUG_FLAGS="-O0 -ggdb -Wall -Wno-strict-aliasing -fkeep-inline-functions"

	# Check for OSX binary architecture
	AC_MSG_CHECKING([for OSX binary architectures])
	AC_ARG_WITH([arch],
	    AC_HELP_STRING([--with-arch@<:@=LIST@:>@],[specify comma-separated list of OSX architectures]),
    	[with_arch=$withval],
	    [with_arch=default])
	if test "$with_arch" = "no"; then
		AC_MSG_RESULT(no)
	elif test "$with_arch" = "default"; then
		AC_MSG_RESULT(no)
# Note: Pion seems to crashe consistently when built with x86_64
#		AC_MSG_RESULT(x86_64)
#		with_arch="-arch x86_64"
#		PION_OPT_FLAGS="$PION_OPT_FLAGS $with_arch"
#		PION_DEBUG_FLAGS="$PION_DEBUG_FLAGS $with_arch"
	else
		AC_MSG_RESULT($with_arch)
		with_arch="-arch `echo $with_arch | sed 's#,# -arch #g'`"
		PION_OPT_FLAGS="$PION_OPT_FLAGS $with_arch"
		PION_DEBUG_FLAGS="$PION_DEBUG_FLAGS $with_arch"
	fi

	;;
*cygwin*)
	# Cygwin GCC compiler (Windows)
	CPPFLAGS="$CPPFLAGS -mthreads -D_REENTRANT -DPION_WIN32 -D__USE_W32_SOCKETS"
	LDFLAGS="$LDFLAGS -mthreads -Wl,-E"
	PION_OPT_FLAGS="-O2 -ggdb -Wall -Wno-strict-aliasing -DNDEBUG"
	PION_DEBUG_FLAGS="-O0 -ggdb -Wall -Wno-strict-aliasing -fkeep-inline-functions"
	PION_EXTERNAL_LIBS="-lws2_32 -lmswsock"
	;;
*freebsd*)
	# FreeBSD: GCC compiler
	CPPFLAGS="$CPPFLAGS -pthread -D_REENTRANT"
	LDFLAGS="$LDFLAGS -pthread -Wl,-E"
	PION_OPT_FLAGS="-O2 -ggdb -Wall -Wno-strict-aliasing -DNDEBUG"
	PION_DEBUG_FLAGS="-O0 -ggdb -Wall -Wno-strict-aliasing -fkeep-inline-functions"
	;;
*)
	# Other (Linux): GCC compiler
	CPPFLAGS="$CPPFLAGS -pthread -D_REENTRANT"
	LDFLAGS="$LDFLAGS -pthread -lrt -ldl -Wl,-E"
	PION_OPT_FLAGS="-O2 -ggdb -Wall -Wno-strict-aliasing -DNDEBUG"
	PION_DEBUG_FLAGS="-O0 -ggdb -Wall -Wno-strict-aliasing -fkeep-inline-functions"
	;;
esac


# Check for --with-cpu (gcc CPU architecture)
AC_MSG_CHECKING([for specific CPU architecture])
AC_ARG_WITH([cpu],
	AC_HELP_STRING([--with-cpu@<:@=MARCH@:>@],[specify specific CPU architecture for gcc]),
	[with_cpu=$withval],
	[with_cpu=default])
if test "$with_cpu" = "no"; then
	AC_MSG_RESULT(no)
else
	if test "$with_cpu" = "default"; then
		if test "$with_arch" = "no" || test "$with_arch" = "default" ; then
		if test "$ax_cv_cxx_compiler_vendor" = "gnu"; then
			# check __x86_64__ to determine default
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[
			#if defined(__x86_64__)
				// x86_64 defined -> leave undefined
			#else
				#error __x86_64__ not defined -> i686
			#endif
			]])], [], [with_cpu="i686"])
		fi
		fi
	fi
	if test "$with_cpu" = "default"; then
		AC_MSG_RESULT(no)
	else
		# check to make sure it works
		CFLAGS_SAVED=$CFLAGS
		CXXFLAGS_SAVED=$CXXFLAGS
		CFLAGS="$CFLAGS -march=$with_cpu"
		CXXFLAGS="$CXXFLAGS -march=$with_cpu"
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])], [
			AC_MSG_RESULT($with_cpu)
			PION_OPT_FLAGS="$PION_OPT_FLAGS -march=$with_cpu"
			PION_DEBUG_FLAGS="$PION_DEBUG_FLAGS -march=$with_cpu"
			], [
			AC_MSG_RESULT(no)
			])
		CFLAGS=$CFLAGS_SAVED
		CXXFLAGS=$CXXFLAGS_SAVED
	fi
fi


# Check for debug
AC_MSG_CHECKING([for debugging])
AC_ARG_WITH([debug],
    AC_HELP_STRING([--with-debug],[build with debugging information]),
    [with_debug=$withval],
    [with_debug=no])
if test "$with_debug" = "no"; then
	AC_MSG_RESULT(no)
	CFLAGS="$CFLAGS $PION_OPT_FLAGS"
	CXXFLAGS="$CXXFLAGS $PION_OPT_FLAGS"
else
	AC_MSG_RESULT(yes)
	CFLAGS="$CFLAGS $PION_DEBUG_FLAGS"
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
		PION_PLUGINS_DIRECTORY=/usr/local/share/pion/plugins
	else
		PION_PLUGINS_DIRECTORY=${prefix}/share/pion/plugins
	fi
else
	PION_PLUGINS_DIRECTORY=$with_plugins
fi
AC_MSG_RESULT($PION_PLUGINS_DIRECTORY)
AC_DEFINE_UNQUOTED([PION_PLUGINS_DIRECTORY],["$PION_PLUGINS_DIRECTORY"],[Define to the directory where Pion Plug-ins are installed.])
AC_SUBST(PION_PLUGINS_DIRECTORY)
