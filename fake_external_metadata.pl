#! /usr/bin/perl
###########################################################################
# This is an external script that I use for testing streamripper's 
# external program interface.  It implements the following:
#
#   1) Generates (random) title and artist information
#   2) Sends the information to streamripper
# 
# To invoke the script, do this:
#    streamripper URL -E "perl fake_external_metadata.pl META_URL"
#
# This script is in the public domain. You are free to use, modify and 
# redistribute without restrictions.
###########################################################################

$repno = 1 + rand * 5;

$ts = "AAA";
$as = "001";
$title = "TITLE=$ts\n";
$artist = "ARTIST=$as\n";
while (1) {
    if ($repno-- < 0) {
	$repno = 5 + rand * 5;
	$as++;
	$ts++;
	$title = "TITLE=$ts\n";
	$artist = "ARTIST=$as\n";
    }
    $end_of_record = ".\n";
    $meta_data = $title . $artist . $end_of_record;
    syswrite (STDOUT, $meta_data, length($meta_data));
    sleep (10);
}
