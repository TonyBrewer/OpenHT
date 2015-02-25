#ifndef __UTIL__
#define __UTIL__

#include "client.h"
#include "types.h"
#include "protocol_binary.h"

#define CMD_PORT 12345
#define CTL_PORT 12346
#define CTL_GROUP "225.0.0.37"
#define MSGBUFSIZE 1024

long long int tv_diff_nsec(struct timespec *then, struct timespec *now);
void mcast_wait(void);
inline void print_udp_frame_header(udp_frame_header_t *frame);
inline void print_bin_resp_header(protocol_binary_response_header *resp);
#endif
