#!/usr/bin/perl -w

use warnings;
use strict;
use HTVar;

################################################
# Globals
################################################

################################################
# Global Chances / Switches
################################################
my @ARR_TestCycCnt = (1..5);

my $PCT_TestHTIDW_GTZ   = 0.50;
my @ARR_TestHTIDW_HTIDW = (1..6);

my $PCT_TestStaging_STG     = 0.50;
my $PCT_TestStaging_SAME_PE = 0.50;
my $PCT_TestStaging_GSAME   = 0.50;
my $PCT_TestStaging_GDEF    = 0.50;
my @ARR_TestStaging_STG     = (1..5);

my $PCT_InstrRW_DEF = 0.50;
my $PCT_InstrRW_TRUE = 0.50;

my $PCT_TestClk1x2x_1X   = 0.75;
my $PCT_TestClk1x2x_2X   = 0.25;
my $PCT_TestClk1x2x_CDEF = 0.50;

my $PCT_CmnElemCnt_GEN = 0.50;
my $PCT_CmnElemCnt_MUL = 0.75;
my @ARR_CmnElemCnt_CNT = (2..8);

my $CNT_MaxQWPerCyc_MAX = 8;

################################################
# Static Variables
################################################


package HTTest;

# Initialize HTTest Object
sub new {
  my $class = shift;

  my $testStr = shift;

  my $self = {
	      # Basic Test Attributes
	      _testStr      => $testStr,
	      _testCycCnt   => -1,
	      _testHTIDW    => -1,
	      _testStaging  => -1,
	      _testExStg    => -1,
	      _testPrWrStg  => -1,
	      _testWRCHKCnt => -1,

	      # Variables
	      _testClk1x2x => -1,
	      _testSrcVars => -1,
	      _testDstVars => -1,
	     };

  my $obj = bless($self, $class);
  $obj->genTestCycCnt();
  $obj->genTestHTIDW();
  $obj->genTestClk1x2x();
  $obj->resetVars();
  for (my $idx = 0; $idx < $obj->getTestCycCnt(); $idx++) {
    $obj->genSrcVar($idx);
    $obj->genDstVar($idx);
    $obj->getSrcVar($idx)->markInternalWrap();
    $obj->getDstVar($idx)->markInternalWrap();
    $obj->genCmnElemCnt($idx);
  }
  $obj->genGlbInstRW();
  $obj->genTestStaging();
  if (($obj->getTestStaging() != 0) && ($obj->getTestExStg() != 1)) {
    for (my $idx = 0; $idx < $obj->getTestCycCnt(); $idx++) {
      $obj->getSrcVar($idx)->genVarIsBlkRam();
      if ($obj->getSrcVar($idx)->getVarName() eq $obj->getDstVar($idx)->getVarName()) {
	$obj->getDstVar($idx)->setVarIsBlkRam($obj->getSrcVar($idx)->getVarIsBlkRam());
      } else {
	$obj->getDstVar($idx)->genVarIsBlkRam();
      }
    }
  } else {
    for (my $idx = 0; $idx < $obj->getTestCycCnt(); $idx++) {
      $obj->getSrcVar($idx)->setVarIsBlkRam("");
      $obj->getDstVar($idx)->setVarIsBlkRam("");
    }
  }
  return $obj;
}


################################################
# Generic Utilities
################################################

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

sub minVal {
  my $valX = shift;
  my $valY = shift;
  return ($valX < $valY) ? $valX : $valY;
}

sub maxVal {
  my $valX = shift;
  my $valY = shift;
  return ($valX >= $valY) ? $valX : $valY;
}


################################################
# Print / String Utilities
################################################

sub getTestStructDefStr {
  my ($self) = @_;
  my $rtnStr = "";
  for (my $idx = 0; $idx < $self->getTestCycCnt(); $idx++) {
    my $srcVar = $self->getSrcVar($idx);
    my $dstVar = $self->getDstVar($idx);
    $rtnStr .= $srcVar->getStructStrWrap();
    if ($srcVar->getVarTopParent()->getVarName() ne $dstVar->getVarTopParent()->getVarName()) {
      $rtnStr .= $dstVar->getStructStrWrap();
    }
  }
  return $rtnStr;
}

