#include "Ht.h"
#include "PersWr.h"

void CPersWr::PersWr() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case WR_ENTRY: {
			P_bStandalonePause = false;
			P_reqIdx = 0;
			P_loopIdx = 0;
			P_memOffset = 0;

			HtBarrier(WR_DATA, 8);
			break;
		}
		case WR_DATA: {
			S_data.write_addr(PR_loopIdx);
			S_data.write_mem(PR_loopIdx & 0x7f);

			if (PR_loopIdx == 0xff) {
				P_loopIdx = 0;
				HtContinue(WR_REQ);
			} else {
				P_loopIdx += 1;
				HtContinue(WR_DATA);
			}
			break;
		}
		case WR_REQ: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			ht_uint9 elemCnt = ((PR_threadId + PR_reqIdx) & 7) + 1;
			ht_uint48 addr = PR_memAddr + P_memOffset * 8;
			ht_uint8 dataIdx = (addr >> 3) & 0x7f;

			WriteMem_data(addr, dataIdx, elemCnt);

			P_memOffset += elemCnt;

			if (PR_reqIdx == 7) {
				if (PR_bStandalonePause) {
					HtContinue(WR_PAUSE);
				} else {
					WriteMemPause(WR_LOOP);
				}

				P_bStandalonePause ^= 1;
				P_reqIdx = 0;
			} else {
				P_reqIdx += 1;
				HtContinue(WR_REQ);
			}
			break;
		}
		case WR_PAUSE: {
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMemPause(WR_LOOP);
			break;
		}
		case WR_LOOP: {
			if (PR_loopIdx == 255) {
				HtContinue(WR_RTN);
			} else {
				HtContinue(WR_REQ);
			}

			P_loopIdx += 1;
			break;
		}
		case WR_RTN: {
			if (SendReturnBusy_wrPause()) {
				HtRetry();
				break;
			}
			SendReturn_wrPause();
			break;
		}
		default:
			assert(0);
		}
	}
}
