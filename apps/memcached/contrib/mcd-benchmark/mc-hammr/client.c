#include "client.h"
#include "connection.h"
#include "thread.h"
#include <inttypes.h>

void start_client(config_t *config) {
  thread_t *threads=NULL;
  int i;
  int j;
  int rc;
  int  *status;
  int mode;
  conn_stats_t **tstat;
  pthread_attr_t attr;
  struct rlimit limits;
  struct sigaction action;
  sigset_t mask;
  getrlimit( RLIMIT_NPROC ,&limits);
  if(limits.rlim_max<config->threads+2) {
    error_at_line(0,0,__FILE__,__LINE__, "Insufficient resources: NPROC < %d\n", config->threads+2);
    return;
  }
  limits.rlim_cur=limits.rlim_max;
  setrlimit(RLIMIT_NPROC, &limits);
  
  threads=malloc(sizeof(thread_t)*config->threads);
  memset(threads, 0, sizeof(thread_t)*config->threads);
  
  for(i=0; i<config->threads; i++) {
    threads[i].thread_idx=i;
    if(config->recv_mode==ASYNC) {
      if(i%2) {
	mode=THREAD_ASYNC_RECV;
      } else {
	mode=THREAD_ASYNC_SEND;
      }
    } else {
      mode=THREAD_SYNC;
    }
    
    switch(mode) {
      case THREAD_SYNC:

	break;
      case THREAD_ASYNC_RECV:
	threads[i].mode=THREAD_ASYNC_RECV;
	break;
      case THREAD_ASYNC_SEND:
	threads[i].mode=THREAD_ASYNC_SEND;
	threads[i].read=&threads[i+1];
	threads[i].efd_write=epoll_create1(0);
	threads[i].write=&threads[i];	
	//Since we are in ASYNC mode, we are hiding the extra 2x threads
	//for stats keeping.	
	threads[i].thread_stats=&(config->stats->thread_stats[i/2]);
      default:
	break;
    }   
    threads[i].mode=mode;
    if(mode==THREAD_SYNC || mode==THREAD_ASYNC_SEND) {    
      	threads[i].conns=malloc(sizeof(connection_t)*config->conns_per_thread);
	memset(threads[i].conns,0,sizeof(connection_t)*config->conns_per_thread);
	threads[i].write=&threads[i];
	threads[i].efd_write=epoll_create1(0);
	if(mode==THREAD_SYNC) {
	  threads[i].read=&threads[i];
	  threads[i].efd_read=epoll_create1(0);	
	  threads[i].thread_stats=&(config->stats->thread_stats[i]);
	} else {
	  threads[i].read=&threads[i+1];
	//Since we are in ASYNC mode, we are hiding the extra 2x threads
	//for stats keeping.
	  threads[i].thread_stats=&(config->stats->thread_stats[i/2]);
	}
    } else {
	threads[i].read=&threads[i];
	threads[i].write=&threads[i-1];
	threads[i].efd_read=epoll_create1(0);
	threads[i].thread_stats=&(config->stats->thread_stats[i/2]);  
	threads[i].conns=threads[i-1].conns;
    }
    threads[i].cfg=config;
    if(mode==SYNC || i%2==0) {
      for(j=0; j<config->conns_per_thread; j++) {
	tstat=&threads[i].conns[j].conn_stats;
	threads[i].conns[j].conn_stats=&threads[i].thread_stats->conn_stats[j];
	threads[i].conns[j].cfg=config;
	init_connection_data(config, &threads[i].conns[j], mode);
      }
    }

  }
  if(config->mcast_wait) {
    //printf("mcast_wait\n");
    mcast_wait();
  }
  action.sa_flags=SA_SIGINFO;
  action.sa_sigaction=run_timeout;
  sigemptyset(&action.sa_mask);
  sigaction(SIG_RUNTIMEEXP, &action, NULL);
  sigemptyset(&mask);
  sigaddset(&mask, SIG_RUNTIMEEXP);
  if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1) {
      perror("sigprocmask");
  }
  if(config->threads==1) {
    rc=(long int)client_thread((void*)&threads[0]);
  } else {
    for(i=0; i<config->threads; i++) {
      pthread_attr_init(&attr);
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
      rc=pthread_create(&threads[i].id, &attr, client_thread, (void *)&threads[i]);
      pthread_attr_destroy(&attr);
    }
    for(i=0; i<config->threads; i++) {
      //printf("waiting on thread %d... ", i);
      rc=pthread_join(threads[i].id,(void **)&status);
      //printf("joined thread %d, status=%ld\n",(int)threads[i].id, (long int)status);
    }
  }
  sigemptyset(&mask);
  sigaddset(&mask, SIG_RUNTIMEEXP);
  if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
      perror("sigprocmask");
  }
  action.sa_flags=0;
  action.sa_handler=SIG_IGN;
  sigemptyset(&action.sa_mask);
  sigaction(SIG_RUNTIMEEXP, &action, NULL);
  
  for(i=0; i<config->threads; i++) {
    if(threads[i].conns) {
      for(j=0;j<config->conns_per_thread; j++) {
	if(threads[i].conns[j].req_buffer) {
	  free(threads[i].conns[j].req_buffer);
	}
	if(threads[i].conns[j].rx_buff) {
	  free(threads[i].conns[j].rx_buff);
	}
	if(threads[i].conns[j].tx_buffer) {
	  free(threads[i].conns[j].tx_buffer);
	}
	if(threads[i].conns[j].frame_buffer) {
	  free(threads[i].conns[j].frame_buffer);
	}

	//if(threads[i].conns[j].out_transactions) {
	//  free(threads[i].conns[j].out_transactions);
	//}
      }
      free(threads[i].conns);
      threads[i].read->conns=NULL;
      threads[i].write->conns=NULL;
    }
  }
  
  free(threads);
  return;
  
}

