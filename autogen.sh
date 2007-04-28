#!/bin/sh
#
# This script initializes the GNU autotools environment for Pion
#

#autoheader
aclocal -I build
autoconf
automake -a
