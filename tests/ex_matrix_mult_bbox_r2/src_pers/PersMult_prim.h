#pragma once

// state for add clocked primitive
ht_state struct mult_prm_state {
	uint64_t	p[5];
};

// 32-bit mult, clocked primitive
//   function definition located in .cpp file
ht_prim ht_clk("clk") void multiplier (
	uint32_t const & a, 
	uint32_t const & b, 
	uint64_t & p, 
	mult_prm_state &s);
