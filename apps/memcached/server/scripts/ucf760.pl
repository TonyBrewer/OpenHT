#!/usr/bin/perl

$b = "ae_top/core/cae_pers/top/pPersAuTop\$%d/pPers";
$m = "Mram_mem";

$pktRspCmd = "${b}Pkt/r_t7_\*RspCmd\*";
$pktAccCnt = "${b}Pkt/r_accQueCnt\*";
$pktRegs = "${b}Pkt/r_\*";
$accRegs = "${b}Acc/r_\*";
@LUTMap = (8,63,79,127,143,188,204,252,268,324);

#							X	Y	36?
@brams = (
[ "${b}Pkt/m_connList_pConn\$0/${m}",			0,	4*0,	1],
[ "${b}Pkt/m_connList_pConn\$1/${m}",			0,	4*1,	1],
[ "${b}Pkt/m_connList_pConn\$2/${m}",			0,	4*2,	1],
[ "${b}Pkt/m_connList_pConn\$3/${m}",			0,	4*3,	1],
[ "${b}Pkt/m_connList_pConn\$4/${m}",			0,	4*4,	1],
[ "${b}Pkt/m_connList_pConn\$5/${m}",			0,	4*5,	1],
[ "${b}Sch/m_queDat/${m}1",				0,	4*7,	1],
[ "${b}Sch/m_queDat/${m}2",				0,	4*8,	0],

[ "${b}Acc/m_ramDat/${m}1",                             2,      4*1,    1],
[ "${b}Acc/m_ramDat/${m}2",                             2,      4*1+2,  1],
[ "${b}Acc/m_ramDat/${m}3",                             2,      4*2,    1],
[ "${b}Acc/m_ramDat/${m}6",                             2,      4*2+2,  0],
[ "${b}Acc/m_ramDat/${m}4",                             2,      4*3,    1],
[ "${b}Acc/m_ramDat/${m}5",                             2,      4*3+2,  1],

[ "${b}Pkt/m_connList_pConn\$10/${m}",			1,	4*0,	1],
[ "${b}Pkt/m_connList_pConn\$9/${m}",			1,	4*0+2,	1],
[ "${b}Pkt/m_prePktDat_qw/${m}",    			1,	4*1+2,	1],
[ "${b}Pkt/m_rdPktDat_qw/${m}",    			1,	4*2+0,	1],
[ "${b}Pkt/m_connList_pConn\$8/${m}",			1,	4*3,	1],
[ "${b}Pkt/m_connList_pConn\$7/${m}",			1,	4*3+2,	1],
[ "${b}Pkt/m_connList_pConn\$6/${m}",			1,	4*4,	1],
[ "${b}Pkt/m_hashResA/${m}",    			1,	4*5+0,	0],
[ "${b}Pkt/m_hashResB/${m}",    			1,	4*5+1,	0],
[ "${b}Pkt/m_hashResC/${m}",    			1,	4*5+2,	0],

);

#RAMB18 coordinates;	760=>10x144(9*16)
#			330=>6x96
my $s = 2*8;
my $o = 96;
@orig = (
    [0, -143+0*$s+$o], 
    [0,   -1+0*$s+$o],
    [-9,   -1+0*$s+$o], 
    [-9, -143+0*$s+$o],
    );

