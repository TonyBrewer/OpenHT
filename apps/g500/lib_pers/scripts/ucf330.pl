#!/usr/bin/perl

$b = "ae_top/core/cae_pers/top/pPersAuTop\$%d/pPers";
$m = "Mram_mem";

#							X	Y	36?
@brams = (
[ "${b}Bmap/m__GBL__bmap__dataIr/${m}",			0,	-1,	1],
[ "${b}Bmap/m_htPriv/${m}1",				0,	-1,	1],
[ "${b}Bmap/m_htPriv/${m}2",				0,	-1,	1],
[ "${b}Bmap/m_htPriv/${m}3",				0,	-1,	1],
[ "${b}Scatter/m_htPriv/${m}",				0,	-1,	1],

[ "${b}Bufp/m_htPriv/${m}1",				1,	-1,	1],
[ "${b}Bufp/m_htPriv/${m}2",				1,	-1,	0],
[ "${b}Bufp/m__GBL__bufp__bmapIr/${m}",			1,	-1,	1],
[ "${b}Bufp/m__GBL__bufp__xadjIr/${m}",			1,	-1,	0],
[ "${b}Bufp/m__GBL__bufp__xoff0Ir/${m}",		1,	-1,	0],
[ "${b}Bufp/m__GBL__bufp__xoff1Ir/${m}",		1,	-1,	0],
[ "${b}Scatter/m__GBL__scatter__addrMemIr/${m}",	1,	-1,	1],
[ "${b}Scatter/m__GBL__scatter__valMemIr/${m}",		1,	-1,	1],
);

#RAMB18 coordinates;	760=>10x144(9*16)
#			330=>6x96
my $s = 96/4;
my $o = 4;
@orig = (
[1, -95+0*$s+$o],	[-4, -95+0*$s+$o],
[1, -95+1*$s+$o],	[-4, -95+1*$s+$o],
#[1,     1*$s+$o],	[-4,     1*$s+$o],
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
