#!/usr/bin/perl

use Getopt::Long;
use File::Basename;

my $DIR="/opt/convey/examples/cnymemcached/mcd-benchmark";
my $MCHAMMR="";
my $MEMCACHED="/opt/convey/cnymemcached/bin/memcached";
my $CTRL="";
my $DEBUG=1;

my $server_mem=16384;
my $server_conns=4096;
my $server_threads=16;
my $delay_max=225000;
my $conns_per_thread=32;

#total number of connections to use from all clients.  This works out to
#4000 connections at a full 16 threads and scales down appropriately as
#we drop the number of server threads
my $client_conns=$conns_per_thread*$server_threads;

#hosts used to provide the benchmark load
my @bhosts=();
#host used to probe latency
my $phost="grizzly6";

#run time for each test point
my $time=30;
#number of gets in a multiget.  
my $mget=32;
#number of client threads to run per client
my $threads=8;

#memcached server IP
my $host="10.2.0.128";
#This is the name/ip used to ssh into the memcached server.  May not match the server
#IP is the test network is on a private network.
my $host_ssh="coconino";
#port to use for memcached
my $port=11211;
my $port_count=1;
#this is our latency cutoff.  We will stop the test run when latency rises above this
#point (is microseconds)
my $lat_cut=1000;
#data size to SETs and GETs
my $data_size=32;
my $key_size=8;

my $delay_delta_base=.7;
#my $key_count=65536;
my $op="MGET";
my $op_count=2048;
my $keys_per_op=32;
my $out=1;
my $delay=1000;
my $delay_start=225000;
my $delay_end=1000;
my $mcast="224.0.0.37";

GetOptions("dir=s" => \$DIR,
	   "mchammr=s" => \$MCHAMMR,
	   "memcached=s" => \$MEMCACHED,
	   "conns=i" => \$client_conns,
	   "server_threads=i" => \$server_threads,
	   "client_threads=i" => \$threads,
	   "server_name=s" => \$host_ssh,
	   "probe_host=s" => \$phost,
	   "load_host=s" => \@bhosts,
	   "max_latency=i" => \$lat_cut,
	   "data_size=i" => \$data_size,
	   "key_size=i" => \$key_size,
	   "server_ip=s" => \$host,
	   "base_port=i" => \$port,
	   "port_count=i" => \$port_count,
	   "op_count=i" => \$op_count,
	   "keys_per_op=i" => \$keys_per_op,
	   "op=s" => \$op,
	   "delta_base=f" => \$delay_delta_base,
	   "out=i" => \$out,
	   "delay=i" => \$delay,
	   "delay_start=i" => \$delay_start,
	   "delay_end=i" => \$delay_end,
	   "time=i" => \$time,
	   "mcast_net=s" => \$mcast,
);

$MCHAMMR="$DIR/mc-hammr/mc-hammr" if($MCHAMMR eq "");
$CTRL="$DIR/ctrl/mcast_msg" if ($CTRL eq "");
$threads=int($threads/$port_count);
$threads=1 if($threads<1);
#calculate number of connections per thread to make the total connections add up
#to what we want
my $conns=int($client_conns/scalar(@bhosts)/$threads/$port_count);


#config file names for various functions
my $load_conf="/tmp/load.conf";
my $bench_conf="/tmp/bench.conf";
my $probe_conf="/tmp/probe.conf";

my $gps;
my $maxgps;
my $lastgps=0;
my $lat;
my $delay_delta;
my %results=();
my $lat_repeat=0;
my $gps_repeat=0;

my $fork=0;
if($port_count>1) {
  $fork=1;
}

#Turn on autoflush for STDOUT
$|=1;
		 

#make the config file we will use to load the freshly started server with some
#data for benchmarking
print "making \"load\" config file...\n";
if(!make_conf_file($load_conf,
	       {host=>$host,
		port=>$port,
		port_count=>$port_count,
		send=>"SET",
		key_len=>$key_size,
		value_size=>$data_size,
		out=>1,
		threads=>1,
		conns=>1,
		loop=>1,
		wait=>0,
		op_count=>($op_count*$keys_per_op),
		fork=>$fork,
	       })) {
  die("error making config file: $load_conf");
}

