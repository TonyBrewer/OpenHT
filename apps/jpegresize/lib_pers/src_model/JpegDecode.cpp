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
#include "JpegDecode.h"
#include "JpegIdct.h"
#include "JpegZigZag.h"
#include "JpegDecMsg.h"

void jpegDecodeBlk( HuffDec &huffDec, int16_t (&block)[8][8] );
void jpegDecodeRestart( JobInfo * pJobInfo, queue<JpegDecMsg> & jpegDecMsgQue, uint16_t rstIdx );
void jpegIdctBlk( JobInfo * pJobInfo, int ci, int16_t (&coefBlock)[8][8], uint8_t (&outBlock)[8][8] );
void jpegWriteBlk ( JobInfo * pJobInfo, my_uint2 ci, my_uint11 mcuRow, my_uint11 mcuCol,
	my_uint2 blkRow, my_uint2 blkCol, uint8_t (&outBlock)[8][8] );

void jpegSendBlk ( JobInfo * pJobInfo, queue<JpegDecMsg> &jpegDecMsgQue,
	my_uint2 ci, my_uint3 rstIdx, my_uint11 mcuRow, my_uint11 mcuCol,
	my_uint2 mcuBlkRow, my_uint2 mcuBlkCol, bool bLastBlkCol, uint8_t (&outBlock)[8][8] );

// Decode jpeg image using parameter from JobInfo

int jpegDecBlkCnt = 0;

void jpegDecode( JobInfo * pJobInfo, queue<JpegDecMsg> & jpegDecMsgQue )
{
	for (int rstIdx = 0; rstIdx < pJobInfo->m_dec.m_rstCnt; rstIdx += 1)
		// Async fork
		jpegDecodeRestart( pJobInfo, jpegDecMsgQue, rstIdx );
}

void jpegDecodeRestart( JobInfo * pJobInfo, queue<JpegDecMsg> & jpegDecMsgQue, uint16_t rstIdx )
{
	HuffDec huffDec( pJobInfo, rstIdx );

	int16_t coefBlock[8][8];
	uint8_t outBlock[8][8];

	my_uint11 mcuDecodedCnt = 0;
	my_uint11 mcuRow = pJobInfo->m_dec.m_rstInfo[rstIdx].m_mcuRow;
	my_uint11 mcuCol = pJobInfo->m_dec.m_rstInfo[rstIdx].m_mcuCol;

	goto restart;

	for (mcuRow = 0; mcuRow < pJobInfo->m_dec.m_mcuRows; mcuRow += 1) {
		for (mcuCol = 0; mcuCol < pJobInfo->m_dec.m_mcuCols; mcuCol += 1) {
restart:
			for (my_uint2 ci = 0; ci < pJobInfo->m_dec.m_compCnt; ci += 1) {

				huffDec.setCompId( ci );

				for (my_uint2 blkRow = 0; blkRow < pJobInfo->m_dec.m_dcp[ci].m_blkRowsPerMcu; blkRow += 1) {
					for (my_uint2 blkCol = 0; blkCol < pJobInfo->m_dec.m_dcp[ci].m_blkColsPerMcu; blkCol += 1) {

						jpegDecodeBlk( huffDec, coefBlock );

						jpegIdctBlk( pJobInfo, (int)ci, coefBlock, outBlock );

						bool bLastBlkCol = mcuCol == pJobInfo->m_dec.m_mcuCols-1 &&
							blkCol == pJobInfo->m_dec.m_dcp[ci].m_blkColsPerMcu-1;

						jpegSendBlk( pJobInfo, jpegDecMsgQue, ci, rstIdx & 0x7, mcuRow, mcuCol, blkRow, blkCol, bLastBlkCol, outBlock );

						jpegDecBlkCnt += 1;
					}
				}
			}

			mcuDecodedCnt += 1;
			if (pJobInfo->m_dec.m_rstMcuCnt > 0 && pJobInfo->m_dec.m_rstMcuCnt == mcuDecodedCnt)
				return;
		}
	}

	huffDec.checkEndOfScan();
}

