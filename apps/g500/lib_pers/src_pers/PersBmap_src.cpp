#include "Ht.h"
#include "PersBmap.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

static ht_uint6 tzc(uint16_t v)
{
	if (v == 0) return 16;
	unsigned n = 1;
	if ((v & 0x000000ff) == 0) { n +=  8; v >>=  8; }
	if ((v & 0x0000000f) == 0) { n +=  4; v >>=  4; }
	if ((v & 0x00000003) == 0) { n +=  2; v >>=  2; }
	return n - (v & 1);
}

void
CPersBmap::PersBmap()
{
	// Set address for reading bmap memory
	P_bmapMemRdPtr = PR_htId;

	if (PR_htValid) {
		switch (PR_htInst) {
		case BMAP_LD: {
			BUSY_RETRY(ReadMemBusy());

			// Calculate memory read address
			MemAddr_t memRdAddr = SR_bmapOldAddr + (MemAddr_t)(P_bmapIdx << 3);

			ReadMem_bmapMem(memRdAddr, PR_htId);

			ReadMemPause(BMAP_ZERO);
		}
		break;
		case BMAP_ZERO: {
			P_bitCnt = 0;
			P_bmapUpdCnt = 0;
			P_mask = GR_bmapMem_data();

			if (P_mask == 0)
				HtContinue(BMAP_ST);
			else
				HtContinue(BMAP_PROCESS);
		}
		break;
		case BMAP_PROCESS: {
			BUSY_RETRY(SendCallBusy_bufp());

			uint32_t vertex = (uint32_t)(P_bmapIdx * 64) + P_bitCnt;

			if (P_mask & 1)
				SendCallFork_bufp(BMAP_UPD, vertex);

			// find next set bit
			BCW_t    rem  = (BCW_t)(63 - P_bitCnt);
			ht_uint6 skip = tzc((uint16_t)((P_mask>>1)<<1));

			if (skip > rem) {
				RecvReturnPause_bufp(BMAP_ST);
				break;
			}

			P_bitCnt = (BCW_t)(P_bitCnt + skip);
			P_mask   = P_mask >> skip;

			HtContinue(BMAP_PROCESS);
		}
		break;
		case BMAP_UPD: {
			uint64_t newBmap = GR_bmapMem_data() & ~((uint64_t)P_vtxUpdated << P_updBitIdx);
			if (P_vtxUpdated) {
				GW_bmapMem_data(PR_htId, newBmap);

				P_bmapUpdCnt++;
			}

			RecvReturnJoin_bufp();
		}
		break;
		case BMAP_ST: {
			BUSY_RETRY(WriteMemBusy());

			// Calculate memory read address
			MemAddr_t memWrAddr = SR_bmapNewAddr + (MemAddr_t)(P_bmapIdx << 3);

			// Issue write memory request
			WriteMem(memWrAddr, GR_bmapMem_data());

			WriteMemPause(BMAP_RTN);
		}
		break;
		case BMAP_RTN: {
			BUSY_RETRY(SendReturnBusy_bmap());

			SendReturn_bmap(P_bmapUpdCnt);
		}
		break;
		default:
			assert(0);
		}
	}
}
