# ------------------------------------
# pion-net automake configuration file 
# ------------------------------------

AM_CPPFLAGS = -I@PION_COMMON_HOME@/include -I../include

bin_PROGRAMS = PionHelloServer PionWebServer

PionHelloServer_SOURCES = PionHelloServer.cpp
PionHelloServer_LDADD = ../src/libpion-net.la @PION_COMMON_LIB@ @PION_EXTERNAL_LIBS@
PionHelloServer_DEPENDENCIES = ../src/libpion-net.la

PionWebServer_SOURCES = PionWebServer.cpp
PionWebServer_LDADD = ../src/libpion-net.la @PION_COMMON_LIB@ @PION_EXTERNAL_LIBS@
PionWebServer_DEPENDENCIES = ../src/libpion-net.la

EXTRA_DIST = sslkey.pem testservices.html *.conf *.vcproj
