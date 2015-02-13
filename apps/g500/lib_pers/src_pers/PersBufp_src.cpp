#include "Ht.h"
#include "PersBufp.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersBufp::PersBufp()
{
	bool isXadj32 = SR_bfsCtl == 32;

	if (PR_htValid) {
		switch (PR_htInst) {
		case BUFP_XOFF0_LD: {
			BUSY_RETRY(ReadMemBusy());

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_xoffAddr + (2 * P_vertex << 3);

			// Issue read request to memory
			ReadMem_xoff0(memRdAddr, PR_htId);

			HtContinue(BUFP_XOFF1_LD);
		}
		break;
		case BUFP_XOFF1_LD: {
			BUSY_RETRY(ReadMemBusy());

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_xoffAddr + ((2 * P_vertex + 1) << 3);

			// Issue read request to memory
			ReadMem_xoff1(memRdAddr, PR_htId);

			ReadMemPause(BUFP_XOFF_CHK);
		}
		break;
		case BUFP_XOFF_CHK: {
			if ((GR_arrayMem_xoff1() - GR_arrayMem_xoff0()) <= 0) {
				P_vtxUpdated = 1;
				HtContinue(BUFP_RTN);
			} else {
				HtContinue(BUFP_XADJ_LD);
			}
			P_vso = GR_arrayMem_xoff0();
		}
		break;
		case BUFP_XADJ_LD: {
			if ((GR_arrayMem_xoff1() - PR_vso) <= 0) {
				P_vtxUpdated = 0;
				HtContinue(BUFP_RTN);
				break;
			}

			BUSY_RETRY(ReadMemBusy());

			// Calculate memory read address
			MemAddr_t off = (MemAddr_t)(P_vso << (isXadj32 ? 2 : 3));
			MemAddr_t memRdAddr = SR_xadjAddr + off;

			// Issue read request to memory
			ReadMem_xadj(memRdAddr, PR_htId);

			// increment vso for next iteration
			P_vso++;

			ReadMemPause(BUFP_BFS_LD);
		}
		break;
		case BUFP_BFS_LD: {
			BUSY_RETRY(ReadMemBusy());

			// Calculate memory read address--index = xadj/64
			uint32_t bmapIdx = (uint32_t)(GR_arrayMem_xadj() >> 6);
			BVW_t bmapBitIdx = (GR_arrayMem_xadj() & 0x3fLL);
			MemAddr_t memRdAddr = SR_bmapOldAddr + (bmapIdx << 3);

			// Issue read request to memory
			ReadMem_bmap(memRdAddr, PR_htId);

			// Save the bit index so we know which bit of rsp data to look at
			P_bitIdx = bmapBitIdx;

			ReadMemPause(BUFP_BFS_RSP);
		}
		break;
		case BUFP_BFS_RSP: {
			if (((GR_arrayMem_bmap() >> PR_bitIdx) & 1LL) == 0) {
				BUSY_RETRY(WriteMemBusy());

				// Calculate memory read address
				MemAddr_t memWrAddr = SR_bfsAddr + (P_vertex << 3);

				// Issue write request to memory
				WriteMem(memWrAddr, (uint64_t)GR_arrayMem_xadj());
				WriteMemPause(BUFP_RTN);

				// Update bmap, we're done with this vertex
				P_vtxUpdated = 1;
				break;
			}

			HtContinue(BUFP_XADJ_LD);
		}
		break;
		case BUFP_RTN: {
			BUSY_RETRY(SendReturnBusy_bufp());

			SendReturn_bufp(P_vtxUpdated, P_vertex % 64);
		}
		break;
		default:
			assert(0);
		}
	}
}
