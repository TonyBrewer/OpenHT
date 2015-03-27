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
#include "VertResize.h"

#define MAX_HORZ_ASYNC_THREADS		2

void vertResizeStripe( JobInfo * pJobInfo, queue<VertResizeMsg> &vertResizeMsgQue,
	my_uint11 outMcuRowStart, my_uint11 outMcuColStart, my_uint14 inImageRowStart, my_uint14 inImageRowEnd );

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

void vertResize( JobInfo * pJobInfo, queue<HorzResizeMsg> &horzResizeMsgQue, queue<VertResizeMsg> &vertResizeMsgQue )
{
	JobVert & vert = pJobInfo->m_vert;

	my_uint14 inImageRowStart = 0;	// first row of input image required
	my_uint14 inImageRowEnd = 0;		// last+1 row of input image required

	my_uint11 outMcuRowStart = 0;		// next output MCU row
	my_uint11 outMcuColStart = 0;		// next output MCU col

	// process horz resize messages
	bool bEndOfImage = false;
	for (;;) {

		// determine next mcuRowStart that provides enough mcu rows
		//   to keep 8 encoders busy

		if (inImageRowStart == inImageRowEnd) {
			my_uint14 outImageRowFirst = (uint64_t)(outMcuRowStart * vert.m_maxBlkRowsPerMcu * DCTSIZE);
			my_int6 filterOffset = (17 - vert.m_filterWidth);
			inImageRowStart = vert.m_pntInfo[outImageRowFirst].m_pntWghtStart < -filterOffset ? 0 :
					(vert.m_pntInfo[outImageRowFirst].m_pntWghtStart + filterOffset);

			my_uint11 outMcuRowEnd = outMcuRowStart + 8;
			my_uint14 outImageRowLast = (uint64_t)(outMcuRowEnd * vert.m_maxBlkRowsPerMcu * DCTSIZE - 1);
			if (outImageRowLast >= vert.m_outImageRows) outImageRowLast = vert.m_outImageRows-1;
			inImageRowEnd = vert.m_pntInfo[outImageRowLast].m_pntWghtStart + COPROC_PNT_WGHT_CNT-1;
			if (inImageRowEnd > vert.m_inImageRows) inImageRowEnd = vert.m_inImageRows;
		}

		if (bEndOfImage)
			break;

		HorzResizeMsg & horzMsg = horzResizeMsgQue.front();
		bEndOfImage = horzMsg.m_bEndOfImage;

		// for each message, determine if we have enough input rows to
		//   generage 8 MCU rows of output

		if ((horzMsg.m_mcuRowFirst + 8) * vert.m_maxBlkRowsPerMcu * DCTSIZE < inImageRowEnd) {
			horzResizeMsgQue.pop();
			continue;
		}

		vertResizeStripe( pJobInfo, vertResizeMsgQue, outMcuRowStart, outMcuColStart, inImageRowStart, inImageRowEnd );

		if (outMcuColStart >= vert.m_outMcuCols-1ull) {
			outMcuColStart = 0;
			outMcuRowStart += 8;
			inImageRowStart = inImageRowEnd;
		} else
			outMcuColStart += 1;

		horzResizeMsgQue.pop();
	}

	if (outMcuRowStart < vert.m_outMcuRows) {
		for (outMcuColStart = 0; outMcuColStart < vert.m_outMcuCols; outMcuColStart += 1)
			vertResizeStripe( pJobInfo, vertResizeMsgQue, outMcuRowStart, outMcuColStart, inImageRowStart, inImageRowEnd );
	}
}

struct VertResizeState {
	bool m_bUpScale;
	my_int14 m_pntWghtStart;
	my_uint13 m_pntWghtIdx;
	my_int14 m_rowDataPos;
	my_uint14 m_inRow;
	my_uint14 m_outRow;
	my_uint14 m_outRowEnd;
	bool m_outRowDone;
	//my_uint10 m_outMemLine;
	bool m_bBlkColCompleted;
	my_uint2 m_mcuBlkRowFirst;
};