for (my $t=0; $t<($#orig+1); $t++) {
    # current y for x
    my %cy = ();
    my $minPktXLUT = 1000;
    my $minPktYLUT = 1000;
    my $maxPktXLUT = -1;
    my $maxPktYLUT = -1;
    my $minAccXLUT = 1000;
    my $minAccYLUT = 1000;
    my $maxAccXLUT = -1;
    my $maxAccYLUT = -1;


    for (my $r=0; $r<@brams; $r++) {
	my $rx = $brams[$r][1];
	my $ry = $brams[$r][2];
	my $rz = $brams[$r][3];

	my $x, $y;
	if ($orig[$t][0] < 0) {
	    $x = abs($orig[$t][0]) - $rx;
	} else {
	    $x = $orig[$t][0] + $rx;
	}
	if (! exists $cy{$rx}) {
	    $y = abs($orig[$t][1]);
	    if ($orig[$t][1] < 0) {
		$y -= $ry if $ry > 0;
	    } else {
		$y += $ry if $ry > 0;
	    }
	} else {
	    $y = $ry < 0 ? $cy{$rx} :
	    	 $orig[$t][1] < 0 ? abs($orig[$t][1]) - $ry :
				    $ry + $orig[$t][1]; 
	}
	#print "y=$y ";
	if ($rz) {
	    if ($orig[$t][1] < 0) {
		if (!($y % 2)) {
		    #print "* ";
		    $y -= 1;
		}
	    } else {
		if (($y % 2)) {
		    #print "* ";
		    $y += 1
		}
	    }
	} else {
	    if ($orig[$t][1] < 0) {
		if (!($y % 2)) {
		    #print "* ";
		    #$y -= 1;
		}
	    } else {
		    $y += 2
	    }
	}

	printf "INST $brams[$r][0]", $t;
	printf " LOC = RAMB%s_X%dY%d ;\n",
	    $rz ? "36" : "18",
	    $x, $rz ? $y/2 : $y;

        # key LUT area group off of pkt x36 RAMS
        if ($rz &&  $brams[$r][0] =~ /Pkt/) {
            if ($maxPktXLUT < $x) {$maxPktXLUT = $x;}
            if ($maxPktYLUT < $y/2) {$maxPktYLUT = $y/2;}
            if ($minPktXLUT > $x) {$minPktXLUT = $x;}
            if ($minPktYLUT > $y/2) {$minPktYLUT = $y/2;}
        }
        if ($rz &&  $brams[$r][0] =~ /Acc/) {
            if ($maxAccXLUT < $x) {$maxAccXLUT = $x;}
            if ($maxAccYLUT < $y/2) {$maxAccYLUT = $y/2;}
            if ($minAccXLUT > $x) {$minAccXLUT = $x;}
            if ($minAccYLUT > $y/2) {$minAccYLUT = $y/2;}
        }

	$y += $orig[$t][1] < 0 ? ($rz ? -2 : -1) : ($rz ? 2 : 1);
	$cy{$rx} = $y;
    }
    # now do area groups
    printf "INST $pktRegs AREA_GROUP = AG_pkt%d ;\n", $t, $t;
#    printf "INST $pktRspCmd AREA_GROUP = AG_pkt%d ;\n", $t, $t;
#    printf "INST $pktAccCnt AREA_GROUP = AG_pkt%d ;\n", $t, $t;
    printf "INST $accRegs AREA_GROUP = AG_acc%d ;\n", $t, $t;
    #printf "%d,%d %d,%d\n", $minPktXLUT, $minPktYLUT, $maxPktXLUT, $maxPktYLUT;
    if ($maxPktYLUT < 40) {
        printf "AREA_GROUP AG_pkt%d RANGE = SLICE_X%dY%d:SLICE_X%dY%d ;\n", $t, 
            $LUTMap[$minPktXLUT], (int($minPktYLUT)*5), $LUTMap[$maxPktXLUT], (int($maxPktYLUT)*5)+4;
    } else {
        printf "AREA_GROUP AG_pkt%d RANGE = SLICE_X%dY%d:SLICE_X%dY%d ;\n", $t, 
            $LUTMap[$minPktXLUT], (int($minPktYLUT)*5), $LUTMap[$maxPktXLUT], (int($maxPktYLUT)*5)+4;
    }
    if ($maxPktYLUT < 40) {
	if ($maxAccXLUT < 5) {
	    printf "AREA_GROUP AG_acc%d RANGE = SLICE_X%dY%d:SLICE_X%dY%d ;\n", $t, 
            $LUTMap[$maxPktXLUT], (int($minAccYLUT)*5), $LUTMap[$maxPktXLUT]+64, (int($maxPktYLUT)*5)+4;
	} else {
	    printf "AREA_GROUP AG_acc%d RANGE = SLICE_X%dY%d:SLICE_X%dY%d ;\n", $t, 
            $LUTMap[$minPktXLUT]-64, (int($minAccYLUT)*5), $LUTMap[$minPktXLUT], (int($maxPktYLUT)*5)+4;
	}
    } else {
	if ($maxAccXLUT < 5) {
	    printf "AREA_GROUP AG_acc%d RANGE = SLICE_X%dY%d:SLICE_X%dY%d ;\n", $t, 
            $LUTMap[$maxPktXLUT], (int($minPktYLUT)*5), $LUTMap[$maxPktXLUT]+64, (int($maxAccYLUT)*5)+4;
	} else {
	    printf "AREA_GROUP AG_acc%d RANGE = SLICE_X%dY%d:SLICE_X%dY%d ;\n", $t, 
            $LUTMap[$minPktXLUT]-64, (int($minPktYLUT)*5), $LUTMap[$minPktXLUT], (int($maxAccYLUT)*5)+4;
	}
    }
}
