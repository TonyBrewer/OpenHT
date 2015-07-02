#include "Ht.h"
#include "PersGv13.h"

void CPersGv13::PersGv13()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV13_ENTRY:
			if (SendCallBusy_gv14a()) {
				HtRetry();
				break;
			}

			GW_Gv14a.data = 0x1234;

			SendCall_gv14a(GV13_gv14b, PR_addr);
			break;
		case GV13_gv14b:
			if (SendCallBusy_gv14b()) {
				HtRetry();
				break;
			}

			P_addr1 = 5;

			GW_Gv14b.write_addr(P_addr1);
			GW_Gv14b.data = 0x2345;

			SendCall_gv14b(GV13_gv14c_WR, P_addr1);
			break;
		case GV13_gv14c_WR:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_uint16_t(PR_addr, 0x3456);

			WriteMemPause(GV13_gv14c_RD);
			break;
		case GV13_gv14c_RD:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			ReadMem_Gv14c(PR_addr);

			ReadMemPause(GV13_gv14c);
			break;
		case GV13_gv14c:
			if (SendCallBusy_gv14c()) {
				HtRetry();
				break;
			}

			SendCall_gv14c(GV13_gv14d_WR, PR_addr);
			break;
		case GV13_gv14d_WR:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_uint16_t(PR_addr, 0x4567);

			WriteMemPause(GV13_gv14d_RD);
			break;
		case GV13_gv14d_RD:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_addr1 = 6;

			ReadMem_Gv14d(PR_addr, P_addr1, 1);

			ReadMemPause(GV13_gv14d);
			break;
		case GV13_gv14d:
			if (SendCallBusy_gv14d()) {
				HtRetry();
				break;
			}

			SendCall_gv14d(GV13_RETURN, PR_addr1);
			break;
		case GV13_RETURN:
			if (SendReturnBusy_gv13()) {
				HtRetry();
				break;
			}

			SendReturn_gv13();
			break;
		default:
			assert(0);
		}
	}
}
