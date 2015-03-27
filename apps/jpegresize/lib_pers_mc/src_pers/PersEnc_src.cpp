/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "PersConfig.h"

#if defined(ENC)

#include "Ht.h"
#include "PersEnc.h"
#include "JobInfoOffsets.h"
#include "ZigZag.h"

#ifndef _HTV
#include "JobInfo.h"
#define ASSERT_MSG assert_msg
#else
#define ASSERT_MSG(...)
#endif

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

static int16_t descale(int x, int n) {
	return ( (int)x + (1 << (n-1))) >> n;
}

void
CPersEnc::PersEnc()
{
	EncImages_t t1_eImgIdx = P1_imageIdx & ENC_IMAGES_MASK;

	bool bFound = false;
	epipe_id_t nxtSel = P1_pipeSel;
	for (int i=0; i<EPIPE_CNT; i++) {
		epipe_id_t nxt = (epipe_id_t)(P1_pipeSel+1+i);
		if (!bFound && SR_memBufCnt[nxt][t1_eImgIdx]) {
			bFound = true;
			nxtSel = nxt;
		}
	}


	for (int i=0; i<EPIPE_CNT; i++)
		S_memBufInfo[i].read_addr(S_memBufRdSel[nxtSel]);
	eMemInfo nxtBufInfo = S_memBufInfo[nxtSel].read_mem();

	T1_nxtVal = bFound;
	T1_nxtSel = nxtSel;
	T1_nxtBufInfo = nxtBufInfo;

	EncImages_t t2_eImgIdx = P2_imageIdx & ENC_IMAGES_MASK;
	//assert(S_jobInfo[t2_eImgIdx].m_rstOffset == (ht_uint20)S_jobInfo[t2_eImgIdx].m_rstOffset);
	T2_rstOff = (ht_uint20)S_jobInfo[t2_eImgIdx].m_rstOffset * T2_nxtBufInfo.m_rstIdx;
	T2_rstAdd = S_jobInfo[t2_eImgIdx].m_pRstBase + P2_len[T2_nxtSel];

	ASSERT_MSG(offsetof(struct JobInfo, m_enc) == ENC_OFFSET,
			"ENC_OFFSET = 0x%x\n",
			(int)offsetof(struct JobInfo, m_enc));

	ASSERT_MSG(offsetof(struct JobEnc, m_rstLength) == ENC_RSTLEN_DOFF*4,
			"ENC_RSTLEN_DOFF = 0x%x\n",
			(int)offsetof(struct JobEnc, m_rstLength)/4);

	T2_pLenBuf = P2_pJobInfo + ENC_OFFSET + ENC_RSTLEN_DOFF * 4;

	if (PR3_htValid) {
		EncImages_t t3_eImgIdx = P3_imageIdx & ENC_IMAGES_MASK;

		switch (PR3_htInst) {
		case ENC_WRITE: {
			BUSY_RETRY(WriteMemBusy());

#if defined(FORCE_ENC_STALLS) && !defined(_HTV)
			static int stallCnt = 0;
			if (stallCnt < 20) {
				stallCnt += 1;
				BUSY_RETRY(true);
			} else
				stallCnt = 0;
#endif

			if (!T3_nxtVal) {
				HtContinue(ENC_WRITE);
				break;
			}

			P3_pipeSel = T3_nxtSel;
			P3_memBufInfo = T3_nxtBufInfo;

			ht_uint48 addr = T3_rstAdd + T3_rstOff;
			if (P3_memBufInfo.m_len) {
				WriteMem_memBuf(addr, S_memBufRdSel[P3_pipeSel], 0, P3_pipeSel, 8);
				WriteReqPause(ENC_WAIT);
			} else
				HtContinue(ENC_WAIT);
		}
		break;
		case ENC_WAIT: {
			P3_len[P3_pipeSel] += P3_memBufInfo.m_len;

			S_memBufCnt[P3_pipeSel][t3_eImgIdx] -= 1;
			S_memBufRdSel[P3_pipeSel] += 1u;

			if (!P3_memBufInfo.m_last) {
				HtContinue(ENC_WRITE);
			} else
				HtContinue(ENC_LENGTH);
		}
		break;
		case ENC_LENGTH: {
			BUSY_RETRY(WriteMemBusy());

			ht_uint48 addr = T3_pLenBuf + P3_memBufInfo.m_rstIdx * 4;
			WriteMem_uint32(addr, P3_len[P3_pipeSel]);

			P3_len[P3_pipeSel] = 0;
			P3_rstCnt += 1;

			EncImages_t t3_eImgIdx = P3_imageIdx & ENC_IMAGES_MASK;
			if (P3_rstCnt < S_jobInfo[t3_eImgIdx].m_rstCnt)
				HtContinue(ENC_WRITE);
			else
				WriteMemPause(ENC_RETURN);
		}
		break;
		case ENC_RETURN: {
			BUSY_RETRY(SendReturnBusy_enc());

			SendReturn_enc();
		}
		break;
		default:
			assert(0);
		}
	}

	RecvJobInfo();
	RecvVertMsg();
	Fdct();
	Enc();
}

////////////////////////////////////////

#define ESM_IDLE	0
#define ESM_DC0		1
#define ESM_DC1		2
#define ESM_DC2		3
#define ESM_AC		4
#define ESM_ZERO	5
#define ESM_WAIT	6

static ht_uint5 numBits(uint16_t bits) {
	ht_uint5 num = 0;
	for (int i=0; i<16; i++) {
		if ((bits >> i) & 1)
			num = i+1;
	}
	return num;
}

