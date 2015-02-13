#include "Ht.h"
#include "PersCtl.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

enum CommandType { INIT, SCATTER, BFS };

void
CPersCtl::PersCtl()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CTL_ENTRY: {
			if (P_command == INIT)
				HtContinue(CTL_INIT);
			else if (P_command == SCATTER)
				HtContinue(CTL_SCATTER);
			else if (P_command == BFS)
				HtContinue(CTL_BMAP);
			else
				assert(0);
		}
		break;
		case CTL_INIT: {
			BUSY_RETRY(SendCallBusy_init());

			SendCallFork_init(CTL_INIT_JOIN, P_bmapIdx, P_bmapStride);
			RecvReturnPause_init(CTL_RTN);
		}
		break;
		case CTL_SCATTER: {
			BUSY_RETRY(SendCallBusy_scatter());

			if (P_bmapIdx < SR_bfsSize) {
				SendCallFork_scatter(CTL_SCATTER_JOIN, P_bmapIdx);
				HtContinue(CTL_SCATTER);
				P_bmapIdx += P_bmapStride;
			} else {
				// done loading bitmaps
				RecvReturnPause_scatter(CTL_RTN);
			}
		}
		break;
		case CTL_BMAP: {
			BUSY_RETRY(SendCallBusy_bmap());

			if (P_bmapIdx * 64 < SR_bfsSize) {
				SendCallFork_bmap(CTL_BMAP_JOIN, P_bmapIdx);
				HtContinue(CTL_BMAP);
				P_bmapIdx += P_bmapStride;
			} else {
				// done loading bitmaps
				RecvReturnPause_bmap(CTL_RTN);
			}
		}
		break;
		case CTL_INIT_JOIN: {
			RecvReturnJoin_init();
		}
		break;
		case CTL_SCATTER_JOIN: {
			RecvReturnJoin_scatter();
		}
		break;
		case CTL_BMAP_JOIN: {
			P_updCnt += P_bmapUpdCnt;
			RecvReturnJoin_bmap();
		}
		break;
		case CTL_RTN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain(P_updCnt);
		}
		break;
		default:
			assert(0);
		}
	}
}
