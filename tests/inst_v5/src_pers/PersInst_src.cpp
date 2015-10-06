#include "Ht.h"
#include "PersInst.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersInst::PersInst()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case INST_REPL: {
			BUSY_RETRY(SendCallBusy_repl());

			SendCall_repl(INST_RTN, PR_instInstId, SR_replId);
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
