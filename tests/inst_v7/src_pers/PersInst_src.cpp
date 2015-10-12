#include "Ht.h"
#include "PersInst.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersInst::PersInst()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case INST_ENTRY: {
			HtContinue(INST_RTN);
			break;
		}
		case INST_RTN: {
			BUSY_RETRY(SendReturnBusy_inst());

			SendReturn_inst();
		}
		break;
		default:
			assert(0);
		}
	}
}
