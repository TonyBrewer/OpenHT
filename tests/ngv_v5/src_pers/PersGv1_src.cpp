#include "Ht.h"
#include "PersGv1.h"

void CPersGv1::PersGv1()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV1_ENTRY:
		{
			P_gvAddr1 = 5;

			HtContinue(GV1_WR);
			break;
		}
		case GV1_WR:
		{
					   GW_Gv1.write_addr(PR_gvAddr1);
					   GW_Gv1 = 0x4523;

					   HtContinue(GV1_RD);
					   break;
		}
		case GV1_RD:
		{
					   if (GR_Gv1 != 0x4523)
						   HtAssert(0, 0);

					   HtContinue(GV1_RETURN);
			break;
		}
		case GV1_RETURN:
		{
			if (SendReturnBusy_gv1()) {
				HtRetry();
				break;
			}

			SendReturn_gv1();
			break;
		}
		default:
			assert(0);
		}
	}
}
