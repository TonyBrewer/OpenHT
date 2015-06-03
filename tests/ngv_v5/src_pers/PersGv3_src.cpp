#include "Ht.h"
#include "PersGv3.h"

void CPersGv3::PersGv3()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV3_ENTRY:
		{
			P_gvAddr1 = 5;

			HtContinue(GV3_WR);
			break;
		}
		case GV3_WR:
		{
					   if (WriteMemBusy()) {
						   HtRetry();
						   break;
					   }

					   WriteMem_uint16_t(PR_addr, 0x4523);

					   WriteMemPause(GV3_RD);
					   break;
		}
		case GV3_RD:
		{
					   if (ReadMemBusy()) {
						   HtRetry();
						   break;
					   }

					   ReadMem_Gv3(PR_addr, PR_gvAddr1, 1);

					   ReadMemPause(GV3_TST);
					   break;
		}
		case GV3_TST:
		{
					   if (GR_Gv3.data != 0x4523)
						   HtAssert(0, 0);

					   HtContinue(GV3_RETURN);
					   break;
		}
		case GV3_RETURN:
		{
						if (SendReturnBusy_gv3()) {
							HtRetry();
							break;
						}

						SendReturn_gv3();
						break;
		}
		default:
			assert(0);
		}
	}
}
