#!/usr/bin/perl
use strict;

use IO::File;

# Open up the disk image for random modifications (+<)
my $f = new IO::File("+<./disk0.img") || die("cannot open ./disk0.img: $!\n");
# Seek to the next pointer of the last i-node in block 7.  That really
# shouldn't be hard coded.
die "Seek failed: $!\n" unless $f->seek(0x7f80, 0);
# verify data
my $byte;
$f->read($byte, 1); $byte = ord $byte; if ($byte != 0xe0) { die "Verification failed: data pattern is not 0xe0000000\n"; }
$f->read($byte, 1); $byte = ord $byte; if ($byte != 0x00) { die "Verification failed: data pattern is not 0xe0000000\n"; }
$f->read($byte, 1); $byte = ord $byte; if ($byte != 0x00) { die "Verification failed: data pattern is not 0xe0000000\n"; }
$f->read($byte, 1); $byte = ord $byte; if ($byte != 0x00) { die "Verification failed: data pattern is not 0xe0000000\n"; }
# shouldn't be hard coded.
die "2nd Seek failed: $!\n" unless $f->seek(0x7f80, 0);
# Write a -1 (0xffffffff) there.
die "Write failed: $!\n" unless $f->write(pack("l", -1, 4));
$f->close();
