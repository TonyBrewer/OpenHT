#!/usr/bin/perl

use warnings;
use strict;

use HTTest;

#########################################
## Globals
#########################################

# Basic Test Variables
my $numTests = 32;

my @testList = ();

#########################################
## Main Program
#########################################
my $seed = time;
#my $seed = ;
srand($seed);
print sprintf("gen_test.pl (%d)\n\n", $seed);

#$SIG{__WARN__} = sub { $DB::single = 1 };

# Remove old tests
print "Cleaning old tests...\n";
`make distclean`;
`rm -rf src_pers *~`;
`mkdir -p src_pers`;
print "  Done\n\n";

print "Generating random test data...\n";
&genTestData();
print "  Generating HTI\n";
&genHTI();
print "  Generating HTD\n";
&genHTD();
print "  Generating PersCtl\n";
&genCTL();
my $multTestStr = sprintf(" -> PersTest%02d", ($numTests-1));
print "  Generating PersTest00".(($numTests == 1) ? "" : "$multTestStr")."\n";
&genTest();
print "  Done\n\n";

exit(0);


#########################################
## Subroutines
#########################################
sub genTestData {
  for (my $curTest = 0; $curTest < $numTests; $curTest++) {
    my $newTest = new HTTest($curTest);
    push(@testList, $newTest);
  }
}

sub genHTI {
  # Port 0 is already used once for Host Intf, so start at 1
  my $memPort = 0;
  my $useCnt = 1;
  my $file = "src_pers/memPSGRand.hti";

  open(OFH, ">$file") or die "Couldn't open $file for writing: $!\n";

  for (my $curTest = 0; $curTest < $numTests; $curTest++) {
    my $testStr = sprintf("%02d", $curTest);
    print OFH "AddModInstParams(unit=Au, modPath=\"/Ctl/Test${testStr}\", memPort=$memPort);\n";

    $useCnt++;
    if ($useCnt >= 8) {
      $useCnt = 0;
      $memPort++;
    }

    if ($memPort >= 8) {
      close(OFH);
      die "ERROR: requires more memory ports than avaiable! Test count cannot exceed 63.\n";
    }
  }

  close(OFH);
  return;
}

