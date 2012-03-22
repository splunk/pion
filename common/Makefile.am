# ---------------------------------------
# pion-common automake configuration file 
# ---------------------------------------

ACLOCAL_AMFLAGS = -I build

SUBDIRS = include src @PION_TESTS_MAKEDIRS@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pion-common.pc

EXTRA_DIST = autogen.sh *.sln *.xcodeproj doc build/*.m4 build/*.vsprops build/*.pl build/*.sh build/*.bat  build/*.msm build/*.dll build/*.ico

include build/doxygen.inc
include build/common.inc
