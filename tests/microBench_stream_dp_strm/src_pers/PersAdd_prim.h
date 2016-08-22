#pragma once

// state for add clocked primitive
ht_state struct add_prm_state {
	uint64_t	res[5];
	bool    	vld[5];
};

// 32-bit add, clocked primitive
//   function definition located in .cpp file
ht_prim ht_clk("ck") void add_wrap (
	bool const & rst,
	uint64_t const & i_a, 
	uint64_t const & i_b, 
	bool const & i_vld,
	bool & o_rdy,
	uint64_t & o_res, 
	bool & o_vld,
	add_prm_state &s);
