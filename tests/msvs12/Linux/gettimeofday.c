#include <Windows.h>
#include <time.h>
#include <assert.h>
#include "gettimeofday.h"

#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	FILETIME ft;
	unsigned __int64 tmpres = 0;
	static int tzflag;

	if (NULL != tv) {
		GetSystemTimeAsFileTime(&ft);

		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		/*converting file time to unix epoch*/
		tmpres /= 10; /*convert into microseconds*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS;
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

	if (NULL != tz) {
		if (!tzflag) {
			_tzset();
			tzflag++;
		}
		assert(tz == 0);
		//tz->tz_minuteswest = _timezone / 60;
		//tz->tz_dsttime = _daylight;
	}

	return 0;
}
