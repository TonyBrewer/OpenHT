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

#include "Ht.h"
using namespace Ht;

#include "JobDV.h"
#include "JpegDecodeDV.h"
#include "HorzResizeDV.h"
#include "VertResizeDV.h"
#include "JpegEncodeDV.h"

int findFirstDiff(uint8_t const * pBuf1, uint8_t const * pBuf2, size_t size, size_t validBytesPerQuadword=8)
{
	if (validBytesPerQuadword == 8 && memcmp(pBuf1, pBuf2, size) == 0)
		return -1;

	for (size_t i = 0; i < size; i += 1) {
		if ((i & 0x7) >= validBytesPerQuadword)
			continue;
		if (pBuf1[i] != pBuf2[i])
			return (int)i;
	}

	return -1;
}

// Run job phases required before phases that will be run on coproc
void preJobDV( JobInfo * pJobInfo, int jobDV )
{
	if (((jobDV & DV_DEC) && !(jobDV & DV_HORZ) && (jobDV & (DV_VERT | DV_ENC))) ||
		((jobDV & DV_HORZ) && !(jobDV & DV_VERT) && (jobDV & DV_ENC))) 
	{
		printf("Illegal combination of DV_DEC/DV_HORZ/DV_VERT/DV_ENC\n");
		exit(1);
	}

	// These routines are run on host to setup inputs to coproc phases
	if (!(jobDV & DV_DEC))
		// decode input image
		jpegDecodeDV( pJobInfo );

	if (!(jobDV & DV_DEC) && !(jobDV & DV_HORZ))
		// horizontal resize image
		horzResizeDV( pJobInfo );

	if (!(jobDV & DV_DEC) && !(jobDV & DV_HORZ) && !(jobDV & DV_VERT))
		// vertical resize image
		vertResizeDV( pJobInfo );

	if (!(jobDV & DV_DEC) && !(jobDV & DV_HORZ) && !(jobDV & DV_VERT) && !(jobDV & DV_ENC))
		// jpeg encode output image
		jpegEncodeDV( pJobInfo );
}

