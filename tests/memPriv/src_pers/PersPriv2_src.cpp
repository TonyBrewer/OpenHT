#include "Ht.h"
#include "PersPriv2.h"

void CPersPriv2::PersPriv2()
{
	if (PR_htValid) {
		switch (PR_htInstr) {
		case PRIV2_ENTRY:
		{
			P_rdAddr = 3;

			PW_data.write_addr(3);
			PW_data = 0x778899aabbccddeeLL;

			HtContinue(PRIV2_WRITE);
			break;
		}
		case PRIV2_WRITE:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_data(PR_memAddr, 3, 1);

			WriteMemPause(PRIV2_READ);
			break;
		}
		case PRIV2_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_data(PR_memAddr, 3, 1);

			ReadMemPause(PRIV2_RETURN);
			break;
		}
		case PRIV2_RETURN:
		{
			if (SendReturnBusy_priv2()) {
				HtRetry();
				break;
			}

			if (PR_data != 0x778899aabbccddeeLL) P_err = true;

			SendReturn_priv2(P_err);
			break;
		}
		default:
			assert(0);
		}
	}
}
