#include "Ht.h"
#include "PersModA.h"

void CPersModA::PersModA()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY:
			{
				if (SendCallBusy_ModX()) {
					HtRetry();
					break;
				}

				SendCall_ModX(RETURN, P_shft, P_value, P_result);
			}
			break;
		case RETURN:
			{
				if (SendReturnBusy_ModA()) {
					HtRetry();
					break;
				}

				SendReturn_ModA(P_result);
			}
			break;
		default:
			assert(0);
		}
	}
}
