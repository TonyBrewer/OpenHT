// -D_HTV -IC:/Users/TBrewer/Documents/llvm-project/my_tests/ht_lib -IC:/Users/TBrewer/Documents/llvm-project/my_tests/Clk2x C:/Users/TBrewer/Documents/llvm-project/my_tests/Clk2x/PersAuClk2x.cpp

/*****************************************************************************/
// Generated with htl
//
// Hybrid Thread File - PersAuClk2x.cpp
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2013 : Created by Convey Computer Corp. This file is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
#include "Ht.h"
#include "PersAuClk2x.h"

#ifdef _HTV
#include "PersAuClk2x_src.cpp"
#endif

#ifndef _HTV
char const * CPersAuClk2x::m_pInstrNames[] = {
		"CLK2X_ENTRY",
		"CLK2X_RTN",
	};
#endif

#define CLK2X_RND_RETRY false

// Hardware thread methods
void CPersAuClk2x::HtRetry()
{
	// Verify that no previous thread control routine was called
	assert(c_t3_htCtrl == HT_INVALID);
	c_t3_htCtrl = HT_RETRY;
}

void CPersAuClk2x::HtContinue(sc_uint<1> nextInst)
{
	// Verify that no previous thread control routine was called
	assert(c_t3_htCtrl == HT_INVALID);
	c_t3_htCtrl = HT_CONT;
	c_t3_htNextInstr = nextInst;
}

void CPersAuClk2x::HtTerminate()
{
	// Verify that no previous thread control routine was called
	assert(c_t3_htCtrl == HT_INVALID);
	c_t3_htCtrl = HT_TERM;
}

bool CPersAuClk2x::SendReturnBusy_clk2x()
{
	// verify thread is valid
	assert(r_t3_htValid);

#ifdef _HTV
	return r_clk2xToCtl_Clk2xRtnAvlCntZero;
#else
	return (c_clk2xToCtl_Clk2xRtnAvlCntBusy = r_clk2xToCtl_Clk2xRtnAvlCntZero)
		|| CLK2X_RND_RETRY && ((g_rndRetry() & 1) == 1);
#endif
}

void CPersAuClk2x::SendReturn_clk2x()
{
	// verify thread is valid
	assert(r_t3_htValid);

	c_clk2xToCtl_Clk2xRtnRdy = true;
	c_clk2xToCtl_Clk2xRtn.m_rtnInst = (CtlInst_t)r_t3_htPriv.m_rtnInst;
	c_t3_htCtrl = HT_RTN;
}

bool CPersAuClk2x::SendCallBusy_clk1x()
{
	// verify thread is valid
	assert(r_t3_htValid);

#ifdef _HTV
	return r_clk2xToClk1x_Clk1xCallAvlCntZero || (r_clk2xToClk1x_Clk1xCallRdy & r_phase);
#else
	return (c_clk2xToClk1x_Clk1xCallAvlCntBusy = r_clk2xToClk1x_Clk1xCallAvlCntZero) || (r_clk2xToClk1x_Clk1xCallRdy & r_phase)
		|| CLK2X_RND_RETRY && ((g_rndRetry() & 1) == 1);
#endif
}

void CPersAuClk2x::SendCall_clk1x(sc_uint<1> rtnInst)
{
	// verify thread is valid
	assert(r_t3_htValid);

	c_clk2xToClk1x_Clk1xCallRdy = true;
	c_clk2xToClk1x_Clk1xCall.m_rtnInst = rtnInst;
	c_t3_htCtrl = HT_CALL;
}

bool CPersAuClk2x::SendHostMsgBusy()
{
#ifdef _HTV
	return r_clk2xToHti_ohmAvlCnt == 0;
#else
	return (c_ts_ohmAvlCntBusy = (r_clk2xToHti_ohmAvlCnt == 0)) || CLK2X_RND_RETRY && ((g_rndRetry() & 1) == 1);
#endif
}

void CPersAuClk2x::SendHostMsg(sc_uint<7> msgType, sc_uint<56> msgData)
{
	// Verify busy routine was called for outbound host message
	assert(!c_ts_ohmAvlCntBusy);
	c_clk2xToHti_ohm.m_bValid = true;
	c_clk2xToHti_ohm.m_msgType = msgType;
	c_clk2xToHti_ohm.m_msgData = msgData;
}