sub getTestWRStr {
  my ($self, $stg, $elem) = @_;
  my $rtnStr = "";
  for (my $idx = 0; $idx < $self->getTestCycCnt(); $idx++) {
    my $locStr = "";
    my $srcVar = $self->getSrcVar($idx);
    my $dstVar = $self->getDstVar($idx);
    if ($self->getTestStaging == 0) {
      $locStr = $srcVar->getWRStrWrap($stg, $dstVar);
    } else {
      if ($srcVar->getVarPSG() eq "P") {
	if ($self->getTestPrWrStg() == $stg) {
	  $locStr = $srcVar->getWRStrWrap($stg, $dstVar);
	}
      } elsif ($srcVar->getVarPSG() eq "S") {
	if ($self->getTestExStg() == $stg) {
	  $locStr = $srcVar->getWRStrWrap($stg, $dstVar);
	}
      } elsif ($srcVar->getVarPSG() eq "G") {
	if ($srcVar->getVarGlbWrStg() == 0) {
	  if ($self->getTestExStg() == $stg) {
	    $locStr = $srcVar->getWRStrWrap($stg, $dstVar);
	  }
	} elsif ($srcVar->getVarGlbWrStg() == $stg) {
	  $locStr = $srcVar->getWRStrWrap($stg, $dstVar);
	}
      }
    }

    # Manipulate in case of elem >= 1
    if ($elem >= 1) {
      if (($srcVar->getElemInfo() != -1) && ($srcVar->getElemInfo()->{ActElemCnt} > $elem)) {
	# Valid, Manip
	  $locStr = $srcVar->manipToElemStr($locStr, $elem);
      } else {
	# Invalid, Clear
	$locStr = "";
      }
    }

    # Manipulate in case of !VAR & Block Ram
    if ($srcVar->getVarTopParent()->getVarClass() ne "VAR") {
      if ($srcVar->getVarIsBlkRam() && $srcVar->getVarPSG() eq "S") {
	if ($locStr ne "") {
	  $locStr = $srcVar->manipToBRAMWRStr($locStr);
	}
      }
    }
    $rtnStr .= $locStr
  }
  return $rtnStr;
}

sub getTestWRRAStr {
  my ($self, $stg, $elem) = @_;
  my $rtnStr = "";
  if ($elem == $self->getTestWRCHKCnt()-1) {
    for (my $idx = 0; $idx < $self->getTestCycCnt(); $idx++) {
      my $srcVar = $self->getSrcVar($idx);
      if (($srcVar->getAddr1Var() != 0) && ($srcVar->getAddr1Var()->{PSG} eq "P")) {
	if ($self->getTestStaging == 0) {
	  $rtnStr .= $srcVar->getWRRA1Str($stg);
	} elsif ($self->getTestPrWrStg() == $stg) {
	  $rtnStr .= $srcVar->getWRRA1Str($stg);
	}
      }
      if (($srcVar->getAddr2Var() != 0) && ($srcVar->getAddr2Var()->{PSG} eq "P")) {
	if ($self->getTestStaging == 0) {
	  $rtnStr .= $srcVar->getWRRA2Str($stg);
	} elsif ($self->getTestPrWrStg() == $stg) {
	  $rtnStr .= $srcVar->getWRRA2Str($stg);
	}
      }
    }
  }
  return $rtnStr;
}

sub getTestSTStr {
  my ($self, $stg, $idx) = @_;
  my $srcVar = $self->getSrcVar($idx);
  my $dstVar = $self->getDstVar($idx);
  return $srcVar->getSTStr($stg, $dstVar);
}

