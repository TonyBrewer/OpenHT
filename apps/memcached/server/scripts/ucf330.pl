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
[ "${b}Pkt/m_connList_pConn\$0/${m}",			0,	4*0,	1],
[ "${b}Pkt/m_connList_pConn\$1/${m}",			0,	4*1,	1],
[ "${b}Pkt/m_connList_pConn\$2/${m}",			0,	4*2,	1],
[ "${b}Pkt/m_connList_pConn\$3/${m}",			0,	4*3,	1],
[ "${b}Pkt/m_connList_pConn\$4/${m}",			0,	4*4,	1],
[ "${b}Pkt/m_connList_pConn\$5/${m}",			0,	4*5,	1],
[ "${b}Pkt/m_prePktDat_qw/${m}",			0,	4*5+2,	1],
[ "${b}Pkt/m_rdPktDat_qw/${m}",				0,	4*6,	1],
[ "${b}Acc/m_ramDat/${m}1",				0,	4*6+2,	1],
[ "${b}Acc/m_ramDat/${m}2",				0,	4*7,	1],
[ "${b}Acc/m_ramDat/${m}3",				0,	4*7+2,	1],

[ "${b}Pkt/m_connList_pConn\$10/${m}",			1,	4*0,	1],
[ "${b}Pkt/m_connList_pConn\$9/${m}",			1,	4*1,	1],
[ "${b}Pkt/m_connList_pConn\$8/${m}",			1,	4*2,	1],
[ "${b}Pkt/m_connList_pConn\$7/${m}",			1,	4*3,	1],
[ "${b}Pkt/m_connList_pConn\$6/${m}",			1,	4*4,	1],
[ "${b}Pkt/m_hashResA/${m}",				1,	4*5,	0],
[ "${b}Pkt/m_hashResB/${m}",				1,	4*5+1,	0],
[ "${b}Pkt/m_hashResC/${m}",				1,	4*5+2,	0],
[ "${b}Acc/m_ramDat/${m}6",				1,	4*5+3,	0],
[ "${b}Acc/m_ramDat/${m}5",				1,	4*6,	1],
[ "${b}Acc/m_ramDat/${m}4",				1,	4*6+2,	1],
[ "${b}Sch/m_queDat/${m}1",				1,	4*7,	1],
[ "${b}Sch/m_queDat/${m}2",				1,	4*7+2,	0],
);


#RAMB18 coordinates;	760=>10x144(9*16)
#			330=>6x96
my $s = 4*9;
my $o = 4;
@orig = (
[1, -95+0*$s+$o],	[-4, -95+0*$s+$o],
[1,     0*$s+$o],	[-4,     0*$s+$o],
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
