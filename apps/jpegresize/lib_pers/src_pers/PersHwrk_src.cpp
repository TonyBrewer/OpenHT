/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(HORZ)

#include "Ht.h"
#include "PersHwrk.h"

#ifndef _HTV
extern bool IsCheck;
#endif

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

static ht_int10 descale(ht_int21 x, int n) {
	return ( (int)x + (1 << (n-1))) >> n; 
}

static uint8_t clampPixel(ht_int10 inInt)
{
	if (inInt < 0)
		return 0;
	if (inInt > 255)
		return 255;
	return (uint8_t) inInt;
}

void CPersHwrk::PersHwrk()
{
	S_hwmPop = false;
	S_hwmEmpty = SR_hwmPop || S_writeMsgQue.empty();
	S_hwm = S_writeMsgQue.front();
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case HWRK_ENTRY: {
			P1_loopCnt = 0;
			P1_endOfMcuRowCnt = 0;

			P1_blkRowsPerMcu = S_horz.m_hcp[0].m_blkRowsPerMcu + S_horz.m_hcp[1].m_blkRowsPerMcu + S_horz.m_hcp[2].m_blkRowsPerMcu;

			HtContinue(HWRK_INIT_HDS);
		}
		break;
		case HWRK_INIT_HDS: {

			HorzDecState hds;
			hds.m_bStartOfBlkRow = true;
			hds.m_bLastOfBlkRow = false;
			hds.m_mcuRow = 0;
			hds.m_mcuBlkRow = 0;

			S_hds.write_addr(P1_loopCnt & ((1<<DEC_IHUF_IDX_W)-1) );
			S_hds.write_mem( hds );

			if (P1_loopCnt+1 == (1 << DEC_IHUF_IDX_W)) {
				S_bStateRdy = true;
				HtContinue(HWRK_WRITE_LOOP);
			} else
				HtContinue(HWRK_INIT_HDS);

			P1_loopCnt += 1;
		}
		break;
		case HWRK_WRITE_LOOP: {
			BUSY_RETRY (WriteMemBusy() || SendMsgFull_hrm());

#if defined(FORCE_HORZ_WRITE_STALLS) && !defined(_HTV)
			static int stallCnt = 0;
			if (stallCnt < 20) {
				stallCnt += 1;
				BUSY_RETRY(true);
			} else
				stallCnt = 0;
#endif

			bool bEndOfImage = false;

			if (!SR_hwmEmpty) {
				HorzWriteMsg hwm = SR_hwm;

				T1_hwmRd = hwm;

				ht_uint48 memAddr = S_horz.m_hcp[hwm.m_compIdx].m_pOutCompBuf + (hwm.m_outPos << 6);
				TransBufIdx_t varAddr1 = hwm.m_transBlkIdx << 3;
				ht_uint4 qwCnt = 8;

				WriteMem_rslt(memAddr, varAddr1, (ht_uint9)qwCnt);

				if (hwm.m_bEndOfMcuRow) {

					P1_endOfCompRowCnt[hwm.m_decPipeId] += 1;
					if (P1_endOfCompRowCnt[hwm.m_decPipeId] == P1_blkRowsPerMcu) {
						P1_endOfCompRowCnt[hwm.m_decPipeId] = 0;

						P1_endOfMcuRowCnt += 1;

#ifndef _HTV
						printf("hrm(%d): cycle %d, mcuRow %d\n", (int)i_replId.read(), (int)HT_CYCLE(), (int)hwm.m_mcuRow);
						fflush(stdout);
#endif

						bEndOfImage = P1_endOfMcuRowCnt == S_horz.m_mcuRows;

						HorzResizeMsg hrm;
						hrm.m_imageHtId = P1_imageHtId;
						hrm.m_mcuRow = hwm.m_mcuRow;
						hrm.m_imageIdx = P1_imageIdx;
						hrm.m_bEndOfImage = bEndOfImage;

						if (P1_persMode == 3)
							// only send message if DH and VE are enabled
							SendMsg_hrm( hrm );
					}
				}

				S_hwmPop = true;
				S_writeMsgQue.pop();
			}

			if (bEndOfImage)
				WriteMemPause(HWRK_RETURN);
			else
				HtContinue(HWRK_WRITE_LOOP);
		}
		break;
		case HWRK_RETURN: {
			BUSY_RETRY (SendReturnBusy_hwrk());

			SendReturn_hwrk();
		}
		break;
		default:
			assert(0);
		}
	}

	////////////////////////
	// Data path

	FilterControl();

	// pipelined filter
	FilterDataPath();

	HandleWriteTransfer();
	RecvJobInfo();
}

