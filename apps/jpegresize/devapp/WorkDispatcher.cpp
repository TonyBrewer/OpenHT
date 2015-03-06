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
#include <assert.h>

#include "Ht.h"
using namespace Ht;

#include "WorkDispatcher.h"
#include "AppArgs.h"
#include "JobInfo.h"
#include "JpegHdr.h"
#include "JobDV.h"

WorkDispatcher * g_pWorkDispatcher;

bool WorkDispatcher::getWork( int threadId, AppJob & appJob )
{
	ObtainLock();

	if (!g_pAppArgs->IsLoopTest() && !g_pAppArgs->IsBenchmark()) {
		// process each job once

		if (g_pAppArgs->GetJobs() == m_jobIdx) {
			ReleaseLock();
			return false;
		}

		appJob = g_pAppArgs->GetAppJob(m_jobIdx);

		m_jobIdx += 1;
			
		ReleaseLock();
		return true;
	}

	if (g_pAppArgs->IsBenchmark()) {
		if (m_bFirst) {
			m_startTime = ht_usec();
			m_bFirst = false;
		}

		if ((int64_t)(ht_usec() - m_startTime) > g_pAppArgs->GetDuration() * 1000000LL) {
			ReleaseLock();
			return false;
		}

		appJob = g_pAppArgs->GetAppJob(m_jobIdx % g_pAppArgs->GetJobs());

		m_jobIdx += 1;
		
		ReleaseLock();
		return true;
	}
	if (g_pAppArgs->IsLoopTest()) {
		// performing the loop test
		if (m_bFirst) {
			m_tempFile = "loopOnImage.jpeg";

			m_inAppJob.m_inputFile = g_pAppArgs->GetInputFile(0);
			m_inAppJob.m_outputFile = m_tempFile;

			// find size of input image
			findImageSize( threadId, m_inAppJob.m_inputFile, m_inAppJob.m_width, m_inAppJob.m_height );

			m_outAppJob.m_inputFile = m_tempFile;
			m_outAppJob.m_outputFile = g_pAppArgs->GetOutputFile(0);
			m_outAppJob.m_width = m_inAppJob.m_width+10;
			m_outAppJob.m_height = m_inAppJob.m_height+10;

			// scale input image to new size using host routines
			resizeImage( threadId, m_inAppJob );

			m_bFirst = false;
			m_pass = 0;
		}

		// loop through on input image, reducing height and width by 1 each pass
		bool bMinHeight = m_outAppJob.m_height < (32+10);
		bool bMinWidth = m_outAppJob.m_width < (32+10);

		if (bMinHeight || bMinWidth) {
			if (++m_pass > 10) {
				if (g_pAppArgs->IsAspectRatio()) {
					// keep the same aspect ratio
					if (m_inAppJob.m_height < m_inAppJob.m_width) {
						m_inAppJob.m_height -= 7;
						m_inAppJob.m_width = m_inAppJob.m_height * m_inAppJob.m_width / (m_inAppJob.m_height + 7);
					} else {
						m_inAppJob.m_width -= 7;
						m_inAppJob.m_height = m_inAppJob.m_width * m_inAppJob.m_height / (m_inAppJob.m_width + 7);
					}
				} else {
					m_inAppJob.m_height -= 7;
					m_inAppJob.m_width -= 7;
				}

				if (m_inAppJob.m_height < 8 || m_inAppJob.m_width < 8) {
					ReleaseLock();
					return false;
				}

				resizeImage( threadId, m_inAppJob );

				m_outAppJob.m_height = m_inAppJob.m_height;
				m_outAppJob.m_width = m_inAppJob.m_width;
				m_pass = 0;
			} else {
				if (g_pAppArgs->IsAspectRatio()) {
					// keep the same aspect ratio
					if (m_inAppJob.m_height < m_inAppJob.m_width) {
						m_outAppJob.m_height = m_inAppJob.m_height-m_pass;
						m_outAppJob.m_width = m_outAppJob.m_height * m_inAppJob.m_width / m_inAppJob.m_height;
					} else {
						m_outAppJob.m_width = m_inAppJob.m_width-m_pass;
						m_outAppJob.m_height = m_outAppJob.m_width * m_inAppJob.m_height / m_inAppJob.m_width;
					}
				} else {
					m_outAppJob.m_width = m_inAppJob.m_width-m_pass;
					m_outAppJob.m_height = m_inAppJob.m_height-m_pass;
				}
			}
		} else {
			if (g_pAppArgs->IsAspectRatio()) {
				// keep the same aspect ratio
				if (m_inAppJob.m_height < m_inAppJob.m_width) {
					m_outAppJob.m_height -= 10;
					m_outAppJob.m_width = m_outAppJob.m_height * m_inAppJob.m_width / m_inAppJob.m_height;
				} else {
					m_outAppJob.m_width -= 10;
					m_outAppJob.m_height = m_outAppJob.m_width * m_inAppJob.m_height / m_inAppJob.m_width;
				}
			} else {
				m_outAppJob.m_height -= 10;
				m_outAppJob.m_width -= 10;
			}
		}
		if (m_outAppJob.m_height < 8 || m_outAppJob.m_width < 8) {
		  ReleaseLock();
		  return false;
		}

		appJob = m_outAppJob;
		appJob.m_mode = MODE_INFILE | MODE_OUTFILE;
	}

	ReleaseLock();
	return true;
}

