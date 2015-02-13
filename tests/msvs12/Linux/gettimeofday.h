#include <Windows.h>
#include <time.h>

//struct timezone {
//	int tz_minuteswest;     /* minutes W of Greenwich */
//	int tz_dsttime;         /* type of dst correction */
//};

#ifdef __cplusplus
extern "C" {
#endif

int gettimeofday(struct timeval *tv, struct timezone *tz);

#ifdef __cplusplus
}
#endif
