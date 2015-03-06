/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "JpegCommon.h"
#include "JpegDecodeDV.h"
#include "JpegIdctDV.h"
#include "JpegZigZag.h"

//LenStats g_lenStats;

void jpegDecodeBlkDV( HuffDecDV &huffDec, short (&block)[8][8] );
void jpegDecodeRestartDV( JobInfo * pJobInfo, uint16_t rstIdx );
void jpegIdctBlkDV( JobInfo * pJobInfo, int ci, short (&coefBlock)[8][8], uint8_t (&outBlock)[8][8] );
void jpegWriteBlkDV( JobInfo * pJobInfo, int ci, int mcuRow, int mcuCol,
	int blkRow, int blkCol, uint8_t (&outBlock)[8][8] );

// Decode jpeg image using parameter from JobInfo

int jpegDecBlkCntDV = 0;

void jpegDecodeDV( JobInfo * pJobInfo )
{
	for (int rstIdx = 0; rstIdx < pJobInfo->m_dec.m_rstCnt; rstIdx += 1)
		// Async fork
		jpegDecodeRestartDV( pJobInfo, rstIdx );
}

void jpegDecodeRestartDV( JobInfo * pJobInfo, uint16_t rstIdx )
{
	HuffDecDV huffDec( pJobInfo, rstIdx );

	short coefBlock[8][8];
	uint8_t outBlock[8][8];

	int mcuDecodedCnt = 0;
	int mcuRstCnt = 0;
	int mcuRow = pJobInfo->m_dec.m_rstInfo[rstIdx].m_mcuRow;
	int mcuCol = pJobInfo->m_dec.m_rstInfo[rstIdx].m_mcuCol;
	int rstMcuCntOrig = pJobInfo->m_dec.m_rstMcuCntOrig;
	if (pJobInfo->m_dec.m_rstInfo[rstIdx].m_firstRstMcuCnt != 0)
		rstMcuCntOrig = pJobInfo->m_dec.m_rstInfo[rstIdx].m_firstRstMcuCnt;
	goto restart;

	for (mcuRow = 0; mcuRow < pJobInfo->m_dec.m_mcuRows; mcuRow += 1) {
		for (mcuCol = 0; mcuCol < pJobInfo->m_dec.m_mcuCols; mcuCol += 1) {
restart:
			for (int ci = 0; ci < pJobInfo->m_dec.m_compCnt; ci += 1) {

				huffDec.setCompId( ci );

				for (int blkRow = 0; blkRow < pJobInfo->m_dec.m_dcp[ci].m_blkRowsPerMcu; blkRow += 1) {
					for (int blkCol = 0; blkCol < pJobInfo->m_dec.m_dcp[ci].m_blkColsPerMcu; blkCol += 1) {

						jpegDecodeBlkDV( huffDec, coefBlock );

						jpegIdctBlkDV( pJobInfo, ci, coefBlock, outBlock );

						jpegWriteBlkDV( pJobInfo, ci, mcuRow, mcuCol, blkRow, blkCol, outBlock );

						jpegDecBlkCntDV += 1;
					}
				}
			}

			mcuDecodedCnt += 1;
			mcuRstCnt += 1;
			if (pJobInfo->m_dec.m_rstMcuCnt > 0 && pJobInfo->m_dec.m_rstMcuCnt == mcuDecodedCnt)
				return;
			if (rstMcuCntOrig > 0 && (mcuRstCnt == rstMcuCntOrig)) {
				huffDec.nextRestart();
				rstMcuCntOrig = pJobInfo->m_dec.m_rstMcuCntOrig;
				mcuRstCnt = 0;
			}
		}
	}

	huffDec.checkEndOfScan();
}

