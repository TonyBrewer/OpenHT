#!/usr/bin/perl

$isV5 = 1;
$model = $isV5 ? "14" : "17";
$hier = $isV5 ? "/" : " ";

#
# Header
#
print <<EOF;
#ChipScope Core Inserter Project File Version 3.0
Project.device.designInputFile=cae_fpga.ngc
Project.device.designOutputFile=cae_fpga.ngo
Project.device.deviceFamily=$model
Project.device.enableRPMs=true
Project.device.outputDirectory=_ngo
Project.device.useSRL16=true
Project.filter.dimension=1
Project.filter<0>=
Project.icon.boundaryScanChain=1
Project.icon.enableExtTriggerIn=false
Project.icon.enableExtTriggerOut=false
Project.icon.triggerInPinName=
Project.icon.triggerOutPinName=
Project.unit.dimension=1
Project.unit<0>.clockChannel=ae_top/core${hier}cae_pers clk
Project.unit<0>.clockEdge=Rising
Project.unit<0>.dataDepth=16384
Project.unit<0>.dataEqualsTrigger=false
Project.unit<0>.enableGaps=false
Project.unit<0>.enableStorageQualification=true
Project.unit<0>.enableTimestamps=false
Project.unit<0>.timestampDepth=0
Project.unit<0>.timestampWidth=0
Project.unit<0>.triggerConditionCountWidth=0
Project.unit<0>.triggerMatchCount<0>=4
Project.unit<0>.triggerMatchCountWidth<0><0>=13
Project.unit<0>.triggerMatchCountWidth<0><1>=13
Project.unit<0>.triggerMatchCountWidth<0><2>=13
Project.unit<0>.triggerMatchCountWidth<0><3>=13
Project.unit<0>.triggerMatchType<0><0>=0
Project.unit<0>.triggerMatchType<0><1>=0
Project.unit<0>.triggerMatchType<0><2>=0
Project.unit<0>.triggerMatchType<0><3>=0
Project.unit<0>.triggerPortCount=1
Project.unit<0>.triggerPortIsData<0>=true
Project.unit<0>.triggerSequencerLevels=16
Project.unit<0>.triggerSequencerType=1
Project.unit<0>.type=ilapro
EOF

###############################################################################

$aucnt = 1;

#
# trigger
#
$out=""; $c=0;

$per="ae_top/core${hier}cae_pers";

for ($u=0; $u<$aucnt; $u++) {
    $au="$per top pPersAuTop\$$u";
    #							start	len
    push @tri, ["$au pPersHif r_iBlkState", 0, 2];
    push @tri, ["$au pPersHif r_iBlkRdTids", 0, 8];
    push @tri, ["$au pPersHif r_iCtlState", 0, 2];
    push @tri, ["$au pPersHif r_oBlkState", 0, 3];
    push @tri, ["$au pPersHif r_oCtlState", 0, 2];

    push @tri, ["$au pPersAuCtl r_t5_htValid", -1, 0];
    push @tri, ["$au pPersAuCtl r_t5_htInstr", 0, 3];

    push @tri, ["$au pPersAuSch r_t4_htValid", -1, 0];
    push @tri, ["$au pPersAuSch r_t4_htInstr", -1, 0];

    push @tri, ["$au pPersAuPkt r_t6_htValid", -1, 0];
    push @tri, ["$au pPersAuPkt r_t6_htInstr", 0, 6];

    push @tri, ["$au pPersAuMif0 r_mifToXbar_reqRdy", -1, 0];
    push @tri, ["$au pPersAuMif0 r_mifToXbar_req", 115, 2];
}
push @tri, ["$per dis/r_st_idle",			-1,	0];

$base = "Project.unit<0>.triggerChannel<0><%d>=";
for ($s=0; $s<@tri; $s++) {
    for ($i=$tri[$s][1]; $i<($tri[$s][2]==0?0:$tri[$s][1]+$tri[$s][2]); $i++) {
	$out .= sprintf "${base}%s", $c++, $tri[$s][0];
	$out .= sprintf "[$i]" if $tri[$s][1] >= 0;
	$out .= sprintf "\n";
    }
}

printf "Project.unit<0>.triggerPortWidth<0>=%d\n$out", $c;

#
# data
#
$out=""; $c=0;

