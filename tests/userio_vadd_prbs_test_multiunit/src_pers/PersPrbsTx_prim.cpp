#include "Ht.h"
#include "PersPrbsTx_prim.h"

ht_prim ht_clk("clk") void prbs_gen (
	bool const & rst,
	bool const & i_req,
	bool & o_rdy,
	bool & o_vld,
	ht_uint64 & o_res_lower,
	ht_uint64 & o_res_upper,
	prbs_gen_prm_state &s)
{
#ifndef _HTV


	// NOTE! This does not accurately model the verilog.
	// This cheats and does a much less strenous sequence
	// Modeling the PRBS pattern in SystemC was deemed not critical

	for (int i = 1; i > 0; i--) {
		s.rnd_upper[i] = (rst) ? (ht_uint64)0x0123456789ABCDEFLL : s.rnd_upper[i - 1];
		s.rnd_lower[i] = (rst) ? (ht_uint64)0xFEDCBA9876543210LL : s.rnd_lower[i - 1];
	}

	// Shift Operation (only advance with request)
	ht_uint64 newUpper, newLower;
	newUpper = (ht_uint64)(
			       (s.rnd_upper[1] << 1) |
			       (s.rnd_lower[1] >> 63) & 0x1LL
			       );
        newLower = (ht_uint64)(
			       (s.rnd_lower[1] << 1) |
			       (
				((s.rnd_upper[1] >> 63) & 0x1) ^
				((s.rnd_upper[1] >> 61) & 0x1) ^
				((s.rnd_upper[1] >> 36) & 0x1) ^
				((s.rnd_upper[1] >> 34) & 0x1)
				)
			       );

	// Inputs are valid on the inital input stage - (Stage 1)
	// Output is valid 1 cycles later - (Stage 2)

	// p is valid on Stage 2
	o_rdy = !rst;
	o_vld = s.vld[0];
	o_res_lower = s.res_lower[0];
	o_res_upper = s.res_upper[0];

	if (i_req) {
		s.res_lower[0] = s.rnd_lower[0];
		s.res_upper[0] = s.rnd_upper[0];
		s.rnd_lower[0] = newLower;
		s.rnd_upper[0] = newUpper;
	} else {
		s.rnd_lower[0] = s.rnd_lower[1];
		s.rnd_upper[0] = s.rnd_upper[1];
	}

	s.vld[0] = i_req;

#endif
}
