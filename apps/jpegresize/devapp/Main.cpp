/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <string>

#include "Ht.h"
using namespace Ht;
#include "AppArgs.h"
#include "JpegHdr.h"
#include "JobInfo.h"
#include "JobDV.h"
#include "JpegResizeHif.h"
//#include "LoopOnImageTest.h"
#include "WorkDispatcher.h"
#include "ProcessJob.h"
#ifdef ENABLE_DAEMON
#include "RpcServer.hpp"
#include "RemoteProcessJob.hpp"
#include <msgpack/rpc/server.h>
#include <msgpack/rpc/client.h>

namespace rpc {
	using namespace msgpack;
	using namespace msgpack::rpc;
}  // namespace rpc


#endif //ENABLE_DAEMON

JpegResizeHif * g_pJpegResizeHif = 0;
volatile uint32_t g_completedWorkerThreadCnt = 0;
volatile bool g_forceErrorExit = false;
long long g_coprocJobUsecs = 0;
long long g_coprocJobs = 0;

void * WorkerThread(void *);

inline void jobStats (uint64_t & lastTime, long long & lastCoprocJobs, long long & lastCoprocJobUsecs) {
	double seconds = (ht_usec() - lastTime) / 1000000.0;
	uint64_t timeStamp;
	g_pJpegResizeHif->jobStats(timeStamp, g_coprocJobs, g_coprocJobUsecs);
	long long jobCnt = g_coprocJobs - lastCoprocJobs;

	printf("%lld jobs completed in %5.3f seconds (%4.2f jobs/sec)\n",
		jobCnt, seconds, jobCnt / seconds);

	printf("   Coproc had an average of %4.2f jobs\n",
		(g_coprocJobUsecs - lastCoprocJobUsecs) /1000000.0/ seconds);

	if (jobCnt > 0)
		printf("   Coproc average time per job %4.2f usec\n",
		(g_coprocJobUsecs - lastCoprocJobUsecs) / (double)jobCnt);

	lastCoprocJobs = g_coprocJobs;
	lastCoprocJobUsecs = g_coprocJobUsecs;
	lastTime = ht_usec();
}

int main(int argc, char **argv)
{
	g_pAppArgs = new AppArgs(argc, argv);

	if (!g_pAppArgs->IsHostOnly()) {
		if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, "Creating Hif ...");

		CHtHifParams hifParams;
		// host memory accessed by coproc per image
		//   jobInfo, input image, output buffer
		size_t alignedJobInfoSize = (sizeof(JobInfo) + 63) & ~63;
		hifParams.m_appUnitMemSize = (alignedJobInfoSize + 3*MAX_JPEG_IMAGE_SIZE) * g_pAppArgs->GetThreads();

		g_pJpegResizeHif = new JpegResizeHif(&hifParams);

		JobInfo::SetThreadCnt( g_pAppArgs->GetThreads() );
		JobInfo::SetHifMemBase( g_pJpegResizeHif->GetAppUnitMemBase(0) );
		if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, " done\n");
		if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, "Coprocessor Frequency ... %d MHz\n",
						      g_pJpegResizeHif->GetUnitMHz());
		if (g_pAppArgs->IsHostMem())
			printf("Note: Host memory only\n");
		fflush(stdout);
	} else {
		printf("### Coprocessor resources not allocated (host_only) ###\n");

		size_t alignedJobInfoSize = (sizeof(JobInfo) + 63) & ~63;
		size_t appUnitMemSize = (alignedJobInfoSize + 3 * MAX_JPEG_IMAGE_SIZE) * g_pAppArgs->GetThreads();

		uint8_t * pHostMem;
		bool bFail = ht_posix_memalign((void **)&pHostMem, 64, appUnitMemSize );
		assert(!bFail);
		JobInfo::SetThreadCnt( g_pAppArgs->GetThreads() );
		JobInfo::SetHifMemBase( pHostMem );
	}
	uint64_t lastTime = ht_usec();
	long long lastCoprocJobs = 0;
	long long lastCoprocJobUsecs = ht_usec();

