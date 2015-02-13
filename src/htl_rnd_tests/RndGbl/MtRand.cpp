/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// mtrand.cpp, see include file mtrand.h for information

#include "MtRand.h"
// non-inline function definitions and static member definitions cannot
// reside in header file because of the risk of multiple declarations

MTRand_int32 g_rndInit;
MTRand_int32 g_rndRetry;

void MTRand_int32::gen_state() { // generate new state vector
	assert(lockX);
	for (int i = 0; i < (n - m); ++i)
		stateX[i] = stateX[i + m] ^ twiddle(stateX[i], stateX[i + 1]);
	for (int i = n - m; i < (n - 1); ++i)
		stateX[i] = stateX[i + m - n] ^ twiddle(stateX[i], stateX[i + 1]);
	stateX[n - 1] = stateX[m - 1] ^ twiddle(stateX[n - 1], stateX[0]);
	pX = 0; // reset position
}

void MTRand_int32::seed(unsigned long s, bool bLock) { // init by 32 bit seed
	if (bLock) lock();
	assert(lockX);
	stateX[0] = s & 0xFFFFFFFFUL; // for > 32 bit machines
	for (int i = 1; i < n; ++i) {
		stateX[i] = 1812433253UL * (stateX[i - 1] ^ (stateX[i - 1] >> 30)) + i;
		// see Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier
		// in the previous versions, MSBs of the seed affect only MSBs of the array state
		// 2002/01/09 modified by Makoto Matsumoto
		stateX[i] &= 0xFFFFFFFFUL; // for > 32 bit machines
	}
	pX = n; // force gen_state() to be called for next random number
	if (bLock) unlock();
}

void MTRand_int32::seed(const unsigned long* array, int size, bool bLock) { // init by array
	if (bLock) lock();
	assert(lockX);
	seed(19650218UL, false);
	int i = 1, j = 0;
	for (int k = ((n > size) ? n : size); k; --k) {
		stateX[i] = (stateX[i] ^ ((stateX[i - 1] ^ (stateX[i - 1] >> 30)) * 1664525UL))
			+ array[j] + j; // non linear
		stateX[i] &= 0xFFFFFFFFUL; // for > 32 bit machines
		++j; j %= size;
		if ((++i) == n) { stateX[0] = stateX[n - 1]; i = 1; }
	}
	for (int k = n - 1; k; --k) {
		stateX[i] = (stateX[i] ^ ((stateX[i - 1] ^ (stateX[i - 1] >> 30)) * 1566083941UL)) - i;
		stateX[i] &= 0xFFFFFFFFUL; // for > 32 bit machines
		if ((++i) == n) { stateX[0] = stateX[n - 1]; i = 1; }
	}
	stateX[0] = 0x80000000UL; // MSB is 1; assuring non-zero initial array
	pX = n; // force gen_state() to be called for next random number
	if (bLock) unlock();
}
