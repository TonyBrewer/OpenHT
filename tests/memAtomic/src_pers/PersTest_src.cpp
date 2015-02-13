#include "Ht.h"
#include "PersTest.h"

void CPersTest::PersTest()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case RD_CP0:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_testPassed = true;

			// Make request for atomic set bit 63
			SetBit63Mem_sharedData_coproc(SR_coprocAddr + 8);

			ReadMemPause(RD_CP0V);
		}
		break;
		case RD_CP0V:
		{
			HtAssert(S_sharedData == 0x01234567deadbef0ULL, 0);
			if (S_sharedData != 0x01234567deadbef0ULL)
				P_testPassed = false;

			HtContinue(RD_CP1);
		}
		break;
		case RD_CP1:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Make request for atomic set bit 63
			SetBit63Mem_sharedData_coproc(SR_coprocAddr + 8);

			ReadMemPause(RD_CP1V);
		}
		break;
		case RD_CP1V:
		{
			HtAssert(S_sharedData == 0x8000000000000000ULL, 0);
			if (S_sharedData != 0x8000000000000000ULL)
				P_testPassed = false;

			HtContinue(RD_CP2);
		}
		break;
		case RD_CP2:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMem(SR_coprocAddr + 8, 0x01234567deadbef0ULL);

			WriteMemPause(RD_CP3);
		}
		break;
		case RD_CP3:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Make request for atomic set bit 63
			SetBit63Mem_sharedData_coproc(SR_coprocAddr + 8);

			ReadMemPause(RD_CP3V);
		}
		break;
		case RD_CP3V:
		{
			HtAssert(S_sharedData == 0x01234567deadbef0ULL, 0);
			if (S_sharedData != 0x01234567deadbef0ULL)
				P_testPassed = false;

			HtContinue(RD_CP4);
		}
		break;
		case RD_CP4:
		{
			// test completed
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			// Return to host interface
			SendReturn_htmain(P_testPassed);
		}
		break;
		default:
			assert(0);
		}
	}
}
