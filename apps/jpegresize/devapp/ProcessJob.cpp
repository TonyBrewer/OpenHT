/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Ht.h"
using namespace Ht;

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

#include "ProcessJob.h"
#include "JpegHdr.h"
#include "JobInfo.h"
#include "JobDV.h"
#include "JpegResizeHif.h"
#include "AppArgs.h"
#ifdef MAGICK_FALLBACK
#include <Magick++.h>
using namespace Magick; 
#endif

extern volatile bool g_forceErrorExit;
extern long long g_dataMoverUsecs;
extern long long g_dataMoverCnt;

bool processJob( int threadId, AppJob & appJob )
{
	JobInfo * pJobInfo = new (threadId, g_pAppArgs->IsCheck(), g_pAppArgs->IsHostMem(), 1) JobInfo;

	if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, "Processing Job 0x%llx\n", (long long)pJobInfo);
	if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, "  Reading file ...");

	uint8_t *pInPic = NULL;
	struct stat buf;
	if (appJob.m_mode & MODE_INFILE) {
		// open files
		FILE * inFp;
		if (!(inFp = fopen(appJob.m_inputFile.c_str(), "rb"))) {
			printf("Could not open input file '%s'\n", appJob.m_inputFile.c_str());
			exit(1);
		}
		// read input file to memory buffer
		fstat( fileno(inFp), &buf );
		pJobInfo->m_inPicSize = (int)buf.st_size;
		if (fread( pJobInfo->m_pInPic, 1, pJobInfo->m_inPicSize, inFp ) != pJobInfo->m_inPicSize) {
			printf("Could not read input file\n");
			exit(1);
		}
		pInPic = pJobInfo->m_pInPic;

		fclose(inFp);
	} else {
		pJobInfo->m_inPicSize = appJob.m_inputBufferSize;
		pInPic = appJob.m_inputBuffer;
	}

	if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, " done\n");
#ifdef MAGICK_FALLBACK
	if ((appJob.m_mode & MODE_MAGICK) || g_pAppArgs->IsMagickMode()) {
	    bool status=magickFallback((const void *)pInPic, pJobInfo->m_inPicSize, appJob);
	    delete pJobInfo;
	    return status;
	    
	}
#endif
	if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, "  Parsing image ...");

	assert(pInPic != NULL);
	char const * pErrMsg = parseJpegHdr( pJobInfo, pInPic,g_pAppArgs->IsAddRestarts());
	if (pErrMsg) {
	      bool status=true;
	      printf("%s - Unsupported image format: %s\n", appJob.m_inputFile.c_str(), pErrMsg);
#ifdef MAGICK_FALLBACK
	      printf("Attempting to process using ImageMagick fallback\n");
	      status=magickFallback((const void *)pInPic, pJobInfo->m_inPicSize, appJob);
#endif
	      delete pJobInfo;
	      return status;		
	}

	// check for unsupported features
	pErrMsg = jobInfoCheck( pJobInfo, appJob.m_width, appJob.m_height, appJob.m_scale);
	if (pErrMsg) {
	      bool status=true;
	      printf("%s - Unsupported image format: %s\n", appJob.m_inputFile.c_str(), pErrMsg);
#ifdef MAGICK_FALLBACK
	      printf("Attempting to process using ImageMagick fallback\n");
	      status=magickFallback((const void *)pInPic, pJobInfo->m_inPicSize, appJob);
#endif
	      delete pJobInfo;
	      return status;		
	}

	if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, " done\n");
	if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, "  JobInfo init ...");

	// parseJpegHdr filled in some info, calculate derived values
	jobInfoInit( pJobInfo, appJob.m_height, appJob.m_width, appJob.m_scale, g_pAppArgs->IsCheck() );

	if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, " done\n");

	int jobDV = 0;
#if defined(DHIMG_FIXTURE)
	jobDV = DV_HORZ;
#elif defined(VEIMG_FIXTURE)
	jobDV = DV_VERT | DV_ENC;
#elif defined(VERT_FIXTURE)
	jobDV = DV_VERT;
#elif defined(HORZ_FIXTURE)
	jobDV = DV_HORZ;
#elif defined(HT_MODEL)
	jobDV = DV_DEC | DV_HORZ | DV_VERT | DV_ENC;
#else
	jobDV = DV_DEC_HORZ | DV_VERT_ENC;
