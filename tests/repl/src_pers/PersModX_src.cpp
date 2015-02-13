#include "Ht.h"
#include "PersModX.h"

void CPersModX::PersModX()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY:
			{
				if (SendReturnBusy_ModX()) {
					HtRetry();
					break;
				}

				P_result |= (uint32_t)(P_value << P_shft);

				SendReturn_ModX(P_result);
			}
			break;
		default:
			assert(0);
		}
	}
}
