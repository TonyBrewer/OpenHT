#!/usr/bin/perl -w

use warnings;
use strict;

use Clone;

################################################
# Globals
################################################
my @GLB_STDVarTypes = (
		       {
			'uintX_t' => {
				      'Values' => [8,16,32,64]
				     }
		       },
		       {
			'intX_t' => {
				     'Values' => [8,16,32,64]
				    }
		       }
		      );
my @GLB_CSTVarTypes = (
		       {
			'ht_uintX' => {
				       'Values' => [1..64]
				      }
		       },
		       {
			'ht_intX' => {
				      'Values' => [1..64]
				     }
		       },
#		       {
#			'sc_uint<X>' => {
#					 'Values' => [1..64]
#					}
#		       },
#		       {
#			'sc_int<X>' => {
#					'Values' => [1..64]
#				       }
#		       }
		      );


################################################
# Global Chances / Switches
################################################
my $PCT_VarPSG_P = 0.33;
my $PCT_VarPSG_S = 0.33;
my $PCT_VarPSG_G = 0.34;

my $PCT_VarClass_VAR = 0.70;
my $PCT_VarClass_STR = 0.20;
my $PCT_VarClass_UNN = 0.10;

my $PCT_VarAnon_ANON = 0.50;

my $PCT_VarWrType_VAR = 0.50;

my $ENB_RtnVar_STR = 1;
my $ENB_RtnVar_UNN = 1;
my $PCT_GetVarFromList_FROMSRC = 0.25;

my $PCT_BitField_STRBF = 0.20;
my $PCT_BitField_TOTBF = 0.50;
my $PCT_BitField_VAR = 0.50;

my $PCT_ChkStr_CHKALLUNN = 1.00;

my $CNT_VarFields_MAX = 4;
my $CNT_VarDepth_MAX  = 4;

my $CNT_MaxQWPerCyc_MAX = 8;

my $PCT_VarRdSfx_R = 0.50;
my $PCT_VarWrSfx_W = 0.50;

my $PCT_ModVarDimAddr_MOD = 0.50;

my $PCT_IsBlkRam_DEF = 0.50;
my $PCT_IsBlkRam_BRAM = 0.50;

my @ARR_VarDimSel      = (1..5);
my $PCT_VarDimSel_DIM  = 0.50;
my $PCT_VarDimDel_HTD = 0.50;
my $PCT_VarDimDSel_HTD = 0.50;

my @ARR_VarAddrWidth    = (1..5);
my $PCT_VarAddrSel_ADDR = 0.50;
my $PCT_VarAddrDSel_HTD = 0.50;

my $PCT_GenVarIdx_ADR = 0.50;
my $PCT_GenVarIdx_A1  = 0.50;
my $PCT_GenVarIdx_A2  = 0.50;
my $PCT_GenVarIdx_D1  = 0.50;
my $PCT_GenVarIdx_D2  = 0.50;

################################################
# Static Variables
################################################
my $STATIC_VarUniq = 0;
my $STATIC_StrUniq = 0;
my $STATIC_UnnUniq = 0;


package HTVar;

