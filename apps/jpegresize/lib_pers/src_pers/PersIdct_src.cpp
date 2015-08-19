/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#ifdef DEC

#include "Ht.h"
#include "PersIdct.h"
#include "ZigZag.h"
#include "JobInfoOffsets.h"

#ifndef _HTV
#include "JobInfo.h"
#define ASSERT_MSG assert_msg
#else
#define ASSERT_MSG(...)
#endif

#define RC(r,c) (ht_uint6)((r*8)+c)

#define MAXJSAMPLE	255
#define RANGE_MASK  (MAXJSAMPLE * 4 + 3) /* 2 bits wider than legal samples */

#define CONST_BITS  13
#define PASS1_BITS  2

#define FIX_0_298631336  ((int16_t)  2446)	/* FIX(0.298631336) */
#define FIX_0_390180644  ((int16_t)  3196)	/* FIX(0.390180644) */
#define FIX_0_541196100  ((int16_t)  4433)	/* FIX(0.541196100) */
#define FIX_0_765366865  ((int16_t)  6270)	/* FIX(0.765366865) */
#define FIX_0_899976223  ((int16_t)  7373)	/* FIX(0.899976223) */
#define FIX_1_175875602  ((int16_t)  9633)	/* FIX(1.175875602) */
#define FIX_1_501321110  ((int16_t)  12299)	/* FIX(1.501321110) */
#define FIX_1_847759065  ((int16_t)  15137)	/* FIX(1.847759065) */
#define FIX_1_961570560  ((int16_t)  16069)	/* FIX(1.961570560) */
#define FIX_2_053119869  ((int16_t)  16819)	/* FIX(2.053119869) */
#define FIX_2_562915447  ((int16_t)  20995)	/* FIX(2.562915447) */
#define FIX_3_072711026  ((int16_t)  25172)	/* FIX(3.072711026) */

/*
* Perform dequantization and inverse DCT on one block of coefficients.
*/
uint8_t jpegIdctRangeLimit(int x)
{
	if (x < 0x80)
		return x | 0x80;
	if (x < 0x200)
		return 0xff;
	if (x < 0x380)
		return 0;
	return x & 0x7f;
}

inline int idct_descale(int64_t x, int n) {
	return ( (int)x + (1 << (n-1))) >> n;
}


