#include "Ht.h"
#include "PersInc10.h"

void
CPersInc10::PersInc10()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case INC10_INIT:
		{
			P_loopIdx = 0;

			P_wrGrpId = PR_htId ^ 0x2;

			if (m_htIdPool.empty())
				// wait until all threads have started
				HtContinue(INC10_READ);
			else
				HtContinue(INC10_INIT);
		}
		break;
		case INC10_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx) << 3);

			Inc10Ptr_t arrayMemWrPtr = (PR_htId << 7) | P_loopIdx;

			// Issue read request to memory
			ReadMem_arrayMem10(memRdAddr, arrayMemWrPtr);

			bool bLast = P_loopIdx == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx = 0;
				ReadMemPause(INC10_WRITE);
			} else {
				P_loopIdx += 1;
				HtContinue(INC10_READ);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx;
		}
		break;
		case INC10_WRITE:
		{
			//if (WriteMemBusy(rspGrpId)) {
			if (WriteMemBusy()) {
				//if (WriteMemBusy(wrRspGrpId)) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR_arrayMem10_data() + 1;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx) << 3);

			// Issue write memory request
			//WriteMem(wrRspGrpId, memWrAddr, memWrData);
			WriteMem(memWrAddr, memWrData);

			bool bLast = P_loopIdx == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx = 0;
				//WriteMemPause(wrRspGrpId, INC_RTN);
				WriteMemPause(INC10_RTN);
				//HtPause();
			} else {
				P_loopIdx += 1;
				HtContinue(INC10_WRITE);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx;
		}
		break;
		case INC10_RTN:
		{
			if (SendReturnBusy_inc10()) {
				HtRetry();
				break;
			}

			SendReturn_inc10(P_elemCnt);

			break;
		}
		default:
			assert(0);
		}
	}
}
