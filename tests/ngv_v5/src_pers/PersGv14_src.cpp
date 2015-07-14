#include "Ht.h"
#include "PersGv14.h"

void CPersGv14::PersGv14()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV14_ENTRYa:
			if (SendReturnBusy_gv14a()) {
				HtRetry();
				break;
			}

			if (GR_Gv14a.data != 0x1234)
				HtAssert(0, 0);

			SendReturn_gv14a();
			break;
		case GV14_ENTRYb:
			if (SendReturnBusy_gv14b()) {
				HtRetry();
				break;
			}

			if (GR_Gv14b.data != 0x2345)
				HtAssert(0, 0);

			SendReturn_gv14b();
			break;
		case GV14_ENTRYc:
			if (SendReturnBusy_gv14c()) {
				HtRetry();
				break;
			}

			if (GR_Gv14c.data != 0x3456)
				HtAssert(0, 0);

			SendReturn_gv14c();
			break;
		case GV14_ENTRYd:
			if (SendReturnBusy_gv14d()) {
				HtRetry();
				break;
			}

			if (GR_Gv14d.data != 0x4567)
				HtAssert(0, 0);

			SendReturn_gv14d();
			break;
		default:
			assert(0);
		}
	}
}
