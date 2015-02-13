#pragma once

// state for add clocked primitive
ht_state struct add_prm_state {
	uint64_t	res[6];
};

// 16-bit add, clocked primitive
//   function definition located in .cpp file
ht_prim ht_clk("ck") void add_5stage (
	uint64_t const & t1_a, 
	uint64_t const & t1_b, 
	uint64_t & t6_res, 
	add_prm_state &s);