sub genHTD {
  my $file = "src_pers/memPSGRand.htd";
  open(OFH, ">$file") or die "Couldn't open $file for writing: $!\n";

  # Header
  print OFH "typedef sc_uint<48> MemAddr_t;\n";
  print OFH "\n";

  # Structs
  print OFH &getStructDefStr();
  print OFH "\n";
  print OFH "\n";

  # Ctl
  {
    # Header
    print OFH "/////////////////////////////////////\n";
    print OFH "// CTL\n";
    print OFH "\n";
    print OFH "dsnInfo.AddModule(name=Ctl, htIdW=0);\n";
    print OFH "\n";

    # Instructions
    for (my $curTest = 0; $curTest < $numTests; $curTest++) {
      print OFH sprintf("Ctl.AddInstr(name=CTL_TEST%02d);\n", $curTest);
    }
    print OFH "Ctl.AddInstr(name=CTL_RTN);\n";
    print OFH "\n";

    # Entry
    print OFH "Ctl.AddEntry(func=main, instr=CTL_TEST00, host=true)\n";
    print OFH "\t.AddParam(hostType=uint64_t *, type=MemAddr_t, name=memAddr)\n";
    print OFH "\t;\n";
    print OFH "\n";

    # Return
    print OFH "Ctl.AddReturn(func=main)\n";
    print OFH "\t;\n";
    print OFH "\n";

    # Calls
    for (my $curTest = 0; $curTest < $numTests; $curTest++) {
      print OFH sprintf("Ctl.AddCall(func=test%02d);\n", $curTest);
    }
    print OFH "\n";

    # Vars
    print OFH "Ctl.AddPrivate()\n";
    print OFH "\t.AddVar(type=MemAddr_t, name=memAddr)\n";
    print OFH "\t;\n";
    print OFH "\n";
    print OFH "\n";
  }

  # Tests
  foreach my $test (@testList) {
    my $testStr = sprintf("%02d", $test->getTestStr());

    # Header
    print OFH "/////////////////////////////////////\n";
    print OFH "// TEST${testStr}\n";
    print OFH "\n";
    print OFH sprintf("#define TEST%s_HTID_W %d\n",
		      $testStr,
		      $test->getTestHTIDW()
		     );
    my $clkStr = "";
    if ($test->getTestClk1x2x() != 0) {
      $clkStr = sprintf(", clock=%dx", $test->getTestClk1x2x());
    }
    print OFH "dsnInfo.AddModule(name=Test${testStr}, htIdW=TEST${testStr}_HTID_W${clkStr});\n";
    print OFH "\n";

    # Instructions
    print OFH "Test${testStr}.AddInstr(name=TEST${testStr}_ENTRY);\n";
    for (my $idx = 0; $idx < $test->getTestWRCHKCnt; $idx++) {
      print OFH "Test${testStr}.AddInstr(name=TEST${testStr}_WR${idx});\n";
    }
    for (my $idx = 0; $idx < $test->getTestCycCnt; $idx++) {
      print OFH "Test${testStr}.AddInstr(name=TEST${testStr}_ST${idx});\n";
    }
    for (my $idx = 0; $idx < $test->getTestCycCnt; $idx++) {
      print OFH "Test${testStr}.AddInstr(name=TEST${testStr}_LD${idx});\n";
    }
    for (my $idx = 0; $idx < $test->getTestWRCHKCnt; $idx++) {
      print OFH "Test${testStr}.AddInstr(name=TEST${testStr}_CHK${idx});\n";
    }
    print OFH "Test${testStr}.AddInstr(name=TEST${testStr}_RTN);\n";
    print OFH "\n";

    # Entry
    print OFH "Test${testStr}.AddEntry(func=test${testStr}, instr=TEST${testStr}_ENTRY)\n";
    print OFH "\t.AddParam(hostType=uint64_t *, type=MemAddr_t, name=memAddr)\n";
    print OFH "\t;\n";
    print OFH "\n";

    # Return
    print OFH "Test${testStr}.AddReturn(func=test${testStr})\n";
    print OFH "\t;\n";
    print OFH "\n";

    # Staging
    if ($test->getTestStaging() != 0) {
      print OFH sprintf("Test${testStr}.AddStage(execStg=%d, privWrStg=%d)\n",
			$test->getTestExStg(),
			$test->getTestPrWrStg()
			);
      print OFH "\t;\n";
      print OFH "\n";
    }

    # Private Vars
    print OFH "Test${testStr}.AddPrivate()\n";
    print OFH "\t.AddVar(type=MemAddr_t, name=memAddr)\n";
    foreach my $var (@{$test->getVarsByPSG("P")}) {
      print OFH sprintf("\t.AddVar(type=%s, name=%s%s%s%s%s)\n",
			$var->getVarTopParent()->getVarType(),
			$var->getVarTopParent()->getVarName(),
			($var->getAddr1Width() == 0) ? "" : ", addr1=".$var->getAddr1Var()->{Name},
			($var->getAddr2Width() == 0) ? "" : ", addr2=".$var->getAddr2Var()->{Name},
			($var->getVarTopParent()->getDim1Size() == 0) ? "" : ", dimen1=".$var->getVarTopParent()->getDim1Size(),
			($var->getVarTopParent()->getDim2Size() == 0) ? "" : ", dimen2=".$var->getVarTopParent()->getDim2Size()
		       );
    }
    foreach my $var (@{$test->getRAVarsByPSG("P")}) {
      print OFH sprintf("\t.AddVar(type=%s, name=%s)\n",
			$var->{Type},
			$var->{Name}
		       );
    }
    print OFH "\t;\n";
    print OFH "\n";

    # Shared Vars
    if ($#{$test->getVarsByPSG("S")} != -1) {
    print OFH "Test${testStr}.AddShared()\n";
      foreach my $var (@{$test->getVarsByPSG("S")}) {
	print OFH sprintf("\t.AddVar(type=%s, name=%s%s%s%s%s%s)\n",
			  $var->getVarTopParent()->getVarType(),
			  $var->getVarTopParent()->getVarName(),
			  ($var->getAddr1Width() == 0) ? "" : ", addr1W=".$var->getAddr1Width(),
			  ($var->getAddr2Width() == 0) ? "" : ", addr2W=".$var->getAddr2Width(),
			  ($var->getVarTopParent()->getDim1Size() == 0) ? "" : ", dimen1=".$var->getVarTopParent()->getDim1Size(),
			  ($var->getVarTopParent()->getDim2Size() == 0) ? "" : ", dimen2=".$var->getVarTopParent()->getDim2Size(),
			  ($var->getVarIsBlkRam() eq "") ? "" : ", blockRam=".$var->getVarIsBlkRam()
			 );
      }
      print OFH "\t;\n";
      print OFH "\n";
    }

    # Global Vars
    if ($#{$test->getVarsByPSG("G")} != -1) {
    print OFH "Test${testStr}.AddGlobal()\n";
      foreach my $var (@{$test->getVarsByPSG("G")}) {
	print OFH sprintf("\t.AddVar(type=%s, name=%s%s%s%s%s%s%s%s%s%s)\n",
			  $var->getVarTopParent()->getVarType(),
			  $var->getVarTopParent()->getVarName(),
			  ($var->getAddr1Width() == 0) ? "" : ", addr1=".$var->getAddr1Var()->{Name},
			  ($var->getAddr2Width() == 0) ? "" : ", addr2=".$var->getAddr2Var()->{Name},
			  ($var->getVarTopParent()->getDim1Size() == 0) ? "" : ", dimen1=".$var->getVarTopParent()->getDim1Size(),
			  ($var->getVarTopParent()->getDim2Size() == 0) ? "" : ", dimen2=".$var->getVarTopParent()->getDim2Size(),
			  ($var->getVarGlbRdStg() != 0) ? ", rdStg=".$var->getVarGlbRdStg() : "",
			  ($var->getVarGlbWrStg() != 0) ? ", wrStg=".$var->getVarGlbWrStg() : "",
			  ($var->getVarGlbInstRStr() eq "") ? "" : ", instrRead=".$var->getVarGlbInstRStr(),
			  ($var->getVarGlbInstWStr() eq "") ? "" : ", instrWrite=".$var->getVarGlbInstWStr(),
			  ($var->getVarIsBlkRam() eq "") ? "" : ", blockRam=".$var->getVarIsBlkRam()
			 );
      }
      print OFH "\t;\n";
      print OFH "\n";
    }


    # Read Mem
    print OFH "Test${testStr}.AddReadMem()\n";
    for (my $idx = 0; $idx < $test->getTestCycCnt(); $idx++) {
      my $dstVar = $test->getDstVar($idx);
      print OFH sprintf("\t.AddDst(var=%s, name=%s, memSrc=host%s)\n",
			$dstVar->getChainNameHTD(),
			$dstVar->getVarName(),
			$dstVar->getVarRWTypeStr("R")
		       );
    }
    print OFH "\t;\n";
    print OFH "\n";

    # Write Mem
    print OFH "Test${testStr}.AddWriteMem()\n";
    my %tempTypes = ();
    for (my $idx = 0; $idx < $test->getTestCycCnt(); $idx++) {
      my $srcVar = $test->getSrcVar($idx);
      if ($srcVar->getVarWrType() eq "Var") {
	print OFH sprintf("\t.AddSrc(var=%s, name=%s, memDst=host%s)\n",
			  $srcVar->getChainNameHTD(),
			  $srcVar->getVarName(),
			  $srcVar->getVarRWTypeStr("W")
			 );
      } else {
	if (!defined($tempTypes{$srcVar->getVarType()})) {
	  print OFH sprintf("\t.AddSrc(type=%s, memDst=host%s)\n",
			    $srcVar->getVarType(),
			    $srcVar->getVarRWTypeStr("W")
			   );
	  $tempTypes{$srcVar->getVarType()} = 1;
	}
      }
    }
    print OFH "\t;\n";
    print OFH "\n";
    print OFH "\n";
  }

  close(OFH);
  return;
}

