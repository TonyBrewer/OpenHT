#include "Ht.h"
#include "PersAdd.h"

// Include file with primitives
#include "PersAdd_prim.h"

void
CPersAdd::PersAdd()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ADD_ENTRY: {
			// P_ia, P_ib, P_ic valid on entry
			S_vld = false;
			S_done = false;
			HtContinue(ADD_SETUP);
		}
		break;
		case ADD_SETUP: {
			S_vld = true;
			HtContinue(ADD_RTN);
		}
		break;
		case ADD_RTN: {
			if (SendReturnBusy_add()) {
				HtRetry();
				break;
			}

			// Return Result from private variable 'rslt'
			if (S_done == false) {
				HtRetry();
			} else {
				SendReturn_add(S_rsltC, S_rsltP, S_rsltS);
			}
		}
		break;
		default:
			assert(0);
		}
	}


	T1_bVld = S_vld;

	if (T1_bVld == true) {
		T1_a = P_ia;
		T1_b = P_ib;
		T1_c = P_ic;

		S_vld = false;
	}

	// Use T_ variables as outputs of clocked adder prim
	addC(T1_a, T1_b,     T2_outC1, addCState1);
	addC(T2_c, T2_outC1, T3_outC2, addCState2);

	// Use T_ variables as outputs of propagational adder prim
	addP(T1_a, T1_b,      T1_outP1);
	addP(T2_c, TR2_outP1, T2_outP2);

	// Use T_ variables as outputs of propagational adder prim
	T1_inS1.a = T1_a;
	T1_inS1.b = T1_b;
	addS(T1_inS1, T1_outS1);

	T2_inS2.a = TR2_outS1.res;
	T2_inS2.b = T2_c;
	addS(T2_inS2, T2_outS2);

	if (T3_bVld == true) {
		S_done = true;
		S_rsltC = T3_outC2;
		S_rsltP = TR3_outP2;
		S_rsltS = TR3_outS2.res;
	}
}
