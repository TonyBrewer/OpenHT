/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// SyscClock.h: Clock Generator
//
//////////////////////////////////////////////////////////////////////

#pragma once

#ifndef _HTV
#define HT_CYCLE() ((long long)sc_time_stamp().value() / 10000)
#endif

SC_MODULE(CSyscClock) {
	sc_out<bool> o_clock2x;
	sc_out<bool> o_clock1x;
	sc_out<bool> o_clockhx;
	sc_out<bool> o_reset;

#	ifndef _HTV
	sc_clock *m_pClock;
	sc_uint<1> r_phase1;
	sc_uint<2> r_phase2;
	sc_uint<3> r_reset;
#	endif

	void Clock();
	void start_of_simulation();

	// Constructor
	SC_CTOR(CSyscClock) {
#		ifndef _HTV
		r_phase1 = 0;
		r_phase2 = 0;
		r_reset = 1;

		m_pClock = new sc_clock("clock", sc_time(5.0, SC_NS), 0.5, SC_ZERO_TIME, true);

		SC_METHOD(Clock);
		sensitive << *m_pClock;
#		endif // _HTV
	}
};
