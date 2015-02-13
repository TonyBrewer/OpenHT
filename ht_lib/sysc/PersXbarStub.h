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

SC_MODULE(CPersXbarStub) {

	// Memory Interface
	sc_out<bool> o_stubToXbar_reqRdy;
	sc_out<CMemRdWrReqIntf> o_stubToXbar_req;
	sc_in<bool> ht_noload i_xbarToStub_reqFull;

	sc_in<bool> ht_noload i_xbarToStub_rdRspRdy;
	sc_in<CMemRdRspIntf> ht_noload i_xbarToStub_rdRsp;
	sc_out<bool> o_stubToXbar_rdRspFull;

	sc_in<bool> ht_noload i_xbarToStub_wrRspRdy;
	sc_in<sc_uint<MIF_TID_W> > ht_noload i_xbarToStub_wrRspTid;
	sc_out<bool> o_stubToXbar_wrRspFull;

	void PersXbarStub();

	SC_CTOR(CPersXbarStub) {
		SC_METHOD(PersXbarStub);
	}
};
