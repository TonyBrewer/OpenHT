#include "Ht.h"
#include "PersRdSub.h"

void CPersRdSub::PersRdSub()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case WS_READ:
			{
				if (ReadMemBusy()) {
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