int cycles = 0;
int pixels = 0;

void jpegDecodeBlk( HuffDec &huffDec, int16_t (&block)[8][8] )
{
	memset(block, 0, sizeof(int16_t)*64);

	pixels += 64;

	// find DC value
	my_uint5 s1 = (uint64_t)huffDec.getNext(true);

	my_int16 s2 = 0;
	if (s1 != 0) {
		huffDec.fillBits();
		my_uint11 r = (uint64_t)huffDec.getBits(s1);
		s2 = huffDec.huffExtendDc(r, s1);
	}
	if (true /*pJobInfo->m_dec.m_bCurDcNeeded*/) {
		my_int16 s3 = s2 + huffDec.getLastDcValueRef();
		huffDec.getLastDcValueRef() = (int16_t)s3;
		block[0][0] = (int16_t)s3;
	}

	// get AC values
	for (my_uint7  k = 1; k < 64; k += 1) {
		my_uint10 s1 = (uint64_t)huffDec.getNext(false);
		my_uint6 r1 = s1 >> 4;
		my_uint4 s2 = s1 & 0xf;

#define JPEG_HUFF_DEC_DEBUG 0
#if JPEG_HUFF_DEC_DEBUG == 3
		if (jpegDecBlkCnt <= 8)
			printf("s=%d, r=%d, k=%d\n", s, r, k);
#endif
		if (s2) {
			k += r1;
			huffDec.fillBits();
			if (true /*pJobInfo->m_dec.m_bCurAcNeeded*/) {
				my_uint10 r2 = (uint64_t)huffDec.getBits((my_uint5)s2);
				my_int16 s3 = huffDec.huffExtendAc(r2, s2);
				block[jpegZigZag[k].m_r][jpegZigZag[k].m_c] = (int16_t)s3;
			} else
				huffDec.dropBits((my_uint5)s2);
		} else {
			if (r1 != 15) break;
			k += 15;
		}
	}

#if JPEG_HUFF_DEC_DEBUG == 1
	if (jpegDecBlkCnt < 8)
		printf("D blk=%d ci=%d [%4d %4d %4d %4d %4d %4d %4d %4d]\n",
			jpegDecBlkCnt, huffDec.getCompId(), block[0][0], block[0][1],
			block[0][2], block[0][3], block[0][4], block[0][5], block[0][6], block[0][7]);
#endif
#if JPEG_HUFF_DEC_DEBUG == 2
	if (jpegDecBlkCnt < 16) {
		printf("blk=%d ci=%d\n",
			jpegDecBlkCnt, huffDec.getCompId());
		for (int i = 0; i < 8; i += 1)
			printf("    [%4d %4d %4d %4d %4d %4d %4d %4d]\n",
				block[i][0], block[i][1],
				block[i][2], block[i][3], block[i][4], block[i][5], block[i][6], block[i][7]);
	}
	if (jpegDecBlkCnt == 16)
		exit(0);
#endif
}

void jpegIdctBlk( JobInfo * pJobInfo, int ci, int16_t (&coefBlock)[8][8], uint8_t (&outBlock)[8][8] )
{
	uint8_t * pOutPtrs[8];
	for (int i = 0; i < 8; i += 1)
		pOutPtrs[i] = outBlock[i];

	jpegIdct ( pJobInfo, ci, coefBlock, pOutPtrs );

#define JPEG_IDCT_DEBUG 0
#if JPEG_IDCT_DEBUG == 1
	printf("blk=%d ci=%d [%3d %3d %3d %3d %3d %3d %3d %3d]\n",
		jpegDecBlkCnt, ci, 
		outBlock[0][0], outBlock[0][1], outBlock[0][2], outBlock[0][3],
		outBlock[0][4], outBlock[0][5], outBlock[0][6], outBlock[0][7]);
	//if (jpegDecBlkCnt == 128)
	//	exit(0);
#endif
#if JPEG_IDCT_DEBUG == 2
	if (jpegDecBlkCnt <= 6) {
		printf("blk=%d ci=%d\n",
			jpegDecBlkCnt, ci);
		for (int i = 0; i < 8; i += 1)
			printf("    [%3d %3d %3d %3d %3d %3d %3d %3d]\n",
				outBlock[i][0], outBlock[i][1],
				outBlock[i][2], outBlock[i][3], outBlock[i][4], outBlock[i][5], outBlock[i][6], outBlock[i][7]);
	}
	if (jpegDecBlkCnt == 128)
		exit(0);
#endif
}

