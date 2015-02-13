#!/usr/bin/perl

$b = "ae_top/core/cae_pers/top/pPersAuTop\$%d/pPers";
$m = "Mram_mem";

#							X	Y	36?
@brams = (
[ "${b}Bufp/m_htPriv/${m}1",				0,	-1,	1],
[ "${b}Bufp/m_htPriv/${m}2",				0,	-1,	1],
[ "${b}Bufp/m_htPriv/${m}3",				0,	-1,	1],
[ "${b}Bufp/m_htPriv/${m}4",				0,	-1,	1],
[ "${b}Bufp/m_htPriv/${m}5",				0,	-1,	1],
[ "${b}Bufp/m_htPriv/${m}6",				0,	-1,	1],
[ "${b}Bufp/m_htPriv/${m}7",                            0,      -1, 0],

[ "${b}OUT__1__6827__/m_htPriv/${m}1",			1,	-1,	1],
[ "${b}OUT__1__6827__/m_htPriv/${m}2",			1,	-1,	1],
[ "${b}OUT__1__6827__/m_htPriv/${m}3",			1,	-1,	1],
[ "${b}OUT__1__6827__/m_htPriv/${m}4",			1,	-1,	1],
[ "${b}OUT__1__6827__/m_htPriv/${m}5",			1,	-1,	1],
[ "${b}OUT__1__6827__/m_htPriv/${m}6",			1,	-1,	1],
[ "${b}OUT__1__6827__/m_htPriv/${m}7",			1,	-1,	1],
[ "${b}OUT__1__6827__/m_htPriv/${m}8",			1,	-1,	1],
);

#RAMB18 coordinates;	760=>10x144(9*16)
#			330=>6x96
my $s = 144/8;
my $o = 1;
@orig = (
[0, -143+0*$s+$o],	[-9, -143+0*$s+$o],
[0, -143+1*$s+$o],	[-9, -143+1*$s+$o],
[0, -143+2*$s+$o],	[-9, -143+2*$s+$o],
[0, -143+3*$s+$o],	[-9, -143+3*$s+$o],
[0, 3*$s+$o],		[-9, 3*$s+$o],
[0, 2*$s+$o],		[-9, 2*$s+$o],
[0, 1*$s+$o],		[-9, 1*$s+$o],
[0, 0*$s+$o],		[-9, 0*$s+$o],
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
	printf " LOC = RAMB%s_X%dY%d ;\n",
	    $rz ? "36" : "18",
	    $x, $rz ? $y/2 : $y;

	$y += $orig[$t][1] < 0 ? ($rz ? -2 : -1) : ($rz ? 2 : 1);
	$cy{$rx} = $y;
    }
}
