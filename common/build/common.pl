# ------------------------------------------------
# pion-common common perl subroutines used by pion
# ------------------------------------------------

use File::Spec;
use File::Path;
use File::Copy;

# ----------------------
# some helpful functions
# ----------------------

# recursively copies an entire directory tree, excluding anything that starts with a dot
sub copyDirWithoutDotFiles(@) {
	my $src_dir = shift();
	my $dst_dir = shift();
	
	# clear out old copy (if one exists)
	rmtree($dst_dir);
	mkpath($dst_dir);

	# get list of files in source directory
	opendir(DIR, $src_dir);
	my @files = readdir(DIR);
	closedir(DIR);

	# iterate through source files
	foreach (@files) {
		if ( ! /^\./ ) {
			my $src_file = File::Spec->catfile($src_dir, $_);
			my $dst_file = File::Spec->catfile($dst_dir, $_);
			if (-d $src_file) {
				copyDirWithoutDotFiles($src_file, $dst_file);
			} else {
				copy($src_file, $dst_file);
			}
		}
	}
}

1;