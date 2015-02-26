#include "Ht.h"
#include "PersInc8.h"

void
CPersInc8::PersInc8()
{
	if (PR_htValid) {
		//sc_uint<INC_WR_RSP_GRP_ID_W> wrRspGrpId = (sc_uint<INC_WR_RSP_GRP_ID_W>)((P_htId << 2) | (P_htId & 3));
		switch (PR_htInst) {
		case INC8_INIT:
		{
			P_loopIdx = 0;

			P_rdGrpId = PR_htId ^ 0xa;

			if (m_htIdPool.empty())
				// wait until all threads have started
				HtContinue(INC8_READ);
			else
				HtContinue(INC8_INIT);
		}
		break;
		case INC8_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx) << 3);

			Inc8Ptr_t arrayMemWrPtr = (PR_htId << 7) | P_loopIdx;

			// Issue read request to memory
			ReadMem_arrayMem8(PR_rdGrpId, memRdAddr, arrayMemWrPtr);

			bool bLast = P_loopIdx == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx = 0;
				ReadMemPause(PR_rdGrpId, INC8_WRITE);
			} else {
				P_loopIdx += 1;
				HtContinue(INC8_READ);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx;
		}
		break;
		case INC8_WRITE:
		{
			//if (WriteMemBusy(rspGrpId)) {
			if (WriteMemBusy()) {
				//if (WriteMemBusy(wrRspGrpId)) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR_arrayMem8.data + 1;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx) << 3);

			// Issue write memory request
			//WriteMem(wrRspGrpId, memWrAddr, memWrData);
			WriteMem(memWrAddr, memWrData);

			bool bLast = P_loopIdx == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx = 0;
				//WriteMemPause(wrRspGrpId, INC8_RTN);
				WriteMemPause(INC8_RTN);
				//HtPause();
			} else {
				P_loopIdx += 1;
				HtContinue(INC8_WRITE);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx;
		}
		break;
		case INC8_RTN:
		{
			if (SendReturnBusy_inc8()) {
				HtRetry();
				break;
			}

			SendReturn_inc8(P_elemCnt);

			break;
		}
		default:
			assert(0);
		}
	}
}
