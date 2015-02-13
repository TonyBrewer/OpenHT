#include "Ht.h"
#include "PersModB.h"

void CPersModB::PersModB()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY:
			{
				if (SendCallBusy_ModC()) {
					HtRetry();
					break;
				}

				P_outData |= 0x000800 + (P_inData & 0x0c);

				SendCall_ModC(RETURN, P_inData, P_outData);
			}
			break;
		case RETURN:
			{
				if (SendReturnBusy_ModB()) {
					HtRetry();
					break;
				}

				P_outData |= 0x080000;

				SendReturn_ModB(P_outData, P_rtnInData);
			}
			break;
		default:
			assert(0);
		}
	}
}