void CPersEnc::Enc()
{
 for (int i=0; i<EPIPE_CNT; i++) {

	S_cBlkInfo[i].read_addr(S_cBlkRdIdx[i]);

	T1_bubble[i] = false;
	T1_valid[i] = false;
	T1_nbits[i] = 0;
	T1_data[i] = 0;
	T1_size[i] = 0;
	T1_flush[i] = false;

	Eight_i16 cBlkQw = S_cBlks[i].read_mem();
	S_coef[i] = cBlkQw.m_i16[zigZagRow(S_k[i]+1)];

	switch (S_state[i]) {
	case ESM_IDLE: {
		S_memQw[i] = 0;
		S_memBufWa[i] = 0;
		S_k[i] = 63;

		for (int j=0; j<3; j++)
			S_lastDcValueRef[i][j] = 0;

		if (SR_cBlkCnt[i])
			S_state[i] = ESM_DC0;
	}
	break;
	case ESM_DC0: {
		if (!SR_cBlkCnt[i])
			break;

		S_bDc[i] = 1;

		S_esmInfo[i] = S_cBlkInfo[i].read_mem();

		S_state[i] = ESM_DC1;
	}
	break;
	case ESM_DC1: {
		S_dcTmp[i] = SR_coef[i] - S_lastDcValueRef[i][S_esmInfo[i].m_compIdx];
		S_lastDcValueRef[i][S_esmInfo[i].m_compIdx] = SR_coef[i];

		S_k[i] += 1;
		S_state[i] = ESM_DC2;
	}
	break;
	case ESM_DC2: {
		int16_t tmp = S_dcTmp[i];
		int16_t tmp2 = tmp;

		if (tmp < 0) {
			tmp = -tmp;
			tmp2 -= 1;
		}

		T1_valid[i] = true;
		T1_nbits[i] = numBits(tmp);
		T1_data[i] = tmp2;
		T1_size[i] = (ht_uint5)T1_nbits[i];

		S_r[i] = 0;
		S_k[i] += 1;
		S_state[i] = ESM_AC;
	}
	break;
	case ESM_AC: {
		if (S_words[i].size() > 8) {
			T1_bubble[i] = true;
			break;
		}

		S_bDc[i] = 0;

		int16_t tmp = SR_coef[i];
		ht_uint4 nbitsP = (ht_uint4)numBits(tmp);
		ht_uint4 nbitsN = (ht_uint4)numBits(-tmp);
		ht_uint4 nbits = (tmp < 0) ? nbitsN : nbitsP;

		if (!tmp) {
			S_r[i] += 1;
		} else {
			if (S_r[i] > 15) {
				S_r[i] -= 16;
				T1_bubble[i] = true;
				S_state[i] = ESM_ZERO;
				break;
			}

			int16_t tmp2 = tmp;
			if (tmp < 0) {
				tmp = -tmp;
				tmp2 -= 1;
			}

			T1_valid[i] = true;
			T1_nbits[i] = (uint8_t)((S_r[i] << 4) | nbits);
			T1_data[i] = tmp2;
			T1_size[i] = nbits;

			S_r[i] = 0;
		}

		if (S_k[i] == 63) {
			S_cBlkRdIdx[i] += 1;
			S_cBlkWrCnt[i] -= 1;
			S_cBlkCnt[i] -= 1;

			if (S_r[i])
				S_state[i] = ESM_ZERO;
			else {
				if (S_esmInfo[i].m_bEndOfMcuRow) {
					T1_flush[i] = true;
					S_state[i] = ESM_WAIT;
				} else {
					S_state[i] = ESM_DC0;
					break;
				}
			}
		}

		S_k[i] += 1;
	}
	break;
	case ESM_ZERO: {
		bool eob = S_k[i] == 0 /*63+1*/;

		T1_bubble[i] = true;
		T1_valid[i] = true;
		T1_nbits[i] = eob ? 0 : 0xf0;
		T1_data[i] = 0;
		T1_size[i] = 0;
		T1_flush[i] = eob ? S_esmInfo[i].m_bEndOfMcuRow : false;

		if (eob) {
			if (S_esmInfo[i].m_bEndOfMcuRow)
				S_state[i] = ESM_WAIT;
			else {
				S_k[i] = 63;
				S_state[i] = ESM_DC0;
			}
		} else
			S_state[i] = ESM_AC;
	}
	break;
	case ESM_WAIT: {
		EncImages_t esm_eImgIdx = S_esmInfo[i].m_imageIdx & ENC_IMAGES_MASK;

		if (T2_valid[i] || T3_valid[i] || S_words[i].size() ||
		    S_bitValCnt[i] || S_bytes[i].size() ||
		    S_memBufCnt[i][esm_eImgIdx])
			break;

		S_state[i] = ESM_IDLE;
	}
	break;
	default:
		assert(0);
	}

	S_cBlks[i].read_addr(S_cBlkRdIdx[i], zigZagCol((ht_uint6)(S_k[i] + 1)));

	if (T1_bubble[i]) S_coef[i] = SR_coef[i];


	//
	// encode datapath
	//
	EncImages_t esm_eImgIdx = S_esmInfo[i].m_imageIdx & ENC_IMAGES_MASK;
	ht_uint2 esm_compIdx = S_esmInfo[i].m_compIdx;

	T1_adr1[i]  = (ht_uint3)((ht_uint3)esm_eImgIdx << 2);
	T1_adr1[i] |= (ht_uint3)((ht_uint3)S_bDc[i] << 1);
	if (S_bDc[i])
		T1_adr1[i] |= S_jobInfo[esm_eImgIdx].m_ecp[esm_compIdx].m_dcDhtId;
	else
		T1_adr1[i] |= S_jobInfo[esm_eImgIdx].m_ecp[esm_compIdx].m_acDhtId;

	S_huffCode[i].read_addr(T1_adr1[i], T1_nbits[i]>>2);
	S_huffSize[i].read_addr(T1_adr1[i], T1_nbits[i]>>3);

	T2_code[i]  = (uint16_t)(S_huffCode[i].read_mem() >> (16*(T2_nbits[i] & 0x3)));
	T2_hsize[i] = (ht_uint5)(S_huffSize[i].read_mem() >> (8*(T2_nbits[i] & 0x7)));

	uint16_t code = T3_code[i] & ~((uint16_t)-1 << T3_hsize[i]);
	uint32_t data = T3_data[i] & ~((uint32_t)-1 << T3_size[i]);

	eWords word;
	word.m_flush = T3_flush[i];
	word.m_word  = (uint32_t)(((uint32_t)code << T3_size[i]) | data);
	word.m_size = T3_hsize[i] + T3_size[i];
	if (T3_valid[i])
		S_words[i].push(word);

	//
	// bitstream
	//
	if (!S_words[i].empty() && S_bitValCnt[i] < 16) {
		eWords w = S_words[i].front();
		S_words[i].pop();
		assert_msg(S_bitValCnt[i] + w.m_size <= 48,
			"S_bitValCnt[%d](%d) + w.m_size(%d) > 48\n",
			i, (int)S_bitValCnt[i], (int)w.m_size);
		S_bitFlush[i] |= w.m_flush;
		S_bitValCnt[i] += w.m_size;
		S_bitBuffer[i] = (ht_uint48)((S_bitBuffer[i] << w.m_size) | w.m_word);
	}

	bool empty = S_bytes[i].empty();

	if (S_bitValCnt[i] > 7 && !S_bytes[i].full()) {
		ht_uint6 sh = (ht_uint6)(S_bitValCnt[i] - 8);

		eBytes byte;
		byte.m_flush = false;
		byte.m_val = true;
		byte.m_u8 = (uint8_t)(S_bitBuffer[i] >> sh);
		S_bytes[i].push(byte);

		S_bitValCnt[i] -= 8;
		S_bitBuffer[i] &= ~(uint64_t)(0xffull << sh);
	} else if (S_bitFlush[i] && !S_bytes[i].full()) {
		ht_uint3 sh = (ht_uint3)(8 - S_bitValCnt[i]);

		eBytes byte;
		byte.m_flush = true;
		byte.m_val = !!S_bitValCnt[i];
		byte.m_u8 = (uint8_t)(S_bitBuffer[i] << sh);
		S_bytes[i].push(byte);

		S_bitValCnt[i] = 0;
		S_bitBuffer[i] = 0;
		S_bitFlush[i] = false;
	}

	//
	// memBuf
	//
	S_memBuf[i].write_addr(S_memBufWrSel[i], S_memBufWa[i]>>3);
	S_memBufInfo[i].write_addr(S_memBufWrSel[i]);

	if ((!empty || S_memPushZero[i]) && SR_memBufCnt[i][esm_eImgIdx] < 2) {
		bool flush = S_bytes[i].front().m_flush;
		bool val = S_bytes[i].front().m_val;

		ht_uint3 bidx = S_memBufWa[i] & 0x7;
		if (!S_memPushZero[i]) {
			uint8_t byte = S_bytes[i].front().m_u8;
			S_memQw[i].m_u8[bidx] = byte;
			if (byte == 0xff)
				S_memPushZero[i] = true;
			else
				S_bytes[i].pop();
		} else {
			S_memQw[i].m_u8[bidx] = 0;
			S_memPushZero[i] = false;
			S_bytes[i].pop();
		}

		if ((S_memBufWa[i] & 0x7) == 7 || flush) {
			S_memBuf[i].write_mem(S_memQw[i].m_u64);
			S_memQw[i] = 0;
			if (S_memBufWa[i] == 63 || flush) {
				ht_uint7 len = !flush ? (ht_uint7)64 :
					       (ht_uint7)(S_memBufWa[i] + val);

				eMemInfo info;
				info.m_rstIdx = S_esmInfo[i].m_rstIdx;
				info.m_last = flush;
				info.m_len = len;
				S_memBufInfo[i].write_mem(info);

				assert(S_memBufCnt[i][esm_eImgIdx] <= 2);
				S_memBufCnt[i][esm_eImgIdx] += 1;
				S_memBufWrSel[i] += 1u;
			}
		}
		S_memBufWa[i] += 1;
	}

	if (GR_htReset)
		S_state[i] = ESM_IDLE;
 }
}

