#!/usr/bin/perl
#
# Copyright (C) 2014 Convey Computer Corporation. All rights reserved.
#
use warnings;
use strict;

my $this;
{
  my @path = split(/\//, $0);
  $this = pop(@path);
}

my $compare = '/usr/bin/compare';

if (! -x $compare) {
  die "$this: unable to locate executable $compare. Is ImageMagick installed?\n";
}

my $fuzz;
my @posArg;

while (defined (my $arg = shift)) {
  if ($arg =~ /^-/) {
    if ($arg eq '-fuzz') {
      $fuzz = shift or die "$this: -fuzz requires an argument";
    }

    else {
      die "$this: Unknown option $arg";
    }
  } else {
    push(@posArg, $arg);
  }
}

my $refImg  = shift @posArg or 
    die "$this: reference image (or directory) required\n";

my $testImg = shift @posArg or
    die "$this: test image (or directory) required\n";

my $outImg = shift @posArg;

$outImg = '/dev/null' if ! defined $outImg;

if (-d $refImg) {
  die "$this: $testImg - directory expected\n" if ! -d $testImg;

  if ($outImg ne '/dev/null') {
    die "$this: $outImg - directory expected\n"  if (-e $outImg && ! -d $outImg);

    if (! -e $outImg) {
      print "Directory $outImg not found - creating\n";
      mkdir $outImg;
    }
  }
} else {
  die "$this: $testImg - file expected\n" if -d $testImg;
  die "$this: $outImg - file expected\n"  if (-e $outImg && -d $outImg);
}

die "$this: Unable to locate $refImg\n"  if ! -e $refImg;
die "$this: Unable to locate $testImg\n" if ! -e $testImg;

if (defined $fuzz) {
  if ($fuzz =~ /^(\d+)%?$/) {
    my $f = $1;
    if ($f < 0 || $f > 100) {
      die "$this: fuzz value out of range (0-100)\n";
    } else {
      $compare .= " -fuzz $f%";
    }
  } else {
    die "$this: Illegal argument to -fuzz ($fuzz)";
  }
}



if (-d $refImg) {
  # Directory mode
  opendir(REFDIR, $refImg) or die "$this: unable to open dir $refImg\n";
  opendir(TESTDIR, $testImg) or die "$this: unable to open dir $testImg\n";

  my @refFiles  = sort grep !/^\.\.?$/, readdir REFDIR;
  my @testFiles = sort grep !/^\.\.?$/, readdir TESTDIR;

  closedir(TESTDIR);
  closedir(REFDIR);

  my $done = 0;

  while (! $done) {

    while (defined $refFiles[0] && defined $testFiles[0] &&
	   ($refFiles[0] eq $testFiles[0]) ) {

      my $filename = shift(@refFiles); shift(@testFiles);
      my $ref  = "$refImg/$filename";
      my $test = "$testImg/$filename";
      my $out = ($outImg eq '/dev/null') ? $outImg : "$outImg/$filename";

      my $pixelDiff = compare1($ref, $test, $out);
      print "$filename: $pixelDiff pixel(s) differ\n";
    }

    if (defined $refFiles[0] && defined $testFiles[0]) {

      if ($refFiles[0] lt $testFiles[0]) {
	print "INFO: Skipping $refImg/$refFiles[0] ";
	print "(doesn't exist in $testImg)\n";
	shift(@refFiles);
      } else {
	print "INFO: Skipping $testImg/$testFiles[0] ";
	print "(doesn't exist in $refImg)\n";
	shift(@testFiles);
      }

    } elsif (defined $refFiles[0]) {
      print "INFO: Skipping $refImg/$refFiles[0] ";
      print "(doesn't exist in $testImg) EOF\n";
      shift(@refFiles);

    } elsif (defined $testFiles[0]) {
      print "INFO: Skipping $testImg/$testFiles[0] ";
      print "(doesn't exist in $refImg) EOF\n";
      shift(@testFiles);

    } else {
      $done = 1;
    }
    
  }
}

else {
  # Single file mode

  my $pixelDiff = compare1($refImg, $testImg, $outImg);
  print "$refImg and $testImg differ by $pixelDiff pixels\n";
}

exit(0);


sub compare1 {
  my ($ref, $test, $out) = @_;

  my $cmd = "$compare -metric AE $ref $test $out";
  #print "$cmd\n";

  # The compare command prints the value we want to STDERR, so we
  # have to jump through some hoops in order to capture it.
  
  my $pid = open(COMPARE, "-|");	# Implicit fork()

  if (!defined $pid) {
    die "$this: Unable to fork: $!\n";
  } elsif ($pid == 0) {
    # Child
    open STDERR, ">&STDOUT";
    #select STDOUT; $| = 1;
    exec($cmd);
    die "Unreachable";
  }

  # Parent
  my $pixelDiff = <COMPARE>;
  chomp $pixelDiff;
  #die "$this: Unexpected output ($pixelDiff) from compare" unless $pixelDiff =~ /^\d+$/;

  close(COMPARE);
  
  return $pixelDiff;
}