int build_request(config_t *cfg, char *buffer, int key) {
  protocol_binary_request_get *bin_get=0;
  protocol_binary_request_set *bin_set=0;
  udp_frame_header_t frame_header;
  int offset=0;
  int i;
  int prefix_len=strlen(cfg->key_prefix);
  bin_get=(protocol_binary_request_get *)buffer;
  bin_set=(protocol_binary_request_set *)buffer;
  if(cfg->proto==ASCII) {
    switch(cfg->req_type) {
      case GET:
	offset+=sprintf(buffer, "get %s%0*x\r\n", cfg->key_prefix, cfg->key_len-prefix_len, key);
	break;
      case MGET:
	offset+=sprintf(buffer, "get");
	for(i=0; i<cfg->ops_per_frame; i++) {
	  offset+=sprintf(buffer+offset, " %s%0*x", cfg->key_prefix, cfg->key_len-prefix_len,key);
	  key++;
	}
	offset+=sprintf(buffer+offset, "\r\n");
	break;
      case GETQ:
	offset+=sprintf(buffer, "gets %s%0*x noreply\r\n", cfg->key_prefix, cfg->key_len-prefix_len,key);
	break;
      case MGETQ:
	offset+=sprintf(buffer, "gets");
	for(i=0; i<cfg->ops_per_frame; i++) {
	  offset+=sprintf(buffer+offset, " %s%0*x", cfg->key_prefix, cfg->key_len-prefix_len,key);
	  key++;
	}
	offset+=sprintf(buffer+offset, "\r\n");
	break;
      case SET:
	offset+=sprintf(buffer, "set %s%0*x 0 0 %d\r\n", cfg->key_prefix, cfg->key_len-prefix_len,key, cfg->value_size);
	break;
      case SETQ:
	offset+=sprintf(buffer, "set %s%0*x 0 0 %d noreply\r\n", cfg->key_prefix, cfg->key_len-prefix_len,key, cfg->value_size);
	break;
    }
  } else if (cfg->proto==BINARY) {
    switch(cfg->req_type) {
      case GET:
      case MGET:
	bin_get->message.header.request.magic = PROTOCOL_BINARY_REQ;
	bin_get->message.header.request.opcode = PROTOCOL_BINARY_CMD_GET;
	bin_get->message.header.request.keylen = htons(cfg->key_len);
	bin_get->message.header.request.extlen = 0;
	bin_get->message.header.request.bodylen = htonl(cfg->key_len);
	bin_get->message.header.request.opaque = htonl(key);
	bin_get->message.header.request.cas = 0; 
	offset+=sizeof(protocol_binary_request_get);
	break;
      case GETQ:
      case MGETQ:
	bin_get->message.header.request.magic = PROTOCOL_BINARY_REQ;
	bin_get->message.header.request.opcode = PROTOCOL_BINARY_CMD_GETQ;
	bin_get->message.header.request.keylen = htons(cfg->key_len);
	bin_get->message.header.request.extlen = 0;
	bin_get->message.header.request.bodylen = htonl(cfg->key_len);
	bin_get->message.header.request.opaque = htonl(key);
	bin_get->message.header.request.cas = 0; 
	offset+=sizeof(protocol_binary_request_get);
	break;
      case SET:
	bin_set->message.header.request.magic = PROTOCOL_BINARY_REQ;
	bin_set->message.header.request.opcode = PROTOCOL_BINARY_CMD_SET;
	bin_set->message.header.request.keylen = htons(cfg->key_len);
	bin_set->message.header.request.extlen = 8; /* flags + exptime */
	bin_set->message.header.request.bodylen = htonl(cfg->key_len+cfg->value_size+8);
	bin_set->message.header.request.opaque = htonl(key);
	bin_set->message.header.request.cas = 0; 
	bin_set->message.body.flags = 0;
	bin_set->message.body.expiration = 0; 
	offset+=sizeof(protocol_binary_request_set);
	break;
      case SETQ:
	bin_set->message.header.request.magic = PROTOCOL_BINARY_REQ;
	bin_set->message.header.request.opcode = PROTOCOL_BINARY_CMD_SETQ;
	bin_set->message.header.request.keylen = htons(cfg->key_len);
	bin_set->message.header.request.extlen = 8; /* flags + exptime */
	bin_set->message.header.request.bodylen = htonl(cfg->key_len+cfg->value_size+8);
	bin_set->message.header.request.opaque = htonl(key);
	bin_set->message.header.request.cas = 0; 
	bin_set->message.body.flags = 0;
	bin_set->message.body.expiration = 0;
	offset+=sizeof(protocol_binary_request_set);
	break;
      default:
	error_at_line(2,0,__FILE__,__LINE__, "Unknown binary request: %d\n", cfg->proto);
	break;
    }   
    bin_set->message.header.request.datatype=0;
    bin_set->message.header.request.reserved=0;
    offset+=sprintf(buffer+offset, "%s%0*x", cfg->key_prefix, cfg->key_len-prefix_len,key);
  } else {
    error_at_line(2,0,__FILE__,__LINE__, "Unknown protocol: %d\n", cfg->proto);
  }
  //If we have data then we need to init the data portion of the packet.
  if(cfg->req_type==SET || cfg->req_type==SETQ) {
    memset(buffer+offset, 0x5A, cfg->value_size);
    offset+=cfg->value_size;
    if(cfg->proto==ASCII) {
      offset+=sprintf(buffer+offset, "\r\n");
    }
  }
  return(offset);
}