void WorkDispatcher::findImageSize( int threadId, string &inputFile, int &width, int &height )
{
	FILE * inFp;
	if (!(inFp = fopen(inputFile.c_str(), "rb"))) {
		printf("Could not open input file '%s'\n", inputFile.c_str());
		exit(1);
	}

	JobInfo * pJobInfo = new (threadId, g_pAppArgs->IsCheck(), g_pAppArgs->IsHostMem()) JobInfo;

	// read input file to memory buffer
	struct stat buf;
	fstat( fileno(inFp), &buf );
	pJobInfo->m_inPicSize = (int)buf.st_size;

	if (fread( pJobInfo->m_pInPic, 1, pJobInfo->m_inPicSize, inFp ) != pJobInfo->m_inPicSize) {
		printf("Could not read input file\n");
		exit(1);
	}

	char const * pErrMsg = parseJpegHdr( pJobInfo, pJobInfo->m_pInPic, g_pAppArgs->IsAddRestarts());
	if (pErrMsg) {
		printf("Unable to resize image: %s\n", pErrMsg);
		exit(0);
	}

	width = pJobInfo->m_dec.m_imageCols;
	height = pJobInfo->m_dec.m_imageRows;

	fclose(inFp);

	delete pJobInfo;
}

void WorkDispatcher::resizeImage( int threadId, AppJob & appJob )
{
	// open files
	FILE * inFp;
	if (!(inFp = fopen(appJob.m_inputFile.c_str(), "rb"))) {
		printf("Could not open input file '%s'\n", appJob.m_inputFile.c_str());
		exit(1);
	}

	JobInfo * pJobInfo = new (threadId, g_pAppArgs->IsCheck(), g_pAppArgs->IsHostMem(), 1) JobInfo;

	// read input file to memory buffer
	struct stat buf;
	fstat( fileno(inFp), &buf );
	pJobInfo->m_inPicSize = (int)buf.st_size;

	if (fread( pJobInfo->m_pInPic, 1, pJobInfo->m_inPicSize, inFp ) != pJobInfo->m_inPicSize) {
		printf("Could not read input file\n");
		exit(1);
	}

	fclose( inFp );

	char const * pErrMsg = parseJpegHdr( pJobInfo, pJobInfo->m_pInPic,g_pAppArgs->IsAddRestarts());
	if (pErrMsg) {
		printf("Unable to resize image: %s\n", pErrMsg);
		exit(0);
	}

	// parseJpegHdr filled in some info, calculate derived values
	int scale = 0;
	jobInfoInit( pJobInfo, appJob.m_height, appJob.m_width, scale, g_pAppArgs->IsCheck() );

	preJobDV( pJobInfo, 0 );

	FILE * outFp;
	if (!(outFp = fopen(appJob.m_outputFile.c_str(), "wb"))) {
		printf("Could not open output file '%s'\n", appJob.m_outputFile.c_str());
		exit(1);
	}

	// write output file
	jpegWriteFile( pJobInfo, outFp );

	fclose( outFp );

	delete pJobInfo;
}
