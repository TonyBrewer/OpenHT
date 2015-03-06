/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(HSMW)

#include "Ht.h"
#include "PersHsmw.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

/////////////////////////////////////
// Mimic messaging from Jpeg Decoder
//	 Each decoder will handle a single image before transitioning the another image.
//   Each decoder can handle a fixed number of MCU rows at a time.
//   The number of rows that can be simultaneously handled is a build time parameter.
//   A mcu block is transmitted as eight successive message on consecutive clocks.
//   A restart is handled by a single IHUF module.

// Each thread:
//  1) reads the cache lines for its restart index
//  2) queues the data to be sent to the messaging interface
//  3) reads from the queue and sends the message to horz

void CPersHsmw::PersHsmw()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case HSMW_ENTRY: {

			P_compIdx = 0;
			P_mcuBlkRow = 0;
			P_mcuBlkCol = 0;
			P_rowInBlk = 0;

			P_mcuPendCnt = 0;
			P_readDone = false;
			P_pollDone = false;

			P_bRstFirst = true;

			P_rdReqGrpId = (PR_htId << 1) | S_readBufIdx[PR_htId];

			P_mcuCol = 0;
			P_mcuRow = P_rstIdx;

			HtContinue(HSMW_LOOP);
		}
		break;
		case HSMW_LOOP: {
			if (S_bufInUse[PR_htId] < 2 && !P_readDone && !ReadMemBusy() ) {

				// issue read
				ht_uint13 compRow = (ht_uint13)((P_mcuRow * S_dec.m_dcp[P_compIdx].m_blkRowsPerMcu + P_mcuBlkRow) * 8 + P_rowInBlk);

				if (compRow < S_dec.m_dcp[P_compIdx].m_compRows) {

					ht_uint27 rowPos = (ht_uint27)(compRow * S_dec.m_dcp[P_compIdx].m_compBufColLines * MEM_LINE_SIZE);
					ht_uint13 colPos = (ht_uint13)((P_mcuCol * S_dec.m_dcp[P_compIdx].m_blkColsPerMcu + (P_mcuBlkCol * 8)) * 8);
					assert(rowPos + colPos < S_dec.m_dcp[P_compIdx].m_compRows * S_dec.m_dcp[P_compIdx].m_compBufColLines * MEM_LINE_SIZE);

					ht_uint48 memAddr = S_dec.m_dcp[P_compIdx].m_pCompBuf + rowPos + colPos;

					sc_uint<HSMW_HTID_W+5> varAddr1 = (PR_htId << 5) | (S_readBufIdx[PR_htId] << 4) | (P_compIdx << 2) | (P_mcuBlkRow << 1) | P_mcuBlkCol;

					//printf("Read offset ci %d, mcuBlkCol %d, mcuBlkRow %d, rowPos %d, colPos %d, varAddr1 0x%x\n",
					//	(int)P_compIdx, (int)P_mcuBlkCol, (int)P_mcuBlkRow, (int)rowPos, (int)colPos, (int)varAddr1);

					ReadMem_mcuBuf(memAddr, varAddr1, 0, P_rowInBlk, 8);
				}

				if (P_rowInBlk+1 == DCTSIZE) {
					P_rowInBlk = 0;
					if (P_mcuBlkCol+1 == S_dec.m_dcp[P_compIdx].m_blkColsPerMcu) {
						P_mcuBlkCol = 0;
						if (P_mcuBlkRow+1 == S_dec.m_dcp[P_compIdx].m_blkRowsPerMcu) {
							P_mcuBlkRow = 0;
							if (P_compIdx+1 == S_dec.m_compCnt) {
								P_compIdx = 0;

								S_readBufIdx[PR_htId] ^= 1;
								P_rdReqGrpId = (PR_htId << 1) | S_readBufIdx[PR_htId];

								P_mcuPendCnt += 1;
								S_bufInUse[PR_htId] += 1;

								if (P_mcuCol+8 >= S_dec.m_mcuCols) {
									P_mcuCol = 0;
									P_mcuRow += 1;

									P_readDone = S_dec.m_rstMcuCnt > 0 || P_mcuRow >= S_dec.m_mcuRows;
								} else
									P_mcuCol += 8;

								//printf("McuRead done: PR_htId %d, S_mcuBufInUseCnt[%d] %d, P_mcuReadPendCnt %d, P_readBufIdx %d\n",
								//	(int)PR_htId, (int)PR_htId, (int)S_mcuBufInUseCnt[PR_htId], (int)P_mcuReadPendCnt, (int)P_readBufIdx);
							} else
								P_compIdx += 1;
						} else
							P_mcuBlkRow += 1;
					} else
						P_mcuBlkCol += 1;
				} else
					P_rowInBlk += 1;
			}

			sc_uint<HSMW_HTID_W+1> pollReqGrpId = (PR_htId << 1) | S_pollBufIdx[PR_htId];
			if (P_mcuPendCnt > 0 && !S_hsmwIhufQue[PR_htId].full() && !ReadMemPoll(pollReqGrpId)) {
				HsmwIhufMsg msg;

				msg.m_jobId = 0;
				msg.m_imageIdx = P_imageIdx;
				msg.m_rstIdx = (ht_uint10)P_rstIdx;
				msg.m_bufIdx = S_pollBufIdx[PR_htId];
				msg.m_bRstFirst = P_bRstFirst;
				msg.m_bRstLast = false;
				msg.m_bEndOfImage = false;

				P_bRstFirst = false;

				S_hsmwIhufQue[PR_htId].push( msg );

				P_mcuPendCnt -= 1;
				S_pollBufIdx[PR_htId] ^= 1;
			}

			if (P_readDone && P_mcuPendCnt == 0)
				HtContinue(HSMW_RETURN);
			else
				HtContinue(HSMW_LOOP);
		}
		break;
		case HSMW_RETURN: {
			BUSY_RETRY( SendReturnBusy_hsmw() );

			SendReturn_hsmw();
		}
		break;
		default:
			assert(0);
		}
	}

	T1_bPushJdm = false;

	if (!GR_htReset && !S_hsmwIhufQue[S_ihufQueIdx].empty() && !SendMsgBusy_jdm()) {
		HsmwIhufMsg msg = S_hsmwIhufQue[S_ihufQueIdx].front();

		if (msg.m_bRstFirst)
			S_mcuRow[S_ihufQueIdx] = msg.m_rstIdx;

		T1_bPushJdm = true;
		T1_jdm.m_compIdx = S_compIdx[S_ihufQueIdx];
		T1_jdm.m_decPipeId = S_ihufQueIdx;		// ihuf index
		T1_jdm.m_rstIdx = msg.m_rstIdx;
		T1_jdm.m_bRstLast = false;		// last data for restart
		T1_jdm.m_bEndOfImage = false;	// last data for image
		T1_jdm.m_bEndOfMcuRow = false;
		T1_jdm.m_imageIdx = msg.m_imageIdx;		// image index for double buffering
		T1_jdm.m_jobId = msg.m_jobId;		// host job info index (for debug)

		T1_outCol = S_outCol[S_ihufQueIdx];
		T1_mcuCol = S_mcuCol[S_ihufQueIdx];
		T1_mcuBlkCol = S_mcuBlkCol[S_ihufQueIdx];
		T1_mcuBlkRow = S_mcuBlkRow[S_ihufQueIdx];

		ht_uint10 blkInRow = (ht_uint10)(S_mcuCol[S_ihufQueIdx] * S_dec.m_dcp[S_compIdx[S_ihufQueIdx]].m_blkColsPerMcu + S_mcuBlkCol[S_ihufQueIdx]);

		ht_uint1 blkColSel = S_dec.m_dcp[S_compIdx[S_ihufQueIdx]].m_blkColsPerMcu == 1 ? 0 : ((S_mcuCol[S_ihufQueIdx] >> 2) & 1);
		sc_uint<HSMW_HTID_W+5> varAddr1 = (sc_uint<HSMW_HTID_W+5>)((S_ihufQueIdx << 5) | (msg.m_bufIdx << 4) | (S_compIdx[S_ihufQueIdx] << 2) |
			(S_mcuBlkRow[S_ihufQueIdx] << 1) | blkColSel);

		for (int r = 0; r < 8; r += 1)
			S_mcuBuf[r].read_addr(varAddr1, blkInRow & 7);

		if (S_outCol[S_ihufQueIdx] == 7) {
			S_outCol[S_ihufQueIdx] = 0;

			if (S_mcuBlkCol[S_ihufQueIdx]+1 == S_dec.m_dcp[S_compIdx[S_ihufQueIdx]].m_blkColsPerMcu) {
				S_mcuBlkCol[S_ihufQueIdx] = 0;
				if (S_mcuBlkRow[S_ihufQueIdx]+1 == S_dec.m_dcp[S_compIdx[S_ihufQueIdx]].m_blkRowsPerMcu) {
					S_mcuBlkRow[S_ihufQueIdx] = 0;
					if (S_compIdx[S_ihufQueIdx]+1 == S_dec.m_compCnt) {
						S_compIdx[S_ihufQueIdx] = 0;

						if ((S_mcuCol[S_ihufQueIdx] & 7) == 7 || S_mcuCol[S_ihufQueIdx]+1 == S_dec.m_mcuCols) {
							//printf("S_mcuRow 0x%x, S_mcuCol 0x%x\n", (int)S_mcuRow[S_ihufQueIdx], (int)S_mcuCol[S_ihufQueIdx]);

							if (S_mcuCol[S_ihufQueIdx]+1 == S_dec.m_mcuCols) {
								S_mcuCol[S_ihufQueIdx] = 0;

								T1_jdm.m_bEndOfMcuRow = true;

								if (S_dec.m_rstMcuCnt > 0)
									T1_jdm.m_bRstLast = true;

								if (S_mcuRow[S_ihufQueIdx]+1 == S_dec.m_mcuRows) {
									S_mcuRow[S_ihufQueIdx] = 0;
									T1_jdm.m_bEndOfImage = true;
									T1_jdm.m_bRstLast = true;
								} else
									S_mcuRow[S_ihufQueIdx] += 1;

							} else
								S_mcuCol[S_ihufQueIdx] += 1;

							S_bufInUse[S_ihufQueIdx] -= 1;

							S_hsmwIhufQue[S_ihufQueIdx].pop();
						} else
							S_mcuCol[S_ihufQueIdx] += 1;

						//printf("McuRead done: PR_htId %d, S_mcuBufInUseCnt[%d] %d, P_mcuReadPendCnt %d, P_readBufIdx %d\n",
						//	(int)PR_htId, (int)PR_htId, (int)S_mcuBufInUseCnt[PR_htId], (int)P_mcuReadPendCnt, (int)P_readBufIdx);
					} else
						S_compIdx[S_ihufQueIdx] += 1;
				} else
					S_mcuBlkRow[S_ihufQueIdx] += 1;
			} else
				S_mcuBlkCol[S_ihufQueIdx] += 1;

			if (S_skipIdx == 4)
				S_skipIdx = 0;
			else
#if HSMW_HTID_W==0
				S_ihufQueIdx = 0;
#else
				S_ihufQueIdx = S_ihufQueIdx + 1u;
#endif
		} else
			S_outCol[S_ihufQueIdx] += 1;
	} else
