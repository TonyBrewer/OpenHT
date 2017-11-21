#include "Ht.h"
#include "PersAddLd.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersAddLd::PersAddLd()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ADD_LD1: {
			BUSY_RETRY(ReadMemBusy());

			// Memory read request
			MemAddr_t memRdAddr = SR_op1Addr + (P_vecIdx << 3);
			ReadMem_op1(memRdAddr);
			HtContinue(ADD_LD2);
		}
		break;
		case ADD_LD2: {
			BUSY_RETRY(ReadMemBusy());

			// Memory read request
			MemAddr_t memRdAddr = SR_op2Addr + (P_vecIdx << 3);
			ReadMem_op2(memRdAddr);
			ReadMemPause(ADD_RTN);
		}
		break;
		case ADD_RTN: {
			ht_uint1 dimenIdx = (ht_uint1)(PR_vecIdx & 0x1);

			BUSY_RETRY(SendReturnBusy_addLd());
			BUSY_RETRY(SendUioBusy_ldOutA(dimenIdx));
			BUSY_RETRY(SendUioBusy_ldOutB(dimenIdx));

			//fprintf(stderr, "LD %d -> Sending %ld %ld\n", (int)SR_replId, (uint64_t)PR_op1, (uint64_t)PR_op2);

			SendUioData_ldOutA(dimenIdx, PR_op1);
			SendUioData_ldOutB(dimenIdx, PR_op2);

			SendReturn_addLd();
		}
		break;
		default:
			assert(0);
		}
	}
}
