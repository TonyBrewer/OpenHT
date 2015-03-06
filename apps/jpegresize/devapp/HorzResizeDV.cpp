/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <assert.h>
#include <string.h>

#include "HorzResizeDV.h"
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

void horzResizeDV( JobInfo * pJobInfo )
{
	JobHorz & horz = pJobInfo->m_horz;

	for (int ci = 0; ci < 3; ci += 1) {
		JobHcp & hcp = horz.m_hcp[ci];

		bool bRowUpScale = horz.m_maxBlkRowsPerMcu == 2 && hcp.m_blkRowsPerMcu == 1;
		bool bColUpScale = horz.m_maxBlkColsPerMcu == 2 && hcp.m_blkColsPerMcu == 1;

		int outRows = (hcp.m_outCompRows + (hcp.m_blkRowsPerMcu == 2 ? 15 : 7)) & ~(hcp.m_blkRowsPerMcu == 2 ? 15 : 7);
		int outCols = (hcp.m_outCompCols + (hcp.m_blkColsPerMcu == 2 ? 15 : 7)) & ~(hcp.m_blkColsPerMcu == 2 ? 15 : 7);

		for (int row = 0; row < outRows; row += 1) {
			for (int outCol = 0; outCol < outCols; outCol += 1) {

				if (outCol < hcp.m_outCompCols) {

					int outColScaled = outCol << (bColUpScale ? 1 : 0);
					int inColStart = horz.m_pntInfo[outColScaled].m_pntWghtStart + 1;
					int pntWghtIdx = horz.m_pntInfo[outColScaled].m_pntWghtIdx;
					JobPntWght &pntWght = horz.m_pntWghtList[pntWghtIdx];

					int pixel = 0;
					for (int c = 0; c < COPROC_PNT_WGHT_CNT; c += 1) {
						int inCompCol = (inColStart+c) >> (bColUpScale ? 1 : 0);
						if (inCompCol >= 0 && inCompCol < hcp.m_inCompCols) {
							uint8_t data = hcp.m_pInCompBuf[row * hcp.m_inCompBufCols + inCompCol];
							pixel += data * pntWght.m_w[c];
						} /*else
							assert(pntWght.m_w[c] == 0);*/
					}

					uint64_t blkRowPos = (row >> 3) * (hcp.m_outCompBlkCols << 6);
					uint64_t blkColPos = (outCol << 3) + (row & 0x7);

					hcp.m_pOutCompBuf[blkRowPos + blkColPos] = clampPixel(descale(pixel, COPROC_PNT_WGHT_FRAC_W));

				} else {
					uint64_t blkRowPos = (row >> 3) * (hcp.m_outCompBlkCols << 6);
					uint64_t blkColPos = (outCol << 3) + (row & 0x7);
					uint64_t srcBlkColPos = ((hcp.m_outCompCols-1) << 3) + (row & 0x7);

					hcp.m_pOutCompBuf[blkRowPos + blkColPos] = hcp.m_pOutCompBuf[blkRowPos + srcBlkColPos];
				}
			}
		}
	}
}
