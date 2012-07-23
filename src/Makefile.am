# ------------------------------------
# pion-net automake configuration file 
# ------------------------------------

AM_CPPFLAGS = -I../include

lib_LTLIBRARIES = libpion-net.la

libpion_net_la_SOURCES = \
	PionLogger.cpp PionPlugin.cpp PionScheduler.cpp PionAdminRights.cpp \
	PionProcess.cpp PionAlgorithms.cpp \
	TCPServer.cpp HTTPTypes.cpp HTTPMessage.cpp TCPTimer.cpp \
	HTTPParser.cpp HTTPReader.cpp HTTPWriter.cpp HTTPServer.cpp \
	HTTPAuth.cpp HTTPBasicAuth.cpp HTTPCookieAuth.cpp WebServer.cpp

libpion_net_la_LDFLAGS = -no-undefined -release $(PION_LIBRARY_VERSION)
libpion_net_la_LIBADD = @PION_EXTERNAL_LIBS@

EXTRA_DIST = *.vcproj
