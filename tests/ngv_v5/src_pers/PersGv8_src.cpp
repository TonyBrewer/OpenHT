#include "Ht.h"
#include "PersGv8.h"

void CPersGv8::PersGv8()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV8_ENTRYa:
			if (SendReturnBusy_gv8a()) {
				HtRetry();
				break;
			}

			if (GR_Gv8a.data != 0x1234)
				HtAssert(0, 0);

			SendReturn_gv8a();
			break;
		case GV8_ENTRYb:
			if (SendReturnBusy_gv8b()) {
				HtRetry();
				break;
			}

			if (GR_Gv8b.data != 0x2345)
				HtAssert(0, 0);

			SendReturn_gv8b();
			break;
		case GV8_ENTRYc:
			if (SendReturnBusy_gv8c()) {
				HtRetry();
				break;
			}

			if (GR_Gv8c.data != 0x3456)
				HtAssert(0, 0);

			SendReturn_gv8c();
			break;
		case GV8_ENTRYd:
			if (SendReturnBusy_gv8d()) {
				HtRetry();
				break;
			}

			if (GR_Gv8d.data != 0x4567)
				HtAssert(0, 0);

			SendReturn_gv8d();
			break;
		case GV8_ENTRYeIw:
			if (SendReturnBusy_gv8eIw()) {
				HtRetry();
				break;
			}

			if (GR_Gv8e.data != 0x5678)
				HtAssert(0, 0);

			SendReturn_gv8eIw();
			break;
		case GV8_ENTRYeMw:
			if (SendReturnBusy_gv8eMw()) {
				HtRetry();
				break;
			}

			if (GR_Gv8e.data != 0x6789)
				HtAssert(0, 0);

			SendReturn_gv8eMw();
			break;
		case GV8_ENTRYfIw:
			if (SendReturnBusy_gv8fIw()) {
				HtRetry();
				break;
			}

			if (GR_Gv8f.data != 0x789a)
				HtAssert(0, 0);

			SendReturn_gv8fIw();
			break;
		case GV8_ENTRYfMw:
			if (SendReturnBusy_gv8fMw()) {
				HtRetry();
				break;
			}

			if (GR_Gv8f.data != 0x89ab)
				HtAssert(0, 0);

			SendReturn_gv8fMw();
			break;
		default:
			assert(0);
		}
	}
}