////////////////////////////////////////

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

#define JPEG_ENC_PASS1_BITS	2
#define JPEG_ENC_CONST_BITS	13
#define JPEG_ENC_RECIP_BITS	8

inline int8_t rmBias(uint8_t x) { return (int8_t)(x - 128); }

void CPersEnc::Fdct()
{
	#define ROW_IDLE 0
	#define ROW_LOOP 1

	S_vBlkInfo.read_addr(S_vBlkRdIdx);
	S_wsInfo.write_addr(S_rowWsSel);

	Eight_u8 inBlock;
	inBlock.m_u64 = S_vBlks.read_mem();

	S_rowVal = false;

	switch(S_rowState) {
	case ROW_IDLE: {
		if (!SR_vBlkCnt || S_wsCnt > 1)
			break;
		S_wsInfo.write_mem(S_vBlkInfo.read_mem());

		S_rowVal = true;

		S_rowState = ROW_LOOP;
	}
	break;
	case ROW_LOOP: {
		if (S_row == 7) {
			S_blkCol = 0;
			S_vBlkCnt -= 1;
			S_vBlkRdIdx += 1u;
			S_rowWsSel += 1u;

			S_rowState = ROW_IDLE;
		}

		S_rowVal = true;
		S_row += 1;
	}
	break;
	default:
		assert(0);
	}

	S_vBlks.read_addr(S_vBlkRdIdx, S_row);

	//
	// Row datapath
	//
	T1_row = SR_row;
	T1_rowVal = SR_rowVal;
	T1_rowWsSel = SR_rowWsSel;

	T1_a0 = rmBias(inBlock.m_u8[0]) + rmBias(inBlock.m_u8[7]);
	T1_a1 = rmBias(inBlock.m_u8[1]) + rmBias(inBlock.m_u8[6]);
	T1_a2 = rmBias(inBlock.m_u8[2]) + rmBias(inBlock.m_u8[5]);
	T1_a3 = rmBias(inBlock.m_u8[3]) + rmBias(inBlock.m_u8[4]);

	T2_b0 = T2_a0 + T2_a3;
	T2_b3 = T2_a0 - T2_a3;
	T2_b1 = T2_a1 + T2_a2;
	T2_b2 = T2_a1 - T2_a2;

	T3_c = (T3_b2 + T3_b3) * FIX_0_541196100;

	T4_d0 = (T4_b0 + T4_b1) << JPEG_ENC_PASS1_BITS;
	T4_d1 = (T4_b0 - T4_b1) << JPEG_ENC_PASS1_BITS;
	T4_d2 = T4_c + T4_b3 * FIX_0_765366865;
	T4_d3 = T4_c + T4_b2 * - FIX_1_847759065;

	T1_e0 = rmBias(inBlock.m_u8[3]) - rmBias(inBlock.m_u8[4]);
	T1_e1 = rmBias(inBlock.m_u8[2]) - rmBias(inBlock.m_u8[5]);
	T1_e2 = rmBias(inBlock.m_u8[1]) - rmBias(inBlock.m_u8[6]);
	T1_e3 = rmBias(inBlock.m_u8[0]) - rmBias(inBlock.m_u8[7]);

	T2_f4 = T2_e0 + T2_e3;
	T2_f5 = T2_e1 + T2_e2;
	T2_f6 = T2_e0 + T2_e2;
	T2_f7 = T2_e1 + T2_e3;

	T3_g = (T3_f6 + T3_f7) * FIX_1_175875602;

	T4_h0 = T4_e0 * FIX_0_298631336;
	T4_h1 = T4_e1 * FIX_2_053119869;
	T4_h2 = T4_e2 * FIX_3_072711026;
	T4_h3 = T4_e3 * FIX_1_501321110;

	T5_i0 = T5_f4 * - FIX_0_899976223;
	T5_i1 = T5_f5 * - FIX_2_562915447;
	T5_i2 = T5_g + T5_f6 * - FIX_1_961570560;
	T5_i3 = T5_g + T5_f7 * - FIX_0_390180644;

	T6_j0 = T6_h0 + T6_i0 + T6_i2;
	T6_j1 = T6_h1 + T6_i1 + T6_i3;
	T6_j2 = T6_h2 + T6_i1 + T6_i2;
	T6_j3 = T6_h3 + T6_i0 + T6_i3;

	int16_t ws[8];
	ws[(ht_uint3)(0 + T7_row)] = (int16_t) T7_d0;
	ws[(ht_uint3)(4 + T7_row)] = (int16_t) T7_d1;
	ws[(ht_uint3)(2 + T7_row)] = descale((int)T7_d2, JPEG_ENC_CONST_BITS-JPEG_ENC_PASS1_BITS);
	ws[(ht_uint3)(6 + T7_row)] = descale((int)T7_d3, JPEG_ENC_CONST_BITS-JPEG_ENC_PASS1_BITS);

	ws[(ht_uint3)(7 + T7_row)] = descale((int)T7_j0, JPEG_ENC_CONST_BITS-JPEG_ENC_PASS1_BITS);
	ws[(ht_uint3)(5 + T7_row)] = descale((int)T7_j1, JPEG_ENC_CONST_BITS-JPEG_ENC_PASS1_BITS);
	ws[(ht_uint3)(3 + T7_row)] = descale((int)T7_j2, JPEG_ENC_CONST_BITS-JPEG_ENC_PASS1_BITS);
	ws[(ht_uint3)(1 + T7_row)] = descale((int)T7_j3, JPEG_ENC_CONST_BITS-JPEG_ENC_PASS1_BITS);

	for (int c=0; c<8; c++)
		S_ws[c].write_addr(T7_rowWsSel, T7_row);

	if (T7_rowVal) {
		for (int c=0; c<8; c++)
			S_ws[c].write_mem(ws[c]);
	}

	if (T8_rowVal && T8_row == 7) S_wsCnt += 1;

	/////////

	#define COL_IDLE 0
	#define COL_LOOP 1

	S_wsInfo.read_addr(S_colWsSel);

	EncImages_t wsInfo_eImgIdx = S_wsInfo.read_mem().m_imageIdx & ENC_IMAGES_MASK;
	ht_uint2 wsInfo_compIdx = S_wsInfo.read_mem().m_compIdx;

	S_colVal = false;

	switch(S_colState) {
	case COL_IDLE: {
		S_colImgIdx = wsInfo_eImgIdx;
		S_colPipe = (epipe_id_t)S_wsInfo.read_mem().m_rstIdx;
		S_colBlkIdx = S_cBlkWrIdx[S_colPipe];
		S_colDqt = S_jobInfo[wsInfo_eImgIdx].m_ecp[wsInfo_compIdx].m_dqtId;

		if (!SR_wsCnt || S_cBlkWrCnt[S_colPipe] > 5)
			break;

		for (int i=0; i<EPIPE_CNT; i++)
			S_cBlkInfo[i].write_addr(S_colBlkIdx);
		S_cBlkInfo[S_colPipe].write_mem(S_wsInfo.read_mem());

		S_cBlkWrCnt[S_colPipe] += 1;
		S_cBlkWrIdx[S_colPipe] += 1;

		S_colVal = true;

		S_colState = COL_LOOP;
	}
	break;
	case COL_LOOP: {
		if (S_col == 7) {
			S_colWsSel += 1u;
			S_wsCnt -= 1;
			S_colVal = false;
			S_colState = COL_IDLE;
		} else
			S_colVal = true;

		S_col += 1;
	}
	break;
	default:
		assert(0);
	}

	for (int c=0; c<8; c++)
		S_ws[c].read_addr(S_colWsSel, (ht_uint3)(c - S_col)); // blockRam so address is one cycle earlier
	//
	// Col datapath
	//
	T1_col = SR_col;
	T1_colVal = SR_colVal;
	T1_colWsSel = SR_colWsSel;
	T1_colDqt = SR_colDqt;
	T1_colImgIdx = SR_colImgIdx;
	T1_colPipe = SR_colPipe;
	T1_colBlkIdx = SR_colBlkIdx;
	T1_colDqt = SR_colDqt;

	T1_k0 = S_ws[(ht_uint3)(T1_col + 0)].read_mem() + S_ws[(ht_uint3)(T1_col + 7)].read_mem();
	T1_k1 = S_ws[(ht_uint3)(T1_col + 1)].read_mem() + S_ws[(ht_uint3)(T1_col + 6)].read_mem();
	T1_k2 = S_ws[(ht_uint3)(T1_col + 2)].read_mem() + S_ws[(ht_uint3)(T1_col + 5)].read_mem();
	T1_k3 = S_ws[(ht_uint3)(T1_col + 3)].read_mem() + S_ws[(ht_uint3)(T1_col + 4)].read_mem();

	T2_m0 = T2_k0 + T2_k3;
	T2_m3 = T2_k0 - T2_k3;
	T2_m1 = T2_k1 + T2_k2;
	T2_m2 = T2_k1 - T2_k2;

	T3_n = (T3_m2 + T3_m3) * FIX_0_541196100;

	T4_p0 = T4_m0 + T4_m1;
	T4_p1 = T4_m0 - T4_m1;
	T4_p2 = (ht_int31)(T4_n + T4_m3 * FIX_0_765366865);
	T4_p3 = (ht_int31)(T4_n + T4_m2 * - FIX_1_847759065);

	T7_w[0] = descale((int)T7_p0, JPEG_ENC_PASS1_BITS);
	T7_w[4] = descale((int)T7_p1, JPEG_ENC_PASS1_BITS);
	T7_w[2] = descale((int)T7_p2, JPEG_ENC_CONST_BITS+JPEG_ENC_PASS1_BITS);
	T7_w[6] = descale((int)T7_p3, JPEG_ENC_CONST_BITS+JPEG_ENC_PASS1_BITS);

	T1_q0 = S_ws[(ht_uint3)(T1_col + 0)].read_mem() - S_ws[(ht_uint3)(T1_col + 7)].read_mem();
	T1_q1 = S_ws[(ht_uint3)(T1_col + 1)].read_mem() - S_ws[(ht_uint3)(T1_col + 6)].read_mem();
	T1_q2 = S_ws[(ht_uint3)(T1_col + 2)].read_mem() - S_ws[(ht_uint3)(T1_col + 5)].read_mem();
	T1_q3 = S_ws[(ht_uint3)(T1_col + 3)].read_mem() - S_ws[(ht_uint3)(T1_col + 4)].read_mem();

	T2_r0 = T2_q3 + T2_q0;
	T2_r1 = T2_q2 + T2_q1;
	T2_r2 = T2_q3 + T2_q1;
	T2_r3 = T2_q2 + T2_q0;

	T3_s = (ht_int31)((T3_r2 + T3_r3) * FIX_1_175875602);

	T4_t0 = (ht_int31)(T4_q3 * FIX_0_298631336);
	T4_t1 = (ht_int31)(T4_q2 * FIX_2_053119869);
	T4_t2 = (ht_int31)(T4_q1 * FIX_3_072711026);
	T4_t3 = (ht_int31)(T4_q0 * FIX_1_501321110);

	T5_u0 = T5_r0 * - FIX_0_899976223;
	T5_u1 = (ht_int31)(T5_r1 * - FIX_2_562915447);
	T5_u2 = (ht_int31)(T5_s + T5_r2 * - FIX_1_961570560);
	T5_u3 = T5_s + T5_r3 * - FIX_0_390180644;

	T6_v0 = T6_t0 + T6_u0 + T6_u2;
	T6_v1 = T6_t1 + T6_u1 + T6_u3;
	T6_v2 = T6_t2 + T6_u1 + T6_u2;
	T6_v3 = T6_t3 + T6_u0 + T6_u3;

	for (int i=0; i<8; i++) {
		ht_uint3 adr1 = T7_colImgIdx << 2 | T7_colDqt;
		S_quantTbl[i].read_addr(adr1, (ht_uint3)(i - T7_col));
	}

	for (int i=0; i<8; i++)
		T7_quantTbl[i] = S_quantTbl[(ht_uint3)(T7_col + i)].read_mem();

	T7_w[7] = descale((int)T7_v0, JPEG_ENC_CONST_BITS+JPEG_ENC_PASS1_BITS);
	T7_w[5] = descale((int)T7_v1, JPEG_ENC_CONST_BITS+JPEG_ENC_PASS1_BITS);
	T7_w[3] = descale((int)T7_v2, JPEG_ENC_CONST_BITS+JPEG_ENC_PASS1_BITS);
	T7_w[1] = descale((int)T7_v3, JPEG_ENC_CONST_BITS+JPEG_ENC_PASS1_BITS);

	for (int r=0; r<8; r++)
		T8_x[r] = descale((int)(T8_w[r] * T8_quantTbl[r]), JPEG_ENC_RECIP_BITS+3);

	for (int i=0; i<EPIPE_CNT; i++)
		S_cBlks[i].write_addr(T9_colBlkIdx, T9_col);

	if (T9_colVal) {
		assert(S_cBlkCnt[T9_colPipe] < 8);

		if (T9_col == 7) S_cBlkCnt[T9_colPipe] += 1;
		Eight_i16 data;
		for (int r=0; r<8; r++)
			data.m_i16[r] = T9_x[r];
		S_cBlks[T9_colPipe].write_mem(data);
	}

//#define DUMP_COEF
#if defined(DUMP_COEF) && !defined(_HTV)
	{ static int coefCnt; static int16_t coefBlk[8][8];
	if (T9_colVal) {
		for (int r=0; r<8; r++)
			coefBlk[r][T9_col] = T9_x[r];
		if (T9_col == 7) {
			printf("coef(cnt=%d) colPipe=%d @ %lld\n", coefCnt++, (int)T9_colPipe, HT_CYCLE());
			for (int r=0; r<8; r++)
				printf("    [%4d %4d %4d %4d %4d %4d %4d %4d]\n",
					coefBlk[r][0], coefBlk[r][1],
					coefBlk[r][2], coefBlk[r][3],
					coefBlk[r][4], coefBlk[r][5],
					coefBlk[r][6], coefBlk[r][7]);
			fflush(stdout);
		}
	} }
#endif

}