# Initialize HTVar Object
sub new {
  my $class = shift;

  # Args
  my $parent     = shift;
  my $testStr    = shift;
  my $testCyc    = shift;
  my $varDepth   = shift;
  my $testSrcDst = shift;
  my $testRef    = shift;

  # Statics
  my $STATIC_VarList;
  if ($parent == 0) {
    %{$STATIC_VarList} = ();
    shift;
  } else {
    $STATIC_VarList = shift;
  }


  my $self = {
	      # Basic Variable Attributes
	      _varClass     => "bad_class",
	      _varName      => "bad_name",
	      _varBaseSize  => -1,
	      _varActSize   => -1,
	      _varType      => "bad_type",
	      _varPSG       => -1,
	      _varValue     => -1,
	      _varWrType    => -1,
	      _varUsedField => -1,
	      _varBitField  => -1,
	      _varBitFType  => "",

	      # Struct / Union Attributes
	      _varDepth   => $varDepth,
	      _varAnon    => 0,

	      # Test Attributes
	      _testStr    => $testStr,
	      _testCyc    => $testCyc,
	      _testSrcDst => $testSrcDst,
	      _uniqStr    => "bad_uniq",

	      # Dimension Modifiers
	      _varDim1Size => -1,
	      _varDim1Sel  => -1,
	      _varDim1DSel => -1,
	      _varDim2Size => -1,
	      _varDim2Sel  => -1,
	      _varDim2DSel => -1,
	      _varIdxDim1  => 0,
	      _varIdxDim2  => 0,

	      # Address Modifiers
	      _varAddr1Width => -1,
	      _varAddr1Sel   => -1,
	      _varAddr1DSel  => -1,
	      _varAddr2Width => -1,
	      _varAddr2Sel   => -1,
	      _varAddr2DSel  => -1,
	      _varIdxAddr1   => 0,
	      _varIdxAddr2   => 0,

	      # Element Count Modifiers
	      _varElemInfo => -1,

	      # BRAM/DRAM
	      _varIsBlkRam => -1,

	      # Other Notes
	      _varContUNN => 0,

	      # Staging Modifiers
	      _varGlbRdStg => -1,
	      _varGlbWrStg => -1,

	      # Global WR Markers
	      _varGlbInstW => -1,
	      _varGlbInstR => -1,

	      # Parent / Children
	      _varParent => $parent,
	      _varFields => -1,

	      # Statics
	      _zS_VAR_UNIQ => \$STATIC_VarUniq,
	      _zS_STR_UNIQ => \$STATIC_StrUniq,
	      _zS_UNN_UNIQ => \$STATIC_UnnUniq,
	      _zS_VAR_LIST => $STATIC_VarList,
	     };

  my $obj = bless($self, $class);
  my $retObj = $obj;
  $obj->genVarClass();
  $obj->genVarName();
  $obj->genVarType();
  $obj->genVarPSG() if ($obj->getVarParent() == 0);
  $obj->genVarDim1();
  $obj->genVarDim2();
  $obj->genVarAddr1() if ($obj->getVarParent() == 0);
  $obj->genVarAddr2() if ($obj->getVarParent() == 0);
  $obj->genVarBitField();
  $obj->genVarBitFType();
  $obj->genVarValue();
  $obj->genVarWrType();
  $obj->setVarUsedField(0);
  $obj->genVarFields();
  $obj->pushToList();

  if ($obj->getVarParent() == 0) {
    $retObj = $obj->getVarFromList($testRef);
    $retObj->genVarIdx();
    unless ($testRef == 0) {
      if (&genChance($PCT_ModVarDimAddr_MOD, 1, 0)) {
	$retObj->modVarDimAddrWrap();
      }
    }
    $obj->resetUniq();
  }

  return $retObj;
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

sub genChance3 {
  my $pct1 = shift;
  my $pct2 = shift;
  my $pct3 = shift;
  my $val1 = shift;
  my $val2 = shift;
  my $val3 = shift;

  my $rand = int(rand(100))+1;

  if ((0 < $rand) && ($rand <= $pct1*100)) {
    return $val1;
  } elsif (($pct1*100 < $rand) && ($rand <= ($pct1+$pct2)*100)) {
    return $val2;
  } elsif ((($pct1+$pct2)*100 < $rand) && ($rand <= 100)) {
    return $val3;
  } else {
    die "ERROR in genChance3 $!";
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
# Object Utilities
################################################

sub getVarChain {
  my ($self) = @_;
  my @chainArr = ();
  while ($self->getVarParent() != 0) {
    push(@chainArr, $self) if ($self->getVarAnon() == 0);
    $self = $self->getVarParent();
  }
  push(@chainArr, $self);
  @chainArr = reverse @chainArr;
  return \@chainArr;
}

sub findVarByName {
  my ($self, $name) = @_;
  if ($self->getVarName eq $name) {
    return $self;
  } else {
    if ($self->getVarClass() eq "VAR") {
      return 0;
    } else {
      my $ret = 0;
      foreach my $field (@{$self->getVarFields()}) {
	my $temp = $field->findVarByName($name);
	$ret = $temp if ($temp != 0);
      }
      return $ret;
    }
  }
}

sub markInternalWrap {
  my ($self) = @_;
  if ($self->getVarClass() ne "VAR") {
    foreach my $field (@{$self->getVarFields()}) {
      $field->markInternal();
    }
  }
  return;
}

sub markInternal {
  my ($self) = @_;
  $self->setDim1Sel("X") if ($self->getDim1Size() != 0);
  $self->setDim2Sel("X") if ($self->getDim2Size() != 0);
  if ($self->getVarClass() ne "VAR") {
    foreach my $field (@{$self->getVarFields()}) {
      $field->markInternal();
    }
  }
  return;
}

sub unmarkUsedFields {
  my ($self) = @_;
  $self->setVarUsedField(0);
  if ($self->getVarClass() ne "VAR") {
    foreach my $field (@{$self->getVarFields()}) {
      if ($field->getVarUsedField() == 1) {
	$field->unmarkUsedFields();
      }
    }
  }
  return;
}

sub markUsedFields {
  my ($self) = @_;
  $self->setVarUsedField(1);
  if ($self->getVarClass() eq "STR") {
    foreach my $field (@{$self->getVarFields()}) {
      $field->markUsedFields();
    }
  } elsif ($self->getVarClass() eq "UNN") {
    my $field = $self->getVarFields()->[rand @{$self->getVarFields()}];
    $field->markUsedFields();
  }
  return;
}

sub getWrList {
  my ($self, $arrRef) = @_;
  if ($self->getVarClass() eq "VAR") {
    if ($self->getVarUsedField() == 1) {
      push(@$arrRef, $self);
    }
  } else {
    foreach my $field (@{$self->getVarFields()}) {
      $field->getWrList($arrRef);
    }
  }
  return;
}

sub getExpMem {
  my ($self, $srcVar, $memRef) = @_;

  my @wrList = ();
  $self->getWrList(\@wrList);
  $memRef->{TotSize} = 0;
  $memRef->{DataIdx} = 0;
  $memRef->{SizeIdx} = 0;

  my $elemCnt = 1;
  if ($self->getElemInfo() != -1) {
    $elemCnt = $self->getElemInfo()->{ActElemCnt};
  }

#  if ($elemCnt > 1) {
#    print "UNN CHK / ELEM > 1 -> ".$self->getChainName()."\n";
#  }
#	if ($self->getVarAlignment() < $self->getVarParent()->getVarFields()->[$curIdx+1]->getVarAlignment()) {
#	  $discardSize = $self->getVarParent()->getVarFields()->[$curIdx+1]->getVarAlignment() - $self->getVarAlignment();
#	}

  for (my $elem = 0; $elem < $elemCnt; $elem++) {
    for (my $idx = 0; $idx <= $#wrList; $idx++) {
      my $field = $wrList[$idx];

      my $mVar = $srcVar->findVarByName($field->getVarName());
      if ($mVar == 0) {
	$mVar = $srcVar; # FIXME? Catch all old regular cases
      }
      my $tempData = $mVar->getVarValue();
      my $tempWidth = 0;
      if ($mVar->getBFVarType() =~ /(\d+)/) {
	$tempWidth = $1;
      }
      my $tempMask = 0;
      for (my $i = 0; $i < $tempWidth; $i++) {
	$tempMask <<= 1;
	$tempMask |= 1;
      }
      $tempData &= $tempMask;

      my $numDims = 1;
      if ($field->getDim1Size() != 0) {
	$numDims = $field->getDim1Size();
	if ($field->getDim2Size() != 0) {
	  $numDims *= $field->getDim2Size();
	}
      }
      for (my $idx = 0; $idx < $numDims; $idx++) {
	push(@{$memRef->{DataArr}}, $tempData);
	push(@{$memRef->{SizeArr}}, $tempWidth);
	$memRef->{TotSize} += $tempWidth;
      }

      if ($field->getVarParent()->getVarClass() eq "STR") {
	if (($idx != $#wrList) && ($field->getVarParent()->getVarName() eq $wrList[$idx+1]->getVarParent()->getVarName())) {
	  my $curOffset = $memRef->{TotSize};
	  while ($curOffset > 64) {
	    $curOffset -= 64;
	  }

	  # Next Field has Larger Alignment!
	  if ($curOffset < $wrList[$idx+1]->getVarAlignment()) {
	    my $discardSize = $wrList[$idx+1]->getVarAlignment() - $curOffset;
	    push(@{$memRef->{DataArr}}, 0);
	    push(@{$memRef->{SizeArr}}, $discardSize);
	    $memRef->{TotSize} += $discardSize;
	  }
	} else {
	  # MIGHT NEED THE OTHER CASE HERE! UNSURE FIXME
	}
      }
    }
  }
  return;
}

sub hasExpMemData {
  my ($self, $memRef) = @_;
  if ($memRef->{DataIdx} <= $#{$memRef->{DataArr}}) {
    return 1;
  } else {
    return 0;
  }
}

sub getExpMemAlignment {
  my ($self, $memRef) = @_;

  my $curOff = 0;
  for (my $idx = 0; $idx < $memRef->{DataIdx}; $idx++) {
    $curOff += $memRef->{SizeArr}[$idx];
  }
  $curOff += $memRef->{SizeIdx};

  while ($curOff > 64) {
    $curOff -= 64;
  }

  return $curOff;
}

sub isLastIdx {
  my ($self, $d1, $d2) = @_;

  my $d1Size = $self->getDim1Size();
  my $d2Size = $self->getDim2Size();

  if (($d1Size != 0) && ($d2Size != 0)) {
    if (($d1 == $d1Size-1) && ($d2 == $d2Size-1)) {
      return 1;
    }
  }

  elsif ($d1Size != 0) {
    if ($d1 == $d1Size-1) {
      return 1;
    }
  }

  return 0;
}

sub getExpMemData {
  my ($self, $size, $memRef, $typeRef, $valRef) = @_;

  if ($memRef->{DataIdx} > $#{$memRef->{DataArr}}) {
    die "ERROR: no space in data: $!";
  }

  my $startDataIdx = $memRef->{DataIdx};
  my $startSizeIdx = $memRef->{SizeIdx};
  my $curDataIdx = $memRef->{DataIdx};
  my $curSizeIdx = $memRef->{SizeIdx};

  while ($size != 0) {
    if ($curDataIdx > $#{$memRef->{DataArr}}) {
      # Not enough space for a full compare...
      # Kill here and let the data grabber get a partial amount
      $size = 0;
    } else {
      if ($size >= $memRef->{SizeArr}->[$curDataIdx]-$curSizeIdx) {
	$size -= ($memRef->{SizeArr}->[$curDataIdx]-$curSizeIdx);
	$curDataIdx++;
	$curSizeIdx = 0;
      } else {
	$curSizeIdx += $size;
	$size = 0;
      }
    }
  }

  my $retSize = 0;
  $self->expMemGrabber($memRef, $startDataIdx, $startSizeIdx, $curDataIdx, $curSizeIdx, \$retSize, $valRef);
  $memRef->{DataIdx} = $curDataIdx;
  $memRef->{SizeIdx} = $curSizeIdx;

  $$typeRef = sprintf("ht_%sint%d", (($self->getVarType() =~ /uint/) ? "u" : ""), $retSize);

  return;
}

sub expMemGrabber {
  my ($self, $memRef, $cDIdx, $cSIdx, $eDIdx, $eSIdx, $sizeRef, $valRef) = @_;
  my $rtnSize = 0;
  my $rtnVal = 0;
  while (($eDIdx != $cDIdx) || ($eSIdx != $cSIdx)) {
    if ($eDIdx == $cDIdx) {
      my $sizeLeft = $eSIdx - $cSIdx;

      my $tempMask = 0;
      for (my $i = 0; $i < $sizeLeft; $i++) {
	$tempMask <<= 1;
	$tempMask |= 1;
      }
      my $tempData = $memRef->{DataArr}->[$cDIdx];
      $tempData >>= $cSIdx;
      $tempData &= $tempMask;

      $rtnVal |= ($tempData << $rtnSize);

      $rtnSize += $sizeLeft;
      $cSIdx = $eSIdx;

    } else {

      my $sizeLeft = $memRef->{SizeArr}->[$cDIdx] - $cSIdx;

      my $tempMask = 0;
      for (my $i = 0; $i < $sizeLeft; $i++) {
	$tempMask <<= 1;
	$tempMask |= 1;
      }
      my $tempData = $memRef->{DataArr}->[$cDIdx];
      $tempData >>= $cSIdx;
      $tempData &= $tempMask;

      $rtnVal |= ($tempData << $rtnSize);

      $rtnSize += $sizeLeft;
      $cSIdx = 0;
      $cDIdx++;

    }
  }

  $$sizeRef = $rtnSize;
  $$valRef = $rtnVal;

  return;
}

sub modVarDimAddrWrap {
  my ($self) = @_;
  $self->getVarTopParent()->genVarAddr1Sel();
  $self->getVarTopParent()->genVarAddr2Sel();
  $self->modVarDimAddr();
  return;
}

sub modVarDimAddr {
  my ($self) = @_;
  $self->genVarDim1Sel();
  $self->genVarDim2Sel();
  if ($self->getVarClass() ne "VAR") {
    foreach my $field (@{$self->getVarFields()}) {
      $field->modVarDimAddr();
    }
  }
  return;
}

sub getSpaceAvl {
  my ($self, $STSRef, $STERef) = @_;
  $$STSRef = -1;
  $$STERef = -1;

  if (($self->getIsIdxAddr1() != 0) &&
      ($self->getIsIdxAddr2() != 0) &&
      ($self->getAddr1Width() != 0) &&
      ($self->getAddr2Width() != 0)) {
      # Addr1 & Addr2
      $$STSRef = $self->getAddr2Sel();
      $$STSRef += (2**$self->getAddr2Width())*$self->getAddr1Sel();
      $$STERef = ((2**$self->getAddr2Width())*(2**$self->getAddr1Width())) - $$STSRef - 1;
      $self->setAddr1DSel("SRC");
      $self->setAddr2DSel("SRC");
  }

  elsif (($self->getIsIdxAddr1() == 0) &&
	 ($self->getIsIdxAddr2() != 0) &&
	 ($self->getAddr2Width() != 0)) {
      # !Addr1 & Addr2
      $$STSRef = $self->getAddr2Sel();
      $$STERef = (2**$self->getAddr2Width()) - $$STSRef - 1;
      $self->setAddr2DSel("SRC");
  }

  elsif (($self->getIsIdxAddr1() != 0) &&
	 ($self->getAddr1Width() != 0)) {
      # Addr1
      $$STSRef = $self->getAddr1Sel();
      $$STERef = (2**$self->getAddr1Width()) - $$STSRef - 1;
      $self->setAddr1DSel("SRC");
  }

  elsif (($self->getIsIdxDim1() != 0) &&
	 ($self->getIsIdxDim2() != 0) &&
	 ($self->getDim1Size() != 0) &&
	 ($self->getDim2Size() != 0)) {
      # Dim1 & Dim2
      $$STSRef = $self->getDim2Sel();
      $$STSRef += $self->getDim2Size()*$self->getDim1Sel();
      $$STERef = ($self->getDim2Size()*$self->getDim1Size()) - $$STSRef - 1;
      $self->setDim1DSel("SRC");
      $self->setDim2DSel("SRC");
  }

  elsif (($self->getIsIdxDim1() == 0) &&
	 ($self->getIsIdxDim2() != 0) &&
	 ($self->getDim2Size() != 0)) {
      # !Dim1 & Dim2
      $$STSRef = $self->getDim2Sel();
      $$STERef = $self->getDim2Size() - $$STSRef - 1;
      $self->setDim2DSel("SRC");
  }

  elsif (($self->getIsIdxDim1() != 0) &&
	 ($self->getDim1Size() != 0)) {
      # Dim1
      $$STSRef = $self->getDim1Sel();
      $$STERef = $self->getDim1Size() - $$STSRef - 1;
      $self->setDim1DSel("SRC");
  }

  return;
}

sub getSpaceOffsets {
  my ($self, $hashRef, $startRelPos, $endRelPos, $actElemCnt) = @_;
  my $saveStartPos = $startRelPos;

  # Check for Addressing
  if ((($self->getIsIdxAddr1() != 0) || ($self->getIsIdxAddr2() != 0)) && ($self->getVarParent == 0)) { # Parent check needed!
    my $addr1 = -1;
    my $addr2 = -1;
    my $cnt = -1;

    if ($self->getAddr2Width() != 0) {
      $addr1 = $self->getAddr1Sel();
      $addr2 = $self->getAddr2Sel();
      # Addr1 & Addr2
      while ($startRelPos != 0) {
	if ($addr2 != 0) {
	  # Take what we can from Addr2
	  my $spaceToTake = &minVal($addr2, $startRelPos);
	  $addr2 -= $spaceToTake;
	  $startRelPos -= $spaceToTake;
	}
	if ($startRelPos != 0 && $addr2 == 0) {
	  #Reset Addr2, Decr Addr1
	  $addr1--;
	  $addr2 = 2**$self->getAddr2Width()-1;
	  $startRelPos--;
	}
      }
    } else {
      $addr1 = $self->getAddr1Sel();
      # Only Addr1
      $addr1 -= $startRelPos;
      $startRelPos = 0;
    }

    $hashRef->{Addr1} = $addr1;
    $hashRef->{Addr2} = $addr2;
    $hashRef->{TotElemCnt} = $endRelPos + $saveStartPos + $actElemCnt;
  }

  # Check for Dimensioning
  elsif ($self->getDim1Size() != 0) {
    my $dim1 = -1;
    my $dim2 = -1;
    my $cnt = -1;

    if ($self->getDim2Size() != 0) {
      $dim1 = $self->getDim1Sel();
      $dim2 = $self->getDim2Sel();
      # Dimen1 & Dimen2
      while ($startRelPos != 0) {
	if ($dim2 != 0) {
	  # Take what we can from Addr2
	  my $spaceToTake = &minVal($dim2, $startRelPos);
	  $dim2 -= $spaceToTake;
	  $startRelPos -= $spaceToTake;
	}
	if ($startRelPos != 0 && $dim2 == 0) {
	  #Reset Addr2, Decr Addr1
	  $dim1--;
	  $dim2 = $self->getDim2Size()-1;
	  $startRelPos--;
	}
      }
    } else {
      $dim1 = $self->getDim1Sel();
      # Only Addr1
      $dim1 -= $startRelPos;
      $startRelPos = 0;
    }

    delete $hashRef->{Addr1};
    delete $hashRef->{Addr2};
    $hashRef->{Dim1} = $dim1;
    $hashRef->{Dim2} = $dim2;
    $hashRef->{TotElemCnt} = $endRelPos + $saveStartPos + $actElemCnt;
  }

  return;
}


################################################
# Print / String Utilities
################################################

sub getStructStrWrap {
  my ($self) = @_;
  my @strArr = ();
  $self->getStructArr($self->getVarTopParent(), \@strArr);
  @strArr = reverse @strArr;
  my $rtnStr = "";
  foreach my $struct (@strArr) {
    $rtnStr .= $self->getStructStr($struct, 0)."\n";
  }
  return $rtnStr;
}

sub getStructArr {
  my ($self, $top, $arrRef) = @_;
  if ($top->getVarClass() ne "VAR") {
    push(@$arrRef, $top) if ($top->getVarAnon() == 0);
    foreach my $field (@{$top->getVarFields}) {
      $self->getStructArr($field, $arrRef);
    }
  }
  return $arrRef;
}

sub getStructStr {
  my ($self, $top, $level) = @_;

  my $levelOffset = "";
  for (my $i = 0; $i < $level; $i++) {
    $levelOffset .= "  ";
  }

  my $rtnStr = "";
  if (($top->getVarClass() ne "VAR") && (($top->getVarAnon == 1) || ($level == 0))) {
    $rtnStr .= sprintf("%s%s %s{\n",
		       $levelOffset,
		       ($top->getVarClass() eq "STR") ? "struct" : "union",
		       ($level == 0) ? $top->getVarType()." " : ""
		      );
    foreach my $field (@{$top->getVarFields}) {
      $rtnStr .= $self->getStructStr($field, $level+1);
    }
    $rtnStr .= sprintf("%s};\n", $levelOffset);
  } else {
    $rtnStr .= sprintf("%s%s %s%s;\n",
		       $levelOffset,
		       $top->getVarType(),
		       $top->getVarNameWDimDef(),
		       $top->getVarBitFNumStr(),
		      );
  }
  return $rtnStr;
}

sub getWRStrWrap {
  my ($self, $stg, $dstVar) = @_;
  my $rtnStr = "";
  $rtnStr .= $self->getWriteAddrStr($stg);
  my $d1 = ($self->getDim1Size() == 0) ? "-" : $self->getDim1Sel();
  my $d2 = ($self->getDim2Size() == 0) ? "-" : $self->getDim2Sel();
  my $wrSfxReq = 0;
  if ($self->getVarTopParent()->getVarName() eq $dstVar->getVarTopParent()->getVarName()) {
    if ($self->getVarContUNN() == 1) {
      if ($self->getVarPSG() eq "P") {
	$wrSfxReq = 1;
      }
    }
  }
  $rtnStr .= $self->getWRStr($stg, $d1, $d2, $wrSfxReq);
  return $rtnStr;
}

sub getWRStr {
  my ($self, $stg, $d1, $d2, $wrSfxReq) = @_;
  my $rtnStr = "";

  if ($self->getVarUsedField() != 1) {
    return "";
  }

  # If STR/UNN
  if ($self->getVarClass() ne "VAR") {
    foreach my $field (@{$self->getVarFields()}) {
      my @d1Arr = ();
      my @d2Arr = ();
      $field->getDim1Arr(\@d1Arr);
      $field->getDim2Arr(\@d2Arr);
      foreach my $d1 (@d1Arr) {
	foreach my $d2 (@d2Arr) {
	  $rtnStr .= $field->getWRStr($stg, $d1, $d2, $wrSfxReq);
	}
      }
    }
  }

  # If VAR
  else {
    # Top Level S/Addr Var
    if (($self->getVarPSG() eq "S") && ($self->getVarParent() == 0) && ($self->getAddr1Width() != 0)) {
      $rtnStr .= sprintf("%s%s%s_%s.write_mem((%s)0x%016xLL);\n",
			 $self->getVarPSG(),
			 $self->getRandWrSfx(),
			 $self->getMaskedStage($stg),
			 $self->getVarNameWDim(),
			 $self->getBFVarType(),
			 $self->getVarValue()
			);
    }

    # All other VARs
    else {
      my $chainRef = $self->getVarChain();
      $rtnStr .= sprintf("%s%s%s_%s%s",
			 $self->getVarPSG(),
			 ($wrSfxReq == 1) ? "W" : $chainRef->[0]->getRandWrSfx(),
			 $self->getMaskedStage($stg),
			 $chainRef->[0]->getVarNameWDim(),
			 (($self->getVarPSG() eq "S") && ($self->getAddr1Width() != 0)) ? ".write_mem()" : ""
			);
      splice(@$chainRef, 0, 1);
      foreach my $var (@$chainRef) {
	$rtnStr .= sprintf(".%s",
			   $var->getVarNameWDim()
			  );
      }
      $rtnStr .= sprintf(" = ((%s)0x%016xLL);\n",
			 $self->getBFVarType(),
			 $self->getVarValue()
			);
    }
  }

  # Replace D1/D2
  my $varName = $self->getVarName();
  $rtnStr =~ s/$varName\[X\]\[X\]/$varName\[$d1\]\[$d2\]/g;
  $rtnStr =~ s/$varName\[X\]/$varName\[$d1\]/g;
  return $rtnStr;
}

sub manipToElemStr {
  my ($self, $rtnStr, $elem) = @_;

  # Check Addressing
  if ((($self->getIsIdxAddr1() != 0) || ($self->getIsIdxAddr2() != 0)) && ($self->getVarParent == 0)) { # Parent check needed!
    if ($self->getAddr2Width() != 0) {
      # Addr1 & Addr2
      my $oldAddr1 = $self->getAddr1Sel();
      my $oldAddr2 = $self->getAddr2Sel();
      my $newAddr1 = $self->getAddr1Sel();
      my $newAddr2 = $self->getAddr2Sel();
      my $amtToIncr = $elem;
      $newAddr2 += $amtToIncr;
      while ($newAddr2 > 2**$self->getAddr2Width()-1) {
	$newAddr1++;
	$newAddr2 -= 2**$self->getAddr2Width();
      }
      $rtnStr =~ s/_addr\($oldAddr1, $oldAddr2\)/_addr\($newAddr1, $newAddr2\)/g;
    } else {
      # Only Addr1
      my $oldAddr1 = $self->getAddr1Sel();
      my $newAddr1 = $self->getAddr1Sel();
      my $amtToIncr = $elem;
      $newAddr1 += $amtToIncr;
      $rtnStr =~ s/_addr\($oldAddr1\)/_addr\($newAddr1\)/g;
    }
  }

  # Check for Dimensioning
  elsif ($self->getDim1Size() != 0) {
    if ($self->getDim2Size() != 0) {
      # Dimen1 & Dimen2
      my $oldDim1 = $self->getDim1Sel();
      my $oldDim2 = $self->getDim2Sel();
      my $newDim1 = $self->getDim1Sel();
      my $newDim2 = $self->getDim2Sel();
      my $amtToIncr = $elem;
      $newDim2 += $amtToIncr;
      while ($newDim2 > $self->getDim2Size()-1) {
	$newDim1++;
	$newDim2 -= $self->getDim2Size();
      }
      my $varName = $self->getVarName();
      $rtnStr =~ s/$varName\[$oldDim1\]\[$oldDim2\]/$varName\[$newDim1\]\[$newDim2\]/g;
    } else {
      # Only Dimen1
      my $oldDim1 = $self->getDim1Sel();
      my $newDim1 = $self->getDim1Sel();
      my $amtToIncr = $elem;
      $newDim1 += $amtToIncr;
      my $varName = $self->getVarName();
      $rtnStr =~ s/$varName\[$oldDim1\]/$varName\[$newDim1\]/g;
    }
  }

  return $rtnStr;
}

sub manipToRAElemStr {
  my ($self, $rtnStr, $elem) = @_;

  # Check Addressing
  if ((($self->getIsIdxAddr1() != 0) || ($self->getIsIdxAddr2() != 0)) && ($self->getVarParent == 0)) { # Parent check needed!
    if ($self->getAddr2Width() != 0) {
      # Addr1 & Addr2
      my $oldAddr1 = $self->getAddr1Sel();
      my $oldAddr2 = $self->getAddr2Sel();
      my $newAddr1 = $self->getAddr1Sel();
      my $newAddr2 = $self->getAddr2Sel();
      my $amtToIncr = $elem;
      $newAddr2 += $amtToIncr;
      while ($newAddr2 > 2**$self->getAddr2Width()-1) {
	$newAddr1++;
	$newAddr2 -= 2**$self->getAddr2Width();
      }
      $rtnStr =~ s/RdAddr1(.*)$oldAddr1;/RdAddr1$1$newAddr1;/g;
      $rtnStr =~ s/RdAddr2(.*)$oldAddr2;/RdAddr2$1$newAddr2;/g;
    } else {
      # Only Addr1
      my $oldAddr1 = $self->getAddr1Sel();
      my $newAddr1 = $self->getAddr1Sel();
      my $amtToIncr = $elem;
      $newAddr1 += $amtToIncr;
      $rtnStr =~ s/RdAddr1(.*)$oldAddr1;/RdAddr1$1$newAddr1;/g;
    }
  }

  return $rtnStr;
}

sub manipToBRAMWRStr {
  my ($self, $rtnStr) = @_;

  # Remove write_addr lines
  my $newStr = "";
  my @lines = split(/\n/, $rtnStr);
  DEL_LOOP: for (my $idx = 0; $idx <= $#lines; $idx++) {
    if ($lines[$idx] =~ /write_addr/) {
      $newStr .= $lines[$idx];
      $newStr .= "\n";
      splice(@lines, $idx, 1);
      redo DEL_LOOP;
    }
  }

  # Create temp variable
  my $tempType = $self->getVarTopParent()->getVarType();
  my $tempName = "temp_".$self->getVarTopParent()->getVarName();
  $newStr .= "$tempType $tempName;\n";

  # Manipulate Assignment Lines
  for (my $idx = 0; $idx <= $#lines; $idx++) {
    $lines[$idx] =~ s/\.write_mem\(\)//;
    $lines[$idx] =~ s/^\w*?\d*?_/temp_/;
    $lines[$idx] =~ s/$tempName\[.*?\]/$tempName/;
    $lines[$idx] =~ s/$tempName\[.*?\]/$tempName/;
    $newStr .= $lines[$idx];
    $newStr .= "\n";
  }

  # Create assignment line
  if ($rtnStr =~ /(.*)\.write_mem/) {
    $newStr .= $1.".write_mem(($tempType)$tempName);\n";
  } else {
    die "Bad match in manipToBRAMWRStr! $!";
  }

  return $newStr;
}

sub getWRRA1Str {
  my ($self, $stg) = @_;
  my $rtnStr = "";
  if ($self->getVarWrType() eq "Type") {
    $rtnStr .= $self->getAddr1VarReadStr($stg);
  }
  return $rtnStr;
}

sub getWRRA2Str {
  my ($self, $stg) = @_;
  my $rtnStr = "";
  if ($self->getVarWrType() eq "Type") {
    $rtnStr .= $self->getAddr2VarReadStr($stg);
  }
  return $rtnStr;
}

sub getSTStr {
  my ($self, $stg, $dstVar) = @_;
  my $rtnStr = "";
  my $chainRef = $self->getVarChain();

  # Var
  if ($self->getVarWrType() eq "Var") {
    my $addrStr = $self->getMemAddrStr();
    my $dimStr = "";
    foreach my $var (@$chainRef) {
      $dimStr .= $var->getMemDimStr();
    }
    my $eleStr = (($addrStr ne "") || ($dimStr ne "")) ? $self->getEleMemStr() : "";
    $rtnStr .= sprintf("WriteMem_%s(PR%s_memAddr + %d%s%s%s);\n",
		       $self->getVarName(),
		       $stg,
		       ($self->getTestCyc() * 8 * $CNT_MaxQWPerCyc_MAX),
		       $addrStr,
		       $dimStr,
		       $eleStr
		      );
  }

  # Type
  else {
    if ($self->getVarIsBlkRam() ne "true") {
      $rtnStr .= $self->getReadAddrStr($stg);
    }
    # Multi Level Var
    my $rdSfxReq = 0;
    if ($self->getVarTopParent()->getVarName() eq $dstVar->getVarTopParent()->getVarName()) {
      if ($self->getVarContUNN() == 1) {
	if ($self->getVarPSG() eq "P") {
	  $rdSfxReq = 1;
	}
      }
    }
    $rtnStr .= sprintf("WriteMem_%s(PR%s_memAddr + %d, %s%s%s_%s%s",
		       $self->getVarType(),
		       $stg,
		       ($self->getTestCyc() * 8 * $CNT_MaxQWPerCyc_MAX),
		       $self->getVarPSG(),
		       ($rdSfxReq == 1) ? "R" : $self->getRandRdSfx(),
		       $self->getMaskedStage($stg),
		       $chainRef->[0]->getVarNameWDim(),
		       (($self->getVarPSG() eq "S") && ($self->getAddr1Width() != 0)) ? ".read_mem()" : ""
		      );
    splice(@$chainRef, 0, 1);
    foreach my $var (@$chainRef) {
      $rtnStr .= sprintf(".%s",
			 $var->getVarNameWDim()
			);
    }
    $rtnStr .= ");\n";
  }
  return $rtnStr;
}

sub getLDStr {
  my ($self, $stg) = @_;
  my $chainRef = $self->getVarChain();
  my $addrStr = $self->getMemAddrStr();
  my $dimStr = "";
  foreach my $var (@$chainRef) {
    $dimStr .= $var->getMemDimStr();
  }
  my $eleStr = (($addrStr ne "") || ($dimStr ne "")) ? $self->getEleMemStr() : "";
  return sprintf("ReadMem_%s(PR%s_memAddr + %d%s%s%s);\n",
		 $self->getVarName(),
		 $stg,
		 ($self->getTestCyc() * 8 * $CNT_MaxQWPerCyc_MAX),
		 $addrStr,
		 $dimStr,
		 $eleStr
		);
}

sub getLDRA1Str {
  my ($self, $stg) = @_;
  my $rtnStr = "";
  $rtnStr .= $self->getAddr1VarReadStr($stg);
  return $rtnStr;
}

sub getLDRA2Str {
  my ($self, $stg) = @_;
  my $rtnStr = "";
  $rtnStr .= $self->getAddr2VarReadStr($stg);
  return $rtnStr;
}

sub getCHKStrWrap {
  my ($self, $srcVar, $stg) = @_;
  my $rtnStr = "";
  if ($self->getVarIsBlkRam() ne "true") {
    $rtnStr .= $self->getReadAddrStr($stg);
  }
  my $d1 = ($self->getDim1Size() == 0) ? "-" : $self->getDim1Sel();
  my $d2 = ($self->getDim2Size() == 0) ? "-" : $self->getDim2Sel();
  $rtnStr .= $self->getCHKStr($srcVar, $stg, $d1, $d2);
  return $rtnStr;
}

sub getCHKStr {
  my ($self, $srcVar, $stg, $d1, $d2) = @_;
  my $rtnStr = "";

  if ($self->getVarUsedField() != 1) {
    return "";
  }

  # If STR
  if ($self->getVarClass() eq "STR") {
    foreach my $field (@{$self->getVarFields()}) {
      my @d1Arr = ();
      my @d2Arr = ();
      $field->getDim1Arr(\@d1Arr);
      $field->getDim2Arr(\@d2Arr);
      foreach my $d1 (@d1Arr) {
	foreach my $d2 (@d2Arr) {
	  $rtnStr .= $field->getCHKStr($srcVar, $stg, $d1, $d2);
	}
      }
    }
  }

  # If UNN
  elsif ($self->getVarClass() eq "UNN") {
    if (&genChance($PCT_ChkStr_CHKALLUNN, 1, 0) && $self->getVarBitField() == 0) { # Workaround for bitfields for now
      $rtnStr .= $self->getUNNCHKStrWrap($srcVar, $stg);
    } else {
      foreach my $field (@{$self->getVarFields()}) {
	my @d1Arr = ();
	my @d2Arr = ();
	$field->getDim1Arr(\@d1Arr);
	$field->getDim2Arr(\@d2Arr);
	foreach my $d1 (@d1Arr) {
	  foreach my $d2 (@d2Arr) {
	    $rtnStr .= $field->getCHKStr($srcVar, $stg, $d1, $d2);
	  }
	}
      }
    }
  }

  # If VAR
  else {
    my $matchingVar = $srcVar->findVarByName($self->getVarName());
    if ($matchingVar == 0) {
      $matchingVar = $srcVar; # FIXME? Catch all old regular cases
    }
    my $chainRef = $self->getVarChain();
    my $rdSfxReq = 0;
    if ($chainRef->[0]->getVarContUNN() == 1) {
      if ($chainRef->[0]->getVarPSG() eq "P") {
	$rdSfxReq = 1;
      }
    }
    $rtnStr .= sprintf("if (%s%s%s_%s",
		       #$matchingVar->getBFVarType(),
		       $chainRef->[0]->getVarPSG(),
		       ($rdSfxReq == 1) ? "R" : $chainRef->[0]->getRandRdSfx(),
		       $chainRef->[0]->getMaskedStage($stg),
		       $chainRef->[0]->getVarNameWDim()
		      );
    splice(@$chainRef, 0, 1);
    if (($self->getAddr1Width() != 0) && ($self->getVarPSG() eq "S")) {
      $rtnStr .= ".read_mem()";
    }
    foreach my $var (@$chainRef) {
      $rtnStr .= sprintf(".%s",
			 $var->getVarNameWDim()
			);
    }
    $rtnStr .= sprintf(" != ((%s)0x%016xLL)) {\n",
		       $matchingVar->getBFVarType(),
		       $matchingVar->getVarValue()
		      );
    $rtnStr .= "\tHtAssert(0, 0);\n";
    $rtnStr .= "}\n";
  }

  # Replace D1/D2
  my $varName = $self->getVarName();
  $rtnStr =~ s/$varName\[X\]\[X\]/$varName\[$d1\]\[$d2\]/g;
  $rtnStr =~ s/$varName\[X\]/$varName\[$d1\]/g;
  return $rtnStr;
}

sub getUNNCHKStrWrap {
  my ($self, $srcVar, $stg) = @_;
  my $rtnStr = "";
  my %expMem = ();
  $self->getExpMem($srcVar, \%expMem);
  foreach my $field (@{$self->getVarFields()}) {
    my @d1Arr = ();
    my @d2Arr = ();
    $field->getDim1Arr(\@d1Arr);
    $field->getDim2Arr(\@d2Arr);
    my $saveDataIdx = $expMem{DataIdx};
    my $saveSizeIdx = $expMem{SizeIdx};
    foreach my $d1 (@d1Arr) {
      foreach my $d2 (@d2Arr) {
	if ($field->hasExpMemData(\%expMem)) {
	  $rtnStr .= $field->getUNNCHKStr($srcVar, $stg, \%expMem, $d1, $d2);
	}
      }
    }
    $expMem{DataIdx} = $saveDataIdx;
    $expMem{SizeIdx} = $saveSizeIdx;
  }
  return $rtnStr;
}

sub getUNNCHKStr {
  my ($self, $srcVar, $stg, $memRef, $d1, $d2) = @_;
  my $rtnStr = "";

  # If STR
  if ($self->getVarClass() eq "STR") {
    foreach my $field (@{$self->getVarFields()}) {
      my @d1Arr = ();
      my @d2Arr = ();
      $field->getDim1Arr(\@d1Arr);
      $field->getDim2Arr(\@d2Arr);
      foreach my $d1 (@d1Arr) {
	foreach my $d2 (@d2Arr) {
	  if ($field->hasExpMemData($memRef)) {
	    $rtnStr .= $field->getUNNCHKStr($srcVar, $stg, $memRef, $d1, $d2);
	  }
	}
      }
    }
  }

  # If UNN
  elsif ($self->getVarClass() eq "UNN") {
    foreach my $field (@{$self->getVarFields()}) {
      my @d1Arr = ();
      my @d2Arr = ();
      $field->getDim1Arr(\@d1Arr);
      $field->getDim2Arr(\@d2Arr);
      my $saveDataIdx = $memRef->{DataIdx};
      my $saveSizeIdx = $memRef->{SizeIdx};
      foreach my $d1 (@d1Arr) {
	foreach my $d2 (@d2Arr) {
	  if ($field->hasExpMemData($memRef)) {
	    $rtnStr .= $field->getUNNCHKStr($srcVar, $stg, $memRef, $d1, $d2);
	  }
	}
      }
      $memRef->{DataIdx} = $saveDataIdx;
      $memRef->{SizeIdx} = $saveSizeIdx;
    }
  }

  # If VAR
  else {
    my $srcType = "BAD_TYPE";
    my $srcVal  = 0;
    $self->getExpMemData($self->getBaseVarSize(), $memRef, \$srcType, \$srcVal);
    # FIXME - Structure organization is weird
    # PAD (kill off extra data in memRef) IF:
    #  1. The next field has a larger alignment than the current field
    #  2. This is the last field in the structure and padding is required
    my $discardSize = 0;
    if ($self->getVarParent()->getVarClass() eq "STR") {
      my $curIdx = $self->getTestCyc();
      # If not last
      if ($self->isLastIdx($d1, $d2) == 1) {
	if ($curIdx < $#{$self->getVarParent()->getVarFields()}) {
	  # If next field has larger alignment
	  if ($self->getExpMemAlignment($memRef) < $self->getVarParent()->getVarFields()->[$curIdx+1]->getVarAlignment()) {
	    $discardSize = $self->getVarParent()->getVarFields()->[$curIdx+1]->getVarAlignment() - $self->getExpMemAlignment($memRef);
	  }
	} elsif ($curIdx == $#{$self->getVarParent()->getVarFields()}) {
	  foreach my $field (@{$self->getVarParent()->getVarFields()}) {
	    if ($field->getVarAlignment() > $discardSize) {
	      $discardSize = $field->getVarAlignment();
	    }
	  }
	  $discardSize -= $self->getExpMemAlignment($memRef);
	}
      }
      unless ($discardSize == 0) {
	my $discardType = "";
	my $discardVal = 0;
	if ($self->hasExpMemData($memRef)) {
	  $self->getExpMemData($discardSize, $memRef, \$discardType, \$discardVal);
	}
      }
    }
    my $chainRef = $self->getVarChain();
    my $rdSfxReq = 0;
    if ($chainRef->[0]->getVarContUNN() == 1) {
      if ($chainRef->[0]->getVarPSG() eq "P") {
	$rdSfxReq = 1;
      }
    }
    # Check to see if it fills the variable...
    my $compVarSize = $self->getBFVarType();
    my $srcSize = $srcType;
    $compVarSize = $1 if ($compVarSize =~ /(\d+)/);
    $srcSize = $1 if ($srcSize =~ /(\d+)/);
    if ($srcSize != $compVarSize) {
      if ($chainRef->[0]->getVarPSG() eq "S") {
	#print "OMMITTING CHECK VS ".$chainRef->[0]->getVarNameWDim()."\n";
	return "";
      }
      unless ($self->getBFVarType() =~ /uint/) {
	#print "OMMITTING CHECK VS ".$chainRef->[0]->getVarNameWDim()."\n";
	return "";
      }
    }

    $rtnStr .= sprintf("if (%s%s%s_%s",
		       #$srcType,
		       $chainRef->[0]->getVarPSG(),
		       ($rdSfxReq == 1) ? "R" : $chainRef->[0]->getRandRdSfx(),
		       $chainRef->[0]->getMaskedStage($stg),
		       $chainRef->[0]->getVarNameWDim()
		      );
    splice(@$chainRef, 0, 1);
    if (($self->getAddr1Width() != 0) && ($self->getVarPSG() eq "S")) {
      $rtnStr .= ".read_mem()";
    }
    foreach my $var (@$chainRef) {
      $rtnStr .= sprintf(".%s",
			 $var->getVarNameWDim()
			);
    }
    $rtnStr .= sprintf(" != ((%s)(%s)0x%016xLL)) {\n",
		       $self->getBFVarType(),
		       $srcType,
		       $srcVal
		      );
    $rtnStr .= "\tHtAssert(0, 0);\n";
    $rtnStr .= "}\n";
  }

  # Replace D1/D2
  my $varName = $self->getVarName();
  $rtnStr =~ s/$varName\[X\]\[X\]/$varName\[$d1\]\[$d2\]/g;
  $rtnStr =~ s/$varName\[X\]/$varName\[$d1\]/g;
  return $rtnStr;
}

sub getChainNameHTD {
  my ($self) = @_;
  my $chainRef = $self->getVarChain();
  my $rtnStr = $chainRef->[0]->getVarNameWDimHTD();
  splice(@$chainRef, 0, 1);
  foreach my $var (@$chainRef) {
    $rtnStr .= ".".$var->getVarNameWDimHTD();
  }
  return $rtnStr;
}

sub getChainName {
  my ($self) = @_;
  my $chainRef = $self->getVarChain();
  my $rtnStr = $chainRef->[0]->getVarNameWDim();
  splice(@$chainRef, 0, 1);
  foreach my $var (@$chainRef) {
    $rtnStr .= ".".$var->getVarNameWDim();
  }
  return $rtnStr;
}


################################################
# _varClass
################################################

sub genVarClass {
  my ($self) = @_;

  my $svu = &genChance3(
			$PCT_VarClass_VAR,
			$PCT_VarClass_STR,
			$PCT_VarClass_UNN,
			"VAR",
			"STR",
			"UNN"
		       );
  $self->{_varClass} = $svu;
  $self->{_varClass} = "VAR" if ($self->getVarDepth >= $CNT_VarDepth_MAX-1);
  $svu = "VAR" if (($self->getVarParent() != 0) && ($self->getVarParent()->getVarBitField() == 2));
  $self->setVarContUNN(1) if ($self->getVarClass() eq "UNN");
  return $self->{_varClass};
}

sub getVarClass {
  my ($self) = @_;
  return $self->{_varClass};
}


################################################
# _varName
################################################

sub genVarName {
  my ($self) = @_;
  $self->{_varName} = sprintf("test%02d_%s_%s_%s_data",
			      $self->getTestStr(),
			      $self->getTestCyc(),
			      $self->getTestSrcDst(),
			      $self->genUniqStr()
			     );
  return $self->{_varName};
}

sub getVarName {
  my ($self) = @_;
  return $self->{_varName};
}

sub getVarNameWDimHTD {
  my ($self) = @_;
  my $rtnStr = "";
  $rtnStr .= sprintf("%s%s%s",
		     $self->{_varName},
		     $self->getDim1LitStrHTD(),
		     $self->getDim2LitStrHTD()
		    );
  if ($self->getVarParent() == 0) {
    $rtnStr .= sprintf("%s%s",
		       $self->getAddr1LitStrHTD(),
		       $self->getAddr2LitStrHTD()
		      );
  }
  return $rtnStr;
}

sub getVarNameWDimDef {
  my ($self) = @_;
  return sprintf("%s%s%s",
		 $self->{_varName},
		 $self->getDim1LitDefStr(),
		 $self->getDim2LitDefStr()
		);
}


sub getVarNameWDim {
  my ($self) = @_;
  return sprintf("%s%s%s",
		 $self->{_varName},
		 $self->getDim1LitStr(),
		 $self->getDim2LitStr()
		);
}


################################################
# _varSize
################################################

sub updateVarSize {
  my ($self) = @_;
  my $baseSize = $self->getBaseVarSize();

  if ($self->getVarClass() eq "VAR") {
    if ($self->getVarBitFType() ne "") {
      $baseSize = $1 if ($self->getVarBitFType() =~ /(\d+)/);
      $self->{_varBaseSize} = $baseSize;
    } elsif ($self->getVarType() =~ /(\d+)/) {
      $baseSize = $1;
      $self->{_varBaseSize} = $baseSize;
    }
  }

  $baseSize *= $self->getDim1Size() if ($self->getDim1Size() > 0);
  $baseSize *= $self->getDim2Size() if ($self->getDim2Size() > 0);

  $self->{_varActSize} = $baseSize;
  $self->{_varParent}->updateFieldSize() if ($self->{_varParent} != 0);
  return $self->{_varActSize};
}

sub updateFieldSize {
  my ($self) = @_;
  my $baseSize = 0;

  foreach my $field (@{$self->{_varFields}}) {
    $baseSize += $field->getActVarSize();
  }

  $self->{_varBaseSize} = $baseSize;
  $self->updateVarSize();
  return $self->{_varActSize};
}

sub getBaseVarSize {
  my ($self) = @_;
  return $self->{_varBaseSize};
}

sub getActVarSize {
  my ($self) = @_;
  return $self->{_varActSize};
}


################################################
# _varType
################################################

sub modVarType {
  my ($self, $type) = @_;
  $self->{_varType} = $type;
  $self->updateVarSize();
  return $self->{_varType};
}

sub genVarType {
  my ($self) = @_;
  my $svu = $self->getVarClass();
  if ($svu eq "VAR") {
    my @VarTypes = ();
    push(@VarTypes, @GLB_STDVarTypes);
    push(@VarTypes, @GLB_CSTVarTypes) if ($self->getVarParent() == 0);
    my $typeIdx = rand @VarTypes;
    my $type = (keys %{$VarTypes[$typeIdx]})[0];
    my $valIdx = rand @{$VarTypes[$typeIdx]{$type}{Values}};
    my $val = $VarTypes[$typeIdx]{$type}{Values}[$valIdx];
    $type =~ s/X/$val/;
    $self->{_varType} = $type;
    $self->updateVarSize();
  } elsif ($svu eq "STR") {
    $self->{_varType} = $self->getVarName()."_struct";
    $self->{_varAnon} = ($self->getVarDepth() == 0) ? 0 : &genChance($PCT_VarAnon_ANON, 1, 0);
  } elsif ($svu eq "UNN") {
    $self->{_varType} = $self->getVarName()."_union";
    $self->{_varAnon} = ($self->getVarDepth() == 0) ? 0 : &genChance($PCT_VarAnon_ANON, 1, 0);
  }
  return $self->{_varType};
}

sub getVarType {
  my ($self) = @_;
  return $self->{_varType};
}

sub getBFVarType {
  my ($self) = @_;
  if ($self->getVarBitFType() ne "") {
    return $self->getVarBitFType();
  } else {
    return $self->{_varType};
  }
}

sub getVarRWTypeStr {
  my ($self, $rw) = @_;

  # Determine rdType / wrType
  unless (($self->getVarType() =~ /int\d+_t/) || ($self->getVarType() =~ /data/)) {
    if ($self->getBFVarType() =~ /(\d+)/) {
      return sprintf(", %sType=uint%d_t",
		     ($rw eq "R") ? "rd" : "wr",
		     $self->getVarAlignment()
		    );
    }
  } else {
    return "";
  }
}

sub getVarAlignment {
  my ($self) = @_;
  return 64 if ($self->getBaseVarSize() > 32);
  return 32 if ($self->getBaseVarSize() <= 32 && $self->getBaseVarSize() > 16);
  return 16 if ($self->getBaseVarSize() <= 16 && $self->getBaseVarSize() > 8);
  return 8  if ($self->getBaseVarSize() <= 8);
}

sub getVarTotAlignment {
  my ($self) = @_;
  my $alignment = 0;
  if ($self->getVarClass ne "VAR") {
    foreach my $field (@{$self->getVarFields()}) {
      $alignment = &maxVal($alignment, $field->getVarTotAlignment());
    }
  } else {
    $alignment = $self->getVarAlignment();
  }
  return $alignment;
}


################################################
# _varAnon
################################################

#sub setVarAnon

sub getVarAnon {
  my ($self) = @_;
  return $self->{_varAnon};
}


################################################
# _varPSG
################################################

sub genVarPSG {
  my ($self) = @_;
  $self->{_varPSG} = &genChance3(
				 $PCT_VarPSG_P,
				 $PCT_VarPSG_S,
				 $PCT_VarPSG_G,
				 "P",
				 "S",
				 "G"
				);
  return $self->{_varPSG};
}

sub getVarPSG {
  my ($self) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->getVarPSG();
  } else {
    return $self->{_varPSG};
  }
}

sub getRandRdSfx {
  my ($self) = @_;
  if ($self->getVarPSG() eq "G") {
    return "R";
  } elsif (($self->getVarPSG() eq "P") && ($self->getAddr1Var() != 0)) {
    # Addressed Private Vars require read suffix
    return "R";
  } else {
    return &genChance($PCT_VarRdSfx_R, "R", "");
  }
}

sub getRandRARdSfx {
  my ($self) = @_;
  if ($self->getVarPSG() eq "G") {
    return "R";
  } elsif ($self->getVarPSG() eq "P") {
    # Addressed Private Vars require read suffix
    return "R";
  } else {
    return &genChance($PCT_VarWrSfx_W, "R", "");
  }
}

sub getRandWrSfx {
  my ($self) = @_;
  if ($self->getVarPSG() eq "G") {
    return "W";
  } elsif (($self->getVarPSG() eq "P") && ($self->getAddr1Var() != 0)) {
    # Addressed Private Vars require read suffix
    return "W";
  } elsif ($self->getVarPSG() eq "S") {
    return "";
  }
}

sub getRandRAWrSfx {
  my ($self) = @_;
  if ($self->getVarPSG() eq "G") {
    return "W";
  } elsif ($self->getVarPSG() eq "P") {
    # Addressed Private Vars require write suffix
    return "W";
  } elsif ($self->getVarPSG() eq "S") {
    return "";
  }
}


################################################
# _varValue
################################################

sub genVarValue {
  my ($self) = @_;
  if ($self->getVarClass eq "VAR") {
    $self->{_varValue} = int(rand(2**53));
  } else {
    $self->{_varValue} = 0;
  }
  return $self->{_varValue};
}

sub getVarValue {
  my ($self) = @_;
  return $self->{_varValue};
}


################################################
# _varWrType
################################################

sub genVarWrType {
  my ($self) = @_;
  $self->{_varWrType} = &genChance($PCT_VarWrType_VAR, "Var", "Type");
  if ($self->{_varWrType} eq "Type") {
    my $chainRef = $self->getVarChain();
    foreach my $var (@$chainRef) {
      $var->setDim1DSel("SRC");
      $var->setDim2DSel("SRC");
    }
  }
  return $self->{_varWrType};
}

sub setVarWrType {
  my ($self, $wrType) = @_;
  $self->{_varWrType} = $wrType;
  return $self->{_varWrType};
}

sub getVarWrType {
  my ($self) = @_;
  return $self->{_varWrType};
}


################################################
# _varIsBlkRam
################################################

sub genVarIsBlkRam {
  my ($self) = @_;
  if (
      ($self->getVarPSG() eq "S") &&
      ($self->getAddr1Width() != 0) &&
      ($self->getBaseVarSize() <= 64) &&
      ($self->getBaseVarSize() <= $self->getVarTotAlignment()) &&
      (
       (($self->getVarParent() != 0) && ($self->getBaseVarSize() == $self->getVarTopParent()->getBaseVarSize())) ||
       ($self->getVarParent() == 0)
      )
     ) {
    my $isBram = &genChance($PCT_IsBlkRam_BRAM, "true", "false");
    return $self->setVarIsBlkRam(($isBram eq "false") ? &genChance($PCT_IsBlkRam_DEF, $isBram, "") : $isBram);
  } elsif ($self->getVarPSG() eq "G") {
    my $isBram = &genChance($PCT_IsBlkRam_BRAM, "true", "false");
    return $self->setVarIsBlkRam(($isBram eq "false") ? &genChance($PCT_IsBlkRam_DEF, $isBram, "") : $isBram);
  } else {
    return $self->setVarIsBlkRam("");
  }
  return $self->{_varIsBlkRam};
}

sub setVarIsBlkRam {
  my($self, $val) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->setVarIsBlkRam($val);
  } else {
    $self->{_varIsBlkRam} = $val;
    return $self->{_varIsBlkRam};
  }
}

sub getVarIsBlkRam {
  my ($self) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->getVarIsBlkRam();
  } else {
    return $self->{_varIsBlkRam};
  }
}


################################################
# _varContUNN
################################################

sub setVarContUNN {
  my ($self, $val) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->setVarContUNN($val);
  } else {
    $self->{_varContUNN} = $val;
    return $self->{_varContUNN};
  }
}

