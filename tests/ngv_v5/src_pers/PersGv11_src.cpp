#include "Ht.h"
#include "PersGv11.h"

void CPersGv11::PersGv11()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV11_ENTRY:
			if (SendCallBusy_gv12a()) {
				HtRetry();
				break;
			}

			GW_Gv12a.data = 0x1234;

			SendCall_gv12a(GV11_gv12b, PR_addr);
			break;
		case GV11_gv12b:
			if (SendCallBusy_gv12b()) {
				HtRetry();
				break;
			}

			P_addr1 = 5;

			GW_Gv12b.write_addr(P_addr1);
			GW_Gv12b.data = 0x2345;

			SendCall_gv12b(GV11_gv12c_WR, P_addr1);
			break;
		case GV11_gv12c_WR:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_uint16_t(PR_addr, 0x3456);

			WriteMemPause(GV11_gv12c_RD);
			break;
		case GV11_gv12c_RD:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			ReadMem_Gv12c(PR_addr);

			ReadMemPause(GV11_gv12c);
			break;
		case GV11_gv12c:
			if (SendCallBusy_gv12c()) {
				HtRetry();
				break;
			}

			SendCall_gv12c(GV11_gv12d_WR, PR_addr);
			break;
		case GV11_gv12d_WR:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_uint16_t(PR_addr, 0x4567);

			WriteMemPause(GV11_gv12d_RD);
			break;
		case GV11_gv12d_RD:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_addr1 = 6;

			ReadMem_Gv12d(PR_addr, P_addr1, 1);

			ReadMemPause(GV11_gv12d);
			break;
		case GV11_gv12d:
			if (SendCallBusy_gv12d()) {
				HtRetry();
				break;
			}

			SendCall_gv12d(GV11_RETURN, PR_addr1);
			break;
		case GV11_RETURN:
			if (SendReturnBusy_gv11()) {
				HtRetry();
				break;
			}

			SendReturn_gv11();
			break;
		default:
			assert(0);
		}
	}
}
