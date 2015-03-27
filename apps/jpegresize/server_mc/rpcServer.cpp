/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <exception>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <iostream>
#include <fstream>
#include <signal.h>

#include "rpcServer.hpp"
#include "cmdArgs.hpp"
#include "profile.hpp"

using namespace std;
using msgpack::type::raw_ref;

//bool processJob(jobDefinition &job) { return 1; }

//main RPC call handler.  This is called each time a remote client exectutes
//a client.call().
void resizeDispatcher::dispatch(request req) {
  string method;
  try {
    //pull the method we are trying to call out of the request
    req.method().convert(&method);
    //All we know about right now is "resize"
    if(method == "resize") {
      //pass the request on to the handler for the resize method
      resizeReqEnqueue(req);
      //if we don't know what the method is, then we return an error.      
    } else if(method == "stats") {
      stats(req);
    } else {
      req.error(msgpack::rpc::NO_METHOD_ERROR);
    }
    //If the method argument is bad, then we are going to get an exception
    //when we try and convert it to a string.  Trap that exception and 
    //return and argument error the to the caller.
  } catch (msgpack::type_error& e) {
    req.error(msgpack::rpc::ARGUMENT_ERROR);
    return;
    //If we get some uknown error then we will return the string describing
    //the error back to the caller.
  } catch (exception& e) {
    req.error(string(e.what()));
    return;
  }
}

resizeDispatcher::resizeDispatcher(int threadCount, int unitCount, int jobSlots, int responders) {
  m_unitCount=unitCount;
  units=new resizeWorker *[unitCount];
  for(int i=0; i<unitCount; i++) {
    units[i]=new resizeWorker(this, threadCount, i, jobSlots, responders);
  }
}

resizeDispatcher::~resizeDispatcher() {
  for(int i=0; i<m_unitCount; i++) {
    delete units[i];
  }
  delete units;
}

void resizeDispatcher::resizeReqEnqueue(request req) {
  std::unique_lock<std::mutex> mlock(requestQueueMutex);
  requestQueue.push(req);
  mlock.unlock();
  requestQueueCond.notify_one();
}

rpc::request resizeDispatcher::resizeReqDequeue(void) {
  std::unique_lock<std::mutex> mlock(requestQueueMutex);
  while(requestQueue.empty()) {
    PROFILE_FUNC(requestQueueCond.wait(mlock));
  }
  rpc::request req(requestQueue.front());
  requestQueue.pop();
  return req;
}

void resizeDispatcher::jobStats(double &seconds, long long &jobCnt, double &jobActiveAvg, double &jobTimeAvg) {
  double tmpSeconds;
  long long tmpJobCnt;
  double tmpJobActiveAvg;
  double tmpJobTimeAvg;
  seconds=0;
  jobCnt=0;
  jobActiveAvg=0;
  jobTimeAvg=0;
  for(int i=0; i<m_unitCount; i++) {
    units[i]->jobStats(tmpSeconds, tmpJobCnt, tmpJobActiveAvg, tmpJobTimeAvg);
    seconds+=tmpSeconds;
    jobCnt+=tmpJobCnt;
    jobActiveAvg+=tmpJobActiveAvg;
    jobTimeAvg+=tmpJobTimeAvg;
  }
  seconds=tmpSeconds;
  jobActiveAvg/=m_unitCount;
  jobTimeAvg/=m_unitCount;
}

void resizeDispatcher::stats(request req) {
  uint64_t timeStamp;
  long long jobCount;
  long long totalJobCount=0;
  long long jobTime;
  long long totalJobTime=0;
  
  for(int i=0; i<m_unitCount; i++) {
    units[i]->jobStats(timeStamp, jobCount, jobTime);
    totalJobCount+=jobCount;
    totalJobTime+=jobTime;
  }
  
  //This extra contortion is necessary because msgpack-rpc only allows a single return value,
  //similar to a normal C function.  We serialize the four values into a single string and return
  //that.  The receiver will be responsible for de-serializing it.
  msgpack::type::tuple<uint64_t, long long, long long> src(timeStamp, totalJobCount, totalJobTime);
  // serialize the object into the buffer.
  // any classes that implements write(const char*,size_t) can be a buffer.
  stringstream buffer;
  msgpack::pack(buffer, src);
  req.result(buffer.str());
  return;
}