void vertResizeStripe( JobInfo * pJobInfo, queue<VertResizeMsg> &vertResizeMsgQue,
	my_uint11 outMcuRowStart, my_uint11 outMcuColStart, my_uint14 inImageRowStart, my_uint14 inImageRowEnd )
{
	printf("vertResizeStripe: outMcuRowStart %d, outMcuColStart %d, inImageRowStart %d, inImageRowEnd %d\n",
		(int)outMcuRowStart, (int)outMcuColStart, (int)inImageRowStart, (int)inImageRowEnd);

	// Horz resize state
	VertResizeState vertResizeState[MAX_MCU_COMPONENTS][MAX_MCU_BLK_COLS];	// each mcu col can be two blocks wide

	JobVert & vert = pJobInfo->m_vert;

	uint64_t inMcuRowStart = inImageRowStart / (vert.m_maxBlkRowsPerMcu * DCTSIZE);
	uint64_t inMcuRowEnd = (inImageRowEnd + vert.m_maxBlkRowsPerMcu * DCTSIZE - 1) / (vert.m_maxBlkRowsPerMcu * DCTSIZE);

	my_uint3 blkColCnt = 0;

	// initialize state per component block column
	for (uint64_t ci = 0; ci < vert.m_compCnt; ci += 1) {
		JobVcp & vcp = vert.m_vcp[ci];

		for (uint64_t mcuBlkCol = 0; mcuBlkCol < vcp.m_blkColsPerMcu; mcuBlkCol += 1) {

			VertResizeState &vrs = vertResizeState[ci][mcuBlkCol];

			vrs.m_bUpScale = vert.m_maxBlkRowsPerMcu == 2 && vcp.m_blkRowsPerMcu == 1;
			vrs.m_outRow = outMcuRowStart * vcp.m_blkRowsPerMcu * DCTSIZE;
			vrs.m_outRowEnd = vrs.m_outRow + 8 * vcp.m_blkRowsPerMcu * DCTSIZE;
			if (vrs.m_outRowEnd > vcp.m_outCompRows) vrs.m_outRowEnd = vcp.m_outCompRows;
			vrs.m_outRowDone = false;

			my_uint14 outImageRow = vrs.m_outRow << (vrs.m_bUpScale ? 1 : 0);
			vrs.m_pntWghtIdx = vert.m_pntInfo[outImageRow].m_pntWghtIdx;
			vrs.m_pntWghtStart = vert.m_pntInfo[outImageRow].m_pntWghtStart;

			vrs.m_bBlkColCompleted = false;

			if (vrs.m_bUpScale) {
				my_int6 filterWidth = (vert.m_filterWidth >> 1) + 1;
				my_int6 filterOffset = (18 - (filterWidth << 1));
				vrs.m_inRow = vrs.m_pntWghtStart < -filterOffset ? 0 : ((vrs.m_pntWghtStart + filterOffset) >> 1);
				vrs.m_rowDataPos = vrs.m_pntWghtStart < -filterOffset ? -18 : ((vrs.m_pntWghtStart & ~1) - (filterWidth << 1));
				vrs.m_mcuBlkRowFirst = 0;
			} else {
				my_int6 filterOffset = (17 - vert.m_filterWidth);
				vrs.m_inRow = vrs.m_pntWghtStart < -filterOffset ? 0 : (vrs.m_pntWghtStart + filterOffset);
				vrs.m_rowDataPos = vrs.m_pntWghtStart < -filterOffset ? -17 : (vrs.m_pntWghtStart - vert.m_filterWidth);
				vrs.m_mcuBlkRowFirst = (vrs.m_inRow >> 3) & (vcp.m_blkRowsPerMcu-1);
			}

			blkColCnt += 1;
		}
	}

	uint8_t rowPref[MAX_HORZ_ASYNC_THREADS][MAX_MCU_COMPONENTS][MAX_MCU_BLK_ROWS][MAX_MCU_BLK_COLS][DCTSIZE][DCTSIZE];
	static uint8_t vertResizeBuf[MAX_MCU_COMPONENTS][MAX_MCU_BLK_COLS][VERT_FILTER_COLS][COPROC_PNT_WGHT_CNT];	// buffering for pixel weight calculation
	VertResizeMsgData vertRsltBuf;

	memset(vertResizeBuf, 0, sizeof(vertResizeBuf));

	bool bFirstMcu = true;
	my_uint3 blkColDoneCnt = 0;
	for (uint64_t preMcuRow = inMcuRowStart; blkColDoneCnt < blkColCnt; preMcuRow += 1) {

		// prefetch input data for a full MCU
		if (preMcuRow < inMcuRowEnd) {
			for (uint64_t ci = 0; ci < vert.m_compCnt; ci += 1) {
				JobVcp & vcp = vert.m_vcp[ci];

				my_uint2 mcuBlkRowFirst = bFirstMcu ? vertResizeState[ci][0].m_mcuBlkRowFirst : my_uint2(0);

				for (uint64_t mcuBlkRow = mcuBlkRowFirst; mcuBlkRow < vcp.m_blkRowsPerMcu; mcuBlkRow += 1) {
					for (uint64_t mcuBlkCol = 0; mcuBlkCol < vcp.m_blkColsPerMcu; mcuBlkCol += 1) {

						uint64_t blkRow = preMcuRow * vcp.m_blkRowsPerMcu + mcuBlkRow;
						uint64_t blkCol = outMcuColStart * vcp.m_blkColsPerMcu + mcuBlkCol;
						uint64_t pos = (blkRow * vcp.m_inCompBlkCols + blkCol) * MEM_LINE_SIZE;

						memcpy( &rowPref[0][ci][mcuBlkRow][mcuBlkCol][0][0], &vcp.m_pInCompBuf[pos], 64);
					}
				}
			}
		}

		// shift prefetched data through filter
		for (my_uint3 ci = 0; ci < vert.m_compCnt; ci += 1) {
			JobVcp & vcp = vert.m_vcp[ci];

			my_uint2 mcuBlkRowFirst = bFirstMcu ? vertResizeState[ci][0].m_mcuBlkRowFirst : my_uint2(0);

			for (my_uint2 mcuBlkRow = mcuBlkRowFirst; mcuBlkRow < vcp.m_blkRowsPerMcu && !vertResizeState[ci][0].m_outRowDone; mcuBlkRow += 1) {
				for (my_uint2 mcuBlkCol = 0; mcuBlkCol < vcp.m_blkColsPerMcu; mcuBlkCol += 1) {

					VertResizeState &vrs = vertResizeState[ci][mcuBlkCol];

					for (uint64_t r = vrs.m_inRow & 7; r < DCTSIZE && !vrs.m_outRowDone; r += 1) {

						my_uint1 even = ~vrs.m_pntWghtStart & 1;
						JobPntWght &pntWght = vert.m_pntWghtList[vrs.m_pntWghtIdx];

						// check if we have the data to produce an output pixel
						bool bGenOutPixel = !vrs.m_bBlkColCompleted && (vrs.m_pntWghtStart == vrs.m_rowDataPos ||
							(vrs.m_bUpScale && (vrs.m_pntWghtStart >> 1) == (vrs.m_rowDataPos >> 1)));

						for (my_uint14 c = 0; c < 8; c += 1) {
							my_int21 pixel = 0;
							for (my_uint5 r = 0; r < COPROC_PNT_WGHT_CNT; r += 1) {
								uint8_t data = vrs.m_bUpScale ? vertResizeBuf[ci][mcuBlkCol][c][(even ? 7 : 8)+(r>>1)+(even&r)] : vertResizeBuf[ci][mcuBlkCol][c][r];
								pixel += data * pntWght.m_w[r];
							}
							uint8_t clampedPixel = clampPixel(descale(pixel, COPROC_PNT_WGHT_FRAC_W));

							vertRsltBuf.m_b[c] = clampedPixel;

						}

						if (bGenOutPixel) {
							// push info to send 8B to jpeg encoder
							//my_uint2 outMcuBlkCol = (uint64_t)((hrs.m_outCol-1ull) / DCTSIZE % hcp.m_blkColsPerMcu);

							my_uint11 outMcuRow = vcp.m_blkRowsPerMcu == 2 ? (vrs.m_outRow >> 4) : (vrs.m_outRow >> 3);
							my_uint2 outMcuBlkRow = vcp.m_blkRowsPerMcu == 2 ? (vrs.m_outRow & 0x8) >> 3 : 0;
							my_uint3 outBlkRow = vrs.m_outRow & 0x7;

							bool bFirstBlkInMcuRow = outMcuColStart == 0 && mcuBlkCol == 0;
							bool bLastBlkInMcuRow = outMcuColStart == vert.m_outMcuCols-1 && mcuBlkCol == vcp.m_blkColsPerMcu-1;

							//printf("vertResizeMsg: ci %d, mcuRow %d, blkRowInMcu %d, rowInBlk %d, firstBlkInMcuRow %d, lastBlkInMcuRow %d, blkColInMcu %d, data\n",
							//	(int)ci, (int)outMcuRow, (int)outMcuBlkRow, (int)outBlkRow, (int)bFirstBlkInMcuRow, (int)bLastBlkInMcuRow, (int)mcuBlkCol);

							vertResizeMsgQue.push( VertResizeMsg( ci, outMcuRow, outMcuBlkRow, outBlkRow, 
								bFirstBlkInMcuRow, bLastBlkInMcuRow, mcuBlkCol, vertRsltBuf ) );

							// write to memory for DV to compare, not done in HW
							uint64_t pos = vrs.m_outRow * vcp.m_outCompBufCols + (outMcuColStart * vcp.m_blkColsPerMcu + mcuBlkCol) * 8;
							memcpy( &vcp.m_pOutCompBuf[pos], &vertRsltBuf.m_b[0], 8);
						}

						if (bGenOutPixel)
							vrs.m_outRow += 1;

						vrs.m_bBlkColCompleted = vrs.m_outRow == vcp.m_outCompRows;

						vrs.m_outRowDone = vrs.m_outRow == vrs.m_outRowEnd;
						if (vrs.m_outRowDone)
							blkColDoneCnt += 1;
						
						my_uint14 outImageRow = vrs.m_outRow << (vrs.m_bUpScale ? 1 : 0);
						vrs.m_pntWghtStart = vert.m_pntInfo[outImageRow].m_pntWghtStart;
						vrs.m_pntWghtIdx = vert.m_pntInfo[outImageRow].m_pntWghtIdx;

						// shift resize filter buf
						for (my_uint4 c = 0; c < DCTSIZE; c += 1)
							memmove(&vertResizeBuf[ci][mcuBlkCol][c][0], &vertResizeBuf[ci][mcuBlkCol][c][1], COPROC_PNT_WGHT_CNT-1);

						// insert new row
						for (my_uint4 c = 0; c < DCTSIZE; c += 1)
							vertResizeBuf[ci][mcuBlkCol][c][COPROC_PNT_WGHT_CNT-1] = rowPref[0][ci][mcuBlkRow][mcuBlkCol][c][r];

						vrs.m_inRow += 1;

						vrs.m_rowDataPos += vrs.m_bUpScale ? 2 : 1;
					}
				}
			}

			bFirstMcu = false;
		}
	}
}
