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

SC_MODULE(CPersMiStub) {

	sc_in<bool> ht_noload i_mi_valid;
	sc_in<uint32_t> ht_noload i_mi_data;
	sc_out<bool> o_mi_stall;

	void PersMiStub();

	SC_CTOR(CPersMiStub) {
		SC_METHOD(PersMiStub);
	}
};
