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
}
