/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

//#include "AppArgs.h"
#include "JpegHdr.h"
#include "JpegZigZag.h"
#include "JobInfo.h"
#include "JpegHuffman.h"
//#include "AppArgs.h"
#include "JpegHdrDec.h"
#include "profile.hpp"

#define JPEG_HDR_DEBUG 0
void debugMsg(char const * pFmt, ... ) {
#if (JPEG_HDR_DEBUG==1)
	va_list args;
	va_start( args, pFmt );
	vprintf(pFmt, args);
#endif
}

#define MARKER_INV	0
#define MARKER_TEM	1
#define MARKER_RES	2
#define MARKER_SOF	3
#define MARKER_DHT	4
#define MARKER_DAC	5
#define MARKER_RST	6
#define MARKER_SOI	7
#define MARKER_EOI	8
#define MARKER_SOS	9
#define MARKER_DQT	10
#define MARKER_DNL	11
#define MARKER_DRI	12
#define MARKER_DHP	13
#define MARKER_EXP	14
#define MARKER_APP	15
#define MARKER_JPG	16
#define MARKER_COM	17

uint8_t getJpegHdrNibU( uint8_t * pInPtr ) { return (pInPtr[0] >> 4) & 0xf; }
uint8_t getJpegHdrNibL( uint8_t * pInPtr ) { return pInPtr[0] & 0xf; }
uint8_t getJpegHdrByte( uint8_t * pInPtr ) { return pInPtr[0]; }
uint16_t getJpegHdrWord( uint8_t * pInPtr ) { return (pInPtr[0] << 8) | pInPtr[1]; }

void putJpegHdrNib( FILE *outFp, uint8_t hi, uint8_t lo ) {
	uint8_t data = (hi << 4) | (lo & 0xf);
	fwrite( &data, sizeof(uint8_t), 1, outFp );
}

void putJpegHdrNib( uint8_t *& buffer, uint8_t hi, uint8_t lo ) {
	*buffer = (hi << 4) | (lo & 0xf);
	buffer++;
}
void putJpegHdrByte( FILE *outFp, uint8_t byte ) { fwrite( &byte, sizeof(uint8_t), 1, outFp ); }

void putJpegHdrByte( uint8_t *& buffer , uint8_t byte ) { 
  *buffer=byte;
  buffer++;
}
void putJpegHdrWord( FILE *outFp, uint16_t word ) { 
	uint16_t data = (word >> 8) | (word << 8);
	fwrite( &data, sizeof(uint16_t), 1, outFp );
}
void putJpegHdrWord( uint8_t *& buffer, uint16_t word ) { 
	*(uint16_t *)buffer = (word >> 8) | (word << 8);
	buffer+=sizeof(uint16_t);
}

inline void expandBuffer(uint8_t *& base, uint8_t *&work, unsigned int newSize) {
    unsigned int offset=(uint32_t)(work-base);
    base=(uint8_t *)realloc(base, newSize);
    work=&base[offset];
}

inline int64_t jdivRoundUp (int64_t a, int64_t b)
{
  return (a + b - 1) / b;
}

