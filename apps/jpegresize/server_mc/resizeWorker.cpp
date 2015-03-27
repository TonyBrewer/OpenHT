/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <iostream>
#include <fstream>
#include <signal.h>

#include "rpcServer.hpp"
#include "job.hpp"
#include "cmdArgs.hpp"
#include "coproc.hpp"
#include "profile.hpp"
#include "JpegHdr.h"
#include "JobInfo.h"

resizeWorker::resizeWorker(resizeDispatcher * disp, int threadCount, int unit, int jobSlots, int responders) {
  id=unit;
  //pthread_rwlock_init(&threadLock, 0);
  dispatcher=disp;
  coproc=new class coproc(threadCount);
  while(threadCount) {
    pthread_t tid;
    pthread_create(&tid, NULL, &startResizeReqWorker, (void *)this);
    //pthread_rwlock_wrlock(&threadLock);
    reqThreads.push_back(tid);
    //pthread_rwlock_unlock(&threadLock);
    threadCount--;
  }
  while(responders) {
    pthread_t tid;
    pthread_create(&tid, NULL, &startResizeRespWorker, (void *)this);
    //pthread_rwlock_wrlock(&threadLock);
    respThreads.push_back(tid);
    //pthread_rwlock_unlock(&threadLock);
    responders--;
  }    
}

extern "C" void * startResizeReqWorker(void * ptr) {
  ((resizeWorker *)ptr)->reqWorker();
  return (void *)0;
}

extern "C" void * startResizeRespWorker(void * ptr) {
  ((resizeWorker *)ptr)->respWorker();
   return (void *)0;
}


void resizeWorker::reqWorker(void) {
  while(1) {
    PROFILE_FUNC(rpc::request req=dispatcher->resizeReqDequeue());
    PROFILE_FUNC(resize(req));
  }
}

resizeWorker::~resizeWorker() {
  //pthread_rwlock_wrlock(&threadLock);  
  while(!reqThreads.empty()) {
    pthread_kill(reqThreads.front(), SIGKILL);
    pthread_join(reqThreads.front(), 0);
    reqThreads.erase(reqThreads.begin());
  }
  while(!respThreads.empty()) {
    pthread_kill(respThreads.front(), SIGKILL);
    pthread_join(respThreads.front(), 0);
    respThreads.erase(respThreads.begin());
  }
  
  //pthread_rwlock_unlock(&threadLock);
  //pthread_rwlock_destroy(&threadLock);
}


