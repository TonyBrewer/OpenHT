#include "stats.h"

int stats_shmid;

process_stats_t * init_stats(config_t **config, int cfg_count, int thread_count, int conn_count) {
  int stats_shm_size=0;
  process_stats_t * stats;
  thread_stats_t * thread_stats;
  conn_stats_t * conn_stats;
  int i;
  int j;
  int tmp_thread_count;
  stats_shm_size=sizeof(conn_stats_t)*conn_count+sizeof(thread_stats_t)*thread_count+sizeof(process_stats_t)*cfg_count;
  stats_shmid=shmget(IPC_PRIVATE,stats_shm_size, 0644 | IPC_CREAT);
  if(stats_shmid==-1) {
      perror("shmget");
  }
  stats=shmat(stats_shmid, (void *)0, 0);
  if(stats == (process_stats_t *)(-1)) {
    perror("shmat");
  }
  //printf("stats:  shared memory base - %x, shared memory max - %x\n", stats, ((long int)stats)+stats_shm_size);
  memset(stats, 0, stats_shm_size);
  //We are going to place the thread_stats right after the last "process" stats structure
  //and put the connection stats right after the last thread stats structure.
  //printf("stats:  proc stats start: %x\n", stats);
  thread_stats=(thread_stats_t *)(&stats[cfg_count]);
  //printf("stats:  thread stats start: %x\n", thread_stats);
  conn_stats=(conn_stats_t *)(&thread_stats[thread_count]);
  //printf("stats:  conn starts start: %x\n", conn_stats);
  //rather than try and stuff this in with the rest of the startup, we will just build the stats structure tree
  //here.  We are building up the tree as we walk through all the processes and threads.  Beware: pointer foo
  for(i=0; i<cfg_count; i++) {
    stats[i].thread_stats=thread_stats;
    //printf("stats:  proc %d thread stats: %x\n", i, thread_stats);
    //remember that we are hiding the 2x thread count in the case of ASYNC mode for stats generation
    if(config[i]->recv_mode==SYNC) {
      tmp_thread_count=config[i]->threads;
    } else {
      tmp_thread_count=config[i]->threads/2;
    }
    thread_stats=&thread_stats[tmp_thread_count];
    for(j=0; j<tmp_thread_count; j++) {
      //printf("stats: proc %d, thread %d, stats: %x\n", i, j, &stats[i].thread_stats[j]);
      stats[i].thread_stats[j].conn_stats=conn_stats;
      //printf("stats:  proc %d, thread %d, conn stats: %x\n", i, j, conn_stats);
      //for(k=0; k<config[i]->conns_per_thread; k++) {
	//printf("stats:  proc %d, thread %d, conn %d, stats: %x\n", i, j, k, &stats[i].thread_stats[j].conn_stats[k]);
      //}
      conn_stats=&conn_stats[config[i]->conns_per_thread];
    }
  }
  return(stats);
}

