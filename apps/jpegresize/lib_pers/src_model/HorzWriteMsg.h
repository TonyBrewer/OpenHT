/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include "MyInt.h"

struct HorzWriteMsg {
	HorzWriteMsg(my_uint2 compIdx, my_uint11 mcuRow, my_uint2 mcuBlkRow, my_uint11 mcuCol, my_uint2 mcuBlkCol,
		my_uint10 outMemLine, bool bBlkRowComplete, my_uint2 bufIdx)
		: m_compIdx(compIdx), m_mcuRow(mcuRow), m_mcuBlkRow(mcuBlkRow),
		m_mcuCol(mcuCol), m_mcuBlkCol(mcuBlkCol), m_outMemLine(outMemLine), m_bBlkRowComplete(bBlkRowComplete), m_bufIdx(bufIdx) {}

	my_uint2 m_compIdx;
	my_uint11 m_mcuRow;
	my_uint2 m_mcuBlkRow;
	my_uint11 m_mcuCol;
	my_uint2 m_mcuBlkCol;
	my_uint10 m_outMemLine;
	bool m_bBlkRowComplete;
	my_uint2 m_bufIdx;
};
