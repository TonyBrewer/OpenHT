#!/usr/bin/perl

$b = "ae0/same_pers.cae_pers/top/pPersAuTop\$%d/pPers";
$m = "mem_reg";

#						X	Y	36?
@brams = (
[ "${b}Acc/m_ramDat/${m}_0",			0,	-1,	1],
[ "${b}Acc/m_ramDat/${m}_1",			0,	-1,	1],
[ "${b}Acc/m_ramDat/${m}_2",			0,	-1,	1],
[ "${b}Acc/m_ramDat/${m}_3",			0,	-1,	1],
[ "${b}Acc/m_ramDat/${m}_4",			0,	-1,	1],
[ "${b}Acc/m_ramDat/${m}_5",			0,	-1,	0],
[ "${b}Sch/m_queDat/${m}_1",			0,	-1,	0],
[ "${b}Sch/m_queDat/${m}_0",			0,	-1,	1],
[ "${b}Pkt/m_connList_pConn\$0/${m}",		0,	-1,	1],
[ "${b}Pkt/m_connList_pConn\$1/${m}",		0,	-1,	1],
[ "${b}Pkt/m_connList_pConn\$2/${m}",		0,	-1,	1],
[ "${b}Pkt/m_connList_pConn\$3/${m}",		0,	-1,	1],
[ "${b}Pkt/m_connList_pConn\$4/${m}",		0,	-1,	1],
[ "${b}Pkt/m_connList_pConn\$5/${m}",		0,	-1,	1],
[ "${b}Pkt/m_connList_pConn\$6/${m}",		0,	-1,	1],
[ "${b}Pkt/m_connList_pConn\$7/${m}",		0,	-1,	1],
[ "${b}Pkt/m_connList_pConn\$8/${m}",		0,	-1,	1],
[ "${b}Pkt/m_connList_pConn\$9/${m}",		0,	-1,	1],
[ "${b}Pkt/m_connList_pConn\$10/${m}",		0,	-1,	1],
[ "${b}Pkt/m_hashResA/${m}",			0,	-1,	0],
[ "${b}Pkt/m_hashResB/${m}",			0,	-1,	0],
[ "${b}Pkt/m_hashResC/${m}",			0,	-1,	0],
[ "${b}Pkt/m_prePktDat_qw/${m}",		0,	-1,	1],
[ "${b}Pkt/m_rdPktDat_qw/${m}",			0,	-1,	1],

);

#RAMB18 coordinates;	2k =>11x240
#			690=>15x200
my $ys = 200/4;
@orig = (
    [1, 2+$ys*0], [3, 2+$ys*0], [5, 2+$ys*0], [7, 2+$ys*0],
    [1, 2+$ys*1], [3, 2+$ys*1], [5, 2+$ys*1], [7, 2+$ys*1],
    [1, 2+$ys*2], [3, 2+$ys*2], [5, 2+$ys*2], [7, 2+$ys*2],
    [1, 2+$ys*3], [3, 2+$ys*3], [5, 2+$ys*3], [7, 2+$ys*3],
);

for (my $t=0; $t<($#orig+1); $t++) {
    # current y for x
    my $ox = $orig[$t][0];
    my $oy = $orig[$t][1];
    my %cy = ();

    for (my $r=0; $r<@brams; $r++) {
	my $rx = $brams[$r][1];
	my $ry = $brams[$r][2];
	my $rz = $brams[$r][3];

	my $x, $y;
	if ($ox < 0) {
	    $x = abs($ox) - $rx;
	} else {
	    $x = $ox + $rx;
	}
	if (! exists $cy{$rx}) {
	    $y = abs($oy);
	    if ($oy < 0) {
		$y -= $ry if $ry > 0;
	    } else {
		$y += $ry if $ry > 0;
	    }
	} else {
	    $y = $ry < 0 ? $cy{$rx} :
		 $oy < 0 ? abs($oy) - $ry : $ry + $oy; 
	}
	#print "y=$y ";
	if ($rz) {
	    if ($oy < 0) {
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

	printf "set_property LOC RAMB%s_X%dY%d ",
	    $rz ? "36" : "18",
	    $x, $rz ? $y/2 : $y;
	printf " [get_cells {$brams[$r][0]}]\n", $t;

	$y += $oy < 0 ? ($rz ? -2 : -1) : ($rz ? 2 : 1);
	$cy{$rx} = $y;
    }
}
