#include "Ht.h"
#include "PersAddSt.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersAddSt::PersAddSt()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ADD_ST: {
			BUSY_RETRY(WriteMemBusy());

			tmp_t rslt;
			bool bRead = false;
			if (!S_rsltQue[0].empty()) {
				rslt = S_rsltQue[0].front();
				S_rsltQue[0].pop();
				//fprintf(stderr, "ST 0 -> Popping %ld\n", (uint64_t)rslt);
				bRead = true;
			}
			else if (!S_rsltQue[1].empty()) {
				rslt = S_rsltQue[1].front();
				S_rsltQue[1].pop();
				//fprintf(stderr, "ST 1 -> Popping %ld\n", (uint64_t)rslt);
				bRead = true;
			}

			if (!bRead) {
				HtRetry();
			} else {

				// Memory write request
				MemAddr_t memWrAddr = SR_resAddr + (P_vecIdx << 3);
				WriteMem(memWrAddr, rslt);

				P_vecIdx = PR_vecIdx + 1;

				if (P_vecIdx == PR_vecLen) {
					WriteMemPause(ADD_RTN);
				} else {
					HtContinue(ADD_ST);
				}
			}
			
		}
		break;
		case ADD_RTN: {
			BUSY_RETRY(SendReturnBusy_addSt());

			SendReturn_addSt();
		}
		break;
		default:
			assert(0);
		}
	}

	if (RecvUioReady_stInA(0) && RecvUioReady_stInB(0) && !S_rsltQue[0].full()) {
		tmp_t a, b, rslt;
	  	a = RecvUioData_stInA(0);
		b = RecvUioData_stInB(0);
		//fprintf(stderr, "ST 0 -> Receiving %ld %ld\n", (uint64_t)a, (uint64_t)b);
		rslt = a + b;
		S_rsltQue[0].push(rslt);
	}
	if (RecvUioReady_stInA(1) && RecvUioReady_stInB(1) && !S_rsltQue[1].full()) {
		tmp_t a, b, rslt;
	  	a = RecvUioData_stInA(1);
		b = RecvUioData_stInB(1);
		//fprintf(stderr, "ST 1 -> Receiving %ld %ld\n", (uint64_t)a, (uint64_t)b);
		rslt = a + b;
		S_rsltQue[1].push(rslt);
	}

	
}
