#!/usr/bin/perl

# V5
if (0) { $model = "14"; }

# V6
if (0) { $model = "17"; }

# V7
if (1) { $model = "19"; }

$per = "ae0 cae0.cae_pers";

#
# Header
#
print <<EOF;
#ChipScope Core Inserter Project File Version 3.0
Project.device.designInputFile=synth.edn
Project.device.designOutputFile=synth_cs.ngc
Project.device.deviceFamily=$model
Project.device.enableRPMs=true
Project.device.outputDirectory=.
Project.device.useSRL16=true
Project.filter.dimension=1
Project.filter<0>=
Project.icon.boundaryScanChain=1
Project.icon.enableExtTriggerIn=false
Project.icon.enableExtTriggerOut=false
Project.icon.triggerInPinName=
Project.icon.triggerOutPinName=
Project.unit.dimension=1
Project.unit<0>.clockChannel=$per clk
Project.unit<0>.clockEdge=Rising
Project.unit<0>.dataDepth=4096
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

#
# trigger
#
$out=""; $c=0;

$au  = "$per top pPersAuTop\$0";

#								start	len
@tri = ();
push @tri, ["$per disp_idle",					-1,	1];

push @tri, ["$per mc_rq_vld",					0,	1];
push @tri, ["$per mc_rq_cmd",					0,	3];
push @tri, ["$per mc_rs_vld",					0,	1];
push @tri, ["$per mc_rs_cmd",					0,	3];

push @tri, ["$au pPersHif r_iCtlState",				0,	2];
push @tri, ["$au pPersHif r_oCtlState",				0,	2];
push @tri, ["$au pPersHif r_iBlkState",				0,	2];
push @tri, ["$au pPersHif r_oBlkState",				0,	3];

#
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

#								start	len
@dat = ();
push @dat, ["$per mc_rq_data",					0,	64];
push @dat, ["$per mc_rq_rtnctl",				0,	31];
push @dat, ["$per mc_rq_scmd",					0,	3];
push @dat, ["$per mc_rq_size",					0,	2];
push @dat, ["$per mc_rq_stall",					0,	1];
push @dat, ["$per mc_rq_vadr",					0,	48];
push @dat, ["$per mc_rs_data",					0,	64];
push @dat, ["$per mc_rs_rtnctl",				3,	24];
push @dat, ["$per mc_rs_scmd",					0,	3];

#
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
