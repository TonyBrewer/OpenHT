#include "Ht.h"
#include "PersModA.h"

void CPersModA::PersModA()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY:
			{
				if (SendCallBusy_ModB(S_destId)) {
					HtRetry();
					break;
				}

				P_outData |= 0x000300 + (P_inData & 0x03);

				SendCall_ModB(RETURN, S_destId, P_inData, P_outData);

				S_destId ^= 1;
			}
			break;
		case RETURN:
			{
				if (SendReturnBusy_ModA()) {
					HtRetry();
					break;
				}

				P_outData |= 0x030000;

				SendReturn_ModA(P_outData, P_rtnInData);
			}
			break;
		default:
			assert(0);
		}
	}

	if (GR_htReset)
		S_destId = 0;
}
