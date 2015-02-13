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

void CDsnInfo::GenHtMon()
{
#ifndef OLD_HT_MON
	
	string cppFileName = g_appArgs.GetOutputFolder() + "/SyscMon.cpp";

	CHtFile cppFile(cppFileName, "w");

	GenerateBanner(cppFile, cppFileName.c_str(), false);

	fprintf(cppFile, "#include \"Ht.h\"\n");
	fprintf(cppFile, "#include \"sysc/SyscMon.h\"\n");

	fprintf(cppFile, "\n");
	fprintf(cppFile, "namespace Ht {\n");
	fprintf(cppFile, "\tCSyscMon g_syscMon(\"%s\", %d);\n", g_appArgs.GetCoprocAsStr(), g_appArgs.GetAeUnitCnt());
	fprintf(cppFile, "\tEHtCoproc htCoproc = %s;\n", g_appArgs.GetCoprocAsStr());
	fprintf(cppFile, "\tint avgMemLatency[2] = { %d, %d };\n",
		g_appArgs.GetArgMemLatency(0), g_appArgs.GetArgMemLatency(1));
	fprintf(cppFile, "\tbool bMemBankModel = %s;\n", g_appArgs.IsMemTraceEnabled() ? "true" : "false");
	fprintf(cppFile, "\tbool g_bMemReqRndFull = %s;\n", g_appArgs.IsRndRetryEnabled() ? "true" : "false");
	fprintf(cppFile, "\tbool bMemTraceEnable = %s;\n", g_appArgs.IsMemTraceEnabled() ? "true" : "false");
	fprintf(cppFile, "\n");

	if (g_appArgs.IsInstrTraceEnabled())
		fprintf(cppFile, "\tFILE * g_instrTraceFp = fopen(\"InstrTrace.txt\", \"w\");\n");

	fprintf(cppFile, "\tlong long g_vcdStartCycle = %d;\n", g_appArgs.GetVcdStartCycle());
	fprintf(cppFile, "\tsc_trace_file * g_vcdp = 0;\n");
	fprintf(cppFile, "\tchar const * g_vcdn = %s;\n",
		(g_appArgs.IsVcdAllEnabled() || g_appArgs.IsVcdUserEnabled()) ? "\"wave\"" : "0");
	fprintf(cppFile, "}\n");

	cppFile.FileClose();

#else
	string incFileName = g_appArgs.GetOutputFolder() + "/HtMon.h";

	CHtFile incFile(incFileName, "w");

	GenerateBanner(incFile, incFileName.c_str(), true);

	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

	fprintf(incFile, "#include \"Pers%sCommon.h\"\n", unitNameUc.c_str());
	fprintf(incFile, "\n");

	fprintf(incFile, "enum EHtCoproc { hc1, hc1ex, hc2, hc2ex, wx690, wx2k };\n\n");
	fprintf(incFile, "\n");

	fprintf(incFile, "struct CHtMon {\n");
	fprintf(incFile, "\tCHtMon() { Init(); m_bIsInit = false; m_bReportGenerated = false; }\n");
	fprintf(incFile, "\t~CHtMon() { Report(); }\n");
	fprintf(incFile, "\tvoid Init();\n");
	fprintf(incFile, "\tvoid Report();\n");
	fprintf(incFile, "\n");

	fprintf(incFile, "\tbool m_bIsInit;\n");
	fprintf(incFile, "\tbool m_bReportGenerated;\n");
	fprintf(incFile, "\tlong long m_minMemClks[2];\n");
	fprintf(incFile, "\tlong long m_maxMemClks[2];\n");
	fprintf(incFile, "\tlong long m_totalMemClks[2];\n");
	fprintf(incFile, "\tlong long m_totalMemOps[2];\n");
	fprintf(incFile, "\tlong long m_totalMemReads[2];\n");
	fprintf(incFile, "\tlong long m_totalMemWrites[2];\n");
	fprintf(incFile, "\tlong long m_totalMemRead64s[2];\n");
	fprintf(incFile, "\tlong long m_totalMemWrite64s[2];\n");
	fprintf(incFile, "\tlong long m_totalMemReadBytes[2];\n");
	fprintf(incFile, "\tlong long m_totalMemWriteBytes[2];\n");

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;

		char indexStr[8] = { 0 };
		if (mod.m_modInstList.size() > 1)
			sprintf(indexStr, "[%d]", (int)mod.m_modInstList.size());

		fprintf(incFile, "\tlong long m_%sReadCnt%s;\n", mod.m_modName.Lc().c_str(), indexStr);
		fprintf(incFile, "\tlong long m_%sWriteCnt%s;\n", mod.m_modName.Lc().c_str(), indexStr);
		fprintf(incFile, "\tlong long m_%sRead64Cnt%s;\n", mod.m_modName.Lc().c_str(), indexStr);
		fprintf(incFile, "\tlong long m_%sWrite64Cnt%s;\n", mod.m_modName.Lc().c_str(), indexStr);
		fprintf(incFile, "\tlong long m_%sReadByteCnt%s;\n", mod.m_modName.Lc().c_str(), indexStr);
		fprintf(incFile, "\tlong long m_%sWriteByteCnt%s;\n", mod.m_modName.Lc().c_str(), indexStr);

		if (mod.m_modName.Lc() == "hif") continue;

		fprintf(incFile, "\tlong long m_%sValidCnt%s;\n", mod.m_modName.Lc().c_str(), indexStr);
		fprintf(incFile, "\tlong long m_%sRetryCnt%s;\n", mod.m_modName.Lc().c_str(), indexStr);
		if (mod.m_bHasThreads) {
			fprintf(incFile, "\tlong long m_%sInstrValidCnt%s[1 << %s_INSTR_W];\n",
				mod.m_modName.Lc().c_str(), indexStr, mod.m_modName.Upper().c_str());
			fprintf(incFile, "\tlong long m_%sInstrRetryCnt%s[1 << %s_INSTR_W];\n",
				mod.m_modName.Lc().c_str(), indexStr, mod.m_modName.Upper().c_str());
		}

		bool bHtIdPoolNeeded = mod.m_threads.m_htIdW.AsInt() > 0;
		if (bHtIdPoolNeeded) {
			fprintf(incFile, "\tlong long m_%sMaxHtCnt%s;\n", mod.m_modName.Lc().c_str(), indexStr);
			fprintf(incFile, "\tlong long m_%sActiveHtCnt%s;\n", mod.m_modName.Lc().c_str(), indexStr);
			fprintf(incFile, "\tlong long m_%sPipeHtCnt%s;\n", mod.m_modName.Lc().c_str(), indexStr);
			fprintf(incFile, "\tlong long m_%sRunningHtCnt%s;\n", mod.m_modName.Lc().c_str(), indexStr);
		}
		fprintf(incFile, "\tlong long m_%sActiveClks%s;\n", mod.m_modName.Lc().c_str(), indexStr);
	}

	fprintf(incFile, "};\n");
	fprintf(incFile, "\n");
	fprintf(incFile, "extern CHtMon htMon;\n");
	fprintf(incFile, "extern sc_trace_file *g_vcdp;\n");
	fprintf(incFile, "extern const char *g_vcdn;\n");

	incFile.FileClose();

	///////////////////////////////////////////////////////////////

	string cppFileName = g_appArgs.GetOutputFolder() + "/HtMon.cpp";

	CHtFile cppFile(cppFileName, "w");

	GenerateBanner(cppFile, cppFileName.c_str(), false);

	fprintf(cppFile, "#include \"Ht.h\"\n");
	fprintf(cppFile, "#define TOPOLOGY_HEADER\n");
	fprintf(cppFile, "#include \"HtMon.h\"\n");

	for (size_t modIdx = 1; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;

		int prevInstId = -1;
		for (size_t modInstIdx = 0; modInstIdx < mod.m_modInstList.size(); modInstIdx += 1) {
			CModInst &modInst = mod.m_modInstList[modInstIdx];

			if (modInst.m_instParams.m_instId == prevInstId)
				continue;

			prevInstId = modInst.m_instParams.m_instId;

			string instIdStr = GenIndexStr(mod.m_instIdCnt > 1, "%d", modInst.m_instParams.m_instId);

			fprintf(cppFile, "#include \"Pers%s%s%s.h\"\n",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str());
		}
	}
	fprintf(cppFile, "\n");

	fprintf(cppFile, "#include <vector>\n");
	fprintf(cppFile, "#include <algorithm>\n");
	fprintf(cppFile, "using namespace std;\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "struct activeCnt {\n");
	fprintf(cppFile, "\tint idx;\n");
	fprintf(cppFile, "\tlong long cnt;\n");
	fprintf(cppFile, "};\n");
	fprintf(cppFile, "bool sort_cnts(const activeCnt &a, const activeCnt &b)\n");
	fprintf(cppFile, "{\n\treturn a.cnt > b.cnt;\n}\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "CHtMon htMon;\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "void CHtMon::Init()\n");
	fprintf(cppFile, "{\n");

	fprintf(cppFile, "\tfor (int i = 0; i < 2; i += 1) {\n");
	fprintf(cppFile, "\t\tm_minMemClks[i] = 0x7fffffffffffffffLL;\n");
	fprintf(cppFile, "\t\tm_maxMemClks[i] = 0;\n");
	fprintf(cppFile, "\t\tm_totalMemClks[i] = 0;\n");
	fprintf(cppFile, "\t\tm_totalMemOps[i] = 0;\n");
	fprintf(cppFile, "\t\tm_totalMemReads[i] = 0;\n");
	fprintf(cppFile, "\t\tm_totalMemWrites[i] = 0;\n");
	fprintf(cppFile, "\t\tm_totalMemRead64s[i] = 0;\n");
	fprintf(cppFile, "\t\tm_totalMemWrite64s[i] = 0;\n");
	fprintf(cppFile, "\t\tm_totalMemReadBytes[i] = 0;\n");
	fprintf(cppFile, "\t\tm_totalMemWriteBytes[i] = 0;\n");
	fprintf(cppFile, "\t}\n");

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;

		for (int replIdx = 0; replIdx < (int)mod.m_modInstList.size(); replIdx += 1) {
			char indexStr[8] = { 0 };
			if (mod.m_modInstList.size() > 1)
				sprintf(indexStr, "[%d]", replIdx);

			fprintf(cppFile, "\tm_%sReadCnt%s = 0;\n", mod.m_modName.Lc().c_str(), indexStr);
			fprintf(cppFile, "\tm_%sWriteCnt%s = 0;\n", mod.m_modName.Lc().c_str(), indexStr);
			fprintf(cppFile, "\tm_%sRead64Cnt%s = 0;\n", mod.m_modName.Lc().c_str(), indexStr);
			fprintf(cppFile, "\tm_%sWrite64Cnt%s = 0;\n", mod.m_modName.Lc().c_str(), indexStr);
			fprintf(cppFile, "\tm_%sReadByteCnt%s = 0;\n", mod.m_modName.Lc().c_str(), indexStr);
			fprintf(cppFile, "\tm_%sWriteByteCnt%s = 0;\n", mod.m_modName.Lc().c_str(), indexStr);

			if (mod.m_modName.Lc() == "hif") continue;

			fprintf(cppFile, "\tm_%sValidCnt%s = 0;\n", mod.m_modName.Lc().c_str(), indexStr);
			fprintf(cppFile, "\tm_%sRetryCnt%s = 0;\n", mod.m_modName.Lc().c_str(), indexStr);
			if (mod.m_bHasThreads) {
				string name = mod.m_modName.Upper() + "_INSTR_W";
				bool bIsSigned = m_defineTable.FindStringIsSigned(name);

				fprintf(cppFile, "\tfor (%s i = 0; i < (1 << %s); i += 1) {\n", bIsSigned ? "int" : "unsigned", name.c_str());
				fprintf(cppFile, "\t\tm_%sInstrValidCnt%s[i] = 0;\n", mod.m_modName.Lc().c_str(), indexStr);
				fprintf(cppFile, "\t\tm_%sInstrRetryCnt%s[i] = 0;\n", mod.m_modName.Lc().c_str(), indexStr);
				fprintf(cppFile, "\t}\n");
			}

			bool bHtIdPoolNeeded = mod.m_threads.m_htIdW.AsInt() > 0;
			if (bHtIdPoolNeeded) {
				fprintf(cppFile, "\tm_%sMaxHtCnt%s = 0;\n", mod.m_modName.Lc().c_str(), indexStr);
				fprintf(cppFile, "\tm_%sActiveHtCnt%s = 0;\n", mod.m_modName.Lc().c_str(), indexStr);
				fprintf(cppFile, "\tm_%sPipeHtCnt%s = 0;\n", mod.m_modName.Lc().c_str(), indexStr);
				fprintf(cppFile, "\tm_%sRunningHtCnt%s = 0;\n", mod.m_modName.Lc().c_str(), indexStr);
			}
			fprintf(cppFile, "\tm_%sActiveClks%s = 0;\n", mod.m_modName.Lc().c_str(), indexStr);
		}
	}

	fprintf(cppFile, "}\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "#ifndef max\n");
	fprintf(cppFile, "#define max(a,b) ((a)>(b)?(a):(b))\n");
	fprintf(cppFile, "#endif\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "#ifndef min\n");
	fprintf(cppFile, "#define min(a,b) ((a)<(b)?(a):(b))\n");
	fprintf(cppFile, "#endif\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "void CHtMon::Report()\n");
	fprintf(cppFile, "{\n");
	fprintf(cppFile, "\tif (m_bReportGenerated)\n");
	fprintf(cppFile, "\t\treturn;\n");
	fprintf(cppFile, "\tm_bReportGenerated = true;\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "\tFILE * monFp;\n");
	fprintf(cppFile, "\tif (!(monFp = fopen(\"HtMonRpt.txt\", \"w\"))) {\n");
	fprintf(cppFile, "\t\tfprintf(stderr, \"Could not open HtMonRpt.txt\\n\");\n");
	fprintf(cppFile, "\t\texit(1);\n");
	fprintf(cppFile, "\t}\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "\tlong long sim_cyc = (long long)sc_time_stamp().value()/10000;\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "\tfprintf(monFp, \"Simulated Cycles\t: %%8lld\\n\", sim_cyc);\n");
	fprintf(cppFile, "\tfprintf(monFp, \"Platform\t\t: %%8s\\n\", \"%s\");\n", g_appArgs.GetCoprocName());
	fprintf(cppFile, "\tfprintf(monFp, \"Unit Count\t\t: %%8d\\n\", %d);\n", g_appArgs.GetUnitCnt());
	fprintf(cppFile, "\n");

	// Sort active counts
	fprintf(cppFile, "\n");
	fprintf(cppFile, "\tactiveCnt cnt;\n");
	fprintf(cppFile, "\tvector<activeCnt> cnts;\n");
	int idx = 0;
	for (size_t modIdx = 1; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;
		for (int replIdx = 0; replIdx < (int)mod.m_modInstList.size(); replIdx += 1) {
			char indexStr[8] = { 0 };
			if (mod.m_modInstList.size() > 1)
				sprintf(indexStr, "[%d]", replIdx);

			fprintf(cppFile, "\tcnt.idx = %d; cnt.cnt = m_%sActiveClks%s;\n",
				idx++, mod.m_modName.Lc().c_str(), indexStr);
			fprintf(cppFile, "\tcnts.push_back(cnt);\n");
		}
	}
	fprintf(cppFile, "\tstd::sort(cnts.begin(), cnts.end(), sort_cnts);\n");
	fprintf(cppFile, "\n");

	bool is_wx = g_appArgs.GetCoproc() == wx690 || g_appArgs.GetCoproc() == wx2k;

	fprintf(cppFile, "\tfprintf(monFp, \"\\n#\\n\");\n");
	fprintf(cppFile, "\tfprintf(monFp, \"# Memory Summary\\n\");\n");
	fprintf(cppFile, "\tfprintf(monFp, \"#\\n\");\n");
	fprintf(cppFile, "\tfprintf(monFp, \"Host Latency (cyc)\t: %%8lld / %%9lld / %%8lld  (Min / Avg / Max)\\n\",\n");
	fprintf(cppFile, "\t\tm_minMemClks[1], m_totalMemClks[1] / max(1, m_totalMemOps[1]), m_maxMemClks[1]);\n");
	fprintf(cppFile, "\tfprintf(monFp, \"CP Latency (cyc)\t: %%8lld / %%9lld / %%8lld  (Min / Avg / Max)\\n\",\n");
	fprintf(cppFile, "\t\tmin(m_minMemClks[0], m_maxMemClks[0]), m_totalMemClks[0] / max(1, m_totalMemOps[0]), m_maxMemClks[0]);\n");
	fprintf(cppFile, "\tfprintf(monFp, \"Host Operations\t\t: %%8lld / %%9lld\\t\\t   (Read / Write)\\n\",\n");
	fprintf(cppFile, "\t\tm_totalMemReads[1] + m_totalMemRead64s[1], m_totalMemWrites[1] + m_totalMemWrite64s[1]);\n");
	fprintf(cppFile, "\tfprintf(monFp, \"CP Operations\t\t: %%8lld / %%9lld\\t\\t   (Read / Write)\\n\",\n");
	fprintf(cppFile, "\t\tm_totalMemReads[0] + m_totalMemRead64s[0], m_totalMemWrites[0] + m_totalMemWrite64s[0]);\n");
	fprintf(cppFile, "\tfprintf(monFp, \"Host Efficiency\t\t: %%7.2f%%%% / %%8.2f%%%%\\t\\t   (Read / Write)\\n\",\n");
	fprintf(cppFile, "\t\t100.0 * m_totalMemReadBytes[1] / (64 * m_totalMemReads[1] + 64 * m_totalMemRead64s[1]),\n");
	fprintf(cppFile, "\t\t100.0 * m_totalMemWriteBytes[1] / (64 * m_totalMemWrites[1] + 64 * m_totalMemWrite64s[1]));\n");
	fprintf(cppFile, "\tfprintf(monFp, \"CP Efficiency\t\t: %%7.2f%%%% / %%8.2f%%%%\\t\\t   (Read / Write)\\n\",\n");
	fprintf(cppFile, "\t\t100.0 * m_totalMemReadBytes[0] / max(1, (%d * m_totalMemReads[0] + 64 * m_totalMemRead64s[0])),\n",
		is_wx ? 64 : 8);
	fprintf(cppFile, "\t\t100.0 * m_totalMemWriteBytes[0] / max(1, (%d * m_totalMemWrites[0] + 64 * m_totalMemWrite64s[0])));\n",
		is_wx ? 64 : 8);
	fprintf(cppFile, "\tlong long rq_cyc, rs_cyc;\n");
	fprintf(cppFile, "\trq_cyc  = m_totalMemReads[1] + m_totalMemRead64s[1];\n");
	fprintf(cppFile, "\trq_cyc += m_totalMemWrites[1] + m_totalMemWrite64s[1] * 8;\n");
	fprintf(cppFile, "\trs_cyc  = m_totalMemReads[1] + m_totalMemRead64s[1] * 8;\n");
	fprintf(cppFile, "\trs_cyc += m_totalMemWrites[1] + m_totalMemWrite64s[1];\n");
	fprintf(cppFile, "\tfprintf(monFp, \"Host Utiliztion\t\t: %%7.2f%%%% / %%8.2f%%%%\\t\\t   (Req / Resp)\\n\",\n");
	fprintf(cppFile, "\t\t100.0 * rq_cyc / cnts[0].cnt / %d,\n", g_appArgs.GetUnitCnt());
	fprintf(cppFile, "\t\t100.0 * rs_cyc / cnts[0].cnt / %d);\n", g_appArgs.GetUnitCnt());
	fprintf(cppFile, "\trq_cyc  = m_totalMemReads[0] + m_totalMemRead64s[0];\n");
	fprintf(cppFile, "\trq_cyc += m_totalMemWrites[0] + m_totalMemWrite64s[0] * 8;\n");
	fprintf(cppFile, "\trs_cyc  = m_totalMemReads[0] + m_totalMemRead64s[0] * 8;\n");
	fprintf(cppFile, "\trs_cyc += m_totalMemWrites[0] + m_totalMemWrite64s[0];\n");
	fprintf(cppFile, "\tfprintf(monFp, \"CP Utiliztion\t\t: %%7.2f%%%% / %%8.2f%%%%\\t\\t   (Req / Resp)\\n\",\n");
	fprintf(cppFile, "\t\t100.0 * rq_cyc / cnts[0].cnt / %d,\n", g_appArgs.GetUnitCnt());
	fprintf(cppFile, "\t\t100.0 * rs_cyc / cnts[0].cnt / %d);\n", g_appArgs.GetUnitCnt());
	fprintf(cppFile, "\n");

	fprintf(cppFile, "\tfprintf(monFp, \"\\n#\\n\");\n");
	fprintf(cppFile, "\tfprintf(monFp, \"%%-35s %%10s %%10s %%10s %%10s\\n\",\n");
	fprintf(cppFile, "\t\t\"# Memory Operations\", \"Read   \", \"ReadMw  \", \"Write  \", \"WriteMw \");\n");
	fprintf(cppFile, "\tfprintf(monFp, \"%%-35s %%10s %%10s %%10s %%10s\\n\",\n");
	fprintf(cppFile, "\t\t\"#\", \"----------\", \"----------\", \"----------\", \"----------\");\n");
	fprintf(cppFile, "\tfprintf(monFp, \"%%-35s %%10lld %%10lld %%10lld %%10lld\\n\",\n");
	fprintf(cppFile, "\t\t\"<total>\", m_totalMemReads[0] + m_totalMemReads[1],\n");
	fprintf(cppFile, "\t\tm_totalMemRead64s[0] + m_totalMemRead64s[1],\n");
	fprintf(cppFile, "\t\tm_totalMemWrites[0] + m_totalMemWrites[1],\n");
	fprintf(cppFile, "\t\tm_totalMemWrite64s[0] + m_totalMemWrite64s[1]);\n");

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;

		for (int replIdx = 0; replIdx < (int)mod.m_modInstList.size(); replIdx += 1) {
			char indexStr[8] = { 0 };
			if (mod.m_modInstList.size() > 1)
				sprintf(indexStr, "[%d]", replIdx);

			if (!mod.m_mif.m_bMif) continue;

			string path("");
			if (mod.m_modInstList.size() && mod.m_modInstList[0].m_modPaths.size()) {
				size_t pos;
				path = mod.m_modInstList[0].m_modPaths[0];
				pos = path.find_last_of("/");
				if (pos < path.size()) path.erase(pos, path.size() - pos);
				pos = path.find_first_of(":/");
				if (pos < path.size()) path.erase(0, pos+2);
				if (path.size()) path.append("/");
			}

			fprintf(cppFile, "\tfprintf(monFp, \"%%-35s %%10lld %%10lld %%10lld %%10lld\\n\",\n");
			fprintf(cppFile, "\t\t\"%s%s%s\", m_%sReadCnt%s, m_%sRead64Cnt%s, m_%sWriteCnt%s, m_%sWrite64Cnt%s);\n",
				path.c_str(), mod.m_modName.Upper().c_str(), indexStr,
				mod.m_modName.Lc().c_str(), indexStr,
				mod.m_modName.Lc().c_str(), indexStr,
				mod.m_modName.Lc().c_str(), indexStr,
				mod.m_modName.Lc().c_str(), indexStr);
		}
	}

	fprintf(cppFile, "\tfprintf(monFp, \"\\n#\\n\");\n");
	fprintf(cppFile, "\tfprintf(monFp, \"%%-35s %%10s %%10s %%10s %%10s\\n\",\n");
	fprintf(cppFile, "\t\t\"# Thread Utilization\", \"Avg. Run \", \"Avg. Alloc\", \"Max. Alloc\", \"Available\");\n");
	fprintf(cppFile, "\tfprintf(monFp, \"%%-35s %%10s %%10s %%10s %%10s\\n\",\n");
	fprintf(cppFile, "\t\t\"#\", \"----------\", \"----------\", \"----------\", \"----------\");\n");

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;

		bool bHtIdPoolNeeded = mod.m_threads.m_htIdW.AsInt() > 0;
		if (!bHtIdPoolNeeded) continue;

		string path("");
		if (mod.m_modInstList.size() && mod.m_modInstList[0].m_modPaths.size()) {
			size_t pos;
			path = mod.m_modInstList[0].m_modPaths[0];
			pos = path.find_last_of("/");
			if (pos < path.size()) path.erase(pos, path.size() - pos);
			pos = path.find_first_of(":/");
			if (pos < path.size()) path.erase(0, pos+2);
			if (path.size()) path.append("/");
		}

		for (int replIdx = 0; replIdx < (int)mod.m_modInstList.size(); replIdx += 1) {
			char indexStr[8] = { 0 };
			if (mod.m_modInstList.size() > 1)
				sprintf(indexStr, "[%d]", replIdx);

			fprintf(cppFile, "\tfprintf(monFp, \"%%-35s %%10lld %%10lld %%10lld %%10lld\\n\",\n");
			fprintf(cppFile, "\t\t\"%s%s%s\",\n", path.c_str(), mod.m_modName.Upper().c_str(), indexStr);
			fprintf(cppFile, "\t\tm_%sRunningHtCnt%s / max(1, m_%sActiveClks%s),\n",
				mod.m_modName.Lc().c_str(), indexStr, mod.m_modName.Lc().c_str(), indexStr);
			fprintf(cppFile, "\t\tm_%sActiveHtCnt%s / max(1, m_%sActiveClks%s),\n",
				mod.m_modName.Lc().c_str(), indexStr, mod.m_modName.Lc().c_str(), indexStr);
			fprintf(cppFile, "\t\tm_%sMaxHtCnt%s, %dll);\n",
				mod.m_modName.Lc().c_str(), indexStr, 1 << mod.m_threads.m_htIdW.AsInt());
		}
	}

	fprintf(cppFile, "\tfprintf(monFp, \"\\n#\\n\");\n");
	fprintf(cppFile, "\tfprintf(monFp, \"%%-35s %%10s %%10s %%10s %%10s\\n\",\n");
	fprintf(cppFile, "\t\t\"# Module Utilization\", \"Valid  \", \"Retry  \", \"Active Cyc\", \"Act. Util\");\n");
	fprintf(cppFile, "\tfprintf(monFp, \"%%-35s %%10s %%10s %%10s %%10s\\n\",\n");
	fprintf(cppFile, "\t\t\"#\", \"----------\", \"----------\", \"----------\", \"----------\");\n");

	fprintf(cppFile, "\n");
	fprintf(cppFile, "\tfor (int idx = 0; idx < %d; idx++) {\n", idx);
	fprintf(cppFile, "\t\tswitch(cnts[idx].idx) {\n");
	idx = 0;
	for (size_t modIdx = 1; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;

		string path("");
		if (mod.m_modInstList.size() && mod.m_modInstList[0].m_modPaths.size()) {
			size_t pos;
			path = mod.m_modInstList[0].m_modPaths[0];
			pos = path.find_last_of("/");
			if (pos < path.size()) path.erase(pos, path.size() - pos);
			pos = path.find_first_of(":/");
			if (pos < path.size()) path.erase(0, pos+2);
			if (path.size()) path.append("/");
		}

		for (int replIdx = 0; replIdx < (int)mod.m_modInstList.size(); replIdx += 1) {
			char indexStr[8] = { 0 };
			if (mod.m_modInstList.size() > 1)
				sprintf(indexStr, "[%d]", replIdx);

			string modName = path + mod.m_modName.Upper() + indexStr;

			fprintf(cppFile, "\t\tcase %d:\n", idx++);
			fprintf(cppFile, "\t\t\tfprintf(monFp, \"%%-35s %%10lld %%10lld %%10lld %%9.2f%%%%\\n\",\n");
			fprintf(cppFile, "\t\t\t\t\"%s\", m_%sValidCnt%s, m_%sRetryCnt%s, m_%sActiveClks%s,\n",
				modName.c_str(), mod.m_modName.Lc().c_str(), indexStr,
				mod.m_modName.Lc().c_str(), indexStr,
				mod.m_modName.Lc().c_str(), indexStr);
			fprintf(cppFile, "\t\t\t\t100.0 * m_%sActiveClks%s / sim_cyc);\n",
				mod.m_modName.Lc().c_str(), indexStr);

			if (mod.m_bHasThreads) {
				string name = mod.m_modName.Upper() + "_INSTR_W";
				bool bIsSigned = m_defineTable.FindStringIsSigned(name);

				fprintf(cppFile, "\t\t\tfor (%s i = 0; i < (1 << %s); i += 1) {\n", bIsSigned ? "int" : "unsigned", name.c_str());
				fprintf(cppFile, "\t\t\t\tif (m_%sInstrValidCnt%s[i] > 0 || m_%sInstrRetryCnt%s[i] > 0)\n", mod.m_modName.Lc().c_str(), indexStr, mod.m_modName.Lc().c_str(), indexStr);
				fprintf(cppFile, "\t\t\t\t\tfprintf(monFp, \"  %%-33s %%10lld %%10lld\\n\",\n");
				fprintf(cppFile, "\t\t\t\t\t\tCPers%s%s::m_pInstrNames[i], m_%sInstrValidCnt%s[i], m_%sInstrRetryCnt%s[i]);\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(),
					mod.m_modName.Lc().c_str(), indexStr, mod.m_modName.Lc().c_str(), indexStr);
			}
			fprintf(cppFile, "\t\t\t}\n");
			fprintf(cppFile, "\t\tbreak;\n");
		}
	}
	fprintf(cppFile, "\t\tdefault: assert(0);\n");
	fprintf(cppFile, "\t\tbreak;\n");
	fprintf(cppFile, "\t\t}\n");
	fprintf(cppFile, "\t}\n");

	fprintf(cppFile, "\n");
	fprintf(cppFile, "\tfclose(monFp);\n");
	fprintf(cppFile, "}\n");

	fprintf(cppFile, "\n");

	if (g_appArgs.IsInstrTraceEnabled()) {
		fprintf(cppFile, "\n");
		fprintf(cppFile, "FILE * instrTraceFp = fopen(\"InstrTrace.txt\", \"w\");\n");
	}

	fprintf(cppFile, "long long g_vcdStartCycle = %d;\n", g_appArgs.GetVcdStartCycle());
	fprintf(cppFile, "sc_trace_file * g_vcdp = 0;\n");
	fprintf(cppFile, "char const * g_vcdn = %s;\n",
		(g_appArgs.IsVcdAllEnabled() || g_appArgs.IsVcdUserEnabled()) ? "\"wave\"" : "0");

	fprintf(cppFile, "\n");
	fprintf(cppFile, "namespace Ht {\n");
	fprintf(cppFile, "\tEHtCoproc htCoproc = %s;\n", g_appArgs.GetCoprocAsStr());
	fprintf(cppFile, "\tint avgMemLatency[2] = { %d, %d };\n",
		g_appArgs.GetArgMemLatency(0), g_appArgs.GetArgMemLatency(1));
	fprintf(cppFile, "\tbool bMemBankModel = %s;\n", g_appArgs.IsMemTraceEnabled() ? "true" : "false");
	fprintf(cppFile, "\tbool g_bMemReqRndFull = %s;\n", g_appArgs.IsRndRetryEnabled() ? "true" : "false");
	fprintf(cppFile, "\tbool bMemTraceEnable = %s;\n", g_appArgs.IsMemTraceEnabled() ? "true" : "false");
	fprintf(cppFile, "}\n");

	cppFile.FileClose();
#endif
}
