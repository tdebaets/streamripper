#! /usr/bin/perl
###########################################################################
# This is an example script that sends external metadata to streamripper.
# You invoke the script like this:
#    streamripper URL -E "fetch_external_metadata.pl META_URL"
#
# Of course, you will need perl and LWP::Simple installed. I assume 
# you already have perl installed.  To install LWP::Simple, simply 
# do this (as root):
#    perl -MCPAN -e 'install LWP::Simple';
#
# And of course, you'll need to modify this script to suit your needs.
#
# This script is in the public domain.  Modify and distribute as needed.
###########################################################################


use LWP::Simple;

if ($#ARGV != 0) {
    die "Usage: fetch_external_metadata.pl URL\n";
}
$url = $ARGV[0];

while (1) {
    my $content = get $url;

    if ($content =~ m/title="(.*)" artist="(.*)"/) {
	$title = "TITLE=$1\n";
	$artist = "ARTIST=$2\n";
	$end_of_record = ".\n";
	$meta_data = $title . $artist . $end_of_record;
	syswrite (STDOUT, $meta_data, length($meta_data));
    }
    sleep (10);
}
