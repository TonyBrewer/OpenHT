/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "JpegHdrDec.h"


void jpegDecodeBlk( JpegHuffDec &huffDec, bool bFirstMcuInRow, bool bInRestart=false )
{
	// find DC value
	int s = huffDec.getNext(true, !(bFirstMcuInRow || bInRestart));

	if (s != 0) {
		huffDec.fillBits();
		int r = huffDec.getBits(s, !(bFirstMcuInRow || bInRestart));
		s = huffDec.huffExtend(r, s);
	}

	if (!bInRestart)
	s += huffDec.getLastDcValueRef();

	if (bFirstMcuInRow) {
		// write the DC value
		int tmp = s;
		int tmp2 = s;

		if (tmp < 0) {
			tmp = -tmp;
			tmp2--;
		}

		int nbits = 0;
		while (tmp) {
			nbits++;
			tmp >>= 1;
		}

		/* Emit the Huffman-coded symbol for the number of bits */
		huffDec.putNbits(nbits, true);
		huffDec.putBits(tmp2, nbits);
	} else if (bInRestart) {
		// convert the DC value to a difference
		int tmp = s - huffDec.getLastDcValueRef();
		int tmp2 = tmp;
		if (tmp < 0) {
			tmp = -tmp;
			tmp2--;
	}

		int nbits = 0;
		while (tmp) {
			nbits++;
			tmp >>= 1;
		}

		/* Emit the Huffman-coded symbol for the number of bits */
		huffDec.putNbits(nbits, true);
		huffDec.putBits(tmp2, nbits);
	}
	huffDec.getLastDcValueRef() = s;

	// get AC values
	for (int k = 1; k < 64; k += 1) {
		s = huffDec.getNext(false, true);
		int r = s >> 4;
		s &= 0xf;

		if (s) {
			k += r;
			huffDec.fillBits();
			if (true /*pJobInfo->m_dec.m_bCurAcNeeded*/) {
				huffDec.getBits(s, true);
			} else
				huffDec.getBits(s, true);
		} else {
			if (r != 15) break;
			k += 15;
		}
	}
}

void jpegDecodeRst( JobInfo * pJobInfo, JpegHuffDec &huffDec )
{
	bool bFirstMcuInRow = true;
	for (int mcuCol = 0; mcuCol < pJobInfo->m_dec.m_mcuCols; mcuCol += 1) {
		for (int ci = 0; ci < pJobInfo->m_dec.m_compCnt; ci += 1) {

			huffDec.setCompId( ci );

			for (int blkRow = 0; blkRow < pJobInfo->m_dec.m_dcp[ci].m_blkRowsPerMcu; blkRow += 1) {
				for (int blkCol = 0; blkCol < pJobInfo->m_dec.m_dcp[ci].m_blkColsPerMcu; blkCol += 1) {

					jpegDecodeBlk( huffDec, bFirstMcuInRow && blkCol == 0 && blkRow == 0 );
				}
			}
		}
		bFirstMcuInRow = false;
	}
}

void jpegDecodeChangeRst( JobInfo * pJobInfo, JpegHuffDec &huffDec,
		int &mcuRow, int &mcuCol  )
{

	int mcuDecodedCnt = 0;
	goto restart;

	for (mcuRow = 0; mcuRow < pJobInfo->m_dec.m_mcuRows; mcuRow += 1) {
		for (mcuCol = 0; mcuCol < pJobInfo->m_dec.m_mcuCols; mcuCol += 1) {
restart:
			for (int ci = 0; ci < pJobInfo->m_dec.m_compCnt; ci += 1) {

				huffDec.setCompId( ci );

				for (int blkRow = 0; blkRow < pJobInfo->m_dec.m_dcp[ci].m_blkRowsPerMcu; blkRow += 1) {
					for (int blkCol = 0; blkCol < pJobInfo->m_dec.m_dcp[ci].m_blkColsPerMcu; blkCol += 1) {

						jpegDecodeBlk( huffDec, mcuCol == 0 && blkCol == 0 && blkRow == 0,
								mcuDecodedCnt == 0 && blkCol == 0 && blkRow == 0);
					}
				}
			}

			mcuDecodedCnt += 1;
			if (pJobInfo->m_dec.m_rstMcuCnt > 0 && pJobInfo->m_dec.m_rstMcuCnt == mcuDecodedCnt) {
				mcuCol++;
				if (mcuCol == pJobInfo->m_dec.m_mcuCols) {
					mcuCol = 0;
					mcuRow++;
					uint8_t *pRstPtr = huffDec.addRstMarker();
					pJobInfo->m_dec.m_rstCnt += 1;
					pJobInfo->m_dec.m_rstInfo[ pJobInfo->m_dec.m_rstCnt ].m_u64 = 0;
					pJobInfo->m_dec.m_rstInfo[ pJobInfo->m_dec.m_rstCnt ].m_offset = pRstPtr - pJobInfo->m_dec.m_pRstBase;
					pJobInfo->m_dec.m_rstInfo[ pJobInfo->m_dec.m_rstCnt ].m_firstRstMcuCnt = 0;
				}
				return;
			}
		}
		int mcuRestartCnt = 0;
		if (pJobInfo->m_dec.m_rstMcuCnt > 0)
			mcuRestartCnt = pJobInfo->m_dec.m_rstMcuCnt - mcuDecodedCnt;
		uint8_t *pRstPtr = huffDec.addRstMarker();
		pJobInfo->m_dec.m_rstCnt += 1;
		pJobInfo->m_dec.m_rstInfo[ pJobInfo->m_dec.m_rstCnt ].m_u64 = 0;
		pJobInfo->m_dec.m_rstInfo[ pJobInfo->m_dec.m_rstCnt ].m_offset = pRstPtr - pJobInfo->m_dec.m_pRstBase;
		pJobInfo->m_dec.m_rstInfo[ pJobInfo->m_dec.m_rstCnt ].m_firstRstMcuCnt = mcuRestartCnt;
	}
}

