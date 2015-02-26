#include "Ht.h"
#include "PersJacob.h"

// Jacobi Relaxation

// Original version

#include <stdio.h>
#if 0
#include <stdlib.h>
#include <assert.h>
#endif

#ifndef min
#define min(a, b) ((a < b) ? a : b)
#endif

#define fmax(a, b) ((a) > (b) ? (a) : (b))
#define fabs(a) (((a) > 0) ? (a) : (0 - (a)))

#define P_a             P_p_a
#define P_newa          P_p_newa
#define P_m             P_p_m
#define P_n             P_p_n
#define P_w0            P_p_w0
#define P_w1            P_p_w1
#define P_w2            P_p_w2
#define P_tolerance     P_p_tolerance

#define M_hiftid        P_hiftid
#define M_iteration     P_iteration
#define M_ii            P_ii
#define M_jj            P_jj
#define M_iiblk         P_iiblk
#define M_jjblk         P_jjblk
#define M_tchg          P_tchg

void
CPersJacob::PersJacob()
{
	if (PR_htValid) {
		//    fprintf(stderr,"jacobi was called and P_htValid is %d and state is %d and thread id is %d\n", P_htValid, (int)P_htInst, (int)c_ts_htId);
		switch (PR_htInst) {
		case JSTART:
		{
			c_stencil_count = 0;

			M_iteration = 1;

#if 0
			fprintf(stderr, "in JSTART values of args are\n");
			fprintf(stderr, "P_p_a = %llx\n", P_p_a);
			fprintf(stderr, "P_p_newa = %llx\n", P_p_newa);
			fprintf(stderr, "P_p_m = %x\n", P_p_m);
			fprintf(stderr, "P_p_n = %x\n", P_p_n);
			fprintf(stderr, "P_p_w0 = %llx\n", P_p_w0);
			fprintf(stderr, "P_p_w1 = %llx\n", P_p_w1);
			fprintf(stderr, "P_p_w2 = %llx\n", P_p_w2);
			fprintf(stderr, "P_p_tolerance = %llx\n", P_p_tolerance);
#endif

			HtContinue(DOLOOP_TOP);
		}
		break;

		case DOLOOP_TOP:
		{
			c_change = 0;
			M_ii = 0;
			HtContinue(II_LOOP_TOP);
		}
		break;

		case II_LOOP_TOP:
		{
#if 0
			fprintf(stderr, "II_LOOP_TOP\n");
			fprintf(stderr, "M_ii is %d and P_m is %d\n", M_ii, P_m);
#endif
			if (!(M_ii < P_m)) {
				RecvReturnPause_stencil(DOLOOP_LATCH);
				break;
			}
			M_jj = 0;
			HtContinue(JJ_LOOP_TOP);
		}
		break;

		case JJ_LOOP_TOP:
		{
			if (!(M_jj < P_n)) {
				HtContinue(II_LOOP_LATCH);
				break;
			}
			HtContinue(JJ_LOOP_BODY);
		}
		break;

		case JJ_LOOP_BODY:
		{
			if (SendCallBusy_stencil()) {
				//fprintf(stderr,"blocking on RecvReturn_stencil\n");
				HtRetry();
				break;
			}

			M_iiblk = (M_ii + IBLK < P_m ? IBLK + 2 : IBLK);
			M_jjblk = (M_jj + JBLK < P_n ? JBLK + 2 : JBLK);

			// alternating two arrays here will im_prove performance
#if 0
			fprintf(stderr, "doing call for sizes %d %d and indices %d %d from jacob tid %d\n",
				(int)M_iiblk, (int)M_jjblk, (int)M_ii, (int)M_jj, 0);// , (int)c_ts_htId);
#endif
			if (M_iteration & 0x1)
				SendCallFork_stencil(STENCIL_JOIN, P_a, P_newa, M_ii, M_jj, M_iiblk, M_jjblk, P_w0, P_w1, P_w2);
			else
				SendCallFork_stencil(STENCIL_JOIN, P_newa, P_a, M_ii, M_jj, M_iiblk, M_jjblk, P_w0, P_w1, P_w2);

			HtContinue(JJ_LOOP_LATCH);
		}
		break;

		case JJ_LOOP_LATCH:
		{
			// increment M_jj and loop back
			M_jj = M_jj + JBLK;
			HtContinue(JJ_LOOP_TOP);
		}
		break;

		case II_LOOP_LATCH:
		{
			// increment ii and loop back
			M_ii = M_ii + IBLK;
			HtContinue(II_LOOP_TOP);
		}
		break;

		case STENCIL_JOIN:
		{
			// Wait for all stencil asyncCalls to complete
			c_change = fmax(c_change, M_tchg);
#if 0
			fprintf(stderr, "In join: c_change is %g and M_tchg is %g\n",
				(double)(c_change) / (double)ONE,
				(double)(M_tchg) / (double)ONE);
#endif
			RecvReturnJoin_stencil();
			break;
		}

		case DOLOOP_LATCH:
		{
#ifndef _HTV
			fprintf(stderr, "iteration %d change is %g\n",
				(int)M_iteration,
				(double)(c_change) / (double)ONE);
#endif
			if (c_change > P_tolerance) {
				M_iteration = M_iteration + 1;
				HtContinue(DOLOOP_TOP);
			} else {
#ifndef _HTV
				fprintf(stderr, "complete after %d iterations\n",
					(int)M_iteration);
				fflush(stderr);
#endif
				HtContinue(JDONE);
			}
		}
		break;

		default:
		case JDONE:
		{
			if (SendReturnBusy_jacob()) {
				HtRetry();
				break;
			}
			SendReturn_jacob(M_iteration);
			// return M_iteration;
		}
		break;
		}
	} // P_htValid
}

#undef P_a
#undef P_newa
#undef P_n
#undef P_m
#undef P_w0
#undef P_w1
#undef P_w2
#undef P_tolerance

#undef M_iteration
#undef M_ii
#undef M_jj
#undef M_iiblk
#undef M_jjblk
#undef M_tchg
