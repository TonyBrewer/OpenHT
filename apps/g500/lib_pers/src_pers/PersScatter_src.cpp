#include "Ht.h"
#include "PersScatter.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

//#define BFS_PACKED_X(k) (bfs_packed[2*k])
//#define VLIST_X(k) (bfs_packed[2*k+1])
//
//  for (k=0; k<(*k2); k++) {
//    bfs_tree[VLIST_X(k)] = BFS_PACKED_X(k);
//  }
void
CPersScatter::PersScatter()
{
	S_addrMem.read_addr(PR_htId);
	S_valMem.read_addr(PR_htId);
	if (PR_htValid) {
		switch (PR_htInst) {
		case SCATTER_LD1: {
			BUSY_RETRY(ReadMemBusy());

			// Memory read request
			MemAddr_t memRdAddr = SR_bfsPackedAddr + ((2 * P_k) << 3);
			ReadMem_valMem(memRdAddr, PR_htId);
			HtContinue(SCATTER_LD2);
		}
		break;
		case SCATTER_LD2: {
			BUSY_RETRY(ReadMemBusy());

			// Memory read request
			MemAddr_t memRdAddr = SR_bfsPackedAddr + ((2 * P_k + 1) << 3);
			ReadMem_addrMem(memRdAddr, PR_htId);
			ReadMemPause(SCATTER_ST);
		}
		break;
		case SCATTER_ST: {
			BUSY_RETRY(WriteMemBusy());

			// Memory write request
			MemAddr_t memWrAddr = (MemAddr_t)(SR_bfsAddr + (S_addrMem.read_mem() << 3));
			WriteMem(memWrAddr, S_valMem.read_mem());
			WriteMemPause(SCATTER_RTN);
		}
		break;
		case SCATTER_RTN: {
			BUSY_RETRY(SendReturnBusy_scatter());

			SendReturn_scatter();
		}
		break;
		default:
			assert(0);
		}
	}
}
