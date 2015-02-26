#include "Ht.h"
#include "PersMemSub.h"

#ifndef _HTV
int entryMask = 0;
#endif

void CPersMemSub::PersMemSub()
{
	if (PR2_htValid) {
		switch (PR2_htInst) {
		case WS_READ1:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_array(PR2_rdAddr1, 0, 11, true);

			ReadMemPause(WS_WRITE1);
			break;
		}
		case WS_WRITE1:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_array(PR2_wrAddr1, 0, 11, true);

			WriteMemPause(WS_READ2);
			break;
		}
		case WS_READ2:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_array2(PR2_rdAddr2, 3, 7, true);

			ReadMemPause(WS_WRITE2);
			break;
		}
		case WS_WRITE2:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_array2(PR2_wrAddr2, 3, 7, true);

			WriteMemPause(WS_READ3);
			break;
		}
		case WS_READ3:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}
			ReadMem_privArray(PR2_rdAddr3, 2, 6, true);

			ReadMemPause(WS_WRITE3);
			break;
		}
		case WS_WRITE3:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}
			WriteMem_privArray(PR2_wrAddr3, 2, 6, true);

			WriteMemPause(WS_RETURN);
			break;
		}
		case WS_RETURN:
		{
			if (SendReturnBusy_MemSub()) {
				HtRetry();
				break;
			}

			SendReturn_MemSub();
			break;
		}
		default:
			assert(0);
		}
	}
}
