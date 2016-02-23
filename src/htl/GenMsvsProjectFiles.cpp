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

#ifdef _WIN32

#include "MsvsProject.h"

#define CD32	0
#define CD64	1
#define CR32	2
#define CR64	3

#define HT_TOOLS_PATH "HtToolsPath_v1x"
#define HT_TOOLS_PATH_MACRO "$(HtToolsPath_v1x)"

bool PathCmp(string &strA, string &strB) 
{
	const char * pStrA = strA.c_str();
	const char * pStrB = strB.c_str();

	while (*pStrA != '\0' && *pStrB != '\0') {
		if (tolower(*pStrA) == tolower(*pStrB) ||
			*pStrA == '\\' && *pStrB == '/' ||
			*pStrA == '/' && *pStrB == '\\') {
				pStrA++; pStrB++;
		} else
			return false;
	}
	return *pStrA == '\0' && *pStrB == '\0';
}

void CDsnInfo::GenMsvsProjectFiles()
{
	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

	CMsvsProject msvs(g_appArgs.GetProjName(), m_unitName);

	// Filters
	msvs.AddFilter("ht_lib", "");
	msvs.AddFilter("ht", "");
	msvs.AddFilter("ht_lib\\host", "h;cpp;c");
	msvs.AddFilter("ht_lib\\sysc", "h;cpp;c");
	msvs.AddFilter("ht\\verilog", "");
	msvs.AddFilter("ht\\sysc", "h;cpp;c");
	msvs.AddFilter("src", "h;cpp;c");
	msvs.AddFilter("src_pers", "h;cpp;c");
	msvs.AddFilter("src_model", "h;cpp;c");

	bool bAeNext = false;
	bool bAePrev = false;
	for (size_t connIdx = 0; connIdx < getMsgIntfConnListSize(); connIdx += 1) {
		HtiFile::CMsgIntfConn * pMsgIntfConn = getMsgIntfConn(connIdx);

		if (pMsgIntfConn->m_aeNext)
			bAeNext = true;
		if (pMsgIntfConn->m_aePrev)
			bAePrev = true;
	}

	// Include files
	msvs.AddFile(Include, string("..\\ht\\sysc\\HostIntf.h"), "ht\\sysc");
	msvs.AddFile(Include, string("..\\ht\\sysc\\UnitIntf.h"), "ht\\sysc");

	char mifName[16];
    if (!g_appArgs.IsModelOnly()) {
	    msvs.AddFile(Include, string("..\\ht\\sysc\\Pers" + unitNameUc + "Common.h"), "ht\\sysc");
	    msvs.AddFile(Include, string("..\\ht\\sysc\\PersAeTop.h"), "ht\\sysc");
	    msvs.AddFile(Include, string("..\\ht\\sysc\\Pers" + m_unitName.Uc() + "Top.h"), "ht\\sysc");
	    msvs.AddFile(Include, string("..\\ht\\sysc\\PersHif.h"), "ht\\sysc");
	    msvs.AddFile(Include, string("..\\ht\\sysc\\Pers" + m_unitName.Uc() + "Hti.h"), "ht\\sysc");
	    msvs.AddFile(Include, string("..\\ht\\sysc\\Pers" + m_unitName.Uc() + "Hta.h"), "ht\\sysc");

	    for (size_t mifIdx = 0; mifIdx < m_mifInstList.size(); mifIdx += 1) {
		    sprintf(mifName, "Mif%d.h", m_mifInstList[mifIdx].m_mifId);
		    msvs.AddFile(Include, string("..\\ht\\sysc\\Pers" + m_unitName.Uc() + mifName), "ht\\sysc");
	    }

	    for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		    CModule &mod = *m_modList[modIdx];

		    if (!mod.m_bIsUsed || mod.m_bHostIntf) continue;

			int prevInstId = -1;
			for (size_t modInstIdx = 0; modInstIdx < mod.m_modInstList.size(); modInstIdx += 1) {
				CModInst &modInst = mod.m_modInstList[modInstIdx];

				if (modInst.m_instParams.m_instId == prevInstId)
					continue;

				prevInstId = modInst.m_instParams.m_instId;

				string instIdStr = GenIndexStr(mod.m_instIdCnt > 1, "%d", modInst.m_instParams.m_instId);

				msvs.AddFile(Include, string("..\\ht\\sysc\\Pers" + unitNameUc + mod.m_modName.Uc() + instIdStr + ".h"), "ht\\sysc");

				if (mod.m_barrierList.size() > 0 && mod.m_modInstList.size() > 1)
					msvs.AddFile(Include, string("..\\ht\\sysc\\Pers" + unitNameUc + mod.m_modName.Uc() + instIdStr + "BarCtl.h"), "ht\\sysc");
			}
	    }

		for (size_t gvIdx = 0; gvIdx < m_ngvList.size(); gvIdx += 1) {
			CNgvInfo * pNgvInfo = m_ngvList[gvIdx];
			CRam * pNgv = pNgvInfo->m_modInfoList[0].m_pNgv;
			msvs.AddFile(Include, string("..\\ht\\sysc\\PersGbl" + pNgv->m_gblName.Uc() + ".h"), "ht\\sysc");
		}

		if (bAeNext) {
		    msvs.AddFile(Include, string("..\\ht\\sysc\\PersMonSb.h"), "ht\\sysc");
		    msvs.AddFile(Include, string("..\\ht\\sysc\\PersMipSb.h"), "ht\\sysc");
		}

		if (bAePrev) {
		    msvs.AddFile(Include, string("..\\ht\\sysc\\PersMopSb.h"), "ht\\sysc");
		    msvs.AddFile(Include, string("..\\ht\\sysc\\PersMinSb.h"), "ht\\sysc");
		}

		msvs.AddFile(Include, string("..\\ht\\sysc\\SyscAeTop.h"), "ht\\sysc");
		msvs.AddFile(Include, string("..\\ht\\sysc\\SyscTop.h"), "ht\\sysc");
    }

	msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\host\\HtCtrlMsg.h"), "ht_lib\\host");
	msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\Ht.h"), "ht_lib");
	msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\host\\HtHif.h"), "ht_lib\\host");
	msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\host\\HtHifLib.h"), "ht_lib\\host");
	msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\host\\HtModel.h"), "ht_lib\\host");
	msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\host\\HtModelLib.h"), "ht_lib\\host");
	msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\host\\HtPlatform.h"), "ht_lib\\host");

	msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\mtrand.h"), "ht_lib\\sysc");
    if (!g_appArgs.IsModelOnly()) {
	    msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\MemRdWrIntf.h"), "ht_lib\\sysc");
	    msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\Params.h"), "ht_lib\\sysc");
	    msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\PersUnitCnt.h"), "ht_lib\\sysc");
	    msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\PersXbarStub.h"), "ht_lib\\sysc");
	    msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\PersMiStub.h"), "ht_lib\\sysc");
	    msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\PersMoStub.h"), "ht_lib\\sysc");
	    msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\HtMemTypes.h"), "ht_lib\\sysc");
	    msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\SyscClock.h"), "ht_lib\\sysc");
	    msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\SyscDisp.h"), "ht_lib\\sysc");
	    msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\SyscMemLib.h"), "ht_lib\\sysc");
	    msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\SyscMem.h"), "ht_lib\\sysc");
	    msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\SyscMonLib.h"), "ht_lib\\sysc");
		msvs.AddFile(Include, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\SyscMon.h"), "ht_lib\\sysc");
    }

	// Compile files
    msvs.AddFile(Compile, string("..\\ht\\sysc\\UnitIntf.cpp"), "ht\\sysc");

    if (!g_appArgs.IsModelOnly()) {
	    msvs.AddFile(Compile, string("..\\ht\\sysc\\PersHif.cpp"), "ht\\sysc");
	    msvs.AddFile(Compile, string("..\\ht\\sysc\\Pers" + m_unitName.Uc() + "Hti.cpp"), "ht\\sysc");
	    msvs.AddFile(Compile, string("..\\ht\\sysc\\Pers" + m_unitName.Uc() + "Hta.cpp"), "ht\\sysc");

	    for (size_t mifIdx = 0; mifIdx < m_mifInstList.size(); mifIdx += 1) {
		    sprintf(mifName, "Mif%d.cpp", m_mifInstList[mifIdx].m_mifId);
		    msvs.AddFile(Compile, string("..\\ht\\sysc\\Pers" + m_unitName.Uc() + mifName), "ht\\sysc");
	    }

	    for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		    CModule &mod = *m_modList[modIdx];

		    if (!mod.m_bIsUsed || mod.m_bHostIntf) continue;

			int prevInstId = -1;
			for (size_t modInstIdx = 0; modInstIdx < mod.m_modInstList.size(); modInstIdx += 1) {
				CModInst &modInst = mod.m_modInstList[modInstIdx];

				if (modInst.m_instParams.m_instId == prevInstId)
					continue;

				prevInstId = modInst.m_instParams.m_instId;

				string instIdStr = GenIndexStr(mod.m_instIdCnt > 1, "%d", modInst.m_instParams.m_instId);

				msvs.AddFile(Compile, string("..\\ht\\sysc\\Pers" + unitNameUc + mod.m_modName.Uc() + instIdStr + ".cpp"), "ht\\sysc");

				if (mod.m_barrierList.size() > 0 && mod.m_modInstList.size() > 1)
					msvs.AddFile(Compile, string("..\\ht\\sysc\\Pers" + unitNameUc + mod.m_modName.Uc() + instIdStr + "BarCtl.cpp"), "ht\\sysc");
			}

			msvs.AddFile(Compile, string("..\\src_pers\\Pers" + unitNameUc + mod.m_modName.Uc() + "_src.cpp"), "src_pers");
	    }

		for (size_t gvIdx = 0; gvIdx < m_ngvList.size(); gvIdx += 1) {
			CNgvInfo * pNgvInfo = m_ngvList[gvIdx];
			CRam * pNgv = pNgvInfo->m_modInfoList[0].m_pNgv;
			msvs.AddFile(Compile, string("..\\ht\\sysc\\PersGbl" + pNgv->m_gblName.Uc() + ".cpp"), "ht\\sysc");
		}

	    msvs.AddFile(Compile, string("..\\ht\\sysc\\SyscMon.cpp"), "ht\\sysc");

		if (bAeNext) {
		    msvs.AddFile(Compile, string("..\\ht\\sysc\\PersMonSb.cpp"), "ht\\sysc");
		    msvs.AddFile(Compile, string("..\\ht\\sysc\\PersMipSb.cpp"), "ht\\sysc");
		}

		if (bAePrev) {
		    msvs.AddFile(Compile, string("..\\ht\\sysc\\PersMopSb.cpp"), "ht\\sysc");
		    msvs.AddFile(Compile, string("..\\ht\\sysc\\PersMinSb.cpp"), "ht\\sysc");
		}
    }

	msvs.AddFile(Compile, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\host\\HtHif.cpp"), "ht_lib\\host");
	msvs.AddFile(Compile, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\host\\HtHifLib.cpp"), "ht_lib\\host");
	msvs.AddFile(Compile, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\host\\HtModelLib.cpp"), "ht_lib\\host");
	msvs.AddFile(Compile, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\host\\HtPlatform.cpp"), "ht_lib\\host");
	msvs.AddFile(Compile, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\mtrand.cpp"), "ht_lib\\sysc");

    if (!g_appArgs.IsModelOnly()) {
	    msvs.AddFile(Compile, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\PersUnitCnt.cpp"), "ht_lib\\sysc");
	    msvs.AddFile(Compile, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\PersXbarStub.cpp"), "ht_lib\\sysc");
	    msvs.AddFile(Compile, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\PersMiStub.cpp"), "ht_lib\\sysc");
	    msvs.AddFile(Compile, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\PersMoStub.cpp"), "ht_lib\\sysc");
	    msvs.AddFile(Compile, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\SyscClockLib.cpp"), "ht_lib\\sysc");
	    msvs.AddFile(Compile, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\SyscDispLib.cpp"), "ht_lib\\sysc");
	    msvs.AddFile(Compile, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\SyscMemLib.cpp"), "ht_lib\\sysc");
	    msvs.AddFile(Compile, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\SyscMonLib.cpp"), "ht_lib\\sysc");
    }

	// Non-Build files
    for (int i = 0; i < g_appArgs.GetInputFileCnt(); i += 1) {
        string fileName = g_appArgs.GetInputFile(i);
        int pos = fileName.find_last_of("/\\");
		if (pos >= 0)
			fileName = fileName.substr(pos+1);

	    msvs.AddFile(NonBuild, string("..\\src_pers\\" + fileName), "src_pers");
    }

	// Custom Build files
    if (!g_appArgs.IsModelOnly()) {
	    msvs.AddFile(Custom, string("..\\src_pers\\" + g_appArgs.GetHtlName()), "src_pers");
	    msvs.AddFile(Custom, string("..\\ht\\sysc\\Pers" + m_unitName.Uc() + "Top.sc"), "ht\\sysc");
	    msvs.AddFile(Custom, string("..\\ht\\sysc\\PersAeTop.sc"), "ht\\sysc", string("..\\ht\\sysc\\Pers" + m_unitName.Uc() + "Top.h"));
	    msvs.AddFile(CustomHtLib, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\SyscAeTop.sc"), "ht_lib\\sysc", "..\\ht\\sysc\\PersAeTop.h");
	    msvs.AddFile(CustomHtLib, string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc\\SyscTop.sc"), "ht_lib\\sysc", "..\\ht\\sysc\\SyscAeTop.h");

	    msvs.AddFile(Custom, string("..\\ht\\verilog\\PersHif.v_"), "ht\\verilog");
	    msvs.AddFile(Custom, string("..\\ht\\verilog\\Pers" + m_unitName.Uc() + "Hti.v_"), "ht\\verilog");
	    msvs.AddFile(Custom, string("..\\ht\\verilog\\Pers" + m_unitName.Uc() + "Hta.v_"), "ht\\verilog");

	    for (size_t mifIdx = 0; mifIdx < m_mifInstList.size(); mifIdx += 1) {
		    sprintf(mifName, "Mif%d.v_", m_mifInstList[mifIdx].m_mifId);
		    msvs.AddFile(Custom, string("..\\ht\\verilog\\Pers" + m_unitName.Uc() + mifName), "ht\\verilog");
	    }

	    for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		    CModule &mod = *m_modList[modIdx];

		    if (!mod.m_bIsUsed || mod.m_bHostIntf) continue;

		    bool bGenFx = mod.m_modName == g_appArgs.GetFxModName();

		    msvs.AddFile(Custom, string("..\\ht\\verilog\\Pers" + unitNameUc + mod.m_modName.Uc() + ".v_"), "ht\\verilog", "", bGenFx);
		
			if (mod.m_barrierList.size() > 0 && mod.m_modInstList.size() > 1)
			    msvs.AddFile(Custom, string("..\\ht\\verilog\\Pers" + unitNameUc + mod.m_modName.Uc() + "BarCtl.v_"), "ht\\verilog", "", bGenFx);
	    }

		for (size_t gvIdx = 0; gvIdx < m_ngvList.size(); gvIdx += 1) {
			CNgvInfo * pNgvInfo = m_ngvList[gvIdx];
			CRam * pNgv = pNgvInfo->m_modInfoList[0].m_pNgv;
			msvs.AddFile(Custom, string("..\\ht\\verilog\\PersGbl" + pNgv->m_gblName.Uc() + ".v_"), "ht\\verilog");
		}

		if (bAeNext) {
		    msvs.AddFile(Custom, string("..\\ht\\verilog\\PersMonSb.v_"), "ht\\verilog");
		    msvs.AddFile(Custom, string("..\\ht\\verilog\\PersMipSb.v_"), "ht\\verilog");
		}

		if (bAePrev) {
		    msvs.AddFile(Custom, string("..\\ht\\verilog\\PersMopSb.v_"), "ht\\verilog");
		    msvs.AddFile(Custom, string("..\\ht\\verilog\\PersMinSb.v_"), "ht\\verilog");
		}

	    msvs.AddFile(CustomHtLib, string("..\\ht\\verilog\\PersUnitCnt.v_"), "ht\\verilog");
	    msvs.AddFile(CustomHtLib, string("..\\ht\\verilog\\PersXbarStub.v_"), "ht\\verilog");
	    msvs.AddFile(CustomHtLib, string("..\\ht\\verilog\\PersMiStub.v_"), "ht\\verilog");
	    msvs.AddFile(CustomHtLib, string("..\\ht\\verilog\\PersMoStub.v_"), "ht\\verilog");
    }

	if (msvs.CheckIfProjFilterFileOkay() == false) {
		msvs.GenMsvsFiles();
		exit(1);
	} else if (msvs.CheckIfProjFileOkay() == false) {
		msvs.GenMsvsFiles();
		exit(1);
	//} else {
	//	msvs.GenMsvsFiles();
	//	ErrorExit();
	}
}

bool CMsvsProject::CheckIfProjFileOkay()
{
	FILE * fp = fopen((m_projName.AsStr() + ".vcxproj").c_str(), "r");

	if (fp == 0)
		return false;

	vector<string> curFilterList;
	vector<CMsvsFile> curFileList;

	vector<string> additionalIncDirList;
	additionalIncDirList.push_back("../ht/sysc");
	additionalIncDirList.push_back("../src");
	additionalIncDirList.push_back("../src_pers");
	additionalIncDirList.push_back(string(HT_TOOLS_PATH_MACRO) + string("/ht_lib"));
	additionalIncDirList.push_back(string(HT_TOOLS_PATH_MACRO) + string("/ht_lib/sysc"));
	additionalIncDirList.push_back(string(HT_TOOLS_PATH_MACRO) + string("/systemc-2.3.1/src"));
	additionalIncDirList.push_back(string(HT_TOOLS_PATH_MACRO) + string("/pthread-win32-2.9.0"));

	char line[4096];
	m_pLine = "";

	// get next property
	for(;;) {
		while (*m_pLine != '\0' && *m_pLine != '<') m_pLine += 1;
		if (*m_pLine == '\0') {
			if (fgets(line, 4096, fp) == 0)
				break;

			m_pLine = line;
			continue;
		}

		m_pLine += 1;
		if (*m_pLine == '/')
			// closing property
			continue;

		char const *pStart = m_pLine;
		while (*m_pLine != '\0' && *m_pLine != ' ' && *m_pLine != '>') m_pLine += 1;

		if (*m_pLine != ' ' && *m_pLine != '>')
			return false;

		*m_pLine++ = '\0';
		string prop = pStart;

		EMsvsFile propType = Undefined;
		if (prop == "AdditionalIncludeDirectories" && !CheckRequiredProperties(additionalIncDirList))
			return false;
	}

	fclose(fp);

	return true;
}

bool CMsvsProject::CheckRequiredProperties(vector<string> &reqPropList)
{
	// create list of properties, check against required list
	string valueStr;
	ReadPropertyValue(valueStr);

	vector<string> valueList;
	ParsePropertyValueStr(valueStr, valueList);

	HtlAssert(reqPropList.size() < 10);
	bool bUsedList[10] = { 0 };

	size_t cnt = 0;
	for (size_t i = 0; i < valueList.size(); i += 1) {
		for (size_t j = 0; j < reqPropList.size(); j += 1) {
			if (PathCmp(valueList[i], reqPropList[j])) {
				if (bUsedList[j])
					return false;
				bUsedList[j] = true;
				cnt += 1;
				break;
			}
		}
	}

	// if a required include directory is missing then regenerate project file
	return cnt == reqPropList.size();
}

bool CMsvsProject::CheckIfProjFilterFileOkay()
{
	// check if current project files need updated
	//  read current filter file and compare filter and file lists
	//  if a filter or file is found in current project file then add to msvs structure
	//  if a filter or file is missing in current project then an update is needed

	FILE * fp = fopen((m_projName.AsStr() + ".vcxproj.filters").c_str(), "r");

	if (fp == 0)
		return false;

	vector<string> curFilterList;
	vector<CMsvsFile> curFileList;

	char line[4096];
	char *pLine = line;
	fgets(line, 4096, fp);

	// get next property
	for(;;) {
		while (*pLine != '\0' && *pLine != '<') pLine += 1;
		if (*pLine == '\0') {
			if (fgets(line, 4096, fp) == 0)
				break;

			pLine = line;
			continue;
		}

		pLine += 1;
		if (*pLine == '/')
			// closing property
			continue;

		char const *pStart = pLine;
		while (*pLine != '\0' && *pLine != ' ' && *pLine != '>') pLine += 1;

		if (*pLine != ' ' && *pLine != '>')
			return false;

		*pLine++ = '\0';
		string prop = pStart;

		EMsvsFile propType = Undefined;
		if (prop == "ClCompile")
			propType = Compile;
		else if (prop == "ClInclude")
			propType = Include;
		else if (prop == "CustomBuild")
			propType = Custom;
		else if (prop == "None")
			propType = NonBuild;
		else if (prop == "Filter")
			propType = Filter;
		else
			continue;

		// parse the filter or path name
		while (*pLine == ' ') pLine += 1;
		if (strncmp(pLine, "Include=", 8) != 0)
			return false;

		pLine += 8;

		// parse string with quotes
		if (*pLine++ != '"')
			return false;

		pStart = pLine;
		while (*pLine != '\0' && *pLine != '"') pLine += 1;
		if (*pLine != '"')
			return false;

		*pLine++ = '\0';
		string pathName = pStart;

		if (propType == Filter) {
			curFilterList.push_back(pathName);
			continue;
		}

		// find filter property for file
		for(;;) {
			while (*pLine != '\0' && *pLine != '<') pLine += 1;
			if (*pLine == '\0') {
				if (fgets(line, 4096, fp) == 0)
					return false;

				pLine = line;
				continue;
			}

			pLine += 1;
			if (*pLine == '/')
				// closing property
				continue;

			char const *pStart = pLine;
			while (*pLine != '\0' && *pLine != '>') pLine += 1;

			if (*pLine != '>')
				return false;

			*pLine++ = '\0';
			string prop2 = pStart;

			if (prop2 == "Filter")
				break;
		}

		// parse filter name
		pStart = pLine;
		while (*pLine != '\0' && *pLine != '<') pLine += 1;
		if (*pLine != '<')
			return false;

		*pLine++ = '\0';
		string filterName = pStart;

		curFileList.push_back(CMsvsFile(propType, pathName, filterName, "", false, true));
	}

	fclose(fp);

	///////////////////////////////////////////////
	// compare list of filters and files

	bool bProjFilterFilesOkay = true;

	// first check filters
	for (size_t curIdx = 0; curIdx < curFilterList.size(); curIdx += 1) {
		string &curFilter = curFilterList[curIdx];

		size_t newIdx;
		for (newIdx = 0; newIdx < m_filterList.size(); newIdx += 1) {
			CMsvsFilter &newFilter = m_filterList[newIdx];

			if (newFilter.m_filterName == curFilter)
				break;
		}

		if (newIdx < m_filterList.size()) {
			m_filterList[newIdx].m_bFound = true;
			continue;	// found it
		}

		// curFilter was not found in newFilterList, just add it in
		AddFilter(curFilter, "", true);
	}

	// check if any filters are missing
	for (size_t newIdx = 0; newIdx < m_filterList.size(); newIdx += 1) {
		CMsvsFilter &newFilter = m_filterList[newIdx];

		if (!newFilter.m_bFound)
			bProjFilterFilesOkay = false; // filter is missing, generate new project files
	}

	// second check files
	for (size_t curIdx = 0; curIdx < curFileList.size(); curIdx += 1) {
		CMsvsFile &file1 = curFileList[curIdx];

		size_t newIdx;
		for (newIdx = 0; newIdx < m_fileList.size(); newIdx += 1) {
			CMsvsFile &file2 = m_fileList[newIdx];

			if (file2.m_pathName == file1.m_pathName)
				break;
		}

		if (newIdx < m_fileList.size()) {
			m_fileList[newIdx].m_bFound = true;
			continue;	// found it
		}

		if (file1.m_filterName.substr(0, 3) != "src") {
			bProjFilterFilesOkay = false; // added file in filter other than src/*, generate new project files
			continue;
		}

		// file1 was not found in file2 list, just add it in
		AddFile(file1.m_fileType, file1.m_pathName, file1.m_filterName, file1.m_dependencies, false, true);
	}

	// check if any files are missing
	for (size_t newIdx = 0; newIdx < m_fileList.size(); newIdx += 1) {
		CMsvsFile &file2 = m_fileList[newIdx];

		if (!file2.m_bFound)
			bProjFilterFilesOkay = false; // file is missing, generate new project files
	}

	return bProjFilterFilesOkay;
}

void CMsvsProject::GenMsvsFiles()
{
	//////////////////////////////////////////////////////////
	// Generate Msvs project filter file

	FILE * fp = fopen((m_projName.AsStr() + ".vcxproj.filters").c_str(), "w");

	GenMsvsFilterHeader(fp);
	GenMsvsFiltersXml(fp);
	GenMsvsFilterFilesXml(fp, Include);
	GenMsvsFilterFilesXml(fp, Compile);
	GenMsvsFilterFilesXml(fp, Custom);
	GenMsvsFilterFilesXml(fp, CustomHtLib);
	GenMsvsFilterFilesXml(fp, NonBuild);
	GenMsvsFilterTrailer(fp);

	fclose(fp);

	//////////////////////////////////////////////////////////
	// Generate Msvs project file

	// first read existing vcxproj properties
	m_fp = fopen((m_projName.AsStr() + ".vcxproj").c_str(), "r");

	ReadMsvsProjectFile();

	// now write new vcxproj file
	fp = fopen((m_projName.AsStr() + ".vcxproj").c_str(), "w");

	GenMsvsProjectHeader(fp);
	GenMsvsProjectFiles(fp, Include);
	GenMsvsProjectFiles(fp, Compile);
	GenMsvsProjectFiles(fp, Custom);
	GenMsvsProjectFiles(fp, CustomHtLib);
	GenMsvsProjectFiles(fp, NonBuild);
	GenMsvsProjectTrailer(fp);

	fclose(fp);
}

string CMsvsProject::GenGuid()
{
#ifdef _WIN32
	GUID guid;
	int hr = ::UuidCreate(&guid);

	char guidStr[64];
	sprintf(guidStr, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
		(int)guid.Data1, (int)guid.Data2, (int)guid.Data3, guid.Data4[0], guid.Data4[1],
		guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

	return string(guidStr);
#else
	return string();
#endif
}

void CMsvsProject::GenMsvsFilterHeader(FILE *fp)
{
	fprintf(fp, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
	fprintf(fp, "<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n");
}

void CMsvsProject::GenMsvsFiltersXml(FILE *fp)
{
	fprintf(fp, "  <ItemGroup>\n");

	for (size_t filterIdx = 0; filterIdx < m_filterList.size(); filterIdx += 1) {
		CMsvsFilter &filter = m_filterList[filterIdx];

		fprintf(fp, "    <Filter Include=\"%s\">\n", filter.m_filterName.c_str());
		fprintf(fp, "      <UniqueIdentifier>%s</UniqueIdentifier>\n", GenGuid().c_str());
		if (filter.m_extensions.size() > 0)
			fprintf(fp, "      <Extensions>%s</Extensions>\n", filter.m_extensions.c_str());
		fprintf(fp, "    </Filter>\n");
	}
	fprintf(fp, "  </ItemGroup>\n");
}

void CMsvsProject::GenMsvsFilterFilesXml(FILE *fp, EMsvsFile eFileType)
{
	char * pFileType = 0;
	switch (eFileType) {
	case Include: pFileType = "ClInclude"; break;
	case Compile: pFileType = "ClCompile"; break;
	case Custom:  pFileType = "CustomBuild"; break;
	case CustomHtLib:  pFileType = "CustomBuild"; break;
	case NonBuild: pFileType = "None"; break;
	default: HtlAssert(0);
	}

	fprintf(fp, "  <ItemGroup>\n");

	for (size_t fileIdx = 0; fileIdx < m_fileList.size(); fileIdx += 1) {
		CMsvsFile &file = m_fileList[fileIdx];

		if (file.m_fileType != eFileType) continue;

		fprintf(fp, "    <%s Include=\"%s\">\n", pFileType, file.m_pathName.c_str());
		fprintf(fp, "      <Filter>%s</Filter>\n", file.m_filterName.c_str());
		fprintf(fp, "    </%s>\n", pFileType);
	}
	fprintf(fp, "  </ItemGroup>\n");
}

void CMsvsProject::GenMsvsFilterTrailer(FILE *fp)
{
	fprintf(fp, "</Project>\n");
}

void CMsvsProject::ReadMsvsProjectFile()
{
	if (m_fp == 0) return;

	// for now, just save ItemDefinitionGroup info
	m_line[0] = '\0';
	m_pLine = m_line;
	m_bEof = false;

	string propStr;
	bool bOpen;
	//fgets(line, 4096, fp);

	// get next property
	for(;;) {
		ReadNextProperty(propStr, bOpen);
		if (m_bEof) break;

		if (propStr == "ItemDefinitionGroup" && bOpen)
			ReadItemDefinitionGroup();
		else if (propStr == "PropertyGroup" && bOpen)
			ReadPropertyGroup();
		else if (propStr == "ItemGroup" && bOpen)
			ReadItemGroup();
	}

	fclose(m_fp);
}

void CMsvsProject::ReadItemDefinitionGroup()
{
	string conditionStr;
	if (!ReadCondition(conditionStr)) {
		printf("Missing condition for ItemDefinitionGroup\n");
		return;
	}

	int condIdx;
	if (conditionStr == "Debug|Win32")
		condIdx = 0;
	else if (conditionStr == "Debug|x64")
		condIdx = 1;
	else if (conditionStr == "Release|Win32")
		condIdx = 2;
	else if (conditionStr == "Release|x64")
		condIdx = 3;
	else {
		printf("Unknown configuration '%s'\n", conditionStr.c_str());
		return;
	}

	string propStr;
	bool bOpen;

	for (;;) {
		// read ItemDefinitionGroup sub properties
		ReadNextProperty(propStr, bOpen);
		if (m_bEof) {
			printf("Found end-of-file within <ItemDefinitionGroup>\n");
			return;
		}

		if (!bOpen) {
			if (propStr == "ItemDefinitionGroup")
				break;
			else
				continue;
		}

		if (propStr == "PreprocessorDefinitions")
			ReadPreprocessorDefinitions(condIdx);
		else if (propStr == "AdditionalIncludeDirectories")
			ReadAdditionalIncludeDirectories(condIdx);
		else if (propStr == "AdditionalLibraryDirectories")
			ReadAdditionalLibraryDirectories(condIdx);
		else if (propStr == "AdditionalDependencies")
			ReadAdditionalDependencies(condIdx);
		else if (propStr == "ForcedIncludeFiles")
			ReadForcedIncludeFiles(condIdx);
		else if (propStr == "DisableSpecificWarnings")
			ReadDisableSpecificWarnings(condIdx);
	}
}

void CMsvsProject::ReadPropertyGroup()
{
	string conditionStr;
	if (!ReadCondition(conditionStr))
		return;

	int condIdx;
	if (conditionStr == "Debug|Win32")
		condIdx = CD32;
	else if (conditionStr == "Debug|x64")
		condIdx = CD64;
	else if (conditionStr == "Release|Win32")
		condIdx = CR32;
	else if (conditionStr == "Release|x64")
		condIdx = CR64;
	else {
		printf("Unknown configuration '%s'\n", conditionStr.c_str());
		return;
	}

	string propStr;
	bool bOpen;

	for (;;) {
		// read ItemDefinitionGroup sub properties
		ReadNextProperty(propStr, bOpen);
		if (m_bEof) {
			printf("Found end-of-file within <PropertyGroup>\n");
			return;
		}

		if (!bOpen) {
			if (propStr == "PropertyGroup")
				break;
			else
				continue;
		}

		if (propStr == "ConfigurationType")
			ReadConfigurationType(condIdx);
	}
}

void CMsvsProject::ReadItemGroup()
{
	string propStr;
	bool bOpen;
	string nameStr, valueStr;
	string itemGroupTypeStr;

	// ignore labeled ItemGroups
	if (ReadPropertyQualifier(nameStr, valueStr))
		return;

	for (;;) {
		// read ItemDefinitionGroup sub properties
		ReadNextProperty(itemGroupTypeStr, bOpen);
		if (m_bEof) {
			printf("Found end-of-file within <ItemGroup>\n");
			return;
		}

		if (!bOpen) {
			if (itemGroupTypeStr == "ItemGroup")
				break;
			else
				continue;
		}

		if (itemGroupTypeStr == "ClInclude" || itemGroupTypeStr == "ClCompile" || itemGroupTypeStr == "CustomBuild") {
			if (!ReadPropertyQualifier(nameStr, valueStr) || nameStr != "Include") {
				printf("Expected Include for ItemGroup\n");
				return;
			}

			// find Include file
			size_t idx;
			for (idx = 0; idx < m_fileList.size(); idx += 1)
				if (PathCmp(valueStr, m_fileList[idx].m_pathName) == true)
					break;

			if (idx == m_fileList.size() && PathCmp(valueStr.c_str(), "../src_model", 12) != 0 && PathCmp(valueStr.c_str(), "../src_pers", 11) != 0)
				printf("Did not find file %s in project filter file\n", valueStr.c_str());

			for (;;) {
				// read ItemDefinitionGroup sub properties
				ReadNextProperty(propStr, bOpen);
				if (m_bEof) {
					printf("Found end-of-file within <ItemDefinitionGroup>\n");
					return;
				}

				if (!bOpen) {
					if (propStr == itemGroupTypeStr)
						break;
					else
						continue;
				}

				if (propStr == "ForcedIncludeFiles") {
					string conditionStr;
					if (ReadCondition(conditionStr) && idx < m_fileList.size()) {

						int condIdx;
						if (conditionStr == "Debug|Win32")
							condIdx = 0;
						else if (conditionStr == "Debug|x64")
							condIdx = 1;
						else if (conditionStr == "Release|Win32")
							condIdx = 2;
						else if (conditionStr == "Release|x64")
							condIdx = 3;
						else {
							printf("Unknown configuration '%s'\n", conditionStr.c_str());
							return;
						}

						ReadForcedIncludeFiles(m_fileList[idx].m_forcedIncludeFiles[condIdx]);
					}
				}

				if (propStr == "ExcludedFromBuild") {
					string conditionStr;
					if (ReadCondition(conditionStr) && idx < m_fileList.size())
						m_fileList[idx].m_excludeFromBuild.push_back(conditionStr);
				}

				if (propStr == "Command") {
					string conditionStr;
					if (ReadCondition(conditionStr) && idx < m_fileList.size()) {
						//m_fileList[idx].m_excludeFromBuild.push_back(conditionStr);

						int condIdx;
						if (conditionStr == "Debug|Win32")
							condIdx = 0;
						else if (conditionStr == "Debug|x64")
							condIdx = 1;
						else if (conditionStr == "Release|Win32")
							condIdx = 2;
						else if (conditionStr == "Release|x64")
							condIdx = 3;
						else {
							printf("Unknown configuration '%s'\n", conditionStr.c_str());
							return;
						}

						ReadCommandDefines(m_fileList[idx].m_extraDefines[condIdx]);
					}
				}
			}
		}
	}
}

void CMsvsProject::ReadNextProperty(string &propStr, bool &bOpen)
{
	// skip until '<' found
	static string prevPropStr;
	bool bInStr = false;
	for (;;) {
		if (*m_pLine == 0) {
			if (fgets(m_line, 4096, m_fp) == 0) {
				m_bEof = true;
				return;
			}
			m_pLine = m_line;
			continue;
		}
		if (*m_pLine == '"')
			bInStr = !bInStr;

		if ((*m_pLine == '<' || *m_pLine == '/') && !bInStr) {
			if (*m_pLine == '<')
				m_pLine += 1;
			break;
		}

		m_pLine += 1;
	}

	if (*m_pLine == '/') {
		bOpen = false;
		m_pLine += 1;
	} else
		bOpen = true;

	// get property, ends with space or '>'
	char *pStart = m_pLine;
	while (*m_pLine != ' ' && *m_pLine != '>' && *m_pLine != '\0')
		m_pLine += 1;

	if (m_pLine == pStart)
		propStr = prevPropStr;
	else
		prevPropStr = propStr = string(pStart, m_pLine - pStart);
}

bool CMsvsProject::ReadCondition(string &conditionStr)
{
	string nameStr, valueStr;

	for (;;) {
		if (!ReadPropertyQualifier(nameStr, valueStr))
			return false;

		if (nameStr != "Condition")
			continue;

		const char *pStr = valueStr.c_str();
		while ((*pStr != '=' || pStr[1] != '=') && pStr[1] != '\0')
			pStr += 1;

		if (*pStr == '\0')
			return false;

		pStr += 2;

		if (*pStr != '\'')
			return false;

		pStr += 1;

		const char *pStart = pStr;
		while (*pStr != '\'' && *pStr != '\0')
			pStr += 1;

		if (*pStr == '\0')
			return false;

		conditionStr = string(pStart, pStr - pStart);

		return true;
	}
}

bool CMsvsProject::ReadPropertyQualifier(string &nameStr, string &valueStr)
{
	// read name="value"
	while (*m_pLine == ' ')
		m_pLine += 1;

	const char *pStart = m_pLine;
	while (*m_pLine != '=' && *m_pLine != '\0')
		m_pLine += 1;

	if (*m_pLine == '\0')
		return false;

	nameStr = string(pStart, m_pLine - pStart);

	m_pLine += 1;

	if (*m_pLine != '"')
		return false;

	m_pLine += 1;

	pStart = m_pLine;
	while (*m_pLine != '"' && *m_pLine != '\0')
		m_pLine += 1;

	if (*m_pLine == '\0')
		return false;

	valueStr = string(pStart, m_pLine - pStart);

	m_pLine += 1;

	return true;
}

bool CMsvsProject::PathCmp(string path1, string path2, size_t len)
{
    // compare two windows paths for equal (/ and \ are treated as the same)
    if (path1.size() != path2.size())
        return false;

    for (size_t i = 0; i < path1.size() && i < len; i += 1)
        if (tolower(path1[i]) != tolower(path2[i]) && !(path1[i] == '/' && path2[i] == '\\' || path1[i] == '\\' && path2[i] == '/'))
            return false;

    return true;
}

void CMsvsProject::ReadConfigurationType(int condIdx)
{
	if (*m_pLine++ != '>') return;

	string valueStr;
	ReadPropertyValue(valueStr);

	vector<string> valueList;
	ParsePropertyValueStr(valueStr, valueList);

	for (size_t i = 0; i < valueList.size(); i += 1) {
		// found extra string
		AddStringToList(m_idg[condIdx].m_configurationType, valueList[i]);
	}
}

void CMsvsProject::ReadPreprocessorDefinitions(int condIdx)
{
	if (*m_pLine++ != '>') return;

	string valueStr;
	ReadPropertyValue(valueStr);

	vector<string> valueList;
	ParsePropertyValueStr(valueStr, valueList);

	bool bHtSysC = false;
	bool bHtModel = false;

	for (size_t i = 0; i < valueList.size(); i += 1) {
		if (valueList[i] == "HT_SYSC") { bHtSysC = true; continue; }
		if (valueList[i] == "HT_MODEL") { bHtModel = true; continue; }
		if (valueList[i] == "_SYSTEM_C") continue;
		if (valueList[i] == "WIN32") continue;
		if (valueList[i] == "NDEBUG") continue;
		if (valueList[i] == "_DEBUG") continue;
		if (valueList[i] == "_CONSOLE") continue;
		if (valueList[i] == "%(PreprocessorDefinitions)") continue;

		// found extra string
		AddStringToList(m_idg[condIdx].m_preprocessorDefintions, valueList[i]);
	}

	m_idg[condIdx].m_bHtModel = g_appArgs.IsModelOnly();
	string str = m_idg[condIdx].m_bHtModel ? "HT_MODEL" : "HT_SYSC";
	AddStringToList(m_idg[condIdx].m_preprocessorDefintions, str);
}

void CMsvsProject::ReadForcedIncludeFiles(int condIdx)
{
	if (*m_pLine++ != '>') return;

	string valueStr;
	ReadPropertyValue(valueStr);

	vector<string> valueList;
	ParsePropertyValueStr(valueStr, valueList);

	for (size_t i = 0; i < valueList.size(); i += 1) {

		if (valueList[i] == "../../msvs10/linux/linux.h")
			valueList[i] = "../../msvs12/linux/linux.h";

		// found extra string
		AddStringToList(m_idg[condIdx].m_forcedIncludeFiles, valueList[i]);
	}
}

void CMsvsProject::ReadDisableSpecificWarnings(int condIdx)
{
	if (*m_pLine++ != '>') return;

	string valueStr;
	ReadPropertyValue(valueStr);

	vector<string> valueList;
	ParsePropertyValueStr(valueStr, valueList);

	for (size_t i = 0; i < valueList.size(); i += 1) {

		// found extra string
		AddStringToList(m_idg[condIdx].m_disableSpecificWarnings, valueList[i]);
	}
}

void CMsvsProject::ReadCommandDefines(vector<string> &extraDefines)
{
	if (*m_pLine++ != '>') return;

	string valueStr;
	ReadPropertyValue(valueStr);

	vector<string> valueList;
	ParseCommandValueStr(valueStr, valueList);

	for (size_t i = 0; i < valueList.size(); i += 1) {
		if (strncmp(valueList[i].c_str(), "-D", 2) != 0)
			continue;

		if (strcmp(valueList[i].c_str() + 2, "HT_SYSC") == 0) continue;
		if (strcmp(valueList[i].c_str() + 2, "HT_MODEL") == 0) continue;

		// found extra string
		AddStringToList(extraDefines, valueList[i]);
	}
}

void CMsvsProject::ReadForcedIncludeFiles(vector<string> &forcedIncludeFiles)
{
	if (*m_pLine++ != '>') return;

	string valueStr;
	ReadPropertyValue(valueStr);

	vector<string> valueList;
	ParseCommandValueStr(valueStr, valueList);

	for (size_t i = 0; i < valueList.size(); i += 1) {

		// found extra string
		AddStringToList(forcedIncludeFiles, valueList[i]);
	}
}

void CMsvsProject::AddStringToList(vector<string> & list, string & str)
{
	for (size_t i = 0; i < list.size(); i += 1)
		if (list[i] == str)
			return;

	list.push_back(str);
}

void CMsvsProject::ReadAdditionalIncludeDirectories(int condIdx)
{
	if (*m_pLine++ != '>') return;

	string valueStr;
	ReadPropertyValue(valueStr);

	vector<string> valueList;
	ParsePropertyValueStr(valueStr, valueList);

	for (size_t i = 0; i < valueList.size(); i += 1) {
		if (PathCmp(valueList[i], "..\\ht\\sysc")) continue;
		if (PathCmp(valueList[i], "..\\src")) continue;
		if (PathCmp(valueList[i], "..\\src_pers")) continue;
		if (PathCmp(valueList[i], "..\\src_model")) continue;
		if (PathCmp(valueList[i], string("C:\\ht\\ht_lib"))) continue;
		if (PathCmp(valueList[i], string("C:\\ht\\ht_lib\\sysc"))) continue;
		if (PathCmp(valueList[i], string("C:\\ht\\systemc-2.3.0\\src"))) continue;
		if (PathCmp(valueList[i], string("C:\\ht\\systemc-2.3.1\\src"))) continue;
		if (PathCmp(valueList[i], string("C:\\ht\\pthread-win32-2.9.0"))) continue;
		if (PathCmp(valueList[i], string("$(HtToolsPath)\\ht_lib"))) continue;
		if (PathCmp(valueList[i], string("$(HtToolsPath)\\ht_lib\\sysc"))) continue;
		if (PathCmp(valueList[i], string("$(HtToolsPath)\\systemc-2.3.0\\src"))) continue;
		if (PathCmp(valueList[i], string("$(HtToolsPath)\\systemc-2.3.1\\src"))) continue;
		if (PathCmp(valueList[i], string("$(HtToolsPath)\\pthread-win32-2.9.0"))) continue;
		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib"))) continue;
		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\ht_lib\\sysc"))) continue;
		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\systemc-2.3.0\\src"))) continue;
		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\systemc-2.3.1\\src"))) continue;
		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\pthread-win32-2.9.0"))) continue;

		// check for illegal format include path
		char const * pStr = valueList[i].c_str();
		size_t strLen = strlen(pStr);
		bool found = false;
		for (size_t j = 0; j < strLen; j += 1) {
			if (j > 0 && pStr[j] == '.' && pStr[j + 1] == '.' && pStr[j - 1] != '/' && pStr[j - 1] != '\\' && pStr[j - 1] != ';') {
				found = true;
				break;
			}
		}

		if (found) continue;

		// found extra string
		AddStringToList(m_idg[condIdx].m_additionalIncludeDirectories, valueList[i]);
	}
}

void CMsvsProject::ReadAdditionalLibraryDirectories(int condIdx)
{
	if (*m_pLine++ != '>') return;

	string valueStr;
	ReadPropertyValue(valueStr);

	vector<string> valueList;
	ParsePropertyValueStr(valueStr, valueList);

	for (size_t i = 0; i < valueList.size(); i += 1) {
		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\systemc-2.3.0\\win_x86_32\\Debug"))) continue;
		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\systemc-2.3.0\\win_x86_32\\Release"))) continue;
		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\systemc-2.3.0\\win_x86_64\\Debug"))) continue;
		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\systemc-2.3.0\\win_x86_64\\Release"))) continue;

		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\systemc-2.3.1\\win_x86_32\\Debug"))) continue;
		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\systemc-2.3.1\\win_x86_32\\Release"))) continue;
		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\systemc-2.3.1\\win_x86_64\\Debug"))) continue;
		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\systemc-2.3.1\\win_x86_64\\Release"))) continue;

		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\pthread-win32-2.9.0\\win_x86_32\\Debug"))) continue;
		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\pthread-win32-2.9.0\\win_x86_32\\Release"))) continue;
		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\pthread-win32-2.9.0\\win_x86_64\\Debug"))) continue;
		if (PathCmp(valueList[i], string(HT_TOOLS_PATH_MACRO) + string("\\pthread-win32-2.9.0\\win_x86_64\\Release"))) continue;

		// found extra string
		AddStringToList(m_idg[condIdx].m_additionalLibraryDirectories, valueList[i]);
	}
}

void CMsvsProject::ReadAdditionalDependencies(int condIdx)
{
	if (*m_pLine++ != '>') return;

	string valueStr;
	ReadPropertyValue(valueStr);

	vector<string> valueList;
	ParsePropertyValueStr(valueStr, valueList);

	for (size_t i = 0; i < valueList.size(); i += 1) {
		if (valueList[i] == "pthread.lib") continue;
		if (valueList[i] == "systemc.lib") continue;
		//if (valueList[i] == "kernel32.lib") continue;
		//if (valueList[i] == "user32.lib") continue;
		//if (valueList[i] == "gdi32.lib") continue;
		//if (valueList[i] == "winspool.lib") continue;
		//if (valueList[i] == "comdlg32.lib") continue;
		//if (valueList[i] == "advapi32.lib") continue;
		//if (valueList[i] == "shell32.lib") continue;
		//if (valueList[i] == "ole32.lib") continue;
		//if (valueList[i] == "oleaut32.lib") continue;
		//if (valueList[i] == "uuid.lib") continue;
		//if (valueList[i] == "odbc32.lib") continue;
		//if (valueList[i] == "odbccp32.lib") continue;
		if (valueList[i] == "%(AdditionalDependencies)") continue;

		// found extra string
		AddStringToList(m_idg[condIdx].m_additionalDependencies, valueList[i]);
	}
}

void CMsvsProject::ReadPropertyValue(string &valueStr)
{
	valueStr = "";
	const char * pStart = m_pLine;
	while (*m_pLine != '<') {
		if (*m_pLine == '\0' || *m_pLine == '\n' || *m_pLine == '\r') {
			valueStr += string(pStart, m_pLine - pStart);
			if (m_bEof || fgets(m_line, 4096, m_fp) == 0) {
				m_bEof = true;
				return;
			}
			m_pLine = m_line;
			// skip spaces
			while (*m_pLine == ' ' || *m_pLine == '\t')
				m_pLine += 1;
			pStart = m_pLine;
			continue;
		}
		m_pLine += 1;
	}

	if (*m_pLine != '<')
		return;

	valueStr = string(pStart, m_pLine - pStart);
}

void CMsvsProject::ParsePropertyValueStr(string &valueStr, vector<string> &valueList)
{
	// parse value string into semi-colon separated strings
	const char *pStr = valueStr.c_str();
	for (;;) {
		const char *pStart = pStr;
		while (*pStr == ' ' || *pStr == '\t')
			pStr += 1;
		while (*pStr != ';' && *pStr != '\0')
			pStr += 1;

		if (pStr != pStart) {
			string value = string(pStart, pStr - pStart);
			AddStringToList(valueList, value);
		}

		if (*pStr == '\0')
			break;

		pStr += 1;
	}
}

void CMsvsProject::ParseCommandValueStr(string &valueStr, vector<string> &valueList)
{
	// parse value string into semi-colon separated strings
	const char *pStr = valueStr.c_str();
	for (;;) {
		const char *pStart = pStr;
		while (*pStr == ' ' || *pStr == '\t')
			pStr += 1;
		while (*pStr != ' ' && *pStr != '\0')
			pStr += 1;

		if (pStr != pStart) {
			string value = string(pStart, pStr - pStart);
			AddStringToList(valueList, value);
		}

		if (*pStr == '\0')
			break;

		pStr += 1;
	}
}

void CMsvsProject::GenMsvsProjectHeader(FILE *fp)
{
	fprintf(fp, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
	fprintf(fp, "<Project DefaultTargets=\"Build\" ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n");
	fprintf(fp, "  <ItemGroup Label=\"ProjectConfigurations\">\n");
	fprintf(fp, "    <ProjectConfiguration Include=\"Debug|Win32\">\n");
	fprintf(fp, "      <Configuration>Debug</Configuration>\n");
	fprintf(fp, "      <Platform>Win32</Platform>\n");
	fprintf(fp, "    </ProjectConfiguration>\n");
	fprintf(fp, "    <ProjectConfiguration Include=\"Debug|x64\">\n");
	fprintf(fp, "      <Configuration>Debug</Configuration>\n");
	fprintf(fp, "      <Platform>x64</Platform>\n");
	fprintf(fp, "    </ProjectConfiguration>\n");
	fprintf(fp, "    <ProjectConfiguration Include=\"Release|Win32\">\n");
	fprintf(fp, "      <Configuration>Release</Configuration>\n");
	fprintf(fp, "      <Platform>Win32</Platform>\n");
	fprintf(fp, "    </ProjectConfiguration>\n");
	fprintf(fp, "    <ProjectConfiguration Include=\"Release|x64\">\n");
	fprintf(fp, "      <Configuration>Release</Configuration>\n");
	fprintf(fp, "      <Platform>x64</Platform>\n");
	fprintf(fp, "    </ProjectConfiguration>\n");
	fprintf(fp, "  </ItemGroup>\n");
	fprintf(fp, "  <PropertyGroup Label=\"Globals\">\n");
	fprintf(fp, "    <ProjectGuid>%s</ProjectGuid>\n", GenGuid().c_str());
	fprintf(fp, "    <Keyword>Win32Proj</Keyword>\n");
	fprintf(fp, "    <RootNamespace>dsn2_new</RootNamespace>\n");
	fprintf(fp, "  </PropertyGroup>\n");
	fprintf(fp, "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n");
	fprintf(fp, "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\" Label=\"Configuration\">\n");

	fprintf(fp, "      <ConfigurationType>");
	for (size_t i = 0; i < m_idg[0].m_configurationType.size(); i += 1)
		fprintf(fp, "%s%s", (i == 0) ? "" : ";", m_idg[0].m_configurationType[i].c_str());
	fprintf(fp, "</ConfigurationType>\n");

	fprintf(fp, "    <UseDebugLibraries>true</UseDebugLibraries>\n");
	fprintf(fp, "    <CharacterSet>Unicode</CharacterSet>\n");
	fprintf(fp, "    <PlatformToolset>v120</PlatformToolset>\n");
	fprintf(fp, "  </PropertyGroup>\n");
	fprintf(fp, "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\" Label=\"Configuration\">\n");

	fprintf(fp, "      <ConfigurationType>");
	for (size_t i = 0; i < m_idg[1].m_configurationType.size(); i += 1)
		fprintf(fp, "%s%s", (i == 0) ? "" : ";", m_idg[1].m_configurationType[i].c_str());
	fprintf(fp, "</ConfigurationType>\n");

	fprintf(fp, "    <UseDebugLibraries>true</UseDebugLibraries>\n");
	fprintf(fp, "    <CharacterSet>Unicode</CharacterSet>\n");
	fprintf(fp, "    <PlatformToolset>v120</PlatformToolset>\n");
	fprintf(fp, "  </PropertyGroup>\n");
	fprintf(fp, "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\" Label=\"Configuration\">\n");

	fprintf(fp, "      <ConfigurationType>");
	for (size_t i = 0; i < m_idg[2].m_configurationType.size(); i += 1)
		fprintf(fp, "%s%s", (i == 0) ? "" : ";", m_idg[2].m_configurationType[i].c_str());
	fprintf(fp, "</ConfigurationType>\n");

	fprintf(fp, "    <UseDebugLibraries>false</UseDebugLibraries>\n");
	fprintf(fp, "    <WholeProgramOptimization>true</WholeProgramOptimization>\n");
	fprintf(fp, "    <CharacterSet>Unicode</CharacterSet>\n");
	fprintf(fp, "    <PlatformToolset>v120</PlatformToolset>\n");
	fprintf(fp, "  </PropertyGroup>\n");
	fprintf(fp, "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\" Label=\"Configuration\">\n");

	fprintf(fp, "      <ConfigurationType>");
	for (size_t i = 0; i < m_idg[3].m_configurationType.size(); i += 1)
		fprintf(fp, "%s%s", (i == 0) ? "" : ";", m_idg[3].m_configurationType[i].c_str());
	fprintf(fp, "</ConfigurationType>\n");

	fprintf(fp, "    <UseDebugLibraries>false</UseDebugLibraries>\n");
	fprintf(fp, "    <WholeProgramOptimization>true</WholeProgramOptimization>\n");
	fprintf(fp, "    <CharacterSet>Unicode</CharacterSet>\n");
	fprintf(fp, "    <PlatformToolset>v120</PlatformToolset>\n");
	fprintf(fp, "  </PropertyGroup>\n");
	fprintf(fp, "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n");
	fprintf(fp, "  <ImportGroup Label=\"ExtensionSettings\">\n");
	fprintf(fp, "  </ImportGroup>\n");
	fprintf(fp, "  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">\n");
	fprintf(fp, "    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n");
	fprintf(fp, "  </ImportGroup>\n");
	fprintf(fp, "  <ImportGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\" Label=\"PropertySheets\">\n");
	fprintf(fp, "    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n");
	fprintf(fp, "  </ImportGroup>\n");
	fprintf(fp, "  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">\n");
	fprintf(fp, "    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n");
	fprintf(fp, "  </ImportGroup>\n");
	fprintf(fp, "  <ImportGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\" Label=\"PropertySheets\">\n");
	fprintf(fp, "    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n");
	fprintf(fp, "  </ImportGroup>\n");
	fprintf(fp, "  <PropertyGroup Label=\"UserMacros\" />\n");
	fprintf(fp, "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">\n");
	fprintf(fp, "    <LinkIncremental>true</LinkIncremental>\n");
	fprintf(fp, "  </PropertyGroup>\n");
	fprintf(fp, "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">\n");
	fprintf(fp, "    <LinkIncremental>true</LinkIncremental>\n");
	fprintf(fp, "  </PropertyGroup>\n");
	fprintf(fp, "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">\n");
	fprintf(fp, "    <LinkIncremental>false</LinkIncremental>\n");
	fprintf(fp, "  </PropertyGroup>\n");
	fprintf(fp, "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">\n");
	fprintf(fp, "    <LinkIncremental>false</LinkIncremental>\n");
	fprintf(fp, "  </PropertyGroup>\n");
	fprintf(fp, "  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">\n");
	fprintf(fp, "    <ClCompile>\n");
	fprintf(fp, "      <PrecompiledHeader>\n");
	fprintf(fp, "      </PrecompiledHeader>\n");
	fprintf(fp, "      <WarningLevel>Level3</WarningLevel>\n");
	fprintf(fp, "      <Optimization>Disabled</Optimization>\n");

	fprintf(fp, "      <PreprocessorDefinitions>");
	for (size_t i = 0; i < m_idg[0].m_preprocessorDefintions.size(); i += 1)
		fprintf(fp, "%s;", m_idg[0].m_preprocessorDefintions[i].c_str());
	fprintf(fp, "WIN32;_DEBUG;_CONSOLE;%%(PreprocessorDefinitions)</PreprocessorDefinitions>\n");

	fprintf(fp, "      <AdditionalIncludeDirectories>");
	for (size_t i = 0; i < m_idg[0].m_additionalIncludeDirectories.size(); i += 1)
		fprintf(fp, "%s;", m_idg[0].m_additionalIncludeDirectories[i].c_str());
	fprintf(fp, "../ht/sysc;../src;../src_pers;$(%s)/ht_lib;$(%s)/ht_lib/sysc;$(%s)/systemc-2.3.1/src;$(%s)/pthread-win32-2.9.0</AdditionalIncludeDirectories>\n", 
		HT_TOOLS_PATH, HT_TOOLS_PATH, HT_TOOLS_PATH, HT_TOOLS_PATH);

	fprintf(fp, "      <MultiProcessorCompilation>true</MultiProcessorCompilation>\n");
	fprintf(fp, "      <RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>\n");
	fprintf(fp, "      <MinimalRebuild>false</MinimalRebuild>\n");

	fprintf(fp, "      <ForcedIncludeFiles>");
	for (size_t i = 0; i < m_idg[0].m_forcedIncludeFiles.size(); i += 1)
		fprintf(fp, "%s%s", (i == 0) ? "" : ";", m_idg[0].m_forcedIncludeFiles[i].c_str());
	fprintf(fp, "</ForcedIncludeFiles>\n");

	fprintf(fp, "      <DisableSpecificWarnings>");
	for (size_t i = 0; i < m_idg[0].m_disableSpecificWarnings.size(); i += 1)
		fprintf(fp, "%s%s", (i == 0) ? "" : ";", m_idg[0].m_disableSpecificWarnings[i].c_str());
	fprintf(fp, "</DisableSpecificWarnings>\n");

	fprintf(fp, "      <AdditionalOptions>/bigobj</AdditionalOptions>\n");

	fprintf(fp, "    </ClCompile>\n");
	fprintf(fp, "    <Link>\n");
	fprintf(fp, "      <SubSystem>Console</SubSystem>\n");
	fprintf(fp, "      <GenerateDebugInformation>true</GenerateDebugInformation>\n");

	fprintf(fp, "      <AdditionalLibraryDirectories>");
	for (size_t i = 0; i < m_idg[0].m_additionalLibraryDirectories.size(); i += 1)
		fprintf(fp, "%s;", m_idg[0].m_additionalLibraryDirectories[i].c_str());
	fprintf(fp, "$(%s)/tests/msvs12/win32/Debug;$(%s)/systemc-2.3.1/win_x86_32/Debug;$(%s)/pthread-win32-2.9.0/win_x86_32/Debug</AdditionalLibraryDirectories>\n",
		HT_TOOLS_PATH, HT_TOOLS_PATH, HT_TOOLS_PATH);

	fprintf(fp, "      <AdditionalDependencies>");
	for (size_t i = 0; i < m_idg[0].m_additionalDependencies.size(); i += 1)
		fprintf(fp, "%s;", m_idg[0].m_additionalDependencies[i].c_str());
	fprintf(fp, "pthread.lib;systemc.lib;%%(AdditionalDependencies)</AdditionalDependencies>\n");

	fprintf(fp, "    </Link>\n");
	fprintf(fp, "  </ItemDefinitionGroup>\n");
	fprintf(fp, "  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">\n");
	fprintf(fp, "    <ClCompile>\n");
	fprintf(fp, "      <PrecompiledHeader>\n");
	fprintf(fp, "      </PrecompiledHeader>\n");
	fprintf(fp, "      <WarningLevel>Level3</WarningLevel>\n");
	fprintf(fp, "      <Optimization>Disabled</Optimization>\n");

	fprintf(fp, "      <PreprocessorDefinitions>");
	for (size_t i = 0; i < m_idg[1].m_preprocessorDefintions.size(); i += 1)
		fprintf(fp, "%s;", m_idg[1].m_preprocessorDefintions[i].c_str());
	fprintf(fp, "WIN32;_DEBUG;_CONSOLE;%%(PreprocessorDefinitions)</PreprocessorDefinitions>\n");

	fprintf(fp, "      <AdditionalIncludeDirectories>");
	for (size_t i = 0; i < m_idg[1].m_additionalIncludeDirectories.size(); i += 1)
		fprintf(fp, "%s;", m_idg[1].m_additionalIncludeDirectories[i].c_str());
	fprintf(fp, "..\\ht\\sysc;..\\src;..\\src_pers;$(%s)\\ht_lib;$(%s)\\ht_lib\\sysc;$(%s)/systemc-2.3.1\\src;$(%s)/pthread-win32-2.9.0</AdditionalIncludeDirectories>\n", 
		HT_TOOLS_PATH, HT_TOOLS_PATH, HT_TOOLS_PATH, HT_TOOLS_PATH);

	fprintf(fp, "      <RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>\n");
	fprintf(fp, "      <MultiProcessorCompilation>true</MultiProcessorCompilation>\n");
	fprintf(fp, "      <MinimalRebuild>false</MinimalRebuild>\n");

	fprintf(fp, "      <ForcedIncludeFiles>");
	for (size_t i = 0; i < m_idg[1].m_forcedIncludeFiles.size(); i += 1)
		fprintf(fp, "%s%s", (i == 0) ? "" : ";", m_idg[1].m_forcedIncludeFiles[i].c_str());
	fprintf(fp, "</ForcedIncludeFiles>\n");

	fprintf(fp, "      <DisableSpecificWarnings>");
	for (size_t i = 0; i < m_idg[1].m_disableSpecificWarnings.size(); i += 1)
		fprintf(fp, "%s%s", (i == 0) ? "" : ";", m_idg[1].m_disableSpecificWarnings[i].c_str());
	fprintf(fp, "</DisableSpecificWarnings>\n");

	fprintf(fp, "      <AdditionalOptions>/bigobj</AdditionalOptions>\n");

	fprintf(fp, "    </ClCompile>\n");
	fprintf(fp, "    <Link>\n");
	fprintf(fp, "      <SubSystem>Console</SubSystem>\n");
	fprintf(fp, "      <GenerateDebugInformation>true</GenerateDebugInformation>\n");

	fprintf(fp, "      <AdditionalLibraryDirectories>");
	for (size_t i = 0; i < m_idg[1].m_additionalLibraryDirectories.size(); i += 1)
		fprintf(fp, "%s;", m_idg[1].m_additionalLibraryDirectories[i].c_str());
	fprintf(fp, "$(%s)/tests/msvs12/x64/Debug;$(%s)/systemc-2.3.1/win_x86_64/Debug;$(%s)/pthread-win32-2.9.0/win_x86_64/Debug</AdditionalLibraryDirectories>\n",
		HT_TOOLS_PATH, HT_TOOLS_PATH, HT_TOOLS_PATH);

	fprintf(fp, "      <AdditionalDependencies>");
	for (size_t i = 0; i < m_idg[1].m_additionalDependencies.size(); i += 1)
		fprintf(fp, "%s;", m_idg[1].m_additionalDependencies[i].c_str());
	fprintf(fp, "pthread.lib;systemc.lib;%%(AdditionalDependencies)</AdditionalDependencies>\n");

	fprintf(fp, "    </Link>\n");
	fprintf(fp, "  </ItemDefinitionGroup>\n");
	fprintf(fp, "  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">\n");
	fprintf(fp, "    <ClCompile>\n");
	fprintf(fp, "      <WarningLevel>Level3</WarningLevel>\n");
	fprintf(fp, "      <PrecompiledHeader>\n");
	fprintf(fp, "      </PrecompiledHeader>\n");
	fprintf(fp, "      <Optimization>MaxSpeed</Optimization>\n");
	fprintf(fp, "      <FunctionLevelLinking>true</FunctionLevelLinking>\n");
	fprintf(fp, "      <IntrinsicFunctions>true</IntrinsicFunctions>\n");

	fprintf(fp, "      <PreprocessorDefinitions>");
	for (size_t i = 0; i < m_idg[2].m_preprocessorDefintions.size(); i += 1)
		fprintf(fp, "%s;", m_idg[2].m_preprocessorDefintions[i].c_str());
	fprintf(fp, "WIN32;_CONSOLE;%%(PreprocessorDefinitions)</PreprocessorDefinitions>\n");

	fprintf(fp, "      <AdditionalIncludeDirectories>");
	for (size_t i = 0; i < m_idg[2].m_additionalIncludeDirectories.size(); i += 1)
		fprintf(fp, "%s;", m_idg[2].m_additionalIncludeDirectories[i].c_str());
	fprintf(fp, "..\\ht\\sysc;..\\src;..\\src_pers;$(%s)\\ht_lib;$(%s)\\ht_lib\\sysc;$(%s)\\systemc-2.3.1\\src;$(%s)\\pthread-win32-2.9.0</AdditionalIncludeDirectories>\n",
		HT_TOOLS_PATH, HT_TOOLS_PATH, HT_TOOLS_PATH, HT_TOOLS_PATH);

	fprintf(fp, "      <RuntimeLibrary>MultiThreaded</RuntimeLibrary>\n");
	fprintf(fp, "      <MultiProcessorCompilation>true</MultiProcessorCompilation>\n");
	fprintf(fp, "      <MinimalRebuild>false</MinimalRebuild>\n");

	fprintf(fp, "      <ForcedIncludeFiles>");
	for (size_t i = 0; i < m_idg[2].m_forcedIncludeFiles.size(); i += 1)
		fprintf(fp, "%s%s", (i == 0) ? "" : ";", m_idg[2].m_forcedIncludeFiles[i].c_str());
	fprintf(fp, "</ForcedIncludeFiles>\n");

	fprintf(fp, "      <DisableSpecificWarnings>");
	for (size_t i = 0; i < m_idg[2].m_disableSpecificWarnings.size(); i += 1)
		fprintf(fp, "%s%s", (i == 0) ? "" : ";", m_idg[2].m_disableSpecificWarnings[i].c_str());
	fprintf(fp, "</DisableSpecificWarnings>\n");

	fprintf(fp, "      <AdditionalOptions>/bigobj</AdditionalOptions>\n");

	fprintf(fp, "    </ClCompile>\n");
	fprintf(fp, "    <Link>\n");
	fprintf(fp, "      <SubSystem>Console</SubSystem>\n");
	fprintf(fp, "      <GenerateDebugInformation>true</GenerateDebugInformation>\n");
	fprintf(fp, "      <EnableCOMDATFolding>true</EnableCOMDATFolding>\n");
	fprintf(fp, "      <OptimizeReferences>true</OptimizeReferences>\n");

	fprintf(fp, "      <AdditionalLibraryDirectories>");
	for (size_t i = 0; i < m_idg[2].m_additionalLibraryDirectories.size(); i += 1)
		fprintf(fp, "%s;", m_idg[2].m_additionalLibraryDirectories[i].c_str());
	fprintf(fp, "$(%s)/tests/msvs12/win32/Release;$(%s)/systemc-2.3.1/win_x86_32/Release;$(%s)/pthread-win32-2.9.0/win_x86_32/Release</AdditionalLibraryDirectories>\n", 
		HT_TOOLS_PATH, HT_TOOLS_PATH, HT_TOOLS_PATH);

	fprintf(fp, "      <AdditionalDependencies>");
	for (size_t i = 0; i < m_idg[2].m_additionalDependencies.size(); i += 1)
		fprintf(fp, "%s;", m_idg[2].m_additionalDependencies[i].c_str());
	fprintf(fp, "pthread.lib;systemc.lib;%%(AdditionalDependencies)</AdditionalDependencies>\n");

	fprintf(fp, "    </Link>\n");
	fprintf(fp, "  </ItemDefinitionGroup>\n");
	fprintf(fp, "  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">\n");
	fprintf(fp, "    <ClCompile>\n");
	fprintf(fp, "      <WarningLevel>Level3</WarningLevel>\n");
	fprintf(fp, "      <PrecompiledHeader>\n");
	fprintf(fp, "      </PrecompiledHeader>\n");
	fprintf(fp, "      <Optimization>MaxSpeed</Optimization>\n");
	fprintf(fp, "      <FunctionLevelLinking>true</FunctionLevelLinking>\n");
	fprintf(fp, "      <IntrinsicFunctions>true</IntrinsicFunctions>\n");

	fprintf(fp, "      <PreprocessorDefinitions>");
	for (size_t i = 0; i < m_idg[3].m_preprocessorDefintions.size(); i += 1)
		fprintf(fp, "%s;", m_idg[3].m_preprocessorDefintions[i].c_str());
	fprintf(fp, "WIN32;_CONSOLE;%%(PreprocessorDefinitions)</PreprocessorDefinitions>\n");

	fprintf(fp, "      <AdditionalIncludeDirectories>");
	for (size_t i = 0; i < m_idg[3].m_additionalIncludeDirectories.size(); i += 1)
		fprintf(fp, "%s;", m_idg[3].m_additionalIncludeDirectories[i].c_str());
	fprintf(fp, "..\\ht\\sysc;..\\src;..\\src_pers;$(%s)\\ht_lib;$(%s)\\ht_lib\\sysc;$(%s)/systemc-2.3.1\\src;$(%s)/pthread-win32-2.9.0</AdditionalIncludeDirectories>\n", 
		HT_TOOLS_PATH, HT_TOOLS_PATH, HT_TOOLS_PATH, HT_TOOLS_PATH);

	fprintf(fp, "      <RuntimeLibrary>MultiThreaded</RuntimeLibrary>\n");
	fprintf(fp, "      <MultiProcessorCompilation>true</MultiProcessorCompilation>\n");
	fprintf(fp, "      <MinimalRebuild>false</MinimalRebuild>\n");

	fprintf(fp, "      <ForcedIncludeFiles>");
	for (size_t i = 0; i < m_idg[3].m_forcedIncludeFiles.size(); i += 1)
		fprintf(fp, "%s%s", (i == 0) ? "" : ";", m_idg[3].m_forcedIncludeFiles[i].c_str());
	fprintf(fp, "</ForcedIncludeFiles>\n");

	fprintf(fp, "      <DisableSpecificWarnings>");
	for (size_t i = 0; i < m_idg[3].m_disableSpecificWarnings.size(); i += 1)
		fprintf(fp, "%s%s", (i == 0) ? "" : ";", m_idg[3].m_disableSpecificWarnings[i].c_str());
	fprintf(fp, "</DisableSpecificWarnings>\n");

	fprintf(fp, "      <AdditionalOptions>/bigobj</AdditionalOptions>\n");

	fprintf(fp, "    </ClCompile>\n");
	fprintf(fp, "    <Link>\n");
	fprintf(fp, "      <SubSystem>Console</SubSystem>\n");
	fprintf(fp, "      <GenerateDebugInformation>true</GenerateDebugInformation>\n");
	fprintf(fp, "      <EnableCOMDATFolding>true</EnableCOMDATFolding>\n");
	fprintf(fp, "      <OptimizeReferences>true</OptimizeReferences>\n");

	fprintf(fp, "      <AdditionalLibraryDirectories>");
	for (size_t i = 0; i < m_idg[3].m_additionalLibraryDirectories.size(); i += 1)
		fprintf(fp, "%s;", m_idg[3].m_additionalLibraryDirectories[i].c_str());
	fprintf(fp, "$(%s)/tests/msvs12/x64/Release;$(%s)/systemc-2.3.1/win_x86_64/Release;$(%s)/pthread-win32-2.9.0/win_x86_64/Release</AdditionalLibraryDirectories>\n", 
		HT_TOOLS_PATH, HT_TOOLS_PATH, HT_TOOLS_PATH);

	fprintf(fp, "      <AdditionalDependencies>");
	for (size_t i = 0; i < m_idg[3].m_additionalDependencies.size(); i += 1)
		fprintf(fp, "%s;", m_idg[3].m_additionalDependencies[i].c_str());
	fprintf(fp, "pthread.lib;systemc.lib;%%(AdditionalDependencies)</AdditionalDependencies>\n");

	fprintf(fp, "    </Link>\n");
	fprintf(fp, "  </ItemDefinitionGroup>\n");
}


void CMsvsProject::GenMsvsProjectFiles(FILE *fp, EMsvsFile eFileType)
{
	char * pFileType = 0;
	switch (eFileType) {
	case Include: pFileType = "ClInclude"; break;
	case Compile: pFileType = "ClCompile"; break;
	case Custom:  pFileType = "CustomBuild"; break;
	case CustomHtLib:  pFileType = "CustomBuild"; break;
	case NonBuild: pFileType = "None"; break;
	default: HtlAssert(0);
	}

	fprintf(fp, "  <ItemGroup>\n");

	for (size_t fileIdx = 0; fileIdx < m_fileList.size(); fileIdx += 1) {
		CMsvsFile &file = m_fileList[fileIdx];

		if (file.m_fileType != eFileType) continue;

		bool bSrcModel = file.m_filterName == "src_model";
		bool bSrcPers = file.m_filterName == "src_pers";

		if (eFileType == Custom || eFileType == CustomHtLib) {
			fprintf(fp, "    <%s Include=\"%s\">\n", pFileType, file.m_pathName.c_str());

			fprintf(fp, "      <FileType>Document</FileType>\n");

			char const * pDir = eFileType == CustomHtLib ? HT_TOOLS_PATH_MACRO "\\ht_lib" : "..\\ht";

			if (file.m_pathName.substr(file.m_pathName.size()-3) == ".sc") {
				fprintf(fp, "      <Command Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">$(%s)\\bin\\htv -method"
					" -I ..\\src -I ..\\src_pers -I ..\\ht\\sysc -I $(%s)\\ht_lib -I $(%s)\\ht_lib\\sysc -DHT_SYSC %s\\sysc\\%%(Filename).sc"
					" ..\\ht\\sysc\\%%(Filename).h ..\\ht\\verilog\\%%(Filename).v</Command>\n",
					HT_TOOLS_PATH, HT_TOOLS_PATH, HT_TOOLS_PATH, pDir);
				fprintf(fp, "      <Command Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">$(%s)\\bin\\htv -method"
					" -I ..\\src -I ..\\src_pers -I ..\\ht\\sysc -I $(%s)\\ht_lib -I $(%s)\\ht_lib\\sysc -DHT_SYSC %s\\sysc\\%%(Filename).sc"
					" ..\\ht\\sysc\\%%(Filename).h ..\\ht\\verilog\\%%(Filename).v</Command>\n",
					HT_TOOLS_PATH, HT_TOOLS_PATH, HT_TOOLS_PATH, pDir);
				fprintf(fp, "      <Command Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">$(%s)\\bin\\htv -method"
					" -I ..\\src -I ..\\src_pers -I ..\\ht\\sysc -I $(%s)\\ht_lib -I $(%s)\\ht_lib\\sysc -DHT_SYSC %s\\sysc\\%%(Filename).sc"
					" ..\\ht\\sysc\\%%(Filename).h ..\\ht\\verilog\\%%(Filename).v</Command>\n",
					HT_TOOLS_PATH, HT_TOOLS_PATH, HT_TOOLS_PATH, pDir);
				fprintf(fp, "      <Command Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">$(%s)\\bin\\htv -method"
					" -I ..\\src -I ..\\src_pers -I ..\\ht\\sysc -I $(%s)\\ht_lib -I $(%s)\\ht_lib\\sysc -DHT_SYSC %s\\sysc\\%%(Filename).sc"
					" ..\\ht\\sysc\\%%(Filename).h ..\\ht\\verilog\\%%(Filename).v</Command>\n",
					HT_TOOLS_PATH, HT_TOOLS_PATH, HT_TOOLS_PATH, pDir);

				fprintf(fp, "      <Message Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">htv %%(Filename) ...</Message>\n");
				fprintf(fp, "      <Message Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">htv %%(Filename) ...</Message>\n");
				fprintf(fp, "      <Message Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">htv %%(Filename) ...</Message>\n");
				fprintf(fp, "      <Message Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">htv %%(Filename) ...</Message>\n");

				fprintf(fp, "      <Outputs Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">..\\ht\\sysc\\%%(Filename).h</Outputs>\n");
				fprintf(fp, "      <Outputs Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">..\\ht\\sysc\\%%(Filename).h</Outputs>\n");
				fprintf(fp, "      <Outputs Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">..\\ht\\sysc\\%%(Filename).h</Outputs>\n");
				fprintf(fp, "      <Outputs Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">..\\ht\\sysc\\%%(Filename).h</Outputs>\n");

				if (file.m_dependencies.size() > 0) {
					fprintf(fp, "      <AdditionalInputs Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">%s</AdditionalInputs>\n", file.m_dependencies.c_str());
					fprintf(fp, "      <AdditionalInputs Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">%s</AdditionalInputs>\n", file.m_dependencies.c_str());
					fprintf(fp, "      <AdditionalInputs Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">%s</AdditionalInputs>\n", file.m_dependencies.c_str());
					fprintf(fp, "      <AdditionalInputs Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">%s</AdditionalInputs>\n", file.m_dependencies.c_str());
				}

			} else if (file.m_pathName.substr(file.m_pathName.size()-4) == ".htl") {
				for (int i = 0; i < 4; i += 1) {
					string condName;
					switch (i) {
					case CD32: condName = "Debug|Win32"; break;
					case CD64: condName = "Debug|x64"; break;
					case CR32: condName = "Release|Win32"; break;
					case CR64: condName = "Release|x64"; break;
					}
					if (bSrcModel)
						fprintf(fp, "      <ExcludedFromBuild Condition=\"'$(Configuration)|$(Platform)'=='%s'\">true</ExcludedFromBuild>\n", condName.c_str());
				}

				fprintf(fp, "      <Command Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">$(%s)\\bin\\htl -cl %s</Command>\n", HT_TOOLS_PATH, file.m_pathName.c_str());
				fprintf(fp, "      <Command Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">$(%s)\\bin\\htl -cl %s</Command>\n", HT_TOOLS_PATH, file.m_pathName.c_str());
				fprintf(fp, "      <Command Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">$(%s)\\bin\\htl -cl %s</Command>\n", HT_TOOLS_PATH, file.m_pathName.c_str());
				fprintf(fp, "      <Command Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">$(%s)\\bin\\htl -cl %s</Command>\n", HT_TOOLS_PATH, file.m_pathName.c_str());

				fprintf(fp, "      <Message Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">htl %%(Filename) ...</Message>\n");
				fprintf(fp, "      <Message Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">htl %%(Filename) ...</Message>\n");
				fprintf(fp, "      <Message Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">htl %%(Filename) ...</Message>\n");
				fprintf(fp, "      <Message Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">htl %%(Filename) ...</Message>\n");

				fprintf(fp, "      <Outputs Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">");
				fprintf(fp, "..\\ht\\sysc\\HostIntf.h");
				fprintf(fp, "</Outputs>\n");

				fprintf(fp, "      <Outputs Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">");
				char * pTab = "";
				for (size_t fileIdx = 0; fileIdx < m_fileList.size(); fileIdx += 1) {
					CMsvsFile &file = m_fileList[fileIdx];
					char lastCh = file.m_pathName[file.m_pathName.size()-1];
					if (file.m_filterName == "ht\\sysc" || file.m_filterName == "ht\\verilog" && lastCh == '_') {
						fprintf(fp, "%s%s", pTab, file.m_pathName.c_str());
						pTab = ";";
					}
				}
				fprintf(fp, "</Outputs>\n");

				fprintf(fp, "      <Outputs Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">..\\ht\\sysc\\HostIntf.h</Outputs>\n");
				fprintf(fp, "      <Outputs Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">..\\ht\\sysc\\HostIntf.h</Outputs>\n");
			
			} else if (file.m_pathName.substr(file.m_pathName.size()-3) == ".v_") {

				char const *pFx = file.m_bGenFx ? "-fx " : "";

				fprintf(fp, "      <Command Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">$(%s)\\bin\\htv -method", HT_TOOLS_PATH);
				for (size_t i = 0; i < file.m_extraDefines[0].size(); i += 1)
					fprintf(fp, " %s", file.m_extraDefines[0][i].c_str());
				fprintf(fp, " %s-DHT_SYSC -I ..\\src -I ..\\src_pers -I ..\\ht\\sysc -I $(%s)\\ht_lib -I $(%s)\\ht_lib\\sysc %s\\sysc\\%%(Filename).cpp"
					" ..\\ht\\verilog\\%%(Filename).v</Command>\n", pFx, HT_TOOLS_PATH, HT_TOOLS_PATH, pDir);
				fprintf(fp, "      <Command Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">$(%s)\\bin\\htv -method", HT_TOOLS_PATH);
				for (size_t i = 0; i < file.m_extraDefines[1].size(); i += 1)
					fprintf(fp, " %s", file.m_extraDefines[1][i].c_str());
				fprintf(fp, " %s-DHT_SYSC -I ..\\src -I ..\\src_pers -I ..\\ht\\sysc -I $(%s)\\ht_lib -I $(%s)\\ht_lib\\sysc %s\\sysc\\%%(Filename).cpp"
					" ..\\ht\\verilog\\%%(Filename).v</Command>\n", pFx, HT_TOOLS_PATH, HT_TOOLS_PATH, pDir);
				fprintf(fp, "      <Command Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">$(%s)\\bin\\htv -method", HT_TOOLS_PATH);
				for (size_t i = 0; i < file.m_extraDefines[2].size(); i += 1)
					fprintf(fp, " %s", file.m_extraDefines[2][i].c_str());
				fprintf(fp, " %s-DHT_SYSC -I ..\\src -I ..\\src_pers -I ..\\ht\\sysc -I $(%s)\\ht_lib -I $(%s)\\ht_lib\\sysc %s\\sysc\\%%(Filename).cpp"
					" ..\\ht\\verilog\\%%(Filename).v</Command>\n", pFx, HT_TOOLS_PATH, HT_TOOLS_PATH, pDir);
				fprintf(fp, "      <Command Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">$(%s)\\bin\\htv -method", HT_TOOLS_PATH);
				for (size_t i = 0; i < file.m_extraDefines[3].size(); i += 1)
					fprintf(fp, " %s", file.m_extraDefines[3][i].c_str());
				fprintf(fp, " %s-DHT_SYSC -I ..\\src -I ..\\src_pers -I ..\\ht\\sysc -I $(%s)\\ht_lib -I $(%s)\\ht_lib\\sysc %s\\sysc\\%%(Filename).cpp"
					" ..\\ht\\verilog\\%%(Filename).v</Command>\n", pFx, HT_TOOLS_PATH, HT_TOOLS_PATH, pDir);

				fprintf(fp, "      <Message Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">htv %%(Filename) ...</Message>\n");
				fprintf(fp, "      <Message Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">htv %%(Filename) ...</Message>\n");
				fprintf(fp, "      <Message Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">htv %%(Filename) ...</Message>\n");
				fprintf(fp, "      <Message Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">htv %%(Filename) ...</Message>\n");

				fprintf(fp, "      <Outputs Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">..\\ht\\verilog\\%%(Filename).v</Outputs>\n");
				fprintf(fp, "      <Outputs Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">..\\ht\\verilog\\%%(Filename).v</Outputs>\n");
				fprintf(fp, "      <Outputs Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">..\\ht\\verilog\\%%(Filename).v</Outputs>\n");
				fprintf(fp, "      <Outputs Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">..\\ht\\verilog\\%%(Filename).v</Outputs>\n");

				fprintf(fp, "      <AdditionalInputs Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">%s\\sysc\\%%(Filename).cpp;%s\\sysc\\%%(Filename).h</AdditionalInputs>\n", pDir, pDir);
				fprintf(fp, "      <AdditionalInputs Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">%s\\sysc\\%%(Filename).cpp;%s\\sysc\\%%(Filename).h</AdditionalInputs>\n", pDir, pDir);
				fprintf(fp, "      <AdditionalInputs Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">%s\\sysc\\%%(Filename).cpp;%s\\sysc\\%%(Filename).h</AdditionalInputs>\n", pDir, pDir);
				fprintf(fp, "      <AdditionalInputs Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">%s\\sysc\\%%(Filename).cpp;%s\\sysc\\%%(Filename).h</AdditionalInputs>\n", pDir, pDir);
			}

			fprintf(fp, "    </%s>\n", pFileType);

		} else {
			fprintf(fp, "    <%s Include=\"%s\">", pFileType, file.m_pathName.c_str());
			bool bProp = false;

			if (file.m_pathName.substr(file.m_pathName.size() - 4) == ".cpp" && (bSrcModel || bSrcPers)) {
				for (int i = 0; i < 4; i += 1) {
					string condName;
					switch (i) {
					case CD32: condName = "Debug|Win32"; break;
					case CD64: condName = "Debug|x64"; break;
					case CR32: condName = "Release|Win32"; break;
					case CR64: condName = "Release|x64"; break;
					}
					if (bSrcModel && !m_idg[i].m_bHtModel || bSrcPers && m_idg[i].m_bHtModel) {
						if (!bProp) {
							fprintf(fp, "\n");
							bProp = true;
						}
						fprintf(fp, "      <ExcludedFromBuild Condition=\"'$(Configuration)|$(Platform)'=='%s'\">true</ExcludedFromBuild>\n", condName.c_str());
					}
				}
			} else {
				if (!bProp && file.m_excludeFromBuild.size() > 0) {
					fprintf(fp, "\n");
					bProp = true;
				}
				for (size_t idx = 0; idx < file.m_excludeFromBuild.size(); idx += 1)
					fprintf(fp, "        <ExcludedFromBuild Condition=\"'$(Configuration)|$(Platform)'=='%s'\">true</ExcludedFromBuild>\n", file.m_excludeFromBuild[idx].c_str());
			}

			for (int i = 0; i < 4; i += 1) {
				string condName;
				switch (i) {
				case CD32: condName = "Debug|Win32"; break;
				case CD64: condName = "Debug|x64"; break;
				case CR32: condName = "Release|Win32"; break;
				case CR64: condName = "Release|x64"; break;
				}
				if (!bProp && file.m_forcedIncludeFiles[i].size() > 0) {
					fprintf(fp, "\n");
					bProp = true;
				}
				if (file.m_forcedIncludeFiles[i].size() > 0) {
					fprintf(fp, "        <ForcedIncludeFiles Condition=\"'$(Configuration)|$(Platform)'=='%s'\">", condName.c_str());
					for (size_t idx = 0; idx < file.m_forcedIncludeFiles[i].size(); idx += 1)
						fprintf(fp, "%s%s", idx > 0 ? ";" : "", file.m_forcedIncludeFiles[i][idx].c_str());
					fprintf(fp, "</ForcedIncludeFiles>\n");
				}
			}

			if (bProp)
				fprintf(fp, "    ");
			fprintf(fp, "</%s>\n", pFileType);
		}
	}
	fprintf(fp, "  </ItemGroup>\n");
}

void CMsvsProject::GenMsvsProjectTrailer(FILE *fp)
{
	fprintf(fp, "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n");
	fprintf(fp, "  <ImportGroup Label=\"ExtensionTargets\">\n");
	fprintf(fp, "  </ImportGroup>\n");
	fprintf(fp, "</Project>\n");
}

#endif
