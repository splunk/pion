#!/usr/bin/perl
# --------------------------------------------------
# pion-platform configuration directory build script
# --------------------------------------------------

use File::Spec;
use File::Path;
use File::Copy;
use File::Glob ':glob';

# include perl source with common subroutines
require File::Spec->catfile( ("common", "build"), "common.pl");


# -----------------------------------
# process argv & set global variables
# -----------------------------------

# check command line parameters
die("usage: make_config.pl <SRCDIR> <DESTDIR> [<KEY>=<VALUE>]+") if ($#ARGV < 1);

# set some global variables
$SRCDIR = $ARGV[0];
$DESTDIR = $ARGV[1];


# ------------
# main process
# ------------

# check source directory
die "Source directory does not exist: $SRCDIR" if (! -d $SRCDIR);

# build template parameter hash (start with default values)
my %templates = ("PION_PLUGINS_DIRECTORY" => "../../platform/codecs/.libs/</PluginPath><PluginPath>../../platform/databases/.libs/</PluginPath><PluginPath>../../platform/reactors/.libs</PluginPath><PluginPath>../../platform/protocols/.libs/</PluginPath><PluginPath>../../platform/services/.libs/</PluginPath><PluginPath>../../net/services/.libs/",
	"PION_DATA_DIRECTORY" => ".",
	"PION_UI_DIRECTORY" => "../../platform/ui",
	"PION_LOG_CONFIG" => "logconfig.txt");

# update template parameter hash using values provided
for ($n = 2; $n <= $#ARGV; ++$n) {
	die "Bad template argument: \"$ARGV[$n]\"" if (! ($ARGV[$n] =~ m/([^=]+)=(.*)/) );
	$templates{$1} = $2;
}

# clear out old files and directories at destination
@oldfiles = bsd_glob($DESTDIR . "/*");
foreach (@oldfiles) {
	rmtree($_);
}

# copy files using templates
copyDirWithoutDotFiles($SRCDIR, $DESTDIR, %templates);

print "* Done building configuration directory ($SRCDIR => $DESTDIR)\n";
