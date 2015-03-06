/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "cmdArgs.hpp"
#include "rpcServer.hpp"
#include "coproc.hpp"
#include <msgpack/rpc/server.h>

namespace rpc {
  using namespace msgpack::rpc;
}  

int main(int argc, char **argv)
{
  double seconds;
  long long jobCnt;
  double jobActiveAvg;
  double jobTimeAvg;
  long long delay=5000000;
  if(!parseArgs(argc, argv))
    exit(1);
  if(appArguments.stats) {
    delay=appArguments.stats*1000000;
  }
#ifdef DEBUG
  printArgs();
#endif
  
  coprocInit(appArguments.threads);
  rpc::server svr;
  std::auto_ptr<rpc::dispatcher> dp(new rpcServer);
  
  svr.serve(dp.get());
  //bind to interfaces  
  svr.listen(appArguments.iface, appArguments.port);
  
  //start the server threads 
  svr.start(appArguments.threads);
  //go to sleep to keep from burning CPU.  There is no program exit.  We will
  //simply sleep in this loop.  Might consider handling cleanup and exit in
  //a signal handler.
  printf("Initialization complete.\n");
  while(1) {
    usleep(delay);
    if(appArguments.stats) {
      g_pJpegResizeHif->jobStats(seconds, jobCnt, jobActiveAvg, jobTimeAvg);
      printf("%lld jobs completed in %5.3f seconds (%4.2f jobs/sec)\n", jobCnt, seconds, jobCnt / seconds);
      printf("   Coproc had an average of %4.2f jobs\n",jobActiveAvg);
      printf("   Coproc average time per job %4.2f usec\n",jobTimeAvg);   
    }
  };		  
  return(0);
}