void CPersHwrk::FilterControl()
{
	T2_bGenOutPixel = false;

	// perform T2 operations that feed into T1
#if !defined(_HTV) && defined(CHECK_WGHT_MATCH)
	ht_int14 t2_pntWghtStart = S_pntWghtStart.read_mem();
	// keep following for checking
	bool bOldPntWghtMatch = (t2_pntWghtStart >> 1) == (TR2_hrs.m_colDataPos >> 1) &&
		(TR2_hrs.m_bUpScale || (t2_pntWghtStart & 1) == (TR2_hrs.m_colDataPos & 1));
	ht_uint1 t2_bOldEven = (t2_pntWghtStart ^ 1) & 1;
#endif
	T2_hrs.m_bBlkRowComplete = TR2_outCol == S_horz.m_hcp[TR2_hds.m_compIdx].m_outCompCols;
	ht_uint4 pntWghtOut = S_pntWghtOut.read_mem();
	bool pntWghtOut0 = ((pntWghtOut >> 0) & 0x1) == 0x1;
	bool pntWghtOut1 = ((pntWghtOut >> 1) & 0x1) == 0x1;
	bool pntWghtOutUp0 = ((pntWghtOut >> 2) & 0x1) == 0x1;
	bool pntWghtOutUp1 = ((pntWghtOut >> 3) & 0x1) == 0x1;
	bool bPntWghtMatch = TR2_hrs.m_bUpScale ? (pntWghtOutUp0 || pntWghtOutUp1) :
			((TR2_inCol & 0x1) ? pntWghtOut1 : pntWghtOut0);

	T2_bGenOutPixel = TR2_bValid && !T2_hrs.m_bBlkRowComplete && bPntWghtMatch;

	T2_hrs.m_bEndOfMcuRow = bPntWghtMatch &&
		TR2_outCol == S_horz.m_hcp[TR2_hds.m_compIdx].m_outCompCols-1 &&
		TR2_hds.m_mcuBlkRow == S_horz.m_hcp[TR2_hds.m_compIdx].m_blkRowsPerMcu-1;

	ImageCols_t t2_outColNxt = TR2_bLastOfBlkRowHold ? (ImageCols_t)0 : (ImageCols_t)TR2_outCol;
	ImageCols_t t2_outColP1Nxt = TR2_bLastOfBlkRowHold ? (ImageCols_t)0 : (ImageCols_t)TR2_outColP1;

	ImageCols_t t2_outCol = T2_bGenOutPixel ? t2_outColP1Nxt : t2_outColNxt;

	T2_bEven = TR2_hrs.m_bUpScale ? (ht_uint1)pntWghtOutUp0 : (ht_uint1)((TR2_hrs.m_colDataPos ^ 1) & 1);

	if (TR2_bValid) {
#if !defined(_HTV) && defined(CHECK_WGHT_MATCH)
		if (!T2_hrs.m_bBlkRowComplete && bOldPntWghtMatch != bPntWghtMatch)
			fprintf(stderr, "PersHwrk: t2_pntWghtStart=%d colDataPos=%d inCol=%d bOldPntWghtMatch=%d bPntWghtMatch=%d bUpScale=%d @ %lld\n",
					(int)t2_pntWghtStart, (int)TR2_hrs.m_colDataPos, (int)T2_inCol, (int)bOldPntWghtMatch, (int)bPntWghtMatch,
					(int)TR2_hrs.m_bUpScale,
					HT_CYCLE());
		assert(T2_hrs.m_bBlkRowComplete || bOldPntWghtMatch == bPntWghtMatch);
		assert(!bPntWghtMatch || (T2_bEven == t2_bOldEven));
#endif
		S_outCol.write_addr( TR2_hrsAddr );
		S_outCol.write_mem( t2_outCol );
	}


	T1_bValid = false;

	// the queue must be kept small to avoid buffer being overwritten
	// queue is pushed on average once every 8 clocks, but there can
	// be short bursts
	// if single component we can only allow 2 cache lines in flight
	// to avoid overflowing colRslt buffer
	T1_bTransMsgQueFull = (S_horz.m_compCnt==1) ? (S_transMsgQue.size() > 2) :
			(S_transMsgQue.size() >= 4);

	JpegDecMsg t1_jdm = PeekMsg_jdm();

	S_hds.read_addr(t1_jdm.m_decPipeId);
	HorzDecState t1_hds = S_hds.read_mem();

	T1_hrsAddr = (t1_hds.m_compIdx << 4) | (t1_jdm.m_decPipeId << 1) | t1_hds.m_mcuBlkRow;

	S_hrs.read_addr( T1_hrsAddr );
	S_outCol.read_addr( T1_hrsAddr );

	HorzState t1_hrs = S_hrs.read_mem();
	ImageCols_t t1_outCol = t1_hds.m_bStartOfBlkRow ? (ImageCols_t)0 : S_outCol.read_mem();

	bool t1_bDiffBlkRow = !TR2_bValid || T1_hrsAddr != TR2_hrsAddr;
	T1_bDiffBlkRow = t1_bDiffBlkRow;

	T1_outCol = t1_bDiffBlkRow ? (ImageCols_t)t1_outCol : t2_outCol;

	T1_inCol = t1_hrs.m_inCol;
	T1_outColP1 = T1_outCol + 1;

	T1_outImageCol = (ImageCols_t)(T1_outCol << (t1_hrs.m_bUpScale ? 1 : 0));
#if !defined(_HTV) && defined(CHECK_WGHT_MATCH)
	S_pntWghtStart.read_addr(T1_outImageCol);
#endif
	S_pntWghtOut.read_addr(T1_inCol >> 1);

	T1_hds = t1_hds;
	T1_hrs = t1_hrs;
	T1_jdm = t1_jdm;

	T1_bLastOfBlkRowHold = t1_hds.m_bLastOfBlkRow && t1_hds.m_lastOfBlkRowCnt == 8;

	if (RecvMsgReady_jdm() && SR_bStateRdy && !TR2_bTransMsgQueFull) {

#if !defined(_HTV)
		{
			static int cnt = 0;
			if (t1_hds.m_compIdx != t1_jdm.m_compIdx)
				cnt += 1;
			if (cnt > 0) {
				cnt += 1;
				if (cnt == 20)
					assert(0);	// delay the assert so vcd gets written
			}
		}
#endif

		T1_bValid = !t1_hds.m_bStartOfBlkRow || S_bRowFirstProcessed;
		if (!T1_bValid) {

			t1_hrs.m_bUpScale = S_horz.m_maxBlkColsPerMcu == 2 && S_horz.m_hcp[t1_hds.m_compIdx].m_blkColsPerMcu == 1;
			t1_hrs.m_pntWghtStart = S_horz.m_pntWghtStart;

			if (t1_hrs.m_bUpScale) {
				ht_int6 filterWidth = (S_horz.m_filterWidth >> 1) + 1;
				ht_int6 filterOffset = (ht_int6)(18 - (filterWidth << 1));
				t1_hrs.m_colDataPos = t1_hrs.m_pntWghtStart < -filterOffset ? -18 : ((t1_hrs.m_pntWghtStart & ~1) - (filterWidth << 1));
			} else {
				ht_int6 filterOffset = (17 - S_horz.m_filterWidth);
				t1_hrs.m_colDataPos = t1_hrs.m_pntWghtStart < -filterOffset ? -17 : (t1_hrs.m_pntWghtStart - S_horz.m_filterWidth);
			}

			t1_hrs.m_bBlkRowComplete = false;

			if (t1_jdm.m_rstIdx > 0)
				t1_hds.m_mcuRow = t1_jdm.m_rstIdx;

			t1_hds.m_lastOfBlkRowCnt = 0;
			t1_hrs.m_inCol = 0;

			// don't pop the message from the queue, we will process it next clock
			S_bRowFirstProcessed = true;
		} else {
			S_bRowFirstProcessed = false;

			t1_hrs.m_inCol += t1_hrs.m_bUpScale ? 2 : 1;

			t1_hrs.m_colDataPos += t1_hrs.m_bUpScale ? 2 : 1;

			// pop the queue if not the last column in the row, or
			//   if the last output column has been processed. The last
			//   jdm for a row is used repetitively until the last
			//   output column is generated.
			T1_bJdmPop = !t1_hds.m_bLastOfBlkRow || T1_bLastOfBlkRowHold;

			if (t1_hds.m_bLastOfBlkRow)
				t1_hds.m_lastOfBlkRowCnt += 1;
			else
				t1_hds.m_lastOfBlkRowCnt = 0;

#if defined(CHECK_INBOUND_HORZ_MSG) && !defined(_HTV)
			
			if (g_IsCheck) {
				uint64_t inRow = (t1_hds.m_mcuRow * S_horz.m_hcp[t1_hds.m_compIdx].m_blkRowsPerMcu + t1_hds.m_mcuBlkRow) * DCTSIZE;
				uint64_t inCol = (t1_hds.m_mcuCol * S_horz.m_hcp[t1_hds.m_compIdx].m_blkColsPerMcu + t1_hds.m_mcuBlkCol) * DCTSIZE + t1_hds.m_blkCol;

				Eight_u8 chkData;
				bool bError = false;
				for (int r = 0; r < 8 && inRow + r < S_horz.m_hcp[t1_hds.m_compIdx].m_inCompRows; r += 1) {
					uint64_t pos = (inRow + r) * S_horz.m_hcp[t1_hds.m_compIdx].m_inCompBufCols + inCol;
					uint8_t * pInCompBuf = (uint8_t *)(uint64_t)S_horz.m_hcp[t1_hds.m_compIdx].m_pInCompBuf;
					chkData.m_u8[r] = *(pInCompBuf + pos);
					bError |= chkData.m_u8[r] != t1_jdm.m_data.m_u8[r];
				}
				if (bError) {
					printf("JpegDecMsg data error: ci %d, mcuRow %d, mcuBlkRow %d, mcuCol %d, mcuBlkCol %d:\n   expected 0x%016llx, actual 0x%016llx @ %lld\n",
						(int)t1_hds.m_compIdx, (int)t1_hds.m_mcuRow, (int)t1_hds.m_mcuBlkRow, (int)t1_hds.m_mcuCol, (int)t1_hds.m_mcuBlkCol,
						(long long)chkData.m_u64, (long long)t1_jdm.m_data.m_u64, HT_CYCLE());
				}
			}
#endif
			bool bBlkColZero = T1_bJdmPop && (t1_hds.m_blkCol+1 == DCTSIZE || T1_bLastOfBlkRowHold);
			bool bMcuBlkColZero = bBlkColZero && t1_hds.m_mcuBlkCol+1 == S_horz.m_hcp[t1_hds.m_compIdx].m_blkColsPerMcu;
			bool bMcuBlkRowZero = bMcuBlkColZero && t1_hds.m_mcuBlkRow+1 == S_horz.m_hcp[t1_hds.m_compIdx].m_blkRowsPerMcu;
			bool bCompIdxZero = bMcuBlkRowZero && t1_hds.m_compIdx+1 == S_horz.m_compCnt;
			bool bMcuColZero = bCompIdxZero && t1_hds.m_mcuCol == S_horz.m_mcuColsM1;

			t1_hds.m_bStartOfBlkRow = t1_hds.m_bStartOfBlkRow && !T1_bJdmPop ||
				!t1_hds.m_bStartOfBlkRow && bMcuBlkColZero && !bCompIdxZero && t1_hds.m_mcuCol == 0 ||
				!t1_hds.m_bStartOfBlkRow && bMcuColZero;

			t1_hds.m_bLastOfBlkRow = t1_hds.m_mcuCol == S_horz.m_mcuColsM1 &&
				t1_hds.m_mcuBlkCol+1 == S_horz.m_hcp[t1_hds.m_compIdx].m_blkColsPerMcu &&
				(!T1_bJdmPop && t1_hds.m_blkCol+1 == DCTSIZE || T1_bJdmPop && t1_hds.m_blkCol == DCTSIZE-2 && !T1_bLastOfBlkRowHold);

			// advance horizontal decode pos
			if (T1_bJdmPop) {
				if (t1_hds.m_blkCol+1 == DCTSIZE || T1_bLastOfBlkRowHold) {
					t1_hds.m_blkCol = 0;
					if (t1_hds.m_mcuBlkCol+1 == S_horz.m_hcp[t1_hds.m_compIdx].m_blkColsPerMcu) {
						t1_hds.m_mcuBlkCol = 0;
						if (t1_hds.m_mcuBlkRow+1 == S_horz.m_hcp[t1_hds.m_compIdx].m_blkRowsPerMcu) {
							t1_hds.m_mcuBlkRow = 0;
							if (t1_hds.m_compIdx+1 == S_horz.m_compCnt) {
								t1_hds.m_compIdx = 0;
								if (t1_hds.m_mcuCol == S_horz.m_mcuColsM1) {
									t1_hds.m_mcuCol = 0;
									if (t1_hds.m_mcuRow == S_horz.m_mcuRowsM1) {
										t1_hds.m_mcuRow = 0;
									} else
										t1_hds.m_mcuRow += 1;
								} else
									t1_hds.m_mcuCol += 1;
							} else
								t1_hds.m_compIdx += 1;
						} else
							t1_hds.m_mcuBlkRow += 1;
					} else
						t1_hds.m_mcuBlkCol += 1u;
				} else
					t1_hds.m_blkCol += 1;
			}

#if !defined(_HTV)
			bool bStartOfBlkRow = t1_hds.m_blkCol == 0 && t1_hds.m_mcuBlkCol == 0 && t1_hds.m_mcuCol == 0;
			assert(!SR_bStateRdy || t1_hds.m_bStartOfBlkRow == bStartOfBlkRow);

			bool bLastOfBlkRow = t1_hds.m_mcuCol == S_horz.m_mcuColsM1 &&
				t1_hds.m_mcuBlkCol+1 == S_horz.m_hcp[t1_hds.m_compIdx].m_blkColsPerMcu &&
				t1_hds.m_blkCol+1 == DCTSIZE;  // very last column of last block in mcu row
			assert(!SR_bStateRdy || t1_hds.m_bLastOfBlkRow == bLastOfBlkRow);
#endif

			if (T1_bJdmPop)
				RecvMsg_jdm();
		}

		S_hrs.write_addr( T1_hrsAddr );
		S_hrs.write_mem( t1_hrs );

		S_hds.write_addr(t1_jdm.m_decPipeId);
		S_hds.write_mem( t1_hds );
	}
}

