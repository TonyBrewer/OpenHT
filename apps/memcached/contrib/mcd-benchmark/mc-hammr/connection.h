#ifndef __CONNECTION__
#define __CONNECTION__

#include "config.h"
#include "stats.h"
#include "mc-hammr.h"

enum enum_conn_read_state {
  HEADER,
  DATA,
};

enum enum_conn_resp_state {
  RESP_START,
  RESP_KEY_PREFIX,
  RESP_KEY,
  RESP_FLAGS,
  RESP_DATA_LEN,
  RESP_DATA,
  RESP_PROCESS_RESP,
  RESP_END,
  RESP_ERROR
};

enum enum_conn_send_state {
  SEND_INIT,
  SEND_BLOCK,
  SEND_FAIL,
  SEND_PARTIAL,
  SEND_COMPLETE,
  SEND_SLEEP,
  SEND_EXIT_DONE
};

struct connection_struct {
  int fd;
  char * tx_buffer;
  frame_t * frame_buffer;
  request_t * req_buffer;
  unsigned long req_buffer_off;
  unsigned long req_buffer_len;
  int request_size;
  int bytes_to_send;
  frame_t *frame_send;
  frame_t *frame_recv;
  char *send_ptr;
  long int life_left;
  long int request_count;
  long int receive_count;

  
  char *rx_buff;
  int response_size;
  int bytes_to_recv;
  int resp_buffer_off;
  unsigned int rx_buff_size;
  conn_stats_t *conn_stats;
  int loop;

  int resp_div;
  int thread_idx;
  int proc_idx;
  int conn_idx;
  config_t *cfg;
  
  sem_t frame_count;
  sem_t hold_close;
  int write_done;
  int read_done;
  conn_read_state_t read_state;
  conn_resp_state_t resp_state;
  conn_send_state_t send_state;
  uint16_t req_id;
};

void init_connection_data(config_t *, connection_t  *, thread_mode_t);
int open_conns(thread_t * thread);
int close_conns(thread_t * thread);

#endif
