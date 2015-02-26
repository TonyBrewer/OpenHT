#include "Ht.h"
#include "PersIncRd.h"

struct CTest {
	uint32_t	a : 31;
	uint32_t	b : 3;
	uint64_t	c : 48;
	uint64_t	d : 64;
};

void CPersIncRd::PersIncRd()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case INCRD_INIT:
		{
			P_loopCnt = 0;

			HtContinue(INCRD_READ);
		}
		break;
		case INCRD_READ:
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
				ReadMem_arrayMem(memRdAddr, PR_htId);

				ReadMemPause(INCRD_WRITE);
			}
		}
		break;
		case INCRD_WRITE:
		{
			if (SendCallBusy_incWr()) {
				HtRetry();
				break;
			}

			SendCall_incWr(INCRD_READ, P_loopCnt, PR_htId);

			// Increment loop count
			P_loopCnt = P_loopCnt + 1;
		}
		break;
		default:
			assert(0);
		}
	}
}

void CPersIncRd::ReadMemResp_arrayMem(sc_uint<INCRD_RD_GRP_ID_W> rdRspGrpId, sc_uint<INCRD_HTID_W> rdRspInfo, sc_uint<64> rdRspData)
{
	assert(rdRspGrpId == rdRspInfo);

	HtId_t wrAddr = rdRspGrpId | rdRspInfo;

	GW_arrayMem.write_addr(wrAddr);
	GW_arrayMem.data = rdRspData + 1;
}
