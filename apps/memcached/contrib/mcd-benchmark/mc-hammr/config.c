#include "config.h"
#include "mc-hammr.h"
#include <pthread.h>
#include <sys/epoll.h>
#include <signal.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/resource.h>
#include "protocol_binary.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <netinet/in.h>
char *const key_options[] = {
  [SEND]             	= "send",
  [RECV]             	= "recv",
  [CONNS_PER_THREAD]	= "conns_per_thread",
  [THREADS]		= "threads",
  [KEY_PREFIX]       	= "key_prefix",
  [KEY_LEN]		= "key_len",
  [OP_COUNT]        	= "op_count",    
  [VALUE_SIZE]       	= "value_size",
  [HOST]             	= "host",
  [SRC]			= "src",
  [PORT]             	= "port",
  [RATE]    	        = "rate",
  [DELAY]        	= "delay",
  [LOOP]             	= "loop",
  [JITTER]           	= "jitter",  
  [OUT]			= "out",
  [FORK]		= "fork",
  [TIME]		= "time",
  [TX_RES]		= "tx_res",
  [RND]			= "rnd",  
  [OPS_PER_CONN]	= "ops_per_conn",
  [PROTO]		= "proto",
  [MCAST_WAIT]		= "mcast_wait",
  [UDP]			= "udp",
  [MGET_SIZE]		= "mget_size",
  [OPS_PER_FRAME]	= "ops_per_frame",
  [MAX_LAT]		= "max_lat",
  NULL	
};

char *const req_type_string[] = {
  [GET]		="GET",
  [GETQ]	="GETQ",
  [MGET]	="MGET",
  [MGETQ]	="MGETQ",
  [SET]		="SET",
  [SETQ]	="SETQ",
  NULL
};

int load_config(config_t ***config, int *thread_count, int *conn_count, char *filename) {
  int i=0;
  config_t *tmp_cfg;
  char line[4096];
  FILE * cfg_file;
  *thread_count=0;
  *conn_count=0;    
  cfg_file = fopen(filename, "r");
  if (cfg_file == NULL) {
    perror("Opening config file: ");
    exit(1);
  }

  while (fgets(line, 4096, cfg_file) != NULL) {
    if((tmp_cfg=parse_config_line(line))) {
      //we got back a valid config structure and need to store it in our array.  If the pointer
      //comes back NULL, then the line was either commented out or empty and we can ignore it.
      *config=realloc(*config, sizeof(config_t *)*(i+1));
      (*config)[i]=tmp_cfg;
      //We need to track the thread count and connection count so we can allocate enough 
      //shared memory for all processes to dump there stats to.  We are going to hide the fact
      //that ASYNC mode actual creates 2x the number of threads listed in the config file since
      //stats will be shared between the reader and writer threads.
      if(tmp_cfg->recv_mode==ASYNC) {
	*thread_count+=tmp_cfg->threads/2;
	*conn_count+=(tmp_cfg->conns_per_thread*(tmp_cfg->threads/2));
      } else {
	*thread_count+=tmp_cfg->threads;
	*conn_count+=(tmp_cfg->conns_per_thread*tmp_cfg->threads);
      }
      i++;
    }    
  }
  fclose(cfg_file);
  return(i);
}


