# -----------------------------------
# libpion automake configuration file 
# -----------------------------------

AM_CPPFLAGS = -I../include

libpion_modulesdir = @PION_MODULES_DIRECTORY@
libpion_modules_LTLIBRARIES = HelloModule.la EchoModule.la \
	CookieModule.la LogModule.la FileModule.la

HelloModule_la_SOURCES = HelloModule.hpp HelloModule.cpp
HelloModule_la_LDFLAGS = -no-undefined -module -avoid-version
HelloModule_la_LIBADD = ../src/libpion.la @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ @PION_SSL_LIB@ @PION_LOG_LIB@
HelloModule_la_DEPENDENCIES = ../src/libpion.la

EchoModule_la_SOURCES = EchoModule.hpp EchoModule.cpp
EchoModule_la_LDFLAGS = -no-undefined -module -avoid-version
EchoModule_la_LIBADD = ../src/libpion.la @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ @PION_SSL_LIB@ @PION_LOG_LIB@
EchoModule_la_DEPENDENCIES = ../src/libpion.la

CookieModule_la_SOURCES = CookieModule.hpp CookieModule.cpp
CookieModule_la_LDFLAGS = -no-undefined -module -avoid-version
CookieModule_la_LIBADD = ../src/libpion.la @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ @PION_SSL_LIB@ @PION_LOG_LIB@
CookieModule_la_DEPENDENCIES = ../src/libpion.la

LogModule_la_SOURCES = LogModule.hpp LogModule.cpp
LogModule_la_LDFLAGS = -no-undefined -module -avoid-version
LogModule_la_LIBADD = ../src/libpion.la @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ @PION_SSL_LIB@ @PION_LOG_LIB@
LogModule_la_DEPENDENCIES = ../src/libpion.la

FileModule_la_SOURCES = FileModule.hpp FileModule.cpp
FileModule_la_LDFLAGS = -no-undefined -module -avoid-version
FileModule_la_LIBADD = ../src/libpion.la @BOOST_THREAD_LIB@ @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @PION_SSL_LIB@ @PION_LOG_LIB@
FileModule_la_DEPENDENCIES = ../src/libpion.la