int calc_request_size(config_t *cfg) {
  int req_size;
  char tmp_data_size[32];
  if(cfg->proto==BINARY) {
    switch(cfg->req_type) {
      case GET:
      case GETQ:
      case MGET:
      case MGETQ:
	req_size=sizeof(protocol_binary_request_no_extras)+cfg->key_len;
	break;
      case SET:
      case SETQ:
	req_size=sizeof(protocol_binary_request_set)+cfg->key_len+cfg->value_size;
	break;
      default:
	error_at_line(2,0,__FILE__,__LINE__, "Unknown binary request: %d\n", cfg->proto);
	break;
    }
  } else if(cfg->proto==ASCII) {
    switch(cfg->req_type) {     
      case GET:
	//"get "=4
	
	//key_len=x
	//"\r\n"=2
	//4+x+2=6+x
	req_size=6+cfg->key_len;
	break;
      case GETQ:
	//"get "=5
	//key_len=x
	//"\r\n"=2
	//4+x+2=6+x
	req_size=7+cfg->key_len;	
	break;
      case MGET:
	//get=3
	//for each key, " "=1
	//for each key, key_len=x
	//"\r\n"=2
	//3+((1+x)*count)+2=5+((1+x)*count)
	req_size=5+((cfg->key_len+1)*cfg->ops_per_frame);
	break;
      case MGETQ:
	//gets=4
	//for each key, " "=1
	//for each key, key_len=x
	//"\r\n"=2
	//4+((1+x)*count)+2=6+((1+x)*count)
	req_size=6+((cfg->key_len+1)*cfg->ops_per_frame);
	break;	
      case SET:
	//"set "=4
	//key_len=x
	//" 0 0 "=5
	//strlen(value_size)=y
	//"\r\n"=2
	//sizeof(value)=z
	//"\r\n"=2
	//4+x+5+y+2+z+2=13+x+y+z
	req_size=13+cfg->key_len+sprintf(tmp_data_size, "%d", cfg->value_size)+cfg->value_size;
	break;
      case SETQ:
	//"set "=4
	//key_len=x
	//" 0 0 "=5
	//strlen(value_size)=y
	//" noreply"=8
	//"\r\n"=2
	//sizeof(value)=z
	//"\r\n"=2
	//4+x+5+y+8+2+z+2=13+x+y+z
	req_size=21+cfg->key_len+sprintf(tmp_data_size, "%d", cfg->value_size)+cfg->value_size;	
	break;
      default:
	error_at_line(2,0,__FILE__,__LINE__, "Unknown ASCII request: %d\n", cfg->proto);
	break;
    } 
  } else {
    error_at_line(2,0,__FILE__,__LINE__, "Unknown protocol: %d\n", cfg->proto);
  }    
  return(req_size);
}

