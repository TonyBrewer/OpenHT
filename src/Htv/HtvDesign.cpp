/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */

#include "Htv.h"
#include "HtvDesign.h"

CHtvDesign::CHtvDesign()
{
	g_pHtfeDesign = this;
	m_bGenedMinFixtureH = false;

	m_prmFp = 0;

	m_genCppStartOffset = 0;

	m_alwaysBlockIdx = 0;
	m_bClockedAlwaysAt = false;
	m_bInAlwaysAtBlock = false;
	m_bIsHtDistRamsPresent = false;
	m_bIs1CkHtBlockRamsPresent = false;
	m_bIs2CkHtBlockRamsPresent = false;
	m_bIs1CkDoRegHtBlockRamsPresent = false;
	m_bIs2CkDoRegHtBlockRamsPresent = false;
	m_bIs1CkHtMrdBlockRamsPresent = false;
	m_bIs1CkHtMwrBlockRamsPresent = false;
	m_bIs2CkHtMrdBlockRamsPresent = false;
	m_bIs2CkHtMwrBlockRamsPresent = false;
	m_bIs1CkDoRegHtMrdBlockRamsPresent = false;
	m_bIs1CkDoRegHtMwrBlockRamsPresent = false;
	m_bIs2CkDoRegHtMrdBlockRamsPresent = false;
	m_bIs2CkDoRegHtMwrBlockRamsPresent = false;
	m_bIsHtQueueRamsPresent = false;

}

void CHtvDesign::OpenFiles()
{
	m_pathName = g_htvArgs.GetInputPathName();
	int slashPos = m_pathName.find_last_of("\\/");
	if (slashPos > 0)
		m_pathName.erase(slashPos+1);
	else
		m_pathName = "";

	if (g_htvArgs.IsGenFixture()) {
		// Create directory for backed up files
		//char nameBuf[64];
		//time_t seconds;
		//time(&seconds);
		//sprintf(nameBuf, "HtvBkup", (int)seconds);
		m_backupPath = m_pathName + "HtvBkup";

#ifdef _WIN32
		mkdir(m_backupPath.c_str());
#else
		mkdir(m_backupPath.c_str(), 0x755);
#endif
		m_backupPath += "/";
	}		

	if (!OpenInputFile(g_htvArgs.GetInputPathName())) {
		if (g_htvArgs.IsGenFixture() && g_htvArgs.IsScCode())
			GenScCodeFiles(g_htvArgs.GetInputPathName());
		else {
			printf("  Could not open input file '%s', exiting\n", g_htvArgs.GetInputPathName().c_str());
			ErrorExit();
		}
	}

	if (g_htvArgs.IsScMain()) {
		if (!m_cppFile.Open(g_htvArgs.GetCppFileName().c_str(), "wt")) {
			printf("Could not open cpp file '%s'\n", g_htvArgs.GetCppFileName().c_str());
			ErrorExit();
		}
	} else {
		//if (!m_vFile.Open(g_htvArgs.GetVFileName().c_str(), "wt")) {
		//	printf("Could not open .v file '%s'\n", g_htvArgs.GetVFileName().c_str());
		//	exit(1);
		//}

		// Keep Verilog file name consistent with top-level module name.
		// This doesn't work, GetVFileName() returns full path to file...
		string vFileName;
		//if (!g_htvArgs.GetDesignPrefixString().empty())
		//	pVFileName = GenVerilogName(g_htvArgs.GetVFileName().c_str());
		//else
			vFileName = g_htvArgs.GetVFileName().c_str();
		if (!m_vFile.Open(vFileName, "wt")) {
			printf("Could not open .v file '%s'\n", vFileName.c_str());
			ErrorExit();
		}

		GenVFileHeader();
	}

	if (g_htvArgs.IsScHier() && !m_incFile.Open(g_htvArgs.GetIncFileName().c_str(), "wt")) {
		printf("Could not open .h file '%s'\n", g_htvArgs.GetIncFileName().c_str());
		ErrorExit();
	}

	if (g_htvArgs.IsGenFixture() && g_htvArgs.IsScHier()) {
		// Create a minimal Fixture.h file
		string baseName = g_htvArgs.GetInputPathName();
		int slashPos = baseName.find_last_of("\\/");
		if (slashPos > 0)
			baseName.erase(0, slashPos+1);

		if (baseName == "FixtureTop.sc") {
			//string fileName = m_pathName + "Fixture.h";
			string backupName = m_backupPath + "MinFixture.h";
			//rename(fileName.c_str(), backupName.c_str());

            SetInterceptFileName("Fixture.h");
            SetReplaceFileName(backupName);

			m_bGenedMinFixtureH = true;
			FILE *fp;
			if (!(fp = fopen(backupName.c_str(), "w"))) {
				printf("Could not open file '%s' for writing\n", backupName.c_str());
				ErrorExit();
			}

			//fprintf(fp, "sc_fixture\nSC_MODULE(CFixture) {\n\tsc_in<bool>\ti_clock;\n\tvoid Fixture();\n\tSC_CTOR(CFixture) {\n\t\tSC_METHOD(Fixture);\n\t\tsensitive_neg << i_clock;\n\t}\n};\n");
			fprintf(fp, "sc_fixture\nSC_MODULE(CFixture) {\n\tvoid Fixture();\n\tSC_CTOR(CFixture) {\n\t\tSC_METHOD(Fixture);\n\t}\n};\n");
			fclose(fp);
		}
	}

	if (g_htvArgs.IsPrmEnabled()) {
		string prmName = g_htvArgs.GetInputPathName();
		int periodPos = prmName.find_last_of(".");
		if (periodPos > 0)
			prmName.erase(periodPos+1);
		prmName += "prm";

		if (!(m_prmFp = fopen(prmName.c_str(), "w"))) {
			printf("Could not open file '%s' for writing\n", prmName.c_str());
			ErrorExit();
		}
	}
}

void CHtvDesign::CloseFiles()
{
	if (g_htvArgs.IsScMain())
		m_cppFile.Close();
	else
		m_vFile.Close();

	if (g_htvArgs.IsScHier())
		m_incFile.Close();

	if (m_prmFp)
		fclose(m_prmFp);
}

void CHtvDesign::DeleteFiles()
{
	if (g_htvArgs.IsScMain())
		m_cppFile.Delete();
	else
		m_vFile.Delete();

	if (g_htvArgs.IsScHier())
		m_incFile.Delete();

	if (m_prmFp)
		fclose(m_prmFp);
}

void CHtvDesign::HandleInputFileInit()
{
	if (g_htvArgs.IsScHier())
		m_incFile.Dup(GetInputFd(), m_genCppStartOffset, GetInputFileOffset());
}

void CHtvDesign::HandleScMainInit()
{
	m_cppFile.Dup(GetInputFd(), m_genCppStartOffset, GetInputFileOffset());
}

void CHtvDesign::HandleScMainStart()
{
	m_genCppStartOffset = GetInputFileOffset();
}

void CHtvDesign::HandleGenerateScMain()
{
	if (g_htvArgs.IsScMain()) {
		CheckMissingInstancePorts();
		CheckConsistentPortTypes();
		GenerateScMain();
	}
}

void CHtvDesign::HandleGenerateScModule(CHtfeIdent * pClass)
{
	if (pClass->IsInputFile() && GetErrorCnt() == 0 && GetWarningCnt() == 0)
		GenerateScModule((CHtvIdent *)pClass);
}
