# ---------------------------------------
# pion-common automake configuration file 
# ---------------------------------------

AM_CPPFLAGS = -I../include

lib_LTLIBRARIES = libpion-common.la

libpion_common_la_SOURCES = PionPlugin.cpp 

libpion_common_la_LDFLAGS = -no-undefined -release $(PION_LIBRARY_VERSION)
libpion_common_la_LIBADD = @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @PION_SSL_LIB@ @PION_LOG_LIB@
