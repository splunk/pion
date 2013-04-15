# ------------------------------------------------
# Pion Network Library autoconf configuration file
# ------------------------------------------------

# Set pion version information
AC_INIT([pion], [5.0.1], [pion-users@lists.sourceforge.net])

# Initialize some other things
AC_PREREQ([2.59])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([m4])
AC_CONFIG_SRCDIR(src/algorithm.cpp)

# package and version args are now taken from AC_INIT()
AM_INIT_AUTOMAKE([foreign])

# Use "silent mode" for less verbose build commands in automake 1.11+
m4_ifdef( [AM_SILENT_RULES], [AM_SILENT_RULES([yes])] )

# Needed to set per-target compilation flags
AM_PROG_CC_C_O

# Setup libtool
AC_LIBTOOL_WIN32_DLL
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

# Setup doxygen support
DX_DOXYGEN_FEATURE(ON)
DX_HTML_FEATURE(ON)
DX_INIT_DOXYGEN(pion, doc/Doxyfile, doc)

# Define CXXFLAGS before AC_PROG_CXX to suppress the default autoconf
# compiler options
CFLAGS="$CFLAGS -DPION"
CXXFLAGS="$CXXFLAGS -DPION"

# Include pion-config.inc
m4_include([build/pion-setup.inc])
m4_include([build/pion-boost.inc])
m4_include([build/pion-config.inc])

# Output Makefiles
AC_OUTPUT(pion.pc Makefile
	include/Makefile include/pion/Makefile include/pion/tcp/Makefile
	include/pion/http/Makefile include/pion/test/Makefile
	include/pion/spdy/Makefile src/Makefile
	services/Makefile utils/Makefile tests/Makefile tests/plugins/Makefile)
