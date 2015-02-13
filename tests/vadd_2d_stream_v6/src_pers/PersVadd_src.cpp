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
			SendHostMsg(VADD_TYPE_SIZE, (XDIM_LEN << 8) | TYPE_SIZE);
			HtTerminate();
			break;
		case VADD_ENTER:
			S_yIdx[PR_htId] = 0;
			S_yDimLen[PR_htId] = PR_yDimLen;
			S_xIdx[PR_htId] = 0;
			S_xDimLen[PR_htId] = PR_xDimLen;
			S_sum[PR_htId] = 0;

			P_addrA = SR_addrA + PR_yAddrOff;
			P_addrB = SR_addrB + PR_yAddrOff;
			P_addrC = SR_addrC + PR_yAddrOff;

			HtContinue(VADD_OPEN);

			break;
		case VADD_OPEN:

			// Open read stream A, once for each xDim to be processed
			if (PR_yOpenAIdx < PR_yDimLen && !ReadStreamBusy_A(PR_htId)) {
				ht_uint32 remLen = (ht_uint32)((PR_yDimLen - PR_yOpenAIdx) * PR_xDimLen);
				ReadStreamOpen_A(PR_htId, PR_addrA, remLen > 0x3f ? (ht_uint6)0x3f : (ht_uint6)remLen, P_yOpenAIdx);

				P_addrA+= PR_xDimLen * TYPE_SIZE;
				P_yOpenAIdx += 1;
			}

			// Open read stream B, once for each xDim to be processed
			if (PR_yOpenBIdx < PR_yDimLen && !ReadStreamBusy_B(PR_htId)) {
				ReadStreamOpen_B(PR_htId, PR_addrB, PR_xDimLen);

				P_addrB += PR_xDimLen * TYPE_SIZE;
				P_yOpenBIdx += 1;
			}

			// Open write stream, once for each xDim to be processed
			if (PR_yOpenCIdx < SR_yDimLen[PR_htId] && !WriteStreamBusy_C(PR_htId)) {
#if VADD_STRM_RSP_GRP_HTID == 0 && VADD_HTID_W == 0 && VADD_RSP_GRP_W > 0
				WriteStreamOpen_C(PR_htId, 1u, PR_addrC);
#elif VADD_STRM_RSP_GRP_HTID || VADD_HTID_W == 0
				WriteStreamOpen_C(PR_htId, PR_addrC);
#else
				WriteStreamOpen_C(PR_htId, PR_htId ^ 1, PR_addrC);
#endif
				P_addrC += PR_xDimLen * TYPE_SIZE;
				P_yOpenCIdx += 1;
			}

			if (PR_yOpenAIdx == PR_yDimLen && PR_yOpenBIdx == PR_yDimLen && PR_yOpenCIdx == PR_yDimLen)
#if VADD_STRM_RSP_GRP_HTID == 0 && VADD_HTID_W == 0 && VADD_RSP_GRP_W > 0
				WriteStreamPause_C(1, VADD_RETURN);
#elif VADD_STRM_RSP_GRP_HTID || VADD_HTID_W == 0
				WriteStreamPause_C(VADD_RETURN);
#else
				WriteStreamPause_C(PR_htId ^ 1, VADD_RETURN);
#endif
			else
				HtContinue(VADD_OPEN);

			break;
		case VADD_RETURN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain(S_sum[PR_htId]);
		}
		break;
		default:
			assert(0);
		}
	}

	for (int strmId = 0; strmId < VADD_HTID_CNT; strmId += 1) {
		if (SR_yIdx[strmId] < SR_yDimLen[strmId] &&
			ReadStreamReady_A(strmId) &&
			ReadStreamReady_B(strmId) &&
			WriteStreamReady_C(strmId))
		{
			PersType_t a, b;
			a = ReadStream_A(strmId);
			b = ReadStream_B(strmId);

			PersType_t c = a + b;

			S_sum[strmId] += (ht_uint32)c;
			WriteStream_C(strmId, c);

			assert_msg(SR_yIdx[strmId] == ReadStreamTag_A(strmId), "ReadStreamTag_A() error");

			if (SR_xIdx[strmId] + 1 < SR_xDimLen[strmId])
				S_xIdx[strmId] += 1;
			else {
				ReadStreamClose_A(strmId);
				WriteStreamClose_C(strmId);
				S_xIdx[strmId] = 0;
				S_yIdx[strmId] += 1;
			}
		}
	}
}
