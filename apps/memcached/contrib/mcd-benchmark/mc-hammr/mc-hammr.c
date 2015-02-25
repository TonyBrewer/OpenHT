#include "mc-hammr.h"
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
#include "thread.h"
#include "util.h"
#include "client.h"


int main(int argc, char *argv[]) {
  config_t **config=NULL;
  int i=0;
  int cfg_count=0;
  int pid;
  int child_count=0;
  int *pid_list=NULL;
  int status;
  process_stats_t * stats;

  int conn_count=0;
  int thread_count=0;
  
  cfg_count=load_config(&config, &thread_count, &conn_count, argv[1]);
  stats=init_stats(config, cfg_count, thread_count, conn_count);

  //It is possible that one or more config lines specified that we will fork. Rather than try and
  //figure out how many, we will just assume that every one wants to fork and allocate pid storage
  //big enough to hold all of them.  To ensure we can figure out which actually hold valid pids
  //later, we initialize the whole lot to 0.
  pid_list=malloc(sizeof(int)*cfg_count);
  memset(pid_list,0,sizeof(int)*cfg_count);
  for(i=0; i<cfg_count; i++) {
    config[i]->proc_idx=i;
    config[i]->stats=&stats[i];
    if(config[i]->do_fork) {
      pid=fork();
      if(pid) {
	//We are the parent.  Store the child pid in our list and keep track of how many children
	//we have for use later when we have to wait on them.
	pid_list[child_count]=pid;
	child_count++;
      } else {
	//A child cannot have further children.  We could go ahead and free up the memory for the
	//pid list here, but since the code already exists later and there isn't any great harm
	//in keeping in around, we'll wait.
	child_count=0;
	//we are the child and can start client setup
	start_client(config[i]);
	//We don't want to continue with client stetup since we forked.  Each fork only handles its
	//individual client.
	exit(0);
      }
    } else {
      start_client(config[i]);
    }
  }
  
  while(child_count) {
    printf("waiting on %d children\n",child_count);
    pid=wait(&status);
    for(i=0; i<cfg_count; i++) {
      if(pid_list[i]==pid) {
	printf("  reaped child: %d\n",pid);
	pid_list[i]=0;
	child_count--;
      }
    }    
  }
  free(pid_list);
  generate_stats(config, stats, cfg_count);
  cleanup_stats(stats);
  //Clean up all the memory well still have allocated.
  for(i=0; i<cfg_count; i++) {
    free(config[i]->key_prefix);
    free(config[i]);
  }
  free(config);
  return(0);
}



