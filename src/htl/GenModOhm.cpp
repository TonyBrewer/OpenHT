/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "CnyHt.h"
#include "DsnInfo.h"

// Generate a module's outbound host message (ohm) interface

void CDsnInfo::InitAndValidateModOhm()
{
	// Internal ram checks
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed || !mod.m_ohm.m_bOutHostMsg) continue;

		if (mod.m_modInstList.size() > 1)
			ParseMsg(Error, mod.m_ohm.m_lineInfo, "Outbound host message from replicated/multi-instance module not currently supported");
	}
}

void CDsnInfo::GenModOhmStatements(CModule &mod)
{
	if (!mod.m_ohm.m_bOutHostMsg)
		return;

	CHtCode	& ohmPreInstr = mod.m_clkRate == eClk1x ? m_ohmPreInstr1x : m_ohmPreInstr2x;
	CHtCode	& ohmPostInstr = mod.m_clkRate == eClk1x ? m_ohmPostInstr1x : m_ohmPostInstr2x;
	CHtCode	& ohmReg = mod.m_clkRate == eClk1x ? m_ohmReg1x : m_ohmReg2x;
	CHtCode	& ohmPostReg = mod.m_clkRate == eClk1x ? m_ohmPostReg1x : m_ohmPostReg2x;

	// Generate I/O declartions
	m_ohmIoDecl.Append("\t// outbound host message interface\n");
	m_ohmIoDecl.Append("\tsc_out<CHostCtrlMsgIntf> o_%sToHti_ohm;\n", mod.m_modName.Lc().c_str());
	m_ohmIoDecl.Append("\tsc_in<bool> i_htiTo%s_ohmAvl;\n", mod.m_modName.Uc().c_str());
	m_ohmIoDecl.Append("\n");

	// Generate register declarations
	if (mod.m_clkRate == eClk1x)
		m_ohmRegDecl.Append("\tCHostCtrlMsgIntf r_%sToHti_ohm;\n", mod.m_modName.Lc().c_str());
	else {
		m_ohmRegDecl.Append("\tCHostCtrlMsgIntf r_%sToHti_ohm_1x;\n", mod.m_modName.Lc().c_str());
		m_ohmRegDecl.Append("\tsc_signal<CHostCtrlMsgIntf> r_%sToHti_ohm_2x;\n", mod.m_modName.Lc().c_str());
		m_ohmRegDecl.Append("\tsc_signal<bool> r_htiTo%s_ohmAvl_1x;\n", mod.m_modName.Uc().c_str());
	}
	m_ohmRegDecl.Append("\tht_uint1 r_%sToHti_ohmAvlCnt;\n", mod.m_modName.Lc().c_str());
	m_ohmRegDecl.Append("\n");

	m_ohmAvlCntChk.Append("\t\tassert(r_%sToHti_ohmAvlCnt == 1);\n", mod.m_modName.Lc().c_str());

	// Generate pre-instruction statements
	m_ohmRegDecl.Append("\tCHostCtrlMsgIntf c_%sToHti_ohm;\n", mod.m_modName.Lc().c_str());
	ohmPreInstr.Append("\tc_%sToHti_ohm.m_data64 = 0;\n", mod.m_modName.Lc().c_str());
	ohmPreInstr.Append("\n");

	// Generate post-instruction statements
	m_ohmRegDecl.Append("\tht_uint1 c_%sToHti_ohmAvlCnt;\n", mod.m_modName.Lc().c_str());
	ohmPostInstr.Append("\tc_%sToHti_ohmAvlCnt = r_%sToHti_ohmAvlCnt;\n", mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());
	if (mod.m_clkRate == eClk1x)
		ohmPostInstr.Append("\tif (i_htiTo%s_ohmAvl.read() != c_%sToHti_ohm.m_bValid)\n", mod.m_modName.Uc().c_str(), mod.m_modName.Lc().c_str());
	else
		ohmPostInstr.Append("\tif ((bool)(r_htiTo%s_ohmAvl_1x.read() & r_phase) != c_%sToHti_ohm.m_bValid)\n", mod.m_modName.Uc().c_str(), mod.m_modName.Lc().c_str());
	ohmPostInstr.Append("\t\tc_%sToHti_ohmAvlCnt ^= 1u;\n", mod.m_modName.Lc().c_str());
	ohmPostInstr.NewLine();

	if (mod.m_clkRate == eClk2x) {
		ohmPostInstr.Append("\tif (r_%sToHti_ohm_2x.read().m_bValid && r_phase && !c_reset1x)\n", mod.m_modName.Lc().c_str());
		ohmPostInstr.Append("\t\tc_%sToHti_ohm = r_%sToHti_ohm_2x;\n", mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());
		ohmPostInstr.NewLine();
	}

	// Generate register statements
	if (mod.m_clkRate == eClk1x)
		ohmReg.Append("\tr_%sToHti_ohm = c_%sToHti_ohm;\n", mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());
	else {
		m_ohmReg1x.Append("\tr_htiTo%s_ohmAvl_1x = i_htiTo%s_ohmAvl.read();\n", mod.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str());
		m_ohmReg1x.Append("\tr_%sToHti_ohm_1x = r_%sToHti_ohm_2x;\n", mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());
		ohmReg.Append("\tr_%sToHti_ohm_2x = c_%sToHti_ohm;\n", mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());
	}
	ohmReg.Append("\tr_%sToHti_ohmAvlCnt = c_reset1x ? (ht_uint1)1 : c_%sToHti_ohmAvlCnt;\n", mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());
	ohmReg.Append("\n");

	// Generate output statements
	if (mod.m_clkRate == eClk1x)
		m_ohmOut1x.Append("\to_%sToHti_ohm = r_%sToHti_ohm;\n", mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());
	else
		m_ohmOut1x.Append("\to_%sToHti_ohm = r_%sToHti_ohm_1x;\n", mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());
	m_ohmOut1x.Append("\n");

	///////////////////////////////////////////////////////////
	// generate HostMsgSendBusy routine

	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

	g_appArgs.GetDsnRpt().AddLevel("Outbound Host Message\n");

	g_appArgs.GetDsnRpt().AddItem("bool SendHostMsgBusy()\n");
	m_ohmFuncDecl.Append("\tbool SendHostMsgBusy();\n");
	m_ohmMacros.Append("bool CPers%s%s::SendHostMsgBusy()\n",
		unitNameUc.c_str(), mod.m_modName.Uc().c_str());
	m_ohmMacros.Append("{\n");

	ohmPostReg.Append("#ifdef _HTV\n");
	ohmPostReg.Append("\tc_bSendHostMsgBusy = r_%sToHti_ohmAvlCnt == 0;\n", mod.m_modName.Lc().c_str());
	ohmPostReg.Append("#else\n");
	ohmPostReg.Append("\tc_bSendHostMsgBusy = r_%sToHti_ohmAvlCnt == 0 || %s_RND_RETRY && ((g_rndRetry() & 1) == 1);\n",
		mod.m_modName.Lc().c_str(), mod.m_modName.Upper().c_str());
	ohmPostReg.Append("#endif\n");

	m_ohmMacros.Append("\tc_bSendHostMsgAvail = !c_bSendHostMsgBusy;\n");
	m_ohmMacros.Append("\treturn c_bSendHostMsgBusy;\n");
	m_ohmMacros.Append("}\n");
	m_ohmMacros.Append("\n");

	g_appArgs.GetDsnRpt().AddItem("void SendHostMsg(sc_uint<7> msgType, sc_uint<56> msgData)\n");
	m_ohmFuncDecl.Append("\tvoid SendHostMsg(sc_uint<7> msgType, sc_uint<56> msgData);\n");
	m_ohmMacros.Append("void CPers%s%s::SendHostMsg(sc_uint<7> msgType, sc_uint<56> msgData)\n",
		unitNameUc.c_str(), mod.m_modName.Uc().c_str());
	m_ohmMacros.Append("{\n");
	m_ohmMacros.Append("\tassert_msg(c_bSendHostMsgAvail, \"Runtime check failed in CPers%s::SendHostMsgBusy()"
		" - expected SendHostMsgBusy() to have been called and not busy\");\n", mod.m_modName.Uc().c_str());
	m_ohmMacros.Append("\tc_%sToHti_ohm.m_bValid = true;\n", mod.m_modName.Lc().c_str());
	m_ohmMacros.Append("\tc_%sToHti_ohm.m_msgType = msgType;\n", mod.m_modName.Lc().c_str());
	m_ohmMacros.Append("\tc_%sToHti_ohm.m_msgData = msgData;\n", mod.m_modName.Lc().c_str());
	m_ohmMacros.Append("}\n");
	m_ohmMacros.Append("\n");

	g_appArgs.GetDsnRpt().EndLevel();
}