#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersIdct::PersIdct()
{
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case IDCT_ACTIVE: {
			BUSY_RETRY(SendReturnBusy_ihuf());

			if (S_resActCnt[P1_pid]) {
				HtContinue(IDCT_ACTIVE);
				break;
			}

			SendReturn_ihuf(P1_pid);
		}
		break;
		default:
			assert(0);
		}
	}

	//
	// Row machine
	//
	#define ROW_LOAD 0
	#define ROW_CALC 1
	#define ROW_DONE 2

	for (int c = 0; c < 8; c++) {
		S_coefBuf[SR_bufResSel][c].read_addr(SR_row, SR_bufActIdx[SR_bufResSel]);
	}

	uint8_t tmpCoefBufClr;
	tmpCoefBufClr = (uint8_t)(S_coefBufClr[SR_bufResSel][SR_bufActIdx[SR_bufResSel]] >> (SR_row<<3));
	bool coefBufClr[8];
	for (int  c=0; c<8; c++) {
		coefBufClr[c] = ((tmpCoefBufClr >> c) & 0x1) == 0x1;
	}

	for (int c = 0; c < 8; c++) {
		  S_quantBuf[c].read_addr(SR_row,SR_bufDqtId);
	}
	//
	bool bFound = false;
	S_nextResSel = SR_bufResSel;
	for (int i=0; i<DPIPE_CNT; i++) {
		assert(DPIPE_CNT <= 8);
		ht_uint4 modP = SR_bufResSel+1+i;
		if (modP > DPIPE_CNT-1) modP -= DPIPE_CNT;
		dpipe_id_t nxt = (dpipe_id_t)modP;
		if (!bFound && (S_coefBufCnt[nxt] != 0)) {
			bFound = true;
			S_nextResSel = nxt;
		}
	}

	S_wsInfo.write_addr(S_rowWsSel);

	switch (S_rowState) {
	case ROW_LOAD: {
		if (!S_coefBufCnt[SR_nextResSel] || S_wsCnt > 1)
		  S_rowState = ROW_LOAD;
		else
		  S_rowState = ROW_CALC;

		S_bufResSel = SR_nextResSel;
		S_bufDqtId = S_coefBufInfo[SR_nextResSel][S_bufActIdx[SR_nextResSel]].dqtId;
	}
	break;
	case ROW_CALC: {
		if (S_row == 0) {
			S_wsInfo.write_mem(S_coefBufInfo[SR_bufResSel][SR_bufActIdx[SR_bufResSel]]);
			S_wsLast[S_rowWsSel] = S_coefBufLast[SR_bufResSel][SR_bufActIdx[SR_bufResSel]];
		}

		if (S_row == 7) {
			S_bufActIdx[S_bufResSel] += 1u;
			S_coefBufCnt[S_bufResSel] -= 1u;

			S_rowWsSel += 1u;

			S_bufResSel = SR_nextResSel;
			if (!S_coefBufCnt[SR_nextResSel] || S_wsCnt > 1) {
				S_rowState = ROW_LOAD;
			}
			S_bufDqtId = S_coefBufInfo[SR_nextResSel][S_bufActIdx[SR_nextResSel]].dqtId;

		}

		S_row += 1;
	}
	break;
	default:
		assert(GR_htReset);
	}

	//
	// Row Datapath
	//
	T1_rowVal = SR_rowState == ROW_CALC;
	T1_row = SR_row;
	T1_rowWsSel = SR_rowWsSel;
	for (int c=0; c<8; c++) {
		T1_coefBlock[c] = coefBufClr[c] ? 0 :
				S_coefBuf[SR_bufResSel][c].read_mem();
		T1_quantTbl[c] = S_quantBuf[c].read_mem();
	}

//#define DUMP_COEF
#if defined(DUMP_COEF) && !defined(_HTV)
	{ static int coefCnt;
	if (T1_rowVal && S_coefBufInfo[SR_bufResSel][SR_bufActIdx[SR_bufResSel]].rstIdx==0) {
		if (T1_row==0)
			fprintf(stderr, "Coef(rstIdx=%d ci=%d cnt=%d) bufResSel=%d @ %lld\n",
				(int)S_coefBufInfo[SR_bufResSel][SR_bufActIdx[SR_bufResSel]].rstIdx,
				(int)S_coefBufInfo[SR_bufResSel][SR_bufActIdx[SR_bufResSel]].compIdx,
				coefCnt++, (int)SR_bufResSel,
				HT_CYCLE());
		fprintf(stderr, "%d  [%4d %4d %4d %4d %4d %4d %4d %4d]\n",
				(int)T1_row,
				(int)T1_coefBlock[0], (int)T1_coefBlock[1],
				(int)T1_coefBlock[2], (int)T1_coefBlock[3],
				(int)T1_coefBlock[4], (int)T1_coefBlock[5],
				(int)T1_coefBlock[6], (int)T1_coefBlock[7]);
	} }