void CPersHwrk::FilterDataPath()
{
	// perform T2 pipeline operations
	S_pntWghtIdx.read_addr(TR2_outImageCol);

	T3_outMcuCol = S_horz.m_hcp[TR3_hds.m_compIdx].m_blkColsPerMcu == 2 ? (McuCols_t)(TR3_outCol >> 4) : (McuCols_t)(TR3_outCol >> 3);
	T3_outMcuBlkCol = S_horz.m_hcp[TR3_hds.m_compIdx].m_blkColsPerMcu == 2 ? (TR3_outCol & 0x8) >> 3 : 0;
	T3_outBlkCol = TR3_outCol & 0x7;

	PntWghtIdx_t t3_pntWghtIdx = 0;
	if (!GR_htReset && TR3_bValid) {
		t3_pntWghtIdx = S_pntWghtIdx.read_mem();
	}

	Four_i13 t4_pntWght[4];
	for (int i = 0; i < 4; i += 1) {
		S_pntWghtList[i].read_addr(t3_pntWghtIdx);
		t4_pntWght[i] = S_pntWghtList[i].read_mem();
	}

	Eight_u8 t10_colRslt;
	for (int r = 0; r < DCTSIZE; r += 1) {

		S_colData[r].read_addr(TR3_hrsAddr);

		bool bBypassColData = TR5_bValid && TR4_hrsAddr == TR5_hrsAddr;
		Sixteen_u8 t4_colData = bBypassColData ? TR5_shiftedData[r] : S_colData[r].read_mem();

		T4_shiftedData[r].m_u64[1] = (uint64_t)(((uint64_t)TR4_jdm.m_data.m_u8[r] << 56) | (t4_colData.m_u64[1] >> 8));
		T4_shiftedData[r].m_u64[0] = (uint64_t)((t4_colData.m_u64[1] << 56) | (t4_colData.m_u64[0] >> 8));

		S_colData[r].write_addr(TR4_hrsAddr);
		if (TR4_bValid)
			S_colData[r].write_mem(T4_shiftedData[r]);

		// apply filter
		for (int c = 0; c < COPROC_PNT_WGHT_CNT; c += 1) {
			ht_uint4 ci = TR4_hrs.m_bUpScale ? (TR4_bEven ? 7 : 8)+(c>>1)+(TR4_bEven&c) : c;

			T4_pntWght[c] = t4_pntWght[c>>2].m_i13[c&3];
			T4_pntData[r][c] = t4_colData.m_u8[ci];
		}

		for (int i = 0; i < 16; i += 1)
			T5_pntRslt16[r][i] = TR5_pntData[r][i] * TR5_pntWght[i];
		for (int i = 0; i < 8; i += 1)
			T6_pntRslt8[r][i] = TR6_pntRslt16[r][i*2+0] + TR6_pntRslt16[r][i*2+1];
		for (int i = 0; i < 4; i += 1)
			T7_pntRslt4[r][i] = TR7_pntRslt8[r][i*2+0] + TR7_pntRslt8[r][i*2+1];
		for (int i = 0; i < 2; i += 1)
			T8_pntRslt2[r][i] = TR8_pntRslt4[r][i*2+0] + TR8_pntRslt4[r][i*2+1];

		T9_pntRslt[r] = TR9_pntRslt2[r][0] + TR9_pntRslt2[r][1];

		ht_int10 t10_descaledPixel = descale(T10_pntRslt[r], COPROC_PNT_WGHT_FRAC_W);

		uint8_t t10_clampedPixel = clampPixel(t10_descaledPixel);

		t10_colRslt.m_u8[r] = t10_clampedPixel;
	}

	S_hrbBufBase.read_addr(TR9_hrsAddr);
	T9_hrbBufIdx = (HrbBufIdx_t)(S_hrbBufBase.read_mem() + (TR9_outCol >> 3));

	ColRsltAddr_t t10_colRsltAddr = (TR10_hrsAddr << HRB_BUF_IDX_W) | TR10_hrbBufIdx;
	S_colRslt.write_addr( t10_colRsltAddr, TR10_outCol & 0x7 );

#if !defined(_HTV) && defined(CHECK_HTB_BUF_CONFLICTS)
	if (!GR_htReset && TR10_bGenOutPixel) {
		if ((TR10_outCol & 7) == 0) {
			if (S_bHrbBufInUse[t10_colRsltAddr] != false) {
				printf("S_bHrbBufInUse[t10_colRsltAddr] != false\n");
				printf("colRsltAddr = 0x%x\n", (int)t10_colRsltAddr);
				for (int i = 0; i < 256; i += 1)
					printf("S_bHrbBufInUse[0x%x] = %d\n", i, (int)S_bHrbBufInUse[i]);
				printf("S_transMsgQue.size() = %d\n", (int)S_transMsgQue.size());
				for (uint32_t i = 0; i < S_transMsgQue.size(); i += 1)
					printf("que[%d].m_colRsltAddr = 0x%x\n", i, (int)S_transMsgQue.read_entry_debug(i).m_colRsltAddr);
				exit(0);
			}
		} else {
			if (S_bHrbBufInUse[t10_colRsltAddr] != true) {
				printf("S_bHrbBufInUse[t10_colRsltAddr] != true\n");
				exit(0);
			}
			//assert(bHrbBufInUse[t10_colRsltAddr] == true);
		}
		S_bHrbBufInUse[t10_colRsltAddr] = true;
	}
#endif

	if (!GR_htReset && TR10_bGenOutPixel) {
		S_colRslt.write_mem( t10_colRslt.m_u64 );

#if !defined(_HTV)// && defined(_WIN32)
		uint64_t blkRow = TR10_hds.m_mcuRow * S_horz.m_hcp[TR10_hds.m_compIdx].m_blkRowsPerMcu + TR10_hds.m_mcuBlkRow;
		uint64_t outPos = blkRow * (S_horz.m_hcp[TR10_hds.m_compIdx].m_outCompBlkCols << 6) + (TR10_outCol * 8);
		assert(outPos < S_horz.m_hcp[TR10_hds.m_compIdx].m_outCompBlkRows * S_horz.m_hcp[TR10_hds.m_compIdx].m_outCompBlkCols * MEM_LINE_SIZE);
		uint64_t * pMemAddr = (uint64_t *)(S_horz.m_hcp[TR10_hds.m_compIdx].m_pOutCompBuf + outPos);
		if (*pMemAddr != t10_colRslt.m_u64) {
			bool stop = true;
		}
#endif
	}

	// early staging for multiply
	T5_transMsg_blkRow = (ht_uint10)((TR5_hds.m_mcuRow << (S_horz.m_hcp[TR5_hds.m_compIdx].m_blkRowsPerMcu == 2 ? 1 : 0)) | TR5_hds.m_mcuBlkRow);
	T5_outCompBlkCols = S_horz.m_hcp[TR5_hds.m_compIdx].m_outCompBlkCols;

	T6_transMsg_outPos = TR6_transMsg_blkRow * TR6_outCompBlkCols;

	T9_bEndOfMcuRow = TR9_outCol == S_horz.m_hcp[TR9_hds.m_compIdx].m_outCompCols-1;

	HrsAddr_t hrbAddr = TR10_hrsAddr;
	HrbBufIdx_t hrbData = T10_hrbBufIdx + 1;
	bool resetHrbBufBase = false;
	/* this seems to avoid the vivado HARTNlOptGeneric::nloptAssert issue
	// disable reset for timing
	if (S_hrbBufBaseAddr != (1<<HRS_ADDR_W)) {
		hrbAddr = (HrsAddr_t)S_hrbBufBaseAddr;
		S_hrbBufBaseAddr += 1;
		hrbData = 0;
		resetHrbBufBase = true;
	}
	*/

	if (resetHrbBufBase || (TR10_bGenOutPixel && TR10_bEndOfMcuRow)) {
		S_hrbBufBase.write_addr(hrbAddr);
		S_hrbBufBase.write_mem(hrbData);
	}

	if (TR10_bGenOutPixel && ((TR10_outCol & 0x7) == 7 || TR10_bEndOfMcuRow)) {
		// Queue message to transfer memory line
		HorzTransMsg t10_htm;
		t10_htm.m_bEndOfMcuRow = T10_bEndOfMcuRow;
		t10_htm.m_bFillSecondBlk = T10_bEndOfMcuRow && S_horz.m_hcp[TR10_hds.m_compIdx].m_blkColsPerMcu == 2;
		t10_htm.m_lastCol = TR10_outCol & ((S_horz.m_hcp[TR10_hds.m_compIdx].m_blkColsPerMcu == 2) ? 0xf : 0x7);
		t10_htm.m_colRsltAddr = t10_colRsltAddr;
		t10_htm.m_compIdx = TR10_hds.m_compIdx;
		t10_htm.m_mcuRow = TR10_hds.m_mcuRow & 0x3ff;
		t10_htm.m_decPipeId = TR10_jdm.m_decPipeId;

		t10_htm.m_outPos = (ht_uint20)((TR10_transMsg_outPos * 64 + (TR10_outCol * 8)) >> 6);

		assert(!S_transMsgQue.full());
		S_transMsgQue.push( t10_htm );
	}
}

