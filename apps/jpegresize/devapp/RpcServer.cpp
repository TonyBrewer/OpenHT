/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#ifdef ENABLE_DAEMON
#include "Ht.h"
using namespace Ht;
#include <stdlib.h>
#include <msgpack/rpc/server.h>
#include "RpcServer.hpp"
#include <exception>
#include <iostream>
#include <cstring>
#include <string>
#include "AppArgs.h"
#include "RemoteProcessJob.hpp"
#include "ProcessJob.h"
#include "JpegResizeHif.h"
#include <vector>
#include <iterator>

using namespace std;
using msgpack::type::raw_ref;

//main RPC call handler.  This is called each time a remote client exectutes
//a client.call().
void rpcServer::dispatch(request req) {
  std::string method;
  try {
    //pull the method we are trying to call out of the request
    req.method().convert(&method);
    //All we know about right now is "ProcessJob" and "err", however we could add others
    if(method == "ProcessJob") {
      //pass the request on to the handler for the resize method
      __ProcessJob(req);      
    } else if(method == "exit") {
      req.result(1);
    } else if(method == "err") {
      err(req);     
    //if we don't know what the method is, then we return an error.      
    } else {
      req.error(msgpack::rpc::NO_METHOD_ERROR);
    }
    
  } catch (msgpack::type_error& e) {
    req.error(msgpack::rpc::ARGUMENT_ERROR);
    return;
    
  } catch (std::exception& e) {
    req.error(std::string(e.what()));
    return;
  }
}


void rpcServer::__ProcessJob(request req) {
  AppJob appJob;
  raw_ref inputBuffer;
  bool status;
  //extract the parameters.  We should really wrap this in a try/catch
  //and check for invalid parameter types, missing paramenters, etc.
  //Order and type is critical here and must match the order in the call
  //in ProcessRemoteJob()
  msgpack::type::tuple<uint32_t, string, raw_ref, string, int, int, int, int> req_params;
  req.params().convert(&req_params);
  appJob.m_mode=req_params.get<0>();
  appJob.m_inputFile=req_params.get<1>();
  inputBuffer=req_params.get<2>();
  appJob.m_inputBuffer=(uint8_t *)inputBuffer.ptr;
  appJob.m_inputBufferSize=inputBuffer.size;
  appJob.m_outputFile=req_params.get<3>();
  appJob.m_width=req_params.get<4>();
  appJob.m_height=req_params.get<5>();
  appJob.m_scale=req_params.get<6>();
  appJob.m_quality=req_params.get<7>();
  //find our "process index"
  pthread_t myThreadId=pthread_self();
  pthread_rwlock_rdlock(&threadLock);
  std::vector<pthread_t>::iterator it = std::find(threads.begin(), threads.end(), myThreadId);
  if(it==threads.end()) {
      pthread_rwlock_unlock(&threadLock);
      pthread_rwlock_wrlock(&threadLock);
      threads.push_back(myThreadId);
      it=threads.end() - 1 ;
  }
  pthread_rwlock_unlock(&threadLock);
  int threadId=std::distance(threads.begin(), it);
  status=processJob(threadId, appJob);
  //We either return a buffer with the image after resize, or a pass/fail boolean.  In the case
  //where we return a buffer, then the buffer will be empty (NULL) in case of failure.
  if(appJob.m_mode & MODE_OUTBUFF) {
    if(status) {
      req.result(raw_ref((const char *)appJob.m_outputBuffer, appJob.m_outputBufferSize));  
    } else {
      req.result(raw_ref((const char *)NULL, 0));  
    }
    if(appJob.m_outputBuffer) {
      free(appJob.m_outputBuffer);
    }
  } else {
    req.result(status);
  }
}	

void rpcServer::err(request req) {
  req.error(std::string("always fail"));
}

void rpcServer::lockInit(void) {
  pthread_rwlock_init(&threadLock, NULL);
}

#endif
