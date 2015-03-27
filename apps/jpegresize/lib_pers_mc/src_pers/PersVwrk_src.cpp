/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(VERT)

#include "Ht.h"
#include "PersFunc.h"
#include "PersVwrk.h"

#ifndef _HTV
extern bool g_IsCheck;
#endif

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersVwrk::PersVwrk()
{
	VertImages_t p1_vImgIdx = PR1_imageIdx & VERT_IMAGES_MASK;

	// staging for memAddr to allow Vivado to infer a DSP for the multiply
	T1_blkCol = (McuCols_t)((P1_outMcuColStart <<
			(S_jobInfo[p1_vImgIdx].m_vcp[P1_compIdx].m_blkColsPerMcu == 2 ? 1 : 0)) | P1_preMcuBlkCol);
	T1_blkRow = (McuRows_t)((P1_preMcuRow <<
			(S_jobInfo[p1_vImgIdx].m_vcp[P1_compIdx].m_blkRowsPerMcu == 2 ? 1 : 0)) | P1_preMcuBlkRow);
	T1_inCompBlkCols = S_jobInfo[p1_vImgIdx].m_vcp[P1_compIdx].m_inCompBlkCols;

	VertImages_t p2_vImgIdx = PR2_imageIdx & VERT_IMAGES_MASK;
	T2_jobInfo = S_jobInfo[p2_vImgIdx];
	// use signed math since the DSP adder is signed
	T2_memAddrSum1 = (ht_int48)(T2_jobInfo.m_vcp[P2_compIdx].m_pInCompBuf + (T2_blkCol << MEM_LINE_SIZE_W));
	T2_memAddrSum2 = (ht_int48)(T2_blkRow * (T2_inCompBlkCols << MEM_LINE_SIZE_W));

	T3_memAddr = T3_memAddrSum1 + T3_memAddrSum2; // will not use DSP adder because no output reg before use?

    T2_loopVcp = T2_jobInfo.m_vcp[P2_compIdx];

	T1_bReadMem = false;

	// fix timing for memory read instructions
	T2_preMcuRow_lt_inMcuRowEnd = PR2_preMcuRow < PR2_inMcuRowEnd;
	T2_pendMcuRow_lt_inMcuRowEnd = PR2_pendMcuRow < PR2_inMcuRowEnd;
	T2_mcuBufInUseCnt_lt_VERT_PREFETCH_MCUS = S_mcuBufInUseCnt[PR2_htId] < VERT_PREFETCH_MCUS_FULL;
	T2_preMcuBlkColP1_eq_blkColsPerMcu = PR2_preMcuBlkCol+1 == S_jobInfo[p2_vImgIdx].m_vcp[PR2_compIdx].m_blkColsPerMcu;

	if (PR3_htValid) {

		switch (PR3_htInst) {
		case VWRK_ENTRY: {

			if (!PR3_bHtIdPushed) {
				S_readOrderQue.push(PR3_htId);
				S_pendOrderQue.push(PR3_htId);
				P3_bHtIdPushed = true;
			}

			VertState vrs;
			vrs.m_bUpScale = T3_jobInfo.m_maxBlkRowsPerMcu == 2 && T3_loopVcp.m_blkRowsPerMcu == 1;
			ImageRows_t outRow = (ImageRows_t)(P3_outMcuRowStart * DCTSIZE << (T3_loopVcp.m_blkRowsPerMcu == 2 ? 1 : 0));
			ImageRows_t outRowEnd = (ImageRows_t)(outRow + ((4 * DCTSIZE) << (T3_loopVcp.m_blkRowsPerMcu == 2 ? 1 : 0)));
			if (outRowEnd > T3_loopVcp.m_outCompRows) outRowEnd = T3_loopVcp.m_outCompRows;

			ht_uint1 mcuBlkRowFirst;

			if (vrs.m_bUpScale) {
				PntWghtCpInt_t filterWidth = (PntWghtCpInt_t)((T3_jobInfo.m_filterWidth >> 1) + 1);
				PntWghtCpInt_t filterOffset = (PntWghtCpInt_t)(18 - (filterWidth << 1));
				PntWghtCpInt_t negFilterOffset = -filterOffset;
				bool bInRowSel = P3_pntWghtStart < negFilterOffset;
				vrs.m_inRow = bInRowSel ? 0 : ((P3_pntWghtStart + filterOffset) >> 1);
				vrs.m_rowDataPos = bInRowSel ? (PntWghtCpInt_t)-18 : (PntWghtCpInt_t)((P3_pntWghtStart & ~1) - (filterWidth << 1));
				vrs.m_inRowOutDiff = 0;
				vrs.m_inRowIgnore = 0;
				mcuBlkRowFirst = 0;
			} else {
				PntWghtCpInt_t filterWidth = (PntWghtCpInt_t)T3_jobInfo.m_filterWidth;
				PntWghtCpInt_t filterOffset = (PntWghtCpInt_t)(17 - filterWidth);
				PntWghtCpInt_t negFilterOffset = -filterOffset;
				bool bInRowSel = P3_pntWghtStart < negFilterOffset;
				vrs.m_inRow = bInRowSel ? (ImageRows_t)0 : (ImageRows_t)(P3_pntWghtStart + filterOffset);
				vrs.m_rowDataPos = bInRowSel ? -17 : (P3_pntWghtStart - filterWidth);
				vrs.m_inRowOutDiff = 0;
				vrs.m_inRowIgnore = 0;
				mcuBlkRowFirst = (ht_uint1)((vrs.m_inRow >> 3) & (T3_loopVcp.m_blkRowsPerMcu-1));
			}

			P3_preMcuBlkRowFirst[P3_compIdx] = mcuBlkRowFirst;
			P3_wrkMcuBlkRowFirst[P3_compIdx] = mcuBlkRowFirst;

			// setup for VWRK_LOOP
			P3_preMcuRow = (McuRows_t)(P3_inImageRowStart >> 
				((T3_jobInfo.m_maxBlkRowsPerMcu == 2 ? 1 : 0) + DCTSIZE_W));
			P3_pendMcuRow = P3_preMcuRow;
			P3_inMcuRowEnd = (McuRows_t)((P3_inImageRowEnd + 
				(T3_jobInfo.m_maxBlkRowsPerMcu == 2 ? 2 : 1) * DCTSIZE - 1) >>
				((T3_jobInfo.m_maxBlkRowsPerMcu == 2 ? 1 : 0) + DCTSIZE_W));
			P3_readBufIdx = 0;
			P3_pendBufIdx = 0;
			S_mcuBufInUseCnt[PR3_htId] = 0;
			P3_preMcuBlkRow = P3_preMcuBlkRowFirst[0];
			P3_preMcuBlkCol = 0;
			P3_rdReqGrpId = PR3_htId << VERT_PREFETCH_MCUS_W;
			P3_rdPollGrpId = PR3_htId << VERT_PREFETCH_MCUS_W;
			P3_mcuReadPendCnt = 0;
			P3_bFirstWorkMcu = true;
			
			P3_mcuBlkCol += 1;
			if (P3_mcuBlkCol == T3_loopVcp.m_blkColsPerMcu) {
				P3_mcuBlkCol = 0;
				P3_compIdx += 1;
				if (P3_compIdx == T3_jobInfo.m_compCnt) {
					P3_compIdx = 0;

					HtContinue(VWRK_PREREAD_WAIT);
					break;
				}
			}

			HtContinue(VWRK_ENTRY);
		}
		break;
		case VWRK_PREREAD_WAIT: {
			// wait for other threads to complete reads
			if (S_readBusy != 0 || S_readOrderQue.front() != PR3_htId) {
				S_readPaused[PR3_htId] = true;
				HtPause(VWRK_PREREAD_WAIT);
			} else {
				S_readPaused[PR3_htId] = false;
				S_readBusy = true;
				S_readHtId = PR3_htId;
				S_readOrderQue.pop();
				HtContinue(VWRK_PREREAD);
			}
		}
		break;
		case VWRK_PREREAD: {
			T1_bMcuBufFull = S_mcuBufInUseCnt[PR3_htId] == VERT_PREFETCH_MCUS_FULL;
			T1_bMcuRowEnd = P3_preMcuRow == P3_inMcuRowEnd;
			T1_bReadMemBusy = ReadMemBusy();

			// issue reads until all needed reads are issued or buffer space is exceeded
			BUSY_RETRY(ReadMemBusy());

#ifndef _HTV
			// these next statements were moved to the P1 stage to give the multiple additional registers stages
			McuRows_t blkRow = (McuRows_t)(P3_preMcuRow * (T3_loopVcp.m_blkRowsPerMcu == 2 ? 2 : 1) | P3_preMcuBlkRow);
			McuCols_t blkCol = (McuCols_t)(P3_outMcuColStart * (T3_loopVcp.m_blkColsPerMcu == 2 ? 2 : 1) | P3_preMcuBlkCol);
			ht_uint26 pos = (ht_uint26)((blkRow * T3_loopVcp.m_inCompBlkCols + blkCol) * MEM_LINE_SIZE);
			ht_uint48 memAddr = T3_loopVcp.m_pInCompBuf + pos;
			assert(memAddr == T3_memAddr);
#endif
			if (SR_mcuBufInUseCnt[PR3_htId] < VERT_PREFETCH_MCUS_FULL && PR3_preMcuRow < PR3_inMcuRowEnd) {
				sc_uint<4+VERT_PREFETCH_MCUS_W> bufIdx = (P3_compIdx << (VERT_PREFETCH_MCUS_W+2)) | 
					(P3_preMcuBlkRow << (VERT_PREFETCH_MCUS_W+1)) | (P3_preMcuBlkCol << VERT_PREFETCH_MCUS_W) | P3_readBufIdx;

				ReadMem_rowPref(T3_memAddr, bufIdx, PR3_htId, 0, 8);

				T1_bReadMem = true;

				if (TR3_preMcuBlkColP1_eq_blkColsPerMcu) {
					P3_preMcuBlkCol = 0;
					if (P3_preMcuBlkRow+1 == T3_loopVcp.m_blkRowsPerMcu) {
						if (P3_compIdx+1 == T3_jobInfo.m_compCnt) {
							P3_compIdx = 0;
							P3_preMcuRow += 1;
							P3_rdReqGrpId = (PR3_htId << VERT_PREFETCH_MCUS_W) | ((P3_rdReqGrpId+1) & (VERT_PREFETCH_MCUS-1));
							P3_preMcuBlkRowFirst[P3_compIdx] = 0;

							P3_readBufIdx += 1;
							P3_mcuReadPendCnt += 1;
							S_mcuBufInUseCnt[PR3_htId] += 1;

						} else
							P3_compIdx += 1;
						P3_preMcuBlkRow = P3_preMcuBlkRowFirst[P3_compIdx];
					} else
						P3_preMcuBlkRow += 1;
				} else
					P3_preMcuBlkCol += 1;

				HtContinue(VWRK_PREREAD);

			} else {
				if (PR3_preMcuRow == PR3_inMcuRowEnd) {
					// free read interface for next thread
					S_readBusy = false;
				}

				HtContinue(VWRK_VRS_WAIT);
			}
		}
		break;
		case VWRK_VRS_WAIT: {
			// wait until a vrs structure is available, we double buffer
			BUSY_RETRY(S_pendOrderQue.front() != PR3_htId || S_vrsAvl == 0);

			P3_vrsIdx = (S_vrsAvl & 1) ? 0 : 1;
			S_vrsAvl &= (S_vrsAvl & 1) ? 2u : 1u;
				
			HtContinue(VWRK_VRS_INIT);
		}
		break;
		case VWRK_VRS_INIT: {
			// init vrs structure

			VertState vrs;
			vrs.m_bUpScale = T3_jobInfo.m_maxBlkRowsPerMcu == 2 && T3_loopVcp.m_blkRowsPerMcu == 1;
			ImageRows_t outRow = (ImageRows_t)(P3_outMcuRowStart * DCTSIZE << (T3_loopVcp.m_blkRowsPerMcu == 2 ? 1 : 0));
			ImageRows_t outRowEnd = (ImageRows_t)(outRow + ((4 * DCTSIZE) << (T3_loopVcp.m_blkRowsPerMcu == 2 ? 1 : 0)));
			if (outRowEnd > T3_loopVcp.m_outCompRows) outRowEnd = T3_loopVcp.m_outCompRows;

			if (vrs.m_bUpScale) {
				PntWghtCpInt_t filterWidth = (PntWghtCpInt_t)((T3_jobInfo.m_filterWidth >> 1) + 1);
				PntWghtCpInt_t filterOffset = (PntWghtCpInt_t)(18 - (filterWidth << 1));
				PntWghtCpInt_t negFilterOffset = -filterOffset;
				bool bInRowSel = P3_pntWghtStart < negFilterOffset;
				vrs.m_inRow = bInRowSel ? 0 : ((P3_pntWghtStart + filterOffset) >> 1);
				vrs.m_rowDataPos = bInRowSel ? (PntWghtCpInt_t)-18 : (PntWghtCpInt_t)((P3_pntWghtStart & ~1) - (filterWidth << 1));
				vrs.m_inRowOutDiff = 0;
				vrs.m_inRowIgnore = 0;
			} else {
				PntWghtCpInt_t filterWidth = (PntWghtCpInt_t)T3_jobInfo.m_filterWidth;
				PntWghtCpInt_t filterOffset = (PntWghtCpInt_t)(17 - filterWidth);
				PntWghtCpInt_t negFilterOffset = -filterOffset;
				bool bInRowSel = P3_pntWghtStart < negFilterOffset;
				vrs.m_inRow = bInRowSel ? (ImageRows_t)0 : (ImageRows_t)(P3_pntWghtStart + filterOffset);
				vrs.m_rowDataPos = bInRowSel ? -17 : (P3_pntWghtStart - filterWidth);
				vrs.m_inRowOutDiff = bInRowSel ? (ImageRows_t)0 : (ImageRows_t)(12);
				vrs.m_inRowIgnore = bInRowSel ? (ImageRows_t)0 : (ImageRows_t)(vrs.m_rowDataPos);
			}

			ht_uint3 vrsWrAddr = (P3_compIdx << 1) | P3_mcuBlkCol;
			S_vrs[P3_vrsIdx].write_addr(vrsWrAddr);
			S_vrs[P3_vrsIdx].write_mem( vrs );
			S_outRow[P3_vrsIdx].write_addr(vrsWrAddr);
			S_outRow[P3_vrsIdx].write_mem( outRow );
			S_outRowVal[P3_vrsIdx].write_addr(vrsWrAddr);
			S_outRowVal[P3_vrsIdx].write_mem( false );
			S_outRowEnd[P3_vrsIdx].write_addr(vrsWrAddr);
			S_outRowEnd[P3_vrsIdx].write_mem( outRowEnd );

			if (P3_mcuBlkCol+1 == T3_loopVcp.m_blkColsPerMcu) {
				P3_mcuBlkCol = 0;
				if (P3_compIdx+1 == T3_jobInfo.m_compCnt) {
					P3_compIdx = 0;

					HtContinue(VWRK_WORK_WAIT);
					break;
				} else
					P3_compIdx += 1;
			} else
				P3_mcuBlkCol += 1;

			HtContinue(VWRK_VRS_INIT);
		}
		break;
		case VWRK_WORK_WAIT: {
			// wait for thread to finish issuing work
			if (SR_pendBusy != 0 || S_pendOrderQue.front() != PR3_htId) {
				S_pendPaused[PR3_htId] = true;
				HtPause(VWRK_WORK_WAIT);
			} else {
				S_pendPaused[PR3_htId] = false;
				S_pendBusy = true;
				S_pendHtId = PR3_htId;
				S_pendOrderQue.pop();
				HtContinue(VWRK_WORK_LOOP);
			}
		}
		break;
		case VWRK_WORK_LOOP: {
			T1_bMcuBufFull = S_mcuBufInUseCnt[PR3_htId] == VERT_PREFETCH_MCUS_FULL;
			T1_bMcuRowEnd = P3_preMcuRow == P3_inMcuRowEnd;
			T1_bReadMemBusy = ReadMemBusy();

			if (SR_readBusy && SR_readHtId == PR3_htId && TR3_mcuBufInUseCnt_lt_VERT_PREFETCH_MCUS && TR3_preMcuRow_lt_inMcuRowEnd && !ReadMemBusy()) {

#ifndef _HTV
				// these next statements were moved to the P1 stage to give the multiple additional registers stages
				McuRows_t blkRow = (McuRows_t)(P3_preMcuRow * (T3_loopVcp.m_blkRowsPerMcu == 2 ? 2 : 1) | P3_preMcuBlkRow);
				McuCols_t blkCol = (McuCols_t)(P3_outMcuColStart * (T3_loopVcp.m_blkColsPerMcu == 2 ? 2 : 1) | P3_preMcuBlkCol);
				ht_uint26 pos = (ht_uint26)((blkRow * T3_loopVcp.m_inCompBlkCols + blkCol) * MEM_LINE_SIZE);
				ht_uint48 memAddr = T3_loopVcp.m_pInCompBuf + pos;
				assert(memAddr == T3_memAddr);
#endif

				sc_uint<4+VERT_PREFETCH_MCUS_W> bufIdx = (P3_compIdx << (VERT_PREFETCH_MCUS_W+2)) | 
					(P3_preMcuBlkRow << (VERT_PREFETCH_MCUS_W+1)) | (P3_preMcuBlkCol << VERT_PREFETCH_MCUS_W) | P3_readBufIdx;

				ReadMem_rowPref(T3_memAddr, bufIdx, PR3_htId, 0, 8);

				T1_bReadMem = true;

				if (TR3_preMcuBlkColP1_eq_blkColsPerMcu) {
					P3_preMcuBlkCol = 0;
					if (P3_preMcuBlkRow+1 == T3_loopVcp.m_blkRowsPerMcu) {
						if (P3_compIdx+1 == T3_jobInfo.m_compCnt) {
							P3_compIdx = 0;
							P3_preMcuRow += 1;
							P3_rdReqGrpId = (PR3_htId << VERT_PREFETCH_MCUS_W) | ((P3_rdReqGrpId+1) & (VERT_PREFETCH_MCUS-1));
							P3_preMcuBlkRowFirst[P3_compIdx] = 0;

							P3_readBufIdx += 1;
							P3_mcuReadPendCnt += 1;
							S_mcuBufInUseCnt[PR3_htId] += 1;
						} else
							P3_compIdx += 1;

						P3_preMcuBlkRow = P3_preMcuBlkRowFirst[P3_compIdx];
					} else
						P3_preMcuBlkRow += 1;
				} else
					P3_preMcuBlkCol += 1;
			}

			T1_bPendHtId = S_pendHtId == PR3_htId;
			T1_bMcuReadPendCnt = PR3_mcuReadPendCnt > 0;
			T1_bReadMemPoll = !ReadMemPoll(P3_rdPollGrpId);
			T1_bWorkQueNotFull = !S_workQue.full();

			if (SR_pendBusy && SR_pendHtId == PR3_htId && PR3_mcuReadPendCnt > 0 && !ReadMemPoll(P3_rdPollGrpId) && !S_workQue.full()) {

				// send mcu to data path
				VertWorkMsg work;
				work.m_htId = PR3_htId;

				work.m_mcuBlkRowFirst[0] = P3_bFirstWorkMcu ? P3_wrkMcuBlkRowFirst[0] : (ht_uint1)0;
				work.m_mcuBlkRowFirst[1] = P3_bFirstWorkMcu ? P3_wrkMcuBlkRowFirst[1] : (ht_uint1)0;
				work.m_mcuBlkRowFirst[2] = P3_bFirstWorkMcu ? P3_wrkMcuBlkRowFirst[2] : (ht_uint1)0;

				work.m_pntWghtEnd = P3_pntWghtEnd;
				work.m_imageIdx = P3_imageIdx;
				work.m_bEndOfColumn = P3_pendMcuRow+1 == P3_inMcuRowEnd;
				work.m_bufIdx = P3_pendBufIdx;
				work.m_outMcuColStart = P3_outMcuColStart;
				work.m_vrsIdx = P3_vrsIdx;

				S_workQue.push( work );

				P3_bFirstWorkMcu = false;
				P3_rdPollGrpId = (PR3_htId << VERT_PREFETCH_MCUS_W) | ((P3_rdPollGrpId+1) & (VERT_PREFETCH_MCUS-1));
				P3_mcuReadPendCnt -= 1;
				P3_pendMcuRow += 1;
				P3_pendBufIdx += 1;
			}

			if (SR_readBusy && SR_readHtId == PR3_htId)
				S_readBusy = TR3_preMcuRow_lt_inMcuRowEnd;

			if (SR_pendBusy && SR_pendHtId == PR3_htId)
				S_pendBusy = TR3_pendMcuRow_lt_inMcuRowEnd;

			if (TR3_preMcuRow_lt_inMcuRowEnd || TR3_pendMcuRow_lt_inMcuRowEnd)
				HtContinue(VWRK_WORK_LOOP);
			else
				HtContinue(VWRK_RETURN);
		}
		break;
		case VWRK_RETURN: {
			BUSY_RETRY(SendReturnBusy_vwrk() || S_mcuBufInUseCnt[PR3_htId] > 0);

			S_vrsAvl |= (1 << PR3_vrsIdx) & ((1<<VRS_AVL_W)-1);

			SendReturn_vwrk();
		}
		break;
		default:
			assert(0);
		}
	}

	// resume threads if read or pend not busy
	if (!SR_readBusy && SR_readPaused[S_readOrderQue.front()]) {

		S_readPaused[S_readOrderQue.front()] = false;
		HtResume(S_readOrderQue.front());

	} else if (!SR_pendBusy && SR_pendPaused[S_pendOrderQue.front()]) {

		S_pendPaused[S_pendOrderQue.front()] = false;
		HtResume(S_pendOrderQue.front());

	}

	////////////////////////
	// Data path

	FilterControl();

	// pipelined filter
	FilterDataPath();

	RecvJobInfo();

	if (GR_htReset) {
		S_bFirstInRow = true;
		S_bFirstInMcuCol = 0xff;
		S_vrsAvl = ((1u<<VRS_AVL_W)-1);
	}
}