sub getTestSTRAStr {
  my ($self, $stg, $idx) = @_;
  my $srcVar = $self->getSrcVar($idx);
  if (($srcVar->getAddr1Width() != 0) && ($srcVar->getVarPSG() eq "S")) {
    if ($srcVar->getVarIsBlkRam() eq "true") {
      if ($srcVar->getVarWrType() eq "Type") {
	if ($self->getTestExStg()-1 == $stg) {
	  return $srcVar->getReadAddrStr($stg);
	}
      }
    }
  }
}

sub getTestLDStr {
  my ($self, $stg, $idx) = @_;
  return $self->getDstVar($idx)->getLDStr($stg);
}

sub getTestLDRAStr {
  my ($self, $stg, $idx, $elem) = @_;
  my $rtnStr = "";
  my $dstVar = $self->getDstVar($idx);
  if (($dstVar->getAddr1Var() != 0) && ($dstVar->getAddr1Var()->{PSG} eq "P")) {
    if ($self->getTestStaging == 0) {
      $rtnStr .= $dstVar->getLDRA1Str($stg);
    } elsif ($self->getTestPrWrStg() == $stg) {
      $rtnStr .= $dstVar->getLDRA1Str($stg);
    }
  }
  if (($dstVar->getAddr2Var() != 0) && ($dstVar->getAddr2Var()->{PSG} eq "P")) {
    if ($self->getTestStaging == 0) {
      $rtnStr .= $dstVar->getLDRA2Str($stg);
    } elsif ($self->getTestPrWrStg() == $stg) {
      $rtnStr .= $dstVar->getLDRA2Str($stg);
    }
  }

  # Manipulate in case of elem >= 1
  if ($elem >= 1) {
    if (($dstVar->getElemInfo() != -1) && ($dstVar->getElemInfo()->{ActElemCnt} > $elem)) {
      # Valid, Manip
      $rtnStr = $dstVar->manipToRAElemStr($rtnStr, $elem);
    } else {
      # Invalid, Clear
      $rtnStr = "";
    }
  }
  return $rtnStr;
}

sub getTestCHKStr {
  my ($self, $stg, $idx, $elem) = @_;
  my $rtnStr = "";
  my $srcVar = $self->getSrcVar($idx);
  my $dstVar = $self->getDstVar($idx);
  if ($self->getTestStaging == 0) {
    $rtnStr .= $dstVar->getCHKStrWrap($srcVar, $stg);
  } else {
    if ($dstVar->getVarPSG() eq "P") {
      if ($self->getTestExStg() == $stg) {
	$rtnStr .= $dstVar->getCHKStrWrap($srcVar, $stg);
      }
    } elsif ($dstVar->getVarPSG() eq "S") {
      if ($self->getTestExStg() == $stg) {
	$rtnStr .= $dstVar->getCHKStrWrap($srcVar, $stg);
      }
    } elsif ($dstVar->getVarPSG() eq "G") {
      if ($dstVar->getVarGlbRdStg() == 0) {
	if ($self->getTestExStg() == $stg) {
	  $rtnStr .= $dstVar->getCHKStrWrap($srcVar, $stg);
	}
      } elsif ($dstVar->getVarGlbRdStg() == $stg) {
	$rtnStr .= $dstVar->getCHKStrWrap($srcVar, $stg);
      }
    }
    if (($dstVar->getAddr1Width() != 0) && ($dstVar->getVarPSG() eq "S")) {
      if ($dstVar->getVarIsBlkRam() eq "true") {
	if ($self->getTestExStg()-1 == $stg) {
	  $rtnStr .= $dstVar->getReadAddrStr($stg);
	}
      }
    }
  }

  # Manipulate in case of elem >= 1
  if ($elem >= 1) {
    if (($dstVar->getElemInfo() != -1) && ($dstVar->getElemInfo()->{ActElemCnt} > $elem)) {
      # Valid, Manip
      $rtnStr = $dstVar->manipToElemStr($rtnStr, $elem);
    } else {
      # Invalid, Clear
      $rtnStr = "";
    }
  }
  return $rtnStr;
}

################################################
# _testStr
################################################

#sub setTestStr

