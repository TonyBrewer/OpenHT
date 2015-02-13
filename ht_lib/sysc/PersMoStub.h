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

SC_MODULE(CPersMoStub) {

	sc_out<bool> o_mo_valid;
	sc_out<uint32_t> o_mo_data;
	sc_in<bool> ht_noload i_mo_stall;

	void PersMoStub();

	SC_CTOR(CPersMoStub) {
		SC_METHOD(PersMoStub);
	}
};
