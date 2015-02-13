#include "Ht.h"
#include "PersPipe_prim.h"
#include "PersPipe.h"
#include "BlackBox_execute.h"

void
bbox_wrap(
	bool &			vm,
	uint64_t &		vt,
	bool const &		reset,
	uint64_t const &	scalar,
	ht_uint12 const &	arc0,
	uint64_t const &	va,
	uint64_t const &	vb,
	bbox_prim_state &	s
	)
{
	bool c_vm0;
	uint64_t c_vt0;

	BlackBox_execute(0, (int)arc0, va, vb, scalar, c_vt0, c_vm0);

#ifndef _HTV

	// stage the results for 25 cycles to match the verilog
	// state must be passed in because all local state is lost on each return
	int i;
	for (i = 24; i > 0; i--) {
		s.r_vm[i] = s.r_vm[i - 1];
		s.r_vt[i] = s.r_vt[i - 1];
	}
	s.r_vm[0] = c_vm0;
	s.r_vt[0] = c_vt0;
	vm = s.r_vm[24];
	vt = s.r_vt[24];
#endif
}
