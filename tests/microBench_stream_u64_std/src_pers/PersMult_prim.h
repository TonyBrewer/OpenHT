#pragma once

// state for add clocked primitive
ht_state struct mult_prm_state {
	uint64_t	res[18];
	ht_uint9	htId[18];
	bool    	vld[18];
};

// 32-bit mult, clocked primitive
//   function definition located in .cpp file
ht_prim ht_clk("ck") void mult_wrap (
	uint64_t const & i_a, 
	uint64_t const & i_b, 
	ht_uint9 const & i_htId,
	bool const & i_vld,
	uint64_t & o_res, 
	ht_uint9 & o_htId,
	bool & o_vld,
	mult_prm_state &s);
