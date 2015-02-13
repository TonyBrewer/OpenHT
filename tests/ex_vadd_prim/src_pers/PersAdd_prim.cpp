#include "Ht.h"
#include "PersAdd_prim.h"

ht_prim ht_clk("ck") void add_5stage(
	uint64_t const &	i_a,
	uint64_t const &	i_b,
	ht_uint7 const &	i_htId,
	bool const &		i_vld,
	uint64_t &		o_res,
	ht_uint7 &		o_htId,
	bool &			o_vld,
	add_prm_state &		s)
{
#ifndef _HTV

	o_res = s.res[5];
	o_htId = s.htId[5];
	o_vld = s.vld[5];

	// stage the results for 5 cycles to match the verilog
	// state must be passed in because all local state is lost on each return
	int i;
	for (i = 5; i > 1; i--) {
		s.res[i] = s.res[i - 1];
		s.htId[i] = s.htId[i - 1];
		s.vld[i] = s.vld[i - 1];
	}

	s.res[1] = i_a + i_b;
	s.htId[1] = i_htId;
	s.vld[1] = i_vld;

#endif
}