//////////////////////////////////////////////////
//  Transfer data from S_colRslt to S_colWrite
//    Two reasons for doing the transfer
//    1) An image may not fill a full MCU
//    2) Having the transfer buffer allows freeing the
//       space in S_colRslt quickly making backpressure
//       much easier to handle

void CPersHwrk::HandleWriteTransfer()
{
	T1_bTransMsgQueEmpty = S_transMsgQue.empty();
	T1_transMsgQueFront = S_transMsgQue.front();
	T1_transCol = S_transCol;
	if (!GR_htReset && !T1_bTransMsgQueEmpty && S_writeMsgQue.size()<15) {

		T1_bTransValid = true;
		HorzTransMsg htm = T1_transMsgQueFront;
		ht_uint4 fillCntM1 = (htm.m_bFillSecondBlk && htm.m_lastCol <= 7) ? 0xf : 0x7;
		if (S_transCol < fillCntM1)
			S_transCol += 1;
		else {
			S_transCol = 0;
			S_transMsgQue.pop();
#if !defined(_HTV) && defined(CHECK_HTB_BUF_CONFLICTS)
			assert(S_bHrbBufInUse[htm.m_colRsltAddr] == true);
			S_bHrbBufInUse[htm.m_colRsltAddr] = false;
#endif
		}
	}
	if (!GR_htReset && TR2_bTransValid) {
		HorzTransMsg htm = T2_transMsgQueFront;

		// read data from colRslt
		ht_uint4 fillCntM1 = (htm.m_bFillSecondBlk && htm.m_lastCol <= 7) ? 0xf : 0x7;
		ht_uint3 colIdx = (ht_uint3)(TR2_transCol < (htm.m_lastCol & fillCntM1) ? TR2_transCol : htm.m_lastCol);
		S_colRslt.read_addr( htm.m_colRsltAddr, colIdx );

		if ((TR2_transCol & 7) == 7) {
			HorzWriteMsg hwm;
			hwm.m_compIdx = htm.m_compIdx;
			hwm.m_mcuRow = htm.m_mcuRow & 0x3ff;
			hwm.m_bEndOfMcuRow = htm.m_bEndOfMcuRow && TR2_transCol == fillCntM1;
			hwm.m_transBlkIdx = S_transBufWrIdx >> 3;
			hwm.m_outPos = htm.m_outPos + (TR2_transCol >> 3);
			hwm.m_decPipeId = htm.m_decPipeId;

			assert(!S_writeMsgQue.full());
			S_writeMsgQue.push( hwm );
		}

		T2_transBufWrIdx = S_transBufWrIdx;
		S_transBufWrIdx += 1;
	}
	if (!GR_htReset && TR3_bTransValid) {
		uint64_t rslt = S_colRslt.read_mem();

		S_transBuf.write_addr(TR3_transBufWrIdx);
		S_transBuf.write_mem( rslt );
	}
}

