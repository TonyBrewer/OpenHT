#include "Ht.h"
#include "PersMulAdd_prim.h"

ht_prim ht_clk("clk") void muladd_wrap (
	bool const &            rst,
	uint64_t const &	i_a,
	uint64_t const &	i_b,
	uint64_t const &	i_c,
	ht_uint9 const &        i_htId,
	bool const &            i_vld,
	bool &                  o_rdy,
	uint64_t &		o_res,
	ht_uint9 &              o_htId,
	bool &                  o_vld,
	muladd_prm_state &	s)
{
#ifndef _HTV

	// Inputs a, b are valid on the inital input stage - (Stage 1)
	// Output p is valid on 11 cycles later - (Stage 12)

	// p is valid on Stage 12
	o_rdy = true;
	o_res = s.res[10];
	o_htId = s.htId[10];
	o_vld  = s.vld[10];

	// s.p[i] is valid on Stage (i+1) (s.p[4] on Stage 5, s.p[3] on Stage 4, etc...)
	int i;
	for (i = 10; i > 0; i--) {
		s.res[i] = s.res[i - 1];
		s.htId[i] = s.htId[i - 1];
		s.vld[i] = s.vld[i - 1];
	}

	// a, b, and s.p[0] are all valid on the initial stage - (Stage 1)
	double a_tmp = *((double*)&i_a);
	double b_tmp = *((double*)&i_b);
	double c_tmp = *((double*)&i_c);
	double r_tmp = a_tmp + b_tmp * c_tmp;
	s.res[0] = *(uint64_t*)&r_tmp;
	s.htId[0] = i_htId;
	s.vld[0]  = i_vld;

#endif
}
