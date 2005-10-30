#! /usr/bin/perl

use LWP::Simple;

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
    sleep (1);
}
