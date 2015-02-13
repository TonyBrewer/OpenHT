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
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		mod.m_bMultiThread = false;
		mod.m_bHtId = false;

		mod.m_rsmSrcCnt += mod.m_threads.m_bPause ? 1 : 0;

		if ((mod.m_instrList.size() > 0) != (mod.m_cxrEntryList.size() > 0)) {
			if (mod.m_instrList.size() > 0)
				ParseMsg(Error, mod.m_threads.m_lineInfo, "module has AddInstr, but no AddEntry");
			else
				ParseMsg(Error, mod.m_threads.m_lineInfo, "module has AddEntry, but no AddInstr");
		} else {
			if (mod.m_instrList.size() == 0) {
				mod.m_bHasThreads = false;

				if (mod.m_threads.m_resetInstr.size() > 0)
					ParseMsg(Error, mod.m_threads.m_lineInfo, "module has AddModule( resetInstr= ), but no AddInstr");

				if (mod.m_threads.m_bPause)
					ParseMsg(Error, mod.m_threads.m_lineInfo, "module has AddModule( pause=true ), but no AddInstr");
			}
		}

		if (mod.m_bHasThreads) {
			mod.m_threads.m_htIdW.InitValue(mod.m_threads.m_lineInfo);

			mod.m_bMultiThread |= mod.m_threads.m_htIdW.AsInt() > 0;

			char defineName[64];
			sprintf(defineName, "%s_HTID_W", mod.m_modName.Upper().c_str());

			int defineHtIdW;
			bool bIsSigned;
			if (!m_defineTable.FindStringValue(mod.m_threads.m_lineInfo, string(defineName), defineHtIdW, bIsSigned))
				AddDefine(&mod.m_threads, defineName, VA("%d", mod.m_threads.m_htIdW.AsInt()));
			else if (defineHtIdW != mod.m_threads.m_htIdW.AsInt())
				ParseMsg(Error, mod.m_threads.m_lineInfo, "user defined identifier '%s'=%d has different value from AddThreads '%s'=%d",
				defineName, defineHtIdW, mod.m_threads.m_htIdW.AsStr().c_str(), mod.m_threads.m_htIdW.AsInt());

			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				char typeName[64];
				sprintf(typeName, "%sHtId_t", mod.m_modName.Uc().c_str());

				if (!FindTypeDef(typeName))
					AddTypeDef(typeName, "uint32_t", defineName, true);

				mod.m_bHtId = true;
			}
		}

		// check if a private variable has depth
		for (size_t prIdx = 0; prIdx < mod.m_threads.m_htPriv.m_fieldList.size(); prIdx += 1) {
			CField & priv = mod.m_threads.m_htPriv.m_fieldList[prIdx];

			if (priv.m_addr1W.AsInt() == 0) continue;

			if (priv.m_pPrivGbl == 0) {
				// create a global variable
				string rdStg = "1";
				string wrStg = mod.m_stage.m_execStg.AsStr();
				string nullStr;

				priv.m_pPrivGbl = & mod.AddIntRam(priv.m_name, priv.m_addr1Name, priv.m_addr2Name,
					priv.m_addr1W.AsStr(), priv.m_addr2W.AsStr(), nullStr,
					nullStr, rdStg, wrStg);
				priv.m_pPrivGbl->m_addr0W = mod.m_threads.m_htIdW;

				priv.m_pPrivGbl->m_bPrivGbl = true;
				priv.m_pPrivGbl->m_addr0W.InitValue(priv.m_pPrivGbl->m_lineInfo, false, 0);
				priv.m_pPrivGbl->m_addr1W.InitValue(priv.m_pPrivGbl->m_lineInfo, false, 0);
				priv.m_pPrivGbl->m_addr2W.InitValue(priv.m_pPrivGbl->m_lineInfo, false, 0);
				priv.m_pPrivGbl->DimenListInit(priv.m_pPrivGbl->m_lineInfo);
				priv.m_pPrivGbl->m_rdStg.InitValue(priv.m_pPrivGbl->m_lineInfo, false, 1);
				priv.m_pPrivGbl->m_wrStg.InitValue(priv.m_pPrivGbl->m_lineInfo, false, 1);
				priv.m_pPrivGbl->InitDimen();

				priv.m_pPrivGbl->AddGlobalField(priv.m_type, "data");
				CField & field = priv.m_pPrivGbl->m_fieldList.back();
				field.m_gblFieldIdx = 0;
				field.m_dimenList = priv.m_dimenList;
				field.DimenListInit(priv.m_pPrivGbl->m_lineInfo);
				field.InitDimen();
			}
		}

		// determine write stage
		int tsStg = 2;
		if (mod.m_bMultiThread)
			tsStg += mod.m_clkRate == eClk2x ? 3 : 1;
		else
			tsStg += mod.m_clkRate == eClk2x ? 1 : 0;

		mod.m_gblBlockRam = mod.m_extRamList.size() > 0;
		mod.m_gblDistRam = mod.m_intGblList.size() > 0;
		for (size_t ngvIdx = 0; ngvIdx < mod.m_globalVarList.size(); ngvIdx += 1) {

			if (mod.m_globalVarList[ngvIdx]->m_addr1W.AsInt() == 0) continue;

			if (mod.m_globalVarList[ngvIdx]->m_pNgvInfo->m_ramType == eBlockRam)
				mod.m_gblBlockRam = true;
			else
				mod.m_gblDistRam = true;
		}

		if (mod.m_gblBlockRam)
			tsStg += 3;
		else if (mod.m_gblDistRam)
			tsStg += 2;

		mod.m_tsStg = tsStg;
		mod.m_execStg = tsStg + mod.m_stage.m_execStg.AsInt() - 1;
		mod.m_wrStg = tsStg + mod.m_stage.m_privWrStg.AsInt();
	}
}

struct CHtSelName {
	CHtSelName(string name, string replDecl, string replIndex) : m_name(name), m_replDecl(replDecl), m_replIndex(replIndex) {}

	string m_name;
	string m_replDecl;
	string m_replIndex;
};

