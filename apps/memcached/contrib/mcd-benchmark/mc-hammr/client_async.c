#include "client_async.h"
#include "connection.h"
#include "client.h"

int async_send(thread_t * thread) {
  int i;
  int wbytes;
  int ready;
  struct epoll_event *wr_events;
  int loop=thread->cfg->loop;
  int status;
  int conn_done=0;
  int done=0;
  long int passes=0;
  long int conns_ready=0;
  int req_idx;
  struct timespec delay; 
  struct epoll_event event;
  connection_t * conn;
  struct timespec now;
  wr_events=malloc(sizeof(struct epoll_event)*thread->cfg->conns_per_thread);
  while(!done) {
    if(thread->cfg->delay) {
      delay.tv_sec=0;
      delay.tv_nsec=thread->cfg->delay*1000;
      if(thread->cfg->jitter) {
	delay.tv_nsec+=(rand()%thread->cfg->jitter)*1000;
      }
      status=nanosleep(&delay, NULL);
      if(status) {
	//printf("sleep interrrupted....\n");
	if(thread->do_exit) {
	  done=1;
	  continue;
	}
      }
    }
    ready=epoll_wait(thread->efd_write, wr_events, thread->cfg->conns_per_thread, 5000);
    conns_ready+=ready;
    passes++;
    if(ready<0) {
      if(errno != EINTR) {
	perror("connection write -- epoll_wait()");
	thread->do_exit=1;
	break;
      }
    } else if(ready==0) {
      printf("No sockets became ready for write within 5 seconds\n");
      thread->do_exit=1;
      break;
    } else {
      for(i=0;i<ready;i++) {
	conn=(connection_t *)wr_events[i].data.ptr;
	//short circuit send limits for UDP data.  We just drop the responses on the
	//floor
	if(conn->bytes_to_send==0) {
	  if(thread->cfg->sock_type!=SOCK_DGRAM) {
	    if(sem_trywait(&conn->frame_count)==-1) {
	      conn->send_state=SEND_BLOCK;
	      continue;
	    }
	  }
	  for(req_idx=conn->frame_send->req_start_index; req_idx<=conn->frame_send->req_end_index; req_idx++) {
	    conn->req_buffer[req_idx].active=1;
	  }
	  //start and end indexs are inclusive.  We allow between 1 and 64 requests (read keys)
	  //per frame.
	  conn->frame_send->active=conn->frame_send->req_start_index;
	  conn->send_ptr=conn->frame_send->ptr;
	  conn->bytes_to_send=conn->frame_send->size;
	  clock_gettime(CLOCK_MONOTONIC, &(conn->frame_send->ts_send));
	}	
	wbytes=send(conn->fd,conn->send_ptr, conn->bytes_to_send, 0);  
	if(wbytes==-1) {
	  conn->send_state=SEND_FAIL;
	  continue;
	}
	conn->send_ptr+=wbytes;
	conn->bytes_to_send-=wbytes;
	if(conn->bytes_to_send==0) {
	  conn->send_state=SEND_COMPLETE;
	  conn->request_count++;
	  conn->life_left--;
	  conn->conn_stats->request_count+=thread->cfg->ops_per_frame;
	  conn->frame_send=conn->frame_send->next;
	  if(conn->frame_send->index==0) {
	    if(conn->loop) {
	      conn->loop--;
	      if(!conn->loop) {
		conn->write_done=1;
	      }
	    }
	  }	    
	  //Check to see if we have a specific number of operations allowed on this connection and
	  //if so, have we exceeded that number.  If this is the case, this connection is done
	  //and can be flagged for shutdown.
	  if(thread->cfg->frames_per_conn && conn->life_left<=0) {
	    conn->write_done=1;
	    conn->send_state=SEND_EXIT_DONE;
	  }	
	  
	  //If this connection has been flagged for shutdown, we will remove it from the list of
	  //active file descriptors and note that we shutdown another connection.
	  if(conn->write_done) {
	    status=epoll_ctl(thread->efd_write, EPOLL_CTL_DEL, conn->fd, &event);
	    if (status == -1) {
	      perror ("epoll_ctl -- write_done");
	    }
	    conn_done++;
	  }	  
	  //If we have shutdown all the connections then we are done sending requests and can exit.
	  //if we are exiting because we have looped the required number of times, then not only
	  //do we exit the send, but we also need to shutdown the whole thread as we won't be coming
	  //back.
	  if(conn_done>=thread->cfg->conns_per_thread) {
	    done=1;
	    if(thread->cfg->loop && conn->loop==0) {
	      thread->do_exit=1;
	    }
	  } 
	} else {
	  conn->send_state=SEND_PARTIAL;
	}
      }
    }
    if(thread->do_exit) {
      done=1;
    }
  }
  //We get here either because all the connections are done with the number of operations they should do, we have
  //looped through the keys on each connection the specified number of times, or our run timer has expired.  The
  //later two will cause the thread to exit.  The first will cause the connections to be closed and re-opened. 
  //In the exit case, we need to check to see what connections are done (if any) and clean things up.
  if(thread->do_exit) {
    //notify the receive thread that we are shutting down so it can exit once it has received the last of the 
    //responses it expects;
    
    //printf("thread - do_exit=1\n");
    thread->read->do_exit=1;
    for(i=0; i<thread->cfg->conns_per_thread; i++) {
      //printf("conn %d - write_done=%d\n", thread->conns[i].write_done);
      if(!thread->conns[i].write_done) {
	//printf("  conn %d - done\n", i);
	status=epoll_ctl(thread->efd_write, EPOLL_CTL_DEL, thread->conns[i].fd, &event);
	if (status == -1) {
	  perror ("epoll_ctl -- do_exit");
	}	
	thread->conns[i].write_done=1;
      }
    }
  }
  //right before we exit, we want to wake up the read thread, just in case it went to sleep while we
  //were cleaning up connections.  This prevents a spurious extra one second in run time.
  //pthread_sigqueue(&thread->read->id, SIG_RUNTIMEEXP, 0);
  //printf("average connections ready: %f\n", (double)conns_ready/(double)passes);
  free(wr_events);
  //thread->read->do_exit=1;
  //sem_wait(&thread->exit_hold);
  //printf("Sender exiting: %d\n", thread->thread_idx);
  return(thread->do_exit); 
}