config_t * parse_config_line(char *cfg_line) {
  config_t *cfg;
  char *tmp=0;
  char *in_progress;
  char *token;
  char *value;
  cfg_key_t key = 0;
  int rate;
  //skip any config lines that are commented out
  if(*cfg_line=='#') {
    printf("ignoring comment: %s\n", cfg_line);
    return(NULL);
  }
  //Chomp the ending newline
  tmp = rindex(cfg_line, '\n');
  if (tmp != NULL) {
    *tmp = '\0';
  }
  //see if all we had was a new line, and if so, we skip
  if(*cfg_line=='\0') {
    return(NULL);
  } 
  
  cfg=malloc(sizeof(config_t));
  if(!cfg) {
    perror("Unable to allocate config structure.");
    return(0);
  }
  //We will give the config some sane defaults.  If no other information is given, these will be 
  //used.  By default we will send 1000 binarys GETs.  For each get, we will setup a connection.
  //Perform the transaction and close down.  
  strcpy(cfg->ip_addr, "127.0.0.1");
  cfg->src_addr[0]=0;
  cfg->port_num=11211;
  cfg->req_type=GET;
  cfg->recv_mode=SYNC;
  cfg->key_count=1000;
  cfg->value_size=64;
  cfg->delay=0;
  cfg->jitter=0;
  cfg->loop=1;
  cfg->time=0;
  cfg->key_prefix=NULL;
  cfg->key_len=4;
  cfg->rnd_keys=0;
  cfg->tx_res=0;
  cfg->ops_per_frame=1;
  cfg->frames_per_conn=0;
  cfg->conns_per_thread=1;
  cfg->threads=1;
  cfg->max_outstanding=1;
  cfg->tx_watermark=0;
  cfg->do_fork=0;
  cfg->proto=BINARY;
  cfg->mcast_wait=0;
  cfg->sock_type=SOCK_STREAM;
  cfg->max_lat=2;
  
  //We tokenize the line and check each token in a big switch statement.
  while ((token = strtok_r(cfg_line, ",", &in_progress)) != NULL) {
    cfg_line = NULL;	
    value = index(token, '=');
    //Don't core dump if the user didn't specify a value for this key.  Just skip it
    //and move on
    if(!value) {
      continue;
    }
    *value = '\0';
    value++;
    key=0;
    //We have a key and we need to determine which one we have.  
    while (key_options[key] != NULL) {
      if (strcmp(token, key_options[key]) == 0)
	break;
      key++;
    }  
    //With a key identified, go do whatever setup is needed.  
    switch (key) { 
      case SEND:
	//figure out what type of 
	cfg->req_type=0;
	while(req_type_string[cfg->req_type]!=NULL) {
	  if(strcmp(req_type_string[cfg->req_type], value)==0) {
	    break;
	  }
	  cfg->req_type++;
	}
	if(cfg->req_type==INV) {
	  error_at_line(1,0,__FILE__,__LINE__, "Invalid request type in config file: %s\n", value);
	}
	
	break;
      case RECV:
	if(strcmp("sync", value)==0) {
	  cfg->recv_mode=SYNC;
	} else if(strcmp("async", value)==0) {
	  cfg->recv_mode=ASYNC;
	} else {
	  error_at_line(1,0,__FILE__,__LINE__, "Invalid receive mode in config file: %s\n", value);
	}  
	break;
      case CONNS_PER_THREAD:
	cfg->conns_per_thread=atoi(value);
	break;
      case THREADS:
	cfg->threads=atoi(value);
	break;
      case KEY_PREFIX:
	cfg->key_prefix=malloc(strlen(value)+1);
	strcpy(cfg->key_prefix, value);
	break;
      case KEY_LEN:
	cfg->key_len=atoi(value);
	break;
      case OP_COUNT:
	cfg->frame_count=atoi(value);
	break;
      case VALUE_SIZE:
	cfg->value_size=atoi(value);
	break;
      case HOST:
	strcpy(cfg->ip_addr, value);
	break;
      case SRC:
	strcpy(cfg->src_addr, value);
	break;
      case PORT:
	cfg->port_num=atoi(value);
	break;
      case RATE:
	rate=atoi(value);
	//if a rate was specified, we need to convert that into a intertransaction delay
	if(rate) {
	  cfg->delay=1000000.0/rate;
	} else {
	  cfg->delay=0;
	}
	break;
      case DELAY:
	cfg->delay=strtod(value, NULL);
	break;
      case LOOP:
	cfg->loop=atoi(value);
	break;
      case JITTER:
	cfg->jitter=atoi(value);
	break;
      case OUT:
	cfg->max_outstanding=atoi(value);
	break;
      case FORK:
	cfg->do_fork=atoi(value);
	break;
      case TIME:
	cfg->time=atoi(value);
	break;
      case TX_RES:
	cfg->tx_watermark=atoi(value);
	break;
      case RND:
	cfg->rnd_keys=atoi(value);
	break;	  
      case MGET_SIZE:
      case OPS_PER_FRAME:
	cfg->ops_per_frame=atoi(value);
	break;	  
      case OPS_PER_CONN:
	cfg->frames_per_conn=atoi(value);
	break;	  
      case PROTO:
	if(strcmp("ascii", value)==0) {
	  cfg->proto=ASCII;
	} else {
	  cfg->proto=BINARY;
	}
	break;	 
      case MCAST_WAIT:
	cfg->mcast_wait=atoi(value);
	break;
      case UDP:
	if(atoi(value)) {
	  cfg->sock_type=SOCK_DGRAM;
	}
	break;
      case MAX_LAT:
	cfg->max_lat=atoi(value);
	break;
      default:
	printf("warning:  unknown config option -- %s=%s\n", token, value);
	break;
    }
  }
  //At this point we have fully parsed the config file line and extracted everything from it.  Some of the 
  //options will interact and we need to sanitize the full option set.
  
  //specifying a runtime overrides specifying a number of times to loop across all keys.
  if(cfg->time) {
    cfg->loop=0;
  }
   
  //if we are doing async receives then we need a receive thread for every send thread so we need to
  //double out thread count here.  The start_client() code later understands what to do with the extra
  //threads
  if(cfg->recv_mode==ASYNC) {
    cfg->threads*=2;
  }
  
  
  //fixup jitter if jitter is greater max delay.  
  if(cfg->jitter>cfg->delay) {
    cfg->jitter-=cfg->delay;
  } else {
    //jitter is a max variation in the delay between packets.  This is implemented as a +/- variation in delay.
    //To max the logic easier later, we decrease delay by half the jitter and then later we can just add a random
    //value between 0 and jitter.  The average will get us back to the original delay.
    cfg->delay-=cfg->jitter/2;
  }  
    
  return(cfg);
  
}
