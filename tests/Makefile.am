# ------------------------------------
# pion-net automake configuration file 
# ------------------------------------

AM_CPPFLAGS = -I@PION_COMMON_HOME@/include -I../include

check_PROGRAMS = PionNetUnitTests
TESTS = $(check_PROGRAMS)

PionNetUnitTests_SOURCES = PionNetUnitTests.cpp HTTPRequestTests.cpp TCPServerTests.cpp WebServerTests.cpp FileServiceTests.cpp
PionNetUnitTests_LDADD = ../src/libpion-net.la @PION_COMMON_LIB@ @PION_EXTERNAL_LIBS@ @BOOST_TEST_LIB@
PionNetUnitTests_DEPENDENCIES = ../src/libpion-net.la

EXTRA_DIST = *.vcproj
