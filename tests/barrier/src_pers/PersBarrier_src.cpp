#include "Ht.h"
#include "PersBarrier.h"

void CPersBarrier::PersBarrier()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case BARRIER_ENTRY:
			{
				if (P_loopCnt == PR_threadId)
					HtContinue(BARRIER_INC);
				else
					HtContinue(BARRIER_ENTRY);

				P_loopCnt += 1;
			}
			break;
		case BARRIER_INC:
			{
				SendMsg_inc(true);
				HtContinue(BARRIER_WAIT1);
			}
			break;
		case BARRIER_WAIT1:
			{
#	ifdef BARRIER_NAMED
				HtBarrier_sync(((1u << BARRIER_BARID_W)-1u), BARRIER_WAIT2, (sc_uint<BARRIER_THREAD_W+1>) PR_threadCnt);
#	else
				HtBarrier(((1u << BARRIER_BARID_W)-1u), BARRIER_WAIT2, (sc_uint<BARRIER_THREAD_W+1>) PR_threadCnt);
#	endif
			}
			break;
		case BARRIER_WAIT2:
			{
#	ifdef BARRIER_MULTIPLE
				HtBarrier_sync2(1, BARRIER_WAIT3, (sc_uint<BARRIER_THREAD_W+1>) PR_threadCnt);
#	else
#		ifdef BARRIER_NAMED
				HtBarrier_sync(((1u << BARRIER_BARID_W)-1u), BARRIER_WAIT3, (sc_uint<BARRIER_THREAD_W+1>) PR_threadCnt);
#		else
				HtBarrier(((1u << BARRIER_BARID_W)-1u), BARRIER_WAIT3, (sc_uint<BARRIER_THREAD_W+1>) PR_threadCnt);
#		endif
#	endif
			}
			break;
		case BARRIER_WAIT3:
			{
#	if ((1<<BARRIER_BARID_W) >= BARRIER_REPL_CNT)
#		if (BARRIER_BARID_W == 0)
#			define BARID_W 1
#		else
#			define BARID_W BARRIER_BARID_W
#		endif
#		ifdef BARRIER_NAMED
				HtBarrier_sync((sc_uint<BARID_W>)i_replId.read(), BARRIER_RETURN, (sc_uint<BARRIER_THREAD_W+1>) PR_threadCnt/BARRIER_REPL_CNT);
#		else
				HtBarrier((sc_uint<BARID_W>)i_replId.read(), BARRIER_RETURN, (sc_uint<BARRIER_THREAD_W+1>) PR_threadCnt/BARRIER_REPL_CNT);
#		endif
#	else
				HtContinue(BARRIER_RETURN);
#	endif
			}
			break;
		case BARRIER_RETURN:
			{
				if (SendReturnBusy_barrier()) {
					HtRetry();
					break;
				}

				SendReturn_barrier(S_inc != PR_threadCnt);
			}
			break;
		default:
			assert(0);
		}
	}

	for (int replIdx = 0; replIdx < BARRIER_REPL_CNT; replIdx += 1) {
		if (RecvMsgReady_inc((ht_uint1)replIdx) && RecvMsg_inc((ht_uint1)replIdx))
			S_inc += 1;
	}


	if (RecvMsgReady_clr() && RecvMsg_clr())
		S_inc = 0;
}
