#include "Ht.h"
#include "PersCtl.h"

void
CPersCtl::PersCtl()
{
	if (PR_htValid) {
#ifndef _HTV
		static const char *instructions[] = { "CTL_ENTRY", "CTL_RUN", "CTL_RTN" };
		if (0 || (PR_htInst == CTL_ENTRY))
			fprintf(stderr, "CTL: cmd=%s PR_cmd=%s SR_rqAddr=%llx @ %lld\n",
				instructions[(int)PR_htInst],
				(PR_cmd == CMD_LD) ? "LD" : "ST",
				(long long)SR_rqAddr,
				HT_CYCLE());
#endif

		switch (PR_htInst) {
		case CTL_ENTRY: {
			P_rqIdx = 0;
			P_rqCnt = 0;

			HtContinue(CTL_RUN);
		}
		break;
		case CTL_RUN: {
			if (ReadMemBusy() || WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Memory request
			MemAddr_t memAddr = SR_rqAddr + (P_rqIdx << 3);

			if (PR_cmd == CMD_LD)
				ReadMem_memRsp(memAddr);
			else
				WriteMem(memAddr, 0x600dbeef);

			P_rqCnt = P_rqCnt + 1;

			if ((P_rqIdx + 1) >= SR_arrayLen)
				P_rqIdx = 0;
			else
				P_rqIdx = P_rqIdx + 1;

			if (P_rqCnt >= P_numReqs)
				HtContinue(CTL_RTN);
			else
				HtContinue(CTL_RUN);
		}
		break;
		case CTL_RTN: {
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			SendReturn_htmain(P_rqCnt);
		}
		break;
		default:
			assert(0);
		}
	}
}
