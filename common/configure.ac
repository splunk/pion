# -----------------------------------------------
# Pion Common Library autoconf configuration file
# -----------------------------------------------

# Set Pion version information
AC_INIT([pion-common], [4.0.12], [support@atomiclabs.com])

# Initialize some other things
AC_PREREQ([2.59])
AC_CONFIG_AUX_DIR(build)
AC_CONFIG_SRCDIR(src/PionPlugin.cpp)

# package and version args are now taken from AC_INIT()
AM_INIT_AUTOMAKE

# Setup libtool
AC_LIBTOOL_WIN32_DLL
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

# Setup doxygen support
DX_DOXYGEN_FEATURE(ON)
DX_HTML_FEATURE(ON)
DX_INIT_DOXYGEN(pion-common, doc/Doxyfile, doc)

# Define CXXFLAGS before AC_PROG_CXX to suppress the default autoconf
# compiler options
CFLAGS="-DPION_COMMON_LIBRARY"
CXXFLAGS="-DPION_COMMON_LIBRARY"

# Currently only the embedded common library can be used
PION_COMMON_HOME=`pwd`
PION_COMMON_LIB="$PION_COMMON_HOME/src/libpion-common.la"
AC_SUBST(PION_COMMON_HOME)
AC_SUBST(PION_COMMON_LIB)

# Include pion-config.inc
m4_include([build/pion-setup.inc])
m4_include([build/pion-boost.inc])
m4_include([build/pion-config.inc])

# Output Makefiles
AC_OUTPUT(pion-common.pc Makefile
	include/Makefile include/pion/Makefile src/Makefile 
	tests/Makefile tests/PluginsUsedByUnitTests/Makefile)
