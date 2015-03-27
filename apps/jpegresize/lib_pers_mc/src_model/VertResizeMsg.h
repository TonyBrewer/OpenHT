/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include "MyInt.h"

union VertResizeMsgData {
	uint64_t	m_qw;
	uint8_t		m_b[8];
};

struct VertResizeMsg {
	VertResizeMsg(my_uint3 compIdx, my_uint11 mcuRow, my_uint2 mcuBlkRow,
		my_uint3 blkRow, bool bMcuRowFirstBlk, bool bMcuRowLastBlk,
		my_uint2 mcuBlkCol, VertResizeMsgData data )
		: m_compIdx(compIdx), m_mcuRow(mcuRow), m_mcuBlkRow(mcuBlkRow), m_blkRow(blkRow),
		m_bMcuRowFirstBlk(bMcuRowFirstBlk), m_bMcuRowLastBlk(bMcuRowLastBlk),
		m_mcuBlkCol(mcuBlkCol), m_data(data) {}

	my_uint3	m_compIdx;
	my_uint11	m_mcuRow;
	my_uint2	m_mcuBlkRow;
	my_uint3	m_blkRow;
	bool		m_bMcuRowFirstBlk;
	bool		m_bMcuRowLastBlk;
	my_uint2	m_mcuBlkCol;
	VertResizeMsgData	m_data;
};
