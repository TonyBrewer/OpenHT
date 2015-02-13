#include "Ht.h"
#include "PersModC.h"

void CPersModC::PersModC()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY:
			{
				if (SendCallBusy_ModD()) {
					HtRetry();
					break;
				}

				P_outData |= 0x001000 + (P_inData & 0x30);

				SendCall_ModD(RETURN, P_inData, P_outData);
			}
			break;
		case RETURN:
			{
				if (SendReturnBusy_ModC()) {
					HtRetry();
					break;
				}

				P_outData |= 0x100000;

				SendReturn_ModC(P_outData, P_rtnInData);
			}
			break;
		default:
			assert(0);
		}
	}
}