sub getVarContUNN {
  my ($self) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->getVarContUNN();
  } else {
    return $self->{_varContUNN};
  }
}


################################################
# _varUsedField
################################################

sub setVarUsedField {
  my ($self, $val) = @_;
  $self->{_varUsedField} = $val;
  return $self->{_varUsedField};
}

sub getVarUsedField {
  my ($self) = @_;
  return $self->{_varUsedField};
}


################################################
# _varBitField
################################################

sub genVarBitField {
  my ($self) = @_;
  if ($self->getVarClass() ne "VAR") {
    my $BF = &genChance($PCT_BitField_STRBF, 1, 0);
    if ($BF == 1) {
      $BF = &genChance($PCT_BitField_TOTBF, 2, 1);
    }
    $self->setVarBitField($BF);
  } else {
    $self->setVarBitField(0);
  }
  return $self->{_varBitField};
}

sub setVarBitField {
  my ($self, $val) = @_;
  $self->{_varBitField} = $val;
  return $self->{_varBitField};
}

sub getVarBitField {
  my ($self) = @_;
  return $self->{_varBitField};
}


################################################
# _varBitFType
################################################

sub genVarBitFType {
  my ($self) = @_;
  my $BFType = "";
  if (
      ($self->getVarParent() != 0) &&
      ($self->getVarParent()->getVarBitField() != 0) &&
      ($self->getVarClass eq "VAR")
     ) {
    if (($self->getVarParent()->getVarBitField() == 2) || (&genChance($PCT_BitField_VAR, 1, 0))) {
      my @avlWidth = (1..$self->getBaseVarSize());
      my $selWidth = $avlWidth[rand @avlWidth];
      $BFType = sprintf("ht_%sint%d", ($self->getVarType() =~ /uint/) ? "u" : "", $selWidth);
      $self->setDim1Size(0);
      $self->setDim2Size(0);
    }
  }
  $self->{_varBitFType} = $BFType;
  $self->updateVarSize();
  return $self->{_varBitFType};
}

