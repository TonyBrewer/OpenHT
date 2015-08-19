/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(DEC) && defined(IHUF2)

#include "Ht.h"
#include "PersIhuf2.h"
#include "PersIhuf2_src.h"
#include "JobInfoOffsets.h"

#ifndef _HTV
//FILE *fp = fopen("Both_nb.txt", "w");
#include "JobInfo.h"
#define ASSERT_MSG assert_msg
#else
#define ASSERT_MSG(...)
#endif

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void CPersIhuf2::PersIhuf2()
{
	//
	// Threads are used to prefetch data into memBuf's
	//
	bool bRdDone = P1_pInPtr >= P1_pInPtrEnd;

	if (PR1_htValid) {
		switch (PR1_htInst) {
		case IHUF_ENTRY: {
			S_htId[P1_pid] = PR1_htId;
			S_paused[P1_pid] = false;
			S_jobId = P1_jobId;
			S_imageIdx = P1_imageIdx;
			S_rstIdx[P1_pid] = P1_rstIdx;
			S_numBlk = P1_numBlk;
			S_compCnt = P1_compCnt;
			S_mcuCols = P1_mcuCols;
			S_firstRstMcuCnt[P1_pid] = P1_firstRstMcuCnt;
			for (int j=0; j<IHUF_MEM_CNT; j++)
				S_bufRdy[P1_pid][j] = false;

			S_bufSel[P1_pid] = 0;
			S_bufRa[P1_pid] = P1_pInPtr & ((1<<6)-1);

			P1_pInPtr = ((P1_pInPtr) >> 6) << 6;

			HtContinue(IHUF_READ);
		}
		break;
		case IHUF_READ: {
			BUSY_RETRY(ReadMemBusy());
			BUSY_RETRY(S_bufRdy[P1_pid][P1_bufSel]);

			if (bRdDone) {
				if (P1_cnt)
					ReadMemPause(IHUF_RD_WAIT);
				else {
					S_paused[P1_pid] = true;
					HtPause(IHUF_XFER);
				}
				break;
			}

			ReadMem_memBuf(P1_pInPtr, P1_bufSel, P1_cnt * 8, P1_pid, 8);
			P1_pInPtr += 64;

			if (P1_cnt++ == IHUF_MEM_LINES-1) {
				P1_cnt = 0;
				ReadMemPause(IHUF_RD_WAIT);
			} else
				HtContinue(IHUF_READ);
		}
		break;
		case IHUF_RD_WAIT: {
			S_bufRdy[P1_pid][P1_bufSel] = true;
			P1_bufSel += 1u;

			if (bRdDone) {
				S_paused[P1_pid] = true;
				//printf("ReplId %d, Pausing PR1_htId %d, P1_pid %d\n", (int)i_replId.read(), (int)PR1_htId, (int)P1_pid);
				HtPause(IHUF_XFER);
			} else
				HtContinue(IHUF_READ);
		}
		break;
		case IHUF_XFER: {
			BUSY_RETRY(SendTransferBusy_idct());

			SendTransfer_idct(P1_pid, P1_rstIdx);
		}
		break;
		default:
			assert(0);
		}
	}

	//
	// Main block decode loop is hand rolled
	//
#define BLK_IDLE	0
#define BLK_DEC0	1
#define BLK_DEC1	2
#define BLK_DEC2	3
#define BLK_DEC3	4
#define BLK_RDY		5
#define BLK_WAIT	6
#define BLK_DONE	7
#define BLK_DUMP_ZERO	8
#define BLK_DUMP_MARKER	9

#ifndef _HTV
	static int bitsLeftStall = 0;
	static int coefQueStall = 0;
#endif

	for (int i=0; i<DPIPE_CNT; i++) {
		uint8_t nextByte = (uint8_t)(S_memBuf[i].read_mem()>>(8*(SR_bufRa[i]&0x7)));
		T1_nextByte[i] = nextByte;

		// Lookup current symbol
		IhufLookup lookup  = S_lookup[i].read_mem();
		ht_int16 huffCode = S_huffCode[i].read_mem();
		ht_int18 maxCode = S_maxCode[i].read_mem();
		ht_uint8 valOff = S_valOff[i].read_mem();

		ht_uint4 skipK = 0;
		ht_uint1 incK = 0;
		ht_uint4 dropBits = 0;
		ht_uint4 doneDropBits = (ht_uint4)(SR_bitsLeft[i] & 0x7);

		bool bBlkRdy = false;
		bool bBlkDone = false;
		bool bCoefVal = false;
		bool bValueZero = true;
		bool bClrDc = false;

		T1_coefMsgQueSize[i] = S_coefMsgQue[i].size();
		bool bCoefMsgFull  = T1_coefMsgQueSize[i] > ((1<<9) - 72);
		bool bCoefMsgEmpty = S_coefMsgQue[i].empty();

#ifndef _HTV
		long long time = HT_CYCLE();
		if (time == 0x1da8)
			bool stop = true;
#endif

		switch (S_state[i]) {
		case BLK_IDLE:
			{
				if (SR_bufRdy[i][SR_bufSel[i]] && !bCoefMsgFull)
					S_state[i] = BLK_DEC0;

				bClrDc = true;

				S_bLastFF[i] = false;
				S_nextByteVal[i] = false;
				S_bitBuffer[i] = 0;
				S_bitsLeft[i] = 0;
				S_mcuCnt[i] = 0;
				S_firstRst[i] = S_firstRstMcuCnt[i] != 0;
			}
			break;
		case BLK_DEC0:
			{
				if (S_bitsLeft[i] < 8) {
#ifndef _HTV
					bitsLeftStall += 1;
#endif
					break;
				}

				ht_uint4 huffBits = lookup.m_huffBits;
				S_valueBits[i] = lookup.m_valueBits;

				assert(GR_htReset || (huffBits + S_valueBits[i] <= 8) == lookup.m_bFastDec);

				if (lookup.m_bFastDec) {
					skipK = lookup.m_skipK;
					incK = 1;

					T1_huffExtendValue[i] = peekBits(S_bitBuffer[i], lookup.m_peekBits);
					T1_huffExtendBits[i] = S_valueBits[i];

					assert(huffBits + S_valueBits[i] == lookup.m_peekBits);

					bValueZero = lookup.m_bValueZero;
					dropBits = lookup.m_peekBits;

					bCoefVal = true;

				} else if (huffBits <= HUFF_LOOKAHEAD) {
					S_saveSkipK[i] = lookup.m_skipK;

					assert(GR_htReset || ((huffBits + S_valueBits[i]) & 0xf) == lookup.m_peekBits);
					S_dec1Buf[i] = peekBits(S_bitBuffer[i], lookup.m_peekBits & 7);

					dropBits = lookup.m_peekBits & 7;
					S_dec1Bytes[i] = (huffBits + S_valueBits[i]) >> 3;

					S_state[i] = BLK_DEC1;

				} else {
					// start iterations for huffman symbol larger than LOOKAHEAD bits
					S_iterBuf[i] = peekBits(S_bitBuffer[i], 8) & 0xff;
					S_iterBits[i] = 9;

					dropBits = 8;

					S_state[i] = BLK_DEC2;
				}
			}
			break;
		case BLK_DEC1:
			{
				if (S_bitsLeft[i] < 8) {
#ifndef _HTV
					bitsLeftStall += 1;
#endif
					break;
				}

				S_dec1Buf[i] = (ht_int16)(((SR_dec1Buf[i] << 8) | peekBits(S_bitBuffer[i], 8)));

				dropBits = 8;

				T1_huffExtendValue[i] = (ht_int16)((SR_dec1Buf[i] << 8) | peekBits(S_bitBuffer[i], 8));
				T1_huffExtendBits[i] = S_valueBits[i];

				if (S_dec1Bytes[i] == 1) {
					skipK = S_saveSkipK[i];
					incK = 1;

					bValueZero = S_valueBits[i] == 0;

					S_state[i] = BLK_DEC0;

					bCoefVal = true;
				}

				S_dec1Bytes[i] -= 1;
			}
			break;
		case BLK_DEC2:
			{
				if (S_bitsLeft[i] < 8) {
#ifndef _HTV
					bitsLeftStall += 1;
#endif
					break;
				}

				S_iterBuf[i] = (ht_int20)((S_iterBuf[i] << 1) | (peekBits(S_bitBuffer[i], 1) & 1));
				dropBits = 1;

				if (S_iterBuf[i] > maxCode)
					S_iterBits[i] += 1;
				else
					S_state[i] = BLK_DEC3;

				S_huffCodeAddr[i] = (S_iterBuf[i] + valOff) & 0xFF;
			}
			break;
		case BLK_DEC3:
			{
				if (S_bitsLeft[i] < 8) {
#ifndef _HTV
					bitsLeftStall += 1;
#endif
					break;
				}

				S_valueBits[i] = huffCode & 0xf;

				if (S_valueBits[i] <= 8) {
					skipK = (huffCode >> 4) & 0xf;
					incK = 1;

					T1_huffExtendValue[i] = peekBits(S_bitBuffer[i], S_valueBits[i]);
					T1_huffExtendBits[i] = S_valueBits[i];

					bValueZero = S_valueBits[i] == 0;

					dropBits = S_valueBits[i];

					S_state[i] = BLK_DEC0;

					bCoefVal = true;
				} else {
					S_saveSkipK[i] = (huffCode >> 4) & 0xf;

					S_dec1Buf[i] = peekBits(S_bitBuffer[i], S_valueBits[i] & 7);// & ((1 << (S_valueBits[i] & 7))-1);

					dropBits = S_valueBits[i] & 7;
					S_dec1Bytes[i] = S_valueBits[i] >> 3;

					S_state[i] = BLK_DEC1;
				}
			}
			break;
		case BLK_RDY:
			{
				bool bRstBlkDone = false;
				McuCols_t rstCols = S_firstRst[i] ? S_firstRstMcuCnt[i] : (McuCols_t)SR_mcuCols;
				if (SR_mcuBlkCol[i]+1 == S_blkColsPerMcu[ SR_compIdx[i] ]) {
					S_mcuBlkCol[i] = 0;
					if (SR_mcuBlkRow[i]+1 == S_blkRowsPerMcu[ SR_compIdx[i] ]) {
						S_mcuBlkRow[i] = 0;
						if (SR_compIdx[i]+1 == S_compCnt) {
							S_compIdx[i] = 0;
							if (SR_mcuCnt[i]+1 == rstCols) {
								bRstBlkDone = true;
								S_mcuCnt[i] = 0;
							} else
								S_mcuCnt[i] += 1;
						} else
							S_compIdx[i] += 1;
					} else
						S_mcuBlkRow[i] += 1u;
				} else
					S_mcuBlkCol[i] += 1u;

				bBlkRdy = true;
				bBlkDone = SR_blkCnt[i] == (SR_numBlk - 1);

				S_k[i] = 0;

				if (bBlkDone) {
					S_blkCnt[i] = 0;

					S_bufRdy[i][SR_bufSel[i]] = false;
					S_state[i] = BLK_DONE;

				} else if (bRstBlkDone) {
					S_blkCnt[i] += 1;
					// take an additional cycle to avoid longer wc timing path
					//if (S_bitsLeft[i] > 7) {
					//	dropBits = doneDropBits;
					//}
					S_state[i] = BLK_DUMP_ZERO;
				} else {
					S_blkCnt[i] += 1;

					if (!bCoefMsgFull)
						S_state[i] = BLK_DEC0;
					else {
						S_state[i] = BLK_WAIT;
#ifndef _HTV
						coefQueStall += 1;
#endif
					}
				}
			}
			break;
		case BLK_WAIT:
			{
				if (!bCoefMsgFull)
					S_state[i] = BLK_DEC0;
				else {
#ifndef _HTV
					coefQueStall += 1;
#endif
				}
			}
			break;
		case BLK_DUMP_ZERO:
		{
			if (S_bitsLeft[i] > 7) {
				if (doneDropBits != 0)
					dropBits = doneDropBits;
				else {
					dropBits = 8;
					assert_msg(peekBits(S_bitBuffer[i], 8) == 0xff,
							"Expected first 8 bits of restart end marker to be 0xff\n");
					S_state[i] = BLK_DUMP_MARKER;
				}
			}
		}
		break;
		case BLK_DUMP_MARKER:
		{
			// bLastFF logic dumps the byte after 0xff
			bClrDc = true;
			S_firstRst[i] = false;
			if (!bCoefMsgFull)
				S_state[i] = BLK_DEC0;
			else {
				S_state[i] = BLK_WAIT;
#ifndef _HTV
				coefQueStall += 1;
#endif
			}
		}
		break;
		case BLK_DONE:
			{
				// wait for prev reset to be sent to Idct
				if (!T2_coefMsgVal[i] && !T3_coefMsgVal[i] && !T4_coefMsgVal[i] && !T5_coefMsgVal[i] && !bCoefMsgEmpty)
					break;

				// wait for thread to pause before resuming
				if (!SR_paused[i])
					break;

				// wait for my TDM window to issue a HtResume
				if (i != S_tdmResume)
					break;

				HtResume(S_htId[i]);

				S_paused[i] = false;
				for (int j=0; j<IHUF_MEM_CNT; j++)
					S_bufRdy[i][j] = false;

				S_state[i] = BLK_IDLE;
			}
			break;
		default:
			assert(GR_htReset);
		}

		if (incK) {
			if (!bValueZero || SR_k[i] == 0)
				S_k[i] += skipK + incK;
			else if (skipK != 15)
				S_k[i] = 64;
			else
				S_k[i] += 16;

			if (S_k[i] >= 64)
				S_state[i] = BLK_RDY;
		}

		// fill bits
		bool bitBufferLoad = SR_state[i] != BLK_IDLE && SR_nextByteVal[i] && (SR_bitsLeft[i] <= 15 || S_bLastFF[i]);

		if (bitBufferLoad) {
			if (S_bLastFF[i])
				S_bLastFF[i] = false;
			else {
				S_bLastFF[i] = SR_nextByte[i] == 0xff;

				ht_uint4 sh = (ht_uint4)(15 - SR_bitsLeft[i]);
				S_bitBuffer[i] |= (ht_uint23)(SR_nextByte[i] << sh);
				S_bitsLeft[i] += 8u;

				//fprintf(fp, "(%d) rst %d, nb 0x%02x @ %lld\n", (int)i_replId.read(), (int)SR_rstIdx[i], nextByte, HT_CYCLE());
			}

			S_nextByteVal[i] = false;
		}

		bool bufRdy = SR_state[i] != BLK_IDLE &&
				  SR_bufRdy[i][SR_bufSel[i]];

		bool nextByteLoad = bufRdy && !S_nextByteVal[i];

		if (nextByteLoad) {
			S_nextByteVal[i] = true;
			S_nextByte[i] = nextByte;

			if (SR_bufRa[i] == ((1 << (IHUF_MEM_BUF_W+3))-1) ) {
				S_bufRdy[i][SR_bufSel[i]] = false;
				S_bufSel[i] += 1u;
			}
			S_bufRa[i] = SR_bufRa[i] + 1;
		}

		// drop bits
		assert(dropBits <= 8);
		assert(SR_bitsLeft[i] >= dropBits);
		S_bitBuffer[i] =  (ht_int23)(S_bitBuffer[i] << dropBits);
		S_bitsLeft[i] -= dropBits;

		// RAM registered addresses
		S_memBuf[i].read_addr(S_bufSel[i], S_bufRa[i]>>3);

		ht_uint1 dhtIdx = S_k[i] == 0 ? S_dcDhtId[S_compIdx[i]] : S_acDhtId[S_compIdx[i]];
		ht_uint2 luAddr1 = (S_k[i] == 0 ? 2u : 0) | dhtIdx;

		ht_uint8 luAddr2 = (ht_uint8)(S_bitBuffer[i] >> 15);
		S_lookup[i].read_addr(luAddr1, luAddr2);

		S_maxCode[i].read_addr(luAddr1, S_iterBits[i]);
		S_valOff[i].read_addr(luAddr1, S_iterBits[i]);

		S_huffCode[i].read_addr(luAddr1, S_huffCodeAddr[i]);

		// push message into FIFO
		T1_coefMsg[i].jobId = SR_jobId;
		T1_coefMsg[i].imageIdx = SR_imageIdx;
		T1_coefMsg[i].rstIdx = SR_rstIdx[i];
		T1_coefMsg[i].compIdx = SR_compIdx[i];
		T1_coefMsg[i].done = bBlkDone;
		T1_coefMsg[i].clr = bCoefVal && SR_k[i] == 0;
		T1_coefMsg[i].rdy = bBlkRdy;

		T1_coefMsg[i].dqtId = S_dcpDqtId[SR_compIdx[i]];

		T1_coefMsg[i].coef.val = bCoefVal;
		T1_coefMsg[i].coef.k = (SR_k[i] + skipK) & 0x3f;

		T1_coefMsgVal[i] = T1_coefMsg[i].done || T1_coefMsg[i].clr || T1_coefMsg[i].rdy
			    || T1_coefMsg[i].coef.val;

		T1_compIdx[i] = SR_compIdx[i];

		T1_bClrDc[i] = bClrDc;

		T2_coefMsg[i].coef.coef = T2_huffExtendValue[i] & ((1 << T2_huffExtendBits[i])-1);

		T3_huffExtendOp[i] = huffExtendOp(T3_coefMsg[i].coef.coef, T3_huffExtendBits[i]);

		T4_coefMsg[i].coef.coef += T4_huffExtendOp[i];

		if (T5_coefMsg[i].clr) {
			T5_coefMsg[i].coef.coef += SR_lastDcValue[i][T5_compIdx[i]];
			S_lastDcValue[i][T5_compIdx[i]] = T5_coefMsg[i].coef.coef;
		}
		if (T5_bClrDc[i]) { // cordMsg.clr and bClrDc will not be on at the same cycle
			for (int j=0; j<3; j++)
				S_lastDcValue[i][j] = 0;
		}

		//T3_coefMsg[i].coef.val &= T3_coefMsg[i].coef.coef != 0;

		if (T5_coefMsgVal[i]) {
			S_coefMsgQue[i].push(T5_coefMsg[i]);
		}

		// Idct i/f
		if (!S_coefMsgQue[i].empty() && !SendMsgBusy_coef(i)) {
			SendMsg_coef(i, S_coefMsgQue[i].front());
			S_coefMsgQue[i].pop();
		}
	}

	S_tdmResume = (S_tdmResume + 1) % DPIPE_CNT;

	RecvJobInfo();
}

