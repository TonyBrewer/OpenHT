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

			S_addrA += (uint32_t)(PR_yDimOff * PR_xDimLen * TYPE_SIZE);
			S_addrB += (uint32_t)(PR_yDimOff * PR_xDimLen * TYPE_SIZE);
			S_addrC += (uint32_t)(PR_yDimOff * PR_xDimLen * TYPE_SIZE);

			HtContinue(VADD_OPEN);

			break;
		case VADD_OPEN:
			{
				// Open read stream A & B, once for each xDim to be processed
				sc_uint<UNROLL_CNT> rdStrm_mask = (sc_uint<UNROLL_CNT>)(PR_yDimLen - PR_yOpenAIdx >= UNROLL_CNT ? ((1 << UNROLL_CNT)-1) : ((1 << (PR_yDimLen - PR_yOpenAIdx))-1));

				if (PR_yOpenAIdx < PR_yDimLen && 
					!ReadStreamBusyMask_A(rdStrm_mask) &&
					!ReadStreamBusyMask_B(rdStrm_mask))
				{
					for (int i = 0; i < UNROLL_CNT; i += 1) {
						if (rdStrm_mask & (1 << i)) {
							ReadStreamOpen_A(i, S_addrA, PR_xDimLen);
							ReadStreamOpen_B(i, S_addrB, PR_xDimLen);

							S_addrA += PR_xDimLen * TYPE_SIZE;
							P_yOpenAIdx += 1;

							S_addrB += PR_xDimLen * TYPE_SIZE;
							P_yOpenBIdx += 1;
						}
					}
				}

				// Open write stream, once for each xDim to be processed
				sc_uint<UNROLL_CNT> wrStrm_mask = (sc_uint<UNROLL_CNT>)(PR_yDimLen - PR_yOpenCIdx >= UNROLL_CNT ? ((1 << UNROLL_CNT)-1) : ((1 << (PR_yDimLen - PR_yOpenCIdx))-1));

				if (PR_yOpenCIdx < SR_yDimLen && !WriteStreamBusyMask_C(wrStrm_mask)) {
					for (int i = 0; i < UNROLL_CNT; i += 1) {
						if (wrStrm_mask & (1 << i)) {
							WriteStreamOpen_C(i, S_addrC, PR_xDimLen);

							S_addrC += PR_xDimLen * TYPE_SIZE;
							P_yOpenCIdx += 1;
						}
					}
				}

				if (PR_yOpenAIdx == PR_yDimLen && PR_yOpenBIdx == PR_yDimLen && PR_yOpenCIdx == PR_yDimLen)
					WriteStreamPause_C(VADD_RETURN);
				else
					HtContinue(VADD_OPEN);
			}
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

	sc_uint<UNROLL_CNT> strm_mask = (sc_uint<UNROLL_CNT>)(SR_yDimLen - SR_yIdx >= UNROLL_CNT ? ((1 << UNROLL_CNT)-1) : ((1 << (SR_yDimLen - SR_yIdx))-1));

	if (SR_yIdx < SR_yDimLen &&
		ReadStreamReadyMask_A(strm_mask) &&
		ReadStreamReadyMask_B(strm_mask) &&
		WriteStreamReadyMask_C(strm_mask))
	{
		for (int i = 0; i < UNROLL_CNT; i += 1) {
			if (strm_mask & (1 << i)) {

				PersType_t a, b;
				a = ReadStream_A(i);
				b = ReadStream_B(i);

				PersType_t c = a + b;

				S_sum += (uint32_t)c;
				WriteStream_C(i, c);
			}
		}

		if (SR_xIdx+1 < SR_xDimLen)
			S_xIdx += 1;
		else {
			S_xIdx = 0;
			S_yIdx += UNROLL_CNT;
		}
	}
}
