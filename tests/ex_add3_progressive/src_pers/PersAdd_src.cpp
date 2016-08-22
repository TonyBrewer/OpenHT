#include "Ht.h"
#include "PersAdd.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

void
CPersAdd::PersAdd()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY: {
			P_idx = 0;
			HtContinue(LD1);
		}
		break;
		case LD1: {
			BUSY_RETRY(ReadMemBusy());

			ReadMem_op1Mem(P_op1 + P_idx*8, P_idx);

			if (P_idx == 7) {
				P_idx = 0;
				HtContinue(LD2);
			} else {
				P_idx += 1;
				HtContinue(LD1);
			}
		}
		break;
		case LD2: {
			BUSY_RETRY(ReadMemBusy());

			ReadMem_op2Mem(P_op2 + P_idx*8, P_idx);

			if (P_idx == 7) {
				P_idx = 0;
				ReadMemPause(ADD);
			} else {
				P_idx += 1;
				HtContinue(LD2);
			}
		}
		break;
		case ADD: {
			for (int i=0; i<8; i++)  {
				S_res[i] = S_op1Mem[i] + S_op2Mem[i];
#ifndef _HTV
fprintf(stderr, "%lld = %lld + %lld\n", (long long)S_res[i], (long long)S_op1Mem[i], (long long)S_op2Mem[i]);
#endif
			}
			HtContinue(WR);
		}
		break;
		case WR: {
			BUSY_RETRY(WriteMemBusy());

			WriteMem(P_op3 + P_idx*8, S_res[P_idx]);

			if (P_idx == 7) {
				P_idx = 0;
				WriteMemPause(RTN);
			} else {
				P_idx += 1;
				HtContinue(WR);
			}
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
