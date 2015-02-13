#include "Ht.h"
#include "PersVadd.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

#ifndef min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#endif

void
CPersVadd::PersVadd()
{
	ht_uint1 wrStrm_C_Id = 0;

	ht_uint1 rdStrm_A_Id0 = 0;
	ht_uint1 rdStrm_A_Id1 = 1;
	ht_uint2 rdStrmMask = 3;

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

			S_addrA += (uint32_t)(PR_yDimOff * PR_xDimLen * TYPE_SIZE);
			S_addrB += (uint32_t)(PR_yDimOff * PR_xDimLen * TYPE_SIZE);
			S_addrC += (uint32_t)(PR_yDimOff * PR_xDimLen * TYPE_SIZE);

			WriteStreamPause_C(VADD_OPEN);

			break;
		case VADD_OPEN:

			// Open read stream A, once for each xDim to be processed
			if (PR_yOpenAIdx < PR_yDimLen && !ReadStreamBusyMask_A(rdStrmMask)) {

				ReadStreamOpen_A(rdStrm_A_Id0, SR_addrA, PR_xDimLen);
				ReadStreamOpen_A(rdStrm_A_Id1, SR_addrB, PR_xDimLen);

				S_addrA += PR_xDimLen * TYPE_SIZE;
				P_yOpenAIdx += 1;

				S_addrB += PR_xDimLen * TYPE_SIZE;
				P_yOpenBIdx += 1;
			}

			// Open write stream, once for each xDim to be processed
			if (PR_yOpenCIdx < SR_yDimLen && !WriteStreamBusy_C(wrStrm_C_Id)) {
				WriteStreamOpen_C(wrStrm_C_Id, SR_addrC, PR_xDimLen);

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
		ReadStreamReady_A(rdStrm_A_Id0) &&
		ReadStreamReady_A(rdStrm_A_Id1) &&
		WriteStreamReady_C(wrStrm_C_Id))
	{
		PersType_t a, b;
		a = ReadStream_A(rdStrm_A_Id0);
		b = ReadStream_A(rdStrm_A_Id1);

		PersType_t c = a + b;

		S_sum += (ht_uint32)c;
		WriteStream_C(wrStrm_C_Id, c);

		if (SR_xIdx+1 < SR_xDimLen)
			S_xIdx += 1;
		else {
			S_xIdx = 0;
			S_yIdx += 1;
		}
	}
}
