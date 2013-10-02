# --------------------------------
# pion automake configuration file
# --------------------------------

SUBDIRS = plugins

AM_CPPFLAGS = -I../include @PION_TESTS_CPPFLAGS@

check_PROGRAMS = piontests
TESTS = $(check_PROGRAMS)

piontests_SOURCES = piontests.cpp \
	algorithm_tests.cpp file_service_tests.cpp http_message_tests.cpp \
	http_parser_tests.cpp http_plugin_server_tests.cpp http_request_tests.cpp \
	http_response_tests.cpp http_types_tests.cpp plugin_manager_tests.cpp \
	plugin_tests.cpp process_tests.cpp spdy_parser_tests.cpp tcp_server_tests.cpp tcp_stream_tests.cpp
piontests_LDADD = ../src/libpion.la @PION_EXTERNAL_LIBS@ @BOOST_TEST_LIB@
piontests_DEPENDENCIES = ../src/libpion.la \
	plugins/hasCreateAndDestroy.la plugins/hasCreateButNoDestroy.la \
	plugins/hasNoCreate.la

EXTRA_DIST = *.vcxproj *.vcxproj.filters boost*.xsd boost*.xsl config doc \
	http_parser_tests_data.inc spdy_parser_tests_data.inc