void CPersIhuf2::RecvJobInfo()
{
	if (!GR_htReset && !RecvMsgBusy_jobInfo()) {
		JobInfoMsg msg = RecvMsg_jobInfo();

		if (msg.m_imageIdx == i_replId && msg.m_sectionId == DEC_SECTION) {
			// DCP
			ASSERT_MSG(offsetof(JobDec, m_dcp[0]) == DEC_DCP0_QOFF*8,
				"offsetof(JobDec, m_dcp[0]) == 0x%x\n",
				(int)offsetof(JobDec, m_dcp[0])/8);

			if (msg.m_rspQw == DEC_DCP0_QOFF) {
				S_blkRowsPerMcu[0] = (ht_uint2)(msg.m_data >> 8);
				S_blkColsPerMcu[0] = (ht_uint2)(msg.m_data >> 10);
				S_dcDhtId[0] = (ht_uint1)(msg.m_data >> 12);
				S_acDhtId[0] = (ht_uint1)(msg.m_data >> 13);
				S_dcpDqtId[0] = (ht_uint2)(msg.m_data >> 14);
			}
			if (msg.m_rspQw == DEC_DCP1_QOFF) {
				S_blkRowsPerMcu[1] = (ht_uint2)(msg.m_data >> 8);
				S_blkColsPerMcu[1] = (ht_uint2)(msg.m_data >> 10);
				S_dcDhtId[1] = (ht_uint1)(msg.m_data >> 12);
				S_acDhtId[1] = (ht_uint1)(msg.m_data >> 13);
				S_dcpDqtId[1] = (ht_uint2)(msg.m_data >> 14);
			}
			if (msg.m_rspQw == DEC_DCP2_QOFF) {
				S_blkRowsPerMcu[2] = (ht_uint2)(msg.m_data >> 8);
				S_blkColsPerMcu[2] = (ht_uint2)(msg.m_data >> 10);
				S_dcDhtId[2] = (ht_uint1)(msg.m_data >> 12);
				S_acDhtId[2] = (ht_uint1)(msg.m_data >> 13);
				S_dcpDqtId[2] = (ht_uint2)(msg.m_data >> 14);
			}

			// DC0 DHT
			ASSERT_MSG(offsetof(JobDec, m_dcDht[0].m_dhtTbls) == DEC_DCDHT0_HUFF_QOFF * 8,
				"DEC_DCDHT0_HUFF_QOFF = 0x%x\n", (int)offsetof(JobDec, m_dcDht[0].m_dhtTbls)/8);

			if (msg.m_rspQw >= DEC_DCDHT0_HUFF_QOFF && msg.m_rspQw < DEC_DCDHT0_HUFF_QOFF + DEC_DHT_HUFF_QCNT) {
				ht_uint8 idx = (msg.m_rspQw - DEC_DCDHT0_HUFF_QOFF) & 0xff;
				for (int i=0; i<DPIPE_CNT; i++) {
					S_huffCode[i].write_addr(2, idx);
					S_huffCode[i].write_mem(msg.m_data & 0xff);
				}
			}

			ASSERT_MSG(offsetof(JobDec, m_dcDht[0].m_dhtTbls) == DEC_DCDHT0_MAX_QOFF * 8,
				"DEC_DCDHT0_MAX_QOFF = 0x%x\n", (int)offsetof(JobDec, m_dcDht[0].m_dhtTbls)/8);

			if (msg.m_rspQw >= DEC_DCDHT0_MAX_QOFF && msg.m_rspQw < DEC_DCDHT0_MAX_QOFF + DEC_DHT_MAX_QCNT) {
				ht_uint5 idx = (msg.m_rspQw - DEC_DCDHT0_MAX_QOFF) & 0x1f;
				for (int i=0; i<DPIPE_CNT; i++) {
					S_maxCode[i].write_addr(2, idx);
					S_maxCode[i].write_mem( (msg.m_data >> 26) & 0x3ffff );
				}
			}

			ASSERT_MSG(offsetof(JobDec, m_dcDht[0].m_dhtTbls) == DEC_DCDHT0_VAL_QOFF * 8,
				"DEC_DCDHT0_VAL_QOFF = 0x%x\n", (int)offsetof(JobDec, m_dcDht[0].m_dhtTbls)/8);

			if (msg.m_rspQw >= DEC_DCDHT0_VAL_QOFF && msg.m_rspQw < DEC_DCDHT0_VAL_QOFF + DEC_DHT_VAL_QCNT) {
				ht_uint5 idx = (msg.m_rspQw - DEC_DCDHT0_VAL_QOFF) & 0x1f;
				for (int i=0; i<DPIPE_CNT; i++) {
					S_valOff[i].write_addr(2, idx);
					S_valOff[i].write_mem( (msg.m_data >> 44) & 0xff );
				}
			}

			ASSERT_MSG(offsetof(JobDec, m_dcDht[0].m_dhtTbls) == DEC_DCDHT0_LOOK_QOFF * 8,
				"DEC_DCDHT0_LOOK_QOFF = 0x%x\n", (int)offsetof(JobDec, m_dcDht[0].m_dhtTbls)/8);

			if (msg.m_rspQw >= DEC_DCDHT0_LOOK_QOFF && msg.m_rspQw < DEC_DCDHT0_LOOK_QOFF + DEC_DHT_LOOK_QCNT) {
				ht_uint8 idx = (msg.m_rspQw - DEC_DCDHT0_LOOK_QOFF) & 0xff;
				for (int i=0; i<DPIPE_CNT; i++) {
					IhufLookup lookup;
					lookup.m_u18 = (msg.m_data >> 8) & 0x3ffff;
					S_lookup[i].write_addr(2, idx);
					S_lookup[i].write_mem( lookup );
				}
			}

			// DC1 DHT
			ASSERT_MSG(offsetof(JobDec, m_dcDht[1].m_dhtTbls) == DEC_DCDHT1_HUFF_QOFF * 8,
				"DEC_DCDHT1_HUFF_QOFF = 0x%x\n", (int)offsetof(JobDec, m_dcDht[1].m_dhtTbls)/8);

			if (msg.m_rspQw >= DEC_DCDHT1_HUFF_QOFF && msg.m_rspQw < DEC_DCDHT1_HUFF_QOFF + DEC_DHT_HUFF_QCNT) {
				ht_uint8 idx = (msg.m_rspQw - DEC_DCDHT1_HUFF_QOFF) & 0xff;
				for (int i=0; i<DPIPE_CNT; i++) {
					S_huffCode[i].write_addr(3, idx);
					S_huffCode[i].write_mem(msg.m_data & 0xff);
				}
			}

			ASSERT_MSG(offsetof(JobDec, m_dcDht[1].m_dhtTbls) == DEC_DCDHT1_MAX_QOFF * 8,
				"DEC_DCDHT1_MAX_QOFF = 0x%x\n", (int)offsetof(JobDec, m_dcDht[1].m_dhtTbls)/8);

			if (msg.m_rspQw >= DEC_DCDHT1_MAX_QOFF && msg.m_rspQw < DEC_DCDHT1_MAX_QOFF + DEC_DHT_MAX_QCNT) {
				ht_uint5 idx = (msg.m_rspQw - DEC_DCDHT1_MAX_QOFF) & 0x1f;
				for (int i=0; i<DPIPE_CNT; i++) {
					S_maxCode[i].write_addr(3, idx);
					S_maxCode[i].write_mem( (msg.m_data >> 26) & 0x3ffff );
				}
			}

			ASSERT_MSG(offsetof(JobDec, m_dcDht[1].m_dhtTbls) == DEC_DCDHT1_VAL_QOFF * 8,
				"DEC_DCDHT1_VAL_QOFF = 0x%x\n", (int)offsetof(JobDec, m_dcDht[1].m_dhtTbls)/8);

			if (msg.m_rspQw >= DEC_DCDHT1_VAL_QOFF && msg.m_rspQw < DEC_DCDHT1_VAL_QOFF + DEC_DHT_VAL_QCNT) {
				ht_uint5 idx = (msg.m_rspQw - DEC_DCDHT1_VAL_QOFF) & 0x1f;
				for (int i=0; i<DPIPE_CNT; i++) {
					S_valOff[i].write_addr(3, idx);
					S_valOff[i].write_mem( (msg.m_data >> 44) & 0xff );
				}
			}

			ASSERT_MSG(offsetof(JobDec, m_dcDht[1].m_dhtTbls) == DEC_DCDHT1_LOOK_QOFF * 8,
				"DEC_DCDHT1_LOOK_QOFF = 0x%x\n", (int)offsetof(JobDec, m_dcDht[1].m_dhtTbls)/8);

			if (msg.m_rspQw >= DEC_DCDHT1_LOOK_QOFF && msg.m_rspQw < DEC_DCDHT1_LOOK_QOFF + DEC_DHT_LOOK_QCNT) {
				ht_uint8 idx = (msg.m_rspQw - DEC_DCDHT1_LOOK_QOFF) & 0xff;
				for (int i=0; i<DPIPE_CNT; i++) {
					IhufLookup lookup;
					lookup.m_u18 = (msg.m_data >> 8) & 0x3ffff;
					S_lookup[i].write_addr(3, idx);
					S_lookup[i].write_mem( lookup );
				}
			}

			// AC0 DHT
			ASSERT_MSG(offsetof(JobDec, m_acDht[0].m_dhtTbls) == DEC_ACDHT0_HUFF_QOFF * 8,
				"DEC_ACDHT0_HUFF_QOFF = 0x%x\n", (int)offsetof(JobDec, m_acDht[0].m_dhtTbls)/8);

			if (msg.m_rspQw >= DEC_ACDHT0_HUFF_QOFF && msg.m_rspQw < DEC_ACDHT0_HUFF_QOFF + DEC_DHT_HUFF_QCNT) {
				ht_uint8 idx = (msg.m_rspQw - DEC_ACDHT0_HUFF_QOFF) & 0xff;
				for (int i=0; i<DPIPE_CNT; i++) {
					S_huffCode[i].write_addr(0, idx);
					S_huffCode[i].write_mem(msg.m_data & 0xff);
				}
			}

			ASSERT_MSG(offsetof(JobDec, m_acDht[0].m_dhtTbls) == DEC_ACDHT0_MAX_QOFF * 8,
				"DEC_ACDHT0_MAX_QOFF = 0x%x\n", (int)offsetof(JobDec, m_acDht[0].m_dhtTbls)/8);

			if (msg.m_rspQw >= DEC_ACDHT0_MAX_QOFF && msg.m_rspQw < DEC_ACDHT0_MAX_QOFF + DEC_DHT_MAX_QCNT) {
				ht_uint5 idx = (msg.m_rspQw - DEC_ACDHT0_MAX_QOFF) & 0x1f;
				for (int i=0; i<DPIPE_CNT; i++) {
					S_maxCode[i].write_addr(0, idx);
					S_maxCode[i].write_mem( (msg.m_data >> 26) & 0x3ffff );
				}
			}

			ASSERT_MSG(offsetof(JobDec, m_acDht[0].m_dhtTbls) == DEC_ACDHT0_VAL_QOFF * 8,
				"DEC_ACDHT0_VAL_QOFF = 0x%x\n", (int)offsetof(JobDec, m_acDht[0].m_dhtTbls)/8);

			if (msg.m_rspQw >= DEC_ACDHT0_VAL_QOFF && msg.m_rspQw < DEC_ACDHT0_VAL_QOFF + DEC_DHT_VAL_QCNT) {
				ht_uint5 idx = (msg.m_rspQw - DEC_ACDHT0_VAL_QOFF) & 0x1f;
				for (int i=0; i<DPIPE_CNT; i++) {
					S_valOff[i].write_addr(0, idx);
					S_valOff[i].write_mem( (msg.m_data >> 44) & 0xff );
				}
			}

			ASSERT_MSG(offsetof(JobDec, m_acDht[0].m_dhtTbls) == DEC_ACDHT0_LOOK_QOFF * 8,
				"DEC_ACDHT0_LOOK_QOFF = 0x%x\n", (int)offsetof(JobDec, m_acDht[0].m_dhtTbls)/8);

			if (msg.m_rspQw >= DEC_ACDHT0_LOOK_QOFF && msg.m_rspQw < DEC_ACDHT0_LOOK_QOFF + DEC_DHT_LOOK_QCNT) {
				ht_uint8 idx = (msg.m_rspQw - DEC_ACDHT0_LOOK_QOFF) & 0xff;
				for (int i=0; i<DPIPE_CNT; i++) {
					IhufLookup lookup;
					lookup.m_u18 = (msg.m_data >> 8) & 0x3ffff;
					S_lookup[i].write_addr(0, idx);
					S_lookup[i].write_mem( lookup );
				}
			}

			// AC1 DHT
			ASSERT_MSG(offsetof(JobDec, m_acDht[1].m_dhtTbls) == DEC_ACDHT1_HUFF_QOFF * 8,
				"DEC_ACDHT1_HUFF_QOFF = 0x%x\n", (int)offsetof(JobDec, m_acDht[1].m_dhtTbls)/8);

			if (msg.m_rspQw >= DEC_ACDHT1_HUFF_QOFF && msg.m_rspQw < DEC_ACDHT1_HUFF_QOFF + DEC_DHT_HUFF_QCNT) {
				ht_uint8 idx = (msg.m_rspQw - DEC_ACDHT1_HUFF_QOFF) & 0xff;
				for (int i=0; i<DPIPE_CNT; i++) {
					S_huffCode[i].write_addr(1, idx);
					S_huffCode[i].write_mem(msg.m_data & 0xff);
				}
			}

			ASSERT_MSG(offsetof(JobDec, m_acDht[1].m_dhtTbls) == DEC_ACDHT1_MAX_QOFF * 8,
				"DEC_ACDHT1_MAX_QOFF = 0x%x\n", (int)offsetof(JobDec, m_acDht[1].m_dhtTbls)/8);

			if (msg.m_rspQw >= DEC_ACDHT1_MAX_QOFF && msg.m_rspQw < DEC_ACDHT1_MAX_QOFF + DEC_DHT_MAX_QCNT) {
				ht_uint5 idx = (msg.m_rspQw - DEC_ACDHT1_MAX_QOFF) & 0x1f;
				for (int i=0; i<DPIPE_CNT; i++) {
					S_maxCode[i].write_addr(1, idx);
					S_maxCode[i].write_mem( (msg.m_data >> 26) & 0x3ffff );
				}
			}

			ASSERT_MSG(offsetof(JobDec, m_acDht[1].m_dhtTbls) == DEC_ACDHT1_VAL_QOFF * 8,
				"DEC_ACDHT1_VAL_QOFF = 0x%x\n", (int)offsetof(JobDec, m_acDht[1].m_dhtTbls)/8);

			if (msg.m_rspQw >= DEC_ACDHT1_VAL_QOFF && msg.m_rspQw < DEC_ACDHT1_VAL_QOFF + DEC_DHT_VAL_QCNT) {
				ht_uint5 idx = (msg.m_rspQw - DEC_ACDHT1_VAL_QOFF) & 0x1f;
				for (int i=0; i<DPIPE_CNT; i++) {
					S_valOff[i].write_addr(1, idx);
					S_valOff[i].write_mem( (msg.m_data >> 44) & 0xff );
				}
			}

			ASSERT_MSG(offsetof(JobDec, m_acDht[1].m_dhtTbls) == DEC_ACDHT1_LOOK_QOFF * 8,
				"DEC_ACDHT1_LOOK_QOFF = 0x%x\n", (int)offsetof(JobDec, m_acDht[1].m_dhtTbls)/8);

			if (msg.m_rspQw >= DEC_ACDHT1_LOOK_QOFF && msg.m_rspQw < DEC_ACDHT1_LOOK_QOFF + DEC_DHT_LOOK_QCNT) {
				ht_uint8 idx = (msg.m_rspQw - DEC_ACDHT1_LOOK_QOFF) & 0xff;
				for (int i=0; i<DPIPE_CNT; i++) {
					IhufLookup lookup;
					lookup.m_u18 = (msg.m_data >> 8) & 0x3ffff;
					S_lookup[i].write_addr(1, idx);
					S_lookup[i].write_mem( lookup );
				}
			}
		}
	}
}

#endif
