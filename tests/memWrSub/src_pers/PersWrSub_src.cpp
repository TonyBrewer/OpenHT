#include "Ht.h"
#include "PersWrSub.h"

void CPersWrSub::PersWrSub()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case WS_WRITE:
			{
				if (WriteMemBusy()) {
					HtRetry();
					break;
				}

				switch (P_wrSize) {
				case MEM_REQ_U8:
					WriteMem_uint8(P_wrAddr, (uint8_t)P_wrData);
					break;
				case MEM_REQ_U16:
					WriteMem_uint16(P_wrAddr, (uint16_t)P_wrData);
					break;
				case MEM_REQ_U32:
					WriteMem_uint32(P_wrAddr, (uint32_t)P_wrData);
					break;
				case MEM_REQ_U64:
					WriteMem_uint64(P_wrAddr, (uint64_t)P_wrData);
					break;
				default:
					assert(0);
				}

				WriteMemPause(WS_RETURN);
			}
			break;
		case WS_RETURN:
			{
				if (SendReturnBusy_WrSub()) {
					HtRetry();
					break;
				}

				SendReturn_WrSub(true);
			}
			break;
		default:
			assert(0);
		}
	}
}
