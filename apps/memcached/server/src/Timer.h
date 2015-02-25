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

class CTimerLink {
    friend class CTimerBase;
    friend class CTimer;
    friend class CSample;
public:
    virtual ~CTimerLink() {}
    virtual void Report(uint64_t timerInterval) = 0;

private:
    string          m_name;
    CTimerLink *    m_pNext;
};

class CSample : CTimerLink {
    friend class CTimerBase;
public:

    void Sample(int64_t value, int tid=0) {
#ifdef MCD_TIMER
        __sync_synchronize();

        if (m_pState[tid].m_minValue > value) m_pState[tid].m_minValue = value;
        m_pState[tid].m_avgValue += value;
        if (m_pState[tid].m_maxValue < value) m_pState[tid].m_maxValue = value;

        m_pState[tid].m_sampleCnt += 1;
#endif
    }

private:
    CSample(const char * pName, int threadCnt) {
        m_name = pName;
        m_threadCnt = threadCnt;
        posix_memalign((void **)(uint64_t)&m_pState, 64, sizeof(CSampleState) * threadCnt);
        for (int i = 0; i < threadCnt; i += 1)
            m_pState[i].Clear();
    }
    void Report(uint64_t timerInterval);

private:
    // separate state per thread to avoid locks
    union CSampleState {
        struct {
            long long   m_minValue;
            long long   m_avgValue;
            long long   m_maxValue;
            long long   m_sampleCnt;
        };
        uint64_t    m_pad[8];   // pad to a cache line to avoid cache thrashing

        void Clear() {
            m_minValue = 0x7fffffffffffffffLL;
            m_avgValue = 0;
            m_maxValue = 0;
            m_sampleCnt = 0;
        }

        void operator += (CSampleState &s) {
            if (m_minValue > s.m_minValue) m_minValue = s.m_minValue;
            if (m_maxValue < s.m_maxValue) m_maxValue = s.m_maxValue;
            m_avgValue += s.m_avgValue;
            m_sampleCnt += s.m_sampleCnt;
        }
    };

private:
    CSampleState *  m_pState;
    int             m_threadCnt;
};

class CTimer : CTimerLink {
    friend class CTimerBase;
public:
    void Start(int tid=0) {
#ifdef MCD_TIMER
        m_pState[tid].m_startTime = Rdtsc();

        __sync_synchronize();   // memory barrier
#endif
    }

    void Stop(int tid=0) {
#ifdef MCD_TIMER
        __sync_synchronize();

        if (m_pState[tid].m_startTime == 0) return;
        long long delta = Rdtsc() - m_pState[tid].m_startTime;

        if (m_pState[tid].m_minTime > delta) m_pState[tid].m_minTime = delta;
        m_pState[tid].m_avgTime += delta;
        if (m_pState[tid].m_maxTime < delta) m_pState[tid].m_maxTime = delta;

        m_pState[tid].m_sampleCnt += 1;
        m_pState[tid].m_startTime = 0;
#endif
    }

    void Sample(int64_t delta, int tid=0) {
#ifdef MCD_TIMER
        if (m_pState[tid].m_minTime > delta) m_pState[tid].m_minTime = delta;
        m_pState[tid].m_avgTime += delta;
        if (m_pState[tid].m_maxTime < delta) m_pState[tid].m_maxTime = delta;

        m_pState[tid].m_sampleCnt += 1;
        m_pState[tid].m_bOverlapped = true;
#endif
    }

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

private:
    CTimer(const char * pName, int threadCnt) {
        m_name = pName;
        m_threadCnt = threadCnt;
        posix_memalign((void **)(uint64_t)&m_pState, 64, sizeof(CTimerState) * threadCnt);
        for (int i = 0; i < threadCnt; i += 1)
            m_pState[i].Clear();
    }
    void Report(uint64_t timerInterval);

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
            bool        m_bOverlapped;
        };
        uint64_t    m_pad[8];   // pad to a cache line to avoid cache thrashing

        void Clear() {
            m_minTime = 0x7fffffffffffffffLL;
            m_avgTime = 0;
            m_maxTime = 0;
            m_sampleCnt = 0;
            m_bOverlapped = false;
        }

        void operator += (CTimerState &s) {
            if (m_minTime > s.m_minTime) m_minTime = s.m_minTime;
            if (m_maxTime < s.m_maxTime) m_maxTime = s.m_maxTime;
            m_avgTime += s.m_avgTime;
            m_sampleCnt += s.m_sampleCnt;
            m_bOverlapped |= s.m_bOverlapped;
        }
    };

private:
    CTimerState *   m_pState;
    int             m_threadCnt;
};

// CTimerBase
//   Has a linked list to each timer instance
//   Uses pthreads to sleep between reporting interval
//
class CTimerBase {
    friend class CTimer;
    friend class CSample;
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

    CSample * NewSample(const char * pName, int threadCnt=1) {
        Lock();

        CSample * pSample = new CSample(pName, threadCnt);
        pSample->m_pNext = 0;
        if (m_pTimerHead == 0)
            m_pTimerHead = pSample;
        else
            m_pTimerTail->m_pNext = pSample;
        m_pTimerTail = pSample;

        size_t len = strlen(pName);
        if (len > m_maxNameLen)
            m_maxNameLen = len;

        Unlock();

        return pSample;
    }

    static double GetCpuClksPerUsec() { return m_cpuClksPerUsec; }

private:
    void Lock() {
        while (__sync_fetch_and_or(&m_timerLock, 1) != 0);
    }
    void Unlock() { __sync_synchronize(); m_timerLock = 0; }

    void Report(uint64_t timerInterval);

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
    CTimerLink *        m_pTimerHead;
    CTimerLink *        m_pTimerTail;
    volatile int64_t    m_timerLock;
};
