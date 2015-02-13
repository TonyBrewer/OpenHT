#include "Ht.h"
#include "PersIdivr.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

#define CMP_SHIFT (IDIVR_NUM_W - IDIVR_RADIX_W)

void
CPersIdivr::PersIdivr()
{
	S_cmp[0] = 0;

	if (PR_htValid) {
		switch (PR_htInst) {
		case DIV_ENTRY: {
			assert_msg(IDIVR_NUM_W < 128, "IdivR counter overflow\n");
			assert_msg(P_d != 0, "IdivR divide by zero\n");

			S_n = P_n;
			S_d = P_d;
			S_c = IDIVR_NUM_W;
			S_q = 0;

			// invert negative numbers for signed division
			if (P_sign) {
				idivr_sn_t sn = (idivr_sn_t)P_n;
				idivr_sd_t sd = (idivr_sd_t)P_d;
				if (sn < 0) S_n = (idivr_n_t)-sn;
				if (sd < 0) S_d = (idivr_d_t)-sd;
			}

			// populate comparision array
			for (int i = 1; i < IDIVR_RADIX; i++) {
				S_cmp[i] = (idivr_mul_t)(S_d * i);
				S_cmp[i] = (idivr_cmp_t)(S_cmp[i] << CMP_SHIFT);
			}

			HtPause(DIV_RTN);
		}
		break;
		case DIV_RTN: {
			BUSY_RETRY(SendReturnBusy_idivr());

			idivr_n_t quo = S_q;
			// fixup sign of result
			if (P_sign) {
				idivr_n_t squo = (idivr_n_t)quo;
				bool bNeg = P_n[IDIVR_N_W-1] != P_d[IDIVR_D_W-1];
				if (bNeg) quo = (idivr_n_t)-squo;

				bool oFlow = (ht_uint1)bNeg != quo[IDIVR_N_W-1];
				assert_msg(!oFlow, "IdivR overflow\n");
			}

			SendReturn_idivr(quo, (idivr_n_t)S_n);
		}
		break;
		default:
			assert(0);
		}
	}

	if (SR_c > 0) {
		// find proper divisor
		bool cmp_mask[IDIVR_RADIX];
		for (int i=0; i < IDIVR_RADIX-1; i++)
			cmp_mask[i] = SR_n < SR_cmp[i+1];
		cmp_mask[IDIVR_RADIX-1] = true;

		uint8_t q = 0;
		idivr_n_t s = 0;
		for (int i=0; i < IDIVR_RADIX; i++) {
			sc_uint<IDIVR_RADIX_W> idx = IDIVR_RADIX-1-i;
			if (cmp_mask[idx]) {
				q = idx;
				s = (idivr_n_t)SR_cmp[idx];
			}
		}

		// update numerator and quotient
		S_n = SR_n - s;
		S_q = (idivr_n_t)(SR_q << IDIVR_RADIX_W) | q;

		for (int i = 1; i < IDIVR_RADIX; i++)
			S_cmp[i] = SR_cmp[i] >> IDIVR_RADIX_W;

		S_c = SR_c - IDIVR_RADIX_W;

		// done
		if (SR_c <= IDIVR_RADIX_W) HtResume(0);
	}
}
