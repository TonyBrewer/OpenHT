#include "Ht.h"
#include "PersCtl.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

void
CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY: {
		  
			// Build write buffer
			S_wrBuf[0] = 0;
			S_wrBuf[1] = 1;
			S_wrBuf[2] = 2;
			S_wrBuf[3] = 3;
			S_wrBuf[4] = 4;
			S_wrBuf[5] = 5;
			S_wrBuf[6] = 6;
			S_wrBuf[7] = 7;

			P_wrGrpId = 1;

			HtContinue(CTL_ST);
		}
		break;
		case CTL_ST: {

			BUSY_RETRY(WriteMemBusy());

			if (PR_cnt == PR_length) {
				// Done Writes
				WriteMemPause(PR_wrGrpId, CTL_RTN);
			} else {
				// More writes to do
				WriteMem_data(PR_wrGrpId, S_addr, 0, 8);
				S_addr += 8*8;
				P_cnt = PR_cnt + 1;
				HtContinue(CTL_ST);
			}

		}
		break;
		case CTL_RTN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}
}
