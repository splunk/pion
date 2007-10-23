# ---------------------------------------
# pion-common automake configuration file 
# ---------------------------------------

SUBDIRS = PluginsUsedByUnitTests

AM_CPPFLAGS = -I../include

check_PROGRAMS = PionPluginPtrTests
TESTS = $(check_PROGRAMS)

PionPluginPtrTests_SOURCES = PionPluginPtrTests.cpp
PionPluginPtrTests_LDADD = ../src/libpion-common.la @PION_EXTERNAL_LIBS@ @BOOST_TEST_LIB@
PionPluginPtrTests_DEPENDENCIES = ../src/libpion-common.la \
	PluginsUsedByUnitTests/hasCreateAndDestroy.la PluginsUsedByUnitTests/hasCreateButNoDestroy.la \
	PluginsUsedByUnitTests/hasNoCreate.la

EXTRA_DIST = *.vcproj