#make the config file we will use to probe the latency of the server under load
print "making \"probe\" config file...\n";
if(!make_conf_file($probe_conf,
	       {host=>$host,
		port=>$port,
		port_count=>$port_count,
		send=>"GET",
		delay=>$delay_end,
		time=>$time,
		threads=>($port_count<4 ? 4 : $port_count),
		size=>$size,
		key_len=>$key_size,
		value_size=>$data_size,
		op_count=>($op_count*$keys_per_op),
		out=>1,
		conns=>1,
		fork=>$fork,
		mcast=>$mcast,
		wait=>1})) {
  die("error making config file: $probe_conf");
}

#start up the server
$host_port_string="";
for($p=$port; $p<$port+$port_count; $p++) {
  $host_port_string .= "-l $host:$p ";
}
if($port_count==1) {
  print "starting server on $host:$port...\n";
} else {
  print "starting server on $host:$port-" . ($port + $port_count - 1) . "...\n";
}
if(!start_server($host_ssh, "-m $server_mem -c $server_conns  $host_port_string", $server_threads)) {
  die("error starting memcached server");
}
print "waiting on server to stabilize (60 sec)\n";
sleep(60);
#put the "load" config file on the client system we will use to load the data
print "loading the \"load\" config on $phost...\n";
if(!load_conf($load_conf, $phost)) {
  die("error loading config file: $load_conf");
}
#now populate the cache
print "populating cache...\n";
if(populate_cache($phost)==0) {
  stop_server($host_ssh);
  die("error populating cache");
}
#copy the probe config over to the probe host to ready it for measurement duty
print "loading the probe config on $phost...\n";
if(!load_conf($probe_conf, $phost)) {
  die("error loading config file: $probe_conf");
}

#We scale the decrease in delay based on how close we are getting to the maximum throughput.  We
#know the latency jump is sharp and we want to increase the number of samples as we approach the 
#max throughput to ensure we get as smooth a graph as possible.  For that, we have to have a 
#reasonable idea what the maximum throughput for the config will be.


print "determining max throughput....\n";
#create a "max throughput" config file for the benchmark machines
if(!make_conf_file($bench_conf,
		   {host=>$host,
		    port=>$port,
		    port_count=>$port_count,
		    send=>"MGET",
		    threads=>$threads,
		    conns=>$conns,
		    op_count=>$op_count,
		    keys_per_op=>$keys_per_op,
		    delay=>$delay_end,
		    time=>$time,
		    out=>$out,
		    value_size=>$data_size,
		    key_len=>$key_size,
		    fork=>$fork,	
		    mcast=>$mcast,
		    wait=>1})) {
  die("error making config file: $bench_conf");
}
#and get it loaded
print "load benchmark config file on hosts: " . join(", ", @bhosts) . "\n";
if(!load_conf($bench_conf, @bhosts)) {
  die("error loading config file: $bench_conf");
}
#This will get us somewhere near the max we will measure.  In fact, it may get us above the max as
#we cut of the run when we cross the max latency threshold.
print "running benchmark...\n";
($maxgps, $lat)=bench($time+120, $port_count, $phost, @bhosts);
print "max gps is $maxgps\n";

