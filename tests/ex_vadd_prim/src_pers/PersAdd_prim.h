#pragma once

// state for add clocked primitive
ht_state struct add_prm_state {
	uint64_t	res[6];
	ht_uint7	htId[6];
	bool    	vld[6];
};

// 16-bit add, clocked primitive
//   function definition located in .cpp file
ht_prim ht_clk("ck") void add_5stage (
	uint64_t const & i_a, 
	uint64_t const & i_b,
	ht_uint7 const & i_htId,
	bool const & i_vld,
	uint64_t & o_res, 
	ht_uint7 & o_htId,
	bool & o_vld,
	add_prm_state &s);
