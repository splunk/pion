#!/usr/bin/perl
# --------------------------------------------------
# make_rpm.pl: script for building pion rpm packages
# --------------------------------------------------

use Cwd;
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
die("usage: make_rpm.pl <VERSION> <RELEASE.ARCH> [--nostrip]")
	if ($#ARGV < 1 || $#ARGV > 2 || ($#ARGV == 2 && $ARGV[2] ne "--nostrip"));

# must be run as root
die("This script must be run as root!") if $>!=0;

# set some global variables
$VERSION = $ARGV[0];
$RELEASE = $ARGV[1];

# check for --nostrip option
if ($ARGV[2] eq "--nostrip") {
	$INSTALL_BIN = 'install';
	$SPEC_OPTIONS = '%define __os_install_post %{nil}';
} else {
	$INSTALL_BIN = 'install -s';
	$SPEC_OPTIONS = '';
}

# check validity of RELEASE parameter
die("Second parameter must be format <RELEASE.ARCH> (i.e. \"1.el5\")")
	if ($RELEASE !~ m/^\d+\..+$/);

# find binary directory
$BIN_DIR = "bin";
$RPMS_DIR = "/usr/src/redhat/RPMS";
$BUILD_DIR = "/usr/src/redhat/BUILD";
$DIR_GLOB = $BIN_DIR . "/pion-*-" . $VERSION;
@PACKAGES = bsd_glob( $DIR_GLOB );
die("error: unable to find binary directory") if ($#PACKAGES != 0);
$EDITION = $PACKAGE_BASE = $PACKAGE_DIR = $PACKAGES[0];
$PACKAGE_BASE =~ s/.*(pion-[a-z]+)-.*/$1/;
$EDITION =~ s/.*pion-([a-z]+)-.*/$1/;
$BIN_SRC_BASE = $PACKAGE_BASE . "-" . $VERSION . "-" . $RELEASE;
$BIN_SRC_DIR = "$BUILD_DIR/$BIN_SRC_BASE";
$SPEC_FILE_NAME = "/tmp/$BIN_SRC_BASE.spec";
$CONFIG_DIR = File::Spec->catdir( ($PACKAGE_DIR, "config") );
$PLUGINS_DIR = File::Spec->catdir( ($PACKAGE_DIR, "plugins") );
$LIBS_DIR = File::Spec->catdir( ($PACKAGE_DIR, "libs") );
$UI_DIR = File::Spec->catdir( ($PACKAGE_DIR, "ui") );


# ------------
# main process
# ------------

die("error: $PACKAGE_DIR is not a directory") if (! -d $PACKAGE_DIR);
die("error: $CONFIG_DIR is not a directory") if (! -d $CONFIG_DIR);
die("error: $PLUGINS_DIR is not a directory") if (! -d $PLUGINS_DIR);
die("error: $LIBS_DIR is not a directory") if (! -d $LIBS_DIR);
die("error: $UI_DIR is not a directory") if (! -d $UI_DIR);

print "* Building RPM binary package for " . $TARBALL_BASE . "\n";

print "* Generating RPM spec file..\n";

# prepare some vars for spec file
if ($EDITION eq "community") {
	$spec_license = "GPL";
	$config_file_glob = "*.{xml,txt,pem}";
	$install_perl_scripts = "";
} else {
	$spec_license = "commercial";
	$config_file_glob = "*.{xml,txt,pem,cap}";
	$install_perl_scripts = "install -m 660 $BIN_SRC_BASE/config/*.pl \$RPM_BUILD_ROOT/var/lib/pion";
}
$SPEC_POST="/sbin/ldconfig";
$SPEC_POSTUN="/sbin/ldconfig";
@spec_libs = bsd_glob($LIBS_DIR . "/*");
@purge_scripts = bsd_glob($CONFIG_DIR . "/*.pl");

# open and write the spec file
open(SPEC_FILE, ">$SPEC_FILE_NAME") or die("Unable to open spec file: $SPEC_FILE_NAME");
print SPEC_FILE << "END_SPEC_FILE";
Summary: Software for real-time data capture, processing and integration
Name: $PACKAGE_BASE
Version: $VERSION
Release: $RELEASE
Vendor: Atomic Labs, Inc.
License: $spec_license
Group: System Environment/Daemons
BuildRoot: /tmp/\%{name}-buildroot

\%define debug_package \%{nil}

\%description
Pion captures millions of events per second from hard to manage sources such as
log files, live network traffic and clickstream data. Data is filtered and
sorted before being delivered to Pion's visual event processing interface.
Events captured by Pion are processed in real-time through Pion's visual event
processing interface. Data can easily be filtered, transformed, sessionized and
aggregated before being delivered to the data warehouse for storage and future
use. Data can be easily loaded into a variety of popular databases, or into an
embedded SQLite database for quick access. Connect Pion's data up to your
reporting packages and real-time dashboards for up-to-the-minute operational
intelligence.

\%prep

\%build

\%pre
useradd -r -c "Pion" pion 2> /dev/null || true

\%post
$SPEC_POST

\%postun
#userdel pion 2> /dev/null || true
$SPEC_POSTUN

\%install
rm -rf \$RPM_BUILD_ROOT
mkdir -p \$RPM_BUILD_ROOT/etc/rc.d/init.d
mkdir -p \$RPM_BUILD_ROOT/etc/pion
mkdir -p \$RPM_BUILD_ROOT/etc/pion/vocabularies
mkdir -p \$RPM_BUILD_ROOT/var/log/pion
mkdir -p \$RPM_BUILD_ROOT/var/lib/pion
mkdir -p \$RPM_BUILD_ROOT/usr/bin
mkdir -p \$RPM_BUILD_ROOT/usr/lib
mkdir -p \$RPM_BUILD_ROOT/usr/share/pion/ui
mkdir -p \$RPM_BUILD_ROOT/usr/share/pion/plugins
mkdir -p \$RPM_BUILD_ROOT/usr/share/doc/$PACKAGE_BASE-$VERSION

install -m 660 $BIN_SRC_BASE/config/$config_file_glob \$RPM_BUILD_ROOT/etc/pion
$install_perl_scripts
install -m 660 $BIN_SRC_BASE/config/vocabularies/*.xml \$RPM_BUILD_ROOT/etc/pion/vocabularies
install -m 775 $BIN_SRC_BASE/pion.service \$RPM_BUILD_ROOT/etc/rc.d/init.d/pion
$INSTALL_BIN $BIN_SRC_BASE/plugins/* \$RPM_BUILD_ROOT/usr/share/pion/plugins
$INSTALL_BIN $BIN_SRC_BASE/libs/* \$RPM_BUILD_ROOT/usr/lib
$INSTALL_BIN $BIN_SRC_BASE/pion \$RPM_BUILD_ROOT/usr/bin/pion
install $BIN_SRC_BASE/pget.py \$RPM_BUILD_ROOT/usr/bin/pget.py
install $BIN_SRC_BASE/pmon.py \$RPM_BUILD_ROOT/usr/bin/pmon.py
cp -r $BIN_SRC_BASE/ui/* \$RPM_BUILD_ROOT/usr/share/pion/ui


\%clean
rm -rf \$RPM_BUILD_ROOT


%define debug_package %{nil}
$SPEC_OPTIONS


\%files

\%defattr(-,pion,pion)
\%config /etc/pion/
\%dir /var/lib/pion

\%defattr(-,root,root)
\%doc $BIN_SRC_BASE/HISTORY.txt $BIN_SRC_BASE/LICENSE.txt $BIN_SRC_BASE/README.txt $BIN_SRC_BASE/pion-manual.pdf

\%defattr(755,root,root)
/usr/bin/pion
/usr/bin/pget.py*
/usr/bin/pmon.py*

\%defattr(-,root,root)
/var/log/pion
/usr/share/pion/ui

\%defattr(755,root,root)
/etc/rc.d/init.d/pion
/usr/share/pion/plugins
END_SPEC_FILE

# output library file names
foreach $_ (@spec_libs) {
	s[$LIBS_DIR][/usr/lib];
	print SPEC_FILE $_ . "\n";
}

# output purge scripts (if any)
foreach $_ (@purge_scripts) {
	s[$CONFIG_DIR][/var/lib/pion];
	print SPEC_FILE $_ . "\n";
}


# close the spec file
close(SPEC_FILE);


print "* Preparing binary source directory..\n";

`rm -rf $BIN_SRC_DIR`;
copyDirWithoutDotFiles($PACKAGE_DIR, $BIN_SRC_DIR);
if ($EDITION eq "community") {
	copyDirWithoutDotFiles("platform/build/rpm", $BIN_SRC_DIR);
} else {
	# find the pion-platform directory
	$_ = getcwd();
	if (/pion-[^-]+-/) {
		s,/$,,;
		s,\\$,,;
		s,pion-[^-]+-(.*),pion-platform-$1,;
		if (-d $_) {
			$PION_PLATFORM_DIR = $_;
		} else {
			$PION_PLATFORM_DIR = File::Spec->catdir( ("..", "pion-platform") );
		}
	} else {
		$PION_PLATFORM_DIR = File::Spec->catdir( ("..", "pion-platform") );
	}
	die("Could not find pion-platform directory: $PION_PLATFORM_DIR") if (! -d $PION_PLATFORM_DIR);
	copyDirWithoutDotFiles($PION_PLATFORM_DIR . "/platform/build/rpm", $BIN_SRC_DIR);
	copyDirWithoutDotFiles("enterprise/build/rpm", $BIN_SRC_DIR);
}


print "* Creating RPM files..\n";

`rpmbuild --quiet -bb $SPEC_FILE_NAME`;


print "* Cleaning up..\n";

`mv $RPMS_DIR/*/* .`;
`rm $SPEC_FILE_NAME`;
`rm -rf $BIN_SRC_DIR`;

print "* Done.\n";