#all that to get to the actual benchmark run.  We will loop through, decreasing the delay each pass
#Once the latency crosses the specified level, we stop.  We also stop if we manage to get down to
#a delay of 10us.  We shouldn't ever get there, but it will keep us from ending up in an infinite
#loop if something weird happens
for($delay=$delay_start; $delay>=$delay_end;) {
  #check to see how many times we have repeated this test point and exit if we seem to be stuck
  if($lat_repeat==10) {
    print "To many latency failures ($lat_repeat) at the current test point:  $delay, $gps, $lat\n";
    print "Terminating test run!\n";
    last;
  }

  print "making benchmark config file...\n";
  if(!make_conf_file($bench_conf,
		     {host=>$host,
		      port=>$port,
      		      port_count=>$port_count,
		      send=>"MGET",
		      threads=>$threads,
		      conns=>$conns,
		      op_count=>$op_count,
		      keys_per_op=>$keys_per_op,
		      delay=>$delay,
		      time=>$time,
		      out=>$out,
		      value_size=>$data_size,
		      key_len=>$key_size,
		      fork=>$fork,
		      mcast=>$mcast,
		      wait=>1})) {
    die("error making config file: $bench_conf");
  }

  print "load benchmark config file on hosts: " . join(", ", @bhosts) . "\n";
  if(!load_conf($bench_conf, @bhosts)) {
    die("error loading config file: $bench_conf");
  }

  print "running benchmark...\n";
  ($gps, $lat)=bench($time+120, $port_count, $phost, @bhosts);
  print "\n\ndelay=$delay, gps=$gps, lat=$lat\n\n";
  #if for some reason we did not get a latency report, or a GPS number, re-run the test point
  if(!defined($lat) || !defined($gps)) {
    $lat_repeat++;
    print "No latency value returned, repeating test point: $lat_repeat\n";
    next;
  }
  #Since our delay is always getting smaller, our gets/sec should never go backwards
  if($gps<$lastgps) {
    $gps_repeat++;
    if($gps_repeat<10) {
      print "Throughput decreased, assuming results invalid and repeating test point: $gps_repeat\n";
      next;
    } else {
      print "Too many tries at the current test point without increasing throughtput.\n";
      print "Moving on to next test point\n";
      $gps=$lastgps;
    }
  }
  $lastgps=$gps;
  #We collect the stats for each run to print at the end so we don't have to sort through all the
  #other stuff that gets printed out.
  $results{$delay}{gps}=$gps;
  $results{$delay}{lat}=$lat;
  #exit if we have exceeded our max latency cutoff;
  last if($lat>=$lat_cut);
  #now calculte our delay modifier.  As we get closer and closer to our measured max, we descrease
  #the delay by less and less
  #$delay_delta=$delay_delta_base+(((1-$delay_delta_base)*($gps/$maxgps))*1.1);
  #$delay_delta=$delay_delta_base+(((1-$delay_delta_base)*($gps/$maxgps)));
  #Should we exceed the max we measured earlier, we don't want to indvertantly increase our delay so
  #we will cap it.  At most, we will do two runs with the same delay value.
  #$delay_delta=.99 if($delay_delta>.99);
  $delay=int($delay*$delay_delta_base);
  print "delay_delta=$delay_delta, new delay=$delay\n";
  $lat_repeat=0;
  $gps_repeat=0;
}

#being done we need to clean stuff up and stop the server
print "shutting down server\n";
stop_server($host_ssh);
print "\n\n\n";

#display our results in nice CSV form
printf"delay,gps,lat\n";
for $delay (sort {$b <=> $a} (keys(%results))) {
  print "$delay,$results{$delay}{gps},$results{$delay}{lat}\n";
}
print "\n\n";
exit(0);

sub populate_cache {
  my $client=$_[0];
  my $out="";
  $out=`ssh $client $MCHAMMR /tmp/mc-hammr.conf`;
  if($?>>8) {
    print $out;
    return(0);
  }
  if($DEBUG) {
    print $out;
  }
  return(1);
}

sub bench {
  my $timeout=shift;
  my $port_count=shift;	
  my $phost=shift;
  my $ready_count=(scalar(@_)+1)*$port_count;
  my $line="";
  my $lat=undef;
  my $gps=0;
  my $client;
  my $host;
  print "pdsh -R ssh -u $timeout -w $phost,". join(",", @_) . " $MCHAMMR /tmp/mc-hammr.conf 2>/dev/null \n";
  open(RUN, "pdsh -R ssh -u $timeout -w $phost,". join(",", @_) . " $MCHAMMR /tmp/mc-hammr.conf 2>/dev/null |");
  printf("waiting on benchmark clients to be ready: $ready_count");
  while($ready_count) {
    $line=<RUN>;
    if($line=~/bound/) {
      $ready_count--;
      print " $ready_count";
    }
  }
  sleep(2);
  print " go\n";
  `$CTRL go`;
  while(<RUN>) {
    next unless(/bound/ || /cumulative: /);
    print;
    m/^(.*):\s*cumulative.*rate: (\d+\.\d\d), .*lat: (\d+\.\d\d)/;
    $host=$1;
    $gps+=$2;
    $lat=$3 if($host eq $phost)
  }
  close(RUN);
  clean_client("$phost,". join(",", @_));
  return($gps, $lat);
}

