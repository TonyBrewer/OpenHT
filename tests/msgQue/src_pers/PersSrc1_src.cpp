#include "Ht.h"
#include "PersSrc1.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void CPersSrc1::PersSrc1()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case SRC1_ENTRY:
		{
			P_msgCnt = 0;

			HtContinue(SRC1_SEND);
		}
		break;
		case SRC1_SEND:
		{
			BUSY_RETRY(SendMsgFull_Tst());

			SendMsg_Tst(0x5A00 | P_msgCnt);

			if (P_msgCnt == 16)
				HtContinue(SRC1_RETURN);
			else {
				P_msgCnt += 1;
				HtContinue(SRC1_SEND);
			}
		}
		break;
		case SRC1_RETURN:
		{
			BUSY_RETRY(SendReturnBusy_src1());

			// Return to host interface
			SendReturn_src1(P_modId);
		}
		break;
		default:
			assert(0);
		}
	}
}