#if HSMW_HTID_W==0
		S_ihufQueIdx = 0;
#else
		S_ihufQueIdx = S_ihufQueIdx + 1u;
#endif

	if (!GR_htReset && TR2_bPushJdm) {

		for (int r = 0; r < 8; r += 1)
			T2_jdm.m_data.m_u8[r] = (S_mcuBuf[r].read_mem() >> (TR2_outCol * 8)) & 0xff;

#ifndef _HTV
		static uint64_t DV_mcuRow[(1 << HSMW_HTID_W)];
		static bool DV_bRstFirst[1 << HSMW_HTID_W] = { true, true, true, true };

		if (DV_bRstFirst[T2_jdm.m_decPipeId]) {
			DV_mcuRow[T2_jdm.m_decPipeId] = T2_jdm.m_rstIdx;
			DV_bRstFirst[T2_jdm.m_decPipeId] = false;
		}

		uint64_t blkRow = DV_mcuRow[TR2_jdm.m_decPipeId] * S_dec.m_dcp[TR2_jdm.m_compIdx].m_blkRowsPerMcu + TR2_mcuBlkRow;

		for (int r = 0; r < 8 && (blkRow * 8) + r < S_dec.m_dcp[TR2_jdm.m_compIdx].m_compRows; r += 1) {

			uint64_t rowPos = ((DV_mcuRow[TR2_jdm.m_decPipeId] * S_dec.m_dcp[TR2_jdm.m_compIdx].m_blkRowsPerMcu + TR2_mcuBlkRow) * 8 + r) * S_dec.m_dcp[TR2_jdm.m_compIdx].m_compBufColLines * MEM_LINE_SIZE;

			uint64_t colPos = (TR2_mcuCol * S_dec.m_dcp[TR2_jdm.m_compIdx].m_blkColsPerMcu + TR2_mcuBlkCol) * 8 + TR2_outCol;
			assert(rowPos + colPos < S_dec.m_dcp[TR2_jdm.m_compIdx].m_compRows * S_dec.m_dcp[TR2_jdm.m_compIdx].m_compBufColLines * MEM_LINE_SIZE);

			uint8_t * pMemAddr = (uint8_t *)(uint64_t)S_dec.m_dcp[TR2_jdm.m_compIdx].m_pCompBuf + rowPos + colPos;

			if (*pMemAddr != T2_jdm.m_data.m_u8[r])
				bool stop = true;
			//assert(*pMemAddr == T2_jdm.m_data.m_u8[r]);
		}

		if (TR2_jdm.m_bRstLast)
			DV_bRstFirst[T2_jdm.m_decPipeId] = true;

		if (TR2_jdm.m_bEndOfMcuRow)
			DV_mcuRow[T2_jdm.m_decPipeId] += 1;
#endif

		SendMsg_jdm( T2_jdm );
	}

	RecvJobInfo();
}

