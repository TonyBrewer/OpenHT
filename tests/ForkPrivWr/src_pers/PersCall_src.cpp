#include "Ht.h"
#include "PersCall.h"

void CPersCall::PersCall()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CALL_ENTRY:
		{
			if (SendReturnBusy_call()) {
				HtRetry();
				break;
			}
			SendReturn_call();
		}
		break;
		default:
			assert(0);
		}
	}
}

