/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <queue>
#include <string>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include "msgpack.h"
#include "cmdArgs.hpp"
#include "job.hpp"
#include "resize.hpp"
#include "msgpack/rpc/server.h"

namespace rpc {
        using namespace msgpack;
        using namespace msgpack::rpc;
}  // namespace rpc

using msgpack::type::raw_ref;
using namespace std;

pthread_rwlock_t queueLock;
std::vector<rpc::session_pool *> sessionPool;
std::vector<pthread_t> threads;
pthread_rwlock_t threadLock;


void * resizeThread(void *arg);
int resize(job &resizeJob, rpc::session_pool *p);

long batchResize(job &globalJobArgs) {
  queue <job> batchQueue=buildBatchQueue(globalJobArgs);
  long jobCount=batchQueue.size();
  pthread_t threads[globalJobArgs.threads];
  pthread_rwlock_init(&queueLock, 0);
  for(int i=0; i<globalJobArgs.threads; i++) {
    sessionPool.push_back(new rpc::session_pool);
  }
  for(int i=0; i<globalJobArgs.threads; i++) {
    pthread_create(&threads[i], 0, &resizeThread, (void *)&batchQueue);
  }
  for(int i=0; i<globalJobArgs.threads; i++) {
    int threadRet;
    void *threadRetPtr=(void *)&threadRet;
    pthread_join(threads[i], &threadRetPtr);
  }
  while(!sessionPool.empty()) {
    delete sessionPool.back();
    sessionPool.pop_back();
  }
  pthread_rwlock_destroy(&queueLock); 
  return(jobCount);
}

void * resizeThread(void *arg) {
  pthread_t myThreadId=pthread_self();
  queue <job> *workQueue=(queue<job> *)arg;
  
  pthread_rwlock_rdlock(&threadLock);
  std::vector<pthread_t>::iterator it = std::find(threads.begin(), threads.end(), myThreadId);
  if(it==threads.end()) {
      pthread_rwlock_unlock(&threadLock);
      pthread_rwlock_wrlock(&threadLock);
      threads.push_back(myThreadId);
      it=threads.end() - 1 ;
  }
  pthread_rwlock_unlock(&threadLock);
  int thread=std::distance(threads.begin(), it); 
  
  bool done=0;
#ifdef DEBUG
  printf("Starting thread: %u\n", thread);
#endif
  while(!done) {
    pthread_rwlock_wrlock(&queueLock);
    if(workQueue->empty()) {
      pthread_rwlock_unlock(&queueLock);
      done=1;
      continue;
    }
#ifdef DEBUG
    printf("thread %u: %lu jobs remaining\n", thread, workQueue->size());
#endif
    job jobArgs=workQueue->front();
    workQueue->pop();
    pthread_rwlock_unlock(&queueLock);
#ifdef DEBUG
    printf("thread %u:  processing job\n", thread);
#endif
//    printArgs(jobArgs);
    resize(jobArgs, sessionPool[thread]);
  }
#ifdef DEBUG
  printf("Ending thread: %u\n", thread);
#endif
}

int resize(job &resizeJob) {
  resize(resizeJob, new rpc::session_pool);
}

int resize(job &resizeJob, rpc::session_pool *p) {
  raw_ref inputBuffer;
  int status=0;
  if(!(resizeJob.jobMode & MODE_SERVER_READ)) {
    //open the input file an position the file pointer at the end of the file
    //A failure gets kicked back to the caller
    ifstream imageIn(resizeJob.inputFile, ios::binary | ios::ate);
    if(imageIn.fail()) {
      return 0;
    }
    //file pointer is at the end of the file.  Its position will give us the file size
    //A failure gets kicked back to the caller
    inputBuffer.size=imageIn.tellg();
    if(imageIn.fail()) {
      return 0;
    }
    //back to the start of file for reading the contents
    //This shouldn't fail, but if it does, we kick back to the 
    //caller.
    imageIn.seekg(ios::beg);
    if(imageIn.fail()) {
      return 0;
    }    
    //Create a buffer to  hold the raw file contents. 
    //This needs to be converted to use the slot in the pre-allocated
    //buffer for the coproc
    inputBuffer.ptr=(const char *)new uint8_t[inputBuffer.size];
    imageIn.read((char *)inputBuffer.ptr, inputBuffer.size);
    //We have a couple of failure cases here.  If there was an actual problem reading
    //the file, then we have an issue.  If we didn't read the number of bytes we 
    //were expecting then that is an error as well.
    if(imageIn.fail() || imageIn.tellg()!=inputBuffer.size) {
      delete inputBuffer.ptr;
      return 0;
    }      
    //Done with the file.
    imageIn.close();
  }    
  msgpack::rpc::session s=p->get_session(resizeJob.server, resizeJob.port);
  msgpack::rpc::future fs=s.call("resize", resizeJob.jobMode, resizeJob.inputFile, inputBuffer, resizeJob.outputFile, resizeJob.sizeX, resizeJob.sizeY, resizeJob.scale, resizeJob.quality);
  if(!(resizeJob.jobMode & MODE_SERVER_WRITE)) {
    raw_ref outputBuffer=fs.get<raw_ref>();
    if(outputBuffer.size==0) {
      //If there was a failure in the buffer case, then the returned buffer will be empty.
      status=0;
    } else {
      ofstream imageOut;
      imageOut.open(resizeJob.outputFile, ios::binary | ios::out | ios::trunc);
      imageOut.write((char *)outputBuffer.ptr, outputBuffer.size);
      imageOut.close();    
      status=1;
    }
  } else {
    status=fs.get<int>();
  }
  if(!(resizeJob.jobMode & MODE_SERVER_READ)) {
    delete inputBuffer.ptr;
  }
  return status;
}