#endif

	T2_a0 = (ht_int24)(T2_coefBlock[0] * T2_quantTbl[0]);
	T2_a1 = (ht_int24)(T2_coefBlock[2] * T2_quantTbl[2]);
	T2_a2 = (ht_int24)(T2_coefBlock[4] * T2_quantTbl[4]);
	T2_a3 = (ht_int24)(T2_coefBlock[6] * T2_quantTbl[6]);

	T3_b = (ht_int27)((T3_a1 + T3_a3) * FIX_0_541196100);

	T4_c0 = (ht_int27)((T4_a0 + T4_a2) << CONST_BITS);
	T4_c1 = (ht_int27)(T4_b + T4_a1 * FIX_0_765366865);
	T4_c2 = (ht_int27)((T4_a0 - T4_a2) << CONST_BITS);
	T4_c3 = (ht_int27)(T4_b + T4_a3 * - FIX_1_847759065);

	T5_d0 = (ht_int27)(T5_c0 + T5_c1);
	T5_d1 = (ht_int27)(T5_c0 - T5_c1);
	T5_d2 = (ht_int27)(T5_c2 + T5_c3);
	T5_d3 = (ht_int27)(T5_c2 - T5_c3);

	T2_e0 = (ht_int24)(T2_coefBlock[7] * T2_quantTbl[7]);
	T2_e1 = (ht_int24)(T2_coefBlock[5] * T2_quantTbl[5]);
	T2_e2 = (ht_int24)(T2_coefBlock[3] * T2_quantTbl[3]);
	T2_e3 = (ht_int24)(T2_coefBlock[1] * T2_quantTbl[1]);

	T3_f0 = (ht_int27)(T3_e0 + T3_e3);
	T3_f1 = (ht_int27)(T3_e1 + T3_e2);
	T3_f2 = (ht_int27)(T3_e0 + T3_e2);
	T3_f3 = (ht_int27)(T3_e1 + T3_e3);

	T3_g0 = (ht_int27)(T3_e0 * FIX_0_298631336);
	T3_g1 = (ht_int27)(T3_e1 * FIX_2_053119869);
	T3_g2 = (ht_int27)(T3_e2 * FIX_3_072711026);
	T3_g3 = (ht_int27)(T3_e3 * FIX_1_501321110);

	T4_h = (ht_int27)((T4_f2 + T4_f3) * FIX_1_175875602);

	T5_i0 = (ht_int27)(T5_f0 * - FIX_0_899976223);
	T5_i1 = (ht_int27)(T5_f1 * - FIX_2_562915447);
	T5_i2 = (ht_int27)(T5_h + T5_f2 * - FIX_1_961570560);
	T5_i3 = (ht_int27)(T5_h + T5_f3 * - FIX_0_390180644);

	T6_j0 = (ht_int27)(T6_g0 + T6_i0 + T6_i2);
	T6_j1 = (ht_int27)(T6_g1 + T6_i1 + T6_i3);
	T6_j2 = (ht_int27)(T6_g2 + T6_i1 + T6_i2);
	T6_j3 = (ht_int27)(T6_g3 + T6_i0 + T6_i3);

	for (int c=0; c<8; c++)
		S_ws[c].write_addr(T7_rowWsSel, T7_row);

	int16_t ws[8];
	ws[(ht_uint3)(0 + T7_row)] = (int16_t) idct_descale(T7_d0 + T7_j3, CONST_BITS-PASS1_BITS);
	ws[(ht_uint3)(4 + T7_row)] = (int16_t) idct_descale(T7_d1 - T7_j0, CONST_BITS-PASS1_BITS);
	ws[(ht_uint3)(2 + T7_row)] = (int16_t) idct_descale(T7_d3 + T7_j1, CONST_BITS-PASS1_BITS);
	ws[(ht_uint3)(6 + T7_row)] = (int16_t) idct_descale(T7_d2 - T7_j2, CONST_BITS-PASS1_BITS);
	ws[(ht_uint3)(1 + T7_row)] = (int16_t) idct_descale(T7_d2 + T7_j2, CONST_BITS-PASS1_BITS);
	ws[(ht_uint3)(5 + T7_row)] = (int16_t) idct_descale(T7_d3 - T7_j1, CONST_BITS-PASS1_BITS);
	ws[(ht_uint3)(3 + T7_row)] = (int16_t) idct_descale(T7_d1 + T7_j0, CONST_BITS-PASS1_BITS);
	ws[(ht_uint3)(7 + T7_row)] = (int16_t) idct_descale(T7_d0 - T7_j3, CONST_BITS-PASS1_BITS);
	if (T7_rowVal) {
		for (int c=0; c<8; c++)
			S_ws[c].write_mem(ws[c]);
	}

	if (T8_rowVal && T8_row == 7) S_wsCnt += 1;


	//
	// Column machine
	//

	#define COL_IDLE 0
	#define COL_CALC 1

	T1_decMsgQueSize = S_decMsgQue.size();
	S_wsInfo.read_addr(SR_colWsSel);

	switch (S_colState) {
	case COL_IDLE: {
		S_col = 0;
		if (!SR_wsCnt || T1_decMsgQueSize > 16)
			break;

		S_colState = COL_CALC;
	}
	break;
	case COL_CALC: {
		if (S_col == 7) {
			S_wsCnt -= 1;
			S_resActCnt[S_wsInfo.read_mem().dpipeId] -= 1;
			S_colWsSel += 1u;

			if (S_wsCnt == 0 || T1_decMsgQueSize > 16)
				S_colState = COL_IDLE;
		}

		S_col += 1;
	}
	break;
	default:
		assert(0);
	}

	//
	// Col Datapath
	//
	for (int c=0; c<8; c++)
		S_ws[c].read_addr(S_colWsSel, (ht_uint3)(c - S_col)); // blockRam so address is one cycle earlier
	T1_colVal = SR_colState == COL_CALC;
	T1_col = SR_col;
	T1_colWsSel = SR_colWsSel;
	T1_outColInfo = S_wsInfo.read_mem();
	T1_outColLast = S_wsLast[SR_colWsSel];
	for (int r=0; r<8; r++)
		T1_ws[r] = S_ws[(ht_uint3)(T1_col + r)].read_mem();

	T2_k = (ht_int28)((T2_ws[2] + T2_ws[6]) * FIX_0_541196100);

	T2_m0  = (ht_int28)((T2_ws[0] + T2_ws[4]) << CONST_BITS);
	T2_m1  = (ht_int28)((T2_ws[0] - T2_ws[4]) << CONST_BITS);
	T2_m2a = (ht_int28)(T2_ws[6] * - FIX_1_847759065);
	T2_m3a = (ht_int28)(T2_ws[2] * FIX_0_765366865);

	T3_m2 = (ht_int28)(T3_k + T3_m2a);
	T3_m3 = (ht_int28)(T3_k + T3_m3a);

	T4_n0 = (ht_int28)(T4_m0 + T4_m3);
	T4_n3 = (ht_int28)(T4_m0 - T4_m3);
	T4_n1 = (ht_int28)(T4_m1 + T4_m2);
	T4_n2 = (ht_int28)(T4_m1 - T4_m2);

	T2_p0 = (ht_int17)(T2_ws[7] + T2_ws[1]);
	T2_p1 = (ht_int17)(T2_ws[5] + T2_ws[3]);
	T2_p2 = (ht_int17)(T2_ws[7] + T2_ws[3]);
	T2_p3 = (ht_int17)(T2_ws[5] + T2_ws[1]);

	T2_r0 = (ht_int28)(T2_ws[7] * FIX_0_298631336);
	T2_r1 = (ht_int28)(T2_ws[5] * FIX_2_053119869);
	T2_r2 = (ht_int28)(T2_ws[3] * FIX_3_072711026);
	T2_r3 = (ht_int28)(T2_ws[1] * FIX_1_501321110);

	T3_q  = (ht_int28)((T3_p2 + T3_p3) * FIX_1_175875602);

	T4_s1 = (ht_int28)(T4_p0 * - FIX_0_899976223);
	T4_s2 = (ht_int28)(T4_p1 * - FIX_2_562915447);
	T4_s3 = (ht_int28)(T4_q + T4_p2 * - FIX_1_961570560);
	T4_s4 = (ht_int28)(T4_q + T4_p3 * - FIX_0_390180644);

	T5_t0 = (ht_int28)(T5_r0 + T5_s1 + T5_s3);
	T5_t1 = (ht_int28)(T5_r1 + T5_s2 + T5_s4);
	T5_t2 = (ht_int28)(T5_r2 + T5_s2 + T5_s3);
	T5_t3 = (ht_int28)(T5_r3 + T5_s1 + T5_s4);

	/* Final output stage */
	T6_outCol[0] = jpegIdctRangeLimit((int) idct_descale(T6_n0 + T6_t3, CONST_BITS+PASS1_BITS+3) & RANGE_MASK);
	T6_outCol[7] = jpegIdctRangeLimit((int) idct_descale(T6_n0 - T6_t3, CONST_BITS+PASS1_BITS+3) & RANGE_MASK);
	T6_outCol[1] = jpegIdctRangeLimit((int) idct_descale(T6_n1 + T6_t2, CONST_BITS+PASS1_BITS+3) & RANGE_MASK);
	T6_outCol[6] = jpegIdctRangeLimit((int) idct_descale(T6_n1 - T6_t2, CONST_BITS+PASS1_BITS+3) & RANGE_MASK);
	T6_outCol[2] = jpegIdctRangeLimit((int) idct_descale(T6_n2 + T6_t1, CONST_BITS+PASS1_BITS+3) & RANGE_MASK);
	T6_outCol[5] = jpegIdctRangeLimit((int) idct_descale(T6_n2 - T6_t1, CONST_BITS+PASS1_BITS+3) & RANGE_MASK);
	T6_outCol[3] = jpegIdctRangeLimit((int) idct_descale(T6_n3 + T6_t0, CONST_BITS+PASS1_BITS+3) & RANGE_MASK);
	T6_outCol[4] = jpegIdctRangeLimit((int) idct_descale(T6_n3 - T6_t0, CONST_BITS+PASS1_BITS+3) & RANGE_MASK);

