/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// SyscMem.h
//
//////////////////////////////////////////////////////////////////////
#pragma once

#ifndef _HTV
namespace Ht {
	struct CSyscMemLib;
}
#endif

SC_MODULE(CSyscMem) {
	sc_in<bool> i_clock1x;
	sc_in<bool> i_reset;

	// Interface to PDK Xbar
	sc_in<bool> i_mifToXbar_reqRdy;
	sc_in<CMemRdWrReqIntf> i_mifToXbar_req;
	sc_out<bool> o_xbarToMif_reqFull;

	sc_out<bool> o_xbarToMif_rdRspRdy;
	sc_out<CMemRdRspIntf> o_xbarToMif_rdRsp;
	sc_in<bool> i_mifToXbar_rdRspFull;

	sc_out<bool> o_xbarToMif_wrRspRdy;
	sc_out<sc_uint<MIF_TID_W> > o_xbarToMif_wrRspTid;
	sc_in<bool> i_mifToXbar_wrRspFull;

	// Register declarations 
	//////////////////////////////

#	ifndef _HTV
	struct Ht::CSyscMemLib * m_pLib;
#	endif

	void SyscMem();

	// Constructor
	SC_CTOR(CSyscMem) {
		SC_METHOD(SyscMem);
		sensitive << i_clock1x.pos();

#		ifndef _HTV
		m_pLib = 0;
#		endif
	}
};
