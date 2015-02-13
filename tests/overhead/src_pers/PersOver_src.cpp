#include "Ht.h"
#include "PersOver.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

void
CPersOver::PersOver()
{
	if (PR_htValid) {
		switch (PR_htInstr) {
		case OVER_RD: {
			BUSY_RETRY(ReadMemBusy());

			ReadMem_data(P_addr);
			ReadMemPause(OVER_WR);
		}
		break;
		case OVER_WR: {
			BUSY_RETRY(WriteMemBusy());

			WriteMem(P_addr, ~PR_data);
			WriteMemPause(OVER_RSM);
		}
		break;
		case OVER_RSM: {
			S_bResume = true;
			HtPause(OVER_RTN);
		}
		break;
		case OVER_RTN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}

	if (SR_bResume) {
		S_bResume = false;
		HtResume(0);
	}
}