//#define DUMP_BLOCK
#if defined(DUMP_BLOCK) && !defined(_HTV)
	if (T6_colVal && T6_outColInfo.rstIdx==0) {
		static int outBlock[8][8], blkCnt = 0;
		for (int i=0; i<8; i++)
			outBlock[i][T6_col] = T6_outCol[i];

		if (T6_col == 7) {
			fprintf(stderr, "Block(rstIdx=%d ci=%d cnt=%d) @ %lld\n",
				(int)T6_outColInfo.rstIdx,
				(int)T6_outColInfo.compIdx,
				blkCnt++,
				HT_CYCLE());
			for (int r=0; r<8; r++)
				fprintf(stderr, "  [  %02x   %02x   %02x   %02x   %02x   %02x   %02x   %02x]\n",
					outBlock[r][0], outBlock[r][1],
					outBlock[r][2], outBlock[r][3],
					outBlock[r][4], outBlock[r][5],
					outBlock[r][6], outBlock[r][7]);
		}
	}
#endif

	if (GR_htReset) {
		S_rowState = ROW_LOAD;
		S_colState = COL_IDLE;
	}

	//
	// Horz message i/f
	//
	if (!GR_htReset && T6_colVal) {
		JpegDecMsg msg;
		msg.m_jobId = T6_outColInfo.jobId;
		msg.m_imageIdx = T6_outColInfo.imageIdx;
		msg.m_rstIdx = T6_outColInfo.rstIdx;
		msg.m_decPipeId = T6_outColInfo.dpipeId;
		msg.m_compIdx = T6_outColInfo.compIdx;
		assert(msg.m_compIdx < 3);
		msg.m_bRstLast = T6_outColLast;
		for (int i=0; i<8; i++)
			msg.m_data.m_u8[i] = T6_outCol[i];
		S_decMsgQue.push(msg);
	}
	if (!S_decMsgQue.empty() && !SendMsgBusy_jdm()) {
		SendMsg_jdm(S_decMsgQue.front());
		S_decMsgQue.pop();
	};

	RecvCoef();
	RecvJobInfo();
}