sub setVarBitFType {
  my ($self, $BFType) = @_;
  $self->{_varBitFType} = $BFType;
  $self->updateVarSize();
  return $self->{_varBitFType};
}

sub getVarBitFType {
  my ($self) = @_;
  return $self->{_varBitFType};
}

sub getVarBitFNumStr {
  my ($self) = @_;
  my $rtnStr = "";
  if ($self->getVarBitFType() ne "") {
    $rtnStr .= " : ";
    $rtnStr .= $1 if ($self->getVarBitFType() =~ /(\d+)/);
  }
  return $rtnStr;
}


################################################
# _varDepth
################################################

#sub setVarDepth

sub getVarDepth {
  my ($self) = @_;
  return $self->{_varDepth};
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
# _testCyc
################################################

#sub setTestCyc

sub getTestCyc {
  my ($self) = @_;
  return $self->{_testCyc};
}


################################################
# _testSrcDst
################################################

#sub setTestSrcDst

sub getTestSrcDst {
  my ($self) = @_;
  return $self->{_testSrcDst};
}


################################################
# _uniqStr
################################################

sub genUniqStr {
  my ($self) = @_;
  my $svu = $self->getVarClass();
  if ($svu eq "VAR") {
    $self->{_uniqStr} = "v".$self->genVarUniq();
  } elsif ($svu eq "STR") {
    $self->{_uniqStr} = "s".$self->genStrUniq();
  } elsif ($svu eq "UNN") {
    $self->{_uniqStr} = "u".$self->genUnnUniq();
  }
  return $self->{_uniqStr};
}

sub genVarUniq {
  my ($self) = @_;
  my $rtn = ${$self->{_zS_VAR_UNIQ}};
  ${$self->{_zS_VAR_UNIQ}}++;
  return $rtn;
}

sub genStrUniq {
  my ($self) = @_;
  my $rtn = ${$self->{_zS_STR_UNIQ}};
  ${$self->{_zS_STR_UNIQ}}++;
  return $rtn;
}

sub genUnnUniq {
  my ($self) = @_;
  my $rtn = ${$self->{_zS_UNN_UNIQ}};
  ${$self->{_zS_UNN_UNIQ}}++;
  return $rtn;
}

sub resetUniq {
  my ($self) = @_;
  ${$self->{_zS_VAR_UNIQ}} = 0;
  ${$self->{_zS_STR_UNIQ}} = 0;
  ${$self->{_zS_UNN_UNIQ}} = 0;
  #delete $self->{_zS_VAR_UNIQ};
  #delete $self->{_zS_STR_UNIQ};
  #delete $self->{_zS_UNN_UNIQ};
  return 0;
}

sub getUniqStr {
  my ($self) = @_;
  return $self->{_uniqStr};
}


################################################
# _varList
################################################

sub pushToList {
  my ($self) = @_;
  unless (($self->getVarClass() ne "VAR") && ($self->getVarAnon == 1)) {
    push(@{$self->{_zS_VAR_LIST}{$self->getVarClass()}}, $self);
    push(@{$self->{_zS_VAR_LIST}{Size}{$self->getBaseVarSize()}}, $self);
  }
  return 1;
}

sub removeFromList {
  my ($self) = @_;

  my $idx = 0;
  my $arrRef = $self->{_zS_VAR_LIST}{$self->getVarClass()};
  $idx++ until ($arrRef->[$idx]->getVarName() eq $self->getVarName());
  splice(@$arrRef, $idx, 1);
  delete $self->{_zS_VAR_LIST}{$self->getVarClass()} if ($#$arrRef == -1);

  $idx = 0;
  $arrRef = $self->{_zS_VAR_LIST}{Size}{$self->getBaseVarSize()};
  $idx++ until ($arrRef->[$idx]->getVarName() eq $self->getVarName());
  splice(@$arrRef, $idx, 1);
  delete $self->{_zS_VAR_LIST}{Size}{$self->getBaseVarSize()} if ($#$arrRef == -1);
  return 1;
}

sub getVarList {
  my ($self) = @_;
  return $self->{_zS_VAR_LIST};
}

sub getAggVarList {
  my ($self) = @_;

  my $listRef = $self->getVarList();
  my @tempList = ();

  foreach my $var (@{$listRef->{VAR}}) {
    push(@tempList, $var);
  }

  if ($ENB_RtnVar_STR) {
    if (defined($listRef->{STR})) {
      foreach my $str (@{$listRef->{STR}}) {
	push(@tempList, $str);
      }
    }
  }

  if ($ENB_RtnVar_UNN) {
    if (defined($listRef->{UNN})) {
      foreach my $unn (@{$listRef->{UNN}}) {
	push(@tempList, $unn);
      }
    }
  }

  return \@tempList;
}

sub getVarFromList {
  my ($self, $testRef) = @_;
  my $listRef = $self->getAggVarList();
  my $rtnVar = $listRef->[rand @$listRef];
  my $watchDog = 0;
  while (
	 ($rtnVar->getActVarSize() > $CNT_MaxQWPerCyc_MAX*64) ||
	 (($rtnVar->getVarClass() eq "VAR") && ($rtnVar->getVarBitFType() ne ""))
	) {
    # Force us to pick something that will fit...
    $rtnVar = $listRef->[rand @$listRef];
    $watchDog++;
    if (($watchDog > 100) && ($rtnVar->getVarClass eq "VAR")) {
      $rtnVar->removeFromList();
      $rtnVar->setDim1Size(0);
      $rtnVar->setDim2Size(0);
      $rtnVar->setVarBitFType("");
      $rtnVar->pushToList();
    }
  }
  $watchDog = 0;

  if ($testRef == 0) {
    # Determine which fields are used
    $rtnVar->markUsedFields();
    $rtnVar->genVarIdx();
  }

  else {
    # Check for same var
    my $cyc = $self->getTestCyc();
    my $srcVar = $testRef->getSrcVar($cyc);
    if (&genChance($PCT_GetVarFromList_FROMSRC, 1, 0)) {
      $listRef = $srcVar->getAggVarList();
      $rtnVar = $listRef->[rand @$listRef];
      $watchDog = 0;
      while (
	     ($rtnVar->getActVarSize() > $CNT_MaxQWPerCyc_MAX*64) ||
	     (($rtnVar->getVarClass() eq "VAR") && ($rtnVar->getVarBitFType() ne ""))
	    ) {
	# Force us to pick something that will fit...
	$rtnVar = $listRef->[rand @$listRef];
	$watchDog++;
	if (($watchDog > 100) && ($rtnVar->getVarClass eq "VAR")) {
	  $rtnVar->removeFromList();
	  $rtnVar->setDim1Size(0);
	  $rtnVar->setDim2Size(0);
	  $rtnVar->setVarBitFType("");
	  $rtnVar->pushToList();
	}
      }
      $watchDog = 0;
    }

    # Dst, modify so that types match!!!
    # Var->Var
    if (($srcVar->getVarClass eq "VAR") && ($rtnVar->getVarClass eq "VAR")) {
      $rtnVar->markUsedFields();
      $rtnVar->genVarIdx();
      $rtnVar->removeFromList();
      if (($srcVar->getVarType !~ /int\d+_t/) && ($rtnVar->getVarType =~ /int\d+_t/)) {
	# Special case - SrcVar is CST type / RtnVar is STD type
	$srcVar->removeFromList();
	$srcVar->modVarType($rtnVar->getVarType());
	$srcVar->pushToList();
      } else {
	# Standard case, copy from SrcVar
	$rtnVar->modVarType($srcVar->getVarType());
      }
      my $newVar = Clone::clone($rtnVar);
      $rtnVar = $newVar;
      $rtnVar->pushToList();
    }

    # ???->Var
    elsif ($srcVar->getVarClass ne "VAR") {
      $rtnVar = Clone::clone($srcVar);
    }

    # Var->???
    elsif ($rtnVar->getVarClass ne "VAR") {
      # FIXME? Best way to overwrite srcVar? Or just not allow this case?
      $srcVar->unmarkUsedFields();
      $rtnVar->markUsedFields();
      $rtnVar->genVarIdx();
      $testRef->setSrcVar($cyc, Clone::clone($rtnVar));
    }

    else {
      die "Unimplemented Src/Dst pair! $!";
    }
  }

  return $rtnVar;
}


################################################
# _varDim1
################################################

sub genVarDim1 {
  my ($self) = @_;
  $self->{_varDim1Size} = &genChance($PCT_VarDimSel_DIM, $ARR_VarDimSel[rand @ARR_VarDimSel], 0);
  $self->{_varDim1Sel} = int(rand($self->getDim1Size()));
  $self->{_varDim1DSel} = &genChance($PCT_VarDimDSel_HTD, "HTD", "SRC");
  $self->updateVarSize();
  return $self->{_varValue};
}

sub genVarDim1Sel {
  my ($self) = @_;
  $self->{_varDim1Sel} = int(rand($self->getDim1Size()));
  return $self->{_varDim1Sel};
}

sub setDim1Size {
  my ($self, $size) = @_;
  $self->{_varDim1Size} = $size;
  $self->updateVarSize();
  return $self->{_varDim1Size};
}

sub setDim1DSel {
  my ($self, $dSel) = @_;
  $self->{_varDim1DSel} = $dSel;
  return $self->{_varDim1DSel};
}

sub setDim1Sel {
  my ($self, $sel) = @_;
  $self->{_varDim1Sel} = $sel;
  return $self->{_varDim1Sel};
}

sub getDim1Size {
  my ($self) = @_;
  return $self->{_varDim1Size};
}

sub getDim1Sel {
  my ($self) = @_;
  return $self->{_varDim1Sel};
}

sub getDim1DSel {
  my ($self) = @_;
  return $self->{_varDim1DSel};
}

sub getDim1LitStr {
  my ($self) = @_;
  if ($self->getDim1Size != 0) {
    return "[".$self->{_varDim1Sel}."]";
  } else {
    return "";
  }
}

sub getIsIdxDim1 {
  my ($self) = @_;
  return $self->{_varIdxDim1};
}

sub setIsIdxDim1 {
  my ($self, $set) = @_;
  $self->{_varIdxDim1} = $set;
  return $self->{_varIdxDim1};
}

sub getDim1LitDefStr {
  my ($self) = @_;
  if ($self->getDim1Size != 0) {
    return "[".$self->{_varDim1Size}."]";
  } else {
    return "";
  }
}

sub getDim1LitStrHTD {
  my ($self) = @_;
  if ($self->getDim1Size != 0) {
    my $sel = ($self->getDim1DSel() eq "HTD") ? $self->{_varDim1Sel} : ($self->getIsIdxDim1() == 1) ? "#" : "";
    return "[${sel}]";
  } else {
    return "";
  }
}

sub getMemDimStr {
  my ($self) = @_;
  if ($self->getDim1Size() != 0) {
    return sprintf("%s%s",
		   $self->getMemDim1Str(),
		   $self->getMemDim2Str()
		  );
  } else {
    return "";
  }
}

sub getMemDim1Str {
  my ($self) = @_;
  if ($self->getDim1Size() && ($self->getDim1DSel() eq "SRC")) {
    return $self->getDim1Str();
  } else {
    return "";
  }
}

sub getDim1Str {
  my ($self) = @_;
  if ($self->getDim1Size != 0) {
    if ($self->getElemInfo() != -1) {
      return sprintf(", %d",
		     $self->getElemInfo()->{Dim1}
		    );
    } else {
      return sprintf(", %d",
		     $self->getDim1Sel()
		    );
    }
  } else {
    return "";
  }
}

sub getDim1Arr {
  my ($self, $rtnArrRef) = @_;
  if ($self->getDim1Size() == 0 || $self->getVarAnon() == 1) {
    push(@$rtnArrRef, '-');
  } else {
    for(my $idx = 0; $idx < $self->getDim1Size(); $idx++) {
      push(@$rtnArrRef, $idx);
    }
  }
  return;
}


################################################
# _varDim2
################################################

sub genVarDim2 {
  my ($self) = @_;
  $self->{_varDim2Size} = &genChance($PCT_VarDimSel_DIM, $ARR_VarDimSel[rand @ARR_VarDimSel], 0);
  $self->{_varDim2Size} = 0 if ($self->getDim1Size() == 0);
  $self->{_varDim2Sel} = int(rand($self->getDim2Size()));
  $self->{_varDim2DSel} = &genChance($PCT_VarDimDSel_HTD, "HTD", "SRC");
  $self->updateVarSize();
  return $self->{_varValue};
}

sub genVarDim2Sel {
  my ($self) = @_;
  $self->{_varDim2Sel} = int(rand($self->getDim2Size()));
  return $self->{_varDim2Sel};
}

sub setDim2Size {
  my ($self, $size) = @_;
  $self->{_varDim2Size} = $size;
  $self->updateVarSize();
  return $self->{_varDim2Size};
}

sub setDim2DSel {
  my ($self, $dSel) = @_;
  $self->{_varDim2DSel} = $dSel;
  return $self->{_varDim2DSel};
}

sub setDim2Sel {
  my ($self, $sel) = @_;
  $self->{_varDim2Sel} = $sel;
  return $self->{_varDim2Sel};
}

sub getDim2Size {
  my ($self) = @_;
  return $self->{_varDim2Size};
}

sub getDim2Sel {
  my ($self) = @_;
  return $self->{_varDim2Sel};
}

sub getDim2DSel {
  my ($self) = @_;
  return $self->{_varDim2DSel};
}

sub getDim2LitStr {
  my ($self) = @_;
  if ($self->getDim2Size != 0) {
    return "[".$self->{_varDim2Sel}."]";
  } else {
    return "";
  }
}

sub getIsIdxDim2 {
  my ($self) = @_;
  return $self->{_varIdxDim2};
}

sub setIsIdxDim2 {
  my ($self, $set) = @_;
  $self->{_varIdxDim2} = $set;
  return $self->{_varIdxDim2};
}

sub getDim2LitDefStr {
  my ($self) = @_;
  if ($self->getDim2Size != 0) {
    return "[".$self->{_varDim2Size}."]";
  } else {
    return "";
  }
}

sub getDim2LitStrHTD {
  my ($self) = @_;
  if ($self->getDim2Size != 0) {
    my $sel = ($self->getDim2DSel() eq "HTD") ? $self->{_varDim2Sel} : ($self->getIsIdxDim2() == 1) ? "#" : "";
    return "[${sel}]";
  } else {
    return "";
  }
}

sub getMemDim2Str {
  my ($self) = @_;
  if ($self->getDim2Size() && ($self->getDim2DSel() eq "SRC")) {
    return $self->getDim2Str();
  } else {
    return "";
  }
}

sub getDim2Str {
  my ($self) = @_;
  if ($self->getDim2Size != 0) {
    if ($self->getElemInfo() != -1) {
      return sprintf(", %d",
		     $self->getElemInfo()->{Dim2}
		    );
    } else {
      return sprintf(", %d",
		     $self->getDim2Sel()
		    );
    }
  } else {
    return "";
  }
}

sub getDim2Arr {
  my ($self, $rtnArrRef) = @_;
  if ($self->getDim2Size() == 0 || $self->getVarAnon() == 1) {
    push(@$rtnArrRef, '-');
  } else {
    for(my $idx = 0; $idx < $self->getDim2Size(); $idx++) {
      push(@$rtnArrRef, $idx);
    }
  }
  return;
}


################################################
# _varAddr1
################################################

sub genVarAddr1 {
  my ($self) = @_;
  $self->{_varAddr1Width} = &genChance($PCT_VarAddrSel_ADDR, $ARR_VarAddrWidth[rand @ARR_VarAddrWidth], 0);
  $self->{_varAddr1Sel} = int(rand(2**$self->getAddr1Width()));
  $self->{_varAddr1DSel}  = &genChance($PCT_VarAddrDSel_HTD, "HTD", "SRC");

  if (($self->getAddr1Width() != 0) && ($self->getVarPSG() =~ /[PG]/)) {
    $self->{_varAddr1Var}{PSG} = "P";
    $self->{_varAddr1Var}{Name} = sprintf("%s_RdAddr1", $self->getVarName());
    $self->{_varAddr1Var}{Type} = sprintf("ht_uint%d", $self->getAddr1Width());
  }
  return 0;
}

sub genVarAddr1Sel {
  my ($self) = @_;
  $self->{_varAddr1Sel} = int(rand(2**$self->getAddr1Width()));
  return $self->{_varAddr1Sel};
}

sub getIsIdxAddr1 {
  my ($self) = @_;
  return $self->{_varIdxAddr1};
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->getIsIdxAddr1();
  } else {
    return $self->{_varIdxAddr1};
  }
}

sub setIsIdxAddr1 {
  my ($self, $set) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->setIsIdxAddr1($set);
  } else {
    $self->{_varIdxAddr1} = $set;
    return $self->{_varIdxAddr1};
  }
}

sub getAddr1Width {
  my ($self) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->getAddr1Width();
  } else {
    return $self->{_varAddr1Width};
  }
}

