#pragma once

// state for add clocked primitive
ht_state struct add_prm_state {
	uint64_t	res[5];
	ht_uint9	htId[5];
	bool    	vld[5];
};

// 32-bit add, clocked primitive
//   function definition located in .cpp file
ht_prim ht_clk("ck") void add_wrap (
	bool const & rst,
	uint64_t const & i_a, 
	uint64_t const & i_b, 
	ht_uint9 const & i_htId,
	bool const & i_vld,
        bool & o_rdy, 
	uint64_t & o_res, 
	ht_uint9 & o_htId,
	bool & o_vld,
	add_prm_state &s);
