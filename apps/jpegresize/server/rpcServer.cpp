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

#include "rpcServer.hpp"
#include "job.hpp"
#include "cmdArgs.hpp"
#include "coproc.hpp"

using namespace std;
using msgpack::type::raw_ref;

bool processJob(jobDefinition &job) { return 1; }

//main RPC call handler.  This is called each time a remote client exectutes
//a client.call().
void rpcServer::dispatch(request req) {
  string method;
  try {
    //pull the method we are trying to call out of the request
    req.method().convert(&method);
    //All we know about right now is "resize"
    if(method == "resize") {
      //pass the request on to the handler for the resize method
      resize(req);      
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

void rpcServer::stats(request req) {
  uint64_t timeStamp;
  long long jobCount;
  long long jobTime;
  g_pJpegResizeHif->jobStats(timeStamp, jobCount, jobTime);
  
  //This extra contortion is necessary because msgpack-rpc only allows a single return value,
  //similar to a normal C function.  We serialize the four values into a single string and return
  //that.  The receiver will be responsible for de-serializing it.
  msgpack::type::tuple<uint64_t, long long, long long> src(timeStamp, jobCount, jobTime);
  // serialize the object into the buffer.
  // any classes that implements write(const char*,size_t) can be a buffer.
  stringstream buffer;
  msgpack::pack(buffer, src);
  req.result(buffer.str());
  return;
}

void rpcServer::resize(request req) {
  jobDefinition job;
  try {
    //We define the parameters we expect from the call
    msgpack::type::tuple<uint32_t, string, raw_ref, string, int, int, int, int> req_params;
    //an convert the incoming call.  A parameter mismatch will throw and exception in this
    //section of code, in which case we are going to return an ARGUMENT_ERROR;
    req.params().convert(&req_params);
    job.jobMode=req_params.get<0>();
    job.inputFile=req_params.get<1>();
    job.inputBuffer=req_params.get<2>();
    job.outputFile=req_params.get<3>();
    job.sizeX=req_params.get<4>();
    job.sizeY=req_params.get<5>();
    job.scale=req_params.get<6>();
    job.quality=req_params.get<7>();
    job.outputBuffer.ptr=0;
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
  //We keep a vector of all threads that have processed a call.  If we don't find the
  //current thread id then this is the first time this thread has procecessed a call 
  //and it gets added to the vector, effectively assigning it the next process index.  
  pthread_t myThreadId=pthread_self();
  //This vector is shared by all threads and so must be controlled by a lock to make
  //sure it is thread safe.  Start with a read lock while we search for ourselves.
  pthread_rwlock_rdlock(&threadLock);
  std::vector<pthread_t>::iterator it = std::find(threads.begin(), threads.end(), myThreadId);
  if(it==threads.end()) {
    //Since we didn't find the current thread, switch to a write like before we update
    //the vector contents
    pthread_rwlock_unlock(&threadLock);
    pthread_rwlock_wrlock(&threadLock);
    threads.push_back(myThreadId);
    it=threads.end() - 1 ;
  }
  pthread_rwlock_unlock(&threadLock);
  job.threadId=std::distance(threads.begin(), it);
  
  if(job.jobMode & MODE_SERVER_READ) {
    if(appArguments.noRead) {
      req.error((int)jobStatusReadNotAllowed);
      return;
    }
    //open the input file an position the file pointer at the end of the file
    //A failure gets kicked back to the caller
    ifstream imageIn(job.inputFile, ios::binary | ios::ate);
    if(imageIn.fail()) {
      req.error((int)jobStatusUnableToOpenInput);
      return;
    }
    //file pointer is at the end of the file.  Its position will give us the file size
    //A failure gets kicked back to the caller
    job.inputBuffer.size=imageIn.tellg();
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
    job.inputBuffer.ptr=(const char *)new uint8_t[job.inputBuffer.size];
    imageIn.read((char *)job.inputBuffer.ptr, job.inputBuffer.size);
    //We have a couple of failure cases here.  If there was an actual problem reading
    //the file, then we have an issue.  If we didn't read the number of bytes we 
    //were expecting then that is an error as well.
    if(imageIn.fail() || imageIn.tellg()!=job.inputBuffer.size) {
      delete job.inputBuffer.ptr;
      req.error((int)jobStatusErrorReadingInput);
      imageIn.close();
      return;
    }      
    //Done with the file.
    imageIn.close();
  }
  
  if(coprocResize(job)) {
    //resize was successfull.  We either return the resulting image buffer or a 
    //success status
    if(job.jobMode & MODE_SERVER_WRITE) {
      ofstream imageOut(job.outputFile, ios::binary | ios::trunc);
      if(imageOut.fail()) {
	req.error((int)jobStatusUnableToOpenOutput);
	free((void *)job.outputBuffer.ptr);
	return;
      }
      imageOut.write(job.outputBuffer.ptr, job.outputBuffer.size);
      if(imageOut.fail()) {
	req.error((int)jobStatusErrorWritingOutput);
	imageOut.close();
	free((void *)job.outputBuffer.ptr);
	return;
      }
      imageOut.close();
      free((void *)job.outputBuffer.ptr);
      req.result((int)job.status);        
    } else {
      req.result(job.outputBuffer);  
      free((void *)job.outputBuffer.ptr);
    }
  } else {
    //resize was not successful.  The job.status value will indicate what the
    //failure was.
    req.error((int)job.status);
    if(job.outputBuffer.ptr!=0) {
      free((void *)job.outputBuffer.ptr);
    }
  }
  if(job.jobMode & MODE_SERVER_READ) {
    //Done with the input buffer
    delete job.inputBuffer.ptr;
  }
}	

