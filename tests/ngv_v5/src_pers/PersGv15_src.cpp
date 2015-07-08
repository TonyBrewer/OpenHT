#include "Ht.h"
#include "PersGv15.h"

void CPersGv15::PersGv15()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV15_ENTRY:
		{
			P_gvAddr1 = 5;

			HtContinue(GV15_WR);
			break;
		}
		case GV15_WR:
		{
			GW_Gv15.write_addr(PR_gvAddr1);
			GW_Gv15.m_u64 = 0x25364758697a8b9cLL;

			HtContinue(GV15_RD);
			break;
		}
		case GV15_RD:
		{
			if (GR_Gv15.m_u64 != 0x25364758697a8b9cLL)
				HtAssert(0, 0);

			HtContinue(GV15_RETURN);
			break;
		}
		case GV15_RETURN:
		{
			if (SendReturnBusy_gv15()) {
				HtRetry();
				break;
			}

			SendReturn_gv15();
			break;
		}
		default:
			assert(0);
		}
	}
}
