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
using namespace Magick; 

using namespace Ht;

extern long long g_dataMoverUsecs;
extern long long g_dataMoverCnt;
JpegResizeHif * g_pJpegResizeHif=0;

bool magickFallback(jobDefinition &job);

bool coprocInit(int threads) {
  CHtHifParams hifParams;
  // host memory accessed by coproc per image
  //   jobInfo, input image, output buffer
  size_t alignedJobInfoSize = (sizeof(JobInfo) + 63) & ~63;
  hifParams.m_appUnitMemSize = (alignedJobInfoSize + 3*MAX_JPEG_IMAGE_SIZE) * threads;
  g_pJpegResizeHif = new JpegResizeHif(&hifParams);
  JobInfo::SetThreadCnt(threads);
  JobInfo::SetHifMemBase( g_pJpegResizeHif->GetAppUnitMemBase(0) );
//klc  printf("HT frequency is %d MHz\n", g_pJpegResizeHif->GetUnitMHz());
  fflush(stdout);
  return(1);
}

bool coprocResize(jobDefinition & job) {
  JobInfo * pJobInfo = new ( job.threadId ) JobInfo;
  uint8_t *pInPic = NULL;
  //struct stat buf;
  pJobInfo->m_inPicSize = job.inputBuffer.size;
  pInPic = (uint8_t *)job.inputBuffer.ptr;
  if ((job.jobMode & MODE_MAGICK)) {
    bool status=magickFallback(job);
    delete pJobInfo;
    return status;
    
  }

  pJobInfo->m_pInPic = (uint8_t *)job.inputBuffer.ptr;
  pJobInfo->m_inPicSize = job.inputBuffer.size;
  pInPic=pJobInfo->m_pInPic;
  
  assert(pInPic != NULL);
  char const * pErrMsg = parseJpegHdr( pJobInfo, pInPic, appArguments.addRestarts );
  if (pErrMsg) {
    bool status=true;
    printf("%s - Unsupported image format: %s\n", job.inputFile.c_str(), pErrMsg);
    printf("Attempting to process using ImageMagick fallback\n");
    status=magickFallback(job);
    delete pJobInfo;
    return status;		
  }
  if(job.jobMode & MODE_KEEP_ASPECT) {
    int newX,newY;
    if(job.sizeX == 0) {
      newY = job.sizeY;
      newX = (pJobInfo->m_dec.m_imageCols * newY) / pJobInfo->m_dec.m_imageRows;
    }else if (job.sizeY == 0) {
      newX = job.sizeX;
      newY = (pJobInfo->m_dec.m_imageRows * newX) / pJobInfo->m_dec.m_imageCols;
    } else {
      // adjust target size to fit in bounding box
      // BB is defined by job.sizeX by job.sizeY
      // input filesize is pJobInfo->m_dec.m_imageCols by pJobInfo->m_dec.m_imageRows 
      // start by assuming x is controlling dimension
      // then scale y by same ratio (newX/imageCols)
      newX = job.sizeX;
      newY = (pJobInfo->m_dec.m_imageRows * newX) / pJobInfo->m_dec.m_imageCols;
      if(newY > job.sizeY) { 
        // doesn't fit, y is controlling dimension
        newY = job.sizeY;
        newX = (pJobInfo->m_dec.m_imageCols * newY) / pJobInfo->m_dec.m_imageRows;
      }
    }
    job.sizeX = newX;
    job.sizeY = newY;
  }

  if(job.jobMode & MODE_NO_UPSCALE) {
    if((pJobInfo->m_dec.m_imageRows < job.sizeY) && (pJobInfo->m_dec.m_imageCols < job.sizeX)) {
    // upscaling with MODE_NO_UPSCALE set - just return a copy of the input image
      job.outputBuffer.ptr = (char *) malloc(pJobInfo->m_inPicSize);
      memcpy((void *)job.outputBuffer.ptr,pJobInfo->m_pInPic,pJobInfo->m_inPicSize);
      job.outputBuffer.size = pJobInfo->m_inPicSize;
      delete pJobInfo;
      return 1;
    }
  }
  
  // check for unsupported features
  pErrMsg = jobInfoCheck( pJobInfo, job.sizeX, job.sizeY, job.scale );
  if (pErrMsg) {
    bool status=true;
//    printf("%s - Unsupported image format: %s\n", job.inputFile.c_str(), pErrMsg);
//    printf("Attempting to process using ImageMagick fallback\n");
    status=magickFallback(job);
    delete pJobInfo;
    return status;		
  }
  // parseJpegHdr filled in some info, calculate derived values
  jobInfoInit( pJobInfo, job.sizeY, job.sizeX, job.scale);
  g_pJpegResizeHif->SubmitJob( pJobInfo );
 
  job.outputBuffer.size=jpegWriteBuffer(pJobInfo, (uint8_t *&)job.outputBuffer.ptr);
  

  delete pJobInfo;	// return pJobInfo back to jobInfo pool
  
  return 1;
}


bool magickFallback(jobDefinition &job) {
  Blob blob(job.inputBuffer.ptr, job.inputBuffer.size);
  Image image;
  image.magick("JPEG");
  try {
    image.read(blob);
  } catch(...) {
    printf("Error reading JPEG blob in ImageMagick fallback\n");
    return(0);
  }
  Geometry scaleFactor;
  if(job.jobMode & MODE_KEEP_ASPECT) {
    scaleFactor.aspect(false);
  } else {
    scaleFactor.aspect(true);
  }
  if(job.scale > 0) {
    scaleFactor.percent(true);
    scaleFactor.width(job.scale);
    scaleFactor.height(job.scale);
  } else {
    scaleFactor.percent(false);
    scaleFactor.width(job.sizeX);
    scaleFactor.height(job.sizeY);
  }
  //a simple resize
  try {
    image.resize(scaleFactor);
  } catch(...) {
    printf("Error resizing JPEG in ImageMagick fallback\n");
    return(0);    
  }
  if(job.quality) {
    image.quality(job.quality);
  }
  try {
    image.write( &blob );
  } catch(...) {
    printf("Error writing JPEG blob in ImageMagick fallback\n");  
  }
  job.outputBuffer.ptr=(char *)malloc(blob.length());
  memcpy((void *)job.outputBuffer.ptr, blob.data(), blob.length());
  job.outputBuffer.size=blob.length();
  return 1;
}