void CPersAuClk2x::PersAuClk2x_1x()
{
	if (i_clk1xToClk2x_Clk1xRtnRdy.read())
		m_clk1xToClk2x_Clk1xRtnQue.push(i_clk1xToClk2x_Clk1xRtn);

	//////////////////////////////////////////////////////
	// Register assignments

	r_clk2xToHta_assert_1x = r_clk2xToHta_assert;

	r_hifToAu_ihm_1x = i_hifToAu_ihm;

	r_htiToClk2x_ohmAvl_1x = i_htiToClk2x_ohmAvl.read();
	r_clk2xToHti_ohm_1x = r_clk2xToHti_ohm_2x;
	m_clk1xToClk2x_Clk1xRtnQue.push_clock(c_reset1x);

	r_clk2xToClk1x_Clk1xCallRdy_1x = r_clk2xToClk1x_Clk1xCallRdy;
	r_clk2xToClk1x_Clk1xCall_1x = r_clk2xToClk1x_Clk1xCall;
	r_clk2xToClk1x_Clk1xRtnAvl_1x = r_clk2xToClk1x_Clk1xRtnAvlCnt_2x.read() > 0;

	ht_attrib(equivalent_register_removal, r_reset1x, "no");
	ResetFDSE(r_reset1x, i_reset.read());
	c_reset1x = r_reset1x;

	//////////////////////////////////////////////////////
	// Output assignments

	o_clk2xToHta_assert = r_clk2xToHta_assert_1x;
	o_clk2xToHti_ohm = r_clk2xToHti_ohm_1x;

	o_clk2xToClk1x_Clk1xCallRdy = r_clk2xToClk1x_Clk1xCallRdy_1x;
	o_clk2xToClk1x_Clk1xCall = r_clk2xToClk1x_Clk1xCall_1x;
	o_clk2xToClk1x_Clk1xRtnAvl = r_clk2xToClk1x_Clk1xRtnAvl_1x;

}