char const * parseJpegHdr( JobInfo *pJobInfo, uint8_t *pInPic, bool addRestarts )
{
	static bool bParseJpegHdrInit = true;
	static uint8_t markerTbl[256];
	PROFILE_START("bParseJpegHdrInit");
	if (bParseJpegHdrInit) {
		
		markerTbl[0x00] = MARKER_INV;
		markerTbl[0x01] = MARKER_TEM;

		for (int i = 2; i <= 0xBF; i += 1)
			markerTbl[i] = MARKER_RES;

		for (int i = 0xC0; i <= 0xC3; i += 1)
			markerTbl[i] = MARKER_SOF;

		markerTbl[0xC4] = MARKER_DHT;

		for (int i = 0xC5; i <= 0xCB; i += 1)
			markerTbl[i] = MARKER_SOF;

		markerTbl[0xCC] = MARKER_DAC;

		for (int i = 0xCD; i <= 0xCF; i += 1)
			markerTbl[i] = MARKER_SOF;

		for (int i = 0xD0; i <= 0xD7; i += 1)
			markerTbl[i] = MARKER_RST;

		markerTbl[0xD8] = MARKER_SOI;
		markerTbl[0xD9] = MARKER_EOI;
		markerTbl[0xDA] = MARKER_SOS;
		markerTbl[0xDB] = MARKER_DQT;
		markerTbl[0xDC] = MARKER_DNL;
		markerTbl[0xDD] = MARKER_DRI;
		markerTbl[0xDE] = MARKER_DHP;
		markerTbl[0xDF] = MARKER_EXP;

		for (int i = 0xE0; i <= 0xEF; i += 1)
			markerTbl[i] = MARKER_APP;

		for (int i = 0xF0; i <= 0xFD; i += 1)
			markerTbl[i] = MARKER_JPG;

		markerTbl[0xFE] = MARKER_COM;
		markerTbl[0xFF] = MARKER_INV;
		bParseJpegHdrInit = false;

	}
	
	PROFILE_END("bParseJpegHdrInit");
	jobInfoZero( pJobInfo );
	uint32_t inPicSize = pJobInfo->m_inPicSize;

	uint8_t *pInPtr = pInPic;
	uint8_t * pInRstBase = 0;
	PROFILE_START("PARSEHDR");
	do {
		if (*pInPtr++ != 0xff)
			return "unexpected file format";

		uint8_t marker = *pInPtr++;
		switch (markerTbl[marker]) {
		case MARKER_INV:
			debugMsg("INV\n");
			return "INV marker found";
		case MARKER_TEM:
			debugMsg("TEM\n");
			return "TEM marker found";
		case MARKER_RES:
			debugMsg("RES\n");
			return "RES marker found";
		case MARKER_SOF:
			PROFILE_START("MARKER_SOF")
			{
				
				debugMsg("SOF%d\n", marker - 0xC0);
				int P = getJpegHdrByte(pInPtr+2);
				debugMsg("  P = %d\n", P);
				if (P != 8)
					return "SOF.P != 8";
				pJobInfo->m_dec.m_imageRows = getJpegHdrWord(pInPtr+3);
				debugMsg("  Y = %d\n", pJobInfo->m_dec.m_imageRows);
				pJobInfo->m_dec.m_imageCols = getJpegHdrWord(pInPtr+5);
				debugMsg("  X = %d\n", pJobInfo->m_dec.m_imageCols);
				pJobInfo->m_dec.m_compCnt = getJpegHdrByte(pInPtr+7);
				debugMsg("  Nf = %d\n", pJobInfo->m_dec.m_compCnt);

				// limit is the size of the pntInfo array
				if (pJobInfo->m_dec.m_imageCols >= (IMAGE_ROWS-PNT_WGHT_SIZE) ||
						pJobInfo->m_dec.m_imageRows >= (IMAGE_COLS-PNT_WGHT_SIZE))
					return "exceeded maximum supported image size";

				uint8_t * pTmpPtr = pInPtr+8;
				for (int ci = 0; ci < pJobInfo->m_dec.m_compCnt; ci += 1) {
					pJobInfo->m_dec.m_dcp[ci].m_compId = getJpegHdrByte(pTmpPtr);
					debugMsg("    C%d = %d\n", ci, pJobInfo->m_dec.m_dcp[ci].m_compId);
					pJobInfo->m_dec.m_dcp[ci].m_blkColsPerMcu = getJpegHdrNibU(pTmpPtr+1);
					debugMsg("    H%d = %d\n", ci, pJobInfo->m_dec.m_dcp[ci].m_blkColsPerMcu);
					pJobInfo->m_dec.m_dcp[ci].m_blkRowsPerMcu = getJpegHdrNibL(pTmpPtr+1);
					debugMsg("    V%d = %d\n", ci, pJobInfo->m_dec.m_dcp[ci].m_blkRowsPerMcu);
					pJobInfo->m_dec.m_dcp[ci].m_dqtId = getJpegHdrByte(pTmpPtr+2);
					debugMsg("    Tq%d = %d\n", ci, pJobInfo->m_dec.m_dcp[ci].m_dqtId);
					pTmpPtr += 3;
				}

				pInPtr += getJpegHdrWord(pInPtr);
				assert(pTmpPtr == pInPtr);
				
			}
			PROFILE_END("MARKER_SOF")
			break;
		case MARKER_DHT:
			PROFILE_START("MARKER_DHT");
			{
				debugMsg("DHT\n");
				uint8_t * pTmpPtr = pInPtr+2;
				do {
					uint8_t tc = getJpegHdrNibU(pTmpPtr);	// 0 DC, 1 AC
					debugMsg("  Tc = %d\n", tc);
					uint8_t th = getJpegHdrNibL(pTmpPtr);
					debugMsg("  Th = %d\n", th);
					JobDht & dht = tc == 0 ? pJobInfo->m_dec.m_dcDht[th] : pJobInfo->m_dec.m_acDht[th];
					dht.m_bUsed = true;

					debugMsg("  L = { ");
					int cnt = 0;
					for (int i = 0; i < 16; i += 1) {
						cnt += dht.m_bits[i] = getJpegHdrByte(pTmpPtr+1+i);
						debugMsg("%d ", dht.m_bits[i]);
					}
					debugMsg("}\n");
					assert(cnt <= 256);
					pTmpPtr += 17;
					int base = 0;
					for (int i = 0; i < 16; i += 1) {
						debugMsg("  V%d = { ", i);
						for (int j = 0; j < dht.m_bits[i]; j += 1) {
							dht.m_dhtTbls[base + j].m_huffCode = getJpegHdrByte(pTmpPtr++);
							debugMsg("%3d ", dht.m_dhtTbls[base + j].m_huffCode);
							if (j % 16 == 15 && j < dht.m_bits[i]-1)
								debugMsg("\n          ");
						}
						base += dht.m_bits[i];
						debugMsg("}\n");
					}
				} while (pTmpPtr < pInPtr + getJpegHdrWord(pInPtr));
				pInPtr += getJpegHdrWord(pInPtr);
				assert(pInPtr == pTmpPtr);
				
			}
			PROFILE_END("MARKER_DHT");
			break;
		case MARKER_DAC:
			debugMsg("DAC\n");
			return "DAC marker found";
		case MARKER_RST:
		  	PROFILE_START("MARKER_RST");
			{
				debugMsg("RST%d\n", marker - 0xD0);
				if (pJobInfo->m_dec.m_rstCnt >= MAX_RST_LIST_SIZE)
					return "Exceeded maximum supported restart marker count";

				int ri = pJobInfo->m_dec.m_rstCnt;
				pJobInfo->m_dec.m_rstInfo[ri].m_u64 = 0;
				pJobInfo->m_dec.m_rstInfo[ri].m_offset = pInPtr - pJobInfo->m_dec.m_pRstBase;
				pJobInfo->m_dec.m_rstCnt += 1;

				// skip to next marker
				while (pInPtr - pInPic < inPicSize && (pInPtr[0] != 0xFF || pInPtr[1] == 0x00))
					pInPtr += 1;
			}
			PROFILE_END("MARKER_RST");
			break;
		case MARKER_SOI:
			debugMsg("SOI\n");
			break;
		case MARKER_EOI:
			{
				debugMsg("EOI\n");
//    some images have data after EOI, especially if they have been processed with Windows and stored with CIFS
//				if (pInPtr - pInPic < inPicSize)
//					printf("Warning - extra data found after EOI\n");
			}
			return 0;
		case MARKER_SOS:
		  	PROFILE_START("MARKER_SOS");
			{
				debugMsg("SOS\n");
				int Ns = getJpegHdrByte(pInPtr+2);
				debugMsg("  Ns = %d\n", Ns);
				if (Ns != pJobInfo->m_dec.m_compCnt)
					return "SOS.Ns != SOF.Nf";
				uint8_t * pTmpPtr = pInPtr+3;
				PROFILE_START("COMPIDs");
				for (int ci = 0; ci < pJobInfo->m_dec.m_compCnt; ci += 1) {
					uint8_t compId = getJpegHdrByte(pTmpPtr);
					debugMsg("  Cs%d = %d\n", ci, pJobInfo->m_dec.m_dcp[ci].m_compId);
					pJobInfo->m_dec.m_dcp[ci].m_dcDhtId = getJpegHdrNibU(pTmpPtr+1);
					debugMsg("  Td%d = %d\n", ci, pJobInfo->m_dec.m_dcp[ci].m_dcDhtId);
					pJobInfo->m_dec.m_dcp[ci].m_acDhtId = getJpegHdrNibL(pTmpPtr+1);
					debugMsg("  Ta%d = %d\n", ci, pJobInfo->m_dec.m_dcp[ci].m_acDhtId);
					pTmpPtr += 2;

					if (pJobInfo->m_dec.m_dcp[ci].m_compId != compId)
						return "SOF/SOS order of compIds don't match";
				}
				PROFILE_END("COMPIDs")
				int Ss = getJpegHdrByte(pTmpPtr);
				debugMsg("  Ss = %d\n", Ss);
				int Se = getJpegHdrByte(pTmpPtr+1);
				debugMsg("  Se = %d\n", Se);
				int Ah = getJpegHdrNibU(pTmpPtr+2);
				debugMsg("  Ah = %d\n", Ah);
				int Al = getJpegHdrNibL(pTmpPtr+2);
				debugMsg("  Al = %d\n", Al);

				if (Ss != 0 || Se != 63 || Ah != 0 || Al != 0)
					return "unsupported SOS.Ss, SOS.Se, SOS.Ah, SOS.Al";

				pInPtr += getJpegHdrWord(pInPtr);
				assert(pInPtr == pTmpPtr+3);

				JobDec & dec = pJobInfo->m_dec;
				JobEnc & enc = pJobInfo->m_enc;

				dec.m_maxBlkColsPerMcu = 0;
				dec.m_maxBlkRowsPerMcu = 0;

				PROFILE_START("BLKPERMCU")
				for (int ci = 0; ci < dec.m_compCnt; ci += 1) {
					dec.m_maxBlkColsPerMcu = MAX( dec.m_maxBlkColsPerMcu, dec.m_dcp[ci].m_blkColsPerMcu );
					dec.m_maxBlkRowsPerMcu = MAX( dec.m_maxBlkRowsPerMcu, dec.m_dcp[ci].m_blkRowsPerMcu );
				}
				PROFILE_END("BLKPERMCU")
				dec.m_mcuRows = jdivRoundUp( dec.m_imageRows, dec.m_maxBlkRowsPerMcu * DCTSIZE );
				dec.m_mcuCols = jdivRoundUp( dec.m_imageCols, dec.m_maxBlkColsPerMcu * DCTSIZE );

				dec.m_rstMcuCntOrig = 0;

				// calculate derived huffman table info
				calcDhtDerivedTbl( dec.m_dcDht[0] );
				calcDhtDerivedTbl( dec.m_dcDht[1] );
				calcDhtDerivedTbl( dec.m_acDht[0] );
				calcDhtDerivedTbl( dec.m_acDht[1] );

				//if (dec.m_rstMcuCnt > 0 && (dec.m_rstMcuCnt != dec.m_mcuCols))
				//	return("image has restarts but not 1 restart per MCU column");

				assert(dec.m_rstCnt == 0);
				//dec.m_pRstBase = pInPtr;
				dec.m_rstInfo[0].m_u64 = 0;
				//dec.m_rstCnt += 1;

				pInRstBase = pInPtr;
				uint8_t * pRstPtr = dec.m_pRstBase;

				if ((dec.m_rstMcuCnt > 0 && (dec.m_rstMcuCnt == dec.m_mcuCols)) ||
						(dec.m_rstMcuCnt == 0 && !addRestarts)) {
					// restarts are already in images just copy to restart buffer
					PROFILE_START("RESTART_COPY")
					for (;;) {
						while (pInPtr - pInPic < inPicSize && (pInPtr[0] != 0xFF || pInPtr[1] == 0x00))
							*pRstPtr++ = *pInPtr++;
						dec.m_rstCnt += 1;
						debugMsg("rstCnt=%d offset=%d\n",
								(int)dec.m_rstCnt, (int)(pInPtr - pInRstBase + 2));
						dec.m_rstInfo[ dec.m_rstCnt ].m_u64 = 0;
						dec.m_rstInfo[ dec.m_rstCnt ].m_offset = pInPtr - pInRstBase + 2;

						*pRstPtr++ = pInPtr[0];
						*pRstPtr++ = pInPtr[1];

						if (pInPtr - pInPic >= inPicSize || (pInPtr[0] == 0xFF && !(pInPtr[1] >= 0xD0 && pInPtr[1] <= 0xD7)))
							break;

						pInPtr += 2;
					}
					PROFILE_END("RESTART_COPY")
				} else {
					PROFILE_START("RESTART_CALC")
					compute_huffman_table(enc.m_dcDht[0].m_huffCode, enc.m_dcDht[0].m_huffSize, dec.m_dcDht[0]);
					compute_huffman_table(enc.m_acDht[0].m_huffCode, enc.m_acDht[0].m_huffSize, dec.m_acDht[0]);
					if (pJobInfo->m_dec.m_compCnt > 1)
					{
						compute_huffman_table(enc.m_dcDht[1].m_huffCode, enc.m_dcDht[1].m_huffSize, dec.m_dcDht[1]);
						compute_huffman_table(enc.m_acDht[1].m_huffCode, enc.m_acDht[1].m_huffSize, dec.m_acDht[1]);
					}

					if (dec.m_rstMcuCnt == 4) {
					        PROFILE_START("RESTART_BREAK");
						// not the correct number of restarts. break restarts at row boundaries into 2
						// allow coproc to handle multiple restarts per row
						int mcuRow = 0;
						int mcuCol = 0;

						for (;;) {
							if ((mcuCol+dec.m_rstMcuCnt) < dec.m_mcuCols) {
								// just copy these to restart buffer
								while (pInPtr - pInPic < inPicSize && (pInPtr[0] != 0xFF || pInPtr[1] == 0x00))
									*pRstPtr++ = *pInPtr++;

								*pRstPtr++ = pInPtr[0];
								*pRstPtr++ = pInPtr[1];

								mcuCol += dec.m_rstMcuCnt;
							} else {
								// this one is potentially split before copying
								JpegHuffDec huffDec( pJobInfo, pInPtr, pRstPtr );
								jpegDecodeChangeRst( pJobInfo, huffDec, mcuRow, mcuCol);
								pInPtr = huffDec.getInPtr();
								if (mcuCol > 0 && mcuCol < dec.m_rstMcuCnt) {
									pRstPtr = huffDec.addRstMarker();
								} else {
									pRstPtr = huffDec.getOutPtr();
								}
							}
							if (pInPtr - pInPic >= inPicSize || (pInPtr[0] == 0xFF && !(pInPtr[1] >= 0xD0 && pInPtr[1] <= 0xD7)))
								break;
							pInPtr += 2;
						}
						dec.m_rstMcuCntOrig = dec.m_rstMcuCnt;
						dec.m_rstMcuCnt = dec.m_mcuCols;
						PROFILE_START("RESTART_BREAK");
					} else if (dec.m_rstMcuCnt > 0) {
						PROFILE_START("RESTART_INS");
						// not the correct number of restarts. decode the image and insert 1 restart per row

						int mcuRow = 0;
						int mcuCol = 0;

						JpegHuffDec huffDec( pJobInfo, pInPtr, pRstPtr );
						for (;;) {
							huffDec.setStartPtr(pInPtr);
							jpegDecodeChangeRst( pJobInfo, huffDec, mcuRow, mcuCol);
							pInPtr = huffDec.getInPtr();
							if (pInPtr - pInPic >= inPicSize || (pInPtr[0] == 0xFF && !(pInPtr[1] >= 0xD0 && pInPtr[1] <= 0xD7)))
								break;
							pInPtr+=2;
							if (pInPtr[0] == 0xFF && pInPtr[1] == 0xD9)
								break;
						}
						dec.m_rstMcuCnt = dec.m_mcuCols;
						PROFILE_END("RESTART_INS");
					} else {
						// decode images and insert restart markers as the image data is copied to restart buffer
						PROFILE_START("RESTART_DEC_INS");
						JpegHuffDec huffDec( pJobInfo, pInPtr, pRstPtr );

						for (int mcuRow = 0; mcuRow < dec.m_mcuRows; mcuRow += 1) {

							PROFILE_FUNC(jpegDecodeRst( pJobInfo, huffDec ));

							PROFILE_FUNC(pRstPtr = huffDec.addRstMarker());

							dec.m_rstCnt += 1;
							dec.m_rstInfo[ dec.m_rstCnt ].m_u64 = 0;
							dec.m_rstInfo[ dec.m_rstCnt ].m_offset = pRstPtr - dec.m_pRstBase;
						}

						PROFILE_FUNC(pInPtr = huffDec.getInPtr());

						dec.m_rstMcuCnt = dec.m_mcuCols;
						PROFILE_END("RESTART_DEC_INS");
					}
					PROFILE_END("RESTART_CALC");
				}
				if (dec.m_rstCnt>MAX_RST_LIST_SIZE)
					return("restart count greater than max number supported");
			}			
			PROFILE_END("MARKER_SOS");
			break;
		case MARKER_DQT:
			PROFILE_START("MARKER_DQT");		  
			{
				debugMsg("DQT\n");
				uint8_t * pTmpPtr = pInPtr+2;
				do {
					int P = getJpegHdrNibU(pTmpPtr);
					debugMsg("  P = %d\n", P);
					if (P != 0)
						return "DQT.P != 0";
					uint8_t Tq = getJpegHdrNibL(pTmpPtr);
					debugMsg("  Tq = %d\n", Tq);
					pJobInfo->m_dec.m_dqtValid |= 1LL << Tq;
					JobDqt & dqt = pJobInfo->m_dec.m_dqt[Tq];
					pTmpPtr += 1;
					debugMsg("  Q = { ");
					for (int i = 0; i < 64; i += 1) {
						uint16_t Qk = P == 0 ? getJpegHdrByte(pTmpPtr) : getJpegHdrWord(pTmpPtr);
						dqt.m_quantTbl[jpegZigZag[i].m_r][jpegZigZag[i].m_c] = Qk;
						pTmpPtr += P == 0 ? 1 : 2;
						debugMsg("%3d ", Qk);
						if (i % 16 == 15 && i != 63)
							debugMsg("\n        ");
					}
					debugMsg("}\n");
				} while (pTmpPtr < pInPtr + getJpegHdrWord(pInPtr));
				pInPtr += getJpegHdrWord(pInPtr);
				assert(pInPtr == pTmpPtr);
			}
			PROFILE_END("MARKER_DQT");			
			break;
		case MARKER_DNL:
			debugMsg("DNL\n");
			return "DNL marker found";
		case MARKER_DRI:
			debugMsg("DRI\n");
			pJobInfo->m_dec.m_rstMcuCnt = getJpegHdrWord(pInPtr+2);
			debugMsg("  Ri = %d\n", pJobInfo->m_dec.m_rstMcuCnt);
			pInPtr += getJpegHdrWord(pInPtr);
			break;
		case MARKER_DHP:
			debugMsg("DHP\n");
			return "DHP marker found";
		case MARKER_EXP:
			debugMsg("EXP\n");
			return "EXP marker found";
		case MARKER_APP:
		  	PROFILE_START("MARKER_APP");
			{
				debugMsg("APP\n");
				if (pJobInfo->m_app.m_appCnt < MAX_APP_MARKER_CNT) {
					int appIdx = pJobInfo->m_app.m_appCnt++;
					pJobInfo->m_app.m_appMarker[appIdx] = marker;
					pJobInfo->m_app.m_appLength[appIdx] = getJpegHdrWord(pInPtr);
					pJobInfo->m_app.m_pAppBase[appIdx] = pInPtr + 2;
				}
				debugMsg("  L = %d\n", getJpegHdrWord(pInPtr));
				pInPtr += getJpegHdrWord(pInPtr);
			}
			PROFILE_END("MARKER_APP");			
			break;
		case MARKER_JPG:
			debugMsg("JPG\n");
			return "JPG marker found";
		case MARKER_COM:
			PROFILE_START("MARKER_COM");
			debugMsg("COM\n");
			if (pJobInfo->m_com.m_comCnt++ < MAX_APP_MARKER_CNT) {
				pJobInfo->m_com.m_comMarker = marker;
				pJobInfo->m_com.m_comLength = getJpegHdrWord(pInPtr);
				pJobInfo->m_com.m_pComBase = pInPtr + 2;
			} else {
				return "more than one COM marker found";
			}
			debugMsg("  L = %d\n", getJpegHdrWord(pInPtr));
			pInPtr += getJpegHdrWord(pInPtr);
			PROFILE_END("MARKER_COM");
			break;
		default:
			assert(0);
		}

	} while (pInPtr - pInPic < inPicSize);
	PROFILE_END("PARSEHDR");
	return 0;
}

