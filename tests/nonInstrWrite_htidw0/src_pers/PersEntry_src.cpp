#include "Ht.h"
#include "PersEntry.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersEntry::PersEntry()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY_CALL_WR1: {
			BUSY_RETRY(SendCallBusy_wr1());

			SendCallFork_wr1(ENTRY_JOIN_WR1);
			HtContinue(ENTRY_CALL_WR2);
		}
		break;
		case ENTRY_CALL_WR2: {
			BUSY_RETRY(SendCallBusy_wr2());

			SendCallFork_wr2(ENTRY_JOIN_WR2);
			HtContinue(ENTRY_CALL_WR3);
		}
		break;
		case ENTRY_CALL_WR3: {
			BUSY_RETRY(SendCallBusy_wr3());

			SendCallFork_wr3(ENTRY_JOIN_WR3);
			HtContinue(ENTRY_WAIT_WR1);
		}
		break;

		case ENTRY_WAIT_WR1: {
			RecvReturnPause_wr1(ENTRY_WAIT_WR2);
		}
		break;
		case ENTRY_WAIT_WR2: {
			RecvReturnPause_wr2(ENTRY_WAIT_WR3);
		}
		break;
		case ENTRY_WAIT_WR3: {
			RecvReturnPause_wr3(ENTRY_RTN);
		}
		break;

		case ENTRY_JOIN_WR1: {
			RecvReturnJoin_wr1();
		}
		break;
		case ENTRY_JOIN_WR2: {
			RecvReturnJoin_wr2();
		}
		break;
		case ENTRY_JOIN_WR3: {
			RecvReturnJoin_wr3();
		}
		break;

		case ENTRY_RTN: {
			BUSY_RETRY(SendReturnBusy_htmain());
			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}
}
