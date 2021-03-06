#include "Ht.h"
#include "PersMult_prim.h"

ht_prim ht_clk("clk") void mult_wrap (
	uint32_t const &	i_a,
	uint32_t const &	i_b,
	ht_uint4 const &        i_htId,
	bool const &            i_vld,
	uint64_t &		o_res,
	ht_uint4 &              o_htId,
	bool &                  o_vld,
	mult_prm_state &	s)
{
#ifndef _HTV

	// Inputs a, b are valid on the inital input stage - (Stage 1)
	// Output p is valid on 5 cycles later - (Stage 6)

	// p is valid on Stage 6
	o_res = s.res[4];
	o_htId = s.htId[4];
	o_vld  = s.vld[4];

	// s.p[i] is valid on Stage (i+1) (s.p[4] on Stage 5, s.p[3] on Stage 4, etc...)
	int i;
	for (i = 4; i > 0; i--) {
		s.res[i] = s.res[i - 1];
		s.htId[i] = s.htId[i - 1];
		s.vld[i] = s.vld[i - 1];
	}

	// a, b, and s.p[0] are all valid on the initial stage - (Stage 1)
	s.res[0] = i_a * i_b;
	s.htId[0] = i_htId;
	s.vld[0]  = i_vld;

#endif
}
