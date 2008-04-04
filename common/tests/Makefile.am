# ---------------------------------------
# pion-common automake configuration file 
# ---------------------------------------

SUBDIRS = PluginsUsedByUnitTests

AM_CPPFLAGS = -I../include @PION_TESTS_CPPFLAGS@

check_PROGRAMS = PionCommonUnitTests
TESTS = $(check_PROGRAMS)

PionCommonUnitTests_SOURCES = PionCommonUnitTests.cpp PionCounterTests.cpp PionPluginPtrTests.cpp PluginManagerTests.cpp PionDateTimeTests.cpp
PionCommonUnitTests_LDADD = ../src/libpion-common.la @PION_EXTERNAL_LIBS@ @BOOST_TEST_LIB@
PionCommonUnitTests_DEPENDENCIES = ../src/libpion-common.la \
	PluginsUsedByUnitTests/hasCreateAndDestroy.la PluginsUsedByUnitTests/hasCreateButNoDestroy.la \
	PluginsUsedByUnitTests/hasNoCreate.la

EXTRA_DIST = *.vcproj
