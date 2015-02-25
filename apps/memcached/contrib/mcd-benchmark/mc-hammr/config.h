#ifndef __CONFIG__
#define __CONFIG__

#include <pthread.h>
#include <sys/epoll.h>
#include <signal.h>
#include <semaphore.h>
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
#include <netinet/in.h>
#include "stats.h"

enum enum_req_type {
  GET = 0,
  GETQ,
  MGET,
  MGETQ,
  SET,
  SETQ,
  INV,
};

enum enum_proto {
  BINARY,
  ASCII,
};

enum enum_recv_mode {
  SYNC,
  ASYNC,
};

enum enum_thread_mode {
  THREAD_SYNC,
  THREAD_ASYNC_SEND,
  THREAD_ASYNC_RECV,
};

enum enum_cfg_key {
  SEND = 0,
  RECV,
  CONNS_PER_THREAD,
  THREADS,
  KEY_PREFIX,
  KEY_LEN,
  OP_COUNT,
  VALUE_SIZE,
  HOST,
  SRC,
  PORT,
  RATE,
  DELAY,
  LOOP,
  JITTER,
  OUT,
  FORK,
  TIME,
  TX_RES,
  RND,
  OPS_PER_CONN,
  PROTO,
  MCAST_WAIT,
  UDP,
  MGET_SIZE,
  OPS_PER_FRAME,
  MAX_LAT,
};

struct config_struct {
  //connection info
  char ip_addr[60];
  char src_addr[60];
  int port_num;
  int sock_type;
  req_type_t req_type;
  recv_mode_t recv_mode;
  int value_size;
  int max_lat;
  double delay;			//delay between start of two transactions
  unsigned long jitter;			//variablility in intertransaction delay
  int loop;         			//Control the number of loops through all keys before exiting
  int time;				//Run time for each test
  char *key_prefix;			//string that is prepended to the key number for a transaction
  unsigned char key_len;		
  unsigned int key_count;		//how many transactions do we run
  unsigned long rnd_keys;		//flag to trigger random key generation
  //int out;
  int tx_res;
  int frame_count;
  int ops_per_frame;
  int frames_per_conn;
  int conns_per_thread;
  int threads;
  int max_outstanding;
  int do_fork;
  int tx_watermark;
  proto_t proto;
  process_stats_t * stats;
  
  int mcast_wait;
  
  int proc_idx;
};


config_t * parse_config_line(char *);
int load_config(config_t ***config, int *thread_count, int *conn_count, char *filename);


#endif // __CONFIG__