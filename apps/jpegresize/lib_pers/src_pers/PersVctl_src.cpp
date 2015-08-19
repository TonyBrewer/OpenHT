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
#include "PersVctl.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void CPersVctl::PersVctl()
{
	if (PR1_htValid) {
		ht_uint1 vImgIdx = P1_imageIdx & VERT_REPL_MASK;

		switch (PR1_htInst) {
		case VCTL_ENTRY: {

			P1_bEndOfImage = false;
			P1_inImageRowStart = 0;
			P1_inImageRowEnd = 0;

			P1_outMcuRowStart = 0;
			P1_outMcuColStart = 0;

			P1_pntWghtStartRdAddr1 = 0;
			P1_pntWghtStartRdAddr2 = vImgIdx;

			if (S_mcuRowClearIdx[PR1_htId] < S_jobInfo[vImgIdx].m_inMcuRows) {
				PW1_mcuRowComplete.write_addr(S_mcuRowClearIdx[PR1_htId]);
				PW1_mcuRowComplete = false;
				S_mcuRowClearIdx[PR1_htId] += 1;

				HtContinue(VCTL_ENTRY);
			} else
				HtContinue(VCTL_CALC_START);
		}
		break;
		case VCTL_CALC_START: {
			PntWghtCpInt_t filterOffset = (PntWghtCpInt_t)(17 - S_jobInfo[vImgIdx].m_filterWidth);
			P1_pntWghtStart = GR1_pntWghtStart;
			PntWghtCpInt_t pntWghtStartSum = P1_pntWghtStart + filterOffset;
			P1_inImageRowStart = pntWghtStartSum < 0 ? (ImageRows_t)0 : (ImageRows_t)pntWghtStartSum;

			McuRows_t outMcuRowEnd = P1_outMcuRowStart + VERT_MCU_ROWS;
			ImageRows_t outImageRowLast = (ImageRows_t)(outMcuRowEnd * S_jobInfo[vImgIdx].m_maxBlkRowsPerMcu * DCTSIZE);
			if (outImageRowLast >= S_jobInfo[vImgIdx].m_outImageRows) {
				P1_bEndOfColumn = true;
				P1_pntWghtStartRdAddr1 = (PntWghtAddr_t)(S_jobInfo[vImgIdx].m_outImageRows-1);
			} else {
				P1_bEndOfColumn = false;
				P1_pntWghtStartRdAddr1 = outImageRowLast;
			}

			fprintf(stdout, "VCTL: P1_outMcuRowStart %d @ %lld\n", (int)P1_outMcuRowStart, HT_CYCLE());

			HtContinue(VCTL_CALC_END);
		}
		break;
		case VCTL_CALC_END: {
			P1_pntWghtEnd = GR1_pntWghtStart;
			P1_inImageRowEnd  = (ImageRows_t)(GR1_pntWghtStart + COPROC_PNT_WGHT_CNT+1);
			if (P1_inImageRowEnd > S_jobInfo[vImgIdx].m_inImageRows) P1_inImageRowEnd = S_jobInfo[vImgIdx].m_inImageRows;
			HtContinue(VCTL_MSG_LOOP);
		}
		break;
		case VCTL_MSG_LOOP: {
			// The message loop does the following operations
			// 1. Recv messages from Horz resizer that indicates an MCU row has completed.
			//    The rows may not complete in order
			// 2. Set a bit in an array indicating the row has completed. The array is
			//    cleared prior to the image starting.
			// 3. Monitor the set bits to determine when enough MCU rows have completed
			//    to start processing an output strip of MCU rows. Keep a variable indicating
			//    the next unfinished MCU row. Clear array bits as we increment the variable.
			// 4. Send work to vwrk to process the vert resizing. One call to vwrk per
			//    MCU column.

			// first check if a horz MCU row complete message is ready
		  	HorzResizeMsg hrm = PeekMsg_hrm(vImgIdx);
			if (!P1_bEndOfImage && RecvMsgReady_hrm(vImgIdx) &&
			    (hrm.m_imageHtId == P1_imageHtId)) {
				hrm = RecvMsg_hrm(vImgIdx);
				T1_hrm_bValid = true;
				T1_hrm = hrm;
				PW1_mcuRowComplete.write_addr(hrm.m_mcuRow);
				PW1_mcuRowComplete = true;
				HtContinue(VCTL_MSG_LOOP);
				break;
			}

			// next check if the next MCU row is complete
			P1_bEndOfImage = PR1_mcuRowCompletionCnt == S_jobInfo[vImgIdx].m_inMcuRows;
			if ((PR1_mcuRowComplete || P1_persMode == 2) && !P1_bEndOfImage) {
				PW1_mcuRowComplete.write_addr(P1_mcuRowCompletionCnt);
				PW1_mcuRowComplete = false;
				P1_mcuRowCompletionCnt += 1;
			}

			// finally check if there is work the for vert resizer
			if (S_vertWorkLock && S_vertWorkHtId != PR1_htId) {
				HtContinue(VCTL_MSG_LOOP);
				break;	// another thread is submitting work to vwrk, must wait
			}

			if ((P1_mcuRowCompletionCnt * DCTSIZE << (S_jobInfo[vImgIdx].m_maxBlkRowsPerMcu == 2 ? 1 : 0)) < P1_inImageRowEnd) {
				HtContinue(VCTL_MSG_LOOP);
				break;
			}

			S_vertWorkLock = true;
			S_vertWorkHtId = PR1_htId;

			BUSY_RETRY(SendCallBusy_vwrk());

			bool bEndOfImage = false;
			SendCallFork_vwrk(VCTL_JOIN, bEndOfImage, P1_bEndOfColumn, P1_imageIdx, P1_outMcuRowStart,
				P1_outMcuColStart, P1_inImageRowStart, P1_inImageRowEnd, P1_pntWghtStart, P1_pntWghtEnd );

			if (P1_outMcuColStart >= S_jobInfo[vImgIdx].m_outMcuCols-1) {
				P1_outMcuColStart = 0;
				P1_outMcuRowStart += VERT_MCU_ROWS;
				P1_inImageRowStart = P1_inImageRowEnd;

				S_vertWorkLock = false;

				ImageRows_t outImageRowFirst = (ImageRows_t)(P1_outMcuRowStart * DCTSIZE << (S_jobInfo[vImgIdx].m_maxBlkRowsPerMcu == 2 ? 1 : 0));
				P1_pntWghtStartRdAddr1 = outImageRowFirst & IMAGE_ROWS_MASK;

				if (P1_bEndOfColumn)
					RecvReturnPause_vwrk(VCTL_RETURN);
				else
					RecvReturnPause_vwrk(VCTL_CALC_START);
			} else {
				P1_outMcuColStart += 1;

				HtContinue(VCTL_MSG_LOOP);
			}
		}
		break;
		case VCTL_JOIN: {
			RecvReturnJoin_vwrk();
		}
		break;
		case VCTL_RETURN: {
			BUSY_RETRY(SendReturnBusy_vctl());
			// collect any unneeded messages from horz
			// this can occur when resizing to less than 1/16th
		  	HorzResizeMsg hrm = PeekMsg_hrm(vImgIdx);
			if (!P1_bEndOfImage && RecvMsgReady_hrm(vImgIdx) &&
			    (hrm.m_imageHtId == P1_imageHtId)) {
				hrm = RecvMsg_hrm(vImgIdx);
				PW1_mcuRowComplete.write_addr(hrm.m_mcuRow);
				PW1_mcuRowComplete = true;
				HtContinue(VCTL_RETURN);
				break;
			}
			// next check if the next MCU row is complete
			P1_bEndOfImage = PR1_mcuRowCompletionCnt == S_jobInfo[vImgIdx].m_inMcuRows;
			if ((PR1_mcuRowComplete || P1_persMode == 2) && !P1_bEndOfImage) {
				PW1_mcuRowComplete.write_addr(P1_mcuRowCompletionCnt);
				PW1_mcuRowComplete = false;
				P1_mcuRowCompletionCnt += 1;
			}
			P1_bEndOfImage = PR1_mcuRowCompletionCnt == S_jobInfo[vImgIdx].m_inMcuRows;
			if (P1_bEndOfImage)
				SendReturn_vctl();
			else
				HtContinue(VCTL_RETURN);
		}
		break;
		default:
			assert(0);
		}
	}

	RecvJobInfo();
}

