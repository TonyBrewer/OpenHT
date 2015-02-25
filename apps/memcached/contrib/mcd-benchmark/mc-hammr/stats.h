#ifndef __STATS__
#define __STATS__

#include "types.h"
#include "config.h"

#define DV2DBL(dv) (dv.tv_sec+(dv.tv_nsec/1000000000.0))


struct conn_stats_struct {
  long int request_count;
  long int response_count;
  long int hit_count;
  long long int hit_time;
  int hit_min;
  int hit_max;
  int hit_mean;
  int hit_M2;
  long int miss_count;
  long long int miss_time;
  int miss_min;
  int miss_max;
  int miss_mean;
  int miss_M2;
  long int timeout_count;
  long long int timeout_time;
  int timeout_min;
  int timeout_max;
  int timeout_mean;
  int timeout_M2;
  int proc;
  int thread;
  int conn;
};

struct thread_stats_struct {
  struct timespec start;
  struct timespec end;
  conn_stats_t * conn_stats;
};

struct process_stats_struct {
  thread_stats_t * thread_stats;
};


void generate_stats(config_t * *, process_stats_t *, int);
process_stats_t * init_stats(config_t **config, int cfg_count, int thread_count, int conn_count);
void cleanup_stats(process_stats_t * stats);
#endif