#include "Ht.h"
#include "PersAdd.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

void
CPersAdd::PersAdd()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY: {
			HtContinue(LD1);
		}
		break;
		case LD1: {
			BUSY_RETRY(ReadMemBusy());

			ReadMem_op1Mem(P_op1, 0, 8);
			HtContinue(LD2);
		}
		break;
		case LD2: {
			BUSY_RETRY(ReadMemBusy());

			ReadMem_op2Mem(P_op2, 0, 8);
			ReadMemPause(ADD);
		}
		break;
		case ADD: {
			for (int i=0; i<8; i++)
				S_res[i] = S_op1Mem[i] + S_op2Mem[i];
			HtContinue(WR);
		}
		break;
		case WR: {
			BUSY_RETRY(WriteMemBusy());

			WriteMem_res(P_op3, 0, 8);
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