sub genCTL {
  my $file = "src_pers/PersCtl_src.cpp";

  open(OFH, ">$file") or die "Couldn't open $file for writing: $!\n";

  # Header
  print OFH "#include \"Ht.h\"\n";
  print OFH "#include \"PersCtl.h\"\n";
  print OFH "\n";
  print OFH "void CPersCtl::PersCtl() {\n";
  print OFH "\tif (PR_htValid) {\n";
  print OFH "\t\tswitch (PR_htInst) {\n";

  # Tests
  foreach my $test (@testList) {
    my $testStr = sprintf("%02d", $test->getTestStr());
    my $nextInstStr = ($test->getTestStr() == ($numTests-1) ? "RTN" : sprintf("TEST%02d", $test->getTestStr()+1));
    print OFH "\t\tcase CTL_TEST${testStr}: {\n";
    print OFH "\t\t\tif (SendCallBusy_test${testStr}()) {\n";
    print OFH "\t\t\t\tHtRetry();\n";
    print OFH "\t\t\t\tbreak;\n";
    print OFH "\t\t\t}\n";
    print OFH "\t\t\tSendCall_test${testStr}(CTL_${nextInstStr}, PR_memAddr);\n";
    print OFH "\t\t\tbreak;\n";
    print OFH "\t\t}\n";
  }

  # Footer
  print OFH "\t\tcase CTL_RTN: {\n";
  print OFH "\t\t\tif (SendReturnBusy_main()) {\n";
  print OFH "\t\t\t\tHtRetry();\n";
  print OFH "\t\t\t\tbreak;\n";
  print OFH "\t\t\t}\n";
  print OFH "\t\t\tSendReturn_main();\n";
  print OFH "\t\t\tbreak;\n";
  print OFH "\t\t}\n";
  print OFH "\t\tdefault:\n";
  print OFH "\t\t\tassert(0);\n";
  print OFH "\t\t}\n";
  print OFH "\t}\n";
  print OFH "}\n";

  close(OFH);
  return;
}


