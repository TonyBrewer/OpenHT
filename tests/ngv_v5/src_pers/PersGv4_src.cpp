#include "Ht.h"
#include "PersGv4.h"

void CPersGv4::PersGv4()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV4_ENTRY:
		{
			P_gvAddr1 = 5;

			HtContinue(GV4_WR);
			break;
		}
		case GV4_WR:
		{
					   if (WriteMemBusy()) {
						   HtRetry();
						   break;
					   }

					   WriteMem_uint16_t(PR_addr, 0x4523);

					   WriteMemPause(GV4_RD);
					   break;
		}
		case GV4_RD:
		{
					   if (ReadMemBusy()) {
						   HtRetry();
						   break;
					   }

					   ReadMem_Gv4(PR_addr, PR_gvAddr1, 1);

					   ReadMemPause(GV4_TST);
					   break;
		}
		case GV4_TST:
		{
					   if (GR_Gv4.data != 0x4523)
						   HtAssert(0, 0);

					   HtContinue(GV4_RETURN);
					   break;
		}
		case GV4_RETURN:
		{
						if (SendReturnBusy_gv4()) {
							HtRetry();
							break;
						}

						SendReturn_gv4();
						break;
		}
		default:
			assert(0);
		}
	}
}
