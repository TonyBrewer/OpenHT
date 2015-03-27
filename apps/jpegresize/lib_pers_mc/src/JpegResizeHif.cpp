/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "JpegResizeHif.h"
#include "JobInfo.h"
#include <pthread.h>

bool g_IsCheck=0;

#define MAX_THREAD_LOG 1000000

JpegResizeHif::JpegResizeHif(CHtHifParams * pHifParams, int threads) : CHtHif(pHifParams) {
  //m_pUnit = AllocAuUnit();
  
  m_pPoolHead = 0;
  m_threadIdx = 0;
  m_pThreadJobInfo = 0;
  m_pThreadLog = 0;
  m_threadLogWrIdx = 0;
  m_threadCnt=threads;
  m_pUnit = new CHtAuUnit(this);
  m_pHifInPicBase = GetAppUnitMemBase(0);
  m_pHifRstBase = m_pHifInPicBase + (long long)m_threadCnt * MAX_JPEG_IMAGE_SIZE;
  m_pHifJobInfoBase = m_pHifInPicBase + (long long)m_threadCnt * 3 * MAX_JPEG_IMAGE_SIZE;
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

JobInfo *JpegResizeHif::getJobInfoSlot(int threadId, JpegResizeHif * resizeHif, bool check, bool useHostMem, bool preAllocSrcMem) {
  	Lock();
	if (m_pThreadJobInfo == 0) {
		m_pThreadJobInfo = new bool[m_threadCnt];
		memset(m_pThreadJobInfo, 0, sizeof(bool)*m_threadCnt);

		m_pThreadLog = new uint8_t[MAX_THREAD_LOG];
	}

	if (m_threadLogWrIdx < MAX_THREAD_LOG)
		m_pThreadLog[m_threadLogWrIdx++] = threadId;
	/*
	if (m_pThreadJobInfo[threadId]) {
		printf("Internal error: new JobInfo - threadId %d requested a second jobInfo\n", threadId);
		FILE *fp = fopen("JobInfoLog.txt", "w");
		fprintf(fp, "Internal error: new JobInfo - threadId %d requested a second jobInfo\n", threadId);
		for (unsigned int i = 0; i < m_threadCnt; i += 1)
			fprintf(fp, "thread[%d] = %d\n", i, m_pThreadJobInfo[i]);
		for (int i = 0; i < m_threadLogWrIdx; i += 1)
			fprintf(fp, "Log %d: %s %d\n", i, (m_pThreadLog[i] & 0x80) == 0 ? "new" : "delete", m_pThreadLog[i] & 0x7f);
		fclose(fp);		
		exit(1);
	}
	*/
	m_pThreadJobInfo[threadId] = true;

	if (m_pPoolHead == 0) {
/*
		if (m_threadIdx == m_threadCnt) {
			printf("Internal error: new JobInfo exceeded thread count, threadIdx=%d, threadCnt=%d\n", m_threadIdx, m_threadCnt);
			for (unsigned int i = 0; i < m_threadCnt; i += 1)
				printf("thread[%d] = %d\n", i, m_pThreadJobInfo[i]);
			exit(0);
		}
*/
		size_t alignedJobInfoSize = (sizeof(JobInfo) + 63) & ~63;
		m_pPoolHead = (JobInfo *)(m_pHifJobInfoBase + (long long)m_threadIdx * alignedJobInfoSize);
		m_pPoolHead->m_pPoolNext = 0;
		m_pPoolHead->m_bFirstUse = true;

		// allocate memory needed by image assuming max image size
		m_pPoolHead->m_dec.m_pRstBase = m_pHifInPicBase + (long long) m_threadIdx * MAX_JPEG_IMAGE_SIZE;
		if(preAllocSrcMem) {
		  m_pPoolHead->m_pInPic = (uint8_t *)malloc(MAX_JPEG_IMAGE_SIZE);
		}

		for (int ci = 0; ci < 3; ci += 1) {
			if (check) {
				if (useHostMem) {
					int fail = ht_posix_memalign( (void **)&m_pPoolHead->m_dec.m_dcp[ci].m_pCompBuf,
						MAX_IMAGE_COMP_SIZE, MAX_IMAGE_COMP_SIZE );
					assert(!fail);
				} else {
				  m_pPoolHead->m_dec.m_dcp[ci].m_pCompBuf = (uint8_t*)resizeHif->MemAllocAlign(MAX_IMAGE_COMP_SIZE, MAX_IMAGE_COMP_SIZE);
				  assert(m_pPoolHead->m_dec.m_dcp[ci].m_pCompBuf!=NULL);
				}
			} else {
				m_pPoolHead->m_dec.m_dcp[ci].m_pCompBuf = 0;
			}
		}

		uint8_t * pCpMem;
		if (useHostMem) {
			bool fail = ht_posix_memalign( (void **)&pCpMem, MAX_IMAGE_COMP_SIZE, MAX_IMAGE_COMP_SIZE * 3 );
			assert(!fail);
		} else {
		  pCpMem = (uint8_t*)resizeHif->MemAllocAlign(MAX_IMAGE_COMP_SIZE, MAX_IMAGE_COMP_SIZE*3);
		  assert(pCpMem!=NULL);
		}

		for (int ci = 0; ci < 3; ci += 1)
			m_pPoolHead->m_horz.m_hcp[ci].m_pOutCompBuf = pCpMem + ci * MAX_IMAGE_COMP_SIZE;

		for (int ci = 0; ci < 3; ci += 1) {
			if (check) {
				if (useHostMem) {
					bool fail = ht_posix_memalign( (void **)&m_pPoolHead->m_vert.m_vcp[ci].m_pOutCompBuf,
						MAX_IMAGE_COMP_SIZE, MAX_IMAGE_COMP_SIZE );
					assert(!fail);
				} else {
				  m_pPoolHead->m_vert.m_vcp[ci].m_pOutCompBuf = (uint8_t*)resizeHif->MemAllocAlign(MAX_IMAGE_COMP_SIZE, MAX_IMAGE_COMP_SIZE*3);
				  assert(m_pPoolHead->m_vert.m_vcp[ci].m_pOutCompBuf!=NULL);
				}
			} else {
				m_pPoolHead->m_vert.m_vcp[ci].m_pOutCompBuf = 0;
			}
		}

		m_pPoolHead->m_enc.m_pRstBase = m_pHifRstBase + (long long) m_threadIdx * MAX_JPEG_IMAGE_SIZE;

		m_threadIdx += 1;
	}

	JobInfo * pJobInfo = m_pPoolHead;
	m_pPoolHead = m_pPoolHead->m_pPoolNext;
	
	pJobInfo->m_threadId = threadId;
	pJobInfo->m_pJpegResizeHif=resizeHif;
	Unlock();
    return(pJobInfo);
}

void JpegResizeHif::releaseJobInfoSlot(JobInfo *pJobInfo) {
 	Lock();
	if (m_threadLogWrIdx < MAX_THREAD_LOG)
		m_pThreadLog[m_threadLogWrIdx++] = 0x80 | pJobInfo->m_threadId;
/*
	if (!m_pThreadJobInfo[pJobInfo->m_threadId]) {
		printf("Internal error: delete JobInfo - threadId %d\n", pJobInfo->m_threadId);
		FILE *fp = fopen("JobInfoLog.txt", "w");
		fprintf(fp, "Internal error: delete JobInfo - threadId %d\n", pJobInfo->m_threadId);
		for (unsigned int i = 0; i < m_threadCnt; i += 1)
			fprintf(fp, "thread[%d] = %d\n", i, m_pThreadJobInfo[i]);
		for (int i = 0; i < m_threadLogWrIdx; i += 1)
			fprintf(fp, "Log %d: %s %d\n", i, (m_pThreadLog[i] & 0x80) == 0 ? "new" : "delete", m_pThreadLog[i] & 0x7f);
		fclose(fp);		
		exit(1);
	}
*/	
	m_pThreadJobInfo[pJobInfo->m_threadId] = false;
	
	pJobInfo->m_pPoolNext = m_pPoolHead;
	m_pPoolHead = pJobInfo;

	Unlock(); 
  
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
  m_pJobSlot[jobId]=pJobInfo;
  
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
  pJobInfo->startUsec = ht_usec();
  if(!pJobInfo->job) {
  // wait for job to finish
  sem_wait(&m_jobSem[jobId]);
  
  uint64_t endUsec = ht_usec();

  ObtainLock();
  m_coprocJobUsecs += endUsec - pJobInfo->startUsec;
  m_coprocJobs += 1;
  
  // free jobId
  m_bJobBusyVec[jobId] = false;
  m_jobIdQue.push(jobId);
  
  ReleaseLock();
  }
}

void JpegResizeHif::RecvMsgHandler(void) {
    uint8_t recvJobId;
    while(!m_bRecvMsgHandlerExit) {
      pthread_mutex_lock(&m_coprocLock);
      if (m_pUnit->RecvReturn_htmain(recvJobId)) {
	uint64_t endUsec = ht_usec();
	assert(recvJobId < MAX_COPROC_JOBS && m_bJobBusyVec[recvJobId] == true);
	m_bJobDoneVec[recvJobId] = true;      
	pthread_mutex_unlock(&m_coprocLock);	
	if(m_pJobSlot[recvJobId]->job) {
	  m_coprocJobUsecs += endUsec - m_pJobSlot[recvJobId]->startUsec;
	  m_coprocJobs += 1;	  
	  //put the coproc results on queue for processing by response thread
	  outputEnqueue(m_pJobSlot[recvJobId]);
	  m_pJobSlot[recvJobId]=0;
	  ObtainLock();
	  m_bJobBusyVec[recvJobId] = false;
	  m_jobIdQue.push(recvJobId);
	  ReleaseLock();
	} else {
	  sem_post(&m_jobSem[recvJobId]);
	}
      } else {
	pthread_mutex_unlock(&m_coprocLock);
	usleep(1);
      }
    }
}

void JpegResizeHif::outputEnqueue(JobInfo * pJobInfo) {
  std::unique_lock<std::mutex> mlock(responseQueueMutex);
  responseQueue.push(pJobInfo);
  mlock.unlock();
  responseQueueCond.notify_one();
}

JobInfo * JpegResizeHif::outputDequeue(void) {
  std::unique_lock<std::mutex> mlock(responseQueueMutex);
  while(responseQueue.empty()) {
    responseQueueCond.wait(mlock);
  }
  JobInfo *pJobInfo=responseQueue.front();
  responseQueue.pop();
  return pJobInfo;
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
