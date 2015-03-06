#ifndef __REMOTE_PROCESS_JOB__
#define __REMOTE_PROCESS_JOB__

#ifdef ENABLE_DAEMON
#include <string>

#include <msgpack/rpc/client.h>
#include <msgpack/rpc/server.h>

namespace rpc {
        using namespace msgpack;
        using namespace msgpack::rpc;
}  // namespace rpc
using namespace std;


struct AppJob;

struct rpcCallResponse {
    int status;
    unsigned int len;
    uint8_t buffer;
};

static rpc::session_pool g_sessionPool;

bool RemoteProcessJob( AppJob &appJob );

#endif
#endif