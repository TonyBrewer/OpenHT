/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <assert.h>
#include <string.h>
#include "MyInt.h"

#include "JpegCommon.h"
#include "JpegDecMsg.h"
#include "HorzResize.h"
#include "HorzWriteMsg.h"
#include "HorzResizeMsg.h"

#define MAX_JPEG_DEC_RST_CNT		8
#define MAX_HORZ_MCU_ROWS			8
#define MAX_HRS						(MAX_MCU_COMPONENTS * MAX_HORZ_MCU_ROWS * MAX_MCU_BLK_ROWS)
#define RSLT_BUFFERS				4

typedef uint8_t HorzRsltBuf_t [MAX_HRS][RSLT_BUFFERS][64];

void horzResizeDecMsg( JobInfo * pJobInfo, queue<JpegDecMsg> & jpegDecMsgQue, queue<HorzWriteMsg> & horzWriteMsgQue, HorzRsltBuf_t &horzRsltBuf );
bool horzResizeWriteRslt( JobInfo * pJobInfo, queue<HorzWriteMsg> & horzWriteMsgQue, HorzRsltBuf_t &horzRsltBuf, queue<HorzResizeMsg> & horzResizeMsgQue );

static inline int descale(int64_t x, int n) {
	return ( (int)x + (1 << (n-1))) >> n; 
}

static inline uint8_t clampPixel(int inInt)
{
	if (inInt < 0)
		return 0;
	if (inInt > 255)
		return 255;
	return (uint8_t) inInt;
}

void horzResize( JobInfo * pJobInfo, queue<JpegDecMsg> & jpegDecMsgQue, queue<HorzResizeMsg> & horzResizeMsgQue )
{
	// Horizontal resizing
	//  the decoder works on up to 8 MCU rows at a time, sending completed MCU blocks
	//  to horizontal resizer as they are finished. 

	queue<HorzWriteMsg> horzWriteMsgQue;
	HorzRsltBuf_t horzRsltBuf;	// horz resize results buffer

	bool bHorzResizeComplete;
	do {

		horzResizeDecMsg( pJobInfo, jpegDecMsgQue, horzWriteMsgQue, horzRsltBuf );

		bHorzResizeComplete = horzResizeWriteRslt( pJobInfo, horzWriteMsgQue, horzRsltBuf, horzResizeMsgQue );

	} while (!bHorzResizeComplete);
}

struct HorzDecState {
	bool		m_bStartOfBlkRow;
	bool		m_bEndOfMcuRow;
	my_uint11	m_mcuRow;	// mcu row within the image
	my_uint11	m_mcuCol;	// mcu column within the image
	my_uint2	m_compIdx;	// compIdx within mcu
	my_uint2	m_mcuBlkRow;	// row block within the mcu
	my_uint1	m_mcuBlkCol;	// column block within the mcu
	my_uint3	m_blkCol;	// column within the block
};

void advanceHorzDecPos( JobHorz & horz, JobHcp & hcp, HorzDecState & hds );

struct HorzResizeState {
	bool m_bUpScale;
	my_int14 m_pntWghtStart;
	my_uint13 m_pntWghtIdx;
	my_int14 m_colDataPos;
	my_uint14 m_inCol;
	my_uint14 m_outCol;
	my_uint10 m_outMemLine;
	bool m_bBlkRowCompleted;
};

