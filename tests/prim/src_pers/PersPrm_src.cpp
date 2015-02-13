#include "Ht.h"
#include "PersPrm.h"

// Include file with primitives
#include "PersPrm_prim.h"

// 8-bit adder, non-clocked function
ht_uint1 CPersPrm::add8(
	ht_uint8 const &	a,
	ht_uint8 const &	b,
	ht_uint1 const &	cyin,
	ht_uint8 &		c)
{
	ht_uint9 t = a + b + cyin;

	c = t(7, 0);
	return t(8, 8);
}

void
CPersPrm::PersPrm()
{
	T1_al = P1_a & 0xff;
	T1_bl = P1_b & 0xff;

	if (PR1_htValid) {
//        printf("foo\n");
	}

	// use primitive for lower add8, not clocked
	add8_prm(T1_al, T1_bl, 0, T1_r1l, T1_cy);
	T1_au = P1_a >> 8;
	T1_bu = P1_b >> 8;

	// use clocked primitive
	add16_prm(P1_a, P1_b, T2_r2, add16_prm_state1);

	if (PR2_htValid) {
		switch (PR2_htInst) {
		case PRM_ENTRY: {
			// use function for upper add8
			ht_uint8 r1u;
			add8(T2_au, T2_bu, T2_cy, r1u);

			P2_r1 = (r1u << 8) | T2_r1l;
			P2_r2 = T2_r2;

			HtContinue(PRM_RTN);
		}
		break;
		case PRM_RTN: {
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			SendReturn_htmain(P2_r1, P2_r2);
		}
		break;
		default:
			assert(0);
		}
	}
}
