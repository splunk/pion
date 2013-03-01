# --------------------------------
# pion automake configuration file
# --------------------------------

AM_CPPFLAGS = -I../include

bin_PROGRAMS = helloserver piond

helloserver_SOURCES = helloserver.cpp
helloserver_LDADD = ../src/libpion.la @PION_EXTERNAL_LIBS@
helloserver_DEPENDENCIES = ../src/libpion.la

piond_SOURCES = piond.cpp
piond_LDADD = ../src/libpion.la @PION_EXTERNAL_LIBS@
piond_DEPENDENCIES = ../src/libpion.la

EXTRA_DIST = sslkey.pem testservices.html *.conf *.vcxproj *.vcxproj.filters