void horzResizeDecMsg( JobInfo * pJobInfo, queue<JpegDecMsg> & jpegDecMsgQue, 
	queue<HorzWriteMsg> & horzWriteMsgQue, HorzRsltBuf_t &horzRsltBuf )
{
	// One quadword of data is received per input message.
	//   Each message is tagged with mcuRow, ci, blkRow, blkCol, and col.
	//   This routine has buffer space for upto 8 mcuRows, 3 componenets per mcuRow,
	//   and two blkRows per mcuRow. Each block row has sufficient buffering to
	//   minimize backpressure due to running out of buffer space.
	//
	// The output of the horizontal scaler is written to memory. A message is sent
	//   to the vertical scaler when each mcu row is complete. The vertical
	//   scaler uses the message to know when it can process a set of MCU rows from
	//   memory.

	// Horz resize state
	static HorzDecState horzDecState[MAX_JPEG_DEC_RST_CNT];

	static bool bReset = true;
	if (bReset) {
		bReset = false;

		for (int i = 0; i < MAX_JPEG_DEC_RST_CNT; i += 1) {
			HorzDecState & hds = horzDecState[i];

			hds.m_mcuRow = i;
			hds.m_mcuCol = 0;
			hds.m_compIdx = 0;
			hds.m_mcuBlkRow = 0;
			hds.m_mcuBlkCol = 0;
			hds.m_blkCol = 0;
			hds.m_bStartOfBlkRow = true;
			hds.m_bEndOfMcuRow = false;
		}
	}

	static HorzResizeState horzResizeState[MAX_MCU_COMPONENTS * MAX_HORZ_MCU_ROWS * 2];	// each mcu row can be two blocks tall
	static bool bRowFirstProcessed = false;

	static uint8_t horzResizeBuf[MAX_HRS][MAX_HORZ_MCU_ROWS][COPROC_PNT_WGHT_CNT];	// buffering for pixel weight calculation

	// First store the decoder message to the input queue. If queue has more than 
	//   a few entries then tell decoder to stop sending.

	// process the message and the head of the queue
	if (jpegDecMsgQue.empty())
		return;

	JpegDecMsg & jdm = jpegDecMsgQue.front();

	JobHorz & horz = pJobInfo->m_horz;
	JobHcp & hcp = horz.m_hcp[jdm.m_compIdx];

	HorzDecState & hds = horzDecState[jdm.m_rstIdx & 0x7];

	assert(jdm.m_compIdx == hds.m_compIdx);

	my_uint6 hrsIdx = (jdm.m_compIdx << 4) | ((hds.m_mcuRow & 7) << 1) | hds.m_mcuBlkRow;
	HorzResizeState & hrs = horzResizeState[hrsIdx];

	if (hds.m_bStartOfBlkRow && !bRowFirstProcessed) {
		// Take a clock and initialize state
		hrs.m_bUpScale = horz.m_maxBlkColsPerMcu == 2 && hcp.m_blkColsPerMcu == 1;
		hrs.m_pntWghtStart = horz.m_pntInfo[0].m_pntWghtStart;
		hrs.m_pntWghtIdx = horz.m_pntInfo[0].m_pntWghtIdx;

		if (hrs.m_bUpScale) {
			my_int6 filterWidth = (horz.m_filterWidth >> 1) + 1;
			my_int6 filterOffset = (18 - (filterWidth << 1));
			hrs.m_colDataPos = hrs.m_pntWghtStart < -filterOffset ? -18 : ((hrs.m_pntWghtStart & ~1) - (filterWidth << 1));
		} else {
			my_int6 filterOffset = (17 - horz.m_filterWidth);
			hrs.m_colDataPos = hrs.m_pntWghtStart < -filterOffset ? -17 : (hrs.m_pntWghtStart - horz.m_filterWidth);
		}

		hrs.m_inCol = 0;
		hrs.m_outCol = 0;
		hrs.m_outMemLine = 0;
		hrs.m_bBlkRowCompleted = false;

		// don't pop the message from the queue, we will process it next clock
		bRowFirstProcessed = true;
		return;
	}

	bRowFirstProcessed = false;

	my_uint1 even = ~hrs.m_pntWghtStart & 1;
	JobPntWght &pntWght = horz.m_pntWghtList[hrs.m_pntWghtIdx];
	int rsltBufIdx = (hrs.m_outCol >> 6) & 3;

	// check if we have the data to produce an output pixel
	bool bGenOutPixel = !hrs.m_bBlkRowCompleted && (hrs.m_pntWghtStart == hrs.m_colDataPos ||
		(hrs.m_bUpScale && (hrs.m_pntWghtStart >> 1) == (hrs.m_colDataPos >> 1)));

	for (my_uint4 r = 0; r < DCTSIZE; r += 1) {
		my_int21 pixel = 0;
		for (my_uint5 c = 0; c < COPROC_PNT_WGHT_CNT; c += 1) {
			uint8_t data = hrs.m_bUpScale ? horzResizeBuf[hrsIdx][r][(even ? 7 : 8)+(c>>1)+(even&c)] : horzResizeBuf[hrsIdx][r][c];
			pixel += data * pntWght.m_w[c];
		}

		uint8_t clampedPixel = clampPixel(descale(pixel, COPROC_PNT_WGHT_FRAC_W));

		if (bGenOutPixel)
			horzRsltBuf[hrsIdx][rsltBufIdx][((hrs.m_outCol & 0x7) << 3) | r] = clampedPixel;
	}

	if (bGenOutPixel)
		hrs.m_outCol += 1;

	hrs.m_bBlkRowCompleted = hrs.m_outCol == hcp.m_outCompCols;

	if (bGenOutPixel && ((hrs.m_outCol & 0x7) == 0 || hrs.m_bBlkRowCompleted)) {
		// push info to write 64B to memory
		my_uint2 outMcuBlkCol = (uint64_t)((hrs.m_outCol-1ull) / DCTSIZE % hcp.m_blkColsPerMcu);
		my_uint11 outMcuCol = (uint64_t)((hrs.m_outCol-1ull) / DCTSIZE / hcp.m_blkColsPerMcu);

		horzWriteMsgQue.push( HorzWriteMsg( jdm.m_compIdx, hds.m_mcuRow, hds.m_mcuBlkRow, 
			outMcuCol, outMcuBlkCol, hrs.m_outMemLine, hrs.m_bBlkRowCompleted, rsltBufIdx ) );

		hrs.m_outMemLine += 1;
	}

	my_uint14 outColScaled = hrs.m_outCol << (hrs.m_bUpScale ? 1 : 0);
	hrs.m_pntWghtStart = horz.m_pntInfo[outColScaled].m_pntWghtStart;
	hrs.m_pntWghtIdx = horz.m_pntInfo[outColScaled].m_pntWghtIdx;

	// shift column data
	for (my_uint4 r = 0; r < DCTSIZE; r += 1)
		memmove(&horzResizeBuf[hrsIdx][r][0], &horzResizeBuf[hrsIdx][r][1], COPROC_PNT_WGHT_CNT-1);

	// insert new column
	for (my_uint4 r = 0; r < DCTSIZE; r += 1)
		horzResizeBuf[hrsIdx][r][COPROC_PNT_WGHT_CNT-1] = (uint8_t)jdm.m_data.m_b[r];

	hrs.m_inCol += 1;

	hrs.m_colDataPos += hrs.m_bUpScale ? 2 : 1;

	// pop the queue if not the last column in the row, or
	//   if the last output column has been processed. The last
	//   jdm for a row is used repetitively until the last
	//   output column is generated.

	if (!hds.m_bEndOfMcuRow || hrs.m_bBlkRowCompleted) {

		advanceHorzDecPos( horz, hcp, hds );

		jpegDecMsgQue.pop();
	}
}

