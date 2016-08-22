#pragma once

// state for add clocked primitive
ht_state struct muladd_prm_state {
	uint64_t	res[19];
	ht_uint7	htId[19];
	bool    	vld[19];
};

// 64-bit muladd, clocked primitive
//   function definition located in .cpp file
ht_prim ht_clk("ck") void muladd_wrap (
	uint64_t const & i_a, 
	uint64_t const & i_b, 
	uint64_t const & i_c, 
	ht_uint7 const & i_htId,
	bool const & i_vld,
	uint64_t & o_res, 
	ht_uint7 & o_htId,
	bool & o_vld,
	muladd_prm_state &s);