void jpegWriteFile( JobInfo * pJobInfo, FILE *outFp )
{
	// SOI marker
	putJpegHdrWord( outFp, 0xFFD8 );

	// COM marker
	if (pJobInfo->m_com.m_comCnt > 0) {
		putJpegHdrWord( outFp, 0xFF00 | pJobInfo->m_com.m_comMarker );
		putJpegHdrWord( outFp, pJobInfo->m_com.m_comLength );

		fwrite( pJobInfo->m_com.m_pComBase, pJobInfo->m_com.m_comLength-2, 1, outFp );
	}

	// APP markers
	for (uint32_t i = 0; i < pJobInfo->m_app.m_appCnt; i += 1) {
		putJpegHdrWord( outFp, 0xFF00 | pJobInfo->m_app.m_appMarker[i] );
		putJpegHdrWord( outFp, pJobInfo->m_app.m_appLength[i] );

		fwrite( pJobInfo->m_app.m_pAppBase[i], pJobInfo->m_app.m_appLength[i]-2, 1, outFp );
	}

	// DQT marker
	for (int i = 0; i < 4; i += 1) {
		if (!(pJobInfo->m_dec.m_dqtValid & (1LL << i))) continue;
		JobDqt * pDqt = &pJobInfo->m_dec.m_dqt[i];	// decode tables are used for encoding

		putJpegHdrWord( outFp, 0xFFDB );

		int length = 67;
		putJpegHdrWord( outFp, length );

		int P = 0;
		int Tq = i;
		putJpegHdrNib( outFp, P, Tq );

		for (int k = 0; k < 64; k += 1) {
			uint8_t Qk = (uint8_t)pDqt->m_quantTbl[jpegZigZag[k].m_r][jpegZigZag[k].m_c];
			fwrite( &Qk, sizeof(uint8_t), 1, outFp );
		}
	}

	// SOF0 marker
	{
		putJpegHdrWord( outFp, 0xFFC0 );

		int sofLen = 8 + 3 * (int)pJobInfo->m_enc.m_compCnt;
		putJpegHdrWord( outFp, sofLen );

		int P = 8;
		putJpegHdrByte( outFp, P );

		int Y = pJobInfo->m_enc.m_imageRows;
		putJpegHdrWord( outFp, Y );

		int X = pJobInfo->m_enc.m_imageCols;
		putJpegHdrWord( outFp, X );

		int Nf = pJobInfo->m_enc.m_compCnt;
		putJpegHdrByte( outFp, Nf );

		for (int ci = 0; ci < Nf; ci += 1) {
			putJpegHdrByte( outFp, pJobInfo->m_enc.m_ecp[ci].m_compId );

			putJpegHdrNib( outFp, pJobInfo->m_enc.m_ecp[ci].m_blkColsPerMcu,
				pJobInfo->m_enc.m_ecp[ci].m_blkRowsPerMcu );

			putJpegHdrByte( outFp, pJobInfo->m_enc.m_ecp[ci].m_dqtId );
		}
	}

	// DHT marker
	for (int i = 0; i < 4; i += 1) {
		JobEncDht * pDht;
		int tc, th;
		uint8_t *pBits;
		uint8_t * pVal;
		switch (i) {
		case 0: pDht = &pJobInfo->m_enc.m_dcDht[0]; tc = 0; th = 0; pBits = s_dc_lum_bits; pVal = s_dc_lum_val; break;
		case 1: pDht = &pJobInfo->m_enc.m_dcDht[1]; tc = 0; th = 1; pBits = s_dc_chroma_bits; pVal = s_dc_chroma_val; break;
		case 2: pDht = &pJobInfo->m_enc.m_acDht[0]; tc = 1; th = 0; pBits = s_ac_lum_bits; pVal = s_ac_lum_val; break;
		case 3: pDht = &pJobInfo->m_enc.m_acDht[1]; tc = 1; th = 1; pBits = s_ac_chroma_bits; pVal = s_ac_chroma_val; break;
		}

		putJpegHdrWord( outFp, 0xFFC4 );

		uint16_t bitsSum = 0;
		for (int j = 1; j <= 16; j += 1)
			bitsSum += pBits[j];

		uint16_t dhtLen = 19 + bitsSum;
		putJpegHdrWord( outFp, dhtLen );

		putJpegHdrNib( outFp, tc, th );

		for (int j = 1; j <= 16; j += 1)
			putJpegHdrByte( outFp, pBits[j] );

		fwrite( pVal, bitsSum, 1, outFp );
	}

	// DRI marker
	{
		putJpegHdrWord( outFp, 0xFFDD );

		int driLen = 4;
		putJpegHdrWord( outFp, driLen );

		putJpegHdrWord( outFp, (uint16_t)(pJobInfo->m_enc.m_mcuRowsPerRst * pJobInfo->m_enc.m_mcuCols) );
	}

	// SOS marker
	{
		putJpegHdrWord( outFp, 0xFFDA );

		int sosLen = 6 + 2 * (int)pJobInfo->m_enc.m_compCnt;
		putJpegHdrWord( outFp, sosLen );

		putJpegHdrByte( outFp, pJobInfo->m_enc.m_compCnt );

		for (int ci = 0; ci < pJobInfo->m_enc.m_compCnt; ci += 1) {
			putJpegHdrByte( outFp, pJobInfo->m_enc.m_ecp[ci].m_compId );
		
			putJpegHdrNib( outFp, pJobInfo->m_enc.m_ecp[ci].m_dcDhtId,
				pJobInfo->m_enc.m_ecp[ci].m_acDhtId );
		}

		putJpegHdrByte( outFp, 0 );
		putJpegHdrByte( outFp, 63 );
		putJpegHdrNib( outFp, 0, 0 );

		uint8_t * pEncData = pJobInfo->m_enc.m_pRstBase;
		int encDataLen = pJobInfo->m_enc.m_rstLength[0];
		fwrite( pEncData, encDataLen, 1, outFp );
	}

	// RST markers
	for (int i = 1; i < pJobInfo->m_enc.m_rstCnt; i += 1) {
		putJpegHdrWord( outFp, 0xFFD0 + (i-1)%8 );

		uint8_t * pEncData = pJobInfo->m_enc.m_pRstBase + i * pJobInfo->m_enc.m_rstOffset;
		int encDataLen = pJobInfo->m_enc.m_rstLength[i];
		fwrite( pEncData, encDataLen, 1, outFp );
	}

	// EOI marker
	putJpegHdrWord( outFp, 0xFFD9 );
}

