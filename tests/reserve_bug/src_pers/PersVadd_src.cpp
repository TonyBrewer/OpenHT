#include "Ht.h"
#include "PersVadd.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

#ifndef min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#endif

void
CPersVadd::PersVadd()
{
    T1_bufRdAddr = TR2_bufRdAddr;
    if (PR2_htValid) {
        switch (PR2_htInst) {
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
                S_xIdx = 0;
                S_xDimLen = (ht_uint12) PR2_xDimLen;
                S_sum = 0;
                T1_bufRdAddr = 0xFFF;//ht_uint12(-1);
                BUSY_RETRY(SendCallBusy_in2());
                SendCallFork_in2(VADD_JOIN, PR2_yDimOff, (ht_uint12)PR2_xDimLen);
                S_inBbusy = true;
                S_addrA += (uint32_t)(PR2_yDimOff * PR2_xDimLen * TYPE_SIZE);
                S_addrC += (uint32_t)(PR2_yDimOff * PR2_xDimLen * TYPE_SIZE);
		HtContinue(VADD_OPEN);
                break;

            case VADD_OPEN:
                // Open read stream A, once
                BUSY_RETRY(ReadStreamBusy_A());
		ReadStreamOpen_A(SR_addrA, (ht_uint12)PR2_xDimLen);
		S_inAbusy = true;
		HtContinue(VADD_CLOSE);
                break;

            case VADD_CLOSE:
                // wait for bBuf to be written
                BUSY_RETRY(SR_inAbusy);
                BUSY_RETRY(SR_inBbusy);
                // Open write stream, once
                BUSY_RETRY(WriteStreamBusy_C());
		WriteStreamOpen_C(SR_addrC, (ht_uint12)PR2_xDimLen);
                WriteStreamPause_C(VADD_RETURN);
                break;

            case VADD_JOIN:
		RecvReturnJoin_in2();
                S_inBbusy = 0;
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
    
    if (ReadStreamReady_A())
    {
        GW1_aBuf.write_addr(SR_xIdx);
        GW1_aBuf = ReadStream_A();
        S_xIdx += 1;
	S_inAbusy = !ReadStreamLast_A();
    }

    PersType_t a, b;
    a = GR6_aBuf;
    b = GR6_bBuf;
    PersType_t c = a + b;

    T1_wrEn = 0;
    if (WriteStreamReady_C() && (TR2_bufRdAddr != SR_xDimLen - 1))
    {
	T1_wrEn = 1;
	T1_bufRdAddr = TR2_bufRdAddr + 1;
    }
    
    if (TR6_wrEn)
    {
	S_sum += (ht_uint32)c;
	// MDJ: Is it safe to WriteStream_C() here without checking WriteStreamReady_C() again?
	// Test passes, but I wonder if it is possible that buffer full could change since last checked
	WriteStream_C(c);
	// I thought that reserve=5 would be appropriate to cover this gap, but setting reserve to anything in AddWriteStream
	// causes test to fail
    }
}