void advanceHorzDecPos( JobHorz & horz, JobHcp & hcp, HorzDecState & hds )
{
	bool bBlkColLast = hds.m_blkCol == DCTSIZE-1;
	bool bMcuBlkColLast = hds.m_mcuBlkCol == hcp.m_blkColsPerMcu-1;
	bool bMcuBlkRowLast = hds.m_mcuBlkRow == hcp.m_blkRowsPerMcu-1;
	bool bCompIdxLast = hds.m_compIdx == horz.m_compCnt-1;
	bool bMcuColLast = hds.m_mcuCol == horz.m_mcuCols-1;
	bool bMcuRowLast = hds.m_mcuRow == horz.m_mcuRows-1;

	hds.m_blkCol = bBlkColLast ? 0 : hds.m_blkCol+1;

	if (bMcuBlkColLast && bBlkColLast)
		hds.m_mcuBlkCol = 0;
	else if (bBlkColLast)
		hds.m_mcuBlkCol += 1;

	if (bMcuBlkRowLast && bMcuBlkColLast && bBlkColLast)
		hds.m_mcuBlkRow = 0;
	else if (bMcuBlkColLast && bBlkColLast)
		hds.m_mcuBlkRow += 1;

	if (bCompIdxLast && bMcuBlkRowLast && bMcuBlkColLast && bBlkColLast)
		hds.m_compIdx = 0;
	else if (bMcuBlkRowLast && bMcuBlkColLast && bBlkColLast)
		hds.m_compIdx += 1;

	if (bMcuColLast && bCompIdxLast && bMcuBlkRowLast && bMcuBlkColLast && bBlkColLast)
		hds.m_mcuCol = 0;
	else if (bCompIdxLast && bMcuBlkRowLast && bMcuBlkColLast && bBlkColLast)
		hds.m_mcuCol += 1;

	if (bMcuRowLast && bMcuColLast && bCompIdxLast && bMcuBlkRowLast && bMcuBlkColLast && bBlkColLast)
		hds.m_mcuRow = 0;
	else if (bMcuColLast && bCompIdxLast && bMcuBlkRowLast && bMcuBlkColLast && bBlkColLast)
		hds.m_mcuRow += horz.m_mcuRowRstInc;

	hds.m_bStartOfBlkRow = hds.m_blkCol == 0 && hds.m_mcuBlkCol == 0 && hds.m_mcuCol == 0;

	hds.m_bEndOfMcuRow = bMcuColLast && bMcuBlkColLast && hds.m_blkCol == DCTSIZE-1;  // very last column of last block in mcu row
}

