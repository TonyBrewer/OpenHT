#include "Ht.h"
#include "PersGupsCore.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

void
CPersGupsCore::PersGupsCore()
{
    MemAddr_t rndIdx = SR_base + ((PR_ran & (SR_tblSize - 1)) << 3); // QW index
    if (PR_htValid) {
        switch (PR_htInst) {
            case PGC_ENTRY: {
                P_ran = (uint64_t)(PR_ran << 1) ^ ((int64_t) PR_ran < 0 ? POLY : 0);
		HtContinue(PGC_READ);
		break;
            }
            case PGC_READ: {
                BUSY_RETRY(ReadMemBusy());
                ReadMem_intData(rndIdx);
                ReadMemPause(PGC_WRITE);
		break;
            }
            case PGC_WRITE: {
                BUSY_RETRY(WriteMemBusy());
                WriteMem(rndIdx, PR_intData^PR_ran, /*orderChk=*/ false);
                P_ran = (uint64_t)(PR_ran << 1) ^ ((int64_t) PR_ran < 0 ? POLY : 0);
                P_vecIdx += 1;
                if (P_vecIdx < SR_vecLen) {
                    HtContinue(PGC_READ);
                } else
                    HtContinue(PGC_RTN);
		break;
            }
            case PGC_RTN: {
                BUSY_RETRY(SendReturnBusy_GupsCore());
                SendReturn_GupsCore();
		break;
            }
            default:
                assert(0);
        }
    }
}
