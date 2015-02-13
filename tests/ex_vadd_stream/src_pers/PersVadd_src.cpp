#include "Ht.h"
#include "PersVadd.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

void
CPersVadd::PersVadd()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case VADD_ENTER: {
			BUSY_RETRY(ReadStreamBusy_A());
			BUSY_RETRY(ReadStreamBusy_B());
			BUSY_RETRY(WriteStreamBusy_C());

			S_sum = 0;

			if (!PR_vecLen) {
				HtContinue(VADD_RETURN);
				break;
			}

			MemAddr_t addrA = SR_op1Addr + PR_offset * sizeof(uint64_t);
			MemAddr_t addrB = SR_op2Addr + PR_offset * sizeof(uint64_t);
			MemAddr_t addrC = SR_resAddr + PR_offset * sizeof(uint64_t);

			ReadStreamOpen_A(addrA, PR_vecLen);
			ReadStreamOpen_B(addrB, PR_vecLen);
			WriteStreamOpen_C(addrC, PR_vecLen);

			WriteStreamPause_C(VADD_RETURN);
			break;
		}
		case VADD_RETURN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain(S_sum);
		}
		break;
		default:
			assert(0);
		}
	}

	if (ReadStreamReady_A() &&
	    ReadStreamReady_B() &&
	    WriteStreamReady_C()) {
		uint64_t a = ReadStream_A();
		uint64_t b = ReadStream_B();
		uint64_t c = a + b;

		S_sum += c;
		WriteStream_C(c);
	}
}
