#include "Ht.h"
#include "PersAdd_prim.h"

ht_prim ht_clk("ck") void add_5stage (
	uint64_t const &	t1_a,
	uint64_t const &	t1_b,
	uint64_t &		t6_res,
	add_prm_state &		s)
{
#ifndef _HTV

	t6_res = s.res[5];

	// stage the results for 5 cycles to match the verilog
	// state must be passed in because all local state is lost on each return
	int i;
	for (i = 5; i > 1; i--) {
		s.res[i] = s.res[i - 1];
	}
	s.res[1] = t1_a + t1_b;

#endif
}
