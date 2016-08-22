#include "Ht.h"
#include "PersCtl.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY: {
			P_addr = PR_dPtr;
			HtContinue(CTL_LD);
			break;
		}
		break;
		case CTL_LD: {
			BUSY_RETRY(ReadMemBusy());

			// Memory read request
			ReadMem_addr(PR_addr);
			if (PR_curIdx < PR_totIdx) {
				P_curIdx = PR_curIdx + 1;
				ReadMemPause(CTL_LD);
			} else {
				ReadMemPause(CTL_RETURN);
			}
		}
		break;
		case CTL_RETURN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}
}
