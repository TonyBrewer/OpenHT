#include "Ht.h"
#include "PersGv17.h"

void CPersGv17::PersGv17()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV17_ENTRY:
			GW_Gv17.write_addr(2, 4);
			GW_Gv17 = 0x1829304a5b6c7d8eLL;

			HtContinue(GV17a_WR);
			break;
		case GV17a_WR:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_Gv17a(PR_addr, 2, 4);

			WriteMemPause(GV17a_RD);
			break;
		case GV17a_RD:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_addr1 = 3;
			P_addr2 = 5;

			ReadMem_Gv17a(PR_addr, 3, 5);

			ReadMemPause(GV17a_TST);
			break;
		case GV17a_TST:
			if (GR_Gv17 != 0x1829304a5b6c7d8eLL)
				HtAssert(0, 0);

			HtContinue(GV17b_INIT);
			break;
		case GV17b_INIT:
			GW_Gv17.write_addr(1, 3);
			GW_Gv17 = 0x29304a5b6c7d8e18LL;

			HtContinue(GV17b_WR);
			break;
		case GV17b_WR:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_Gv17b(PR_addr, 3);

			WriteMemPause(GV17b_RD);
			break;
		case GV17b_RD:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_addr1 = 2;
			P_addr2 = 4;

			ReadMem_Gv17b(PR_addr, 4);

			ReadMemPause(GV17b_TST);
			break;
		case GV17b_TST:
			if (GR_Gv17 != 0x29304a5b6c7d8e18LL)
				HtAssert(0, 0);

			HtContinue(GV17_RETURN);
			break;
		case GV17_RETURN:
			if (SendReturnBusy_gv17()) {
				HtRetry();
				break;
			}

			SendReturn_gv17();
			break;
		default:
			assert(0);
		}
	}
}
