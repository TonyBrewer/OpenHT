#ifndef __CLIENT_ASYNC__
#define __CLIENT_ASYNC__
#include "types.h"
#include "thread.h"


int async_send(thread_t * thread);
int async_recv(thread_t * thread);

#endif