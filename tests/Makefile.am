# ------------------------------------
# pion-net automake configuration file 
# ------------------------------------

SUBDIRS = PluginsUsedByUnitTests

AM_CPPFLAGS = -I../include @PION_TESTS_CPPFLAGS@

check_PROGRAMS = PionNetUnitTests
TESTS = $(check_PROGRAMS)

PionNetUnitTests_SOURCES = PionNetUnitTests.cpp \
	PionPluginPtrTests.cpp PluginManagerTests.cpp PionAlgorithmsTests.cpp \
	HTTPTypesTests.cpp \
	HTTPMessageTests.cpp HTTPRequestTests.cpp HTTPResponseTests.cpp \
	TCPStreamTests.cpp TCPServerTests.cpp WebServerTests.cpp \
	FileServiceTests.cpp HTTPParserTests.cpp
PionNetUnitTests_LDADD = ../src/libpion-net.la @PION_EXTERNAL_LIBS@ @BOOST_TEST_LIB@
PionNetUnitTests_DEPENDENCIES = ../src/libpion-net.la \
	PluginsUsedByUnitTests/hasCreateAndDestroy.la PluginsUsedByUnitTests/hasCreateButNoDestroy.la \
	PluginsUsedByUnitTests/hasNoCreate.la

EXTRA_DIST = *.vcproj HTTPParserTestsData.inc config doc
