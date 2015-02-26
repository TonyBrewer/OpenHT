#include "Ht.h"
#include "PersTst.h"

// Test global variable references
//    Test 1R/1W    (same module)
// 1. tst: MifWr+InstRd                     (tst)
// 2. tst: InstWr+InstRd                    (tst)
// 3. tst: MifWr+InstRd                     (tst3)
// 4. tst: InstWr+InstRd                    (tst3)

//    Test 1R/1W    (two modules)
// 5. tst: MifWr            tst2: InstRd    (tst)
// 6. tst: InstWr           tst2: InstRd    (tst)
// 7. tst: MifWr            tst2: InstRd    (tst2)
// 8. tst: InstWr           tst2: InstRd    (tst2)
// 9. tst: MifWr            tst2: InstRd    (tst3)
// 10. tst: InstWr          tst2: InstRd    (tst3)

//    Test 1R/2W    (same module)
// 11. tst: MifWr+InstWr+InstRd             (tst)
// 12. tst: MifWr+InstWr+InstRd             (tst3)

//    Test 1R/2W    (two modules)
// 1. tst: InstWr+InstRd    tst2: MifWr     (tst)
// 1. tst: MifWr+InstRd     tst2: InstWr    (tst)
// 1. tst: InstWr+InstRd    tst2: InstWr    (tst)
// 1. tst: MifWr+InstRd     tst2: MifWr     (tst)
// 1. tst: InstWr+InstRd    tst2: MifWr     (tst2)
// 1. tst: MifWr+InstRd     tst2: InstWr    (tst2)
// 1. tst: InstWr+InstRd    tst2: InstWr    (tst2)
// 1. tst: MifWr+InstRd     tst2: MifWr     (tst2)
// 1. tst: InstWr+InstRd    tst2: MifWr     (tst3)
// 1. tst: MifWr+InstRd     tst2: InstWr    (tst3)
// 1. tst: InstWr+InstRd    tst2: InstWr    (tst3)
// 1. tst: MifWr+InstRd     tst2: MifWr     (tst3)

//    Test 1R/2W    (three modules)
// 1. tst: InstRd   tst2: InstWr    tst3: InstWr     (tst)
// 1. tst: InstRd   tst2: MifWr     tst3: InstWr     (tst)
// 1. tst: InstRd   tst2: InstWr    tst3: MifWr      (tst)
// 1. tst: InstRd   tst2: MifWr     tst3: MifWr      (tst)
// 1. tst: InstRd   tst2: InstWr    tst3: InstWr     (tst2)
// 1. tst: InstRd   tst2: MifWr     tst3: InstWr     (tst2)
// 1. tst: InstRd   tst2: InstWr    tst3: MifWr      (tst2)
// 1. tst: InstRd   tst2: MifWr     tst3: MifWr      (tst2)
// 1. tst: InstRd   tst2: InstWr    tst3: InstWr     (tst3)
// 1. tst: InstRd   tst2: MifWr     tst3: InstWr     (tst3)
// 1. tst: InstRd   tst2: InstWr    tst3: MifWr      (tst3)
// 1. tst: InstRd   tst2: MifWr     tst3: MifWr      (tst3)
// 1. tst: InstRd   tst2: InstWr    tst3: InstWr     (tst4)
// 1. tst: InstRd   tst2: MifWr     tst3: InstWr     (tst4)
// 1. tst: InstRd   tst2: InstWr    tst3: MifWr      (tst4)
// 1. tst: InstRd   tst2: MifWr     tst3: MifWr      (tst4)

// 1. Test 2R/1W    (two modules)
// 1. tst: InstRd   tst2: InstRd+InstWr             (tst)
// 1. tst: InstRd   tst2: InstRd+MifWr              (tst)
// 1. tst: InstRd   tst2: InstRd+InstWr             (tst2)
// 1. tst: InstRd   tst2: InstRd+MifWr              (tst2)
// 1. tst: InstRd   tst2: InstRd+InstWr             (tst3)
// 1. tst: InstRd   tst2: InstRd+MifWr              (tst3)

// 1. Test 2R/1W    (three modules)
// 1. tst: InstRd   tst2: InstRd    tst3: InstWr    (tst)
// 1. tst: InstRd   tst2: InstRd    tst3: MifWr     (tst)
// 1. tst: InstRd   tst2: InstRd    tst3: InstWr    (tst2)
// 1. tst: InstRd   tst2: InstRd    tst3: MifWr     (tst2)
// 1. tst: InstRd   tst2: InstRd    tst3: InstWr    (tst3)
// 1. tst: InstRd   tst2: InstRd    tst3: MifWr     (tst3)
// 1. tst: InstRd   tst2: InstRd    tst3: InstWr    (tst4)
// 1. tst: InstRd   tst2: InstRd    tst3: MifWr     (tst4)