void CPersHwrk::RecvJobInfo()
{
	// Receive hinfo messages, these may come in any order so
	//   keep a count to know when all have arrived

	if (!GR_htReset && !RecvMsgBusy_jobInfo()) {
		JobInfoMsg hinfoMsg = RecvMsg_jobInfo();

		if (hinfoMsg.m_imageIdx == i_replId && hinfoMsg.m_sectionId == JOB_INFO_SECTION_HORZ) {
			switch (hinfoMsg.m_rspQw) {
			case 0:
				{
					S_horz.m_compCnt = (hinfoMsg.m_data >> 0) & 0x3;
					S_horz.m_outImageCols = (hinfoMsg.m_data >> 44) & IMAGE_COLS_MASK;
					S_horz.m_maxBlkColsPerMcu = (hinfoMsg.m_data >> 58) & 0x3;
				}
				break;
			case 1: // second quadword of jobInfo.m_dec
				{
					S_horz.m_mcuRows = (hinfoMsg.m_data >> 0) & MCU_ROWS_MASK;
					S_horz.m_mcuCols = (hinfoMsg.m_data >> 11) & MCU_COLS_MASK;

					S_horz.m_mcuRowsM1 = S_horz.m_mcuRows-1;
					S_horz.m_mcuColsM1 = S_horz.m_mcuCols-1;
				}
				break;
			case 2:
			case 6:
			case 10:
				{
					ht_uint2 compIdx = (ht_uint2)((hinfoMsg.m_rspQw - 2)/4);
					S_horz.m_hcp[compIdx].m_blkRowsPerMcu = (hinfoMsg.m_data >> 0) & 0x3;
					S_horz.m_hcp[compIdx].m_blkColsPerMcu = (hinfoMsg.m_data >> 2) & 0x3;
#ifdef CHECK_INBOUND_HORZ_MSG
					S_horz.m_hcp[compIdx].m_inCompRows = (hinfoMsg.m_data >> 4) & IMAGE_ROWS_MASK;
					S_horz.m_hcp[compIdx].m_inCompBufCols = (hinfoMsg.m_data >> 46) & 0x7ff;
#endif
				}
				break;
			case 3:
			case 7:
			case 11:
				{
#ifdef CHECK_INBOUND_HORZ_MSG
					ht_uint2 compIdx = (ht_uint2)((hinfoMsg.m_rspQw - 3)/4);
					S_horz.m_hcp[compIdx].m_pInCompBuf = (hinfoMsg.m_data >> 0) & 0xffffffffffffLL;
#endif
				}
				break;
			case 4:
			case 8:
			case 12:
				{
					ht_uint2 compIdx = (ht_uint2)((hinfoMsg.m_rspQw - 4)/4);
					S_horz.m_hcp[compIdx].m_outCompCols = (hinfoMsg.m_data >> 14) & IMAGE_COLS_MASK;
					S_horz.m_hcp[compIdx].m_outCompBlkRows = (hinfoMsg.m_data >> 28) & MCU_ROWS_MASK;
					S_horz.m_hcp[compIdx].m_outCompBlkCols = (hinfoMsg.m_data >> 39) & MCU_COLS_MASK;
				}
				break;
			case 5:
			case 9:
			case 13:
				{
					ht_uint2 compIdx = (ht_uint2)((hinfoMsg.m_rspQw - 5)/4);
					S_horz.m_hcp[compIdx].m_pOutCompBuf = (hinfoMsg.m_data >> 0) & 0xffffffffffffLL;
				}
				break;
			case 18:
				{
					S_horz.m_filterWidth = (hinfoMsg.m_data >> 0) & 0x1f;
					S_horz.m_pntWghtListSize = (hinfoMsg.m_data >> 16) & IMAGE_COLS_MASK;
				}
				break;
			default:
				{
					if (hinfoMsg.m_rspQw == HINFO_PNT_INFO_QOFF)
						S_horz.m_pntWghtStart = (hinfoMsg.m_data >> 0) & 0x3fff;

					if (hinfoMsg.m_rspQw >= HINFO_PNT_INFO_QOFF && hinfoMsg.m_rspQw < HINFO_PNT_WGHT_LIST_QOFF) {
						sc_uint<IMAGE_COLS_W> wrAddr = (sc_uint<IMAGE_COLS_W>)(hinfoMsg.m_rspQw - HINFO_PNT_INFO_QOFF);
#if !defined(_HTV)
						S_pntWghtStart.write_addr(wrAddr);
						S_pntWghtStart.write_mem( (hinfoMsg.m_data >> 0) & 0x3fff );
#endif

						S_pntWghtOut.write_addr(wrAddr);
						S_pntWghtOut.write_mem( (hinfoMsg.m_data >> 32) & 0xf );

						S_pntWghtIdx.write_addr(wrAddr);
						S_pntWghtIdx.write_mem( (PntWghtIdx_t)(hinfoMsg.m_data >> 16) );

					}
					if (hinfoMsg.m_rspQw >= HINFO_PNT_WGHT_LIST_QOFF /*&& hinfoMsg.m_rspQw - HINFO_PNT_WGHT_LIST_QOFF < S_horz.m_pntWghtListSize*4*/) {
						uint16_t fullAddr = (uint16_t)(hinfoMsg.m_rspQw - HINFO_PNT_WGHT_LIST_QOFF);
						ht_uint2 wrIdx = fullAddr & 0x0003;
						PntWghtIdx_t wrAddr = (PntWghtIdx_t)(fullAddr >> 2);

						Four_i13 wrData;
						wrData.m_i13[0] = (hinfoMsg.m_data >> 0) & 0x1fff;
						wrData.m_i13[1] = (hinfoMsg.m_data >> 16) & 0x1fff;
						wrData.m_i13[2] = (hinfoMsg.m_data >> 32) & 0x1fff;
						wrData.m_i13[3] = (hinfoMsg.m_data >> 48) & 0x1fff;
						S_pntWghtList[wrIdx].write_addr( wrAddr );
						S_pntWghtList[wrIdx].write_mem( wrData );
					}
				}
				break;
			}
		}
	}
}

#endif
