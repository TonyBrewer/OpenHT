#!/usr/bin/perl

my $MCHAMMR="/work/pmcnamara/mcd-benchmark/mc-hammr/mc-hammr";
my $MEMCACHED="/scratch/mcd/memcached";
my $DEBUG=1;

my $server_mem=16384;
my $server_conns=4096;
my $server_threads=16;
my $conns_per_thread=75;

#total number of connections to use from all clients.  This works out to
#4000 connections at a full 16 threads and scales down appropriately as
#we drop the number of server threads
my $client_conns=$conns_per_thread*$server_threads;

#hosts used to provide the benchmark load
my @bhosts=qw(coconino mcdtest1 mcdtest2 fox6 grizzly5);
#host used to probe latency
my $phost="grizzly6";

#run time for each test point
my $time=10;
#number of client threads to run per client
my $threads=16;
#calculate number of connections per thread to make the total connections add up
#to what we want
my $conns=int($client_conns/scalar(@bhosts)/$threads);
#memcached server IP
#my $host="10.2.0.128";
my $host="10.2.0.16";
#This is the name/ip used to ssh into the memcached server.  May not match the server
#IP is the test network is on a private network.
#my $host_ssh="coconino";
my $host_ssh="dell720";
#port to use for memcached
my $port=11211;

#data size to SETs and GETs
my $size=32;
#key size
my $key_len=8;

my $delay=0;

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

#Turn on autoflush for STDOUT
$|=1;

#make the config file we will use to load the freshly started server with some
#data for benchmarking
print "making \"load\" config file...\n";
if(!make_conf_file($load_conf,
	       {host=>$host,
		port=>$port,
		send=>"SET",
		key_len=>$key_len,
		size=>$size,
		loop=>1
	       })) {
  die("error making config file: $load_conf");
}

#make the config file we will use to probe the latency of the server under load
print "making \"probe\" config file...\n";
if(!make_conf_file($probe_conf,
	       {host=>$host,
		port=>$port,
		send=>"GET",
		delay=>0,
		time=>$time,
		threads=>8,
		key_len=>$key_len,
		size=>$size,
		wait=>1})) {
  die("error making config file: $probe_conf");
}

#start up the server
print "starting server on $host:$port...\n";
if(!start_server($host_ssh, "-m $server_mem -c $server_conns -l $host:$port", $server_threads)) {
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


#do one run to pre-allocated connection rx/tx buffers
print "warming server buffers\n";
print "making benchmark config file...\n";
if(!make_conf_file($bench_conf,
		   {host=>$host,
		    port=>$port,
		    port_count=>1,
		    send=>"GET",
		    threads=>$threads,
		    conns=>$conns,
		    mget=>1,
		    delay=>$delay,
		    time=>$time,
		    out=>1,
		    key_len=>$key_len,
		    size=>$size,
		    wait=>1})) {
  die("error making config file: $bench_conf");
}

print "load benchmark config file on hosts: " . join(", ", @bhosts) . "\n";
if(!load_conf($bench_conf, @bhosts)) {
  die("error loading config file: $bench_conf");
}
print "running benchmark...\n";
($gps, $lat)=bench($phost, @bhosts);
print "\n\nmget=1, gps=$gps, lat=$lat\n\n";

print "running benchmark\n";
for($mget_size=1; $mget_size<=48; $mget_size++) {

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
		      port_count=>1,
		      send=>"GET",
		      threads=>$threads,
		      conns=>$conns,
		      mget=>$mget_size,
		      delay=>$delay,
		      time=>$time,
		      out=>$mget_size,
		      key_len=>$key_len,
		      size=>$size,
		      wait=>1})) {
    die("error making config file: $bench_conf");
  }

  print "load benchmark config file on hosts: " . join(", ", @bhosts) . "\n";
  if(!load_conf($bench_conf, @bhosts)) {
    die("error loading config file: $bench_conf");
  }
#exit(0);
  print "running benchmark...\n";
  ($gps, $lat)=bench($phost, @bhosts);
  print "\n\nmget=$mget_size, gps=$gps, lat=$lat\n\n";
  #if for some reason we did not get a latency report, or a GPS number, re-run the test point
  if(!defined($lat) || !defined($gps)) {
    $lat_repeat++;
    print "No latency value returned, repeating test point: $lat_repeat\n";
    $mget_size--;
    next;
  }

  #We collect the stats for each run to print at the end so we don't have to sort through all the
  #other stuff that gets printed out.
  $results{$mget_size}{gps}=$gps;
  $results{$mget_size}{lat}=$lat;

  $lat_repeat=0;

}

#being done we need to clean stuff up and stop the server
print "shutting down server\n";
stop_server($host_ssh);
print "\n\n\n";

#display our results in nice CSV form
printf"mget,gps,lat\n";
for $mget_size (sort {$b <=> $a} (keys(%results))) {
  print "$mget_size,$results{$mget_size}{gps},$results{$mget_size}{lat}\n";
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
  my $phost=shift;
  my $ready_count=scalar(@_)+1;
  my $line="";
  my $lat=undef;
  my $gps;
  my $client;
  my $host;
  open(RUN, "pdsh -R ssh -w $phost,". join(",", @_) . " /work/pmcnamara/mcd-benchmark/mc-hammr/mc-hammr /tmp/mc-hammr.conf 2>/dev/null |");
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
  `/work/pmcnamara/mcd-benchmark/ctrl/mcast_msg go`;
  while(<RUN>) {
    next unless(/bound/ || /proc: 0/);
    print;
    m/^(.*): proc.*rate: (\d+\.\d\d), .*lat: (\d+\.\d\d)/;
    $host=$1;
    $gps+=$2;
    $lat=$3 if($host eq $phost)
  }
  close(RUN);
 
  return($gps, $lat);
}
sub start_server {
  my $rc;
  $rc=system("ssh -f $_[0] /scratch/mcd/memcached -t $_[2] $_[1] 2>&1");
  if($rc>>8) {
    return(0);
  }  
  return(1);
}

sub stop_server {
  system "ssh -q $_[0] pkill memcached";
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
  my @required=qw(send host port);
  my $key;
  my $port;
  for $key (@required) {
    unless (exists($cfg{$key})) {
      print "missing required config entry: $cfg\n";
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
      print CONF ",value_size=64";
    }
    if(exists($cfg{key_len})) {
      print CONF ",key_len=".$cfg{key_len};
    } else {
      print CONF ",key_len=36";
    }
    if(exists($cfg{key_count})) {
      print CONF ",key_count=".$cfg{key_count};
    } else {
      print CONF ",key_count=65536";
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
    if(exists($cfg{mget})) {
      print CONF ",ops_per_send=".$cfg{mget};
    } else {
      print CONF ",ops_per_send=1";
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
    print CONF "\n";
  }
  close(CONF);
  return(1);
}



