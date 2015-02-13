/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include "Ht.h"

SC_MODULE(CPersUnitCnt) {
	sc_in<bool> ht_noload i_clock2x;
	sc_in<sc_uint<1> > i_hifToDisp_busy[HT_UNIT_CNT];
	sc_out<uint8_t> o_unitCnt;
	sc_out<sc_uint<1> > o_hifToDisp_dispBusy;

	void PersUnitCnt();

	SC_CTOR(CPersUnitCnt) {
		SC_METHOD(PersUnitCnt);
		for (int i=0; i<HT_UNIT_CNT; i += 1) {
			sensitive << i_hifToDisp_busy[i];
		}
	}
};
