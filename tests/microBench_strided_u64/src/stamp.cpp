#include "Ht.h"

// microsecond timestamp useful for performance analysis
uint64_t delta_usec(uint64_t *usr_stamp) {
	static uint64_t stamp = 0;
	uint64_t prev, curr, delta = 0;

	struct timeval st;
	gettimeofday(&st, NULL);
	curr = st.tv_sec * 1000000ULL + (uint64_t)st.tv_usec;

	prev = usr_stamp == NULL ? stamp : *usr_stamp;

	if (prev >= 0)
		delta = curr - prev;

	if (usr_stamp == NULL)
		stamp = curr;
	else
		*usr_stamp = curr;

	return delta;
}