sub getTestStr {
  my ($self) = @_;
  return $self->{_testStr};
}


################################################
# _testCycCnt
################################################

sub genTestCycCnt {
  my ($self) = @_;
  $self->{_testCycCnt} = $ARR_TestCycCnt[rand @ARR_TestCycCnt];
  return $self->{_testCycCnt};
}

sub getTestCycCnt {
  my ($self) = @_;
  return $self->{_testCycCnt};
}


################################################
# _testHTIDW
################################################

sub genTestHTIDW {
  my ($self) = @_;
  $self->{_testHTIDW} = &genChance($PCT_TestHTIDW_GTZ, $ARR_TestHTIDW_HTIDW[rand @ARR_TestHTIDW_HTIDW], 0);
  return $self->{_testHTIDW};
}

sub getTestHTIDW {
  my ($self) = @_;
  return $self->{_testHTIDW};
}


################################################
# _testClk1x2x
################################################

sub genTestClk1x2x {
  my ($self) = @_;
  my $clkSel = &genChance($PCT_TestClk1x2x_1X, 1, 2);
  if ($clkSel == 1) {
    $clkSel = &genChance($PCT_TestClk1x2x_CDEF, 1, 0);
  }
  $self->{_testClk1x2x} = $clkSel;
  return $self->{_testClk1x2x};
}

sub getTestClk1x2x {
  my ($self) = @_;
  return $self->{_testClk1x2x};
}


################################################
# _var
################################################

sub resetVars {
  my ($self) = @_;
  undef $self->{_testSrcVars};
  undef $self->{_testDstVars};
  @{$self->{_testSrcVars}} = ();
  @{$self->{_testDstVars}} = ();
  return 1;
}

sub genSrcVar {
  my ($self, $cyc) = @_;

  my $newVar = new HTVar(
			 0,                   # Parent
			 $self->getTestStr(), # TestStr
			 $cyc,                # TestStr
			 0,                   # Depth
			 "src",               # TestSrcDst
			 0,                   # Test Object
			);
  push(@{$self->getSrcVars()}, $newVar);
  return $self->{_testSrcVars};
}

sub genDstVar {
  my ($self, $cyc) = @_;

  my $newVar = new HTVar(
			 0,                      # Parent
			 $self->getTestStr(),    # TestStr
			 $cyc,                   # TestStr
			 0,                      # Depth
			 "dst",                  # TestSrcDst
			 $self,                  # Test Object
			);
  push(@{$self->getDstVars()}, $newVar);
  return $self->{_testDstVars};
}

sub setSrcVar {
  my ($self, $cyc, $srcVar) = @_;
  $self->{_testSrcVars}->[$cyc] = $srcVar;
  return $self->{_testSrcVars}->[$cyc];
}

sub getSrcVar {
  my ($self, $cyc) = @_;
  return $self->{_testSrcVars}->[$cyc];
}

sub getSrcVars {
  my ($self) = @_;
  return $self->{_testSrcVars};
}

sub getDstVar {
  my ($self, $cyc) = @_;
  return $self->{_testDstVars}->[$cyc];
}

sub getDstVars {
  my ($self) = @_;
  return $self->{_testDstVars};
}

sub getVarsByPSG {
  my ($self, $psg) = @_;
  my @vars = ();
  for (my $idx = 0; $idx < $self->getTestCycCnt; $idx++) {
    my $srcVar = $self->getSrcVar($idx);
    my $srcTopVar = $srcVar->getVarTopParent();
    my $dstVar = $self->getDstVar($idx);
    my $dstTopVar = $dstVar->getVarTopParent();

    if ($srcVar->getVarPSG() eq $psg) {
      push(@vars, $srcVar);
    }

    if (($dstVar->getVarPSG() eq $psg) && ($srcTopVar->getVarName() ne $dstTopVar->getVarName())) {
      push(@vars, $dstVar);
    }
  }
  return \@vars;
}