void CPersAuClk2x::PersAuClk2x_2x()
{
	if (i_ctlToClk2x_Clk2xCallRdy.read())
		m_ctlToClk2x_Clk2xCallQue.push(i_ctlToClk2x_Clk2xCall);
	bool c_clk2xToCtl_Clk2xCallAvl = false;
	bool c_clk2xToClk1x_Clk1xRtnAvl = false;

	// First pipe stage: ht arbitration
	sc_uint<2> c_t1_htSel = 0;
	bool c_t1_htValid = false;
	bool c_htBusy = r_htBusy;
	bool c_htCmdValid = r_htCmdValid;

	bool c_resetComplete = !c_reset1x;

	if (!m_ctlToClk2x_Clk2xCallQue.empty() && !r_htBusy && r_resetComplete) {
		c_t1_htSel = CLK2X_HT_SEL_CTL_CLK2X;
		c_t1_htValid = true;
		m_ctlToClk2x_Clk2xCallQue.pop();
		c_clk2xToCtl_Clk2xCallAvl = true;
		c_htBusy = true;
	} else if (!m_clk1xToClk2x_Clk1xRtnQue.empty() && r_resetComplete) {
		c_t1_htSel = CLK2X_HT_SEL_CLK1X_CLK1X;
		c_t1_htValid = true;
		m_clk1xToClk2x_Clk1xRtnQue.pop();
		c_clk2xToClk1x_Clk1xRtnAvl = true;
	} else if (r_htCmdValid && r_resetComplete) {
		c_t1_htSel = CLK2X_HT_SEL_SM;
		c_t1_htValid = true;
		c_htCmdValid = false;
	}
	CCtlToClk2x_Clk2xCall c_t1_ctlToClk2x_Clk2xCall = m_ctlToClk2x_Clk2xCallQue.front();
	CClk1xToClk2x_Clk1xRtn c_t1_clk1xToClk2x_Clk1xRtn = m_clk1xToClk2x_Clk1xRtnQue.front();
	CHtCmd c_t1_htCmd = r_htCmd;

	// htId select

	// htPriv / input command select
	bool c_t2_htValid = r_t2_htValid;
	sc_uint<2> c_t2_htSel = r_t2_htSel;
	CHtPriv c_t2_htPriv = r_htPriv;
	sc_uint<CLK2X_INST_W> c_t2_htInstr;

	CCtlToClk2x_Clk2xCall c_t2_ctlToClk2x_Clk2xCall = r_t2_ctlToClk2x_Clk2xCall;
	CClk1xToClk2x_Clk1xRtn c_t2_clk1xToClk2x_Clk1xRtn = r_t2_clk1xToClk2x_Clk1xRtn;
	CHtCmd c_t2_htCmd = r_t2_htCmd;

	switch (c_t2_htSel) {
	case CLK2X_HT_SEL_CTL_CLK2X:
		c_t2_htInstr = CLK2X_ENTRY;
		c_t2_htPriv = 0;
		c_t2_htPriv.m_rtnInst = (Clk2xRtnInst_t)c_t2_ctlToClk2x_Clk2xCall.m_rtnInst;
		break;
	case CLK2X_HT_SEL_CLK1X_CLK1X:
		c_t2_htInstr = c_t2_clk1xToClk2x_Clk1xRtn.m_rtnInst;
		break;
	case CLK2X_HT_SEL_SM:
		c_t2_htInstr = c_t2_htCmd.m_htInstr;
		break;
	default:
		assert(0);
	}

	// Instruction Stage #1
	c_t3_htPriv = r_t3_htPriv;

	c_t3_htCtrl = HT_INVALID;
	c_t3_htNextInstr = r_t3_htInstr;

	c_msg = r_msg;

	#ifndef _HTV
	c_clk2xToClk1x_Clk1xCallAvlCntBusy = true;
	c_clk2xToCtl_Clk2xRtnAvlCntBusy = true;
	c_ts_ohmAvlCntBusy = true;
	#endif

	c_clk2xToClk1x_Clk1xCallRdy = false;
	c_clk2xToClk1x_Clk1xCall = r_clk2xToClk1x_Clk1xCall;

	c_clk2xToCtl_Clk2xRtnRdy = false;
	c_clk2xToCtl_Clk2xRtn = 0;

	c_clk2xToHti_ohm.m_data64 = 0;

	c_htAssert = 0;
#ifdef HT_ASSERT
	if (r_clk2xToHta_assert.read().m_bAssert && r_phase && !c_reset1x)
		c_htAssert = r_clk2xToHta_assert;
#endif

	PersAuClk2x();

	// Verify that thread control routine was called
	assert(!r_t3_htValid || c_t3_htCtrl != HT_INVALID);

	#ifndef _HTV
	if (!r_t3_htValid) {
		htMon.m_clk2xValidCnt += 1;
		htMon.m_clk2xInstValidCnt[(int)r_t3_htInstr] += 1;
	}
	if (c_t3_htCtrl == HT_RETRY) {
		htMon.m_clk2xRetryCnt += 1;
		htMon.m_clk2xInstRetryCnt[(int)r_t3_htInstr] += 1;
	}
	htMon.m_clk2xBusyCnt += r_htBusy ? 1 : 0;
	#endif

	// verify that users called busy routine for call/transfer/return
	assert(!c_clk2xToClk1x_Clk1xCallRdy || !c_clk2xToClk1x_Clk1xCallAvlCntBusy);
	assert(!c_clk2xToCtl_Clk2xRtnRdy || !c_clk2xToCtl_Clk2xRtnAvlCntBusy);
	// verify that users called busy routine for outbound host message
	assert(!c_clk2xToHti_ohm.m_bValid || !c_ts_ohmAvlCntBusy);

	CHtPriv c_htPriv = r_htPriv;
	CHtCmd c_htCmd = r_htCmd;

	switch (r_t3_htSel) {
	case CLK2X_HT_SEL_CTL_CLK2X:
	case CLK2X_HT_SEL_CLK1X_CLK1X:
	case CLK2X_HT_SEL_SM:
		if (r_t3_htValid)
			c_htPriv = c_t3_htPriv;
		if (r_t3_htValid && c_t3_htCtrl != HT_JOIN) {
			c_htCmdValid = c_t3_htCtrl == HT_CONT || c_t3_htCtrl == HT_JOIN_AND_CONT || c_t3_htCtrl == HT_RETRY;
			c_htCmd.m_htInstr = c_t3_htNextInstr;
		}
		break;
	default:
		assert(0);
	}

	if (r_clk2xToCtl_Clk2xRtnRdy)
		c_htBusy = false;

	sc_uint<6> c_clk2xToClk1x_Clk1xCallAvlCnt = r_clk2xToClk1x_Clk1xCallAvlCnt
		+ (r_phase & i_clk1xToClk2x_Clk1xCallAvl.read()) - c_clk2xToClk1x_Clk1xCallRdy;
	bool c_clk2xToClk1x_Clk1xCallAvlCntZero = c_clk2xToClk1x_Clk1xCallAvlCnt == 0;
	bool c_clk2xToClk1x_Clk1xCallHold = r_clk2xToClk1x_Clk1xCallRdy & r_phase;
	c_clk2xToClk1x_Clk1xCallRdy |= c_clk2xToClk1x_Clk1xCallHold;
	sc_uint<6> c_clk2xToCtl_Clk2xRtnAvlCnt = r_clk2xToCtl_Clk2xRtnAvlCnt
		+ i_ctlToClk2x_Clk2xRtnAvl.read() - c_clk2xToCtl_Clk2xRtnRdy;
	bool c_clk2xToCtl_Clk2xRtnAvlCntZero = c_clk2xToCtl_Clk2xRtnAvlCnt == 0;

	if (r_hifToAu_ihm_1x.read().m_bValid && r_phase) {
		switch (r_hifToAu_ihm_1x.read().m_msgType) {
			case IHM_MSG2X:
				c_msg = (ht_uint9)(r_hifToAu_ihm_1x.read().m_msgData >> 0);
				break;
			default:
				break;
		}
	}

	c_clk2xToHti_ohmAvlCnt = r_clk2xToHti_ohmAvlCnt;
	if ((r_htiToClk2x_ohmAvl_1x.read() & r_phase) != c_clk2xToHti_ohm.m_bValid)
		c_clk2xToHti_ohmAvlCnt ^= 1u;

	if (r_clk2xToHti_ohm_2x.read().m_bValid && r_phase && !c_reset1x)
		c_clk2xToHti_ohm = r_clk2xToHti_ohm_2x;

	sc_uint<6> c_clk2xToClk1x_Clk1xRtnAvlCnt_2x = r_clk2xToClk1x_Clk1xRtnAvlCnt_2x.read() + c_clk2xToClk1x_Clk1xRtnAvl
		- (r_clk2xToClk1x_Clk1xRtnAvlCnt_2x.read() > 0 && !r_phase);
	//////////////////////////////////////////////////////
	// Register assignments

	r_resetComplete = !c_reset1x && c_resetComplete;

	r_t2_htSel = c_t1_htSel;
	r_t2_ctlToClk2x_Clk2xCall = c_t1_ctlToClk2x_Clk2xCall;
	r_t2_clk1xToClk2x_Clk1xRtn = c_t1_clk1xToClk2x_Clk1xRtn;
	r_t2_htValid = !c_reset1x && c_t1_htValid;
	r_t2_htCmd = c_t1_htCmd;

	r_t3_htValid = !c_reset1x && c_t2_htValid;
	r_t3_htSel = c_t2_htSel;
	r_t3_htPriv = c_t2_htPriv;
	r_t3_htInstr = c_t2_htInstr;

	// Instruction Stage #1

	r_msg = c_msg;

	r_htCmd = c_htCmd;
	r_clk2xToHta_assert = c_htAssert;
	r_htCmdValid = !c_reset1x && c_htCmdValid;
	r_htPriv = c_htPriv;
	r_htBusy = !c_reset1x && c_htBusy;

	// t1 select stage
	r_clk2xToCtl_Clk2xCallAvl = !c_reset1x && c_clk2xToCtl_Clk2xCallAvl;

	r_clk2xToClk1x_Clk1xCall = c_clk2xToClk1x_Clk1xCall;
	r_clk2xToClk1x_Clk1xCallRdy = !c_reset1x && c_clk2xToClk1x_Clk1xCallRdy;
	r_clk2xToClk1x_Clk1xCallAvlCnt = c_reset1x ? (sc_uint<6>)32 : c_clk2xToClk1x_Clk1xCallAvlCnt;
	r_clk2xToClk1x_Clk1xCallAvlCntZero = !c_reset1x && c_clk2xToClk1x_Clk1xCallAvlCntZero;

	r_clk2xToCtl_Clk2xRtn = c_clk2xToCtl_Clk2xRtn;
	r_clk2xToCtl_Clk2xRtnRdy = !c_reset1x && c_clk2xToCtl_Clk2xRtnRdy;
	r_clk2xToCtl_Clk2xRtnAvlCnt = c_reset1x ? (sc_uint<6>)32 : c_clk2xToCtl_Clk2xRtnAvlCnt;
	r_clk2xToCtl_Clk2xRtnAvlCntZero = !c_reset1x && c_clk2xToCtl_Clk2xRtnAvlCntZero;

	r_clk2xToClk1x_Clk1xRtnAvlCnt_2x = c_reset1x ? (sc_uint<6>) 0 : c_clk2xToClk1x_Clk1xRtnAvlCnt_2x;

	r_clk2xToHti_ohm_2x = c_clk2xToHti_ohm;
	r_clk2xToHti_ohmAvlCnt = c_reset1x ? (sc_uint<1>)1 : c_clk2xToHti_ohmAvlCnt;

	m_ctlToClk2x_Clk2xCallQue.clock(c_reset1x);
	m_clk1xToClk2x_Clk1xRtnQue.pop_clock(c_reset1x);

	ht_attrib(equivalent_register_removal, r_phase, "no");
	r_phase = c_reset1x.read() || !r_phase;

	//////////////////////////////////////////////////////
	// Output assignments

	o_clk2xToCtl_Clk2xCallAvl = r_clk2xToCtl_Clk2xCallAvl;
	o_clk2xToCtl_Clk2xRtnRdy = r_clk2xToCtl_Clk2xRtnRdy;
	o_clk2xToCtl_Clk2xRtn = r_clk2xToCtl_Clk2xRtn;

}
