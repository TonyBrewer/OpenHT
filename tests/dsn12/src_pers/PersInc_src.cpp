#include "Ht.h"
#include "PersInc.h"

void CPersInc::PersInc()
{
#if INC_RSM_CNT == 1
	if (GR_htReset) {
		S_rsmRdy = 0;
	} else if (SR_rsmRdy) {
		S_rsmRdy = 0;
#if INC_HTID_W == 0
		HtResume(0);
#else
		HtResume(SR_rsmHtId);
#endif
	}
#endif

	if (PR_htValid) {
		switch (PR_htInst) {
		case INC_INIT:
		{
			P_loopCnt = 0;

#if INC_RSM_CNT == 1
			S_rsmHtId = PR_htId;
			S_rsmRdy = true;

			HtPause(INC_READ);
#else
			HtContinue(INC_READ);
#endif
		}
		break;
		case INC_READ:
		{
			if (ReadMemBusy() || SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			// Check if end of loop
			if (P_loopCnt == P_elemCnt) {
				// Return to host interface
				SendReturn_htmain(P_loopCnt);
			} else {
				// Calculate memory read address
				MemAddr_t memRdAddr = SR_arrayAddr + (P_loopCnt << 3);

				// Issue read request to memory
#if INC_RSP_GRP_W == 0
#	if INC_HTID_W == 0
				ReadMem_arrayMem(memRdAddr);
#	else
				ReadMem_arrayMem(memRdAddr, PR_htId);
#	endif
#else
#	if INC_HTID_W == 0
				ReadMem_arrayMem(P_rspGrpVar, memRdAddr);
#	else
				ReadMem_arrayMem(P_rspGrpVar, memRdAddr, PR_htId);
#	endif
#endif
				// Set address for reading memory response data
				P_arrayMemRdPtr = PR_htId;

#if INC_RD_POLL==1
				HtContinue(INC_READ_POLL);
#else
#	if INC_RSP_GRP_W == 0
				ReadMemPause(INC_WRITE);
#	else
				ReadMemPause(PR_rspGrpVar, INC_WRITE);
#	endif
#endif
			}
			break;
		}
#if INC_RD_POLL==1
		case INC_READ_POLL:
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

#	if INC_RSP_GRP_W == 0
			ReadMemPoll(INC_WRITE);
#	else
			ReadMemPoll(PR_rspGrpVar, INC_WRITE);
#	endif
			break;
#endif
		case INC_WRITE:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Increment memory data
#if INC_HTID_W == 0
			uint64_t memWrData = SR_arrayMem + 1;
#else
			SR_arrayMem.read_addr(PR_htId);
			uint64_t memWrData = SR_arrayMem.read_mem() + 1;
#endif
			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + (P_loopCnt << 3);

			// Issue write memory request
			WriteMem(memWrAddr, memWrData);

			// Increment loop count
			P_loopCnt = P_loopCnt + 1;

#if INC_WR_POLL==1
			HtContinue(INC_WRITE_POLL);
#else
			WriteMemPause(INC_READ);
#endif
		}
		break;
#if INC_WR_POLL==1
		case INC_WRITE_POLL:
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			WriteMemPoll(INC_READ);
			break;
#endif
		default:
			assert(0);
		}
	}
}

#if INC_HTID_W == 0 && INC_RSP_GRP_W == 0
void CPersInc::ReadMemResp_arrayMem(sc_uint<64> rdRspData)
{
	S_arrayMem = rdRspData;
}
#elif INC_HTID_W == 0 && INC_RSP_GRP_W != 0
void CPersInc::ReadMemResp_arrayMem(sc_uint<INC_RD_GRP_ID_W> ht_noload rdRspGrpId, sc_uint<64> rdRspData)
{
	S_arrayMem = rdRspData;
}
#else
void CPersInc::ReadMemResp_arrayMem(sc_uint<INC_RD_GRP_ID_W> rdRspGrpId, sc_uint<INC_HTID_W> rdRspInfo, sc_uint<64> rdRspData)
{
	assert(rdRspGrpId == rdRspInfo);

	HtId_t wrAddr = (rdRspGrpId | rdRspInfo) & ((1u << INC_HTID_W)-1);

	S_arrayMem.write_addr(wrAddr);
	S_arrayMem.write_mem(rdRspData);
}
#endif
