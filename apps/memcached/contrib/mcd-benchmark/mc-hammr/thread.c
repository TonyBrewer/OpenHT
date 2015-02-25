#include "thread.h"
#include <signal.h>

void *client_thread(void *arg) {
  int status=0;
  int done=0;
  thread_t *thread=(thread_t *)arg;
  struct sigevent sig_event;
  struct itimerspec timeout;

  //printf("Started thread %d\n", (int)thread->id);
  clock_gettime(CLOCK_MONOTONIC, &thread->thread_stats->start);
  //if(thread->cfg->time && thread->mode!=THREAD_ASYNC_RECV) {
  if(thread->cfg->time) {
    memset(&sig_event, 0, sizeof(struct sigevent));
    sig_event.sigev_notify=SIGEV_SIGNAL;
    sig_event.sigev_signo=SIG_RUNTIMEEXP;
    //sig_event.sigev_notify_thread_id=gettid();
    sig_event.sigev_value.sival_ptr=thread;
  
    if(timer_create(CLOCK_MONOTONIC, &sig_event, &(thread->run_timer))) {
      perror("timer_create");
      thread->read->do_exit=1;
      status=1;
      return((void *)(long int)status);
    }
    timeout.it_interval.tv_sec = 0;
    timeout.it_interval.tv_nsec = 0;
    timeout.it_value.tv_sec = thread->cfg->time;
    timeout.it_value.tv_nsec = 0;

    if (timer_settime (thread->run_timer, 0, &timeout, 0)) {
      perror("timer_settime");
      thread->read->do_exit=1;
      status=1;
      return((void *)(long int)status);
    }
  }

  while(!done) {
    switch(thread->mode) {
      case THREAD_SYNC:
	break;
      case THREAD_ASYNC_SEND:
	open_conns(thread);
	done=async_send(thread);
	close_conns(thread);
	break;
      case THREAD_ASYNC_RECV:
	done=async_recv(thread);
	break;
      default:
	error_at_line(0,0,__FILE__,__LINE__, "Unknown thread mode: %d\n", thread->mode);
	status=2;
	break;
    }
  }
  //if(thread->cfg->time && thread->mode!=THREAD_ASYNC_RECV) {
  if(thread->cfg->time) {    
    timer_delete(thread->run_timer);
  }  

  clock_gettime(CLOCK_MONOTONIC, &thread->thread_stats->end);
  return((void *)(long int)status);
}

void run_timeout(int sig, siginfo_t *si, void *uc) {
  thread_t *thread=si->si_value.sival_ptr;
  if(sig!=SIG_RUNTIMEEXP) {
      return;
  }
  if(thread) {
    thread->do_exit=1;
    //printf("run timer expired\n");
  }
}