void CPersHsmw::RecvJobInfo()
{
	// Receive hinfo messages, these may come in any order so
	//   keep a count to know when all have arrived

	if (!GR_htReset && !RecvMsgBusy_jobInfo()) {
		JobInfoMsg hinfoMsg = RecvMsg_jobInfo();

		if (hinfoMsg.m_sectionId == JOB_INFO_SECTION_DEC) {

			switch (hinfoMsg.m_rspQw) {
			case 0: // second quadword of jobInfo.m_dec
				{
					S_dec.m_compCnt = (hinfoMsg.m_data >> 0) & 0x3;
					S_dec.m_mcuRows = (hinfoMsg.m_data >> 34) & 0x7ff;
					S_dec.m_mcuCols = (hinfoMsg.m_data >> 45) & 0x7ff;
				}
				break;
			case 1: // second quadword of jobInfo.m_dec
				{
					S_dec.m_rstMcuCnt = (hinfoMsg.m_data >> 0) & 0x7ff;
					S_dec.m_rstCnt = (hinfoMsg.m_data >> 16) & 0x7ff;
				}
				break;
			case START_OF_DEC_DCP_QW:
			case START_OF_DEC_DCP_QW+2:
			case START_OF_DEC_DCP_QW+4:
				{
					S_dec.m_dcp[((hinfoMsg.m_rspQw - START_OF_DEC_DCP_QW) >> 1) & 0x3].m_blkRowsPerMcu = (hinfoMsg.m_data >> 8) & 0x3;
					S_dec.m_dcp[((hinfoMsg.m_rspQw - START_OF_DEC_DCP_QW) >> 1) & 0x3].m_blkColsPerMcu = (hinfoMsg.m_data >> 10) & 0x3;
					S_dec.m_dcp[((hinfoMsg.m_rspQw - START_OF_DEC_DCP_QW) >> 1) & 0x3].m_compRows = (hinfoMsg.m_data >> 16) & 0x3fff;
					S_dec.m_dcp[((hinfoMsg.m_rspQw - START_OF_DEC_DCP_QW) >> 1) & 0x3].m_compCols = (hinfoMsg.m_data >> 30) & 0x3fff;
					S_dec.m_dcp[((hinfoMsg.m_rspQw - START_OF_DEC_DCP_QW) >> 1) & 0x3].m_compBufColLines = (hinfoMsg.m_data >> 44) & 0xff;
				}
				break;
			case START_OF_DEC_DCP_QW+1:
			case START_OF_DEC_DCP_QW+3:
			case START_OF_DEC_DCP_QW+5:
				{
					S_dec.m_dcp[((hinfoMsg.m_rspQw - START_OF_DEC_DCP_QW-1) >> 1) & 0x3].m_pCompBuf = hinfoMsg.m_data & 0xffffffffffffLL;
				}
				break;
			default:
				break;
			}
		}
	}
}

#endif
