#!/usr/bin/perl

use strict;
use warnings;
use Cwd;

print "Execute perl cgi\n";

# Get the environment variable
my $fileName = $ENV{'filename'}; # Replace 'YOUR_ENV_VARIABLE_NAME' with the actual environment variable name
my $fullPath = $ENV{'fullpath'};
my $fd = int($ENV{'fd'});
# my $fullPath = "/home/petcha_nop/bb2/html/download/vnilprap.tar";

my $cwd = getcwd();

my $path = $cwd . "/html" . $fullPath;

print "$path\n";
print "$fullPath\n";

my $success = "FILE $fileName deleted successfully.\n";
my $fail = "File $path not found.\n";

if (-e $path) {
    unlink $path or die $fail;
    print $fd "HTTP/1.1 200 OK\r\n\r\nContent-Length: length($success)\r\nContent-Type: plain/text\r\n\r\n$success\r\n\r\n";
} else {
    print $fd $fail;
}