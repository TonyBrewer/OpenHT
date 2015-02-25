#include "connection.h"
#include "client.h"
#include "config.h"
#include "thread.h"

void init_connection_data(config_t *cfg, connection_t *conn, thread_mode_t mode) {
  long int tx_buff_size=0;
  long int rx_buff_size=0;
  udp_frame_header_t *frame_header;
  int op_count=cfg->frame_count*cfg->ops_per_frame;
  int req_count;
  int frame_idx;
  int op_idx;
  frame_t *frame;
  request_t *request;
  unsigned char * tx_buff;
  //send stuff
  conn->request_size=calc_request_size(cfg);
  if(cfg->sock_type==SOCK_DGRAM) {
      tx_buff_size+=sizeof(udp_frame_header_t)*cfg->frame_count;
  }
  tx_buff_size+=op_count*conn->request_size;
  
  //The +1 here is to allow for the last request to be ASCII and have a NULL
  //terminator at the end.  All the prior ones will have been overwritten.
  //Without the +1, we would likely SEGV when we wrote the last request
  conn->tx_buffer=malloc(tx_buff_size+1);
  if(conn->tx_buffer==NULL) {
    error_at_line(0,0,__FILE__,__LINE__, "Insufficient memory to allocate tx_buffer (%ld)", (tx_buff_size+1));
  }
  conn->req_buffer=malloc(op_count*sizeof(request_t));
  if(conn->req_buffer==NULL) {
    error_at_line(0,0,__FILE__,__LINE__, "Insufficient memory to allocate request_buffer (%ld)", (op_count*sizeof(request_t)));
  }
  conn->frame_buffer=malloc(cfg->frame_count*sizeof(frame_t));
  if(conn->frame_buffer==NULL) {
    error_at_line(0,0,__FILE__,__LINE__, "Insufficient memory to allocate frane_buffer (%ld)", (cfg->frame_count*sizeof(frame_t)));
  }
  conn->frame_send=conn->frame_buffer;
  conn->frame_recv=conn->frame_send;
  tx_buff=conn->tx_buffer;
  req_count=0;
  for(frame_idx=0; frame_idx<cfg->frame_count; frame_idx++) {
    frame=&conn->frame_buffer[frame_idx];
    frame->index=frame_idx;
    frame->ptr=tx_buff;
    if(cfg->proto==ASCII) {
      frame->size=conn->request_size;
    } else {
      frame->size=cfg->ops_per_frame*conn->request_size;
    }
    frame->next=&conn->frame_buffer[frame_idx+1];
    frame->req_start_index=req_count;
    frame->req_end_index=req_count+cfg->ops_per_frame-1;
    frame->ptr=tx_buff;
    frame->req_start=&conn->req_buffer[req_count];
    if(cfg->sock_type==SOCK_DGRAM) {
      frame_header=(udp_frame_header_t *)tx_buff;
      memset((unsigned char *)frame_header, 0, sizeof(udp_frame_header_t));
      frame_header->req_id=htons(frame_idx);
      frame_header->seq=0;
      frame_header->count=htons(1);
      tx_buff+=sizeof(udp_frame_header_t);
      frame->size+=sizeof(udp_frame_header_t);
    }    
    frame->active=-1;
    if(cfg->proto==ASCII) {
      build_request(cfg, tx_buff, req_count);
      
      for(op_idx=0; op_idx<cfg->ops_per_frame; op_idx++) {
	request=&conn->req_buffer[req_count];
	request->index=op_idx;
	request->active=-1;
	request->ptr=tx_buff;
	request->frame=frame;
	req_count++;
	request->next=&conn->req_buffer[req_count];
      }
      tx_buff+=conn->request_size;
    } else {
      for(op_idx=0; op_idx<cfg->ops_per_frame; op_idx++) {
	build_request(cfg, tx_buff, req_count);
	request=&conn->req_buffer[req_count];
	request->index=req_count;
	request->active=-1;
	request->ptr=tx_buff;
	request->frame=&conn->frame_buffer[frame_idx];
	req_count++;
	request->next=&conn->req_buffer[req_count];
	tx_buff+=conn->request_size;

      }
    }
  }
  conn->frame_buffer[cfg->frame_count-1].next=conn->frame_buffer;
  conn->req_buffer[req_count-1].next=conn->req_buffer;
  
  conn->loop=cfg->loop;
  
  conn->request_count=0;
  conn->req_buffer_off=0;
  conn->bytes_to_send=0;
  conn->resp_state=RESP_START;
  
  //receive stuff
  conn->response_size=calc_response_size(cfg);
  
  
  conn->rx_buff_size=conn->response_size*((cfg->ops_per_frame*cfg->max_outstanding)+1);
  if(cfg->sock_type==SOCK_DGRAM) {
    conn->rx_buff_size+=sizeof(udp_frame_header_t)*cfg->max_outstanding;
  }
  conn->rx_buff=malloc(conn->rx_buff_size);
  if(conn->req_buffer==NULL) {
    error_at_line(0,0,__FILE__,__LINE__, "Insufficient memory to allocate rx_buffer (%ld)", (conn->rx_buff_size));
  }  
  
  
  conn->bytes_to_recv=sizeof(protocol_binary_response_header);
  conn->resp_buffer_off=0;
  conn->read_state=HEADER;
  
  sem_init(&conn->frame_count, 0, cfg->max_outstanding);
  sem_init(&conn->hold_close, 0, 0);
  
  return;
}