void CPersVctl::RecvJobInfo()
{
	// Receive vinfo messages, these may come in any order so
	//   keep a count to know when all have arrived

	if (!GR_htReset && !RecvMsgBusy_veInfo()) {
		JobInfoMsg veInfoMsg = RecvMsg_veInfo();

		ht_uint1 vImgIdx = veInfoMsg.m_imageIdx & VERT_REPL_MASK;

		if ((veInfoMsg.m_imageIdx >> 1) == i_replId.read() && veInfoMsg.m_sectionId == JOB_INFO_SECTION_VERT) {

			switch (veInfoMsg.m_rspQw) {
			case 0: // first quadword of jobInfo.m_vert
				{
					S_jobInfo[vImgIdx].m_inImageRows = (veInfoMsg.m_data >> 2) & IMAGE_ROWS_MASK;
					S_jobInfo[vImgIdx].m_outImageRows = (veInfoMsg.m_data >> 30) & IMAGE_ROWS_MASK;
					S_jobInfo[vImgIdx].m_maxBlkRowsPerMcu = (veInfoMsg.m_data >> 58) & 0x3;
				}
				break;
			case 1: // second quadword of jobInfo.m_vert
				{
					S_jobInfo[vImgIdx].m_inMcuRows = (veInfoMsg.m_data >> 0) & MCU_ROWS_MASK;
					S_jobInfo[vImgIdx].m_outMcuCols = (veInfoMsg.m_data >> 33) & MCU_COLS_MASK;
				}
				break;
			case 18: // eighteenth quadword
				{
					S_jobInfo[vImgIdx].m_filterWidth = (veInfoMsg.m_data >> 0) & 0x1f;
				}
				break;
			default:
				{
					if (veInfoMsg.m_rspQw >= START_OF_PNT_INFO_QOFF &&
						veInfoMsg.m_rspQw - START_OF_PNT_INFO_QOFF <= S_jobInfo[vImgIdx].m_outImageRows)
					{
						PntWghtAddr_t wrAddr = (PntWghtAddr_t)(veInfoMsg.m_rspQw - START_OF_PNT_INFO_QOFF);
						GW1_pntWghtStart.write_addr(wrAddr, vImgIdx);
						GW1_pntWghtStart = (PntWghtCpInt_t)veInfoMsg.m_data;
					}
				}
				break;
			}
		}
	}
}
#endif
