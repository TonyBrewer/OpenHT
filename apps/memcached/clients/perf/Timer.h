/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

#include <string>
using namespace std;

#if defined(_WIN32)
#include <intrin.h>
#endif

// CPU cycle timer
//
// Used to time regions of code with CPU clock accuracy

class CTimerBase;

class CTimer {
    friend class CTimerBase;
public:
    void Start(int tid=0) {
#ifdef MCD_TIMER
        m_pState[tid].m_startTime = Rdtsc();

        __sync_synchronize();   // memory barrier
#endif
    }

    long long Stop(int tid=0) {
#ifdef MCD_TIMER
        __sync_synchronize();

        if (m_pState[tid].m_startTime == 0) return 0;
        long long delta = Rdtsc() - m_pState[tid].m_startTime;

        if (m_pState[tid].m_minTime > delta) m_pState[tid].m_minTime = delta;
        m_pState[tid].m_avgTime += delta;
        if (m_pState[tid].m_maxTime < delta) m_pState[tid].m_maxTime = delta;

        m_pState[tid].m_sampleCnt += 1;
        m_pState[tid].m_startTime = 0;

        return delta;
#else
        return 0;
#endif
    }

private:
    CTimer(const char * pName, int threadCnt) {
        m_name = pName;
        m_threadCnt = threadCnt;
        posix_memalign((void **)&m_pState, 64, sizeof(CTimerState) * threadCnt);
        for (int i = 0; i < threadCnt; i += 1)
            m_pState[i].Clear();
    }
    void Report();

    static long long Rdtsc() {
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

    static uint64_t GetTimeInUsec() {
	    struct timeval st;
	    gettimeofday(&st, NULL);
	    return st.tv_sec * 1000000ULL + (uint64_t)st.tv_usec;
    }

private:
    // separate state per thread to avoid locks
    union CTimerState {
        struct {
            long long   m_startTime;
            long long   m_minTime;
            long long   m_avgTime;
            long long   m_maxTime;
            long long   m_sampleCnt;
        };
        uint64_t    m_pad[8];   // pad to a cache line to avoid cache thrashing

        void Clear() {
            m_minTime = 0x7fffffffffffffffLL;
            m_avgTime = 0;
            m_maxTime = 0;
            m_sampleCnt = 0;
        }

        void operator += (CTimerState &s) {
            if (m_minTime > s.m_minTime) m_minTime = s.m_minTime;
            if (m_maxTime < s.m_maxTime) m_maxTime = s.m_maxTime;
            m_avgTime += s.m_avgTime;
            m_sampleCnt += s.m_sampleCnt;
        }
    };

private:
    string          m_name;
    CTimerState *   m_pState;
    int             m_threadCnt;
    CTimer *        m_pNext;
};

// CTimerBase
//   Has a linked list to each timer instance
//   Uses pthreads to sleep between reporting interval
//
class CTimerBase {
    friend class CTimer;
public:
    CTimerBase(int seconds);

    CTimer * NewTimer(const char * pName, int threadCnt=1) {
        Lock();

        CTimer * pTimer = new CTimer(pName, threadCnt);
        pTimer->m_pNext = 0;
        if (m_pTimerHead == 0)
            m_pTimerHead = pTimer;
        else
            m_pTimerTail->m_pNext = pTimer;
        m_pTimerTail = pTimer;

        size_t len = strlen(pName);
        if (len > m_maxNameLen)
            m_maxNameLen = len;

        Unlock();

        return pTimer;
    }

private:
    void Lock() {
        while (__sync_fetch_and_or(&m_timerLock, 1) != 0);
    }
    void Unlock() { m_timerLock = 0; }

    void Report();

    void IntervalThread();
    static void *StartIntervalThread(void *pThis)
    {
	    ((CTimerBase*)pThis)->IntervalThread();
	    return 0;
    }

private:
    static double    m_cpuClksPerUsec;
    static size_t    m_maxNameLen;

private:
    int                 m_seconds;    // reporting interval
    CTimer *            m_pTimerHead;
    CTimer *            m_pTimerTail;
    volatile int64_t    m_timerLock;
};