sub getRAVarsByPSG {
  my ($self, $psg) = @_;
  my @vars = ();
  for (my $idx = 0; $idx < $self->getTestCycCnt; $idx++) {
    my $srcVar = $self->getSrcVar($idx);
    my $srcAddr1Var = $srcVar->getAddr1Var();
    my $srcAddr2Var = $srcVar->getAddr2Var();
    my $dstVar = $self->getDstVar($idx);
    my $dstAddr1Var = $dstVar->getAddr1Var();
    my $dstAddr2Var = $dstVar->getAddr2Var();

    if (($srcAddr1Var != 0) && ($srcAddr1Var->{PSG} eq $psg)) {
      push(@vars, $srcAddr1Var);
    }

    if (($srcAddr2Var != 0) && ($srcAddr2Var->{PSG} eq $psg)) {
      push(@vars, $srcAddr2Var);
    }

    if (($dstAddr1Var != 0) && ($dstAddr1Var->{PSG} eq $psg)) {
      if (($srcAddr1Var == 0) || ($srcAddr1Var->{Name} ne $dstAddr1Var->{Name})) {
	push(@vars, $dstAddr1Var);
      }
    }

    if (($dstAddr2Var != 0) && ($dstAddr2Var->{PSG} eq $psg)) {
      if (($srcAddr2Var == 0) || ($srcAddr2Var->{Name} ne $dstAddr2Var->{Name})) {
	push(@vars, $dstAddr2Var);
      }
    }
  }
  return \@vars;
}


################################################
# _testCmnECts
################################################

sub genCmnElemCnt {
  my ($self, $idx) = @_;
  my $srcVar = $self->getSrcVar($idx);
  my $dstVar = $self->getDstVar($idx);
  my %tmpHash = (
		 "SRC" => {
			   "Addr1"   => $srcVar->getAddr1Sel(),
			   "Addr2"   => $srcVar->getAddr2Sel(),
			   "Dim1"    => $srcVar->getDim1Sel(),
			   "Dim2"    => $srcVar->getDim2Sel(),
			   "ActElemCnt" => 1,
			   "TotElemCnt" => 1
			  },
		 "DST" => {
			   "Addr1"   => $dstVar->getAddr1Sel(),
			   "Addr2"   => $dstVar->getAddr2Sel(),
			   "Dim1"    => $dstVar->getDim1Sel(),
			   "Dim2"    => $dstVar->getDim2Sel(),
			   "ActElemCnt" => 1,
			   "TotElemCnt" => 1
			  }
		);

  # Get Src Parameters
  my $srcSpcToS = -1;
  my $srcSpcToE = -1;
  $srcVar->getSpaceAvl(\$srcSpcToS, \$srcSpcToE);

  # Get Dst Parameters
  my $dstSpcToS = -1;
  my $dstSpcToE = -1;
  $dstVar->getSpaceAvl(\$dstSpcToS, \$dstSpcToE);

  unless ($srcSpcToS == -1 ||
	  $srcSpcToE == -1 ||
	  $dstSpcToS == -1 ||
	  $dstSpcToE == -1) {
    if (&genChance($PCT_CmnElemCnt_GEN, 1, 0)) {
      my $cntAvl;
      if ($srcVar->getVarClass() ne "VAR") {
	# STR/UNN
	# Max Count for STR/UNN = 8! FIXME
	# Avoiding hassle of finding exact alignment of huge struct
	# Just cap at QW_MAX and reduce if necessary
	$cntAvl = $CNT_MaxQWPerCyc_MAX;
	my $tempSize = $srcVar->getBaseVarSize();
	while ($tempSize*$cntAvl > $CNT_MaxQWPerCyc_MAX*64) {
	  $cntAvl--;
	}
      } else {
	# VAR
	$cntAvl = int($CNT_MaxQWPerCyc_MAX*64 / $srcVar->getBaseVarSize());
      }

      my $startRelPos = int(rand(&minVal($srcSpcToS+1, $dstSpcToS+1)));
      $startRelPos = $cntAvl if ($startRelPos > $cntAvl);

      my $elemCnt = &genChance($PCT_CmnElemCnt_MUL, $ARR_CmnElemCnt_CNT[rand @ARR_CmnElemCnt_CNT], 1);
      if ($elemCnt > &minVal($srcSpcToE+1, $dstSpcToE+1)) {
	$elemCnt = &minVal($srcSpcToE+1, $dstSpcToE+1);
      }
      if ($elemCnt > 1) {
	$srcSpcToE -= $elemCnt-1;
	$dstSpcToE -= $elemCnt-1;
      }

      my $srcEndRelPos = int(rand(&minVal($srcSpcToE+1, $cntAvl-$startRelPos-$elemCnt)));
      my $dstEndRelPos = int(rand(&minVal($dstSpcToE+1, $cntAvl-$startRelPos-$elemCnt)));

      $srcEndRelPos = 0 if ($srcEndRelPos < 0);
      $dstEndRelPos = 0 if ($dstEndRelPos < 0);

      $srcVar->getSpaceOffsets(\%{$tmpHash{SRC}}, $startRelPos, $srcEndRelPos, $elemCnt);
      $dstVar->getSpaceOffsets(\%{$tmpHash{DST}}, $startRelPos, $dstEndRelPos, $elemCnt);

      $tmpHash{SRC}{ActElemCnt} = $elemCnt;
      $tmpHash{DST}{ActElemCnt} = $elemCnt;

      # MUST make src WrType = VAR (otherwise elemcnt won't be used!!!)
      $srcVar->setVarWrType("Var");
    }
  }

  # Push if it makes sense...
  if ($srcSpcToS != -1 || $srcSpcToE != -1) {
    $srcVar->setElemInfo(\%{$tmpHash{SRC}});
  }
  if ($dstSpcToS != -1 || $dstSpcToE != -1) {
    $dstVar->setElemInfo(\%{$tmpHash{DST}});
  }

  $self->setTestWRCHKCnt(&maxVal($self->getTestWRCHKCnt(), $tmpHash{SRC}{ActElemCnt}));

  return;
}