//    Test 2R/2W    (two modules)
// 2. tst: InstWr+InstRd tst2: InstWr+InstRd        (tst)
// 2. tst: InstWr+InstRd tst2: MifWr+InstRd         (tst)
// 2. tst: MifWr+InstRd tst2: InstWr+InstRd         (tst)
// 2. tst: MifWr+InstRd tst2: MifWr+InstRd          (tst)
// 2. tst: InstWr+InstRd tst2: InstWr+InstRd        (tst3)
// 2. tst: InstWr+InstRd tst2: MifWr+InstRd         (tst3)
// 2. tst: MifWr+InstRd tst2: InstWr+InstRd         (tst3)
// 2. tst: MifWr+InstRd tst2: MifWr+InstRd          (tst3)

//    Test 2R/2W    (three modules)
// 2. tst: InstWr+InstRd tst2: InstWr   tst3: InstRd    (tst)
// 2. tst: InstWr+InstRd tst2: MifWr   tst3: InstRd     (tst)
// 2. tst: MifWr+InstRd tst2: InstWr   tst3: InstRd     (tst)
// 2. tst: MifWr+InstRd tst2: MifWr   tst3: InstRd      (tst)
// 2. tst: InstWr+InstRd tst2: InstWr   tst3: InstRd    (tst2)
// 2. tst: InstWr+InstRd tst2: MifWr   tst3: InstRd     (tst2)
// 2. tst: MifWr+InstRd tst2: InstWr   tst3: InstRd     (tst2)
// 2. tst: MifWr+InstRd tst2: MifWr   tst3: InstRd      (tst2)
// 2. tst: InstWr+InstRd tst2: InstWr   tst3: InstRd    (tst4)
// 2. tst: InstWr+InstRd tst2: MifWr   tst3: InstRd     (tst4)
// 2. tst: MifWr+InstRd tst2: InstWr   tst3: InstRd     (tst4)
// 2. tst: MifWr+InstRd tst2: MifWr   tst3: InstRd      (tst4)

//    Test 2R/2W    (four modules)
// 2. tst: InstWr   tst2: InstWr   tst3: InstRd    tst4 InstRd    (tst)
// 2. tst: InstWr   tst2: MifWr    tst3: InstRd    tst4 InstRd    (tst)
// 2. tst: MifWr    tst2: InstWr   tst3: InstRd    tst4 InstRd    (tst)
// 2. tst: MifWr    tst2: MifWr    tst3: InstRd    tst4 InstRd    (tst)
// 2. tst: InstWr   tst2: InstWr   tst3: InstRd    tst4 InstRd    (tst3)
// 2. tst: InstWr   tst2: MifWr    tst3: InstRd    tst4 InstRd    (tst3)
// 2. tst: MifWr    tst2: InstWr   tst3: InstRd    tst4 InstRd    (tst3)
// 2. tst: MifWr    tst2: MifWr    tst3: InstRd    tst4 InstRd    (tst3)
// 2. tst: InstWr   tst2: InstWr   tst3: InstRd    tst4 InstRd    (tst5)
// 2. tst: InstWr   tst2: MifWr    tst3: InstRd    tst4 InstRd    (tst5)
// 2. tst: MifWr    tst2: InstWr   tst3: InstRd    tst4 InstRd    (tst5)
// 2. tst: MifWr    tst2: MifWr    tst3: InstRd    tst4 InstRd    (tst5)

void CPersTst::PersTst()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case TST_READ:
		{
			if (ReadMemBusy()) {
				HtRetry();
				break;
			}

			P_err = 0;

			P_gvarAddr = 0;
			ReadMem_gvar(P_memAddr, P_gvarAddr, P_gvarAddr, 0);

			ReadMemPause(TST_CHK);
		}
		break;
		case TST_CHK:
		{
			if (SendCallBusy_tst2()) {
				HtRetry();
				break;
			}

			if (GR_gvar[0].data != 0xdeadbeef00001234ULL) {
				HtAssert(0, 0);
				P_err += 1;
			}

			SendCall_tst2(TST_CALL, P_memAddr, P_err);
		}
		break;
		case TST_CALL:
		{
			if (SendCallBusy_tst3()) {
				HtRetry();
				break;
			}

			SendCall_tst3(TST_RTN, P_memAddr);
		}
		break;
		case TST_RTN:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			SendReturn_htmain(P_err);
		}
		break;
		default:
			assert(0);
		}
	}
}
