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

void CDsnInfo::GenerateClockStubFiles()
{

	/*bool isSyscSim = false;
	for (int i = 0; i < g_appArgs.GetPreDefinedNameCnt(); i++) {
		if (g_appArgs.GetPreDefinedName(i) == "HT_SYSC") {
			isSyscSim = true;
			break;
		}
	}

	// Not needed in sysc sim
	if (isSyscSim)
		return;
	*/

	// Stub Files
	{
		string fileName = g_appArgs.GetOutputFolder() + "/SyscClockHxStub.h";
	
		CHtFile hFile(fileName, "w");
	
		GenerateBanner(hFile, fileName.c_str(), false);
	
		fprintf(hFile, "#pragma once\n");
		fprintf(hFile, "\n");
		fprintf(hFile, "#include \"Ht.h\"\n");
		fprintf(hFile, "\n");
	
		fprintf(hFile, "SC_MODULE(CSyscClockHxStub) {\n");
		fprintf(hFile, "\n");
	
		fprintf(hFile, "\tsc_in<bool> ht_noload i_clockhx;\n");
		fprintf(hFile, "\n");
	
		fprintf(hFile, "\tvoid SyscClockHxStub();\n");
		fprintf(hFile, "\n");
	
		fprintf(hFile, "\tSC_CTOR(CSyscClockHxStub) {\n");
		fprintf(hFile, "\t\tSC_METHOD(SyscClockHxStub);\n");
		fprintf(hFile, "\t}\n");
		fprintf(hFile, "\n");
	
		fprintf(hFile, "};\n");
	
		hFile.FileClose();
	
	
		fileName = g_appArgs.GetOutputFolder() + "/SyscClockHxStub.cpp";
	
		CHtFile cppFile(fileName, "w");
	
		GenerateBanner(cppFile, fileName.c_str(), false);
	
		fprintf(cppFile, "#include \"Ht.h\"\n");
		fprintf(cppFile, "#include \"SyscClockHxStub.h\"\n");
		fprintf(cppFile, "\n");
	
		fprintf(cppFile, "void CSyscClockHxStub::SyscClockHxStub()\n");
		fprintf(cppFile, "{\n");	
		fprintf(cppFile, "}\n");
	
		cppFile.FileClose();
	}
}
