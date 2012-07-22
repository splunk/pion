# ---------------------------------------
# pion-common automake configuration file
# ---------------------------------------

AM_CPPFLAGS = -I../include

lib_LTLIBRARIES = libpion-common.la

libpion_common_la_SOURCES = PionLogger.cpp PionPlugin.cpp \
	PionScheduler.cpp PionAdminRights.cpp PionAlgorithms.cpp PionProcess.cpp

libpion_common_la_LDFLAGS = -no-undefined -release $(PION_LIBRARY_VERSION)
libpion_common_la_LIBADD = @PION_EXTERNAL_LIBS@ @PION_DLOPEN_LIBS@

EXTRA_DIST = *.vcproj
