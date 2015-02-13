/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// SyscDisp.h:
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Ht.h"

SC_MODULE(CSyscDisp) {
	sc_in<uint8_t> i_aeId;
	sc_in<uint8_t> i_unitCnt;

	sc_in<bool> i_clock1x;
	sc_in<bool> i_reset;
	sc_out<sc_uint<48> > o_dispToHif_ctlQueBase;
	sc_out<bool> o_dispToHif_dispStart;

	sc_in<sc_uint<1> > i_hifToDisp_dispBusy;

	sc_uint<48> r_ctlQueBase;
	sc_uint<4> r_dispState;

	// Constructor
	void SyscDisp();

	SC_CTOR(CSyscDisp) {
		SC_METHOD(SyscDisp);
		sensitive << i_clock1x.pos();
	}
};
