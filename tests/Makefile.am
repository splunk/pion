# -----------------------------------
# libpion automake configuration file 
# -----------------------------------

AM_CPPFLAGS = -I../include

bin_PROGRAMS = PionServerTest PionModuleTest

PionServerTest_SOURCES = PionServerTest.cpp
PionServerTest_LDADD = ../src/libpion.la @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @PION_SSL_LIB@ @PION_LOG_LIB@
PionServerTest_DEPENDENCIES = ../src/libpion.la

PionModuleTest_SOURCES = PionModuleTest.cpp
PionModuleTest_LDADD = ../src/libpion.la @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @PION_SSL_LIB@ @PION_LOG_LIB@
PionModuleTest_DEPENDENCIES = ../src/libpion.la

EXTRA_DIST = nomodules.conf testmodules.conf xcodemodules.conf \
	testmodules.html vcmodules.conf sslkey.pem
