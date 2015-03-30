/*****************************************************************************/
// Generated with htl
//
// Hybrid Thread File - PersAuHta.cpp
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2013 : Created by Convey Computer Corp. This file is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
#include "Ht.h"
#include "PersAuHta.h"

void
CPersAuHta::PersAuHta1x()
{

	/////////////////////////
	// Combinatorial Logic

	CHtAssertIntf c_htaToHif_assert;

	c_htaToHif_assert = CHtAssertIntf();

	c_htaToHif_assert.m_module = 0;
	if (r_ctlToHta_assert.m_bAssert) c_htaToHif_assert.m_module |= 0x0;
	if (r_clk2xToHta_assert.m_bAssert) c_htaToHif_assert.m_module |= 0x1;
	if (r_clk1xToHta_assert.m_bAssert) c_htaToHif_assert.m_module |= 0x2;

	c_htaToHif_assert.m_info = 0;
	c_htaToHif_assert.m_info |= r_ctlToHta_assert.m_info;
	c_htaToHif_assert.m_info |= r_clk2xToHta_assert.m_info;
	c_htaToHif_assert.m_info |= r_clk1xToHta_assert.m_info;

	c_htaToHif_assert.m_lineNum = 0;
	c_htaToHif_assert.m_lineNum |= r_ctlToHta_assert.m_lineNum;
	c_htaToHif_assert.m_lineNum |= r_clk2xToHta_assert.m_lineNum;
	c_htaToHif_assert.m_lineNum |= r_clk1xToHta_assert.m_lineNum;

	bool c_bAssert_1_0 = r_ctlToHta_assert.m_bAssert || r_clk2xToHta_assert.m_bAssert;
	bool c_bCollision_1_0 = r_ctlToHta_assert.m_bAssert && r_clk2xToHta_assert.m_bAssert;
	bool c_bAssert_1_1 = r_clk1xToHta_assert.m_bAssert;
	bool c_bCollision_1_1 = r_clk1xToHta_assert.m_bAssert;

	bool c_bAssert_2_0 = c_bAssert_1_0 || c_bAssert_1_1;
	bool c_bCollision_2_0 = c_bCollision_1_0 || c_bCollision_1_1 || c_bAssert_1_0 && c_bAssert_1_1;

	c_htaToHif_assert.m_bAssert = c_bAssert_2_0;
	c_htaToHif_assert.m_bCollision = c_bCollision_2_0;

	/////////////////////////
	// Register assignments

	r_ctlToHta_assert = i_ctlToHta_assert;
	r_clk2xToHta_assert = i_clk2xToHta_assert;
	r_clk1xToHta_assert = i_clk1xToHta_assert;
	r_htaToHif_assert = c_htaToHif_assert;

	/////////////////////////
	// Output assignments

	o_htaToHif_assert = r_htaToHif_assert;
}