#ifdef ENABLE_DAEMON		
	if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, "daemon functionality available.\n");
	//If we will be running as a daemon, then we start up our RPC server with
	//the appropriate number of threads and put the main thread to sleep.
	if (g_pAppArgs->IsDaemon()) {
	  if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, "starting daemon ...");
	  rpc::server svr;
	  //Our extension of the dispatcher that knows how to handle our methods.
	  std::auto_ptr<rpc::dispatcher> dp(new rpcServer);
	  svr.serve(dp.get());
	  //bind to all interfacaes
	  svr.listen(g_pAppArgs->GetServer(), g_pAppArgs->GetPort());
	  //start the server
	  svr.start(g_pAppArgs->GetThreads());
	  if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, " done\n");
	  //go to sleep to keep from burning CPU.
	  while(1) {
	    usleep(5000000);
	    jobStats(lastTime, lastCoprocJobs, lastCoprocJobUsecs);
	  };		  
	  return(0);
	}
#endif  //ENABLE_DAEMON

	g_pWorkDispatcher = new WorkDispatcher();

	uint64_t startTime = ht_usec();

	// start worker threads
	for (uint32_t i = 0; i < g_pAppArgs->GetThreads(); i += 1) {
		pthread_t workerThread;
		pthread_create(&workerThread, 0, WorkerThread, (void *)(long)i);
	}

	// wait for threads to complete
	while (g_completedWorkerThreadCnt < g_pAppArgs->GetThreads()) {
		if (!g_pAppArgs->IsDaemon())

			usleep(10000);

		else {
			usleep(5000000);	// 5 seconds
			jobStats(lastTime, lastCoprocJobs, lastCoprocJobUsecs);
		}
	}
	uint64_t endTime;
	g_pJpegResizeHif->jobStats(endTime, g_coprocJobs, g_coprocJobUsecs);

	double seconds = (endTime - startTime) / 1000000.0;

	printf("%d jobs completed in %5.3f seconds (%4.2f jobs/sec)\n",
		g_pWorkDispatcher->getJobCnt(), seconds, g_pWorkDispatcher->getJobCnt() / seconds);

	if (!g_pAppArgs->IsHostOnly()) {
		printf("   Coproc had an average of %4.2f jobs\n",
			g_coprocJobUsecs / (double)(endTime - startTime));

		if (g_pWorkDispatcher->getJobCnt() > 0)
			printf("   Coproc average time per job %4.2f usec\n",
				g_coprocJobUsecs / (double)g_pWorkDispatcher->getJobCnt());
	}

//#ifndef HT_SYSC
	if (!g_pAppArgs->IsHostOnly())
		delete g_pJpegResizeHif;
//#endif
	return 0;
}

void * WorkerThread(void * pVoid)
{
	int threadId = (int)(long)pVoid;

	AppJob appJob;
#ifdef MAGIC_FALLBACK
	if(g_pAppArgs->IsMagicMode()) {
	  appJob.m_mode|=MODE_MAGICK;
	}
#endif
	while (!g_forceErrorExit && g_pWorkDispatcher->getWork( threadId, appJob )) {
#ifdef MAGICK_FALLBACK
	if(g_pAppArgs->IsMagickMode()) {
	  appJob.m_mode|=MODE_MAGICK;
	}
#endif	  
#ifdef ENABLE_DAEMON
		//If we are running against a remote server then we need to make the
		//remote version of the processJob call.
		if(g_pAppArgs->IsRemote()) {
			if (!RemoteProcessJob( appJob )) {
				g_forceErrorExit = true;
				break;
			}
		} else {
#endif //ENABLE_DAEMON

//We should never get here with IsDaemon==true.  Remove these commented lines after
//testing
//			if (g_pAppArgs->IsDaemon())
//				processJob( threadId, appJob );
//			else {
				if (!processJob( threadId, appJob )) {
					g_forceErrorExit = true;
					break;
				}
//			}

#ifdef ENABLE_DAEMON
		}
#endif //ENABLE_DAEMON
	}

	__sync_fetch_and_add( &g_completedWorkerThreadCnt, 1);

	return 0;
}
