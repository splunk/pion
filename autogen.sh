#!/bin/sh
#
# This script initializes the GNU autotools environment for Pion
#

# DO NOT USE autoheader -> config.h.in file is NOT automanaged!!!
AUTOHEADER=`which true`
export AUTOHEADER

# Make sure m4 directory exists
if [ ! -d "m4" ]; then
  mkdir m4
fi

# Generate configure script
autoreconf -ifs