################################################
# Global Instr Read/Write
################################################
sub genGlbInstRW {
  my ($self) = @_;

  # Loop over all cycles
  for (my $idx = 0; $idx < $self->getTestCycCnt(); $idx++) {
    my $srcVar = $self->getSrcVar($idx);
    my $dstVar = $self->getDstVar($idx);

    # Basic Requirements...
    if ($srcVar->getVarPSG() eq "G") {
      # Write
      $srcVar->setVarGlbInstW(1);

      # Read
      if ($srcVar->getVarWrType() eq "Type") {
	$srcVar->setVarGlbInstR(1);
      }
    }

    if ($dstVar->getVarPSG() eq "G") {
      # Write

      # Read
      $dstVar->setVarGlbInstR(1);
    }

    if ($srcVar->getVarTopParent()->getVarName() eq $dstVar->getVarTopParent()->getVarName()) {
      my $srcVarW = $srcVar->getVarGlbInstW();
      my $srcVarR = $srcVar->getVarGlbInstR();
      my $dstVarW = $dstVar->getVarGlbInstW();
      my $dstVarR = $dstVar->getVarGlbInstR();

      if (($srcVarW == 1) || ($dstVarW == 1)) {
	$srcVarW = 1;
	$dstVarW = 1;
      } elsif (($srcVarW == 0) || ($dstVarW == 0)) {
	$srcVarW = 0;
	$dstVarW = 0;
      } else {
	$srcVarW = -1;
	$dstVarW = -1;
      }

      if (($srcVarR == 1) || ($dstVarR == 1)) {
	$srcVarR = 1;
	$dstVarR = 1;
      } elsif (($srcVarR == 0) || ($dstVarR == 0)) {
	$srcVarR = 0;
	$dstVarR = 0;
      } else {
	$srcVarR = -1;
	$dstVarR = -1;
      }

      $srcVar->setVarGlbInstW($srcVarW);
      $srcVar->setVarGlbInstR($srcVarR);
      $dstVar->setVarGlbInstW($dstVarW);
      $dstVar->setVarGlbInstR($dstVarR);
    }

    # Randomize the rest...
    if ($srcVar->getVarPSG() eq "G") {
      # Write
      if ($srcVar->getVarGlbInstW() == -1) {
	if (&genChance($PCT_InstrRW_DEF, 1, 0)) {
	  $srcVar->setVarGlbInstW(&genChance($PCT_InstrRW_TRUE, 1, 0));
	}
      }

      # Read
      if ($srcVar->getVarGlbInstR() == -1) {
	if (&genChance($PCT_InstrRW_DEF, 1, 0)) {
	  $srcVar->setVarGlbInstR(&genChance($PCT_InstrRW_TRUE, 1, 0));
	}
      }
    }

    if ($dstVar->getVarPSG() eq "G") {
      # Write
      if ($dstVar->getVarGlbInstW() == -1) {
	if (&genChance($PCT_InstrRW_DEF, 1, 0)) {
	  $dstVar->setVarGlbInstW(&genChance($PCT_InstrRW_TRUE, 1, 0));
	}
      }

      # Read
      if ($dstVar->getVarGlbInstR() == -1) {
	if (&genChance($PCT_InstrRW_DEF, 1, 0)) {
	  $dstVar->setVarGlbInstR(&genChance($PCT_InstrRW_TRUE, 1, 0));
	}
      }
    }
  }

  return;
}


