#!/usr/bin/perl

print "AREA_GROUP AG_mc_csr0 PLACE = CLOSED;\n";
print "AREA_GROUP AG_mc_csr1 PLACE = CLOSED;\n";
print "AREA_GROUP AG_mc_csr2 PLACE = CLOSED;\n";
print "AREA_GROUP AG_mc_csr3 PLACE = CLOSED;\n";
print "\n";

$b = "ae_top/core/cae_pers/top/pPersAuTop\$%d/pPers";
$m = "Mram_mem";

#						X	Y	36?
@brams = (
[ "${b}Read/m_strSetL_data0\$0/${m}",		0,	0*2,	1],
[ "${b}Read/m_strSetL_data0\$1/${m}",		0,	1*2,	1],
[ "${b}Read/m_strSetL_data0\$2/${m}",		0,	2*2,	1],
[ "${b}Read/m_strSetL_data0\$3/${m}",		0,	3*2,	1],
[ "${b}Read/m_strSetL_data0\$4/${m}",		0,	4*2,	1],
[ "${b}Read/m_strSetL_data0\$5/${m}",		0,	5*2,	1],
[ "${b}Read/m_strSetL_data0\$6/${m}",		0,	6*2,	1],
[ "${b}Read/m_strSetL_data0\$7/${m}",		0,	7*2,	1],

[ "${b}Read/m_strSetL_data1\$0/${m}",		0,	8*2,	0],
[ "${b}Read/m_strSetL_data1\$1/${m}",		0,	9*2,	0],
[ "${b}Read/m_strSetL_data1\$2/${m}",		0,	10*2,	0],
[ "${b}Read/m_strSetL_data1\$3/${m}",		0,	11*2,	0],
[ "${b}Read/m_strSetL_data1\$4/${m}",		0,	12*2,	0],
[ "${b}Read/m_strSetL_data1\$5/${m}",		0,	13*2,	0],
[ "${b}Read/m_strSetL_data1\$6/${m}",		0,	14*2,	0],
[ "${b}Read/m_strSetL_data1\$7/${m}",		0,	15*2,	0],

[ "${b}Read/m_strSetR_data0\$0/${m}",		1,	0*2,	1],
[ "${b}Read/m_strSetR_data0\$1/${m}",		1,	1*2,	1],
[ "${b}Read/m_strSetR_data0\$2/${m}",		1,	2*2,	1],
[ "${b}Read/m_strSetR_data0\$3/${m}",		1,	3*2,	1],
[ "${b}Read/m_strSetR_data0\$4/${m}",		1,	4*2,	1],
[ "${b}Read/m_strSetR_data0\$5/${m}",		1,	5*2,	1],
[ "${b}Read/m_strSetR_data0\$6/${m}",		1,	6*2,	1],
[ "${b}Read/m_strSetR_data0\$7/${m}",		1,	7*2,	1],

[ "${b}Read/m_strSetR_data1\$0/${m}",		1,	8*2,	0],
[ "${b}Read/m_strSetR_data1\$1/${m}",		1,	9*2,	0],
[ "${b}Read/m_strSetR_data1\$2/${m}",		1,	10*2,	0],
[ "${b}Read/m_strSetR_data1\$3/${m}",		1,	11*2,	0],
[ "${b}Read/m_strSetR_data1\$4/${m}",		1,	12*2,	0],
[ "${b}Read/m_strSetR_data1\$5/${m}",		1,	13*2,	0],
[ "${b}Read/m_strSetR_data1\$6/${m}",		1,	14*2,	0],
[ "${b}Read/m_strSetR_data1\$7/${m}",		1,	15*2,	0],
);

#RAMB18 coordinates;	760=>10x144(9*16)
#			330=>6x96
my $s = 0*2;
my $o = 8;
@orig = (
[1, -95+0*$s+$o],	[-4, -95+0*$s+$o],
[1, 0*$s+$o],#		[-4, 0*$s+$o],
);

for (my $t=0; $t<($#orig+1); $t++) {
    # current y for x
    my %cy = ();

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
	printf " LOC = RAMB36_X%dY%d", $x, $y/2;
	printf " | BEL = %s", ($y&1) ? "UPPER" : "LOWER" if (! $rz);
	printf ";\n",

	$y += $orig[$t][1] < 0 ? ($rz ? -2 : -1) : ($rz ? 2 : 1);
	$cy{$rx} = $y;
    }
}
