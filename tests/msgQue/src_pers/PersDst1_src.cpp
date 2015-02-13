#include "Ht.h"
#include "PersDst1.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void CPersDst1::PersDst1()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case DST1_ENTRY:
		{
			P_msgCnt = 0;

			HtContinue(DST1_RECV);
		}
		break;
		case DST1_RECV:
		{
			BUSY_RETRY(!RecvMsgReady_Tst());

			uint16_t tstMsg = RecvMsg_Tst();

			P_error |= (tstMsg >> 8) != 0x5A;
			P_error |= (tstMsg & 0xff) != P_msgCnt;

			if (P_msgCnt == 16)
				HtContinue(DST1_RETURN);
			else {
				P_msgCnt += 1;
				HtContinue(DST1_RECV);
			}
		}
		break;
		case DST1_RETURN:
		{
			BUSY_RETRY(SendReturnBusy_dst1());

			// Return to host interface
			SendReturn_dst1(P_modId, P_error);
		}
		break;
		default:
			assert(0);
		}
	}
}
