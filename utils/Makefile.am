# ------------------------------------
# pion-net automake configuration file 
# ------------------------------------

AM_CPPFLAGS = -I@PION_COMMON_HOME@/include -I../include

bin_PROGRAMS = PionServerTest PionServiceTest

PionServerTest_SOURCES = PionServerTest.cpp
PionServerTest_LDADD = ../src/libpion-net.la @PION_COMMON_LIB@ @PION_EXTERNAL_LIBS@
PionServerTest_DEPENDENCIES = ../src/libpion-net.la

PionServiceTest_SOURCES = PionServiceTest.cpp
PionServiceTest_LDADD = ../src/libpion-net.la @PION_COMMON_LIB@ @PION_EXTERNAL_LIBS@
PionServiceTest_DEPENDENCIES = ../src/libpion-net.la

EXTRA_DIST = sslkey.pem testservices.html *.conf *.vcproj
