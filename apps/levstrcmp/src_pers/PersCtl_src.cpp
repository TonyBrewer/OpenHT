/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT levstrcmp application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
/*
**	Pseudo code, first loop through blocks of strings
**	(strLenL, strLenR, mchAdjD, conIdL, conIdR, conSizeL, conSizeR, conBaseL, conBaseR)
**	
**  // match is broken into BLK_STR_CNT_L by BLK_STR_CNT_R sized compares
**	for (int conIdxL = 0; conIdxL < conSizeL; conIdxL += BLK_STR_CNT_L) {
**		for (int conIdxR = 0; conIdxR < conSizeR; conIdxR += BLK_STR_CNT_R) {
**			
**			// perform 8 async calls to read left strings into rams
**			int blkSizeL = conSizeL - conIdxL > BLK_STR_CNT_L ? BLK_STR_CNT_L : (conSizeL - conIdxL);
**			for (int i = 0; i < 8; i += 1)
**				read_AsyncCall(CTL_RD_RTN, conBaseL + 16 * conIdxL, blkSizeL, i, 0);
**
**			// perform 8 async calls to read right strings into rams
**			int blkSizeR = conSizeR - conIdxR > BLK_STR_CNT_R ? BLK_STR_CNT_R : (conSizeR - conIdxR);
**			for (int i = 0; i < 8; i += 1)
**				read_AsyncCall(CTL_RD_RTN, conBaseR + 16 * conIdxR, blkSizeR, i, 1);
**
**			Wait for all read asyncCalls to complete
**
**			// perform match on a left block versus a right block
**			for (int blkIdxR = 0; blkIdxR < blkSizeR; blkIdxR += LINE_STR_CNT_R) {
**		
**				int lineStrCntR = blkSizeR - blkIdxR > LINE_STR_CNT_R ? LINE_STR_CNT_R : (blkSizeR - blkIdxR);
**				int ramIdxR = blkIdxR / LINE_STR_CNT_R + ramBlkIdx * RAM_BLOCK_SIZE;
**	
**				// perform match on a left block versus a right line
**				cmpAllLx1R( MCH_LOOP, strLenL, strLenR, mchAdjD, conIdL, conIdR, ramIdxR, blkSizeL, lineStrCntR, ramBlkIdx );
**			}
**
**			Wait for all cmp asyncCalls to complete
**		}
**	}
**	
*/

#include "Ht.h"
#include "PersCtl.h"

