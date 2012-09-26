# --------------------------------
# pion automake configuration file
# --------------------------------

AM_CPPFLAGS = -I../include

lib_LTLIBRARIES = libpion.la

libpion_la_SOURCES = \
	admin_rights.cpp algorithm.cpp logger.cpp plugin.cpp process.cpp scheduler.cpp \
	spdy_decompressor.cpp spdy_parser.cpp \
	tcp_server.cpp tcp_timer.cpp \
	http_auth.cpp http_basic_auth.cpp http_cookie_auth.cpp http_message.cpp \
	http_parser.cpp http_plugin_server.cpp http_reader.cpp http_server.cpp \
	http_types.cpp http_writer.cpp

libpion_la_LDFLAGS = -no-undefined -release $(PION_LIBRARY_VERSION)
libpion_la_LIBADD = @PION_EXTERNAL_LIBS@

EXTRA_DIST = *.vcproj
