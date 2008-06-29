# ------------------------------------
# pion-net automake configuration file 
# ------------------------------------

AM_CPPFLAGS = -I@PION_COMMON_HOME@/include -I../include

lib_LTLIBRARIES = libpion-net.la

libpion_net_la_SOURCES = TCPServer.cpp HTTPTypes.cpp HTTPMessage.cpp \
	HTTPParser.cpp HTTPMessageParser.cpp HTTPReader.cpp HTTPWriter.cpp \
	HTTPServer.cpp HTTPAuth.cpp HTTPBasicAuth.cpp HTTPCookieAuth.cpp \
	WebServer.cpp

libpion_net_la_LDFLAGS = -no-undefined -release $(PION_LIBRARY_VERSION)
libpion_net_la_LIBADD = @PION_COMMON_LIB@ @PION_EXTERNAL_LIBS@
libpion_net_la_DEPENDENCIES = @PION_COMMON_LIB@

EXTRA_DIST = *.vcproj
