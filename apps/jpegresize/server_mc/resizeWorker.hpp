#pragma once

#include <msgpack.hpp>
#include <msgpack/rpc/server.h>
#include <pthread.h>
#include <vector>
#include <semaphore.h>
#include <queue>
#include "rpcServer.hpp"
#include "coproc.hpp"

namespace rpc {
        using namespace msgpack;
        using namespace msgpack::rpc;
}  // namespace rpc
typedef rpc::request request;


class resizeWorker {
public:
  resizeWorker(class resizeDispatcher * dispatcher, int threads, int unit, int jobSlots, int responders);
  ~resizeWorker();
  inline void jobStats(double &seconds, long long &jobCnt, double &jobActiveAvg, double &jobTimeAvg) {
    if(coproc)
      coproc->jobStats(seconds, jobCnt, jobActiveAvg, jobTimeAvg);
  }
  inline void jobStats(uint64_t &timeStamp, long long &jobCount, long long &jobTime) {
    if(coproc) 
      coproc->jobStats(timeStamp, jobCount, jobTime);
  }
  void reqWorker(void);
  void respWorker(void);
private:
  void resize(request req);
  class resizeDispatcher * dispatcher;
  class coproc * coproc;
  std::vector<pthread_t> reqThreads;
  std::vector<pthread_t> respThreads;
  //pthread_rwlock_t threadLock;
  int id;
};

extern "C" void* startResizeReqWorker(void* ptr);
extern "C" void* startResizeRespWorker(void* ptr);