/*****************************************************************************/
// Generated with htl
//
// Hybrid Thread File - PersAuHta.h
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2013 : Created by Convey Computer Corp. This file is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
#pragma once

#include "PersAuCommon.h"

SC_MODULE(CPersAuHta) {

	ht_attrib(keep_hierarchy, CPersAuHta, "true");
	sc_in<bool> i_clock1x;
	sc_in<CHtAssertIntf> i_ctlToHta_assert;
	sc_in<CHtAssertIntf> i_clk2xToHta_assert;
	sc_in<CHtAssertIntf> i_clk1xToHta_assert;
	sc_out<CHtAssertIntf> o_htaToHif_assert;

	////////////////////////////////////////////
	// Register state

	CHtAssertIntf r_ctlToHta_assert;
	CHtAssertIntf r_clk2xToHta_assert;
	CHtAssertIntf r_clk1xToHta_assert;
	CHtAssertIntf r_htaToHif_assert;

	////////////////////////////////////////////
	// Methods

	void PersAuHta1x();

	SC_CTOR(CPersAuHta) {
		SC_METHOD(PersAuHta1x);
		sensitive << i_clock1x.pos();
	}
};
