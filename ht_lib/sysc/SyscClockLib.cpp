/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#define HT_LIB_SYSC
#include "Ht.h"
#include "SyscClock.h"

void CSyscClock::Clock()
{
	if (*m_pClock) {
		r_phase1 = ~r_phase1;
		r_phase2 = r_phase2 + 1;
		if (r_phase1 == 1)
			r_reset = r_reset + ((r_reset < 7) ? 1 : 0);
	}

	o_clock2x = *m_pClock;
	o_clock1x = r_phase1 == 1;
	o_clockhx = r_phase2 == 3;
	o_reset = r_reset < 7;
}

void CSyscClock::start_of_simulation() {
	o_reset = true;
}
