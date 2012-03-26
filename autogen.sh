#!/bin/sh
#
# This script initializes the GNU autotools environment for Pion
#

# DO NOT USE autoheader -> config.h.in file is NOT automanaged!!!
AUTOHEADER=`which true`
export AUTOHEADER

# Generate configure script
autoreconf -ifs
