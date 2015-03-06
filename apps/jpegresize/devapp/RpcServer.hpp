#ifndef __RPC_SERVER__
#define __RPC_SERVER__

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
    void lockInit(void);
  private:
    void __ProcessJob(request req);
    void err(request req);
    std::vector<pthread_t> threads;
    pthread_rwlock_t threadLock;

};

#endif