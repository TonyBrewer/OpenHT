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

// Generate a module's barrier code

void CDsnInfo::InitAndValidateModBar()
{
	// first initialize HtStrings
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		mod.m_rsmSrcCnt += mod.m_barrierList.size();

		for (size_t barIdx = 0; barIdx < mod.m_barrierList.size(); barIdx += 1) {
			CBarrier * pBar = mod.m_barrierList[barIdx];

			pBar->m_barIdW.InitValue(pBar->m_lineInfo, false, 0);
		}
	}

}

void CDsnInfo::GenModBarStatements(CModule &mod)
{
	if (mod.m_barrierList.size() == 0)
		return;

	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";
	string vcdModName = VA("Pers%s", mod.m_modName.Uc().c_str());

	string reset = mod.m_clkRate == eClk1x ? "r_reset1x" : "c_reset2x";

	int rlsCntBits = 0;
	switch (mod.m_modInstList.size()) {
	case 1: rlsCntBits = 1; break;
	case 2: rlsCntBits = 2; break;
	case 3: rlsCntBits = 2; break;
	case 4: rlsCntBits = 3; break;
	case 5: rlsCntBits = 3; break;
	case 6: rlsCntBits = 3; break;
	case 7: rlsCntBits = 4; break;
	default: HtlAssert(0); break;
	}

	g_appArgs.GetDsnRpt().AddLevel("Barrier\n");

	for (size_t barIdx = 0; barIdx < mod.m_barrierList.size(); barIdx += 1) {
		CBarrier * pBar = mod.m_barrierList[barIdx];

		bool bMimicHtCont = mod.m_threads.m_htIdW.AsInt() == 0 && mod.m_modInstList.size() == 1;

		CHtCode & barPreInstr = mod.m_clkRate == eClk2x ? m_barPreInstr2x : m_barPreInstr1x;
		CHtCode & barPostInstr = mod.m_clkRate == eClk2x ? m_barPostInstr2x : m_barPostInstr1x;
		CHtCode & barReg = mod.m_clkRate == eClk2x ? m_barReg2x : m_barReg1x;
		CHtCode & barOut = mod.m_clkRate == eClk2x ? m_barOut2x : m_barOut1x;

		string barName = pBar->m_name.size() == 0 ? "" : "_" + pBar->m_name.AsStr();

		g_appArgs.GetDsnRpt().AddItem("void HtBarrier%s(", barName.c_str());
		m_barFuncDecl.Append("\tvoid HtBarrier%s(", barName.c_str());
		m_barFuncDef.Append("void CPers%s%s::HtBarrier%s(",
			unitNameUc.c_str(), mod.m_modName.Uc().c_str(), barName.c_str());

		if (pBar->m_barIdW.size() > 0) {
			int barIdW = pBar->m_barIdW.AsInt() > 0 ? pBar->m_barIdW.AsInt() : 1;
			g_appArgs.GetDsnRpt().AddText("ht_uint%d barId, ", barIdW);
			m_barFuncDecl.Append("ht_uint%d barId, ", barIdW);
			m_barFuncDef.Append("ht_uint%d %sbarId, ", barIdW,
				(pBar->m_barIdW.AsInt() == 0 || bMimicHtCont) ? "ht_noload " : "");
		}

		g_appArgs.GetDsnRpt().AddText("sc_uint<%s_INSTR_W> releaseInstr, sc_uint<%s_HTID_W+%d> releaseCnt)\n",
			mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str(), rlsCntBits);
		m_barFuncDecl.Append("sc_uint<%s_INSTR_W> releaseInstr, sc_uint<%s_HTID_W+%d> releaseCnt);\n",
			mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str(), rlsCntBits);
		m_barFuncDef.Append("sc_uint<%s_INSTR_W> releaseInstr, sc_uint<%s_HTID_W+%d> %sreleaseCnt)\n",
			mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str(), rlsCntBits,
			bMimicHtCont ? "ht_noload " : "");
		m_barFuncDef.Append("{\n");
		if (pBar->m_barIdW.size() > 0 && pBar->m_barIdW.AsInt() == 0)
			m_barFuncDef.Append("\tassert_msg(barId == 0, \"Runtime check failed in CPers%s%s::HtBarrier%s(...)"
			"\"\n\t\t\" - barId out of range (barId == %%d)\\n\", (int)barId);\n",
			unitNameUc.c_str(), mod.m_modName.Uc().c_str(), barName.c_str());
		m_barFuncDef.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s%s::HtBarrier%s(...)"
			"\"\n\t\t\" - previous thread control routine was called\\n\");\n",
			mod.m_execStg, unitNameUc.c_str(), mod.m_modName.Uc().c_str(), barName.c_str());
		if (bMimicHtCont) {
			// mimic HtContinue
			m_barFuncDef.Append("\tassert_msg(releaseCnt == 1, \"Runtime check failed in CPers%s%s::HtBarrier%s(...)"
				"\"\n\t\t\" - expected releaseCnt to be 1\\n\");\n",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(), barName.c_str());
			m_barFuncDef.Append("\tc_t%d_htCtrl = HT_CONT;\n", mod.m_execStg);
			m_barFuncDef.Append("\tc_t%d_htNextInstr = releaseInstr;\n", mod.m_execStg);
		} else {
			m_barFuncDef.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
			m_barFuncDef.Append("\tc_t%d_bar%sEnter = true;\n", mod.m_execStg, pBar->m_name.Uc().c_str());
			if (pBar->m_barIdW.AsInt() > 0)
				m_barFuncDef.Append("\tc_t%d_bar%sId = barId;\n", mod.m_execStg, pBar->m_name.Uc().c_str());
			m_barFuncDef.Append("\tc_t%d_bar%sReleaseInstr = releaseInstr;\n", mod.m_execStg, pBar->m_name.Uc().c_str());
			m_barFuncDef.Append("\tc_t%d_bar%sReleaseCnt = releaseCnt;\n", mod.m_execStg, pBar->m_name.Uc().c_str());
		}

		m_barFuncDef.Append("}\n");
		m_barFuncDef.Append("\n");

		if (bMimicHtCont) continue;

		if (mod.m_modInstList.size() > 1) {
			m_barIoDecl.Append("\t// Centralized bar%s interface\n", pBar->m_name.Uc().c_str());
			m_barIoDecl.Append("\tsc_out<bool> o_%sToBarCtl_bar%sEnter;\n",
				mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str());
			if (pBar->m_barIdW.AsInt() > 0)
				m_barIoDecl.Append("\tsc_out<ht_uint%d > o_%sToBarCtl_bar%sId;\n",
				pBar->m_barIdW.AsInt(), mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str());
			m_barIoDecl.Append("\tsc_out<sc_uint<%s_HTID_W+%d> > o_%sToBarCtl_bar%sReleaseCnt;\n",
				mod.m_modName.Upper().c_str(), rlsCntBits, mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str());
			m_barIoDecl.Append("\tsc_in<bool> i_barCtlTo%s_bar%sRelease;\n",
				mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str());
			if (pBar->m_barIdW.AsInt() > 0)
				m_barIoDecl.Append("\tsc_in<ht_uint%d > i_barCtlTo%s_bar%sId;\n",
				pBar->m_barIdW.AsInt(), mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str());
			m_barIoDecl.Append("\n");
		}

		m_barRegDecl.Append("\tbool c_t%d_bar%sEnter;\n", mod.m_execStg, pBar->m_name.Uc().c_str());
		GenModDecl(eVcdAll, m_barRegDecl, vcdModName, "bool", VA("r_t%d_bar%sEnter", mod.m_execStg + 1, pBar->m_name.Uc().c_str()));
		barPreInstr.Append("\tc_t%d_bar%sEnter = false;\n", mod.m_execStg, pBar->m_name.Uc().c_str());
		barReg.Append("\tr_t%d_bar%sEnter = c_t%d_bar%sEnter;\n",
			mod.m_execStg + 1, pBar->m_name.Uc().c_str(), mod.m_execStg, pBar->m_name.Uc().c_str());

		if (pBar->m_barIdW.AsInt() > 0) {
			m_barRegDecl.Append("\tht_uint%d c_t%d_bar%sId;\n", pBar->m_barIdW.AsInt(), mod.m_execStg, pBar->m_name.Uc().c_str());
			GenModDecl(eVcdAll, m_barRegDecl, vcdModName, VA("ht_uint%d", pBar->m_barIdW.AsInt()), VA("r_t%d_bar%sId", mod.m_execStg + 1, pBar->m_name.Uc().c_str()));
			barPreInstr.Append("\tc_t%d_bar%sId = 0;\n", mod.m_execStg, pBar->m_name.Uc().c_str());
			barReg.Append("\tr_t%d_bar%sId = c_t%d_bar%sId;\n",
				mod.m_execStg + 1, pBar->m_name.Uc().c_str(), mod.m_execStg, pBar->m_name.Uc().c_str());
		}

		m_barRegDecl.Append("\tsc_uint<%s_INSTR_W> c_t%d_bar%sReleaseInstr;\n",
			mod.m_modName.Upper().c_str(), mod.m_execStg, pBar->m_name.Uc().c_str());
		GenModDecl(eVcdAll, m_barRegDecl, vcdModName, VA("sc_uint<%s_INSTR_W>", mod.m_modName.Upper().c_str()),
			VA("r_t%d_bar%sReleaseInstr", mod.m_execStg + 1, pBar->m_name.Uc().c_str()));
		barPreInstr.Append("\tc_t%d_bar%sReleaseInstr = 0;\n", mod.m_execStg, pBar->m_name.Uc().c_str());
		barReg.Append("\tr_t%d_bar%sReleaseInstr = c_t%d_bar%sReleaseInstr;\n",
			mod.m_execStg + 1, pBar->m_name.Uc().c_str(), mod.m_execStg, pBar->m_name.Uc().c_str());

		m_barRegDecl.Append("\tsc_uint<%s_HTID_W+%d> c_t%d_bar%sReleaseCnt;\n",
			mod.m_modName.Upper().c_str(), rlsCntBits, mod.m_execStg, pBar->m_name.Uc().c_str());
		GenModDecl(eVcdAll, m_barRegDecl, vcdModName, VA("sc_uint<%s_HTID_W+%d>", mod.m_modName.Upper().c_str(), rlsCntBits),
			VA("r_t%d_bar%sReleaseCnt", mod.m_execStg + 1, pBar->m_name.Uc().c_str()));
		barPreInstr.Append("\tc_t%d_bar%sReleaseCnt = 0;\n", mod.m_execStg, pBar->m_name.Uc().c_str());
		barReg.Append("\tr_t%d_bar%sReleaseCnt = c_t%d_bar%sReleaseCnt;\n",
			mod.m_execStg + 1, pBar->m_name.Uc().c_str(), mod.m_execStg, pBar->m_name.Uc().c_str());

		if (barIdx == 0) {
			m_barRegDecl.Append("\tstruct CBarRlsPtr {\n");
			m_barRegDecl.Append("\t\tbool m_bEol;\n");
			if (mod.m_threads.m_htIdW.AsInt() > 0)
				m_barRegDecl.Append("\t\tsc_uint<%s_HTID_W> m_ptr;\n", mod.m_modName.Upper().c_str());
			m_barRegDecl.Append("#\t\tifndef _HTV\n");
			m_barRegDecl.Append("\t\tfriend void sc_trace(sc_trace_file *tf, const CBarRlsPtr & v, const std::string & NAME ) {\n");
			m_barRegDecl.Append("\t\t\tsc_trace(tf, v.m_bEol, NAME + \".m_bEol\");\n");
			if (mod.m_threads.m_htIdW.AsInt() > 0)
				m_barRegDecl.Append("\t\t\tsc_trace(tf, v.m_ptr, NAME + \".m_ptr\");\n");
			m_barRegDecl.Append("\t\t}\n");
			m_barRegDecl.Append("#\t\tendif\n");
			m_barRegDecl.Append("\t};\n");

			m_barRegDecl.Append("\tstruct CBarRlsInfo {\n");
			m_barRegDecl.Append("\t\tCBarRlsPtr m_link;\n");
			m_barRegDecl.Append("\t\tsc_uint<%s_INSTR_W> m_instr;\n", mod.m_modName.Upper().c_str());
			m_barRegDecl.Append("#\t\tifndef _HTV\n");
			m_barRegDecl.Append("\t\tfriend void sc_trace(sc_trace_file *tf, const CBarRlsInfo & v, const std::string & NAME ) {\n");
			m_barRegDecl.Append("\t\t\tsc_trace(tf, v.m_link, NAME + \".m_link\");\n");
			m_barRegDecl.Append("\t\t\tsc_trace(tf, v.m_instr, NAME + \".m_instr\");\n");
			m_barRegDecl.Append("\t\t}\n");
			m_barRegDecl.Append("#\t\tendif\n");
			m_barRegDecl.Append("\t};\n");
		}

		if (pBar->m_barIdW.AsInt() == 0) {
			GenModDecl(eVcdAll, m_barRegDecl, vcdModName, "bool", VA("r_bar%sActive", pBar->m_name.Uc().c_str()));
			barPreInstr.Append("\tbool c_bar%sActive = r_bar%sActive;\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			barReg.Append("\tr_bar%sActive = !%s && c_bar%sActive;\n",
				pBar->m_name.Uc().c_str(), reset.c_str(), pBar->m_name.Uc().c_str());

			if (mod.m_modInstList.size() == 1) {
				GenModDecl(eVcdAll, m_barRegDecl, vcdModName, VA("sc_uint<%s_HTID_W+%d>", mod.m_modName.Upper().c_str(), rlsCntBits),
					VA("r_bar%sEnterCnt", pBar->m_name.Uc().c_str()));
				barPreInstr.Append("\tsc_uint<%s_HTID_W+%d> c_bar%sEnterCnt = r_bar%sEnterCnt;\n",
					mod.m_modName.Upper().c_str(), rlsCntBits, pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
				barReg.Append("\tr_bar%sEnterCnt = c_bar%sEnterCnt;\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());

				m_barRegDecl.Append("#\tif !defined(_HTV) || defined(HT_ASSERT)\n");
				GenModDecl(eVcdAll, m_barRegDecl, vcdModName, VA("sc_uint<%s_HTID_W+%d>", mod.m_modName.Upper().c_str(), rlsCntBits),
					VA("r_bar%sReleaseCnt", pBar->m_name.Uc().c_str()));
				m_barRegDecl.Append("#\tendif\n");

				barPreInstr.Append("#\tif !defined(_HTV) || defined(HT_ASSERT)\n");
				barPreInstr.Append("\tsc_uint<%s_HTID_W+%d> c_bar%sReleaseCnt = r_bar%sReleaseCnt;\n",
					mod.m_modName.Upper().c_str(), rlsCntBits, pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
				barPreInstr.Append("#\tendif\n");

				barReg.Append("#\tif !defined(_HTV) || defined(HT_ASSERT)\n");
				barReg.Append("\tr_bar%sReleaseCnt = c_bar%sReleaseCnt;\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
				barReg.Append("#\tendif\n");
			}

			GenModDecl(eVcdAll, m_barRegDecl, vcdModName, "CBarRlsInfo", VA("r_bar%sRlsHead", pBar->m_name.Uc().c_str()));
			barPreInstr.Append("\tCBarRlsInfo c_bar%sRlsHead = r_bar%sRlsHead;\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			barReg.Append("\tr_bar%sRlsHead = c_bar%sRlsHead;\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());

		} else {
			m_barRegDecl.Append("\tht_dist_ram<bool, %d> m_bar%sActive;\n",
				pBar->m_barIdW.AsInt(), pBar->m_name.Uc().c_str());

			if (mod.m_modInstList.size() == 1) {
				barPreInstr.Append("\tm_bar%sActive.read_addr(r_t%d_bar%sId);\n",
					pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());
				barPreInstr.Append("\tm_bar%sActive.write_addr(r_t%d_bar%sId);\n",
					pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());
			} else {
				barPreInstr.Append("\tm_bar%sActive.read_addr(r_t%d_bar%sEnter ? r_t%d_bar%sId : r_bar%sRlsId);\n",
					pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str(),
					mod.m_execStg + 1, pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
				barPreInstr.Append("\tm_bar%sActive.write_addr(r_t%d_bar%sEnter ? r_t%d_bar%sId : r_bar%sRlsId);\n",
					pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str(),
					mod.m_execStg + 1, pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			}
			barPreInstr.Append("\tbool c_bar%sActive = m_bar%sActive.read_mem();\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			barReg.Append("\tm_bar%sActive.clock();\n", pBar->m_name.Uc().c_str());

			if (mod.m_modInstList.size() == 1) {
				m_barRegDecl.Append("\tht_dist_ram<sc_uint<%s_HTID_W+%d>, %d> m_bar%sEnterCnt;\n",
					mod.m_modName.Upper().c_str(), rlsCntBits, pBar->m_barIdW.AsInt(), pBar->m_name.Uc().c_str());
				barPreInstr.Append("\tm_bar%sEnterCnt.read_addr(r_t%d_bar%sId);\n",
					pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());
				barPreInstr.Append("\tm_bar%sEnterCnt.write_addr(r_t%d_bar%sId);\n",
					pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());
				barPreInstr.Append("\tsc_uint<%s_HTID_W+%d> c_bar%sEnterCnt = m_bar%sEnterCnt.read_mem();\n",
					mod.m_modName.Upper().c_str(), rlsCntBits, pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
				barReg.Append("\tm_bar%sEnterCnt.clock();\n", pBar->m_name.Uc().c_str());
			}

			if (mod.m_modInstList.size() == 1) {
				m_barRegDecl.Append("#\tif !defined(_HTV) || defined(HT_ASSERT)\n");
				m_barRegDecl.Append("\tht_dist_ram<sc_uint<%s_HTID_W+%d>, %d> m_bar%sReleaseCnt;\n",
					mod.m_modName.Upper().c_str(), rlsCntBits, pBar->m_barIdW.AsInt(), pBar->m_name.Uc().c_str());
				m_barRegDecl.Append("#\tendif\n");

				barPreInstr.Append("#\tif !defined(_HTV) || defined(HT_ASSERT)\n");
				barPreInstr.Append("\tm_bar%sReleaseCnt.read_addr(r_t%d_bar%sId);\n",
					pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());
				barPreInstr.Append("\tm_bar%sReleaseCnt.write_addr(r_t%d_bar%sId);\n",
					pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());
				barPreInstr.Append("\tsc_uint<%s_HTID_W+%d> c_bar%sReleaseCnt = m_bar%sReleaseCnt.read_mem();\n",
					mod.m_modName.Upper().c_str(), rlsCntBits, pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
				barPreInstr.Append("#\tendif\n");

				barReg.Append("#\tif !defined(_HTV) || defined(HT_ASSERT)\n");
				barReg.Append("\tm_bar%sReleaseCnt.clock();\n", pBar->m_name.Uc().c_str());
				barReg.Append("#\tendif\n");
			}

			m_barRegDecl.Append("\tht_dist_ram<CBarRlsInfo, %d> m_bar%sRlsHead;\n",
				pBar->m_barIdW.AsInt(), pBar->m_name.Uc().c_str());
			if (mod.m_modInstList.size() == 1) {
				barPreInstr.Append("\tm_bar%sRlsHead.read_addr(r_t%d_bar%sId);\n",
					pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());
				barPreInstr.Append("\tm_bar%sRlsHead.write_addr(r_t%d_bar%sId);\n",
					pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());
			} else {
				barPreInstr.Append("\tm_bar%sRlsHead.read_addr(r_t%d_bar%sEnter ? r_t%d_bar%sId : r_bar%sRlsId);\n",
					pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str(),
					mod.m_execStg + 1, pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
				barPreInstr.Append("\tm_bar%sRlsHead.write_addr(r_t%d_bar%sEnter ? r_t%d_bar%sId : r_bar%sRlsId);\n",
					pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str(),
					mod.m_execStg + 1, pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			}
			barPreInstr.Append("\tCBarRlsInfo c_bar%sRlsHead = m_bar%sRlsHead.read_mem();\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			barReg.Append("\tm_bar%sRlsHead.clock();\n", pBar->m_name.Uc().c_str());

			m_barRegDecl.Append("\tht_uint%d r_bar%sInitCnt;\n",
				pBar->m_barIdW.AsInt() + 1, pBar->m_name.Uc().c_str());
			barReg.Append("\tr_bar%sInitCnt = %s ? (ht_uint%d)0 : c_bar%sInitCnt;\n",
				pBar->m_name.Uc().c_str(), reset.c_str(), pBar->m_barIdW.AsInt() + 1, pBar->m_name.Uc().c_str());
		}

		GenModDecl(eVcdAll, m_barRegDecl, vcdModName, "CBarRlsInfo", VA("r_bar%sRlsNext", pBar->m_name.Uc().c_str()));
		barPreInstr.Append("\tCBarRlsInfo c_bar%sRlsNext = r_bar%sRlsNext;\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
		barReg.Append("\tr_bar%sRlsNext = c_bar%sRlsNext;\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());

		if (mod.m_modInstList.size() > 1) {
			GenModDecl(eVcdAll, m_barRegDecl, vcdModName, "bool", VA("r_bar%sRelease", pBar->m_name.Uc().c_str()));
			barPreInstr.Append("\tbool c_bar%sRelease = r_bar%sRelease;\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			barReg.Append("\tr_bar%sRelease = !%s && c_bar%sRelease;\n",
				pBar->m_name.Uc().c_str(), reset.c_str(), pBar->m_name.Uc().c_str());

			if (pBar->m_barIdW.AsInt() > 0) {
				GenModDecl(eVcdAll, m_barRegDecl, vcdModName, VA("ht_uint%d", pBar->m_barIdW.AsInt()), VA("r_bar%sRlsId", pBar->m_name.Uc().c_str()));
				barReg.Append("\tr_bar%sRlsId = c_bar%sRlsId;\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());

				if (g_appArgs.IsVcdAllEnabled()) {
					m_barRegDecl.Append("#\tifndef _HTV\n");
					GenModDecl(eVcdAll, m_barRegDecl, vcdModName, "CBarRlsInfo", VA("r_bar%sRlsHead", pBar->m_name.Uc().c_str()));
					m_barRegDecl.Append("#\tendif\n");

					barReg.Append("#\tifndef _HTV\n");
					barReg.Append("\tr_bar%sRlsHead = c_bar%sRlsHead;\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
					barReg.Append("#\tendif\n");
				}
			}
		}

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			m_barRegDecl.Append("\tht_dist_ram<CBarRlsPtr, %d> m_bar%sRlsList;\n", mod.m_threads.m_htIdW.AsInt(), pBar->m_name.Uc().c_str());
			barReg.Append("\tm_bar%sRlsList.clock();\n", pBar->m_name.Uc().c_str());
			barPreInstr.Append("\tm_bar%sRlsList.write_addr(r_t%d_htId);\n",
				pBar->m_name.Uc().c_str(), mod.m_execStg + 1);

			m_barRegDecl.Append("\tht_dist_que<CBarRlsInfo, %d> m_bar%sRlsQue;\n", mod.m_threads.m_htIdW.AsInt(), pBar->m_name.Uc().c_str());
			barReg.Append("\tm_bar%sRlsQue.clock(%s);\n", pBar->m_name.Uc().c_str(), reset.c_str());
		}

		if (mod.m_modInstList.size() > 1 && mod.m_threads.m_htIdW.AsInt() > 0) {
			if (pBar->m_barIdW.AsInt() > 0) {
				m_barRegDecl.Append("\tht_dist_que<ht_uint%d, %d> m_bar%sRlsIdQue;\n",
					pBar->m_barIdW.AsInt(), mod.m_threads.m_htIdW.AsInt(), pBar->m_name.Uc().c_str());
				barReg.Append("\tm_bar%sRlsIdQue.clock(%s);\n", pBar->m_name.Uc().c_str(), reset.c_str());
			} else {
				m_barRegDecl.Append("\tsc_uint<%s_HTID_W+%d> r_bar%sRlsQueCnt;\n",
					mod.m_modName.Upper().c_str(), rlsCntBits, pBar->m_name.Uc().c_str());
				barPreInstr.Append("\tsc_uint<%s_HTID_W+%d> c_bar%sRlsQueCnt = r_bar%sRlsQueCnt;\n",
					mod.m_modName.Upper().c_str(), rlsCntBits, pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
				barReg.Append("\tr_bar%sRlsQueCnt = %s ? (sc_uint<%s_HTID_W+%d>)0 : c_bar%sRlsQueCnt;\n",
					pBar->m_name.Uc().c_str(),
					reset.c_str(),
					mod.m_modName.Upper().c_str(), rlsCntBits, pBar->m_name.Uc().c_str());
			}
		}

		barPostInstr.Append("\tif (r_t%d_bar%sEnter) {\n", mod.m_execStg + 1, pBar->m_name.Uc().c_str());

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			barPostInstr.Append("\t\tif (!c_bar%sActive) {\n", pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\t\tc_bar%sActive = true;\n", pBar->m_name.Uc().c_str());

			if (mod.m_modInstList.size() == 1) {
				barPostInstr.Append("\t\t\tc_bar%sEnterCnt = r_t%d_bar%sReleaseCnt;\n",
					pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());
				barPostInstr.Append("#\t\t\tif !defined(_HTV) || defined(HT_ASSERT)\n");
				barPostInstr.Append("\t\t\tc_bar%sReleaseCnt = r_t%d_bar%sReleaseCnt;\n",
					pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());
				barPostInstr.Append("#\t\t\tendif\n");
			}

			barPostInstr.Append("\t\t\tc_bar%sRlsHead.m_link.m_bEol = true;\n", pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\t} else {\n");
			if (mod.m_modInstList.size() == 1) {
				barPostInstr.Append("\t\t\tassert_msg(%s || c_bar%sReleaseCnt == r_t%d_bar%sReleaseCnt, \"Runtime check failed in CPersBarrier::PersBarrier()\""
					"\n\t\t\t\t\" - calls to HtBarrier have inconsistent releaseCnt value\\n\");\n", 
					reset.c_str(),
					pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());
			}
			barPostInstr.Append("\t\t\tassert_msg(%s || c_bar%sRlsHead.m_instr == r_t%d_bar%sReleaseInstr, \"Runtime check failed in CPersBarrier::PersBarrier()\""
				"\n\t\t\t\t\" - calls to HtBarrier have inconsistent releaseInstr value\\n\");\n",
				reset.c_str(),
				pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\t\tc_bar%sActive = true;\n", pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\t\tc_bar%sRlsHead.m_link.m_bEol = false;\n", pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\t}\n");
			barPostInstr.Append("\n");
			barPostInstr.Append("\t\tm_bar%sRlsList.write_mem(c_bar%sRlsHead.m_link);\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\tc_bar%sRlsHead.m_link.m_ptr = r_t%d_htId;\n",
				pBar->m_name.Uc().c_str(), mod.m_execStg + 1);
			barPostInstr.Append("\t\tc_bar%sRlsHead.m_link.m_bEol = false;\n", pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\tc_bar%sRlsHead.m_instr = r_t%d_bar%sReleaseInstr;\n",
				pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());

		} else {
			barPostInstr.Append("\t\tc_bar%sActive = true;\n", pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\tc_bar%sRlsHead.m_link.m_bEol = false;\n", pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\tc_bar%sRlsHead.m_instr = r_t%d_bar%sReleaseInstr;\n",
				pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());
		}

		if (mod.m_modInstList.size() == 1) {
			barPostInstr.Append("\n");
			barPostInstr.Append("\t\tif (c_bar%sEnterCnt == 1) {\n",
				pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\t\tc_bar%sActive = false;\n", pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\t\tm_bar%sRlsQue.push(c_bar%sRlsHead);\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\t}\n");
			barPostInstr.Append("\n");

			barPostInstr.Append("\t\tc_bar%sEnterCnt -= 1;\n", pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t}\n");
		} else {
			barPostInstr.Append("\t} else if (r_bar%sRelease) {\n", pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\tif (c_bar%sActive)\n", pBar->m_name.Uc().c_str());

			if (mod.m_threads.m_htIdW.AsInt() > 0)
				barPostInstr.Append("\t\t\tm_bar%sRlsQue.push(c_bar%sRlsHead);\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			else
				barPostInstr.Append("\t\t\tc_bar%sRlsNext = c_bar%sRlsHead;\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());

			barPostInstr.Append("\t\tc_bar%sRelease = false;\n", pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\tc_bar%sActive = false;\n", pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t}\n");
		}

		if (pBar->m_barIdW.AsInt() > 0) {
			barPostInstr.Append("\n");
			barPostInstr.Append("\tht_uint%d c_bar%sInitCnt = r_bar%sInitCnt;\n",
				pBar->m_barIdW.AsInt() + 1, pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());

			if (mod.m_modInstList.size() > 1)
				barPostInstr.Append("\tht_uint%d c_bar%sRlsId = r_bar%sRlsId;\n",
				pBar->m_barIdW.AsInt(), pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());

			barPostInstr.Append("\tif (%s)\n", reset.c_str());

			if (mod.m_modInstList.size() == 1)
				barPostInstr.Append("\t\tc_t%d_bar%sId = 0;\n", mod.m_execStg, pBar->m_name.Uc().c_str());
			else
				barPostInstr.Append("\t\tc_bar%sRlsId = 0;\n", pBar->m_name.Uc().c_str());

			barPostInstr.Append("\telse if (r_bar%sInitCnt[%d] == 0) {\n",
				pBar->m_name.Uc().c_str(), pBar->m_barIdW.AsInt());
			barPostInstr.Append("\t\tc_bar%sInitCnt = r_bar%sInitCnt + 1;\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());

			if (mod.m_modInstList.size() == 1)
				barPostInstr.Append("\t\tc_t%d_bar%sId = c_bar%sInitCnt(%d,0);\n",
				mod.m_execStg, pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str(), pBar->m_barIdW.AsInt() - 1);
			else {
				barPostInstr.Append("\t\tc_bar%sRlsId = c_bar%sInitCnt(%d,0);\n",
					pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str(), pBar->m_barIdW.AsInt() - 1);
			}

			barPostInstr.Append("\t\tc_bar%sActive = false;\n", pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t}\n");
		}

		if (pBar->m_barIdW.AsInt() > 0) {
			barPostInstr.Append("\n");
			barPostInstr.Append("\tm_bar%sActive.write_mem(c_bar%sActive);\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			barPostInstr.Append("\tm_bar%sRlsHead.write_mem(c_bar%sRlsHead);\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());

			if (mod.m_modInstList.size() == 1) {
				barPostInstr.Append("\tm_bar%sEnterCnt.write_mem(c_bar%sEnterCnt);\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
				barPostInstr.Append("#\tif !defined(_HTV) || defined(HT_ASSERT)\n");
				barPostInstr.Append("\tm_bar%sReleaseCnt.write_mem(c_bar%sReleaseCnt);\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
				barPostInstr.Append("#\tendif\n");
			}
		}
		barPostInstr.Append("\n");

		if (mod.m_modInstList.size() > 1) {
			barPostInstr.Append("\tif (i_barCtlTo%s_bar%sRelease) {\n", mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str());

			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				if (pBar->m_barIdW.AsInt() > 0)
					barPostInstr.Append("\t\tm_bar%sRlsIdQue.push(i_barCtlTo%s_bar%sId);\n",
					pBar->m_name.Uc().c_str(), mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str());
				else
					barPostInstr.Append("\t\tc_bar%sRlsQueCnt += 1;\n", pBar->m_name.Uc().c_str());
			} else {
				barPostInstr.Append("\t\tc_bar%sRelease = true;\n", pBar->m_name.Uc().c_str());

				if (pBar->m_barIdW.AsInt() > 0)
					barPostInstr.Append("\t\tc_bar%sRlsId = i_barCtlTo%s_bar%sId;\n",
					pBar->m_name.Uc().c_str(), mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str());
			}

			barPostInstr.Append("\t}\n");
			barPostInstr.Append("\n");

			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				if (pBar->m_barIdW.AsInt() > 0)
					barPostInstr.Append("\tif (!r_bar%sRelease && !m_bar%sRlsIdQue.empty()) {\n",
					pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
				else
					barPostInstr.Append("\tif (!r_bar%sRelease && r_bar%sRlsQueCnt > 0) {\n",
					pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());

				barPostInstr.Append("\t\tc_bar%sRelease = true;\n", pBar->m_name.Uc().c_str());

				if (pBar->m_barIdW.AsInt() > 0) {
					barPostInstr.Append("\t\tc_bar%sRlsId = m_bar%sRlsIdQue.front();\n",
						pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
					barPostInstr.Append("\t\tm_bar%sRlsIdQue.pop();\n", pBar->m_name.Uc().c_str());
				} else
					barPostInstr.Append("\t\tc_bar%sRlsQueCnt -= 1;\n", pBar->m_name.Uc().c_str());

				barPostInstr.Append("\t}\n");
				barPostInstr.Append("\n");
			}
		}

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			barPostInstr.Append("\tif (r_bar%sRlsNext.m_link.m_bEol && !m_bar%sRlsQue.empty()) {\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\tc_bar%sRlsNext = m_bar%sRlsQue.front();\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\tm_bar%sRlsQue.pop();\n", pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t}\n");
			barPostInstr.Append("\n");
		}

		barPostInstr.Append("\tif (!r_bar%sRlsNext.m_link.m_bEol && !c_t0_rsmHtRdy) {\n", pBar->m_name.Uc().c_str());

		barPostInstr.Append("\t\tc_t0_rsmHtRdy = true;\n");
		if (mod.m_threads.m_htIdW.AsInt() > 0)
			barPostInstr.Append("\t\tc_t0_rsmHtId = r_bar%sRlsNext.m_link.m_ptr;\n", pBar->m_name.Uc().c_str());
		barPostInstr.Append("\t\tc_t0_rsmHtInstr = r_bar%sRlsNext.m_instr;\n", pBar->m_name.Uc().c_str());

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			barPostInstr.Append("\t\tm_bar%sRlsList.read_addr(r_bar%sRlsNext.m_link.m_ptr);\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			barPostInstr.Append("\t\tc_bar%sRlsNext.m_link = m_bar%sRlsList.read_mem();\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
		} else
			barPostInstr.Append("\t\tc_bar%sRlsNext.m_link.m_bEol = true;\n", pBar->m_name.Uc().c_str());

		barPostInstr.Append("\t}\n");
		barPostInstr.Append("\n");

		barPostInstr.Append("\tc_bar%sRlsNext.m_link.m_bEol = %s || c_bar%sRlsNext.m_link.m_bEol;\n",
			pBar->m_name.Uc().c_str(), reset.c_str(), pBar->m_name.Uc().c_str());

		if (mod.m_modInstList.size() > 1) {
			barOut.Append("\n");
			barOut.Append("\to_%sToBarCtl_bar%sEnter = r_t%d_bar%sEnter;\n",
				mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());
			if (pBar->m_barIdW.AsInt() > 0)
				barOut.Append("\to_%sToBarCtl_bar%sId = r_t%d_bar%sId;\n",
				mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());
			barOut.Append("\to_%sToBarCtl_bar%sReleaseCnt = r_t%d_bar%sReleaseCnt;\n",
				mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(), mod.m_execStg + 1, pBar->m_name.Uc().c_str());
		}

		barPreInstr.Append("\n");
		barPostInstr.Append("\n");
		barReg.Append("\n");
		m_barRegDecl.Append("\n");
	}

	g_appArgs.GetDsnRpt().EndLevel();

	if (mod.m_modInstList.size() == 1) return;

	// Generate barrier centralized control module
	string incName = VA("Pers%sBarCtl.h", mod.m_modName.Uc().c_str());
	string incPath = g_appArgs.GetOutputFolder() + "/" + incName;

	CHtFile incFile(incPath, "w");

	GenerateBanner(incFile, incName.c_str(), false);

	fprintf(incFile, "#pragma once\n");
	fprintf(incFile, "\n");
	fprintf(incFile, "#include \"PersCommon.h\"\n");
	fprintf(incFile, "\n");
	fprintf(incFile, "SC_MODULE(CPers%sBarCtl) {\n", mod.m_modName.Uc().c_str());
	fprintf(incFile, "\n");
	fprintf(incFile, "\tht_attrib(keep_hierarchy, CPers%sBarCtl, \"true\");\n", mod.m_modName.Uc().c_str());
	fprintf(incFile, "\n");
	fprintf(incFile, "\tsc_in<bool> i_clock1x;\n");

	if (mod.m_clkRate == eClk2x)
		fprintf(incFile, "\tsc_in<bool> i_clock2x;\n");

	fprintf(incFile, "\tsc_in<bool> i_reset;\n");
	fprintf(incFile, "\n");

	int replCnt = mod.m_modInstList.size();

	for (size_t barIdx = 0; barIdx < mod.m_barrierList.size(); barIdx += 1) {
		CBarrier * pBar = mod.m_barrierList[barIdx];

		fprintf(incFile, "\t// Centralized bar%s interface\n", pBar->m_name.Uc().c_str());
		fprintf(incFile, "\tsc_in<bool> i_%sToBarCtl_bar%sEnter[%d];\n",
			mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(), replCnt);
		if (pBar->m_barIdW.AsInt() > 0)
			fprintf(incFile, "\tsc_in<ht_uint%d > i_%sToBarCtl_bar%sId[%d];\n",
			pBar->m_barIdW.AsInt(), mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(), replCnt);
		fprintf(incFile, "\tsc_in<sc_uint<%s_HTID_W+%d> > i_%sToBarCtl_bar%sReleaseCnt[%d];\n",
			mod.m_modName.Upper().c_str(), rlsCntBits, mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(), replCnt);
		fprintf(incFile, "\tsc_out<bool> o_barCtlTo%s_bar%sRelease;\n",
			mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str());
		if (pBar->m_barIdW.AsInt() > 0)
			fprintf(incFile, "\tsc_out<ht_uint%d > o_barCtlTo%s_bar%sId;\n",
			pBar->m_barIdW.AsInt(), mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str());
		fprintf(incFile, "\n");

		fprintf(incFile, "\tstruct CBar%sInfo {\n", pBar->m_name.Uc().c_str());
		fprintf(incFile, "\t\tbool m_valid;\n");

		if (pBar->m_barIdW.AsInt() > 0)
			fprintf(incFile, "\t\tht_uint%d m_barId;\n", pBar->m_barIdW.AsInt());

		fprintf(incFile, "\t\tsc_uint<%s_HTID_W+%d> m_releaseCnt;\n", mod.m_modName.Upper().c_str(), rlsCntBits);
		fprintf(incFile, "\t};\n");
		fprintf(incFile, "\n");

		if (mod.m_threads.m_htIdW.AsInt() > 0)
			fprintf(incFile, "\tht_dist_que<CBar%sInfo, %d> m_bar%sInfoQue[%d];\n",
			pBar->m_name.Uc().c_str(), mod.m_threads.m_htIdW.AsInt(), pBar->m_name.Uc().c_str(), replCnt);
		else
			fprintf(incFile, "\tCBar%sInfo r_bar%sInfo[%d];\n",
			pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str(), replCnt);

		if (pBar->m_barIdW.AsInt() > 0) {
			fprintf(incFile, "\tht_dist_ram<bool, %d> m_bar%sActive;\n",
				pBar->m_barIdW.AsInt(), pBar->m_name.Uc().c_str());
			fprintf(incFile, "\tht_dist_ram<sc_uint<%s_HTID_W+%d>, %d> m_bar%sEnterCnt;\n",
				mod.m_modName.Upper().c_str(), rlsCntBits, pBar->m_barIdW.AsInt(), pBar->m_name.Uc().c_str());
		} else {
			fprintf(incFile, "\tbool r_bar%sActive;\n", pBar->m_name.Uc().c_str());
			fprintf(incFile, "\tsc_uint<%s_HTID_W+%d> r_bar%sEnterCnt;\n",
				mod.m_modName.Upper().c_str(), rlsCntBits, pBar->m_name.Uc().c_str());
		}

		fprintf(incFile, "\tCBar%sInfo r_bar%sInfoRd;\n", pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
		fprintf(incFile, "\tbool r_bar%sRelease;\n", pBar->m_name.Uc().c_str());

		if (pBar->m_barIdW.AsInt() > 0) {
			fprintf(incFile, "\tht_uint%d r_bar%sId;\n", pBar->m_barIdW.AsInt(), pBar->m_name.Uc().c_str());
			fprintf(incFile, "\tht_uint%d r_bar%sInitCnt;\n", pBar->m_barIdW.AsInt() + 1, pBar->m_name.Uc().c_str());
			fprintf(incFile, "\n");

			if (g_appArgs.IsVcdAllEnabled()) {
				fprintf(incFile, "#\tifndef _HTV\n");
				fprintf(incFile, "\tbool r_bar%sActive;\n", pBar->m_name.Uc().c_str());
				fprintf(incFile, "\tsc_uint<%s_HTID_W+%d> r_bar%sEnterCnt;\n",
					mod.m_modName.Upper().c_str(), rlsCntBits, pBar->m_name.Uc().c_str());
				fprintf(incFile, "#\tendif\n");
				fprintf(incFile, "\n");
			}
		}
	}

	if (mod.m_clkRate == eClk1x)
		fprintf(incFile, "\tbool r_reset1x;\n");
	else {
		fprintf(incFile, "\tbool r_reset2x;\n");
		fprintf(incFile, "\tsc_signal<bool> c_reset2x;\n");
	}

	fprintf(incFile, "\n");
	fprintf(incFile, "\tvoid Pers%sBarCtl_1x();\n", mod.m_modName.Uc().c_str());

	if (mod.m_clkRate == eClk2x)
		fprintf(incFile, "\tvoid Pers%sBarCtl_2x();\n", mod.m_modName.Uc().c_str());

	fprintf(incFile, "\n");
	fprintf(incFile, "\tSC_CTOR(CPers%sBarCtl) {\n", mod.m_modName.Uc().c_str());
	fprintf(incFile, "\t\tSC_METHOD(Pers%sBarCtl_1x);\n", mod.m_modName.Uc().c_str());
	fprintf(incFile, "\t\tsensitive << i_clock1x.pos();\n");
	fprintf(incFile, "\n");

	if (mod.m_clkRate == eClk2x) {
		fprintf(incFile, "\t\tSC_METHOD(Pers%sBarCtl_2x);\n", mod.m_modName.Uc().c_str());
		fprintf(incFile, "\t\tsensitive << i_clock2x.pos();\n");
		fprintf(incFile, "\n");
	}

	fprintf(incFile, "#\t\tifndef _HTV\n");
	if (mod.m_clkRate == eClk2x)
		fprintf(incFile, "\t\tc_reset2x = true;\n");
	fprintf(incFile, "#\t\tendif\n");

	fprintf(incFile, "\t}\n");
	fprintf(incFile, "\n");

	if (g_appArgs.IsVcdAllEnabled()) {
		fprintf(incFile, "#\tifndef _HTV\n");
		fprintf(incFile, "\tvoid start_of_simulation() {\n");

		for (size_t barIdx = 0; barIdx < mod.m_barrierList.size(); barIdx += 1) {
			CBarrier * pBar = mod.m_barrierList[barIdx];

			fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_bar%sActive, (std::string)name() + \".r_bar%sActive\");\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_bar%sEnterCnt, (std::string)name() + \".r_bar%sEnterCnt\");\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_bar%sRelease, (std::string)name() + \".r_bar%sRelease\");\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			if (pBar->m_barIdW.AsInt() > 0)
				fprintf(incFile, "\t\tsc_trace(Ht::g_vcdp, r_bar%sId, (std::string)name() + \".r_bar%sId\");\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
		}

		fprintf(incFile, "\t}\n");
		fprintf(incFile, "#\tendif\n");
	}

	fprintf(incFile, "};\n");

	incFile.FileClose();

	string cppName = VA("Pers%sBarCtl.cpp", mod.m_modName.Uc().c_str());
	string cppPath = g_appArgs.GetOutputFolder() + "/" + cppName;

	CHtFile cppFile(cppPath, "w");

	GenerateBanner(cppFile, cppName.c_str(), false);

	fprintf(cppFile, "#include \"Ht.h\"\n");
	fprintf(cppFile, "#include \"Pers%sBarCtl.h\"\n", mod.m_modName.Uc().c_str());
	fprintf(cppFile, "\n");

	if (mod.m_clkRate == eClk2x) {
		fprintf(cppFile, "void CPers%sBarCtl::Pers%sBarCtl_1x()\n",
			mod.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str());
		fprintf(cppFile, "{\n");

		fprintf(cppFile, "\tht_attrib(equivalent_register_removal, r_reset2x, \"no\");\n");
		fprintf(cppFile, "\tHtResetFlop(r_reset2x, i_reset.read());\n");
		fprintf(cppFile, "\tc_reset2x = r_reset2x;\n");
		fprintf(cppFile, "}\n");
		fprintf(cppFile, "\n");
	}

	fprintf(cppFile, "void CPers%sBarCtl::Pers%sBarCtl_%s()\n",
		mod.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), mod.m_clkRate == eClk1x ? "1x" : "2x");
	fprintf(cppFile, "{\n");

	for (size_t barIdx = 0; barIdx < mod.m_barrierList.size(); barIdx += 1) {
		CBarrier * pBar = mod.m_barrierList[barIdx];

		fprintf(cppFile, "\tbool bBar%sFoundOne = false;\n", pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\tCBar%sInfo c_bar%sInfoRd;\n",
			pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\tc_bar%sInfoRd.m_valid = false;\n", pBar->m_name.Uc().c_str());
		if (pBar->m_barIdW.AsInt() > 0)
			fprintf(cppFile, "\tc_bar%sInfoRd.m_barId = 0;\n", pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\n");

		if (mod.m_threads.m_htIdW.AsInt() == 0) {
			fprintf(cppFile, "\tCBar%sInfo c_bar%sInfo[%d];\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str(), replCnt);
			for (int replIdx = 0; replIdx < replCnt; replIdx += 1)
				fprintf(cppFile, "\tc_bar%sInfo[%d] = r_bar%sInfo[%d];\n",
				pBar->m_name.Uc().c_str(), replIdx, pBar->m_name.Uc().c_str(), replIdx);
			fprintf(cppFile, "\n");
		}

		fprintf(cppFile, "\tfor (int barIdx = 0; barIdx < %d; barIdx += 1) {\n", replCnt);
		fprintf(cppFile, "\t\tif (i_%sToBarCtl_bar%sEnter[barIdx]) {\n",
			mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\t\t\tCBar%sInfo c_bar%sInfoWr;\n",
			pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\t\t\tc_bar%sInfoWr.m_valid = true;\n", pBar->m_name.Uc().c_str());
		if (pBar->m_barIdW.AsInt() > 0)
			fprintf(cppFile, "\t\t\tc_bar%sInfoWr.m_barId = i_%sToBarCtl_bar%sId[barIdx];\n",
			pBar->m_name.Uc().c_str(), mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\t\t\tc_bar%sInfoWr.m_releaseCnt = i_%sToBarCtl_bar%sReleaseCnt[barIdx];\n",
			pBar->m_name.Uc().c_str(), mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str());

		if (mod.m_threads.m_htIdW.AsInt() > 0)
			fprintf(cppFile, "\t\t\tm_bar%sInfoQue[barIdx].push(c_bar%sInfoWr);\n",
			pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
		else
			fprintf(cppFile, "\t\t\tc_bar%sInfo[barIdx] = c_bar%sInfoWr;\n",
			pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());

		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\n");

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			fprintf(cppFile, "\t\tif (!bBar%sFoundOne && !m_bar%sInfoQue[barIdx].empty()) {\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\t\t\tbBar%sFoundOne = true;\n", pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\t\t\tc_bar%sInfoRd = m_bar%sInfoQue[barIdx].front();\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\t\t\tm_bar%sInfoQue[barIdx].pop();\n", pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\t\t}\n");

		} else {
			fprintf(cppFile, "\t\tif (!bBar%sFoundOne && c_bar%sInfo[barIdx].m_valid) {\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\t\t\tbBar%sFoundOne = true;\n", pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\t\t\tc_bar%sInfoRd = c_bar%sInfo[barIdx];\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\t\t\tc_bar%sInfo[barIdx].m_valid = false;\n", pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\t\t}\n");
		}

		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");

		if (pBar->m_barIdW.AsInt() > 0) {
			fprintf(cppFile, "\tm_bar%sActive.read_addr(r_bar%sInfoRd.m_barId);\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\tm_bar%sEnterCnt.read_addr(r_bar%sInfoRd.m_barId);\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\n");
			fprintf(cppFile, "\tm_bar%sActive.write_addr(r_bar%sInfoRd.m_barId);\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\tm_bar%sEnterCnt.write_addr(r_bar%sInfoRd.m_barId);\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\n");
			fprintf(cppFile, "\tbool c_bar%sActive = m_bar%sActive.read_mem();\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\tsc_uint<%s_HTID_W+%d> c_bar%sEnterCnt = m_bar%sEnterCnt.read_mem();\n",
				mod.m_modName.Upper().c_str(), rlsCntBits, pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
		} else {
			fprintf(cppFile, "\tbool c_bar%sActive = r_bar%sActive;\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\tsc_uint<%s_HTID_W+%d> c_bar%sEnterCnt = r_bar%sEnterCnt;\n",
				mod.m_modName.Upper().c_str(), rlsCntBits, pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
		}
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tbool c_bar%sRelease = false;\n", pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\tif (r_bar%sInfoRd.m_valid) {\n", pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\t\tif (!c_bar%sActive) {\n", pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\t\t\tc_bar%sEnterCnt = r_bar%sInfoRd.m_releaseCnt;\n",
			pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_bar%sActive = true;\n", pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (c_bar%sEnterCnt == 1) {\n", pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\t\t\tc_bar%sActive = false;\n", pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\t\t\tc_bar%sRelease = true;\n", pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_bar%sEnterCnt -= 1;\n", pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");

		if (mod.m_threads.m_htIdW.AsInt() == 0) {
			for (int replIdx = 0; replIdx < replCnt; replIdx += 1) {
				fprintf(cppFile, "\tc_bar%sInfo[%d].m_valid = !%s && c_bar%sInfo[%d].m_valid;\n",
					pBar->m_name.Uc().c_str(), replIdx, reset.c_str(), pBar->m_name.Uc().c_str(), replIdx);
			}
			fprintf(cppFile, "\n");
		}

		if (pBar->m_barIdW.AsInt() > 0) {
			fprintf(cppFile, "\tht_uint%d c_bar%sInitCnt = r_bar%sInitCnt;\n",
				pBar->m_barIdW.AsInt() + 1, pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\tif (%s)\n", reset.c_str());
			fprintf(cppFile, "\t\tc_bar%sInfoRd.m_barId = 0;\n", pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\telse if (r_bar%sInitCnt[%d] == 0) {\n",
				pBar->m_name.Uc().c_str(), pBar->m_barIdW.AsInt());
			fprintf(cppFile, "\t\tc_bar%sInitCnt += 1;\n", pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\t\tc_bar%sInfoRd.m_barId = c_bar%sInitCnt(%d,0);\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str(), pBar->m_barIdW.AsInt() - 1);
			fprintf(cppFile, "\t\tc_bar%sActive = false;\n", pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\t}\n");
			fprintf(cppFile, "\n");

			fprintf(cppFile, "\tm_bar%sActive.write_mem(c_bar%sActive);\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\tm_bar%sEnterCnt.write_mem(c_bar%sEnterCnt);\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\n");

			if (g_appArgs.IsVcdAllEnabled()) {
				fprintf(cppFile, "#\tifndef _HTV\n");
				fprintf(cppFile, "\tr_bar%sActive = c_bar%sActive;\n",
					pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
				fprintf(cppFile, "\tr_bar%sEnterCnt = c_bar%sEnterCnt;\n",
					pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
				fprintf(cppFile, "#\tendif\n");
			}
		} else {
			fprintf(cppFile, "\tr_bar%sActive = !%s && c_bar%sActive;\n",
				pBar->m_name.Uc().c_str(), reset.c_str(), pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\tr_bar%sEnterCnt = c_bar%sEnterCnt;\n",
				pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
		}

		fprintf(cppFile, "\tr_bar%sRelease = c_bar%sRelease;\n",
			pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
		if (pBar->m_barIdW.AsInt() > 0)
			fprintf(cppFile, "\tr_bar%sId = r_bar%sInfoRd.m_barId;\n",
			pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tr_bar%sInfoRd = c_bar%sInfoRd;\n",
			pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\n");

		if (pBar->m_barIdW.AsInt() > 0) {
			fprintf(cppFile, "\tr_bar%sInitCnt = %s ? (ht_uint%d)0 : c_bar%sInitCnt;\n",
				pBar->m_name.Uc().c_str(), reset.c_str(), pBar->m_barIdW.AsInt() + 1, pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\n");
		}

		for (int replIdx = 0; replIdx < replCnt; replIdx += 1) {
			if (mod.m_threads.m_htIdW.AsInt() > 0)
				fprintf(cppFile, "\tm_bar%sInfoQue[%d].clock(%s);\n", pBar->m_name.Uc().c_str(), replIdx, reset.c_str());
			else
				fprintf(cppFile, "\tr_bar%sInfo[%d] = c_bar%sInfo[%d];\n",
				pBar->m_name.Uc().c_str(), replIdx, pBar->m_name.Uc().c_str(), replIdx);
		}

		if (pBar->m_barIdW.AsInt() > 0) {
			fprintf(cppFile, "\tm_bar%sActive.clock();\n", pBar->m_name.Uc().c_str());
			fprintf(cppFile, "\tm_bar%sEnterCnt.clock();\n", pBar->m_name.Uc().c_str());
		}
		fprintf(cppFile, "\n");

		if (mod.m_clkRate == eClk1x && barIdx == mod.m_barrierList.size() - 1) {
			fprintf(cppFile, "\tht_attrib(equivalent_register_removal, r_reset1x, \"no\");\n");
			fprintf(cppFile, "\tHtResetFlop(r_reset1x, i_reset.read());\n");
			fprintf(cppFile, "\n");
		}

		fprintf(cppFile, "\to_barCtlTo%s_bar%sRelease = r_bar%sRelease;\n",
			mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
		if (pBar->m_barIdW.AsInt() > 0)
			fprintf(cppFile, "\to_barCtlTo%s_bar%sId = r_bar%sId;\n",
			mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str(), pBar->m_name.Uc().c_str());
		fprintf(cppFile, "\n");
	}

	fprintf(cppFile, "}\n");

	cppFile.FileClose();
}
