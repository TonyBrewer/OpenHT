/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <stdint.h>

struct JobInfo;

char const * parseJpegHdr( JobInfo * pJobInfo, uint8_t *pInPic, bool addRestarts );
void jpegWriteFile( JobInfo * pJobInfo, FILE *outFp );
unsigned int jpegWriteBuffer(JobInfo * pJobInfo, uint8_t *& buffer);

