/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <string>
#include "serverStats.hpp"
#include "job.hpp"
#include "msgpack/rpc/server.h"

serverStats getServerStats(job globalJobArgs) {
  serverStats stats;
  msgpack::rpc::session_pool p;
  msgpack::rpc::session s=p.get_session(globalJobArgs.server, globalJobArgs.port);
  msgpack::rpc::future fs=s.call("stats");
  string resp=fs.get<string>();
  msgpack::zone mempool;
  msgpack::object deserialized;
  msgpack::unpack(resp.data(), resp.size(), NULL, &mempool, &deserialized);
  msgpack::type::tuple<uint64_t, long long, long long> packedStats;
  deserialized.convert(&packedStats);
  stats.timeStamp=packedStats.get<0>();
  stats.jobCount=packedStats.get<1>();
  stats.jobTime=packedStats.get<2>();
  return stats;
}

//inline void printStats(serverStats start, serverStats finish) {
//  printStats(start, finish, 0);
//}

void printStats(serverStats start, serverStats finish, long long clientJobCount) {
  double seconds=(finish.timeStamp - start.timeStamp) / 1000000.0;
  long long jobCount=finish.jobCount-start.jobCount;
  double avgJobTime=0;
  double avgActiveJobs=0;
  if(jobCount>0) {
    avgJobTime=(finish.jobTime-start.jobTime)/(double)jobCount;
  }
  if(clientJobCount) {
    avgActiveJobs=((double)clientJobCount*avgJobTime)/1000000.0/seconds;    
    jobCount=clientJobCount;
  } else {
    avgActiveJobs=(finish.jobTime-start.jobTime)/1000000.0/seconds;    
  }
  printf("%lld jobs completed in %5.3f seconds (%4.2f jobs/sec)\n", jobCount, seconds, jobCount / seconds);
  printf("   Coproc had an average of %4.2f jobs\n",avgActiveJobs);
  printf("   Coproc average time per job %4.2f usec\n",avgJobTime);   
}