@dat = ();
for ($u=0; $u<$aucnt; $u++) {
    $au="$per top pPersAuTop\$$u";

    push @dat, ["$au pPersHif r_iCtlWrPend", -1, 0];
    push @dat, ["$au pPersHif r_oCtlWrPend", -1, 0];

    push @dat, ["$au pPersHif r_iBlkRdyQue_RdIdx", 0, 5];
    push @dat, ["$au pPersHif r_iBlkRdyQue_WrIdx", 0, 5];
    push @dat, ["$au pPersHif r_iBlkRdCnt", 0, 18];
    push @dat, ["$au pPersHif r_iBlkRdIdx", 0, 4];

    push @dat, ["$au pPersHif r_oBlkAvlCnt", 0, 5];
    push @dat, ["$au pPersHif r_oBlkExpired", -1, 0];
    push @dat, ["$au pPersHif r_oBlkWrAdr", 0, 8];
    push @dat, ["$au pPersHif r_oBlkWrCnt", 0, 18];
    push @dat, ["$au pPersHif r_oBlkWrCntEnd", 0, 18];
    push @dat, ["$au pPersHif r_oBlkWrCntEnd64", 0, 18];
    push @dat, ["$au pPersHif r_oBlkWrDat_WrIdx", 0, 4];
    push @dat, ["$au pPersHif r_oBlkWrDat_RdIdx", 0, 4];
    push @dat, ["$au pPersHif r_oBlkWrIdx", 0, 4];
    push @dat, ["$au pPersHif r_oBlkRdPendCnt\$0", 0, 4];
    push @dat, ["$au pPersHif r_oBlkRdPendCnt\$1", 0, 4];
    push @dat, ["$au pPersHif r_oBlkWrPendCnt\$0", 0, 8];
    push @dat, ["$au pPersHif r_oBlkWrPendCnt\$1", 0, 8];
    push @dat, ["$au pPersHif r_hifToMif_reqAvlCnt", 0, 6];

    push @dat, ["$au pPersAuCtl r_rdGrpRspCnt", 0, 8];
    push @dat, ["$au pPersAuCtl r_t5_rdRspPauseCntZero", -1, 0];

    push @dat, ["$au pPersAuSch r_t4_htId", 0, 4];

    push @dat, ["$au pPersAuPkt r_t6_htId", 0, 4];
    push @dat, ["$au pPersAuPkt r_rdGrpRspCnt\$0", 0, 5];
    push @dat, ["$au pPersAuPkt r_rdGrpRspCnt\$1", 0, 5];
    push @dat, ["$au pPersAuPkt r_rdGrpRspCnt\$2", 0, 5];
    push @dat, ["$au pPersAuPkt r_rdGrpRspCnt\$3", 0, 5];
    push @dat, ["$au pPersAuPkt r_rdGrpRspCntAddr\$0", 0, 4];
    push @dat, ["$au pPersAuPkt r_rdGrpRspCntAddr\$1", 0, 4];
    push @dat, ["$au pPersAuPkt r_rdGrpRspCntAddr\$2", 0, 4];
    push @dat, ["$au pPersAuPkt r_rdGrpRspCntAddr\$3", 0, 4];
    push @dat, ["$au pPersAuPkt r_t6_rdRspPauseCntZero", -1, 0];

    push @dat, ["$au pPersAuMif0 r_mifToXbar_req", 141, 2];
    push @dat, ["$au pPersAuMif0 r_mifToXbar_req", 112, 29];
    push @dat, ["$au pPersAuMif0 r_mifToXbar_req", 64, 48];
    #push @dat, ["$au pPersAuMif0 r_mifToXbar_req", 0, 64];
    push @dat, ["$au pPersAuMif0 r_mifToHif_reqAvl", -1, 0];
    push @dat, ["$au pPersAuMif0 r_mifToCtl_reqAvl", -1, 0];
    push @dat, ["$au pPersAuMif0 r_mifToPkt_reqAvl", -1, 0];
};

$base = "Project.unit<0>.dataChannel<%d>=";

for ($s=0; $s<@dat; $s++) {
    for ($i=$dat[$s][1]; $i<($dat[$s][2]==0?0:$dat[$s][1]+$dat[$s][2]); $i++) {
	$out .= sprintf "${base}%s", $c++, $dat[$s][0];
	$out .= sprintf "[$i]" if $dat[$s][1] >= 0;
	$out .= sprintf "\n";
    }
}
for ($s=0; $s<@tri; $s++) {
    my $skip = 0;
    for ($j=0; $j<@dat; $j++) {
	$skip = 1 if ($tri[$s][0] eq $dat[$j][0] &&
		      $tri[$s][1] == $dat[$j][1] &&
		      $tri[$s][2] == $dat[$j][2]);
    }
    next if $skip;

    for ($i=$tri[$s][1]; $i<($tri[$s][2]==0?0:$tri[$s][1]+$tri[$s][2]); $i++) {
	$out .= sprintf "${base}%s", $c++, $tri[$s][0];
	$out .= sprintf "[$i]" if $tri[$s][1] >= 0;
	$out .= sprintf "\n";
    }
}

printf "Project.unit<0>.dataPortWidth=%d\n$out", $c;
