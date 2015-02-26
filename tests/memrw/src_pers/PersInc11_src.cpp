#include "Ht.h"
#include "PersInc11.h"

void
CPersInc11::PersInc11()
{
	if (PR_htValid) {
		//sc_uint<INC_WR_RSP_GRP_ID_W> wrRspGrpId = (sc_uint<INC_WR_RSP_GRP_ID_W>)((P_htId << 2) | (P_htId & 11));
		switch (PR_htInst) {
		case INC11_INIT:
		{
			P_loopIdx[0][1] = 0;

			P_wrGrpId = PR_htId ^ 0x5;

			//if (m_htIdPool.empty())
			// wait until all threads have started
			HtContinue(INC11_READ);
			//else
			//	HtContinue(INC11_INIT);
		}
		break;
		case INC11_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx[0][1]) << 3);

			Inc11Ptr_t arrayMemWrPtr = (PR_htId << 7) | P_loopIdx[0][1];

			// Issue read request to memory
			ReadMem_arrayMem11(memRdAddr, arrayMemWrPtr);

			bool bLast = P_loopIdx[0][1] == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx[0][1] = 0;
				ReadMemPause(INC11_WRITE);
			} else {
				P_loopIdx[0][1] += 1;
				HtContinue(INC11_READ);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx[0][1];
		}
		break;
		case INC11_WRITE:
		{
			//if (WriteMemBusy(rspGrpId)) {
			if (WriteMemBusy()) {
				//if (WriteMemBusy(wrRspGrpId)) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR1_arrayMem11.data + 1;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + ((P_loopBase + P_loopIdx[0][1]) << 3);

			// Issue write memory request
			//WriteMem(wrRspGrpId, memWrAddr, memWrData);
			WriteMem(PR_wrGrpId, memWrAddr, memWrData);

			bool bLast = P_loopIdx[0][1] == P_elemCnt - 1;
			if (bLast) {
				P_loopIdx[0][1] = 0;
				//WriteMemPause(wrRspGrpId, INC11_RTN);
				WriteMemPause(PR_wrGrpId, INC11_RTN);
				//HtPause();
			} else {
				P_loopIdx[0][1] += 1;
				HtContinue(INC11_WRITE);
			}

			// Set address for reading memory response data
			P_arrayMemRdPtr = (PR_htId << 7) | P_loopIdx[0][1];
		}
		break;
		case INC11_RTN:
		{
			if (SendReturnBusy_inc11()) {
				HtRetry();
				break;
			}

			SendReturn_inc11(P_elemCnt);

			break;
		}
		default:
			assert(0);
		}
	}
}