################################################
# _testStaging
################################################

sub genTestStaging {
  my ($self) = @_;
  $self->{_testStaging} = &genChance($PCT_TestStaging_STG, 1, 0);
  $self->genTestPrWrStg();
  $self->genTestExStg();
  $self->setTestPrWrStg($self->getTestExStg()) if (&genChance($PCT_TestStaging_SAME_PE, 1, 0) && $self->getTestExStg() != 1); #FIXME UGH
  $self->{_testStaging} = &maxVal($self->getTestExStg(), $self->getTestPrWrStg());
  for (my $idx = 0; $idx < $self->getTestCycCnt(); $idx++) {
    if (($self->getSrcVar($idx)->getVarPSG() eq "G") && ($self->getDstVar($idx)->getVarPSG() ne "G")) {
      $self->genTestSrcGStg($idx);
    }

    elsif (($self->getSrcVar($idx)->getVarPSG() ne "G") && ($self->getDstVar($idx)->getVarPSG() eq "G")) {
      $self->genTestDstGStg($idx);
    }

    elsif (($self->getSrcVar($idx)->getVarPSG() eq "G") && ($self->getDstVar($idx)->getVarPSG() eq "G")) {
      $self->genTestSrcGStg($idx);
      if ($self->getSrcVar($idx)->getVarTopParent()->getVarName() eq $self->getDstVar($idx)->getVarTopParent()->getVarName()) {
	$self->getDstVar($idx)->setVarGlbRdStg($self->getSrcVar($idx)->getVarGlbRdStg());
	$self->getDstVar($idx)->setVarGlbWrStg($self->getSrcVar($idx)->getVarGlbWrStg());
      } else {
	$self->genTestDstGStg($idx);
      }
    }
  }
  return $self->{_testStaging};
}

sub getTestStaging {
  my ($self) = @_;
  return $self->{_testStaging};
}

sub getStgList {
  my ($self) = @_;
  my %tempStg = ();
  $tempStg{$self->getTestExStg()} = 1;
  $tempStg{$self->getTestPrWrStg()} = 1;
  for (my $idx = 0; $idx < $self->getTestCycCnt; $idx++) {
    $tempStg{$self->getSrcVar($idx)->getVarGlbWrStg()} = 1 if ($self->getSrcVar($idx)->getVarGlbWrStg() != -1);
    $tempStg{$self->getDstVar($idx)->getVarGlbRdStg()} = 1 if ($self->getDstVar($idx)->getVarGlbRdStg() != -1);
    $tempStg{$self->getTestExStg()-1} = 1 if ($self->getSrcVar($idx)->getVarIsBlkRam() eq "true");
    $tempStg{$self->getTestExStg()-1} = 1 if ($self->getDstVar($idx)->getVarIsBlkRam() eq "true");
  }
  if (scalar keys %tempStg > 1) {
    delete $tempStg{0};
  }
  my @stgs = ();
  foreach my $stg (sort keys %tempStg) {
    push(@stgs, $stg);
  }
  return \@stgs;
}


