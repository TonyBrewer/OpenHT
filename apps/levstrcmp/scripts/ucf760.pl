#!/usr/bin/perl

$b = "ae_top/core/cae_pers/top/pPersAuTop\$%d/pPers";
$m = "Mram_mem";

$htPriv = "${b}Mch/r_t3_htPriv_\*";
# mmap ram columns to desired LUT slice column
@LUTMap = (0,-1,-1,139,-1,-1,192,-1,-1,331);
#							X	Y	36?
@brams = (
[ "${b}Read/m_strSetL_data0\$0/${m}",		0,	0*4,	1],
[ "${b}Read/m_strSetL_data0\$1/${m}",		0,	1*4,	1],
[ "${b}Read/m_strSetL_data0\$2/${m}",		0,	2*4,	1],
[ "${b}Read/m_strSetL_data0\$3/${m}",		0,	3*4,	1],
[ "${b}Read/m_strSetL_data0\$4/${m}",		0,	4*4,	1],
[ "${b}Read/m_strSetL_data0\$5/${m}",		0,	5*4,	1],
[ "${b}Read/m_strSetL_data0\$6/${m}",		0,	6*4,	1],
[ "${b}Read/m_strSetL_data0\$7/${m}",		0,	7*4,	1],

[ "${b}Read/m_strSetL_data1\$0/${m}",		1,	0*4,	0],
[ "${b}Read/m_strSetL_data1\$1/${m}",		1,	1*4,	0],
[ "${b}Read/m_strSetL_data1\$2/${m}",		1,	2*4,	0],
[ "${b}Read/m_strSetL_data1\$3/${m}",		1,	3*4,	0],
[ "${b}Read/m_strSetL_data1\$4/${m}",		1,	4*4,	0],
[ "${b}Read/m_strSetL_data1\$5/${m}",		1,	5*4,	0],
[ "${b}Read/m_strSetL_data1\$6/${m}",		1,	6*4,	0],
[ "${b}Read/m_strSetL_data1\$7/${m}",		1,	7*4,	0],

[ "${b}Read/m_strSetR_data1\$0/${m}",		2,	0*4,	0],
[ "${b}Read/m_strSetR_data1\$1/${m}",		2,	1*4,	0],
[ "${b}Read/m_strSetR_data1\$2/${m}",		2,	2*4,	0],
[ "${b}Read/m_strSetR_data1\$3/${m}",		2,	3*4,	0],
[ "${b}Read/m_strSetR_data1\$4/${m}",		2,	4*4,	0],
[ "${b}Read/m_strSetR_data1\$5/${m}",		2,	5*4,	0],
[ "${b}Read/m_strSetR_data1\$6/${m}",		2,	6*4,	0],
[ "${b}Read/m_strSetR_data1\$7/${m}",		2,	7*4,	0],

[ "${b}Read/m_strSetR_data0\$0/${m}",		3,	0*4,	1],
[ "${b}Read/m_strSetR_data0\$1/${m}",		3,	1*4,	1],
[ "${b}Read/m_strSetR_data0\$2/${m}",		3,	2*4,	1],
[ "${b}Read/m_strSetR_data0\$3/${m}",		3,	3*4,	1],
[ "${b}Read/m_strSetR_data0\$4/${m}",		3,	4*4,	1],
[ "${b}Read/m_strSetR_data0\$5/${m}",		3,	5*4,	1],
[ "${b}Read/m_strSetR_data0\$6/${m}",		3,	6*4,	1],
[ "${b}Read/m_strSetR_data0\$7/${m}",		3,	7*4,	1],
);

#RAMB18 coordinates;	760=>10x144(9*16)
#			330=>6x96
@orig = (
[ 0, -1*(7*16-1)],	[ -9, -1*(7*16-1)],
[ 0, 2*16],		[ -9, 2*16],
[ 0, -1*(9*16-1)],	[ -9, -1*(9*16-1)],
[ 0, 0*16],		[ -9, 0*16],
);

for (my $t=0; $t<($#orig+1); $t++) {
    # current y for x
    my %cy = ();
    my $minXLUT = 1000;
    my $minYLUT = 1000;
    my $maxXLUT = -1;
    my $maxYLUT = -1;

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
		    $y += 1;
		}
	    }
	}

	printf "INST $brams[$r][0]", $t;
	printf " LOC = RAMB%s_X%dY%d ;\n",
	    $rz ? "36" : "18",
	    $x, $rz ? $y/2 : $y;
	# key LUT area group off of x36 RAMS
	if ($rz) {
	    if ($maxXLUT < $x) {$maxXLUT = $x;}
	    if ($maxYLUT < $y/2) {$maxYLUT = $y/2;}
	    if ($minXLUT > $x) {$minXLUT = $x;}
	    if ($minYLUT > $y/2) {$minYLUT = $y/2;}
	}

	$y += $orig[$t][1] < 0 ? ($rz ? -2 : -1) : ($rz ? 2 : 1);
	$cy{$rx} = $y;

    }
    # now do htPriv area group
    printf "INST $htPriv AREA_GROUP = AG_htPriv%d ;\n", $t, $t;
    # printf "%d,%d %d,%d\n", $minXLUT, $minYLUT, $maxXLUT, $maxYLUT;
    if ($maxYLUT < 40) {
	printf "AREA_GROUP AG_htPriv%d RANGE = SLICE_X%dY%d:SLICE_X%dY%d ;\n", $t, 
	    $LUTMap[$minXLUT], (int($minYLUT)*5), $LUTMap[$maxXLUT], (int($maxYLUT)*5)+9;
    } else {
	printf "AREA_GROUP AG_htPriv%d RANGE = SLICE_X%dY%d:SLICE_X%dY%d ;\n", $t, 
            $LUTMap[$minXLUT], (int($minYLUT)*5)-5, $LUTMap[$maxXLUT], (int($maxYLUT)*5)+4;
    }
}