bool horzResizeWriteRslt( JobInfo * pJobInfo, queue<HorzWriteMsg> & horzWriteMsgQue, HorzRsltBuf_t &horzRsltBuf, queue<HorzResizeMsg> & horzResizeMsgQue )
{
	struct HorzWriteState {
		// private (or shared since since thread)
		my_uint17 m_mcuBlkRowCnt[8];	// bits 9-3 of mcuRow concat bits 9-0 of mcuCol
		my_uint17 m_waitingForMcuBlkRowCnt;
		my_uint10 m_mcuRowFirst;
		my_uint10 m_mcuColFirst;
		my_uint4 m_mcuRowsActive;
	};

	static bool newImageStart = true;
	static HorzWriteState hws;

	if (newImageStart) {
		newImageStart = false;

		for (int i = 0; i < 8; i += 1)
			hws.m_mcuBlkRowCnt[i] = 0;

		hws.m_waitingForMcuBlkRowCnt = pJobInfo->m_horz.m_mcuBlkRowCnt;

		hws.m_mcuColFirst = 0;
		hws.m_mcuRowFirst = 0;
		hws.m_mcuRowsActive = (pJobInfo->m_horz.m_mcuRows - hws.m_mcuRowFirst) >= 8 ? 8 : (pJobInfo->m_horz.m_mcuRows - hws.m_mcuRowFirst);
	}

	if (horzWriteMsgQue.empty())
		return false;

	HorzWriteMsg & hwm = horzWriteMsgQue.front();

	JobHorz & horz = pJobInfo->m_horz;
	JobHcp & hcp = horz.m_hcp[hwm.m_compIdx];

	my_uint6 hrsIdx = (hwm.m_compIdx << 4) | ((hwm.m_mcuRow & 7) << 1) | hwm.m_mcuBlkRow;

	// write up to 8 rows of 64B to memory
	my_uint10 blkRow = hwm.m_mcuRow * hcp.m_blkRowsPerMcu + hwm.m_mcuBlkRow;
	my_uint26 outPos = blkRow * (hcp.m_outCompBlkCols << 6) + (hwm.m_outMemLine << 6);
	assert(outPos < hcp.m_outCompBlkRows * hcp.m_outCompBlkCols * MEM_LINE_SIZE);

	// write one multi-quadword memory line
	memcpy(&hcp.m_pOutCompBuf[outPos], &horzRsltBuf[hrsIdx][hwm.m_bufIdx][0], 64);

	/////////////////////
	// Messages are sent to vertical resizer to allow it to start ASAP
	//   A message is sent when a set of MCUs are complete. The MCU set
	//   is one MCU wide, and eight MCUs tall (except the bottom of the
	//   image may be less than eight tall).
	//
	// Since the horz resizer processes in the order the decoder sends
	//   messages, we must be tolerant of result order. It is assumed
	//   that the decoder sends messages in MCU block order, but that
	//   eight concurrent MCU rows are processed concurrently and will
	//   be interleaved. Keep track of number of MCU blocks completed
	//   per restart row. When all restart rows being concurrently
	//   processed hit the appropriate value, and the write responses
	//   for those blocks have been recieved then send the message to
	//   the vert resizer.

	// keep track of completed filter rows
	my_uint3 rowIdx = hwm.m_mcuRow & 0x7;
	if (hwm.m_mcuBlkCol == hcp.m_blkColsPerMcu-1)
		hws.m_mcuBlkRowCnt[rowIdx] += 1;

	// check if all active block rows have completed
	my_uint4 matchCnt = 0;
	for (int fr = 0; fr < 8; fr += 1) {
		if (hws.m_mcuBlkRowCnt[fr] >= hws.m_waitingForMcuBlkRowCnt)
			matchCnt += 1;
		else
			break;
	}

	//printf("HorzWriteMsg: ci %d, mcuRow %d, mcuBlkRow %d, mcuBlkCol %d, complete %d, bufIdx %d, outMemLine %d, match %d\n",
	//	(int)hwm.m_compIdx, (int)hwm.m_mcuRow, (int)hwm.m_mcuBlkRow, (int)hwm.m_mcuBlkCol, hwm.m_bBlkRowComplete, (int)hwm.m_bufIdx, (int)hwm.m_outMemLine,
	//	matchCnt == hws.m_mcuRowsActive);

	bool bSendReSizeComplete = false;

	if (matchCnt == hws.m_mcuRowsActive) {
		// we have all the completion messages for the active filter rows
		//   wait for the write completes and send a message to the vertical
		//   scaler.

		bSendReSizeComplete = hwm.m_mcuRow == pJobInfo->m_horz.m_mcuRows-1 && hwm.m_bBlkRowComplete;

		horzResizeMsgQue.push( HorzResizeMsg( hws.m_mcuRowFirst, hws.m_mcuColFirst, bSendReSizeComplete ) );

		hws.m_waitingForMcuBlkRowCnt += horz.m_mcuBlkRowCnt;

		if (hwm.m_bBlkRowComplete) {
			hws.m_mcuColFirst = 0;
			hws.m_mcuRowFirst += 8;
		} else {
			hws.m_mcuColFirst += 1;
		}

		hws.m_mcuRowsActive = (pJobInfo->m_horz.m_mcuRows - hws.m_mcuRowFirst) >= 8 ? 8 : (pJobInfo->m_horz.m_mcuRows - hws.m_mcuRowFirst);
	}

	horzWriteMsgQue.pop();

	return bSendReSizeComplete;
}
