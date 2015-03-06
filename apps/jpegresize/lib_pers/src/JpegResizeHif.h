/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include "Ht.h"
#include <semaphore.h>
struct JobInfo;

using namespace Ht;
#define MAX_COPROC_JOBS	16

inline long long Rdtsc() {
    #if defined(_WIN32)
	        return  __rdtsc();
    #else
        int64_t low, high;
        __asm__ volatile ("rdtsc\n"
                            "shl $0x20, %%rdx"
                            : "=a" (low), "=d" (high));
        return high | low;
    #endif
}

extern bool g_IsCheck;

extern long long g_coprocJobUsecs;
extern long long g_coprocJobs;

extern "C" void* StartRecvMsgHandler(void* ptr);

class JpegResizeHif : public CHtHif {
public:
	JpegResizeHif(CHtHifParams * pHifParams);

	~JpegResizeHif();
	void SubmitJob(JobInfo * pJobInfo);
	void ObtainLock();
	void ReleaseLock();
	void RecvMsgHandler(void);	
	void jobStats(uint64_t &timeStamp, long long &jobCount, long long &jobTime);
	void jobStats(double &seconds, long long &jobCnt, double &jobActiveAvg, double &jobTimeAvg);

private:
	CHtAuUnit *m_pUnit;

	volatile int64_t	m_lock;
	queue<uint8_t>		m_jobIdQue;
	pthread_mutex_t		m_jobIdQueLock;
	volatile bool		m_bJobDoneVec[MAX_COPROC_JOBS];
	volatile bool		m_bJobBusyVec[MAX_COPROC_JOBS];
	sem_t			m_jobSem[MAX_COPROC_JOBS];
	pthread_mutex_t		m_coprocLock;
	pthread_t		m_recvMsgHandler;
	bool			m_bRecvMsgHandlerExit;
	long long 		m_coprocJobUsecs;
	long long		m_lastCoprocJobUsecs;
	long long 		m_coprocJobs;
	long long		m_lastCoprocJobs;
	uint64_t		m_lastStatsTime;
	
};

extern JpegResizeHif * g_pJpegResizeHif;
