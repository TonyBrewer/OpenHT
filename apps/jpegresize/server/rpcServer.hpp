#pragma once

#include <msgpack.hpp>
#include <msgpack/rpc/server.h>
#include <pthread.h>
#include <vector>

namespace rpc {
        using namespace msgpack;
        using namespace msgpack::rpc;
}  // namespace rpc

typedef rpc::request request;
    
class rpcServer : public rpc::dispatcher {
  public:
    void dispatch(request req);
  private:
    void resize(request req);
    void stats(request req);
    void err(request req);
    std::vector<pthread_t> threads;
    pthread_rwlock_t threadLock;

};