void generate_stats(config_t * *cfg, process_stats_t *stats, int proc_count) {
  int process;
  int thread;
  int conn;
  int tmp_thread_count;
  long int total_req=0;
  long int proc_req=0;
  long int thread_req=0;
  long int total_resp=0;
  long int proc_resp=0;
  long int thread_resp=0;
  long int thread_hit=0;
  long int proc_hit=0;
  long int total_hit=0;
  long int thread_miss=0;
  long int proc_miss=0;
  long int total_miss=0;
  long long int hit_avg;
  long long int miss_avg;
  long long thread_hit_time=0;
  long long thread_miss_time=0;
  long long proc_hit_time=0;
  long long proc_miss_time=0;
  long long total_hit_time=0;
  long long total_miss_time=0;
  double thread_time=0;  
  double proc_time=0;
  double total_time=0;
  double lat=0;
  double rate=0;
  conn_stats_t *conn_stats;
  for(process=0; process<proc_count; process++) {
    proc_resp=0;
    proc_req=0;
    proc_time=0;  
    proc_hit=0;
    proc_miss=0;
    proc_hit_time=0;
    proc_miss_time=0;
    if(cfg[process]->recv_mode==SYNC) {
      tmp_thread_count=cfg[process]->threads;
    } else {
      tmp_thread_count=cfg[process]->threads/2;
    }
    for(thread=0; thread<tmp_thread_count; thread++) {
      thread_req=0;
      thread_resp=0;
      thread_hit_time=0;
      thread_miss_time=0;
      thread_hit=0;
      thread_miss=0;
      for(conn=0; conn<cfg[process]->conns_per_thread; conn++) {
	conn_stats=&stats[process].thread_stats[thread].conn_stats[conn];
	//printf("    conn: %d, ", conn);
	//printf("req: %ld ", conn_stats->request_count);
	thread_req+=conn_stats->request_count;
	//printf("resp: %ld ", conn_stats->response_count);
	thread_resp+=conn_stats->response_count;
	if(conn_stats->hit_count) {
	  hit_avg=conn_stats->hit_time/conn_stats->hit_count;
	  thread_hit_time+=conn_stats->hit_time;
	  thread_hit+=conn_stats->hit_count;
	} else {
	  hit_avg=0;
	}
	//printf("hit: %ld (%.2fus) ", conn_stats->hit_count, hit_avg/1000.0);
	if(conn_stats->miss_count) {
	  miss_avg=conn_stats->miss_time/conn_stats->miss_count;
	  thread_miss_time+=conn_stats->miss_time;
	  thread_miss+=conn_stats->miss_count;
	} else {
	  miss_avg=0;
	}
	//printf("miss: %ld (%.2fus) ", conn_stats->miss_count, miss_avg/1000.0);
	//printf("\n");
      }
      thread_time=DV2DBL(stats[process].thread_stats[thread].end)-DV2DBL(stats[process].thread_stats[thread].start);
      if(thread_hit) {
	hit_avg=thread_hit_time/thread_hit;
	proc_hit+=thread_hit;
	proc_hit_time+=thread_hit_time;
      } else {
	hit_avg=0;
      }
      if(thread_miss) {
	miss_avg=thread_miss_time/thread_miss;
	proc_miss+=thread_miss;
	proc_miss_time+=thread_miss_time;	
      } else {
	miss_avg=0;
      }
      if(thread_time==0) {
	  rate=0;
      } else {
	  rate=thread_req/thread_time;
      }
      if(thread_hit+thread_miss==0) {
	lat=0;
      } else {
	lat=(thread_hit_time+thread_miss_time)/(thread_hit+thread_miss)/1000.0;
      }
      printf("  thread: %d, req: %ld, resp: %ld, run: %.6f, rate: %.2f, hit: %ld (%.2f), miss: %ld (%.2f), lat: %.2f\n", thread, thread_req, thread_resp, thread_time, rate, thread_hit, hit_avg/1000.0, thread_miss, miss_avg/1000.0, lat);
      proc_resp+=thread_resp;
      proc_req+=thread_req;
      if(thread_time>proc_time) {
	proc_time=thread_time;
      }
    }
    if(proc_hit) {
	hit_avg=proc_hit_time/proc_hit;
    } else {
      hit_avg=0;
    }
    if(proc_miss) {
      miss_avg=proc_miss_time/proc_miss;
    } else {
      miss_avg=0;
    }
    if(proc_time==0) {
	rate=0;
    } else {
	rate=proc_req/proc_time;
    }
    if(proc_hit+proc_miss==0) {
	lat=0;
    } else {
	lat=(proc_hit_time+proc_miss_time)/(proc_hit+proc_miss)/1000.0;
    }
    printf("proc: %d, req: %ld, resp: %ld, run: %.06f, rate: %.2f, hit: %ld (%.2f), miss: %ld (%.2f), lat: %.2f\n\n", process, proc_req, proc_resp, proc_time, rate, proc_hit, hit_avg/1000.0, proc_miss, miss_avg/1000.0, lat);
    total_req+=proc_req;
    total_resp+=proc_resp;
    total_hit+=proc_hit;
    total_miss+=proc_miss;
    total_hit_time+=proc_hit_time;
    total_miss_time+=proc_miss_time;
    if(proc_time>total_time) {
	total_time=proc_time;
    }
  }
  if(total_hit) {
    hit_avg=total_hit_time/total_hit;
  } else {
    hit_avg=0;
  }
  if(total_miss) {
    miss_avg=total_miss_time/total_miss;
  } else {
    miss_avg=0;
  }	
  if(total_time==0) {
      rate=0;
  } else {
      rate=total_req/total_time;
  }
  if(total_hit+total_miss==0) {
      lat=0;
  } else {
      lat=(total_hit_time+total_miss_time)/(total_hit+total_miss)/1000.0;
  }
  printf("\ncumulative:  req: %ld, resp: %ld, run: %.06f, rate: %.2f, hit: %ld (%.2f), miss: %ld (%.2f), lat: %.2f\n\n", total_req, total_resp, total_time, rate, total_hit, hit_avg/1000.0, total_miss, miss_avg/1000.0, lat);
}

void cleanup_stats(process_stats_t * stats) {
  shmdt(stats);
  shmctl(stats_shmid, IPC_RMID, NULL);
}