void CPersIdct::RecvCoef()
{
	//
	// Load coef
	//
	for (int i=0; i<DPIPE_CNT; i++) {
		if (SR_coefBufCnt[i] < 2 && RecvMsgReady_coef(i)) {
			coefMsg thisMsg = RecvMsg_coef(i);
			ht_uint3 coefMsgR = zigZagRow(thisMsg.coef.k);
			ht_uint3 coefMsgC = zigZagCol(thisMsg.coef.k);
			ht_uint6 clrIndex = (ht_uint6)(coefMsgR<<3 | coefMsgC);

			for (int c=0; c<8; c++) {
				S_coefBuf[i][c].write_addr(coefMsgR, S_coefBufWrIdx[i]);
			}

			if (thisMsg.done) {
				S_coefBufLast[i][S_coefBufWrIdx[i]] = true;
			}
			if (thisMsg.clr) {
				S_coefBufClr[i][S_coefBufWrIdx[i]] = (uint64_t)0xffffffffffffffffll;
				idctInfo info;
				info.jobId = thisMsg.jobId;
				info.imageIdx = thisMsg.imageIdx;
				info.rstIdx = thisMsg.rstIdx;
				info.compIdx = thisMsg.compIdx;
				info.dqtId = thisMsg.dqtId;
				info.dpipeId = i;
				S_coefBufInfo[i][S_coefBufWrIdx[i]] = info;
			}

			if (thisMsg.coef.val) {
				S_coefBuf[i][coefMsgC].write_mem(thisMsg.coef.coef);
				S_coefBufClr[i][S_coefBufWrIdx[i]] &= ~(uint64_t)(1ll << clrIndex);
			}

			if (thisMsg.rdy) {
				S_coefBufWrIdx[i] += 1u;
				S_coefBufCnt[i] += 1;
				S_resActCnt[i] += 1;
			}
		}
	}

}

