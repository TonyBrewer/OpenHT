#include "Ht.h"
#include "PersGv12.h"

void CPersGv12::PersGv12()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV12_ENTRYa:
			if (SendReturnBusy_gv12a()) {
				HtRetry();
				break;
			}

			if (GR_Gv12a.data != 0x1234)
				HtAssert(0, 0);

			SendReturn_gv12a();
			break;
		case GV12_ENTRYb:
			if (SendReturnBusy_gv12b()) {
				HtRetry();
				break;
			}

			if (GR_Gv12b.data != 0x2345)
				HtAssert(0, 0);

			SendReturn_gv12b();
			break;
		case GV12_ENTRYc:
			if (SendReturnBusy_gv12c()) {
				HtRetry();
				break;
			}

			if (GR_Gv12c.data != 0x3456)
				HtAssert(0, 0);

			SendReturn_gv12c();
			break;
		case GV12_ENTRYd:
			if (SendReturnBusy_gv12d()) {
				HtRetry();
				break;
			}

			if (GR_Gv12d.data != 0x4567)
				HtAssert(0, 0);

			SendReturn_gv12d();
			break;
		default:
			assert(0);
		}
	}
}