void CPersVwrk::FilterControl()
{
	T2_bGenOutPixel = false;
	bool t2_bBlkColComplete = TR2_outRow == TR2_outRowEnd;

	// perform T2 operations that feed into T1
#if !defined(_HTV) && defined(CHECK_WGHT_MATCH)
	ht_int14 t2_pntWghtStart = S_pntWghtStart.read_mem();
	bool t2_bOldGenOutPixel = TR2_bValid && !t2_bBlkColComplete && (t2_pntWghtStart >> 1) == (TR2_rowDataPos >> 1) &&
		(TR2_bUpScale || (t2_pntWghtStart & 1) == (TR2_rowDataPos & 1));
	ht_uint1 t2_bOldEven = (t2_pntWghtStart ^ 1) & 1;
#endif
	ht_uint7 t2_pntWghtOut = S_pntWghtOut.read_mem();
	for (int i = 0; i < VRS_CNT; i += 1) {
		S_outRowVal[i].read_addr( TR2_vrsAddr );
	}
	bool t2_pntWghtOut0 = ((t2_pntWghtOut >> 0) & 0x1) == 0x1;
	bool t2_pntWghtOut1 = ((t2_pntWghtOut >> 1) & 0x1) == 0x1;
	bool t2_pntWghtOutStart0 = ((t2_pntWghtOut >> 4) & 0x1) == 0x1;
	bool t2_pntWghtOutStart1 = ((t2_pntWghtOut >> 5) & 0x1) == 0x1;
	bool t2_pntWghtOutUp0 = ((t2_pntWghtOut >> 2) & 0x1) == 0x1;
	bool t2_pntWghtOutUp1 = ((t2_pntWghtOut >> 3) & 0x1) == 0x1;
	bool t2_pntWghtOutUpStart = ((t2_pntWghtOut >> 6) & 0x1) == 0x1;
	bool t2_pntWghtOutStart =  (TR2_bUpScale ? t2_pntWghtOutUpStart :
			((TR2_inRow & 0x1) ? t2_pntWghtOutStart1 : t2_pntWghtOutStart0)) ||
			S_outRowVal[TR2_work.m_vrsIdx].read_mem();
	bool t2_bPntWghtMatch = (TR2_bUpScale ? (t2_pntWghtOutUp0 || t2_pntWghtOutUp1) :
			((TR2_inRow & 0x1) ? t2_pntWghtOut1 : t2_pntWghtOut0)) && t2_pntWghtOutStart;

	T2_bGenOutPixel = TR2_bValid && !t2_bBlkColComplete && t2_bPntWghtMatch;

	ImageRows_t t2_outRow = T2_bGenOutPixel ? (ImageRows_t)(TR2_outRowP1) : (ImageRows_t)(TR2_outRow);

	T2_bEven = TR2_bUpScale ? (ht_uint1)t2_pntWghtOutUp0 : (ht_uint1)((TR2_rowDataPos ^ 1) & 1);

	if (TR2_bValid) {
		//fprintf(stderr,"PersVwrk: imageIdx=%d compIdx=%d inRow=%d outRow=%d mcuBlkCol=%d outRowEnd=%d bUpScale=%d bGenOutPixel=%d bBlkColComplete=%d bFirstInRow=%d @ %lld\n",
		//		(int)T2_imageIdx, (int)T2_compIdx, (int)T2_inRow, (int)t2_outRow,
		//		(int)T2_mcuBlkCol, (int)TR2_outRowEnd, (int)T2_bUpScale,
		//		(int)T2_bGenOutPixel, (int)t2_bBlkColComplete,(int)SR_bFirstInRow,
		//		HT_CYCLE());
#if !defined(_HTV) && defined(CHECK_WGHT_MATCH)
		if (!t2_bBlkColComplete && T2_bGenOutPixel != t2_bOldGenOutPixel)
			fprintf(stderr,"PersVwrk: inRow=%d outRow=%d t2_outRow=%d rowDataPos=%d pntWghtOutStart=%d pntWghtStart=%d pntWghtOut=%d bUpScale=%d bGenOutPixel=%d bOldGenOutPixel=%d bFirstInRow=%d @ %lld\n",
					(int)TR2_inRow, (int)TR2_outRow, (int)t2_outRow, (int)TR2_rowDataPos, (int)t2_pntWghtOutStart, (int)t2_pntWghtStart,
					(int)t2_pntWghtOut, (int)TR2_bUpScale, (int)T2_bGenOutPixel, (int)t2_bOldGenOutPixel, (int)SR_bFirstInRow,
					HT_CYCLE());
		assert(t2_bBlkColComplete || T2_bGenOutPixel == t2_bOldGenOutPixel);
		assert(t2_bBlkColComplete || !t2_bPntWghtMatch || (T2_bEven == t2_bOldEven));
#endif
	}

	if (TR2_bValid) {
		S_outRow[TR2_work.m_vrsIdx].write_addr( TR2_vrsAddr );
		S_outRow[TR2_work.m_vrsIdx].write_mem( t2_outRow );
		S_outRowVal[TR2_work.m_vrsIdx].write_addr( TR2_vrsAddr );
		S_outRowVal[TR2_work.m_vrsIdx].write_mem( t2_pntWghtOutStart );
	}

	T1_bVertOutQueFull = S_vertOut.capacity() - S_vertOut.size() < 10;
	T1_bWorkQueEmpty = !SR_bWorkVal;
	T1_bStall = !GR_htReset && !T1_bWorkQueEmpty && T1_bVertOutQueFull;
	T1_bValid = !GR_htReset && !T1_bWorkQueEmpty && !T1_bVertOutQueFull && !SR_dpStall;

	// used in bValid section

	T1_work = SR_work;
	T1_bWorkPop = false;

	T1_imageIdx = SR_work.m_imageIdx;
	VertImages_t t1_vImgIdx = T1_imageIdx & VERT_IMAGES_MASK;

	ht_uint3 t1_vrsAddr = (SR_dpCompIdx << 1) | SR_dpMcuBlkCol;
	T1_vrsAddr = t1_vrsAddr;

	for (int i = 0; i < VRS_CNT; i += 1) {
		// we only use i=SR_work.m_vrsIdx, but set read addr for all
		//   to help timing
		S_vrs[i].read_addr(t1_vrsAddr);
		S_outRow[i].read_addr(t1_vrsAddr);
		S_outRowEnd[i].read_addr(t1_vrsAddr);
	}

	VertState t1_vrs = S_vrs[SR_work.m_vrsIdx].read_mem();
	ImageRows_t t1_outRow = S_outRow[SR_work.m_vrsIdx].read_mem();
	T1_outRowEnd = S_outRowEnd[SR_work.m_vrsIdx].read_mem();

	if (T1_bValid && S_bFirstInMcuCol[t1_vrsAddr] == 1)
		S_dpMcuBlkRow = SR_work.m_mcuBlkRowFirst[SR_dpCompIdx];

	T1_dpHtId = SR_work.m_htId;

	T1_outRow = (!TR2_bValid || SR_bFirstInRow) ? t1_outRow : t2_outRow;
	T1_outRowP1 = T1_outRow + 1;

	T1_outImageRow = (ImageRows_t)(T1_outRow << (t1_vrs.m_bUpScale ? 1 : 0));
	ImageRows_t wghtOutAddr = t1_vrs.m_bUpScale ? t1_vrs.m_inRow : (ImageRows_t)(t1_vrs.m_inRow >> 1);

#if !defined(_HTV) && defined(CHECK_WGHT_MATCH)
	S_pntWghtStart.read_addr(T1_outImageRow, t1_vImgIdx);
#endif
	S_pntWghtOut.read_addr(wghtOutAddr, t1_vImgIdx);

	T1_dpBufIdx = (SR_dpCompIdx << (VERT_PREFETCH_MCUS_W+2)) | (S_dpMcuBlkRow << (VERT_PREFETCH_MCUS_W+1))
		| (S_dpMcuBlkCol << VERT_PREFETCH_MCUS_W) | SR_work.m_bufIdx;

	T1_dpRowDataAddr = (SR_dpCompIdx << 1) | S_dpMcuBlkCol;

	T1_rowDataPos = t1_vrs.m_rowDataPos;

	// process 8 points in single clock
	T1_inRow = t1_vrs.m_inRow;
	T1_bUpScale = t1_vrs.m_bUpScale;

	// read rowData from rams and write shifted copy with new byte

	// check if we have the data to produce an output pixel
	T1_compIdx = SR_dpCompIdx;
	T1_mcuBlkCol = S_dpMcuBlkCol & 1;
	T1_outMcuColStart = SR_work.m_outMcuColStart;

	bool bLastRowOfBlkColumn = SR_work.m_bEndOfColumn &&
		(t1_vrs.m_bUpScale ? (t1_vrs.m_rowDataPos>>1) >= (SR_work.m_pntWghtEnd>>1) : 
		t1_vrs.m_rowDataPos >= SR_work.m_pntWghtEnd);
	S_dpStall = false;
	if (T1_bValid) {
		// advance mcu counters
		if ((t1_vrs.m_inRow & 7)+1 == DCTSIZE || bLastRowOfBlkColumn) {
			S_bFirstInMcuCol[t1_vrsAddr] = 0;
			S_bFirstInRow = true;
			S_row = 0;
			if (S_dpMcuBlkCol+1 == S_jobInfo[t1_vImgIdx].m_vcp[SR_dpCompIdx].m_blkColsPerMcu) {
				S_dpMcuBlkCol = 0;
				if (S_dpMcuBlkRow+1 == S_jobInfo[t1_vImgIdx].m_vcp[SR_dpCompIdx].m_blkRowsPerMcu || bLastRowOfBlkColumn) {
					S_dpMcuBlkRow = 0;
					S_bAllBlkColComplete &= bLastRowOfBlkColumn;
					if (S_dpCompIdx+1 == S_jobInfo[t1_vImgIdx].m_compCnt) {
						S_dpCompIdx = 0;

						if (!SR_work.m_bEndOfColumn || S_bAllBlkColComplete) {
							T1_bWorkPop = true;

							//S_workQue.pop();
							S_mcuBufInUseCnt[SR_work.m_htId] -= 1;
							S_bFirstInMcuCol = 0xff;
						}

						S_bAllBlkColComplete = true;

					} else
						S_dpCompIdx += 1;
				} else
					S_dpMcuBlkRow += 1;
			} else
				S_dpMcuBlkCol += 1;

			S_dpStall = S_dpCompIdx == SR_dpCompIdx && S_dpMcuBlkCol == SR_dpMcuBlkCol;
		} else {
			S_row += 1;
			S_bFirstInRow = false;
		}

		t1_vrs.m_inRow += 1;
		t1_vrs.m_rowDataPos += t1_vrs.m_bUpScale ? 2 : 1;

		T1_vrs = t1_vrs;

		S_vrs[SR_work.m_vrsIdx].write_addr( t1_vrsAddr );
		S_vrs[SR_work.m_vrsIdx].write_mem( t1_vrs );
	}

	// register at output of work queue
	S_bWorkVal = !S_workQue.empty() || (!T1_bWorkPop && SR_bWorkVal);

	if (T1_bWorkPop || !SR_bWorkVal) 
		S_work = S_workQue.front();

	if (!S_workQue.empty() && (!SR_bWorkVal || T1_bWorkPop))
		S_workQue.pop();

	if (GR_htReset)
		S_bAllBlkColComplete = true;
}