sub genTest {
  foreach my $test (@testList) {
    my $testStr = sprintf("%02d", $test->getTestStr());
    my $file = "src_pers/PersTest${testStr}_src.cpp";

    open(OFH, ">$file") or die "Couldn't open $file for writing: $!\n";

    # Header
    print OFH "#include \"Ht.h\"\n";
    print OFH "#include \"PersTest${testStr}.h\"\n";
    print OFH "\n";
    print OFH "void CPersTest${testStr}::PersTest${testStr}() {\n";

    # Staging
    my $stgListRef = $test->getStgList();

    foreach my $stg (@$stgListRef) {
      my $ES_VLD = ($stg == 0) ? 1 : (($stg == $test->getTestExStg()) ? 1 : 0);
      $stg = "" if ($stg == 0);

      print OFH "\tif (PR${stg}_htValid) {\n";
      print OFH "\t\tswitch (PR${stg}_htInst) {\n";

      # Entry
      print OFH "\t\tcase TEST${testStr}_ENTRY: {\n";
      if ($ES_VLD) {
	print OFH "\t\t\tHtContinue(TEST${testStr}_WR0);\n";
      }
      print OFH "\t\t\tbreak;\n";
      print OFH "\t\t}\n";

      # WR
      for (my $elem = 0; $elem < $test->getTestWRCHKCnt(); $elem++) {
	print OFH "\t\tcase TEST${testStr}_WR${elem}: {\n";
	foreach my $line (split(/\n/, $test->getTestWRStr($stg, $elem))) {
	  print OFH "\t\t\t$line\n";
	}
	foreach my $line (split(/\n/, $test->getTestWRRAStr($stg, $elem))) {
	  print OFH "\t\t\t$line\n";
	}
	if ($ES_VLD) {
	  if ($elem != $test->getTestWRCHKCnt()-1) {
	    print OFH "\t\t\tHtContinue(TEST${testStr}_WR".($elem+1).");\n";
	  } else {
	    print OFH "\t\t\tHtContinue(TEST${testStr}_ST0);\n";
	  }
	}
	print OFH "\t\t\tbreak;\n";
	print OFH "\t\t}\n";
      }

      # ST
      for (my $idx = 0; $idx < $test->getTestCycCnt(); $idx++) {
	print OFH "\t\tcase TEST${testStr}_ST${idx}: {\n";
	if ($ES_VLD) {
	  print OFH "\t\t\tif (WriteMemBusy()) {\n";
	  print OFH "\t\t\t\tHtRetry();\n";
	  print OFH "\t\t\t\tbreak;\n";
	  print OFH "\t\t\t}\n";
	  foreach my $line (split(/\n/, $test->getTestSTStr($stg, $idx))) {
	    print OFH "\t\t\t$line\n";
	  }
	  if ($idx != $test->getTestCycCnt()-1) {
	    print OFH "\t\t\tHtContinue(TEST${testStr}_ST".($idx+1).");\n";
	  } else {
	    print OFH "\t\t\tWriteMemPause(TEST${testStr}_LD0);\n";
	  }
	} else {
	  foreach my $line (split(/\n/, $test->getTestSTRAStr($stg, $idx))) {
	    print OFH "\t\t\t$line\n";
	  }
	}
	print OFH "\t\t\tbreak;\n";
	print OFH "\t\t}\n";
      }

      # LD
      for (my $idx = 0; $idx < $test->getTestCycCnt(); $idx++) {
	print OFH "\t\tcase TEST${testStr}_LD${idx}: {\n";
	if ($ES_VLD) {
	  print OFH "\t\t\tif (ReadMemBusy()) {\n";
	  print OFH "\t\t\t\tHtRetry();\n";
	  print OFH "\t\t\t\tbreak;\n";
	  print OFH "\t\t\t}\n";
	  foreach my $line (split(/\n/, $test->getTestLDStr($stg, $idx))) {
	    print OFH "\t\t\t$line\n";
	  }
	}
	foreach my $line (split(/\n/, $test->getTestLDRAStr($stg, $idx, 0))) {
	  print OFH "\t\t\t$line\n";
	}
	if ($ES_VLD) {
	  if ($idx != $test->getTestCycCnt()-1) {
	    print OFH "\t\t\tHtContinue(TEST${testStr}_LD".($idx+1).");\n";
	  } else {
	    print OFH "\t\t\tReadMemPause(TEST${testStr}_CHK0);\n";
	  }
	}
	print OFH "\t\t\tbreak;\n";
	print OFH "\t\t}\n";
      }

      # CHK
      for (my $elem = 0; $elem < $test->getTestWRCHKCnt(); $elem++) {
	print OFH "\t\tcase TEST${testStr}_CHK${elem}: {\n";
	for (my $cyc = 0; $cyc < $test->getTestCycCnt(); $cyc++) {
	  foreach my $line (split(/\n/, $test->getTestCHKStr($stg, $cyc, $elem))) {
	    print OFH "\t\t\t$line\n";
	  }
	  if (($test->getTestWRCHKCnt() > 1) && ($elem != $test->getTestWRCHKCnt()-1)) {
	    foreach my $line (split(/\n/, $test->getTestLDRAStr($stg, $cyc, $elem+1))) {
	      print OFH "\t\t\t$line\n";
	    }
	  }
	}
	if ($ES_VLD) {
	  if ($elem != $test->getTestWRCHKCnt()-1) {
	    print OFH "\t\t\tHtContinue(TEST${testStr}_CHK".($elem+1).");\n";
	  } else {
	    print OFH "\t\t\tHtContinue(TEST${testStr}_RTN);\n";
	  }
	}
	print OFH "\t\t\tbreak;\n";
	print OFH "\t\t}\n";
      }

      # Return
      print OFH "\t\tcase TEST${testStr}_RTN: {\n";
      if ($ES_VLD) {
	print OFH "\t\t\tif (SendReturnBusy_test${testStr}()) {\n";
	print OFH "\t\t\t\tHtRetry();\n";
	print OFH "\t\t\t\tbreak;\n";
	print OFH "\t\t\t}\n";
	print OFH "\t\t\tSendReturn_test${testStr}();\n";
      }
      print OFH "\t\t\tbreak;\n";
      print OFH "\t\t}\n";

      # Footer
      print OFH "\t\tdefault:\n";
      print OFH "\t\t\tassert(0);\n";
      print OFH "\t\t}\n";
      print OFH "\t}\n";

    }
    print OFH "}\n";

    close(OFH);
  }
  return;
}