sub getAddr1Sel {
  my ($self) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->getAddr1Sel();
  } else {
    return $self->{_varAddr1Sel};
  }
}

sub getAddr1DSel {
  my ($self) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->getAddr1DSel();
  } else {
    return $self->{_varAddr1DSel};
  }
}

sub setAddr1DSel {
  my ($self, $dSel) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->setAddr1DSel($dSel);
  } else {
    $self->{_varAddr1DSel} = $dSel;
    return $self->{_varAddr1DSel};
  }
}

sub getAddr1Var {
  my ($self) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->getAddr1Var();
  } else {
    if (defined($self->{_varAddr1Var})) {
      return $self->{_varAddr1Var};
    } else {
      return 0;
    }
  }
}

sub getAddr1VarReadStr {
  my ($self, $stg) = @_;
  if ($self->getAddr1Var()) {
    my $addrVarRef = $self->getAddr1Var();
    return sprintf("%s%s_%s = (%s)%d;\n",
		   $addrVarRef->{PSG},
		   $stg,
		   $addrVarRef->{Name},
		   $addrVarRef->{Type},
		   $self->getAddr1Sel()
		  );
  } else {
    return "";
  }
}

sub getReadAddrStr {
  my ($self, $stg) = @_;
  if ($self->getAddr1Width() && ($self->getVarPSG() eq "S")) {
    return sprintf("%s%s%s_%s.read_addr(%d%s);\n",
		   $self->getVarPSG(),
		   $self->getRandRARdSfx(),
		   $self->getMaskedStage($stg),
		   $self->getVarTopParent()->getVarNameWDim(),
		   $self->getAddr1Sel(),
		   $self->getAddr2VStr()
		  );
  } else {
    return "";
  }
}