unsigned int jpegWriteBuffer( JobInfo * pJobInfo,  uint8_t *& buffer )
{
	uint8_t * workBuffer;
	unsigned int bufferSize=sizeof(uint16_t);
	buffer=(uint8_t *)malloc(sizeof(uint16_t));
	workBuffer=buffer;
	// SOI marker
	putJpegHdrWord( workBuffer, 0xFFD8 );

	//Allocate enough space for the markers and lengths.  We will add in the data as we go
	// COM marker
	if (pJobInfo->m_com.m_comCnt > 0) {
		bufferSize+=pJobInfo->m_com.m_comCnt * 2 * sizeof(uint16_t);
		expandBuffer(buffer, workBuffer, bufferSize);

		putJpegHdrWord( workBuffer, 0xFF00 | pJobInfo->m_com.m_comMarker );
		putJpegHdrWord( workBuffer, pJobInfo->m_com.m_comLength );

		bufferSize+=pJobInfo->m_com.m_comLength-2;
		expandBuffer(buffer, workBuffer, bufferSize);
		memcpy(workBuffer, pJobInfo->m_com.m_pComBase, pJobInfo->m_com.m_comLength-2);
		workBuffer+=pJobInfo->m_com.m_comLength-2;
	}

	// APP markers
	bufferSize+=pJobInfo->m_app.m_appCnt * 2 * sizeof(uint16_t);
	expandBuffer(buffer, workBuffer, bufferSize);
	for (uint32_t i = 0; i < pJobInfo->m_app.m_appCnt; i += 1) {
		putJpegHdrWord( workBuffer, 0xFF00 | pJobInfo->m_app.m_appMarker[i] );
		putJpegHdrWord( workBuffer, pJobInfo->m_app.m_appLength[i] );

		bufferSize+=pJobInfo->m_app.m_appLength[i]-2;
		expandBuffer(buffer, workBuffer, bufferSize);
		memcpy(workBuffer, pJobInfo->m_app.m_pAppBase[i], pJobInfo->m_app.m_appLength[i]-2);
		workBuffer+=pJobInfo->m_app.m_appLength[i]-2;
	}

	// DQT marker
	for (int i = 0; i < 4; i += 1) {
		if (!(pJobInfo->m_dec.m_dqtValid & (1LL << i))) continue;
		bufferSize+=sizeof(uint16_t)*2 + sizeof(uint8_t) * 65;
		expandBuffer(buffer, workBuffer, bufferSize);
		JobDqt * pDqt = &pJobInfo->m_dec.m_dqt[i];	// decode tables are used for encoding

		putJpegHdrWord( workBuffer, 0xFFDB );

		int length = 67;
		putJpegHdrWord( workBuffer, length );

		int P = 0;
		int Tq = i;
		putJpegHdrNib( workBuffer, P, Tq );

		for (int k = 0; k < 64; k += 1) {
			*workBuffer = (uint8_t)pDqt->m_quantTbl[jpegZigZag[k].m_r][jpegZigZag[k].m_c];
			workBuffer++;
		}
	}

	// SOF0 marker
	{
		bufferSize+=sizeof(uint16_t)*4 + sizeof(uint8_t)*2;
		expandBuffer(buffer, workBuffer, bufferSize);
		putJpegHdrWord( workBuffer, 0xFFC0 );

		int sofLen = 8 + 3 * (int)pJobInfo->m_enc.m_compCnt;
		putJpegHdrWord( workBuffer, sofLen );

		int P = 8;
		putJpegHdrByte( workBuffer, P );

		int Y = pJobInfo->m_enc.m_imageRows;
		putJpegHdrWord( workBuffer, Y );

		int X = pJobInfo->m_enc.m_imageCols;
		putJpegHdrWord( workBuffer, X );

		int Nf = pJobInfo->m_enc.m_compCnt;
		putJpegHdrByte( workBuffer, Nf );

		bufferSize+=Nf*(3*sizeof(uint8_t));
		expandBuffer(buffer, workBuffer, bufferSize);
		for (int ci = 0; ci < Nf; ci += 1) {
			putJpegHdrByte( workBuffer, pJobInfo->m_enc.m_ecp[ci].m_compId );

			putJpegHdrNib( workBuffer, pJobInfo->m_enc.m_ecp[ci].m_blkColsPerMcu,
				pJobInfo->m_enc.m_ecp[ci].m_blkRowsPerMcu );

			putJpegHdrByte( workBuffer, pJobInfo->m_enc.m_ecp[ci].m_dqtId );
		}
	}

	// DHT marker
	bufferSize+=4*(sizeof(uint16_t)*2+sizeof(uint8_t)*17);
	expandBuffer(buffer, workBuffer, bufferSize);
	for (int i = 0; i < 4; i += 1) {
		JobEncDht * pDht;
		int tc, th;
		uint8_t *pBits;
		uint8_t * pVal;
		switch (i) {
		case 0: pDht = &pJobInfo->m_enc.m_dcDht[0]; tc = 0; th = 0; pBits = s_dc_lum_bits; pVal = s_dc_lum_val; break;
		case 1: pDht = &pJobInfo->m_enc.m_dcDht[1]; tc = 0; th = 1; pBits = s_dc_chroma_bits; pVal = s_dc_chroma_val; break;
		case 2: pDht = &pJobInfo->m_enc.m_acDht[0]; tc = 1; th = 0; pBits = s_ac_lum_bits; pVal = s_ac_lum_val; break;
		case 3: pDht = &pJobInfo->m_enc.m_acDht[1]; tc = 1; th = 1; pBits = s_ac_chroma_bits; pVal = s_ac_chroma_val; break;
		}

		putJpegHdrWord( workBuffer, 0xFFC4 );

		uint16_t bitsSum = 0;
		for (int j = 1; j <= 16; j += 1)
			bitsSum += pBits[j];

		uint16_t dhtLen = 19 + bitsSum;
		putJpegHdrWord( workBuffer, dhtLen );

		putJpegHdrNib( workBuffer, tc, th );

		for (int j = 1; j <= 16; j += 1)
			putJpegHdrByte( workBuffer, pBits[j] );

		bufferSize+=bitsSum;
		expandBuffer(buffer, workBuffer, bufferSize);
		memcpy(workBuffer, pVal, bitsSum);
		workBuffer+=bitsSum;
	}

	// DRI marker
	{
	        bufferSize+=3*sizeof(uint16_t);
		expandBuffer(buffer, workBuffer, bufferSize);
		putJpegHdrWord( workBuffer, 0xFFDD );

		int driLen = 4;
		putJpegHdrWord( workBuffer, driLen );

		putJpegHdrWord( workBuffer, (uint16_t)(pJobInfo->m_enc.m_mcuRowsPerRst * pJobInfo->m_enc.m_mcuCols) );
	}

	// SOS marker
	{
		bufferSize+=2*sizeof(uint16_t)+4*sizeof(uint8_t)+(2*(uint32_t)pJobInfo->m_enc.m_compCnt*sizeof(uint8_t));
		expandBuffer(buffer, workBuffer, bufferSize);
		putJpegHdrWord( workBuffer, 0xFFDA );

		int sosLen = 6 + 2 * (int)pJobInfo->m_enc.m_compCnt;
		putJpegHdrWord( workBuffer, sosLen );

		putJpegHdrByte( workBuffer, pJobInfo->m_enc.m_compCnt );

		for (int ci = 0; ci < pJobInfo->m_enc.m_compCnt; ci += 1) {
			putJpegHdrByte( workBuffer, pJobInfo->m_enc.m_ecp[ci].m_compId );
		
			putJpegHdrNib( workBuffer, pJobInfo->m_enc.m_ecp[ci].m_dcDhtId,
				pJobInfo->m_enc.m_ecp[ci].m_acDhtId );
		}

		putJpegHdrByte( workBuffer, 0 );
		putJpegHdrByte( workBuffer, 63 );
		putJpegHdrNib( workBuffer, 0, 0 );

		uint8_t * pEncData = pJobInfo->m_enc.m_pRstBase;
		int encDataLen = pJobInfo->m_enc.m_rstLength[0];
		bufferSize+=encDataLen;
		expandBuffer(buffer, workBuffer, bufferSize);
		memcpy(workBuffer, pEncData, encDataLen);
		workBuffer+=encDataLen;
	}

	// RST markers
	for (int i = 1; i < pJobInfo->m_enc.m_rstCnt; i += 1) {
		int encDataLen = pJobInfo->m_enc.m_rstLength[i];
		bufferSize+=sizeof(uint16_t)+encDataLen;
		expandBuffer(buffer, workBuffer, bufferSize);
		putJpegHdrWord( workBuffer, 0xFFD0 + (i-1)%8 );

		uint8_t * pEncData = pJobInfo->m_enc.m_pRstBase + i * pJobInfo->m_enc.m_rstOffset;
		memcpy(workBuffer, pEncData, encDataLen);
		workBuffer+=encDataLen;
	}
	bufferSize+=sizeof(uint16_t);
	expandBuffer(buffer, workBuffer, bufferSize);
	// EOI marker
	putJpegHdrWord( workBuffer, 0xFFD9 );
	return bufferSize;
}
