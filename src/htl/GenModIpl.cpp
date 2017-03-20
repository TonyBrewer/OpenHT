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

// Generate a module's instruction pipe line (IPL) code

void CDsnInfo::InitAndValidateModIpl()
{
	// foreach module, determine if any group has multiple threads
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];

		if (!pMod->m_bIsUsed) continue;

		pMod->m_bMultiThread = false;
		pMod->m_bHtId = false;

		pMod->m_rsmSrcCnt += pMod->m_threads.m_bPause ? 1 : 0;

		if ((pMod->m_instrList.size() > 0) != (pMod->m_cxrEntryList.size() > 0)) {
			if (pMod->m_instrList.size() > 0)
				ParseMsg(Error, pMod->m_threads.m_lineInfo, "module has AddInstr, but no AddEntry");
			else
				ParseMsg(Error, pMod->m_threads.m_lineInfo, "module has AddEntry, but no AddInstr");
		} else {
			if (pMod->m_instrList.size() == 0) {
				pMod->m_bHasThreads = false;

				if (pMod->m_threads.m_resetInstr.size() > 0)
					ParseMsg(Error, pMod->m_threads.m_lineInfo, "module has AddModule( resetInstr= ), but no AddInstr");

				if (pMod->m_threads.m_bPause)
					ParseMsg(Error, pMod->m_threads.m_lineInfo, "module has AddModule( pause=true ), but no AddInstr");
			}
		}

		if (pMod->m_bHasThreads) {
			pMod->m_threads.m_htIdW.InitValue(pMod->m_threads.m_lineInfo);

			pMod->m_bMultiThread |= pMod->m_threads.m_htIdW.AsInt() > 0;

			char defineName[64];
			sprintf(defineName, "%s_HTID_W", pMod->m_modName.Upper().c_str());

			int defineHtIdW;
			bool bIsSigned;
			if (!m_defineTable.FindStringValue(pMod->m_threads.m_lineInfo, string(defineName), defineHtIdW, bIsSigned))
				AddDefine(&pMod->m_threads, defineName, VA("%d", pMod->m_threads.m_htIdW.AsInt()));
			else if (defineHtIdW != pMod->m_threads.m_htIdW.AsInt())
				ParseMsg(Error, pMod->m_threads.m_lineInfo, "user defined identifier '%s'=%d has different value from AddThreads '%s'=%d",
				defineName, defineHtIdW, pMod->m_threads.m_htIdW.AsStr().c_str(), pMod->m_threads.m_htIdW.AsInt());

			if (pMod->m_threads.m_htIdW.AsInt() > 0) {
				char typeName[64];
				sprintf(typeName, "%sHtId_t", pMod->m_modName.Uc().c_str());

				if (!FindTypeDef(typeName))
					AddTypeDef(typeName, "uint32_t", defineName, true);

				pMod->m_bHtId = true;
			}
		}

		// determine write stage
		int tsStg = 2;
		if (pMod->m_bMultiThread)
			tsStg += pMod->m_clkRate == eClk2x ? 3 : 1;
		else
			tsStg += pMod->m_clkRate == eClk2x ? 1 : 0;

		pMod->m_gblBlockRam = false;
		pMod->m_gblDistRam = false;
		for (size_t ngvIdx = 0; ngvIdx < pMod->m_ngvList.size(); ngvIdx += 1) {

			if (pMod->m_ngvList[ngvIdx]->m_addr1W.AsInt() == 0) continue;

			if (pMod->m_ngvList[ngvIdx]->m_pNgvInfo->m_ramType == eBlockRam)
				pMod->m_gblBlockRam = true;
			else
				pMod->m_gblDistRam = true;
		}

		if (pMod->m_gblBlockRam)
			tsStg += 3;
		else if (pMod->m_gblDistRam)
			tsStg += 2;

		if (pMod->m_stage.m_privWrStg.AsInt() < pMod->m_stage.m_execStg.AsInt())
			ParseMsg(Error, pMod->m_stage.m_lineInfo, "execStg must be less than or equal to privWrStg");

		pMod->m_tsStg = tsStg;
		pMod->m_execStg = tsStg + pMod->m_stage.m_execStg.AsInt() - 1;
		pMod->m_wrStg = tsStg + pMod->m_stage.m_privWrStg.AsInt();
		pMod->m_gvIwCompStg += tsStg;
	}
}

struct CHtSelName {
	CHtSelName(string name, string replDecl, string replIndex) : m_name(name), m_replDecl(replDecl), m_replIndex(replIndex) {}

	string m_name;
	string m_replDecl;
	string m_replIndex;
};

