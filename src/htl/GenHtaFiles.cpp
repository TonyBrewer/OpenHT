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

void CDsnInfo::GenerateHtaFiles()
{
	string fileName = g_appArgs.GetOutputFolder() + "/Pers" + m_unitName.Uc() + "Hta" + ".h";

	CHtFile incFile(fileName, "w");

	GenPersBanner(incFile, m_unitName.Uc().c_str(), "Hta", true);

	fprintf(incFile, "SC_MODULE(CPers%sHta) {\n", m_unitName.Uc().c_str());
	fprintf(incFile, "\n");
	fprintf(incFile, "\tht_attrib(keep_hierarchy, CPers%sHta, \"true\");\n", m_unitName.Uc().c_str());

	fprintf(incFile, "\tsc_in<bool> i_clock1x;\n");

	for (size_t instIdx = 0; instIdx < m_dsnInstList.size(); instIdx += 1) {
		CInstance * pModInst = m_dsnInstList[instIdx];
		if (!pModInst->m_pMod->m_bIsUsed) continue;

		if (!pModInst->m_pMod->m_bHasThreads || pModInst->m_pMod->m_bHostIntf)
			continue;

		if (pModInst->m_replCnt <= 1) {
			fprintf(incFile, "\tsc_in<CHtAssertIntf> i_%sToHta_assert;\n",
				pModInst->m_instName.Lc().c_str());
		} else if (pModInst->m_replId == 0) {
			fprintf(incFile, "\tsc_in<CHtAssertIntf> i_%sToHta_assert[%d];\n",
				pModInst->m_instName.Lc().c_str(), pModInst->m_replCnt);
		}
	}

	fprintf(incFile, "\tsc_out<CHtAssertIntf> o_htaToHif_assert;\n");
	fprintf(incFile, "\n");

	fprintf(incFile, "\t////////////////////////////////////////////\n");
	fprintf(incFile, "\t// Register state\n");
	fprintf(incFile, "\n");

	for (size_t instIdx = 0; instIdx < m_dsnInstList.size(); instIdx += 1) {
		CInstance * pModInst = m_dsnInstList[instIdx];
		if (!pModInst->m_pMod->m_bIsUsed) continue;

		if (!pModInst->m_pMod->m_bHasThreads || pModInst->m_pMod->m_bHostIntf)
			continue;

		if (pModInst->m_replCnt <= 1) {
			fprintf(incFile, "\tCHtAssertIntf r_%sToHta_assert;\n",
				pModInst->m_instName.Lc().c_str());
		} else if (pModInst->m_replId == 0) {
			fprintf(incFile, "\tCHtAssertIntf r_%sToHta_assert[%d];\n",
				pModInst->m_instName.Lc().c_str(),
				pModInst->m_replCnt);
		}
	}

	fprintf(incFile, "\tCHtAssertIntf r_htaToHif_assert;\n");
	fprintf(incFile, "\n");

	fprintf(incFile, "\t////////////////////////////////////////////\n");
	fprintf(incFile, "\t// Methods\n");
	fprintf(incFile, "\n");

	fprintf(incFile, "\tvoid Pers%sHta1x();\n", m_unitName.Uc().c_str());
	fprintf(incFile, "\n");

	fprintf(incFile, "\tSC_CTOR(CPers%sHta) {\n", m_unitName.Uc().c_str());
	fprintf(incFile, "\t\tSC_METHOD(Pers%sHta1x);\n", m_unitName.Uc().c_str());
	fprintf(incFile, "\t\tsensitive << i_clock1x.pos();\n");
	fprintf(incFile, "\t}\n");
	fprintf(incFile, "};\n");

	incFile.FileClose();

	////////////////////////////////////////////////////////////////
	// Generate cpp file

	fileName = g_appArgs.GetOutputFolder() + "/Pers" + m_unitName.Uc() + "Hta" + ".cpp";

	CHtFile cppFile(fileName, "w");

	GenPersBanner(cppFile, m_unitName.Uc().c_str(), "Hta", false);

	fprintf(cppFile, "void\n");
	fprintf(cppFile, "CPers%sHta::Pers%sHta1x()\n", m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
	fprintf(cppFile, "{\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "\t/////////////////////////\n");
	fprintf(cppFile, "\t// Combinatorial Logic\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\tCHtAssertIntf c_htaToHif_assert;\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\tc_htaToHif_assert.m_module = 0;\n");

	int moduleIdx = 0;
	for (size_t instIdx = 0; instIdx < m_dsnInstList.size(); instIdx += 1) {
		CInstance * pModInst = m_dsnInstList[instIdx];
		if (!pModInst->m_pMod->m_bIsUsed) continue;

		if (!pModInst->m_pMod->m_bHasThreads || pModInst->m_pMod->m_bHostIntf)
			continue;

		string replIdxStr = pModInst->m_replCnt <= 1 ? "" : VA("[%d]", pModInst->m_replId);

		fprintf(cppFile, "\tif (r_%sToHta_assert%s.m_bAssert) c_htaToHif_assert.m_module |= r_%sToHta_assert%s.m_module;\n",
			pModInst->m_instName.Lc().c_str(), replIdxStr.c_str(), pModInst->m_instName.Lc().c_str(), replIdxStr.c_str());

		moduleIdx++;
	}
	fprintf(cppFile, "\n");
	int moduleCnt = moduleIdx;

	fprintf(cppFile, "\tc_htaToHif_assert.m_info = 0;\n");

	for (size_t instIdx = 0; instIdx < m_dsnInstList.size(); instIdx += 1) {
		CInstance * pModInst = m_dsnInstList[instIdx];
		if (!pModInst->m_pMod->m_bIsUsed) continue;

		if (!pModInst->m_pMod->m_bHasThreads || pModInst->m_pMod->m_bHostIntf)
			continue;

		string replIdxStr = pModInst->m_replCnt <= 1 ? "" : VA("[%d]", pModInst->m_replId);

		fprintf(cppFile, "\tc_htaToHif_assert.m_info |= r_%sToHta_assert%s.m_info;\n",
			pModInst->m_instName.Lc().c_str(), replIdxStr.c_str());
	}
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\tc_htaToHif_assert.m_lineNum = 0;\n");

	for (size_t instIdx = 0; instIdx < m_dsnInstList.size(); instIdx += 1) {
		CInstance * pModInst = m_dsnInstList[instIdx];
		if (!pModInst->m_pMod->m_bIsUsed) continue;

		if (!pModInst->m_pMod->m_bHasThreads || pModInst->m_pMod->m_bHostIntf)
			continue;

		string replIdxStr = pModInst->m_replCnt <= 1 ? "" : VA("[%d]", pModInst->m_replId);

		fprintf(cppFile, "\tc_htaToHif_assert.m_lineNum |= r_%sToHta_assert%s.m_lineNum;\n",
			pModInst->m_instName.Lc().c_str(), replIdxStr.c_str());
	}
	fprintf(cppFile, "\n");

	int modIdx = 0;
	for (size_t instIdx = 0; instIdx < m_dsnInstList.size(); instIdx += 1) {
		CInstance * pModInst = m_dsnInstList[instIdx];
		if (!pModInst->m_pMod->m_bIsUsed) continue;

		if (!pModInst->m_pMod->m_bHasThreads || pModInst->m_pMod->m_bHostIntf)
			continue;

		instIdx += 1;

		for (; instIdx < m_dsnInstList.size(); instIdx += 1) {
			CInstance * pModInst2 = m_dsnInstList[instIdx];

			if (!pModInst2->m_pMod->m_bHasThreads || pModInst2->m_pMod->m_bHostIntf)
				continue;

			string replIdxStr = pModInst->m_replCnt <= 1 ? "" : VA("[%d]", pModInst->m_replId);
			string replIdx2Str = pModInst2->m_replCnt <= 1 ? "" : VA("[%d]", pModInst2->m_replId);

			fprintf(cppFile, "\tbool c_bAssert_1_%d = r_%sToHta_assert%s.m_bAssert || r_%sToHta_assert%s.m_bAssert;\n",
				modIdx / 2, pModInst->m_instName.Lc().c_str(), replIdxStr.c_str(),
				pModInst2->m_instName.Lc().c_str(), replIdx2Str.c_str());
			fprintf(cppFile, "\tbool c_bCollision_1_%d = r_%sToHta_assert%s.m_bAssert && r_%sToHta_assert%s.m_bAssert;\n",
				modIdx / 2, pModInst->m_instName.Lc().c_str(), replIdxStr.c_str(),
				pModInst2->m_instName.Lc().c_str(), replIdx2Str.c_str());
			break;
		}
		if (instIdx == m_dsnInstList.size()) {
			string replIdxStr = pModInst->m_replCnt <= 1 ? "" : VA("[%d]", pModInst->m_replId);

			fprintf(cppFile, "\tbool c_bAssert_1_%d = r_%sToHta_assert%s.m_bAssert;\n",
				modIdx / 2, pModInst->m_instName.Lc().c_str(), replIdxStr.c_str());
			if (moduleCnt > 1)
				fprintf(cppFile, "\tbool c_bCollision_1_%d = r_%sToHta_assert%s.m_bAssert && c_bAssert_1_%d;\n",
				modIdx / 2, pModInst->m_instName.Lc().c_str(), replIdxStr.c_str(), (modIdx / 2) - 1);
		}
		modIdx += 2;
	}
	fprintf(cppFile, "\n");

	int size = (moduleCnt + 1) / 2;

	int lvl;
	for (lvl = 1; size > 1; lvl += 1) {
		for (int instIdx = 0; instIdx + 1 < size; instIdx += 2) {
			fprintf(cppFile, "\tbool c_bAssert_%d_%d = c_bAssert_%d_%d || c_bAssert_%d_%d;\n",
				lvl + 1, instIdx / 2, lvl, instIdx, lvl, instIdx + 1);
			fprintf(cppFile, "\tbool c_bCollision_%d_%d = c_bCollision_%d_%d || c_bCollision_%d_%d || (c_bAssert_%d_%d && c_bAssert_%d_%d);\n",
				lvl + 1, instIdx / 2, lvl, instIdx, lvl, instIdx + 1, lvl, instIdx, lvl, instIdx + 1);
		}

		if (size & 1) {
			fprintf(cppFile, "\tbool c_bAssert_%d_%d = c_bAssert_%d_%d;\n",
				lvl + 1, size / 2, lvl, size - 1);
			fprintf(cppFile, "\tbool c_bCollision_%d_%d = c_bCollision_%d_%d;\n",
				lvl + 1, size / 2, lvl, size - 1);
		}
		fprintf(cppFile, "\n");
		size = (size + 1) / 2;
	}

	fprintf(cppFile, "\tc_htaToHif_assert.m_bAssert = c_bAssert_%d_0;\n", lvl);
	if (moduleCnt == 1)
		fprintf(cppFile, "\tc_htaToHif_assert.m_bCollision = false;\n");
	else
		fprintf(cppFile, "\tc_htaToHif_assert.m_bCollision = c_bCollision_%d_0;\n", lvl);
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\t/////////////////////////\n");
	fprintf(cppFile, "\t// Register assignments\n");
	fprintf(cppFile, "\n");

	for (size_t instIdx = 0; instIdx < m_dsnInstList.size(); instIdx += 1) {
		CInstance * pModInst = m_dsnInstList[instIdx];
		if (!pModInst->m_pMod->m_bIsUsed) continue;

		if (!pModInst->m_pMod->m_bHasThreads || pModInst->m_pMod->m_bHostIntf)
			continue;

		string replIdxStr = pModInst->m_replCnt <= 1 ? "" : VA("[%d]", pModInst->m_replId);

		fprintf(cppFile, "\tr_%sToHta_assert%s = i_%sToHta_assert%s;\n",
			pModInst->m_instName.Lc().c_str(), replIdxStr.c_str(),
			pModInst->m_instName.Lc().c_str(), replIdxStr.c_str());
	}

	fprintf(cppFile, "\tr_htaToHif_assert = c_htaToHif_assert;\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\t/////////////////////////\n");
	fprintf(cppFile, "\t// Output assignments\n");
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\to_htaToHif_assert = r_htaToHif_assert;\n");
	fprintf(cppFile, "}\n");

	cppFile.FileClose();
}
