#pragma once

// state for add clocked primitive
ht_state struct mult_prm_state {
	int64_t		p[5];
};

// 32-bit mult, clocked primitive
//   function definition located in .cpp file
ht_prim ht_clk("clk") void multiplier (
	int32_t const & a, 
	int32_t const & b, 
	int64_t & p, 
	mult_prm_state &s);