int calc_response_size(config_t *cfg) {
  int resp_size;
  char tmp_data_size[32];
  if(cfg->proto==BINARY) {
    switch(cfg->req_type) {
      case GET:
      case GETQ:
      case MGET:
      case MGETQ:
	resp_size=sizeof(protocol_binary_response_get)+cfg->value_size;
	break;
      case SET:
      case SETQ:
	resp_size=sizeof(protocol_binary_response_set);
	break;
      default:
	error_at_line(2,0,__FILE__,__LINE__, "Unknown binary request: %d\n", cfg->proto);
	break;
    }
  } else if(cfg->proto==ASCII) {
    switch(cfg->req_type) {     
      case GET:
      case GETQ:
	//"VALUE "=6
	//key_len=x
	//" 0 "=3
	//strlen(value_size)=y
	//"\r\n"=2
	//value_size=z
	//"\r\n"=2
	//"END\r\n"=5
	//6+x+3+y+2+z+2+5=18+x+y+x
	resp_size=18+cfg->key_len+sprintf(tmp_data_size, "%d", cfg->value_size)+cfg->value_size;
	break;
      case MGET:
      case MGETQ:
	//for each key
	//"VALUE "=6
	//key_len=x
	//" 0 "=3
	//strlen(value_size)=y
	//"\r\n"=2
	//value_size=z
	//"\r\n"=2
	//Final "END\r\n"=5
	//(6+x+3+y+2+z+2)*count+5=(13+x+y+z)*count+5
	resp_size=((13+cfg->key_len+sprintf(tmp_data_size, "%d", cfg->value_size)+cfg->value_size)*cfg->ops_per_frame)+5;
	break;
      case SET:
      case SETQ:
	//Max length response is "NOT_STORED\r\n" == 12 characters
	resp_size=12;
	break;
      default:
	error_at_line(2,0,__FILE__,__LINE__, "Unknown ASCII request: %d\n", cfg->proto);
	break;
    } 
  } else {
    error_at_line(2,0,__FILE__,__LINE__, "Unknown protocol: %d\n", cfg->proto);
  }  
  //If we are using UDP, then each response, whether ASCII or binary, will be wrapper in a memcached frame
  //so we have to add the size of the frame header
  if(cfg->sock_type==SOCK_DGRAM) {
    resp_size+=sizeof(udp_frame_header_t);
  }
  return(resp_size);
}