////////////////////////////////////////

void CPersEnc::RecvVertMsg()
{
	T1_jemRdy = RecvMsgReady_jem();
	if (T1_jemRdy) {
		JpegEncMsg msg = PeekMsg_jem();
		T1_jem = msg;

		eBlkInfo info;
		info.m_compIdx = msg.m_compIdx;
		assert(msg.m_compIdx < 3);
		info.m_rstIdx = msg.m_rstIdx;
		info.m_bEndOfMcuRow = msg.m_bEndOfMcuRow;
		info.m_imageIdx = msg.m_imageIdx;

		if (S_vBlkCnt < 8) {
if (false && i_replId.read() == 0) {
fprintf(stderr, "JpegEncMsg:");
fprintf(stderr, " m_compIdx=%d", (int)msg.m_compIdx);
fprintf(stderr, " m_blkRow=%d", (int)msg.m_blkRow);
fprintf(stderr, " m_rstIdx=%d", (int)msg.m_rstIdx);
fprintf(stderr, " m_bEndOfMcuRow=%d", (int)msg.m_bEndOfMcuRow);
fprintf(stderr, " m_imageIdx=%d", (int)msg.m_imageIdx);
fprintf(stderr, " m_data=0x%016llx", (long long)msg.m_data.m_u64);
fprintf(stderr, " @ %lld\n", HT_CYCLE());
}
			S_vBlks.write_addr(S_vBlkWrIdx, msg.m_blkRow);
			S_vBlks.write_mem(msg.m_data.m_u64);
			RecvMsg_jem();
//#define DUMP_BLOCK
#if defined(DUMP_BLOCK) && !defined(_HTV)
	{ static int blkCnt; static uint8_t block[8][8];
		for (int c=0; c<8; c++)
			block[msg.m_blkRow][c] = (uint8_t)(msg.m_data.m_u8[c]);
		if (msg.m_blkRow == 7) {
			printf("block(cnt=%d) ci=%d rstIdx=%d @ %lld\n",
				blkCnt++, (int)msg.m_compIdx, (int)msg.m_rstIdx, HT_CYCLE());
			for (int r=0; r<8; r++)
				printf("    [%4d %4d %4d %4d %4d %4d %4d %4d]\n",
					block[r][0], block[r][1],
					block[r][2], block[r][3],
					block[r][4], block[r][5],
					block[r][6], block[r][7]);
			fflush(stdout);
		}
	}
#endif

			if (msg.m_blkRow == 7) {
				S_vBlkInfo.write_addr(S_vBlkWrIdx);
				S_vBlkInfo.write_mem(info);

				S_vBlkCnt += 1;
				S_vBlkWrIdx += 1u;
			}
		}
	}
}

