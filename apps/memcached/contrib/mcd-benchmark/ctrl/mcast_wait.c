#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define CMD_PORT 12345
#define CTL_PORT 12346
#define CTL_GROUP "225.0.0.37"

#define MSGBUFSIZE 1024

main(int argc, char *argv[])
{
  struct sockaddr_in addr;
  int ctl_fd;
  int cmd_fd;
  int nbytes;
  int addrlen;
  int done=0;
  struct ip_mreq mreq;
  char msgbuf[MSGBUFSIZE];
  char cmd[MSGBUFSIZE];
  char **args;
  int arg_count=0;
  char dir[MSGBUFSIZE];
  pid_t child;
  int i;
  
  u_int opt=1;
  
  
  printf("opening control socket... ");     
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
  
  printf("cmd='%s'\n", argv[1]);
  printf("args:'\n");
  for(i=2; i<argc; i++) {
    printf("  %d: %s\n", i, argv[i-1]);
  }
  printf("Waiting on sync... ");
  fflush(stdout);
  while(1) {
    if ((nbytes=recvfrom(ctl_fd,msgbuf,MSGBUFSIZE,0, (struct sockaddr *) &addr,&addrlen)) < 0) {
      perror("recvfrom");
      exit(1);
    }
    if(strcmp(msgbuf, "go")==0) {
      printf("go\n");
      fflush(stdout);
      break;
    } if(strcmp(msgbuf, "exit")==0) {
      printf("exiting without execution...\n");
      close(ctl_fd);
      exit(0);
    } else {
      printf("invalid sync (%s), ignoring.\n", msgbuf);
      printf("Waiting on sync... ");
      fflush(stdout);
    }
  }
  child=fork();
  if(child) {
    if(child==-1) {
      perror("fork");
      exit(1);
    } else {
      waitpid(child);
    }
  } else {
    close(ctl_fd);
    execv(argv[1], &argv[1]);
    perror("execv");
    exit(1);
  }
  close(ctl_fd);
}
