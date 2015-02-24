#include "Ht.h"
#include "PersBcm.h"


#define BcmByteSwap16(value) \
	((((value) & 0xff) << 8) | (((value) & 0xff00) >> 8))

#define BcmByteSwap32(value) \
	((BcmByteSwap16(value) << 16) | BcmByteSwap16(value >> 16))

#define  SHR(x, n) ((x & 0xFFFFFFFF) >> n)
#define ROTR(x, n) (SHR(x, n) | (uint32_t)(x << (32 - n)))

#define S0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ SHR(x, 3))
#define S1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ SHR(x, 10))

#define S2(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define S3(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))

#define F0(x, y, z) ((x & y) | (z & (x | y)))
#define F1(x, y, z) (z ^ (x & (y ^ z)))

#define P(a, b, c, d, e, f, g, h, x, K) {                \
		temp1 = h + S3(e) + F1(e, f, g) + K + x; \
		temp2 = S2(a) + F0(a, b, c);             \
		d += temp1; h = temp1 + temp2;           \
}

#define Q(s, i, d, c) {                                                                                           \
		P(s.m_state[(i + 0) % 8], s.m_state[(i + 1) % 8], s.m_state[(i + 2) % 8], s.m_state[(i + 3) % 8], \
		  s.m_state[(i + 4) % 8], s.m_state[(i + 5) % 8], s.m_state[(i + 6) % 8], s.m_state[(i + 7) % 8], d.m_dw[0], c) \
		tmp = d.m_dw[0];                                                                                  \
		d.m_dw[0] = d.m_dw[1]; d.m_dw[1] = d.m_dw[2]; d.m_dw[2] = d.m_dw[3]; d.m_dw[3] = d.m_dw[4];       \
		d.m_dw[4] = d.m_dw[5]; d.m_dw[5] = d.m_dw[6]; d.m_dw[6] = d.m_dw[7]; d.m_dw[7] = d.m_dw[8];       \
		d.m_dw[8] = d.m_dw[9]; d.m_dw[9] = d.m_dw[10]; d.m_dw[10] = d.m_dw[11]; d.m_dw[11] = d.m_dw[12];  \
		d.m_dw[12] = d.m_dw[13]; d.m_dw[13] = d.m_dw[14]; d.m_dw[14] = d.m_dw[15]; d.m_dw[15] = tmp;      \
}

#define R(s, i, d, c) {                                                                                           \
		tmp = S1(d.m_dw[14]) + d.m_dw[9] + S0(d.m_dw[1]) + d.m_dw[0];                                     \
		P(s.m_state[(i + 0) % 8], s.m_state[(i + 1) % 8], s.m_state[(i + 2) % 8], s.m_state[(i + 3) % 8], \
		  s.m_state[(i + 4) % 8], s.m_state[(i + 5) % 8], s.m_state[(i + 6) % 8], s.m_state[(i + 7) % 8], tmp, c) \
		d.m_dw[0] = d.m_dw[1]; d.m_dw[1] = d.m_dw[2]; d.m_dw[2] = d.m_dw[3]; d.m_dw[3] = d.m_dw[4];       \
		d.m_dw[4] = d.m_dw[5]; d.m_dw[5] = d.m_dw[6]; d.m_dw[6] = d.m_dw[7]; d.m_dw[7] = d.m_dw[8];       \
		d.m_dw[8] = d.m_dw[9]; d.m_dw[9] = d.m_dw[10]; d.m_dw[10] = d.m_dw[11]; d.m_dw[11] = d.m_dw[12];  \
		d.m_dw[12] = d.m_dw[13]; d.m_dw[13] = d.m_dw[14]; d.m_dw[14] = d.m_dw[15]; d.m_dw[15] = tmp;      \
}

