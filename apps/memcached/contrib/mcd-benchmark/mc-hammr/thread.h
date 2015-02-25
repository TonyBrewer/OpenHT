#ifndef __THREAD__
#define __THREAD__

#include <pthread.h>
#include "types.h"
#include "config.h"

#define SIG_RUNTIMEEXP SIGRTMIN
#define SIG_DELAYEXP SIGRTMIN+1

struct thread {
  pthread_t id;
  int thread_idx;
  int proc_idx;
  connection_t  *conns;
  config_t *cfg;
  
  thread_mode_t mode;
  
  //status information for epoll()
  int efd_write;			//epoll file descriptor for write events
  int efd_read;                        //epoll file descriptor for read events
  
  int do_exit;
  struct thread *read;
  struct thread *write;
  
  timer_t run_timer;
  thread_stats_t * thread_stats;
  
};

void *client_thread(void *);
void run_timeout(int sig, siginfo_t *si, void *uc);

#endif