// Verify job phases run on coproc, and then run any phases that are
//  left after phases run on coproc
bool postJobDV( JobInfo * pJobInfo, int jobDV )
{
	int memToFreeListIdx = 0;
	uint8_t * pMemToFreeList[32];

	printf("\n");

	JobInfo * pJobInfoDV = 0;
	if (jobDV != 0) {
		ht_posix_memalign((void **)&pJobInfoDV, 64, sizeof(struct JobInfo));
		memcpy(pJobInfoDV, pJobInfo, sizeof(JobInfo));

		pMemToFreeList[memToFreeListIdx++] = (uint8_t *)pJobInfoDV;
	}

	// These first routines are run to verify results generated on coproc
#if !defined(HT_VSIM) && !defined(HT_APP)
	if (jobDV & DV_DEC) {
		// set DV component buffers
		JobDec & dec = pJobInfoDV->m_dec;
		for (int ci = 0; ci < dec.m_compCnt; ci += 1) {

			int64_t bufRows = (dec.m_dcp[ci].m_compRows + (dec.m_dcp[ci].m_blkRowsPerMcu * DCTSIZE - 1)) & ~(dec.m_dcp[ci].m_blkRowsPerMcu * DCTSIZE - 1);
			int64_t bufCols = (dec.m_dcp[ci].m_compCols + MEM_LINE_SIZE-1) & ~(MEM_LINE_SIZE-1);

			int fail = ht_posix_memalign( (void **)&dec.m_dcp[ci].m_pCompBuf, 64, bufRows * bufCols );
			assert(!fail);

			pMemToFreeList[memToFreeListIdx++] = dec.m_dcp[ci].m_pCompBuf;
		}

		// decode input image
		jpegDecodeDV( pJobInfoDV );

		// now verify
		for (int ci = 0; ci < dec.m_compCnt; ci += 1) {
			//int64_t bufRows = dec.m_dcp[ci].m_compBufRowBlks * DCTSIZE;
			int64_t bufCols = dec.m_dcp[ci].m_compBufColLines * MEM_LINE_SIZE;

			for (int row = 0; row < dec.m_dcp[ci].m_compRows; row += 1) {
				int col;
				if ((col = findFirstDiff( dec.m_dcp[ci].m_pCompBuf + row * bufCols, pJobInfo->m_dec.m_dcp[ci].m_pCompBuf + row * bufCols, dec.m_dcp[ci].m_compCols )) >= 0) 
				{
					printf("DV_DEC error: ci=%d, row=%d, col=%d\n", ci, row, col);
					printf("  Expected versus Actual\n");
					for (int i = 0; i < dec.m_dcp[ci].m_compCols; i += 8) {
						printf("col %4d:", i);

						int j;
						for (j = 0; j < 8 && i + j < dec.m_dcp[ci].m_compCols; j += 1)
							printf(" %3d", pJobInfoDV->m_dec.m_dcp[ci].m_pCompBuf[row * bufCols + i + j] );

						for ( ; j < 8; j += 1)
							printf("    ");

						printf("  vs  ");

						for (int j = 0; j < 8 && i + j < dec.m_dcp[ci].m_compCols; j += 1)
							printf(" %3d", pJobInfo->m_dec.m_dcp[ci].m_pCompBuf[row * bufCols + i + j] );

						printf("\n");
					}
					printf("Stopping due to previous DV error\n\n");
					return false;
				}
			}
		}
		printf("DV_DEC passed\n");
	} else
		printf("DV_DEC not selected\n");
#endif

	if (jobDV & (DV_DEC_HORZ | DV_HORZ)) {
		// set DV component buffers
		JobHorz & horz = pJobInfoDV->m_horz;
		for (int ci = 0; ci < horz.m_compCnt; ci += 1) {
			uint64_t size = horz.m_hcp[ci].m_outCompBlkRows * horz.m_hcp[ci].m_outCompBlkCols * MEM_LINE_SIZE;

			int fail = ht_posix_memalign( (void **)&horz.m_hcp[ci].m_pOutCompBuf, 64, size );
			assert(!fail);

			pMemToFreeList[memToFreeListIdx++] = horz.m_hcp[ci].m_pOutCompBuf;
		}

		// horizontal resize image
		horzResizeDV( pJobInfoDV );

		// now verify
		bool bError = false;
		for (int ci = 0; ci < horz.m_compCnt; ci += 1) {
			int64_t blkRows = horz.m_hcp[ci].m_outCompBlkRows;
			int64_t bufCols = horz.m_hcp[ci].m_outCompBlkCols * MEM_LINE_SIZE;
			//int64_t cmpCols = horz.m_hcp[ci].m_outCompCols * 8;
			int64_t cmpCols = bufCols;

			for (int row = 0; row < blkRows; row += 1) {

				int64_t remainingRows = horz.m_hcp[ci].m_inCompRows - row * DCTSIZE;
				if (remainingRows <= 0)
					continue;

				uint64_t colsOfQwCnt = remainingRows > 8 ? 8 : remainingRows;

				int col;
				if ((col = findFirstDiff(horz.m_hcp[ci].m_pOutCompBuf + row * bufCols,
					pJobInfo->m_horz.m_hcp[ci].m_pOutCompBuf + row * bufCols, 
					cmpCols, colsOfQwCnt )) >= 0)
				{
					printf("DV_HORZ error: ci=%d, blkRow=%d, col=%d\n", ci, row, col);
					printf("  Expected versus Actual\n");
					for (int i = 0; i < cmpCols; i += 8) {
						printf("col %4d:", i);

						int j;
						for (j = 0; j < 8 && i + j < cmpCols; j += 1)
							printf(" %3d", pJobInfoDV->m_horz.m_hcp[ci].m_pOutCompBuf[row * bufCols + i + j] );

						for ( ; j < 8; j += 1)
							printf("    ");

						printf("  vs  ");

						for (int j = 0; j < 8 && i + j < cmpCols; j += 1)
							printf(" %3d", pJobInfo->m_horz.m_hcp[ci].m_pOutCompBuf[row * bufCols + i + j] );

						printf("\n");
					}
					bError = true;
					return false;
				}
			}
		}
		if (bError) {
			printf("Stopping due to previous DV error\n\n");
			return false;
		} else
			printf("DV_HORZ passed\n");
	} else
		printf("DV_HORZ not selected\n");

#if !defined(HT_VSIM) && !defined(HT_APP)
	if (jobDV & (DV_VERT | DV_VERT_ENC)) {
		// set DV component buffers
		JobVert & vert = pJobInfoDV->m_vert;
		for (int ci = 0; ci < vert.m_compCnt; ci += 1) {
			int64_t bufRows = vert.m_vcp[ci].m_outCompBufRows;
			int64_t bufCols = vert.m_vcp[ci].m_outCompBufCols;

			int fail = ht_posix_memalign( (void **)&vert.m_vcp[ci].m_pOutCompBuf, 64, bufRows * bufCols );
			assert(!fail);

			pMemToFreeList[memToFreeListIdx++] = vert.m_vcp[ci].m_pOutCompBuf;
		}

		// vertical resize image
		vertResizeDV( pJobInfoDV );

		// now verify
		bool bError = false;
		for (int ci = 0; ci < vert.m_compCnt; ci += 1) {
			int64_t bufRows = vert.m_vcp[ci].m_outCompBufRows;
			int64_t bufCols = (vert.m_vcp[ci].m_outCompCols + MEM_LINE_SIZE-1) & ~(MEM_LINE_SIZE-1);

			for (int row = 0; row < bufRows; row += 1) {
				int col;
				if ((col = findFirstDiff(vert.m_vcp[ci].m_pOutCompBuf + row * bufCols, pJobInfo->m_vert.m_vcp[ci].m_pOutCompBuf + row * bufCols, vert.m_vcp[ci].m_outCompCols )) >= 0) {
					printf("DV_VERT error: ci=%d, row=%d, col=%d\n", ci, row, col);
					printf("  Expected versus Actual\n");
					for (int i = 0; i < vert.m_vcp[ci].m_outCompCols; i += 8) {
						printf("col %4d:", i);

						int j;
						for (j = 0; j < 8 && i + j < vert.m_vcp[ci].m_outCompCols; j += 1)
							printf(" %3d", pJobInfoDV->m_vert.m_vcp[ci].m_pOutCompBuf[row * bufCols + i + j] );

						for ( ; j < 8; j += 1)
							printf("    ");

						printf("  vs  ");

						for (int j = 0; j < 8 && i + j < vert.m_vcp[ci].m_outCompCols; j += 1)
							printf(" %3d", pJobInfo->m_vert.m_vcp[ci].m_pOutCompBuf[row * bufCols + i + j] );

						printf("\n");
					}
					bError = true;
					break;
				}
			}
		}
		if (bError) {
			printf("Stopping due to previous DV error\n\n");
			return false;
		} else
			printf("DV_VERT passed\n");
	} else
#endif
		printf("DV_VERT not selected\n");

	if (jobDV & (DV_VERT_ENC | DV_ENC)) {
		// set DV component buffers
		JobEnc & enc = pJobInfoDV->m_enc;
		int fail = ht_posix_memalign( (void **)&enc.m_pRstBase, 64, enc.m_mcuRows * enc.m_rstOffset );
		assert(!fail);

		pMemToFreeList[memToFreeListIdx++] = enc.m_pRstBase;

		// jpeg encode output image
		jpegEncodeDV( pJobInfoDV );

		// now verify
		bool bError = false;
		uint64_t encRstCnt = (enc.m_mcuRows + enc.m_mcuRowsPerRst - 1) / enc.m_mcuRowsPerRst;
		for (uint32_t rst = 0; rst < encRstCnt; rst += 1) {
			if (enc.m_rstLength[rst] != pJobInfo->m_enc.m_rstLength[rst]) {
				printf("DV_ENC error: rstLength does not match, rst %d\n", rst);
				printf("  Expected versus Actual\n");

				for (size_t i = 0; i < encRstCnt; i += 8) {
					printf("rst %4d:", (int)i);

					size_t j;
					for (j = 0; j < 8 && i + j < encRstCnt; j += 1)
						printf(" %3d", enc.m_rstLength[i + j] );

					for ( ; j < 8; j += 1)
						printf("    ");

					printf("  vs  ");

					for (j = 0; j < 8 && i + j < encRstCnt; j += 1)
						printf(" %3d", pJobInfo->m_enc.m_rstLength[i + j] );

					printf("\n");
				}
				bError = true;
				break;
			}
		}
		//if (bError) {
		//	printf("Stopping due to previous DV error\n\n");
		//	return false;
		//}

		// now verify
		for (uint32_t rst = 0; rst < encRstCnt; rst += 1) {
			int col;
			if ((col = findFirstDiff(enc.m_pRstBase + rst * enc.m_rstOffset, pJobInfo->m_enc.m_pRstBase + rst * enc.m_rstOffset, enc.m_rstLength[rst] )) >= 0) {
				printf("DV_ENC error: rstData does not match, rst=%d, pos=%d\n", rst, col);
				printf("  Expected versus Actual\n");
				for (size_t i = 0; i < enc.m_rstLength[rst]; i += 8) {
					printf("col %4d:", (int)i);

					size_t j;
					for (j = 0; j < 8 && i + j < enc.m_rstLength[rst]; j += 1)
						printf(" %3d", enc.m_pRstBase[rst * enc.m_rstOffset + i + j] );

					for ( ; j < 8; j += 1)
						printf("    ");

					printf("  vs  ");

					for (j = 0; j < 8 && i + j < enc.m_rstLength[rst]; j += 1)
						printf(" %3d", pJobInfo->m_enc.m_pRstBase[rst * enc.m_rstOffset + i + j] );

					printf("\n");
				}
				bError = true;
				break;
			}
		}
		if (bError) {
			printf("Stopping due to previous DV error\n\n");
			return false;
		} else
			printf("DV_ENC passed\n");
	} else
		printf("DV_ENC not selected\n");

	// free allocated memory for DV
	for (int i = 0; i < memToFreeListIdx; i += 1)
		ht_free_memalign(pMemToFreeList[i]);

	// This final set of routines generate results after coproc phases
	if ((jobDV & DV_DEC) && !(jobDV & DV_HORZ))
		// horizontal resize image
		horzResizeDV( pJobInfo );

	if ((jobDV & (DV_DEC | DV_DEC_HORZ | DV_HORZ)) && !(jobDV & (DV_VERT | DV_VERT_ENC)))
		// vertical resize image
		vertResizeDV( pJobInfo );

	if ((jobDV & (DV_DEC | DV_DEC_HORZ | DV_HORZ | DV_VERT)) && !(jobDV & DV_ENC))
		// jpeg encode output image
		jpegEncodeDV( pJobInfo );

	printf("\n");
	return true;
}
