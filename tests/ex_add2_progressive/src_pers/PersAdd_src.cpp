#include "Ht.h"
#include "PersAdd.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

void
CPersAdd::PersAdd()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case LD1: {
			BUSY_RETRY(ReadMemBusy());

			ReadMem_op1Mem(P_op1);
			HtContinue(LD2);
		}
		break;
		case LD2: {
			BUSY_RETRY(ReadMemBusy());

			ReadMem_op2Mem(P_op2);
			ReadMemPause(ST);
		}
		break;
		case ST: {
			BUSY_RETRY(WriteMemBusy());

			uint64_t res = S_op1Mem + S_op2Mem;
			WriteMem(P_op3, res);
			WriteMemPause(RTN);
		}
		break;
		case RTN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}
}
