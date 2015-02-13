#include "Ht.h"
#include "PersObk.h"

void
CPersObk::PersObk()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case OBK_PUSH:
		{
			if (P_loopCnt == 0) {
				HtContinue(OBK_RTN);
				break;
			}

			if (SendHostDataBusy()) {
				HtRetry();
				break;
			}

			uint64_t data = (uint64_t)((uint64_t)P_pau << 56);
			data |= (uint64_t)((uint64_t)P_loopCnt);
#ifndef _HTV
			if (!(P_loopCnt % 10000))
				fprintf(stderr, "%d loopCnt = %d\n",
					(int)P_pau, (int)P_loopCnt);
#endif
			SendHostData(data);

			P_loopCnt -= 1;

			HtContinue(OBK_PUSH);
		}
		break;
		case OBK_RTN:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}
			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}
}
