#include "Ht.h"
#include "PersAdd_prim.h"

ht_prim ht_clk("ck") void addC (
	uint32_t const &	i_a,
	uint32_t const &	i_b,
	uint32_t &		o_res,
	addCState &	s)
{
#ifndef _HTV

	// Inputs a, b are valid on the initial input stage - (Stage 1)
	// Output p is valid 1 cycle later - (Stage 2)

	// p is valid on Stage 2
	o_res = s.res;

	// a, b, are all valid on the initial stage - (Stage 1)
	s.res = i_a + i_b;

#endif
}

ht_prim void addP (
	uint32_t const &	i_a,
	uint32_t const &	i_b,
	uint32_t &		o_res)
{
#ifndef _HTV

	// Inputs a, b are valid on the initial input stage - (Stage 1)
	// Output p is valid on input stage

	// a and b, are valid on the initial stage - (Stage 1)
	o_res = i_a + i_b;

#endif
}

ht_prim void addS (
	AddIn const & i_in, 
	AddOut & o_out)
{
#ifndef _HTV
	o_out.res = i_in.a + i_in.b;
#endif
}
