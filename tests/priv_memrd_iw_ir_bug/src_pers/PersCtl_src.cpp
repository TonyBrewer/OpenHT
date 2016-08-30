#include "Ht.h"
#include "PersCtl.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

void
CPersCtl::PersCtl()
{

	uint64_t res1 = (uint64_t)P_mem;
	uint64_t res2 = (uint64_t)PR_mem;

	if (PR_htValid) {
		switch (PR_htInst) {
		case LD: {
			BUSY_RETRY(ReadMemBusy());

			ReadMem_mem(SR_varAddr);
			ReadMemPause(ASSIGN);
		}
		break;
		case ASSIGN: {
			BUSY_RETRY(ReadMemBusy());

			P_res3 = (uint64_t)P_mem;
			P_res4 = (uint64_t)PR_mem;

			HtContinue(RTN);
		}
		break;
		case RTN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain(res1, res2, PR_res3, PR_res4);
		}
		break;
		default:
			assert(0);
		}
	}
}
