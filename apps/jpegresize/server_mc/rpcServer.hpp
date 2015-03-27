#pragma once

#include <msgpack.hpp>
#include <msgpack/rpc/server.h>
#include <vector>
#include <queue>
#include "resizeWorker.hpp"
#include <mutex>
#include <condition_variable>
namespace rpc {
        using namespace msgpack;
        using namespace msgpack::rpc;
}  // namespace rpc

typedef rpc::request request;
    
class resizeDispatcher : public rpc::dispatcher {
  public:
    resizeDispatcher(int threads, int units, int jobSlots, int responders);
    ~resizeDispatcher();
    void dispatch(request req);
    rpc::request resizeReqDequeue(void);
    void jobStats(double &seconds, long long &jobCnt, double &jobActiveAvg, double &jobTimeAvg);
  private:
    void resizeReqEnqueue(request req);
    void stats(request req);
    void err(request req);
    std::queue<rpc::request> requestQueue;
    std::mutex requestQueueMutex;
    std::condition_variable requestQueueCond;
    resizeWorker **units;
    int m_unitCount;
};

