#include "Ht.h"
#include "PersModD.h"

void CPersModD::PersModD()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY:
			{
				if (SendReturnBusy_ModD()) {
					HtRetry();
					break;
				}

				P_outData |= 0x000000 + (P_inData & 0xc0);

				SendReturn_ModD(P_outData, P_inData);
			}
			break;
		default:
			assert(0);
		}
	}
}
