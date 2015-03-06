/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include "JobInfo.h"

#define DV_DEC 1
#define DV_VERT 2
#define DV_HORZ 4
#define DV_ENC 8
#define DV_DEC_HORZ 16
#define DV_VERT_ENC	32

void preJobDV( JobInfo * pJobInfo, int jobDV );
bool postJobDV( JobInfo * pJobInfo, int jobDV );
