# ------------------------------------
# pion-net automake configuration file 
# ------------------------------------

AM_CPPFLAGS = -I@PION_COMMON_HOME@/include -I../include

pion_pluginsdir = @PION_PLUGINS_DIRECTORY@
pion_plugins_LTLIBRARIES = HelloService.la EchoService.la \
	CookieService.la LogService.la FileService.la AllowNothingService.la

HelloService_la_CXXFLAGS = -shared $(AM_CXXFLAGS)
HelloService_la_SOURCES = HelloService.hpp HelloService.cpp
HelloService_la_LDFLAGS = -no-undefined -module -avoid-version
HelloService_la_LIBADD = ../src/libpion-net.la @PION_COMMON_LIB@ @PION_EXTERNAL_LIBS@
HelloService_la_DEPENDENCIES = ../src/libpion-net.la

EchoService_la_CXXFLAGS = -shared $(AM_CXXFLAGS)
EchoService_la_SOURCES = EchoService.hpp EchoService.cpp
EchoService_la_LDFLAGS = -no-undefined -module -avoid-version
EchoService_la_LIBADD = ../src/libpion-net.la @PION_COMMON_LIB@ @PION_EXTERNAL_LIBS@
EchoService_la_DEPENDENCIES = ../src/libpion-net.la

CookieService_la_CXXFLAGS = -shared $(AM_CXXFLAGS)
CookieService_la_SOURCES = CookieService.hpp CookieService.cpp
CookieService_la_LDFLAGS = -no-undefined -module -avoid-version
CookieService_la_LIBADD = ../src/libpion-net.la @PION_COMMON_LIB@ @PION_EXTERNAL_LIBS@
CookieService_la_DEPENDENCIES = ../src/libpion-net.la

LogService_la_CXXFLAGS = -shared $(AM_CXXFLAGS)
LogService_la_SOURCES = LogService.hpp LogService.cpp
LogService_la_LDFLAGS = -no-undefined -module -avoid-version
LogService_la_LIBADD = ../src/libpion-net.la @PION_COMMON_LIB@ @PION_EXTERNAL_LIBS@
LogService_la_DEPENDENCIES = ../src/libpion-net.la

FileService_la_CXXFLAGS = -shared $(AM_CXXFLAGS)
FileService_la_SOURCES = FileService.hpp FileService.cpp
FileService_la_LDFLAGS = -no-undefined -module -avoid-version
FileService_la_LIBADD = ../src/libpion-net.la @PION_COMMON_LIB@ @PION_EXTERNAL_LIBS@
FileService_la_DEPENDENCIES = ../src/libpion-net.la

AllowNothingService_la_CXXFLAGS = -shared $(AM_CXXFLAGS)
AllowNothingService_la_SOURCES = AllowNothingService.hpp AllowNothingService.cpp
AllowNothingService_la_LDFLAGS = -no-undefined -module -avoid-version
AllowNothingService_la_LIBADD = ../src/libpion-net.la @PION_COMMON_LIB@ @PION_EXTERNAL_LIBS@
AllowNothingService_la_DEPENDENCIES = ../src/libpion-net.la

EXTRA_DIST = *.vcproj
