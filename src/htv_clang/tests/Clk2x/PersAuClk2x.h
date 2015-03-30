/*****************************************************************************/
// Generated with htl
//
// Hybrid Thread File - PersAuClk2x.h
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2013 : Created by Convey Computer Corp. This file is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
#pragma once

#include "PersAuCommon.h"


#ifndef _HTV
extern MTRand_int32 g_rndRetry;
extern MTRand_int32 g_rndInit;
#endif

#define CLK2X_HT_SEL_CTL_CLK2X 0
#define CLK2X_HT_SEL_CLK1X_CLK1X 1
#define CLK2X_HT_SEL_SM 2

//////////////////////////////////
// Instruction defines

#define CLK2X_ENTRY		0u
#define CLK2X_RTN		1u

//////////////////////////////////
// Instruction defines

#if !defined(TOPOLOGY_HEADER)
#define GR_htReset c_reset1x

#define PR_htInst r_t3_htInstr

#define PR_htValid r_t3_htValid
#define PR_rtnInst r_t3_htPriv.m_rtnInst
#define P_rtnInst c_t3_htPriv.m_rtnInst

#define S_msg c_msg
#define SR_msg r_msg
#endif

#pragma unroll

SC_MODULE(CPersAuClk2x) {

	ht_attrib(keep_hierarchy, CPersAuClk2x, "true");

	//////////////////////////////////
	// Module clk2x typedefs


	//////////////////////////////////
	// Module clk2x structs and unions


	sc_in<bool> i_clock1x;
	sc_in<bool> ht_noload i_clock2x;
	sc_in<bool> i_reset;

	// HT assert interface
	sc_out<CHtAssertIntf> o_clk2xToHta_assert;

	// Inbound host message interface
	sc_in<CHostCtrlMsgIntf> i_hifToAu_ihm;

	// outbound host message interface
	sc_out<CHostCtrlMsgIntf> o_clk2xToHti_ohm;
	sc_in<bool> i_htiToClk2x_ohmAvl;

	// Inbound call interface from ctl
	sc_in<bool> i_ctlToClk2x_Clk2xCallRdy;
	sc_in<CCtlToClk2x_Clk2xCallIntf> i_ctlToClk2x_Clk2xCall;
	sc_out<bool> o_clk2xToCtl_Clk2xCallAvl;

	// Outbound call interace to clk1x
	sc_out<bool> o_clk2xToClk1x_Clk1xCallRdy;
	sc_out<CClk2xToClk1x_Clk1xCallIntf> o_clk2xToClk1x_Clk1xCall;
	sc_in<bool> i_clk1xToClk2x_Clk1xCallAvl;

	// Outbound return interace to ctl
	sc_out<bool> o_clk2xToCtl_Clk2xRtnRdy;
	sc_out<CClk2xToCtl_Clk2xRtnIntf> o_clk2xToCtl_Clk2xRtn;
	sc_in<bool> i_ctlToClk2x_Clk2xRtnAvl;

	// Inbound return interface from clk1x
	sc_in<bool> i_clk1xToClk2x_Clk1xRtnRdy;
	sc_in<CClk1xToClk2x_Clk1xRtnIntf> i_clk1xToClk2x_Clk1xRtn;
	sc_out<bool> o_clk2xToClk1x_Clk1xRtnAvl;

	////////////////////////////////////////////
	// Internal State

	sc_signal<CHostCtrlMsgIntf> r_hifToAu_ihm_1x;

	CHostCtrlMsgIntf r_clk2xToHti_ohm_1x;
	sc_signal<CHostCtrlMsgIntf> r_clk2xToHti_ohm_2x;
	sc_signal<bool> r_htiToClk2x_ohmAvl_1x;
	bool r_clk2xToHti_ohmAvlCnt;

	CHostCtrlMsgIntf c_clk2xToHti_ohm;
	sc_uint<1> c_clk2xToHti_ohmAvlCnt;
	ht_dist_que<CCtlToClk2x_Clk2xCall, 5> m_ctlToClk2x_Clk2xCallQue;
	ht_dist_que<CClk1xToClk2x_Clk1xRtn, 5> m_clk1xToClk2x_Clk1xRtnQue;
	bool c_clk2xToClk1x_Clk1xCallRdy;
	CClk2xToClk1x_Clk1xCallIntf c_clk2xToClk1x_Clk1xCall;
	bool c_clk2xToCtl_Clk2xRtnRdy;
	CClk2xToCtl_Clk2xRtnIntf c_clk2xToCtl_Clk2xRtn;
	bool r_clk2xToCtl_Clk2xCallAvl;

	sc_signal<bool> r_clk2xToClk1x_Clk1xCallRdy;
	bool r_clk2xToClk1x_Clk1xCallRdy_1x;
	sc_uint<6> r_clk2xToClk1x_Clk1xCallAvlCnt;
	bool r_clk2xToClk1x_Clk1xCallAvlCntZero;

	bool r_clk2xToCtl_Clk2xRtnRdy;
	sc_uint<6> r_clk2xToCtl_Clk2xRtnAvlCnt;
	bool r_clk2xToCtl_Clk2xRtnAvlCntZero;

	bool r_clk2xToClk1x_Clk1xRtnAvl_1x;
	sc_signal<sc_uint<6> > r_clk2xToClk1x_Clk1xRtnAvlCnt_2x;

	sc_signal<CClk2xToClk1x_Clk1xCallIntf> r_clk2xToClk1x_Clk1xCall;
	CClk2xToClk1x_Clk1xCallIntf r_clk2xToClk1x_Clk1xCall_1x;
	CClk2xToCtl_Clk2xRtnIntf r_clk2xToCtl_Clk2xRtn;

	struct CHtCmd {
		//#ifndef _HTV
		CHtCmd() {} // avoid run-time uninitialized error
		//#endif

		uint32_t		m_htInstr:1;
	};
	struct CHtPriv {
		CHtPriv() {} // avoid uninitialized error
		void operator = (int zero) {
			//assert(zero == 0);
			m_rtnInst = 0;
		}
		bool operator == (const CHtPriv & rhs) const {
			return 	m_rtnInst == rhs.m_rtnInst;
		}

		Clk2xRtnInst_t m_rtnInst;
	};

	CHtCmd r_htCmd;
	CHtPriv r_htPriv;

	bool r_resetComplete;

	sc_uint<2> r_t2_htSel;
	CCtlToClk2x_Clk2xCall r_t2_ctlToClk2x_Clk2xCall;
	CClk1xToClk2x_Clk1xRtn r_t2_clk1xToClk2x_Clk1xRtn;
	bool r_t2_htValid;
	CHtCmd r_t2_htCmd;

	bool r_t3_htValid;
	sc_uint<2> r_t3_htSel;
	CHtPriv r_t3_htPriv;
	sc_uint<CLK2X_INST_W> r_t3_htInstr;

	// Instruction Stage #1
	CHtPriv c_t3_htPriv;

	sc_uint<4> c_t3_htCtrl;
	sc_uint<CLK2X_INST_W> c_t3_htNextInstr;

	ht_uint9 c_msg;
	ht_uint9 r_msg;

	#ifndef _HTV
	bool c_clk2xToClk1x_Clk1xCallAvlCntBusy;
	bool c_clk2xToCtl_Clk2xRtnAvlCntBusy;
	bool c_ts_ohmAvlCntBusy;
	#endif
	CHtAssertIntf c_htAssert;
	bool r_htBusy;
	bool r_htCmdValid;
	sc_signal<CHtAssertIntf> r_clk2xToHta_assert;
	CHtAssertIntf r_clk2xToHta_assert_1x;

	bool r_phase;
	bool r_reset1x;
	sc_signal<bool> c_reset1x;

	////////////////////////////////////////////
	// Methods

	void PersAuClk2x();
	void PersAuClk2x_1x();
	void PersAuClk2x_2x();

	void HtRetry();
	void HtContinue(sc_uint<1> nextInst);
	void HtTerminate();
	bool SendReturnBusy_clk2x();
	void SendReturn_clk2x();
	bool SendCallBusy_clk1x();
	void SendCall_clk1x(sc_uint<1> rtnInst);
	bool SendHostMsgBusy();
	void SendHostMsg(sc_uint<7> msgType, sc_uint<56> msgData);

	#ifndef _HTV
	static char const * m_pInstrNames[];
	#endif

	SC_CTOR(CPersAuClk2x) {
		SC_METHOD(PersAuClk2x_1x);
		sensitive << i_clock1x.pos();

		SC_METHOD(PersAuClk2x_2x);
		sensitive << i_clock2x.pos();

		#ifndef _HTV
		c_reset1x = true;
		r_t3_htValid = false;
		#endif
	}

	#ifndef _HTV
	void end_of_simulation() {
		assert(r_clk2xToClk1x_Clk1xCallAvlCnt == 32);
		assert(r_clk2xToCtl_Clk2xRtnAvlCnt == 32);
		assert(r_clk2xToHti_ohmAvlCnt == 1);
	}
	#endif
};
