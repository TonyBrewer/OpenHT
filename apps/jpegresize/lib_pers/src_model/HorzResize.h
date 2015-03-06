/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <queue>
using namespace std;

#include "JobInfo.h"
#include "HorzResizeMsg.h"

void horzResize( JobInfo * pJobInfo, queue<JpegDecMsg> & jpegDecMsgQue, queue<HorzResizeMsg> & horzResizeMsgQue );
