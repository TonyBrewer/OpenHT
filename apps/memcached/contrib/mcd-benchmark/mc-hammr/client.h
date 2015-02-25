#ifndef __CLIENT__
#define __CLIENT__
#include "types.h"
#include "config.h"

struct frame_struct {
  int index;
  uint8_t * ptr;
  int size;
  struct frame_struct *next;
  struct request_struct * req_start;
  int req_start_index;
  int req_end_index;
  int active;
  struct timespec ts_send;
};

struct request_struct {
  int index;
  unsigned char active;
  uint8_t * ptr;
  struct frame_struct *frame;
  struct request_struct *next;
};

struct udp_frame_header_struct {
  uint16_t req_id;
  uint16_t seq;
  uint16_t count;
  uint16_t reserved;
};

#define UPDATE_MIN_MAX(n, t) 	    if(t>n##_max) { \
				      n ##_max=t; \
				    } \
				    if(n##_min==0 || t<n##_min) { \
				      n##_min==t; \
				    }

void start_client(config_t *config);
int build_request(config_t *cfg, char *buffer, int key);
int calc_request_size(config_t *cfg);
int calc_response_size(config_t *cfg);
void miss_remaining_frame(connection_t * conn, struct timespec *ts);
void display_frame(frame_t * frame);



#endif