void CPersIdct::RecvJobInfo()
{
	if (!GR_htReset && !RecvMsgBusy_jobInfo()) {
		JobInfoMsg msg = RecvMsg_jobInfo();

		if (msg.m_imageIdx == i_replId && msg.m_sectionId == DEC_SECTION) {
			ASSERT_MSG(offsetof(struct JobDec, m_dqt[0]) == DEC_DQT0_QOFF*8,
					"offsetof(struct JobDec, m_dqt[0]) == 0x%x\n",
					(int)offsetof(struct JobDec, m_dqt[0])/8);

			if (msg.m_rspQw >= DEC_DQT0_QOFF && msg.m_rspQw < DEC_DQT0_QOFF+DEC_DQT_QCNT) {
				ht_uint4 qw = (ht_uint4)(msg.m_rspQw - (DEC_DQT0_QOFF & 0xf));
				ht_uint3 r = qw >> 1;
				for (int i = 0; i < 4; i++) {
					ht_uint3 c = (qw & 1) << 2 | i;
					int16_t dat = (int16_t)(msg.m_data >> (16 * i));
					S_quantBuf[c].write_addr(r,0);
					S_quantBuf[c].write_mem(dat);
				}
			}
			if (msg.m_rspQw >= DEC_DQT1_QOFF && msg.m_rspQw < DEC_DQT1_QOFF+DEC_DQT_QCNT) {
				ht_uint4 qw = (ht_uint4)(msg.m_rspQw - (DEC_DQT1_QOFF & 0xf));
				ht_uint3 r = qw >> 1;
				for (int i = 0; i < 4; i++) {
					ht_uint3 c = (qw & 1) << 2 | i;
					int16_t dat = (int16_t)(msg.m_data >> (16 * i));
					S_quantBuf[c].write_addr(r,1);
					S_quantBuf[c].write_mem(dat);
				}
			}
			if (msg.m_rspQw >= DEC_DQT2_QOFF && msg.m_rspQw < DEC_DQT2_QOFF+DEC_DQT_QCNT) {
				ht_uint4 qw = (ht_uint4)(msg.m_rspQw - (DEC_DQT2_QOFF & 0xf));
				ht_uint3 r = qw >> 1;
				for (int i = 0; i < 4; i++) {
					ht_uint3 c = (qw & 1) << 2 | i;
					int16_t dat = (int16_t)(msg.m_data >> (16 * i));
					S_quantBuf[c].write_addr(r,2);
					S_quantBuf[c].write_mem(dat);
				}
			}
			if (msg.m_rspQw >= DEC_DQT3_QOFF && msg.m_rspQw < DEC_DQT3_QOFF+DEC_DQT_QCNT) {
				ht_uint4 qw = (ht_uint4)(msg.m_rspQw - (DEC_DQT3_QOFF & 0xf));
				ht_uint3 r = qw >> 1;
				for (int i = 0; i < 4; i++) {
					ht_uint3 c = (qw & 1) << 2 | i;
					int16_t dat = (int16_t)(msg.m_data >> (16 * i));
					S_quantBuf[c].write_addr(r,3);
					S_quantBuf[c].write_mem(dat);
				}
			}
		}
	}
}
#endif
