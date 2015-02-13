#include "Ht.h"
#include "PersCtl.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

void
CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_LD: {
			BUSY_RETRY(ReadMemBusy());

			ReadMem_data(SR_addr, 0, 8);
			ReadMemPause(CTL_ADD);
		}
		break;
		case CTL_ADD: {

			if (PR_cnt == 8) {
				HtContinue(CTL_RTN);
			} else {
				S_mem.read_addr((ht_uint3)PR_cnt);
				P_result = PR_result + S_mem.read_mem();
				P_cnt = PR_cnt + 1;
				HtContinue(CTL_ADD);
			}
		}
		break;
		case CTL_RTN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain(PR_result);
		}
		break;
		default:
			assert(0);
		}
	}
}
