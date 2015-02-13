#include "Ht.h"
#include "PersClk2x.h"

void CPersClk2x::PersClk2x()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CLK2X_ENTRY:
			{
				if (SendCallBusy_clk1x() || SendHostMsgBusy()) {
					HtRetry();
					break;
				}

				HtAssert(SR_msg == 0x123, (int)SR_msg);

				SendHostMsg(OHM_MSG2X, (ht_uint56)SR_msg);

				SendCall_clk1x(CLK2X_RTN);
			}
			break;
		case CLK2X_RTN:
			{
				if (SendReturnBusy_clk2x()) {
					HtRetry();
					break;
				}

				SendReturn_clk2x();
			}
			break;
		default:
			assert(0);
		}
	}
}
