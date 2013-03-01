# --------------------------------
# pion automake configuration file 
# --------------------------------

ACLOCAL_AMFLAGS = -I m4 -I build

SUBDIRS = include src services utils @PION_TESTS_MAKEDIRS@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pion.pc

EXTRA_DIST = README.md index.html autogen.sh *.sln *.xcodeproj *.vcxproj doc \
	build/*.props build/*.sh build/*.pl build/*.bat build/*.dll \
	build/pion.ico build/common.inc

include $(top_srcdir)/build/doxygen.inc

docs: doxygen-doc

clean-docs: doxygen-clean
