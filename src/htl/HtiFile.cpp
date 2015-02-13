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
#include "AppArgs.h"
#include "DsnInfo.h"

void HtiFile::SkipTo(EToken skipTk) 
{
	EToken tk;
	do {
		tk = m_pLex->GetNextTk();
	} while (tk != skipTk && tk != eTkEof);
}


void HtiFile::loadHtiFile(string fileName)
{
	if (fileName.size() == 0)
		return;

	if (!m_pLex->LexOpen(fileName)) {
		printf("Could not open file '%s'\n", g_appArgs.GetInstanceFile().c_str());
		exit(1);
	}
 
    while (m_pLex->GetTk() != eTkEof) {

        if (m_pLex->GetTk() != eTkIdent) {
            CPreProcess::ParseMsg(Error, "Expected instance file command");
            SkipTo(eTkSemi);
            continue;
        }

		ParseHtiMethods();
    }
}

void HtiFile::ParseHtiMethods()
{
    if (false && m_pLex->GetTkString() == "AddUnitInst") {

		string AE;
		string AU;
        string unit;

        CParamList params[] = {
            { "AE",		&AE,	true,	eTkIntSet, 0, 0},
            { "AU",		&AU,	true,	eTkIntSet, 0, 0},
            { "unit",	&unit,	true,	eTkIdent,  0, 0},
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params)) {
            CPreProcess::ParseMsg(Error, "expected AddUnitInst( AE, AU, unit )");
			return;
		}

		vector<int> aeSet = ExpandIntSet(AE);
		vector<int> auSet = ExpandIntSet(AU);

		for (size_t aeSetIdx = 0; aeSetIdx < aeSet.size(); aeSetIdx += 1) {
			for (size_t auSetIdx = 0; auSetIdx < auSet.size(); auSetIdx += 1) {

				AddUnitInst( aeSet[aeSetIdx], auSet[auSetIdx], unit );
			}
		}

    } else if (false && m_pLex->GetTkString() == "AddUnitParams") {

        string unit;
		string entry;
		string memPortCnt;

        CParamList params[] = {
            { "unit",		&unit,			true,	eTkIdent,   0, 0},
            { "entry",		&entry,			true,	eTkIdent,   0, 0},
            { "memPortCnt",	&memPortCnt,	false,	eTkInteger, 0, 0},
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected AddUnitParams( unit, entry {, memPortCnt } )");

		else
			AddUnitParams( unit, entry, memPortCnt );

	} else if (m_pLex->GetTkString() == "AddModInstParams") {

        string unit;
		string modPath;
		vector<int> memPort;
		string instId;
		string replCnt;

        CParamList params[] = {
            { "unit",		&unit,		true,	eTkIdent,   0, 0},
            { "modPath",	&modPath,	true,	eTkIdent,   0, 0},
            { "memPort",	&memPort,	false,	eTkIntList, 0, 0},
			{ "replCnt",	&replCnt,	false,	eTkInteger,	0, 0},
            { "instId",		&instId,	false,	eTkInteger, 0, 0},
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected AddModInstParams( unit, modPath {, memPort } {, instId } )");

		else {
			if (modPath.size() > 0 && modPath[0] != '/')
				modPath = "/" + modPath;

			if (modPath.size() > 0 && modPath[modPath.size()-1] == ']' && instId.size() > 0)
				CPreProcess::ParseMsg(Error, "a replicated module instance can not specify an instId");

			AddModInstParams( unit, modPath, memPort, instId, replCnt );
		}

	} else if (m_pLex->GetTkString() == "AddMsgIntfConn") {

        string inUnit;
        string outUnit;
		string inPath;
		string outPath;
		bool aeNext = false;
		bool aePrev = false;

        CParamList params[] = {
            { "inUnit",		&inUnit,	false,	eTkParamStr, 0, 0},
            { "outUnit",	&outUnit,	false,	eTkParamStr, 0, 0},
            { "inPath",		&inPath,	true,	eTkParamStr, 0, 0},
			{ "outPath",	&outPath,	true,	eTkParamStr, 0, 0},
			{ "aeNext",		&aeNext,	false,  eTkBoolean, 0, 0 },
			{ "aePrev",		&aePrev,	false,  eTkBoolean, 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected AddMsgIntfConn( { outUnit, } outPath, { inUnit, } inPath {, aeNext=<false>}{, aePrev=<false>} )");

		else {
			if (outPath.size() > 0 && outPath[0] != '/')
				outPath = "/" + outPath;

			if (inPath.size() > 0 && inPath[0] != '/')
				inPath = "/" + inPath;

			AddMsgIntfConn( outUnit, outPath, inUnit, inPath, aeNext, aePrev );
		}

	} else
        CPreProcess::ParseMsg(Error, "Expected instance file command");

    if (m_pLex->GetTk() != eTkSemi) {
        CPreProcess::ParseMsg(Error, "expected a ';'");
        SkipTo(eTkSemi);
    }
	m_pLex->GetNextTk();
}

bool HtiFile::ParseParameters(CParamList *params)
{
    if (m_pLex->GetNextTk() != eTkLParen) {
        SkipTo(eTkRParen);
        return false;
    }

    bool bError = false;
    EToken tk;
    while ((tk = m_pLex->GetNextTk()) == eTkIdent) {
        int i;
        for (i = 0; params[i].m_pName; i += 1)
            if (m_pLex->GetTkString() == params[i].m_pName)
                break;

        if (params[i].m_pName == 0) {
            bError = true;
            CPreProcess::ParseMsg(Error, "unexpected parameter name '%s'", m_pLex->GetTkString().c_str());
            break;
        }

        if (params[i].m_bPresent) {
            bError = true;
            CPreProcess::ParseMsg(Error, "duplicate parameters '%s'", m_pLex->GetTkString().c_str());
            break;
        }

        params[i].m_bPresent = true;

        if (m_pLex->GetNextTk() != eTkEqual) {
            bError = true;
            CPreProcess::ParseMsg(Error, "expected '=' after parameter '%s'", params[i].m_pName);
            break;
        }

        bool bValue = true;

		if (params[i].m_token == eTkIntList) {

			vector<int> *pIntList = (vector<int> *)params[i].m_pValue;

	        tk = m_pLex->GetNextTk();

			if (tk == eTkLBrace) {
				tk = m_pLex->GetNextTk();

				for (;;) {
					if (!ParseIntRange(pIntList)) {
						CPreProcess::ParseMsg(Error, "expected an integer or integer range for parameter '%s'", params[i].m_pName);
						break;
					}

					if (m_pLex->GetTk() == eTkComma)
						m_pLex->GetNextTk();
					else
						break;
				}

				if (m_pLex->GetTk() != eTkRBrace)
					CPreProcess::ParseMsg(Error, "expected a '}' for parameter '%s'", params[i].m_pName);
				else
					m_pLex->GetNextTk();

			} else {
				if (!ParseIntRange(pIntList))
					CPreProcess::ParseMsg(Error, "expected an integer or integer range for parameter '%s'", params[i].m_pName);
			}
	        tk = m_pLex->GetTk();

        } else if (params[i].m_token == eTkIntRange) {
			tk = m_pLex->GetNextTk();
            if (tk != eTkInteger) {
                bError = true;
                CPreProcess::ParseMsg(Error, "expected an integer or integer range for parameter '%s'", params[i].m_pName);
                break;
            }
            ((string *)params[i].m_pValue)[0] = m_pLex->GetTkString();

            tk = m_pLex->GetNextTk();

            if (tk != eTkMinus)
                ((string *)params[i].m_pValue)[1] = ((string *)params[i].m_pValue)[0];

            else {
                if (m_pLex->GetNextTk() != eTkInteger) {
                    bError = true;
                    CPreProcess::ParseMsg(Error, "expected an integer or integer range for parameter '%s'", params[i].m_pName);
                    break;
                }
                ((string *)params[i].m_pValue)[1] = m_pLex->GetTkString();

                tk = m_pLex->GetNextTk();
            }

        } else if (params[i].m_token == eTkBoolean) {
			tk = m_pLex->GetNextTk();
            if (tk != eTkIdent) {
                bError = true;
                CPreProcess::ParseMsg(Error, "expected true or false for parameter '%s'", params[i].m_pName);
                break;
            } else {
                if (m_pLex->GetTkString() == "true")
                    bValue = true;
                else if (m_pLex->GetTkString() == "false")
                    bValue = false;
                else
                    CPreProcess::ParseMsg(Error, "expected true or false for parameter '%s'", params[i].m_pName);

                *(bool *)params[i].m_pValue = bValue;
            }

            tk = m_pLex->GetNextTk();

        } else if (params[i].m_token == eTkParamStr) {

			*(string *)params[i].m_pValue = m_pLex->GetExprStr(',');
			tk = m_pLex->GetNextTk();

       } else {
			tk = m_pLex->GetNextTk();
			if (tk == eTkUnknown) {
				bError = true;
				CPreProcess::ParseMsg(Error, "unknown expression for '%s'", params[i].m_pName);
				break;
			} else if (params[i].m_token == eTkIdent && tk != eTkIdent && tk != eTkString) {
				bError = true;
				CPreProcess::ParseMsg(Error, "expected an identifier for parameter '%s'", params[i].m_pName);
				break;
			} else if (params[i].m_token == eTkInteger && tk != eTkIdent && tk != eTkInteger && tk != eTkString) {
				bError = true;
				CPreProcess::ParseMsg(Error, "expected an identifier or integer for parameter '%s'", params[i].m_pName);
				break;
			}

            *(string *)params[i].m_pValue = m_pLex->GetTkString();
            tk = m_pLex->GetNextTk();
        }

        if (tk != eTkComma)
            break;
    }

    if (!bError) {
        for (int i = 0; params[i].m_pName; i += 1) {
            int j = -1;
            if (params[i].m_deprecatedPair != 0) {
                for (j = 0; params[j].m_pName; j += 1)
                    if (params[j].m_deprecatedPair == -params[i].m_deprecatedPair)
                        break;
            }
            if (params[i].m_deprecatedPair < 0 && params[i].m_bPresent) {
                CPreProcess::ParseMsg(Warning, "use of deprecated parameter name '%s', use '%s'",
                    params[i].m_pName, params[j].m_pName);
            }

            if (params[i].m_bRequired && params[i].m_deprecatedPair >= 0 &&
                !params[i].m_bPresent && (j < 0 || !params[j].m_bPresent)) {
                    bError = true;
                    CPreProcess::ParseMsg(Error, "expected required parameter '%s'", params[i].m_pName);
            }
        }
    }

    if (tk != eTkRParen) {
        if (!bError)
            CPreProcess::ParseMsg(Error, "expected a ')'");

        SkipTo(eTkRParen);
		m_pLex->GetNextTk();
        return false;
    }

	m_pLex->GetNextTk();
    return true;
}

bool HtiFile::ParseIntRange(vector<int> * pIntList)
{
	EToken tk = m_pLex->GetTk();
    if (tk != eTkInteger)
        return false;

	int i1 = m_pLex->GetTkInteger();

    tk = m_pLex->GetNextTk();

    if (tk != eTkMinus)
        pIntList->push_back(i1);

    else {
        if (m_pLex->GetNextTk() != eTkInteger) 
			return false;

		int i2 = m_pLex->GetTkInteger();

		for (int i = i1; i <= i2; i += 1)
			pIntList->push_back(i);

        tk = m_pLex->GetNextTk();
    }

	return true;
}

void HtiFile::AddModInstParams(string unit, string modPath, vector<int> &memPortList, string modInst, string replCnt)
{
	int modInstId = -1;
	if (modInst.size() > 0) {
		char * pEnd;
		modInstId = strtol(modInst.c_str(), &pEnd, 10);

		if (pEnd != modInst.c_str() + modInst.size())
			CPreProcess::ParseMsg(Error, "expected an integer value for modInst parameter");
	}

	int replCntInt = 1;
	if (replCnt.size() > 0) {
		CHtString replStr = replCnt;
		replStr.InitValue(CPreProcess::m_lineInfo, false, 1);

		//char * pEnd;
		//replCntInt = strtol(replCnt.c_str(), &pEnd, 10);

		//if (pEnd != replCnt.c_str() + replCnt.size())
		//	CPreProcess::ParseMsg(Error, "expected an integer value for replCnt parameter");

		if (replStr.AsInt() < 1 || replStr.AsInt() > 8)
			CPreProcess::ParseMsg(Error, "expected replCnt in range 1-8");

		replCntInt = replStr.AsInt();
	}

	m_modInstParamsList.push_back(CModInstParams(unit, modPath, memPortList, modInstId, replCntInt));
}

void HtiFile::AddMsgIntfConn( string &outUnit, string &outPath, string &inUnit, string &inPath, bool aeNext, bool aePrev )
{
	m_msgIntfConn.push_back(new CMsgIntfConn(outUnit, outPath, inUnit, inPath, aeNext, aePrev ));
}

HtiFile::CMsgIntfInfo::CMsgIntfInfo(bool bInBound, string &msgUnit, string &msgPath)
{
	m_bInBound = bInBound;
	m_pMsgIntf = 0;

	// parse msgUnit for unitName and unitIdx
	char const * pStr = msgUnit.c_str();
	while (*pStr != '\0' && *pStr != '[') pStr += 1;
	m_unitName = string(msgUnit.c_str(), pStr - msgUnit.c_str());

	if (*pStr == '[') {
		pStr += 1;
		char * pEnd;
		m_unitIdx = strtol(pStr, &pEnd, 10);
		if (pStr == pEnd || *pEnd != ']')
			CPreProcess::ParseMsg(Error, "illegal unit index syntax");
		pStr = pEnd+1;
	} else
		m_unitIdx = -1;

	if (*pStr != '\0')
		CPreProcess::ParseMsg(Error, "illegal unit index syntax");

	// parse msgPath for path, replIdx, msgIntfName and up to two msgIntfIndexes
	int strIdx = msgPath.size()-1;
	char const * pPath = msgPath.c_str();
	int idx2 = -1;
	if (pPath[strIdx] == ']') {
		strIdx -= 1;
		while (strIdx >= 0 && isdigit(pPath[strIdx])) strIdx -= 1;
		char * pEnd;
		idx2 = strtol(pPath+strIdx+1, &pEnd, 10);
		if (pPath[strIdx] != '[' || pEnd == pStr) {
			CPreProcess::ParseMsg(Error, "illegal syntax for message interface index");
			return;
		}
		strIdx -= 1;
	}

	int idx1 = -1;
	if (pPath[strIdx] == ']') {
		strIdx -= 1;
		while (strIdx >= 0 && isdigit(pPath[strIdx])) strIdx -= 1;
		char * pEnd;
		idx1 = strtol(pPath+strIdx+1, &pEnd, 10);
		if (pPath[strIdx] != '[' || pEnd == pStr) {
			CPreProcess::ParseMsg(Error, "illegal syntax for message interface index");
			return;
		}
		strIdx -= 1;
	}

	if (idx1 >= 0) m_msgIntfIdx.push_back(idx1);
	if (idx2 >= 0) m_msgIntfIdx.push_back(idx2);

	// now get message interface name
	pStr = pPath+strIdx+1;
	while (strIdx >= 0 && pPath[strIdx] != '/') strIdx -= 1;
	m_msgIntfName = string(pPath+strIdx+1, pStr - (pPath+strIdx+1));

	if (pPath[strIdx] != '/') {
		CPreProcess::ParseMsg(Error, "illegal syntax for message interface name");
		return;
	}
	strIdx -= 1;

	// parse replication idx if present
	if (pPath[strIdx] == ']') {
		strIdx -= 1;
		while (strIdx >= 0 && isdigit(pPath[strIdx])) strIdx -= 1;
		char * pEnd;
		m_replIdx = strtol(pPath+strIdx+1, &pEnd, 10);
		if (pPath[strIdx] != '[' || pEnd == pStr) {
			CPreProcess::ParseMsg(Error, "illegal syntax for module replication index");
			return;
		}
		strIdx -= 1;
	} else
		m_replIdx = -1;

	// parse module path
	if (strIdx == 0) {
		CPreProcess::ParseMsg(Error, "illegal syntax for module path");
		return;
	}

	m_modPath = string(pPath, strIdx+1);
}

bool HtiFile::isMsgPathMatch(CLineInfo & lineInfo, CMsgIntfInfo & info, CModule &mod, CMsgIntf * pMsgIntf)
{
	if (info.m_msgIntfName != pMsgIntf->m_name || pMsgIntf->m_dir == "out" && info.m_bInBound)
		return false;

	for (size_t modInstIdx = 0; modInstIdx < mod.m_modInstList.size(); modInstIdx += 1) {
		CModInst & modInst = mod.m_modInstList[modInstIdx];

		for (size_t modPathIdx = 0; modPathIdx < modInst.m_modPaths.size(); modPathIdx += 1) {
			string &modPath = modInst.m_modPaths[modPathIdx];

			char const * pStr = modPath.c_str();
			while (*pStr != ':' && *pStr != '\0') pStr += 1;
			string unitName(modPath.c_str(), pStr - modPath.c_str());

			if (*pStr != ':' || pStr[1] != '/') {
				CPreProcess::ParseMsg(Error, lineInfo, "Invalid module path syntax '%s'", modPath.c_str());
				return false;
			}

			pStr += 1;
			string pathName = pStr;

			if (info.m_unitName.size() > 0 && info.m_unitName != unitName) continue;
			if (info.m_modPath != pathName) continue;

			if (pMsgIntf->m_bAutoConn) {
				CPreProcess::ParseMsg(Error, lineInfo, "message interface match to interface with auto connect enabled");
				static bool bErrMsg = true;
				if (bErrMsg) {
					bErrMsg = false;
					CPreProcess::ParseMsg(Info, lineInfo, "use AddMsgIntf(autoConn=false) to allow HTI connection");
				}
			}

			return true;
		}
	}

	return false;
}

void HtiFile::getModInstParams(string modPath, CInstanceParams & modInstParams)
{
	// Look through list of AddModInstParams for specified modPath. May be multiple
	// matches. Must merge results and check for inconsistencies.

	for (size_t i = 0; i < m_modInstParamsList.size(); i += 1) {
		CModInstParams & instParams = m_modInstParamsList[i];

		string paramModPath = instParams.m_unitName + ":" + instParams.m_modPath;

		if (paramModPath != modPath)
			continue;

		instParams.m_wasUsed = true;

		if (modInstParams.m_memPortList.size() == 0)
			modInstParams.m_memPortList = instParams.m_memPortList;
		else
			CPreProcess::ParseMsg(Error, 
				"Instance file specified memPort for module path '%s' multiple times",
				modPath.c_str());

		if (modInstParams.m_replCnt < 0)
			modInstParams.m_replCnt = instParams.m_replCnt;
		else if (instParams.m_replCnt >= 0)
			CPreProcess::ParseMsg(Error, 
			"Instance file specified replCnt for module path '%s' multiple times",
			modPath.c_str());

		modInstParams.m_instId = instParams.m_instId;

		modInstParams.m_lineInfo = instParams.m_lineInfo;
	}

	if (modInstParams.m_replCnt < 0)
		modInstParams.m_replCnt = 1;
				
	if (modInstParams.m_instId < 0)
		modInstParams.m_instId = 0;
}

void HtiFile::checkModInstParamsUsed()
{
	for (size_t i = 0; i < m_modInstParamsList.size(); i += 1) {
		CModInstParams & instParams = m_modInstParamsList[i];

		if (instParams.m_wasUsed) continue;

		CPreProcess::ParseMsg(Error, instParams.m_lineInfo, "AddModInstParams with unit=%s, modPath=%s was not used",
			instParams.m_unitName.c_str(), instParams.m_modPath.c_str());
	}
}