#endif

	if (jobDV != 0 && g_pAppArgs->IsCheck()) {
		if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, "  preJobDV ...");
		preJobDV( pJobInfo, jobDV );
		if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, " done\n");
	}

	if (g_pAppArgs->IsCheck()) {
		printf( "Input file size     : %d\n", pJobInfo->m_inPicSize );
		printf( "Input image size    : %dw x %dh\n", (int)pJobInfo->m_dec.m_imageCols, (int)pJobInfo->m_dec.m_imageRows );
		if (appJob.m_scale > 0)
			printf( "Scale               : %d%%\n", appJob.m_scale);
		printf( "Output image size   : %dw x %dh\n", (int)pJobInfo->m_enc.m_imageCols, (int)pJobInfo->m_enc.m_imageRows );
	}

	if (!g_pAppArgs->IsHostOnly()) {
		if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, "  submitting job ...");
		g_pJpegResizeHif->SubmitJob( pJobInfo );
		if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, " finished\n");
	}

	//Does this case need to be trapped with ImageMagick fallback code?
	//I don't think so as I think this is supposed to be a global shutdown flag
	if (g_forceErrorExit)
		return true;

	//If we are running locally and need to write a file out, then do so.
	//If we are running as a daemon and are supposed to write out a file, do so.

	if ((!g_pAppArgs->IsNoWrite() && !g_pAppArgs->IsDaemon()) || (g_pAppArgs->IsDaemon() && (appJob.m_mode & MODE_OUTFILE))) {
		if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, "  writing output ...");

		FILE * outFp;
		if (!(outFp = fopen(appJob.m_outputFile.c_str(), "wb"))) {
			printf("Could not open output file '%s'\n", appJob.m_outputFile.c_str());
			exit(1);
		}

		// write output file
		jpegWriteFile( pJobInfo, outFp );

		if (g_pAppArgs->IsCheck()) {
			fflush(outFp);
			fstat( fileno(outFp), &buf );
			int outPicSize = (int)buf.st_size;

			printf( "Output file size    : %d\n", outPicSize );
			printf("\n");
		}

		fclose( outFp );
		if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, " done\n");

		//Just becasuse we aren't writing a file out, doesn't mean we are sending back a buffer.  If we are benchmarking
		//or something like that then we may just want to resize the image and drop the results on the floor.
	} else if (g_pAppArgs->IsDaemon() && (appJob.m_mode & MODE_OUTBUFF)) {    
		appJob.m_outputBufferSize=jpegWriteBuffer(pJobInfo, appJob.m_outputBuffer);
	}

	bool bPass = !g_pAppArgs->IsCheck() || postJobDV( pJobInfo, jobDV );

	delete pJobInfo;	// return pJobInfo back to jobInfo pool

	if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, "  done 0x%llx\n", (long long)pJobInfo);

	return bPass;
}

#ifdef MAGICK_FALLBACK
bool magickFallback(const void *jpegImage, int jpegImageSize, AppJob &appJob) {
  Blob blob(jpegImage, jpegImageSize);
  Image image;
  image.magick("JPEG");
  try {
    image.read(blob);
  } catch(...) {
    printf("Error reading JPEG blob in ImageMagick fallback\n");
    return(0);
  }
  Geometry scaleFactor;
  // Note: ImageMagic uses the reverse sense of 'aspect' from us. For IM,
  // a setting of 'true' means 'resize WITHOUT preserving aspect ratio'.
  if(appJob.m_mode & MODE_KEEPASPECT) {
        scaleFactor.aspect(false);
  } else {
         scaleFactor.aspect(true);
  }
  if(appJob.m_scale > 0) {
    scaleFactor.percent(true);
    scaleFactor.width(appJob.m_scale);
    scaleFactor.height(appJob.m_scale);
  } else {
    //Since we are using absolute sizes, turn off aspect ration preservation
    scaleFactor.percent(false);
    scaleFactor.width(appJob.m_width);
    scaleFactor.height(appJob.m_height);
  }
  //a simple resize
  try {
    image.resize(scaleFactor);
  } catch(...) {
    printf("Error resizing JPEG in ImageMagick fallback\n");
    return(0);    
  }
  if(appJob.m_quality) {
    image.quality(appJob.m_quality);
  }
  try {
    image.write( &blob );
  } catch(...) {
      printf("Error writing JPEG blob in ImageMagick fallback\n");  
  }
  if ((!g_pAppArgs->IsNoWrite() && !g_pAppArgs->IsDaemon()) || (g_pAppArgs->IsDaemon() && (appJob.m_mode & MODE_OUTFILE))) {
    if (g_pAppArgs->IsDebugInfo()) fprintf(stderr, "  writing output ...");
    
    FILE * outFp;
    if (!(outFp = fopen(appJob.m_outputFile.c_str(), "wb"))) {
      exit(1);
    }
    fwrite((char *)blob.data(), blob.length(), 1, outFp);
    fclose( outFp );
  } else {
    appJob.m_outputBuffer=(uint8_t *)malloc(blob.length());
    memcpy(appJob.m_outputBuffer, blob.data(), blob.length());
    appJob.m_outputBufferSize=blob.length();
  }
  return 1;
}
#endif

