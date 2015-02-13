#include "Ht.h"
#include "PersStart.h"

void
CPersStart::PersStart()
{
	if (PR_htValid) {
		switch (PR_htInst) {

		// Main entry point from Main.cpp
		// Receives data from calling funtion into private startData variable
		case START_ENTRY:
		{
			if (SendCallBusy_funcA()) {
				HtRetry();
				break;
			}

			// Shift and append a byte to prove we are manipulating the expected data
			P_startData = (uint64_t)(P_startData << 4) | 0xb;

			// Pass control to the Exec1 module with a parameter - private startData variable
			// Upon return, start from START_EXIT instruction
			SendCall_funcA(START_EXIT, P_startData);
		}
		break;

		case START_EXIT:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			P_startData = (uint64_t)(P_startData << 4) | 0xf;

			// Exit application and return to Main.cpp with private startData variable
			SendReturn_htmain(P_startData);

			break;
		}
		default:
			assert(0);
		}
	}
}
