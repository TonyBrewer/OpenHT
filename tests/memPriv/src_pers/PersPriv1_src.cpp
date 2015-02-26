#include "Ht.h"
#include "PersPriv1.h"

#ifndef _HTV
int entryMask = 0;
#endif

void CPersPriv1::PersPriv1()
{
	if (PR_htValid) {
		switch (PR_htInstr) {
		case PRIV1_ENTRY:
		{
			P_data = 0x3434343456565656LL;

			HtContinue(PRIV1_WRITE);
			break;
		}
		case PRIV1_WRITE:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_data(PR_memAddr);

			WriteMemPause(PRIV1_READ);
			break;
		}
		case PRIV1_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_data(PR_memAddr);

			ReadMemPause(PRIV1_RETURN);
			break;
		}
		case PRIV1_RETURN:
		{
			if (SendReturnBusy_priv1()) {
				HtRetry();
				break;
			}

			if (PR_data != 0x3434343456565656LL) P_err = true;

			SendReturn_priv1(P_err);
			break;
		}
		default:
			assert(0);
		}
	}
}