void CPersVwrk::FilterDataPath()
{
	VertImages_t t2_vImgIdx = TR2_imageIdx & VERT_IMAGES_MASK;

	S_pntWghtIdx.read_addr(T2_outImageRow, t2_vImgIdx);

	VertImages_t t3_vImgIdx = TR3_imageIdx & VERT_IMAGES_MASK;

	T3_outMcuRow = S_jobInfo[t3_vImgIdx].m_vcp[TR3_compIdx].m_blkRowsPerMcu == 2 ?
		(McuRows_t)(TR3_outRow >> 4) : (McuRows_t)(TR3_outRow >> 3);
	T3_outMcuBlkRow = S_jobInfo[t3_vImgIdx].m_vcp[TR3_compIdx].m_blkRowsPerMcu == 2 ? (TR3_outRow & 0x8) >> 3 : 0;
	T3_outBlkRow = TR3_outRow & 0x7;

	T3_bOutMcuColStartEq = TR3_outMcuColStart == S_jobInfo[t3_vImgIdx].m_outMcuCols-1;
	T3_bMcuBlkColEq = TR3_mcuBlkCol == S_jobInfo[t3_vImgIdx].m_vcp[TR3_compIdx].m_blkColsPerMcu-1;
	T3_bCompIdxEq = TR3_compIdx == S_jobInfo[t3_vImgIdx].m_compCnt-1;
	T3_bOutBlkRowEq = T3_outBlkRow == 7;
	T3_bOutRowEq = T3_outRow == S_jobInfo[t3_vImgIdx].m_vcp[TR3_compIdx].m_outCompRows-1;

	T4_bEndOfMcuRow = TR4_bOutMcuColStartEq && TR4_bMcuBlkColEq && TR4_bCompIdxEq && (TR4_bOutBlkRowEq || TR4_bOutRowEq);

	T3_pntWghtIdx = 0;
	if (TR3_bValid)
		T3_pntWghtIdx = S_pntWghtIdx.read_mem();

	Four_i13 t4_pntWght[4];
	for (int i = 0; i < 4; i += 1) {
		S_pntWghtList[i].read_addr(T3_pntWghtIdx, t3_vImgIdx);
		t4_pntWght[i] = S_pntWghtList[i].read_mem();
	}

	Eight_u8 t9_rowRslt;
	for (int c = 0; c < DCTSIZE; c += 1) {

		S_rowPref[c].read_addr(TR3_dpBufIdx, TR3_dpHtId);
		Eight_u8 t4_ebCol;
		t4_ebCol.m_u64 = S_rowPref[c].read_mem();
		uint8_t t4_data = t4_ebCol.m_u8[TR4_inRow & 0x7];

		S_rowData[c].read_addr(TR3_dpRowDataAddr);

		bool bBypassColData = TR5_bValid && TR4_dpRowDataAddr == TR5_dpRowDataAddr;
		Sixteen_u8 t4_rowData = bBypassColData ? TR5_shiftedData[c] : S_rowData[c].read_mem();

		//if (TR3_bValid && T3_bGenOutPixel)
		//	bool stop = true;

		T4_shiftedData[c].m_u64[1] = (uint64_t)(((uint64_t)t4_data << 56) | (t4_rowData.m_u64[1] >> 8));
		T4_shiftedData[c].m_u64[0] = (uint64_t)((t4_rowData.m_u64[1] << 56) | (t4_rowData.m_u64[0] >> 8));

		S_rowData[c].write_addr(TR4_dpRowDataAddr);
		if (TR4_bValid)
			S_rowData[c].write_mem(T4_shiftedData[c]);

		// apply filter
		for (int r = 0; r < COPROC_PNT_WGHT_CNT; r += 1) {
			ht_uint4 ri = TR4_bUpScale ? (TR4_bEven ? 7 : 8)+(r>>1)+(TR4_bEven&r) : r;

			T4_pntWght[r] = t4_pntWght[r>>2].m_i13[r&3];
			T4_pntData[c][r] = t4_rowData.m_u8[ri];
		}

		for (int i = 0; i < 16; i += 1)
			T5_pntRslt16[c][i] = TR5_pntData[c][i] * TR5_pntWght[i];
		for (int i = 0; i < 8; i += 1)
			T6_pntRslt8[c][i] = TR6_pntRslt16[c][i*2+0] + TR6_pntRslt16[c][i*2+1];
		for (int i = 0; i < 4; i += 1)
			T7_pntRslt4[c][i] = TR7_pntRslt8[c][i*2+0] + TR7_pntRslt8[c][i*2+1];
		for (int i = 0; i < 2; i += 1)
			T8_pntRslt2[c][i] = TR8_pntRslt4[c][i*2+0] + TR8_pntRslt4[c][i*2+1];

		T9_pntRslt[c] = TR9_pntRslt2[c][0] + TR9_pntRslt2[c][1];

		ht_int10 t9_descaledPixel = descale(T9_pntRslt[c], COPROC_PNT_WGHT_FRAC_W);

		uint8_t t9_clampedPixel = clampPixel(t9_descaledPixel);

		t9_rowRslt.m_u8[c] = t9_clampedPixel;
	}

	T9_rowRslt = t9_rowRslt;

	VertImages_t t9_vImgIdx = TR9_imageIdx & VERT_IMAGES_MASK;
	ImageRows_t t9_outCompRowsM1 = (S_jobInfo[t9_vImgIdx].m_vcp[TR9_compIdx].m_outCompRows-1) & IMAGE_ROWS_MASK;
	T9_bEndOfColumn = TR9_outRow == t9_outCompRowsM1;
	T9_fillSizeM1 = S_jobInfo[t9_vImgIdx].m_vcp[TR9_compIdx].m_blkRowsPerMcu == 2 ? 15 : 7;

	if (!GR_htReset && TR10_bGenOutPixel) {

		//printf("vertResizeMsg: ci %d, mcuRow %d, blkRowInMcu %d, rowInBlk %d, firstBlkInMcuRow %d, lastBlkInMcuRow %d, blkColInMcu %d, data %016llx\n",
		//	(int)TR8_compIdx, (int)TR8_outMcuRow, (int)TR8_outMcuBlkRow, (int)TR8_outBlkRow,
		//	(int)TR8_bFirstBlkInMcuRow, (int)TR8_bLastBlkInMcuRow, (int)TR8_mcuBlkCol, t8_rowRslt.m_u64);

		ht_uint4 fillCnt = T10_fillSizeM1 - (TR10_outRow & T10_fillSizeM1);

		// push info to send 8B to jpeg encoder
		VertResizeMsg vom;
		vom.m_imageIdx = TR10_imageIdx;
		vom.m_compIdx = TR10_compIdx;
		vom.m_mcuBlkRow = TR10_outMcuBlkRow;
		vom.m_mcuBlkCol = TR10_mcuBlkCol;
		vom.m_mcuRow = TR10_outMcuRow;
		vom.m_mcuCol = TR10_outMcuColStart;
		vom.m_blkRow = TR10_outBlkRow;
		vom.m_bEndOfMcuRow = TR10_bEndOfMcuRow;
		vom.m_fillCnt = T10_bEndOfColumn ? fillCnt : (ht_uint4)0;
		vom.m_data = T10_rowRslt.m_u64;

		T10_vom = vom;

		S_vertOut.push(vom);
	}

	if (!S_vertOut.empty()) {
		VertResizeMsg vrm = S_vertOut.front();

		T1_sendVrm = vrm;

#ifndef _HTV
		VertImages_t vrm_vImgIdx = vrm.m_imageIdx & VERT_IMAGES_MASK;
		assert(vrm.m_blkRow + S_vertOutFillCnt < (S_jobInfo[vrm_vImgIdx].m_vcp[vrm.m_compIdx].m_blkRowsPerMcu == 2 ? 16 : 8));
#endif

		vrm.m_mcuBlkRow |= ((vrm.m_blkRow + S_vertOutFillCnt) >= 8 ? 1u : 0u);
		vrm.m_blkRow += S_vertOutFillCnt & 7;

		ht_uint4 vobAddr1 = (vrm.m_compIdx << 2) | (vrm.m_mcuBlkCol << 1) | (vrm.m_mcuBlkRow << 0);
		ht_uint5 vobAddr2 = ((SR_vobWrIdx[vobAddr1] & 3) << 3) | vrm.m_blkRow;

		T1_sendVrm = vrm;
		T1_vobAddr1 = vobAddr1;
		T1_vobAddr2 = vobAddr2;
		T1_vobWrIdx = SR_vobWrIdx[vobAddr1];

		if ((SR_vobRdIdx[vobAddr1] ^ SR_vobWrIdx[vobAddr1]) != 0x4) {

			if (S_vertOutFillCnt < vrm.m_fillCnt) {
				T1_sendVrm.m_bEndOfMcuRow = false;
				S_vertOutFillCnt += 1;
			} else {
				S_vertOutFillCnt = 0;
				S_vertOut.pop();
			}

			if (vrm.m_blkRow == DCTSIZE-1)
				S_vobWrIdx[vobAddr1] += 1;

			T1_bSendVrm = true;

	#if !defined(_HTV)
			// write to memory for DV to compare, not done in HW.
			if (g_IsCheck) {
				uint64_t outRow = (vrm.m_mcuRow * S_jobInfo[vrm_vImgIdx].m_vcp[vrm.m_compIdx].m_blkRowsPerMcu + vrm.m_mcuBlkRow) * DCTSIZE + vrm.m_blkRow;
				uint64_t pos = outRow * S_jobInfo[vrm_vImgIdx].m_vcp[vrm.m_compIdx].m_outCompBufCols +
					(vrm.m_mcuCol * S_jobInfo[vrm_vImgIdx].m_vcp[vrm.m_compIdx].m_blkColsPerMcu + vrm.m_mcuBlkCol) * 8;
				uint64_t pOutCompBuf = S_jobInfo[vrm_vImgIdx].m_vcp[vrm.m_compIdx].m_pOutCompBuf;
				memcpy( &((uint8_t *)pOutCompBuf)[pos], &vrm.m_data, 8);
			}
	#endif
		}
	}
	VertResizeMsg t2_sendVrm = TR2_sendVrm;
	ht_uint4 t2_vobAddr1 = TR2_vobAddr1;
	ht_uint5 t2_vobAddr2 = TR2_vobAddr2;
	bool resetVob = false;
	// don't need to reset this structure but it appears avoid a vivado bug
	// uncomment or comment this if HARTNlOptGeneric::nloptAssert issue occur in PersVert
	/*if (S_vobIdx < (1 << 9)) {
		t2_vobAddr1 = (S_vobIdx>>5) & 0xf;
		t2_vobAddr2 = (S_vobIdx>>0) & 0x1f;
		S_vobIdx += 1;
		t2_sendVrm = 0;
		resetVob = true;
	}
*/
	if (resetVob || TR2_bSendVrm) {
		// write to message reorder buffer
		JpegEncMsg jem;
		jem.m_imageIdx = t2_sendVrm.m_imageIdx;
		jem.m_compIdx = t2_sendVrm.m_compIdx;
		jem.m_blkRow = t2_sendVrm.m_blkRow;
		jem.m_rstIdx = t2_sendVrm.m_mcuRow;
		jem.m_bEndOfMcuRow = t2_sendVrm.m_bEndOfMcuRow;
		jem.m_data.m_u64 = t2_sendVrm.m_data;

		S_vertOrderBuf.write_addr(t2_vobAddr1, t2_vobAddr2);
		S_vertOrderBuf.write_mem(jem);

		S_vobVImgIdx.write_addr(TR2_vobAddr1, TR2_vobWrIdx & 3);
		S_vobVImgIdx.write_mem(TR2_sendVrm.m_imageIdx & VERT_IMAGES_MASK);
	}

	T1_bSendJemMsg = false;
	if (!SendMsgFull_jem()) {
		// check if the next MCU block is in the reorder buffer
		ht_uint4 vobAddr1 = (S_vos.m_compIdx << 2) | (S_vos.m_mcuBlkCol << 1) | (S_vos.m_mcuBlkRow << 0);

		if (SR_vobRdIdx[vobAddr1] != SR_vobWrIdx[vobAddr1]) {
			// send a message

			ht_uint5 vobAddr2 = ((SR_vobRdIdx[vobAddr1] & 3) << 3) | S_vos.m_blkRow;

			S_vertOrderBuf.read_addr(vobAddr1, vobAddr2);

			S_vobVImgIdx.read_addr(vobAddr1, SR_vobRdIdx[vobAddr1] & 3);
			VertImages_t vos_vImgIdx = S_vobVImgIdx.read_mem();
			T1_bSendJemMsg = true;

			// increment MCU index
			if (S_vos.m_blkRow == DCTSIZE-1) {
				S_vos.m_blkRow = 0;
				S_vobRdIdx[vobAddr1] += 1;
				if (S_vos.m_mcuBlkCol+1 == S_jobInfo[vos_vImgIdx].m_vcp[S_vos.m_compIdx].m_blkColsPerMcu) {
					S_vos.m_mcuBlkCol = 0;
					if (S_vos.m_mcuBlkRow+1 == S_jobInfo[vos_vImgIdx].m_vcp[S_vos.m_compIdx].m_blkRowsPerMcu) {
						S_vos.m_mcuBlkRow = 0;
						if (S_vos.m_compIdx+1 == S_jobInfo[vos_vImgIdx].m_compCnt) {
							S_vos.m_compIdx = 0;

						} else
							S_vos.m_compIdx += 1;
					} else
						S_vos.m_mcuBlkRow += 1u;
				} else
					S_vos.m_mcuBlkCol += 1u;
			} else
				S_vos.m_blkRow += 1;
		}
	}

	if (!GR_htReset && TR2_bSendJemMsg) {
		JpegEncMsg jem = S_vertOrderBuf.read_mem();
		SendMsg_jem(jem);

		T2_jem = jem;
	}
}