sub clean_client {
  system "ssh -q $_[0] pkill " . basename($MCHAMMR);
}


sub start_server {
  my $rc;
  print "ssh -f $_[0] $MEMCACHED -t $_[2] $_[1] 2>&1\n";
  #return(1);
  $rc=system("ssh -f $_[0] $MEMCACHED -t $_[2] $_[1] 2>&1");
  if($rc>>8) {
    return(0);
  }  
  return(1);
}

sub stop_server {
  system "ssh -q $_[0] pkill " . basename($MEMCACHED);
  sleep(15);
}

sub load_conf {
  my $conf=shift(@_);
  my $out="";
  my $host;
  for $host (@_) {
    $out=`scp -B -q $conf $host:/tmp/mc-hammr.conf`;
    if($?>>8!=0) {
      print $out;
      return(0);
    }
    if($DEBUG) {
      print $out;
    }
  }
  return(1);

}

sub make_conf_file {
  my $fname=$_[0];
  my %cfg=%{$_[1]};
  my @required=qw(send host port op_count);
  my $key;
  my $port;
  for $key (@required) {
    unless (exists($cfg{$key})) {
      print "missing required config entry: $key\n";
      return(0);
    }
  }
  if(!exists($cfg{port_count})) {
    $cfg{port_count}=1;
  }
  open(CONF, "> $fname");
  for($port=$cfg{port};$port<$cfg{port}+$cfg{port_count}; $port++) {
    print CONF "send=" . $cfg{send};
    print CONF ",recv=async";
    if(exists($cfg{threads})) {
      print CONF ",threads=".$cfg{threads};
    } else {
      print CONF ",threads=1";
    }
    if(exists($cfg{conns})) {
      print CONF ",conns_per_thread=". $cfg{conns};
    } else {
      print CONF ",conns_per_thread=1";
    }
    if(exists($cfg{key_prefix})) {
      print CONF ",key_prefix=" . $cfg{key_prefix};
    } else {
      print CONF ",key_prefix=0:";
    }
    if(exists($cfg{size})) {
      print CONF ",value_size=".$cfg{size};
    } else {
      print CONF ",value_size=32";
    }
    if(exists($cfg{key_len})) {
      print CONF ",key_len=".$cfg{key_len};
    } else {
      print CONF ",key_len=8";
    }
    print CONF ",host=". $cfg{host};
    print CONF ",port=". $port;
    if(exists($cfg{loop})) {
      print CONF ",loop=" . $cfg{loop};
    }
    if(exists($cfg{time})) {
      print CONF ",time=" . $cfg{time};
    }
    if(exists($cfg{delay})) {
      print CONF ",delay=" . $cfg{delay};
  }
    if(exists($cfg{fork})) {
      print CONF ",fork=" . $cfg{fork};
    } else {
      print CONF ",fork=0";
    }
    if(exists($cfg{ops_per_conn})) {
      print CONF ",ops_per_conn=".$cfg{ops_per_conn};
    } else {
      print CONF ",ops_per_conn=0";
    }
    if(exists($cfg{out})) {
      print CONF ",out=" . $cfg{out};
    } else {
      print CONF ",out=1";
    }
    if(exists($cfg{wait})) {
      print CONF ",mcast_wait=".$cfg{wait};
    }
    if(exists($cfg{op_count})) {
      print CONF ",op_count=".$cfg{op_count};
    }
    if(exists($cfg{mcast})) {
      print CONF ",mcast_net=".$cfg{mcast};
    }
    if(exists($cfg{keys_per_op})) {
      print CONF ",ops_per_frame=".$cfg{keys_per_op};
    } else {
      print CONF ",ops_per_frame=1";
    }
    print CONF "\n";
  }
  close(CONF);
  return(1);
}
