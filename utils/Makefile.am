# ------------------------------------
# pion-net automake configuration file 
# ------------------------------------

AM_CPPFLAGS = -I@PION_COMMON_HOME@/include -I../include

bin_PROGRAMS = PionServerTest PionServiceTest

PionServerTest_SOURCES = PionServerTest.cpp
PionServerTest_LDADD = ../src/libpion-net.la @PION_COMMON_LIB@ @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @PION_SSL_LIB@ @PION_LOG_LIB@
PionServerTest_DEPENDENCIES = ../src/libpion-net.la

PionServiceTest_SOURCES = PionServiceTest.cpp
PionServiceTest_LDADD = ../src/libpion-net.la @PION_COMMON_LIB@ @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @PION_SSL_LIB@ @PION_LOG_LIB@
PionServiceTest_DEPENDENCIES = ../src/libpion-net.la

EXTRA_DIST = sslkey.pem testservices.html *.conf *.vcproj
