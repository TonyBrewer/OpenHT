#include "Ht.h"
#include "PersCxr1.h"

void CPersCxr1::PersCxr1()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CXR1_0:
		{
			if (SendCallBusy_cxr2e1() || SendHostMsgBusy()) {
				HtRetry();
				break;
			}

			SendHostMsg(OHM_TEST_MSG, 0x123456789abcdeULL);

			P_trail = (uint64_t)(P_trail << 4) | 0x8;

			SendCall_cxr2e1(CXR1_1, 1, P_trail);
		}
		break;
		case CXR1_1:
		{
			if (SendCallBusy_cxr2e2()) {
				HtRetry();
				break;
			}

			// Uncomment to test HwAssert
			//  Also need to override common.h file by setting #ifdef _HTV to #ifndef near HtAssert macro definition
			//HtAssert(false, 0xabcdef);

			P_trail = (uint64_t)(P_trail << 4) | 0x9;

			SendCall_cxr2e2(CXR1_2, 2, P_trail, 0);
		}
		break;
		case CXR1_2:
		{
			if (SendTransferBusy_cxr1b()) {
				HtRetry();
				break;
			}

			P_trail = (uint64_t)(P_trail << 4) | 0xa;

			SendTransfer_cxr1b(P_trail);

			break;
		}
		default:
			assert(0);
		}
	}
}
