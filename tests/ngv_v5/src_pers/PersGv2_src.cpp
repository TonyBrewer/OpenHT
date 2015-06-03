#include "Ht.h"
#include "PersGv2.h"

void CPersGv2::PersGv2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV2_ENTRY:
		{
			P_gvAddr1 = 5;

			HtContinue(GV2_WR);
			break;
		}
		case GV2_WR:
		{
					   GW_Gv2.write_addr(PR_gvAddr1);
					   GW_Gv2 = 0x4523;

					   HtContinue(GV2_RD);
					   break;
		}
		case GV2_RD:
		{
					   if (GR_Gv2 != 0x4523)
						   HtAssert(0, 0);

					   HtContinue(GV2_RETURN);
			break;
		}
		case GV2_RETURN:
		{
			if (SendReturnBusy_gv2()) {
				HtRetry();
				break;
			}

			SendReturn_gv2();
			break;
		}
		default:
			assert(0);
		}
	}
}
