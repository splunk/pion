# ---------------------------------------
# pion-common automake configuration file 
# ---------------------------------------

AM_CPPFLAGS = -I../../include

# Note: For some reason, libtool does not build shared libs when check_LTLIBRARIES...
pion_pluginsdir = @PION_PLUGINS_DIRECTORY@
pion_plugins_LTLIBRARIES = hasCreateAndDestroy.la hasCreateButNoDestroy.la hasNoCreate.la
#check_LTLIBRARIES = hasCreateAndDestroy.la hasCreateButNoDestroy.la hasNoCreate.la

hasCreateAndDestroy_la_SOURCES = hasCreateAndDestroy.hpp hasCreateAndDestroy.cpp
hasCreateAndDestroy_la_LDFLAGS = -no-undefined -module -avoid-version
hasCreateAndDestroy_la_LIBADD = ../../src/libpion-common.la @PION_EXTERNAL_LIBS@
hasCreateAndDestroy_la_DEPENDENCIES = ../../src/libpion-common.la

hasCreateButNoDestroy_la_SOURCES = hasCreateButNoDestroy.hpp hasCreateButNoDestroy.cpp
hasCreateButNoDestroy_la_LDFLAGS = -no-undefined -module -avoid-version
hasCreateButNoDestroy_la_LIBADD = ../../src/libpion-common.la @PION_EXTERNAL_LIBS@
hasCreateButNoDestroy_la_DEPENDENCIES = ../../src/libpion-common.la

hasNoCreate_la_SOURCES = hasNoCreate.hpp hasNoCreate.cpp
hasNoCreate_la_LDFLAGS = -no-undefined -module -avoid-version
hasNoCreate_la_LIBADD = ../../src/libpion-common.la @PION_EXTERNAL_LIBS@
hasNoCreate_la_DEPENDENCIES = ../../src/libpion-common.la

EXTRA_DIST = *.vcproj
