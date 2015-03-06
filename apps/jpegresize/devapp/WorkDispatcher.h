/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include "Ht.h"
#include "AppArgs.h"

class WorkDispatcher {
public:
	WorkDispatcher() {
		m_bFirst = true;
		m_pass = 0;
		m_lock = 0;
		m_jobIdx = 0;
	}

	bool getWork( int threadId, AppJob & appJob );

	int getJobCnt() { return m_jobIdx; }

private:
	void findImageSize( int threadId, string &inputFile, int &width, int &height );
	void resizeImage( int threadId, AppJob & appJob );

	void ObtainLock() {
		while (__sync_fetch_and_or(&m_lock, 1) == 1)
			usleep(1);
	}

	void ReleaseLock() {
	  __sync_synchronize();
	  m_lock = 0;
	}

private:
	volatile int64_t m_lock;
	int m_jobIdx;
	int m_pass;
	uint64_t m_startTime;
	bool m_bFirst;
	string m_tempFile;
	AppJob m_inAppJob;
	AppJob m_outAppJob;
};

extern WorkDispatcher * g_pWorkDispatcher;
