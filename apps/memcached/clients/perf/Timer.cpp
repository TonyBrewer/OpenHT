/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <stdio.h>
#include <pthread.h>
#include "Timer.h"

double CTimerBase::m_cpuClksPerUsec;
size_t CTimerBase::m_maxNameLen = 0;

CTimerBase::CTimerBase(int seconds)
{
    m_seconds = seconds;
    m_pTimerHead = 0;
    m_timerLock = 0;

#ifdef MCD_TIMER
    static bool bInitialized = false;
    if (!bInitialized) {
        bInitialized = true;

        // determine number of cpu clocks per second
	    uint64_t cycles = CTimer::Rdtsc();
	    uint64_t usec = CTimer::GetTimeInUsec();
	    while (CTimer::GetTimeInUsec() - usec < 250000);	// wait 250 msec
	    cycles = CTimer::Rdtsc() - cycles;

        m_cpuClksPerUsec = cycles * 4 / 1000000.0;
    }

    pthread_t intervalThread;
	pthread_create(&intervalThread, 0, StartIntervalThread, this);
#endif
}

void CTimerBase::IntervalThread()
{
    for (;;) {
        sleep(m_seconds);

        Report();
    }
}

void CTimerBase::Report()
{
    printf("\nTimer Report\n");
    Lock();
    for (CTimer * pTimer = m_pTimerHead; pTimer; pTimer = pTimer->m_pNext) {
        pTimer->Report();
    }
    Unlock();
}

void CTimer::Report()
{
    CTimerState state;
    state.Clear();

    for (int i = 0; i < m_threadCnt; i += 1) {
        // sum each threads state
        state += m_pState[i];
        m_pState[i].Clear();
    }

    long long sampleCnt = (state.m_sampleCnt > 0) ? state.m_sampleCnt : 1;
    long long minTime = (state.m_sampleCnt > 0) ? state.m_minTime : 0;
    printf("  %-*s: min=%5.3f usec, avg=%7.3f, max=%8.3f (%lld samples)\n",
        (int)CTimerBase::m_maxNameLen, m_name.c_str(),
        minTime / CTimerBase::m_cpuClksPerUsec,
        state.m_avgTime / CTimerBase::m_cpuClksPerUsec / sampleCnt,
        state.m_maxTime / CTimerBase::m_cpuClksPerUsec,
        state.m_sampleCnt);
}