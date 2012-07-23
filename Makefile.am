# --------------------------------
# pion automake configuration file 
# --------------------------------

ACLOCAL_AMFLAGS = -I m4 -I build

SUBDIRS = include src services utils @PION_TESTS_MAKEDIRS@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pion.pc

EXTRA_DIST = autogen.sh *.sln *.xcodeproj doc build/*.vsprops *.vcproj

include $(top_srcdir)/build/doxygen.inc
include build/net.inc
