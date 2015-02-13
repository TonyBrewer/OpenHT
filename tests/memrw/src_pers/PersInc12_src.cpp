#include "Ht.h"
#include "PersInc12.h"

void
CPersInc12::PersInc12()
{
	if (PR_htValid) {
		//sc_uint<INC_WR_RSP_GRP_ID_W> wrRspGrpId = (sc_uint<INC_WR_RSP_GRP_ID_W>)((P_htId << 2) | (P_htId & 11));
		switch (PR_htInst) {
		case INC12_INIT:
		{
			P_loopIdx[0][1] = 0;

			P_wrGrpId = PR_htId ^ 0xa;

			//if (m_htIdPool.empty())
			// wait until all threads have started
			HtContinue(INC12_READ);
			//else
			//	HtContinue(INC12_INIT);
		}
		break;
		case INC12_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx[0][1]) << 3);

			Inc12Ptr_t arrayMemWrPtr = (PR_htId << 7) | P_loopIdx[0][1];

			// Issue read request to memory
			ReadMem_arrayMem12(memRdAddr, arrayMemWrPtr);

			bool bLast = P_loopIdx[0][1] == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx[0][1] = 0;
				ReadMemPause(INC12_WRITE);
			} else {
				P_loopIdx[0][1] += 1;
				HtContinue(INC12_READ);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx[0][1];
		}
		break;
		case INC12_WRITE:
		{
			//if (WriteMemBusy(rspGrpId)) {
			if (WriteMemBusy()) {
				//if (WriteMemBusy(wrRspGrpId)) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR1_arrayMem12_data() + 1;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx[0][1]) << 3);

			// Issue write memory request
			//WriteMem(wrRspGrpId, memWrAddr, memWrData);
			WriteMem(memWrAddr, memWrData);

			bool bLast = P_loopIdx[0][1] == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx[0][1] = 0;
				//WriteMemPause(wrRspGrpId, INC12_RTN);
				WriteMemPause(INC12_RTN);
				//HtPause();
			} else {
				P_loopIdx[0][1] += 1;
				HtContinue(INC12_WRITE);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx[0][1];
		}
		break;
		case INC12_RTN:
		{
			if (SendReturnBusy_inc12()) {
				HtRetry();
				break;
			}

			SendReturn_inc12(P_elemCnt);

			break;
		}
		default:
			assert(0);
		}
	}
}