################################################
# _testExStg
################################################

sub genTestExStg {
  my ($self) = @_;
  if ($self->getTestStaging() != 0) {
    my @vldStgs = (1..$self->getTestPrWrStg());
    $self->{_testExStg} = $vldStgs[rand @vldStgs];
  } else {
    $self->{_testExStg} = 0;
  }
  return $self->{_testExStg};
}

sub getTestExStg {
  my ($self) = @_;
  return $self->{_testExStg};
}


################################################
# _testPrWrStg
################################################

sub setTestPrWrStg {
  my ($self, $stg) = @_;
  $self->{_testPrWrStg} = $stg;
  return $self->{_testPrWrStg};
}

sub genTestPrWrStg {
  my ($self) = @_;
  my @vldStgs = @ARR_TestStaging_STG;
  $self->{_testPrWrStg} = ($self->getTestStaging() == 0) ? 0 : $vldStgs[rand @vldStgs];
  return $self->{_testPrWrStg};
}

sub getTestPrWrStg {
  my ($self) = @_;
  return $self->{_testPrWrStg};
}


################################################
# _testWRCHKCnt
################################################

sub setTestWRCHKCnt {
  my ($self, $val) = @_;
  $self->{_testWRCHKCnt} = $val;
  return $self->{_testWRCHKCnt};
}

sub getTestWRCHKCnt {
  my ($self) = @_;
  return $self->{_testWRCHKCnt};
}


################################################
# _testSrcGStg
################################################

sub genTestSrcGStg {
  my ($self, $idx) = @_;
  my $srcVar = $self->getSrcVar($idx);
  my $srcGRStg = 0;
  my $srcGWStg = 0;
  if ($self->getTestStaging() != 0) {
    if (&genChance($PCT_TestStaging_GDEF, 1, 0)) {
      my @vldSrcGRStg = (1..$self->getTestExStg());
      my @vldSrcGWStg = ($self->getTestExStg()..$self->getTestStaging());
      $srcGRStg = $vldSrcGRStg[rand @vldSrcGRStg];
      $srcGWStg = $vldSrcGWStg[rand @vldSrcGWStg];
      if (&genChance($PCT_TestStaging_GSAME, 1, 0)) {
	$srcGRStg = $self->getTestExStg();
	$srcGWStg = $self->getTestExStg();
      }
    }
  }
  $srcVar->setVarGlbRdStg($srcGRStg);
  $srcVar->setVarGlbWrStg($srcGWStg);
  return;
}


################################################
# _testDstGStg
################################################

sub genTestDstGStg {
  my ($self, $idx) = @_;
  my $dstVar = $self->getDstVar($idx);
  my $dstGRStg = 0;
  my $dstGWStg = 0;
  if ($self->getTestStaging() != 0) {
    if (&genChance($PCT_TestStaging_GDEF, 1, 0)) {
      my @vldDstGRStg = (1..$self->getTestExStg());
      my @vldDstGWStg = ($self->getTestExStg()..$self->getTestStaging());
      $dstGRStg = $vldDstGRStg[rand @vldDstGRStg];
      $dstGWStg = $vldDstGWStg[rand @vldDstGWStg];
      if (&genChance($PCT_TestStaging_GSAME, 1, 0)) {
	$dstGRStg = $self->getTestExStg();
	$dstGWStg = $self->getTestExStg();
      }
    }
  }
  $dstVar->setVarGlbRdStg($dstGRStg);
  $dstVar->setVarGlbWrStg($dstGWStg);
  return;
}

# Why do .pm files need this, I'll never know
1;
