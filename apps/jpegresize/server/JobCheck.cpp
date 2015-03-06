/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "JobInfo.h"
#include "job.hpp"

char const * checkJobInfo( JobInfo * pJobInfo, jobDefinition & job )
{
	if (job.scale != 0 && job.scale > 100 ||
			job.scale == 0 && (job.sizeY > pJobInfo->m_dec.m_imageRows ||
					job.sizeX > pJobInfo->m_dec.m_imageCols))
		return "attempt to scale up";

	if (pJobInfo->m_dec.m_compCnt == 2)
		return "jpeg component count equal 2";


	// check if scaling too small
	if (job.scale != 0 && (((float)job.scale * (float)pJobInfo->m_dec.m_imageRows / 100.0 < 8.0) ||
			((float)job.scale * (float)pJobInfo->m_dec.m_imageCols / 100.0 < 16.0)) ||
			job.scale == 0 && job.sizeY < 8 ||
			job.scale == 0 && job.sizeX < 16) {
		return "scaling to less than 16x8 pixels";
	}

	return 0;
}
