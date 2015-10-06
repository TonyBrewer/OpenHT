#include "Ht.h"
#include "PersRepl.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersRepl::PersRepl()
{
	if (PR_htValid) {
		switch (PR_htInstr) {
		case REPL_RTN: {
			BUSY_RETRY(SendReturnBusy_repl());

			printf("InstInstID %d, InstReplID %d, ReplReplID %d\n", PR_instInstId, PR_instReplId, SR_replId);

			SendReturn_repl();
		}
		break;
		default:
			assert(0);
		}
	}
}
