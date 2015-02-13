#include "Ht.h"
#include "PersMult_prim.h"

ht_prim ht_clk("clk") void multiplier(
	uint32_t const &	a,
	uint32_t const &	b,
	uint64_t &		p,
	mult_prm_state &	s)
{
#ifndef _HTV

	// Inputs a, b are valid on the inital input stage - (Stage 1)
	// Output p is valid on 5 cycles later - (Stage 6)

	// p is valid on Stage 6
	p = s.p[4];

	// s.p[i] is valid on Stage (i+1) (s.p[4] on Stage 5, s.p[3] on Stage 4, etc...)
	int i;
	for (i = 4; i > 0; i--)
		s.p[i] = s.p[i - 1];

	// a, b, and s.p[0] are all valid on the initial stage - (Stage 1)
	s.p[0] = a * b;

#endif
}