void CDsnInfo::GenModIplStatements(CModule &mod, int modInstIdx)
{
	string vcdModName = VA("Pers%s", mod.m_modName.Uc().c_str());

	g_appArgs.GetDsnRpt().AddLevel("Module Attributes\n");
	if (mod.m_bHasThreads)
		g_appArgs.GetDsnRpt().AddItem("HtIdW = %d\n", mod.m_threads.m_htIdW.AsInt());
	else
		g_appArgs.GetDsnRpt().AddItem("No Instruction state machine\n");
	g_appArgs.GetDsnRpt().AddItem("Clock rate = %s\n", mod.m_clkRate == eClk2x ? "2x" : "1x");
	g_appArgs.GetDsnRpt().EndLevel();

	bool bStateMachine = mod.m_bHasThreads;

	if (!bStateMachine) return;

	CModInst & modInst = mod.m_modInstList[modInstIdx];

	string instIdStr = GenIndexStr(mod.m_instIdCnt > 1, "%d", modInst.m_instParams.m_instId);
	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

	CHtCode	& iplT0Stg = mod.m_clkRate == eClk1x ? m_iplT0Stg1x : m_iplT0Stg2x;
	CHtCode	& iplT1Stg = mod.m_clkRate == eClk1x ? m_iplT1Stg1x : m_iplT1Stg2x;
	CHtCode	& iplT2Stg = mod.m_clkRate == eClk1x ? m_iplT2Stg1x : m_iplT2Stg2x;
	CHtCode	& iplTsStg = mod.m_clkRate == eClk1x ? m_iplTsStg1x : m_iplTsStg2x;
	CHtCode	& iplPostInstr = mod.m_clkRate == eClk1x ? m_iplPostInstr1x : m_iplPostInstr2x;
	CHtCode	& iplReg = mod.m_clkRate == eClk1x ? m_iplReg1x : m_iplReg2x;
	CHtCode	& iplPostReg = mod.m_clkRate == eClk1x ? m_iplPostReg1x : m_iplPostReg2x;
	CHtCode	& iplOut = mod.m_clkRate == eClk1x ? m_iplOut1x : m_iplOut2x;
	CHtCode & iplClrShared = mod.m_clkRate == eClk1x ? m_iplClrShared1x : m_iplClrShared2x;

	bool bMultiThread = mod.m_threads.m_htIdW.AsInt() > 0;

	int maxInstrLen = 0;
	m_iplMacros.Append("#ifndef _HTV\n");
	if (g_appArgs.IsInstrTraceEnabled()) {
		m_iplMacros.Append("char const * CPers%s%s%s::m_pHtCtrlNames[] = {\n\t\t\"INVALID\",\n\t\t\"CONTINUE\",\n\t\t\"CALL\",\n\t\t\"RETURN\",\n",
			unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str());
		m_iplMacros.Append("\t\t\"JOIN\",\n\t\t\"RETRY\",\n\t\t\"TERMINATE\",\n\t\t\"PAUSE\",\n\t\t\"JOIN_AND_CONT\"\n\t};\n");
	}

	m_iplMacros.Append("char const * CPers%s%s%s::m_pInstrNames[] = {\n",
		unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str());
	for (size_t instrIdx = 0; instrIdx < mod.m_instrList.size(); instrIdx += 1) {
		m_iplMacros.Append("\t\t\"%s\",\n", mod.m_instrList[instrIdx].c_str());
		int instrLen = mod.m_instrList[instrIdx].size();
		if (maxInstrLen < instrLen)
			maxInstrLen = instrLen;
	}
	m_iplMacros.Append("\t};\n");
	m_iplMacros.Append("#endif\n");
	m_iplMacros.Append("\n");

	///////////////////////////////////////////////////////////////////////
	// Generate instruction pipe line macros

	m_iplMacros.Append("#define %s_RND_RETRY %s\n", mod.m_modName.Upper().c_str(), m_bRndRetry ? "true" : "false");
	m_iplMacros.Append("\n");

	m_iplMacros.Append("// Hardware thread methods\n");
		
	g_appArgs.GetDsnRpt().AddLevel("Thread Control\n");

	g_appArgs.GetDsnRpt().AddLevel("HtRetry()\n");
	g_appArgs.GetDsnRpt().EndLevel();

	m_iplFuncDecl.Append("\tvoid HtRetry();\n");
	m_iplMacros.Append("void CPers%s%s%s::HtRetry()\n",
		unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str());
	m_iplMacros.Append("{\n");

	m_iplMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::HtRetry()"
		" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());

	m_iplMacros.Append("\tc_t%d_htCtrl = HT_RETRY;\n", mod.m_execStg);
	m_iplMacros.Append("}\n");
	m_iplMacros.Append("\n");

	g_appArgs.GetDsnRpt().AddLevel("HtContinue(ht_uint%d nextInstr)\n", mod.m_instrW);
	g_appArgs.GetDsnRpt().EndLevel();

	m_iplFuncDecl.Append("\tvoid HtContinue(ht_uint%d nextInstr);\n", mod.m_instrW);
	m_iplMacros.Append("void CPers%s%s%s::HtContinue(ht_uint%d nextInstr)\n",
		unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str(), mod.m_instrW);
	m_iplMacros.Append("{\n");
	m_iplMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::HtContinue()"
		" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());
	m_iplMacros.Append("\tc_t%d_htCtrl = HT_CONT;\n", mod.m_execStg);
	m_iplMacros.Append("\tc_t%d_htNextInstr = nextInstr;\n", mod.m_execStg);
	m_iplMacros.Append("}\n");
	m_iplMacros.Append("\n");

	if (mod.m_threads.m_bPause) {
		g_appArgs.GetDsnRpt().AddLevel("void HtPause(ht_uint%d nextInstr)\n", mod.m_instrW);
		g_appArgs.GetDsnRpt().EndLevel();
		m_iplFuncDecl.Append("\tvoid HtPause(ht_uint%d nextInstr);\n", mod.m_instrW);
		m_iplMacros.Append("void CPers%s%s%s::HtPause(ht_uint%d nextInstr)\n",
			unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str(), mod.m_instrW);
		m_iplMacros.Append("{\n");
		m_iplMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::HtPause()"
			" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());
		m_iplMacros.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			m_iplMacros.Append("\tm_htRsmInstr.write_addr(r_t%d_htId);\n", mod.m_execStg);
			m_iplMacros.Append("\tm_htRsmInstr.write_mem(nextInstr);\n");
			m_iplMacros.Append("#ifndef _HTV\n");
			m_iplMacros.Append("\tassert(m_htRsmWait.read_mem_debug(r_t%d_htId) == false);\n", mod.m_execStg);
			m_iplMacros.Append("\tm_htRsmWait.write_mem_debug(r_t%d_htId) = true;\n", mod.m_execStg);
			m_iplMacros.Append("#endif\n");
		} else {
			m_iplMacros.Append("\tc_htRsmInstr = nextInstr;\n");
			m_iplMacros.Append("#ifndef _HTV\n");
			m_iplMacros.Append("\tassert(r_htRsmWait == false);\n");
			m_iplMacros.Append("\tc_htRsmWait = true;\n");
			m_iplMacros.Append("#endif\n");
		}
		m_iplMacros.Append("}\n");
		m_iplMacros.Append("\n");

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			g_appArgs.GetDsnRpt().AddLevel("void HtResume(ht_uint<%s_HTID_W> htId)\n", mod.m_modName.Upper().c_str());
			g_appArgs.GetDsnRpt().EndLevel();

			m_iplFuncDecl.Append("\tvoid HtResume(sc_uint<%s_HTID_W> htId);\n", mod.m_modName.Upper().c_str());
			m_iplMacros.Append("void CPers%s%s%s::HtResume(sc_uint<%s_HTID_W> htId)\n",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str(), mod.m_modName.Upper().c_str());
		} else if (mod.m_threads.m_htIdW.size() > 0) {
			g_appArgs.GetDsnRpt().AddLevel("void HtResume(ht_uint1 htId)\n");
			g_appArgs.GetDsnRpt().EndLevel();

			m_iplFuncDecl.Append("\tvoid HtResume(ht_uint1 htId);\n");
			m_iplMacros.Append("void CPers%s%s%s::HtResume(ht_uint1 ht_noload htId)\n", 
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str());
		} else {
			g_appArgs.GetDsnRpt().AddLevel("void HtResume()\n");
			g_appArgs.GetDsnRpt().EndLevel();

			m_iplFuncDecl.Append("\tvoid HtResume();\n");
			m_iplMacros.Append("void CPers%s%s%s::HtResume()\n", 
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str());
		}
		m_iplMacros.Append("{\n");
		if (mod.m_threads.m_htIdW.size() > 0 && mod.m_threads.m_htIdW.AsInt() == 0)
			m_iplMacros.Append("\tassert_msg(htId == 0, \"Runtime check failed in CPers%s::HtResume()"
				" - expected htId parameter value to be zero\");\n", mod.m_modName.Uc().c_str());
		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			if (mod.m_rsmSrcCnt > 1) {
				m_iplMacros.Append("\tCHtRsm rsm;\n");
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
			m_iplMacros.Append("#ifndef _HTV\n");
			m_iplMacros.Append("\tassert(c_reset1x || m_htRsmWait.read_mem_debug(htId) == true);\n");
			m_iplMacros.Append("\tm_htRsmWait.write_mem_debug(htId) = false;\n");
			m_iplMacros.Append("#endif\n");
		} else {
			m_iplMacros.Append("\tc_t0_rsmHtRdy = true;\n");
			m_iplMacros.Append("\tc_t0_rsmHtInstr = r_htRsmInstr;\n");
			m_iplMacros.Append("#ifndef _HTV\n");
			m_iplMacros.Append("\tassert(c_reset1x || r_htRsmWait == true);\n");
			m_iplMacros.Append("\tc_htRsmWait = false;\n");
			m_iplMacros.Append("#endif\n");
		}
		m_iplMacros.Append("}\n");
		m_iplMacros.Append("\n");
	}

	g_appArgs.GetDsnRpt().AddLevel("void HtTerminate()\n");
	g_appArgs.GetDsnRpt().EndLevel();

	m_iplFuncDecl.Append("\tvoid HtTerminate();\n");
	m_iplMacros.Append("void CPers%s%s%s::HtTerminate()\n",
		unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str());
	m_iplMacros.Append("{\n");
	m_iplMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::HtTerminate()"
		" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());
	m_iplMacros.Append("\tc_t%d_htCtrl = HT_TERM;\n", mod.m_execStg);
	m_iplMacros.Append("}\n");
	m_iplMacros.Append("\n");

	g_appArgs.GetDsnRpt().EndLevel();

	CHtCode *pIplTxStg = &iplT0Stg;

	// determine HT_SEL type width
	int rsvSelCnt = mod.m_threads.m_resetInstr.size() > 0 ? 1 : 0;
	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrOut) continue;

		rsvSelCnt += 1;
	}
	if (modInst.m_cxrSrcCnt > 1)
		rsvSelCnt += 1;
	int rsvSelW = FindLg2(rsvSelCnt);

	if (mod.m_cxrEntryReserveCnt > 0) {
		char typeName[32];
		sprintf(typeName, "ht_uint%d", rsvSelW);
		mod.m_threads.m_htPriv.AddStructField(typeName, "rtnRsvSel");
	}
	
	char htIdTypeStr[128];
	sprintf(htIdTypeStr, "sc_uint<%s_HTID_W>", mod.m_modName.Upper().c_str());
	char htInstrTypeStr[128];
	sprintf(htInstrTypeStr, "sc_uint<%s_INSTR_W>", mod.m_modName.Upper().c_str());

	{
		m_iplRegDecl.Append("\tstruct CHtCmd {\n");
		m_iplRegDecl.Append("#ifndef _HTV\n");
		m_iplRegDecl.Append("\t\tCHtCmd() {} // avoid run-time uninitialized error\n");
		m_iplRegDecl.Append("#endif\n");
		m_iplRegDecl.Append("\n");
		m_iplRegDecl.Append("\t\tuint32_t\t\tm_htInstr:%d;\n", mod.m_instrW);
		if (mod.m_threads.m_htIdW.AsInt() > 0)
			m_iplRegDecl.Append("\t\tuint32_t\t\tm_htId:%s_HTID_W;\n", mod.m_modName.Upper().c_str());
		m_iplRegDecl.Append("\t};\n");
		m_iplRegDecl.Append("\n");

		// if no private variables then declare one to avoid zero width struct
		bool bFoundField = false;
		for (size_t fldIdx = 0; fldIdx < mod.m_threads.m_htPriv.m_fieldList.size(); fldIdx += 1)
			if (mod.m_threads.m_htPriv.m_fieldList[fldIdx].m_pPrivGbl == 0) {
				bFoundField = true;
				break;
			}

		if (bFoundField == false) {
			string type = "bool";
			string name = "null";

			mod.m_threads.m_htPriv.AddStructField(type, name);
			mod.m_threads.m_htPriv.m_fieldList.back().DimenListInit(mod.m_threads.m_lineInfo);
		}
		//string htPrivName = "CHtPriv";
		mod.m_threads.m_htPriv.m_structName = "CHtPriv";
		mod.m_threads.m_htPriv.m_bCStyle = false;
		GenUserStructs(m_iplRegDecl, mod.m_threads.m_htPriv, "\t");

		string privStructName = VA("CPers%s::CHtPriv", mod.m_modName.Uc().c_str());

		GenUserStructBadData(m_iplBadDecl, true, privStructName,
			mod.m_threads.m_htPriv.m_fieldList, mod.m_threads.m_htPriv.m_bCStyle, "");

		m_iplRegDecl.Append("\tstruct CHtRsm {\n");
		if (mod.m_threads.m_htIdW.AsInt() > 0)
			m_iplRegDecl.Append("\t\tsc_uint<%s_HTID_W> m_htId;\n", mod.m_modName.Upper().c_str());
		m_iplRegDecl.Append("\t\tsc_uint<%s_INSTR_W> m_htInstr;\n", mod.m_modName.Upper().c_str());
		m_iplRegDecl.Append("\t};\n");
		m_iplRegDecl.Append("\n");

		if (mod.m_threads.m_htIdW.AsInt() == 0) {

			m_iplRegDecl.Append("\tCHtCmd r_htCmd;\n");
			m_iplRegDecl.Append("\tCHtPriv r_htPriv;\n");

		} else {
			m_iplRegDecl.Append("\tht_dist_que<CHtCmd, %s_HTID_W> m_htCmdQue;\n",
				mod.m_modName.Upper().c_str());
			if (mod.m_threads.m_ramType == eDistRam)
				m_iplRegDecl.Append("\tht_dist_ram<CHtPriv, %s_HTID_W> m_htPriv;\n",
					mod.m_modName.Upper().c_str());
			else
				m_iplRegDecl.Append("\tht_block_ram<CHtPriv, %s_HTID_W> m_htPriv;\n",
					mod.m_modName.Upper().c_str());
		}

		if (mod.m_threads.m_bCallFork) {
			if (mod.m_threads.m_htIdW.AsInt() <= 2) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("ht_uint%d", 1 << mod.m_threads.m_htIdW.AsInt()), VA("r_htPrivLk") );
				iplReg.Append("\tr_htPrivLk = c_reset1x ? (ht_uint%d)0 : c_htPrivLk;\n",
					1 << mod.m_threads.m_htIdW.AsInt());
			} else {
				for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
					CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

					if (cxrIntf.m_cxrDir == CxrOut) continue;
					if (cxrIntf.m_cxrType == CxrCall || cxrIntf.m_cxrType == CxrTransfer) continue;
					if (cxrIntf.m_srcReplCnt > 1 && cxrIntf.m_srcReplId != 0) continue;

					m_iplRegDecl.Append("\tht_dist_ram<bool, %s_HTID_W> m_%s_%sPrivLk0%s;\n",
						mod.m_modName.Upper().c_str(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
					m_iplRegDecl.Append("\tht_dist_ram<bool, %s_HTID_W> m_%s_%sPrivLk1%s;\n", 
						mod.m_modName.Upper().c_str(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
				}
				m_iplRegDecl.Append("\tht_dist_ram<bool, %s_HTID_W> m_htCmdPrivLk0;\n", 
					mod.m_modName.Upper().c_str());
				m_iplRegDecl.Append("\tht_dist_ram<bool, %s_HTID_W> m_htCmdPrivLk1;\n",
					mod.m_modName.Upper().c_str());

				if (mod.m_rsmSrcCnt > 0) {
					m_iplRegDecl.Append("\tht_dist_ram<bool, %s_HTID_W> m_rsmPrivLk0;\n", 
						mod.m_modName.Upper().c_str());
					m_iplRegDecl.Append("\tht_dist_ram<bool, %s_HTID_W> m_rsmPrivLk1;\n",
						mod.m_modName.Upper().c_str());
				}

				m_iplRegDecl.Append("\n");
			}
		}
	}

	///////////////////////////////////////////////////////////////////////
	// First pipe stage: ht arbitration (always t1)

	pIplTxStg = &iplT1Stg;

	pIplTxStg->Append("\t// First pipe stage: ht arbitration\n");

	if (modInst.m_cxrSrcCnt > 1) {
		pIplTxStg->Append("\tht_uint%d c_t1_htSel = 0;\n", FindLg2(modInst.m_cxrSrcCnt-1));
		//if (mod.m_clkRate == eClk2x) {
		//	m_iplRegDecl.Append("\tht_uint%d r_t2_htSel;\n", FindLg2(modInst.m_cxrSrcCnt-1));
		//	iplReg.Append("\tr_t2_htSel = c_t1_htSel;\n");
		//}
	}

	pIplTxStg->Append("\tbool c_t1_htValid = false;\n");

	if (mod.m_threads.m_htIdW.AsInt() == 0) {

		pIplTxStg->Append("\tbool c_htBusy = r_htBusy;\n");
		pIplTxStg->Append("\tbool c_htCmdValid = r_htCmdValid;\n");

		if (mod.m_threads.m_bCallFork && mod.m_threads.m_htIdW.AsInt() == 0)
			pIplTxStg->Append("\tht_uint%d c_htPrivLk = r_htPrivLk;\n", 
				1 << mod.m_threads.m_htIdW.AsInt());
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	// generate reset complete signal
	bool bResetCnt = false;
	char const * pResetCompStr = "\tbool c_resetComplete = ";

	if (mod.m_mif.m_bMifRd && mod.m_mif.m_mifRd.m_rspGrpIdW > 2) {
		pIplTxStg->Append("%sr_rdRspGrpInitCnt[%s_RD_GRP_ID_W] != 0",
			pResetCompStr, mod.m_modName.Upper().c_str());
		pResetCompStr = " &&\n\t\t";
		bResetCnt = true;
	}

	if (mod.m_mif.m_bMifWr && mod.m_mif.m_mifWr.m_rspGrpIdW > 2) {
		pIplTxStg->Append("%sr_wrRspGrpInitCnt[%s_WR_GRP_ID_W] != 0",
			pResetCompStr, mod.m_modName.Upper().c_str());
		pResetCompStr = " &&\n\t\t";
		bResetCnt = true;
	}

	if (mod.m_bHtIdInit) {
		pIplTxStg->Append("%s!r_t%d_htIdInit", pResetCompStr, mod.m_execStg);
		pResetCompStr = " &&\n\t\t";
		bResetCnt = true;
	}

	for (size_t barIdx = 0; barIdx < mod.m_barrierList.size(); barIdx += 1) {
		CBarrier * pBar = mod.m_barrierList[barIdx];

		bool bMimicHtCont = mod.m_threads.m_htIdW.AsInt() == 0 && mod.m_modInstList.size() == 1;

		if (bMimicHtCont || pBar->m_barIdW.AsInt() <= 2) continue;

		pIplTxStg->Append("%sr_bar%sInitCnt[%d] != 0", pResetCompStr, pBar->m_name.Uc().c_str(), pBar->m_barIdW.AsInt());
		pResetCompStr = " &&\n\t\t";
		bResetCnt = true;
	}

	if (mod.m_threads.m_htIdW.AsInt() > 0) {

		pIplTxStg->Append("%sr_htIdPoolCnt[%s_HTID_W] != 0", 
			pResetCompStr, mod.m_modName.Upper().c_str());

		pResetCompStr = " &&\n\t\t";
		bResetCnt = true;
	}

	if (bResetCnt)
		pIplTxStg->Append(";\n");
	else {
		pIplTxStg->Append("%s!c_reset1x;\n", pResetCompStr);
		bResetCnt = true;
	}

	pIplTxStg->Append("\n");

	m_iplRegDecl.Append("\tbool r_resetComplete;\n");
	iplReg.Append("\tr_resetComplete = !c_reset1x && c_resetComplete;\n");

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	if (mod.m_threads.m_resetInstr.size() > 0) {
		m_iplRegDecl.Append("\tbool r_resetInstrStarted;\n");
		pIplTxStg->Append("\tbool c_resetInstrStarted = r_resetInstrStarted;\n");
		iplReg.Append("\tr_resetInstrStarted = !c_reset1x && c_resetInstrStarted;\n");
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	// generate reserved thread state
	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrOut) continue;
		if (cxrIntf.m_cxrType != CxrCall && cxrIntf.m_cxrType != CxrTransfer) continue;
		if (cxrIntf.m_reserveCnt == 0) continue;

		if (cxrIntf.GetPortReplCnt() <= 1 || cxrIntf.GetPortReplId() == 0) {
			m_iplRegDecl.Append("\tbool r_%s_%sRsvLimitEmpty%s;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
			m_iplRegDecl.Append("\tsc_uint<%s> r_%s_%sRsvLimit%s;\n",
				mod.m_threads.m_htIdW.c_str(),
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
		}

		if (cxrIntf.GetPortReplCnt() <= 1) {
			pIplTxStg->Append("\tsc_uint<%s> c_%s_%sRsvLimit = r_%s_%sRsvLimit;\n",
				mod.m_threads.m_htIdW.c_str(),
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(),
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
		} else {
			if (cxrIntf.GetPortReplId() == 0)
				pIplTxStg->Append("\tsc_uint<%s> c_%s_%sRsvLimit%s;\n",
					mod.m_threads.m_htIdW.c_str(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

			pIplTxStg->Append("\tc_%s_%sRsvLimit%s = r_%s_%sRsvLimit%s;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
		}

		iplReg.Append("\tr_%s_%sRsvLimitEmpty%s = c_%s_%sRsvLimit%s == 0;\n",
			cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
			cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
		iplReg.Append("\tr_%s_%sRsvLimit%s = c_reset1x ? (sc_uint<%s>)%d : c_%s_%sRsvLimit%s;\n",
			cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
			mod.m_threads.m_htIdW.c_str(),
			(1 << mod.m_threads.m_htIdW.AsInt()) - cxrIntf.m_reserveCnt,
			cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrOut) continue;
		if (cxrIntf.m_pSrcMod->m_globalVarList.size() == 0) continue;

		GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("ht_uint%d", cxrIntf.GetQueDepthW() + 1), VA("r_%sCompCnt", cxrIntf.GetIntfName()));
		m_iplRegDecl.Append("\tht_uint%d c_%sCompCnt;\n", cxrIntf.GetQueDepthW() + 1, cxrIntf.GetIntfName());

		if (cxrIntf.m_pSrcMod->m_clkRate == eClk2x && mod.m_clkRate == eClk1x) {
			pIplTxStg->Append("\tc_%sCompCnt = r_%sCompCnt + r_%sCompCnt_2x.read();\n", cxrIntf.GetIntfName(), cxrIntf.GetIntfName(), cxrIntf.GetIntfName());
		} else if (cxrIntf.m_pSrcMod->m_clkRate == eClk1x && mod.m_clkRate == eClk1x) {
			pIplTxStg->Append("\tc_%sCompCnt = r_%sCompCnt + (i_%s_%sCompRdy.read() ? 1 : 0);\n", cxrIntf.GetIntfName(), cxrIntf.GetIntfName(), cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
		}

		iplReg.Append("\tr_%sCompCnt = c_reset1x ? (ht_uint%d)0 : c_%sCompCnt;\n", cxrIntf.GetIntfName(), cxrIntf.GetQueDepthW() + 1, cxrIntf.GetIntfName());

		if (cxrIntf.m_pSrcMod->m_clkRate == eClk2x && mod.m_clkRate == eClk1x) {
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "sc_signal<ht_uint2>", VA("r_%sCompCnt_2x", cxrIntf.GetIntfName()));
			m_iplRegDecl.Append("\tht_uint2 c_%sCompCnt_2x;\n", cxrIntf.GetIntfName());

			m_iplPostInstr2x.Append("\tc_%sCompCnt_2x = r_phase == 0 ? (ht_uint2)0 : r_%sCompCnt_2x;\n", cxrIntf.GetIntfName(), cxrIntf.GetIntfName());
			m_iplPostInstr2x.Append("\tc_%sCompCnt_2x += i_%s_%sCompRdy.read() ? 1 : 0;\n", cxrIntf.GetIntfName(), cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());

			m_iplReg2x.Append("\tr_%sCompCnt_2x = c_%sCompCnt_2x;\n", cxrIntf.GetIntfName(), cxrIntf.GetIntfName());
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
	if (mod.m_threads.m_htIdW.AsInt() == 0) {
		if (mod.m_globalVarList.size() == 0) {
			pIplTxStg->Append("\tbool c_htCmdRdy = r_htCmdValid");
			if (mod.m_threads.m_bCallFork)
					pIplTxStg->Append(" && !r_htPrivLk");
			pIplTxStg->Append(";\n");
		} else {
			pIplTxStg->Append("\tc_htCompQueAvlCnt = r_htCompQueAvlCnt;\n");
			pIplTxStg->Append("\tc_htCompRdy = false;\n");
			pIplTxStg->Append("\tbool c_htCmdRdy = r_htCmdValid && r_htCompRdy;\n");
		}
	} else {
		if (mod.m_globalVarList.size() == 0)
			pIplTxStg->Append("\tbool c_htCmdRdy = !m_htCmdQue.empty();");
		else {
			pIplTxStg->Append("\tc_htCompQueAvlCnt = r_htCompQueAvlCnt;\n");
			pIplTxStg->Append("\tc_htCmdRdyCnt = r_htCmdRdyCnt;\n");
			pIplTxStg->Append("\tbool c_htCmdRdy = r_htCmdRdyCnt > 0 && r_htCompQueAvlCnt > 0;");
		}
	}

	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrType == CxrCall || cxrIntf.m_cxrType == CxrTransfer) continue;
		if (cxrIntf.m_cxrDir == CxrOut) continue;

		char htSelNameStr[128];
		sprintf(htSelNameStr, "%s_%s",
			cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
		htSelNameList.push_back(CHtSelName(htSelNameStr, cxrIntf.GetPortReplDecl(), cxrIntf.GetPortReplIndex()));

		if (cxrIntf.GetPortReplCnt() <= 1)
			pIplTxStg->Append("\tbool c_%sRdy = ", htSelNameStr);
		else {
			if (cxrIntf.GetPortReplId() == 0)
				pIplTxStg->Append("\tbool c_%sRdy%s;\n", htSelNameStr, cxrIntf.GetPortReplDecl());

			pIplTxStg->Append("\tc_%sRdy%s = ", htSelNameStr, cxrIntf.GetPortReplIndex());
		}

		if (cxrIntf.GetQueDepthW() > 0) {
			if (cxrIntf.m_bCxrIntfFields) {
				if (mod.m_clkRate == eClk1x && cxrIntf.m_pSrcMod->m_clkRate == eClk1x)
					pIplTxStg->Append("!m_%s_%sQue%s.empty()",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				else
					pIplTxStg->Append("!r_%s_%sQueEmpty%s",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			} else
				pIplTxStg->Append("r_%s_%sCnt%s > 0",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
		} else
			pIplTxStg->Append("r_t1_%s_%sRdy%s",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

		if (cxrIntf.m_cxrType == CxrReturn) {

			CCxrIntf &cxrCallIntf = modInst.m_cxrIntfList[cxrIntf.m_callIntfIdx];
			CCxrCall & cxrCall = modInst.m_pMod->m_cxrCallList[cxrCallIntf.m_callIdx];

			if (cxrCall.m_pGroup->m_htIdW.AsInt() > 0) {
			} else {
				if (mod.m_bCallFork)
					pIplTxStg->Append(" && !r_htPrivLk");
			}
		}

		if (mod.m_globalVarList.size() > 0) {
			pIplTxStg->Append(" && r_htCompQueAvlCnt > 0", cxrIntf.GetIntfName());
		}

		if (cxrIntf.m_pSrcMod->m_globalVarList.size() > 0) {
			pIplTxStg->Append(" && r_%sCompCnt > 0", cxrIntf.GetIntfName());
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

			uint32_t mask1 = (1ul << selRrCnt)-1;
			uint32_t mask2 = ((1ul << j)-1) << (i+1);
			uint32_t mask3 = (mask2 & mask1) | (mask2 >> selRrCnt);

			pIplTxStg->Append("\t\t(c_%sRdy%s && (r_htSelRr & 0x%x) != 0)%s\n",
				htSelNameList[k].m_name.c_str(), htSelNameList[k].m_replIndex.c_str(), 
				mask3, j == selRrCnt-1 ? "));" : " ||");
		}
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	if (selRrCnt > 1) {
		m_iplRegDecl.Append("\tht_uint%d r_htSelRr;\n", selRrCnt);
		pIplTxStg->Append("\tht_uint%d c_htSelRr = r_htSelRr;\n", selRrCnt);
		iplReg.Append("\tr_htSelRr = c_reset1x ? (ht_uint%d)0x1 : c_htSelRr;\n", selRrCnt);
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	// generate cmds
	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrOut) continue;
		if (!cxrIntf.m_bCxrIntfFields) continue;

		if (cxrIntf.GetQueDepthW() > 0) {
			if (mod.m_clkRate == eClk1x && cxrIntf.m_pSrcMod->m_clkRate == eClk1x) {
				if (cxrIntf.GetPortReplCnt() <= 1)
					pIplTxStg->Append("\tC%s_%s c_t1_%s_%s = m_%s_%sQue.front();\n",
						cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
						cxrIntf.GetSrcToDstLc(), cxrIntf.GetIntfName(),
						cxrIntf.GetSrcToDstLc(), cxrIntf.GetIntfName());
				else {
					if (cxrIntf.GetPortReplId() == 0)
						pIplTxStg->Append("\tC%s_%s c_t1_%s_%s%s;\n",
							cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

					pIplTxStg->Append("\tc_t1_%s_%s%s = m_%s_%sQue%s.front();\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
			} else {
				if (cxrIntf.GetPortReplCnt() <= 1)
					pIplTxStg->Append("\tC%s_%s c_t1_%s_%s = r_%s_%sQueFront;\n",
						cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
						cxrIntf.GetSrcToDstLc(), cxrIntf.GetIntfName(),
						cxrIntf.GetSrcToDstLc(), cxrIntf.GetIntfName());
				else {
					if (cxrIntf.GetPortReplId() == 0)
						pIplTxStg->Append("\tC%s_%s c_t1_%s_%s%s;\n",
							cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

					pIplTxStg->Append("\tc_t1_%s_%s%s = r_%s_%sQueFront%s;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
			}
		} else {
			if (cxrIntf.GetPortReplCnt() <= 1)
				pIplTxStg->Append("\tC%s_%s c_t1_%s_%s = r_t1_%s_%s;\n",
					cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
					cxrIntf.GetSrcToDstLc(), cxrIntf.GetIntfName(),
					cxrIntf.GetSrcToDstLc(), cxrIntf.GetIntfName());
			else {
				if (cxrIntf.GetPortReplId() == 0)
					pIplTxStg->Append("\tC%s_%s c_t1_%s_%s%s;\n",
						cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

				pIplTxStg->Append("\tc_t1_%s_%s%s = r_t1_%s_%s%s;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			}
		}

		if (cxrIntf.m_cxrType == CxrReturn && mod.m_threads.m_htIdW.AsInt() > 0) {
			if (cxrIntf.GetPortReplCnt() <= 1)
				pIplTxStg->Append("\tsc_uint<%s> c_t1_%s_%sHtId%s = c_t1_%s_%s%s.m_rtnHtId;\n",
					mod.m_threads.m_htIdW.c_str(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			else {
				if (cxrIntf.GetPortReplId() == 0)
					pIplTxStg->Append("\tsc_uint<%s> c_t1_%s_%sHtId%s;\n",
						mod.m_threads.m_htIdW.c_str(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

				pIplTxStg->Append("\tc_t1_%s_%sHtId%s = c_t1_%s_%s%s.m_rtnHtId;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			}

			if (mod.m_bCallFork && mod.m_threads.m_htIdW.AsInt() > 2) {
				pIplTxStg->Append("\tm_%s_%sPrivLk0%s.read_addr(c_t1_%s_%sHtId%s);\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				pIplTxStg->Append("\tm_%s_%sPrivLk1%s.read_addr(c_t1_%s_%sHtId%s);\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			}
		}
	}

	if (mod.m_threads.m_htIdW.AsInt() == 0)
		pIplTxStg->Append("\tCHtCmd c_t1_htCmd = r_htCmd;\n");
	else
		pIplTxStg->Append("\tCHtCmd c_t1_htCmd = m_htCmdQue.front();\n");

	if (mod.m_threads.m_htIdW.AsInt() > 0) {
		pIplTxStg->Append("\tsc_uint<%s_HTID_W> c_t1_htCmdHtId = c_t1_htCmd.m_htId;\n",
			mod.m_modName.Upper().c_str());

		if (mod.m_bCallFork && mod.m_threads.m_htIdW.AsInt() > 2) {
			pIplTxStg->Append("\tm_htCmdPrivLk0.read_addr(c_t1_htCmdHtId);\n");
			pIplTxStg->Append("\tm_htCmdPrivLk1.read_addr(c_t1_htCmdHtId);\n");
		}

		pIplTxStg->Append("\tsc_uint<%s_HTID_W> c_t1_htIdPool = m_htIdPool.front();\n",
			mod.m_modName.Upper().c_str());
		pIplTxStg->Append("\tsc_uint<%s_HTID_W> c_t1_htId = 0;\n",
			mod.m_modName.Upper().c_str());

		if (mod.m_threads.m_htIdW.AsInt() > 2 && mod.m_bCallFork) {
			pIplTxStg->Append("\tbool c_t1_htPrivLkData = false;\n");
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("r_t2_htPrivLkData") );
			iplReg.Append("\tr_t2_htPrivLkData = c_t1_htPrivLkData;\n");
		}
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	if (mod.m_rsmSrcCnt > 0) {

		pIplTxStg->Append("\tbool c_t1_rsmHtRdy = r_t1_rsmHtRdy;\n");
		if (mod.m_clkRate == eClk2x) {
			m_iplRegDecl.Append("\tbool r_t2_rsmHtRdy;\n");
			iplReg.Append("\tr_t2_rsmHtRdy = c_t1_rsmHtRdy;\n");
		}

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			pIplTxStg->Append("\tsc_uint<%s_HTID_W> c_t1_rsmHtId = r_t1_rsmHtId;\n", mod.m_modName.Upper().c_str());
			//if (mod.m_clkRate == eClk2x) {
			//	m_iplRegDecl.Append("\tsc_uint<%s> r_t2_rsmHtId;\n", mod.m_threads.m_htIdW.c_str());
			//	iplReg.Append("\tr_t2_rsmHtId = c_t1_rsmHtId;\n");
			//}
		}

		pIplTxStg->Append("\tsc_uint<%s_INSTR_W> c_t1_rsmHtInstr = r_t1_rsmHtInstr;\n", mod.m_modName.Upper().c_str());
		if (mod.m_clkRate == eClk2x) {
			m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> r_t2_rsmHtInstr;\n", mod.m_modName.Upper().c_str());
			iplReg.Append("\tr_t2_rsmHtInstr = c_t1_rsmHtInstr;\n");
		}

		if (mod.m_bCallFork && mod.m_threads.m_htIdW.AsInt() > 2) {
			pIplTxStg->Append("\tm_rsmPrivLk0.read_addr(c_t1_rsmHtId);\n");
			pIplTxStg->Append("\tm_rsmPrivLk1.read_addr(c_t1_rsmHtId);\n");
		}
	}

	pIplTxStg->NewLine();
	m_iplRegDecl.NewLine();
	iplReg.NewLine();

	char *pFirst = "\t";

	int htSelCnt = 0;
	if (mod.m_threads.m_resetInstr.size() > 0) {

		pIplTxStg->Append("%sif (!r_resetInstrStarted", pFirst);

		if (mod.m_threads.m_htIdW.AsInt() > 0)
			pIplTxStg->Append(" && !m_htIdPool.empty()");
		else
			pIplTxStg->Append(" && !r_htBusy");

		if (bResetCnt) pIplTxStg->Append(" && r_resetComplete");

		pIplTxStg->Append(") {\n");

		m_htSelDef.Append("#define %s_HT_SEL_RESET %d\n",
			mod.m_modName.Upper().c_str(), htSelCnt++ );

		pIplTxStg->Append("\t\tc_t1_htSel = %s_HT_SEL_RESET;\n",
			mod.m_modName.Upper().c_str());

		if (mod.m_threads.m_htIdW.AsInt() > 0)
			pIplTxStg->Append("\t\tc_t1_htId = c_t1_htIdPool;\n");

		pIplTxStg->Append("\t\tc_t1_htValid = true;\n");

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			pIplTxStg->Append("\t\tm_htIdPool.pop();\n");
		} else {
			pIplTxStg->Append("\t\tc_htBusy = true;\n");
		}

		if (mod.m_globalVarList.size() > 0)
			pIplTxStg->Append("\t\tc_htCompQueAvlCnt -= 1;\n");

		pIplTxStg->Append("\t\tc_resetInstrStarted = true;\n");

		pIplTxStg->Append("\t} else ");
		pFirst = "";
	}

	if (mod.m_rsmSrcCnt > 0) {
		pIplTxStg->Append("%sif (r_t1_rsmHtRdy", pFirst);

		if (bResetCnt) pIplTxStg->Append(" && r_resetComplete");

		pIplTxStg->Append(") {\n");

		pIplTxStg->Append("\t\tc_t1_htSel = %s_HT_SEL_SM;\n", mod.m_modName.Upper().c_str());

		if (mod.m_threads.m_htIdW.AsInt() == 0) {

			pIplTxStg->Append("\t\tc_t1_htValid = true;\n");

		} else {
			pIplTxStg->Append("\t\tc_t1_htId = c_t1_rsmHtId;\n");

			if (mod.m_bCallFork) {
				if (mod.m_threads.m_htIdW.AsInt() <= 2)
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

		if (mod.m_globalVarList.size() > 0)
			pIplTxStg->Append("\t\tc_htCompQueAvlCnt -= 1;\n");

		pIplTxStg->Append("\t} else ");
		pFirst = "";
	}

	// first handle calls (they have priority over returns
	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrOut) continue;
		if (cxrIntf.m_cxrType != CxrCall && cxrIntf.m_cxrType != CxrTransfer) continue;

		if (cxrIntf.GetQueDepthW() > 0) {
			if (cxrIntf.m_bCxrIntfFields) {
				if (mod.m_clkRate == eClk1x && cxrIntf.m_pSrcMod->m_clkRate == eClk1x)
					pIplTxStg->Append("%sif (!m_%s_%sQue%s.empty()", pFirst,
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				else
					pIplTxStg->Append("%sif (!r_%s_%sQueEmpty%s", pFirst,
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			} else
				pIplTxStg->Append("%sif (r_%s_%sCnt%s > 0", pFirst,
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
		} else
			pIplTxStg->Append("%sif (r_t1_%s_%sRdy%s", pFirst,
			cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

		if (cxrIntf.m_cxrType == CxrCall || cxrIntf.m_cxrType == CxrTransfer) {
			if (cxrIntf.m_pDstGroup->m_htIdW.AsInt() > 0) {
				if (mod.m_cxrEntryReserveCnt > 0) {

					for (size_t intfIdx2 = 0; intfIdx2 < modInst.m_cxrIntfList.size(); intfIdx2 += 1) {
						CCxrIntf &cxrIntf2 = modInst.m_cxrIntfList[intfIdx2];

						if (cxrIntf2.m_cxrDir == CxrOut) continue;
						if (cxrIntf2.m_cxrType != CxrCall && cxrIntf2.m_cxrType != CxrTransfer) continue;
						if (cxrIntf2.m_reserveCnt == 0) continue;
						if (intfIdx2 == intfIdx) continue;

						pIplTxStg->Append(" && !r_%s_%sRsvLimitEmpty%s",
							cxrIntf2.GetPortNameSrcToDstLc(), cxrIntf2.GetIntfName(), cxrIntf.GetPortReplIndex());
					}
				} else
					pIplTxStg->Append(" && !m_htIdPool.empty()");
			} else
				pIplTxStg->Append(" && !r_htBusy");
		}

		if (bResetCnt) pIplTxStg->Append(" && r_resetComplete");

		pIplTxStg->Append(") {\n");

		m_htSelDef.Append("#define %s_HT_SEL_%s_%d_%s %d\n",
			mod.m_modName.Upper().c_str(), cxrIntf.m_srcInstName.Upper().c_str(), cxrIntf.GetPortReplId(), cxrIntf.m_funcName.Upper().c_str(),
			htSelCnt++ );
		pIplTxStg->Append("\t\tc_t1_htSel = %s_HT_SEL_%s_%d_%s;\n",
			mod.m_modName.Upper().c_str(), cxrIntf.m_srcInstName.Upper().c_str(), cxrIntf.GetPortReplId(), cxrIntf.m_funcName.Upper().c_str());

		if (cxrIntf.m_pDstGroup->m_htIdW.AsInt() > 0) {
			if (cxrIntf.m_cxrType == CxrCall || cxrIntf.m_cxrType == CxrTransfer) {
				pIplTxStg->Append("\t\tc_t1_htId = c_t1_htIdPool;\n");
			} else {
				pIplTxStg->Append("\t\tc_t1_htId = c_t1_%s_%s%s.m_rtnHtId;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			}
		}

		pIplTxStg->Append("\t\tc_t1_htValid = true;\n");

		if (cxrIntf.GetQueDepthW() == 0)
			pIplTxStg->Append("\t\tc_t0_%s_%sRdy%s = false;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
		else {
			if (cxrIntf.m_bCxrIntfFields) {
				if (mod.m_clkRate != eClk1x || cxrIntf.m_pSrcMod->m_clkRate != eClk1x)
					pIplTxStg->Append("\t\tc_%s_%sQueEmpty%s = true;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				else
					pIplTxStg->Append("\t\tm_%s_%sQue%s.pop();\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			} else
				pIplTxStg->Append("\t\tc_%s_%sCnt%s -= 1u;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
		}

		pIplTxStg->Append("\t\tc_%s_%sAvl%s = true;\n",
			cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

		if (mod.m_threads.m_htIdW.AsInt() == 0 && mod.m_bCallFork)
			pIplTxStg->Append("\t\tc_htPrivLk = true;\n");

		if (cxrIntf.m_cxrType == CxrCall || cxrIntf.m_cxrType == CxrTransfer) {
			if (cxrIntf.m_pDstGroup->m_htIdW.AsInt() > 0) {
				pIplTxStg->Append("\t\tm_htIdPool.pop();\n");

				for (size_t intfIdx2 = 0; intfIdx2 < modInst.m_cxrIntfList.size(); intfIdx2 += 1) {
					CCxrIntf &cxrIntf2 = modInst.m_cxrIntfList[intfIdx2];

					if (cxrIntf2.m_cxrDir == CxrOut) continue;
					if (cxrIntf2.m_cxrType != CxrCall && cxrIntf2.m_cxrType != CxrTransfer) continue;
					if (cxrIntf2.m_reserveCnt == 0) continue;
					if (intfIdx2 == intfIdx) continue;

					pIplTxStg->Append("\t\tc_%s_%sRsvLimit%s -= 1;\n",
						cxrIntf2.GetPortNameSrcToDstLc(), cxrIntf2.GetIntfName(), cxrIntf.GetPortReplIndex());
				}

			} else {
				pIplTxStg->Append("\t\tc_htBusy = true;\n");
			}
		}

		if (cxrIntf.m_pSrcMod->m_globalVarList.size() > 0) {
			pIplTxStg->Append("\t\tc_%sCompCnt -= 1;\n", cxrIntf.GetIntfName());
		}

		if (mod.m_globalVarList.size() > 0)
			pIplTxStg->Append("\t\tc_htCompQueAvlCnt -= 1;\n");

		pIplTxStg->Append("\t}");
		pFirst = " else ";
	}

	// now handle inbound returns (they get round robin arbitration)
	unsigned selRrIdx = 1;
	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrOut) continue;
		if (cxrIntf.m_cxrType == CxrCall || cxrIntf.m_cxrType == CxrTransfer) continue;

		pIplTxStg->Append("%sif (c_%sSel%s", pFirst,
			htSelNameList[selRrIdx].m_name.c_str(), htSelNameList[selRrIdx].m_replIndex.c_str());

		if (bResetCnt) pIplTxStg->Append(" && r_resetComplete");

		pIplTxStg->Append(") {\n");

		m_htSelDef.Append("#define %s_HT_SEL_%s_%d_%s %d\n",
			mod.m_modName.Upper().c_str(), cxrIntf.m_srcInstName.Upper().c_str(), cxrIntf.GetPortReplId(), cxrIntf.m_funcName.Upper().c_str(),
			htSelCnt++ );
		pIplTxStg->Append("\t\tc_t1_htSel = %s_HT_SEL_%s_%d_%s;\n",
			mod.m_modName.Upper().c_str(), cxrIntf.m_srcInstName.Upper().c_str(), cxrIntf.GetPortReplId(), cxrIntf.m_funcName.Upper().c_str());

		if (cxrIntf.m_pDstGroup->m_htIdW.AsInt() > 0)
			pIplTxStg->Append("\t\tc_t1_htId = c_t1_%s_%sHtId%s;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

		if (!mod.m_bCallFork) {
			pIplTxStg->Append("\t\tc_t1_htValid = true;\n");

			if (cxrIntf.GetQueDepthW() == 0)
				pIplTxStg->Append("\t\tc_t0_%s_%sRdy%s = false;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			else {
				if (cxrIntf.m_bCxrIntfFields) {
					if (mod.m_clkRate != eClk1x || cxrIntf.m_pSrcMod->m_clkRate != eClk1x)
						pIplTxStg->Append("\t\tc_%s_%sQueEmpty%s = true;\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					else
						pIplTxStg->Append("\t\tm_%s_%sQue%s.pop();\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				} else
					pIplTxStg->Append("\t\tc_%s_%sCnt%s -= 1u;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			}

			pIplTxStg->Append("\t\tc_%s_%sAvl%s = true;\n",
				cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

			if (selRrIdx == htSelNameList.size()-1)
				pIplTxStg->Append("\t\tc_htSelRr = 0x1;\n");
			else
				pIplTxStg->Append("\t\tc_htSelRr = 0x%x;\n", 1 << (selRrIdx+1));

		} else {

			if (mod.m_threads.m_htIdW.AsInt() == 0) {
				pIplTxStg->Append("\t\tc_t1_htValid = true;\n");

				if (cxrIntf.GetQueDepthW() == 0)
					pIplTxStg->Append("\t\tc_t0_%s_%sRdy%s = false;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				else {
					if (cxrIntf.m_bCxrIntfFields) {
						if (mod.m_clkRate != eClk1x || cxrIntf.m_pSrcMod->m_clkRate != eClk1x)
							pIplTxStg->Append("\t\tc_%s_%sQueEmpty%s = true;\n",
								cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
						else
							pIplTxStg->Append("\t\tm_%s_%sQue%s.pop();\n",
								cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					} else
						pIplTxStg->Append("\t\tc_%s_%sCnt%s -= 1u;\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}

				pIplTxStg->Append("\t\tc_%s_%sAvl%s = true;\n",
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				if (selRrIdx == htSelNameList.size()-1)
					pIplTxStg->Append("\t\tc_htSelRr = 0x1;\n");
				else
					pIplTxStg->Append("\t\tc_htSelRr = 0x%x;\n", 1 << (selRrIdx+1));

			} else {
				if (mod.m_threads.m_htIdW.AsInt() <= 2)
					pIplTxStg->Append("\t\tc_t1_htValid = !r_htPrivLk[INT(c_t1_htId)];\n");
				else {
					pIplTxStg->Append("\t\tc_t1_htValid = m_%s_%sPrivLk0%s.read_mem() == m_%s_%sPrivLk1%s.read_mem();\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					pIplTxStg->Append("\t\tc_t1_htPrivLkData = !m_%s_%sPrivLk0%s.read_mem();\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					pIplTxStg->Append("\n");
				}

				pIplTxStg->Append("\t\tif (c_t1_htValid) {\n");

				if (cxrIntf.GetQueDepthW() == 0)
					pIplTxStg->Append("\t\t\tc_t0_%s_%sRdy%s = false;\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				else {
					if (cxrIntf.m_bCxrIntfFields) {
						if (mod.m_clkRate != eClk1x || cxrIntf.m_pSrcMod->m_clkRate != eClk1x)
							pIplTxStg->Append("\t\t\tc_%s_%sQueEmpty%s = true;\n",
								cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
						else
							pIplTxStg->Append("\t\t\tm_%s_%sQue%s.pop();\n",
								cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					} else
						pIplTxStg->Append("\t\t\tc_%s_%sCnt%s -= 1u;\n",
							cxrIntf.GetSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}

				pIplTxStg->Append("\t\t\tc_%s_%sAvl%s = true;\n",
					cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				if (selRrIdx == htSelNameList.size()-1)
					pIplTxStg->Append("\t\t\tc_htSelRr = 0x1;\n");
				else
					pIplTxStg->Append("\t\t\tc_htSelRr = 0x%x;\n", 1 << (selRrIdx+1));

				pIplTxStg->Append("\t\t}\n");
			}
		}

		if (mod.m_threads.m_htIdW.AsInt() == 0 && mod.m_bCallFork)
			pIplTxStg->Append("\t\tc_htPrivLk = true;\n");

		if (cxrIntf.m_pSrcMod->m_globalVarList.size() > 0) {
			pIplTxStg->Append("\t\tc_%sCompCnt -= 1;\n", cxrIntf.GetIntfName());
		}

		if (mod.m_globalVarList.size() > 0)
			pIplTxStg->Append("\t\tc_htCompQueAvlCnt -= 1;\n");

		pIplTxStg->Append("\t}");
		pFirst = " else ";

		selRrIdx += 1;
	}

	pIplTxStg->Append(" else if (c_htCmdSel");

	if (bResetCnt) pIplTxStg->Append(" && r_resetComplete");

	pIplTxStg->Append(") {\n");

	if (modInst.m_cxrSrcCnt > 1) {

		m_htSelDef.Append("#define %s_HT_SEL_SM %d\n",
			mod.m_modName.Upper().c_str(), htSelCnt++ );
		pIplTxStg->Append("\t\tc_t1_htSel = %s_HT_SEL_SM;\n",
			mod.m_modName.Upper().c_str());
	}

	if (!mod.m_bCallFork || mod.m_threads.m_htIdW.AsInt() == 0) {
		pIplTxStg->Append("\t\tc_t1_htValid = true;\n");

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			pIplTxStg->Append("\t\tc_t1_htId = c_t1_htCmdHtId;\n");
			pIplTxStg->Append("\t\tm_htCmdQue.pop();\n");
			if (mod.m_globalVarList.size() > 0)
				pIplTxStg->Append("\t\tc_htCmdRdyCnt -= 1u;\n");
		} else {
			pIplTxStg->Append("\t\tc_htCmdValid = false;\n");
			if (mod.m_threads.m_bCallFork)
				pIplTxStg->Append("\t\tc_htPrivLk = true;\n");
		}

		if (selRrCnt > 1)
			pIplTxStg->Append("\t\tc_htSelRr = 0x2;\n");

	} else {
		pIplTxStg->Append("\t\tc_t1_htId = c_t1_htCmdHtId;\n");

		if (mod.m_threads.m_htIdW.AsInt() <= 2)
			pIplTxStg->Append("\t\tc_t1_htValid = !r_htPrivLk[INT(c_t1_htCmdHtId)];\n");
		else {
			pIplTxStg->Append("\t\tc_t1_htValid = m_htCmdPrivLk0.read_mem() == m_htCmdPrivLk1.read_mem();\n");
			pIplTxStg->Append("\t\tc_t1_htPrivLkData = !m_htCmdPrivLk0.read_mem();\n");
			pIplTxStg->Append("\n");
		}

		pIplTxStg->Append("\t\tif (c_t1_htValid) {\n");
		pIplTxStg->Append("\t\t\tm_htCmdQue.pop();\n");

		if (selRrCnt > 1) 
			pIplTxStg->Append("\t\t\tc_htSelRr = 0x2;\n");
		pIplTxStg->Append("\t\t}\n");
		pIplTxStg->Append("\n");
	}

	if (mod.m_globalVarList.size() > 0)
		pIplTxStg->Append("\t\tc_htCompQueAvlCnt -= 1;\n");

	pIplTxStg->Append("\t}");
	pFirst = " else ";

	HtlAssert(rsvSelCnt == htSelCnt);

	pIplTxStg->Append("\n");

	char inStgStr[8];
	char outStgStr[8];
	char regStgStr[8];

	const char *pInStg = inStgStr;
	const char *pOutStg = outStgStr;
	const char *pRegStg = regStgStr;

	int iplStg = 1;

	char htSelTypeStr[128];
	sprintf(htSelTypeStr, "ht_uint%d", FindLg2(modInst.m_cxrSrcCnt-1));

	if (mod.m_clkRate == eClk2x) {

		sprintf(inStgStr, "r_t%d", iplStg);
		sprintf(outStgStr, "c_t%d", iplStg);
		sprintf(regStgStr, "r_t%d", iplStg+1);

		GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelTypeStr, VA("%s_htSel", pRegStg) );
		iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg) );
			iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
		}

		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrOut) continue;
			if (!cxrIntf.m_bCxrIntfFields) continue;

			if (cxrIntf.GetPortReplCnt() <= 1 || cxrIntf.GetPortReplId() == 0)
				m_iplRegDecl.Append("\tC%s_%s %s_%s_%s%s;\n",
					cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
					pRegStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

			iplReg.Append("\t%s_%s_%s%s = %s_%s_%s%s;\n",
				pRegStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
				pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
		}

		GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg) );
		iplReg.Append("\t%s_htValid = !c_reset1x && %s_htValid;\n",
			pRegStg, pOutStg);

		m_iplRegDecl.Append("\tCHtCmd %s_htCmd;\n",
			pRegStg);
		iplReg.Append("\t%s_htCmd = %s_htCmd;\n",
			pRegStg, pOutStg);

		//if (mod.m_threads.m_htIdW.AsInt() > 2 && bIntfCallForkPrivWr) {
		//	pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n",
		//		pRegStg, pOutStg);
		//	//GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("r_t2_htPrivLkData") );
		//	//iplReg.Append("\tr_t2_htPrivLkData = c_t1_htPrivLkData;\n");
		//}

		//if (mod.m_threads.m_htIdW.AsInt() > 0) {
		//	m_iplRegDecl.Append("\tsc_uint<%s> %s_htIdPool;\n",
		//		mod.m_threads.m_htIdW.c_str(), pRegStg);
		//	iplReg.Append("\t%s_htIdPool = %s_htIdPool;\n",
		//		pRegStg, pOutStg);
		//}

		iplStg += 1;
	}

	pIplTxStg->NewLine();

	///////////////////////////////////////////////////////////////////////
	// Second stage: ht Id select, input is registered if 2x clock

	//pIplTxStg->Append("\t// htId select\n");

	sprintf(inStgStr, "r_t%d", iplStg);
	sprintf(outStgStr, "c_t%d", iplStg);
	sprintf(regStgStr, "r_t%d", iplStg+1);

	if (bMultiThread) {

		if (mod.m_clkRate == eClk2x)
			pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n", pOutStg, pInStg);
		GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg));
		iplReg.Append("\t%s_htValid = !c_reset1x && %s_htValid;\n", pRegStg, pOutStg);

		if (modInst.m_cxrSrcCnt > 1) {
			if (mod.m_clkRate == eClk2x)
				pIplTxStg->Append("\tht_uint%d %s_htSel = %s_htSel;\n", FindLg2(modInst.m_cxrSrcCnt-1), pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("ht_uint%d", FindLg2(modInst.m_cxrSrcCnt-1)), VA("%s_htSel", pRegStg));
			iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
		}

		if (mod.m_clkRate == eClk2x)
			pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);
		GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("sc_uint<%s_HTID_W>", mod.m_modName.Upper().c_str()), VA("%s_htId", pRegStg));
		iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);

		// generate cmds
		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrOut) continue;
			if (!cxrIntf.m_bCxrIntfFields) continue;

			if (mod.m_clkRate == eClk2x) {
				if (cxrIntf.GetPortReplCnt() <= 1)
					pIplTxStg->Append("\tC%s_%s %s_%s_%s = %s_%s_%s;\n", 
						cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
						pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(),
						pInStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
				else {
					if (cxrIntf.GetPortReplId() == 0)
						pIplTxStg->Append("\tC%s_%s %s_%s_%s%s;\n", 
							cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
							pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

					pIplTxStg->Append("\t%s_%s_%s%s = %s_%s_%s%s;\n", 
						cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
						pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						pInStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
			}

			if (cxrIntf.GetPortReplCnt() <= 1 || cxrIntf.GetPortReplId() == 0)
				m_iplRegDecl.Append("\tC%s_%s %s_%s_%s%s;\n",
					cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
					pRegStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

			iplReg.Append("\t%s_%s_%s%s = %s_%s_%s%s;\n",
				pRegStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
				pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
		}

		if (mod.m_clkRate == eClk2x)
			pIplTxStg->Append("\tCHtCmd %s_htCmd = %s_htCmd;\n", pOutStg, pInStg);

		m_iplRegDecl.Append("\tCHtCmd %s_htCmd;\n", pRegStg);
		iplReg.Append("\t%s_htCmd = %s_htCmd;\n", pRegStg, pOutStg);

		if (mod.m_threads.m_htIdW.AsInt() > 2 && mod.m_bCallFork) {
			if (mod.m_clkRate == eClk2x) {
				pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n",
					pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg) );
				iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
			}
		}


		//if (mod.m_clkRate == eClk2x && mod.m_bMultiThread) {
		//	if (mod.m_threads.m_htIdW.AsInt() > 0)
		//		pIplTxStg->Append("\tsc_uint<%s> %s_htIdPool = %s_htIdPool;\n",
		//			mod.m_threads.m_htIdW.c_str(), pOutStg, pInStg);
		//}

		IplNewLine(pIplTxStg);

		if (mod.m_rsmSrcCnt > 0) {

			if (mod.m_clkRate == eClk2x || !mod.m_bMultiThread)
				pIplTxStg->Append("\tbool %s_rsmHtRdy = %s_rsmHtRdy;\n", pOutStg, pInStg);

			//if (mod.m_clkRate == eClk2x && mod.m_bMultiThread)
			//	pIplTxStg->Append("\tsc_uint<%s> %s_rsmHtId = %s_rsmHtId;\n",
			//	mod.m_threads.m_htIdW.c_str(), pOutStg, pInStg);

			if (mod.m_bMultiThread) {
				m_iplRegDecl.Append("\tbool %s_rsmHtRdy;\n", pRegStg);
				iplReg.Append("\t%s_rsmHtRdy = %s_rsmHtRdy;\n", pRegStg, pOutStg);
			}

			if (mod.m_clkRate == eClk2x || !mod.m_bMultiThread)
				pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_rsmHtInstr = %s_rsmHtInstr;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);

			if (mod.m_bMultiThread) {
				m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> %s_rsmHtInstr;\n", mod.m_modName.Upper().c_str(), pRegStg);
				iplReg.Append("\t%s_rsmHtInstr = %s_rsmHtInstr;\n", pRegStg, pOutStg);
			}
		}

		IplNewLine(pIplTxStg);

		if (mod.m_bCallFork) {
			if (mod.m_threads.m_htIdW.AsInt() <= 2) {
				pIplTxStg->Append("\tht_uint%d c_htPrivLk = r_htPrivLk;\n", 
					1 << mod.m_threads.m_htIdW.AsInt());
				pIplTxStg->NewLine();

				pIplTxStg->Append("\tif (c_t1_htValid)\n");
				pIplTxStg->Append("\t\tc_htPrivLk[INT(c_t1_htId)] = true;\n");
				pIplTxStg->NewLine();
			} else {
				pIplTxStg->Append("\tif (c_t1_htValid) {\n");
				for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
					CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

					if (cxrIntf.m_cxrDir == CxrOut) continue;
					if (cxrIntf.m_cxrType == CxrCall || cxrIntf.m_cxrType == CxrTransfer) continue;

					pIplTxStg->Append("\t\tm_%s_%sPrivLk0%s.write_addr(c_t1_htId);\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					pIplTxStg->Append("\t\tm_%s_%sPrivLk0%s.write_mem(c_t1_htPrivLkData);\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
				pIplTxStg->Append("\t\tm_htCmdPrivLk0.write_addr(c_t1_htId);\n");
				pIplTxStg->Append("\t\tm_htCmdPrivLk0.write_mem(c_t1_htPrivLkData);\n");

				if (mod.m_rsmSrcCnt > 0) {
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

	if (mod.m_bMultiThread) {
		iplStg += 1;

		sprintf(inStgStr, "r_t%d", iplStg);
		sprintf(outStgStr, "c_t%d", iplStg);
		sprintf(regStgStr, "r_t%d", iplStg+1);

		pIplTxStg = &iplT2Stg;

		pIplTxStg->Append("\t// htPriv access\n");

		pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n",
			pOutStg, pInStg);
		if (mod.m_clkRate == eClk2x) {
			m_iplRegDecl.Append("\tbool %s_htValid;\n", 
				pRegStg);
			iplReg.Append("\t%s_htValid = !c_reset1x && %s_htValid;\n",
				pRegStg, pOutStg);
		}

		if (modInst.m_cxrSrcCnt > 1) {
			pIplTxStg->Append("\tht_uint%d %s_htSel = %s_htSel;\n", FindLg2(modInst.m_cxrSrcCnt-1), pOutStg, pInStg);
			if (mod.m_clkRate == eClk2x) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelTypeStr, VA("%s_htSel", pRegStg) );
				iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
			}
		}

		if (bMultiThread) {
			if (mod.m_bHtIdInit && iplStg == mod.m_execStg-1) {
				pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htIdInit ? (sc_uint<%s_HTID_W>)(%s_htId+1) : %s_htId;\n",
					mod.m_modName.Upper().c_str(), pOutStg, pRegStg, mod.m_modName.Upper().c_str(), pRegStg, pInStg);
				pIplTxStg->Append("\tbool %s_htIdInit = %s_htIdInit && %s_htId != 0x%x;\n",
					pOutStg, pRegStg, pRegStg, (1 << mod.m_threads.m_htIdW.AsInt())-1);
			} else
				pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);

			if (mod.m_clkRate == eClk2x) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg) );
				iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
			}
		}

		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrOut) continue;
			if (!cxrIntf.m_bCxrIntfFields) continue;

			if (mod.m_clkRate == eClk2x) {
				if (cxrIntf.GetPortReplId() == 0)
					m_iplRegDecl.Append("\tC%s_%s %s_%s_%s%s;\n",
						cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
						pRegStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

				iplReg.Append("\t%s_%s_%s%s = %s_%s_%s%s;\n",
					pRegStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			}

			if (cxrIntf.GetPortReplCnt() <= 1)
				pIplTxStg->Append("\tC%s_%s %s_%s_%s = %s_%s_%s;\n",
					cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
					pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(),
					pInStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
			else {
				if (cxrIntf.GetPortReplId() == 0)
					pIplTxStg->Append("\tC%s_%s %s_%s_%s%s;\n",
						cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
						pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

				pIplTxStg->Append("\t%s_%s_%s%s = %s_%s_%s%s;\n",
					pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					pInStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			}
		}

		if (mod.m_clkRate == eClk2x) {
			m_iplRegDecl.Append("\tCHtCmd %s_htCmd;\n",
				pRegStg);

			iplReg.Append("\t%s_htCmd = %s_htCmd;\n",
				pRegStg, pOutStg);
		}

		pIplTxStg->Append("\tCHtCmd %s_htCmd = %s_htCmd;\n",
			pOutStg, 
			pInStg);

		if (mod.m_clkRate == eClk2x) {
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("CHtPriv"), VA("%s_htPriv", pRegStg) );
			iplReg.Append("\t%s_htPriv = %s_htPriv;\n",
				pRegStg, pOutStg);

			if (mod.m_threads.m_htIdW.AsInt() > 2 && mod.m_bCallFork) {
				pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg) );
				iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
			}
		}

		IplNewLine(pIplTxStg);

		if (mod.m_threads.m_ramType == eDistRam)
			pIplTxStg->Append("\tm_htPriv.read_addr(%s_htId);\n", pInStg);
		else {
			string prevOutStg = pOutStg;
			prevOutStg[prevOutStg.size()-1] -= 1;
			pIplTxStg->Append("\tm_htPriv.read_addr(%s_htId);\n",
				prevOutStg.c_str());
		}
		pIplTxStg->Append("\tCHtPriv %s_htPriv = m_htPriv.read_mem();\n",
			pOutStg);

		pIplTxStg->Append("\n");
	}

	IplNewLine(pIplTxStg);

	if (mod.m_rsmSrcCnt > 0) {

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			m_iplRegDecl.Append("#ifndef _HTV\n");
			m_iplRegDecl.Append("\tht_dist_ram<bool, %s_HTID_W> m_htRsmWait;\n", mod.m_modName.Upper().c_str());
			m_iplRegDecl.Append("#endif\n");

			iplReg.Append("#ifndef _HTV\n");
			iplReg.Append("\tm_htRsmWait.clock();\n");
			iplReg.Append("#endif\n");

			m_iplCtorInit.Append("\t\tfor (int i = 0; i < (1 << %s_HTID_W); i += 1)\n", mod.m_modName.Upper().c_str());
			m_iplCtorInit.Append("\t\t\tm_htRsmWait.write_mem_debug(i) = false;\n");
		} else {
			m_iplRegDecl.Append("#ifndef _HTV\n");
			m_iplRegDecl.Append("\tbool c_htRsmWait;\n");
			m_iplRegDecl.Append("\tbool r_htRsmWait;\n", mod.m_modName.Upper().c_str());
			m_iplRegDecl.Append("#endif\n");

			iplPostInstr.Append("#ifndef _HTV\n");
			iplPostInstr.Append("\tc_htRsmWait = r_htRsmWait;\n");
			iplPostInstr.Append("#endif\n");

			iplReg.Append("#ifndef _HTV\n");
			iplReg.Append("\tr_htRsmWait = c_htRsmWait;\n");
			iplReg.Append("#endif\n");

			m_iplCtorInit.Append("\t\tr_htRsmWait = false;\n");
		}

		if (mod.m_threads.m_bPause) {
			if (mod.m_threads.m_htIdW.AsInt() > 0 && mod.m_rsmSrcCnt > 1) {
				m_iplRegDecl.Append("\tht_dist_que<CHtRsm, %s_HTID_W> m_htRsmQue;\n", mod.m_modName.Upper().c_str());
				iplReg.Append("\tm_htRsmQue.clock(c_reset1x);\n");
			}

			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				m_iplRegDecl.Append("\tht_dist_ram<sc_uint<%s_INSTR_W>, %s_HTID_W> m_htRsmInstr;\n",
					mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());
				iplReg.Append("\tm_htRsmInstr.clock();\n");
			} else {
				m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> c_htRsmInstr;\n", mod.m_modName.Upper().c_str());
				iplPostInstr.Append("\tc_htRsmInstr = r_htRsmInstr;\n");
				m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> r_htRsmInstr;\n", mod.m_modName.Upper().c_str());
				iplReg.Append("\tr_htRsmInstr = c_htRsmInstr;\n");
			}
		}

		m_iplRegDecl.Append("\tbool c_t0_rsmHtRdy;\n");
		iplT0Stg.Append("\tc_t0_rsmHtRdy = false;\n");
		m_iplRegDecl.Append("\tbool r_t1_rsmHtRdy;\n");
		iplReg.Append("\tr_t1_rsmHtRdy = c_t0_rsmHtRdy;\n");

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			m_iplRegDecl.Append("\tsc_uint<%s_HTID_W> c_t0_rsmHtId;\n", mod.m_modName.Upper().c_str());
			iplT0Stg.Append("\tc_t0_rsmHtId = r_t1_rsmHtId;\n");
			m_iplRegDecl.Append("\tsc_uint<%s_HTID_W> r_t1_rsmHtId;\n", mod.m_modName.Upper().c_str());
			iplReg.Append("\tr_t1_rsmHtId = c_t0_rsmHtId;\n");
		}

		m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> c_t0_rsmHtInstr;\n", mod.m_modName.Upper().c_str());
		iplT0Stg.Append("\tc_t0_rsmHtInstr = r_t1_rsmHtInstr;\n");
		m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> r_t1_rsmHtInstr;\n", mod.m_modName.Upper().c_str());
		iplReg.Append("\tr_t1_rsmHtInstr = c_t0_rsmHtInstr;\n");

		if (mod.m_threads.m_bPause) {
			if (mod.m_threads.m_htIdW.AsInt() > 0 && mod.m_rsmSrcCnt > 1) {
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

		if (mod.m_bMultiThread)
			pIplTxStg->Append("\tbool %s_rsmHtRdy = %s_rsmHtRdy;\n", pOutStg, pInStg);

		if (mod.m_clkRate == eClk2x && mod.m_bMultiThread) {
			m_iplRegDecl.Append("\tbool %s_rsmHtRdy;\n", pRegStg);
			iplReg.Append("\t%s_rsmHtRdy = %s_rsmHtRdy;\n", pRegStg, pOutStg);
		}

		if (mod.m_bMultiThread)
			pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_rsmHtInstr = %s_rsmHtInstr;\n",
			mod.m_modName.Upper().c_str(), pOutStg, pInStg);

		if (mod.m_clkRate == eClk2x && mod.m_bMultiThread) {
			m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> %s_rsmHtInstr;\n", mod.m_modName.Upper().c_str(), pRegStg);
			iplReg.Append("\t%s_rsmHtInstr = %s_rsmHtInstr;\n", pRegStg, pOutStg);
		}
	}

	IplNewLine(pIplTxStg);

	////////////////////////////////////////////////////////////////////////////
	// Forth stage: htPriv / input command select

	int rdPrivLkStg;
	{
		if (mod.m_clkRate == eClk2x && bMultiThread)
			iplStg += 1;

		rdPrivLkStg = iplStg;

		sprintf(inStgStr, "r_t%d", iplStg);
		sprintf(outStgStr, "c_t%d", iplStg);
		sprintf(regStgStr, "r_t%d", iplStg+1);

		pIplTxStg = mod.m_bMultiThread ? &iplT2Stg : &iplT1Stg;

		pIplTxStg->Append("\t// htPriv / input command select\n");

		if (mod.m_clkRate == eClk2x)
			pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n", 
			pOutStg, pInStg);
		GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg) );
		iplReg.Append("\t%s_htValid = !c_reset1x && %s_htValid;\n",
			pRegStg, pOutStg);

		if (modInst.m_cxrSrcCnt > 1) {
			if (mod.m_clkRate == eClk2x)
				pIplTxStg->Append("\tht_uint%d %s_htSel = %s_htSel;\n", FindLg2(modInst.m_cxrSrcCnt-1), pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelTypeStr, VA("%s_htSel", pRegStg) );
			iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
		}

		if (bMultiThread) {

			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg) );

			if (mod.m_bHtIdInit && iplStg == mod.m_execStg-1) {
				m_iplRegDecl.Append("\tbool %s_htIdInit;\n", pRegStg);

				if (mod.m_clkRate == eClk2x) {
					pIplTxStg->Append("\tbool %s_htIdInit = %s_htIdInit && %s_htId != 0x%x;\n",
						pOutStg, pRegStg, pRegStg, (1 << mod.m_threads.m_htIdW.AsInt())-1);

					pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htIdInit ? (sc_uint<%s_HTID_W>)(%s_htId+1) : %s_htId;\n",
						mod.m_modName.Upper().c_str(), pOutStg, pRegStg, mod.m_modName.Upper().c_str(), pRegStg, pInStg);
				}

				iplReg.Append("\t%s_htId = c_reset1x ? (sc_uint<%s>)0 : %s_htId;\n", pRegStg, mod.m_threads.m_htIdW.c_str(), pOutStg);

				iplReg.Append("\t%s_htIdInit = c_reset1x || %s_htIdInit;\n", pRegStg, pOutStg);
			} else {
				if (mod.m_clkRate == eClk2x)
					pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);
				iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
			}
		}

		if (mod.m_threads.m_htIdW.AsInt() == 0)
			pIplTxStg->Append("\tCHtPriv %s_htPriv = r_htPriv;\n", pOutStg);
		else if (mod.m_clkRate == eClk2x)
			pIplTxStg->Append("\tCHtPriv %s_htPriv = %s_htPriv;\n", pOutStg, pInStg);

		GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("CHtPriv"), VA("%s_htPriv", pRegStg) );
		iplReg.Append("\t%s_htPriv = %s_htPriv;\n", pRegStg, pOutStg);

		if (mod.m_threads.m_htIdW.AsInt() > 2 && mod.m_bCallFork) {
			pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg) );
			iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
		}

		pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_htInstr;\n", mod.m_modName.Upper().c_str(), pOutStg);
		GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htInstrTypeStr, VA("%s_htInstr", pRegStg) );
		iplReg.Append("\t%s_htInstr = %s_htInstr;\n", pRegStg, pOutStg);
		pIplTxStg->Append("\n");

		if (mod.m_bCallFork) {
			pIplTxStg->Append("\tbool %s_rtnJoin = false;\n", pOutStg);
			m_iplRegDecl.Append("\tbool ht_noload %s_rtnJoin;\n", pRegStg);
			iplReg.Append("\t%s_rtnJoin = %s_rtnJoin;\n", pRegStg, pOutStg);
		}

		if (mod.m_clkRate == eClk2x) {
			for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

				if (cxrIntf.m_cxrDir == CxrOut) continue;
				if (!cxrIntf.m_bCxrIntfFields) continue;

				if (cxrIntf.GetPortReplCnt() <= 1)
					pIplTxStg->Append("\tC%s_%s %s_%s_%s = %s_%s_%s;\n",
						cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
						pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(),
						pInStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
				else {
					if (cxrIntf.GetPortReplId() == 0)
						pIplTxStg->Append("\tC%s_%s %s_%s_%s%s;\n",
							cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName(),
							pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());

					pIplTxStg->Append("\t%s_%s_%s%s = %s_%s_%s%s;\n",
						pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						pInStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
			}

			pIplTxStg->Append("\tCHtCmd %s_htCmd = %s_htCmd;\n", pOutStg,  pInStg);

			if (mod.m_rsmSrcCnt > 0) {
				pIplTxStg->Append("\tbool %s_rsmHtRdy = %s_rsmHtRdy;\n", pOutStg, pInStg);

				pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_rsmHtInstr = %s_rsmHtInstr;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);
			}
		}

		pIplTxStg->Append("\n");

		pIplTxStg->Append("\tswitch (%s_htSel) {\n", pOutStg);

		if (mod.m_threads.m_resetInstr.size() > 0) {
			pIplTxStg->Append("\tcase %s_HT_SEL_RESET:\n", mod.m_modName.Upper().c_str());
			pIplTxStg->Append("\t\t%s_htInstr = %s;\n", pOutStg, mod.m_threads.m_resetInstr.c_str());
			pIplTxStg->Append("\t\tbreak;\n");
		}

		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrOut) continue;

			pIplTxStg->Append("\tcase %s_HT_SEL_%s_%d_%s:\n",
				mod.m_modName.Upper().c_str(), cxrIntf.m_srcInstName.Upper().c_str(), cxrIntf.GetPortReplId(), cxrIntf.m_funcName.Upper().c_str());

			if (cxrIntf.IsCallOrXfer()) {
				pIplTxStg->Append("\t\t%s_htInstr = %s;\n",
					pOutStg, cxrIntf.m_entryInstr.c_str());

				// clear priv state on call entry
				pIplTxStg->Append("\t\t%s_htPriv = 0;\n",
					pOutStg);

				if (cxrIntf.m_pDstGroup->m_rtnSelW > 0) {
					if (cxrIntf.IsCall())
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnSel = %d;\n",
						pOutStg,
						cxrIntf.m_rtnSelId);
					else
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnSel = %s_%s_%s%s.m_rtnSel;\n",
						pOutStg,
						pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}

				// call and xfer must save caller's htId, returns do not
				if (cxrIntf.IsCall()) {
					if (cxrIntf.m_pSrcMod->m_instrW > 0)
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnInstr = (%sRtnInstr_t)%s_%s_%s%s.m_rtnInstr;\n",
							pOutStg,
							mod.m_modName.Uc().c_str(), 
							pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

					if (cxrIntf.m_pSrcGroup->m_htIdW.AsInt() > 0)
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnHtId = (%sRtnHtId_t)%s_%s_%s%s.m_rtnHtId;\n",
							pOutStg,
							mod.m_modName.Uc().c_str(), 
							pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					if (mod.m_cxrEntryReserveCnt > 0)
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnRsvSel = %s_HT_SEL_%s_%d_%s;\n",
							pOutStg,
							mod.m_modName.Upper().c_str(), cxrIntf.m_srcInstName.Upper().c_str(), cxrIntf.GetPortReplId(), cxrIntf.m_funcName.Upper().c_str());
				} else {
					if (cxrIntf.m_pSrcGroup->m_rtnInstrW > 0)
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnInstr = (%sRtnInstr_t)%s_%s_%s%s.m_rtnInstr;\n",
							pOutStg,
							mod.m_modName.Uc().c_str(), 
							pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

					if (cxrIntf.m_pSrcGroup->m_rtnHtIdW > 0)
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnHtId = (%sRtnHtId_t)%s_%s_%s%s.m_rtnHtId;\n",
							pOutStg,
							mod.m_modName.Uc().c_str(), 
							pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}

				if (mod.m_bRtnJoin) {
					if (cxrIntf.m_bRtnJoin)
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnJoin = %s_%s_%s%s.m_rtnJoin;\n",
							pOutStg,
							pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
					else
						pIplTxStg->Append("\t\t%s_htPriv.m_rtnJoin = false;\n",
							pOutStg);
				}

				if (mod.m_maxRtnReplCnt > 1) {
					pIplTxStg->Append("\t\t%s_htPriv.m_rtnReplSel = %d;\n",
						pOutStg,
						cxrIntf.m_rtnReplId);
				}

			} else {
				pIplTxStg->Append("\t\t%s_htInstr = %s_%s_%s%s.m_rtnInstr;\n",
					pOutStg,
					pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				CCxrIntf &cxrCallIntf = modInst.m_cxrIntfList[cxrIntf.m_callIntfIdx];
				CCxrCall & cxrCall = modInst.m_pMod->m_cxrCallList[cxrCallIntf.m_callIdx];

				if (cxrCall.m_bCallFork || cxrCall.m_bXferFork) {
					pIplTxStg->Append("\t\t%s_rtnJoin = %s_%s_%s%s.m_rtnJoin;\n",
						pOutStg,
						pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
			}

			for (size_t fldIdx = 0; fldIdx < cxrIntf.m_pFieldList->size(); fldIdx += 1) {
				const char * pFieldName = (*cxrIntf.m_pFieldList)[fldIdx].m_name.c_str();

				pIplTxStg->Append("\t\t%s_htPriv.m_%s = %s_%s_%s%s.m_%s;\n",
					pOutStg, pFieldName,
					pOutStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(), pFieldName);
			}

			pIplTxStg->Append("\t\tbreak;\n");
		}

		pIplTxStg->Append("\tcase %s_HT_SEL_SM:\n",
			mod.m_modName.Upper().c_str());

		if (mod.m_rsmSrcCnt > 0) {
			pIplTxStg->Append("\t\tif (%s_rsmHtRdy)\n", pOutStg);
			pIplTxStg->Append("\t\t\t%s_htInstr = %s_rsmHtInstr;\n", pOutStg, pOutStg);
			pIplTxStg->Append("\t\telse\n\t");
		}

		pIplTxStg->Append("\t\t%s_htInstr = %s_htCmd.m_htInstr;\n", pOutStg, pOutStg);
		pIplTxStg->Append("\t\tbreak;\n");

		pIplTxStg->Append("\tdefault:\n");
		pIplTxStg->Append("\t\tassert(0);\n");
		pIplTxStg->Append("\t}\n");

		IplNewLine(pIplTxStg);

		if (mod.m_gblBlockRam) {
			IplComment(pIplTxStg, iplReg, "\t// External ram stage #1\n");

			iplStg += 1;
			sprintf(inStgStr, "r_t%d", iplStg);
			sprintf(outStgStr, "c_t%d", iplStg);
			sprintf(regStgStr, "r_t%d", iplStg+1);

			pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg) );
			iplReg.Append("\t%s_htValid = !c_reset1x && %s_htValid;\n", pRegStg, pOutStg);

			if (modInst.m_cxrSrcCnt > 1 /*&& !bSingleUnnamedGroup*/) {
				pIplTxStg->Append("\tht_uint%d %s_htSel = %s_htSel;\n", FindLg2(modInst.m_cxrSrcCnt-1), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelTypeStr, VA("%s_htSel", pRegStg) );
				iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
			}

			if (bMultiThread) {
				pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg));
				iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
			}

			pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_htInstr = %s_htInstr;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htInstrTypeStr, VA("%s_htInstr", pRegStg) );
			iplReg.Append("\t%s_htInstr = %s_htInstr;\n", pRegStg, pOutStg);

			if (mod.m_bCallFork) {
				pIplTxStg->Append("\tbool %s_rtnJoin = %s_rtnJoin;\n", pOutStg, pInStg);
				m_iplRegDecl.Append("\tbool %s_rtnJoin;\n", pRegStg);
				iplReg.Append("\t%s_rtnJoin = %s_rtnJoin;\n", pRegStg, pOutStg);
			}

			pIplTxStg->Append("\tCHtPriv %s_htPriv = %s_htPriv;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("CHtPriv"), VA("%s_htPriv", pRegStg) );
			iplReg.Append("\t%s_htPriv = %s_htPriv;\n", pRegStg, pOutStg);

			if (mod.m_threads.m_htIdW.AsInt() > 2 && mod.m_bCallFork) {
				pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg) );
				iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
			}

			IplComment(pIplTxStg, iplReg, "\t// External ram stage #2\n");

			iplStg += 1;
			sprintf(inStgStr, "r_t%d", iplStg);
			sprintf(outStgStr, "c_t%d", iplStg);
			sprintf(regStgStr, "r_t%d", iplStg+1);

			pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg) );
			iplReg.Append("\t%s_htValid = !c_reset1x && %s_htValid;\n", pRegStg, pOutStg);

			if (modInst.m_cxrSrcCnt > 1 /*&& !bSingleUnnamedGroup*/) {
				pIplTxStg->Append("\tht_uint%d %s_htSel = %s_htSel;\n", FindLg2(modInst.m_cxrSrcCnt-1), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelTypeStr, VA("%s_htSel", pRegStg) );
				iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
			}

			if (bMultiThread) {
				pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg) );
				iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
			}

			pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_htInstr = %s_htInstr;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htInstrTypeStr, VA("%s_htInstr", pRegStg) );
			iplReg.Append("\t%s_htInstr = %s_htInstr;\n", pRegStg, pOutStg);

			if (mod.m_bCallFork) {
				pIplTxStg->Append("\tbool %s_rtnJoin = %s_rtnJoin;\n", pOutStg, pInStg);
				m_iplRegDecl.Append("\tbool %s_rtnJoin;\n", pRegStg);
				iplReg.Append("\t%s_rtnJoin = %s_rtnJoin;\n", pRegStg, pOutStg);
			}

			pIplTxStg->Append("\tCHtPriv %s_htPriv = %s_htPriv;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("CHtPriv"), VA("%s_htPriv", pRegStg) );
			iplReg.Append("\t%s_htPriv = %s_htPriv;\n", pRegStg, pOutStg);

			if (mod.m_threads.m_htIdW.AsInt() > 2 && mod.m_bCallFork) {
				pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg) );
				iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
			}

			IplComment(pIplTxStg, iplReg, "\t// External ram stage #3\n");

			iplStg += 1;
			sprintf(inStgStr, "r_t%d", iplStg);
			sprintf(outStgStr, "c_t%d", iplStg);
			sprintf(regStgStr, "r_t%d", iplStg+1);

			pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg) );
			iplReg.Append("\t%s_htValid = !c_reset1x && %s_htValid;\n", pRegStg, pOutStg);

			if (modInst.m_cxrSrcCnt > 1) {
				pIplTxStg->Append("\tht_uint%d %s_htSel = %s_htSel;\n", FindLg2(modInst.m_cxrSrcCnt-1), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelTypeStr, VA("%s_htSel", pRegStg) );
				iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
			}

			if (bMultiThread) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg) );

				if (mod.m_bHtIdInit && iplStg == mod.m_execStg-1) {
					m_iplRegDecl.Append("\tbool %s_htIdInit;\n", pRegStg);
					pIplTxStg->Append("\tbool %s_htIdInit = %s_htIdInit && %s_htId != 0x%x;\n",
						pOutStg, pRegStg, pRegStg, (1 << mod.m_threads.m_htIdW.AsInt())-1);

					pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htIdInit ? (sc_uint<%s_HTID_W>)(%s_htId+1) : %s_htId;\n",
						mod.m_modName.Upper().c_str(), pOutStg, pRegStg, mod.m_modName.Upper().c_str(), pRegStg, pInStg);
					iplReg.Append("\t%s_htId = c_reset1x ? (sc_uint<%s>)0 : %s_htId;\n", pRegStg, mod.m_threads.m_htIdW.c_str(), pOutStg);

					iplReg.Append("\t%s_htIdInit = c_reset1x || %s_htIdInit;\n", pRegStg, pOutStg);
				} else {
					pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);
					iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
				}
			}

			pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_htInstr = %s_htInstr;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htInstrTypeStr, VA("%s_htInstr", pRegStg) );
			iplReg.Append("\t%s_htInstr = %s_htInstr;\n", pRegStg, pOutStg);

			if (mod.m_bCallFork) {
				pIplTxStg->Append("\tbool %s_rtnJoin = %s_rtnJoin;\n", pOutStg, pInStg);
				m_iplRegDecl.Append("\tbool ht_noload %s_rtnJoin;\n", pRegStg);
				iplReg.Append("\t%s_rtnJoin = %s_rtnJoin;\n", pRegStg, pOutStg);
			}

			pIplTxStg->Append("\tCHtPriv %s_htPriv = %s_htPriv;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("CHtPriv"), VA("%s_htPriv", pRegStg) );
			iplReg.Append("\t%s_htPriv = %s_htPriv;\n", pRegStg, pOutStg);

			if (mod.m_threads.m_htIdW.AsInt() > 2 && mod.m_bCallFork) {
				pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg) );
				iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
			}

			pIplTxStg->NewLine();
			m_iplRegDecl.NewLine();
			iplReg.NewLine();

			// external ram interfaces
			if (mod.m_gblBlockRam) {
				for (size_t ramIdx = 0; ramIdx < mod.m_extRamList.size(); ramIdx += 1) {
					CRam &extRam = mod.m_extRamList[ramIdx];

					if (extRam.m_bSrcRead) {
						int pairedUnitCnt = (int)extRam.m_pIntRamMod->m_modInstList.size();

						if ((int)mod.m_modInstList.size() >= pairedUnitCnt) {
							if (extRam.m_pIntGbl->m_dimenList.size() == 0)
								pIplTxStg->Append("\tC%sTo%s_%sRdData %s_%sRdData = i_%sTo%s_%sRdData;\n",
								extRam.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), extRam.m_gblName.c_str(),
								pOutStg, extRam.m_gblName.c_str(),
								extRam.m_modName.Lc().c_str(), mod.m_modName.Uc().c_str(), extRam.m_gblName.c_str());
							else {
								pIplTxStg->Append("\tC%sTo%s_%sRdData %s_%sRdData%s;\n",
									extRam.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), extRam.m_gblName.c_str(),
									pOutStg, extRam.m_gblName.c_str(), extRam.m_pIntGbl->m_dimenDecl.c_str());

								GenRamIndexLoops(*pIplTxStg, "", *extRam.m_pIntGbl);
								pIplTxStg->Append("\t%s_%sRdData%s = i_%sTo%s_%sRdData%s;\n",
									pOutStg, extRam.m_gblName.c_str(), extRam.m_pIntGbl->m_dimenIndex.c_str(),
									extRam.m_modName.Lc().c_str(), mod.m_modName.Uc().c_str(), extRam.m_gblName.c_str(), extRam.m_pIntGbl->m_dimenIndex.c_str());
							}
						} else {
							// select replicated ram's read data
							HtlAssert(pairedUnitCnt % mod.m_modInstList.size() == 0);
							int intfCnt = pairedUnitCnt / (int)mod.m_modInstList.size();

							// if multiple groups have read address variable in htPriv then must select which to use
							string accessSelName = string(pOutStg) + "_" + extRam.m_gblName.AsStr() + "RdAddrSel";
							char width[10];
							sprintf(width, "%d", FindLg2(intfCnt-1));
							string rdDataSelName = GenRamAddr(modInst, extRam, pIplTxStg, width, accessSelName, pInStg, pOutStg, false);

							pIplTxStg->Append("\tC%sTo%s_%sRdData %s_%sRdData%s;\n",
								extRam.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), extRam.m_gblName.c_str(),
								pOutStg, extRam.m_gblName.c_str(), extRam.m_pIntGbl->m_dimenDecl.c_str());

							pIplTxStg->Append("\tswitch (%s & 0x%x) {\n", rdDataSelName.c_str(), (1ul << FindLg2(intfCnt-1))-1);
							string instNum = "0";
							for (int inst = 0; inst < intfCnt; inst += 1, instNum[0] += 1) {
								pIplTxStg->Append("\tcase %d:\n", inst);

								GenRamIndexLoops(*pIplTxStg, "", *extRam.m_pIntGbl);
								pIplTxStg->Append("\t\t%s_%sRdData%s = i_%s%dTo%s_%sRdData%s;\n",
									pOutStg, extRam.m_gblName.c_str(), extRam.m_pIntGbl->m_dimenIndex.c_str(),
									extRam.m_modName.Lc().c_str(), inst, mod.m_modName.Uc().c_str(), extRam.m_gblName.c_str(), extRam.m_pIntGbl->m_dimenIndex.c_str());
								pIplTxStg->Append("\t\tbreak;\n");
							}
							pIplTxStg->Append("\t}\n");
						}

						for (int i = 0; i < extRam.m_rdStg.AsInt(); i += 1) {

							if (i > 0) {
								pIplTxStg->Append("\tC%sTo%s_%sRdData c_t%d_%sRdData%s;\n",
									extRam.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), extRam.m_gblName.c_str(),
									iplStg+i, extRam.m_gblName.c_str(), extRam.m_pIntGbl->m_dimenDecl.c_str());

								GenRamIndexLoops(*pIplTxStg, "", *extRam.m_pIntGbl);
								pIplTxStg->Append("\tc_t%d_%sRdData%s = r_t%d_%sRdData%s;\n",
									iplStg+i, extRam.m_gblName.c_str(), extRam.m_pIntGbl->m_dimenIndex.c_str(),
									iplStg+i, extRam.m_gblName.c_str(), extRam.m_pIntGbl->m_dimenIndex.c_str());
							}

							m_iplRegDecl.Append("\tC%sTo%s_%sRdData r_t%d_%sRdData%s;\n",
								extRam.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), extRam.m_gblName.c_str(),
								iplStg+1+i, extRam.m_gblName.c_str(), extRam.m_pIntGbl->m_dimenDecl.c_str());

							GenRamIndexLoops(iplReg, "", *extRam.m_pIntGbl);
							iplReg.Append("\tr_t%d_%sRdData%s = c_t%d_%sRdData%s;\n",
								iplStg+1+i, extRam.m_gblName.c_str(), extRam.m_pIntGbl->m_dimenIndex.c_str(),
								iplStg+i, extRam.m_gblName.c_str(), extRam.m_pIntGbl->m_dimenIndex.c_str());
						}
					}
				}
				pIplTxStg->Append("\n");
			}
		} else if (mod.m_gblDistRam) {
			IplComment(pIplTxStg, iplReg, "\t// Internal ram stage #1\n");

			iplStg += 1;
			sprintf(inStgStr, "r_t%d", iplStg);
			sprintf(outStgStr, "c_t%d", iplStg);
			sprintf(regStgStr, "r_t%d", iplStg+1);

			pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg) );
			iplReg.Append("\t%s_htValid = !c_reset1x && %s_htValid;\n", pRegStg, pOutStg);

			pIplTxStg->Append("\tCHtPriv %s_htPriv = %s_htPriv;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("CHtPriv"), VA("%s_htPriv", pRegStg) );
			iplReg.Append("\t%s_htPriv = %s_htPriv;\n", pRegStg, pOutStg);

			if (mod.m_threads.m_htIdW.AsInt() > 2 && mod.m_bCallFork) {
				pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg) );
				iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
			}

			if (modInst.m_cxrSrcCnt > 1) {
				pIplTxStg->Append("\tht_uint%d %s_htSel = %s_htSel;\n", FindLg2(modInst.m_cxrSrcCnt-1), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelTypeStr, VA("%s_htSel", pRegStg) );
				iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
			}

			if (bMultiThread) {
				pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg) );
				iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
			}

			pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_htInstr = %s_htInstr;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htInstrTypeStr, VA("%s_htInstr", pRegStg) );
			iplReg.Append("\t%s_htInstr = %s_htInstr;\n", pRegStg, pOutStg);

			if (mod.m_bCallFork) {
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
			sprintf(regStgStr, "r_t%d", iplStg+1);

			pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg) );
			iplReg.Append("\t%s_htValid = !c_reset1x && %s_htValid;\n", pRegStg, pOutStg);

			pIplTxStg->Append("\tCHtPriv %s_htPriv = %s_htPriv;\n", pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("CHtPriv"), VA("%s_htPriv", pRegStg) );
			iplReg.Append("\t%s_htPriv = %s_htPriv;\n", pRegStg, pOutStg);
			pIplTxStg->Append("\n");

			if (mod.m_threads.m_htIdW.AsInt() > 2 && mod.m_bCallFork) {
				pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg) );
				iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
			}

			if (modInst.m_cxrSrcCnt > 1) {
				pIplTxStg->Append("\tht_uint%d %s_htSel = %s_htSel;\n", FindLg2(modInst.m_cxrSrcCnt-1), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelTypeStr, VA("%s_htSel", pRegStg) );
				iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
			}

			if (bMultiThread) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg) );

				if (mod.m_bHtIdInit && iplStg == mod.m_execStg-1) {
					m_iplRegDecl.Append("\tbool %s_htIdInit;\n", pRegStg);
					pIplTxStg->Append("\tbool %s_htIdInit = %s_htIdInit && %s_htId != 0x%x;\n",
						pOutStg, pRegStg, pRegStg, (1 << mod.m_threads.m_htIdW.AsInt())-1);

					pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htIdInit ? (sc_uint<%s_HTID_W>)(%s_htId+1) : %s_htId;\n",
						mod.m_modName.Upper().c_str(), pOutStg, pRegStg, mod.m_modName.Upper().c_str(), pRegStg, pInStg);
					iplReg.Append("\t%s_htId = c_reset1x ? (sc_uint<%s_HTID_W>)0 : %s_htId;\n", pRegStg, mod.m_modName.Upper().c_str(), pOutStg);

					iplReg.Append("\t%s_htIdInit = c_reset1x || %s_htIdInit;\n", pRegStg, pOutStg);
				} else {
					pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);
					iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
				}
			}

			pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_htInstr = %s_htInstr;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htInstrTypeStr, VA("%s_htInstr", pRegStg) );
			iplReg.Append("\t%s_htInstr = %s_htInstr;\n", pRegStg, pOutStg);

			if (mod.m_bCallFork) {
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
		for (int tsIdx = 0; tsIdx < mod.m_stage.m_privWrStg.AsInt(); tsIdx += 1) {
			pIplTxStg->Append("\n");
			m_iplRegDecl.Append("\n");
			iplReg.Append("\n");

			pIplTxStg->Append("\t// Instruction Stage #%d\n", tsIdx+1);
			m_iplRegDecl.Append("\t// Instruction Stage #%d\n", tsIdx+1);
			iplReg.Append("\t// Instruction Stage #%d\n", tsIdx+1);

			iplStg += 1;
			sprintf(inStgStr, "r_t%d", iplStg);
			sprintf(outStgStr, "c_t%d", iplStg);
			sprintf(regStgStr, "r_t%d", iplStg+1);

			if (tsIdx < mod.m_stage.m_privWrStg.AsInt()-1 || mod.m_globalVarList.size() > 0) {

				pIplTxStg->Append("\tbool %s_htValid = %s_htValid;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("%s_htValid", pRegStg) );
				iplReg.Append("\t%s_htValid = !c_reset1x && %s_htValid;\n", pRegStg, pOutStg);
			}

			if (tsIdx < mod.m_stage.m_privWrStg.AsInt()-1) {
				pIplTxStg->Append("\tht_uint%d %s_htSel = %s_htSel;\n", FindLg2(modInst.m_cxrSrcCnt-1), pOutStg, pInStg);
				iplReg.Append("\t%s_htSel = %s_htSel;\n", pRegStg, pOutStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htSelTypeStr, VA("%s_htSel", pRegStg) );
			}

			if (tsIdx < mod.m_stage.m_privWrStg.AsInt()-1) {
				pIplTxStg->Append("\tsc_uint<%s_INSTR_W> %s_htInstr = %s_htInstr;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htInstrTypeStr, VA("%s_htInstr", pRegStg) );
				iplReg.Append("\t%s_htInstr = %s_htInstr;\n", pRegStg, pOutStg);

				if (mod.m_bCallFork) {
					pIplTxStg->Append("\tbool %s_rtnJoin = %s_rtnJoin;\n", pOutStg, pInStg);
					m_iplRegDecl.Append("\tbool %s_rtnJoin;\n", pRegStg);
					iplReg.Append("\t%s_rtnJoin = %s_rtnJoin;\n", pRegStg, pOutStg);
				}
			}

			if (bMultiThread) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, htIdTypeStr, VA("%s_htId", pRegStg) );

				if (mod.m_bHtIdInit && iplStg == mod.m_execStg-1) {
					m_iplRegDecl.Append("\tbool %s_htIdInit;\n", pRegStg);
					pIplTxStg->Append("\tbool %s_htIdInit = %s_htIdInit && %s_htId != 0x%x;\n",
						pOutStg, pRegStg, pRegStg, (1 << mod.m_threads.m_htIdW.AsInt())-1);

					pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htIdInit ? (sc_uint<%s_HTID_W>)(%s_htId+1) : %s_htId;\n",
						mod.m_modName.Upper().c_str(), pOutStg, pRegStg, mod.m_modName.Upper().c_str(), pRegStg, pInStg);
					iplReg.Append("\t%s_htId = c_reset1x ? (sc_uint<%s_HTID_W>)0 : %s_htId;\n", pRegStg, mod.m_modName.Upper().c_str(), pOutStg);

					iplReg.Append("\t%s_htIdInit = c_reset1x || %s_htIdInit;\n", pRegStg, pOutStg);
				} else {
					pIplTxStg->Append("\tsc_uint<%s_HTID_W> %s_htId = %s_htId;\n", mod.m_modName.Upper().c_str(), pOutStg, pInStg);
					iplReg.Append("\t%s_htId = %s_htId;\n", pRegStg, pOutStg);
				}
			}

			m_iplRegDecl.Append("\tCHtPriv %s_htPriv;\n", pOutStg);
			pIplTxStg->Append("\t%s_htPriv = %s_htPriv;\n", pOutStg, pInStg);

			if (bMultiThread || tsIdx < mod.m_stage.m_privWrStg.AsInt()-1) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("CHtPriv"), VA("%s_htPriv", pRegStg) );
				iplReg.Append("\t%s_htPriv = %s_htPriv;\n", pRegStg, pOutStg);
			}

			if (mod.m_threads.m_htIdW.AsInt() > 2 && mod.m_bCallFork) {
				pIplTxStg->Append("\tbool %s_htPrivLkData = %s_htPrivLkData;\n", pOutStg, pInStg);
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, VA("bool"), VA("%s_htPrivLkData", pRegStg) );
				iplReg.Append("\t%s_htPrivLkData = %s_htPrivLkData;\n", pRegStg, pOutStg);
			}

			IplNewLine(pIplTxStg);

			if (iplStg >= mod.m_execStg) {

				if (iplStg == mod.m_execStg) {
					m_iplRegDecl.Append("\tsc_uint<4> c_t%d_htCtrl;\n", iplStg);
					pIplTxStg->Append("\tc_t%d_htCtrl = HT_INVALID;\n", iplStg);
				} else {
					m_iplRegDecl.Append("\tsc_uint<4> c_t%d_htCtrl;\n", iplStg);
					pIplTxStg->Append("\tc_t%d_htCtrl = r_t%d_htCtrl;\n", iplStg, iplStg);
				}

				if (mod.m_wrStg-1 > iplStg) {
					m_iplRegDecl.Append("\tsc_uint<4> r_t%d_htCtrl;\n", iplStg+1);
					iplReg.Append("\tr_t%d_htCtrl = c_t%d_htCtrl;\n", iplStg+1, iplStg);
				}

				if (iplStg == mod.m_execStg) {
					m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> c_t%d_htNextInstr;\n", mod.m_modName.Upper().c_str(), iplStg);
					pIplTxStg->Append("\tc_t%d_htNextInstr = r_t%d_htInstr;\n", iplStg, iplStg);
				} else {
					m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> c_t%d_htNextInstr;\n", mod.m_modName.Upper().c_str(), iplStg);
					pIplTxStg->Append("\tc_t%d_htNextInstr = r_t%d_htNextInstr;\n", iplStg, iplStg);
				}

				if (mod.m_wrStg-1 > iplStg) {
					m_iplRegDecl.Append("\tsc_uint<%s_INSTR_W> r_t%d_htNextInstr;\n", mod.m_modName.Upper().c_str(), iplStg+1);
					iplReg.Append("\tr_t%d_htNextInstr = c_t%d_htNextInstr;\n", iplStg+1, iplStg);
				}
			}

			/////////////////////////////////
			// handle global ram write logic

			for (size_t ramIdx = 0; ramIdx < mod.m_extRamList.size(); ramIdx += 1) {
				CRam &extRam = mod.m_extRamList[ramIdx];

				bool bHasAddr = extRam.m_addr0W.AsInt() + extRam.m_addr1W.AsInt() + extRam.m_addr2W.AsInt() > 0;

				if (extRam.m_wrStg.AsInt()-1 != tsIdx) continue;

				if (extRam.m_bSrcWrite) {

					int wrStg = mod.m_tsStg + extRam.m_wrStg.AsInt() - 1;

					string addrW = extRam.m_addr1W.AsStr();
					if (extRam.m_addr2W.size() > 0)
						addrW = extRam.m_addr2W.AsStr() + " + " + addrW;

					m_iplRegDecl.Append("\tC%sTo%s_%sWrEn c_t%d_%sWrEn%s;\n",
						mod.m_modName.Uc().c_str(), extRam.m_modName.Uc().c_str(), extRam.m_gblName.c_str(),
						wrStg, extRam.m_gblName.c_str(), extRam.m_dimenDecl.c_str());

					GenRamIndexLoops(*pIplTxStg, "", extRam);
					pIplTxStg->Append("\tc_t%d_%sWrEn%s = 0;\n",
						wrStg, extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str());

					m_iplRegDecl.Append("\tC%sTo%s_%sWrEn %s_%sWrEn%s;\n",
						mod.m_modName.Uc().c_str(), extRam.m_modName.Uc().c_str(), extRam.m_gblName.c_str(),
						pRegStg, extRam.m_gblName.c_str(), extRam.m_dimenDecl.c_str());

					GenRamIndexLoops(iplReg, "", extRam);
					iplReg.Append("\t%s_%sWrEn%s = %s_%sWrEn%s;\n",
						pRegStg, extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str(),
						pOutStg, extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str());

					if (bHasAddr) {
						// if multiple groups have read address variable in htPriv then must select which to use
						string accessSelName = string(pOutStg) + "_" + extRam.m_gblName.AsStr() + "WrAddrSel";
						string wrAddrName = GenRamAddr(modInst, extRam, pIplTxStg, addrW, accessSelName, pInStg, pOutStg, true);

						m_iplRegDecl.Append("\tsc_uint<%s> %s_%sWrAddr%s;\n",
							addrW.c_str(),
							pOutStg, extRam.m_gblName.c_str(), extRam.m_dimenDecl.c_str());

						GenRamIndexLoops(*pIplTxStg, "", extRam);
						pIplTxStg->Append("\t%s_%sWrAddr%s = %s;\n",
							pOutStg, extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str(),
							wrAddrName.c_str());

						m_iplRegDecl.Append("\tsc_uint<%s> %s_%sWrAddr%s;\n",
							addrW.c_str(),
							pRegStg, extRam.m_gblName.c_str(), extRam.m_dimenDecl.c_str());

						GenRamIndexLoops(iplReg, "", extRam);
						iplReg.Append("\t%s_%sWrAddr%s = %s_%sWrAddr%s;\n",
							pRegStg, extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str(),
							pOutStg, extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str());
					}

					m_iplRegDecl.Append("\tC%sTo%s_%sWrData c_t%d_%sWrData%s;\n",
						mod.m_modName.Uc().c_str(), extRam.m_modName.Uc().c_str(), extRam.m_gblName.c_str(),
						wrStg, extRam.m_gblName.c_str(), extRam.m_dimenDecl.c_str());

					GenRamIndexLoops(*pIplTxStg, "", extRam);
					pIplTxStg->Append("\tc_t%d_%sWrData%s = 0;\n",
						wrStg, extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str());

					m_iplRegDecl.Append("\tC%sTo%s_%sWrData %s_%sWrData%s;\n",
						mod.m_modName.Uc().c_str(), extRam.m_modName.Uc().c_str(), extRam.m_gblName.c_str(),
						pRegStg, extRam.m_gblName.c_str(), extRam.m_dimenDecl.c_str());

					GenRamIndexLoops(iplReg, "", extRam);
					iplReg.Append("\t%s_%sWrData%s = %s_%sWrData%s;\n",
						pRegStg, extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str(),
						pOutStg, extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str());

					pIplTxStg->NewLine();
					m_iplRegDecl.NewLine();
					iplReg.NewLine();
				}


				if (extRam.m_bMifWrite) {

					string addrW = extRam.m_addr1W.AsStr();
					if (extRam.m_addr2W.size() > 0)
						addrW = extRam.m_addr2W.AsStr() + " + " + addrW;

					m_iplRegDecl.Append("\tC%sTo%s_%sWrEn c_m1_%sWrEn%s;\n",
						mod.m_modName.Uc().c_str(), extRam.m_modName.Uc().c_str(), extRam.m_gblName.c_str(),
						extRam.m_gblName.c_str(), extRam.m_dimenDecl.c_str());

					GenRamIndexLoops(*pIplTxStg, "", extRam);
					pIplTxStg->Append("\tc_m1_%sWrEn%s = 0;\n",
						extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str());

					m_iplRegDecl.Append("\tC%sTo%s_%sWrEn r_m2_%sWrEn%s;\n",
						mod.m_modName.Uc().c_str(), extRam.m_modName.Uc().c_str(), extRam.m_gblName.c_str(),
						extRam.m_gblName.c_str(), extRam.m_dimenDecl.c_str());

					GenRamIndexLoops(iplReg, "", extRam);
					iplReg.Append("\tr_m2_%sWrEn%s = c_m1_%sWrEn%s;\n",
						extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str(),
						extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str());

					if (bHasAddr) {
						m_iplRegDecl.Append("\tsc_uint<%s> c_m1_%sWrAddr%s;\n",
							addrW.c_str(),
							extRam.m_gblName.c_str(), extRam.m_dimenDecl.c_str());

						GenRamIndexLoops(*pIplTxStg, "", extRam);
						pIplTxStg->Append("\tc_m1_%sWrAddr%s = 0;\n",
							extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str());

						m_iplRegDecl.Append("\tsc_uint<%s> r_m2_%sWrAddr%s;\n",
							addrW.c_str(),
							extRam.m_gblName.c_str(), extRam.m_dimenDecl.c_str());

						GenRamIndexLoops(iplReg, "", extRam);
						iplReg.Append("\tr_m2_%sWrAddr%s = c_m1_%sWrAddr%s;\n",
							extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str(),
							extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str());
					}

					m_iplRegDecl.Append("\tC%sTo%s_%sWrData c_m1_%sWrData%s;\n",
						mod.m_modName.Uc().c_str(), extRam.m_modName.Uc().c_str(), extRam.m_gblName.c_str(),
						extRam.m_gblName.c_str(), extRam.m_dimenDecl.c_str());
					GenRamIndexLoops(*pIplTxStg, "", extRam);
					pIplTxStg->Append("\tc_m1_%sWrData%s = 0;\n",
						extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str());

					m_iplRegDecl.Append("\tC%sTo%s_%sWrData r_m2_%sWrData%s;\n",
						mod.m_modName.Uc().c_str(), extRam.m_modName.Uc().c_str(), extRam.m_gblName.c_str(),
						extRam.m_gblName.c_str(), extRam.m_dimenDecl.c_str());

					GenRamIndexLoops(iplReg, "", extRam);
					iplReg.Append("\tr_m2_%sWrData%s = c_m1_%sWrData%s;\n",
						extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str(),
						extRam.m_gblName.c_str(), extRam.m_dimenIndex.c_str());

					pIplTxStg->NewLine();
					m_iplRegDecl.NewLine();
					iplReg.NewLine();
				}
			}

			// generate internal ram interface
			if (bStateMachine) {
				for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
					CRam &intRam = *mod.m_intGblList[ramIdx];

					if (intRam.m_wrStg.AsInt()-1 != tsIdx) continue;

					if (intRam.m_fieldList.size() == 0) continue;

					bool bHasAddr = intRam.m_addr0W.AsInt() + intRam.m_addr1W.AsInt() + intRam.m_addr2W.AsInt() > 0;

					string ramWrEnType;
					string ramWrDataType;
					if (intRam.m_bGlobal) {
						ramWrEnType = "bool";
						ramWrDataType = intRam.m_type;
					} else {
						ramWrEnType = "C" + intRam.m_gblName.Uc() + "WrEn";
						ramWrDataType = "C" + intRam.m_gblName.Uc() + "WrData";
					}

					if (intRam.m_bSrcWrite) {

						CRamPort srcWrPort = intRam.m_gblPortList[0];

						string addrW;
						if (intRam.m_addr0W.size() > 0)
							addrW = intRam.m_addr0W.AsStr();
						if (intRam.m_addr1W.size() > 0)
							addrW += "+";
						addrW += intRam.m_addr1W.AsStr();
						if (intRam.m_addr2W.size() > 0)
							addrW += " + " + intRam.m_addr2W.AsStr();

						int wrStg = mod.m_tsStg + intRam.m_wrStg.AsInt() - 1;

						if (bHasAddr) {
							if (intRam.m_bSrcWrite && intRam.m_addr1Name.size() > 0) {
								// if multiple groups have read address variable in htPriv then must select which to use
								string accessSelName = string(pOutStg) + "_" + intRam.m_gblName.AsStr() + "WrAddrSel";
								string wrAddrName = GenRamAddr(modInst, intRam, pIplTxStg, addrW, accessSelName, pInStg, pOutStg, true);

								m_iplRegDecl.Append("\tsc_uint<%s> c_t%d_%sWrAddr%s;\n",
									addrW.c_str(), wrStg, intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());

								GenRamIndexLoops(*pIplTxStg, "", intRam);
								pIplTxStg->Append("\tc_t%d_%sWrAddr%s = %s;\n",
									wrStg, intRam.m_gblName.c_str(), intRam.m_dimenIndex.c_str(), wrAddrName.c_str());

							} else {
								m_iplRegDecl.Append("\tsc_uint<%s> c_t%d_%sWrAddr%s;\n",
									addrW.c_str(), wrStg, intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());

								GenRamIndexLoops(*pIplTxStg, "", intRam);
								pIplTxStg->Append("\tc_t%d_%sWrAddr%s = 0;\n",
									wrStg, intRam.m_gblName.c_str(), intRam.m_dimenIndex.c_str());
							}

							if (srcWrPort.m_bSecond1xWriteIntf || srcWrPort.m_bFirst1xWriteIntf)
								m_iplRegDecl.Append("\tsc_signal<sc_uint<%s> > %s_%sWrAddr%s;\n",
								addrW.c_str(), pRegStg, intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());
							else
								m_iplRegDecl.Append("\tsc_uint<%s> %s_%sWrAddr%s;\n",
								addrW.c_str(), pRegStg, intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());

							GenRamIndexLoops(iplReg, "", intRam);
							iplReg.Append("\t%s_%sWrAddr%s = %s_%sWrAddr%s;\n",
								pRegStg, intRam.m_gblName.c_str(), intRam.m_dimenIndex.c_str(),
								pOutStg, intRam.m_gblName.c_str(), intRam.m_dimenIndex.c_str());
						}

						m_iplRegDecl.Append("\t%s c_t%d_%sWrEn%s;\n",
							ramWrEnType.c_str(), wrStg, intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());

						if (srcWrPort.m_bSecond1xWriteIntf || srcWrPort.m_bFirst1xWriteIntf)
							m_iplRegDecl.Append("\tsc_signal<%s> %s_%sWrEn%s;\n",
							ramWrEnType.c_str(), pRegStg, intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());
						else
							m_iplRegDecl.Append("\t%s %s_%sWrEn%s;\n",
							ramWrEnType.c_str(), pRegStg, intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());

						GenRamIndexLoops(*pIplTxStg, "", intRam);
						pIplTxStg->Append("\tc_t%d_%sWrEn%s = 0;\n",
							wrStg, intRam.m_gblName.c_str(), intRam.m_dimenIndex.c_str());

						GenRamIndexLoops(iplReg, "", intRam);
						iplReg.Append("\t%s_%sWrEn%s = %s_%sWrEn%s;\n",
							pRegStg, intRam.m_gblName.c_str(),  intRam.m_dimenIndex.c_str(),
							pOutStg, intRam.m_gblName.c_str(),  intRam.m_dimenIndex.c_str());

						m_iplRegDecl.Append("\t%s c_t%d_%sWrData%s;\n",
							ramWrDataType.c_str(), wrStg, intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());

						if (srcWrPort.m_bSecond1xWriteIntf || srcWrPort.m_bFirst1xWriteIntf)
							m_iplRegDecl.Append("\tsc_signal<%s > %s_%sWrData%s;\n",
							ramWrDataType.c_str(), pRegStg, intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());
						else
							m_iplRegDecl.Append("\t%s %s_%sWrData%s;\n",
							ramWrDataType.c_str(), pRegStg, intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());

						GenRamIndexLoops(*pIplTxStg, "", intRam);
						pIplTxStg->Append("\tc_t%d_%sWrData%s = 0;\n",
							wrStg, intRam.m_gblName.c_str(), intRam.m_dimenIndex.c_str());

						GenRamIndexLoops(iplReg, "", intRam);
						iplReg.Append("\t%s_%sWrData%s = %s_%sWrData%s;\n",
							pRegStg, intRam.m_gblName.c_str(),  intRam.m_dimenIndex.c_str(),
							pOutStg, intRam.m_gblName.c_str(),  intRam.m_dimenIndex.c_str());

						pIplTxStg->NewLine();
						m_iplRegDecl.NewLine();
						iplReg.NewLine();
					}

					if (intRam.m_bMifWrite) {

						CRamPort &mifWrPort = intRam.m_gblPortList[0].m_bMifWrite ? intRam.m_gblPortList[0] : intRam.m_gblPortList[1];

						string addrW;
						if (intRam.m_addr0W.size() > 0)
							addrW = intRam.m_addr0W.AsStr();
						if (intRam.m_addr1W.size() > 0)
							addrW += "+";
						addrW += intRam.m_addr1W.AsStr();
						if (intRam.m_addr2W.size() > 0)
							addrW += " + " + intRam.m_addr2W.AsStr();

						if (bHasAddr) {
							if (intRam.m_bSrcWrite && intRam.m_addr1Name.size() > 0) {
								// if multiple groups have read address variable in htPriv then must select which to use
								string accessSelName = string(pOutStg) + "_" + intRam.m_gblName.AsStr() + "WrAddrSel";
								string wrAddrName = GenRamAddr(modInst, intRam, pIplTxStg, addrW, accessSelName, pInStg, pOutStg, true);

								m_iplRegDecl.Append("\tsc_uint<%s> c_m1_%sWrAddr%s;\n",
									addrW.c_str(), intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());

								GenRamIndexLoops(*pIplTxStg, "", intRam);
								pIplTxStg->Append("\tc_m1_%sWrAddr%s = %s;\n",
									intRam.m_gblName.c_str(), intRam.m_dimenIndex.c_str(), wrAddrName.c_str());

							} else {
								m_iplRegDecl.Append("\tsc_uint<%s> c_m1_%sWrAddr%s;\n",
									addrW.c_str(), intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());

								GenRamIndexLoops(*pIplTxStg, "", intRam);
								pIplTxStg->Append("\tc_m1_%sWrAddr%s = 0;\n",
									intRam.m_gblName.c_str(), intRam.m_dimenIndex.c_str());
							}

							if (mifWrPort.m_bSecond1xWriteIntf || mifWrPort.m_bFirst1xWriteIntf)
								m_iplRegDecl.Append("\tsc_signal<sc_uint<%s> > r_m2_%sWrAddr%s;\n",
								addrW.c_str(), intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());
							else
								m_iplRegDecl.Append("\tsc_uint<%s> r_m2_%sWrAddr%s;\n",
								addrW.c_str(), intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());

							GenRamIndexLoops(iplReg, "", intRam);
							iplReg.Append("\tr_m2_%sWrAddr%s = c_m1_%sWrAddr%s;\n",
								intRam.m_gblName.c_str(), intRam.m_dimenIndex.c_str(),
								intRam.m_gblName.c_str(), intRam.m_dimenIndex.c_str());
						}

						m_iplRegDecl.Append("\t%s c_m1_%sWrEn%s;\n",
							ramWrEnType.c_str(), intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());

						if (mifWrPort.m_bSecond1xWriteIntf || mifWrPort.m_bFirst1xWriteIntf)
							m_iplRegDecl.Append("\tsc_signal<%s> r_m2_%sWrEn%s;\n",
							ramWrEnType.c_str(), intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());
						else
							m_iplRegDecl.Append("\t%s r_m2_%sWrEn%s;\n",
							ramWrEnType.c_str(), intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());

						GenRamIndexLoops(*pIplTxStg, "", intRam);
						pIplTxStg->Append("\tc_m1_%sWrEn%s = 0;\n",
							intRam.m_gblName.c_str(), intRam.m_dimenIndex.c_str());

						GenRamIndexLoops(iplReg, "", intRam);
						iplReg.Append("\tr_m2_%sWrEn%s = c_m1_%sWrEn%s;\n",
							intRam.m_gblName.c_str(),  intRam.m_dimenIndex.c_str(),
							intRam.m_gblName.c_str(),  intRam.m_dimenIndex.c_str());

						m_iplRegDecl.Append("\t%s c_m1_%sWrData%s;\n",
							ramWrDataType.c_str(), intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());

						if (mifWrPort.m_bSecond1xWriteIntf || mifWrPort.m_bFirst1xWriteIntf)
							m_iplRegDecl.Append("\tsc_signal<%s > r_m2_%sWrData%s;\n",
							ramWrDataType.c_str(), intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());
						else
							m_iplRegDecl.Append("\t%s r_m2_%sWrData%s;\n",
							ramWrDataType.c_str(), intRam.m_gblName.c_str(), intRam.m_dimenDecl.c_str());

						GenRamIndexLoops(*pIplTxStg, "", intRam);
						pIplTxStg->Append("\tc_m1_%sWrData%s = 0;\n",
							intRam.m_gblName.c_str(), intRam.m_dimenIndex.c_str());

						GenRamIndexLoops(iplReg, "", intRam);
						iplReg.Append("\tr_m2_%sWrData%s = c_m1_%sWrData%s;\n",
							intRam.m_gblName.c_str(),  intRam.m_dimenIndex.c_str(),
							intRam.m_gblName.c_str(),  intRam.m_dimenIndex.c_str());

						pIplTxStg->NewLine();
						m_iplRegDecl.NewLine();
						iplReg.NewLine();
					}
				}
			}

		}

		pIplTxStg->Append("\n");

		/////////////////////////////////
		// declare primitives

		for (size_t primIdx = 0; primIdx < mod.m_scPrimList.size(); primIdx += 1) {
			m_iplRegDecl.Append("\t%s;\n", mod.m_scPrimList[primIdx].m_scPrimDecl.c_str());
			iplReg.Append("\t%s;\n", mod.m_scPrimList[primIdx].m_scPrimFunc.c_str());
		}

		pIplTxStg->NewLine();
		m_iplRegDecl.NewLine();
		iplReg.NewLine();

		/////////////////////////////////
		// declare shared variables

		for (size_t fieldIdx = 0; fieldIdx < mod.m_shared.m_fieldList.size(); fieldIdx += 1) {
			CField &field = mod.m_shared.m_fieldList[fieldIdx];

			string type = GenFieldType(field, false);

			if (field.m_addr1W.size() > 0) {

				static bool bError = false;
				if (field.m_addr2W.size() > 0) {
					m_iplRegDecl.Append("\t%s m_%s%s;\n", type.c_str(), field.m_name.c_str(), field.m_dimenDecl.c_str());

					if (field.m_ramType == eDistRam && field.m_addr1W.AsInt() + field.m_addr2W.AsInt() > 10) {
						ParseMsg(Error, field.m_lineInfo, "unsupported distributed ram depth (addr1W + addr2W > 10)");
						if (!bError) {
							bError = true;
							ParseMsg(Info, field.m_lineInfo, "switch to block ram for greater depth (blockRam=true)");
							ParseMsg(Info, field.m_lineInfo, "shared variable as block ram requires one extra stage for read access");
						}
					}
				} else {
					m_iplRegDecl.Append("\t%s m_%s%s;\n", type.c_str(), field.m_name.c_str(), field.m_dimenDecl.c_str());

					if (field.m_ramType == eDistRam && field.m_addr1W.AsInt() > 10) {
						ParseMsg(Error, field.m_lineInfo, "unsupported distributed ram depth (addr1W > 10)");
						if (!bError) {
							bError = true;
							ParseMsg(Info, field.m_lineInfo, "switch to block ram for greater depth (blockRam=true)");
							ParseMsg(Info, field.m_lineInfo, "shared variable as block ram requires one extra stage for read access");
						}
					}
				}

			} else if (field.m_queueW.size() > 0) {

				m_iplRegDecl.Append("\t%s m_%s%s;\n", type.c_str(), field.m_name.c_str(), field.m_dimenDecl.c_str());

			} else {

				m_iplRegDecl.Append("\t%s c_%s%s;\n", type.c_str(), field.m_name.c_str(), field.m_dimenDecl.c_str());

				GenRamIndexLoops(*pIplTxStg, "", field);
				pIplTxStg->Append("\tc_%s%s = r_%s%s;\n", field.m_name.c_str(), field.m_dimenIndex.c_str(), field.m_name.c_str(), field.m_dimenIndex.c_str());

				m_iplRegDecl.Append("\t%s r_%s%s;\n", field.m_type.c_str(), field.m_name.c_str(), field.m_dimenDecl.c_str());

				GenRamIndexLoops(iplReg, "", field);
				iplReg.Append("\tr_%s%s = c_%s%s;\n", field.m_name.c_str(), field.m_dimenIndex.c_str(), field.m_name.c_str(), field.m_dimenIndex.c_str());
			}
		}

		pIplTxStg->NewLine();
		m_iplRegDecl.NewLine();
		iplReg.NewLine();

		/////////////////////////////////
		// declare staged variables

		for (size_t fieldIdx = 0; fieldIdx < mod.m_stage.m_fieldList.size(); fieldIdx += 1) {
			CField &field = mod.m_stage.m_fieldList[fieldIdx];

			// declare combinatorial variables
			int stgIdx = (field.m_bInit ? 1 : 0) + field.m_rngLow.AsInt();
			int lastStgIdx = field.m_rngHigh.AsInt() + 1;
			for ( ; stgIdx <= lastStgIdx; stgIdx += 1) {
				int varStg = mod.m_tsStg + stgIdx - 1;
				if (stgIdx != field.m_rngLow.AsInt()) {
					const char *pNoLoad = stgIdx == field.m_rngHigh.AsInt()+1 ? " ht_noload" : "";

					if (field.m_dimenList.size() > 0) {
						m_iplRegDecl.Append("\t%s%s c_t%d_%s%s;\n",
							field.m_type.c_str(), pNoLoad, 
							varStg, field.m_name.c_str(), field.m_dimenDecl.c_str());
					} else {
						m_iplRegDecl.Append("\t%s%s c_t%d_%s;\n",
							field.m_type.c_str(), pNoLoad, varStg, field.m_name.c_str());

						iplTsStg.Append("\tc_t%d_%s = r_t%d_%s;\n",
							varStg, field.m_name.c_str(), varStg, field.m_name.c_str());
					}
				} else {
					if (field.m_dimenList.size() > 0) {
						m_iplRegDecl.Append("\t%s c_t%d_%s%s;\n",
							field.m_type.c_str(), 
							varStg, field.m_name.c_str(), field.m_dimenDecl.c_str());
					} else {
						m_iplRegDecl.Append("\t%s c_t%d_%s;\n",
							field.m_type.c_str(), varStg, field.m_name.c_str());
						if (field.m_bZero)
							iplTsStg.Append("\tc_t%d_%s = 0;\n",
								varStg, field.m_name.c_str());
					}
				}
			}
			iplTsStg.Append("\n");

			// Initialize dimensioned combinatorial variables
			const char *pTabs = "";
			if (field.m_dimenList.size() > 0) {
				GenRamIndexLoops(iplTsStg, "", field, true);

				stgIdx = (field.m_bInit ? 1 : 0) + field.m_rngLow.AsInt();
				for ( ; stgIdx <= field.m_rngHigh.AsInt()+1; stgIdx += 1) {
					int varStg = mod.m_tsStg + stgIdx - 1;
					if (stgIdx != field.m_rngLow.AsInt()) {
						iplTsStg.Append("%s\tc_t%d_%s%s = r_t%d_%s%s;\n", pTabs,
							varStg, field.m_name.c_str(), field.m_dimenIndex.c_str(),
							varStg, field.m_name.c_str(), field.m_dimenIndex.c_str());
					} else if (field.m_bZero) {
						iplTsStg.Append("%s\tc_t%d_%s%s = 0;\n", pTabs,
							varStg, field.m_name.c_str(), field.m_dimenIndex.c_str());
					}
				}

				pTabs = field.m_dimenTabs.c_str();

				iplTsStg.Append("%s}\n", pTabs);
				iplTsStg.Append("\n");
			}

			char resetStr[64];
			if (field.m_bReset)
				sprintf(resetStr, "c_reset1x ? (%s)0 : ", field.m_type.c_str());
			else
				resetStr[0] = '\0';

			pTabs = "";
			for (stgIdx = field.m_rngLow.AsInt(); stgIdx <= field.m_rngHigh.AsInt(); stgIdx += 1) {
				int varStg = mod.m_tsStg + stgIdx - 1;

				m_iplRegDecl.Append("\t%s r_t%d_%s%s;\n",
					field.m_type.c_str(), varStg+1, field.m_name.c_str(), field.m_dimenDecl.c_str());

				if (stgIdx == field.m_rngLow.AsInt())
					GenRamIndexLoops(iplReg, "", field, field.m_dimenList.size() > 0);

				if (field.m_bInit && stgIdx == field.m_rngLow.AsInt())
					iplReg.Append("%s\tr_t%d_%s%s = %sc_t%d_htPriv.m_%s%s;\n", pTabs,
					varStg+1, field.m_name.c_str(), field.m_dimenIndex.c_str(),
					resetStr,
					mod.m_tsStg, field.m_name.c_str(), field.m_dimenIndex.c_str());
				else
					iplReg.Append("%s\tr_t%d_%s%s = %sc_t%d_%s%s;\n", pTabs,
					varStg+1, field.m_name.c_str(), field.m_dimenIndex.c_str(),
					resetStr,
					varStg, field.m_name.c_str(), field.m_dimenIndex.c_str());

				if (field.m_dimenList.size() > 0) {
					pTabs = field.m_dimenTabs.c_str();
					if (stgIdx == field.m_rngHigh.AsInt())
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

		if (mod.m_mif.m_bMifRd) {
			m_iplRegDecl.Append("\tbool ht_noload c_t%d_bReadMemBusy;\n", mod.m_execStg);
			m_iplRegDecl.Append("\tbool ht_noload c_t%d_bReadMemAvail;\n", mod.m_execStg);
			GenModTrace(eVcdUser, vcdModName, "ReadMemBusy()", VA("c_t%d_bReadMemBusy", mod.m_execStg));
			pIplTxStg->Append("\tc_t%d_bReadMemAvail = false;\n", mod.m_execStg);
		}

		if (mod.m_mif.m_bMifWr) {
			m_iplRegDecl.Append("\tbool ht_noload c_t%d_bWriteMemBusy;\n", mod.m_execStg);
			m_iplRegDecl.Append("\tbool ht_noload c_t%d_bWriteMemAvail;\n", mod.m_execStg);
			GenModTrace(eVcdUser, vcdModName, "WriteMemBusy()", VA("c_t%d_bWriteMemBusy", mod.m_execStg));
			pIplTxStg->Append("\tc_t%d_bWriteMemAvail = false;\n", mod.m_execStg);
		}

		if (mod.m_ohm.m_bOutHostMsg) {
			m_iplRegDecl.Append("\tbool ht_noload c_bSendHostMsgBusy;\n");
			m_iplRegDecl.Append("\tbool ht_noload c_bSendHostMsgAvail;\n");
			GenModTrace(eVcdUser, vcdModName, "SendHostMsgBusy()", "c_bSendHostMsgBusy");
			pIplTxStg->Append("\tc_bSendHostMsgAvail = false;\n");
		}
	}

	iplStg += 1;
	sprintf(inStgStr, "r_t%d", mod.m_wrStg-1);
	sprintf(outStgStr, "c_t%d", mod.m_wrStg-1);
	sprintf(regStgStr, "r_t%d", mod.m_wrStg);

	////////////////////////////////////////////////////////////////////////////
	// Variable macro/references

	bool bFirstModVar = true;

	if (!mod.m_bHtId) {
		m_iplDefines.Append("#if !defined(TOPOLOGY_HEADER)\n");
		for (int privWrIdx = 1; privWrIdx <= mod.m_stage.m_privWrStg.AsInt(); privWrIdx += 1) {
			char privIdxStr[8];
			if (mod.m_stage.m_privWrStg.AsInt() > 1)
				sprintf(privIdxStr, "%d", privWrIdx);
			else
				privIdxStr[0] = '\0';

			m_iplDefines.Append("#define PR%s_htId 0\n", privIdxStr);
		}
		m_iplDefines.Append("#if defined(_HTV)\n");
	} else
		m_iplDefines.Append("#if !defined(TOPOLOGY_HEADER) && defined(_HTV)\n");

	for (int privWrIdx = 1; privWrIdx <= mod.m_stage.m_privWrStg.AsInt(); privWrIdx += 1) {

		int privStg = mod.m_tsStg + privWrIdx - 1;

		char privIdxStr[8];
		if (mod.m_stage.m_privWrStg.AsInt() > 1) {
			g_appArgs.GetDsnRpt().AddLevel("Private Variables - Stage %d\n", privWrIdx);
			sprintf(privIdxStr, "%d", privWrIdx);
		} else {
			g_appArgs.GetDsnRpt().AddLevel("Private Variables\n");
			privIdxStr[0] = '\0';
		}

		if (g_appArgs.IsVariableReportEnabled()) {
			fprintf(g_appArgs.GetVarRptFp(), "%s PR%s_htId r_t%d_htId\n",
				mod.m_modName.Uc().c_str(),
				privIdxStr,
				privStg);
		}

		g_appArgs.GetDsnRpt().AddLevel("Read Only (PR_)\n");

		// PR_htId macro
		if (mod.m_bHtId) {
			g_appArgs.GetDsnRpt().AddItem("sc_uint<%s> PR%s_htId\n", mod.m_threads.m_htIdW.c_str(), privIdxStr);
			GenModVar(eVcdUser, vcdModName, bFirstModVar,
				VA("sc_uint<%s_HTID_W> const", mod.m_modName.Upper().c_str()),
				"", // dimen
				VA("PR%s_htId", privIdxStr), VA("r_t%d_htId", privStg));
		} else {
			g_appArgs.GetDsnRpt().AddItem("int PR%s_htId\n", privIdxStr);
		}

		g_appArgs.GetDsnRpt().AddItem("sc_uint<%s_INSTR_W> PR%s_htInstr\n", mod.m_modName.Upper().c_str(), privIdxStr);
		GenModVar(eVcdUser, vcdModName, bFirstModVar, 
			VA("sc_uint<%s_INSTR_W> const", mod.m_modName.Upper().c_str()),
			"", // dimen
			VA("PR%s_htInst", privIdxStr), VA("r_t%d_htInstr", privStg));
		GenModVar(eVcdUser, vcdModName, bFirstModVar, 
			VA("sc_uint<%s_INSTR_W> const", mod.m_modName.Upper().c_str()),
			"", // dimen
			VA("PR%s_htInstr", privIdxStr), VA("r_t%d_htInstr", privStg));
		m_iplDefines.Append("\n");

		// Private variable access macros
		g_appArgs.GetDsnRpt().AddItem("bool  PR%s_htValid\n", privIdxStr);
		GenModVar(eVcdUser, vcdModName, bFirstModVar, 
			"bool const", "", VA("PR%s_htValid", privIdxStr), VA("r_t%d_htValid", privStg));

		if (mod.m_threads.m_htPriv.m_fieldList.size() == 0) continue;

		// PR variables
		bool bWriteOnly = false;
		for (size_t privIdx = 0; privIdx < mod.m_threads.m_htPriv.m_fieldList.size(); privIdx += 1) {
			CField & priv = mod.m_threads.m_htPriv.m_fieldList[privIdx];

			if (priv.m_pPrivGbl == 0) {
				g_appArgs.GetDsnRpt().AddItem("%s PR%s_%s%s\n", priv.m_type.c_str(), privIdxStr, priv.m_name.c_str(), priv.m_dimenDecl.c_str());
			} else {
				bWriteOnly = true;
				CRam * pGbl = priv.m_pPrivGbl;
				CField &field = pGbl->m_fieldList[0];

				g_appArgs.GetDsnRpt().AddItem("%s PR%s_%s%s\n", field.m_type.c_str(), privIdxStr, priv.m_name.c_str(), priv.m_dimenDecl.c_str());
			}
		}

		g_appArgs.GetDsnRpt().EndLevel();
		g_appArgs.GetDsnRpt().AddLevel("Read/Write (P_)\n");

		// P variables
		for (size_t privIdx = 0; privIdx < mod.m_threads.m_htPriv.m_fieldList.size(); privIdx += 1) {
			CField & priv = mod.m_threads.m_htPriv.m_fieldList[privIdx];

			if (priv.m_pPrivGbl != 0)
				continue;

			g_appArgs.GetDsnRpt().AddItem("%s P%s_%s%s\n", priv.m_type.c_str(), privIdxStr, priv.m_name.c_str(), priv.m_dimenDecl.c_str());
		}

		g_appArgs.GetDsnRpt().EndLevel();

		if (bWriteOnly) {
			g_appArgs.GetDsnRpt().AddLevel("Write Only (PW_)\n");
			m_iplFuncDecl.NewLine();

			for (size_t privIdx = 0; privIdx < mod.m_threads.m_htPriv.m_fieldList.size(); privIdx += 1) {
				CField & priv = mod.m_threads.m_htPriv.m_fieldList[privIdx];

				if (priv.m_pPrivGbl == 0)
					continue;

				CRam * pGbl = priv.m_pPrivGbl;
				CField &field = pGbl->m_fieldList[0];

				int wrStg = mod.m_tsStg + pGbl->m_wrStg.AsInt() - 1;

				char varWrStg[64] = { '\0' };
				if (mod.m_stage.m_privWrStg.AsInt() > 1)
				//if (pGbl->m_wrStg.size() > 0)
					sprintf(varWrStg, "%d", pGbl->m_wrStg.AsInt());
				//else if (mod.m_stage.m_bStageNums)
				//	sprintf(varWrStg, "%d", mod.m_stage.m_privWrStg.AsInt());

				string apiAddrParams;
				string addrParams;
				string addrValue;
				char tmp[128];

				char htIdStr[32];
				sprintf(htIdStr, "r_t%d_htId", wrStg);

				if (pGbl->m_addr0W.AsInt() > 0 && pGbl->m_addr1W.AsInt() > 0 ||
					pGbl->m_addr1W.AsInt() > 0 && pGbl->m_addr2W.AsInt() > 0)
					addrValue += "(";
				if (pGbl->m_addr0W.AsInt() > 0)
					addrValue += htIdStr;

				if (pGbl->m_addr1W.AsInt() > 0) {
					sprintf(tmp, "ht_uint%d wrAddr1, ", pGbl->m_addr1W.AsInt());
					apiAddrParams += tmp;

					addrParams += "wrAddr1, ";
					if (pGbl->m_addr0W.AsInt() > 0)
						addrValue += ", ";
					addrValue += "wrAddr1";
				}
				if (pGbl->m_addr2W.AsInt() > 0) {
					sprintf(tmp, "ht_uint%d wrAddr2, ", pGbl->m_addr2W.AsInt());
					apiAddrParams += tmp;

					addrParams += "wrAddr2, ";
					addrValue += ", wrAddr2";
				}

				if (pGbl->m_addr0W.AsInt() > 0 && pGbl->m_addr1W.AsInt() > 0 ||
					pGbl->m_addr1W.AsInt() > 0 && pGbl->m_addr2W.AsInt() > 0)
					addrValue += ")";

				string apiFldIdxParams;
				const char *pFldIndexes;
				HtlAssert(field.m_dimenList.size() <= 2);
				if (field.m_dimenList.size() == 2) {
					sprintf(tmp, "ht_uint%d fldIdx1, ht_uint%d fldIdx2, ",
						FindLg2(field.m_dimenList[0].AsInt()), FindLg2(field.m_dimenList[1].AsInt()));
					apiFldIdxParams += tmp;

					pFldIndexes = "[fldIdx1][fldIdx2]";
				} else if (field.m_dimenList.size() == 1) {
					sprintf(tmp, "ht_uint%d fldIdx1, ",
						FindLg2(field.m_dimenList[0].AsInt()));
					apiFldIdxParams += tmp;

					pFldIndexes = "[fldIdx]";
				} else {
					pFldIndexes = "";
				}

				g_appArgs.GetDsnRpt().AddItem("void PW%s_%s(%s%s%s %s)\n",
					varWrStg, priv.m_name.c_str(),
					apiAddrParams.c_str(), apiFldIdxParams.c_str(), field.m_type.c_str(), field.m_name.c_str());

				m_iplFuncDecl.Append("\tvoid PW%s_%s(%s%s%s %s);\n",
					varWrStg, priv.m_name.c_str(),
					apiAddrParams.c_str(), apiFldIdxParams.c_str(), field.m_type.c_str(), field.m_name.c_str());

				m_iplMacros.Append("void CPers%s%s::PW%s_%s(%s%s%s %s)\n",
					mod.m_modName.Uc().c_str(), instIdStr.c_str(), varWrStg, priv.m_name.c_str(),
					apiAddrParams.c_str(), apiFldIdxParams.c_str(), field.m_type.c_str(), field.m_name.c_str());
				m_iplMacros.Append("{\n");
				if (addrValue.size() > 0)
					m_iplMacros.Append("\tc_t%d_%sWrAddr = %s;\n",
						wrStg, priv.m_name.c_str(), addrValue.c_str());
				m_iplMacros.Append("\tc_t%d_%sWrEn.m_%s%s = true;\n",
					wrStg, priv.m_name.c_str(), field.m_name.c_str(), pFldIndexes);
				m_iplMacros.Append("\tc_t%d_%sWrData.m_%s%s = %s;\n",
					wrStg, priv.m_name.c_str(), field.m_name.c_str(), pFldIndexes, field.m_name.c_str());
				m_iplMacros.Append("}\n");
				m_iplMacros.Append("\n");
			}
			g_appArgs.GetDsnRpt().EndLevel();
		}

		int privPos = 0;
		for (size_t privIdx = 0; privIdx < mod.m_threads.m_htPriv.m_fieldList.size(); privIdx += 1) {
			CField & priv = mod.m_threads.m_htPriv.m_fieldList[privIdx];

			if (priv.m_pPrivGbl == 0) {
				GenModVar(eVcdUser, vcdModName, bFirstModVar,
					VA("%s const", priv.m_type.c_str()),
					priv.m_dimenDecl,
					VA("PR%s_%s", privIdxStr, priv.m_name.c_str()),
					VA("r_t%d_htPriv.m_%s", privStg, priv.m_name.c_str()),
					priv.m_dimenList);
				GenModVar(eVcdNone, vcdModName, bFirstModVar,
					VA("%s", priv.m_type.c_str()),
					priv.m_dimenDecl,
					VA("P%s_%s", privIdxStr, priv.m_name.c_str()),
					VA("c_t%d_htPriv.m_%s", privStg, priv.m_name.c_str()),
					priv.m_dimenList);
			} else {
				GenModVar(eVcdUser, vcdModName, bFirstModVar,
					VA("%s const", priv.m_type.c_str()), 
					priv.m_dimenDecl,
					VA("PR%s_%s", privIdxStr, priv.m_name.c_str()),
					VA("r_t%d_%sRdData.m_data", privStg, priv.m_name.c_str()),
					priv.m_dimenList);
			}

			if (g_appArgs.IsVariableReportEnabled()) {

				int privWidth = FindTypeWidth(priv);

				vector<int> refList(priv.m_dimenList.size());

				do {
					string dimIdx = IndexStr(refList);

					fprintf(g_appArgs.GetVarRptFp(), "%s PR%s_%s%s r_t%d_htPriv[%d:%d]\n",
						mod.m_modName.Uc().c_str(),
						privIdxStr, priv.m_name.c_str(), dimIdx.c_str(),
						privStg,
						privPos+privWidth-1, privPos);

					privPos += privWidth;

				} while (DimenIter(priv.m_dimenList, refList));
			}

			m_iplDefines.Append("\n");
		}

		g_appArgs.GetDsnRpt().EndLevel();
	}

	// staged variables
	if (mod.m_stage.m_fieldList.size() > 0) {

		g_appArgs.GetDsnRpt().AddLevel("Stage Variables\n");
		g_appArgs.GetDsnRpt().AddLevel("Read Only (TR_)\n");

		for (size_t fieldIdx = 0; fieldIdx < mod.m_stage.m_fieldList.size(); fieldIdx += 1) {
			CField &field = mod.m_stage.m_fieldList[fieldIdx];

			int stgIdx = (field.m_bInit ? 1 : 0) + field.m_rngLow.AsInt();
			int highIdx = field.m_rngHigh.AsInt() + 1;

			for ( ; stgIdx < highIdx; stgIdx += 1) {
				g_appArgs.GetDsnRpt().AddItem("%s TR%d_%s%s\n", field.m_type.c_str(),
					stgIdx+1, field.m_name.c_str(), field.m_dimenDecl.c_str());
			}
		}

		g_appArgs.GetDsnRpt().EndLevel();
		g_appArgs.GetDsnRpt().AddLevel("Read/Write (T_)\n");

		for (size_t fieldIdx = 0; fieldIdx < mod.m_stage.m_fieldList.size(); fieldIdx += 1) {
			CField &field = mod.m_stage.m_fieldList[fieldIdx];

			int stgIdx = (field.m_bInit ? 1 : 0) + field.m_rngLow.AsInt();
			int highIdx = field.m_rngHigh.AsInt() + 1;

			for ( ; stgIdx < highIdx+1; stgIdx += 1) {
				g_appArgs.GetDsnRpt().AddItem("%s T%d_%s%s\n", field.m_type.c_str(),
					stgIdx, field.m_name.c_str(), field.m_dimenDecl.c_str());
			}
		}

		g_appArgs.GetDsnRpt().EndLevel();
		g_appArgs.GetDsnRpt().EndLevel();

		for (size_t fieldIdx = 0; fieldIdx < mod.m_stage.m_fieldList.size(); fieldIdx += 1) {
			CField &field = mod.m_stage.m_fieldList[fieldIdx];

			int stgIdx = (field.m_bInit ? 1 : 0) + field.m_rngLow.AsInt();
			int highIdx = field.m_rngHigh.AsInt() + 1;
			int varStg;
			for ( ; stgIdx < highIdx; stgIdx += 1) {
				varStg = mod.m_tsStg + stgIdx - 1;

				GenModVar(eVcdNone, vcdModName, bFirstModVar,
					VA("%s", field.m_type.c_str()), 
					field.m_dimenDecl,
					VA("T%d_%s", stgIdx, field.m_name.c_str()),
					VA("c_t%d_%s", varStg, field.m_name.c_str()),
					field.m_dimenList);
				GenModVar(eVcdUser, vcdModName, bFirstModVar,
					VA("%s const", field.m_type.c_str()), 
					field.m_dimenDecl,
					VA("TR%d_%s", stgIdx+1, field.m_name.c_str()),
					VA("r_t%d_%s", varStg+1, field.m_name.c_str()),
					field.m_dimenList);
			}

			varStg = mod.m_tsStg + stgIdx - 1;
			GenModVar(eVcdUser, vcdModName, bFirstModVar,
				VA("%s", field.m_type.c_str()),
				field.m_dimenDecl,
				VA("T%d_%s", stgIdx, field.m_name.c_str()),
				VA("c_t%d_%s", varStg, field.m_name.c_str()),
				field.m_dimenList);
		}

		m_iplDefines.Append("\n");
	}

	{
		// Shared variable access macros
		if (mod.m_shared.m_fieldList.size() > 0) {

			g_appArgs.GetDsnRpt().AddLevel("Shared Variables\n");

			bool bFirst = true;
			for (size_t shIdx = 0; shIdx < mod.m_shared.m_fieldList.size(); shIdx += 1) {
				CField &shared = mod.m_shared.m_fieldList[shIdx];

				if (shared.m_queueW.AsInt() == 0/* && shared.m_addr1W.AsInt() == 0*/) {
					if (bFirst) {
						g_appArgs.GetDsnRpt().AddLevel("Read Only (SR_)\n");
						bFirst = false;
					}
					g_appArgs.GetDsnRpt().AddItem("%s ", shared.m_type.c_str());

					g_appArgs.GetDsnRpt().AddText("SR_%s%s\n",
						shared.m_name.c_str(), shared.m_dimenDecl.c_str());
				}
			}

			if (!bFirst)
				g_appArgs.GetDsnRpt().EndLevel();

			g_appArgs.GetDsnRpt().AddLevel("Read/Write (S_)\n");

			for (size_t shIdx = 0; shIdx < mod.m_shared.m_fieldList.size(); shIdx += 1) {
				CField &shared = mod.m_shared.m_fieldList[shIdx];

				if (shared.m_bIhmReadOnly && shared.m_queueW.size() == 0)
					continue;

				if (shared.m_rdSelW.AsInt() > 0) {

					g_appArgs.GetDsnRpt().AddItem("ht_mrd_block_ram<type=%s, AW1=%d",
						shared.m_type.c_str(), shared.m_addr1W.AsInt());
					if (shared.m_addr2W.size() > 0)
						g_appArgs.GetDsnRpt().AddText(", AW2=%d", shared.m_addr2W.AsInt());
					g_appArgs.GetDsnRpt().AddText("> ");

				} else if (shared.m_wrSelW.AsInt() > 0) {

					g_appArgs.GetDsnRpt().AddItem("ht_mwr_block_ram<type=%s, AW1=%d",
						shared.m_type.c_str(), shared.m_addr1W.AsInt());
					if (shared.m_addr2W.size() > 0)
						g_appArgs.GetDsnRpt().AddText(", AW2=%d", shared.m_addr2W.AsInt());
					g_appArgs.GetDsnRpt().AddText("> ");

				} else if (shared.m_addr1W.size() > 0) {

					const char *pScMem = shared.m_ramType == eDistRam ? "ht_dist_ram" : "ht_block_ram";
					if (shared.m_addr2W.size() > 0)
						g_appArgs.GetDsnRpt().AddItem("%s<%s, %s, %s> ", pScMem, shared.m_type.c_str(),
							shared.m_addr1W.c_str(), shared.m_addr2W.c_str());
					else
						g_appArgs.GetDsnRpt().AddItem("%s<%s, %s> ", pScMem, shared.m_type.c_str(),
							shared.m_addr1W.c_str());

				} else if (shared.m_queueW.size() > 0) {

					const char *pScMem = shared.m_ramType == eDistRam ? "ht_dist_que" : "ht_block_que";
					g_appArgs.GetDsnRpt().AddItem("%s<%s, %s> ", pScMem, shared.m_type.c_str(),
						shared.m_queueW.c_str());

				} else {

					g_appArgs.GetDsnRpt().AddItem("%s ", shared.m_type.c_str());
				}

				g_appArgs.GetDsnRpt().AddText("S_%s%s\n",
					shared.m_name.c_str(), shared.m_dimenDecl.c_str());
			}
			g_appArgs.GetDsnRpt().EndLevel();
			g_appArgs.GetDsnRpt().EndLevel();

			for (size_t shIdx = 0; shIdx < mod.m_shared.m_fieldList.size(); shIdx += 1) {
				CField &shared = mod.m_shared.m_fieldList[shIdx];

				char preCh = 'c';
				char regCh = 'r';
				if (shared.m_queueW.AsInt() > 0 || shared.m_addr1W.AsInt() > 0) {
					preCh = 'm';
					regCh = 'm';
				}

				if (!shared.m_bIhmReadOnly || shared.m_queueW.size() > 0)
					GenModVar(eVcdNone, vcdModName, bFirstModVar,
						GenFieldType(shared, false),
						shared.m_dimenDecl,
						VA("S_%s", shared.m_name.c_str()),
						VA("%c_%s", preCh, shared.m_name.c_str()),
						shared.m_dimenList);

				if (/*preCh == 'c'*/shared.m_queueW.size() == 0)
					GenModVar(eVcdUser, vcdModName, bFirstModVar,
						GenFieldType(shared, true),
						shared.m_dimenDecl,
						VA("SR_%s", shared.m_name.c_str()),
						VA("%c_%s", regCh, shared.m_name.c_str()),
						shared.m_dimenList);
			}

		}

		if (mod.m_mif.m_mifRd.m_bPause && mod.m_mif.m_mifRd.m_rspGrpIdW <= 2)
			iplPostInstr.Append("\tc_t%d_memReadPause = false;\n", mod.m_execStg);
		if (mod.m_mif.m_mifWr.m_bPause && mod.m_mif.m_mifWr.m_rspGrpIdW <= 2)
			iplPostInstr.Append("\tc_t%d_memWritePause = false;\n", mod.m_execStg);

		m_iplRegDecl.Append("\tCHtAssertIntf c_htAssert;\n");
		iplPostInstr.Append("\tc_htAssert = 0;\n");
		if (mod.m_clkRate == eClk2x) {
			iplPostInstr.Append("#ifdef HT_ASSERT\n");
			iplPostInstr.Append("\tif (r_%sToHta_assert.read().m_bAssert && r_phase && !c_reset1x)\n", mod.m_modName.Lc().c_str());
			iplPostInstr.Append("\t\tc_htAssert = r_%sToHta_assert;\n", mod.m_modName.Lc().c_str());
			iplPostInstr.Append("#endif\n");
		}
		iplPostInstr.Append("\n");

		////////////////////////////////////////////////////////////////////////////
		// Instruction execution

		iplPostInstr.Append("\tPers%s%s();\n", 
			unitNameUc.c_str(), mod.m_modName.Uc().c_str());
		iplPostInstr.Append("\n");
		iplPostInstr.Append("\tassert_msg(!r_t%d_htValid || c_t%d_htCtrl != HT_INVALID, \"Runtime check failed in CPers%s::Pers%s_%s()"
			" - expected an Ht control routine to have been called\");\n", 
			mod.m_execStg, mod.m_execStg,
			mod.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), mod.m_clkRate == eClk1x ? "1x" : "2x");
		iplPostInstr.Append("\n");

		////////////////////////////////////////////////////////////////////////////
		// Post instruction stage

		if (m_bPerfMon) {
			iplPostInstr.Append("#ifndef _HTV\n");
			iplPostInstr.Append("\tif (");
			iplPostInstr.Append("r_t%d_htValid", mod.m_execStg);
			iplPostInstr.Append(")\n", mod.m_execStg);
			if (mod.m_bHasThreads)
				iplPostInstr.Append("\t\tHt::g_syscMon.UpdateValidInstrCnt(m_htMonModId, (int)r_t%d_htInstr);\n", mod.m_execStg);
			else
				iplPostInstr.Append("\t\tHt::g_syscMon.UpdateValidInstrCnt(m_htMonModId, (int)r_t%d_htInstr);\n", mod.m_execStg);
			iplPostInstr.Append("\n");

			iplPostInstr.Append("\tif (c_t%d_htCtrl == HT_RETRY)\n", mod.m_execStg);
			if (mod.m_bHasThreads)
				iplPostInstr.Append("\t\tHt::g_syscMon.UpdateRetryInstrCnt(m_htMonModId, (int)r_t%d_htInstr);\n", mod.m_execStg);
			else
				iplPostInstr.Append("\t\tHt::g_syscMon.UpdateRetryInstrCnt(m_htMonModId, (int)r_t%d_htInstr);\n", mod.m_execStg);
			iplPostInstr.Append("\n");

			if (mod.m_mif.m_bMif) {
				CMif & mif = mod.m_mif;

				bool is_wx = g_appArgs.GetCoproc() == wx690 ||
					g_appArgs.GetCoproc() == wx2k;
				bool bNeed_reqBusy = mod.m_mif.m_mifWr.m_bMultiWr 
					|| !is_wx && mod.m_mif.m_mifRd.m_bMultiRd;

				char reqBusy[32];
				if (bNeed_reqBusy)
					sprintf(reqBusy, " && !r_t%d_reqBusy", mod.m_execStg);
				else
					reqBusy[0] = '\0';

				if (mif.m_bMifRd) {
					if (mif.m_bMifWr)
						iplPostInstr.Append("\tif (c_t%d_%sToMif_reqRdy%s && c_t%d_bReadMem) {\n",
						mod.m_execStg, mod.m_modName.Lc().c_str(), reqBusy, mod.m_execStg);
					else
						iplPostInstr.Append("\tif (c_t%d_%sToMif_reqRdy%s) {\n",
						mod.m_execStg, mod.m_modName.Lc().c_str(), reqBusy);

					if (mif.m_mifRd.m_bMultiRd) {
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemReads(m_htMonModId, 0, 1);\n");
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemRead64s(m_htMonModId, 0, c_t%d_reqQwCnt > 1 ? 1 : 0);\n", mod.m_execStg);
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemReadBytes(m_htMonModId, 0, 8 * c_t%d_reqQwCnt);\n", mod.m_execStg);
					} else {
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemReads(m_htMonModId, 0, 1);\n");
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemReadBytes(m_htMonModId, 0, 8);\n");
					}
					iplPostInstr.Append("\t}\n");
					iplPostInstr.Append("\n");
				}

				if (mif.m_bMifWr) {
					if (mif.m_bMifRd)
						iplPostInstr.Append("\tif (c_t%d_%sToMif_reqRdy%s && !c_t%d_bReadMem) {\n",
						mod.m_execStg, mod.m_modName.Lc().c_str(), reqBusy, mod.m_execStg);
					else
						iplPostInstr.Append("\tif (c_t%d_%sToMif_reqRdy%s) {\n",
						mod.m_execStg, mod.m_modName.Lc().c_str(), reqBusy);

					if (mif.m_mifWr.m_bMultiWr) {
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemWrites(m_htMonModId, 0, c_t%d_reqQwCnt == 1 ? 1 : 0);\n", mod.m_execStg);
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemWrite64s(m_htMonModId, 0, c_t%d_reqQwCnt > 1 ? 1 : 0);\n", mod.m_execStg);
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemWriteBytes(m_htMonModId, 0, (8 << c_t%d_%sToMif_req.m_size) * c_t%d_reqQwCnt);\n", 
							mod.m_execStg, mod.m_modName.Lc().c_str(), mod.m_execStg);
					} else {
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemWrites(m_htMonModId, 0, 1);\n");
						iplPostInstr.Append("\t\tHt::g_syscMon.UpdateModuleMemWriteBytes(m_htMonModId, 0, 8 << c_t%d_%sToMif_req.m_size);\n",
							mod.m_execStg, mod.m_modName.Lc().c_str());
					}
					iplPostInstr.Append("\t}\n");
					iplPostInstr.Append("\n");
				}
			}

			if (mod.m_threads.m_htIdW.AsInt() == 0) {
				iplPostInstr.Append("\t\tHt::g_syscMon.UpdateActiveClks(m_htMonModId, r_htBusy ? 1 : 0);\n");

			} else {
				iplPostInstr.Append("\tif (!m_htIdPool.full() && r_resetComplete) {\n");
				iplPostInstr.Append("\t\tuint32_t activeCnt = (1ul << %s_HTID_W) - m_htIdPool.size();\n",
					mod.m_modName.Upper().c_str());
				iplPostInstr.Append("\t\tHt::g_syscMon.UpdateActiveCnt(m_htMonModId, activeCnt);\n");
				iplPostInstr.Append("\t}\n");
				iplPostInstr.Append("\n");

				iplPostInstr.Append("\tuint32_t runningCnt = m_htCmdQue.size() + (r_t%d_htValid ? 1 : 0);\n",
					mod.m_execStg);
				iplPostInstr.Append("\tHt::g_syscMon.UpdateRunningCnt(m_htMonModId, runningCnt);\n");
			}

			iplPostInstr.Append("#endif\n");
			iplPostInstr.Append("\n");
		}

		if (g_appArgs.IsInstrTraceEnabled()) {
			iplPostInstr.Append("#ifndef _HTV\n");
			iplPostInstr.Append("\tif (Ht::g_instrTraceFp && (");
			iplPostInstr.Append("r_t%d_htValid", mod.m_execStg);
			iplPostInstr.Append("))\n");
			iplPostInstr.Append("\t{\n");

			string line = "\t\tfprintf(Ht::g_instrTraceFp, \"" + mod.m_modName.Upper();
			if (mod.m_modInstList.size() > 1)
				line += "%%d:";
			else
				line += ":";
			if (mod.m_bMultiThread)
				line += " HtId=0x%%x,";
			line += " Instr=%%-%ds, Ctrl=%%-9s -> Instr=%%-%ds";
			for (size_t i = 0; i < mod.m_traceList.size(); i += 1)
				line += ", " + mod.m_traceList[i] + "=0x%%llx";
			line += " @ %%lld\\n\",\n";
			iplPostInstr.Append(line.c_str(), maxInstrLen, maxInstrLen);

			char lineStr[128];
			line = "\t\t\t";
			if (mod.m_modInstList.size() > 1)
				line += "(int)i_instId.read(), ";
			if (mod.m_bMultiThread) {
				sprintf(lineStr, "(int)r_t%d_htId, ", mod.m_execStg);
				line += lineStr;
			}
			sprintf(lineStr, "m_pInstrNames[(int)r_t%d_htInstr], m_pHtCtrlNames[(int)c_t%d_htCtrl], m_pInstrNames[(int)c_t%d_htNextInstr], ",
				mod.m_execStg, mod.m_execStg, mod.m_execStg);
			line += lineStr;

			for (size_t i = 0; i < mod.m_traceList.size(); i += 1) {
				sprintf(lineStr, "(long long)c_t%d_htPriv.", mod.m_execStg);
				line += lineStr + mod.m_traceList[i] + ", ";
			}
			line += "(long long)sc_time_stamp().value()/10000);\n";
			iplPostInstr.Append(line.c_str());

			iplPostInstr.Append("\t\tfflush(Ht::g_instrTraceFp);\n");
			iplPostInstr.Append("\t}\n");
			iplPostInstr.Append("#endif\n");
			iplPostInstr.Append("\n");
		}

		if (mod.m_ohm.m_bOutHostMsg) {
			iplPostInstr.Append("\tassert_msg(!c_%sToHti_ohm.m_bValid || c_bSendHostMsgAvail, \"Runtime check failed in CPers%s::Pers%s_%s()"
				" - expected SendHostMsgBusy() to have been called and not busy\");\n", mod.m_modName.Lc().c_str(), mod.m_modName.Uc().c_str(),
				mod.m_modName.Uc().c_str(), mod.m_clkRate == eClk1x ? "1x" : "2x");
		}

		iplPostInstr.Append("\n");

		char regStr[8];
		pRegStg = regStr;
		sprintf(regStr, "r_t%d", mod.m_wrStg);

		// htCtrl
		if (mod.m_bMultiThread) {
			iplPostInstr.Append("\tbool c_t%d_htPrivWe = false;\n", mod.m_wrStg-1);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("r_t%d_htPrivWe", mod.m_wrStg) );
			iplReg.Append("\tr_t%d_htPrivWe = c_t%d_htPrivWe;\n", mod.m_wrStg, mod.m_wrStg-1);

			iplPostInstr.Append("\tbool c_t%d_htCmdRdy = false;\n", mod.m_wrStg-1);
			GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("r_t%d_htCmdRdy", mod.m_wrStg) );
			iplReg.Append("\tr_t%d_htCmdRdy = c_t%d_htCmdRdy;\n", mod.m_wrStg, mod.m_wrStg-1);

			iplPostInstr.Append("\tCHtCmd c_t%d_htCmd;\n", mod.m_wrStg-1);
		} else {
			iplPostInstr.Append("\tCHtPriv c_htPriv = r_htPriv;\n");
			iplPostInstr.Append("\tCHtCmd c_htCmd = r_htCmd;\n");

			if (mod.m_globalVarList.size() > 0) {
				GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "bool", VA("r_t%d_htCmdRdy", mod.m_wrStg));
				iplPostInstr.Append("\tbool c_t%d_htCmdRdy = false;\n", mod.m_wrStg - 1);
			}
		}

		if (mod.m_threads.m_htIdW.AsInt() == 0)
			iplReg.Append("\tr_htCmd = c_htCmd;\n");
		else {
			//GenModDecl(eVcdAll, m_iplRegDecl, vcdModName, "CHtCmd", VA("r_t%d_htCmd", mod.m_wrStg) );
			m_iplRegDecl.Append("\tCHtCmd r_t%d_htCmd;\n", mod.m_wrStg);
			iplReg.Append("\tr_t%d_htCmd = c_t%d_htCmd;\n", mod.m_wrStg, mod.m_wrStg-1);
		}

		if (mod.m_threads.m_resetInstr.size() > 0) {
			m_iplRegDecl.Append("\tbool r_t%d_htTerm;\n", mod.m_wrStg);
			iplReg.Append("\tr_t%d_htTerm = c_t%d_htTerm;\n", mod.m_wrStg, mod.m_wrStg-1);
			iplPostInstr.Append("\tbool c_t%d_htTerm = false;\n", mod.m_wrStg-1);
		}

		iplPostInstr.Append("\n");

		iplPostInstr.Append("\tswitch (r_t%d_htSel) {\n", mod.m_wrStg-1);

		if (mod.m_threads.m_resetInstr.size() > 0)
			iplPostInstr.Append("\tcase %s_HT_SEL_RESET:\n",
			mod.m_modName.Upper().c_str());

		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrOut) continue;
			if (cxrIntf.m_pDstGroup != &mod.m_threads) continue;

			iplPostInstr.Append("\tcase %s_HT_SEL_%s_%d_%s:\n",
				mod.m_modName.Upper().c_str(), cxrIntf.m_srcInstName.Upper().c_str(), cxrIntf.GetPortReplId(), cxrIntf.m_funcName.Upper().c_str());
		}

		iplPostInstr.Append("\tcase %s_HT_SEL_SM:\n",
			mod.m_modName.Upper().c_str());

		if (mod.m_bMultiThread) {
			iplPostInstr.Append("\t\tc_t%d_htPrivWe = r_t%d_htValid;\n",
			mod.m_wrStg-1, mod.m_wrStg-1);

			iplPostInstr.Append("\t\tc_t%d_htCmdRdy = c_t%d_htCtrl == HT_CONT || c_t%d_htCtrl == HT_JOIN_AND_CONT || c_t%d_htCtrl == HT_RETRY",
				mod.m_wrStg-1, mod.m_wrStg-1, mod.m_wrStg-1, mod.m_wrStg-1);

			iplPostInstr.Append(";\n");

			if (mod.m_threads.m_htIdW.AsInt() > 0)
				iplPostInstr.Append("\t\tc_t%d_htCmd.m_htId = r_t%d_htId;\n",
				mod.m_wrStg-1, mod.m_wrStg-1);
			iplPostInstr.Append("\t\tc_t%d_htCmd.m_htInstr = c_t%d_htNextInstr;\n",
				mod.m_wrStg-1, mod.m_wrStg-1);
		} else {
			if (mod.m_threads.m_bCallFork && mod.m_threads.m_htIdW.AsInt() == 0)
				iplPostInstr.Append("\t\tif (r_t%d_htValid) {\n", mod.m_wrStg-1);
			else
				iplPostInstr.Append("\t\tif (r_t%d_htValid)\n", mod.m_wrStg-1);

			iplPostInstr.Append("\t\t\tc_htPriv = c_t%d_htPriv;\n",
				mod.m_wrStg-1);

			if (mod.m_threads.m_bCallFork && mod.m_threads.m_htIdW.AsInt() == 0) {
				iplPostInstr.Append("\t\t\tc_htPrivLk = false;\n");
				iplPostInstr.Append("\t\t}\n");
			}

			iplPostInstr.Append("\t\tif (r_t%d_htValid && c_t%d_htCtrl != HT_JOIN) {\n",
				mod.m_wrStg-1, mod.m_wrStg-1);
			iplPostInstr.Append("\t\t\tc_htCmdValid = c_t%d_htCtrl == HT_CONT || c_t%d_htCtrl == HT_JOIN_AND_CONT || c_t%d_htCtrl == HT_RETRY;\n",
				mod.m_wrStg-1, mod.m_wrStg-1, mod.m_wrStg-1);
			if (mod.m_globalVarList.size() > 0)
				iplPostInstr.Append("\t\t\tc_t%d_htCmdRdy = c_htCmdValid;\n", mod.m_wrStg - 1);
			iplPostInstr.Append("\t\t\tc_htCmd.m_htInstr = c_t%d_htNextInstr;\n",
				mod.m_wrStg-1);
			iplPostInstr.Append("\t\t}\n");
		}

		if (mod.m_threads.m_resetInstr.size() > 0)
			iplPostInstr.Append("\t\tc_t%d_htTerm = c_t%d_htCtrl == HT_TERM;\n",
			mod.m_wrStg-1, mod.m_wrStg-1);

		iplPostInstr.Append("\t\tbreak;\n");

		iplPostInstr.Append("\tdefault:\n");
		iplPostInstr.Append("\t\tassert(0);\n");
		iplPostInstr.Append("\t}\n");
		iplPostInstr.Append("\n");

		if (mod.m_bCallFork) {

			iplPostInstr.Append("\tassert_msg(c_reset1x || !r_t%d_htValid || ", mod.m_wrStg-1);

			if (mod.m_bCallFork)
				iplPostInstr.Append("!r_t%d_rtnJoin || ", mod.m_wrStg-1);
			iplPostInstr.Append("c_t%d_htCtrl == HT_JOIN || c_t%d_htCtrl == HT_JOIN_AND_CONT,\n\t\t\"Runtime check failed in CPers%s::Pers%s_%s()"
				" - expected return from a forked call to performan HtJoin on the entry instruction\");\n",
				mod.m_wrStg-1, mod.m_wrStg-1,
				mod.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), mod.m_clkRate == eClk1x ? "1x" : "2x");

			iplPostInstr.Append("\n");
		}

		// decrement reserved limit counts
		if (mod.m_cxrEntryReserveCnt > 0) {
			iplPostInstr.Append("\tif (c_t%d_htCtrl == HT_RTN) {\n", mod.m_wrStg-1);
			iplPostInstr.Append("\t\tswitch (r_t%d_htPriv.m_rtnRsvSel) {\n", mod.m_wrStg-1);

			if (mod.m_threads.m_resetInstr.size() > 0) {
				iplPostInstr.Append("\t\tcase %s_HT_SEL_RESET:\n",
					mod.m_modName.Upper().c_str());
				iplPostInstr.Append("\t\t\tbreak;\n");
			}

			for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

				if (cxrIntf.m_cxrDir == CxrOut) continue;

				iplPostInstr.Append("\t\tcase %s_HT_SEL_%s_%d_%s:\n",
					mod.m_modName.Upper().c_str(), cxrIntf.m_srcInstName.Upper().c_str(), cxrIntf.GetPortReplId(), cxrIntf.m_funcName.Upper().c_str());

				if (cxrIntf.m_cxrType == CxrCall || cxrIntf.m_cxrType == CxrTransfer) {

					for (size_t intfIdx2 = 0; intfIdx2 < modInst.m_cxrIntfList.size(); intfIdx2 += 1) {
						CCxrIntf &cxrIntf2 = modInst.m_cxrIntfList[intfIdx2];

						if (cxrIntf2.m_cxrDir == CxrOut) continue;
						if (cxrIntf2.m_cxrType != CxrCall && cxrIntf2.m_cxrType != CxrTransfer) continue;
						if (cxrIntf2.m_reserveCnt == 0) continue;
						if (intfIdx2 == intfIdx) continue;

						iplPostInstr.Append("\t\t\tc_%s_%sRsvLimit%s += 1;\n",
							cxrIntf2.GetPortNameSrcToDstLc(), cxrIntf2.GetIntfName(), cxrIntf2.GetPortReplIndex());
					}

				}
				
				iplPostInstr.Append("\t\t\tbreak;\n");
			}

			iplPostInstr.Append("\t\tcase %s_HT_SEL_SM:\n", mod.m_modName.Upper().c_str());

			iplPostInstr.Append("\t\t\tbreak;\n");

			iplPostInstr.Append("\t\tdefault:\n");
			iplPostInstr.Append("\t\t\tassert(0);\n");
			iplPostInstr.Append("\t\t}\n");
			iplPostInstr.Append("\t}\n");
			iplPostInstr.Append("\n");
		}

		// m_htIdPool push
		if (mod.m_globalVarList.size() == 0) {
			for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

				if (cxrIntf.m_cxrDir == CxrIn) continue;
				if (cxrIntf.IsCall()) continue;

				if (cxrIntf.IsXfer() && cxrIntf.m_pDstMod->m_clkRate == eClk1x && cxrIntf.m_pDstMod->m_clkRate != mod.m_clkRate)
					iplPostInstr.Append("\tif (r_%s_%sRdy%s && !r_%s_%sHold%s)\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				else
					iplPostInstr.Append("\tif (r_%s_%sRdy%s)\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				if (cxrIntf.m_pSrcGroup->m_htIdW.AsInt() == 0)
					iplPostInstr.Append("\t\tc_htBusy = false;\n");
				else
					iplPostInstr.Append("\t\tm_htIdPool.push(r_t%d_htId);\n", mod.m_execStg + 1);
				iplPostInstr.Append("\n");
			}
		}

		if (mod.m_threads.m_resetInstr.size() > 0) {

			iplPostInstr.Append("\tif (r_t%d_htTerm)\n", mod.m_execStg+1);

			if (mod.m_threads.m_htIdW.AsInt() == 0)
				iplPostInstr.Append("\t\tc_htBusy = false;\n");
			else
				iplPostInstr.Append("\t\tm_htIdPool.push(r_t%d_htId);\n", mod.m_execStg+1);

			iplPostInstr.Append("\n");
		}

		if (mod.m_bMultiThread) {

			if (mod.m_threads.m_bCallFork) {
				if (mod.m_threads.m_htIdW.AsInt() <= 2) {
					// no code for registered version
				} else {
					for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
						CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

						if (cxrIntf.m_cxrDir == CxrOut) continue;
						if (cxrIntf.m_cxrType == CxrCall || cxrIntf.m_cxrType == CxrTransfer) continue;

						iplPostInstr.Append("\tm_%s_%sPrivLk1%s.write_addr(r_t%d_htId);\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(), 
							mod.m_wrStg);
					}
					iplPostInstr.Append("\tm_htCmdPrivLk1.write_addr(r_t%d_htId);\n", mod.m_wrStg);

					if (mod.m_rsmSrcCnt > 0)
						iplPostInstr.Append("\tm_rsmPrivLk1.write_addr(r_t%d_htId);\n", mod.m_wrStg);

					iplPostInstr.Append("\n");
				}
			}

			if (mod.m_threads.m_htIdW.AsInt() == 0)
				iplPostInstr.Append("\tCHtPriv c_htPriv = r_htPriv;\n");
			else
				iplPostInstr.Append("\tm_htPriv.write_addr(r_t%d_htId);\n", 
				mod.m_wrStg);

			if (mod.m_threads.m_bCallFork)
				iplPostInstr.Append("\tif (r_t%d_htPrivWe) {\n", mod.m_wrStg);
			else
				iplPostInstr.Append("\tif (r_t%d_htPrivWe)\n", mod.m_wrStg);

			if (mod.m_threads.m_htIdW.AsInt() > 0)
				iplPostInstr.Append("\t\tm_htPriv.write_mem( r_t%d_htPriv );\n",
				mod.m_wrStg);
			else
				iplPostInstr.Append("\t\tc_htPriv = r_t%d_htPriv;\n",
				mod.m_wrStg);
			iplPostInstr.Append("\n");

			if (mod.m_threads.m_bCallFork) {
				if (mod.m_threads.m_htIdW.AsInt() == 0)
					iplPostInstr.Append("\t\tc_htPrivLk = 0;\n");
				else if (mod.m_threads.m_htIdW.AsInt() <= 2)
					iplPostInstr.Append("\t\tc_htPrivLk[INT(r_t%d_htId)] = 0;\n", mod.m_wrStg);
				else {
					for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
						CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

						if (cxrIntf.m_cxrDir == CxrOut) continue;
						if (cxrIntf.m_cxrType == CxrCall || cxrIntf.m_cxrType == CxrTransfer) continue;

						iplPostInstr.Append("\t\tm_%s_%sPrivLk1%s.write_mem(r_t%d_htPrivLkData);\n",
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(), 
							mod.m_wrStg);
					}
					iplPostInstr.Append("\t\tm_htCmdPrivLk1.write_mem(r_t%d_htPrivLkData);\n", mod.m_wrStg);

					if (mod.m_rsmSrcCnt > 0)
						iplPostInstr.Append("\t\tm_rsmPrivLk1.write_mem(r_t%d_htPrivLkData);\n", mod.m_wrStg);
				}

				iplPostInstr.Append("\t}\n");
				iplPostInstr.Append("\n");
			}
		}

		// internal command queue
		int stg;
		sscanf(pRegStg, "r_t%d", &stg);

		if (mod.m_bMultiThread) {
			iplPostInstr.Append("\tif (r_t%d_htCmdRdy)\n",
				mod.m_wrStg);

			if (mod.m_threads.m_htIdW.AsInt() == 0)
				iplPostInstr.Append("\t\tc_htCmdValid = true;\n",
				pRegStg);
			else
				iplPostInstr.Append("\t\tm_htCmdQue.push(r_t%d_htCmd);\n",
				mod.m_wrStg);
		}

		iplPostInstr.Append("\n");

		if (mod.m_threads.m_htIdW.AsInt() > 0) {

			iplPostInstr.Append("\tsc_uint<%s_HTID_W+1> c_htIdPoolCnt = r_htIdPoolCnt;\n",
				mod.m_modName.Upper().c_str());

			iplPostInstr.Append("\tif (r_htIdPoolCnt < (1ul << %s_HTID_W)) {\n",
				mod.m_modName.Upper().c_str());

			iplPostInstr.Append("\t\tm_htIdPool.push(r_htIdPoolCnt & ((1ul << %s_HTID_W)-1));\n",
				mod.m_modName.Upper().c_str());

			iplPostInstr.Append("\t\tc_htIdPoolCnt = r_htIdPoolCnt + 1;\n");

			if (mod.m_threads.m_bCallFork && !(mod.m_threads.m_htIdW.AsInt() <= 2)) {
				iplPostInstr.Append("\n");
				iplPostInstr.Append("\t\tc_t%d_htId = r_htIdPoolCnt & ((1ul << %s_HTID_W)-1);\n",
					rdPrivLkStg-1, mod.m_modName.Upper().c_str());
				iplPostInstr.Append("\t\tc_t%d_htId = r_htIdPoolCnt & ((1ul << %s_HTID_W)-1);\n",
					mod.m_wrStg-1, mod.m_modName.Upper().c_str());
			}

			iplPostInstr.Append("\t}\n");
			iplPostInstr.Append("\n");
		}

		if (mod.m_bHasThreads) {
			iplReg.Append("\tr_%sToHta_assert = c_htAssert;\n", mod.m_modName.Lc().c_str());

			if (mod.m_clkRate == eClk2x) {
				m_iplReg1x.Append("\tr_%sToHta_assert_1x = r_%sToHta_assert;\n",
					mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());
				m_iplReg1x.NewLine();
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// IPL register assignments

	for (size_t fieldIdx = 0; fieldIdx < mod.m_shared.m_fieldList.size(); fieldIdx += 1) {
		CField &field = mod.m_shared.m_fieldList[fieldIdx];

		if (field.m_queueW.AsInt() == 0 && field.m_addr1W.AsInt() == 0)
			continue;

		if (field.m_dimenList.size() > 0)
			GenRamIndexLoops(iplReg, "", field);

		if (field.m_queueW.size() > 0)
			iplReg.Append("\tm_%s%s.clock(c_reset1x);\n", field.m_name.c_str(), field.m_dimenIndex.c_str());
		else if (field.m_addr1W.size() > 0)
			iplReg.Append("\tm_%s%s.clock();\n", field.m_name.c_str(), field.m_dimenIndex.c_str());
	}

	if (mod.m_threads.m_htIdW.AsInt() == 0) {
		iplReg.Append("\tr_htCmdValid = !c_reset1x && c_htCmdValid;\n");
		iplReg.Append("\tr_htPriv = c_htPriv;\n");
		iplReg.Append("\tr_htBusy = !c_reset1x && c_htBusy;\n");
		iplReg.Append("\n");
	} else {
		iplReg.Append("\tm_htCmdQue.clock(c_reset1x);\n");
		iplReg.Append("\tm_htPriv.clock();\n");
		iplReg.Append("\tm_htIdPool.clock(c_reset1x);\n");
		iplReg.Append("\n");
	}

	if (mod.m_threads.m_bCallFork && !(mod.m_threads.m_htIdW.AsInt() <= 2)) {
		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrOut) continue;
			if (cxrIntf.m_cxrType == CxrCall || cxrIntf.m_cxrType == CxrTransfer) continue;

			iplPostInstr.Append("\tm_%s_%sPrivLk0%s.clock();\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			iplPostInstr.Append("\tm_%s_%sPrivLk1%s.clock();\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
		}
		iplPostInstr.Append("\tm_htCmdPrivLk0.clock();\n");
		iplPostInstr.Append("\tm_htCmdPrivLk1.clock();\n");
		
		if (mod.m_rsmSrcCnt > 0) {
			iplPostInstr.Append("\tm_rsmPrivLk0.clock();\n");
			iplPostInstr.Append("\tm_rsmPrivLk1.clock();\n");
		}

		iplPostInstr.Append("\n");
	}

	// input cmd registers
	if (bStateMachine)
		iplReg.Append("\t// t1 select stage\n");

	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrOut) continue;
		if (cxrIntf.GetQueDepthW() > 0) continue;

		iplReg.Append("\tr_t1_%s_%sRdy%s = !c_reset1x && c_t0_%s_%sRdy%s;\n",
			cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
			cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

		if (cxrIntf.m_bCxrIntfFields)
			iplReg.Append("\tr_t1_%s_%s%s = c_t0_%s_%s%s;\n",
			cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
			cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

		iplReg.Append("\n");
	}

	////////////////////////////////////////////
	// clear shared variables without depth

	iplClrShared.Append("\n");
	iplClrShared.Append("\tif (r_reset1x) {\n");

	for (size_t shIdx = 0; shIdx < mod.m_shared.m_fieldList.size(); shIdx += 1) {
		CField &shared = mod.m_shared.m_fieldList[shIdx];

		if (shared.m_queueW.AsInt() > 0 || shared.m_addr1W.AsInt() > 0) continue;
		if (shared.m_reset == "false" || shared.m_reset == "" && !mod.m_bResetShared) continue;

		vector<int> refList(shared.m_dimenList.size(), 0);
		do {
			string idxStr = IndexStr(refList);
			iplClrShared.Append("\t\tc_%s%s = 0;\n", shared.m_name.c_str(), idxStr.c_str());

		} while (DimenIter(shared.m_dimenList, refList));
	}

	iplClrShared.Append("\t}\n");
	iplClrShared.Append("\n");

	//#########################################

	if (mod.m_threads.m_htIdW.AsInt() == 0) {
		m_iplRegDecl.Append("\tbool r_htBusy;\n");

		m_iplRegDecl.Append("\tbool r_htCmdValid;\n");
	} else {
		m_iplRegDecl.Append("\tsc_uint<%s_HTID_W+1> r_htIdPoolCnt;\n",
			mod.m_modName.Upper().c_str());

		m_iplRegDecl.Append("\tht_dist_que<sc_uint<%s_HTID_W>, %s_HTID_W> m_htIdPool;\n",
			mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());

		iplReg.Append("\tr_htIdPoolCnt = c_reset1x ? (sc_uint<%s_HTID_W+1>) 0 : c_htIdPoolCnt;\n",
			mod.m_modName.Upper().c_str());

		if (mod.m_threads.m_resetInstr.size() > 0)
			m_iplEosChecks.Append("\t\tassert(m_htIdPool.size() >= %d);\n",
				(1 << mod.m_threads.m_htIdW.AsInt())-1);
		else
			m_iplEosChecks.Append("\t\tassert(m_htIdPool.size() == %d);\n",
				1 << mod.m_threads.m_htIdW.AsInt());
	}

	////////////////////////////////////////////////////////////////////////////
	// IPL output assignments

	// Generate ram interface addresses
	for (size_t ramIdx = 0; ramIdx < mod.m_extInstRamList.size(); ramIdx += 1) {
		CRamPort &ramPort = mod.m_extInstRamList[ramIdx];
		CRam * pExtRam = ramPort.m_pRam;

		bool bHasAddr = pExtRam->m_addr0W.AsInt() + pExtRam->m_addr1W.AsInt() + pExtRam->m_addr2W.AsInt() > 0;

		if (pExtRam->m_bSrcRead && bHasAddr) {
			char inStg[32];
			sprintf(inStg, "r_t%d", mod.m_tsStg - 4 + pExtRam->m_rdStg.AsInt());
			char outStg[32];
			sprintf(outStg, "c_t%d", mod.m_tsStg - 4 + pExtRam->m_rdStg.AsInt());

			string addrW = pExtRam->m_addr1W.AsStr();
			if (pExtRam->m_addr2W.size() > 0)
				addrW = pExtRam->m_addr2W.AsStr() + " + " + addrW;

			string ramAddrName = string(outStg) + "_" + pExtRam->m_gblName.AsStr() + "RdAddr";
			string addrType = "sc_uint<" + addrW + ">";
			GenRamAddr(modInst, *pExtRam, &iplPostReg, addrW, ramAddrName, inStg, outStg, false);
		}
	}

	// external ram interfaces
	for (size_t ramIdx = 0; ramIdx < mod.m_extInstRamList.size(); ramIdx += 1) {
		CRamPort &ramPort = mod.m_extInstRamList[ramIdx];
		CRam * pExtRam = ramPort.m_pRam;

		bool bHasAddr = pExtRam->m_addr0W.AsInt() + pExtRam->m_addr1W.AsInt() + pExtRam->m_addr2W.AsInt() > 0;

		// generate external ram interface
		iplOut.Append("\t// %s external ram interface\n", ramPort.m_instName.Uc().c_str());

		if (pExtRam->m_bSrcRead && bHasAddr) {
			char inStg[32];
			sprintf(inStg, "r_t%d", mod.m_tsStg - 4 + pExtRam->m_rdStg.AsInt());
			char outStg[32];
			sprintf(outStg, "c_t%d", mod.m_tsStg - 4 + pExtRam->m_rdStg.AsInt());

			string addrW = pExtRam->m_addr1W.AsStr();
			if (pExtRam->m_addr2W.size() > 0)
				addrW = pExtRam->m_addr2W.AsStr() + " + " + addrW;

			string accessSelName = string(outStg) + "_" + pExtRam->m_gblName.AsStr() + "RdAddr";
			string rdAddrName = GenRamAddr(modInst, *pExtRam, &iplPostReg, addrW, accessSelName, inStg, outStg, false);

			iplOut.Append("\to_%sTo%s_%sRdAddr = %s;\n",
				mod.m_modName.Lc().c_str(), ramPort.m_instName.Uc().c_str(), pExtRam->m_gblName.c_str(),
				rdAddrName.c_str());
		}

		if (pExtRam->m_bSrcWrite || pExtRam->m_bMifWrite) {
			char ramWrStgBuf[4];
			if (pExtRam->m_bSrcWrite) {
				int ramWrStg = mod.m_tsStg + pExtRam->m_wrStg.AsInt();
				sprintf(ramWrStgBuf, "t%d", ramWrStg);
			} else
				strcpy(ramWrStgBuf, "m2");

			if (bHasAddr) {
				GenRamIndexLoops(iplOut, "", *pExtRam);
				iplOut.Append("\to_%sTo%s_%sWrAddr%s = r_%s_%sWrAddr%s;\n",
					mod.m_modName.Lc().c_str(), ramPort.m_instName.Uc().c_str(), pExtRam->m_gblName.c_str(), pExtRam->m_dimenIndex.c_str(),
					ramWrStgBuf, pExtRam->m_gblName.c_str(), pExtRam->m_dimenIndex.c_str());
			}

			GenRamIndexLoops(iplOut, "", *pExtRam);
			if (ramPort.m_selWidth == 0 || !bHasAddr)
				iplOut.Append("\to_%sTo%s_%sWrEn%s = r_%s_%sWrEn%s;\n",
				mod.m_modName.Lc().c_str(), ramPort.m_instName.Uc().c_str(), pExtRam->m_gblName.c_str(), pExtRam->m_dimenIndex.c_str(),
				ramWrStgBuf, pExtRam->m_gblName.c_str(), pExtRam->m_dimenIndex.c_str());
			else
				iplOut.Append("\to_%sTo%s_%sWrEn%s = (r_%s_%sWrAddr & 0x%lx) == 0x%x ? r_%s_%sWrEn%s : c_%sWrEnZero;\n",
				mod.m_modName.Lc().c_str(), ramPort.m_instName.Uc().c_str(), pExtRam->m_gblName.c_str(), pExtRam->m_dimenIndex.c_str(),
				ramWrStgBuf, pExtRam->m_gblName.c_str(),
				(1ul << ramPort.m_selWidth)-1, ramPort.m_selIdx,
				ramWrStgBuf, pExtRam->m_gblName.c_str(), pExtRam->m_dimenIndex.c_str(),
				pExtRam->m_gblName.c_str());

			GenRamIndexLoops(iplOut, "", *pExtRam);
			iplOut.Append("\to_%sTo%s_%sWrData%s = r_%s_%sWrData%s;\n",
				mod.m_modName.Lc().c_str(), ramPort.m_instName.Uc().c_str(), pExtRam->m_gblName.c_str(), pExtRam->m_dimenIndex.c_str(),
				ramWrStgBuf, pExtRam->m_gblName.c_str(), pExtRam->m_dimenIndex.c_str());
		}
		iplOut.Append("\n");
	}

	if (mod.m_bHasThreads) {
		if (mod.m_clkRate == eClk1x) {
			iplOut.Append("\n");
			iplOut.Append("\to_%sToHta_assert = r_%sToHta_assert;\n", mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());
		} else {
			m_iplOut1x.Append("\n");
			m_iplOut1x.Append("\to_%sToHta_assert = r_%sToHta_assert_1x;\n", mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());
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