sub getWriteAddrStr {
  my ($self, $stg) = @_;
  if ($self->getAddr1Width()) {
    return sprintf("%s%s%s_%s.write_addr(%d%s);\n",
		   $self->getVarPSG(),
		   $self->getRandRAWrSfx(),
		   $self->getMaskedStage($stg),
		   $self->getVarTopParent()->getVarNameWDim(),
		   $self->getAddr1Sel(),
		   $self->getAddr2VStr()
		  );
  } else {
    return "";
  }
}

sub getMemAddrStr {
  my ($self) = @_;
  if ($self->getAddr1Width()) {
    return sprintf("%s%s",
		   $self->getMemAddr1Str(),
		   $self->getMemAddr2Str()
		  );
  } else {
    return "";
  }
}

sub getAddr1Str {
  my ($self) = @_;
  if ($self->getAddr1Width()) {
    if (($self->getElemInfo() != -1) && (exists $self->getElemInfo()->{Addr1})) {
      return sprintf(", %d",
		     $self->getElemInfo()->{Addr1}
		    );
    } else {
      return sprintf(", %d",
		     $self->getAddr1Sel()
		    );
    }
  } else {
    return "";
  }
}

sub getMemAddr1Str {
  my ($self) = @_;
  if ($self->getAddr1Width() && ($self->getAddr1DSel() eq "SRC")) {
    return $self->getAddr1Str();
  } else {
    return "";
  }
}