void CPersBcm::PersBcm()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		// Initial Entry Point from Host Code -> BCM_ENTRY
		//
		// Read in Task information
		// GR_task -> midState[8]/data[3]/initNonce/lastNonce/target[3]
		case BCM_ENTRY: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			ReadMem_task(PR_pBcmTask, 0, 8);

			ReadMemPause(BCM_TASK_VALID);
		}
		break;


		// Force initial task to be valid
		// Setup initial Nonce
		case BCM_TASK_VALID: {
			S_bTaskValid = true;

			S_nonce = S_task.m_initNonce;

			S_hashCount = 0;

			HtContinue(BCM_CHK_RTN);
		}
		break;


		// Main hash loop, sit and check the result queue
		case BCM_CHK_RTN: {
			if (SendReturnBusy_bcm() || SendHostMsgBusy()) {
				HtRetry();
				break;
			}

			if (S_hashCount > 1000000) {
				SendHostMsg(BCM_HASHES_COMPLETED, (ht_uint56)S_hashCount);
				S_hashCount = 0;
			}

			if (S_nonceHitQue.empty()) {
				// No results waiting
				if (S_bTaskValid) {
					HtContinue(BCM_CHK_RTN);
				} else {
					if (S_hashCount > 0) {
						SendHostMsg(BCM_HASHES_COMPLETED, (ht_uint56)S_hashCount);
						S_hashCount = 0;
					}
					SendReturn_bcm(P_pBcmTask);
				}
			} else {
				// Results are waiting to be processed
				if (S_bcmRsltQue.empty())
					HtContinue(BCM_CHK_RTN);
				else
					HtContinue(BCM_WR_RSLT1);
			}

			// Reset index for results to zero (for writing to memory)
			S_rsltQwIdx = 0;
		}
		break;

		// BCM_WR_RSLT1->3, steps to write a result to memory
		case BCM_WR_RSLT1: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			ht_uint48 memAddr = S_bcmRsltQue.front() | (S_rsltQwIdx << 3);

			WriteMem(memAddr, S_task.m_qw[S_rsltQwIdx]);

			S_rsltQwIdx += 1;

			if (S_rsltQwIdx < 5)
				HtContinue(BCM_WR_RSLT1);
			else
				HtContinue(BCM_WR_RSLT2);
		}
		break;

		case BCM_WR_RSLT2: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			ht_uint48 memAddr = S_bcmRsltQue.front() | (5 << 3);
			ht_uint64 memData;

			memData(31, 0) = S_task.m_qw[5] & 0xffffffff;
			memData(63, 32) = BcmByteSwap32(S_nonceHitQue.front());

			S_nonceHitQue.pop();

			WriteMem(memAddr, memData);

			WriteMemPause(BCM_WR_RSLT3);
		}
		break;

		case BCM_WR_RSLT3: {
			if (SendHostMsgBusy()) {
				HtRetry();
				break;
			}

			SendHostMsg(BCM_RSLT_BUF_RDY, (ht_uint56)S_bcmRsltQue.front());

			S_bcmRsltQue.pop();

			HtContinue(BCM_CHK_RTN);
		}
		break;

		default:
			assert(0);
		}
	}

	// temps used within macros
	uint32_t temp1, temp2, tmp;

	// Input Valid Signal reflects Task Valid Signal
	T1_vld = S_bTaskValid;

	// *** State Setup ***

	// Setup T1_data
	T1_data.m_dw[0] = S_task.m_data[0];
	T1_data.m_dw[1] = S_task.m_data[1];
	T1_data.m_dw[2] = S_task.m_data[2];
	T1_data.m_dw[3] = S_nonce;
	T1_data.m_dw[4] = 0x80000000;
	T1_data.m_dw[5] = 0x00000000;
	T1_data.m_dw[6] = 0x00000000;
	T1_data.m_dw[7] = 0x00000000;
	T1_data.m_dw[8] = 0x00000000;
	T1_data.m_dw[9] = 0x00000000;
	T1_data.m_dw[10] = 0x00000000;
	T1_data.m_dw[11] = 0x00000000;
	T1_data.m_dw[12] = 0x00000000;
	T1_data.m_dw[13] = 0x00000000;
	T1_data.m_dw[14] = 0x00000000;
	T1_data.m_dw[15] = 0x00000280;

	// Setup T1_state
	T1_state.m_state[0] = S_task.m_midState.m_state[0];
	T1_state.m_state[1] = S_task.m_midState.m_state[1];
	T1_state.m_state[2] = S_task.m_midState.m_state[2];
	T1_state.m_state[3] = S_task.m_midState.m_state[3];
	T1_state.m_state[4] = S_task.m_midState.m_state[4];
	T1_state.m_state[5] = S_task.m_midState.m_state[5];
	T1_state.m_state[6] = S_task.m_midState.m_state[6];
	T1_state.m_state[7] = S_task.m_midState.m_state[7];

	// Nonce
	T1_nonce = S_nonce;

	// *** Do Work ***

	// 64-deep Hash Cycle 1
	Q(T2_state, 0, T2_data, 0x428A2F98);
	Q(T3_state, 7, T3_data, 0x71374491);
	Q(T4_state, 6, T4_data, 0xB5C0FBCF);
	Q(T5_state, 5, T5_data, 0xE9B5DBA5);
	Q(T6_state, 4, T6_data, 0x3956C25B);
	Q(T7_state, 3, T7_data, 0x59F111F1);
	Q(T8_state, 2, T8_data, 0x923F82A4);
	Q(T9_state, 1, T9_data, 0xAB1C5ED5);
	Q(T10_state, 0, T10_data, 0xD807AA98);
	Q(T11_state, 7, T11_data, 0x12835B01);
	Q(T12_state, 6, T12_data, 0x243185BE);
	Q(T13_state, 5, T13_data, 0x550C7DC3);
	Q(T14_state, 4, T14_data, 0x72BE5D74);
	Q(T15_state, 3, T15_data, 0x80DEB1FE);
	Q(T16_state, 2, T16_data, 0x9BDC06A7);
	Q(T17_state, 1, T17_data, 0xC19BF174);
	R(T18_state, 0, T18_data, 0xE49B69C1);
	R(T19_state, 7, T19_data, 0xEFBE4786);
	R(T20_state, 6, T20_data, 0x0FC19DC6);
	R(T21_state, 5, T21_data, 0x240CA1CC);
	R(T22_state, 4, T22_data, 0x2DE92C6F);
	R(T23_state, 3, T23_data, 0x4A7484AA);
	R(T24_state, 2, T24_data, 0x5CB0A9DC);
	R(T25_state, 1, T25_data, 0x76F988DA);
	R(T26_state, 0, T26_data, 0x983E5152);
	R(T27_state, 7, T27_data, 0xA831C66D);
	R(T28_state, 6, T28_data, 0xB00327C8);
	R(T29_state, 5, T29_data, 0xBF597FC7);
	R(T30_state, 4, T30_data, 0xC6E00BF3);
	R(T31_state, 3, T31_data, 0xD5A79147);
	R(T32_state, 2, T32_data, 0x06CA6351);
	R(T33_state, 1, T33_data, 0x14292967);
	R(T34_state, 0, T34_data, 0x27B70A85);
	R(T35_state, 7, T35_data, 0x2E1B2138);
	R(T36_state, 6, T36_data, 0x4D2C6DFC);
	R(T37_state, 5, T37_data, 0x53380D13);
	R(T38_state, 4, T38_data, 0x650A7354);
	R(T39_state, 3, T39_data, 0x766A0ABB);
	R(T40_state, 2, T40_data, 0x81C2C92E);
	R(T41_state, 1, T41_data, 0x92722C85);
	R(T42_state, 0, T42_data, 0xA2BFE8A1);
	R(T43_state, 7, T43_data, 0xA81A664B);
	R(T44_state, 6, T44_data, 0xC24B8B70);
	R(T45_state, 5, T45_data, 0xC76C51A3);
	R(T46_state, 4, T46_data, 0xD192E819);
	R(T47_state, 3, T47_data, 0xD6990624);
	R(T48_state, 2, T48_data, 0xF40E3585);
	R(T49_state, 1, T49_data, 0x106AA070);
	R(T50_state, 0, T50_data, 0x19A4C116);
	R(T51_state, 7, T51_data, 0x1E376C08);
	R(T52_state, 6, T52_data, 0x2748774C);
	R(T53_state, 5, T53_data, 0x34B0BCB5);
	R(T54_state, 4, T54_data, 0x391C0CB3);
	R(T55_state, 3, T55_data, 0x4ED8AA4A);
	R(T56_state, 2, T56_data, 0x5B9CCA4F);
	R(T57_state, 1, T57_data, 0x682E6FF3);
	R(T58_state, 0, T58_data, 0x748F82EE);
	R(T59_state, 7, T59_data, 0x78A5636F);
	R(T60_state, 6, T60_data, 0x84C87814);
	R(T61_state, 5, T61_data, 0x8CC70208);
	R(T62_state, 4, T62_data, 0x90BEFFFA);
	R(T63_state, 3, T63_data, 0xA4506CEB);
	R(T64_state, 2, T64_data, 0xBEF9A3F7);
	R(T65_state, 1, T65_data, 0xC67178F2);

	// Add following hash cycle 1
	T66_state.m_state[0] = T66_state.m_state[0] + S_task.m_midState.m_state[0];
	T66_state.m_state[1] = T66_state.m_state[1] + S_task.m_midState.m_state[1];
	T66_state.m_state[2] = T66_state.m_state[2] + S_task.m_midState.m_state[2];
	T66_state.m_state[3] = T66_state.m_state[3] + S_task.m_midState.m_state[3];
	T66_state.m_state[4] = T66_state.m_state[4] + S_task.m_midState.m_state[4];
	T66_state.m_state[5] = T66_state.m_state[5] + S_task.m_midState.m_state[5];
	T66_state.m_state[6] = T66_state.m_state[6] + S_task.m_midState.m_state[6];
	T66_state.m_state[7] = T66_state.m_state[7] + S_task.m_midState.m_state[7];

	// State Setup
	// Setup T67_data
	T67_data.m_dw[0] = T67_state.m_state[0];
	T67_data.m_dw[1] = T67_state.m_state[1];
	T67_data.m_dw[2] = T67_state.m_state[2];
	T67_data.m_dw[3] = T67_state.m_state[3];
	T67_data.m_dw[4] = T67_state.m_state[4];
	T67_data.m_dw[5] = T67_state.m_state[5];
	T67_data.m_dw[6] = T67_state.m_state[6];
	T67_data.m_dw[7] = T67_state.m_state[7];
	T67_data.m_dw[8] = 0x00000000;
	T67_data.m_dw[9] = 0x00000000;
	T67_data.m_dw[10] = 0x00000000;
	T67_data.m_dw[11] = 0x00000000;
	T67_data.m_dw[12] = 0x00000000;
	T67_data.m_dw[13] = 0x00000000;
	T67_data.m_dw[14] = 0x00000000;
	T67_data.m_dw[15] = 0x00000000;

	T67_data.m_uc[35] = 0x80;
	T67_data.m_uc[61] = 0x01;

	// Setup T67_state
	T67_state.m_state[0] = 0x6A09E667;
	T67_state.m_state[1] = 0xBB67AE85;
	T67_state.m_state[2] = 0x3C6EF372;
	T67_state.m_state[3] = 0xA54FF53A;
	T67_state.m_state[4] = 0x510E527F;
	T67_state.m_state[5] = 0x9B05688C;
	T67_state.m_state[6] = 0x1F83D9AB;
	T67_state.m_state[7] = 0x5BE0CD19;

	// 64-deep Hash Cycle 2
	Q(T68_state, 0, T68_data, 0x428A2F98);
	Q(T69_state, 7, T69_data, 0x71374491);
	Q(T70_state, 6, T70_data, 0xB5C0FBCF);
	Q(T71_state, 5, T71_data, 0xE9B5DBA5);
	Q(T72_state, 4, T72_data, 0x3956C25B);
	Q(T73_state, 3, T73_data, 0x59F111F1);
	Q(T74_state, 2, T74_data, 0x923F82A4);
	Q(T75_state, 1, T75_data, 0xAB1C5ED5);
	Q(T76_state, 0, T76_data, 0xD807AA98);
	Q(T77_state, 7, T77_data, 0x12835B01);
	Q(T78_state, 6, T78_data, 0x243185BE);
	Q(T79_state, 5, T79_data, 0x550C7DC3);
	Q(T80_state, 4, T80_data, 0x72BE5D74);
	Q(T81_state, 3, T81_data, 0x80DEB1FE);
	Q(T82_state, 2, T82_data, 0x9BDC06A7);
	Q(T83_state, 1, T83_data, 0xC19BF174);
	R(T84_state, 0, T84_data, 0xE49B69C1);
	R(T85_state, 7, T85_data, 0xEFBE4786);
	R(T86_state, 6, T86_data, 0x0FC19DC6);
	R(T87_state, 5, T87_data, 0x240CA1CC);
	R(T88_state, 4, T88_data, 0x2DE92C6F);
	R(T89_state, 3, T89_data, 0x4A7484AA);
	R(T90_state, 2, T90_data, 0x5CB0A9DC);
	R(T91_state, 1, T91_data, 0x76F988DA);
	R(T92_state, 0, T92_data, 0x983E5152);
	R(T93_state, 7, T93_data, 0xA831C66D);
	R(T94_state, 6, T94_data, 0xB00327C8);
	R(T95_state, 5, T95_data, 0xBF597FC7);
	R(T96_state, 4, T96_data, 0xC6E00BF3);
	R(T97_state, 3, T97_data, 0xD5A79147);
	R(T98_state, 2, T98_data, 0x06CA6351);
	R(T99_state, 1, T99_data, 0x14292967);
	R(T100_state, 0, T100_data, 0x27B70A85);
	R(T101_state, 7, T101_data, 0x2E1B2138);
	R(T102_state, 6, T102_data, 0x4D2C6DFC);
	R(T103_state, 5, T103_data, 0x53380D13);
	R(T104_state, 4, T104_data, 0x650A7354);
	R(T105_state, 3, T105_data, 0x766A0ABB);
	R(T106_state, 2, T106_data, 0x81C2C92E);
	R(T107_state, 1, T107_data, 0x92722C85);
	R(T108_state, 0, T108_data, 0xA2BFE8A1);
	R(T109_state, 7, T109_data, 0xA81A664B);
	R(T110_state, 6, T110_data, 0xC24B8B70);
	R(T111_state, 5, T111_data, 0xC76C51A3);
	R(T112_state, 4, T112_data, 0xD192E819);
	R(T113_state, 3, T113_data, 0xD6990624);
	R(T114_state, 2, T114_data, 0xF40E3585);
	R(T115_state, 1, T115_data, 0x106AA070);
	R(T116_state, 0, T116_data, 0x19A4C116);
	R(T117_state, 7, T117_data, 0x1E376C08);
	R(T118_state, 6, T118_data, 0x2748774C);
	R(T119_state, 5, T119_data, 0x34B0BCB5);
	R(T120_state, 4, T120_data, 0x391C0CB3);
	R(T121_state, 3, T121_data, 0x4ED8AA4A);
	R(T122_state, 2, T122_data, 0x5B9CCA4F);
	R(T123_state, 1, T123_data, 0x682E6FF3);
	R(T124_state, 0, T124_data, 0x748F82EE);
	R(T125_state, 7, T125_data, 0x78A5636F);
	R(T126_state, 6, T126_data, 0x84C87814);
	R(T127_state, 5, T127_data, 0x8CC70208);
	R(T128_state, 4, T128_data, 0x90BEFFFA);
	R(T129_state, 3, T129_data, 0xA4506CEB);
	R(T130_state, 2, T130_data, 0xBEF9A3F7);
	R(T131_state, 1, T131_data, 0xC67178F2);

	// Add following hash cycle 2
	T132_state.m_state[0] = BcmByteSwap32(T132_state.m_state[0] + 0x6A09E667);
	T132_state.m_state[1] = BcmByteSwap32(T132_state.m_state[1] + 0xBB67AE85);
	T132_state.m_state[2] = BcmByteSwap32(T132_state.m_state[2] + 0x3C6EF372);
	T132_state.m_state[3] = BcmByteSwap32(T132_state.m_state[3] + 0xA54FF53A);
	T132_state.m_state[4] = BcmByteSwap32(T132_state.m_state[4] + 0x510E527F);
	T132_state.m_state[5] = BcmByteSwap32(T132_state.m_state[5] + 0x9B05688C);
	T132_state.m_state[6] = BcmByteSwap32(T132_state.m_state[6] + 0x1F83D9AB);
	T132_state.m_state[7] = BcmByteSwap32(T132_state.m_state[7] + 0x5BE0CD19);


	// *** End Checks ***

	// Increment Nonce
	if (T1_vld)
		S_nonce++;

	// Send MSG back if we need to!

	// Check for Result Found
	if (T133_vld && !S_nonceHitQue.full()) {
		bool bW7Eq = T133_state.m_state[7] == S_task.m_target[2];
		bool bW7Lt = T133_state.m_state[7] < S_task.m_target[2];

		bool bW6Eq = T133_state.m_state[6] == S_task.m_target[1];
		bool bW6Lt = T133_state.m_state[6] < S_task.m_target[1];

		bool bW5Eq = T133_state.m_state[5] == S_task.m_target[0];
		bool bW5Lt = T133_state.m_state[5] < S_task.m_target[0];

		bool bMatch = bW7Lt || bW7Eq && (bW6Lt || bW6Eq && (bW5Lt || bW5Eq));

		if (bMatch)
			S_nonceHitQue.push(BcmByteSwap32(T133_nonce));

		// Check for Last Nonce / Increment count
		if (S_task.m_lastNonce == T133_nonce)
			S_bTaskValid = false;
		else
			S_hashCount++;
	}

	// Check force return / reset
	if (S_forceReturn) {
		S_bTaskValid = false;
		S_forceReturn = false;
	}

	if (GR_htReset) {
		S_zeroInput = true;
		S_bTaskValid = false;
		S_forceReturn = false;
	}
}
