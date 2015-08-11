#include "Ht.h"
#include "PersRd.h"

void CPersRd::PersRd() {
	if (PR_htValid) {
		switch (PR_htInst) {
		case RD_ENTRY: {
			P_bStandalonePause = false;
			P_reqIdx = 0;
			P_loopIdx = 0;
			P_memOffset = PR_threadId;

			HtBarrier(RD_REQ, 8);
			break;
		}
		case RD_REQ: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			ht_uint5 elemCnt = ((PR_threadId + PR_reqIdx) & 7) + 1;
			if (PR_memOffset + elemCnt + PR_threadId >= 256)
				P_memOffset = PR_threadId;

			ht_uint8 info = PR_threadId + P_memOffset;
			ht_uint48 addr = PR_memAddr + P_memOffset * 8;

			ReadMem_rdFunc(addr, info, elemCnt);

			P_memOffset += elemCnt;

			if (PR_reqIdx == 7) {
				if (PR_bStandalonePause) {
					HtContinue(RD_PAUSE);
				} else {
					ReadMemPause(RD_LOOP);
				}

				P_bStandalonePause ^= 1;
				P_reqIdx = 0;
			} else {
				P_reqIdx += 1;
				HtContinue(RD_REQ);
			}
			break;
		}
		case RD_PAUSE: {
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			ReadMemPause(RD_LOOP);
			break;
		}
		case RD_LOOP: {
			if (PR_loopIdx == 255) {
				HtContinue(RD_RTN);
			} else {
				HtContinue(RD_REQ);
			}

			P_loopIdx += 1;
			break;
		}
		case RD_RTN: {
			if (SendReturnBusy_rdPause()) {
				HtRetry();
				break;
			}
			SendReturn_rdPause();
			break;
		}
		default:
			assert(0);
		}
	}
}

void CPersRd::ReadMemResp_rdFunc(ht_uint4 rdRspIdx, sc_uint<RD_RD_GRP_ID_W> ht_noload rdRspGrpId, ht_uint8 rdRspInfo, uint64_t rdRspData)
{
	if (rdRspData != 123 + rdRspIdx + rdRspInfo)
		HtAssert(0, 0);
}