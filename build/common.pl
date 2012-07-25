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
    my %templates = @_;
    
    # make sure directory exists
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
                copyDirWithoutDotFiles($src_file, $dst_file, %templates);
            } elsif ( scalar keys %templates == 0 ) {
                # no templates specified -> just copy the file as-is
                copy($src_file, $dst_file);
            } else {
                # template parameters were specified
                # process key->value parameters while copying the file
                # WARNING: this probably only works for text files
                open(FROMFILE, $src_file) or die "could not open source template ($src_file)";
                open(TOFILE, ">", $dst_file) or die "could not open $dst_file for writing";
                while ( <FROMFILE> ) {
                    while ( ($key, $value) = each %templates ) {
                        s,\@$key\@,$value,;
                    }
                    print TOFILE $_;
                }
                close(FROMFILE);
                close(TOFILE);
            }
        }
    }
}

1;