void resizeWorker::resize(request req) {
  jobDefinition * job=new jobDefinition(req);
  //job->req=req;
  bool ret;
  //printf("unit %d handling request\n", id);
  try {
    //We define the parameters we expect from the call
    msgpack::type::tuple<uint32_t, string, raw_ref, string, int, int, int, int> req_params;
    //an convert the incoming call.  A parameter mismatch will throw and exception in this
    //section of code, in which case we are going to return an ARGUMENT_ERROR;
    req.params().convert(&req_params);
    job->jobMode=req_params.get<0>();
    job->inputFile=req_params.get<1>();
    job->inputBuffer=req_params.get<2>();
    job->outputFile=req_params.get<3>();
    job->sizeX=req_params.get<4>();
    job->sizeY=req_params.get<5>();
    job->scale=req_params.get<6>();
    job->quality=req_params.get<7>();
    job->outputBuffer.ptr=0;
  //If we blew up trying to get arguments from the call then return an ARGUMENT_ERROR
  //to the caller.
  } catch (msgpack::type_error& e) {
    req.error(msgpack::rpc::ARGUMENT_ERROR);
    return;
  //If we get some uknown error then we will return the string describing
  //the error back to the caller.
  } catch (exception& e) {
    req.error(string(e.what()));
    return;
  }
  //find our "process index" this gets used later to index into a pre-allocated memory
  //buffer that ensures proper alignment of data for sending to the coprocessor
  pthread_t myThreadId=pthread_self();
  //This vector is shared by all threads and so must be controlled by a lock to make
  //sure it is thread safe.  Start with a read lock while we search for ourselves.
  //pthread_rwlock_rdlock(&threadLock);
  std::vector<pthread_t>::iterator it = std::find(reqThreads.begin(), reqThreads.end(), myThreadId);
  if(it==reqThreads.end()) {
    printf("Thread not found in thread list.  This should not happen!\n");
    exit(0);
  }
  //pthread_rwlock_unlock(&threadLock);
  job->threadId=std::distance(reqThreads.begin(), it);
  
  if(job->jobMode & MODE_SERVER_READ) {
    if(appArguments.noRead) {
      req.error((int)jobStatusReadNotAllowed);
      return;
    }
    //open the input file an position the file pointer at the end of the file
    //A failure gets kicked back to the caller
    ifstream imageIn(job->inputFile, ios::binary | ios::ate);
    if(imageIn.fail()) {
      req.error((int)jobStatusUnableToOpenInput);
      return;
    }
    //file pointer is at the end of the file.  Its position will give us the file size
    //A failure gets kicked back to the caller
    job->inputBuffer.size=imageIn.tellg();
    if(imageIn.fail()) {
      req.error((int)jobStatusErrorReadingInput);
      imageIn.close();
      return;
    }
    //back to the start of file for reading the contents
    //This shouldn't fail, but if it does, we kick back to the 
    //caller.
    imageIn.seekg(ios::beg);
    if(imageIn.fail()) {
      req.error((int)jobStatusErrorReadingInput);
      imageIn.close();
      return;
    }    
    //Create a buffer to  hold the raw file contents. 
    //This needs to be converted to use the slot in the pre-allocated
    //buffer for the coproc
    job->inputBuffer.ptr=(const char *)new uint8_t[job->inputBuffer.size];
    imageIn.read((char *)job->inputBuffer.ptr, job->inputBuffer.size);
    //We have a couple of failure cases here.  If there was an actual problem reading
    //the file, then we have an issue.  If we didn't read the number of bytes we 
    //were expecting then that is an error as well.
    if(imageIn.fail() || imageIn.tellg()!=job->inputBuffer.size) {
      delete job->inputBuffer.ptr;
      req.error((int)jobStatusErrorReadingInput);
      imageIn.close();
      return;
    }      
    //Done with the file.
    imageIn.close();
  }
  //Save the RPC request that spawned this job so we can respond to it later.
  //job->req=&req;
  PROFILE_FUNC(ret=coproc->resize(job));
  if(ret) {
    /*
  //if(coproc->resize(job)) {
    //resize was successfull.  We either return the resulting image buffer or a 
    //success status
    if(job->jobMode & MODE_SERVER_WRITE) {
      ofstream imageOut(job->outputFile, ios::binary | ios::trunc);
      if(imageOut.fail()) {
	req.error((int)jobStatusUnableToOpenOutput);
	free((void *)job->outputBuffer.ptr);
	return;
      }
      imageOut.write(job->outputBuffer.ptr, job->outputBuffer.size);
      if(imageOut.fail()) {
	req.error((int)jobStatusErrorWritingOutput);
	imageOut.close();
	free((void *)job->outputBuffer.ptr);
	return;
      }
      imageOut.close();
      free((void *)job->outputBuffer.ptr);
      req.result((int)job->status);        
    } else {
      req.result(job->outputBuffer);  
      free((void *)job->outputBuffer.ptr);
    }
    */
  } else {
    //resize was not successful.  The job->status value will indicate what the
    //failure was.
    req.error((int)job->status);
    if(job->outputBuffer.ptr!=0) {
      free((void *)job->outputBuffer.ptr);
    }
    if((job->jobMode & MODE_SERVER_READ) && job->inputBuffer.ptr) {
    //Done with the input buffer
    delete job->inputBuffer.ptr;
  }
  }
  /*
  if(job->jobMode & MODE_SERVER_READ) {
    //Done with the input buffer
    delete job->inputBuffer.ptr;
  }
  */
}	

void resizeWorker::respWorker(void) {
  JobInfo *pJobInfo;
  jobDefinition *job;
  while(1) {
    //get jobInfo here
    pJobInfo=coproc->outputDequeue();
    job=pJobInfo->job;
    job->outputBuffer.size=jpegWriteBuffer(pJobInfo, (uint8_t *&)job->outputBuffer.ptr);
    //return the job slot back to the pool as soon as possible.
    delete pJobInfo;
    if(job->jobMode & MODE_SERVER_WRITE) {
      ofstream imageOut(job->outputFile, ios::binary | ios::trunc);
      if(imageOut.fail()) {
	job->m_req.error((int)jobStatusUnableToOpenOutput);
	free((void *)job->outputBuffer.ptr);
	return;
      }
      imageOut.write(job->outputBuffer.ptr, job->outputBuffer.size);
      if(imageOut.fail()) {
	job->m_req.error((int)jobStatusErrorWritingOutput);
	imageOut.close();
	free((void *)job->outputBuffer.ptr);
	return;
      }
      imageOut.close();
      free((void *)job->outputBuffer.ptr);
      job->m_req.result((int)job->status);        
    } else {
      job->m_req.result(job->outputBuffer);  
      free((void *)job->outputBuffer.ptr);
    }
    if(job->jobMode & MODE_SERVER_READ) {
      //Done with the input buffer
      delete job->inputBuffer.ptr;
    }
    //delete job->req;
    delete job;
  }
}
