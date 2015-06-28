#include "Ht.h"
#include "PersGv6.h"

void CPersGv6::PersGv6()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV6_ENTRY:
			P_addr1 = 4;

			GW_Gv6a.write_addr(P_addr1);
			GW_Gv6a.data = 0x1234;

			HtContinue(GV6_WR);
			break;
		case GV6_WR:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			if (GR_Gv6a.data != 0x1234)
				HtAssert(0, 0);

			WriteMem_Gv6a(PR_addr, PR_addr1, 1);

			WriteMemPause(GV6_MRD);
			break;
		case GV6_MRD:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_addr1 = 3;

			ReadMem_Gv6b(PR_addr, P_addr1, 1);

			ReadMemPause(GV6_MRD_TST);
			break;
		case GV6_MRD_TST:
			if (GR_Gv6b.data != 0x1234)
				HtAssert(0, 0);

			HtContinue(GV6_IWR);
			break;
		case GV6_IWR:
			P_addr1 = 5;

			GW_Gv6b.write_addr(P_addr1);
			GW_Gv6b.data = 0x5678;

			HtContinue(GV6_IWR_TST);
			break;
		case GV6_IWR_TST:
			if (GR_Gv6b.data != 0x5678)
				HtAssert(0, 0);

			HtContinue(GV6_RETURN);
			break;
		case GV6_RETURN:
			if (SendReturnBusy_gv6()) {
				HtRetry();
				break;
			}

			SendReturn_gv6();
			break;
		default:
			assert(0);
		}
	}
}
