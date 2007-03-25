#! /usr/bin/perl 

$old_version = "";
$new_version = "";

sub replace_file {
    ($file) = @_;

    $tmp_fn = "uv.tmp";
    open TMP, ">$tmp_fn";
    open INF, "<$file";
    while (<INF>) {
	s/$old_version/$new_version/g;
	print TMP;
    }
    close INF;
    close TMP;
#    `mv $tmp_fn $file`;
    rename ($tmp_fn, $file);
}

###############################################################
##  main
###############################################################
$old_version = shift;
$new_version = shift;
if (!$new_version) {
    die "Usage: upgrade_version.pl old_version new_version\n";
}

@files = (
	  "configure.ac",
	  "lib/rip_manager.h",
	  "winamp_plugin/sr2x.nsi",
	  "makedist_win.bat",
	  "streamripper.1.asc",
	  );

for $file (@files) {
    replace_file ($file);
}
print "Don't forget to check the date in streamripper.1.asc\n";
print "Don't forget to recompile streamripper.1.asc\n";
