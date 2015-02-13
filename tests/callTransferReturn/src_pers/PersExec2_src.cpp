#include "Ht.h"
#include "PersExec2.h"

void
CPersExec2::PersExec2()
{
	if (PR_htValid) {

		// Main entry point from PersExec2_src.cpp
		// Receives data from calling funtion into private exec2Data variable
		switch (PR_htInst) {
		case EXEC2_ENTRY:
		{
			if (SendReturnBusy_funcA()) {
				HtRetry();
				break;
			}

			P_exec2Data = (uint64_t)(P_exec2Data << 4) | 0xe;

			// Return to Start???
			// This thread entered Exec2 via SendTransfer from Exec1
			//   Transfers do not allow returns by themselves
			// However, this thread originally entered Exec1 via SendCall from Start
			//   SendCall -does- allow returns, so a return later on (even from
			//   a module after a Transfer) will return to the original SendCall point

			// In this way, returns always return to the original function -call-, not
			// the module.  Therefore return to the original SendCall_funcA
			SendReturn_funcA(P_exec2Data);
		}
		break;

		default:
			assert(0);
		}
	}
}
