# ---------------------------------------
# pion-common automake configuration file 
# ---------------------------------------

AM_CPPFLAGS = -I../include

bin_PROGRAMS = PionPluginPtrTests

PionPluginPtrTests_SOURCES = PionPluginPtrTests.cpp
PionPluginPtrTests_LDADD = ../src/libpion-common.la @PION_EXTERNAL_LIBS@ @BOOST_TEST_LIB@
PionPluginPtrTests_DEPENDENCIES = ../src/libpion-common.la

EXTRA_DIST = *.vcproj
