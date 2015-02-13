#pragma once


// 8-bit adder, non-clocked primitive
ht_prim inline void add8_prm(
	ht_uint8 const		a,
	ht_uint8 const		b,
	ht_uint1 const &	cyin,
	ht_uint8 &		c,
	ht_uint1 &		cyout)
{
#ifndef _HTV
	// function definition located in .h file as an inlined function
	//  this is only needed for SystemC simulation
	ht_uint9 t = a + b + cyin;
	c = t(7, 0);
	cyout = t(8, 8);
#endif
}

// state for add16 clocked primitive
ht_state struct add16_prm_state {
	uint64_t	m_bad[1];
	uint16_t	m_a;
	uint16_t	m_b;
};

// 16-bit add, clocked primitive
//   function definition located in .cpp file
ht_prim ht_clk("intfClk1x") void add16_prm(
	ht_uint16 const & a, 
	ht_uint16 const & b, 
	ht_uint16 & c, 
	add16_prm_state &s);
