/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#ifndef _HTV

extern "C" int ht_posix_memalign(void **memptr, size_t alignment, size_t size);
extern "C" void ht_free_memalign(void *ptr);

#ifdef _WIN32

#ifndef _M_AMD64
#define _M_AMD64
#endif

#include <windows.h>
#include <process.h>
#include <time.h>

#ifndef __sync_fetch_and_or
inline int64_t __sync_fetch_and_or(volatile int64_t * a, int64_t v) { return InterlockedOr64(a, v); }
#endif

#ifndef __sync_fetch_and_sub
inline uint32_t __sync_fetch_and_sub(volatile uint32_t * a, uint32_t v) { return InterlockedExchangeSubtract(a, v); }
#endif

#ifndef __sync_fetch_and_add
inline uint32_t __sync_fetch_and_add(volatile uint32_t * a, uint32_t v) { return InterlockedExchangeAdd(a, v); }
#endif

#ifndef __sync_synchronize
#define __sync_synchronize MemoryBarrier
#endif

#define usleep(us) Sleep((us+999)/1000)

#define ALIGN8
#define __attribute__(x)

#else

#include <unistd.h>
#include <sys/time.h>

#define ALIGN8 __attribute__((__aligned__(8)))

#endif

static inline uint64_t ht_usec()
{
#ifdef _WIN32
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000ULL

	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);

	uint64_t usec = ft.dwHighDateTime;
	usec <<= 32;
	usec |= ft.dwLowDateTime;

	/*converting file time to unix epoch*/
	usec /= 10; /*convert into microseconds*/
	usec -= DELTA_EPOCH_IN_MICROSECS;
#else
	struct timeval st;
	gettimeofday(&st, NULL);
	uint64_t usec = st.tv_sec * 1000000ULL + (uint64_t)st.tv_usec;
#endif
	return usec;
}

#define MIN(a, b) ((a) <= (b) ? (a) : (b))

// Thread safe queue - fixed size
template <class T> class CQueue {
public:

	CQueue(int size)
	{
		S = size;
		Init();
	}

	~CQueue()
	{
		delete [] m_que;
	}

	void Init()
	{
		m_que = new T[S];
		m_Cnt = 0;
		m_wrIdx = 0;
		m_rdIdx = 0;
#ifdef HT_SYSC
		m_fullSleepCnt = 0;
		m_emptySleepCnt = 0;
#endif
	}

	int Size()
	{
		return m_Cnt;
	}

	int Space()
	{
		return S - 1 - Size();
	}

	int SpaceN()
	{
		return MIN( (S - 1 - Size()), (S - (int)m_wrIdx) );
	}

	// ---------------------------

	bool IsEmpty()
	{
		return m_rdIdx == *((volatile uint32_t*)(&m_wrIdx));
	}

	bool Empty()
	{
		return m_rdIdx == *((volatile uint32_t*)(&m_wrIdx));
	}

	T & Front(int off=0)
	{
		while (IsEmpty()) {
			usleep(0);
#ifdef HT_SYSC
			m_emptySleepCnt += 1;
#endif
		}
		return m_que[(m_rdIdx+off)%S];
	}

	void FrontN(T *item, int num)
	{
		while (IsEmpty()) {
			usleep(0);
#ifdef HT_SYSC
			m_emptySleepCnt += 1;
#endif
		}
		int r = *((volatile uint32_t*)(&m_rdIdx));
		for (int i = 0; i < num; i++) {
			item[i] = m_que[r];
			r = (r + 1) % S;
		}
		return;
	}

	void Pop()
	{
		assert(!IsEmpty());
		m_rdIdx = (m_rdIdx + 1) % S;
		__sync_fetch_and_sub(&m_Cnt, 1);
		return;
	}

	void PopN(int num)
	{
		assert(!IsEmpty());
		for (int i = 0; i < num; i++) {
			m_rdIdx = (m_rdIdx + 1) % S;
		}
		__sync_fetch_and_sub(&m_Cnt, num);
		return;
	}

	// ---------------------------

	bool IsFull()
	{
		return ((m_wrIdx + 1) % S) == *((volatile uint32_t*)(&m_rdIdx));
	}

	void Push(const T &item)
	{
		while (IsFull()) {
			usleep(0);
#ifdef HT_SYSC
			m_fullSleepCnt += 1;
#endif
		}
		m_que[m_wrIdx] = item;
		m_wrIdx = (m_wrIdx + 1) % S;
		__sync_fetch_and_add(&m_Cnt, 1);
		return;
	}

	void PushN(const T *item, int num)
	{
		for (int i = 0; i < num; i++) {
			m_que[m_wrIdx] = item[i];
			m_wrIdx = (m_wrIdx + 1) % S;
		}
		__sync_fetch_and_add(&m_Cnt, num);
		return;
	}

	// ---------------------------

	T & GetEntry(int idx)
	{
		return m_que[idx];
	}
#ifdef HT_SYSC
	uint64_t GetFullSleepCnt()
	{
		return m_fullSleepCnt;
	}
	uint64_t GetEmptySleepCnt()
	{
		return m_emptySleepCnt;
	}
#endif

private:
	T *m_que;
	int S;
	unsigned int m_Cnt ALIGN8;
	volatile uint32_t m_wrIdx ALIGN8;
	volatile uint32_t m_rdIdx ALIGN8;
	// SystemC variables (must remain for sizeof() to be consistent
	// with libray builds
	uint64_t m_fullSleepCnt;
	uint64_t m_emptySleepCnt;
};

#endif
