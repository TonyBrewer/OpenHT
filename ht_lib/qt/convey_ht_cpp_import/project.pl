#!/usr/bin/perl -w

use strict;
use Getopt::Long;
use File::Basename;

my $optDryRun = 0;

my $optProjectName = '';
my $optTargetPath = '';
my $dir = '';
my $base = '';
my $ext = '';

GetOptions(
                   "dry-run" => \$optDryRun,
                   "project-name:s" => \$optProjectName,
                   "target-path:s" => \$optTargetPath);

if ($optDryRun) {
	printf "$optProjectName.pro\n";
} else {
	open ProjPro, ">>", "$optTargetPath/dsn13.pro" or die $!;
	opendir ( DIR, "$optTargetPath/src") or die $!;
	while(my $file = readdir(DIR)) {
	    ($base, $dir, $ext) = fileparse($file,'\..*');
	    if($ext eq ".cpp") {
		 print ProjPro "SOURCES\t\t+= src/$file\n";
	    }
	}
	print ProjPro "\n";
	close DIR;

	opendir ( DIR, "$optTargetPath/src") or die $!;
	while(my $file = readdir(DIR)) {
	    ($base, $dir, $ext) = fileparse($file,'\..*');
	    if($ext eq ".h") {
		 print ProjPro "HEADERS\t\t+= src/$file\n";
	    }
	}
	print ProjPro "\n";
	close DIR;

	opendir ( DIR, "$optTargetPath/src") or die $!;
	while(my $file = readdir(DIR)) {
	    ($base, $dir, $ext) = fileparse($file,'\..*');
	    if($ext eq ".htd") {
		 print ProjPro "OTHER_FILES\t+= src/$file\n";
	    }
	}

	print ProjPro "OTHER_FILES\t+= Makefile\n";
	close DIR;
	close ProjPro;
}
