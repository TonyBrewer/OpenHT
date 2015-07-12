#include "Ht.h"
#include "PersGv16.h"

void CPersGv16::PersGv16()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV16_ENTRY:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem_uint64_t(PR_addr+8, 0x1829304a5b6c7d8eLL);

			WriteMemPause(GV16_RD);
			break;
		case GV16_RD:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_gvAddr1 = 0x34;

			ReadMem_Gv16(PR_addr, P_gvAddr1, 4, 2);

			ReadMemPause(GV16_RETURN);
			break;
		case GV16_RETURN:
			if (GR_Gv16[5] != 0x1829304a5b6c7d8eLL)
				HtAssert(0, 0);

			if (SendReturnBusy_gv16()) {
				HtRetry();
				break;
			}

			SendReturn_gv16();
			break;
		default:
			assert(0);
		}
	}
}
