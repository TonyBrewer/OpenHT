#include "Ht.h"
#include "PersGv7.h"

void CPersGv7::PersGv7()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV7_ENTRY:
			if (SendCallBusy_gv8a()) {
				HtRetry();
				break;
			}

			GW_Gv8a.data = 0x1234;

			SendCall_gv8a(GV7_gv8b, PR_addr);
			break;
		case GV7_gv8b:
			if (SendCallBusy_gv8b()) {
				HtRetry();
				break;
			}

			P_addr1 = 5;

			GW_Gv8b.write_addr(P_addr1);
			GW_Gv8b.data = 0x2345;

			SendCall_gv8b(GV7_gv8c_WR, P_addr1);
			break;
		case GV7_gv8c_WR:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_uint16_t(PR_addr, 0x3456);

			WriteMemPause(GV7_gv8c_RD);
			break;
		case GV7_gv8c_RD:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			ReadMem_Gv8c(PR_addr);

			ReadMemPause(GV7_gv8c);
			break;
		case GV7_gv8c:
			if (SendCallBusy_gv8c()) {
				HtRetry();
				break;
			}

			SendCall_gv8c(GV7_gv8d_WR, PR_addr);
			break;
		case GV7_gv8d_WR:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_uint16_t(PR_addr, 0x4567);

			WriteMemPause(GV7_gv8d_RD);
			break;
		case GV7_gv8d_RD:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_addr1 = 6;

			ReadMem_Gv8d(PR_addr, P_addr1, 1);

			ReadMemPause(GV7_gv8d);
			break;
		case GV7_gv8d:
			if (SendCallBusy_gv8d()) {
				HtRetry();
				break;
			}

			SendCall_gv8d(GV7_gv8eIw, PR_addr1);
			break;
		case GV7_gv8eIw:
			if (SendCallBusy_gv8eIw()) {
				HtRetry();
				break;
			}

			GW_Gv8e.data = 0x5678;

			SendCall_gv8eIw(GV7_gv8e_WR);
			break;
		case GV7_gv8e_WR:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_uint16_t(PR_addr, 0x6789);

			WriteMemPause(GV7_gv8e_RD);
			break;
		case GV7_gv8e_RD:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			ReadMem_Gv8e(PR_addr);

			ReadMemPause(GV7_gv8eMw);
			break;
		case GV7_gv8eMw:
			if (SendCallBusy_gv8eMw()) {
				HtRetry();
				break;
			}

			SendCall_gv8eMw(GV7_gv8fIw);
			break;
		case GV7_gv8fIw:
			if (SendCallBusy_gv8fIw()) {
				HtRetry();
				break;
			}

			P_addr1 = 0;

			GW_Gv8f.write_addr(P_addr1);
			GW_Gv8f.data = 0x789a;

			SendCall_gv8fIw(GV7_gv8f_WR, P_addr1);
			break;
		case GV7_gv8f_WR:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_uint16_t(PR_addr, 0x89ab);

			WriteMemPause(GV7_gv8f_RD);
			break;
		case GV7_gv8f_RD:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_addr1 = 1;

			ReadMem_Gv8f(PR_addr, P_addr1, 1);

			ReadMemPause(GV7_gv8fMw);
			break;
		case GV7_gv8fMw:
			if (SendCallBusy_gv8fMw()) {
				HtRetry();
				break;
			}

			SendCall_gv8fMw(GV7_RETURN, PR_addr1);
			break;
		case GV7_RETURN:
			if (SendReturnBusy_gv7()) {
				HtRetry();
				break;
			}

			SendReturn_gv7();
			break;
		default:
			assert(0);
		}
	}
}
