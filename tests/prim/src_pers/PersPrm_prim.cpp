#include "Ht.h"
#include "PersPrm_prim.h"

ht_prim ht_clk("intfClk1x") void add16_prm(
	ht_uint16 const &	a,
	ht_uint16 const &	b,
	ht_uint16 &		c,
	add16_prm_state &	s)
{
#ifndef _HTV
	// perform last stage first for proper staging
	c = s.m_a + s.m_b;

	// now first stage
	s.m_a = (uint16_t)a;
	s.m_b = (uint16_t)b;
#endif
}
