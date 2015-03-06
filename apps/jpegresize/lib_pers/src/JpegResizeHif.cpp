/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "JpegResizeHif.h"
#include <pthread.h>

bool g_IsCheck=0;

JpegResizeHif::JpegResizeHif(CHtHifParams * pHifParams) : CHtHif(pHifParams) {
  //m_pUnit = AllocAuUnit();
  m_pUnit = new CHtAuUnit(this);
  m_lock = 0;
  m_lastStatsTime=ht_usec();
  for (uint8_t jobId = 0; jobId < MAX_COPROC_JOBS; jobId += 1) {
    m_jobIdQue.push(jobId);
    m_bJobDoneVec[jobId] = false;
    m_bJobBusyVec[jobId] = false;
    sem_init(&m_jobSem[jobId], 0, 0);
  }
  pthread_mutex_init(&m_jobIdQueLock, NULL);
  pthread_mutex_init(&m_coprocLock, NULL);
  m_bRecvMsgHandlerExit=0;
  pthread_create(&m_recvMsgHandler, NULL, StartRecvMsgHandler, (void *)this);
}

JpegResizeHif::~JpegResizeHif() {
  void *ret;
  m_bRecvMsgHandlerExit=1;
  pthread_join(m_recvMsgHandler, &ret);
  //delete pHtHif;
}

void JpegResizeHif::SubmitJob(JobInfo * pJobInfo) {
  // multi-threaded job submission routine
  pthread_mutex_lock(&m_jobIdQueLock);
  // obtain job ID
  while (m_jobIdQue.empty()) {
    pthread_mutex_unlock(&m_jobIdQueLock);
    usleep(1);
    pthread_mutex_lock(&m_jobIdQueLock);
  }
  
  uint8_t jobId = m_jobIdQue.front();
  m_jobIdQue.pop();
  pthread_mutex_unlock(&m_jobIdQueLock);
  // clear job finished flag
  m_bJobDoneVec[jobId] = false;
  m_bJobBusyVec[jobId] = true;
  
  // determine persMode, first use of a jobInfo structure run full job
  //   to ensure
  uint8_t persMode = 3;
  /*
   *		uint8_t persMode = g_pAppArgs->GetPersMode();
   *		if (persMode != 3 && pJobInfo->m_bFirstUse) {
   *			persMode = 3;
   *			pJobInfo->m_bFirstUse = false;
}*/
  
  // send job to coproc
  pthread_mutex_lock(&m_coprocLock);
  while (!m_pUnit->SendCall_htmain(jobId, (uint64_t)pJobInfo, persMode)) {
    pthread_mutex_unlock(&m_coprocLock);
    usleep(1);
    pthread_mutex_lock(&m_coprocLock);
  }
  pthread_mutex_unlock(&m_coprocLock);
  uint64_t startUsec = ht_usec();
  
  // wait for job to finish
  sem_wait(&m_jobSem[jobId]);
  
  uint64_t endUsec = ht_usec();

  ObtainLock();
  m_coprocJobUsecs += endUsec - startUsec;
  m_coprocJobs += 1;
  
  // free jobId
  m_bJobBusyVec[jobId] = false;
  m_jobIdQue.push(jobId);
  
  ReleaseLock();
}

void JpegResizeHif::RecvMsgHandler(void) {
    uint8_t recvJobId;
    while(!m_bRecvMsgHandlerExit) {
      pthread_mutex_lock(&m_coprocLock);
      if (m_pUnit->RecvReturn_htmain(recvJobId)) {
	assert(recvJobId < MAX_COPROC_JOBS && m_bJobBusyVec[recvJobId] == true);
	m_bJobDoneVec[recvJobId] = true;      
	pthread_mutex_unlock(&m_coprocLock);	
	sem_post(&m_jobSem[recvJobId]);
      } else {
	pthread_mutex_unlock(&m_coprocLock);
	usleep(1);
      }
    }
}

void JpegResizeHif::ObtainLock() {
  while (__sync_fetch_and_or(&m_lock, 1) == 1)
    usleep(1);
}

void JpegResizeHif::ReleaseLock() {
  __sync_synchronize();
  m_lock = 0;
}

void JpegResizeHif::jobStats(uint64_t &timeStamp, long long &jobCount, long long &jobTime) {
  timeStamp=ht_usec();
  jobCount=m_coprocJobs;
  jobTime=m_coprocJobUsecs;
  return;
}

void JpegResizeHif::jobStats(double &seconds, long long &jobCnt, double &jobActiveAvg, double &jobTimeAvg) {
	uint64_t now=ht_usec();
	seconds = (now - m_lastStatsTime) / 1000000.0;
	jobCnt = m_coprocJobs - m_lastCoprocJobs;
	jobActiveAvg=(m_coprocJobUsecs - m_lastCoprocJobUsecs) /1000000.0/ seconds;
	if (jobCnt > 0) {
	  jobTimeAvg=(m_coprocJobUsecs - m_lastCoprocJobUsecs) / (double)jobCnt;
	} else {
	  jobTimeAvg=0;
	}
	m_lastCoprocJobs = m_coprocJobs;
	m_lastCoprocJobUsecs = m_coprocJobUsecs;
	m_lastStatsTime = now;
}


extern "C" void* StartRecvMsgHandler(void* ptr) {
  ((JpegResizeHif*)ptr)->RecvMsgHandler();
  pthread_exit((void *)0);
}