sub getAddr1LitStrHTD {
  my ($self) = @_;
  if ($self->getAddr1Width != 0) {
    my $sel = ($self->getAddr1DSel() eq "HTD") ? $self->{_varAddr1Sel} : ($self->getIsIdxAddr1() == 1) ? "#" : "";
    return "(${sel}";
  } else {
    return "";
  }
}


################################################
# _varAddr2
################################################

sub genVarAddr2 {
  my ($self) = @_;
  $self->{_varAddr2Width} = &genChance($PCT_VarAddrSel_ADDR, $ARR_VarAddrWidth[rand @ARR_VarAddrWidth], 0);
  $self->{_varAddr2Width} = 0 if ($self->getAddr1Width() == 0);
  $self->{_varAddr2Sel} = int(rand(2**$self->getAddr2Width()));
  $self->{_varAddr2DSel}  = &genChance($PCT_VarAddrDSel_HTD, "HTD", "SRC");

  if (($self->getAddr2Width() != 0) && ($self->getVarPSG() =~ /[PG]/)) {
    $self->{_varAddr2Var}{PSG} = "P";
    $self->{_varAddr2Var}{Name} = sprintf("%s_RdAddr2", $self->getVarName());
    $self->{_varAddr2Var}{Type} = sprintf("ht_uint%d", $self->getAddr2Width());
  }
  return 0;
}

