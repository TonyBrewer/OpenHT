/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <stdint.h>
#include "MyInt.h"

union JpegDecMsgData {
	uint64_t	m_qw;
	uint8_t		m_b[8];
};

// message sent to horz resizer
struct JpegDecMsg {
	JpegDecMsg(my_uint2 compIdx, my_uint3 rstIdx, bool bRstLast, my_uint1 imageIdx, my_uint8 jobId, JpegDecMsgData data)
		: m_compIdx(compIdx), m_rstIdx(rstIdx), m_bRstLast(bRstLast), m_imageIdx(imageIdx), m_jobId(jobId), m_data(data) {}

	my_uint2	m_compIdx;		// component index
	my_uint3	m_rstIdx;		// restart index
	bool		m_bRstLast;		// last data for restart
	my_uint1	m_imageIdx;		// image index for double buffering
	my_uint8	m_jobId;		// host job info index (for debug)
	JpegDecMsgData m_data;		// column of eight bytes from IDCT
};
