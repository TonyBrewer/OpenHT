#include "Ht.h"
#include "PersMuladd_prim.h"

ht_prim ht_clk("clk") void muladd_wrap (
	uint64_t const &	i_a,
	uint64_t const &	i_b,
	uint64_t const &	i_c,
	ht_uint7 const &        i_htId,
	bool const &            i_vld,
	uint64_t &		o_res,
	ht_uint7 &              o_htId,
	bool &                  o_vld,
	muladd_prm_state &	s)
{
#ifndef _HTV

	// Inputs a, b are valid on the inital input stage - (Stage 1)
	// Output p is valid on 19 cycles later - (Stage 20)

	// p is valid on Stage 20
	o_res = s.res[18];
	o_htId = s.htId[18];
	o_vld  = s.vld[18];

	// s.p[i] is valid on Stage (i+1) (s.p[4] on Stage 5, s.p[3] on Stage 4, etc...)
	int i;
	for (i = 18; i > 0; i--) {
		s.res[i] = s.res[i - 1];
		s.htId[i] = s.htId[i - 1];
		s.vld[i] = s.vld[i - 1];
	}

	// a, b, c, are all valid on the initial stage - (Stage 1)
	s.res[0] = i_a + (i_b * i_c);
	s.htId[0] = i_htId;
	s.vld[0]  = i_vld;

#endif
}
