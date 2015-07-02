#include "Ht.h"
#include "PersGv9.h"

void CPersGv9::PersGv9()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV9_ENTRY:
			if (SendCallBusy_gv10a()) {
				HtRetry();
				break;
			}

			GW_Gv10a.data = 0x1234;

			SendCall_gv10a(GV9_gv10b, PR_addr);
			break;
		case GV9_gv10b:
			if (SendCallBusy_gv10b()) {
				HtRetry();
				break;
			}

			P_addr1 = 5;

			GW_Gv10b.write_addr(P_addr1);
			GW_Gv10b.data = 0x2345;

			SendCall_gv10b(GV9_gv10c_WR, P_addr1);
			break;
		case GV9_gv10c_WR:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_uint16_t(PR_addr, 0x3456);

			WriteMemPause(GV9_gv10c_RD);
			break;
		case GV9_gv10c_RD:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			ReadMem_Gv10c(PR_addr);

			ReadMemPause(GV9_gv10c);
			break;
		case GV9_gv10c:
			if (SendCallBusy_gv10c()) {
				HtRetry();
				break;
			}

			SendCall_gv10c(GV9_gv10d_WR, PR_addr);
			break;
		case GV9_gv10d_WR:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_uint16_t(PR_addr, 0x4567);

			WriteMemPause(GV9_gv10d_RD);
			break;
		case GV9_gv10d_RD:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_addr1 = 6;

			ReadMem_Gv10d(PR_addr, P_addr1, 1);

			ReadMemPause(GV9_gv10d);
			break;
		case GV9_gv10d:
			if (SendCallBusy_gv10d()) {
				HtRetry();
				break;
			}

			SendCall_gv10d(GV9_gv10eIw, PR_addr1);
			break;
		case GV9_gv10eIw:
			if (SendCallBusy_gv10eIw()) {
				HtRetry();
				break;
			}

			GW_Gv10e.data = 0x5678;

			SendCall_gv10eIw(GV9_gv10e_WR);
			break;
		case GV9_gv10e_WR:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_uint16_t(PR_addr, 0x6789);

			WriteMemPause(GV9_gv10e_RD);
			break;
		case GV9_gv10e_RD:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			ReadMem_Gv10e(PR_addr);

			ReadMemPause(GV9_gv10eMw);
			break;
		case GV9_gv10eMw:
			if (SendCallBusy_gv10eMw()) {
				HtRetry();
				break;
			}

			SendCall_gv10eMw(GV9_gv10fIw);
			break;
		case GV9_gv10fIw:
			if (SendCallBusy_gv10fIw()) {
				HtRetry();
				break;
			}

			P_addr1 = 0;

			GW_Gv10f.write_addr(P_addr1);
			GW_Gv10f.data = 0x789a;

			SendCall_gv10fIw(GV9_gv10f_WR, P_addr1);
			break;
		case GV9_gv10f_WR:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_uint16_t(PR_addr, 0x89ab);

			WriteMemPause(GV9_gv10f_RD);
			break;
		case GV9_gv10f_RD:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_addr1 = 1;

			ReadMem_Gv10f(PR_addr, P_addr1, 1);

			ReadMemPause(GV9_gv10fMw);
			break;
		case GV9_gv10fMw:
			if (SendCallBusy_gv10fMw()) {
				HtRetry();
				break;
			}

			SendCall_gv10fMw(GV9_RETURN, PR_addr1);
			break;
		case GV9_RETURN:
			if (SendReturnBusy_gv9()) {
				HtRetry();
				break;
			}

			SendReturn_gv9();
			break;
		default:
			assert(0);
		}
	}
}
