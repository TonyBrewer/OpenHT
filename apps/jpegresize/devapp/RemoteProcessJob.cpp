/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <string>
#include <iostream>
#include <fstream>
#include "AppArgs.h"

#ifdef ENABLE_DAEMON

#include <msgpack/rpc/server.h>
#include <msgpack/rpc/client.h>
#include <RemoteProcessJob.hpp>

namespace rpc {
        using namespace msgpack;
        using namespace msgpack::rpc;
}  // namespace rpc

using msgpack::type::raw_ref;
using namespace std;


bool RemoteProcessJob( AppJob & appJob )
{
    bool status;
    raw_ref outputBuffer;
    msgpack::rpc::future fs;
    msgpack::rpc::session s = g_sessionPool.get_session(g_pAppArgs->GetServer(), g_pAppArgs->GetPort());
    if (appJob.m_mode & MODE_INBUFF) {
      //open the input file an position the file pointer at the end of the file
      ifstream imageIn(appJob.m_inputFile, ios::binary | ios::ate);
      //file pointer is at the end of the file.  Its position will give us the file size
      appJob.m_inputBufferSize=imageIn.tellg();
      //back to the start of file for reading the contents
      imageIn.seekg(ios::beg);
      //Create a buffer to  hold the raw file contents. 
      appJob.m_inputBuffer=(uint8_t *)malloc(appJob.m_inputBufferSize * sizeof(uint8_t));
      //Again, no sanity checks.  Just  blindly try and read the data in.
      imageIn.read((char *)appJob.m_inputBuffer, appJob.m_inputBufferSize);
      imageIn.close();
    }
    raw_ref inputBuffer((const char *)appJob.m_inputBuffer, appJob.m_inputBufferSize);
    //inputBuffer.ptr=(const char *)appJob.m_inputBuffer;
    //inputBuffer.size=appJob.m_inputBufferSize;
    //If we will be getting an image buffer back then we call with a return type of raw_ref.  Otherwise we will get back a status
    //flag.
    fs=s.call("ProcessJob", appJob.m_mode, appJob.m_inputFile, inputBuffer, appJob.m_outputFile, appJob.m_width, appJob.m_height, appJob.m_scale, appJob.m_quality);
    if(appJob.m_mode & MODE_OUTBUFF) {
      outputBuffer=fs.get<raw_ref>();
      if(outputBuffer.size==0) {
	//If there was a failure in the buffer case, then the returned buffer will be empty.
	status=0;
      } else {
	ofstream imageOut;
	imageOut.open(appJob.m_outputFile, ios::binary | ios::out | ios::trunc);
	imageOut.write((char *)outputBuffer.ptr, outputBuffer.size);
	imageOut.close();    
	status=1;
      }
    } else {
      status=fs.get<bool>();
    }
    return status;
}
    
#endif //ENABLE_DAEMON  
