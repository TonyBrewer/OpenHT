#include "Ht.h"
#include "PersExec1.h"

void
CPersExec1::PersExec1()
{
	if (PR_htValid) {
		switch (PR_htInst) {

		// Main entry point from PersStart_src.cpp
		// Receives data from calling function into private exec1Data variable
		case EXEC1_ENTRY:
		{

			P_exec1Data = (uint64_t)(P_exec1Data << 4) | 0xc;

			// On the next active cycle, move to EXEC1_TXFR instruction
			HtContinue(EXEC1_TXFR);
		}
		break;

		case EXEC1_TXFR:
		{
			if (SendTransferBusy_funcB()) {
				HtRetry();
				break;
			}

			P_exec1Data = (uint64_t)(P_exec1Data << 4) | 0xd;

			// Transfer control to Exec2 module with a parameter - private exec1Data variable
			// Transfers do not expect a return - this module does not expect reentry
			SendTransfer_funcB(P_exec1Data);
		}
		break;

		default:
			assert(0);
		}
	}
}