void
CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY:
			// entry instruction initializes look indexes to zero
			{
				if (SendReturnBusy_htmain()) {
					HtRetry();
					break;
				}

				P_conIdxL = 0;
				P_conIdxR = 0;

				P_blkSizeL = (BlkSizeL_t)(P_conSizeL - P_conIdxL > BLK_STR_CNT_L 
					? BLK_STR_CNT_L : (P_conSizeL - P_conIdxL));

				P_blkIdxR = 0;
				P_blkSizeR = (BlkSizeR_t)(P_conSizeR - P_conIdxR > BLK_STR_CNT_R 
					? BLK_STR_CNT_R : (P_conSizeR - P_conIdxR));

				P_lineStrIdx = 0;

				if (P_strLenL == 0 && P_strLenR == 0) {
					// end of run indicator, call match
					S_threadActiveCnt += 1;
					HtContinue(CTL_WAIT_TIL_IDLE);
				} else if (P_conSizeL == 0 || P_conSizeR == 0)
					SendReturn_htmain(P_rsltCnt);	// an empty block, just return
				else {
					S_threadActiveCnt += 1;
					HtContinue(CTL_BLK_L_LOOP);
				}
				break;
			}
		case CTL_BLK_L_LOOP:
			//			for (int i = 0; i < 8; i += 1)
			//				read_AsyncCall(CTL_RD_RTN, conBaseL + 16 * conIdxL, blkSizeL, i, 0);
			{
				if (SendCallBusy_Read()) {
					HtRetry();
					break;
				}

				MemAddr_t memAddr = P_conBaseL + SIZE_OF_STRING * P_conIdxL;

				SendCallFork_Read(CTL_RD_RTN, memAddr, P_blkSizeL, P_lineStrIdx, 0, PR_htId);

				if (P_lineStrIdx == 7) {
					P_lineStrIdx = 0;
					HtContinue(CTL_BLK_R_LOOP);
				} else {
					P_lineStrIdx += 1;
					HtContinue(CTL_BLK_L_LOOP);
				}

				break;
			}
		case CTL_BLK_R_LOOP:
			//			for (int i = 0; i < 8; i += 1)
			//				read_AsyncCall(CTL_RD_RTN, conBaseR + 16 * conIdxR, blkSizeR, i, 0);
			{
				if (SendCallBusy_Read()) {
					HtRetry();
					break;
				}

				MemAddr_t memAddr = P_conBaseR + SIZE_OF_STRING * P_conIdxR;

				SendCallFork_Read(CTL_RD_RTN, memAddr, P_blkSizeR, P_lineStrIdx, 1, PR_htId);

				if (P_lineStrIdx == 7)
					RecvReturnPause_Read(CTL_RD_COMPLETE);	// done with reads, idle thread without writing private variables
				else {
					P_lineStrIdx += 1;
					HtContinue(CTL_BLK_R_LOOP);
				}

				break;
			}
		case CTL_RD_RTN:
			{
				//	Wait for all read asyncCalls to complete
				RecvReturnJoin_Read();
				break;
			}
		case CTL_RD_COMPLETE:
			{
				// All reads have returned, proceed to matching
				HtContinue(CTL_MCH_LOOP);

				break;
			}
		case CTL_MCH_LOOP:
			{
				if (SendCallBusy_MchBlkLx1R()) {
					HtRetry();
					break;
				}

				LineStrCntR_t lineStrCntR = (LineStrCntR_t) (P_blkSizeR - P_blkIdxR > LINE_STR_CNT_R 
					? LINE_STR_CNT_R : (P_blkSizeR - P_blkIdxR));

				RamSize_t ramIdxR = (RamSize_t)(P_blkIdxR / LINE_STR_CNT_R + PR_htId * RAM_BLOCK_SIZE);

				// Call MchBlkLx1R
				SendCallFork_MchBlkLx1R(CTL_MCH_RTN, P_strLenL, P_strLenR, P_mchAdjD,
					P_conIdL, P_conIdR, P_conIdxL, P_conIdxR, 
					ramIdxR, P_blkSizeL, lineStrCntR, PR_htId);

				// increment loop index and check for loop termination condition
				P_blkIdxR += LINE_STR_CNT_R;
				if (P_blkIdxR < P_blkSizeR)
					HtContinue(CTL_MCH_LOOP);
				else
					RecvReturnPause_MchBlkLx1R(CTL_MCH_COMPLETE);

				break;
			}
		case CTL_MCH_RTN:
			{
				RecvReturnJoin_MchBlkLx1R();
				break;
			}
		case CTL_MCH_COMPLETE:
			{
				// update container loop indexes (L & R)
				P_conIdxR += BLK_STR_CNT_R;
				if (P_conIdxR < P_conSizeR)
					HtContinue(CTL_BLK_L_LOOP);
				else {
					P_conIdxR = 0;
					P_conIdxL += BLK_STR_CNT_L;
					if (P_conIdxL < P_conSizeL)
						HtContinue(CTL_BLK_L_LOOP);
					else {
						P_conIdxL = 0;

						// Return to host interface
						HtContinue(CTL_WAIT_RTN);
					}
				}

				// initialize for next loop
				P_blkSizeL = (BlkSizeL_t)(P_conSizeL - P_conIdxL > BLK_STR_CNT_L 
					? BLK_STR_CNT_L : (P_conSizeL - P_conIdxL));

				P_blkIdxR = 0;
				P_blkSizeR = (BlkSizeR_t)(P_conSizeR - P_conIdxR > BLK_STR_CNT_R 
					? BLK_STR_CNT_R : (P_conSizeR - P_conIdxR));

				P_lineStrIdx = 0;

				break;
			}
		case CTL_WAIT_TIL_IDLE:
			{
				if (SendCallBusy_MchBlkLx1R()) {
					HtRetry();
					break;
				}

				if (S_threadActiveCnt > 1)
					HtContinue(CTL_WAIT_TIL_IDLE);
				else {
					RamSize_t ramIdxR = 0;
					LineStrCntR_t lineStrCntR = 0;

					SendCall_MchBlkLx1R(CTL_WAIT_RTN, P_strLenL, P_strLenR, P_mchAdjD,
						P_conIdL, P_conIdR, P_conIdxL, P_conIdxR, 
						ramIdxR, P_blkSizeL, lineStrCntR, PR_htId);
				}

				break;
			}
		case CTL_WAIT_RTN:
			{
				if (SendReturnBusy_htmain()) {
					HtRetry();
					break;
				}

				S_threadActiveCnt -= 1;
				SendReturn_htmain(P_rsltCnt);
				break;
			}
		default:
			assert(0);
		}
	}

	if (GR_htReset)
		S_threadActiveCnt = 0;
}
