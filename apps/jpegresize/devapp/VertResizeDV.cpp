/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <assert.h>
#include <string.h>

#include "VertResizeDV.h"
#include "PersConfig.h"

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

void vertResizeDV( JobInfo * pJobInfo )
{
	// This version does 64 columns (i.e. a memory line) at a time, top to bottom.
	// Once an entire vertical stripe is completed it moves to the right one memory line.
	JobVert & vert = pJobInfo->m_vert;

	for (int ci = 0; ci < 3; ci += 1) {
		JobVcp & vcp = vert.m_vcp[ci];

		bool bRowUpScale = vert.m_maxBlkRowsPerMcu == 2 && vcp.m_blkRowsPerMcu == 1;
		bool bColUpScale = vert.m_maxBlkColsPerMcu == 2 && vcp.m_blkColsPerMcu == 1;

		int outRows = (vcp.m_outCompRows + (vcp.m_blkRowsPerMcu == 2 ? 15 : 7)) & ~(vcp.m_blkRowsPerMcu == 2 ? 15 : 7);
		int outCols = (vcp.m_outCompCols + (vcp.m_blkColsPerMcu == 2 ? 15 : 7)) & ~(vcp.m_blkColsPerMcu == 2 ? 15 : 7);

		for (int col = 0; col < outCols; col += 1) {
			for (int outRow = 0; outRow < outRows; outRow += 1) {

				if (outRow < vcp.m_outCompRows) {
					int outRowScaled = outRow << (bRowUpScale ? 1 : 0);
					int inRowStart = vert.m_pntInfo[outRowScaled].m_pntWghtStart + 1;
					int pntWghtIdx = vert.m_pntInfo[outRowScaled].m_pntWghtIdx;
					JobPntWght &pntWght = vert.m_pntWghtList[pntWghtIdx];
					int pixel = 0;
					for (int r = 0; r < COPROC_PNT_WGHT_CNT; r += 1) {
						int inCompRow = (inRowStart+r) >> (bRowUpScale ? 1 : 0);
						if (inCompRow >= 0 && inCompRow < vcp.m_inCompRows) {

							uint64_t blkRowPos = (inCompRow >> 3) * (vcp.m_inCompBlkCols << 6);
							uint64_t blkColPos = (col << 3) + (inCompRow & 0x7);
							assert(blkRowPos + blkColPos < vcp.m_inCompBlkRows * vcp.m_inCompBlkCols * MEM_LINE_SIZE);

							uint8_t data = vcp.m_pInCompBuf[blkRowPos + blkColPos];
							pixel += data * pntWght.m_w[r];
						} else
							assert(pntWght.m_w[r] == 0);
					}

					vcp.m_pOutCompBuf[outRow * vcp.m_outCompBufCols + col] = clampPixel(descale(pixel, COPROC_PNT_WGHT_FRAC_W));
				} else
					vcp.m_pOutCompBuf[outRow * vcp.m_outCompBufCols + col] = vcp.m_pOutCompBuf[(vcp.m_outCompRows-1) * vcp.m_outCompBufCols + col];
			}
		}
	}
}