void CDsnInfo::GenModIplStatements(CInstance * pModInst)
{
	CModule * pMod = pModInst->m_pMod;
	string vcdModName = VA("Pers%s", pModInst->m_instName.Uc().c_str());

	g_appArgs.GetDsnRpt().AddLevel("Module Attributes\n");
	if (pMod->m_bHasThreads)
		g_appArgs.GetDsnRpt().AddItem("HtIdW = %d\n", pMod->m_threads.m_htIdW.AsInt());
	else
		g_appArgs.GetDsnRpt().AddItem("No Instruction state machine\n");
	g_appArgs.GetDsnRpt().AddItem("Clock rate = %s\n", pMod->m_clkRate == eClk2x ? "2x" : "1x");
	g_appArgs.GetDsnRpt().EndLevel();

	bool bStateMachine = pMod->m_bHasThreads;

	if (!bStateMachine) return;

	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

	CHtCode	& iplT0Stg = pMod->m_clkRate == eClk1x ? m_iplT0Stg1x : m_iplT0Stg2x;
	CHtCode	& iplT1Stg = pMod->m_clkRate == eClk1x ? m_iplT1Stg1x : m_iplT1Stg2x;
	CHtCode	& iplT2Stg = pMod->m_clkRate == eClk1x ? m_iplT2Stg1x : m_iplT2Stg2x;
	CHtCode	& iplTsStg = pMod->m_clkRate == eClk1x ? m_iplTsStg1x : m_iplTsStg2x;
	CHtCode	& iplPostInstr = pMod->m_clkRate == eClk1x ? m_iplPostInstr1x : m_iplPostInstr2x;
	CHtCode	& iplReg = pMod->m_clkRate == eClk1x ? m_iplReg1x : m_iplReg2x;
	CHtCode	& iplOut = pMod->m_clkRate == eClk1x ? m_iplOut1x : m_iplOut2x;
	CHtCode & iplReset = pMod->m_clkRate == eClk1x ? m_iplReset1x : m_iplReset2x;

	string reset = pMod->m_clkRate == eClk1x ? "r_reset1x" : "c_reset2x";

	bool bMultiThread = pMod->m_threads.m_htIdW.AsInt() > 0;

	int maxInstrLen = 0;
	m_iplMacros.Append("#ifndef _HTV\n");
	if (g_appArgs.IsInstrTraceEnabled()) {
		m_iplMacros.Append("char const * CPers%s%s::m_pHtCtrlNames[] = {\n\t\"INVALID\",\n\t\"CONTINUE\",\n\t\"CALL\",\n\t\"RETURN\",\n",
			unitNameUc.c_str(), pModInst->m_instName.Uc().c_str());
		m_iplMacros.Append("\t\"JOIN\",\n\t\"RETRY\",\n\t\"TERMINATE\",\n\t\"PAUSE\",\n\t\"JOIN_AND_CONT\"\n};\n");
	}

	m_iplMacros.Append("char const * CPers%s%s::m_pInstrNames[] = {\n",
		unitNameUc.c_str(), pModInst->m_instName.Uc().c_str());
	for (size_t instrIdx = 0; instrIdx < pMod->m_instrList.size(); instrIdx += 1) {
		m_iplMacros.Append("\t\"%s\",\n", pMod->m_instrList[instrIdx].c_str());
		int instrLen = pMod->m_instrList[instrIdx].size();
		if (maxInstrLen < instrLen)
			maxInstrLen = instrLen;
	}
	m_iplMacros.Append("};\n");
	m_iplMacros.Append("#endif\n");
	m_iplMacros.Append("\n");

	///////////////////////////////////////////////////////////////////////
	// Generate instruction pipe line macros

	m_iplMacros.Append("#define %s_RND_RETRY %s\n", pModInst->m_instName.Upper().c_str(), m_bRndRetry ? "true" : "false");
	m_iplMacros.Append("\n");

	m_iplMacros.Append("// Hardware thread methods\n");

	g_appArgs.GetDsnRpt().AddLevel("Thread Control\n");

	g_appArgs.GetDsnRpt().AddLevel("HtRetry()\n");
	g_appArgs.GetDsnRpt().EndLevel();

	m_iplFuncDecl.Append("\tvoid HtRetry();\n");
	m_iplMacros.Append("void CPers%s%s::HtRetry()\n",
		unitNameUc.c_str(), pModInst->m_instName.Uc().c_str());
	m_iplMacros.Append("{\n");

	m_iplMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::HtRetry()"
		" - an Ht control routine was already called\");\n", pMod->m_execStg, pModInst->m_instName.Uc().c_str());

	m_iplMacros.Append("\tc_t%d_htCtrl = HT_RETRY;\n", pMod->m_execStg);
	m_iplMacros.Append("}\n");
	m_iplMacros.Append("\n");

	g_appArgs.GetDsnRpt().AddLevel("HtContinue(ht_uint%d nextInstr)\n", pMod->m_instrW);
	g_appArgs.GetDsnRpt().EndLevel();

	m_iplFuncDecl.Append("\tvoid HtContinue(ht_uint%d nextInstr);\n", pMod->m_instrW);
	m_iplMacros.Append("void CPers%s%s::HtContinue(ht_uint%d nextInstr)\n",
		unitNameUc.c_str(), pModInst->m_instName.Uc().c_str(), pMod->m_instrW);
	m_iplMacros.Append("{\n");
	m_iplMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::HtContinue()"
		" - an Ht control routine was already called\");\n", pMod->m_execStg, pModInst->m_instName.Uc().c_str());
	m_iplMacros.Append("\tc_t%d_htCtrl = HT_CONT;\n", pMod->m_execStg);
	m_iplMacros.Append("\tc_t%d_htNextInstr = nextInstr;\n", pMod->m_execStg);
	m_iplMacros.Append("}\n");
	m_iplMacros.Append("\n");

	if (pMod->m_threads.m_bPause) {
		g_appArgs.GetDsnRpt().AddLevel("void HtPause(ht_uint%d nextInstr)\n", pMod->m_instrW);
		g_appArgs.GetDsnRpt().EndLevel();
		m_iplFuncDecl.Append("\tvoid HtPause(ht_uint%d nextInstr);\n", pMod->m_instrW);
		m_iplMacros.Append("void CPers%s%s::HtPause(ht_uint%d nextInstr)\n",
			unitNameUc.c_str(), pModInst->m_instName.Uc().c_str(), pMod->m_instrW);
		m_iplMacros.Append("{\n");
		m_iplMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::HtPause()"
			" - an Ht control routine was already called\");\n", pMod->m_execStg, pModInst->m_instName.Uc().c_str());
		m_iplMacros.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", pMod->m_execStg);
		if (pMod->m_threads.m_htIdW.AsInt() > 0) {
			m_iplMacros.Append("\tm_htRsmInstr.write_addr(r_t%d_htId);\n", pMod->m_execStg);
			m_iplMacros.Append("\tm_htRsmInstr.write_mem(nextInstr);\n");
			m_iplMacros.Append("#\tifndef _HTV\n");
			m_iplMacros.Append("\tassert(m_htRsmWait.read_mem_debug(r_t%d_htId) == false);\n", pMod->m_execStg);
			m_iplMacros.Append("\tm_htRsmWait.write_mem_debug(r_t%d_htId) = true;\n", pMod->m_execStg);
			m_iplMacros.Append("#\tendif\n");
		} else {
			m_iplMacros.Append("\tc_htRsmInstr = nextInstr;\n");
			m_iplMacros.Append("#\tifndef _HTV\n");
			m_iplMacros.Append("\tassert(r_htRsmWait == false);\n");
			m_iplMacros.Append("\tc_htRsmWait = true;\n");
			m_iplMacros.Append("#\tendif\n");
		}
		m_iplMacros.Append("}\n");
		m_iplMacros.Append("\n");

		if (pMod->m_threads.m_htIdW.AsInt() > 0) {
			g_appArgs.GetDsnRpt().AddLevel("void HtResume(ht_uint<%s_HTID_W> htId)\n", pMod->m_modName.Upper().c_str());
			g_appArgs.GetDsnRpt().EndLevel();

			m_iplFuncDecl.Append("\tvoid HtResume(sc_uint<%s_HTID_W> htId);\n", pMod->m_modName.Upper().c_str());
			m_iplMacros.Append("void CPers%s%s::HtResume(sc_uint<%s_HTID_W> htId)\n",
				unitNameUc.c_str(), pModInst->m_instName.Uc().c_str(), pMod->m_modName.Upper().c_str());
		} else if (pMod->m_threads.m_htIdW.size() > 0) {
			g_appArgs.GetDsnRpt().AddLevel("void HtResume(ht_uint1 htId)\n");
			g_appArgs.GetDsnRpt().EndLevel();

			m_iplFuncDecl.Append("\tvoid HtResume(ht_uint1 htId);\n");
			m_iplMacros.Append("void CPers%s%s::HtResume(ht_uint1 ht_noload htId)\n",
				unitNameUc.c_str(), pModInst->m_instName.Uc().c_str());
		} else {
			g_appArgs.GetDsnRpt().AddLevel("void HtResume()\n");
			g_appArgs.GetDsnRpt().EndLevel();

			m_iplFuncDecl.Append("\tvoid HtResume();\n");
			m_iplMacros.Append("void CPers%s%s::HtResume()\n",
				unitNameUc.c_str(), pModInst->m_instName.Uc().c_str());
		}
		m_iplMacros.Append("{\n");
		if (pMod->m_threads.m_htIdW.size() > 0 && pMod->m_threads.m_htIdW.AsInt() == 0)
			m_iplMacros.Append("\tassert_msg(htId == 0, \"Runtime check failed in CPers%s::HtResume()"
			" - expected htId parameter value to be zero\");\n", pModInst->m_instName.Uc().c_str());
		if (pMod->m_threads.m_htIdW.AsInt() > 0) {
			if (pMod->m_rsmSrcCnt > 1) {
				m_iplMacros.Append("\tCHtCmd rsm;\n");
				m_iplMacros.Append("\trsm.m_htId = htId;\n");
				m_iplMacros.Append("\tm_htRsmInstr.read_addr(htId);\n");
				m_iplMacros.Append("\trsm.m_htInstr = m_htRsmInstr.read_mem();\n");
				m_iplMacros.Append("\tm_htRsmQue.push(rsm);\n");
			} else {
				m_iplMacros.Append("\tc_t0_rsmHtRdy = true;\n");
				m_iplMacros.Append("\tc_t0_rsmHtId = htId;\n");
				m_iplMacros.Append("\tm_htRsmInstr.read_addr(htId);\n");
				m_iplMacros.Append("\tc_t0_rsmHtInstr = m_htRsmInstr.read_mem();\n");
			}
			m_iplMacros.Append("#\tifndef _HTV\n");
			m_iplMacros.Append("\tassert(%s || m_htRsmWait.read_mem_debug(htId) == true);\n", reset.c_str());
			m_iplMacros.Append("\tm_htRsmWait.write_mem_debug(htId) = false;\n");
			m_iplMacros.Append("#\tendif\n");
		} else {
			m_iplMacros.Append("\tc_t0_rsmHtRdy = true;\n");
			m_iplMacros.Append("\tc_t0_rsmHtInstr = r_htRsmInstr;\n");
			m_iplMacros.Append("#\tifndef _HTV\n");
			m_iplMacros.Append("\tassert(%s || r_htRsmWait == true);\n", reset.c_str());
			m_iplMacros.Append("\tc_htRsmWait = false;\n");
			m_iplMacros.Append("#\tendif\n");
		}
		m_iplMacros.Append("}\n");
		m_iplMacros.Append("\n");
	}

	g_appArgs.GetDsnRpt().AddLevel("void HtTerminate()\n");
	g_appArgs.GetDsnRpt().EndLevel();

	m_iplFuncDecl.Append("\tvoid HtTerminate();\n");
	m_iplMacros.Append("void CPers%s%s::HtTerminate()\n",
		unitNameUc.c_str(), pModInst->m_instName.Uc().c_str());
	m_iplMacros.Append("{\n");
	m_iplMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::HtTerminate()"
		" - an Ht control routine was already called\");\n", pMod->m_execStg, pModInst->m_instName.Uc().c_str());
	m_iplMacros.Append("\tc_t%d_htCtrl = HT_TERM;\n", pMod->m_execStg);
	m_iplMacros.Append("}\n");
	m_iplMacros.Append("\n");

	g_appArgs.GetDsnRpt().EndLevel();

	CHtCode *pIplTxStg = &iplT0Stg;

	bool bNeedMifPollQue = (pMod->m_mif.m_mifRd.m_bPoll || pMod->m_mif.m_mifWr.m_bPoll) && pMod->m_threads.m_htIdW.AsInt() > 2;

	// determine HT_SEL type width
	int rsvSelCnt = pMod->m_threads.m_resetInstr.size() > 0 ? 1 : 0;
	for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

		if (pCxrIntf->m_cxrDir == CxrOut) continue;

		rsvSelCnt += 1;
	}
	if (pModInst->m_cxrSrcCnt > 1)
		rsvSelCnt += 1;
	if (bNeedMifPollQue)
		rsvSelCnt += 1;
	int rsvSelW = FindLg2(rsvSelCnt);

	if (pMod->m_cxrEntryReserveCnt > 0) {
		char typeName[32];
		sprintf(typeName, "ht_uint%d", rsvSelW);
		pMod->m_threads.m_htPriv.AddStructField(FindType(typeName), "rtnRsvSel");
	}

	char htIdTypeStr[128];
	sprintf(htIdTypeStr, "sc_uint<%s_HTID_W>", pMod->m_modName.Upper().c_str());
	char htInstrTypeStr[128];
	sprintf(htInstrTypeStr, "sc_uint<%s_INSTR_W> ht_noload", pMod->m_modName.Upper().c_str());

	{
		m_iplRegDecl.Append("\tstruct CHtCmd {\n");
		m_iplRegDecl.Append("#\t\tifndef _HTV\n");
		m_iplRegDecl.Append("\t\tCHtCmd() {} // avoid run-time uninitialized error\n");
		m_iplRegDecl.Append("#\t\tendif\n");
		m_iplRegDecl.Append("\n");
		m_iplRegDecl.Append("\t\tuint32_t\t\tm_htInstr : %s_INSTR_W;\n", pMod->m_modName.Upper().c_str());
		if (pMod->m_threads.m_htIdW.AsInt() > 0)
			m_iplRegDecl.Append("\t\tuint32_t\t\tm_htId : %s_HTID_W;\n", pMod->m_modName.Upper().c_str());
		m_iplRegDecl.Append("\t};\n");
		m_iplRegDecl.Append("\n");

		// if no private variables then declare one to avoid zero width struct
		bool bFoundField = pMod->m_threads.m_htPriv.m_fieldList.size() > 0;

		if (bFoundField == false) {
			string name = "null";

			pMod->m_threads.m_htPriv.AddStructField(&g_bool, name);
			pMod->m_threads.m_htPriv.m_fieldList.back()->InitDimen(pMod->m_threads.m_lineInfo);
		}

		pMod->m_threads.m_htPriv.m_typeName = "CHtPriv";
		pMod->m_threads.m_htPriv.m_bCStyle = false;
		GenUserStructs(m_iplRegDecl, &pMod->m_threads.m_htPriv, "\t");

		string privStructName = VA("CPers%s::CHtPriv", pModInst->m_instName.Uc().c_str());

		GenUserStructBadData(m_iplBadDecl, true, privStructName,
			pMod->m_threads.m_htPriv.m_fieldList, pMod->m_threads.m_htPriv.m_bCStyle, "");

		if (pMod->m_threads.m_htIdW.AsInt() == 0) {

			m_iplRegDecl.Append("\tCHtCmd r_htCmd;\n");
			m_iplRegDecl.Append("\tCHtPriv r_htPriv;\n");

		} else {
			m_iplRegDecl.Append("\tht_dist_que<CHtCmd, %s_HTID_W> m_htCmdQue;\n",
				pMod->m_modName.Upper().c_str());
			if (pMod->m_threads.m_ramType == eDistRam)
				m_iplRegDecl.Append("\tht_dist_ram<CHtPriv, %s_HTID_W> m_htPriv;\n",
				pMod->m_modName.Upper().c_str());
			else
				m_iplRegDecl.Append("\tht_block_ram<CHtPriv, %s_HTID_W> m_htPriv;\n",
				pMod->m_modName.Upper().c_str());
		}

		if (pMod->m_threads.m_bCallFork) {
			if (pMod->m_threads.m_htIdW.AsInt() <= 2) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("ht_uint%d", 1 << pMod->m_threads.m_htIdW.AsInt()), VA("r_htPrivLk"));
				iplReg.Append("\tr_htPrivLk = %s ? (ht_uint%d)0 : c_htPrivLk;\n", reset.c_str(),
					1 << pMod->m_threads.m_htIdW.AsInt());
			} else {
				for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
					CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

					if (pCxrIntf->m_cxrDir == CxrOut) continue;
					if (pCxrIntf->m_cxrType == CxrCall || pCxrIntf->m_cxrType == CxrTransfer) continue;
					if (pCxrIntf->m_srcReplCnt > 1 && pCxrIntf->m_srcReplId != 0) continue;

					m_iplRegDecl.Append("\tht_dist_ram<bool, %s_HTID_W> m_%s_%sPrivLk0%s;\n",
						pMod->m_modName.Upper().c_str(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
					m_iplRegDecl.Append("\tht_dist_ram<bool, %s_HTID_W> m_%s_%sPrivLk1%s;\n",
						pMod->m_modName.Upper().c_str(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
				}
				m_iplRegDecl.Append("\tht_dist_ram<bool, %s_HTID_W> m_htCmdPrivLk0;\n",
					pMod->m_modName.Upper().c_str());
				m_iplRegDecl.Append("\tht_dist_ram<bool, %s_HTID_W> m_htCmdPrivLk1;\n",
					pMod->m_modName.Upper().c_str());

				if (pMod->m_rsmSrcCnt > 0) {
					m_iplRegDecl.Append("\tht_dist_ram<bool, %s_HTID_W> m_rsmPrivLk0;\n",
						pMod->m_modName.Upper().c_str());
					m_iplRegDecl.Append("\tht_dist_ram<bool, %s_HTID_W> m_rsmPrivLk1;\n",
						pMod->m_modName.Upper().c_str());
				}

				m_iplRegDecl.Append("\n");
			}
		}
	}

	///////////////////////////////////////////////////////////////////////
	// First pipe stage: ht arbitration (always t1)

	pIplTxStg = &iplT1Stg;

	pIplTxStg->Append("\t// First pipe stage: ht arbitration\n");

	string htSelType;
	{
		int htSelCnt = 1;
		if (pMod->m_threads.m_resetInstr.size() > 0) htSelCnt += 1;

		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrOut) continue;

			htSelCnt += 1;
		}

		if (bNeedMifPollQue) htSelCnt += 1;

		htSelType = VA("ht_uint%d", FindLg2(htSelCnt - 1));

		pIplTxStg->Append("\t%s c_t1_htSel = 0;\n", htSelType.c_str());
	}

	pIplTxStg->Append("\tbool c_t1_htValid = false;\n");

	if (pMod->m_threads.m_htIdW.AsInt() == 0) {

		pIplTxStg->Append("\tbool c_htBusy = r_htBusy;\n");
		pIplTxStg->Append("\tbool c_htCmdValid = r_htCmdValid;\n");

		if (pMod->m_threads.m_bCallFork && pMod->m_threads.m_htIdW.AsInt() == 0)
			pIplTxStg->Append("\tht_uint%d c_htPrivLk = r_htPrivLk;\n",
			1 << pMod->m_threads.m_htIdW.AsInt());
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	// generate reset complete signal
	bool bResetCnt = false;
	char const * pResetCompStr = "\tbool c_resetComplete = ";

	if (pMod->m_mif.m_bMifRd && pMod->m_mif.m_mifRd.m_rspGrpIdW > 2) {
		pIplTxStg->Append("%sr_rdRspGrpInitCnt[%s_RD_GRP_ID_W] != 0",
			pResetCompStr, pModInst->m_instName.Upper().c_str());
		pResetCompStr = " &&\n\t\t";
		bResetCnt = true;
	}

	if (pMod->m_mif.m_bMifWr && pMod->m_mif.m_mifWr.m_rspGrpIdW > 2) {
		pIplTxStg->Append("%sr_wrRspGrpInitCnt[%s_WR_GRP_ID_W] != 0",
			pResetCompStr, pModInst->m_instName.Upper().c_str());
		pResetCompStr = " &&\n\t\t";
		bResetCnt = true;
	}

	if (pMod->m_bHtIdInit) {
		pIplTxStg->Append("%s!r_t%d_htIdInit", pResetCompStr, pMod->m_execStg);
		pResetCompStr = " &&\n\t\t";
		bResetCnt = true;
	}

	for (size_t barIdx = 0; barIdx < pMod->m_barrierList.size(); barIdx += 1) {
		CBarrier * pBar = pMod->m_barrierList[barIdx];

		bool bMimicHtCont = pMod->m_threads.m_htIdW.AsInt() == 0 && pMod->m_instSet.GetReplCnt(0) == 1;

		if (bMimicHtCont || pBar->m_barIdW.AsInt() <= 2) continue;

		pIplTxStg->Append("%sr_bar%sInitCnt[%d] != 0", pResetCompStr, pBar->m_name.Uc().c_str(), pBar->m_barIdW.AsInt());
		pResetCompStr = " &&\n\t\t";
		bResetCnt = true;
	}

	if (pMod->m_threads.m_htIdW.AsInt() > 0) {

		pIplTxStg->Append("%sr_htIdPoolCnt[%s_HTID_W] != 0",
			pResetCompStr, pMod->m_modName.Upper().c_str());

		pResetCompStr = " &&\n\t\t";
		bResetCnt = true;
	}

	if (bResetCnt)
		pIplTxStg->Append(";\n");
	else {
		pIplTxStg->Append("%s!%s;\n", pResetCompStr, reset.c_str());
		bResetCnt = true;
	}

	pIplTxStg->Append("\n");

	m_iplRegDecl.Append("\tbool r_resetComplete;\n");
	iplReg.Append("\tr_resetComplete = !%s && c_resetComplete;\n", reset.c_str());

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	if (pMod->m_threads.m_resetInstr.size() > 0) {
		m_iplRegDecl.Append("\tbool r_resetInstrStarted;\n");
		pIplTxStg->Append("\tbool c_resetInstrStarted = r_resetInstrStarted;\n");
		iplReg.Append("\tr_resetInstrStarted = !%s && c_resetInstrStarted;\n", reset.c_str());
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	// generate reserved thread state
	for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

		if (pCxrIntf->m_cxrDir == CxrOut) continue;
		if (pCxrIntf->m_cxrType != CxrCall && pCxrIntf->m_cxrType != CxrTransfer) continue;
		if (pCxrIntf->m_reserveCnt == 0) continue;

		if (pCxrIntf->GetPortReplCnt() <= 1 || pCxrIntf->GetPortReplId() == 0) {
			m_iplRegDecl.Append("\tbool r_%s_%sRsvLimitEmpty%s;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
			m_iplRegDecl.Append("\tsc_uint<%s> r_%s_%sRsvLimit%s;\n",
				pMod->m_threads.m_htIdW.c_str(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
		}

		if (pCxrIntf->GetPortReplCnt() <= 1) {
			pIplTxStg->Append("\tsc_uint<%s> c_%s_%sRsvLimit = r_%s_%sRsvLimit;\n",
				pMod->m_threads.m_htIdW.c_str(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName());
		} else {
			if (pCxrIntf->GetPortReplId() == 0)
				pIplTxStg->Append("\tsc_uint<%s> c_%s_%sRsvLimit%s;\n",
				pMod->m_threads.m_htIdW.c_str(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

			pIplTxStg->Append("\tc_%s_%sRsvLimit%s = r_%s_%sRsvLimit%s;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
		}

		iplReg.Append("\tr_%s_%sRsvLimitEmpty%s = c_%s_%sRsvLimit%s == 0;\n",
			pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
			pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
		iplReg.Append("\tr_%s_%sRsvLimit%s = %s ? (sc_uint<%s>)%d : c_%s_%sRsvLimit%s;\n",
			pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
			reset.c_str(),
			pMod->m_threads.m_htIdW.c_str(),
			(1 << pMod->m_threads.m_htIdW.AsInt()) - pCxrIntf->m_reserveCnt,
			pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

		if (pCxrIntf->m_cxrDir == CxrOut) continue;
		if (!pCxrIntf->m_pSrcModInst->m_pMod->m_bGvIwComp) continue;

		if (pCxrIntf->GetPortReplId() == 0) {
			vector<CHtString> replDim;

			if (pCxrIntf->GetPortReplCnt() > 1) {
				CHtString str = pCxrIntf->GetPortReplCnt();
				replDim.push_back(str);
			}

			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("ht_uint%d", pCxrIntf->GetQueDepthW() + 1), VA("r_%s_%sCompCnt", pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName()), replDim);
			m_iplRegDecl.Append("\tht_uint%d c_%s_%sCompCnt%s;\n", pCxrIntf->GetQueDepthW() + 1, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

			if (pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate == eClk2x && pMod->m_clkRate == eClk1x) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "sc_signal<ht_uint2>", VA("r_%s_%sCompCnt_2x", pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName()), replDim);
				m_iplRegDecl.Append("\tht_uint2 c_%s_%sCompCnt_2x%s;\n", pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
			}
		}

		if (pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate == eClk2x && pMod->m_clkRate == eClk1x) {
			pIplTxStg->Append("\tc_%s_%sCompCnt%s = r_%s_%sCompCnt%s + r_%s_%sCompCnt_2x%s.read();\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

		} else if (pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate == eClk1x && pMod->m_clkRate == eClk1x) {
			pIplTxStg->Append("\tc_%s_%sCompCnt%s = r_%s_%sCompCnt%s + (i_%s_%sCompRdy%s.read() ? 1u : 0u);\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
		}

		iplReg.Append("\tr_%s_%sCompCnt%s = %s ? (ht_uint%d)0 : c_%s_%sCompCnt%s;\n",
			pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
			reset.c_str(),
			pCxrIntf->GetQueDepthW() + 1,
			pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

		if (pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate == eClk2x && pMod->m_clkRate == eClk1x) {

			m_iplPostInstr2x.Append("\tc_%s_%sCompCnt_2x%s = r_phase == 0 ? (ht_uint2)0 : r_%s_%sCompCnt_2x%s;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			m_iplPostInstr2x.Append("\tc_%s_%sCompCnt_2x%s += i_%s_%sCompRdy%s.read() ? 1u : 0u;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

			m_iplReg2x.Append("\tr_%s_%sCompCnt_2x%s = c_%s_%sCompCnt_2x%s;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
		}
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();
	m_iplPostInstr2x.NewLine();
	m_iplReg2x.NewLine();

	vector<CHtSelName> htSelNameList;
	int selRrCnt = 1;

	htSelNameList.push_back(CHtSelName("htCmd", "", ""));
	if (pMod->m_threads.m_htIdW.AsInt() == 0) {
		if (!pMod->m_bGvIwComp) {
			pIplTxStg->Append("\tbool c_htCmdRdy = r_htCmdValid");
			if (pMod->m_threads.m_bCallFork)
				pIplTxStg->Append(" && !r_htPrivLk");
			pIplTxStg->Append(";\n");
		} else {
			pIplTxStg->Append("\tc_htCompQueAvlCnt = r_htCompQueAvlCnt;\n");
			pIplTxStg->Append("\tc_htCompRdy = r_htCompRdy;\n");
			if (pMod->m_bCallFork)
				pIplTxStg->Append("\tbool c_htCmdRdy = r_htCmdValid && !r_htPrivLk && r_htCompRdy;\n");
			else
				pIplTxStg->Append("\tbool c_htCmdRdy = r_htCmdValid && r_htCompRdy;\n");
		}
	} else {
		if (!pMod->m_bGvIwComp)
			pIplTxStg->Append("\tbool c_htCmdRdy = !m_htCmdQue.empty();\n");
		else {
			pIplTxStg->Append("\tc_htCompQueAvlCnt = r_htCompQueAvlCnt;\n");
			pIplTxStg->Append("\tc_htCmdRdyCnt = r_htCmdRdyCnt;\n");
			pIplTxStg->Append("\tbool c_htCmdRdy = r_htCmdRdyCnt > 0 && r_htCompQueAvlCnt > 0;\n");
		}
	}

	if (bNeedMifPollQue) {
		// add the mif polling queue
		htSelNameList.push_back(CHtSelName("mifPoll", "", ""));
		selRrCnt += 1;

		if (!pMod->m_bGvIwComp)
			pIplTxStg->Append("\tbool c_mifPollRdy = !m_mifPollQue.empty();\n");
		else {
			pIplTxStg->Append("\tc_mifPollRdyCnt = r_mifPollRdyCnt;\n");
			pIplTxStg->Append("\tbool c_mifPollRdy = r_mifPollRdyCnt > 0 && r_htCompQueAvlCnt > 0;\n");
		}
	}

	for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

		if (pCxrIntf->m_cxrType == CxrCall || pCxrIntf->m_cxrType == CxrTransfer) continue;
		if (pCxrIntf->m_cxrDir == CxrOut) continue;

		char htSelNameStr[128];
		sprintf(htSelNameStr, "%s_%s",
			pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName());
		htSelNameList.push_back(CHtSelName(htSelNameStr, pCxrIntf->GetPortReplDecl(), pCxrIntf->GetPortReplIndex()));

		if (pCxrIntf->GetPortReplCnt() <= 1)
			pIplTxStg->Append("\tbool c_%sRdy = ", htSelNameStr);
		else {
			if (pCxrIntf->GetPortReplId() == 0)
				pIplTxStg->Append("\tbool c_%sRdy%s;\n", htSelNameStr, pCxrIntf->GetPortReplDecl());

			pIplTxStg->Append("\tc_%sRdy%s = ", htSelNameStr, pCxrIntf->GetPortReplIndex());
		}

		if (pCxrIntf->GetQueDepthW() > 0) {
			if (pCxrIntf->m_bCxrIntfFields) {
				if (pMod->m_clkRate == eClk1x && pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate == eClk1x)
					pIplTxStg->Append("!m_%s_%sQue%s.empty()",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				else
					pIplTxStg->Append("!r_%s_%sQueEmpty%s",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			} else
				pIplTxStg->Append("r_%s_%sCnt%s > 0u",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
		} else
			pIplTxStg->Append("r_t1_%s_%sRdy%s",
			pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

		if (pCxrIntf->m_cxrType == CxrReturn) {

			CCxrIntf * pCxrCallIntf = pModInst->m_cxrIntfList[pCxrIntf->m_callIntfIdx];
			CCxrCall * pCxrCall = pModInst->m_pMod->m_cxrCallList[pCxrCallIntf->m_callIdx];

			if (pCxrCall->m_pGroup->m_htIdW.AsInt() > 0) {
			} else {
				if (pMod->m_bCallFork)
					pIplTxStg->Append(" && !r_htPrivLk");
			}
		}

		if (pMod->m_bGvIwComp) {
			pIplTxStg->Append(" && r_htCompQueAvlCnt > 0u", pCxrIntf->GetIntfName());
		}

		if (pCxrIntf->m_pSrcModInst->m_pMod->m_bGvIwComp) {
			pIplTxStg->Append(" && r_%s_%sCompCnt%s > 0u", pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
		}

		pIplTxStg->Append(";\n");

		selRrCnt += 1;
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	for (size_t i = 0; i < htSelNameList.size(); i += 1) {

		if (selRrCnt > 1) {
			if (htSelNameList[i].m_replDecl.size() == 0)
				pIplTxStg->Append("\tbool c_%sSel = c_%sRdy && ((r_htSelRr & 0x%lx) != 0 || !(\n",
				htSelNameList[i].m_name.c_str(), htSelNameList[i].m_name.c_str(), 1ul << i);
			else {
				if (htSelNameList[i].m_replIndex == "[0]")
					pIplTxStg->Append("\tbool c_%sSel%s;\n",
					htSelNameList[i].m_name.c_str(), htSelNameList[i].m_replDecl.c_str());

				pIplTxStg->Append("\tc_%sSel%s = c_%sRdy%s && ((r_htSelRr & 0x%lx) != 0 || !(\n",
					htSelNameList[i].m_name.c_str(), htSelNameList[i].m_replIndex.c_str(),
					htSelNameList[i].m_name.c_str(), htSelNameList[i].m_replIndex.c_str(),
					1ul << i);
			}
		} else {
			if (htSelNameList[i].m_replDecl.size() == 0)
				pIplTxStg->Append("\tbool c_%sSel = c_%sRdy;",
				htSelNameList[i].m_name.c_str(), htSelNameList[i].m_name.c_str());
			else {
				if (htSelNameList[i].m_replIndex == "[0]")
					pIplTxStg->Append("\tbool c_%sSel%s;",
					htSelNameList[i].m_name.c_str(), htSelNameList[i].m_replDecl.c_str());

				pIplTxStg->Append("\tc_%sSel%s = c_%sRdy%s;",
					htSelNameList[i].m_name.c_str(), htSelNameList[i].m_replIndex.c_str(),
					htSelNameList[i].m_name.c_str(), htSelNameList[i].m_replIndex.c_str());
			}
		}

		for (int j = 1; j < selRrCnt; j += 1) {
			int k = (i + j) % selRrCnt;

			uint32_t mask1 = (1ul << selRrCnt) - 1;
			uint32_t mask2 = ((1ul << j) - 1) << (i + 1);
			uint32_t mask3 = (mask2 & mask1) | (mask2 >> selRrCnt);

			pIplTxStg->Append("\t\t(c_%sRdy%s && (r_htSelRr & 0x%x) != 0)%s\n",
				htSelNameList[k].m_name.c_str(), htSelNameList[k].m_replIndex.c_str(),
				mask3, j == selRrCnt - 1 ? "));" : " ||");
		}
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	if (selRrCnt > 1) {
		m_iplRegDecl.Append("\tht_uint%d r_htSelRr;\n", selRrCnt);
		pIplTxStg->Append("\tht_uint%d c_htSelRr = r_htSelRr;\n", selRrCnt);
		iplReg.Append("\tr_htSelRr = %s ? (ht_uint%d)0x1 : c_htSelRr;\n", reset.c_str(), selRrCnt);
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	// generate cmds
	for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

		if (pCxrIntf->m_cxrDir == CxrOut) continue;
		if (!pCxrIntf->m_bCxrIntfFields) continue;

		if (pCxrIntf->GetQueDepthW() > 0) {
			if (pMod->m_clkRate == eClk1x && pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate == eClk1x) {
				if (pCxrIntf->GetPortReplCnt() <= 1)
					pIplTxStg->Append("\tC%s_%s c_t1_%s_%s = m_%s_%sQue.front();\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetSrcToDstLc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetSrcToDstLc(), pCxrIntf->GetIntfName());
				else {
					if (pCxrIntf->GetPortReplId() == 0)
						pIplTxStg->Append("\tC%s_%s c_t1_%s_%s%s;\n",
						pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

					pIplTxStg->Append("\tc_t1_%s_%s%s = m_%s_%sQue%s.front();\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
			} else {
				if (pCxrIntf->GetPortReplCnt() <= 1)
					pIplTxStg->Append("\tC%s_%s c_t1_%s_%s = r_%s_%sQueFront;\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetSrcToDstLc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetSrcToDstLc(), pCxrIntf->GetIntfName());
				else {
					if (pCxrIntf->GetPortReplId() == 0)
						pIplTxStg->Append("\tC%s_%s c_t1_%s_%s%s;\n",
						pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

					pIplTxStg->Append("\tc_t1_%s_%s%s = r_%s_%sQueFront%s;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
			}
		} else {
			if (pCxrIntf->GetPortReplCnt() <= 1)
				pIplTxStg->Append("\tC%s_%s c_t1_%s_%s = r_t1_%s_%s;\n",
				pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
				pCxrIntf->GetSrcToDstLc(), pCxrIntf->GetIntfName(),
				pCxrIntf->GetSrcToDstLc(), pCxrIntf->GetIntfName());
			else {
				if (pCxrIntf->GetPortReplId() == 0)
					pIplTxStg->Append("\tC%s_%s c_t1_%s_%s%s;\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

				pIplTxStg->Append("\tc_t1_%s_%s%s = r_t1_%s_%s%s;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			}
		}

		if (pCxrIntf->m_cxrType == CxrReturn && pMod->m_threads.m_htIdW.AsInt() > 0) {
			if (pCxrIntf->GetPortReplCnt() <= 1)
				pIplTxStg->Append("\tsc_uint<%s> c_t1_%s_%sHtId%s = c_t1_%s_%s%s.m_rtnHtId;\n",
				pMod->m_threads.m_htIdW.c_str(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			else {
				if (pCxrIntf->GetPortReplId() == 0)
					pIplTxStg->Append("\tsc_uint<%s> c_t1_%s_%sHtId%s;\n",
					pMod->m_threads.m_htIdW.c_str(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

				pIplTxStg->Append("\tc_t1_%s_%sHtId%s = c_t1_%s_%s%s.m_rtnHtId;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			}

			if (pMod->m_bCallFork && pMod->m_threads.m_htIdW.AsInt() > 2) {
				pIplTxStg->Append("\tm_%s_%sPrivLk0%s.read_addr(c_t1_%s_%sHtId%s);\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				pIplTxStg->Append("\tm_%s_%sPrivLk1%s.read_addr(c_t1_%s_%sHtId%s);\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			}
		}
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	if (pMod->m_threads.m_htIdW.AsInt() == 0)
		pIplTxStg->Append("\tCHtCmd c_t1_htCmd = r_htCmd;\n");
	else
		pIplTxStg->Append("\tCHtCmd c_t1_htCmd = m_htCmdQue.front();\n");

	if (pMod->m_threads.m_htIdW.AsInt() > 0) {
		pIplTxStg->Append("\tsc_uint<%s_HTID_W> c_t1_htCmdHtId = c_t1_htCmd.m_htId;\n",
			pMod->m_modName.Upper().c_str());

		if (pMod->m_bCallFork && pMod->m_threads.m_htIdW.AsInt() > 2) {
			pIplTxStg->Append("\tm_htCmdPrivLk0.read_addr(c_t1_htCmdHtId);\n");
			pIplTxStg->Append("\tm_htCmdPrivLk1.read_addr(c_t1_htCmdHtId);\n");
		}

		pIplTxStg->Append("\tsc_uint<%s_HTID_W> c_t1_htIdPool = m_htIdPool.front();\n",
			pMod->m_modName.Upper().c_str());
		pIplTxStg->Append("\tsc_uint<%s_HTID_W> c_t1_htId = 0;\n",
			pMod->m_modName.Upper().c_str());

		if (pMod->m_threads.m_htIdW.AsInt() > 2 && pMod->m_bCallFork) {
			pIplTxStg->Append("\tbool c_t1_htPrivLkData = false;\n");
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("r_t2_htPrivLkData"));
			iplReg.Append("\tr_t2_htPrivLkData = c_t1_htPrivLkData;\n");
		}
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	if (bNeedMifPollQue) {
		pIplTxStg->Append("\tCHtCmd c_t1_mifPollCmd = m_mifPollQue.front();\n");

		pIplTxStg->Append("\tsc_uint<%s_HTID_W> c_t1_mifPollHtId = c_t1_mifPollCmd.m_htId;\n",
			pMod->m_modName.Upper().c_str());
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	if (pMod->m_rsmSrcCnt > 0) {

		pIplTxStg->Append("\tbool c_t1_rsmHtRdy = r_t1_rsmHtRdy;\n");
		if (pMod->m_clkRate == eClk2x) {
			m_iplRegDecl.Append("\tbool r_t2_rsmHtRdy;\n");
			iplReg.Append("\tr_t2_rsmHtRdy = c_t1_rsmHtRdy;\n");
		}

		if (pMod->m_threads.m_htIdW.AsInt() > 0) {
			pIplTxStg->Append("\tsc_uint<%s_HTID_W> c_t1_rsmHtId = r_t1_rsmHtId;\n", pMod->m_modName.Upper().c_str());
			//if (pMod->m_clkRate == eClk2x) {
			//	m_iplRegDecl.Append("\tsc_uint<%s> r_t2_rsmHtId;\n", pMod->m_threads.m_htIdW.c_str());
			//	iplReg.Append("\tr_t2_rsmHtId = c_t1_rsmHtId;\n");
			//}
		}

		pIplTxStg->Append("\tsc_uint<%s_INSTR_W> c_t1_rsmHtInstr = r_t1_rsmHtInstr;\n", pMod->m_modName.Upper().c_str());
		if (pMod->m_clkRate == eClk2x) {
			m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> r_t2_rsmHtInstr;\n", pMod->m_modName.Upper().c_str());
			iplReg.Append("\tr_t2_rsmHtInstr = c_t1_rsmHtInstr;\n");
		}

		if (pMod->m_bCallFork && pMod->m_threads.m_htIdW.AsInt() > 2) {
			pIplTxStg->Append("\tm_rsmPrivLk0.read_addr(c_t1_rsmHtId);\n");
			pIplTxStg->Append("\tm_rsmPrivLk1.read_addr(c_t1_rsmHtId);\n");
		}
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	char *pFirst = "\t";

	int htSelCnt = 0;
	if (pMod->m_threads.m_resetInstr.size() > 0) {

		pIplTxStg->Append("%sif (!r_resetInstrStarted", pFirst);

		if (pMod->m_threads.m_htIdW.AsInt() > 0)
			pIplTxStg->Append(" && !m_htIdPool.empty()");
		else
			pIplTxStg->Append(" && !r_htBusy");

		if (bResetCnt) pIplTxStg->Append(" && r_resetComplete");

		pIplTxStg->Append(") {\n");

		m_htSelDef.Append("#define %s_HT_SEL_RESET %d\n",
			pModInst->m_instName.Upper().c_str(), htSelCnt++);

		pIplTxStg->Append("\t\tc_t1_htSel = %s_HT_SEL_RESET;\n",
			pModInst->m_instName.Upper().c_str());

		if (pMod->m_threads.m_htIdW.AsInt() > 0)
			pIplTxStg->Append("\t\tc_t1_htId = c_t1_htIdPool;\n");

		pIplTxStg->Append("\t\tc_t1_htValid = true;\n");

		if (pMod->m_threads.m_htIdW.AsInt() > 0) {
			pIplTxStg->Append("\t\tm_htIdPool.pop();\n");
		} else {
			pIplTxStg->Append("\t\tc_htBusy = true;\n");
		}

		pIplTxStg->Append("\t\tc_resetInstrStarted = true;\n");

		pIplTxStg->Append("\t} else ");
		pFirst = "";
	}

	if (pMod->m_rsmSrcCnt > 0) {
		pIplTxStg->Append("%sif (r_t1_rsmHtRdy", pFirst);

		if (bResetCnt) pIplTxStg->Append(" && r_resetComplete");

		pIplTxStg->Append(") {\n");

		pIplTxStg->Append("\t\tc_t1_htSel = %s_HT_SEL_SM;\n", pModInst->m_instName.Upper().c_str());

		if (pMod->m_threads.m_htIdW.AsInt() == 0) {

			pIplTxStg->Append("\t\tc_t1_htValid = true;\n");

		} else {
			pIplTxStg->Append("\t\tc_t1_htId = c_t1_rsmHtId;\n");

			if (pMod->m_bCallFork) {
				if (pMod->m_threads.m_htIdW.AsInt() <= 2)
					pIplTxStg->Append("\t\tc_t1_htValid = !r_htPrivLk[INT(c_t1_rsmHtId)];\n");
				else {
					pIplTxStg->Append("\t\tc_t1_htValid = m_rsmPrivLk0.read_mem() == m_rsmPrivLk1.read_mem();\n");
					pIplTxStg->Append("\t\tc_t1_htPrivLkData = !m_rsmPrivLk0.read_mem();\n");
					pIplTxStg->Append("\n");
				}
			} else
				pIplTxStg->Append("\t\tc_t1_htValid = true;\n");

			pIplTxStg->Append("\t\tc_t0_rsmHtRdy = !c_t1_htValid;\n");
		}

		pIplTxStg->Append("\t} else ");
		pFirst = "";
	}

	// first handle calls (they have priority over returns
	for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

		if (pCxrIntf->m_cxrDir == CxrOut) continue;
		if (pCxrIntf->m_cxrType != CxrCall && pCxrIntf->m_cxrType != CxrTransfer) continue;

		if (pCxrIntf->GetQueDepthW() > 0) {
			if (pCxrIntf->m_bCxrIntfFields) {
				if (pMod->m_clkRate == eClk1x && pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate == eClk1x)
					pIplTxStg->Append("%sif (!m_%s_%sQue%s.empty()", pFirst,
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				else
					pIplTxStg->Append("%sif (!r_%s_%sQueEmpty%s", pFirst,
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			} else
				pIplTxStg->Append("%sif (r_%s_%sCnt%s > 0", pFirst,
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
		} else
			pIplTxStg->Append("%sif (r_t1_%s_%sRdy%s", pFirst,
			pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

		if (pCxrIntf->m_cxrType == CxrCall || pCxrIntf->m_cxrType == CxrTransfer) {
			if (pCxrIntf->m_pDstGroup->m_htIdW.AsInt() > 0) {
				if (pMod->m_cxrEntryReserveCnt > 0) {

					for (size_t intfIdx2 = 0; intfIdx2 < pModInst->m_cxrIntfList.size(); intfIdx2 += 1) {
						CCxrIntf * pCxrIntf2 = pModInst->m_cxrIntfList[intfIdx2];

						if (pCxrIntf2->m_cxrDir == CxrOut) continue;
						if (pCxrIntf2->m_cxrType != CxrCall && pCxrIntf2->m_cxrType != CxrTransfer) continue;
						if (pCxrIntf2->m_reserveCnt == 0) continue;
						if (intfIdx2 == intfIdx) continue;

						pIplTxStg->Append(" && !r_%s_%sRsvLimitEmpty%s",
							pCxrIntf2->GetPortNameSrcToDstLc(), pCxrIntf2->GetIntfName(), pCxrIntf->GetPortReplIndex());
					}
				} else
					pIplTxStg->Append(" && !m_htIdPool.empty()");
			} else
				pIplTxStg->Append(" && !r_htBusy");
		}

		if (bResetCnt) pIplTxStg->Append(" && r_resetComplete");

		pIplTxStg->Append(") {\n");

		m_htSelDef.Append("#define %s_HT_SEL_%s_%d_%s %d\n",
			pModInst->m_instName.Upper().c_str(), pCxrIntf->m_pSrcModInst->m_instName.Upper().c_str(), pCxrIntf->GetPortReplId(), pCxrIntf->m_modEntry.Upper().c_str(),
			htSelCnt++);
		pIplTxStg->Append("\t\tc_t1_htSel = %s_HT_SEL_%s_%d_%s;\n",
			pModInst->m_instName.Upper().c_str(), pCxrIntf->m_pSrcModInst->m_instName.Upper().c_str(), pCxrIntf->GetPortReplId(), pCxrIntf->m_modEntry.Upper().c_str());

		if (pCxrIntf->m_pDstGroup->m_htIdW.AsInt() > 0) {
			if (pCxrIntf->m_cxrType == CxrCall || pCxrIntf->m_cxrType == CxrTransfer) {
				pIplTxStg->Append("\t\tc_t1_htId = c_t1_htIdPool;\n");
			} else {
				pIplTxStg->Append("\t\tc_t1_htId = c_t1_%s_%s%s.m_rtnHtId;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			}
		}

		pIplTxStg->Append("\t\tc_t1_htValid = true;\n");

		if (pCxrIntf->GetQueDepthW() == 0)
			pIplTxStg->Append("\t\tc_t0_%s_%sRdy%s = false;\n",
			pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
		else {
			if (pCxrIntf->m_bCxrIntfFields) {
				if (pMod->m_clkRate != eClk1x || pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate != eClk1x)
					pIplTxStg->Append("\t\tc_%s_%sQueEmpty%s = true;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				else
					pIplTxStg->Append("\t\tm_%s_%sQue%s.pop();\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			} else
				pIplTxStg->Append("\t\tc_%s_%sCnt%s -= 1u;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
		}

		pIplTxStg->Append("\t\tc_%s_%sAvl%s = true;\n",
			pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

		if (pMod->m_threads.m_htIdW.AsInt() == 0 && pMod->m_bCallFork)
			pIplTxStg->Append("\t\tc_htPrivLk = true;\n");

		if (pCxrIntf->m_cxrType == CxrCall || pCxrIntf->m_cxrType == CxrTransfer) {
			if (pCxrIntf->m_pDstGroup->m_htIdW.AsInt() > 0) {
				pIplTxStg->Append("\t\tm_htIdPool.pop();\n");

				for (size_t intfIdx2 = 0; intfIdx2 < pModInst->m_cxrIntfList.size(); intfIdx2 += 1) {
					CCxrIntf * pCxrIntf2 = pModInst->m_cxrIntfList[intfIdx2];

					if (pCxrIntf2->m_cxrDir == CxrOut) continue;
					if (pCxrIntf2->m_cxrType != CxrCall && pCxrIntf2->m_cxrType != CxrTransfer) continue;
					if (pCxrIntf2->m_reserveCnt == 0) continue;
					if (intfIdx2 == intfIdx) continue;

					pIplTxStg->Append("\t\tc_%s_%sRsvLimit%s -= 1u;\n",
						pCxrIntf2->GetPortNameSrcToDstLc(), pCxrIntf2->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}

			} else {
				pIplTxStg->Append("\t\tc_htBusy = true;\n");
			}
		}

		if (pCxrIntf->m_pSrcModInst->m_pMod->m_bGvIwComp) {
			pIplTxStg->Append("\t\tc_%s_%sCompCnt%s -= 1u;\n", pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
		}

		pIplTxStg->Append("\t}");
		pFirst = " else ";
	}

	// now handle inbound returns (they get round robin arbitration)
	unsigned selRrIdx = bNeedMifPollQue ? 2 : 1;
	for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

		if (pCxrIntf->m_cxrDir == CxrOut) continue;
		if (pCxrIntf->m_cxrType == CxrCall || pCxrIntf->m_cxrType == CxrTransfer) continue;

		pIplTxStg->Append("%sif (c_%sSel%s", pFirst,
			htSelNameList[selRrIdx].m_name.c_str(), htSelNameList[selRrIdx].m_replIndex.c_str());

		if (bResetCnt) pIplTxStg->Append(" && r_resetComplete");

		pIplTxStg->Append(") {\n");

		m_htSelDef.Append("#define %s_HT_SEL_%s_%d_%s %d\n",
			pModInst->m_instName.Upper().c_str(), pCxrIntf->m_pSrcModInst->m_instName.Upper().c_str(), pCxrIntf->GetPortReplId(), pCxrIntf->m_modEntry.Upper().c_str(),
			htSelCnt++);
		pIplTxStg->Append("\t\tc_t1_htSel = %s_HT_SEL_%s_%d_%s;\n",
			pModInst->m_instName.Upper().c_str(), pCxrIntf->m_pSrcModInst->m_instName.Upper().c_str(), pCxrIntf->GetPortReplId(), pCxrIntf->m_modEntry.Upper().c_str());

		if (pCxrIntf->m_pDstGroup->m_htIdW.AsInt() > 0)
			pIplTxStg->Append("\t\tc_t1_htId = c_t1_%s_%sHtId%s;\n",
			pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

		if (!pMod->m_bCallFork) {
			pIplTxStg->Append("\t\tc_t1_htValid = true;\n");

			if (pCxrIntf->GetQueDepthW() == 0)
				pIplTxStg->Append("\t\tc_t0_%s_%sRdy%s = false;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			else {
				if (pCxrIntf->m_bCxrIntfFields) {
					if (pMod->m_clkRate != eClk1x || pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate != eClk1x)
						pIplTxStg->Append("\t\tc_%s_%sQueEmpty%s = true;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					else
						pIplTxStg->Append("\t\tm_%s_%sQue%s.pop();\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				} else
					pIplTxStg->Append("\t\tc_%s_%sCnt%s -= 1u;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			}

			pIplTxStg->Append("\t\tc_%s_%sAvl%s = true;\n",
				pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

			if (selRrIdx == htSelNameList.size() - 1)
				pIplTxStg->Append("\t\tc_htSelRr = 0x1;\n");
			else
				pIplTxStg->Append("\t\tc_htSelRr = 0x%x;\n", 1 << (selRrIdx + 1));

			if (pCxrIntf->m_pSrcModInst->m_pMod->m_bGvIwComp)
				pIplTxStg->Append("\t\tc_%s_%sCompCnt%s -= 1;\n", pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

		} else {

			if (pMod->m_threads.m_htIdW.AsInt() == 0) {
				pIplTxStg->Append("\t\tc_t1_htValid = true;\n");

				if (pCxrIntf->GetQueDepthW() == 0)
					pIplTxStg->Append("\t\tc_t0_%s_%sRdy%s = false;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				else {
					if (pCxrIntf->m_bCxrIntfFields) {
						if (pMod->m_clkRate != eClk1x || pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate != eClk1x)
							pIplTxStg->Append("\t\tc_%s_%sQueEmpty%s = true;\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
						else
							pIplTxStg->Append("\t\tm_%s_%sQue%s.pop();\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					} else
						pIplTxStg->Append("\t\tc_%s_%sCnt%s -= 1u;\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}

				pIplTxStg->Append("\t\tc_%s_%sAvl%s = true;\n",
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				if (selRrIdx == htSelNameList.size() - 1)
					pIplTxStg->Append("\t\tc_htSelRr = 0x1;\n");
				else
					pIplTxStg->Append("\t\tc_htSelRr = 0x%x;\n", 1 << (selRrIdx + 1));

				if (pCxrIntf->m_pSrcModInst->m_pMod->m_bGvIwComp)
					pIplTxStg->Append("\t\tc_%s_%sCompCnt%s -= 1u;\n", pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				pIplTxStg->Append("\t\tc_htPrivLk = true;\n");

			} else {
				if (pMod->m_threads.m_htIdW.AsInt() <= 2)
					pIplTxStg->Append("\t\tc_t1_htValid = !r_htPrivLk[INT(c_t1_htId)];\n");
				else {
					pIplTxStg->Append("\t\tc_t1_htValid = m_%s_%sPrivLk0%s.read_mem() == m_%s_%sPrivLk1%s.read_mem();\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					pIplTxStg->Append("\t\tc_t1_htPrivLkData = !m_%s_%sPrivLk0%s.read_mem();\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					pIplTxStg->Append("\n");
				}

				pIplTxStg->Append("\t\tif (c_t1_htValid) {\n");

				if (pCxrIntf->GetQueDepthW() == 0)
					pIplTxStg->Append("\t\t\tc_t0_%s_%sRdy%s = false;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				else {
					if (pCxrIntf->m_bCxrIntfFields) {
						if (pMod->m_clkRate != eClk1x || pCxrIntf->m_pSrcModInst->m_pMod->m_clkRate != eClk1x)
							pIplTxStg->Append("\t\t\tc_%s_%sQueEmpty%s = true;\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
						else
							pIplTxStg->Append("\t\t\tm_%s_%sQue%s.pop();\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					} else
						pIplTxStg->Append("\t\t\tc_%s_%sCnt%s -= 1u;\n",
						pCxrIntf->GetSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}

				pIplTxStg->Append("\t\t\tc_%s_%sAvl%s = true;\n",
					pCxrIntf->GetPortNameDstToSrcLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				if (selRrIdx == htSelNameList.size() - 1)
					pIplTxStg->Append("\t\t\tc_htSelRr = 0x1;\n");
				else
					pIplTxStg->Append("\t\t\tc_htSelRr = 0x%x;\n", 1 << (selRrIdx + 1));

				if (pCxrIntf->m_pSrcModInst->m_pMod->m_bGvIwComp)
					pIplTxStg->Append("\t\t\tc_%s_%sCompCnt%s -= 1u;\n", pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				pIplTxStg->Append("\t\t}\n");
			}
		}

		pIplTxStg->Append("\t}");
		pFirst = " else ";

		selRrIdx += 1;
	}

	pIplTxStg->Append(" else if (c_htCmdSel");

	if (bResetCnt) pIplTxStg->Append(" && r_resetComplete");

	pIplTxStg->Append(") {\n");

	if (pModInst->m_cxrSrcCnt > 1) {

		m_htSelDef.Append("#define %s_HT_SEL_SM %d\n",
			pModInst->m_instName.Upper().c_str(), htSelCnt++);
		pIplTxStg->Append("\t\tc_t1_htSel = %s_HT_SEL_SM;\n",
			pModInst->m_instName.Upper().c_str());
	}

	if (!pMod->m_bCallFork || pMod->m_threads.m_htIdW.AsInt() == 0) {
		pIplTxStg->Append("\t\tc_t1_htValid = true;\n");

		if (pMod->m_threads.m_htIdW.AsInt() > 0) {
			pIplTxStg->Append("\t\tc_t1_htId = c_t1_htCmdHtId;\n");
			pIplTxStg->Append("\t\tm_htCmdQue.pop();\n");
			if (pMod->m_bGvIwComp)
				pIplTxStg->Append("\t\tc_htCmdRdyCnt -= 1u;\n");
		} else {
			pIplTxStg->Append("\t\tc_htCmdValid = false;\n");
			if (pMod->m_bGvIwComp)
				pIplTxStg->Append("\t\tc_htCompRdy = false;\n");
			if (pMod->m_threads.m_bCallFork)
				pIplTxStg->Append("\t\tc_htPrivLk = true;\n");
		}

		if (selRrCnt > 1)
			pIplTxStg->Append("\t\tc_htSelRr = 0x2;\n");

	} else {
		pIplTxStg->Append("\t\tc_t1_htId = c_t1_htCmdHtId;\n");

		if (pMod->m_threads.m_htIdW.AsInt() <= 2)
			pIplTxStg->Append("\t\tc_t1_htValid = !r_htPrivLk[INT(c_t1_htCmdHtId)];\n");
		else {
			pIplTxStg->Append("\t\tc_t1_htValid = m_htCmdPrivLk0.read_mem() == m_htCmdPrivLk1.read_mem();\n");
			pIplTxStg->Append("\t\tc_t1_htPrivLkData = !m_htCmdPrivLk0.read_mem();\n");
			pIplTxStg->Append("\n");
		}

		pIplTxStg->Append("\t\tif (c_t1_htValid) {\n");
		pIplTxStg->Append("\t\t\tm_htCmdQue.pop();\n");
		if (pMod->m_bGvIwComp)
			pIplTxStg->Append("\t\t\tc_htCmdRdyCnt -= 1u;\n");

		if (selRrCnt > 1)
			pIplTxStg->Append("\t\t\tc_htSelRr = 0x2;\n");

		pIplTxStg->Append("\t\t}\n");
		pIplTxStg->Append("\n");
	}

	pIplTxStg->Append("\t}");

	if (bNeedMifPollQue) {

		pIplTxStg->Append(" else if (c_mifPollSel");

		if (bResetCnt) pIplTxStg->Append(" && r_resetComplete");

		pIplTxStg->Append(") {\n");

		if (pModInst->m_cxrSrcCnt > 1) {

			m_htSelDef.Append("#define %s_HT_SEL_POLL %d\n",
				pModInst->m_instName.Upper().c_str(), htSelCnt++);
			pIplTxStg->Append("\t\tc_t1_htSel = %s_HT_SEL_POLL;\n",
				pModInst->m_instName.Upper().c_str());
		}

		if (!pMod->m_bCallFork || pMod->m_threads.m_htIdW.AsInt() == 0) {
			pIplTxStg->Append("\t\tc_t1_htValid = true;\n");

			if (pMod->m_threads.m_htIdW.AsInt() > 0) {
				pIplTxStg->Append("\t\tc_t1_htId = c_t1_mifPollHtId;\n");
				pIplTxStg->Append("\t\tm_mifPollQue.pop();\n");
				if (pMod->m_bGvIwComp)
					pIplTxStg->Append("\t\tc_mifPollRdyCnt -= 1u;\n");
			} else {
				pIplTxStg->Append("\t\tc_htCmdValid = false;\n");
				if (pMod->m_threads.m_bCallFork)
					pIplTxStg->Append("\t\tc_htPrivLk = true;\n");
			}

			pIplTxStg->Append("\t\tc_htSelRr = 0x%d;\n", htSelNameList.size() == 2 ? 1 : 4);

		} else {
			pIplTxStg->Append("\t\tc_t1_htId = c_t1_mifPollHtId;\n");

			if (pMod->m_threads.m_htIdW.AsInt() <= 2)
				pIplTxStg->Append("\t\tc_t1_htValid = !r_htPrivLk[INT(c_t1_mifPollHtId)];\n");
			else {
				pIplTxStg->Append("\t\tc_t1_htValid = m_htCmdPrivLk0.read_mem() == m_htCmdPrivLk1.read_mem();\n");
				pIplTxStg->Append("\t\tc_t1_htPrivLkData = !m_htCmdPrivLk0.read_mem();\n");
				pIplTxStg->Append("\n");
			}

			pIplTxStg->Append("\t\tif (c_t1_htValid) {\n");
			pIplTxStg->Append("\t\t\tm_mifPollQue.pop();\n");

			pIplTxStg->Append("\t\tc_htSelRr = 0x%d;\n", htSelNameList.size() == 2 ? 1 : 4);

			pIplTxStg->Append("\t\t}\n");
			pIplTxStg->Append("\n");
		}

		pIplTxStg->Append("\t}");
	}
	pIplTxStg->Append(" \n");
	pIplTxStg->NewLine();

	HtlAssert(rsvSelCnt == htSelCnt);

	if (pMod->m_bGvIwComp) {
		pIplTxStg->Append("\tif (c_t1_htValid)\n");
		pIplTxStg->Append("\t\tc_htCompQueAvlCnt -= 1;\n");
		pIplTxStg->NewLine();
	}

	char inStgStr[8];
	char outStgStr[8];
	char regStgStr[8];

	const char *pInStg = inStgStr;
	const char *pOutStg = outStgStr;
	const char *pRegStg = regStgStr;

	int iplStg = 1;

	if (pMod->m_clkRate == eClk2x) {

		sprintf(inStgStr, "r_t%d", iplStg);
		sprintf(outStgStr, "c_t%d", iplStg);
		sprintf(regStgStr, "r_t%d", iplStg + 1);

		GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelType, VA("%s_htSel", pRegStg));
		iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);

		if (pMod->m_threads.m_htIdW.AsInt() > 0) {
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg));
			iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
		}

		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrOut) continue;
			if (!pCxrIntf->m_bCxrIntfFields) continue;

			if (pCxrIntf->GetPortReplCnt() <= 1 || pCxrIntf->GetPortReplId() == 0)
				m_iplRegDecl.Append("\tC%s_%s %s_%s_%s%s;\n",
				pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
				pRegStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

			iplReg.Append("\t%s_%s_%s%s = %s_%s_%s%s;\n",
				pRegStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
		}

		GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg));
		iplReg.Append("\t%s_htValid = !%s && %s_htValid;\n",
			pRegStg, reset.c_str(), pOutStg);

		m_iplRegDecl.Append("\tCHtCmd %s_htCmd;\n",
			pRegStg);
		iplReg.Append("\t%s_htCmd = %s_htCmd;\n",
			pRegStg, pOutStg);

		if (bNeedMifPollQue) {
			m_iplRegDecl.Append("\tCHtCmd %s_mifPollCmd;\n",
				pRegStg);
			iplReg.Append("\t%s_mifPollCmd = %s_mifPollCmd;\n",
				pRegStg, pOutStg);
		}

		iplStg += 1;
	}

	pIplTxStg->NewLine();

	///////////////////////////////////////////////////////////////////////
	// Second stage: ht Id select, input is registered if 2x clock

	//pIplTxStg->Append("\t// htId select\n");

	sprintf(inStgStr, "r_t%d", iplStg);
	sprintf(outStgStr, "c_t%d", iplStg);
	sprintf(regStgStr, "r_t%d", iplStg + 1);

	if (bMultiThread) {

		if (pMod->m_clkRate == eClk2x)
			pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n", pOutStg, pInStg);
		GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg));
		iplReg.Append("\t%s_htValid = !%s && %s_htValid;\n", pRegStg, reset.c_str(), pOutStg);

		if (pModInst->m_cxrSrcCnt > 1) {
			if (pMod->m_clkRate == eClk2x)
				pIplTxStg->Append("\t%s %s_htSel = %s_htSel;\n", htSelType.c_str(), pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelType, VA("%s_htSel", pRegStg));
			iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
		}

		if (pMod->m_clkRate == eClk2x)
			pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);
		GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("sc_uint<%s_HTID_W>", pMod->m_modName.Upper().c_str()), VA("%s_htId", pRegStg));
		iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);

		// generate cmds
		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrOut) continue;
			if (!pCxrIntf->m_bCxrIntfFields) continue;

			if (pMod->m_clkRate == eClk2x) {
				if (pCxrIntf->GetPortReplCnt() <= 1)
					pIplTxStg->Append("\tC%s_%s %s_%s_%s = %s_%s_%s;\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(),
					pInStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName());
				else {
					if (pCxrIntf->GetPortReplId() == 0)
						pIplTxStg->Append("\tC%s_%s %s_%s_%s%s;\n",
						pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
						pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

					pIplTxStg->Append("\t%s_%s_%s%s = %s_%s_%s%s;\n",
						pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
						pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pInStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
			}

			if (pCxrIntf->GetPortReplCnt() <= 1 || pCxrIntf->GetPortReplId() == 0)
				m_iplRegDecl.Append("\tC%s_%s %s_%s_%s%s;\n",
				pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
				pRegStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

			iplReg.Append("\t%s_%s_%s%s = %s_%s_%s%s;\n",
				pRegStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
		}

		if (pMod->m_clkRate == eClk2x)
			pIplTxStg->Append("\tCHtCmd %s_htCmd = %s_htCmd;\n", pOutStg, pInStg);

		m_iplRegDecl.Append("\tCHtCmd %s_htCmd;\n", pRegStg);
		iplReg.Append("\t%s_htCmd = %s_htCmd;\n", pRegStg, pOutStg);

		if (bNeedMifPollQue) {
			if (pMod->m_clkRate == eClk2x)
				pIplTxStg->Append("\tCHtCmd %s_mifPollCmd = %s_mifPollCmd;\n", pOutStg, pInStg);

			m_iplRegDecl.Append("\tCHtCmd %s_mifPollCmd;\n", pRegStg);
			iplReg.Append("\t%s_mifPollCmd = %s_mifPollCmd;\n", pRegStg, pOutStg);
		}

		if (pMod->m_threads.m_htIdW.AsInt() > 2 && pMod->m_bCallFork) {
			if (pMod->m_clkRate == eClk2x) {
				pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n",
					pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg));
				iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
			}
		}

		IplNewLine(pIplTxStg);

		if (pMod->m_rsmSrcCnt > 0) {

			if (pMod->m_clkRate == eClk2x || !pMod->m_bMultiThread)
				pIplTxStg->Append("\tbool %s_rsmHtRdy = %s_rsmHtRdy;\n", pOutStg, pInStg);

			if (pMod->m_bMultiThread) {
				m_iplRegDecl.Append("\tbool %s_rsmHtRdy;\n", pRegStg);
				iplReg.Append("\t%s_rsmHtRdy = %s_rsmHtRdy;\n", pRegStg, pOutStg);
			}

			if (pMod->m_clkRate == eClk2x || !pMod->m_bMultiThread)
				pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_rsmHtInstr = %s_rsmHtInstr;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);

			if (pMod->m_bMultiThread) {
				m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> %s_rsmHtInstr;\n", pMod->m_modName.Upper().c_str(), pRegStg);
				iplReg.Append("\t%s_rsmHtInstr = %s_rsmHtInstr;\n", pRegStg, pOutStg);
			}
		}

		IplNewLine(pIplTxStg);

		if (pMod->m_bCallFork) {
			if (pMod->m_threads.m_htIdW.AsInt() <= 2) {
				pIplTxStg->Append("\tht_uint%d c_htPrivLk = r_htPrivLk;\n",
					1 << pMod->m_threads.m_htIdW.AsInt());
				pIplTxStg->NewLine();

				pIplTxStg->Append("\tif (c_t1_htValid)\n");
				pIplTxStg->Append("\t\tc_htPrivLk[INT(c_t1_htId)] = true;\n");
				pIplTxStg->NewLine();
			} else {
				pIplTxStg->Append("\tif (c_t1_htValid) {\n");
				for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
					CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

					if (pCxrIntf->m_cxrDir == CxrOut) continue;
					if (pCxrIntf->m_cxrType == CxrCall || pCxrIntf->m_cxrType == CxrTransfer) continue;

					pIplTxStg->Append("\t\tm_%s_%sPrivLk0%s.write_addr(c_t1_htId);\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					pIplTxStg->Append("\t\tm_%s_%sPrivLk0%s.write_mem(c_t1_htPrivLkData);\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
				pIplTxStg->Append("\t\tm_htCmdPrivLk0.write_addr(c_t1_htId);\n");
				pIplTxStg->Append("\t\tm_htCmdPrivLk0.write_mem(c_t1_htPrivLkData);\n");

				if (pMod->m_rsmSrcCnt > 0) {
					pIplTxStg->Append("\t\tm_rsmPrivLk0.write_addr(c_t1_htId);\n");
					pIplTxStg->Append("\t\tm_rsmPrivLk0.write_mem(c_t1_htPrivLkData);\n");
				}

				pIplTxStg->Append("\t}\n");
				pIplTxStg->NewLine();
			}
		}
	}

	///////////////////////////////////////////////////////////////////////
	// Third stage: htPriv ram read, input is registered

	if (pMod->m_bMultiThread) {
		iplStg += 1;

		sprintf(inStgStr, "r_t%d", iplStg);
		sprintf(outStgStr, "c_t%d", iplStg);
		sprintf(regStgStr, "r_t%d", iplStg + 1);

		pIplTxStg = &iplT2Stg;

		pIplTxStg->Append("\t// htPriv access\n");

		pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n",
			pOutStg, pInStg);
		if (pMod->m_clkRate == eClk2x) {
			m_iplRegDecl.Append("\tbool %s_htValid;\n",
				pRegStg);
			iplReg.Append("\t%s_htValid = !%s && %s_htValid;\n",
				pRegStg, reset.c_str(), pOutStg);
		}

		if (pModInst->m_cxrSrcCnt > 1) {
			pIplTxStg->Append("\t%s %s_htSel = %s_htSel;\n", htSelType.c_str(), pOutStg, pInStg);
			if (pMod->m_clkRate == eClk2x) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelType, VA("%s_htSel", pRegStg));
				iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
			}
		}

		if (bMultiThread) {
			if (pMod->m_bHtIdInit && iplStg == pMod->m_execStg - 1) {
				pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htIdInit ? (sc_uint<%s_HTID_W>)(%s_htId+1) : %s_htId;\n",
					pMod->m_modName.Upper().c_str(), pOutStg, pRegStg, pMod->m_modName.Upper().c_str(), pRegStg, pInStg);
				pIplTxStg->Append("\tbool %s_htIdInit = %s_htIdInit && %s_htId != 0x%x;\n",
					pOutStg, pRegStg, pRegStg, (1 << pMod->m_threads.m_htIdW.AsInt()) - 1);
			} else
				pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);

			if (pMod->m_clkRate == eClk2x) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg));
				iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
			}
		}

		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrOut) continue;
			if (!pCxrIntf->m_bCxrIntfFields) continue;

			if (pMod->m_clkRate == eClk2x) {
				if (pCxrIntf->GetPortReplId() == 0)
					m_iplRegDecl.Append("\tC%s_%s %s_%s_%s%s;\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pRegStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

				iplReg.Append("\t%s_%s_%s%s = %s_%s_%s%s;\n",
					pRegStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			}

			if (pCxrIntf->GetPortReplCnt() <= 1)
				pIplTxStg->Append("\tC%s_%s %s_%s_%s = %s_%s_%s;\n",
				pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
				pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(),
				pInStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName());
			else {
				if (pCxrIntf->GetPortReplId() == 0)
					pIplTxStg->Append("\tC%s_%s %s_%s_%s%s;\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

				pIplTxStg->Append("\t%s_%s_%s%s = %s_%s_%s%s;\n",
					pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pInStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			}
		}

		if (pMod->m_clkRate == eClk2x) {
			m_iplRegDecl.Append("\tCHtCmd %s_htCmd;\n", pRegStg);

			iplReg.Append("\t%s_htCmd = %s_htCmd;\n", pRegStg, pOutStg);
		}

		pIplTxStg->Append("\tCHtCmd %s_htCmd = %s_htCmd;\n", pOutStg, pInStg);

		IplNewLine(pIplTxStg);

		if (bNeedMifPollQue) {
			if (pMod->m_clkRate == eClk2x) {
				m_iplRegDecl.Append("\tCHtCmd %s_mifPollCmd;\n", pRegStg);

				iplReg.Append("\t%s_mifPollCmd = %s_mifPollCmd;\n", pRegStg, pOutStg);
			}

			pIplTxStg->Append("\tCHtCmd %s_mifPollCmd = %s_mifPollCmd;\n", pOutStg, pInStg);
		}

		IplNewLine(pIplTxStg);

		if (pMod->m_clkRate == eClk2x) {
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("CHtPriv"), VA("%s_htPriv", pRegStg));
			iplReg.Append("\t%s_htPriv = %s_htPriv;\n",
				pRegStg, pOutStg);

			if (pMod->m_threads.m_htIdW.AsInt() > 2 && pMod->m_bCallFork) {
				pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg));
				iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
			}
		}

		IplNewLine(pIplTxStg);

		if (pMod->m_threads.m_ramType == eDistRam)
			pIplTxStg->Append("\tm_htPriv.read_addr(%s_htId);\n", pInStg);
		else {
			string prevOutStg = pOutStg;
			prevOutStg[prevOutStg.size() - 1] -= 1;
			pIplTxStg->Append("\tm_htPriv.read_addr(%s_htId);\n",
				prevOutStg.c_str());
		}
		pIplTxStg->Append("\tCHtPriv %s_htPriv = m_htPriv.read_mem();\n",
			pOutStg);

		pIplTxStg->Append("\n");
	}

	IplNewLine(pIplTxStg);

	if (pMod->m_rsmSrcCnt > 0) {

		if (pMod->m_threads.m_htIdW.AsInt() > 0) {
			m_iplRegDecl.Append("#\tifndef _HTV\n");
			m_iplRegDecl.Append("\tht_dist_ram<bool, %s_HTID_W> m_htRsmWait;\n", pMod->m_modName.Upper().c_str());
			m_iplRegDecl.Append("#\tendif\n");

			iplReg.Append("#\tifndef _HTV\n");
			iplReg.Append("\tm_htRsmWait.clock();\n");
			iplReg.Append("#\tendif\n");

			m_iplCtorInit.Append("\t\tfor (int i = 0; i < (1 << %s_HTID_W); i += 1)\n", pMod->m_modName.Upper().c_str());
			m_iplCtorInit.Append("\t\t\tm_htRsmWait.write_mem_debug(i) = false;\n");
		} else {
			m_iplRegDecl.Append("#\tifndef _HTV\n");
			m_iplRegDecl.Append("\tbool c_htRsmWait;\n");
			m_iplRegDecl.Append("\tbool r_htRsmWait;\n");
			m_iplRegDecl.Append("#\tendif\n");

			iplPostInstr.Append("#\tifndef _HTV\n");
			iplPostInstr.Append("\tc_htRsmWait = r_htRsmWait;\n");
			iplPostInstr.Append("#\tendif\n");

			iplReg.Append("#\tifndef _HTV\n");
			iplReg.Append("\tr_htRsmWait = c_htRsmWait;\n");
			iplReg.Append("#\tendif\n");

			m_iplCtorInit.Append("\t\tr_htRsmWait = false;\n");
		}

		if (pMod->m_threads.m_bPause) {
			if (pMod->m_threads.m_htIdW.AsInt() > 0 && pMod->m_rsmSrcCnt > 1) {
				m_iplRegDecl.Append("\tht_dist_que<CHtCmd, %s_HTID_W> m_htRsmQue;\n", pMod->m_modName.Upper().c_str());
				iplReg.Append("\tm_htRsmQue.clock(%s);\n", reset.c_str());
			}

			if (pMod->m_threads.m_htIdW.AsInt() > 0) {
				m_iplRegDecl.Append("\tht_dist_ram<sc_uint<%s_INSTR_W>, %s_HTID_W> m_htRsmInstr;\n",
					pMod->m_modName.Upper().c_str(), pMod->m_modName.Upper().c_str());
				iplReg.Append("\tm_htRsmInstr.clock();\n");
			} else {
				m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> c_htRsmInstr;\n", pMod->m_modName.Upper().c_str());
				iplPostInstr.Append("\tc_htRsmInstr = r_htRsmInstr;\n");
				m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> r_htRsmInstr;\n", pMod->m_modName.Upper().c_str());
				iplReg.Append("\tr_htRsmInstr = c_htRsmInstr;\n");
			}
		}

		m_iplRegDecl.Append("\tbool c_t0_rsmHtRdy;\n");
		iplT0Stg.Append("\tc_t0_rsmHtRdy = false;\n");
		m_iplRegDecl.Append("\tbool r_t1_rsmHtRdy;\n");
		iplReg.Append("\tr_t1_rsmHtRdy = c_t0_rsmHtRdy;\n");

		if (pMod->m_threads.m_htIdW.AsInt() > 0) {
			m_iplRegDecl.Append("\tsc_uint<%s_HTID_W> c_t0_rsmHtId;\n", pMod->m_modName.Upper().c_str());
			iplT0Stg.Append("\tc_t0_rsmHtId = r_t1_rsmHtId;\n");
			m_iplRegDecl.Append("\tsc_uint<%s_HTID_W> r_t1_rsmHtId;\n", pMod->m_modName.Upper().c_str());
			iplReg.Append("\tr_t1_rsmHtId = c_t0_rsmHtId;\n");
		}

		m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> c_t0_rsmHtInstr;\n", pMod->m_modName.Upper().c_str());
		iplT0Stg.Append("\tc_t0_rsmHtInstr = r_t1_rsmHtInstr;\n");
		m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> r_t1_rsmHtInstr;\n", pMod->m_modName.Upper().c_str());
		iplReg.Append("\tr_t1_rsmHtInstr = c_t0_rsmHtInstr;\n");

		if (pMod->m_threads.m_bPause) {
			if (pMod->m_threads.m_htIdW.AsInt() > 0 && pMod->m_rsmSrcCnt > 1) {
				iplPostInstr.Append("\tif (!m_htRsmQue.empty() && !c_t0_rsmHtRdy) {\n");
				iplPostInstr.Append("\t\tc_t0_rsmHtRdy = true;\n");
				iplPostInstr.Append("\t\tc_t0_rsmHtId = m_htRsmQue.front().m_htId;\n");
				iplPostInstr.Append("\t\tc_t0_rsmHtInstr = m_htRsmQue.front().m_htInstr;\n");
				iplPostInstr.Append("\t\tm_htRsmQue.pop();\n");
				iplPostInstr.Append("\t}\n");
			}
		}

		iplPostInstr.NewLine();
		m_iplRegDecl.NewLine();

		if (pMod->m_bMultiThread)
			pIplTxStg->Append("\tbool %s_rsmHtRdy = %s_rsmHtRdy;\n", pOutStg, pInStg);

		if (pMod->m_clkRate == eClk2x && pMod->m_bMultiThread) {
			m_iplRegDecl.Append("\tbool %s_rsmHtRdy;\n", pRegStg);
			iplReg.Append("\t%s_rsmHtRdy = %s_rsmHtRdy;\n", pRegStg, pOutStg);
		}

		if (pMod->m_bMultiThread)
			pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_rsmHtInstr = %s_rsmHtInstr;\n",
			pMod->m_modName.Upper().c_str(), pOutStg, pInStg);

		if (pMod->m_clkRate == eClk2x && pMod->m_bMultiThread) {
			m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> %s_rsmHtInstr;\n", pMod->m_modName.Upper().c_str(), pRegStg);
			iplReg.Append("\t%s_rsmHtInstr = %s_rsmHtInstr;\n", pRegStg, pOutStg);
		}
	}

	IplNewLine(pIplTxStg);

	////////////////////////////////////////////////////////////////////////////
	// Forth stage: htPriv / input command select

	int rdPrivLkStg;
	{
		if (pMod->m_clkRate == eClk2x && bMultiThread)
			iplStg += 1;

		rdPrivLkStg = iplStg;

		sprintf(inStgStr, "r_t%d", iplStg);
		sprintf(outStgStr, "c_t%d", iplStg);
		sprintf(regStgStr, "r_t%d", iplStg + 1);

		pIplTxStg = pMod->m_bMultiThread ? &iplT2Stg : &iplT1Stg;

		pIplTxStg->Append("\t// htPriv / input command select\n");

		if (pMod->m_clkRate == eClk2x)
			pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n",
			pOutStg, pInStg);
		GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg));
		iplReg.Append("\t%s_htValid = !%s && %s_htValid;\n",
			pRegStg, reset.c_str(), pOutStg);

		if (pModInst->m_cxrSrcCnt > 1) {
			if (pMod->m_clkRate == eClk2x)
				pIplTxStg->Append("\t%s %s_htSel = %s_htSel;\n", htSelType.c_str(), pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelType, VA("%s_htSel", pRegStg));
			iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
		}

		if (bMultiThread) {

			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg));

			if (pMod->m_bHtIdInit && iplStg == pMod->m_execStg - 1) {
				m_iplRegDecl.Append("\tbool %s_htIdInit;\n", pRegStg);

				if (pMod->m_clkRate == eClk2x) {
					pIplTxStg->Append("\tbool %s_htIdInit = %s_htIdInit && %s_htId != 0x%x;\n",
						pOutStg, pRegStg, pRegStg, (1 << pMod->m_threads.m_htIdW.AsInt()) - 1);

					pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htIdInit ? (sc_uint<%s_HTID_W>)(%s_htId+1) : %s_htId;\n",
						pMod->m_modName.Upper().c_str(), pOutStg, pRegStg, pMod->m_modName.Upper().c_str(), pRegStg, pInStg);
				}

				iplReg.Append("\t%s_htId = %s ? (sc_uint<%s>)0 : %s_htId;\n", pRegStg, reset.c_str(), pMod->m_threads.m_htIdW.c_str(), pOutStg);

				iplReg.Append("\t%s_htIdInit = %s || %s_htIdInit;\n", pRegStg, reset.c_str(), pOutStg);
			} else {
				if (pMod->m_clkRate == eClk2x)
					pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);
				iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
			}
		}

		if (pMod->m_threads.m_htIdW.AsInt() == 0)
			pIplTxStg->Append("\tCHtPriv %s_htPriv = r_htPriv;\n", pOutStg);
		else if (pMod->m_clkRate == eClk2x)
			pIplTxStg->Append("\tCHtPriv %s_htPriv = %s_htPriv;\n", pOutStg, pInStg);

		GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("CHtPriv"), VA("%s_htPriv", pRegStg));
		iplReg.Append("\t%s_htPriv = %s_htPriv;\n", pRegStg, pOutStg);

		if (pMod->m_threads.m_htIdW.AsInt() > 2 && pMod->m_bCallFork) {
			pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg));
			iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
		}

		pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_htInstr;\n", pMod->m_modName.Upper().c_str(), pOutStg);
		GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htInstrTypeStr, VA("%s_htInstr", pRegStg));
		iplReg.Append("\t%s_htInstr = %s_htInstr;\n", pRegStg, pOutStg);
		pIplTxStg->Append("\n");

		if (pMod->m_bCallFork) {
			pIplTxStg->Append("\tbool %s_rtnJoin = false;\n", pOutStg);
			m_iplRegDecl.Append("\tbool ht_noload %s_rtnJoin;\n", pRegStg);
			iplReg.Append("\t%s_rtnJoin = %s_rtnJoin;\n", pRegStg, pOutStg);
		}

		if (pMod->m_clkRate == eClk2x) {
			for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

				if (pCxrIntf->m_cxrDir == CxrOut) continue;
				if (!pCxrIntf->m_bCxrIntfFields) continue;

				if (pCxrIntf->GetPortReplCnt() <= 1)
					pIplTxStg->Append("\tC%s_%s %s_%s_%s = %s_%s_%s;\n",
					pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
					pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(),
					pInStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName());
				else {
					if (pCxrIntf->GetPortReplId() == 0)
						pIplTxStg->Append("\tC%s_%s %s_%s_%s%s;\n",
						pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName(),
						pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());

					pIplTxStg->Append("\t%s_%s_%s%s = %s_%s_%s%s;\n",
						pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
						pInStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
			}

			pIplTxStg->Append("\tCHtCmd %s_htCmd = %s_htCmd;\n", pOutStg, pInStg);

			if (pMod->m_rsmSrcCnt > 0) {
				pIplTxStg->Append("\tbool %s_rsmHtRdy = %s_rsmHtRdy;\n", pOutStg, pInStg);

				pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_rsmHtInstr = %s_rsmHtInstr;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);
			}
		}

		pIplTxStg->Append("\n");

		pIplTxStg->Append("\tswitch (%s_htSel) {\n", pOutStg);

		if (pMod->m_threads.m_resetInstr.size() > 0) {
			pIplTxStg->Append("\tcase %s_HT_SEL_RESET:\n", pModInst->m_instName.Upper().c_str());
			pIplTxStg->Append("\t\t%s_htInstr = %s;\n", pOutStg, pMod->m_threads.m_resetInstr.c_str());
			pIplTxStg->Append("\t\tbreak;\n");
		}

		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrOut) continue;

			pIplTxStg->Append("\tcase %s_HT_SEL_%s_%d_%s:\n",
				pModInst->m_instName.Upper().c_str(), pCxrIntf->m_pSrcModInst->m_instName.Upper().c_str(), pCxrIntf->GetPortReplId(), pCxrIntf->m_modEntry.Upper().c_str());

			if (pCxrIntf->IsCallOrXfer()) {
				pIplTxStg->Append("\t\t%s_htInstr = %s;\n",
					pOutStg, pCxrIntf->m_entryInstr.c_str());

				// clear priv state on call entry
				pIplTxStg->Append("\t\t%s_htPriv = 0;\n",
					pOutStg);

				if (pCxrIntf->m_pDstGroup->m_rtnSelW > 0) {
					if (pCxrIntf->IsCall()) {
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnSel = %d;\n",
							pOutStg,
							pCxrIntf->m_rtnSelId);
					} else {
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnSel = %s_%s_%s%s.m_rtnSel;\n",
							pOutStg,
							pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					}
				}

				// call and xfer must save caller's htId, returns do not
				if (pCxrIntf->IsCall()) {
					if (pCxrIntf->m_pSrcModInst->m_pMod->m_instrW > 0)
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnInstr = (%sRtnInstr_t)%s_%s_%s%s.m_rtnInstr;\n",
						pOutStg,
						pMod->m_modName.Uc().c_str(),
						pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

					if (pCxrIntf->m_pSrcGroup->m_htIdW.AsInt() > 0)
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnHtId = (%sRtnHtId_t)%s_%s_%s%s.m_rtnHtId;\n",
						pOutStg,
						pMod->m_modName.Uc().c_str(),
						pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					if (pMod->m_cxrEntryReserveCnt > 0) {
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnRsvSel = %s_HT_SEL_%s_%d_%s;\n",
							pOutStg,
							pModInst->m_instName.Upper().c_str(), pCxrIntf->m_pSrcModInst->m_instName.Upper().c_str(), pCxrIntf->GetPortReplId(), pCxrIntf->m_modEntry.Upper().c_str());
					}
				} else {
					if (pCxrIntf->m_pSrcGroup->m_rtnInstrW > 0)
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnInstr = (%sRtnInstr_t)%s_%s_%s%s.m_rtnInstr;\n",
						pOutStg,
						pMod->m_modName.Uc().c_str(),
						pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

					if (pCxrIntf->m_pSrcGroup->m_rtnHtIdW > 0)
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnHtId = (%sRtnHtId_t)%s_%s_%s%s.m_rtnHtId;\n",
						pOutStg,
						pMod->m_modName.Uc().c_str(),
						pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}

				if (pMod->m_bRtnJoin) {
					if (pCxrIntf->m_bRtnJoin)
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnJoin = %s_%s_%s%s.m_rtnJoin;\n",
						pOutStg,
						pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
					else
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnJoin = false;\n",
						pOutStg);
				}

				if (pMod->m_maxRtnReplCnt > 1) {
					pIplTxStg->Append("\t\t%s_htPriv.m_rtnReplSel = %d;\n",
						pOutStg,
						pCxrIntf->m_rtnReplId);
				}

			} else {
				pIplTxStg->Append("\t\t%s_htInstr = %s_%s_%s%s.m_rtnInstr;\n",
					pOutStg,
					pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				CCxrIntf * pCxrCallIntf = pModInst->m_cxrIntfList[pCxrIntf->m_callIntfIdx];
				CCxrCall * pCxrCall = pModInst->m_pMod->m_cxrCallList[pCxrCallIntf->m_callIdx];

				if (pCxrCall->m_bCallFork || pCxrCall->m_bXferFork) {
					pIplTxStg->Append("\t\t%s_rtnJoin = %s_%s_%s%s.m_rtnJoin;\n",
						pOutStg,
						pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
			}

			for (size_t fldIdx = 0; fldIdx < pCxrIntf->m_pFieldList->size(); fldIdx += 1) {
				const char * pFieldName = (*pCxrIntf->m_pFieldList)[fldIdx]->m_name.c_str();

				pIplTxStg->Append("\t\t%s_htPriv.m_%s = %s_%s_%s%s.m_%s;\n",
					pOutStg, pFieldName,
					pOutStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(), pFieldName);
			}

			pIplTxStg->Append("\t\tbreak;\n");
		}

		pIplTxStg->Append("\tcase %s_HT_SEL_SM:\n", pModInst->m_instName.Upper().c_str());

		if (pMod->m_rsmSrcCnt > 0) {
			pIplTxStg->Append("\t\tif (%s_rsmHtRdy)\n", pOutStg);
			pIplTxStg->Append("\t\t\t%s_htInstr = %s_rsmHtInstr;\n", pOutStg, pOutStg);
			pIplTxStg->Append("\t\telse\n\t");
		}

		pIplTxStg->Append("\t\t%s_htInstr = %s_htCmd.m_htInstr;\n", pOutStg, pOutStg);
		pIplTxStg->Append("\t\tbreak;\n");

		if (bNeedMifPollQue) {
			pIplTxStg->Append("\tcase %s_HT_SEL_POLL:\n", pModInst->m_instName.Upper().c_str());
			pIplTxStg->Append("\t\t%s_htInstr = %s_mifPollCmd.m_htInstr;\n", pOutStg, pOutStg);
			pIplTxStg->Append("\t\tbreak;\n");
		}

		pIplTxStg->Append("\tdefault:\n");
		pIplTxStg->Append("\t\tassert(0);\n");
		pIplTxStg->Append("\t}\n");

		IplNewLine(pIplTxStg);

		if (pMod->m_gblBlockRam) {
			IplComment(pIplTxStg, iplReg, "\t// External ram stage #1\n");

			iplStg += 1;
			sprintf(inStgStr, "r_t%d", iplStg);
			sprintf(outStgStr, "c_t%d", iplStg);
			sprintf(regStgStr, "r_t%d", iplStg + 1);

			pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg));
			iplReg.Append("\t%s_htValid = !%s && %s_htValid;\n", pRegStg, reset.c_str(), pOutStg);

			if (pModInst->m_cxrSrcCnt > 1 /*&& !bSingleUnnamedGroup*/) {
				pIplTxStg->Append("\t%s %s_htSel = %s_htSel;\n", htSelType.c_str(), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelType, VA("%s_htSel", pRegStg));
				iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
			}

			if (bMultiThread) {
				pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg));
				iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
			}

			pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_htInstr = %s_htInstr;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htInstrTypeStr, VA("%s_htInstr", pRegStg));
			iplReg.Append("\t%s_htInstr = %s_htInstr;\n", pRegStg, pOutStg);

			if (pMod->m_bCallFork) {
				pIplTxStg->Append("\tbool %s_rtnJoin = %s_rtnJoin;\n", pOutStg, pInStg);
				m_iplRegDecl.Append("\tbool %s_rtnJoin;\n", pRegStg);
				iplReg.Append("\t%s_rtnJoin = %s_rtnJoin;\n", pRegStg, pOutStg);
			}

			pIplTxStg->Append("\tCHtPriv %s_htPriv = %s_htPriv;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("CHtPriv"), VA("%s_htPriv", pRegStg));
			iplReg.Append("\t%s_htPriv = %s_htPriv;\n", pRegStg, pOutStg);

			if (pMod->m_threads.m_htIdW.AsInt() > 2 && pMod->m_bCallFork) {
				pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg));
				iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
			}

			IplComment(pIplTxStg, iplReg, "\t// External ram stage #2\n");

			iplStg += 1;
			sprintf(inStgStr, "r_t%d", iplStg);
			sprintf(outStgStr, "c_t%d", iplStg);
			sprintf(regStgStr, "r_t%d", iplStg + 1);

			pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg));
			iplReg.Append("\t%s_htValid = !%s && %s_htValid;\n", pRegStg, reset.c_str(), pOutStg);

			if (pModInst->m_cxrSrcCnt > 1 /*&& !bSingleUnnamedGroup*/) {
				pIplTxStg->Append("\t%s %s_htSel = %s_htSel;\n", htSelType.c_str(), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelType, VA("%s_htSel", pRegStg));
				iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
			}

			if (bMultiThread) {
				pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg));
				iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
			}

			pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_htInstr = %s_htInstr;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htInstrTypeStr, VA("%s_htInstr", pRegStg));
			iplReg.Append("\t%s_htInstr = %s_htInstr;\n", pRegStg, pOutStg);

			if (pMod->m_bCallFork) {
				pIplTxStg->Append("\tbool %s_rtnJoin = %s_rtnJoin;\n", pOutStg, pInStg);
				m_iplRegDecl.Append("\tbool %s_rtnJoin;\n", pRegStg);
				iplReg.Append("\t%s_rtnJoin = %s_rtnJoin;\n", pRegStg, pOutStg);
			}

			pIplTxStg->Append("\tCHtPriv %s_htPriv = %s_htPriv;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("CHtPriv"), VA("%s_htPriv", pRegStg));
			iplReg.Append("\t%s_htPriv = %s_htPriv;\n", pRegStg, pOutStg);

			if (pMod->m_threads.m_htIdW.AsInt() > 2 && pMod->m_bCallFork) {
				pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg));
				iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
			}

			IplComment(pIplTxStg, iplReg, "\t// External ram stage #3\n");

			iplStg += 1;
			sprintf(inStgStr, "r_t%d", iplStg);
			sprintf(outStgStr, "c_t%d", iplStg);
			sprintf(regStgStr, "r_t%d", iplStg + 1);

			pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg));
			iplReg.Append("\t%s_htValid = !%s && %s_htValid;\n", pRegStg, reset.c_str(), pOutStg);

			if (pModInst->m_cxrSrcCnt > 1) {
				pIplTxStg->Append("\t%s %s_htSel = %s_htSel;\n", htSelType.c_str(), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelType, VA("%s_htSel", pRegStg));
				iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
			}

			if (bMultiThread) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg));

				if (pMod->m_bHtIdInit && iplStg == pMod->m_execStg - 1) {
					m_iplRegDecl.Append("\tbool %s_htIdInit;\n", pRegStg);
					pIplTxStg->Append("\tbool %s_htIdInit = %s_htIdInit && %s_htId != 0x%x;\n",
						pOutStg, pRegStg, pRegStg, (1 << pMod->m_threads.m_htIdW.AsInt()) - 1);

					pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htIdInit ? (sc_uint<%s_HTID_W>)(%s_htId+1) : %s_htId;\n",
						pMod->m_modName.Upper().c_str(), pOutStg, pRegStg, pMod->m_modName.Upper().c_str(), pRegStg, pInStg);
					iplReg.Append("\t%s_htId = %s ? (sc_uint<%s>)0 : %s_htId;\n",
						pRegStg, reset.c_str(), pMod->m_threads.m_htIdW.c_str(), pOutStg);

					iplReg.Append("\t%s_htIdInit = %s || %s_htIdInit;\n", pRegStg, reset.c_str(), pOutStg);
				} else {
					pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);
					iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
				}
			}

			pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_htInstr = %s_htInstr;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htInstrTypeStr, VA("%s_htInstr", pRegStg));
			iplReg.Append("\t%s_htInstr = %s_htInstr;\n", pRegStg, pOutStg);

			if (pMod->m_bCallFork) {
				pIplTxStg->Append("\tbool %s_rtnJoin = %s_rtnJoin;\n", pOutStg, pInStg);
				m_iplRegDecl.Append("\tbool ht_noload %s_rtnJoin;\n", pRegStg);
				iplReg.Append("\t%s_rtnJoin = %s_rtnJoin;\n", pRegStg, pOutStg);
			}

			pIplTxStg->Append("\tCHtPriv %s_htPriv = %s_htPriv;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("CHtPriv"), VA("%s_htPriv", pRegStg));
			iplReg.Append("\t%s_htPriv = %s_htPriv;\n", pRegStg, pOutStg);

			if (pMod->m_threads.m_htIdW.AsInt() > 2 && pMod->m_bCallFork) {
				pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg));
				iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
			}

			pIplTxStg->NewLine();
			m_iplRegDecl.NewLine();
			iplReg.NewLine();

		} else if (pMod->m_gblDistRam) {
			IplComment(pIplTxStg, iplReg, "\t// Internal ram stage #1\n");

			iplStg += 1;
			sprintf(inStgStr, "r_t%d", iplStg);
			sprintf(outStgStr, "c_t%d", iplStg);
			sprintf(regStgStr, "r_t%d", iplStg + 1);

			pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg));
			iplReg.Append("\t%s_htValid = !%s && %s_htValid;\n", pRegStg, reset.c_str(), pOutStg);

			pIplTxStg->Append("\tCHtPriv %s_htPriv = %s_htPriv;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("CHtPriv"), VA("%s_htPriv", pRegStg));
			iplReg.Append("\t%s_htPriv = %s_htPriv;\n", pRegStg, pOutStg);

			if (pMod->m_threads.m_htIdW.AsInt() > 2 && pMod->m_bCallFork) {
				pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg));
				iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
			}

			if (pModInst->m_cxrSrcCnt > 1) {
				pIplTxStg->Append("\t%s %s_htSel = %s_htSel;\n", htSelType.c_str(), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelType, VA("%s_htSel", pRegStg));
				iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
			}

			if (bMultiThread) {
				pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg));
				iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
			}

			pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_htInstr = %s_htInstr;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htInstrTypeStr, VA("%s_htInstr", pRegStg));
			iplReg.Append("\t%s_htInstr = %s_htInstr;\n", pRegStg, pOutStg);

			if (pMod->m_bCallFork) {
				pIplTxStg->Append("\tbool %s_rtnJoin = %s_rtnJoin;\n", pOutStg, pInStg);
				m_iplRegDecl.Append("\tbool %s_rtnJoin;\n", pRegStg);
				iplReg.Append("\t%s_rtnJoin = %s_rtnJoin;\n", pRegStg, pOutStg);
			}

			pIplTxStg->NewLine();
			m_iplRegDecl.NewLine();
			iplReg.NewLine();

			IplComment(pIplTxStg, iplReg, "\t// Internal ram stage #2\n");

			iplStg += 1;
			sprintf(inStgStr, "r_t%d", iplStg);
			sprintf(outStgStr, "c_t%d", iplStg);
			sprintf(regStgStr, "r_t%d", iplStg + 1);

			pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg));
			iplReg.Append("\t%s_htValid = !%s && %s_htValid;\n", pRegStg, reset.c_str(), pOutStg);

			pIplTxStg->Append("\tCHtPriv %s_htPriv = %s_htPriv;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("CHtPriv"), VA("%s_htPriv", pRegStg));
			iplReg.Append("\t%s_htPriv = %s_htPriv;\n", pRegStg, pOutStg);
			pIplTxStg->Append("\n");

			if (pMod->m_threads.m_htIdW.AsInt() > 2 && pMod->m_bCallFork) {
				pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg));
				iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
			}

			if (pModInst->m_cxrSrcCnt > 1) {
				pIplTxStg->Append("\t%s %s_htSel = %s_htSel;\n", htSelType.c_str(), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelType, VA("%s_htSel", pRegStg));
				iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
			}

			if (bMultiThread) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg));

				if (pMod->m_bHtIdInit && iplStg == pMod->m_execStg - 1) {
					m_iplRegDecl.Append("\tbool %s_htIdInit;\n", pRegStg);
					pIplTxStg->Append("\tbool %s_htIdInit = %s_htIdInit && %s_htId != 0x%x;\n",
						pOutStg, pRegStg, pRegStg, (1 << pMod->m_threads.m_htIdW.AsInt()) - 1);

					pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htIdInit ? (sc_uint<%s_HTID_W>)(%s_htId+1) : %s_htId;\n",
						pMod->m_modName.Upper().c_str(), pOutStg, pRegStg, pMod->m_modName.Upper().c_str(), pRegStg, pInStg);
					iplReg.Append("\t%s_htId = %s ? (sc_uint<%s_HTID_W>)0 : %s_htId;\n",
						pRegStg, reset.c_str(), pMod->m_modName.Upper().c_str(), pOutStg);

					iplReg.Append("\t%s_htIdInit = %s || %s_htIdInit;\n", pRegStg, reset.c_str(), pOutStg);
				} else {
					pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);
					iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
				}
			}

			pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_htInstr = %s_htInstr;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htInstrTypeStr, VA("%s_htInstr", pRegStg));
			iplReg.Append("\t%s_htInstr = %s_htInstr;\n", pRegStg, pOutStg);

			if (pMod->m_bCallFork) {
				pIplTxStg->Append("\tbool %s_rtnJoin = %s_rtnJoin;\n", pOutStg, pInStg);
				m_iplRegDecl.Append("\tbool ht_noload %s_rtnJoin;\n", pRegStg);
				iplReg.Append("\t%s_rtnJoin = %s_rtnJoin;\n", pRegStg, pOutStg);
			}

			pIplTxStg->NewLine();
			m_iplRegDecl.NewLine();
			iplReg.NewLine();
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// State machine stage Ts

	{
		// Pipe stages
		for (int tsIdx = 0; tsIdx < pMod->m_stage.m_privWrStg.AsInt(); tsIdx += 1) {
			pIplTxStg->Append("\n");
			m_iplRegDecl.Append("\n");
			iplReg.Append("\n");

			pIplTxStg->Append("\t// Instruction Stage #%d\n", tsIdx + 1);
			m_iplRegDecl.Append("\t// Instruction Stage #%d\n", tsIdx + 1);
			iplReg.Append("\t// Instruction Stage #%d\n", tsIdx + 1);

			iplStg += 1;
			sprintf(inStgStr, "r_t%d", iplStg);
			sprintf(outStgStr, "c_t%d", iplStg);
			sprintf(regStgStr, "r_t%d", iplStg + 1);

			if (tsIdx < pMod->m_stage.m_privWrStg.AsInt() - 1 || pMod->m_bGvIwComp) {

				pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg));
				iplReg.Append("\t%s_htValid = !%s && %s_htValid;\n", pRegStg, reset.c_str(), pOutStg);
			}

			if (tsIdx < pMod->m_stage.m_privWrStg.AsInt() - 1) {
				pIplTxStg->Append("\t%s %s_htSel = %s_htSel;\n", htSelType.c_str(), pOutStg, pInStg);
				iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelType, VA("%s_htSel", pRegStg));
			}

			if (tsIdx < pMod->m_stage.m_privWrStg.AsInt() - 1) {
				pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_htInstr = %s_htInstr;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htInstrTypeStr, VA("%s_htInstr", pRegStg));
				iplReg.Append("\t%s_htInstr = %s_htInstr;\n", pRegStg, pOutStg);

				if (pMod->m_bCallFork) {
					pIplTxStg->Append("\tbool %s_rtnJoin = %s_rtnJoin;\n", pOutStg, pInStg);
					m_iplRegDecl.Append("\tbool %s_rtnJoin;\n", pRegStg);
					iplReg.Append("\t%s_rtnJoin = %s_rtnJoin;\n", pRegStg, pOutStg);
				}
			}

			if (bMultiThread) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg));

				if (pMod->m_bHtIdInit && iplStg == pMod->m_execStg - 1) {
					m_iplRegDecl.Append("\tbool %s_htIdInit;\n", pRegStg);
					pIplTxStg->Append("\tbool %s_htIdInit = %s_htIdInit && %s_htId != 0x%x;\n",
						pOutStg, pRegStg, pRegStg, (1 << pMod->m_threads.m_htIdW.AsInt()) - 1);

					pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htIdInit ? (sc_uint<%s_HTID_W>)(%s_htId+1) : %s_htId;\n",
						pMod->m_modName.Upper().c_str(), pOutStg, pRegStg, pMod->m_modName.Upper().c_str(), pRegStg, pInStg);
					iplReg.Append("\t%s_htId = %s ? (sc_uint<%s_HTID_W>)0 : %s_htId;\n",
						pRegStg, reset.c_str(), pMod->m_modName.Upper().c_str(), pOutStg);

					iplReg.Append("\t%s_htIdInit = %s || %s_htIdInit;\n", pRegStg, reset.c_str(), pOutStg);
				} else {
					pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);
					iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
				}
			}

			m_iplRegDecl.Append("\tCHtPriv %s_htPriv;\n", pOutStg);
			pIplTxStg->Append("\t%s_htPriv = %s_htPriv;\n", pOutStg, pInStg);

			if (bMultiThread || tsIdx < pMod->m_stage.m_privWrStg.AsInt() - 1) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("CHtPriv"), VA("%s_htPriv", pRegStg));
				iplReg.Append("\t%s_htPriv = %s_htPriv;\n", pRegStg, pOutStg);
			}

			if (pMod->m_threads.m_htIdW.AsInt() > 2 && pMod->m_bCallFork) {
				pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg));
				iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
			}

			IplNewLine(pIplTxStg);

			if (iplStg >= pMod->m_execStg) {

				if (iplStg == pMod->m_execStg) {
					m_iplRegDecl.Append("\tsc_uint<4> c_t%d_htCtrl;\n", iplStg);
					pIplTxStg->Append("\tc_t%d_htCtrl = HT_INVALID;\n", iplStg);
				} else {
					m_iplRegDecl.Append("\tsc_uint<4> c_t%d_htCtrl;\n", iplStg);
					pIplTxStg->Append("\tc_t%d_htCtrl = r_t%d_htCtrl;\n", iplStg, iplStg);
				}

				if (pMod->m_wrStg - 1 > iplStg) {
					m_iplRegDecl.Append("\tsc_uint<4> r_t%d_htCtrl;\n", iplStg + 1);
					iplReg.Append("\tr_t%d_htCtrl = c_t%d_htCtrl;\n", iplStg + 1, iplStg);
				}

				if (iplStg == pMod->m_execStg) {
					m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> c_t%d_htNextInstr;\n", pMod->m_modName.Upper().c_str(), iplStg);
					pIplTxStg->Append("\tc_t%d_htNextInstr = r_t%d_htInstr;\n", iplStg, iplStg);
				} else {
					m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> c_t%d_htNextInstr;\n", pMod->m_modName.Upper().c_str(), iplStg);
					pIplTxStg->Append("\tc_t%d_htNextInstr = r_t%d_htNextInstr;\n", iplStg, iplStg);
				}

				if (pMod->m_wrStg - 1 > iplStg) {
					m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> r_t%d_htNextInstr;\n", pMod->m_modName.Upper().c_str(), iplStg + 1);
					iplReg.Append("\tr_t%d_htNextInstr = c_t%d_htNextInstr;\n", iplStg + 1, iplStg);
				}
			}
		}

		for (int gvIwIdx = pMod->m_tsStg + pMod->m_stage.m_privWrStg.AsInt(); gvIwIdx < pMod->m_gvIwCompStg; gvIwIdx += 1) {
			pIplTxStg->Append("\n");
			m_iplRegDecl.Append("\n");
			iplReg.Append("\n");

			sprintf(inStgStr, "r_t%d", gvIwIdx);
			sprintf(outStgStr, "c_t%d", gvIwIdx);
			sprintf(regStgStr, "r_t%d", gvIwIdx + 1);

			if (gvIwIdx < pMod->m_stage.m_privWrStg.AsInt() - 1 || pMod->m_bGvIwComp) {

				pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg));
				iplReg.Append("\t%s_htValid = !%s && %s_htValid;\n", pRegStg, reset.c_str(), pOutStg);
			}

			if (bMultiThread) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg));

				pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", pMod->m_modName.Upper().c_str(), pOutStg, pInStg);
				iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
			}

			pIplTxStg->Append("\tbool %s_htCmdRdy = %s_htCmdRdy;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htCmdRdy", pRegStg));
			iplReg.Append("\t%s_htCmdRdy = %s_htCmdRdy;\n", pRegStg, pOutStg);

			IplNewLine(pIplTxStg);
		}

		pIplTxStg->Append("\n");

		/////////////////////////////////
		// declare primitives

		for (size_t primIdx = 0; primIdx < pMod->m_scPrimList.size(); primIdx += 1) {
			m_iplRegDecl.Append("\t%s;\n", pMod->m_scPrimList[primIdx].m_scPrimDecl.c_str());
			iplReg.Append("\t%s;\n", pMod->m_scPrimList[primIdx].m_scPrimFunc.c_str());
		}

		pIplTxStg->NewLine();
		m_iplRegDecl.NewLine();
		iplReg.NewLine();

		/////////////////////////////////
		// declare shared variables

		for (size_t fieldIdx = 0; fieldIdx < pMod->m_shared.m_fieldList.size(); fieldIdx += 1) {
			CField * pField = pMod->m_shared.m_fieldList[fieldIdx];

			string type = GenFieldType(pField, false);

			if (pField->m_addr1W.size() > 0) {

				static bool bError = false;
				if (pField->m_addr2W.size() > 0) {
					m_iplRegDecl.Append("\t%s m__SHR__%s%s;\n", type.c_str(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());

					if (pField->m_ramType == eDistRam && pField->m_addr1W.AsInt() + pField->m_addr2W.AsInt() > 10) {
						ParseMsg(Error, pField->m_lineInfo, "unsupported distributed ram depth (addr1W + addr2W > 10)");
						if (!bError) {
							bError = true;
							ParseMsg(Info, pField->m_lineInfo, "switch to block ram for greater depth (blockRam=true)");
							ParseMsg(Info, pField->m_lineInfo, "shared variable as block ram requires one extra stage for read access");
						}
					}
				} else {
					m_iplRegDecl.Append("\t%s m__SHR__%s%s;\n", type.c_str(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());

					if (pField->m_ramType == eDistRam && pField->m_addr1W.AsInt() > 10) {
						ParseMsg(Error, pField->m_lineInfo, "unsupported distributed ram depth (addr1W > 10)");
						if (!bError) {
							bError = true;
							ParseMsg(Info, pField->m_lineInfo, "switch to block ram for greater depth (blockRam=true)");
							ParseMsg(Info, pField->m_lineInfo, "shared variable as block ram requires one extra stage for read access");
						}
					}
				}

			} else if (pField->m_queueW.size() > 0) {

				m_iplRegDecl.Append("\t%s m__SHR__%s%s;\n", type.c_str(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());

			} else {

				m_iplRegDecl.Append("\t%s c__SHR__%s%s;\n", type.c_str(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());

				GenRamIndexLoops(*pIplTxStg, "", *pField);
				pIplTxStg->Append("\tc__SHR__%s%s = r__SHR__%s%s;\n", pField->m_name.c_str(), pField->m_dimenIndex.c_str(), pField->m_name.c_str(), pField->m_dimenIndex.c_str());

				m_iplRegDecl.Append("\t%s r__SHR__%s%s;\n", pField->m_pType->m_typeName.c_str(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());

				GenRamIndexLoops(iplReg, "", *pField);
				iplReg.Append("\tr__SHR__%s%s = c__SHR__%s%s;\n", pField->m_name.c_str(), pField->m_dimenIndex.c_str(), pField->m_name.c_str(), pField->m_dimenIndex.c_str());
			}
		}

		pIplTxStg->NewLine();
		m_iplRegDecl.NewLine();
		iplReg.NewLine();

		/////////////////////////////////
		// declare staged variables

		for (size_t fieldIdx = 0; fieldIdx < pMod->m_stage.m_fieldList.size(); fieldIdx += 1) {
			CField * pField = pMod->m_stage.m_fieldList[fieldIdx];

			// declare combinatorial variables
			int stgIdx = (pField->m_bInit ? 1 : 0) + pField->m_rngLow.AsInt();
			int lastStgIdx = pField->m_rngHigh.AsInt() + 1;
			for (; stgIdx <= lastStgIdx; stgIdx += 1) {
				int varStg = pMod->m_tsStg + stgIdx - 1;
				if (stgIdx != pField->m_rngLow.AsInt()) {
					const char *pNoLoad = stgIdx == pField->m_rngHigh.AsInt() + 1 ? " ht_noload" : "";

					if (pField->m_dimenList.size() > 0) {
						m_iplRegDecl.Append("\t%s%s c_t%d__STG__%s%s;\n",
							pField->m_pType->m_typeName.c_str(), pNoLoad,
							varStg, pField->m_name.c_str(), pField->m_dimenDecl.c_str());
					} else {
						m_iplRegDecl.Append("\t%s%s c_t%d__STG__%s;\n",
							pField->m_pType->m_typeName.c_str(), pNoLoad, varStg, pField->m_name.c_str());

						iplTsStg.Append("\tc_t%d__STG__%s = r_t%d__STG__%s;\n",
							varStg, pField->m_name.c_str(), varStg, pField->m_name.c_str());
					}
				} else {
					if (pField->m_dimenList.size() > 0) {
						m_iplRegDecl.Append("\t%s c_t%d__STG__%s%s;\n",
							pField->m_pType->m_typeName.c_str(),
							varStg, pField->m_name.c_str(), pField->m_dimenDecl.c_str());
					} else {
						m_iplRegDecl.Append("\t%s c_t%d__STG__%s;\n",
							pField->m_pType->m_typeName.c_str(), varStg, pField->m_name.c_str());
						if (pField->m_bZero)
							iplTsStg.Append("\tc_t%d__STG__%s = 0;\n",
							varStg, pField->m_name.c_str());
					}
				}
			}
			iplTsStg.Append("\n");

			// Initialize dimensioned combinatorial variables
			const char *pTabs = "";
			if (pField->m_dimenList.size() > 0) {
				GenRamIndexLoops(iplTsStg, "", *pField, true);

				stgIdx = (pField->m_bInit ? 1 : 0) + pField->m_rngLow.AsInt();
				for (; stgIdx <= pField->m_rngHigh.AsInt() + 1; stgIdx += 1) {
					int varStg = pMod->m_tsStg + stgIdx - 1;
					if (stgIdx != pField->m_rngLow.AsInt()) {
						iplTsStg.Append("%s\tc_t%d__STG__%s%s = r_t%d__STG__%s%s;\n", pTabs,
							varStg, pField->m_name.c_str(), pField->m_dimenIndex.c_str(),
							varStg, pField->m_name.c_str(), pField->m_dimenIndex.c_str());
					} else if (pField->m_bZero) {
						iplTsStg.Append("%s\tc_t%d__STG__%s%s = 0;\n", pTabs,
							varStg, pField->m_name.c_str(), pField->m_dimenIndex.c_str());
					}
				}

				pTabs = pField->m_dimenTabs.c_str();

				iplTsStg.Append("%s}\n", pTabs);
				iplTsStg.Append("\n");
			}

			char resetStr[64];
			if (pField->m_bReset)
				sprintf(resetStr, "%s ? (%s)0 : ", reset.c_str(), pField->m_pType->m_typeName.c_str());
			else
				resetStr[0] = '\0';

			pTabs = "";
			for (stgIdx = pField->m_rngLow.AsInt(); stgIdx <= pField->m_rngHigh.AsInt(); stgIdx += 1) {
				int varStg = pMod->m_tsStg + stgIdx - 1;

				m_iplRegDecl.Append("\t%s r_t%d__STG__%s%s;\n",
					pField->m_pType->m_typeName.c_str(), varStg + 1, pField->m_name.c_str(), pField->m_dimenDecl.c_str());

				if (stgIdx == pField->m_rngLow.AsInt())
					GenRamIndexLoops(iplReg, "", *pField, pField->m_dimenList.size() > 0);

				if (pField->m_bInit && stgIdx == pField->m_rngLow.AsInt())
					iplReg.Append("%s\tr_t%d__STG__%s%s = %sc_t%d_htPriv.m_%s%s;\n", pTabs,
					varStg + 1, pField->m_name.c_str(), pField->m_dimenIndex.c_str(),
					resetStr,
					pMod->m_tsStg, pField->m_name.c_str(), pField->m_dimenIndex.c_str());
				else
					iplReg.Append("%s\tr_t%d__STG__%s%s = %sc_t%d__STG__%s%s;\n", pTabs,
					varStg + 1, pField->m_name.c_str(), pField->m_dimenIndex.c_str(),
					resetStr,
					varStg, pField->m_name.c_str(), pField->m_dimenIndex.c_str());

				if (pField->m_dimenList.size() > 0) {
					pTabs = pField->m_dimenTabs.c_str();
					if (stgIdx == pField->m_rngHigh.AsInt())
						iplReg.Append("%s}\n", pTabs);
				}
			}

			iplReg.NewLine();
			m_iplRegDecl.NewLine();
			iplTsStg.NewLine();
		}

		pIplTxStg->NewLine();
		m_iplRegDecl.NewLine();
		iplReg.NewLine();

		pIplTxStg->Append("\n");

		if (pMod->m_mif.m_bMifRd) {
			m_iplRegDecl.Append("\tbool ht_noload c_t%d_bReadMemBusy;\n", pMod->m_execStg);
			m_iplRegDecl.Append("\tbool ht_noload c_t%d_bReadMemAvail;\n", pMod->m_execStg);
			GenModTrace(eVcdUser, vcdModName, "ReadMemBusy()", VA("c_t%d_bReadMemBusy", pMod->m_execStg));
			pIplTxStg->Append("\tc_t%d_bReadMemAvail = false;\n", pMod->m_execStg);
		}

		if (pMod->m_mif.m_bMifWr) {
			m_iplRegDecl.Append("\tbool ht_noload c_t%d_bWriteMemBusy;\n", pMod->m_execStg);
			m_iplRegDecl.Append("\tbool ht_noload c_t%d_bWriteMemAvail;\n", pMod->m_execStg);
			GenModTrace(eVcdUser, vcdModName, "WriteMemBusy()", VA("c_t%d_bWriteMemBusy", pMod->m_execStg));
			pIplTxStg->Append("\tc_t%d_bWriteMemAvail = false;\n", pMod->m_execStg);
		}

		if (pMod->m_ohm.m_bOutHostMsg) {
			m_iplRegDecl.Append("\tbool ht_noload c_bSendHostMsgBusy;\n");
			m_iplRegDecl.Append("\tbool ht_noload c_bSendHostMsgAvail;\n");
			GenModTrace(eVcdUser, vcdModName, "SendHostMsgBusy()", "c_bSendHostMsgBusy");
			pIplTxStg->Append("\tc_bSendHostMsgAvail = false;\n");
		}
	}

	iplStg += 1;
	sprintf(inStgStr, "r_t%d", pMod->m_wrStg - 1);
	sprintf(outStgStr, "c_t%d", pMod->m_wrStg - 1);
	sprintf(regStgStr, "r_t%d", pMod->m_wrStg);

	////////////////////////////////////////////////////////////////////////////
	// Variable macro/references

	bool bFirstModVar = true;

	if (!pMod->m_bHtId) {
		m_iplDefines.Append("#if !defined(TOPOLOGY_HEADER)\n");
		for (int privWrIdx = 1; privWrIdx <= pMod->m_stage.m_privWrStg.AsInt(); privWrIdx += 1) {
			char privIdxStr[8];
			if (pMod->m_stage.m_bStageNums)
				sprintf(privIdxStr, "%d", privWrIdx);
			else
				privIdxStr[0] = '\0';

			m_iplDefines.Append("#define PR%s_htId 0\n", privIdxStr);
		}
		m_iplDefines.Append("#if defined(_HTV)\n");
	} else
		m_iplDefines.Append("#if !defined(TOPOLOGY_HEADER) && defined(_HTV)\n");

	for (int privWrIdx = 1; privWrIdx <= pMod->m_stage.m_privWrStg.AsInt(); privWrIdx += 1) {

		int privStg = pMod->m_tsStg + privWrIdx - 1;

		char privIdxStr[8];
		if (pMod->m_stage.m_bStageNums) {
			g_appArgs.GetDsnRpt().AddLevel("Private Variables - Stage %d\n", privWrIdx);
			sprintf(privIdxStr, "%d", privWrIdx);
		} else {
			g_appArgs.GetDsnRpt().AddLevel("Private Variables\n");
			privIdxStr[0] = '\0';
		}

		if (g_appArgs.IsVariableReportEnabled()) {
			fprintf(g_appArgs.GetVarRptFp(), "%s PR%s_htId r_t%d_htId\n",
				pMod->m_modName.Uc().c_str(),
				privIdxStr,
				privStg);
		}

		g_appArgs.GetDsnRpt().AddLevel("Read Only (PR_)\n");

		// PR_htId macro
		if (pMod->m_bHtId) {
			g_appArgs.GetDsnRpt().AddItem("sc_uint<%s> PR%s_htId\n", pMod->m_threads.m_htIdW.c_str(), privIdxStr);
			GenModVar(eVcdUser, vcdModName, bFirstModVar,
				VA("sc_uint<%s_HTID_W> const", pMod->m_modName.Upper().c_str()),
				"", // dimen
				VA("PR%s_htId", privIdxStr), VA("r_t%d_htId", privStg));
		} else {
			g_appArgs.GetDsnRpt().AddItem("int PR%s_htId\n", privIdxStr);
		}

		g_appArgs.GetDsnRpt().AddItem("sc_uint<%s_INSTR_W> PR%s_htInstr\n", pMod->m_modName.Upper().c_str(), privIdxStr);
		GenModVar(eVcdUser, vcdModName, bFirstModVar,
			VA("sc_uint<%s_INSTR_W> const", pMod->m_modName.Upper().c_str()),
			"", // dimen
			VA("PR%s_htInst", privIdxStr), VA("r_t%d_htInstr", privStg));
		GenModVar(eVcdUser, vcdModName, bFirstModVar,
			VA("sc_uint<%s_INSTR_W> const", pMod->m_modName.Upper().c_str()),
			"", // dimen
			VA("PR%s_htInstr", privIdxStr), VA("r_t%d_htInstr", privStg));
		m_iplDefines.Append("\n");

		// Private variable access macros
		g_appArgs.GetDsnRpt().AddItem("bool  PR%s_htValid\n", privIdxStr);
		GenModVar(eVcdUser, vcdModName, bFirstModVar,
			"bool const", "", VA("PR%s_htValid", privIdxStr), VA("r_t%d_htValid", privStg));

		if (pMod->m_threads.m_htPriv.m_fieldList.size() == 0) continue;

		// PR variables
		for (size_t privIdx = 0; privIdx < pMod->m_threads.m_htPriv.m_fieldList.size(); privIdx += 1) {
			CField * pPriv = pMod->m_threads.m_htPriv.m_fieldList[privIdx];

			g_appArgs.GetDsnRpt().AddItem("%s PR%s_%s%s\n", pPriv->m_pType->m_typeName.c_str(), privIdxStr, pPriv->m_name.c_str(), pPriv->m_dimenDecl.c_str());

			GenModVar(eVcdUser, vcdModName, bFirstModVar,
				VA("%s const", pPriv->m_pType->m_typeName.c_str()),
				pPriv->m_dimenDecl,
				VA("PR%s_%s", privIdxStr, pPriv->m_name.c_str()),
				VA("r_t%d_htPriv.m_%s", privStg, pPriv->m_name.c_str()),
				pPriv->m_dimenList);
		}

		for (size_t ngvIdx = 0; ngvIdx < pMod->m_ngvList.size(); ngvIdx += 1) {
			CRam * pNgv = pMod->m_ngvList[ngvIdx];

			if (!pNgv->m_bPrivGbl) continue;

			g_appArgs.GetDsnRpt().AddItem("%s PR%s_%s%s\n", pNgv->m_type.c_str(), privIdxStr, pNgv->m_gblName.c_str(), pNgv->m_dimenDecl.c_str());

			if (pNgv->m_addrW == 0) {
				GenModVar(eVcdUser, vcdModName, bFirstModVar,
					VA("%s const", /*(privWrIdx == 1 || pNgv->m_addrW > pMod->m_threads.m_htIdW.AsInt())
					? */pNgv->m_type.c_str()/* : VA("CGW_%s", pNgv->m_pNgvInfo->m_ngvWrType.c_str()).c_str()*/),
					pNgv->m_dimenDecl,
					VA("PR%s_%s", privIdxStr, pNgv->m_privName.c_str()),
					VA("r__GBL__%s", pNgv->m_gblName.c_str()),
					pNgv->m_dimenList);
			} else {
				GenModVar(eVcdUser, vcdModName, bFirstModVar,
					VA("%s const", (privWrIdx == 1 || pNgv->m_addrW > pMod->m_threads.m_htIdW.AsInt())
					? pNgv->m_type.c_str() : VA("CGW_%s", pNgv->m_pNgvInfo->m_ngvWrType.c_str()).c_str()),
					pNgv->m_dimenDecl,
					VA("PR%s_%s", privIdxStr, pNgv->m_privName.c_str()),
					VA("r_t%d_%sI%cData", privStg, pNgv->m_gblName.c_str(), (privWrIdx == 1 || pNgv->m_addrW > pMod->m_threads.m_htIdW.AsInt()) ? 'r' : 'w'),
					pNgv->m_dimenList);
			}
		}

		g_appArgs.GetDsnRpt().EndLevel();
		g_appArgs.GetDsnRpt().AddLevel("Read/Write (P_)\n");

		// P variables
		for (size_t privIdx = 0; privIdx < pMod->m_threads.m_htPriv.m_fieldList.size(); privIdx += 1) {
			CField * pPriv = pMod->m_threads.m_htPriv.m_fieldList[privIdx];

			g_appArgs.GetDsnRpt().AddItem("%s P%s_%s%s\n", pPriv->m_pType->m_typeName.c_str(), privIdxStr, pPriv->m_name.c_str(), pPriv->m_dimenDecl.c_str());

			GenModVar(eVcdNone, vcdModName, bFirstModVar,
				VA("%s", pPriv->m_pType->m_typeName.c_str()),
				pPriv->m_dimenDecl,
				VA("P%s_%s", privIdxStr, pPriv->m_name.c_str()),
				VA("c_t%d_htPriv.m_%s", privStg, pPriv->m_name.c_str()),
				pPriv->m_dimenList);
		}

		bool bWriteOnly = false;
		for (size_t ngvIdx = 0; ngvIdx < pMod->m_ngvList.size(); ngvIdx += 1) {
			CRam * pNgv = pMod->m_ngvList[ngvIdx];

			if (!pNgv->m_bPrivGbl) continue;

			bool bVarWriteOnly = pNgv->m_addrW > pMod->m_threads.m_htIdW.AsInt() || pNgv->m_pType->IsEmbeddedUnion();
			bWriteOnly |= bVarWriteOnly;
			if (bVarWriteOnly) continue;

			GenModVar(eVcdNone, vcdModName, bFirstModVar,
				VA("CGW_%s", pNgv->m_pNgvInfo->m_ngvWrType.c_str()),
				pNgv->m_dimenDecl,
				VA("P%s_%s", privIdxStr, pNgv->m_privName.c_str()),
				VA("c_t%d_%sIwData", privStg, pNgv->m_gblName.c_str()),
				pNgv->m_dimenList);
		}

		g_appArgs.GetDsnRpt().EndLevel();

		if (bWriteOnly) {
			g_appArgs.GetDsnRpt().AddLevel("Write Only (PW_)\n");

			for (size_t ngvIdx = 0; ngvIdx < pMod->m_ngvList.size(); ngvIdx += 1) {
				CRam * pNgv = pMod->m_ngvList[ngvIdx];

				if (!pNgv->m_bPrivGbl) continue;

				bWriteOnly = pNgv->m_addrW > pMod->m_threads.m_htIdW.AsInt() || pNgv->m_pType->IsEmbeddedUnion();
				if (!bWriteOnly) continue;

				g_appArgs.GetDsnRpt().AddItem("%s PW%s_%s%s\n", pNgv->m_type.c_str(), privIdxStr, pNgv->m_gblName.c_str(), pNgv->m_dimenDecl.c_str());

				GenModVar(eVcdNone, vcdModName, bFirstModVar,
					VA("CGW_%s", pNgv->m_pNgvInfo->m_ngvWrType.c_str()),
					pNgv->m_dimenDecl,
					VA("PW%s_%s", privIdxStr, pNgv->m_privName.c_str()),
					VA("c_t%d_%sIwData", privStg, pNgv->m_gblName.c_str()),
					pNgv->m_dimenList);
			}

			g_appArgs.GetDsnRpt().EndLevel();
		}

		if (g_appArgs.IsVariableReportEnabled()) {
			int privPos = 0;
			for (size_t privIdx = 0; privIdx < pMod->m_threads.m_htPriv.m_fieldList.size(); privIdx += 1) {
				CField * pPriv = pMod->m_threads.m_htPriv.m_fieldList[privIdx];

				int privWidth = pPriv->m_pType->GetPackedBitWidth();

				vector<int> refList(pPriv->m_dimenList.size());

				do {
					string dimIdx = IndexStr(refList);

					fprintf(g_appArgs.GetVarRptFp(), "%s PR%s_%s%s r_t%d_htPriv[%d:%d]\n",
						pMod->m_modName.Uc().c_str(),
						privIdxStr, pPriv->m_name.c_str(), dimIdx.c_str(),
						privStg,
						privPos + privWidth - 1, privPos);

					privPos += privWidth;

				} while (DimenIter(pPriv->m_dimenList, refList));

				//m_iplDefines.Append("\n");
			}
		}

		g_appArgs.GetDsnRpt().EndLevel();
	}

	// staged variables
	if (pMod->m_stage.m_fieldList.size() > 0) {

		g_appArgs.GetDsnRpt().AddLevel("Stage Variables\n");
		g_appArgs.GetDsnRpt().AddLevel("Read Only (TR_)\n");

		for (size_t fieldIdx = 0; fieldIdx < pMod->m_stage.m_fieldList.size(); fieldIdx += 1) {
			CField * pField = pMod->m_stage.m_fieldList[fieldIdx];

			int stgIdx = (pField->m_bInit ? 1 : 0) + pField->m_rngLow.AsInt();
			int highIdx = pField->m_rngHigh.AsInt() + 1;

			for (; stgIdx < highIdx; stgIdx += 1) {
				g_appArgs.GetDsnRpt().AddItem("%s TR%d_%s%s\n", pField->m_pType->m_typeName.c_str(),
					stgIdx + 1, pField->m_name.c_str(), pField->m_dimenDecl.c_str());
			}
		}

		g_appArgs.GetDsnRpt().EndLevel();
		g_appArgs.GetDsnRpt().AddLevel("Read/Write (T_)\n");

		for (size_t fieldIdx = 0; fieldIdx < pMod->m_stage.m_fieldList.size(); fieldIdx += 1) {
			CField * pField = pMod->m_stage.m_fieldList[fieldIdx];

			int stgIdx = (pField->m_bInit ? 1 : 0) + pField->m_rngLow.AsInt();
			int highIdx = pField->m_rngHigh.AsInt() + 1;

			for (; stgIdx < highIdx + 1; stgIdx += 1) {
				g_appArgs.GetDsnRpt().AddItem("%s T%d_%s%s\n", pField->m_pType->m_typeName.c_str(),
					stgIdx, pField->m_name.c_str(), pField->m_dimenDecl.c_str());
			}
		}

		g_appArgs.GetDsnRpt().EndLevel();
		g_appArgs.GetDsnRpt().EndLevel();

		for (size_t fieldIdx = 0; fieldIdx < pMod->m_stage.m_fieldList.size(); fieldIdx += 1) {
			CField * pField = pMod->m_stage.m_fieldList[fieldIdx];

			int stgIdx = (pField->m_bInit ? 1 : 0) + pField->m_rngLow.AsInt();
			int highIdx = pField->m_rngHigh.AsInt() + 1;
			int varStg;
			for (; stgIdx < highIdx; stgIdx += 1) {
				varStg = pMod->m_tsStg + stgIdx - 1;

				GenModVar(eVcdNone, vcdModName, bFirstModVar,
					VA("%s", pField->m_pType->m_typeName.c_str()),
					pField->m_dimenDecl,
					VA("T%d_%s", stgIdx, pField->m_name.c_str()),
					VA("c_t%d__STG__%s", varStg, pField->m_name.c_str()),
					pField->m_dimenList);
				GenModVar(eVcdUser, vcdModName, bFirstModVar,
					VA("%s const", pField->m_pType->m_typeName.c_str()),
					pField->m_dimenDecl,
					VA("TR%d_%s", stgIdx + 1, pField->m_name.c_str()),
					VA("r_t%d__STG__%s", varStg + 1, pField->m_name.c_str()),
					pField->m_dimenList);
			}

			varStg = pMod->m_tsStg + stgIdx - 1;
			GenModVar(eVcdUser, vcdModName, bFirstModVar,
				VA("%s", pField->m_pType->m_typeName.c_str()),
				pField->m_dimenDecl,
				VA("T%d_%s", stgIdx, pField->m_name.c_str()),
				VA("c_t%d__STG__%s", varStg, pField->m_name.c_str()),
				pField->m_dimenList);
		}

		m_iplDefines.Append("\n");
	}

	{
		// Shared variable access macros
		if (pMod->m_shared.m_fieldList.size() > 0) {

			g_appArgs.GetDsnRpt().AddLevel("Shared Variables\n");

			bool bFirst = true;
			for (size_t shIdx = 0; shIdx < pMod->m_shared.m_fieldList.size(); shIdx += 1) {
				CField * pShared = pMod->m_shared.m_fieldList[shIdx];

				if (pShared->m_queueW.AsInt() == 0/* && pShared->m_addr1W.AsInt() == 0*/) {
					if (bFirst) {
						g_appArgs.GetDsnRpt().AddLevel("Read Only (SR_)\n");
						bFirst = false;
					}
					g_appArgs.GetDsnRpt().AddItem("%s ", pShared->m_pType->m_typeName.c_str());

					g_appArgs.GetDsnRpt().AddText("SR_%s%s\n",
						pShared->m_name.c_str(), pShared->m_dimenDecl.c_str());
				}
			}

			if (!bFirst)
				g_appArgs.GetDsnRpt().EndLevel();

			g_appArgs.GetDsnRpt().AddLevel("Read/Write (S_)\n");

			for (size_t shIdx = 0; shIdx < pMod->m_shared.m_fieldList.size(); shIdx += 1) {
				CField * pShared = pMod->m_shared.m_fieldList[shIdx];

				if (pShared->m_bIhmReadOnly && pShared->m_queueW.size() == 0)
					continue;

				if (pShared->m_rdSelW.AsInt() > 0) {

					g_appArgs.GetDsnRpt().AddItem("ht_mrd_block_ram<type=%s, AW1=%d",
						pShared->m_pType->m_typeName.c_str(), pShared->m_addr1W.AsInt());
					if (pShared->m_addr2W.size() > 0)
						g_appArgs.GetDsnRpt().AddText(", AW2=%d", pShared->m_addr2W.AsInt());
					g_appArgs.GetDsnRpt().AddText("> ");

				} else if (pShared->m_wrSelW.AsInt() > 0) {

					g_appArgs.GetDsnRpt().AddItem("ht_mwr_block_ram<type=%s, AW1=%d",
						pShared->m_pType->m_typeName.c_str(), pShared->m_addr1W.AsInt());
					if (pShared->m_addr2W.size() > 0)
						g_appArgs.GetDsnRpt().AddText(", AW2=%d", pShared->m_addr2W.AsInt());
					g_appArgs.GetDsnRpt().AddText("> ");

				} else if (pShared->m_addr1W.size() > 0) {

					const char *pScMem = pShared->m_ramType == eDistRam ? "ht_dist_ram" : "ht_block_ram";
					if (pShared->m_addr2W.size() > 0)
						g_appArgs.GetDsnRpt().AddItem("%s<%s, %s, %s> ", pScMem, pShared->m_pType->m_typeName.c_str(),
						pShared->m_addr1W.c_str(), pShared->m_addr2W.c_str());
					else
						g_appArgs.GetDsnRpt().AddItem("%s<%s, %s> ", pScMem, pShared->m_pType->m_typeName.c_str(),
						pShared->m_addr1W.c_str());

				} else if (pShared->m_queueW.size() > 0) {

					const char *pScMem = pShared->m_ramType == eDistRam ? "ht_dist_que" : "ht_block_que";
					g_appArgs.GetDsnRpt().AddItem("%s<%s, %s> ", pScMem, pShared->m_pType->m_typeName.c_str(),
						pShared->m_queueW.c_str());

				} else {

					g_appArgs.GetDsnRpt().AddItem("%s ", pShared->m_pType->m_typeName.c_str());
				}

				g_appArgs.GetDsnRpt().AddText("S_%s%s\n",
					pShared->m_name.c_str(), pShared->m_dimenDecl.c_str());
			}
			g_appArgs.GetDsnRpt().EndLevel();
			g_appArgs.GetDsnRpt().EndLevel();

			for (size_t shIdx = 0; shIdx < pMod->m_shared.m_fieldList.size(); shIdx += 1) {
				CField * pShared = pMod->m_shared.m_fieldList[shIdx];

				char preCh = 'c';
				char regCh = 'r';
				if (pShared->m_queueW.AsInt() > 0 || pShared->m_addr1W.AsInt() > 0) {
					preCh = 'm';
					regCh = 'm';
				}

				if (!pShared->m_bIhmReadOnly || pShared->m_queueW.size() > 0) {
					GenModVar(eVcdNone, vcdModName, bFirstModVar,
						GenFieldType(pShared, false),
						pShared->m_dimenDecl,
						VA("S_%s", pShared->m_name.c_str()),
						VA("%c__SHR__%s", preCh, pShared->m_name.c_str()),
						pShared->m_dimenList);
				}

				if (/*preCh == 'c'*/pShared->m_queueW.size() == 0) {
					GenModVar(eVcdUser, vcdModName, bFirstModVar,
						GenFieldType(pShared, true),
						pShared->m_dimenDecl,
						VA("SR_%s", pShared->m_name.c_str()),
						VA("%c__SHR__%s", regCh, pShared->m_name.c_str()),
						pShared->m_dimenList);
				}
			}

		}

		m_iplRegDecl.Append("\tCHtAssertIntf c_htAssert;\n");
		iplPostInstr.Append("\tc_htAssert = 0;\n");
		if (pMod->m_clkRate == eClk2x) {
			iplPostInstr.Append("#\tifdef HT_ASSERT\n");
			iplPostInstr.Append("\tif (r_%sToHta_assert.read().m_bAssert && r_phase && !%s)\n", pModInst->m_instName.Lc().c_str(), reset.c_str());
			iplPostInstr.Append("\t\tc_htAssert = r_%sToHta_assert;\n", pModInst->m_instName.Lc().c_str());
			iplPostInstr.Append("#\tendif\n");
		}
		iplPostInstr.Append("\n");

		////////////////////////////////////////////////////////////////////////////
		// Instruction execution

		iplPostInstr.Append("\tPers%s%s();\n",
			unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
		iplPostInstr.Append("\n");
		iplPostInstr.Append("\tassert_msg(!r_t%d_htValid || c_t%d_htCtrl != HT_INVALID, \"Runtime check failed in CPers%s::Pers%s_%s()"
			" - expected an Ht control routine to have been called\");\n",
			pMod->m_execStg, pMod->m_execStg,
			pModInst->m_instName.Uc().c_str(), pMod->m_modName.Uc().c_str(), pMod->m_clkRate == eClk1x ? "1x" : "2x");
		iplPostInstr.Append("\n");

		////////////////////////////////////////////////////////////////////////////
		// Post instruction stage

		if (m_bPerfMon) {
			iplPostInstr.Append("#\tifndef _HTV\n");
			iplPostInstr.Append("\tif (");
			iplPostInstr.Append("r_t%d_htValid", pMod->m_execStg);
			iplPostInstr.Append(")\n", pMod->m_execStg);
			if (pMod->m_bHasThreads)
				iplPostInstr.Append("\t\tHt::g_syscMon.UpdateValidInstrCnt(m_htMonModId, (int)r_t%d_htInstr);\n", pMod->m_execStg);
			else
				iplPostInstr.Append("\t\tHt::g_syscMon.UpdateValidInstrCnt(m_htMonModId, (int)r_t%d_htInstr);\n", pMod->m_execStg);
			iplPostInstr.Append("\n");

			iplPostInstr.Append("\tif (c_t%d_htCtrl == HT_RETRY)\n", pMod->m_execStg);
			if (pMod->m_bHasThreads)
				iplPostInstr.Append("\t\tHt::g_syscMon.UpdateRetryInstrCnt(m_htMonModId, (int)r_t%d_htInstr);\n", pMod->m_execStg);
			else
				iplPostInstr.Append("\t\tHt::g_syscMon.UpdateRetryInstrCnt(m_htMonModId, (int)r_t%d_htInstr);\n", pMod->m_execStg);
			iplPostInstr.Append("\n");

			if (pMod->m_mif.m_bMif) {
				CMif & mif = pMod->m_mif;

				if (mif.m_bMifRd) {
					int reqStg = pMod->m_execStg + pMod->m_mif.m_mifReqStgCnt + 1;
					string stg = VA("r_t%d", reqStg);
					if (mif.m_bMifWr) {
						iplPostInstr.Append("\tif (%s_%sToMif_reqRdy && %s_%sToMif_req.m_type == MEM_REQ_RD) {\n",
							stg.c_str(), pMod->m_modName.Lc().c_str(), stg.c_str(), pMod->m_modName.Lc().c_str());
					} else {
						iplPostInstr.Append("\tif (%s_%sToMif_reqRdy) {\n",
							stg.c_str(), pMod->m_modName.Lc().c_str());
					}

					if (mif.m_mifRd.m_bMultiQwRdReq) {
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemReads(m_htMonModId, 0, 1);\n");
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemRead64s(m_htMonModId, 0, (r_t%d_%sToMif_req.m_tid >> 29) != 0 ? 1 : 0);\n",
							reqStg, pMod->m_modName.Lc().c_str());
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemReadBytes(m_htMonModId, 0, (1LL << r_t%d_%sToMif_req.m_size) * ((r_t%d_%sToMif_req.m_tid >> 29) + 1));\n",
							reqStg, pMod->m_modName.Lc().c_str(),
							reqStg, pMod->m_modName.Lc().c_str());
					} else {
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemReads(m_htMonModId, 0, 1);\n");
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemReadBytes(m_htMonModId, 0, 8);\n");
					}
					iplPostInstr.Append("\t}\n");
					iplPostInstr.Append("\n");
				}

				if (mif.m_bMifWr) {
					if (mif.m_bMifRd) {
						iplPostInstr.Append("\tif (r_t%d_%sToMif_reqRdy && r_t%d_%sToMif_req.m_type == MEM_REQ_WR) {\n",
							pMod->m_execStg + 1 + pMod->m_mif.m_mifReqStgCnt, pMod->m_modName.Lc().c_str(),
							pMod->m_execStg + 1 + pMod->m_mif.m_mifReqStgCnt, pMod->m_modName.Lc().c_str());
					} else {
						iplPostInstr.Append("\tif (r_t%d_%sToMif_reqRdy) {\n",
							pMod->m_execStg + 1 + pMod->m_mif.m_mifReqStgCnt, pMod->m_modName.Lc().c_str());
					}

					if (mif.m_mifWr.m_bMultiQwWrReq) {
						iplPostInstr.Append("\t\tbool bWrite64B = (r_t%d_%sToMif_req.m_tid >> 29) == 7 && (r_t%d_%sToMif_req.m_addr & 0x38) == 0;\n",
							pMod->m_execStg + 1 + pMod->m_mif.m_mifReqStgCnt, pMod->m_modName.Lc().c_str(),
							pMod->m_execStg + 1 + pMod->m_mif.m_mifReqStgCnt, pMod->m_modName.Lc().c_str());
						iplPostInstr.Append("\t\tbool bWrite8B = (r_t%d_%sToMif_req.m_tid >> 29) == 0;\n",
							pMod->m_execStg + 1 + pMod->m_mif.m_mifReqStgCnt, pMod->m_modName.Lc().c_str());
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemWrites(m_htMonModId, 0, bWrite8B ? 1 : 0);\n");
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemWrite64s(m_htMonModId, 0, bWrite64B ? 1 : 0);\n");
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemWriteBytes(m_htMonModId, 0, 1 << r_t%d_%sToMif_req.m_size);\n",
							pMod->m_execStg + 1 + pMod->m_mif.m_mifReqStgCnt, pMod->m_modName.Lc().c_str());
					} else {
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemWrites(m_htMonModId, 0, 1);\n");
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemWriteBytes(m_htMonModId, 0, 1 << r_t%d_%sToMif_req.m_size);\n",
							pMod->m_execStg + 1 + pMod->m_mif.m_mifReqStgCnt, pMod->m_modName.Lc().c_str());
					}
					iplPostInstr.Append("\t}\n");
					iplPostInstr.Append("\n");

				}
			}

			if (pMod->m_threads.m_htIdW.AsInt() == 0) {
				iplPostInstr.Append("\tHt::g_syscMon.UpdateActiveClks(m_htMonModId, r_htBusy ? 1 : 0);\n");

			} else {
				iplPostInstr.Append("\tif (!m_htIdPool.full() && r_resetComplete) {\n");
				iplPostInstr.Append("\t\tuint32_t activeCnt = (1ul << %s_HTID_W) - m_htIdPool.size();\n",
					pMod->m_modName.Upper().c_str());
				iplPostInstr.Append("\t\tHt::g_syscMon.UpdateActiveCnt(m_htMonModId, activeCnt);\n");
				iplPostInstr.Append("\t}\n");
				iplPostInstr.Append("\n");

				iplPostInstr.Append("\tuint32_t runningCnt = m_htCmdQue.size() + (r_t%d_htValid ? 1 : 0);\n",
					pMod->m_execStg);
				iplPostInstr.Append("\tHt::g_syscMon.UpdateRunningCnt(m_htMonModId, runningCnt);\n");
			}

			iplPostInstr.Append("#\tendif\n");
			iplPostInstr.Append("\n");
		}

		if (g_appArgs.IsInstrTraceEnabled()) {
			iplPostInstr.Append("#\tifndef _HTV\n");
			iplPostInstr.Append("\tif (Ht::g_instrTraceFp && (");
			iplPostInstr.Append("r_t%d_htValid", pMod->m_execStg);
			iplPostInstr.Append("))\n");
			iplPostInstr.Append("\t{\n");

			string line = "\t\tfprintf(Ht::g_instrTraceFp, \"" + pModInst->m_instName.Upper();
			if (pMod->m_instSet.GetReplCnt(pModInst->m_instId) > 1)
				line += "%%d:";
			else
				line += ":";
			if (pMod->m_bMultiThread)
				line += " HtId=0x%%x,";
			line += " Instr=%%-%ds, Ctrl=%%-9s -> Instr=%%-%ds";
			for (size_t i = 0; i < pMod->m_traceList.size(); i += 1)
				line += ", " + pMod->m_traceList[i] + "=0x%%llx";
			line += " @ %%lld\\n\",\n";
			iplPostInstr.Append(line.c_str(), maxInstrLen, maxInstrLen);

			line = "\t\t\t";
			if (pMod->m_instSet.GetReplCnt(pModInst->m_instId) > 1) {
				line += "(int)i_replId, ";
			}
			if (pMod->m_bMultiThread) {
				line += VA("(int)r_t%d_htId, ", pMod->m_execStg);
			}
			line += VA("m_pInstrNames[(int)r_t%d_htInstr], m_pHtCtrlNames[(int)c_t%d_htCtrl], m_pInstrNames[(int)c_t%d_htNextInstr], ",
				pMod->m_execStg, pMod->m_execStg, pMod->m_execStg);

			for (size_t i = 0; i < pMod->m_traceList.size(); i += 1) {
				line += VA("(long long)c_t%d_htPriv.", pMod->m_execStg);
			}
			line += "(long long)sc_time_stamp().value() / 10000);\n";
			iplPostInstr.Append(line.c_str());

			iplPostInstr.Append("\t\tfflush(Ht::g_instrTraceFp);\n");
			iplPostInstr.Append("\t}\n");
			iplPostInstr.Append("#\tendif\n");
			iplPostInstr.Append("\n");
		}

		if (pMod->m_ohm.m_bOutHostMsg) {
			iplPostInstr.Append("\tassert_msg(!c_%sToHti_ohm.m_bValid || c_bSendHostMsgAvail, \"Runtime check failed in CPers%s::Pers%s_%s()"
				" - expected SendHostMsgBusy() to have been called and not busy\");\n", pModInst->m_instName.Lc().c_str(), pModInst->m_instName.Uc().c_str(),
				pMod->m_modName.Uc().c_str(), pMod->m_clkRate == eClk1x ? "1x" : "2x");
		}

		iplPostInstr.Append("\n");

		char regStr[8];
		pRegStg = regStr;
		sprintf(regStr, "r_t%d", pMod->m_wrStg);

		// htCtrl
		if (pMod->m_bMultiThread) {
			iplPostInstr.Append("\tbool c_t%d_htPrivWe = false;\n", pMod->m_wrStg - 1);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("r_t%d_htPrivWe", pMod->m_wrStg));
			iplReg.Append("\tr_t%d_htPrivWe = c_t%d_htPrivWe;\n", pMod->m_wrStg, pMod->m_wrStg - 1);

			iplPostInstr.Append("\tbool c_t%d_htCmdRdy = false;\n", pMod->m_wrStg - 1);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("r_t%d_htCmdRdy", pMod->m_wrStg));
			iplReg.Append("\tr_t%d_htCmdRdy = c_t%d_htCmdRdy;\n", pMod->m_wrStg, pMod->m_wrStg - 1);

			iplPostInstr.Append("\tCHtCmd c_t%d_htCmd;\n", pMod->m_wrStg - 1);
		} else {
			iplPostInstr.Append("\tCHtPriv c_htPriv = r_htPriv;\n");
			iplPostInstr.Append("\tCHtCmd c_htCmd = r_htCmd;\n");

			if (pMod->m_bGvIwComp) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("r_t%d_htCmdRdy", pMod->m_wrStg));
				iplPostInstr.Append("\tbool c_t%d_htCmdRdy = false;\n", pMod->m_wrStg - 1);
			}
		}

		if (pMod->m_threads.m_htIdW.AsInt() == 0)
			iplReg.Append("\tr_htCmd = c_htCmd;\n");
		else {
			//GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "CHtCmd", VA("r_t%d_htCmd", pMod->m_wrStg) );
			m_iplRegDecl.Append("\tCHtCmd r_t%d_htCmd;\n", pMod->m_wrStg);
			iplReg.Append("\tr_t%d_htCmd = c_t%d_htCmd;\n", pMod->m_wrStg, pMod->m_wrStg - 1);
		}

		if (pMod->m_threads.m_resetInstr.size() > 0) {
			m_iplRegDecl.Append("\tbool r_t%d_htTerm;\n", pMod->m_wrStg);
			iplReg.Append("\tr_t%d_htTerm = c_t%d_htTerm;\n", pMod->m_wrStg, pMod->m_wrStg - 1);
			iplPostInstr.Append("\tbool c_t%d_htTerm = false;\n", pMod->m_wrStg - 1);
		}

		iplPostInstr.Append("\n");

		iplPostInstr.Append("\tswitch (r_t%d_htSel) {\n", pMod->m_wrStg - 1);

		if (pMod->m_threads.m_resetInstr.size() > 0) {
			iplPostInstr.Append("\tcase %s_HT_SEL_RESET:\n",
				pModInst->m_instName.Upper().c_str());
		}

		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrOut) continue;
			if (pCxrIntf->m_pDstGroup != &pMod->m_threads) continue;

			iplPostInstr.Append("\tcase %s_HT_SEL_%s_%d_%s:\n",
				pModInst->m_instName.Upper().c_str(), pCxrIntf->m_pSrcModInst->m_instName.Upper().c_str(), pCxrIntf->GetPortReplId(), pCxrIntf->m_modEntry.Upper().c_str());
		}

		iplPostInstr.Append("\tcase %s_HT_SEL_SM:\n", pModInst->m_instName.Upper().c_str());

		if (bNeedMifPollQue)
			iplPostInstr.Append("\tcase %s_HT_SEL_POLL:\n", pModInst->m_instName.Upper().c_str());

		if (pMod->m_bMultiThread) {
			iplPostInstr.Append("\t\tc_t%d_htPrivWe = r_t%d_htValid;\n",
				pMod->m_wrStg - 1, pMod->m_wrStg - 1);

			iplPostInstr.Append("\t\tc_t%d_htCmdRdy = c_t%d_htCtrl == HT_CONT || c_t%d_htCtrl == HT_JOIN_AND_CONT || c_t%d_htCtrl == HT_RETRY",
				pMod->m_wrStg - 1, pMod->m_wrStg - 1, pMod->m_wrStg - 1, pMod->m_wrStg - 1);

			iplPostInstr.Append(";\n");

			if (pMod->m_threads.m_htIdW.AsInt() > 0)
				iplPostInstr.Append("\t\tc_t%d_htCmd.m_htId = r_t%d_htId;\n",
				pMod->m_wrStg - 1, pMod->m_wrStg - 1);
			iplPostInstr.Append("\t\tc_t%d_htCmd.m_htInstr = c_t%d_htNextInstr;\n",
				pMod->m_wrStg - 1, pMod->m_wrStg - 1);
		} else {
			if (pMod->m_threads.m_bCallFork && pMod->m_threads.m_htIdW.AsInt() == 0)
				iplPostInstr.Append("\t\tif (r_t%d_htValid) {\n", pMod->m_wrStg - 1);
			else
				iplPostInstr.Append("\t\tif (r_t%d_htValid)\n", pMod->m_wrStg - 1);

			iplPostInstr.Append("\t\t\tc_htPriv = c_t%d_htPriv;\n",
				pMod->m_wrStg - 1);

			if (pMod->m_threads.m_bCallFork && pMod->m_threads.m_htIdW.AsInt() == 0) {
				iplPostInstr.Append("\t\t\tc_htPrivLk = false;\n");
				iplPostInstr.Append("\t\t}\n");
			}

			iplPostInstr.Append("\t\tif (r_t%d_htValid && c_t%d_htCtrl != HT_JOIN) {\n",
				pMod->m_wrStg - 1, pMod->m_wrStg - 1);
			iplPostInstr.Append("\t\t\tc_htCmdValid = c_t%d_htCtrl == HT_CONT || c_t%d_htCtrl == HT_JOIN_AND_CONT || c_t%d_htCtrl == HT_RETRY;\n",
				pMod->m_wrStg - 1, pMod->m_wrStg - 1, pMod->m_wrStg - 1);
			if (pMod->m_bGvIwComp)
				iplPostInstr.Append("\t\t\tc_t%d_htCmdRdy = c_htCmdValid;\n", pMod->m_wrStg - 1);
			iplPostInstr.Append("\t\t\tc_htCmd.m_htInstr = c_t%d_htNextInstr;\n",
				pMod->m_wrStg - 1);
			iplPostInstr.Append("\t\t}\n");
		}

		if (pMod->m_threads.m_resetInstr.size() > 0)
			iplPostInstr.Append("\t\tc_t%d_htTerm = c_t%d_htCtrl == HT_TERM;\n",
			pMod->m_wrStg - 1, pMod->m_wrStg - 1);

		iplPostInstr.Append("\t\tbreak;\n");

		iplPostInstr.Append("\tdefault:\n");
		iplPostInstr.Append("\t\tassert(0);\n");
		iplPostInstr.Append("\t}\n");
		iplPostInstr.Append("\n");

		if (pMod->m_bCallFork) {

			iplPostInstr.Append("\tassert_msg(%s || !r_t%d_htValid || ", reset.c_str(), pMod->m_wrStg - 1);

			if (pMod->m_bCallFork)
				iplPostInstr.Append("!r_t%d_rtnJoin || ", pMod->m_wrStg - 1);
			iplPostInstr.Append("c_t%d_htCtrl == HT_JOIN || c_t%d_htCtrl == HT_JOIN_AND_CONT,\n\t\t\"Runtime check failed in CPers%s::Pers%s_%s()"
				" - expected return from a forked call to performan HtJoin on the entry instruction\");\n",
				pMod->m_wrStg - 1, pMod->m_wrStg - 1,
				pModInst->m_instName.Uc().c_str(), pMod->m_modName.Uc().c_str(), pMod->m_clkRate == eClk1x ? "1x" : "2x");

			iplPostInstr.Append("\n");
		}

		// decrement reserved limit counts
		if (pMod->m_cxrEntryReserveCnt > 0) {
			iplPostInstr.Append("\tif (c_t%d_htCtrl == HT_RTN) {\n", pMod->m_wrStg - 1);
			iplPostInstr.Append("\t\tswitch (r_t%d_htPriv.m_rtnRsvSel) {\n", pMod->m_wrStg - 1);

			if (pMod->m_threads.m_resetInstr.size() > 0) {
				iplPostInstr.Append("\t\tcase %s_HT_SEL_RESET:\n",
					pModInst->m_instName.Upper().c_str());
				iplPostInstr.Append("\t\t\tbreak;\n");
			}

			for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

				if (pCxrIntf->m_cxrDir == CxrOut) continue;

				iplPostInstr.Append("\t\tcase %s_HT_SEL_%s_%d_%s:\n",
					pModInst->m_instName.Upper().c_str(), pCxrIntf->m_pSrcModInst->m_instName.Upper().c_str(), pCxrIntf->GetPortReplId(), pCxrIntf->m_modEntry.Upper().c_str());

				if (pCxrIntf->m_cxrType == CxrCall || pCxrIntf->m_cxrType == CxrTransfer) {

					for (size_t intfIdx2 = 0; intfIdx2 < pModInst->m_cxrIntfList.size(); intfIdx2 += 1) {
						CCxrIntf * pCxrIntf2 = pModInst->m_cxrIntfList[intfIdx2];

						if (pCxrIntf2->m_cxrDir == CxrOut) continue;
						if (pCxrIntf2->m_cxrType != CxrCall && pCxrIntf2->m_cxrType != CxrTransfer) continue;
						if (pCxrIntf2->m_reserveCnt == 0) continue;
						if (intfIdx2 == intfIdx) continue;

						iplPostInstr.Append("\t\t\tc_%s_%sRsvLimit%s += 1;\n",
							pCxrIntf2->GetPortNameSrcToDstLc(), pCxrIntf2->GetIntfName(), pCxrIntf2->GetPortReplIndex());
					}

				}

				iplPostInstr.Append("\t\t\tbreak;\n");
			}

			iplPostInstr.Append("\t\tcase %s_HT_SEL_SM:\n", pModInst->m_instName.Upper().c_str());

			if (bNeedMifPollQue)
				iplPostInstr.Append("\t\tcase %s_HT_SEL_POLL:\n", pModInst->m_instName.Upper().c_str());

			iplPostInstr.Append("\t\t\tbreak;\n");

			iplPostInstr.Append("\t\tdefault:\n");
			iplPostInstr.Append("\t\t\tassert(0);\n");
			iplPostInstr.Append("\t\t}\n");
			iplPostInstr.Append("\t}\n");
			iplPostInstr.Append("\n");
		}

		// m_htIdPool push
		if (!pMod->m_bGvIwComp) {
			for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

				if (pCxrIntf->m_cxrDir == CxrIn) continue;
				if (pCxrIntf->IsCall()) continue;

				if (pCxrIntf->IsXfer() && pCxrIntf->m_pDstModInst->m_pMod->m_clkRate == eClk1x && pCxrIntf->m_pDstModInst->m_pMod->m_clkRate != pMod->m_clkRate)
					iplPostInstr.Append("\tif (r_%s_%sRdy%s && !r_%s_%sHold%s)\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				else
					iplPostInstr.Append("\tif (r_%s_%sRdy%s)\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				if (pCxrIntf->m_pSrcGroup->m_htIdW.AsInt() == 0)
					iplPostInstr.Append("\t\tc_htBusy = false;\n");
				else
					iplPostInstr.Append("\t\tm_htIdPool.push(r_t%d_htId);\n", pMod->m_execStg + 1);
				iplPostInstr.Append("\n");
			}
		}

		if (pMod->m_threads.m_resetInstr.size() > 0) {

			iplPostInstr.Append("\tif (r_t%d_htTerm)\n", pMod->m_wrStg);

			if (pMod->m_threads.m_htIdW.AsInt() == 0)
				iplPostInstr.Append("\t\tc_htBusy = false;\n");
			else
				iplPostInstr.Append("\t\tm_htIdPool.push(r_t%d_htId);\n", pMod->m_wrStg);

			iplPostInstr.Append("\n");
		}

		if (pMod->m_bMultiThread) {

			if (pMod->m_threads.m_bCallFork && !pMod->m_bGvIwComp) {
				if (pMod->m_threads.m_htIdW.AsInt() <= 2) {
					// no code for registered version
				} else {
					for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
						CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

						if (pCxrIntf->m_cxrDir == CxrOut) continue;
						if (pCxrIntf->m_cxrType == CxrCall || pCxrIntf->m_cxrType == CxrTransfer) continue;

						iplPostInstr.Append("\tm_%s_%sPrivLk1%s.write_addr(r_t%d_htId);\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
							pMod->m_wrStg);
					}
					iplPostInstr.Append("\tm_htCmdPrivLk1.write_addr(r_t%d_htId);\n", pMod->m_wrStg);

					if (pMod->m_rsmSrcCnt > 0)
						iplPostInstr.Append("\tm_rsmPrivLk1.write_addr(r_t%d_htId);\n", pMod->m_wrStg);

					iplPostInstr.Append("\n");
				}
			}

			if (pMod->m_threads.m_htIdW.AsInt() == 0)
				iplPostInstr.Append("\tCHtPriv c_htPriv = r_htPriv;\n");
			else
				iplPostInstr.Append("\tm_htPriv.write_addr(r_t%d_htId);\n",
				pMod->m_wrStg);

			if (pMod->m_threads.m_bCallFork && !pMod->m_bGvIwComp)
				iplPostInstr.Append("\tif (r_t%d_htPrivWe) {\n", pMod->m_wrStg);
			else
				iplPostInstr.Append("\tif (r_t%d_htPrivWe)\n", pMod->m_wrStg);

			if (pMod->m_threads.m_htIdW.AsInt() > 0)
				iplPostInstr.Append("\t\tm_htPriv.write_mem(r_t%d_htPriv);\n",
				pMod->m_wrStg);
			else
				iplPostInstr.Append("\t\tc_htPriv = r_t%d_htPriv;\n",
				pMod->m_wrStg);
			iplPostInstr.Append("\n");

			if (pMod->m_threads.m_bCallFork && !pMod->m_bGvIwComp) {
				if (pMod->m_threads.m_htIdW.AsInt() == 0)
					iplPostInstr.Append("\t\tc_htPrivLk = 0;\n");
				else if (pMod->m_threads.m_htIdW.AsInt() <= 2)
					iplPostInstr.Append("\t\tc_htPrivLk[INT(r_t%d_htId)] = 0;\n", pMod->m_wrStg);
				else {
					for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
						CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

						if (pCxrIntf->m_cxrDir == CxrOut) continue;
						if (pCxrIntf->m_cxrType == CxrCall || pCxrIntf->m_cxrType == CxrTransfer) continue;

						iplPostInstr.Append("\t\tm_%s_%sPrivLk1%s.write_mem(r_t%d_htPrivLkData);\n",
							pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
							pMod->m_wrStg);
					}
					iplPostInstr.Append("\t\tm_htCmdPrivLk1.write_mem(r_t%d_htPrivLkData);\n", pMod->m_wrStg);

					if (pMod->m_rsmSrcCnt > 0)
						iplPostInstr.Append("\t\tm_rsmPrivLk1.write_mem(r_t%d_htPrivLkData);\n", pMod->m_wrStg);
				}

				iplPostInstr.Append("\t}\n");
				iplPostInstr.Append("\n");
			}
		}

		// internal command queue
		int stg;
		sscanf(pRegStg, "r_t%d", &stg);

		if (pMod->m_bMultiThread) {
			iplPostInstr.Append("\tif (r_t%d_htCmdRdy)\n",
				pMod->m_wrStg);

			if (pMod->m_threads.m_htIdW.AsInt() == 0)
				iplPostInstr.Append("\t\tc_htCmdValid = true;\n",
				pRegStg);
			else
				iplPostInstr.Append("\t\tm_htCmdQue.push(r_t%d_htCmd);\n",
				pMod->m_wrStg);
		}

		iplPostInstr.Append("\n");

		if (pMod->m_threads.m_htIdW.AsInt() > 0) {

			iplPostInstr.Append("\tsc_uint<%s_HTID_W + 1> c_htIdPoolCnt = r_htIdPoolCnt;\n",
				pMod->m_modName.Upper().c_str());

			iplPostInstr.Append("\tif (r_htIdPoolCnt < (1ul << %s_HTID_W)) {\n",
				pMod->m_modName.Upper().c_str());

			iplPostInstr.Append("\t\tm_htIdPool.push(r_htIdPoolCnt & ((1ul << %s_HTID_W) - 1));\n",
				pMod->m_modName.Upper().c_str());

			iplPostInstr.Append("\t\tc_htIdPoolCnt = r_htIdPoolCnt + 1;\n");

			if (pMod->m_threads.m_bCallFork && !(pMod->m_threads.m_htIdW.AsInt() <= 2)) {
				iplPostInstr.Append("\n");
				iplPostInstr.Append("\t\tc_t%d_htId = r_htIdPoolCnt & ((1ul << %s_HTID_W) - 1);\n",
					rdPrivLkStg - 1, pMod->m_modName.Upper().c_str());
				iplPostInstr.Append("\t\tc_t%d_htId = r_htIdPoolCnt & ((1ul << %s_HTID_W) - 1);\n",
					pMod->m_wrStg - 1, pMod->m_modName.Upper().c_str());
			}

			iplPostInstr.Append("\t}\n");
			iplPostInstr.Append("\n");
		}

		if (pMod->m_bHasThreads) {
			iplReg.Append("\tr_%sToHta_assert = c_htAssert;\n", pModInst->m_instName.Lc().c_str());

			if (pMod->m_clkRate == eClk2x) {
				m_iplReg1x.Append("\tr_%sToHta_assert_1x = r_%sToHta_assert;\n",
					pModInst->m_instName.Lc().c_str(), pModInst->m_instName.Lc().c_str());
				m_iplReg1x.NewLine();
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// IPL register assignments

	for (size_t fieldIdx = 0; fieldIdx < pMod->m_shared.m_fieldList.size(); fieldIdx += 1) {
		CField * pField = pMod->m_shared.m_fieldList[fieldIdx];

		if (pField->m_queueW.AsInt() == 0 && pField->m_addr1W.AsInt() == 0)
			continue;

		if (pField->m_dimenList.size() > 0)
			GenRamIndexLoops(iplReg, "", *pField);

		if (pField->m_queueW.size() > 0)
			iplReg.Append("\tm__SHR__%s%s.clock(%s);\n", pField->m_name.c_str(), pField->m_dimenIndex.c_str(), reset.c_str());
		else if (pField->m_addr1W.size() > 0)
			iplReg.Append("\tm__SHR__%s%s.clock(%s);\n", pField->m_name.c_str(), pField->m_dimenIndex.c_str(), reset.c_str());
	}

	if (pMod->m_threads.m_htIdW.AsInt() == 0) {
		iplReg.Append("\tr_htCmdValid = !%s && c_htCmdValid;\n", reset.c_str());
		iplReg.Append("\tr_htPriv = c_htPriv;\n");
		iplReg.Append("\tr_htBusy = !%s && c_htBusy;\n", reset.c_str());
		iplReg.Append("\n");
	} else {
		iplReg.Append("\tm_htCmdQue.clock(%s);\n", reset.c_str());
		iplReg.Append("\tm_htPriv.clock();\n");
		iplReg.Append("\tm_htIdPool.clock(%s);\n", reset.c_str());
		iplReg.Append("\n");
	}

	if (pMod->m_threads.m_bCallFork && !(pMod->m_threads.m_htIdW.AsInt() <= 2)) {
		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrOut) continue;
			if (pCxrIntf->m_cxrType == CxrCall || pCxrIntf->m_cxrType == CxrTransfer) continue;

			iplReg.Append("\tm_%s_%sPrivLk0%s.clock();\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			iplReg.Append("\tm_%s_%sPrivLk1%s.clock();\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
		}
		iplReg.Append("\tm_htCmdPrivLk0.clock();\n");
		iplReg.Append("\tm_htCmdPrivLk1.clock();\n");

		if (pMod->m_rsmSrcCnt > 0) {
			iplReg.Append("\tm_rsmPrivLk0.clock();\n");
			iplReg.Append("\tm_rsmPrivLk1.clock();\n");
		}

		iplReg.Append("\n");
	}

	// input cmd registers
	if (bStateMachine)
		iplReg.Append("\t// t1 select stage\n");

	for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

		if (pCxrIntf->m_cxrDir == CxrOut) continue;
		if (pCxrIntf->GetQueDepthW() > 0) continue;

		iplReg.Append("\tr_t1_%s_%sRdy%s = !%s && c_t0_%s_%sRdy%s;\n",
			pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(), 
			reset.c_str(),
			pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

		if (pCxrIntf->m_bCxrIntfFields)
			iplReg.Append("\tr_t1_%s_%s%s = c_t0_%s_%s%s;\n",
			pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
			pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

		iplReg.Append("\n");
	}

	////////////////////////////////////////////
	// clear shared variables without depth

	for (size_t shIdx = 0; shIdx < pMod->m_shared.m_fieldList.size(); shIdx += 1) {
		CField * pShared = pMod->m_shared.m_fieldList[shIdx];

		if (pShared->m_queueW.AsInt() > 0 || pShared->m_addr1W.AsInt() > 0) continue;
		if (pShared->m_reset == "false" || pShared->m_reset == "" && !pMod->m_bResetShared) continue;

		if (pShared->m_elemCnt < 4) {
			vector<int> refList(pShared->m_dimenList.size(), 0);
			do {
				string idxStr = IndexStr(refList);
				iplReset.Append("\t\tc__SHR__%s%s = 0;\n", pShared->m_name.c_str(), idxStr.c_str());

			} while (DimenIter(pShared->m_dimenList, refList));
		} else {
			GenRamIndexLoops(iplReset, "\t", *pShared);
			iplReset.Append("\t\tc__SHR__%s%s = 0;\n", pShared->m_name.c_str(), pShared->m_dimenIndex.c_str());
		}
	}

	//#########################################

	if (pMod->m_threads.m_htIdW.AsInt() == 0) {
		m_iplRegDecl.Append("\tbool r_htBusy;\n");

		m_iplRegDecl.Append("\tbool r_htCmdValid;\n");
	} else {
		m_iplRegDecl.Append("\tsc_uint<%s_HTID_W + 1> r_htIdPoolCnt;\n",
			pMod->m_modName.Upper().c_str());

		m_iplRegDecl.Append("\tht_dist_que<sc_uint<%s_HTID_W>, %s_HTID_W> m_htIdPool;\n",
			pMod->m_modName.Upper().c_str(), pMod->m_modName.Upper().c_str());

		iplReg.Append("\tr_htIdPoolCnt = %s ? (sc_uint<%s_HTID_W + 1>) 0 : c_htIdPoolCnt;\n", reset.c_str(),
			pMod->m_modName.Upper().c_str());

		if (pMod->m_threads.m_resetInstr.size() > 0)
			m_iplEosChecks.Append("\t\tassert(m_htIdPool.size() >= %d);\n",
			(1 << pMod->m_threads.m_htIdW.AsInt()) - 1);
		else
			m_iplEosChecks.Append("\t\tassert(m_htIdPool.size() == %d);\n",
			1 << pMod->m_threads.m_htIdW.AsInt());
	}

	////////////////////////////////////////////////////////////////////////////
	// IPL output assignments

	if (pMod->m_bHasThreads) {
		if (pMod->m_clkRate == eClk1x) {
			iplOut.Append("\n");
			iplOut.Append("\to_%sToHta_assert = r_%sToHta_assert;\n", pModInst->m_instName.Lc().c_str(), pModInst->m_instName.Lc().c_str());
		} else {
			m_iplOut1x.Append("\n");
			m_iplOut1x.Append("\to_%sToHta_assert = r_%sToHta_assert_1x;\n", pModInst->m_instName.Lc().c_str(), pModInst->m_instName.Lc().c_str());
		}
	}
}

void CDsnInfo::IplNewLine(CHtCode * pIplTxStg)
{
	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	m_iplReg1x.NewLine();
	m_iplReg2x.NewLine();
}

void CDsnInfo::IplComment(CHtCode * pIplTxStg, CHtCode & iplReg, const char * pMsg)
{
	IplNewLine(pIplTxStg);

	pIplTxStg->Append(pMsg);
	m_iplRegDecl.Append(pMsg);
	iplReg.Append(pMsg);
}
