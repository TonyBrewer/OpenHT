#pragma once

// used in black box and main (to check results)
typedef uint64_t Arith_t;

void BlackBox_execute(const int &elem, const int &arc0, const Arith_t &va,
	const Arith_t &vb, const Arith_t &scalar, Arith_t &vt, bool &vm);
