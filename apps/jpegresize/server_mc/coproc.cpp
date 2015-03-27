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
#include "JobInfo.h"
#include "JpegResizeHif.h"
#include "JpegHdr.h"
#include "job.hpp"
#include <Magick++.h>
#include "coproc.hpp"
#include "profile.hpp"
using namespace Magick; 

using namespace Ht;

coproc::coproc(int threads) {
  CHtHifParams hifParams;
  // host memory accessed by coproc per image
  //   jobInfo, input image, output buffer
  size_t alignedJobInfoSize = (sizeof(JobInfo) + 63) & ~63;
  hifParams.m_appUnitMemSize = (alignedJobInfoSize + 3*MAX_JPEG_IMAGE_SIZE) * threads;
  resizeHif = new JpegResizeHif(&hifParams, threads);
}

coproc::~coproc() {
  delete resizeHif;
}

bool coproc::resize(jobDefinition * job) {
  PROFILE_FUNC(JobInfo * pJobInfo = new ( job->threadId, resizeHif ) JobInfo);
  //Save the job that took this slot so we can work our way back up.
  pJobInfo->job=job;
  uint8_t *pInPic = NULL;
  //struct stat buf;
  pJobInfo->m_inPicSize = job->inputBuffer.size;
  pInPic = (uint8_t *)job->inputBuffer.ptr;
  if ((job->jobMode & MODE_MAGICK)) {
    bool status=magickFallback(job);
    delete pJobInfo;
    return status;
    
  }

  pJobInfo->m_pInPic = (uint8_t *)job->inputBuffer.ptr;
  pJobInfo->m_inPicSize = job->inputBuffer.size;
  pInPic=pJobInfo->m_pInPic;
  
  assert(pInPic != NULL);
  PROFILE_FUNC(char const * pErrMsg = parseJpegHdr( pJobInfo, pInPic, appArguments.addRestarts ));
  //!!!! Note that ImageMagick fallback path is broken.  Need to figure out how to patch it in with the new
  //dataflow.  Can either let the response thread do the conversion or add a flag of some sort to bypass 
  //the jpeg container encoding in the response thread.
  if (pErrMsg) {
    bool status=true;
    printf("%s - Unsupported image format: %s\n", job->inputFile.c_str(), pErrMsg);
    //printf("Attempting to process using ImageMagick fallback\n");
    //status=magickFallback(job);
    delete pJobInfo;
    return 0;
    //return status;		
  }
  
  // check for unsupported features
  PROFILE_FUNC(pErrMsg = jobInfoCheck( pJobInfo, job->sizeX, job->sizeY, job->scale ));
  if (pErrMsg) {
    bool status=true;
    printf("%s - Unsupported image format: %s\n", job->inputFile.c_str(), pErrMsg);
    //printf("Attempting to process using ImageMagick fallback\n");
    //status=magickFallback(job);
    delete pJobInfo;
    //return status;		
    return 0;
  }
  // parseJpegHdr filled in some info, calculate derived values
  PROFILE_FUNC(jobInfoInit( pJobInfo, job->sizeY, job->sizeX, job->scale));
  PROFILE_FUNC(resizeHif->SubmitJob( pJobInfo ));
 
  //PROFILE_FUNC(job->outputBuffer.size=jpegWriteBuffer(pJobInfo, (uint8_t *&)job->outputBuffer.ptr));
  

  //delete pJobInfo;	// return pJobInfo back to jobInfo pool
  
  return 1;
}


bool coproc::magickFallback(jobDefinition *job) {
  Blob blob(job->inputBuffer.ptr, job->inputBuffer.size);
  Image image;
  image.magick("JPEG");
  try {
    image.read(blob);
  } catch(...) {
    printf("Error reading JPEG blob in ImageMagick fallback\n");
    return(0);
  }
  Geometry scaleFactor;
  if(job->jobMode & MODE_KEEP_ASPECT) {
    scaleFactor.aspect(false);
  } else {
    scaleFactor.aspect(true);
  }
  if(job->scale > 0) {
    scaleFactor.percent(true);
    scaleFactor.width(job->scale);
    scaleFactor.height(job->scale);
  } else {
    scaleFactor.percent(false);
    scaleFactor.width(job->sizeX);
    scaleFactor.height(job->sizeY);
  }
  //a simple resize
  try {
    image.resize(scaleFactor);
  } catch(...) {
    printf("Error resizing JPEG in ImageMagick fallback\n");
    return(0);    
  }
  if(job->quality) {
    image.quality(job->quality);
  }
  try {
    image.write( &blob );
  } catch(...) {
    printf("Error writing JPEG blob in ImageMagick fallback\n");  
  }
  job->outputBuffer.ptr=(char *)malloc(blob.length());
  memcpy((void *)job->outputBuffer.ptr, blob.data(), blob.length());
  job->outputBuffer.size=blob.length();
  return 1;
}

