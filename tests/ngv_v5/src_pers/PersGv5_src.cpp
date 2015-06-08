#include "Ht.h"
#include "PersGv5.h"

void CPersGv5::PersGv5()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV5_ENTRY:
		{
						  GW_Gv5a.data1 = 0x1234;
						  GW_Gv5a.data2 = 0x5678;

						  HtContinue(GV5_WR);
						  break;
		}
		case GV5_WR:
		{
					   if (WriteMemBusy()) {
						   HtRetry();
						   break;
					   }

					   WriteMem_Gv5a(PR_addr);

					   WriteMemPause(GV5_RD);
					   break;
		}
		case GV5_RD:
		{
					   if (ReadMemBusy()) {
						   HtRetry();
						   break;
					   }

					   ReadMem_Gv5b(PR_addr);

					   ReadMemPause(GV5_TST);
					   break;
		}
		case GV5_TST:
		{
						if (GR_Gv5b.data1 != 0x1234 || GR_Gv5b.data2 != 0x5678)
							HtAssert(0, 0);

						HtContinue(GV5_RETURN);
						break;
		}
		case GV5_RETURN:
		{
						   if (SendReturnBusy_gv5()) {
							   HtRetry();
							   break;
						   }

						   SendReturn_gv5();
						   break;
		}
		default:
			assert(0);
		}
	}
}
