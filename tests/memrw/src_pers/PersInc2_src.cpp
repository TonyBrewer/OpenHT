#include "Ht.h"
#include "PersInc2.h"

void
CPersInc2::PersInc2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case INC2_INIT:
		{
			P_loopIdx = 0;

			//if (m_htIdPool.empty()) {
			// wait until all threads have started
			HtContinue(INC2_READ);
			//} else
			//	HtContinue(INC2_INIT);
		}
		break;
		case INC2_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx) << 3);

			Inc2Ptr_t arrayMemWrPtr = (PR_htId << 7) | P_loopIdx;

			// Issue read request to memory
			ReadMem_arrayMem2(memRdAddr, arrayMemWrPtr);

			bool bLast = P_loopIdx == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx = 0;
				ReadMemPause(INC2_WRITE);
			} else {
				P_loopIdx += 1;
				HtContinue(INC2_READ);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx;
		}
		break;
		case INC2_WRITE:
		{
			//if (WriteMemBusy(rspGrpId)) {
			if (WriteMemBusy()) {
				//if (WriteMemBusy(wrRspGrpId)) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR_arrayMem2_data() + 1;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx) << 3);

			// Issue write memory request
			//WriteMem(wrRspGrpId, memWrAddr, memWrData);
			WriteMem(memWrAddr, memWrData);

			bool bLast = P_loopIdx == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx = 0;
				//WriteMemPause(wrRspGrpId, INC_RTN);
				WriteMemPause(INC2_RTN);
				//HtPause();
			} else {
				P_loopIdx += 1;
				HtContinue(INC2_WRITE);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx;
		}
		break;
		case INC2_RTN:
		{
			if (SendReturnBusy_inc2()) {
				HtRetry();
				break;
			}

			SendReturn_inc2(P_elemCnt);

			break;
		}
		default:
			assert(0);
		}
	}
}
