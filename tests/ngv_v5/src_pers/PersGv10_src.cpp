#include "Ht.h"
#include "PersGv10.h"

void CPersGv10::PersGv10()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV10_ENTRYa:
			if (SendReturnBusy_gv10a()) {
				HtRetry();
				break;
			}

			if (GR_Gv10a.data != 0x1234)
				HtAssert(0, 0);

			SendReturn_gv10a();
			break;
		case GV10_ENTRYb:
			if (SendReturnBusy_gv10b()) {
				HtRetry();
				break;
			}

			if (GR_Gv10b.data != 0x2345)
				HtAssert(0, 0);

			SendReturn_gv10b();
			break;
		case GV10_ENTRYc:
			if (SendReturnBusy_gv10c()) {
				HtRetry();
				break;
			}

			if (GR_Gv10c.data != 0x3456)
				HtAssert(0, 0);

			SendReturn_gv10c();
			break;
		case GV10_ENTRYd:
			if (SendReturnBusy_gv10d()) {
				HtRetry();
				break;
			}

			if (GR_Gv10d.data != 0x4567)
				HtAssert(0, 0);

			SendReturn_gv10d();
			break;
		case GV10_ENTRYeIw:
			if (SendReturnBusy_gv10eIw()) {
				HtRetry();
				break;
			}

			if (GR_Gv10e.data != 0x5678)
				HtAssert(0, 0);

			SendReturn_gv10eIw();
			break;
		case GV10_ENTRYeMw:
			if (SendReturnBusy_gv10eMw()) {
				HtRetry();
				break;
			}

			if (GR_Gv10e.data != 0x6789)
				HtAssert(0, 0);

			SendReturn_gv10eMw();
			break;
		case GV10_ENTRYfIw:
			if (SendReturnBusy_gv10fIw()) {
				HtRetry();
				break;
			}

			if (GR_Gv10f.data != 0x789a)
				HtAssert(0, 0);

			SendReturn_gv10fIw();
			break;
		case GV10_ENTRYfMw:
			if (SendReturnBusy_gv10fMw()) {
				HtRetry();
				break;
			}

			if (GR_Gv10f.data != 0x89ab)
				HtAssert(0, 0);

			SendReturn_gv10fMw();
			break;
		default:
			assert(0);
		}
	}
}
