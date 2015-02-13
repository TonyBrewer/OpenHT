#include "Ht.h"
#include "PersIobk.h"

#ifndef _HTV
extern FILE *tfp;
#endif

void
CPersIobk::PersIobk()
{
	bool flushDec = S_flushCnt != 0;

	if (PR_htValid) {
		switch (PR_htInst) {
		case IOBK_IDLE:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			if (P_cnt) {
				HtContinue(IOBK_POP);
			} else {
#ifndef _HTV
				if (tfp) fprintf(tfp, "AU%d SendReturn(0)\n",
						 (int)P_au);
#endif
				SendReturn_htmain(0);
			}
		}
		break;
		case IOBK_POP:
		{
			// should be able to peek without busy check
			uint64_t peek = PeekHostData();

			if (RecvHostDataBusy()) {
				HtRetry();
				break;
			}

			P_recv = RecvHostData();
			assert(P_recv == peek);
#ifndef _HTV
			if (tfp) fprintf(tfp, "AU%d RecvHostData(0x%016llx)\n",
					 (int)P_au, (long long)P_recv);
#endif

			HtContinue(IOBK_PUSH);
		}
		break;
		case IOBK_PUSH:
		{
			if (SendHostDataBusy() || SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			if (SR_flushArg && !S_flushCnt) {
				FlushHostData();
				S_flushCnt = SR_flushArg;
				flushDec = false;
				HtContinue(IOBK_PUSH);
				break;
			}

			uint64_t exp = 0;
			exp |= (uint64_t)((uint64_t)P_au << 56);
			exp |= (uint64_t)((uint64_t)P_call << 32);
			exp |= (uint64_t)P_word;

			if (exp != P_recv) {
#ifndef _HTV
				printf("ERROR: AU%d expected 0x%016llx != 0x%016llx\n",
				       (int)P_au, (long long)exp, (long long)P_recv);
#endif
				P_errs += 1;
			}
			HtAssert(!P_errs, 0);

#ifndef _HTV
			if (tfp) fprintf(tfp, "AU%d SendHostData(0x%016llx)\n",
					 (int)P_au, (long long)exp);
#endif
			SendHostData(exp);

			P_word += 1;

			if (!P_cnt || P_word == P_cnt) {
#ifndef _HTV
				if (tfp) fprintf(tfp, "AU%d SendReturn(%d)\n",
						 (int)P_au, (int)P_errs);
#endif
				SendReturn_htmain(P_errs);
			} else {
				HtContinue(IOBK_POP);
			}
		}
		break;
		default:
			assert(0);
		}
	}

	if (GR_htReset) {
		S_flushArg = 100;
		S_flushCnt = 0;
	} else if (PR_htValid && flushDec)
		S_flushCnt -= 1;
}
