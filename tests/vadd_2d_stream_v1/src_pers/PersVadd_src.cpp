#include "Ht.h"
#include "PersVadd.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

#ifndef min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#endif

void
CPersVadd::PersVadd()
{
	S_addrA = SR_addrA + SR_addrA_op;
	S_addrB = SR_addrB + SR_addrB_op;
	S_addrC = SR_addrC + SR_addrC_op;

	S_addrA_op = 0;
	S_addrB_op = 0;
	S_addrC_op = 0;
	S_sum_op = 0;

	S_sum = SR_sum + SR_sum_op;

	if (PR_htValid) {
		switch (PR_htInst) {
		case VADD_RESET:
			if (SR_msgDelay < 500 || SendHostMsgBusy()) {
				S_msgDelay += 1;
				HtRetry();
				break;
			}
			SendHostMsg(VADD_TYPE_SIZE, (XDIM_LEN << 8) | TYPE_SIZE);
			HtTerminate();
			break;
		case VADD_ENTER:
			S_yIdx = 0;
			S_yDimLenM1 = PR_yDimLen-1;
			S_xIdx = 0;
			S_xDimLenM1 = PR_xDimLen-1;
			S_sum = 0;

			P_yOpenADone = false;
			P_yOpenBDone = false;
			P_yOpenCDone = false;

			S_addrA = SR_addrOp1;
			S_addrB = SR_addrOp2;
			S_addrC = SR_addrRes;

			S_addrA_op = PR_addrOff;
			S_addrB_op = PR_addrOff;
			S_addrC_op = PR_addrOff;

			HtContinue(VADD_OPEN);

			break;
		case VADD_OPEN:

			// Open read stream A, once for each xDim to be processed
			if (!PR_yOpenADone && !ReadStreamBusy_A()) {

				ReadStreamOpen_A(SR_addrA, PR_xDimLen, PR_yOpenAIdx);

				S_addrA_op = PR_xDimLen * TYPE_SIZE;
				P_yOpenADone = PR_yOpenAIdx == SR_yDimLenM1;
				P_yOpenAIdx += 1;
			}

			// Open read stream B, once for each xDim to be processed
			if (!PR_yOpenBDone && !ReadStreamBusy_B()) {
				ReadStreamOpen_B(SR_addrB, PR_xDimLen);

				S_addrB_op = PR_xDimLen * TYPE_SIZE;
				P_yOpenBDone = PR_yOpenBIdx == SR_yDimLenM1;
				P_yOpenBIdx += 1;
			}

			// Open write stream, once for each xDim to be processed
			if (!PR_yOpenCDone && !WriteStreamBusy_C()) {
				WriteStreamOpen_C(SR_addrC, PR_xDimLen);

				S_addrC_op = PR_xDimLen * TYPE_SIZE;
				P_yOpenCDone = PR_yOpenCIdx == SR_yDimLenM1;
				P_yOpenCIdx += 1;
			}

			if (PR_yOpenADone && PR_yOpenBDone && PR_yOpenCDone)
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

	if (!SR_bEndOfYDim &&
		ReadStreamReady_A() &&
		ReadStreamReady_B() &&
		WriteStreamReady_C())
	{
		PersType_t a, b;
		a = ReadStream_A();
		b = ReadStream_B();

		PersType_t c = a + b;

		S_sum_op = (ht_uint32)c;
		WriteStream_C(c);

		bool bEndOfXDim = SR_xIdx == SR_xDimLenM1;
		assert_msg(bEndOfXDim == ReadStreamLast_B(), "bEndOfXDim error");
		S_xIdx = bEndOfXDim ? 0 : SR_xIdx + 1;
		S_yIdx = bEndOfXDim ? SR_yIdx + 1 : SR_yIdx;
		S_bEndOfYDim = bEndOfXDim ? SR_yIdx == SR_yDimLenM1 : false;
		assert_msg(SR_yIdx == ReadStreamTag_A(), "ReadStreamTag_A() error");
	}
}