sub isStgStrVld {
  my $PSG = shift;
  my $alt = shift;
  my $stgRef = shift;

  my $vld = 1;

  # Check PWS
  if (!defined($stgRef->{PWS}) && ($PSG eq "P")) {
    $vld = 0;
  }

  # Check PWS != ES (Only Assign S/G once!)
  if (!defined($stgRef->{PWS}) || !defined($stgRef->{ES})) {
    if (defined($stgRef->{PWS}) && ($PSG ne "P")) {
      $vld = 0;
    }
  }

  if ($PSG eq "G") {
    if (($alt eq "WR") && !defined($stgRef->{SGW})) {
      $vld = 0;
    }
    if (($alt eq "RD") && !defined($stgRef->{DGR})) {
      $vld = 0;
    }
  }
  return $vld;
}

#########################################
## Utilities
#########################################
sub genChance {
  my $passChance = shift;
  my $passVal = shift;
  my $failVal = shift;

  my $rand = int(rand(100))+1;

  if (($passChance*100) >= $rand) {
    return $passVal;
  } else {
    return $failVal;
  }
}


#########################################
## Variable String Utility Functions
#########################################
sub getStructDefStr {
  my $rtnStr = "";
  foreach my $test (@testList) {
    $rtnStr .= $test->getTestStructDefStr();
  }
  return $rtnStr;
}

#########################################
## Variable Utility Functions
#########################################