void jpegDecodeBlkDV( HuffDecDV &huffDec, short (&block)[8][8] )
{
	memset(block, 0, sizeof(short)*64);

	// find DC value
	int s = huffDec.getNext(true);

	if (s != 0) {
		huffDec.fillBits();
		int r = huffDec.getBits(s);
		s = huffDec.huffExtend(r, s);
	}
	if (true /*pJobInfo->m_dec.m_bCurDcNeeded*/) {
		s += huffDec.getLastDcValueRef();
		huffDec.getLastDcValueRef() = s;
		block[0][0] = s;
	}

	// get AC values
	for (int k = 1; k < 64; k += 1) {
		s = huffDec.getNext(false);
		int r = s >> 4;
		s &= 0xf;

#define JPEG_HUFF_DEC_DEBUG 0
#if JPEG_HUFF_DEC_DEBUG == 3
		if (jpegDecBlkCntDV <= 8)
			printf("s=%d, r=%d, k=%d\n", s, r, k);
#endif
		if (s) {
			k += r;
			huffDec.fillBits();
			if (true /*pJobInfo->m_dec.m_bCurAcNeeded*/) {
				r = huffDec.getBits(s);
				s = huffDec.huffExtend(r, s);
				block[jpegZigZag[k].m_r][jpegZigZag[k].m_c] = s;
			} else
				huffDec.dropBits(s);
		} else {
			if (r != 15) break;
			k += 15;
		}
	}

#if JPEG_HUFF_DEC_DEBUG == 1
	if (jpegDecBlkCntDV < 8)
		printf("D blk=%d ci=%d [%4d %4d %4d %4d %4d %4d %4d %4d]\n",
			jpegDecBlkCntDV, huffDec.getCompId(), block[0][0], block[0][1],
			block[0][2], block[0][3], block[0][4], block[0][5], block[0][6], block[0][7]);
#endif
#if JPEG_HUFF_DEC_DEBUG == 2
	if (jpegDecBlkCntDV < 128) {
		printf("blk=%d ci=%d\n",
			jpegDecBlkCntDV, huffDec.getCompId());
		for (int i = 0; i < 8; i += 1)
			printf("    [%4d %4d %4d %4d %4d %4d %4d %4d]\n",
				block[i][0], block[i][1],
				block[i][2], block[i][3], block[i][4], block[i][5], block[i][6], block[i][7]);
		fflush(stdout);	
	}
	if (jpegDecBlkCntDV == 128)
		exit(0);
#endif
}

void jpegIdctBlkDV( JobInfo * pJobInfo, int ci, short (&coefBlock)[8][8], uint8_t (&outBlock)[8][8] )
{
	uint8_t * pOutPtrs[8];
	for (int i = 0; i < 8; i += 1)
		pOutPtrs[i] = outBlock[i];

	jpegIdctDV ( pJobInfo, ci, coefBlock, pOutPtrs );

#define JPEG_IDCT_DEBUG 0
#if JPEG_IDCT_DEBUG == 1
	printf("blk=%d ci=%d [%3d %3d %3d %3d %3d %3d %3d %3d]\n",
		jpegDecBlkCntDV, ci, 
		outBlock[0][0], outBlock[0][1], outBlock[0][2], outBlock[0][3],
		outBlock[0][4], outBlock[0][5], outBlock[0][6], outBlock[0][7]);
	//if (jpegDecBlkCntDV == 128)
	//	exit(0);
#endif
#if JPEG_IDCT_DEBUG == 2
	if (jpegDecBlkCntDV <= 6) {
		printf("blk=%d ci=%d\n",
			jpegDecBlkCntDV, ci);
		for (int i = 0; i < 8; i += 1)
			printf("    [%3d %3d %3d %3d %3d %3d %3d %3d]\n",
				outBlock[i][0], outBlock[i][1],
				outBlock[i][2], outBlock[i][3], outBlock[i][4], outBlock[i][5], outBlock[i][6], outBlock[i][7]);
	}
	if (jpegDecBlkCntDV == 16)
		exit(0);
#endif
}

void jpegWriteBlkDV( JobInfo * pJobInfo, int ci, int mcuRow, int mcuCol, int blkRow, int blkCol, uint8_t (&outBlock)[8][8] )
{
	// write 8x8 block of 8-bit data to image buffer
	JobDcp &dcp = pJobInfo->m_dec.m_dcp[ci];

	uint64_t row = (mcuRow * pJobInfo->m_dec.m_dcp[ci].m_blkRowsPerMcu + blkRow) * DCTSIZE;
	uint64_t col = (mcuCol * pJobInfo->m_dec.m_dcp[ci].m_blkColsPerMcu + blkCol) * DCTSIZE;

	//if (mcuRow < 2)
	//	printf("WriteBlk (ci=%d, mcuRow=%d, mcuCol=%d, blkRow=%d, blkCol=%d) row=%d, col=%d\n",
	//		ci, mcuRow, mcuCol, blkRow, blkCol, row, col);
	
	for (uint64_t ri = 0; ri < DCTSIZE; ri += 1) {
		//if (row + ri >= dcp.m_compRows)
		//	continue;

		uint64_t imageOffset = (row + ri) * dcp.m_compBufColLines * MEM_LINE_SIZE + col;
		assert(imageOffset < (uint64_t)(dcp.m_compBufRowBlks * DCTSIZE * dcp.m_compBufColLines * MEM_LINE_SIZE));

		*(uint64_t *)(dcp.m_pCompBuf + imageOffset) = *(uint64_t *)outBlock[ri];
	}
}