sub genVarAddr2Sel {
  my ($self) = @_;
  $self->{_varAddr2Sel} = int(rand(2**$self->getAddr2Width()));
  return $self->{_varAddr2Sel};
}

sub getIsIdxAddr2 {
  my ($self) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->getIsIdxAddr2();
  } else {
    return $self->{_varIdxAddr2};
  }
}

sub setIsIdxAddr2 {
  my ($self, $set) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->setIsIdxAddr2($set);
  } else {
    $self->{_varIdxAddr2} = $set;
    return $self->{_varIdxAddr2};
  }
}

sub getAddr2Width {
  my ($self) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->getAddr2Width();
  } else {
    return $self->{_varAddr2Width};
  }
}

sub getAddr2Sel {
  my ($self) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->getAddr2Sel();
  } else {
    return $self->{_varAddr2Sel};
  }
}

sub getAddr2DSel {
  my ($self) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->getAddr2DSel();
  } else {
    return $self->{_varAddr2DSel};
  }
}

sub setAddr2DSel {
  my ($self, $dSel) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->setAddr2DSel($dSel);
  } else {
    $self->{_varAddr2DSel} = $dSel;
    return $self->{_varAddr2DSel};
  }
}

sub getAddr2Var {
  my ($self) = @_;
  if ($self->getVarParent() != 0) {
    return $self->getVarTopParent()->getAddr2Var();
  } else {
    if (defined($self->{_varAddr2Var})) {
      return $self->{_varAddr2Var};
    } else {
      return 0;
    }
  }
}

sub getAddr2VarReadStr {
  my ($self, $stg) = @_;
  if ($self->getAddr2Var()) {
    my $addrVarRef = $self->getAddr2Var();
    return sprintf("%s%s_%s = (%s)%d;\n",
		   $addrVarRef->{PSG},
		   $stg,
		   $addrVarRef->{Name},
		   $addrVarRef->{Type},
		   $self->getAddr2Sel()
		  );
  } else {
    return "";
  }
}

sub getAddr2VStr {
  my ($self) = @_;
  if ($self->getAddr2Width()) {
    return sprintf(", %d",
		   $self->getAddr2Sel()
		  );
  } else {
    return "";
  }
}

sub getAddr2MStr {
  my ($self) = @_;
  if ($self->getAddr2Width()) {
    if (($self->getElemInfo() != -1) && (exists $self->getElemInfo()->{Addr2})) {
      return sprintf(", %d",
		     $self->getElemInfo()->{Addr2}
		    );
    } else {
      return sprintf(", %d",
		     $self->getAddr2Sel()
		    );
    }
  } else {
    return "";
  }
}

sub getMemAddr2Str {
  my ($self) = @_;
  if ($self->getAddr2Width() && ($self->getAddr2DSel() eq "SRC")) {
    return $self->getAddr2MStr;
  } else {
    return "";
  }
}

sub getAddr2LitStrHTD {
  my ($self) = @_;
  if ($self->getAddr2Width != 0) {
    my $sel = ($self->getAddr2DSel() eq "HTD") ? $self->{_varAddr2Sel} : ($self->getIsIdxAddr2() == 1) ? "#" : "";
    return ",${sel})";
  } elsif ($self->getAddr1Width != 0) {
    return ")";
  } else {
    return "";
  }
}


################################################
# varIdx
################################################

sub genVarIdx {
  my ($self) = @_;

  # 0 -> NONE
  # 1 -> Addr
  # 2 -> Dim
  my $idxSw = 0;

  unless (($self->getAddr1Width == 0) && ($self->getDim1Size == 0)) {
    # Something exists

    my $addrPCT = $PCT_GenVarIdx_ADR;
    if (($self->getVarParent() != 0) || ($self->getVarClass() ne "VAR")) {
      $addrPCT = 0;
    }

    $idxSw = &genChance($addrPCT, 1, 2);
    $idxSw = 1 if (($self->getDim1Size == 0) && ($addrPCT != 0));
    $idxSw = 2 if ($self->getAddr1Width == 0);

    if ($idxSw == 1) {
      # Addr
      if ($self->getAddr2Width != 0) {
	$self->setIsIdxAddr2(&genChance($PCT_GenVarIdx_A2, 1, 0));
      } else {
	$self->setIsIdxAddr2(0);
      }
      if (($self->getAddr1Width != 0) && ($self->getIsIdxAddr2 != 0)) {
	$self->setIsIdxAddr1(&genChance($PCT_GenVarIdx_A1, 1, 0));
      } else {
	$self->setIsIdxAddr1(0);
      }
      $self->setIsIdxDim1(0);
      $self->setIsIdxDim2(0);
    }

    elsif ($idxSw == 2) {
      # Dim
      if ($self->getDim2Size != 0) {
	$self->setIsIdxDim2(&genChance($PCT_GenVarIdx_D2, 1, 0));
      } else {
	$self->setIsIdxDim2(0);
      }
      if (($self->getDim1Size != 0) && ($self->getIsIdxDim2 != 0)) {
	$self->setIsIdxDim1(&genChance($PCT_GenVarIdx_D1, 1, 0));
      } else {
	$self->setIsIdxDim1(0);
      }
      $self->setIsIdxAddr1(0);
      $self->setIsIdxAddr2(0);
    }

  } else {
    $self->setIsIdxAddr1(0);
    $self->setIsIdxAddr2(0);
    $self->setIsIdxDim1(0);
    $self->setIsIdxDim2(0);
  }

  $self->setAddr1DSel("HTD") if ($self->getIsIdxAddr1() != 0);
  $self->setAddr2DSel("HTD") if ($self->getIsIdxAddr2() != 0);
  $self->setDim1DSel("HTD") if ($self->getIsIdxDim1() != 0);
  $self->setDim2DSel("HTD") if ($self->getIsIdxDim2() != 0);
  return;
}


################################################
# _varElemInfo
################################################

sub setElemInfo {
  my ($self, $infoRef) = @_;
  $self->{_varElemInfo} = $infoRef;
  return $self->{_varElemInfo};
}

sub getElemInfo {
  my ($self) = @_;
  return $self->{_varElemInfo};
}


################################################
# _varEleMem
################################################

sub getEleMemStr {
  my ($self) = @_;
  if (
      (($self->getAddr1Width() != 0) || ($self->getDim1Size() != 0)) &&
      (($self->getIsIdxAddr1() == 1) || ($self->getIsIdxAddr2() == 1) || ($self->getIsIdxDim1() == 1) || ($self->getIsIdxDim2() == 1) )
     ) {
    if ($self->getElemInfo() != -1) {
      return sprintf(", %d",
		     $self->getElemInfo()->{TotElemCnt}
		    );
    } else {
      return ", 1";
    }
  } else {
    return "";
  }
}


################################################
# _varGlbRdStg
################################################

sub setVarGlbRdStg {
  my ($self, $stg) = @_;
  $self->{_varGlbRdStg} = $stg;
  return $self->{_varGlbRdStg};
}

sub getVarGlbRdStg {
  my ($self) = @_;
  return $self->{_varGlbRdStg};
}

sub getMaskedStage {
  my ($self, $stg) = @_;
  if ($self->getVarPSG() eq "S") {
    return "";
  } else {
    return $stg;
  }
}

################################################
# _varGlbWrStg
################################################

sub setVarGlbWrStg {
  my ($self, $stg) = @_;
  $self->{_varGlbWrStg} = $stg;
  return $self->{_varGlbWrStg};
}

sub getVarGlbWrStg {
  my ($self) = @_;
  return $self->{_varGlbWrStg};
}


################################################
# _varGlbInstW
################################################

sub setVarGlbInstW {
  my ($self, $instW) = @_;
  $self->{_varGlbInstW} = $instW;
  return $self->{_varGlbInstW};
}

sub getVarGlbInstW {
  my ($self) = @_;
  return $self->{_varGlbInstW};
}

sub getVarGlbInstWStr {
  my ($self) = @_;
  if ($self->getVarGlbInstW() == 0) {
    return "false";
  } elsif ($self->getVarGlbInstW() == 1) {
    return "true";
  } else {
    return "";
  }
}


################################################
# _varGlbInstR
################################################

sub setVarGlbInstR {
  my ($self, $instR) = @_;
  $self->{_varGlbInstR} = $instR;
  return $self->{_varGlbInstR};
}

sub getVarGlbInstR {
  my ($self) = @_;
  return $self->{_varGlbInstR};
}

sub getVarGlbInstRStr {
  my ($self) = @_;
  if ($self->getVarGlbInstR() == 0) {
    return "false";
  } elsif ($self->getVarGlbInstR() == 1) {
    return "true";
  } else {
    return "";
  }
}


################################################
# _varParent
################################################

#sub setVarParent

sub getVarParent {
  my ($self) = @_;
  return $self->{_varParent};
}

sub getVarTopParent {
  my ($self) = @_;
  return $self->getVarChain()->[0];
}


################################################
# _varFields
################################################

sub genVarFields {
  my ($self) = @_;
  undef $self->{_varFields};
  $self->{_varFields} = ();
  if ($self->getVarClass() ne "VAR") {
    my $numFields = int(rand($CNT_VarFields_MAX))+1;
    for (my $idx = 0; $idx < $numFields; $idx++) {
      my $newVar = new HTVar(
			     $self,                  # Parent
			     $self->getTestStr(),    # TestStr
			     $self->getTestCyc(),    # TestStr
			     $self->getVarDepth()+1, # Depth
			     $self->getTestSrcDst,   # TestSrcDst
			     0,                      # SrcVar
			     $self->getVarList()     # VarListRef
			    );
      push(@{$self->{_varFields}}, $newVar);
    }
    $self->updateFieldSize();
  }
  return $self->{_varFields};
}

sub getVarFields {
  my ($self) = @_;
  return $self->{_varFields};
}




# Why do .pm files need this, I'll never know
1;