int async_recv(thread_t * thread) {
  
  struct epoll_event *rd_events;
  protocol_binary_response_header *resp_header=0;
  int ready;
  int status=0;
  int i,j,n;
  int bytes;
  unsigned char end_of_frame;
  int resp_count;
  int conns_complete=0;
  float wait_time;
  struct epoll_event event;
  connection_t *conn;
  struct timespec now;
  int exit_message=0;
  int idx;
  char *string;
  char *c;
  uint64_t active_mask;
  unsigned long resp_opaque;
  
  rd_events=malloc(sizeof(struct epoll_event)*thread->cfg->conns_per_thread);
  //Loop until we have no connections left active.
  while(conns_complete!=thread->cfg->conns_per_thread) {
    if(!exit_message && thread->do_exit) {
      exit_message=1;
      //printf("reader (%d): time to exit\n", thread->thread_idx);
    }
    //There is a race at shutdown between the sending thread closing down and
    //the receving thread determining whether it needs to wait for more responses. 
    //Because of of this, the following epoll_wait may get executed one extra time
    //and cause the receive thread to run "timeout" ms longer than it should.  At
    //1 sec test length this causes reported throughput to be 1% low.  At 10 seconds,
    //0.1%.  Given the variability of the test throughput under the best conditions
    //it hasn't been worth finding a better solution that won't impact performance.
    ready = epoll_wait (thread->efd_read, rd_events, thread->cfg->conns_per_thread, thread->cfg->max_lat);
    if(ready<0) {
      if(errno != EINTR) {
	perror("connection read -- epoll_wait()");
	status=errno;
	break;
      }
    } else if(ready==0) {
      //No responses have arrived in the last "max_latency" ms.  If there are any
      //requests outstanding, then we either have a very busy server, or we are sending
      //GETQ or MGETQ requests and are getting back a bunch of misses.
      for(i=0;i<thread->cfg->conns_per_thread;i++) {	
	conn=&thread->conns[i];
	timeout_requests(conn);
      }
    } else {
      for(i=0;i<ready;i++) {
	if(!(rd_events[i].events && EPOLLIN)) {
	  if(rd_events[i].events && EPOLLERR) {
	    printf("read: EPOLLERR, thread: %d, conn: %d, fd: %d\n", thread->thread_idx, i, conn->fd);
	  }
	  if(rd_events[i].events && EPOLLHUP) {
	    printf("read: EPOLLHUP, thread: %d, conn: %d, fd: %d\n", thread->thread_idx, i, conn->fd);
	  }
	  continue;
	}
	conn=(connection_t *)rd_events[i].data.ptr;
	if(thread->cfg->sock_type==SOCK_STREAM) {
	  resp_header=(protocol_binary_response_header *)conn->rx_buff;
	} else {
	  resp_header=(protocol_binary_response_header *)(conn->rx_buff+sizeof(udp_frame_header_t));
	}
	bytes=read(conn->fd, conn->rx_buff+conn->resp_buffer_off, conn->bytes_to_recv);
	if(thread->cfg->sock_type==SOCK_DGRAM) {
	  continue;	//short circuit receive path for UDP packets
	}
	if(bytes==-1) {
	  perror("connection read");
	} else 	if(thread->cfg->proto==BINARY) {
	  if(bytes<conn->bytes_to_recv && thread->cfg->sock_type==SOCK_STREAM) {
	    conn->resp_buffer_off+=bytes;
	    conn->bytes_to_recv-=bytes;
	  } else {	
	    switch(conn->read_state) {
	      case HEADER:
		conn->resp_buffer_off+=bytes;
		conn->bytes_to_recv=ntohl(resp_header->response.bodylen);
		if(conn->bytes_to_recv) {
		  conn->read_state=DATA;
		  break;
		} else {
		  //printf("bytes_to_recv==0\n");
		}
	      case DATA:
		clock_gettime(CLOCK_MONOTONIC, &now);
		if(thread->cfg->sock_type==SOCK_DGRAM) {
		  printf("\n===========================\n");
		  printf("---------- frame ----------\n");
		  print_udp_frame_header(conn->rx_buff);
		  printf("---------- resp -----------\n");
		  print_bin_resp_header(resp_header);
		  printf("===========================\n\n");
		}
		resp_opaque=ntohl(resp_header->response.opaque);
		end_of_frame=process_response(conn, resp_opaque, ntohs(resp_header->response.status), &now);
		conn->resp_buffer_off=0;				
		if(thread->cfg->sock_type==SOCK_STREAM) {
		  conn->read_state=HEADER;	
		  conn->bytes_to_recv=sizeof(protocol_binary_response_header);
		} else {
		  conn->read_state=DATA;
		  conn->bytes_to_recv=thread->conns[i].rx_buff_size;
		}
		break;		
	    } 
	  }
	} else {  //proto==ASCII
	  clock_gettime(CLOCK_MONOTONIC, &now);
	  conn->resp_buffer_off+=bytes;
	  //start at the beginning of the buffer and we are going to processes the entire
	  //chunk of received data.
	  idx=0;
	  //This is how many responses we are expecting.  If we received less than this then
	  //have misses to account for.
	  resp_count=thread->cfg->ops_per_frame;
	  while(idx<conn->resp_buffer_off) {
	    switch(conn->resp_state) {
	      case RESP_START:
		if(conn->rx_buff[idx]=='V') {		//We have a value returned
		  idx+=6;   //skip over 'VALUE ' and point at the beginning of the key
		  conn->resp_state=RESP_KEY_PREFIX;
		  break;
		} else if(conn->rx_buff[idx+1]=='T') {
		  idx+=8;  //skip over 'STORED\r\n' -- can't check that the first charater
		  //is 'S' since that would also match "SERVER ERROR"
		  //we don't bother to jump to the END state as the response to a SET is
		  //self contained.  We just process the response here.  Since we can't know
		  //exactly what SET request this response belongs to, we assume it to be
		  //the currently expected on.  Any dropped responses, in the case of UDP,
		  //will be rolled up at the end of the frame.  This will make their reported
		  //latency much higher though.
		  end_of_frame=process_response(conn, conn->frame_recv->active, 0, &now);
		  break;
		}
		//Anything else is an ERROR at this point
		conn->resp_state=RESP_ERROR;
		break;
	      case RESP_KEY_PREFIX:
		idx+=strlen(thread->cfg->key_prefix);  //skip to the key prefix
		conn->resp_state=RESP_KEY;
		break;
	      case RESP_KEY:
		//Find the end of the key and turn it into a proper NULL terminate string
		c=strchr(&conn->rx_buff[idx], ' ');
		//If we didn't find the space seperator the lets assume that we haven't received
		//it yet and loop back through
		if(!c) {
		  break;
		}
		*c=0;
		//In binary mode we send the key in the opaque field so it comes back to
		//us.  In this case we explicitly get the key as a field in the response.
		//resp_opaque=atoi(&conn->rx_buff[idx]);
		resp_opaque=strtol(&conn->rx_buff[idx], 0, 16);
		idx+=strlen(&conn->rx_buff[idx])+1;
		conn->resp_state=RESP_FLAGS;
		break;
	      case RESP_FLAGS:
		//find the end of the flags field;
		c=strchr(&conn->rx_buff[idx], ' ');
		//If we didn't find the space seperator the lets assume that we haven't received
		//it yet and loop back through
		if(!c) {
		  break;
		}		  
		*c=0;
		//and skip over it
		idx+=strlen(&conn->rx_buff[idx])+1;
		conn->resp_state=RESP_DATA_LEN;
		break;
	      case RESP_DATA_LEN:		  
		//now find the end of the 'bytes' field
		c=strchr(&conn->rx_buff[idx], '\r');
		//If we didn't find the space seperator the lets assume that we haven't received
		//it yet and loop back through
		if(!c) {
		  break;
		}		  		  
		//and turn it into a proper string, then an integer
		*c=0;
		n=atoi(&conn->rx_buff[idx]);
		idx+=strlen(&conn->rx_buff[idx])+2;
		conn->resp_state=RESP_DATA;
		break;
	      case RESP_DATA:
		//now skip the data and the trailing \r\n
		idx+=n+2;
		conn->resp_state=RESP_PROCESS_RESP;
		break;
	      case RESP_PROCESS_RESP:
		end_of_frame=process_response(conn, resp_opaque, 0, &now);		  
		conn->resp_state=RESP_END;
		break;
	      case RESP_END:
		switch(conn->rx_buff[idx]) {
		  case 'V':
		    conn->resp_state=RESP_START;
		    break;
		  case 'E':
		    conn->resp_state=RESP_START;
		    idx+=5;
		    break;
		  default:
		    conn->resp_state=RESP_ERROR;
		    break;
		}
		break;
	      case RESP_ERROR:
		c=strchr(&conn->rx_buff[idx], '\r');
		if(c) {
		  idx+=(c-&conn->rx_buff[idx])+2;
		} else {
		  idx++;
		}
		//We got an ERROR of some sort.  At the very least, the current response we are
		//waiting for is considered a miss.  It is possible that the error is for a later
		//request and the one we are waiting for dropped, in which case we will figure that 
		//somewhere down the road when we get a hit.
		end_of_frame=process_response(conn, conn->frame_recv->active, 1, &now);
		conn->resp_state=RESP_START;
		break;
	      default:
		break;
	    }

	  }
	    conn->resp_buffer_off=0;
	    conn->bytes_to_recv=conn->rx_buff_size;
	}
      }
    }
    for(i=0; i<thread->cfg->conns_per_thread; i++) {
      conn=&thread->conns[i];
      if(conn->write_done && !conn->read_done) {
	if(sem_getvalue(&conn->frame_count, &resp_count)==-1) {
	  perror("sem_getvalue");
	}
	//printf("thread %d: conns done: %d, conn: %d, fd: %d, conn_state: %d, send_count: %d, recv_count: %d, bytes_to_send: %d, bytes_to_recv: %d, resp_count: %d, max_out: %d, resp_div: %d, seq: %d\n", thread->thread_idx, conns_complete, i, conn->fd, conn->read_state, conn->request_count, conn->receive_count, conn->bytes_to_send, conn->bytes_to_recv, resp_count, thread->cfg->max_outstanding, conn->resp_div, ntohl(((protocol_binary_request_header *)conn->out_transactions[conn->out_tail].start)->request.opaque));
	if(conn->request_count==conn->receive_count) {
	  //printf("shutdown (main loop): %d\n", conn->fd);
	  status=epoll_ctl(thread->efd_read, EPOLL_CTL_DEL, conn->fd, &event);
	  if (status == -1) {
	    perror ("shutdown (main loop) epoll_ctl");
	  }
	  sem_post(&conn->hold_close);
	  conn->read_done=1;
	  conns_complete++;
	}
      }
    }
  }
  free(rd_events);
  return(thread->do_exit);
  
}

