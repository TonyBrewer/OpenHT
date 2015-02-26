#include "Ht.h"
#include "PersRdSub.h"

#ifndef _HTV
int entryMask = 0;
#endif

void CPersRdSub::PersRdSub()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case WS_READ:
			{
#ifndef _HTV
				entryMask |= 1 << PR_htId;
#endif
				if (
#ifndef _HTV
					entryMask != ((1ull << (1 << WS_HTID_W))-1) ||
#endif

					ReadMemBusy()) {
					HtRetry();
					break;
				}

				switch (P_rdType) {
				case 0:
					ReadMem_uint8(P_rdAddr, PR_htId);
					break;
				case 1:
					ReadMem_uint16(P_rdAddr, PR_htId);
					break;
				case 2:
					ReadMem_uint32(P_rdAddr, PR_htId);
					break;
				case 3:
					ReadMem_uint64(P_rdAddr, PR_htId);
					break;
				case 4:
					ReadMem_int8(P_rdAddr, PR_htId);
					break;
				case 5:
					ReadMem_int16(P_rdAddr, PR_htId);
					break;
				case 6:
					ReadMem_int32(P_rdAddr, PR_htId);
					break;
				case 7:
					ReadMem_int64(P_rdAddr, PR_htId);
					break;
				default:
					assert(0);
				}

				ReadMemPause(WS_RETURN);
			}
			break;
		case WS_RETURN:
			{
				if (SendReturnBusy_RdSub()) {
					HtRetry();
					break;
				}

				SendReturn_RdSub(S_actual[PR_htId], P_expected);
			}
			break;
		default:
			assert(0);
		}
	}
}