int open_conns(thread_t * thread) {
  int i;
  int sock;
  int ret;
  struct sockaddr_in sin;
  int addrlen = sizeof(sin);
#ifdef SOCK_UNIX
  struct sockaddr_un dest_addr;
#else
  struct sockaddr_in dest_addr;
#endif
  struct sockaddr_in src_addr;
  int flags = 1;
  struct epoll_event event;
  for(i=0; i<thread->cfg->conns_per_thread; i++) {
    thread->conns[i].conn_idx=i;
    //create the connections socket and make sure it is bound to the correct source
    //address in case we are trying to use a specific network connection
#ifdef SOCK_UNIX
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
         perror("socket() failed");
         return(0);
    }
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sun_family = AF_UNIX;
    strcpy(dest_addr.sun_path, thread->cfg->ip_addr);
#else
    sock = socket(AF_INET, thread->cfg->sock_type, 0);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&flags, sizeof(flags));
    if(thread->cfg->src_addr[0]!=0) {
      src_addr.sin_family = AF_INET;
      src_addr.sin_family = htons(0);
      src_addr.sin_addr.s_addr=inet_addr(thread->cfg->src_addr);
      bind(sock, (const struct sockaddr *)&src_addr,sizeof(src_addr));
    }
    
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(thread->cfg->port_num);
    dest_addr.sin_addr.s_addr = inet_addr(thread->cfg->ip_addr);
      
    memset(&(dest_addr.sin_zero), '\0', 8);
      
    if(thread->cfg->sock_type==SOCK_STREAM) {
      setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));
    }
    
#endif
    if ( (flags = fcntl(sock, F_GETFL, 0)) < 0 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
      perror("socket fcntl:");
      close(sock);
      return 0;
    }
    if (connect(sock, (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) == -1) {
      if (errno != EINPROGRESS) {
	close(sock);
	perror("socket connect:");
	return 0;
      }
    }



    thread->conns[i].fd = sock;      
    
    //if(getsockname(sock, (struct sockaddr *)&sin, &addrlen) == 0 && sin.sin_family == AF_INET &&addrlen == sizeof(sin)) {
    //  printf("thread %d, conn %d, port %d\n", thread->thread_idx, i, ntohs(sin.sin_port));
    //}
    
    //store the connection that we want returned when we get an event
    //Since the read and write threads share the connection pool, we don't 
    //need to change the pointer between the read event and write event
    //additions.
    event.data.ptr=&thread->conns[i];
    event.events = EPOLLOUT|EPOLLERR;
    ret = epoll_ctl (thread->write->efd_write, EPOLL_CTL_ADD, sock, &event);
    if (ret == -1) {
      perror ("epoll_ctl");
    }
    event.events = EPOLLIN|EPOLLERR;
    ret = epoll_ctl (thread->read->efd_read, EPOLL_CTL_ADD, sock, &event);
    if (ret == -1) {
      perror ("epoll_ctl");
    }

    //If we are opening a re-opening a connection that has already been used, we need
    //to make sure the shutdown did not come in the middle of a transaction send.  If
    //it did then we need to move forward to the beginning of the next transaction in
    //line to ensure we send valid data at the start.
    if(thread->conns[i].bytes_to_send!=0) {
      thread->conns[i].req_buffer_off+=thread->conns[i].bytes_to_send;
      thread->conns[i].bytes_to_send=0;
    }
    thread->conns[i].resp_buffer_off=0;
    if(thread->cfg->sock_type==SOCK_DGRAM) {
      thread->conns[i].bytes_to_recv+=thread->conns[i].rx_buff_size;
        thread->conns[i].read_state=DATA;
    } else {
      if(thread->cfg->proto==BINARY) {
	thread->conns[i].bytes_to_recv=sizeof(protocol_binary_response_header);
	  thread->conns[i].read_state=HEADER;
      } else {
	thread->conns[i].bytes_to_recv=thread->conns[i].rx_buff_size;
      }	
    }

    thread->conns[i].read_done=0;
    thread->conns[i].write_done=0;
    thread->conns[i].life_left=thread->cfg->frames_per_conn;
    thread->conns[i].thread_idx=thread->thread_idx;
    thread->conns[i].send_state=SEND_INIT;
  }
}

int close_conns(thread_t * thread) {
  int i;
  int hold_close;
  struct epoll_event event;
  for(i=0; i<thread->cfg->conns_per_thread; i++) {
    //if(sem_getvalue(&thread->conns[i].hold_close, &hold_close)==-1) {
    //  perror("cons_close - sem_getvalue");
    //}
    //printf("thread %d, conn: %d, closing fd: %d, hold_close: %d\n", thread->thread_idx, i, thread->conns[i].fd, hold_close);
    while(sem_wait(&thread->conns[i].hold_close)!=0) {
      if(errno!=EINTR) {
	perror("close_conns - sem_wait");
	exit(1);
      }
    }
    close(thread->conns[i].fd);
  }
}
