#!/usr/bin/perl

%op_map=(
  0=>"GET",
  1=>"SET",
);



open(DUMP, "/work/pmcnamara/wireshark-1.8.5/tshark  -r $ARGV[0] tcp.port==11211 -t a -T fields -E occurrence=a -E separator=\\| -e frame.number -e frame.time_relative -e ip.src -e tcp.srcport -e ip.dst -e tcp.dstport -e memcache.magic -e memcache.opcode -e memcache.opaque |" );
%trans=();
$count=0;
$req_count=0;
$resp_count=0;
$time=0;
$min=0;
$max=0;
$ts_start=0;
$ts_end=0;
while($line=<DUMP>) {
  chomp($line);
  ($fnum, $ts,$src_ip,$src_port,$dst_ip,$dst_port,$magic,$op,$seq)=(split('\|', $line))[0,1,2,3,4,5,-3,-2,-1];
  @ops=split(",", $op);
  @seqs=split(",", $seq);
  @magics=split(",", $magic);
  for($i=0; $i<scalar(@ops); $i++) {
    $op=$ops[$i];
    $seq=$seqs[$i];
    $magic=$magics[$i];
    next unless($magic==128 || $magic==129);
    if($magic==128) {
      $dir="Request";
      
    } else {
      $dir="Response";
    }
    $op=$op_map{$op};
    next if(defined($ARGV[1]) && $ARGV[1] ne $op);
    if($dir eq "Request") {
      $req_count++;
      $ts_start=$ts unless($ts_start);
      $trans{"$src_ip:$src_port-$dst_ip:$dst_port"}{$seq}=$ts;
    } elsif ($dir eq "Response") {
      $resp_count++;
      $ts_end=$ts;
      if(exists($trans{"$dst_ip:$dst_port-$src_ip:$src_port"}{$seq})) {
	$lat=$ts-$trans{"$dst_ip:$dst_port-$src_ip:$src_port"}{$seq};

	$time+=$lat;
	if($min) {
	  if($lat<$min) {
	    $min=$lat;
	  }
	} else {
	  $min=$lat;
	}
	if($lat>$max) {
	  $max=$lat;
	}
	$count++;
	delete($trans{"$dst_ip:$dst_port-$src_ip:$src_port"}{$seq});
      } else {
	#print "no request: $dst_ip:$dst_port-$src_ip:$src_port $seq\n";
	#exit(0);
      }
    }
  }
}
close(DUMP);
if($count) {
  $run=$ts_end-$ts_start;
  printf("req: %d, resp %d, complete: %d\n", $req_count, $resp_count, $count);
  printf("capture time: %0.7f sec, %.7f tps\n", $run, $count/$run);
  printf("min: %0.7f,  max %0.7f,  avg: %0.7f\n", $min, $max, ($time/$count));
}
