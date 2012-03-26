# ------------------------------------
# pion-net automake configuration file 
# ------------------------------------

ACLOCAL_AMFLAGS = -I @PION_COMMON_HOME@/build

SUBDIRS = @PION_COMMON_MAKEDIRS@ include src services utils @PION_TESTS_MAKEDIRS@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pion-net.pc

EXTRA_DIST = autogen.sh *.sln *.xcodeproj doc build/*.vsprops *.vcproj

include $(top_srcdir)/common/build/doxygen.inc
include build/net.inc
