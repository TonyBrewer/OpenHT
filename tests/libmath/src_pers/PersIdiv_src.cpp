#include "Ht.h"
#include "PersIdiv.h"

// 64-bit metrics:
//	* 72 clk caller latency
//	* 1k LUT's, 600 FF's

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

void
CPersIdiv::PersIdiv()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case DIV_ENTRY: {
			assert_msg(IDIV_N_W < 128, "Idiv counter overflow\n");
			assert_msg(P_d != 0, "Idiv divide by zero\n");

			S_n = P_n;
			S_d = P_d;
			S_p = 0;

			// invert negative numbers for signed division
			if (P_sign) {
				idiv_sn_t sn = (idiv_sn_t)P_n;
				idiv_sd_t sd = (idiv_sd_t)P_d;
				if (sn < 0) S_n = (idiv_n_t)-sn;
				if (sd < 0) S_d = (idiv_d_t)-sd;
			}

#if IDIV_D_W > IDIV_N_W
			if (S_d >= S_n) {
				S_p = S_n;
				S_n = 0;
				HtContinue(DIV_RTN);
				break;
			} else
#endif
				S_c = IDIV_N_W;

			HtPause(DIV_RTN);
		}
		break;
		case DIV_RTN: {
			BUSY_RETRY(SendReturnBusy_idiv());

			// correct remainder
			if (S_p[IDIV_N_W])
				S_p = (idiv_p_t)(S_p + S_d);
			idiv_n_t rem = (idiv_n_t)S_p;

			idiv_n_t quo = S_n;
			// fixup sign of result
			if (P_sign) {
				idiv_n_t squo = (idiv_n_t)quo;
				bool bNeg = P_n[IDIV_N_W-1] != P_d[IDIV_D_W-1];
				if (bNeg) quo = (idiv_n_t)-squo;

				bool oFlow = (ht_uint1)bNeg != quo[IDIV_N_W-1];
				assert_msg(!oFlow, "Idiv overflow\n");
			}

			SendReturn_idiv(quo, rem);
		}
		break;
		default:
			assert(0);
		}
	}

	if (SR_c > 0) {
		S_p = (idiv_p_t)(SR_p << 1) | SR_n[IDIV_N_W-1];
		if (SR_p[IDIV_N_W])
			S_p = (idiv_p_t)(S_p + S_d);
		else
			S_p = (idiv_p_t)(S_p - S_d);

		S_n = (idiv_n_t)(SR_n << 1) | (~S_p[IDIV_N_W] & 1);

		if (SR_c == 1) HtResume(0);

		S_c -= 1;
	}
}