unsigned char process_response(connection_t *conn, unsigned long key, unsigned char status, struct timespec *ts) {
  int ret=0;
  int n;
  int j;
  unsigned long long int elapsed;
  //This assignment has a small potential for blowing up if sizeof(int) is such that an update
  //is not atomic.  Specifically if sizeof(int) > than the native word size of the processor.
  int stop_index=conn->frame_send->index;
  //might be a timed out response.  Need to see if the frame is active.  If not, just drop the
  //response as we have already accounted for it.  If the frame is active then we have a bunch
  //of misses we need to clean up.
  if(key!=conn->frame_recv->active  ) {
    //printf("op: %ld, act: %d, fstart: %d, fend: %d\n", resp_opaque, conn->frame_recv->active,
    //conn->frame_recv->req_start_index,conn->frame_recv->req_end_index);
    //received a response that does not match the expected key.  One of two things has happened. 
    //Either we are sending GETQ/MGETQ and have had some misses, or we have exeeded the max
    //latency value and this is a response to a request we already timed out.
    
    //first lets check and see if the request correspoding to this response is still active.
    if(conn->req_buffer[key].active) {
      //So we have a response to an active request that doesn't line up with the response id
      //we were expecting.  We either have dropped packets (UDP) or misses on GETQ/MGETQ
      //requests.  We are going to have to walk through our outstanding frames until we find 
      //the request and mark everything we pass as a miss and inactive.
      while(conn->frame_recv->index!=stop_index) {
	//Check to see if the response belongs to the current frame.  We have to check both 
	//start and end because the frame list is a circular list and the request id will 
	//loop around.
	if(key < conn->frame_recv->req_start_index || key > conn->frame_recv->req_end_index) {
	  miss_remaining_frame(conn, ts);
	} else {
	  //The response falls somewhere in the current frame.
	  //Figure out how many misses we have in this frame.
	  n=key-conn->frame_recv->req_start_index;
	  elapsed=tv_diff_nsec(&conn->frame_recv->ts_send ,ts);
	  if(n) {
	    printf("MISS 0!!!!\n");
	    conn->conn_stats->miss_count+=n;
	    conn->conn_stats->miss_time+=n*elapsed;
	    UPDATE_MIN_MAX(conn->conn_stats->miss,elapsed)	    
	    conn->frame_recv->active+=n;
	    for(j=conn->frame_recv->active; j<key; j++) {
	      conn->req_buffer[j].active=0;
	    }    
	  }
	  //We found the request and now we need to account for the hit or miss response
	  if(status==0) {	
	    conn->conn_stats->hit_count++;
	    conn->conn_stats->hit_time+=elapsed;	
	    UPDATE_MIN_MAX(conn->conn_stats->hit, elapsed)
//	    if(elapsed>conn->conn_stats->hit_max) {
//	      conn->(conn_stats->hit_max=elapsed;
//	    } 
//	    if(conn->conn_stats->hit_min==0 || elapsed<conn->conn_stats->hit_min) {
//	      conn_stats->hit_min==elapsed
//	    }
	  } else {
	    printf("MISS 1!!!!\n");
	    conn->conn_stats->miss_count++;
	    conn->conn_stats->miss_time+=elapsed;		    
	    UPDATE_MIN_MAX(conn->conn_stats->miss,elapsed)
	  }	  
	  //mark the request inactive
	  conn->req_buffer[key].active=0;
	  if(conn->frame_recv->active==conn->frame_recv->req_end_index) {
	    //Mark frame inactive
	    conn->frame_recv->active=-1;
	    sem_post(&conn->frame_count);	  
	    //and move on to the next frame
	    conn->frame_recv=conn->frame_recv->next;
	    conn->receive_count++;
	    ret=1;
	  } else {
	    conn->frame_recv->active++;
	  }
	  break;
	}
      }		    
    } else { //conn->req_buffer[resp_opaque].active
		    //Got a response to a request that is already inactive.  This is either a duplicate 
		    //response, or a response to a request that has been timed out.  In any case we have
		    //already accounted for the response as either a hit or miss as appropriate.  We can
		    //just drop this response on the floor and carry on.
		    
		    //Might want to do some validation of the memory structures here at some point.
    }
  } else {
    //We got a response to the current request.  Need to account for the hit, or miss, as
    //appropriate.
    elapsed=tv_diff_nsec(&conn->frame_recv->ts_send ,ts);
    conn->conn_stats->response_count++;
    if(status==0) {	
      conn->conn_stats->hit_count++;
      conn->conn_stats->hit_time+=elapsed;
      UPDATE_MIN_MAX(conn->conn_stats->hit,elapsed)
    } else {
      printf("MISS 2!!!\n");
      conn->conn_stats->miss_count++;
      conn->conn_stats->miss_time+=elapsed;
      UPDATE_MIN_MAX(conn->conn_stats->miss,elapsed)
    }
    //mark the request inactive
    conn->req_buffer[conn->frame_recv->active].active=0;
    //check to see if this was the last active request in this frame
    if(conn->frame_recv->active==conn->frame_recv->req_end_index) {
      //Mark frame inactive
      conn->frame_recv->active=-1;
      sem_post(&conn->frame_count);	  
      //and move on to the next frame
      conn->frame_recv=conn->frame_recv->next;
      conn->receive_count++;
      ret=1;
    } else {
      //Still outstanding requests for this frame, so move on to the next one
      conn->frame_recv->active++;
    }		  
  }  
  return(ret);
  
}

