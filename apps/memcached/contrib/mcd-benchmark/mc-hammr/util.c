#include <netinet/in.h>
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"

long long int tv_diff_nsec(struct timespec *then, struct timespec *now) {
  struct timespec diff;
  if ((now->tv_nsec-then->tv_nsec)<0) {
    diff.tv_sec = now->tv_sec-then->tv_sec-1;
    diff.tv_nsec = 1000000000+now->tv_nsec-then->tv_nsec;
  } else {
    diff.tv_sec = now->tv_sec-then->tv_sec;
    diff.tv_nsec = now->tv_nsec-then->tv_nsec;
  } 
  if(diff.tv_sec) {
      return((diff.tv_sec * 1000000000)+diff.tv_nsec);
  }
  return(diff.tv_nsec);
}


void mcast_wait(void) {
  struct sockaddr_in addr;
  int ctl_fd;
  int nbytes;
  int addrlen;
  struct ip_mreq mreq;
  char msgbuf[MSGBUFSIZE];
  char cmd[MSGBUFSIZE];
  char dir[MSGBUFSIZE];
  u_int opt=1;
  
  //printf("opening control socket... ");     
  /* create what looks like an ordinary UDP socket */
  if ((ctl_fd=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
    perror("socket");
    exit(1);
  }
  
  if (setsockopt(ctl_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt)) < 0) {
    perror("Reusing ADDR failed");
    exit(1);
  }
  
  /* set up destination address */
  memset(&addr,0,sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=htonl(INADDR_ANY);
  addr.sin_port=htons(CTL_PORT);
  
  /* bind to receive address */
  if (bind(ctl_fd,(struct sockaddr *) &addr,sizeof(addr)) < 0) {
    perror("bind");
    exit(1);
  }
  
  /* use setsockopt() to request that the kernel join a multicast group */
  mreq.imr_multiaddr.s_addr=inet_addr(CTL_GROUP);
  mreq.imr_interface.s_addr=htonl(INADDR_ANY);
  if (setsockopt(ctl_fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
    perror("setsockopt");
    exit(1);
  }
  printf("bound\n");

  //printf("Waiting on sync... ");
  fflush(stdout);
  while(1) {
    if ((nbytes=recvfrom(ctl_fd,msgbuf,MSGBUFSIZE,0, (struct sockaddr *) &addr,&addrlen)) < 0) {
      perror("recvfrom");
      exit(1);
    }
    if(strncmp(msgbuf, "go",nbytes)==0) {
      //printf("go\n");
      fflush(stdout);
      break;
    }  else {
      printf("invalid sync (%s), ignoring.\n", msgbuf);
      //printf("Waiting on sync... ");
      fflush(stdout);
    }
  }
  close(ctl_fd);
}

inline void print_udp_frame_header(udp_frame_header_t *frame) {
  printf("reqid: %d, seq: %d, n: %d, res: %d\n",
	 ntohs(frame->req_id), ntohs(frame->seq), 
	 ntohs(frame->count), ntohs(frame->reserved));
}

inline void print_bin_resp_header(protocol_binary_response_header *resp) {
  printf("magic: %d, op: %d, keylen: %d\nextlen: %d, data type: %d, status: %d\nbodylen: %d, opaque: 0x%x, cas: %d\n",
	 resp->response.magic, resp->response.opcode, ntohs(resp->response.keylen), resp->response.extlen, 
	 resp->response.datatype, ntohs(resp->response.status),
	 ntohl(resp->response.bodylen), ntohl(resp->response.opaque), ntohl(resp->response.cas));
}