void CPersVwrk::RecvJobInfo()
{

	// Receive vinfo messages, these may come in any order so
	//   keep a count to know when all have arrived

	if (!GR_htReset && !RecvMsgBusy_veInfo()) {
		JobInfoMsg veInfoMsg = RecvMsg_veInfo();

		VertImages_t vImgIdx = veInfoMsg.m_imageIdx & VERT_IMAGES_MASK;

		if ((veInfoMsg.m_imageIdx >> 1) == i_replId.read() && veInfoMsg.m_sectionId == JOB_INFO_SECTION_VERT) {

			switch (veInfoMsg.m_rspQw) {
			case 0: // first quadword of jobInfo.m_vert
				{
					S_jobInfo[vImgIdx].m_compCnt = (veInfoMsg.m_data >> 0) & 0x3;
					S_jobInfo[vImgIdx].m_outImageRows = (veInfoMsg.m_data >> 30) & IMAGE_ROWS_MASK;
					S_jobInfo[vImgIdx].m_maxBlkRowsPerMcu = (veInfoMsg.m_data >> 58) & 0x3;
				}
				break;
			case 1:
				{
					S_jobInfo[vImgIdx].m_inMcuRows = (veInfoMsg.m_data >> 0) & MCU_ROWS_MASK;
					S_jobInfo[vImgIdx].m_outMcuCols = (veInfoMsg.m_data >> 33) & MCU_COLS_MASK;
				}
				break;
			case 2:
			case 6:
			case 10:
				{
					ht_uint2 compIdx = (ht_uint2)((veInfoMsg.m_rspQw - 2)/4);
					S_jobInfo[vImgIdx].m_vcp[compIdx].m_blkRowsPerMcu = (veInfoMsg.m_data >> 0) & 0x3;
					S_jobInfo[vImgIdx].m_vcp[compIdx].m_blkColsPerMcu = (veInfoMsg.m_data >> 2) & 0x3;
					S_jobInfo[vImgIdx].m_vcp[compIdx].m_inCompBlkCols = (veInfoMsg.m_data >> 43) & MCU_COLS_MASK;
				}
				break;
			case 3:
			case 7:
			case 11:
				{
					ht_uint2 compIdx = (ht_uint2)((veInfoMsg.m_rspQw - 3)/4);
					S_jobInfo[vImgIdx].m_vcp[compIdx].m_pInCompBuf = (ht_uint48)veInfoMsg.m_data;
				}
				break;
			case 4:
			case 8:
			case 12:
				{
					ht_uint2 compIdx = (ht_uint2)((veInfoMsg.m_rspQw - 4)/4);
					S_jobInfo[vImgIdx].m_vcp[compIdx].m_outCompRows = (veInfoMsg.m_data >> 0) & IMAGE_ROWS_MASK;
#ifndef _HTV
					S_jobInfo[vImgIdx].m_vcp[compIdx].m_outCompBufRows = (veInfoMsg.m_data >> 28) & IMAGE_ROWS_MASK;
					S_jobInfo[vImgIdx].m_vcp[compIdx].m_outCompBufCols = (veInfoMsg.m_data >> 42) & IMAGE_COLS_MASK;
#endif
				}
				break;
			case 5:
			case 9:
			case 13:
				{
#ifndef _HTV
					ht_uint2 compIdx = (ht_uint2)((veInfoMsg.m_rspQw - 5)/4);
					S_jobInfo[vImgIdx].m_vcp[compIdx].m_pOutCompBuf = (ht_uint48)veInfoMsg.m_data;
#endif
				}
				break;
			case 18: // eighteenth quadword
				{
					S_jobInfo[vImgIdx].m_filterWidth = (veInfoMsg.m_data >> 0) & 0x1f;
					S_jobInfo[vImgIdx].m_pntWghtListSize = (veInfoMsg.m_data >> 16) & IMAGE_ROWS_MASK;
				}
				break;
			default:
				{
					if (veInfoMsg.m_rspQw >= START_OF_PNT_INFO_QOFF && 
						veInfoMsg.m_rspQw < START_OF_PNT_WGHT_LIST_QW)
					{
						ImageRows_t wrAddr = (ImageRows_t)(veInfoMsg.m_rspQw - START_OF_PNT_INFO_QOFF);

#if !defined(_HTV)
						S_pntWghtStart.write_addr(wrAddr, vImgIdx);
						S_pntWghtStart.write_mem( (veInfoMsg.m_data >> 0) & 0x3fff );
#endif

						S_pntWghtIdx.write_addr(wrAddr, vImgIdx);
						S_pntWghtIdx.write_mem( (PntWghtIdx_t)(veInfoMsg.m_data >> 16) );

						S_pntWghtOut.write_addr(wrAddr, vImgIdx);
						S_pntWghtOut.write_mem( (veInfoMsg.m_data >> 32) & 0x7f );
					}

					if (veInfoMsg.m_rspQw >= START_OF_PNT_WGHT_LIST_QW /*&&
						veInfoMsg.m_rspQw - START_OF_PNT_WGHT_LIST_QW < S_jobInfo[vImgIdx].m_pntWghtListSize*4*/)
					{
						ht_uint2 wrIdx = (ht_uint2)((veInfoMsg.m_rspQw - START_OF_PNT_WGHT_LIST_QW) & 0x3);
						PntWghtIdx_t wrAddr = (PntWghtIdx_t)((veInfoMsg.m_rspQw - START_OF_PNT_WGHT_LIST_QW) >> 2);

						Four_i13 wrData;
						wrData.m_i13[0] = (veInfoMsg.m_data >> 0) & 0x1fff;
						wrData.m_i13[1] = (veInfoMsg.m_data >> 16) & 0x1fff;
						wrData.m_i13[2] = (veInfoMsg.m_data >> 32) & 0x1fff;
						wrData.m_i13[3] = (veInfoMsg.m_data >> 48) & 0x1fff;

						S_pntWghtList[ wrIdx ].write_addr( wrAddr, vImgIdx );
						S_pntWghtList[ wrIdx ].write_mem( wrData );
					}
				}
				break;
			}
		}
	}
}

#endif