void miss_remaining_frame(connection_t *conn, struct timespec *ts) {
  int n;
  int j;
  unsigned long long int elapsed=tv_diff_nsec(&conn->frame_recv->ts_send,ts);
  int send_index=conn->frame_send->index;
  n=(conn->frame_recv->req_end_index-conn->frame_recv->active)+1;
  #ifdef TIMEOUT_DEBUG
  printf("miss remaining: thread=%d, connection=%d\n", conn->thread_idx, conn->conn_idx); 
  printf("  conn->frame_recv->index=%d, conn->frame_send->index=%d\n", conn->frame_recv->index,send_index);      
  printf("    wait_time=%lld\n", elapsed); 
  printf("    timing out frame %d\n", conn->frame_recv->index);	
  printf("       conn->frame_recv->active=%d,  conn->frame_recv->req_end_index=%d, n=%d\n",
	 conn->frame_recv->active, conn->frame_recv->req_end_index, n);	
  if(n<1) {
    printf("!!! n<1.  This should not happen.  Dumping frames...\n");
    printf("=============  RECV FRAMES =============\n");
    //we don't have a prev pointer in the frame structure so we have to go through a bit 
    //of gymnastics to figure out the previous frame.
    if(conn->frame_recv->index==0) {
      display_frame(&conn->frame_buffer[conn->cfg->frame_count]);
    } else {
      display_frame(&conn->frame_buffer[conn->frame_recv->index-1]);
    }
    display_frame(conn->frame_recv);
    display_frame(conn->frame_recv->next);
    
    printf("=============  SEND FRAMES =============\n");
    if(send_index==0) {
      display_frame(&conn->frame_buffer[conn->cfg->frame_count]);
    } else {
      display_frame(&conn->frame_buffer[send_index-1]);
    }
    display_frame(&conn->frame_buffer[send_index]);
    display_frame(conn->frame_buffer[send_index].next);	  
  }
  #endif 
  conn->conn_stats->miss_count+=n;
  conn->conn_stats->miss_time+=n*elapsed;
  UPDATE_MIN_MAX(conn->conn_stats->miss,elapsed)
  //mark all the remaining requests in this frame inactive
  for(j=conn->frame_recv->active; j<=conn->frame_recv->req_end_index; j++) {
    conn->req_buffer[j].active=0;
  }
  //mark the frame inactive
  conn->frame_recv->active=-1;
  //and free up the slot in the senders outstanding count
  sem_post(&conn->frame_count);	  
  //And now move on to the next frame.  We should never receive a reponse that we
  //have a request but which we have no active frame which means that if hit the head of the queue, 
  //something is pretty broken and we need to just stop
  conn->frame_recv=conn->frame_recv->next;
  conn->receive_count++;
  if(conn->frame_recv->index==conn->frame_send->index && conn->frame_recv->active==-1) {
    error_at_line(0,0,__FILE__,__LINE__, "Response to active request with no active frame\nMemory structures corrupted.");
    exit(1);
  } 
}