////////////////////////////////////////

void CPersEnc::RecvJobInfo()
{
	T1_veInfoRdy = !GR_htReset && RecvMsgReady_veInfo();
	if (T1_veInfoRdy) {
		JobInfoMsg msg = RecvMsg_veInfo();
		T1_veInfo = msg;

		EncImages_t eImgIdx = msg.m_imageIdx & ENC_IMAGES_MASK;

		if ((msg.m_imageIdx >> 1) == i_replId.read() && msg.m_sectionId == JOB_INFO_SECTION_ENC) {

			// DHT
			ASSERT_MSG(offsetof(JobEnc, m_dcDht[0].m_huffCode) ==  ENC_DCDHT0_HUFF_QOFF*8,
					"offsetof(JobEnc, m_dcDht[0].m_huffCode) == %d\n",
					(int)offsetof(JobEnc, m_dcDht[0].m_huffCode));

			if (msg.m_rspQw >= ENC_DCDHT0_HUFF_QOFF && msg.m_rspQw < ENC_DCDHT0_HUFF_QOFF+ENC_DHT_HUFF_QCNT) {
				ht_uint6 idx = (ht_uint6)(msg.m_rspQw - (ENC_DCDHT0_HUFF_QOFF & 0x3f));
				for (int i=0; i<EPIPE_CNT; i++) {
					ht_uint3 adr1 = eImgIdx << 2 | 2;
					S_huffCode[i].write_addr(adr1, idx);
					S_huffCode[i].write_mem(msg.m_data);
				}
			}

			ASSERT_MSG(offsetof(JobEnc, m_dcDht[0].m_huffSize) == ENC_DCDHT0_SIZE_QOFF*8,
					"offsetof(JobEnc, m_dcDht[0].m_huffSize) == %d\n",
					(int)offsetof(JobEnc, m_dcDht[0].m_huffSize));

			if (msg.m_rspQw >= ENC_DCDHT0_SIZE_QOFF && msg.m_rspQw < ENC_DCDHT0_SIZE_QOFF+ENC_DHT_SIZE_QCNT) {
				ht_uint5 idx = (ht_uint5)(msg.m_rspQw - (ENC_DCDHT0_SIZE_QOFF & 0x1f));
				for (int i=0; i<EPIPE_CNT; i++) {
					ht_uint3 adr1 = eImgIdx << 2 | 2;
					S_huffSize[i].write_addr(adr1, idx);
					S_huffSize[i].write_mem(msg.m_data);
				}
			}

			if (msg.m_rspQw >= ENC_DCDHT1_HUFF_QOFF && msg.m_rspQw < ENC_DCDHT1_HUFF_QOFF+ENC_DHT_HUFF_QCNT) {
				ht_uint6 idx = (ht_uint6)(msg.m_rspQw - (ENC_DCDHT1_HUFF_QOFF & 0x3f));
				for (int i=0; i<EPIPE_CNT; i++) {
					ht_uint3 adr1 = eImgIdx << 2 | 3;
					S_huffCode[i].write_addr(adr1, idx);
					S_huffCode[i].write_mem(msg.m_data);
				}
			}
			if (msg.m_rspQw >= ENC_DCDHT1_SIZE_QOFF && msg.m_rspQw < ENC_DCDHT1_SIZE_QOFF+ENC_DHT_SIZE_QCNT) {
				ht_uint5 idx = (ht_uint5)(msg.m_rspQw - (ENC_DCDHT1_SIZE_QOFF & 0x1f));
				for (int i=0; i<EPIPE_CNT; i++) {
					ht_uint3 adr1 = eImgIdx << 2 | 3;
					S_huffSize[i].write_addr(adr1, idx);
					S_huffSize[i].write_mem(msg.m_data);
				}
			}

			if (msg.m_rspQw >= ENC_ACDHT0_HUFF_QOFF && msg.m_rspQw < ENC_ACDHT0_HUFF_QOFF+ENC_DHT_HUFF_QCNT) {
				ht_uint6 idx = (ht_uint6)(msg.m_rspQw - (ENC_ACDHT0_HUFF_QOFF & 0x3f));
				for (int i=0; i<EPIPE_CNT; i++) {
					ht_uint3 adr1 = eImgIdx << 2 | 0;
					S_huffCode[i].write_addr(adr1, idx);
					S_huffCode[i].write_mem(msg.m_data);
				}
			}
			if (msg.m_rspQw >= ENC_ACDHT0_SIZE_QOFF && msg.m_rspQw < ENC_ACDHT0_SIZE_QOFF+ENC_DHT_SIZE_QCNT) {
				ht_uint5 idx = (ht_uint5)(msg.m_rspQw - (ENC_ACDHT0_SIZE_QOFF & 0x1f));
				for (int i=0; i<EPIPE_CNT; i++) {
					ht_uint3 adr1 = eImgIdx << 2 | 0;
					S_huffSize[i].write_addr(adr1, idx);
					S_huffSize[i].write_mem(msg.m_data);
				}
			}

			if (msg.m_rspQw >= ENC_ACDHT1_HUFF_QOFF && msg.m_rspQw < ENC_ACDHT1_HUFF_QOFF+ENC_DHT_HUFF_QCNT) {
				ht_uint6 idx = (ht_uint6)(msg.m_rspQw - (ENC_ACDHT1_HUFF_QOFF & 0x3f));
				for (int i=0; i<EPIPE_CNT; i++) {
					ht_uint3 adr1 = eImgIdx << 2 | 1;
					S_huffCode[i].write_addr(adr1, idx);
					S_huffCode[i].write_mem(msg.m_data);
				}
			}
			if (msg.m_rspQw >= ENC_ACDHT1_SIZE_QOFF && msg.m_rspQw < ENC_ACDHT1_SIZE_QOFF+ENC_DHT_SIZE_QCNT) {
				ht_uint5 idx = (ht_uint5)(msg.m_rspQw - (ENC_ACDHT1_SIZE_QOFF & 0x1f));
				for (int i=0; i<EPIPE_CNT; i++) {
					ht_uint3 adr1 = eImgIdx << 2 | 1;
					S_huffSize[i].write_addr(adr1, idx);
					S_huffSize[i].write_mem(msg.m_data);
				}
			}

			ASSERT_MSG(offsetof(struct JobEnc, m_dqt[0]) == ENC_DQT0_QOFF*8,
					"offsetof(struct JobEnc, m_dqt[0]) == %d\n",
					(int)offsetof(struct JobEnc, m_dqt[0]));

			if (msg.m_rspQw >= ENC_DQT0_QOFF && msg.m_rspQw < ENC_DQT0_QOFF+ENC_DQT_QCNT) {
				ht_uint4 qw = (ht_uint4)(msg.m_rspQw - (ENC_DQT0_QOFF & 0xf));
				ht_uint3 r = qw >> 1;

				for (int i = 0; i < 4; i++) {
					ht_uint3 c = (qw & 1) << 2 | i;
					int16_t dat = (int16_t)(msg.m_data >> (16 * i));
					ht_uint3 adr1 = eImgIdx << 2 | 0;
					S_quantTbl[(ht_uint3)(r + c)].write_addr(adr1, r);
					S_quantTbl[(ht_uint3)(r + c)].write_mem(dat);
				}
			}
			if (msg.m_rspQw >= ENC_DQT1_QOFF && msg.m_rspQw < ENC_DQT1_QOFF+ENC_DQT_QCNT) {
				ht_uint4 qw = (ht_uint4)(msg.m_rspQw - (ENC_DQT1_QOFF & 0xf));
				ht_uint3 r = qw >> 1;

				for (int i = 0; i < 4; i++) {
					ht_uint3 c = (qw & 1) << 2 | i;
					int16_t dat = (int16_t)(msg.m_data >> (16 * i));
					ht_uint3 adr1 = eImgIdx << 2 | 1;
					S_quantTbl[(ht_uint3)(r + c)].write_addr(adr1, r);
					S_quantTbl[(ht_uint3)(r + c)].write_mem(dat);
				}
			}
			if (msg.m_rspQw >= ENC_DQT2_QOFF && msg.m_rspQw < ENC_DQT2_QOFF+ENC_DQT_QCNT) {
				ht_uint4 qw = (ht_uint4)(msg.m_rspQw - (ENC_DQT2_QOFF & 0xf));
				ht_uint3 r = qw >> 1;

				for (int i = 0; i < 4; i++) {
					ht_uint3 c = (qw & 1) << 2 | i;
					int16_t dat = (int16_t)(msg.m_data >> (16 * i));
					ht_uint3 adr1 = eImgIdx << 2 | 2;
					S_quantTbl[(ht_uint3)(r + c)].write_addr(adr1, r);
					S_quantTbl[(ht_uint3)(r + c)].write_mem(dat);
				}
			}
			if (msg.m_rspQw >= ENC_DQT3_QOFF && msg.m_rspQw < ENC_DQT3_QOFF+ENC_DQT_QCNT) {
				ht_uint4 qw = (ht_uint4)(msg.m_rspQw - (ENC_DQT3_QOFF & 0xf));
				ht_uint3 r = qw >> 1;

				for (int i = 0; i < 4; i++) {
					ht_uint3 c = (qw & 1) << 2 | i;
					int16_t dat = (int16_t)(msg.m_data >> (16 * i));
					ht_uint3 adr1 = eImgIdx << 2 | 3;
					S_quantTbl[(ht_uint3)(r + c)].write_addr(adr1, r);
					S_quantTbl[(ht_uint3)(r + c)].write_mem(dat);
				}
			}

			// ECP
			ASSERT_MSG(offsetof(JobEnc, m_ecp[0]) == ENC_ECP0_QOFF*8,
					"offsetof(JobEnc, m_ecp[0]) == %d\n",
					(int)offsetof(JobEnc, m_ecp[0]));

			if (msg.m_rspQw == ENC_ECP0_QOFF) {
				S_jobInfo[eImgIdx].m_ecp[0].m_dcDhtId = (msg.m_data >> 58) & 1;
				S_jobInfo[eImgIdx].m_ecp[0].m_acDhtId = (msg.m_data >> 59) & 1;
				S_jobInfo[eImgIdx].m_ecp[0].m_dqtId = (msg.m_data >> 60) & 3;
			}
			if (msg.m_rspQw == ENC_ECP1_QOFF) {
				S_jobInfo[eImgIdx].m_ecp[1].m_dcDhtId = (msg.m_data >> 58) & 1;
				S_jobInfo[eImgIdx].m_ecp[1].m_acDhtId = (msg.m_data >> 59) & 1;
				S_jobInfo[eImgIdx].m_ecp[1].m_dqtId = (msg.m_data >> 60) & 3;
			}
			if (msg.m_rspQw == ENC_ECP2_QOFF) {
				S_jobInfo[eImgIdx].m_ecp[2].m_dcDhtId = (msg.m_data >> 58) & 1;
				S_jobInfo[eImgIdx].m_ecp[2].m_acDhtId = (msg.m_data >> 59) & 1;
				S_jobInfo[eImgIdx].m_ecp[2].m_dqtId = (msg.m_data >> 60) & 3;
			}

			// RST INFO
			if (msg.m_rspQw == ENC_RSTCNT_QOFF) {
				S_jobInfo[eImgIdx].m_rstCnt = msg.m_data & 0x7ff;
			}
			if (msg.m_rspQw == ENC_RSTBASE_QOFF) {
				S_jobInfo[eImgIdx].m_pRstBase = (ht_uint48)msg.m_data;
			}
			if (msg.m_rspQw == ENC_RSTOFF_QOFF) {
				S_jobInfo[eImgIdx].m_rstOffset = (uint32_t)msg.m_data;
			}
		}
	}
}

#endif
