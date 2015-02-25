#!/usr/bin/perl


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
my $time=2;
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

#data size to SETs and GETs
my $data_size=32;
my $key_size=8;


#my $key_count=65536;
my $op="MGET";
my $op_count=2048;
my $keys_per_op=32;
my $out=1;
my $delay=1000;
my $samples=10;

GetOptions("dir=s" => \$DIR,
	   "mchammr=s" => \$MCHAMMR,
	   "memcached=s" => \$MEMCACHED,
	   "conns=i" => \$client_conns,
	   "server_threads=i" => \$server_threads,
	   "client_threads=i" => \$threads,
	   "server_name=s" => \$host_ssh,
	   "probe_host=s" => \$phost,
	   "load_host=s" => \@bhosts,
	   "data_size=i" => \$data_size,
	   "key_size=i" => \$key_size,
	   "server_ip=s" => \$host,
	   "base_port=i" => \$port,
	   "port_count=i" => \$port_count,
	   "op_count=i" => \$op_count,
	   "keys_per_op=i" => \$keys_per_op,
	   "op=s" => \$op,
	   "out=i" => \$out,
	   "delay=i" => \$delay,
	   "time=i" => \$time,
	   "samples=i" => \$samples,
);

$MCHAMMR="$DIR/mc-hammr/mc-hammr" if($MCHAMMR eq "");
$CTRL="$DIR/ctrl/mcast_msg" if ($CTRL eq "");

#calculate number of connections per thread to make the total connections add up
#to what we want
my $conns=int($client_conns/scalar(@bhosts)/$threads);


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
		delay=>0,
		time=>$time,
		threads=>4,
		size=>$size,
		key_len=>$key_size,
		value_size=>$data_size,
		op_count=>($op_count*$keys_per_op),
		out=>1,
		conns=>1,
		wait=>1})) {
  die("error making config file: $probe_conf");
}

$t_min=1;
$t_max=16;
for $mget_size (qw(1 16 32)) {
  print("mget=$mget_size\n");
  print("threads,g/s,lat\n");
  for($threads=$t_min; $threads<=$t_max; $threads++) {
    ($gps, $avg_lat)=mget_bench($mget_size);
    printf("%d, %.2f, %.2f\n", $threads, $gps, $avg_lat);
  }
  print "\n\n";
}

exit(0);

sub mget_bench {
  my $mget_size=shift;
  #put the "load" config file on the client system we will use to load the data
  print "loading the \"load\" config on $phost...\n";
  if(!load_conf($load_conf, $phost)) {
    die("error loading config file: $load_conf");
  }
  start_server($host, $port, $port_count, $threads, 16384, (256*$threads));
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
		      wait=>1})) {
    die("error making config file: $bench_conf");
  }
  print "load benchmark config file on hosts: " . join(", ", @bhosts) . "\n";
  if(!load_conf($bench_conf, @bhosts)) {
    die("error loading config file: $bench_conf");
  }

  ($gps, $avg_lat)=bench($samples, $conns_per_thread*$threads, $period, $mget_size);
  stop_server($host);
  return($gps, $avg_lat);
}

sub bench {
  my $timeout=shift;
  my $phost=shift;
  my $ready_count=scalar(@_)+1;
  my $line="";
  my $lat=undef;
  my $gps;
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
    next unless(/bound/ || /proc: 0/);
    print;
    m/^(.*): proc.*rate: (\d+\.\d\d), .*lat: (\d+\.\d\d)/;
    $host=$1;
    $gps+=$2;
    $lat=$3 if($host eq $phost)
  }
  close(RUN);
  clean_client("$phost,". join(",", @_));
  return($gps, $lat);
}

sub start_server {
  #start up the server
  my $host_port_string="";
  my $host=shift;
  my $port=shift;
  my $port_count=shift;
  my $server_threads=shift;
  my $server_mem=shift;
  my $server_conns=shift;

  for($p=$port; $p<$port+$port_count; $p++) {
    $host_port_string .= "-l $host:$p ";
  }
  if($port_count==1) {
    print "starting server on $host:$port...\n";
  } else {
    print "starting server on $host:$port-" . ($port + $port_count - 1) . "...\n";
  }
  print "ssh -f $host_ssh $MEMCACHED -t $server_threads -m $server_mem -c $server_conns  $host_port_string 2>&1\n";
  #return(1);
  $rc=system("ssh -f $host_ssh $MEMCACHED -t $server_threads -m $server_mem -c $server_conns  $host_port_string 2>&1");
  if($rc>>8) {
    return(0);
  }  
  print "waiting on server to stabilize (60 sec)\n";
  sleep(60);
  return(1);
}


sub stop_server {
  system "ssh -q $_[0] pkill " . basename($MEMCACHED);
  sleep(15);
}

sub clean_client {
  system "ssh -q $_[0] pkill " . basename($MCHAMMR);
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