void jpegSendBlk ( JobInfo * pJobInfo, queue<JpegDecMsg> &jpegDecMsgQue,
	my_uint2 ci, my_uint3 rstIdx, my_uint11 mcuRow, my_uint11 mcuCol,
	my_uint2 mcuBlkRow, my_uint2 mcuBlkCol, bool bLastBlkCol, uint8_t (&outBlock)[8][8] )
{
	for (uint64_t c = 0; c < DCTSIZE; c += 1) {
		//bool bInColFirst = mcuCol == 0 && mcuBlkCol == 0 && c == 0;
		//bool bInColLast = bLastBlkCol && c == 0x7;

		JpegDecMsgData data;
		for (uint64_t r = 0; r < DCTSIZE; r += 1)
			data.m_b[r] = outBlock[r][c];

		//printf("JpegDecMsg: bInColFirst %d, bInColLast %d, mcuRow %d, ci %d, mcuBlkRow %d, mcuBlkCol %d, c %d, data\n",
		//	bInColFirst, bInColLast, (int)mcuRow, (int)ci, (int)mcuBlkRow, (int)mcuBlkCol, (int)c);

		// Send a message to the horz resizer
		bool bRstLast = false;
		my_uint1 imageIdx = 0;
		my_uint8 jobId = 0;
		jpegDecMsgQue.push( JpegDecMsg( ci, rstIdx, bRstLast, imageIdx, jobId, data));
	}

	// write block to memory so jpeg decoder can be verified against "golden version"
	//   This is not done in hardware
	jpegWriteBlk( pJobInfo, ci, mcuRow, mcuCol, mcuBlkRow, mcuBlkCol, outBlock );
}

void jpegWriteBlk ( JobInfo * pJobInfo, my_uint2 ci, my_uint11 mcuRow, my_uint11 mcuCol,
	my_uint2 blkRow, my_uint2 blkCol, uint8_t (&outBlock)[8][8] )
{
	// write 8x8 block of 8-bit data to image buffer
	JobDcp &dcp = pJobInfo->m_dec.m_dcp[ci];

	my_uint14 row = (uint64_t)((mcuRow * pJobInfo->m_dec.m_dcp[ci].m_blkRowsPerMcu + blkRow) * DCTSIZE);
	my_uint14 col = (uint64_t)((mcuCol * pJobInfo->m_dec.m_dcp[ci].m_blkColsPerMcu + blkCol) * DCTSIZE);

	//if (mcuRow < 2)
	//	printf("WriteBlk (ci=%d, mcuRow=%d, mcuCol=%d, blkRow=%d, blkCol=%d) row=%d, col=%d\n",
	//		ci, mcuRow, mcuCol, blkRow, blkCol, row, col);
	
	for (my_uint4 ri = 0; ri < DCTSIZE; ri += 1) {
		//if (row + ri >= dcp.m_compRows)
		//	continue;

		my_uint26 imageOffset = (row + ri) * dcp.m_compBufColLines * MEM_LINE_SIZE + col;
		assert((uint64_t)imageOffset < (uint64_t)(dcp.m_compBufRowBlks * DCTSIZE * dcp.m_compBufColLines * MEM_LINE_SIZE));

		*(uint64_t *)(dcp.m_pCompBuf + imageOffset) = *(uint64_t *)outBlock[ri];
	}
}

