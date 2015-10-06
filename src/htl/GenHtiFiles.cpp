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

void
CDsnInfo::GenerateHtiFiles()
{
	string fileName = g_appArgs.GetOutputFolder() + "/Pers" + m_unitName.Uc() + "Hti" + ".h";

	CHtFile incFile(fileName, "w");

	CModule & mod = *m_modList[0];
	if (!mod.m_bHostIntf)
		ParseMsg(Fatal, "Expected first module to be host interface\n");

	GenPersBanner(incFile, m_unitName.Uc().c_str(), "Hti", true);

	// check if any needed modules have a host message interface
	bool bIhm = false;
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule & mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;

		if (mod.m_bInHostMsg)
			bIhm = true;
	}

	if (mod.m_modInstList.size() != 1 || mod.m_modInstList[0].m_cxrIntfList.size() != 2)
		ParseMsg(Fatal, "Expected host interface to have a single call and a single return interface");

	CModInst & modInst = mod.m_modInstList[0];

	int callIdx = -1;
	int rtnIdx = -1;

	if (modInst.m_cxrIntfList[0].m_cxrType == CxrCall && modInst.m_cxrIntfList[0].m_cxrDir == CxrOut)
		callIdx = 0;
	else if (modInst.m_cxrIntfList[1].m_cxrType == CxrCall && modInst.m_cxrIntfList[1].m_cxrDir == CxrOut)
		callIdx = 1;

	if (modInst.m_cxrIntfList[0].m_cxrType == CxrReturn && modInst.m_cxrIntfList[0].m_cxrDir == CxrIn)
		rtnIdx = 0;
	else if (modInst.m_cxrIntfList[1].m_cxrType == CxrReturn && modInst.m_cxrIntfList[1].m_cxrDir == CxrIn)
		rtnIdx = 1;

	if (callIdx < 0 || rtnIdx < 0)
		ParseMsg(Fatal, "Expected host interface to have a single call and a single return interface");

	CCxrIntf & rtnIntf = modInst.m_cxrIntfList[rtnIdx];
	CCxrIntf & callIntf = modInst.m_cxrIntfList[callIdx];

	bool bCallCmdFields = callIntf.m_bCxrIntfFields;
	bool bRtnCmdFields = rtnIntf.m_bCxrIntfFields;

	bool bCallClk2x = callIntf.m_pDstModInst->m_pMod->m_clkRate == eClk2x;
	bool bRtnClk2x = rtnIntf.m_pSrcModInst->m_pMod->m_clkRate == eClk2x;

	fprintf(incFile, "// internal states\n");
	fprintf(incFile, "#define HTI_IDLE	0\n");
	fprintf(incFile, "#define HTI_ARGS	1\n");
	fprintf(incFile, "#define HTI_CMD		2\n");
	fprintf(incFile, "#define HTI_DATA	3\n");
	fprintf(incFile, "\n");
	fprintf(incFile, "SC_MODULE(CPers%sHti) {\n", m_unitName.Uc().c_str());
	fprintf(incFile, "\tht_attrib(keep_hierarchy, CPers%sHti, \"true\");\n", m_unitName.Uc().c_str());
	fprintf(incFile, "\n");
	fprintf(incFile, "\t//\n");
	fprintf(incFile, "\t// Ports\n");
	fprintf(incFile, "\t//\n");
	fprintf(incFile, "\tsc_in<bool> i_clock1x;\n");

	if (bCallClk2x || bRtnClk2x)
		fprintf(incFile, "\tsc_in<bool> i_clock2x;\n");

	fprintf(incFile, "\tsc_in<bool> i_reset;\n");
	fprintf(incFile, "\n");
	fprintf(incFile, "\t//\n");
	fprintf(incFile, "\t// Host side\n");
	fprintf(incFile, "\t//\n");
	fprintf(incFile, "\n");
	fprintf(incFile, "\t// inbound control queue interface\n");
	fprintf(incFile, "\tsc_in<bool> i_hifToHti_ctlRdy;\n");
	fprintf(incFile, "\tsc_in<CHostCtrlMsgIntf> i_hifToHti_ctl;\n");
	fprintf(incFile, "\tsc_out<bool> o_htiToHif_ctlAvl;\n");
	fprintf(incFile, "\n");
	fprintf(incFile, "\t// inbound block queue interface\n");
	fprintf(incFile, "\tsc_in<bool> i_hifToHti_datRdy;\n");
	fprintf(incFile, "\tsc_in<CHostDataQueIntf> i_hifToHti_dat;\n");
	fprintf(incFile, "\tsc_out<bool> o_htiToHif_datAvl;\n");
	fprintf(incFile, "\n");
	fprintf(incFile, "\t// outbound control queue interface\n");
	fprintf(incFile, "\tsc_out<bool> o_htiToHif_ctlRdy;\n");
	fprintf(incFile, "\tsc_out<CHostCtrlMsgIntf> o_htiToHif_ctl;\n");
	fprintf(incFile, "\tsc_in<bool> i_hifToHti_ctlAvl;\n");
	fprintf(incFile, "\n");
	fprintf(incFile, "\t// outbound block queue interface\n");
	fprintf(incFile, "\tsc_out<bool> o_htiToHif_datRdy;\n");
	fprintf(incFile, "\tsc_out<CHostDataQueIntf> o_htiToHif_dat;\n");
	fprintf(incFile, "\tsc_in<bool> i_hifToHti_datAvl;\n");
	fprintf(incFile, "\n");
	fprintf(incFile, "\t//\n");
	fprintf(incFile, "\t// Unit side\n");
	fprintf(incFile, "\t//\n");
	fprintf(incFile, "\n");

	if (bIhm) {
		fprintf(incFile, "\tsc_out<CHostCtrlMsgIntf> o_hifTo%s_ihm;\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\n");
	}

	fprintf(incFile, "\tsc_out<bool> o_%s_%sRdy;\n",
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName());

	if (bCallCmdFields)
		fprintf(incFile, "\tsc_out<C%s_%sIntf> o_%s_%s;\n",
		callIntf.GetSrcToDstUc(), callIntf.GetIntfName(),
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName());

	fprintf(incFile, "\tsc_in<bool> i_%s_%sAvl;\n",
		callIntf.GetDstToSrcLc(), callIntf.GetIntfName());

	fprintf(incFile, "\n");

	fprintf(incFile, "\tsc_in<bool> i_%s_%sRdy;\n",
		rtnIntf.GetSrcToDstLc(), rtnIntf.GetIntfName());

	if (bRtnCmdFields)
		fprintf(incFile, "\tsc_in<C%s_%sIntf> i_%s_%s;\n",
		rtnIntf.GetSrcToDstUc(), rtnIntf.GetIntfName(),
		rtnIntf.GetSrcToDstLc(), rtnIntf.GetIntfName());

	fprintf(incFile, "\tsc_out<bool> o_%s_%sAvl;\n",
		rtnIntf.GetDstToSrcLc(), rtnIntf.GetIntfName());

	fprintf(incFile, "\n");

	CQueIntf * pInQueIntf = 0;
	CQueIntf * pOutQueIntf = 0;

	bool bIhdClk2x = false;
	bool bOhdClk2x = false;
	if (mod.m_queIntfList.size() > 0) {
		fprintf(incFile, "\t// queue interfaces\n");

		int inQueCnt = 0;
		int outQueCnt = 0;

		for (size_t queIdx = 0; queIdx < mod.m_queIntfList.size(); queIdx += 1) {
			CQueIntf & queIntf = mod.m_queIntfList[queIdx];

			if (queIntf.m_queType == Push) {
				pInQueIntf = &queIntf;
				inQueCnt += 1;

				for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
					CModule &mod = *m_modList[modIdx];

					if (mod.m_modName == queIntf.m_modName)
						bIhdClk2x = mod.m_clkRate == eClk2x;
				}
			} else {
				pOutQueIntf = &queIntf;
				outQueCnt += 1;

				for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
					CModule &mod = *m_modList[modIdx];

					if (mod.m_modName == queIntf.m_modName)
						bOhdClk2x = mod.m_clkRate == eClk2x;
				}
			}
		}

		if (inQueCnt > 1 || outQueCnt > 1)
			ParseMsg(Fatal, "At most one inbound and one outbound HIF queue supported");
	}

	if (pInQueIntf) {
		// in bound interface
		fprintf(incFile, "\tsc_out<bool> o_hifTo%s_%sRdy;\n",
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
		string structName = pInQueIntf->m_typeName + (pInQueIntf->m_bInclude ? "_inc" : "");
		fprintf(incFile, "\tsc_out<%s> o_hifTo%s_%s;\n",
			structName.c_str(),
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
		fprintf(incFile, "\tsc_in<bool> i_%sToHif_%sAvl;\n",
			pInQueIntf->m_modName.Lc().c_str(), pInQueIntf->m_queName.Lc().c_str());
		fprintf(incFile, "\n");
	}

	if (pOutQueIntf) {
		// out bound interface
		fprintf(incFile, "\tsc_in<bool> i_%sToHif_%sRdy;\n",
			pOutQueIntf->m_modName.Lc().c_str(), pOutQueIntf->m_queName.Lc().c_str());
		string structName = pOutQueIntf->m_typeName + (pOutQueIntf->m_bInclude ? "_inc" : "");
		fprintf(incFile, "\tsc_in<%s> i_%sToHif_%s;\n",
			structName.c_str(),
			pOutQueIntf->m_modName.Lc().c_str(), pOutQueIntf->m_queName.Lc().c_str());
		fprintf(incFile, "\tsc_out<bool> o_hifTo%s_%sAvl;\n",
			pOutQueIntf->m_modName.Uc().c_str(), pOutQueIntf->m_queName.Lc().c_str());
		fprintf(incFile, "\n");
	}

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_ohm.m_bOutHostMsg) continue;

		fprintf(incFile, "\tsc_in<CHostCtrlMsgIntf> i_%sToHti_ohm;\n", mod.m_modName.Lc().c_str());
		fprintf(incFile, "\tsc_out<bool> o_htiTo%s_ohmAvl;\n", mod.m_modName.Uc().c_str());
		fprintf(incFile, "\n");
	}

	fprintf(incFile, "\t//\n");
	fprintf(incFile, "\t// Internal State\n");
	fprintf(incFile, "\t//\n");
	fprintf(incFile, "\n");
	if (bIhm)
		fprintf(incFile, "\tht_dist_que<CHostCtrlMsgIntf, 5> m_iCtlQue;\n");
	fprintf(incFile, "\tht_dist_que<CHostDataQueIntf, 5> m_iDatQue;\n");
	if (bRtnCmdFields)
		fprintf(incFile, "\tht_dist_que<C%s_%sIntf, 5> m_oCmdQue;\n",
		rtnIntf.GetSrcToDstUc(), rtnIntf.GetIntfName());
	else
		fprintf(incFile, "\tsc_uint<5> r_oCmdCnt;\n");

	if (pOutQueIntf) {
		string structName = pOutQueIntf->m_typeName + (pOutQueIntf->m_bInclude ? "_inc" : "");
		fprintf(incFile, "\tht_dist_que<%s, 5> m_%sToHif_%sQue;\n",
			structName.c_str(),
			pOutQueIntf->m_modName.Lc().c_str(), pOutQueIntf->m_queName.Lc().c_str());
	}

	fprintf(incFile, "\n");
	fprintf(incFile, "\tbool r_htiToHif_ctlAvl;\n");
	fprintf(incFile, "\tbool r_htiToHif_datAvl;\n");
	fprintf(incFile, "\n");
	fprintf(incFile, "\tsc_uint<2> r_callState;\n");
	fprintf(incFile, "\tuint8_t r_callArgCnt;\n");
	fprintf(incFile, "\tuint8_t r_callActiveCnt;\n");

	if (bIhm)
		fprintf(incFile, "\tCHostCtrlMsgIntf r_hifTo%s_ihm;\n", m_unitName.Uc().c_str());

	if (bCallClk2x) {
		fprintf(incFile, "\tsc_signal<bool> r_%s_%sRdy;\n", callIntf.GetSrcToDstLc(), callIntf.GetIntfName());
		fprintf(incFile, "\tbool r_%s_%sRdy_2x;\n", callIntf.GetSrcToDstLc(), callIntf.GetIntfName());
	} else
		fprintf(incFile, "\tbool r_%s_%sRdy;\n", callIntf.GetSrcToDstLc(), callIntf.GetIntfName());

	if (bCallCmdFields)
		fprintf(incFile, "\tC%s_%sIntf r_%s_%s;\n",
		callIntf.GetSrcToDstUc(), callIntf.GetIntfName(),
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName());

	if (bCallClk2x) {
		fprintf(incFile, "\tsc_signal<ht_uint1> r_%s_%sAvl_2x1;\n", callIntf.GetDstToSrcLc(), callIntf.GetIntfName());
		fprintf(incFile, "\tsc_signal<ht_uint1> r_%s_%sAvl_2x2;\n", callIntf.GetDstToSrcLc(), callIntf.GetIntfName());
	}

	fprintf(incFile, "\tht_uint1 r_%s_%sAvlCnt;\n", callIntf.GetSrcToDstLc(), callIntf.GetIntfName());

	if (bRtnClk2x) {
		fprintf(incFile, "\tsc_signal<bool> r_%s_%sAvl;\n", rtnIntf.GetDstToSrcLc(), rtnIntf.GetIntfName());
		fprintf(incFile, "\tbool r_%s_%sAvl_2x;\n", rtnIntf.GetDstToSrcLc(), rtnIntf.GetIntfName());
		if (!bRtnCmdFields)
			fprintf(incFile, "\tsc_signal<bool> r_%s_%sRdy_2x;\n", rtnIntf.GetSrcToDstLc(), rtnIntf.GetIntfName());
	} else
		fprintf(incFile, "\tbool r_%s_%sAvl;\n", rtnIntf.GetDstToSrcLc(), rtnIntf.GetIntfName());

	fprintf(incFile, "\n");
	fprintf(incFile, "\tsc_uint<2> r_rtnState;\n");
	fprintf(incFile, "\tuint8_t r_rtnCnt;\n");
	fprintf(incFile, "\tbool r_htiToHif_ctlRdy;\n");
	fprintf(incFile, "\tCHostCtrlMsgIntf r_htiToHif_ctl;\n");
	fprintf(incFile, "\tht_uint1 r_htiToHif_ctlAvlCnt;\n");
	fprintf(incFile, "\tbool r_htiToHif_datRdy;\n");
	fprintf(incFile, "\tCHostDataQueIntf r_htiToHif_dat;\n");
	fprintf(incFile, "\tht_uint6 r_htiToHif_datAvlCnt;\n");
	fprintf(incFile, "\n");

	if (pInQueIntf) {
		fprintf(incFile, "\tbool r_hifTo%s_%sRdy;\n",
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
		string structName = pInQueIntf->m_typeName + (pInQueIntf->m_bInclude ? "_inc" : "");
		fprintf(incFile, "\t%s r_hifTo%s_%s;\n",
			structName.c_str(),
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
		fprintf(incFile, "\tht_uint6  r_hifTo%s_%sAvlCnt;\n",
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());

		if (bIhdClk2x) {
			fprintf(incFile, "\tsc_signal<bool> r_%sToHif_%sAvl_2x1;\n",
				pInQueIntf->m_modName.Lc().c_str(), pInQueIntf->m_queName.Lc().c_str());
			fprintf(incFile, "\tsc_signal<bool> r_%sToHif_%sAvl_2x2;\n",
				pInQueIntf->m_modName.Lc().c_str(), pInQueIntf->m_queName.Lc().c_str());
		}
	}

	if (pOutQueIntf) {
		fprintf(incFile, "\tbool r_hifTo%s_%sAvl;\n",
			pOutQueIntf->m_modName.Uc().c_str(), pOutQueIntf->m_queName.Lc().c_str());
	}

	int ohmCnt = 0;
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_ohm.m_bOutHostMsg) continue;
		ohmCnt += 1;

		fprintf(incFile, "\tCHostCtrlMsgIntf r_%sToHti_ohm;\n", mod.m_modName.Lc().c_str());
		fprintf(incFile, "\tbool r_htiTo%s_ohmAvl;\n", mod.m_modName.Uc().c_str());
		fprintf(incFile, "\n");
	}

	if (ohmCnt > 1) {
		fprintf(incFile, "\tht_uint%d r_htSelRr;\n", ohmCnt);
		fprintf(incFile, "\n");
	}

	fprintf(incFile, "\tbool r_reset1x;\n");

	if (bCallClk2x || bRtnClk2x) {
		fprintf(incFile, "\tsc_signal<bool> c_reset2x;\n");
		fprintf(incFile, "\tbool r_phase;\n");
	}

	fprintf(incFile, "\n");
	fprintf(incFile, "\t//\n");
	fprintf(incFile, "\t// Methods\n");
	fprintf(incFile, "\t//\n");
	fprintf(incFile, "\tvoid Pers%sHti_1x();\n", m_unitName.Uc().c_str());

	if (bCallClk2x || bRtnClk2x)
		fprintf(incFile, "\tvoid Pers%sHti_2x();\n", m_unitName.Uc().c_str());

	fprintf(incFile, "\n");
	fprintf(incFile, "\tSC_CTOR(CPers%sHti) {\n", m_unitName.Uc().c_str());
	fprintf(incFile, "\t\tSC_METHOD(Pers%sHti_1x);\n", m_unitName.Uc().c_str());
	fprintf(incFile, "\t\tsensitive << i_clock1x.pos();\n");

	if (bCallClk2x || bRtnClk2x) {
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tSC_METHOD(Pers%sHti_2x);\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\t\tsensitive << i_clock2x.pos();\n");
	}

	fprintf(incFile, "\t}\n");
	fprintf(incFile, "\n");

	fprintf(incFile, "\t#ifndef _HTV\n");
	fprintf(incFile, "\tvoid end_of_simulation()\n");
	fprintf(incFile, "\t{\n");
	fprintf(incFile, "\t\tassert(r_htiToHif_ctlAvlCnt == 1);\n");
	fprintf(incFile, "\t\tassert(r_htiToHif_datAvlCnt == 32);\n");
	fprintf(incFile, "\t\tassert(r_%s_%sAvlCnt == 1);\n", callIntf.GetSrcToDstLc(), callIntf.GetIntfName());
	fprintf(incFile, "\t}\n");
	fprintf(incFile, "\t#endif\n");
	fprintf(incFile, "};\n");

	incFile.FileClose();

	////////////////////////////////////////////////////////////////
	// Generate cpp file

	fileName = g_appArgs.GetOutputFolder() + "/Pers" + m_unitName.Uc() + "Hti" + ".cpp";

	CHtFile cppFile(fileName, "w");

	GenPersBanner(cppFile, m_unitName.Uc().c_str(), "Hti", false);

	fprintf(cppFile, "void\n");
	fprintf(cppFile, "CPers%sHti::Pers%sHti_1x()\n", m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
	fprintf(cppFile, "{\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\t//\n");
	fprintf(cppFile, "\t// Call logic\n");
	fprintf(cppFile, "\t//\n");

	if (bIhm) {
		fprintf(cppFile, "\tif (i_hifToHti_ctlRdy.read())\n");
		fprintf(cppFile, "\t\tm_iCtlQue.push(i_hifToHti_ctl);\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\tbool c_htiToHif_ctlAvl = false;\n");
		fprintf(cppFile, "\tCHostCtrlMsgIntf c_hifTo%s_ihm;\n", m_unitName.Uc().c_str());
		fprintf(cppFile, "\tif (!m_iCtlQue.empty() && !(r_callState == HTI_CMD || r_callState == HTI_ARGS)) {\n");
		fprintf(cppFile, "\t\tm_iCtlQue.pop();\n");
		fprintf(cppFile, "\t\tc_hifTo%s_ihm = m_iCtlQue.front();\n", m_unitName.Uc().c_str());
		fprintf(cppFile, "\t\tc_htiToHif_ctlAvl = true;\n");
		fprintf(cppFile, "\t} else\n");
		fprintf(cppFile, "\t\tc_hifTo%s_ihm.m_data64 = 0;\n", m_unitName.Uc().c_str());
	} else {
		fprintf(cppFile, "\tbool c_htiToHif_ctlAvl = i_hifToHti_ctlRdy.read();\n");
		fprintf(cppFile, "\tCHostCtrlMsgIntf ht_noload c_hifToHti_ctl = i_hifToHti_ctl.read();\n");
	}
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\tif (i_hifToHti_datRdy.read())\n");
	fprintf(cppFile, "\t\tm_iDatQue.push(i_hifToHti_dat);\n");
	fprintf(cppFile, "\tCHostDataQueIntf iDatQueDat = m_iDatQue.front();\n");
	fprintf(cppFile, "\tbool c_htiToHif_datAvl = false;\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\tsc_uint<2> c_callState = r_callState;\n");
	fprintf(cppFile, "\tuint8_t c_callArgCnt = r_callArgCnt;\n");
	fprintf(cppFile, "\tuint8_t c_callActiveCnt = r_callActiveCnt;\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\tbool c_%s_%sRdy = false;\n", callIntf.GetSrcToDstLc(), callIntf.GetIntfName());
	if (bCallCmdFields)
		fprintf(cppFile, "\tC%s_%sIntf c_%s_%s = r_%s_%s;\n",
		callIntf.GetSrcToDstUc(), callIntf.GetIntfName(),
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName(),
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName());
	fprintf(cppFile, "\n");

	if (pInQueIntf) {
		fprintf(cppFile, "\tbool c_hifTo%s_%sRdy = false;\n",
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
		string structName = pInQueIntf->m_typeName + (pInQueIntf->m_bInclude ? "_inc" : "");
		fprintf(cppFile, "\t%s c_hifTo%s_%s;\n",
			structName.c_str(),
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
		fprintf(cppFile, "\tc_hifTo%s_%s.m_data = 0;\n",
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
		fprintf(cppFile, "\tc_hifTo%s_%s.m_ctl = HT_DQ_DATA;\n",
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
	}

	fprintf(cppFile, "\tbool c_htiToHif_datRdy = false;\n");
	fprintf(cppFile, "\tCHostDataQueIntf c_htiToHif_dat;\n");
	fprintf(cppFile, "\tc_htiToHif_dat.m_data = 0;\n");
	fprintf(cppFile, "\tc_htiToHif_dat.m_ctl = HT_DQ_DATA;\n");
	fprintf(cppFile, "\n");

	int callQwCnt = 0;
	for (size_t fldIdx = 0; fldIdx < callIntf.m_pFieldList->size(); fldIdx += 1) {
		CField * pField = (*callIntf.m_pFieldList)[fldIdx];

		int fldQwCnt = (FindHostTypeWidth(pField) + 63) / 64;
		callQwCnt += fldQwCnt;
	}

	fprintf(cppFile, "\tswitch (r_callState) {\n");
	fprintf(cppFile, "\tcase HTI_IDLE:\n");
	fprintf(cppFile, "\t\tif (m_iDatQue.empty())\n");
	fprintf(cppFile, "\t\t\tbreak;\n\n");

	fprintf(cppFile, "\t\tswitch (iDatQueDat.m_ctl) {\n");
	fprintf(cppFile, "\t\tcase HT_DQ_DATA:\n");
	fprintf(cppFile, "\t\t\tc_callState = HTI_DATA;\n");
	fprintf(cppFile, "\t\tbreak;\n");
	fprintf(cppFile, "\t\tcase HT_DQ_CALL:\n");
	fprintf(cppFile, "\t\t\tassert(r_callActiveCnt < 255);\n");
	fprintf(cppFile, "\t\t\tassert(iDatQueDat.m_data < 255);\n");
	fprintf(cppFile, "\t\t\tc_callState = HTI_ARGS;\n");
	fprintf(cppFile, "\t\t\tc_callArgCnt = (uint8_t)iDatQueDat.m_data;\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\t\t\tc_htiToHif_datAvl = true;\n");
	fprintf(cppFile, "\t\t\tm_iDatQue.pop();\n");
	fprintf(cppFile, "\t\tbreak;\n");
	fprintf(cppFile, "\t\tcase HT_DQ_HALT:\n");
	fprintf(cppFile, "\t\t\tif (r_htiToHif_datAvlCnt != 0\n");
	fprintf(cppFile, "\t\t\t\t&& r_rtnState == HTI_IDLE\n");
	fprintf(cppFile, "\t\t\t\t&& r_callActiveCnt == 0)\n");
	fprintf(cppFile, "\t\t\t{\n");
	fprintf(cppFile, "\t\t\t\tc_htiToHif_datRdy = true;\n");
	fprintf(cppFile, "\t\t\t\tc_htiToHif_dat.m_ctl = HT_DQ_HALT;\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "\t\t\t\tc_htiToHif_datAvl = true;\n");
	fprintf(cppFile, "\t\t\t\tm_iDatQue.pop();\n");
	fprintf(cppFile, "\t\t\t}\n");
	fprintf(cppFile, "\t\tbreak;\n");
	fprintf(cppFile, "\t\tdefault:; /*nothing*/\n");
	fprintf(cppFile, "\t\t}\n");
	fprintf(cppFile, "\t\tbreak;\n");
	fprintf(cppFile, "\tcase HTI_ARGS:\n");
	fprintf(cppFile, "\t\tif (r_callArgCnt == 0) {\n");
	fprintf(cppFile, "\t\t\tc_callState = HTI_CMD;\n");
	fprintf(cppFile, "\t\t\tbreak;\n");
	fprintf(cppFile, "\t\t}\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\t\tif (m_iDatQue.empty())\n");
	fprintf(cppFile, "\t\t\tbreak;\n");
	fprintf(cppFile, "\n");

	if (bCallCmdFields) {
		fprintf(cppFile, "\t\tswitch (r_callArgCnt) {\n");

		int argQwIdx = 0;
		for (size_t fldIdx = 0; fldIdx < callIntf.m_pFieldList->size(); fldIdx += 1) {
			CField * pField = (*callIntf.m_pFieldList)[fldIdx];

			if (pField->m_pType->IsInt()) {
				fprintf(cppFile, "\t\tcase %d: {\n", callQwCnt - argQwIdx);
				if (pField->m_pType->m_typeName == "bool") {
					fprintf(cppFile, "\t\t\tc_%s_%s.m_%s = (iDatQueDat.m_data & 1) != 0;\n",
						callIntf.GetSrcToDstLc(), callIntf.GetIntfName(), pField->m_name.c_str());
				} else {
					fprintf(cppFile, "\t\t\tc_%s_%s.m_%s = (%s)iDatQueDat.m_data;\n",
						callIntf.GetSrcToDstLc(), callIntf.GetIntfName(), pField->m_name.c_str(),
						pField->m_pType->m_typeName.c_str());
				}
				fprintf(cppFile, "\t\t\tbreak;\n");
				fprintf(cppFile, "\t\t}\n");
				argQwIdx += 1;
			} else {
				int fldQwCnt = (FindHostTypeWidth(pField) + 63) / 64;

				for (int fldQwIdx = 0; fldQwIdx < fldQwCnt; fldQwIdx += 1) {
					bool bBasicType;
					CTypeDef * pTypeDef;
					string type = pField->m_pType->m_typeName;
					while (!(bBasicType = IsBaseType(type))) {
						if (!(pTypeDef = FindTypeDef(pField->m_pType->m_typeName)))
							break;
						type = pTypeDef->m_type;
					}

					fprintf(cppFile, "\t\tcase %d: {\n", callQwCnt - argQwIdx);

					if (!bBasicType) {
						fprintf(cppFile, "\t\t\tunion Arg_%s {\n", pField->m_name.c_str());
						fprintf(cppFile, "\t\t\t\t%s m_%s;\n", pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());
						fprintf(cppFile, "\t\t\t\tuint64_t m_data[%d];\n", fldQwCnt);
						fprintf(cppFile, "\t\t\t} arg;\n");

						fprintf(cppFile, "\t\t\targ.m_%s = c_%s_%s.m_%s;\n", pField->m_name.c_str(),
							callIntf.GetSrcToDstLc(), callIntf.GetIntfName(), pField->m_name.c_str());
						fprintf(cppFile, "\t\t\targ.m_data[%d] = iDatQueDat.m_data;\n", fldQwIdx);
						fprintf(cppFile, "\t\t\tc_%s_%s.m_%s = arg.m_%s;\n",
							callIntf.GetSrcToDstLc(), callIntf.GetIntfName(), pField->m_name.c_str(),
							pField->m_name.c_str());
					} else {
						if (pField->m_pType->m_typeName == "bool") {
							fprintf(cppFile, "\t\t\tc_%s_%s.m_%s = (iDatQueDat.m_data & 1) != 0;\n",
								callIntf.GetSrcToDstLc(), callIntf.GetIntfName(), pField->m_name.c_str());
						} else {
							fprintf(cppFile, "\t\t\tc_%s_%s.m_%s = (%s)iDatQueDat.m_data;\n",
								callIntf.GetSrcToDstLc(), callIntf.GetIntfName(), pField->m_name.c_str(),
								pField->m_pType->m_typeName.c_str());
						}
					}
					fprintf(cppFile, "\t\t\tbreak;\n");
					fprintf(cppFile, "\t\t}\n");
					argQwIdx += 1;
				}
			}
		}

		fprintf(cppFile, "\t\tdefault:\n");
		fprintf(cppFile, "\t\t\tassert(0);\n");
		fprintf(cppFile, "\t\t}\n");
	}

	fprintf(cppFile, "\t\tc_callArgCnt -= 1;\n");
	fprintf(cppFile, "\t\tc_htiToHif_datAvl = true;\n");
	fprintf(cppFile, "\t\tm_iDatQue.pop();\n");
	fprintf(cppFile, "\t\tbreak;\n");
	fprintf(cppFile, "\tcase HTI_CMD:\n");

	fprintf(cppFile, "\t\tif (r_%s_%sAvlCnt == 0)\n", callIntf.GetSrcToDstLc(), callIntf.GetIntfName());
	fprintf(cppFile, "\t\t\tbreak;\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\t\tc_callActiveCnt += 1;\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\t\tc_callState = HTI_DATA;\n");
	fprintf(cppFile, "\t\tc_%s_%sRdy = true;\n", callIntf.GetSrcToDstLc(), callIntf.GetIntfName());

	fprintf(cppFile, "\t\tbreak;\n");

	fprintf(cppFile, "\tcase HTI_DATA:\n");
	fprintf(cppFile, "\t\tif (m_iDatQue.empty())\n");
	fprintf(cppFile, "\t\t\tbreak;\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\t\tif (iDatQueDat.m_ctl == HT_DQ_CALL || iDatQueDat.m_ctl == HT_DQ_HALT) {\n");
	fprintf(cppFile, "\t\t\tc_callState = HTI_IDLE;\n");
	fprintf(cppFile, "\t\t\tbreak;\n");
	fprintf(cppFile, "\t\t}\n");
	fprintf(cppFile, "\n");

	if (pInQueIntf) {
		fprintf(cppFile, "\t\tif (r_hifTo%s_%sAvlCnt == 0)\n",
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\t\tc_hifTo%s_%sRdy = true;\n",
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
		fprintf(cppFile, "\t\tc_hifTo%s_%s = iDatQueDat;\n",
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
		fprintf(cppFile, "\t\tc_htiToHif_datAvl = true;\n");
		fprintf(cppFile, "\t\tm_iDatQue.pop();\n");
	}
	fprintf(cppFile, "\t\tbreak;\n");

	fprintf(cppFile, "\tdefault:\n");
	fprintf(cppFile, "\t\tassert(0);\n");
	fprintf(cppFile, "\t}\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\tht_uint1 c_%s_%sAvlCnt = r_%s_%sAvlCnt\n",
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName(),
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName());
	if (bCallClk2x)
		fprintf(cppFile, "\t\t+ r_%s_%sAvl_2x1.read() + r_%s_%sAvl_2x2.read() - c_%s_%sRdy;\n",
		callIntf.GetDstToSrcLc(), callIntf.GetIntfName(),
		callIntf.GetDstToSrcLc(), callIntf.GetIntfName(),
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName());
	else
		fprintf(cppFile, "\t\t+ i_%s_%sAvl.read() - c_%s_%sRdy;\n",
		callIntf.GetDstToSrcLc(), callIntf.GetIntfName(),
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName());

	fprintf(cppFile, "\n");

	if (pInQueIntf) {
		fprintf(cppFile, "\tht_uint6 c_hifTo%s_%sAvlCnt = r_hifTo%s_%sAvlCnt\n",
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str(),
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());

		if (bIhdClk2x)
			fprintf(cppFile, "\t\t+ r_%sToHif_%sAvl_2x1.read() + r_%sToHif_%sAvl_2x2.read() - c_hifTo%s_%sRdy;\n",
			pInQueIntf->m_modName.Lc().c_str(), pInQueIntf->m_queName.Lc().c_str(),
			pInQueIntf->m_modName.Lc().c_str(), pInQueIntf->m_queName.Lc().c_str(),
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
		else
			fprintf(cppFile, "\t\t+ i_%sToHif_%sAvl.read() - c_hifTo%s_%sRdy;\n",
			pInQueIntf->m_modName.Lc().c_str(), pInQueIntf->m_queName.Lc().c_str(),
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
	}

	fprintf(cppFile, "\t//\n");
	fprintf(cppFile, "\t// Return logic\n");
	fprintf(cppFile, "\t//\n");
	fprintf(cppFile, "\n");

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_ohm.m_bOutHostMsg) continue;

		fprintf(cppFile, "\tCHostCtrlMsgIntf c_%sToHti_ohm;\n", mod.m_modName.Lc().c_str());
		fprintf(cppFile, "\tif (i_%sToHti_ohm.read().m_bValid)\n", mod.m_modName.Lc().c_str());
		fprintf(cppFile, "\t\tc_%sToHti_ohm = i_%sToHti_ohm;\n", mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());
		fprintf(cppFile, "\telse\n");
		fprintf(cppFile, "\t\tc_%sToHti_ohm = r_%sToHti_ohm;\n", mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());
		fprintf(cppFile, "\n");
	}

	if (bRtnCmdFields) {
		if (!bRtnClk2x) {
			fprintf(cppFile, "\tif (i_%s_%sRdy.read())\n",
				rtnIntf.GetSrcToDstLc(), rtnIntf.GetIntfName());
			fprintf(cppFile, "\t\tm_oCmdQue.push(i_%s_%s);\n",
				rtnIntf.GetSrcToDstLc(), rtnIntf.GetIntfName());
		}
	} else {
		fprintf(cppFile, "\tsc_uint<5> c_oCmdCnt = r_oCmdCnt;\n");

		if (!bRtnClk2x)
			fprintf(cppFile, "\tif (i_%s_%sRdy.read())\n",
			rtnIntf.GetSrcToDstLc(), rtnIntf.GetIntfName());
		else
			fprintf(cppFile, "\tif (r_%s_%sRdy_2x.read())\n",
			rtnIntf.GetSrcToDstLc(), rtnIntf.GetIntfName());
		fprintf(cppFile, "\t\tc_oCmdCnt += 1;\n");
	}

	fprintf(cppFile, "\tbool c_%s_%sAvl = false;\n",
		rtnIntf.GetDstToSrcLc(), rtnIntf.GetIntfName());
	if (bRtnCmdFields)
		fprintf(cppFile, "\tC%s_%sIntf oCmdQueDat = m_oCmdQue.front();\n",
		rtnIntf.GetSrcToDstUc(), rtnIntf.GetIntfName());

	fprintf(cppFile, "\tsc_uint<2> c_rtnState = r_rtnState;\n");
	fprintf(cppFile, "\tuint8_t c_rtnCnt = r_rtnCnt;\n");

	fprintf(cppFile, "\tbool c_htiToHif_ctlRdy = false;\n");
	fprintf(cppFile, "\tCHostCtrlMsgIntf c_htiToHif_ctl;\n");
	fprintf(cppFile, "\tc_htiToHif_ctl.m_data64 = 0;\n");

	if (pOutQueIntf) {
		if (!bOhdClk2x)
			fprintf(cppFile, "\tif (i_%sToHif_%sRdy.read())\n\t\tm_%sToHif_%sQue.push(i_%sToHif_%s);\n",
			pOutQueIntf->m_modName.Lc().c_str(), pOutQueIntf->m_queName.Lc().c_str(), pOutQueIntf->m_modName.Lc().c_str(),
			pOutQueIntf->m_queName.Lc().c_str(), pOutQueIntf->m_modName.Lc().c_str(), pOutQueIntf->m_queName.Lc().c_str());

		string structName = pOutQueIntf->m_typeName + (pOutQueIntf->m_bInclude ? "_inc" : "");
		fprintf(cppFile, "\t%s %s = m_%sToHif_%sQue.front();\n",
			structName.c_str(), pOutQueIntf->m_queName.Lc().c_str(),
			pOutQueIntf->m_modName.Lc().c_str(), pOutQueIntf->m_queName.Lc().c_str());
		fprintf(cppFile, "\tbool c_hifTo%s_%sAvl = false;\n",
			pOutQueIntf->m_modName.Uc().c_str(), pOutQueIntf->m_queName.Lc().c_str());
	}

	int rtnQwCnt = 0;
	for (size_t fldIdx = 0; fldIdx < rtnIntf.m_pFieldList->size(); fldIdx += 1) {
		CField * pField = (*rtnIntf.m_pFieldList)[fldIdx];

		int fldQwCnt = (FindHostTypeWidth(pField) + 63) / 64;
		rtnQwCnt += fldQwCnt;
	}

	fprintf(cppFile, "\tswitch (r_rtnState) {\n");
	fprintf(cppFile, "\tcase HTI_IDLE:\n");

	if (pOutQueIntf) {
		fprintf(cppFile, "\t\tif (!m_%sToHif_%sQue.empty()) {\n",
			pOutQueIntf->m_modName.Lc().c_str(), pOutQueIntf->m_queName.Lc().c_str());
		fprintf(cppFile, "\t\t\tc_rtnState = HTI_DATA;\n");
		fprintf(cppFile, "\t\t} else\n");
	}

	if (bRtnCmdFields)
		fprintf(cppFile, "\t\tif (!m_oCmdQue.empty()) {\n");
	else
		fprintf(cppFile, "\t\tif (r_oCmdCnt > 0) {\n");

	fprintf(cppFile, "\t\t\tc_rtnState = HTI_CMD;\n");
	fprintf(cppFile, "\t\t}\n");

	fprintf(cppFile, "\t\tbreak;\n");
	fprintf(cppFile, "\tcase HTI_CMD:\n");
	fprintf(cppFile, "\t\tif (r_htiToHif_datAvlCnt == 0)\n");
	fprintf(cppFile, "\t\t\tbreak;\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\t\tc_htiToHif_datRdy = true;\n");
	fprintf(cppFile, "\t\tc_htiToHif_dat.m_ctl = HT_DQ_CALL;\n");
	fprintf(cppFile, "\t\tc_htiToHif_dat.m_data = %d;\n", rtnQwCnt);
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\t\tc_rtnCnt = %d;\n", rtnQwCnt);
	fprintf(cppFile, "\t\tc_rtnState = HTI_ARGS;\n");
	fprintf(cppFile, "\t\tbreak;\n");
	fprintf(cppFile, "\tcase HTI_ARGS:\n");
	fprintf(cppFile, "\t\tif (r_htiToHif_datAvlCnt == 0)\n");
	fprintf(cppFile, "\t\t\tbreak;\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "\t\tif (r_rtnCnt == 0) {\n");
	fprintf(cppFile, "\t\t\tc_htiToHif_datRdy = true;\n");
	fprintf(cppFile, "\t\t\tc_htiToHif_dat.m_ctl = HT_DQ_FLSH;\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "\t\t\tc_callActiveCnt -= 1;\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "\t\t\tc_rtnState = HTI_IDLE;\n");
	fprintf(cppFile, "\n");

	if (bRtnCmdFields)
		fprintf(cppFile, "\t\t\tm_oCmdQue.pop();\n");
	else
		fprintf(cppFile, "\t\t\tc_oCmdCnt -= 1;\n");

	fprintf(cppFile, "\t\t\tc_%s_%sAvl = true;\n", rtnIntf.GetDstToSrcLc(), rtnIntf.GetIntfName());
	fprintf(cppFile, "\t\t\tbreak;\n");
	fprintf(cppFile, "\t\t}\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\t\tc_rtnCnt -= 1;\n");
	fprintf(cppFile, "\t\tc_htiToHif_datRdy = true;\n");

	if (bRtnCmdFields) {

		fprintf(cppFile, "\t\tswitch (r_rtnCnt) {\n");

		int argQwIdx = 0;
		for (size_t fldIdx = 0; fldIdx < rtnIntf.m_pFieldList->size(); fldIdx += 1) {
			CField * pField = (*rtnIntf.m_pFieldList)[fldIdx];

			if (pField->m_pType->IsInt()) {
				fprintf(cppFile, "\t\tcase %d: {\n", rtnQwCnt - argQwIdx);
				fprintf(cppFile, "\t\t\tc_htiToHif_dat.m_data = (uint64_t)oCmdQueDat.m_%s;\n",
					pField->m_name.c_str());
				fprintf(cppFile, "\t\t\tbreak;\n");
				fprintf(cppFile, "\t\t}\n");
				argQwIdx += 1;

			} else {
				int fldQwCnt = (FindHostTypeWidth(pField) + 63) / 64;

				for (int fldQwIdx = 0; fldQwIdx < fldQwCnt; fldQwIdx += 1) {
					bool bBasicType;
					CTypeDef * pTypeDef;
					string type = pField->m_pType->m_typeName;
					while (!(bBasicType = IsBaseType(type))) {
						if (!(pTypeDef = FindTypeDef(pField->m_pType->m_typeName)))
							break;
						type = pTypeDef->m_type;
					}

					fprintf(cppFile, "\t\tcase %d: {\n", rtnQwCnt - argQwIdx);

					if (!bBasicType) {
						fprintf(cppFile, "\t\t\tunion Arg_%s {\n", pField->m_name.c_str());
						fprintf(cppFile, "\t\t\t\t%s m_%s;\n", pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());
						fprintf(cppFile, "\t\t\t\tuint64_t m_data[%d];\n", fldQwCnt);
						fprintf(cppFile, "\t\t\t} arg;\n");

						fprintf(cppFile, "\t\t\targ.m_%s = oCmdQueDat.m_%s;\n", pField->m_name.c_str(), pField->m_name.c_str());
						fprintf(cppFile, "\t\t\tc_htiToHif_dat.m_data = arg.m_data[%d];\n", fldQwIdx);
					} else {
						fprintf(cppFile, "\t\t\tc_htiToHif_dat.m_data = (uint64_t)oCmdQueDat.m_%s;\n", pField->m_name.c_str());
					}
					fprintf(cppFile, "\t\t\tbreak;\n");
					fprintf(cppFile, "\t\t}\n");
					argQwIdx += 1;
				}
			}
		}

		fprintf(cppFile, "\t\tdefault:\n");
		fprintf(cppFile, "\t\t\tassert(0);\n");
		fprintf(cppFile, "\t\t}\n");
	}
	fprintf(cppFile, "\t\tbreak;\n");

	if (pOutQueIntf) {
		fprintf(cppFile, "\tcase HTI_DATA:\n");
		fprintf(cppFile, "\t\tif (m_%sToHif_%sQue.empty()) {\n",
			pOutQueIntf->m_modName.Lc().c_str(), pOutQueIntf->m_queName.Lc().c_str());
		fprintf(cppFile, "\t\t\tc_rtnState = HTI_IDLE;\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\t\tif (r_htiToHif_datAvlCnt == 0)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\t\tif (%s.m_ctl == HT_DQ_NULL)\n", pOutQueIntf->m_queName.Lc().c_str());
		fprintf(cppFile, "\t\t\tc_rtnState = HTI_IDLE;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_htiToHif_datRdy = true;\n");
		fprintf(cppFile, "\t\tc_htiToHif_dat = %s;\n", pOutQueIntf->m_queName.Lc().c_str());
		fprintf(cppFile, "\t\tm_%sToHif_%sQue.pop();\n",
			pOutQueIntf->m_modName.Lc().c_str(), pOutQueIntf->m_queName.Lc().c_str());
		fprintf(cppFile, "\t\tc_hifTo%s_%sAvl = true;\n",
			pOutQueIntf->m_modName.Uc().c_str(), pOutQueIntf->m_queName.Lc().c_str());
		fprintf(cppFile, "\t\tbreak;\n");
	}

	fprintf(cppFile, "\tdefault:\n");
	fprintf(cppFile, "\t\tassert(0);\n");
	fprintf(cppFile, "\t}\n");
	fprintf(cppFile, "\n");

	vector<CModule> ohmSelNameList;
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_ohm.m_bOutHostMsg) continue;

		ohmSelNameList.push_back(mod);
		fprintf(cppFile, "\tbool c_htiTo%s_ohmAvl = false;\n", mod.m_modName.Uc().c_str());
	}
	int ohmSelCnt = ohmSelNameList.size();

	if (ohmSelCnt > 1)
		fprintf(cppFile, "\tht_uint%d c_htSelRr = r_htSelRr;\n", ohmSelCnt);

	if (ohmSelCnt == 1) {
		fprintf(cppFile, "\tbool c_%sohmSel = r_%sToHti_ohm.m_bValid;\n", ohmSelNameList[0].m_modName.Uc().c_str(), ohmSelNameList[0].m_modName.Lc().c_str());
	} else for (int i = 0; i < ohmSelCnt; i += 1) {
		fprintf(cppFile, "\tbool c_%sohmSel = r_%sToHti_ohm.m_bValid && ((r_htSelRr & 0x%lx) != 0 || !(\n",
			ohmSelNameList[i].m_modName.Uc().c_str(), ohmSelNameList[i].m_modName.Lc().c_str(), 1ul << i);
		for (int j = 1; j < ohmSelCnt; j += 1) {
			int k = (i + j) % ohmSelCnt;

			uint32_t mask1 = (1ul << ohmSelCnt) - 1;
			uint32_t mask2 = ((1ul << j) - 1) << (i + 1);
			uint32_t mask3 = (mask2 & mask1) | (mask2 >> ohmSelCnt);

			fprintf(cppFile, "\t\t(r_%sToHti_ohm.m_bValid && (r_htSelRr & 0x%x) != 0)%s\n",
				ohmSelNameList[k].m_modName.Lc().c_str(),
				mask3, j == ohmSelCnt - 1 ? "));" : " ||");
		}
	}

	if (ohmSelCnt) {
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tif (r_rtnState != HTI_CMD && r_htiToHif_ctlAvlCnt != 0) {\n");

		const char *pSeparator = "\t\t";
		for (int i = 0; i < ohmSelCnt; i += 1) {
			fprintf(cppFile, "%sif (c_%sohmSel) {\n", pSeparator, ohmSelNameList[i].m_modName.Uc().c_str());
			fprintf(cppFile, "\t\t\tc_htiToHif_ctlRdy = true;\n");
			fprintf(cppFile, "\t\t\tc_htiToHif_ctl = r_%sToHti_ohm;\n", ohmSelNameList[i].m_modName.Lc().c_str());
			fprintf(cppFile, "\t\t\tc_htiTo%s_ohmAvl = true;\n", ohmSelNameList[i].m_modName.Uc().c_str());
			fprintf(cppFile, "\t\t\tc_%sToHti_ohm.m_bValid = false;\n", ohmSelNameList[i].m_modName.Lc().c_str());
			if (ohmSelCnt > 1)
				if (i == ohmSelCnt - 1)
					fprintf(cppFile, "\t\t\tc_htSelRr = 0x1;\n");
				else
					fprintf(cppFile, "\t\t\tc_htSelRr = 0x%x;\n", 1 << (i + 1));

			fprintf(cppFile, "\t\t}");

			pSeparator = " else ";
		}
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");
	}

	fprintf(cppFile, "\tht_uint1 c_htiToHif_ctlAvlCnt = r_htiToHif_ctlAvlCnt;\n");
	fprintf(cppFile, "\tif (i_hifToHti_ctlAvl.read() != c_htiToHif_ctlRdy)\n");
	fprintf(cppFile, "\tc_htiToHif_ctlAvlCnt ^= 1u;\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\tht_uint6 c_htiToHif_datAvlCnt = r_htiToHif_datAvlCnt\n");
	fprintf(cppFile, "\t\t+ i_hifToHti_datAvl.read()\n");
	fprintf(cppFile, "\t\t- c_htiToHif_datRdy;\n");
	fprintf(cppFile, "\n");

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_ohm.m_bOutHostMsg) continue;

		fprintf(cppFile, "\tif (r_reset1x) c_%sToHti_ohm.m_bValid = false;\n", mod.m_modName.Lc().c_str());
	}
	if (ohmSelCnt)
		fprintf(cppFile, "\n");

	fprintf(cppFile, "\t///////////////////////////////////////////////////////////////////////////\n");
	fprintf(cppFile, "\t// Register assignments\n");
	fprintf(cppFile, "\t///////////////////////////////////////////////////////////////////////////\n");
	fprintf(cppFile, "\n");

	if (bIhm)
		fprintf(cppFile, "\tm_iCtlQue.clock(r_reset1x);\n");
	fprintf(cppFile, "\tm_iDatQue.clock(r_reset1x);\n");

	if (bRtnCmdFields) {
		if (bRtnClk2x)
			fprintf(cppFile, "\tm_oCmdQue.pop_clock(r_reset1x);\n");
		else
			fprintf(cppFile, "\tm_oCmdQue.clock(r_reset1x);\n");
	} else
		fprintf(cppFile, "\tr_oCmdCnt = r_reset1x ? (sc_uint<5>)0 : c_oCmdCnt;\n");

	if (pOutQueIntf) {
		if (!bOhdClk2x)
			fprintf(cppFile, "\tm_%sToHif_%sQue.clock(r_reset1x);\n",
			pOutQueIntf->m_modName.Lc().c_str(), pOutQueIntf->m_queName.Lc().c_str());
		else
			fprintf(cppFile, "\tm_%sToHif_%sQue.pop_clock(r_reset1x);\n",
			pOutQueIntf->m_modName.Lc().c_str(), pOutQueIntf->m_queName.Lc().c_str());
	}

	fprintf(cppFile, "\n");
	fprintf(cppFile, "\tr_htiToHif_ctlAvl = c_htiToHif_ctlAvl;\n");
	fprintf(cppFile, "\tr_htiToHif_datAvl = c_htiToHif_datAvl;\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\tr_callState = r_reset1x ? (sc_uint<2>)HTI_IDLE : c_callState;\n");
	fprintf(cppFile, "\tr_callArgCnt = c_callArgCnt;\n");
	fprintf(cppFile, "\tr_callActiveCnt = r_reset1x ? 0 : c_callActiveCnt;\n");
	fprintf(cppFile, "\n");

	if (bIhm) {
		fprintf(cppFile, "\tr_hifTo%s_ihm = c_hifTo%s_ihm;\n", m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
		fprintf(cppFile, "\n");
	}

	fprintf(cppFile, "\tr_%s_%sRdy = c_%s_%sRdy;\n",
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName(),
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName());
	if (bCallCmdFields)
		fprintf(cppFile, "\tr_%s_%s = c_%s_%s;\n",
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName(),
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName());
	fprintf(cppFile, "\tr_%s_%sAvlCnt = r_reset1x ? (ht_uint1)1 : c_%s_%sAvlCnt;\n",
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName(),
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName());

	fprintf(cppFile, "\tr_rtnState = r_reset1x ? (sc_uint<2>)HTI_IDLE : c_rtnState;\n");
	fprintf(cppFile, "\tr_rtnCnt = c_rtnCnt;\n");
	fprintf(cppFile, "\tr_htiToHif_ctlRdy = c_htiToHif_ctlRdy;\n");
	fprintf(cppFile, "\tr_htiToHif_ctl = c_htiToHif_ctl;\n");
	fprintf(cppFile, "\tr_htiToHif_ctlAvlCnt = r_reset1x ? (ht_uint1)1 : c_htiToHif_ctlAvlCnt;\n");
	fprintf(cppFile, "\tr_htiToHif_datRdy = c_htiToHif_datRdy;\n");
	fprintf(cppFile, "\tr_htiToHif_dat = c_htiToHif_dat;\n");
	fprintf(cppFile, "\tr_htiToHif_datAvlCnt = r_reset1x ? (ht_uint6)32 : c_htiToHif_datAvlCnt;\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\tr_%s_%sAvl = c_%s_%sAvl;\n",
		rtnIntf.GetDstToSrcLc(), rtnIntf.GetIntfName(),
		rtnIntf.GetDstToSrcLc(), rtnIntf.GetIntfName());

	fprintf(cppFile, "\tr_htiToHif_ctlRdy = c_htiToHif_ctlRdy;\n");
	fprintf(cppFile, "\n");

	if (pInQueIntf) {
		fprintf(cppFile, "\tr_hifTo%s_%sRdy = c_hifTo%s_%sRdy;\n",
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str(),
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
		fprintf(cppFile, "\tr_hifTo%s_%s = c_hifTo%s_%s;\n",
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str(),
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
		fprintf(cppFile, "\tr_hifTo%s_%sAvlCnt = r_reset1x ? (ht_uint6)32 : c_hifTo%s_%sAvlCnt;\n",
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str(),
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
	}

	if (pOutQueIntf) {
		fprintf(cppFile, "\tr_hifTo%s_%sAvl = c_hifTo%s_%sAvl;\n",
			pOutQueIntf->m_modName.Uc().c_str(), pOutQueIntf->m_queName.Lc().c_str(),
			pOutQueIntf->m_modName.Uc().c_str(), pOutQueIntf->m_queName.Lc().c_str());
	}

	fprintf(cppFile, "\n");

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_ohm.m_bOutHostMsg) continue;

		fprintf(cppFile, "\tr_%sToHti_ohm = c_%sToHti_ohm;\n", mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());
		fprintf(cppFile, "\tr_htiTo%s_ohmAvl = c_htiTo%s_ohmAvl;\n", mod.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str());
		fprintf(cppFile, "\n");
	}

	if (ohmSelCnt > 1)
		fprintf(cppFile, "\tr_htSelRr = r_reset1x ? (ht_uint%d)1 : c_htSelRr;\n", ohmSelCnt);

	fprintf(cppFile, "\tHtResetFlop(r_reset1x, i_reset.read());\n");
	if (bCallClk2x || bRtnClk2x)
		fprintf(cppFile, "\tc_reset2x = r_reset1x;\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\t// register properties\n");
	fprintf(cppFile, "\tht_attrib(keep, r_callState, \"true\");\n");
	fprintf(cppFile, "\tht_attrib(keep, r_rtnState, \"true\");\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\t///////////////////////////////////////////////////////////////////////////\n");
	fprintf(cppFile, "\t// Output assignments\n");
	fprintf(cppFile, "\t///////////////////////////////////////////////////////////////////////////\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\to_htiToHif_ctlAvl = r_htiToHif_ctlAvl;\n");
	fprintf(cppFile, "\to_htiToHif_datAvl = r_htiToHif_datAvl;\n");
	fprintf(cppFile, "\to_htiToHif_ctlRdy = r_htiToHif_ctlRdy;\n");
	fprintf(cppFile, "\to_htiToHif_ctl = r_htiToHif_ctl;\n");
	fprintf(cppFile, "\to_htiToHif_datRdy = r_htiToHif_datRdy;\n");
	fprintf(cppFile, "\to_htiToHif_dat = r_htiToHif_dat;\n");
	fprintf(cppFile, "\n");

	if (bIhm) {
		fprintf(cppFile, "\to_hifTo%s_ihm = r_hifTo%s_ihm;\n", m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
		fprintf(cppFile, "\n");
	}

	if (!bCallClk2x)
		fprintf(cppFile, "\to_%s_%sRdy = r_%s_%sRdy;\n",
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName(),
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName());
	if (bCallCmdFields)
		fprintf(cppFile, "\to_%s_%s = r_%s_%s;\n",
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName(),
		callIntf.GetSrcToDstLc(), callIntf.GetIntfName());
	if (!bRtnClk2x)
		fprintf(cppFile, "\to_%s_%sAvl = r_%s_%sAvl;\n",
		rtnIntf.GetDstToSrcLc(), rtnIntf.GetIntfName(),
		rtnIntf.GetDstToSrcLc(), rtnIntf.GetIntfName());
	fprintf(cppFile, "\n");

	if (pInQueIntf) {
		fprintf(cppFile, "\to_hifTo%s_%sRdy = r_hifTo%s_%sRdy;\n",
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str(),
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
		fprintf(cppFile, "\to_hifTo%s_%s = r_hifTo%s_%s;\n",
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str(),
			pInQueIntf->m_modName.Uc().c_str(), pInQueIntf->m_queName.Lc().c_str());
	}

	if (pOutQueIntf) {
		fprintf(cppFile, "\to_hifTo%s_%sAvl = r_hifTo%s_%sAvl;\n",
			pOutQueIntf->m_modName.Uc().c_str(), pOutQueIntf->m_queName.Lc().c_str(),
			pOutQueIntf->m_modName.Uc().c_str(), pOutQueIntf->m_queName.Lc().c_str());
	}

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_ohm.m_bOutHostMsg) continue;

		fprintf(cppFile, "\to_htiTo%s_ohmAvl = r_htiTo%s_ohmAvl;\n", mod.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str());
	}
	if (ohmSelCnt)
		fprintf(cppFile, "\n");

	fprintf(cppFile, "}\n");

	if (bCallClk2x || bRtnClk2x) {
		fprintf(cppFile, "\n");
		fprintf(cppFile, "void CPers%sHti::Pers%sHti_2x()\n", m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
		fprintf(cppFile, "{\n");

		if (bRtnClk2x && bRtnCmdFields) {
			fprintf(cppFile, "\tif (i_%s_%sRdy.read())\n",
				rtnIntf.GetSrcToDstLc(), rtnIntf.GetIntfName());
			fprintf(cppFile, "\t\tm_oCmdQue.push(i_%s_%s);\n",
				rtnIntf.GetSrcToDstLc(), rtnIntf.GetIntfName());
			fprintf(cppFile, "\n");
		}

		if (bOhdClk2x)
			fprintf(cppFile, "\tif (i_%sToHif_%sRdy.read())\n\t\tm_%sToHif_%sQue.push(i_%sToHif_%s);\n",
			pOutQueIntf->m_modName.Lc().c_str(), pOutQueIntf->m_queName.Lc().c_str(), pOutQueIntf->m_modName.Lc().c_str(),
			pOutQueIntf->m_queName.Lc().c_str(), pOutQueIntf->m_modName.Lc().c_str(), pOutQueIntf->m_queName.Lc().c_str());

		fprintf(cppFile, "\t///////////////////////////////////////////////////////////////////////////\n");
		fprintf(cppFile, "\t// Register assignments\n");
		fprintf(cppFile, "\t///////////////////////////////////////////////////////////////////////////\n");
		fprintf(cppFile, "\n");

		if (bRtnClk2x && bRtnCmdFields)
			fprintf(cppFile, "\tm_oCmdQue.push_clock(c_reset2x);\n\n");

		if (pOutQueIntf && bOhdClk2x)
			fprintf(cppFile, "\tm_%sToHif_%sQue.push_clock(r_reset1x);\n",
			pOutQueIntf->m_modName.Lc().c_str(), pOutQueIntf->m_queName.Lc().c_str());

		if (bCallClk2x) {
			fprintf(cppFile, "\tr_%s_%sRdy_2x = r_%s_%sRdy & r_phase;\n",
				callIntf.GetSrcToDstLc(), callIntf.GetIntfName(),
				callIntf.GetSrcToDstLc(), callIntf.GetIntfName());
			fprintf(cppFile, "\n");

			fprintf(cppFile, "\tr_%s_%sAvl_2x2 = r_%s_%sAvl_2x1;\n",
				callIntf.GetDstToSrcLc(), callIntf.GetIntfName(),
				callIntf.GetDstToSrcLc(), callIntf.GetIntfName());
			fprintf(cppFile, "\tr_%s_%sAvl_2x1 = i_%s_%sAvl.read();\n",
				callIntf.GetDstToSrcLc(), callIntf.GetIntfName(),
				callIntf.GetDstToSrcLc(), callIntf.GetIntfName());
			fprintf(cppFile, "\n");
		}

		if (bRtnClk2x) {
			if (!bRtnCmdFields)
				fprintf(cppFile, "\tr_%s_%sRdy_2x = i_%s_%sRdy || r_%s_%sRdy_2x && r_phase;\n",
				rtnIntf.GetSrcToDstLc(), rtnIntf.GetIntfName(),
				rtnIntf.GetSrcToDstLc(), rtnIntf.GetIntfName(),
				rtnIntf.GetSrcToDstLc(), rtnIntf.GetIntfName());

			fprintf(cppFile, "\tr_%s_%sAvl_2x = r_%s_%sAvl & r_phase;\n\n",
				rtnIntf.GetDstToSrcLc(), rtnIntf.GetIntfName(),
				rtnIntf.GetDstToSrcLc(), rtnIntf.GetIntfName());
		}

		if (bIhdClk2x) {
			fprintf(cppFile, "\tr_%sToHif_%sAvl_2x2 = r_%sToHif_%sAvl_2x1;\n",
				pInQueIntf->m_modName.Lc().c_str(), pInQueIntf->m_queName.Lc().c_str(),
				pInQueIntf->m_modName.Lc().c_str(), pInQueIntf->m_queName.Lc().c_str());
			fprintf(cppFile, "\tr_%sToHif_%sAvl_2x1 = i_%sToHif_%sAvl.read();\n",
				pInQueIntf->m_modName.Lc().c_str(), pInQueIntf->m_queName.Lc().c_str(),
				pInQueIntf->m_modName.Lc().c_str(), pInQueIntf->m_queName.Lc().c_str());
			fprintf(cppFile, "\n");
		}

		fprintf(cppFile, "\tht_attrib(equivalent_register_removal, r_phase, \"no\");\n");
		fprintf(cppFile, "\tr_phase = c_reset2x.read() || !r_phase;\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\t///////////////////////////////////////////////////////////////////////////\n");
		fprintf(cppFile, "\t// Output assignments\n");
		fprintf(cppFile, "\t///////////////////////////////////////////////////////////////////////////\n");
		fprintf(cppFile, "\n");

		if (bCallClk2x)
			fprintf(cppFile, "\to_%s_%sRdy = r_%s_%sRdy_2x;\n",
			callIntf.GetSrcToDstLc(), callIntf.GetIntfName(),
			callIntf.GetSrcToDstLc(), callIntf.GetIntfName());

		if (bRtnClk2x)
			fprintf(cppFile, "\to_%s_%sAvl = r_%s_%sAvl_2x;\n",
			rtnIntf.GetDstToSrcLc(), rtnIntf.GetIntfName(),
			rtnIntf.GetDstToSrcLc(), rtnIntf.GetIntfName());

		fprintf(cppFile, "}\n");
	}

	cppFile.FileClose();
}
