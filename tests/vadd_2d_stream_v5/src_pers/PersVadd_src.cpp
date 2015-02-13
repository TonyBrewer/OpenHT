#include "Ht.h"
#include "PersVadd.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

#ifndef min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#endif

void
CPersVadd::PersVadd()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case VADD_RESET:
			if (SR_msgDelay < 500 || SendHostMsgBusy()) {
				S_msgDelay += 1;
				HtRetry();
				break;
			}
			SendHostMsg(VADD_TYPE_SIZE, TYPE_SIZE);
			HtTerminate();
			break;
		case VADD_ENTER:
			S_yIdx = 0;
			S_yDimLen = PR_yDimLen;
			S_xIdx = 0;
			S_xDimLen = PR_xDimLen;
			S_sum = 0;

			S_addrA += (ht_uint32)(PR_yDimOff * PR_xDimLen * TYPE_SIZE);
			S_addrB += (ht_uint32)(PR_yDimOff * PR_xDimLen * TYPE_SIZE);
			S_addrC += (ht_uint32)(PR_yDimOff * PR_xDimLen * TYPE_SIZE);

			//HtContinue(VADD_WRITE);
			HtBarrier(VADD_WRITE, 1);
			break;
		case VADD_WRITE:
			BUSY_RETRY(WriteMemBusy());
			WriteMem_uint32(S_addrC & ~0x3ull, 0xabcdabcd);
			WriteMemPause(VADD_READ);
			break;
		case VADD_READ:
			BUSY_RETRY(ReadMemBusy());
			ReadMem_readVar(S_addrC & ~0x3ull);
			ReadMemPause(VADD_READ_CHECK);
			break;
		case VADD_READ_CHECK:
			BUSY_RETRY(SendCallBusy_sub());
			HtAssert(SR_readVar == 0xabcdabcd, 0);
			SendCallFork_sub(VADD_JOIN);
			RecvReturnPause_sub(VADD_OPEN);
			break;
		case VADD_JOIN:
			RecvReturnJoin_sub();
			break;
		case VADD_OPEN:

			// Open read stream A, once for each xDim to be processed
			if (PR_yOpenAIdx < PR_yDimLen && !ReadStreamBusy_A()) {
				ReadStreamOpen_A(SR_addrA, PR_xDimLen);

				S_addrA += PR_xDimLen * TYPE_SIZE;
				P_yOpenAIdx += 1;
			}

			// Open read stream B, once for each xDim to be processed
			if (PR_yOpenBIdx < PR_yDimLen && !ReadStreamBusy_B()) {
				ReadStreamOpen_B(SR_addrB, PR_xDimLen);

				S_addrB += PR_xDimLen * TYPE_SIZE;
				P_yOpenBIdx += 1;
			}

			// Open write stream, once for each xDim to be processed
			if (PR_yOpenCIdx < SR_yDimLen && !WriteStreamBusy_C()) {
				WriteStreamOpen_C(SR_addrC, (ht_uint10)PR_xDimLen);

				S_addrC += PR_xDimLen * TYPE_SIZE;
				P_yOpenCIdx += 1;
			}

			if (PR_yOpenAIdx == PR_yDimLen && PR_yOpenBIdx == PR_yDimLen && PR_yOpenCIdx == PR_yDimLen)
				WriteStreamPause_C(VADD_RETURN);
			else
				HtContinue(VADD_OPEN);

			break;
		case VADD_RETURN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain(S_sum);
		}
		break;
		default:
			assert(0);
		}
	}

	if (SR_yIdx < SR_yDimLen &&
		ReadStreamReady_A() &&
		ReadStreamReady_B() &&
		WriteStreamReady_C())
	{
		PersType_t a, b;
		a = ReadStream_A();
		b = ReadStream_B();

		PersType_t c = a + b;

		S_sum += (ht_uint32)c;
		WriteStream_C(c);

		if (SR_xIdx+1 < SR_xDimLen)
			S_xIdx += 1;
		else {
			S_xIdx = 0;
			S_yIdx += 1;
		}
	}
}
