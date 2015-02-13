#pragma once

// state for add clocked primitive
ht_state struct mult_prm_state {
	uint64_t	res[5];
	ht_uint4	htId[5];
	bool    	vld[5];
};

// 32-bit mult, clocked primitive
//   function definition located in .cpp file
ht_prim ht_clk("ck") void mult_wrap (
	uint32_t const & i_a, 
	uint32_t const & i_b, 
	ht_uint4 const & i_htId,
	bool const & i_vld,
	uint64_t & o_res, 
	ht_uint4 & o_htId,
	bool & o_vld,
	mult_prm_state &s);