void timeout_requests(connection_t *conn) {
  struct timespec now;
  int j;
  int n;
  int stop_index=conn->frame_send->index;
  long long int wait_time;
  if(conn->frame_recv->index!=stop_index) {
    clock_gettime(CLOCK_MONOTONIC, &now);
    //We have outstanding requests.  We need to drop any frames that have been 
    //waiting longer than our max timeout. We'll walk throught the frame till
    //we find one that has been waiting less than the timeout value, or we run
    //out of frames.
#ifdef TIMEOUT_DEBUG
    printf("timeout: thread=%d, connection=%d\n", conn->thread_idx, conn->conn_idx);
#endif
    while(conn->frame_recv->index!=stop_index) {	  
      wait_time=tv_diff_nsec(&conn->frame_recv->ts_send,&now);
#ifdef TIMEOUT_DEBUG      
      printf("  conn->frame_recv->index=%d, stop_index=%d, conn->frame_send->index=%d\n", conn->frame_recv->index,stop_index, conn->frame_send->index);      
      printf("    wait_time=%lld\n", wait_time);
#endif
      if(wait_time>1000000*conn->cfg->max_lat) {
	n=(conn->frame_recv->req_end_index-conn->frame_recv->active)+1;
#ifdef TIMEOUT_DEBUG      	
	printf("    timing out frame %d\n", conn->frame_recv->index);	
	printf("       conn->frame_recv->active=%d,  conn->frame_recv->req_end_index=%d, n=%d\n",
	       conn->frame_recv->active, conn->frame_recv->req_end_index, n);	
	if(n<1) {
	  printf("!!! n<1.  This should not happen.  Dumping frames...\n");
	  printf("=============  RECV FRAMES =============\n");
	  //we don't have a prev pointer in the frame structure so we have to go through a bit 
	  //of gymnastics to figure out the previous frame.
	  if(conn->frame_recv->index==0) {
	    display_frame(&conn->frame_buffer[conn->cfg->frame_count]);
	  } else {
	    display_frame(&conn->frame_buffer[conn->frame_recv->index-1]);
	  }
	  display_frame(conn->frame_recv);
	  display_frame(conn->frame_recv->next);
	  
	  printf("=============  SEND FRAMES =============\n");
	  if(stop_index==0) {
	    display_frame(&conn->frame_buffer[conn->cfg->frame_count]);
	  } else {
	    display_frame(&conn->frame_buffer[stop_index-1]);
	  }
	  display_frame(&conn->frame_buffer[stop_index]);
	  display_frame(conn->frame_buffer[stop_index].next);	  
	}
#endif
	conn->conn_stats->miss_count+=n;
	conn->conn_stats->timeout_count+=n;
	conn->conn_stats->miss_time+=n*wait_time;
	conn->conn_stats->timeout_time+=n*wait_time;	
	UPDATE_MIN_MAX(conn->conn_stats->miss,wait_time)
	//mark all the remaining requests in this frame inactive
	for(j=conn->frame_recv->active; j<=conn->frame_recv->req_end_index; j++) {
	  conn->req_buffer[j].active=0;
	}
	//mark this frame inactive
	conn->frame_recv->active=-1;
	conn->frame_recv=conn->frame_recv->next;
	conn->receive_count++;
	sem_post(&conn->frame_count);
      } else {
	break;
      }
    }	    	
  }  
  
}

#define PRIxPTR_WIDTH ((int)(sizeof(uintptr_t)*2))

void display_frame(frame_t * frame) {
  printf("--- frame dump ---\n");
  printf("  struct frame_struct {\n");
  printf("    int index=%d\n", frame->index);
  printf("    uint8_t *ptr=0x%0*" PRIxPTR "\n", PRIxPTR_WIDTH, (uintptr_t)frame->ptr);
  printf("    int size=%d\n", frame->size);
  printf("    frame_t *next=0x%0*" PRIxPTR "\n", PRIxPTR_WIDTH, (uintptr_t)frame->next);
  printf("    request_t *req_start=0x%0*" PRIxPTR "\n", PRIxPTR_WIDTH, (uintptr_t)frame->req_start);
  printf("    int req_start_index=%d\n", frame->req_start_index);
  printf("    int req_end_index=%d\n", frame->req_end_index);
  printf("    int active=%d\n", frame->active);
  printf("    struct timespec ts_send {\n");
  printf("      time_t tv_sec=%d\n", (int)frame->ts_send.tv_sec);
  printf("      long tv_nsec=%ld\n", frame->ts_send.tv_nsec);
  printf("    }\n");
  printf("  }\n");
  printf("\n");
}