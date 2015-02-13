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

bool HtdFile::loadHtdFile(string fileName)
{
    if (!m_pLex->LexOpen(fileName)) {
        printf("Could not open file '%s'\n", fileName.c_str());
        exit(1);
    }

    m_pDsn = new HtdDsn;
    m_pParseMethod = 0;

    while (m_pLex->GetTk() != eTkEof) {
        if (!m_pParseMethod) {
            if (m_pLex->GetTk() != eTkIdent) {
                CPreProcess::ParseMsg(Error, "Expected either dsnInfo or a module name");
				SkipTo(eTkSemi);
				m_pLex->GetNextTk();
                continue;									// drop the whole line
            }

			if (m_pLex->GetTkString() == "typedef") {
				ParseTypedef();
				continue;
			}

			if (m_pLex->GetTkString() == "struct" || m_pLex->GetTkString() == "class" || m_pLex->GetTkString() == "union") {
				ParseRecord();
				continue;
			}

            if (m_pLex->GetTkString() == "dsnInfo")
				m_pLex->GetNextTk();
			else {
                // look for existing module
                int idx = m_pDsn->m_modList.index(m_pLex->GetTkString());
				m_pLex->GetNextTk();
                if (idx >= 0) {
                    m_pOpenMod = m_pDsn->m_modList[idx];
                    m_pParseMethod = &HtdFile::ParseModuleMethods;
                    m_currModuleHandle = m_pOpenMod->m_handle;	// make current module accessable to editor
                } else {
                    CPreProcess::ParseMsg(Error, "Unknown module name   %s", m_pLex->GetTkString().c_str());
					SkipTo(eTkSemi);
					m_pLex->GetNextTk();
                    continue;							
                }
            }
        }

        if (m_pLex->GetTk() == eTkSemi) {

            if (m_pParseMethod)
                m_pParseMethod = 0;
            else
                CPreProcess::ParseMsg(Error, "Unexpected ';'");

			m_pLex->GetNextTk();
			continue;
        }

        if (m_pLex->GetTk() != eTkPeriod) {
            CPreProcess::ParseMsg(Error, "Expected a '.'");
            m_pParseMethod = 0;
            SkipTo(eTkSemi);
			m_pLex->GetNextTk();
            continue;
        }

        if (m_pLex->GetNextTk() != eTkIdent) {
            CPreProcess::ParseMsg(Error, "Expected design or module method");
            m_pParseMethod = 0;
            SkipTo(eTkSemi);
			m_pLex->GetNextTk();
            continue;
        }

        if (m_pParseMethod)
            (this->*m_pParseMethod)();
        else
            ParseDsnInfoMethods();

        if (m_pParseMethod) {
            if (m_pLex->GetTk() == eTkSemi) {
                m_pParseMethod = 0;
                m_pLex->GetNextTk();
            }
        } else {
			if (m_pLex->GetTk() != eTkSemi) {
				CPreProcess::ParseMsg(Error, "expected ';'");
				SkipTo(eTkSemi);
			}
			m_pLex->GetNextTk();
		}
    }

	AddIncludeList(m_pLex->GetIncludeList());

    delete m_pDsn;

    return true;
}

void HtdFile::ParseTypedef()
{
	bool bIsInputFile = m_pLex->IsInputFile();

	m_pLex->GetNextTk();
	
	int builtinWidth;
	bool bSignedBuiltin;
	string type = ParseType(builtinWidth, bSignedBuiltin);

	if (type.size() == 0)
		return;

	if (m_pLex->GetTk() != eTkIdent) {
		CPreProcess::ParseMsg(Error, "expected identifier");
		SkipTo(eTkSemi);
		m_pLex->GetNextTk();
		return;
	}

	string name = m_pLex->GetTkString();

    m_pDsn->m_typedefNameList.insert(name);

	string width;
    AddTypeDef(name, type, width, bIsInputFile ? "unit" : "include");

	if (m_pLex->GetNextTk() != eTkSemi) {
		CPreProcess::ParseMsg(Error, "syntax error, expected ';'");
		SkipTo(eTkSemi);
		m_pLex->GetNextTk();
		return;
	}
	m_pLex->GetNextTk();
}

void HtdFile::ParseRecord()
{
	bool bUnion = m_pLex->GetTkString() == "union";
	bool bIsInputFile = m_pLex->IsInputFile();

	if (m_pLex->GetNextTk() != eTkIdent) {
		CPreProcess::ParseMsg(Error, "expected struct/union name");
		SkipTo(eTkSemi);
		m_pLex->GetNextTk();
		return;
	}

	string structName = m_pLex->GetTkString();

    if (m_pDsn->m_structNameList.isInList(structName)) {
        CPreProcess::ParseMsg(Error, "duplicate struct/union name '%s'", structName.c_str());
		SkipTo(eTkSemi);
		m_pLex->GetNextTk();
		return;
	}

	if (m_pLex->GetNextTk() == eTkSemi) {
		// forward declaration, ignore
		m_pLex->GetNextTk();
		return;
	}

	if (m_pLex->GetTk() != eTkLBrace) {
		CPreProcess::ParseMsg(Error, "expected '{'");
		SkipTo(eTkSemi);
		m_pLex->GetNextTk();
		return;
	}
	m_pLex->GetNextTk();

    m_pDsn->m_structNameList.insert(structName);

	EScope scope = bIsInputFile ? eUnit : eHost;
    m_pOpenStruct = AddStruct(structName, true, bUnion, scope, !bIsInputFile, "");

	m_pDsn->m_structFieldList.clear();

	ParseFieldList();
}

void HtdFile::ParseFieldList()
{
	// parse field list
	for (;;) {
		if (m_pLex->GetTk() == eTkEof)
			break;

		bool bAtomicInc = false;
		bool bAtomicSet = false;
		bool bAtomicAdd = false;
		if (m_pLex->GetTk() == eTkPound && m_pLex->GetNextTk() == eTkIdent && m_pLex->GetTkString() == "pragma") {
			if (m_pLex->GetNextTk() != eTkIdent || m_pLex->GetTkString() != "htl" ||
				m_pLex->GetNextTk() != eTkIdent ||
				!((bAtomicInc = (m_pLex->GetTkString() == "atomic_inc")) ||
				(bAtomicSet = (m_pLex->GetTkString() == "atomic_set")) ||
				(bAtomicAdd = (m_pLex->GetTkString() == "atomic_add"))) ||
				m_pLex->GetNextTk() != eTkPound) 
			{
				CPreProcess::ParseMsg(Error, "expected #pragma htl {atomic_inc | atomic_set | atomic_add}");
				SkipTo(eTkSemi);
				m_pLex->GetNextTk();
				continue;
			}
			m_pLex->GetNextTk();
		}

		if (m_pLex->GetTk() != eTkIdent) {
			CPreProcess::ParseMsg(Error, "expected builtin type, typedef name or struct/union name");
			SkipTo(eTkSemi);
			m_pLex->GetNextTk();
			continue;
		}

		bool bUnion = false;
		bool bStruct = false;
		if (m_pLex->GetTkString() == "struct") {
			bStruct = true;
			m_pLex->GetNextTk();
		} else if (m_pLex->GetTkString() == "union") {
			bUnion = true;
			m_pLex->GetNextTk();
		}

		if (m_pLex->GetTk() == eTkLBrace) {
			if (!bUnion && !bStruct) {
				CPreProcess::ParseMsg(Error, "unexpected '{' within struct/union definition");
				SkipTo(eTkSemi);
				m_pLex->GetNextTk();
				continue;
			}

			if (bAtomicInc || bAtomicSet)
				CPreProcess::ParseMsg(Error, "unexpected pragma on struct / union");

			// anonamous struct/union

			m_pLex->GetNextTk();

			void * pSavedOpenStruct = m_pOpenStruct;

			m_pOpenStruct = AddStructField(m_pOpenStruct, bUnion ? "union" : "struct", "");

			ParseFieldList();

			m_pOpenStruct = pSavedOpenStruct;

		} else {

			if (m_pLex->GetTk() != eTkIdent) {
				CPreProcess::ParseMsg(Error, "expected type name");
				SkipTo(eTkSemi);
				m_pLex->GetNextTk();
				continue;
			}

			if ((bUnion || bStruct) && !m_pDsn->m_structNameList.isInList(m_pLex->GetTkString()))
				CPreProcess::ParseMsg(Error, "expected type name to be a struct or union");

			int builtinWidth;
			bool bSignedBuiltin;
			string type = ParseType(builtinWidth, bSignedBuiltin);

			for(;;) {
				if (m_pLex->GetTk() != eTkIdent) {
					CPreProcess::ParseMsg(Error, "expected member name");
					SkipTo(eTkSemi);
					m_pLex->GetNextTk();
					return;
				}

				string name = m_pLex->GetTkString();
				m_pLex->GetNextTk();

				vector<CHtString> dimenList;
				while (m_pLex->GetTk() == eTkLBrack)
					dimenList.push_back( ParseArrayDimen() );

				string bitWidth;
				if (dimenList.size() == 0 && m_pLex->GetTk() == eTkColon) {
					if (builtinWidth < 0) {
						CPreProcess::ParseMsg(Error, "bit field width not allowed with type '%s'", type.c_str());
						SkipTo(eTkSemi);
					} else
						bitWidth = m_pLex->GetExprStr(';');

					m_pLex->GetNextTk();
				}

				int atomicMask = (bAtomicInc ? ATOMIC_INC : 0) |
					(bAtomicSet ? ATOMIC_SET : 0) |
					(bAtomicAdd ? ATOMIC_ADD : 0);

				if (name.size() > 0 && m_pDsn->m_structFieldList.isInList(name))
					CPreProcess::ParseMsg(Error, "duplicate struct/union field name '%s'", name.c_str());
				else {
					m_pDsn->m_structFieldList.insert(name);
					string base;
					AddStructField(m_pOpenStruct, type, name, bitWidth, base, dimenList,
						true, true, false, false, HtdFile::eDistRam, atomicMask);
				}

				if (m_pLex->GetTk() == eTkComma)
					m_pLex->GetNextTk();
				else
					break;
			}

			if (m_pLex->GetTk() != eTkSemi) {
				CPreProcess::ParseMsg(Error, "expected ';'");
				SkipTo(eTkSemi);
			}
			m_pLex->GetNextTk();
		}

		if (m_pLex->GetTk() == eTkRBrace) {
			m_pLex->GetNextTk();
			break;
		}
	}

	if (m_pLex->GetTk() != eTkSemi) {
		CPreProcess::ParseMsg(Error, "expected ';'");
		SkipTo(eTkSemi);
	}
	m_pLex->GetNextTk();
}

string HtdFile::ParseArrayDimen()
{
	char const *pStr = m_pLex->GetLinePos();
	char const *pEnd = pStr;

	while (*pEnd != ']' && *pEnd != '\0') pEnd += 1;

	if (*pEnd != ']') {
		CPreProcess::ParseMsg(Error, "expected [integer expr]");
		return string("1");
	}

	string dimen = string(pStr, pEnd-pStr);

	m_pLex->SetLinePos(pEnd);
	m_pLex->GetNextTk();	// eTkLBrack
	m_pLex->GetNextTk();
	return dimen;
}

struct CBuiltinTypes {
	const char * m_pName;
	int m_width;
	bool m_bSigned;
} builtinTypes[] = {
	{ "char", 8, true },
	{ "short", 16, true },
	{ "int", 32, true },
	{ "bool", 1, false },
	{ "uint8_t", 8, false },
	{ "uint16_t", 16, false },
	{ "uint32_t", 32, false },
	{ "uint64_t", 64, false },
	{ "int8_t", 8, true },
	{ "int16_t", 16, true },
	{ "int32_t", 32, true },
	{ "int64_t", 64, true },
	{ 0, 0, false }
};

bool HtdFile::FindBuiltinType(string name, int & width, bool & bSigned)
{
	for (int i = 0; builtinTypes[i].m_pName; i += 1) {
		if (name == builtinTypes[i].m_pName) {
			width = builtinTypes[i].m_width;
			bSigned = builtinTypes[i].m_bSigned;
			return true;
		}
	}
	return false;
}

string HtdFile::ParseType(int & builtinWidth, bool & bSignedBuiltin)
{
	string typeStr;
	string errorStr = "";
	builtinWidth = -1;
	bSignedBuiltin = false;

	// Either builtin type, struct/union or typedef
	if (m_pLex->GetTk() != eTkIdent) {
		CPreProcess::ParseMsg(Error, "expected builtin type, typedef name or struct/union name");
		SkipTo(eTkSemi);
		m_pLex->GetNextTk();
		return errorStr;
	}

	string name = m_pLex->GetTkString();

	if (IsInTypeList(name) || IsInStructList(name)) {
		m_pLex->GetNextTk();
		return name;
	}

	if (name == "sc_uint" || name == "sc_int" || name == "sc_biguint" || name == "sc_bigint") {
		typeStr = name;

		if (m_pLex->GetNextTk() != eTkLess) {
			CPreProcess::ParseMsg(Error, "expected template argument '< >'");
			SkipTo(eTkSemi);
			m_pLex->GetNextTk();
			return errorStr;
		}

		char buf[256];
		char *pBuf = buf;
		char const *pStr = m_pLex->GetLinePos();

		int parenCnt = 0;
		while ((parenCnt > 0 || *pStr != '>') && *pStr != '\0') {
			if (*pStr == '(') parenCnt += 1;
			if (*pStr == ')') parenCnt -= 1;
			*pBuf++ = *pStr++;
		}
		*pBuf = '\0';

		if (parenCnt != 0) {
			CPreProcess::ParseMsg(Error, "template argument has unbalanced parenthesis");
			SkipTo(eTkSemi);
			m_pLex->GetNextTk();
			return errorStr;
		}

		if (*pStr != '>') {
			CPreProcess::ParseMsg(Error, "expected template argument '< >'");
			SkipTo(eTkSemi);
			m_pLex->GetNextTk();
			return errorStr;
		}

		typeStr += "<" + string(buf) + ">";

		m_pLex->SetLinePos(pStr+1);

		m_pLex->GetNextTk();
		return typeStr;
	}

	if (strncmp(name.c_str(), "ht_uint", 7) == 0) {
		char * pEnd;
		int width = strtol(name.c_str()+7, & pEnd, 10);

		if (*pEnd != '\0' || width <= 0 || width > 64) {
			CPreProcess::ParseMsg(Error, "expected ht_uint<1-64>");
			SkipTo(eTkSemi);
			m_pLex->GetNextTk();
			return errorStr;
		}

		m_pLex->GetNextTk();
		return name;
	}

	if (strncmp(name.c_str(), "ht_int", 6) == 0) {
		char * pEnd;
		int width = strtol(name.c_str()+6, & pEnd, 10);

		if (*pEnd != '\0' || width <= 0 || width > 64) {
			CPreProcess::ParseMsg(Error, "expected ht_int<1-64>");
			SkipTo(eTkSemi);
			m_pLex->GetNextTk();
			return errorStr;
		}

		m_pLex->GetNextTk();
		return name;
	}

	// must be builtin type
	bool bType = false;
	bool bSign = false;
	bSignedBuiltin = true;

	for (;;) {
		if (m_pLex->GetTk() != eTkIdent) {
			CPreProcess::ParseMsg(Error, "expected builtin type");
			SkipTo(eTkSemi);
			m_pLex->GetNextTk();
			return errorStr;
		}

		name = m_pLex->GetTkString();

		if (name == "unsigned") {
			if (bSign) {
				CPreProcess::ParseMsg(Error, "unsigned specified multiple times");
				SkipTo(eTkSemi);
				m_pLex->GetNextTk();
				return errorStr;
			}
			bSign = true;
			bSignedBuiltin = false;

			if (typeStr.size() > 0) typeStr += " ";
			typeStr += name;

			m_pLex->GetNextTk();
			continue;
		}

		int idx = 0;
		for (idx = 0; builtinTypes[idx].m_pName; idx += 1) {
			if (name != builtinTypes[idx].m_pName)
				continue;

			if (bType) {
				CPreProcess::ParseMsg(Error, "builtin type specified multiple times");
				SkipTo(eTkSemi);
				m_pLex->GetNextTk();
				return errorStr;
			}
			bType = true;

			if (typeStr.size() > 0) typeStr += " ";
			typeStr += name;

			builtinWidth = builtinTypes[idx].m_width;

			if (idx >= 3) {
				if (bSign) {
					CPreProcess::ParseMsg(Error, "unexpected type after unsigned");
					SkipTo(eTkSemi);
					m_pLex->GetNextTk();
					return errorStr;
				}
				bSign = true;
				bSignedBuiltin = builtinTypes[idx].m_bSigned;
			}

			m_pLex->GetNextTk();
			continue;
		}

		if (name == "long") {
			if (bType) {
				CPreProcess::ParseMsg(Error, "builtin type specified multiple times");
				SkipTo(eTkSemi);
				m_pLex->GetNextTk();
				return errorStr;
			}
			bType = true;

			if (typeStr.size() > 0) typeStr += " ";
			typeStr += name;

			if (m_pLex->GetNextTk() == eTkIdent && m_pLex->GetTkString() == "long") {
				typeStr += " long";

				builtinWidth = 64;

				m_pLex->GetNextTk();
			} else
				builtinWidth = 32;

			continue;
		}

		if (typeStr.size() == 0) {
			CPreProcess::ParseMsg(Error, "expected builtin type, typedef name or struct/union name");
			SkipTo(eTkSemi);
			m_pLex->GetNextTk();
			return errorStr;
		}

		return typeStr;
	}
}

void HtdFile::ParseDsnInfoMethods()
{
    if (m_pLex->GetTkString() == "AddDefine") {

		static bool bWarn = true;
        if(bWarn == true) {
			CPreProcess::ParseMsg(Warning, "AddDefine has been deprecated, use C/C++ #define within HTD file");
        	bWarn = false;
        }

       //
        //	AddDefine is allowed at unit scope
        //
        string name;
        string value;

        CParamList params[] = {
            { "name",	&name,	true,	eTkIdent   , 0, 0},
            { "value",	&value,	true,	eTkInteger , 0, 0},
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected dsnInfo.AddDefine( name, value )");

        if (m_pDsn->m_defineNameList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate define name '%s'", name.c_str());
        else {
            m_pDsn->m_defineNameList.insert(name);
            AddDefine(m_pOpenMod->m_handle, name, value);
        }

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddTypeDef") {

		static bool bWarn = true;
        if(bWarn == true) {
        	CPreProcess::ParseMsg(Warning, "AddTypeDef has been deprecated, use C/C++ typedef within HTD file");
        	bWarn = false;
        }

        string name;
        string type;
        string width;

        CParamList params[] = {
            { "name",	&name,	true,	eTkIdent,	0, 0},
            { "type",	&type,	true,	eTkTypeStr, 0, 0},
            { "width",	&width,	false,	eTkInteger, 0, 0},
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected dsnInfo.AddTypeDef( name, type {, width } )");

        if (m_pDsn->m_typedefNameList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate typedef name '%s'", name.c_str());
        else {
            m_pDsn->m_typedefNameList.insert(name);
            AddTypeDef(name, type, width);
        }

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddModule") {
        string name;
		string clock = "1x";
        string htIdW;
        string reset;
		bool bPause = false;

        CParamList params[] = {
            { "name",	&name,		true,	eTkIdent, 0, 0 },
			{ "clock",	&clock,		false,	eTkIdent, 0, 0 },
            { "htIdW",	&htIdW,		false,	eTkInteger, 1, 0 },
            { "reset",	&reset,		false,	eTkIdent,   0, 0 },
            { "pause",	&bPause,	false,	eTkBoolean, 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected dsnInfo.AddModule( name, {, clock=1x} {, htIdW=\"\"} {, pause=false } {, reset=\"\"} )");

		EClkRate clkRate = eClk1x;
		if (clock == "1x")
			clkRate = eClk1x;
		else if (clock == "2x")
			clkRate = eClk2x;
		else
			CPreProcess::ParseMsg(Error, "expected clock parameter to be a value of '1x' or '2x'");

        if (m_pDsn->m_modList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate module name '%s'", name.c_str());
        else {
            m_pOpenMod = m_pDsn->m_modList.insert(name);
            m_pOpenMod->m_handle = AddModule( name, clkRate );
            m_pParseMethod = &HtdFile::ParseModuleMethods;
  
			AddThreads(m_pOpenMod->m_handle, htIdW, reset, bPause);
       }

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddStruct" || m_pLex->GetTkString() == "AddUnion") {

		static bool bWarn = true;
        if(bWarn == true) {
        	CPreProcess::ParseMsg(Warning, "AddStruct/AddUnion has been deprecated, use C/C++ struct/union within HTD file");
        	bWarn = false;
        }

        bool bUnion = m_pLex->GetTkString() == "AddUnion";

        string name;
        bool bHost = false;

        CParamList params[] = {
            { "name",	&name,	true,	eTkIdent , 0, 0 },
            { "host",	&bHost, false,	eTkBoolean , 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params)) {
            if (bUnion)
                CPreProcess::ParseMsg(Error, "expected dsnInfo.AddUnion( name )");
            else
                CPreProcess::ParseMsg(Error, "expected dsnInfo.AddStruct( name )");
        }

        if (m_pDsn->m_structNameList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate struct/union name '%s'", name.c_str());
        else {
            m_pDsn->m_structNameList.insert(name);
            m_pOpenStruct = AddStruct(name, false, bUnion, bHost ? eHost : eUnit, false, "");
            m_pParseMethod = &HtdFile::ParseStructMethods;
            m_pDsn->m_structFieldList.clear();
        }

		m_pLex->GetNextTk();

    } else
        CPreProcess::ParseMsg(Error, "Expected DsnInfo method");
}

void HtdFile::ParseModuleMethods()
{
    if (m_pLex->GetTkString() == "AddReadMem") {

        string queueW = "5";
        string rspGrpId;
        string rspCntW;
        bool bMaxBw = false;
		string pause;
		string poll = "false";
 
        CParamList params[] = {
            { "queueW",		&queueW,	false,	eTkInteger, 0, 0 },
            { "rspGrpId",	&rspGrpId,	false,	eTkIdent,   0, 0 },
            { "rspCntW",	&rspCntW,	false,	eTkInteger, 0, 0 },
            { "maxBw",		&bMaxBw,	false,	eTkBoolean, 0, 0 },
            { "pause",      &pause,     false,  eTkIdent, 0, 0 },
            { "poll",       &poll,      false,  eTkIdent, 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddReadMem( { queueW } {, rspGrpId } {, rspCntW } {, maxBw } {, pause } {, poll } )");

		bool bPoll = poll != "false";

		if (pause.size() == 0)
			pause = poll == "false" ? "true" : "false";

		bool bPause = pause != "false";

		if (pause != "true" && pause != "false")
			CPreProcess::ParseMsg(Error, "expected pause parameter value to be true or false");

		if (poll != "true" && poll != "false")
			CPreProcess::ParseMsg(Error, "expected poll parameter value to be true or false");

		if (!bPoll && !bPause)
			CPreProcess::ParseMsg(Error, "unsupported parameter combination, pause=false and poll=false");

        m_pOpenMifRd = AddReadMem( m_pOpenMod->m_handle, queueW, rspGrpId, rspCntW, bMaxBw, bPause, bPoll);
        m_pParseMethod = &HtdFile::ParseMifRdMethods;
        m_pDsn->m_mifRdDstList.clear();

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddWriteMem") {

        string queueW = "5";
        string rspGrpId;
        string rspCntW;
        bool bMaxBw = false;
        bool bPause = true;
		bool bReqPause = false;
        bool bPoll = false;

        CParamList params[] = {
            { "queueW",		&queueW,	false,	eTkInteger, 0, 0 },
            { "rspGrpId",	&rspGrpId,	false,	eTkIdent,   0, 0 },
            { "rspCntW",	&rspCntW,	false,	eTkInteger, 0, 0 },
            { "maxBw",		&bMaxBw,	false,	eTkBoolean, 0, 0 },
            { "pause",      &bPause,    false,  eTkBoolean, 0, 0 },
            { "reqPause",   &bReqPause, false,  eTkBoolean, 0, 0 },
            { "poll",       &bPoll,     false,  eTkBoolean, 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddWriteMem( { queueW } {, rspGrpId } {, rspCntW } {, maxBw } {, pause } {, poll } {, reqPause} )");

        m_pOpenMifWr = AddWriteMem( m_pOpenMod->m_handle, queueW, rspGrpId, rspCntW, bMaxBw, bPause, bPoll, bReqPause);
        m_pParseMethod = &HtdFile::ParseMifWrMethods;
        m_pDsn->m_mifWrSrcList.clear();

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddCall") {

        string func;
		string call;
		string fork="false";
        string queueW = "5";
		string dest="auto";

        CParamList params[] = {
            { "func",		&func,		true,	eTkIdent,   0, 0 },
            { "async",		&fork,		false,	eTkIdent,  -1, 0 },
            { "call",		&call,		false,	eTkIdent,   0, 0 },
            { "fork",		&fork,		false,	eTkIdent,   1, 0 },
            { "queueW",		&queueW,	false,	eTkInteger, 0, 0 },
			{ "dest",		&dest,		false,	eTkIdent,	0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddCall( func {, call=true} {, fork=false} {, queueW=5 } {, dest=auto } )");

		bool bFork = fork != "false";

		if (call.size() == 0)
			call = fork == "false" ? "true" : "false";

		bool bCall = call != "false";

		if (call != "true" && call != "false")
			CPreProcess::ParseMsg(Error, "expected call parameter value to be true or false");

		if (fork != "true" && fork != "false")
			CPreProcess::ParseMsg(Error, "expected fork parameter value to be true or false");

		if (!bFork && !bCall)
			CPreProcess::ParseMsg(Error, "unsupported parameter combination, call=false and fork=false");

        if (m_pOpenMod->m_callList.isInList(func))
            CPreProcess::ParseMsg(Error, "duplicate call/transfer function name '%s'", func.c_str());
        else {
            m_pOpenMod->m_callList.insert(func);
            AddCall(m_pOpenMod->m_handle, func, bCall, bFork, queueW, dest);
        }

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddTransfer") {

        string func;
        string queueW = "5";

        CParamList params[] = {
            { "func",		&func,		true,	eTkIdent , 0, 0 },
            { "queueW",		&queueW,	false,	eTkInteger , 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected <module>.AddTransfer( func {, queueW=5 } )");

        if (m_pOpenMod->m_callList.isInList(func))
            CPreProcess::ParseMsg(Error, "duplicate call/transfer function name '%s'", func.c_str());
        else {
            m_pOpenMod->m_callList.insert(func);
            AddXfer(m_pOpenMod->m_handle, func, queueW);
        }

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddEntry") {

        string func;
        string inst;
		string reserve;
		bool bHost = false;
		static bool bAddEntryHost = false;

        CParamList params[] = {
            { "func",		&func,	true,	eTkIdent , 0, 0 },
            { "inst",		&inst,	true,	eTkIdent , 0, 0 },
			{ "host",		&bHost,	false,	eTkBoolean, 0, 0 },
			{ "reserve",	&reserve, false, eTkInteger, 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <mod>.AddEntry( func, inst {, host=<true or false>} )");

		if (bHost && g_appArgs.GetEntryName().size() > 0)
			CPreProcess::ParseMsg(Error, "specifing both htl option -en and AddEntry(host=true) not supported");

		if (bHost && bAddEntryHost)
			CPreProcess::ParseMsg(Error, "AddEntry(host=true) was specified on multiple AddEntry commands");
			
		bAddEntryHost |= bHost;

        if (m_pOpenMod->m_entryList.isInList(func))
            CPreProcess::ParseMsg(Error, "duplicate entry function name '%s'", func.c_str());
        else {
            m_pOpenMod->m_entryList.insert(func);
            m_pOpenEntry = AddEntry(m_pOpenMod->m_handle, func, inst, reserve, bHost);
            m_pParseMethod = &HtdFile::ParseEntryMethods;
            m_pDsn->m_entryParamList.clear();
        }

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddReturn") {

        string func;

        CParamList params[] = {
            { "func",		&func,	true,	eTkIdent , 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected <module>.AddReturn( func )");

        if (m_pOpenMod->m_returnList.isInList(func))
            CPreProcess::ParseMsg(Error, "duplicate return function name '%s'", func.c_str());
        else {
            m_pOpenMod->m_returnList.insert(func);
            m_pOpenReturn = AddReturn(m_pOpenMod->m_handle, func);
            m_pParseMethod = &HtdFile::ParseReturnMethods;
            m_pDsn->m_returnParamList.clear();
        }

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddGlobal") {

        string var;
        string dimen1;
        string dimen2;
        string addr1W;
        string addr2W;
        string addr1;
        string addr2;
        string rdStg;
        string wrStg;
        bool bExtern = false;

        CParamList params[] = {
            { "var",		&var,		true,	eTkIdent , 0, 0 },
            { "dimen1",		&dimen1,	false,	eTkInteger , 0, 0 },
            { "dimen2",		&dimen2,	false,	eTkInteger , 0, 0 },
            { "addr1W",		&addr1W,	false,	eTkInteger , 0, 0 },
            { "addr2W",		&addr2W,	false,	eTkInteger , 0, 0 },
            { "addr1",		&addr1,		false,	eTkIdentRef , 0, 0 },
            { "addr2",		&addr2,		false,	eTkIdentRef , 0, 0 },
            { "rdStg",		&rdStg,		false,	eTkInteger , 0, 0 },
            { "wrStg",		&wrStg,		false,	eTkInteger, 0, 0 },
            { "extern",		&bExtern,	false,	eTkBoolean , 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

		bool newGlobal = g_appArgs.IsNewGlobalVarEnabled();
        if (!ParseParameters(params, &newGlobal)) {
            CPreProcess::ParseMsg(Error, "expected <mod>.AddGlobal( var {, dimen1=0 {, dimen2=0 }} {, addr1W=0 {, addr2W=0 }} ");
            CPreProcess::ParseMsg(Info, "      {, addr1=\"\" {, addr2=\"\" }} {, rdStg=1 } {, wrStg=1 } {, extern=false } )");
        }

		if (newGlobal) {

			m_pOpenStruct = m_pDsnInfo->AddGlobal(m_pOpenMod->m_handle);
 			m_pParseMethod = &HtdFile::ParseNewGlobalMethods;

		} else {

			if (addr2W.size() > 0 && addr1W.size() == 0)
				CPreProcess::ParseMsg(Error, "unsupported parameter combination, addr2W is specified but addr1W is not");

			if (dimen2.size() > 0 && dimen1.size() == 0)
				CPreProcess::ParseMsg(Error, "unsupported parameter combination, dimen2 is specified but dimen1 is not");

			if (m_pOpenMod->m_globalList.isInList(var))
				CPreProcess::ParseMsg(Error, "duplicate global variable name '%s'", var.c_str());
			else {
				m_pOpenMod->m_globalList.insert(var);
				m_pOpenStruct = AddGlobalRam(m_pOpenMod->m_handle, var, dimen1, dimen2, addr1W, addr2W, addr1, addr2, rdStg, wrStg, bExtern);
				m_pParseMethod = &HtdFile::ParseGlobalMethods;
				m_pDsn->m_globalFieldList.clear();
			}
		}

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddInst" || m_pLex->GetTkString() == "AddInstr") {

        string name;

        CParamList params[] = {
            { "name",	&name,	true,	eTkIdent, 0, 0  },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected <mod>.AddInstr( name )");

        if (m_pOpenMod->m_instrList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate instruction name '%s'", name.c_str());
        else {
            m_pOpenMod->m_instrList.insert(name);
            AddInstr(m_pOpenMod->m_handle, name);
        }

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddBarrier") {

        string name;
		string barIdW;

        CParamList params[] = {
            { "name",		&name,		false,	eTkIdent,	0, 0 },
 			{ "barIdW",		&barIdW,	false,	eTkInteger, 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddBarrier( { name=<barrierName>, } { barIdW=0 } );");

		string barrierName = name.size() == 0 ? "unnamed barrier" : name;

        if (m_pOpenMod->m_barrierList.isInList(barrierName))
            CPreProcess::ParseMsg(Error, "duplicate barrier name '%s'", barrierName.c_str());
        else {
            m_pOpenMod->m_barrierList.insert(barrierName);
            AddBarrier(m_pOpenMod->m_handle, name, barIdW);
        }

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddReadStream") {

		string name;
		string type;
		string strmBw;
		string elemCntW = "32";
		string strmCnt;
		string memSrc = "coproc";
		vector<int> memPort;
		string access = "all";
		bool paired = false;
		bool bClose = false;
		string tag;
		string rspGrpW;

        CParamList params[] = {
            { "name",		&name,		false,	eTkIdent,	0, 0 },
            { "type",		&type,		true,	eTkTypeStr,	0, 0 },
			{ "strmBw",		&strmBw,	false,	eTkInteger,	0, 0 },
			{ "elemCntW",	&elemCntW,	false,	eTkInteger,	0, 0 },
			{ "strmCnt",	&strmCnt,	false,	eTkInteger,	0, 0 },
			{ "close",		&bClose,	false,	eTkBoolean,	0, 0 },
            { "memSrc",		&memSrc,	false,	eTkIdent,	0, 0 },
			{ "memPort",	&memPort,	false,	eTkIntList,	0, 0 },
			{ "tag",		&tag,		false,  eTkTypeStr, 0, 0 },
//            { "access",		&access,	false,	eTkIdent,	0, 0 },
			{ "rspGrpW",	&rspGrpW,	false,	eTkInteger, 0, 0 },
			{ 0, 0, 0, eTkUnknown, 0, 0 }
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddReadStream( name, type, {, strmCnt=<1>}{, elemCntW=<12>}\n"
			"\t{, strmBw=<10>}{, memSrc=<coproc> }{, memPort=<0> }{, tag=<type>})");

        if (memSrc != "coproc" && memSrc != "host")
            CPreProcess::ParseMsg(Error, "invalid value for memSrc (must be coproc or host)");

		if (memPort.size() == 0)
			memPort.push_back(0);

		for (size_t i = 0; i < memPort.size(); i += 1)
			if (memPort[i] >= 32)
				CPreProcess::ParseMsg(Error, "expected memPort parameter values to be less than 32");

		string listName = name + "$Read";

        if (m_pOpenMod->m_streamReadList.isInList(listName))
            CPreProcess::ParseMsg(Error, "duplicate stream name '%s'", name.c_str());
        else {
            m_pOpenMod->m_streamReadList.insert(listName);
			bool bRead = true;
			string pipeLen = "0";
            m_pDsnInfo->AddStream(m_pOpenMod->m_handle, bRead, name, type, strmBw, elemCntW, 
				strmCnt, memSrc, memPort, access, pipeLen, paired, bClose, tag, rspGrpW);
        }

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddWriteStream") {

		string name;
		string type;
		string strmBw;
		string elemCntW = "32";
		string strmCnt;
		string memDst = "coproc";
		vector<int> memPort;
		string access = "all";
		bool paired = false;
		string reserve = "0";
		bool bClose = false;
		string rspGrpW;

        CParamList params[] = {
            { "name",		&name,		false,	eTkIdent,	0, 0 },
            { "type",		&type,		true,	eTkTypeStr,	0, 0 },
			{ "elemCntW",	&elemCntW,	false,	eTkInteger,	0, 0 },
			{ "strmCnt",	&strmCnt,	false,	eTkInteger,	0, 0 },
			{ "close",		&bClose,	false,	eTkBoolean,	0, 0 },
            { "memDst",		&memDst,	false,	eTkIdent,	0, 0 },
			{ "memPort",	&memPort,	false,	eTkIntList,	0, 0 },
//            { "access",		&access,	false,	eTkIdent,	0, 0 },
 			{ "reserve",	&reserve,	false,	eTkInteger,	0, 0 },
			{ "rspGrpW",	&rspGrpW,	false,	eTkInteger, 0, 0 },
			{ 0, 0, 0, eTkUnknown, 0, 0 }
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddWriteStream( name, type, {, elemCntW=<12>}\n"
				"\t{, strmCnt=<1>}{, memDst=<coproc> }{, memPort=<0> }{, reserve=<0> })");

        if (memDst != "coproc" && memDst != "host")
            CPreProcess::ParseMsg(Error, "invalid value for memDst (must be coproc or host)");

		if (memPort.size() == 0)
			memPort.push_back(0);

		for (size_t i = 0; i < memPort.size(); i += 1)
			if (memPort[i] >= 32)
				CPreProcess::ParseMsg(Error, "expected memPort parameter values to be less than 32");

		string listName = name + "$Write";

        if (m_pOpenMod->m_streamReadList.isInList(listName))
            CPreProcess::ParseMsg(Error, "duplicate stream name '%s'", name.c_str());
        else {
            m_pOpenMod->m_streamReadList.insert(listName);
			bool bRead = false;
			string tag;
			m_pDsnInfo->AddStream(m_pOpenMod->m_handle, bRead, name, type, strmBw, elemCntW,
				strmCnt, memDst, memPort, access, reserve, paired, bClose, tag, rspGrpW);
        }

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddStencilBuffer") {

		string name;
		string type;
		vector<int> gridSize;
		vector<int> stencilSize;
		string pipeLen;

       CParamList params[] = {
            { "name",			&name,			false,	eTkIdent,	0, 0 },
            { "type",			&type,			true,	eTkTypeStr,	0, 0 },
			{ "gridSize",		&gridSize,		true,	eTkIntList, 0, 0 },
			{ "stencilSize",	&stencilSize,	true,	eTkIntList, 0, 0 },
			{ "pipeLen",		&pipeLen,		false,	eTkInteger, 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddStencilBuffer( {name,} type, gridSize, stencilSize {, pipeLen}");

        if (m_pOpenMod->m_stencilBufferList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate stencil buffer name '%s'", name.c_str());
        else {
            m_pOpenMod->m_stencilBufferList.insert(name);
            m_pDsnInfo->AddStencilBuffer(m_pOpenMod->m_handle, name, type, gridSize, stencilSize, pipeLen);
        }

		m_pLex->GetNextTk();

		// insert forward declaration for associated structures
		string inStructName = "CStencilBufferIn";
		string outStructName = "CStencilBufferOut";
		if (name.size() > 0) {
			inStructName += "_" + name;
			outStructName += "_" + name;
		}

		if (m_pDsn->m_structNameList.isInList(inStructName)) {
			CPreProcess::ParseMsg(Error, "previously defined stream buffer struct name '%s'", inStructName.c_str());
			SkipTo(eTkSemi);
			m_pLex->GetNextTk();
			return;
		}

		if (m_pDsn->m_structNameList.isInList(outStructName)) {
			CPreProcess::ParseMsg(Error, "previously defined stream buffer struct name '%s'", inStructName.c_str());
			SkipTo(eTkSemi);
			m_pLex->GetNextTk();
			return;
		}

		m_pDsn->m_structNameList.insert(inStructName);
		m_pDsn->m_structNameList.insert(outStructName);

		AddStruct(inStructName, true, false, eModule, false, "");
		AddStruct(outStructName, true, false, eModule, false, "");

	} else if (m_pLex->GetTkString() == "AddMsgIntf") {

        string dir;
        string name;
        string type;
		string dimen;
		string fanin;
		string fanout;
		string queueW = "0";
		string reserve;
		bool autoConn = true;

        CParamList params[] = {
            { "dir",		&dir,		true,	eTkIdent,	0, 0 },
            { "name",		&name,		true,	eTkIdent,	0, 0 },
            { "type",		&type,		true,	eTkTypeStr,	0, 0 },
			{ "dimen",		&dimen,		false,	eTkInteger, 0, 0 },
			{ "fanin",		&fanin,		false,	eTkInteger, 0, 0 },
			{ "fanout",		&fanout,	false,	eTkInteger, 0, 0 },
            { "queueW",		&queueW,	false,	eTkInteger, 0, 0 },
			{ "reserve",	&reserve,	false,	eTkInteger, 0, 0 },
			{ "autoConn",	&autoConn,	false,  eTkBoolean, 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddMsgIntf( name, dir=<in or out>, type {, dimen=<int>}{, replCnt=<int>}{, queueW=<int> ){, reserve=<int> }{, autoConn=<true>}");

        if (dir != "in" && dir != "out")
            CPreProcess::ParseMsg(Error, "expected dir parameter to have value 'in' or 'out'");

		if (dir == "in" && reserve.size() != 0)
			CPreProcess::ParseMsg(Error, "expected reserve parameter to be used with dir=out");

		if (fanin.size() > 0 && dir != "in")
			CPreProcess::ParseMsg(Error, "expected fanin parameter to be used with dir=in");

		if (fanout.size() > 0 && dir != "out")
			CPreProcess::ParseMsg(Error, "expected fanout parameter to be used with dir=out");

		string fan = dir == "in" ? fanin : fanout;

		string listName = name + "$" + dir;

        if (m_pOpenMod->m_msgIntfList.isInList(listName))
            CPreProcess::ParseMsg(Error, "duplicate message interface name '%s'", name.c_str());
        else {
            m_pOpenMod->m_msgIntfList.insert(listName);
            m_pDsnInfo->AddMsgIntf(m_pOpenMod->m_handle, name, dir, type, dimen, fan, queueW, reserve, autoConn);
        }

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddHostMsg") {

        string dir;
        string name;

        CParamList params[] = {
            { "dir",		&dir,	true,	eTkIdent , 0, 0 },
            { "name",		&name,	true,	eTkIdent , 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected <module>.AddHostMsg( dir=<in or out>, name )");

		if (dir == "Inbound" || dir == "Outbound") {
            static bool bErrMsg = false;
            if (!bErrMsg) {
                CPreProcess::ParseMsg(Warning, "Inbound and Outbound are deprecated, use \"in\" or \"out\" instead");
                bErrMsg = true;
            }
			if (dir == "Inbound")
				dir = "in";
			else
				dir = "out";
		}

        EHostMsgDir eMsgDir = InAndOutbound;
        if (dir == "in")
            eMsgDir = Inbound;
        else if (dir == "out")
            eMsgDir = Outbound;
        else
            CPreProcess::ParseMsg(Error, "expected <module>.AddHostMsg( dir=<in or out>, name )");

        if (m_pOpenMod->m_hostMsgList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate host message name '%s'", name.c_str());
        else {
            m_pOpenMod->m_hostMsgList.insert(name);

            if (eMsgDir == Inbound) {
                m_pOpenIhm = AddHostMsg(m_pOpenMod->m_handle, eMsgDir, name);
                m_pParseMethod = &HtdFile::ParseIhmMethods;
            } else if (eMsgDir == Outbound) {
                AddHostMsg(m_pOpenMod->m_handle, eMsgDir, name);
            }
        }

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddHostData") {

        string dir;
		bool bMaxBw = false;

        CParamList params[] = {
            { "dir",		&dir,		true,	eTkIdent , 0, 0 },
            { "maxBw",		&bMaxBw,	false,	eTkBoolean, 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddHostData( dir=<in or out> {, maxBw=<false>} )");

        EHostMsgDir eMsgDir = InAndOutbound;
        if (dir == "in")
            eMsgDir = Inbound;
        else if (dir == "out")
            eMsgDir = Outbound;
        else
            CPreProcess::ParseMsg(Error, "expected <module>.AddHostData( dir=<in or out> )");

		if (eMsgDir == Inbound && g_appArgs.GetIqModName().size() > 0)
			CPreProcess::ParseMsg(Error, "specifing both htl option -iq and AddHostData(in) not supported");

		else if (eMsgDir == Outbound && g_appArgs.GetOqModName().size() > 0)
			CPreProcess::ParseMsg(Error, "specifing both htl option -oq and AddHostData(out) not supported");

		else
            AddHostData(m_pOpenMod->m_handle, eMsgDir, bMaxBw);

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddPrivate") {

        CParamList params[] = {
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected <module>.AddPrivate( )");

        m_pOpenStruct = AddPrivate(m_pOpenMod->m_handle );
        m_pParseMethod = &HtdFile::ParsePrivateMethods;
        m_pDsn->m_privateFieldList.clear();

        m_pOpenMod = 0;

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddStage") {

        string privWrStg;
        string execStg = "1";

        CParamList params[] = {
            { "privWrStg",	&privWrStg,	false,	eTkInteger,     0, 0 },
            { "execStg",    &execStg,   false,  eTkInteger,     0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddStage( { execStg=1 } {, privWrStg=<execStg> } )");

		if (privWrStg.size() == 0)
			privWrStg = execStg;

        m_pOpenStruct = AddStage(m_pOpenMod->m_handle, privWrStg, execStg );
        m_pParseMethod = &HtdFile::ParseStageMethods;

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddShared") {

        string reset = "true";

        CParamList params[] = {
            { "reset",	&reset,	false,	eTkIdent,     0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddShared( { reset=true } )");

		if (reset != "true" && reset != "false")
			CPreProcess::ParseMsg(Error, "expected reset parameter to be true or false");

        m_pOpenStruct = AddShared(m_pOpenMod->m_handle, reset == "true");
        m_pParseMethod = &HtdFile::ParseSharedMethods;

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddGroup" || m_pLex->GetTkString() == "AddThreadGroup") {

        static bool bFirst = true;
        if (bFirst) {
            bFirst = false;
            CPreProcess::ParseMsg(Warning, "AddThreadGroup has been deprecated, use AddModule(name, clock, htIdW, reset, pause)");
        }

        string groupW;
        string reset;
		bool bPause = false;

        CParamList params[] = {
            { "groupW",		&groupW,	true,	eTkInteger, 1, 0 },
            { "reset",		&reset,		false,	eTkIdent,   0, 0 },
            { "pause",		&bPause,	false,	eTkBoolean, 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddGroup( {, groupW=5 }{, pause=false }{, reset=\"\" } )");

        AddThreads(m_pOpenMod->m_handle, groupW, reset, bPause);

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddFunction") {

        string name;
        string type;

        CParamList params[] = {
            { "name",	&name,	true,	eTkIdent , 0, 0 },
            { "type",	&type,	true,	eTkTypeStr , 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected <module>.AddFunction( type, name )");
        else
            if (m_pOpenMod->m_functionList.isInList(name))
                CPreProcess::ParseMsg(Error, "duplicate function name '%s'", name.c_str());
            else {
                m_pOpenMod->m_functionList.insert(name);
                m_pOpenFunction = AddFunction(m_pOpenMod->m_handle, type, name);
                m_pParseMethod = &HtdFile::ParseFunctionMethods;
                m_pDsn->m_functionParamList.clear();
            }

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddScPrim") {

        bool bError = false;
        if (m_pLex->GetNextTk() != eTkLParen || m_pLex->GetNextTk() != eTkString)
            bError = true;

        string scPrimDecl = m_pLex->GetTkString();

        if (m_pLex->GetNextTk() != eTkComma || m_pLex->GetNextTk() != eTkString)
            bError = true;

        string scPrimCall = m_pLex->GetTkString();

        if (m_pLex->GetNextTk() != eTkRParen)
            bError = true;
		m_pLex->GetNextTk();

        if (bError)
            CPreProcess::ParseMsg(Error, "expected .AddScPrim(\"<ScPrim State Decl>\", \"<ScPrim Function Call>\")");

        AddScPrim(m_pOpenMod->m_handle, scPrimDecl, scPrimCall);

    } else if (m_pLex->GetTkString() == "AddTrace") {

        string name;

        CParamList params[] = {
            { "name",	&name,	true,	eTkIdent , 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected <module>.AddTrace( name )");

        if (m_pOpenMod->m_traceList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate trace name '%s'", name.c_str());
        else {
            m_pOpenMod->m_traceList.insert(name);
            AddTrace(m_pOpenMod->m_handle, name);
        }

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddDefine") {	// define in module
 
		static bool bWarn = true;
        if(bWarn == true) {
			CPreProcess::ParseMsg(Warning, "AddDefine has been deprecated, use C/C++ #define within HTD file");
        	bWarn = false;
        }

        string name;
        string value;
        string scope = "unit";

        CParamList params[] = {
            { "name",	&name,	true,	eTkIdent , 0, 0 },
            { "value",	&value,	true,	eTkInteger , 0, 0 },
            { "scope",	&scope, false,	eTkIdent , 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected dsnInfo.AddDefine( name, value {, scope } )");

        if(scope.compare("host") != 0 && scope.compare("unit") != 0)
            CPreProcess::ParseMsg(Error, "invalid scope '%s'   only "
            "'host' and 'unit' allowed\n", scope.c_str());
        else if (m_pOpenMod->m_defineList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate define name '%s'", name.c_str());
        else {
            m_pOpenMod->m_defineList.insert(name);
            AddDefine(m_pOpenMod->m_handle, name, value, scope, m_pOpenMod->m_name);
        }

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddTypeDef") {	// typedef in module

		static bool bWarn = true;
        if(bWarn == true) {
        	CPreProcess::ParseMsg(Warning, "AddTypeDef has been deprecated, use C/C++ typedef within HTD file");
        	bWarn = false;
        }

        string name;
        string type;
        string width;
        string scope = "module";

        CParamList params[] = {
            { "name",	&name,	true,	eTkIdent, 0, 0  },
            { "type",	&type,	true,	eTkTypeStr , 0, 0 },
            { "scope",	&scope, false,	eTkIdent , 0, 0 },
            { "width",	&width,	false,	eTkInteger, 0, 0  },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };
        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected dsnInfo.AddTypeDef( name, type {, scope } {, width } )");
        if(scope.compare("host") != 0 && scope.compare("unit") != 0
            && scope.compare("module") != 0)
            CPreProcess::ParseMsg(Error, "AddTypeDef invalid scope '%s'   only "
            "'module', 'unit', 'host' allowed\n", scope.c_str());
        else if (m_pDsn->m_typedefNameList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate typedef name '%s'", name.c_str());
        else {
            m_pOpenMod->m_typedefNameList.insert(name);
            AddTypeDef(name, type, width, scope, m_pOpenMod->m_name);
        }

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddStruct" || m_pLex->GetTkString() == "AddUnion") { // AddStruct in Module

		static bool bWarn = true;
        if(bWarn == true) {
        	CPreProcess::ParseMsg(Warning, "AddStruct/AddUnion has been deprecated, use C/C++ struct/union within HTD file");
        	bWarn = false;
        }

        bool bUnion = m_pLex->GetTkString() == "AddUnion";		// is it a union ?

        string name;
        string scope = "module";

        CParamList params[] = {
            { "name",	&name,	true,	eTkIdent , 0, 0 },
            { "scope",	&scope, false,	eTkIdent , 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params)) {
            if (bUnion)
                CPreProcess::ParseMsg(Error, "expected dsnInfo.AddUnion( name {, scope } )");
            else
                CPreProcess::ParseMsg(Error, "expected dsnInfo.AddStruct( name {, scope } )");
        }
        if(scope.compare("host") != 0 && scope.compare("unit") != 0
            && scope.compare("module") != 0)
            CPreProcess::ParseMsg(Error, "AddTypeDef invalid scope '%s'   only "
            "'module', 'unit', 'host' allowed\n", scope.c_str());
        else if (m_pDsn->m_structNameList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate struct/union name '%s'", name.c_str());
        else {
			EScope escope = scope == "host" ? eHost : (scope == "unit" ? eUnit : eModule);
            m_pDsn->m_structNameList.insert(name);
            m_pOpenStruct = AddStruct(name, false, bUnion, escope, false, m_pOpenMod->m_name);
            m_pParseMethod = &HtdFile::ParseStructMethods;
            m_pDsn->m_structFieldList.clear();
        }

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddPrimState") {

        string type;
        string name;
        string include;

        CParamList params[] = {
            { "type",		&type,		true,	eTkTypeStr,	0, 0 },
            { "name",		&name,		true,	eTkIdent,	0, 0 },
            { "include",	&include,	true,	eTkString,	0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0 }
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected <module>.AddPrimState(type=<type>, name=<name>, include=\"<file>\")");

        if (m_pOpenMod->m_primStateList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate prim state name '%s'", name.c_str());
        else {
            m_pOpenMod->m_primStateList.insert(name);
            AddPrimstate(m_pOpenMod->m_handle, type, name, include);
        }

		m_pLex->GetNextTk();

    } else
        CPreProcess::ParseMsg(Error, "Unknown module capability %s", m_pLex->GetTkString().c_str());
}

void HtdFile::ParseFunctionMethods()
{
    if (m_pLex->GetTkString() == "AddParam") {

        string dir;
        string type;
        string name;

        CParamList params[] = {
            { "dir",		&dir,		true,	eTkIdent,	0, 0 },
            { "type",		&type,		true,	eTkTypeStr,	0, 0 },
            { "name",		&name,		true,	eTkIdent,	0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected AddFunction(...).AddParam( dir, type, name )");

        if (dir != "input" && dir != "output" && dir != "inout")
            CPreProcess::ParseMsg(Error, "expected dir={input|output|inout}");

        if (m_pDsn->m_functionParamList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate function parameter name '%s'", name.c_str());
        else {
            m_pDsn->m_functionParamList.insert(name);
            AddFunctionParam(m_pOpenFunction, dir, type, name);
        }

		m_pLex->GetNextTk();

    } else 
        CPreProcess::ParseMsg(Error, "Expected an AddFunction method");
}

void HtdFile::ParseEntryMethods()
{
    if (m_pLex->GetTkString() == "AddParam") {

        string hostType;
        string type;
        string name;
        bool bUnused = false;

        CParamList params[] = {
            { "hostType", &hostType, false, eTkParamStr, 0, 0  },
            { "type",	&type,		true,	eTkTypeStr , 0, 0 },
            { "name",	&name,		true,	eTkIdent , 0, 0 },
            { "unused",	&bUnused,	false,	eTkBoolean , 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected AddEntry(...).AddParam( { hostType=<type>, } type, name {, unused=false } )");

        if (hostType.size() == 0)
            hostType = type;

        if (m_pDsn->m_entryParamList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate entry parameter name '%s'", name.c_str());
        else {
            m_pDsn->m_entryParamList.insert(name);
            AddEntryParam(m_pOpenEntry, hostType, type, name, !bUnused);
        }

		m_pLex->GetNextTk();

    } else 
        CPreProcess::ParseMsg(Error, "Expected an AddEntry method");
}

void HtdFile::ParseReturnMethods()
{
    if (m_pLex->GetTkString() == "AddParam") {

        string hostType;
        string type;
        string name;
        bool bUnused = false;

        CParamList params[] = {
            { "hostType", &hostType, false, eTkParamStr,	0, 0  },
            { "type",	&type,		true,	eTkTypeStr,		0, 0 },
            { "name",	&name,		true,	eTkIdent,		0, 0 },
            { "unused",	&bUnused,	false,	eTkBoolean,		0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected AddReturn(...).AddParam(type, name {, unused=false } )");

        if (hostType.size() == 0)
            hostType = type;

        if (m_pDsn->m_returnParamList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate return parameter name '%s'", name.c_str());
        else {
            m_pDsn->m_returnParamList.insert(name);
            AddReturnParam(m_pOpenReturn, hostType, type, name, !bUnused);
        }

		m_pLex->GetNextTk();

    } else 
        CPreProcess::ParseMsg(Error, "Expected an AddReturn method");
}

void HtdFile::ParseGlobalMethods()
{
    if (m_pLex->GetTkString() == "AddField") {

        string type;
        string name;
        string dimen1;
        string dimen2;
        bool bRead=false;
        bool bWrite=false;

        CParamList params[] = {
            { "type",	&type,		true,	eTkTypeStr,	0, 0 },
            { "name",	&name,		true,	eTkIdent,	0, 0 },
            { "dimen1",	&dimen1,	false,	eTkInteger, 0, 0 },
            { "dimen2",	&dimen2,	false,	eTkInteger, 0, 0 },
            { "read",	&bRead,		false,	eTkBoolean, 0, 0 },
            { "write",	&bWrite,	false,	eTkBoolean, 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected AddField(type, name {, dimen1=0 {, dimen2=0 }} {, read=false} {, write=false} )");

        if (dimen2.size() > 0 && dimen1.size() == 0)
            CPreProcess::ParseMsg(Error, "unsupported parameter combination, dimen2 is specified but dimen1 is not");

        if (m_pDsn->m_globalFieldList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate global field name '%s'", name.c_str());
        else {
            m_pDsn->m_globalFieldList.insert(name);
            AddGlobalField(m_pOpenStruct, type, name, dimen1, dimen2, bRead, bWrite, eDistRam);
        }

		m_pLex->GetNextTk();

    } else
        CPreProcess::ParseMsg(Error, "Expected an AddGlobal method");
}

void HtdFile::ParseNewGlobalMethods()
{
    if (m_pLex->GetTkString() == "AddVar") {

        string type;
        string name;
        string dimen1;
        string dimen2;
        string addr1W;
        string addr2W;
        string addr1;
        string addr2;
        string rdStg;
        string wrStg;
		bool bExtern = false;
		bool bMaxIw = false;
		bool bMaxMw = false;
		string blockRam;

		CParamList params[] =
		{
			{ "type", &type, true, eTkTypeStr, 0, 0 },
			{ "name", &name, true, eTkIdent, 0, 0 },
			{ "dimen1", &dimen1, false, eTkInteger, 0, 0 },
			{ "dimen2", &dimen2, false, eTkInteger, 0, 0 },
			{ "addr1W", &addr1W, false, eTkInteger, 0, 0 },
			{ "addr2W", &addr2W, false, eTkInteger, 0, 0 },
			{ "addr1", &addr1, false, eTkIdentRef, 0, 0 },
			{ "addr2", &addr2, false, eTkIdentRef, 0, 0 },
			{ "rdStg", &rdStg, false, eTkInteger, 0, 0 },
			{ "wrStg", &wrStg, false, eTkInteger, 0, 0 },
			{ "extern", &bExtern, false, eTkBoolean, 0, 0 },
			{ "maxIw", &bMaxIw, false, eTkBoolean, 0, 0 },
			{ "maxMw", &bMaxMw, false, eTkBoolean, 0, 0 },
			{ "blockRam", &blockRam, false, eTkIdent, 0, 0 },
			{ 0, 0, 0, eTkUnknown, 0, 0 }
		};

        if (!ParseParameters(params)) {
            CPreProcess::ParseMsg(Error, "expected .AddVar( type, var {, dimen1=0 {, dimen2=0 }} {, addr1W=0 {, addr2W=0 }} ");
			CPreProcess::ParseMsg(Info, "      {, addr1=\"\" {, addr2=\"\" }} {, rdStg=1 } {, wrStg=1 } {, maxIw=false} {, maxMw=false} )");
		}

		ERamType ramType = eAutoRam;
		if (blockRam == "true")
			ramType = eBlockRam;
		else if (blockRam == "false")
			ramType = eDistRam;
		else if (blockRam.size() > 0)
			CPreProcess::ParseMsg(Error, "expected blockRam value of <true or false>");

        if (dimen2.size() > 0 && dimen1.size() == 0)
            CPreProcess::ParseMsg(Error, "unsupported parameter combination, dimen2 is specified but dimen1 is not");

		if (addr2W.size() > 0 && addr1W.size() == 0)
			CPreProcess::ParseMsg(Error, "unsupported parameter combination, addr2W is specified but addr1W is not");

        //if (!bExtern && m_pDsn->m_globalVarList.isInList(name))
        //    CPreProcess::ParseMsg(Error, "duplicate global variable name '%s'", name.c_str());
        //else {
        //    m_pDsn->m_globalVarList.insert(name);
            m_pDsnInfo->AddGlobalVar(m_pOpenStruct, type, name, dimen1, dimen2, addr1W, addr2W, addr1, addr2, rdStg, wrStg, bMaxIw, bMaxMw, ramType);
        //}

		m_pLex->GetNextTk();

    } else
        CPreProcess::ParseMsg(Error, "Expected an AddGlobal method");
}

void HtdFile::ParsePrivateMethods()
{
    if (m_pLex->GetTkString() == "AddVar") {

        string type;
        string name;
        string dimen1;
        string dimen2;
        string addr1W;
        string addr2W;
        string addr1;
        string addr2;

        CParamList params[] = {
            { "type",		&type,		true,	eTkTypeStr,	0, 0 },
            { "name",		&name,		true,	eTkIdent,	0, 0 },
            { "dimen1",		&dimen1,	false,	eTkInteger, 0, 0 },
            { "dimen2",		&dimen2,	false,	eTkInteger, 0, 0 },
            { "addr1W",		&addr1W,	false,	eTkInteger, 0, 0 },
            { "addr2W",		&addr2W,	false,	eTkInteger, 0, 0 },
            { "addr1",		&addr1,		false,	eTkIdent,	0, 0 },
            { "addr2",		&addr2,		false,	eTkIdent,	0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params)) {
            CPreProcess::ParseMsg(Error, "expected AddVar( type, name {, dimen1=0 {, dimen2=0 }} {, addr1W=0 {, addr2W=0 }}");
			CPreProcess::ParseMsg(Info, "{, addr1=\"\" {, addr2=\"\" }} )");
		}

        if (addr2W.size() > 0 && addr1W.size() == 0)
            CPreProcess::ParseMsg(Error, "unsupported parameter combination, addr2W is specified but addr1W is not");

		if (addr1W.size() > 0 && addr1.size() == 0)
			CPreProcess::ParseMsg(Error, "addr1W is specified but a private variable for addr1 was not specified");

		if (addr2W.size() > 0 && addr2.size() == 0)
			CPreProcess::ParseMsg(Error, "addr2W is specified but a private variable for addr2 was not specified");

        if ((addr2W.size() > 0 || addr2.size() > 0) && addr1W.size() == 0 && addr1.size() == 0)
            CPreProcess::ParseMsg(Error, "unsupported parameter conbination, addr2/addr2W is specified but addr1/addr1W is not");

        if (m_pDsn->m_privateFieldList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate private field name '%s'", name.c_str());
        else {
            m_pDsn->m_privateFieldList.insert(name);
            AddPrivateField(m_pOpenStruct, type, name, dimen1, dimen2, addr1W, addr2W, addr1, addr2);
        }

		m_pLex->GetNextTk();

    } else
        CPreProcess::ParseMsg(Error, "Expected an AddPrivate method");
}

void HtdFile::ParseStructMethods()
{
    if (m_pLex->GetTkString() == "AddField") {

        string base;
        string type;
        string name;
        string dimen1;
        string dimen2;

        CParamList params[] = {
            { "base",	&base,  false,	eTkIdent,	0, 0 },
            { "type",	&type,	true,	eTkTypeStr, 0, 0 },
            { "name",	&name,	true,	eTkIdent , 0, 0 },
            { "dimen1",	&dimen1, false,	eTkInteger , 0, 0 },
            { "dimen2",	&dimen2, false,	eTkInteger , 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected AddField( {base, } type, name {, dimen1 {, dimen2 }} )");

		vector<CHtString> dimenList;
		if (dimen2.size() > 0 && dimen1.size() == 0)
			CPreProcess::ParseMsg(Error, "unsupported parameter conbination, dimen2 is specified but dimen1 is not");
		else {
			if (dimen1.size() > 0)
				dimenList.push_back( dimen1 );
			if (dimen2.size() > 0)
				dimenList.push_back( dimen2 );
		}

		if (name.size() > 0 && m_pDsn->m_structFieldList.isInList(name))
			CPreProcess::ParseMsg(Error, "duplicate struct/union field name '%s'", name.c_str());
        else {
            m_pDsn->m_structFieldList.insert(name);
			string bitWidth;
            AddStructField(m_pOpenStruct, type, name, bitWidth, base, dimenList);
        }

		m_pLex->GetNextTk();

    } else
        CPreProcess::ParseMsg(Error, "Expected an AddStruct or AddUnion method");
}

void HtdFile::ParseStageMethods()
{
    if (m_pLex->GetTkString() == "AddVar") {

        string type;
        string name;
        string dimen1;
        string dimen2;
        string range[2];
        bool bInit = false;
        bool bConn = true;
		bool bReset = false;
		bool bZero = true;
		bool bPrimOut = false;

        CParamList params[] = {
            { "type",		&type,		true,	eTkTypeStr, 0, 0 },
            { "name",		&name,		true,	eTkIdent, 0, 0  },
            { "dimen1",		&dimen1,	false,	eTkInteger, 0, 0  },
            { "dimen2",		&dimen2,	false,	eTkInteger , 0, 0 },
            { "range",		range,		true,	eTkIntRange , 0, 0 },
            { "init",		&bInit,		false,	eTkBoolean, 0, 0 },
            { "conn",		&bConn,		false,	eTkBoolean , 0, 0 },
            { "reset",		&bReset,	false,	eTkBoolean , 0, 0 },
			//{ "zero",		&bZero,		false,  eTkBoolean, 0, 0  },	// obsolete
			{ "primOut",	&bPrimOut,	false,	eTkBoolean, 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected AddVar( type, name {, dimen1=0 {, dimen2=0 }}, range {, init=false } {, conn=false } {, reset=false } )");

        if (dimen2.size() > 0 && dimen1.size() == 0)
            CPreProcess::ParseMsg(Error, "unsupported parameter conbination, dimen2 is specified but dimen1 is not");

		if (bPrimOut) bZero = false;

        if (m_pOpenMod->m_stageList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate stage variable name '%s'", name.c_str());
        else {
            m_pOpenMod->m_stageList.insert(name);
            AddStageField(m_pOpenStruct, type, name, dimen1, dimen2, range, bInit, bConn, bReset, bZero);
        }

		m_pLex->GetNextTk();

    } else
        CPreProcess::ParseMsg(Error, "Expected an AddVar method");
}

void HtdFile::ParseSharedMethods()
{
    if (m_pLex->GetTkString() == "AddVar") {

        string type;
        string name;
        string dimen1;
        string dimen2;
        string rdSelW;
        string wrSelW;
        string addr1W;
        string addr2W;
        string queueW;
        string blockRam;
		string reset;

        CParamList params[] = {
            { "type",		&type,		false,	eTkTypeStr, 0, 0 },
            { "name",		&name,		true,	eTkIdent , 0, 0 },
            { "dimen1",		&dimen1,	false,	eTkInteger , 0, 0 },
            { "dimen2",		&dimen2,	false,	eTkInteger , 0, 0 },
            { "rdSelW",		&rdSelW,	false,	eTkInteger , 0, 0 },
            { "wrSelW",		&wrSelW,	false,	eTkInteger , 0, 0 },
            { "addr1W",		&addr1W,	false,	eTkInteger , 0, 0 },
            { "addr2W",		&addr2W,	false,	eTkInteger , 0, 0 },
            { "queueW",		&queueW,	false,	eTkInteger , 0, 0 },
            { "blockRam",	&blockRam,	false,	eTkIdent,	0, 0 },
            { "reset",		&reset,		false,	eTkIdent,	0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected AddVar( type, name {, dimen1=0 {, dimen2=0 }} {, rdSelW=0}{, wrSelW=0}{, addr1W=0 {, addr2W=0 }} {, queueW=0 } {, blockRam=false } {, reset=true } )");

        bool bBlockRam = false;
        if (blockRam == "true")
            bBlockRam = true;
        else if (blockRam == "false")
            bBlockRam = false;
        else if (blockRam.size() > 0)
            CPreProcess::ParseMsg(Error, "expected blockRam value of <true or false>");

		if (bBlockRam) {
			if (rdSelW.size() > 0 && wrSelW.size() > 0)
				CPreProcess::ParseMsg(Error, "unsupported parameter combination, only one of rdSelW or wrSelW may be specified");
		} else {
			if (rdSelW.size() > 0 || wrSelW.size() > 0)
				CPreProcess::ParseMsg(Error, "unsupported parameter combination, block=false and rdSelW or wrSelW specified");
		}

        if (bBlockRam && addr1W.size() == 0 && queueW.size() == 0)
            CPreProcess::ParseMsg(Error, "unsupported parameter combination, block=true and neither addr1W or queueW specified");

        if (queueW.size() > 0 && addr1W.size() > 0)
            CPreProcess::ParseMsg(Error, "unsupported parameter combination, addr1W and queueW can not both be specified");

        if (addr2W.size() > 0 && addr1W.size() == 0)
            CPreProcess::ParseMsg(Error, "unsupported parameter conbination, addr2W is specified but addr1W is not");

        if (dimen2.size() > 0 && dimen1.size() == 0)
            CPreProcess::ParseMsg(Error, "unsupported parameter conbination, dimen2 is specified but dimen1 is not");

		if (reset.size() > 0 && reset != "true" && reset != "false")
			CPreProcess::ParseMsg(Error, "expected reset value to be true or false");

        ERamType ramType = bBlockRam ? eBlockRam : eDistRam;

        if (m_pOpenMod->m_sharedList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate shared variable name '%s'", name.c_str());
        else {
            m_pOpenMod->m_sharedList.insert(name);
            AddSharedField(m_pOpenStruct, type, name, dimen1, dimen2, rdSelW, wrSelW, addr1W, addr2W, queueW, ramType, reset);
        }

		m_pLex->GetNextTk();

    } else
        CPreProcess::ParseMsg(Error, "Expected an AddShared method");
}

void HtdFile::ParseMifRdMethods()
{
    if (m_pLex->GetTkString() == "AddRdDst" || m_pLex->GetTkString() == "AddDst") {

        if (m_pLex->GetTkString() == "AddRdDst")
            CPreProcess::ParseMsg(Warning, "AddRdDst has been deprecated, use AddDst");

        string name;
        string var;
        string dataLsb="0";
        string field;
        bool bMultiRd=false;
        string dstIdx;
        string memSrc="coproc";
        string atomic="read";
		string rdType="uint64";

        CParamList pavars[] = {
            { "name",		&name,		false,	eTkIdent,   0, 0 },
            { "var",		&var,		true,	eTkIdent,   0, 0 },
            { "field",		&field,		false,	eTkIdent,   0, 0 },
            { "dataLsb",	&dataLsb,	false,	eTkInteger, 0, 0 },
            { "multiRd",    &bMultiRd,  false,  eTkBoolean, 0, 0 },
            { "dstIdx",     &dstIdx,    false,  eTkIdent,   0, 0 },
            { "memSrc",     &memSrc,    false,  eTkIdent,   0, 0 },
			{ "rdType",		&rdType,	false,  eTkTypeStr, 0, 0 },
            { "atomic",     &atomic,    false,  eTkIdent,   0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(pavars))
			CPreProcess::ParseMsg(Error, "expected AddReadMem(...).AddDst( name, var, field {, dataLsb=0 }{, dstIdx }{, memSrc=both }{, rdType=uint64 } )");

        if (name.size() == 0)
            name = var;

		if (rdType != "uint64" && rdType != "uint32" && rdType != "uint16" && rdType != "uint8" &&
			rdType != "int64" && rdType != "int32" && rdType != "int16" && rdType != "int8")
            CPreProcess::ParseMsg(Error, "expected AddDst rdType parameter to be uint64, uint32, uint16, uint8, int64, int32, int16 or int8");

        if (atomic != "read" && bMultiRd)
            CPreProcess::ParseMsg(Error, "multiRd is not supported with atomic operations");

        if (atomic != "read" && atomic != "setBit63")
            CPreProcess::ParseMsg(Error, "invalid value for atomic (must be read or setBit63)");

        if (memSrc != "coproc" && memSrc != "host")
            CPreProcess::ParseMsg(Error, "invalid value for memSrc (must be coproc or host)");

        if (!bMultiRd && dstIdx.size() > 0)
            CPreProcess::ParseMsg(Error, "Single quadword reads do not require dstIdx to be specified");

        if (dstIdx != "" && dstIdx != "varAddr1" && dstIdx != "varAddr2" && dstIdx != "varIdx1" && dstIdx != "varIdx2" && dstIdx != "fldIdx1" && dstIdx != "fldIdx2")
            CPreProcess::ParseMsg(Error, "invalid value for dstIdx (must be varAddr1, varAddr2, varIdx1, varIdx2, fldIdx1 or fldIdx2)");

        if (m_pDsn->m_mifRdDstList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate memory read interface destination name '%s'", name.c_str());
        else {
            m_pDsn->m_mifRdDstList.insert(name);
            AddDst(m_pOpenMifRd, name, var, field, dataLsb, bMultiRd, dstIdx, memSrc, atomic, rdType);
        }

		m_pLex->GetNextTk();

    } else if (m_pLex->GetTkString() == "AddFunction") {

        string name;
        string infoW;
        string stgCnt = "0";
        bool bMultiRd=false;
        string memSrc="coproc";
		string rdType="uint64";

        CParamList params[] = {
            { "name",		&name,		true,	eTkIdent,   0, 0 },
            { "rspInfoW",	&infoW,		true,	eTkInteger, 0, 0 },
            { "rsmDly",		&stgCnt,	false,	eTkInteger, 0, 0 },
            { "multiRd",    &bMultiRd,  false,  eTkBoolean, 0, 0 },
            { "memSrc",     &memSrc,    false,  eTkIdent,   0, 0 },
			{ "rdType",		&rdType,	false,  eTkTypeStr, 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
            CPreProcess::ParseMsg(Error, "expected AddFunc( name, rspInfoW, rsmDly {, multiRd=false}, {, memSrc=\"coproc\"}{, rdType=uint64 } )");

		if (rdType != "uint64" && rdType != "uint32" && rdType != "uint16" && rdType != "uint8" &&
			rdType != "int64" && rdType != "int32" && rdType != "int16" && rdType != "int8")
            CPreProcess::ParseMsg(Error, "expected AddDst rdType parameter to be uint64, uint32, uint16, uint8, int64, int32, int16 or int8");

        if (memSrc != "coproc" && memSrc != "host")
            CPreProcess::ParseMsg(Error, "invalid value for memSrc (must be coproc or host)");

        if (m_pDsn->m_mifRdDstList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate memory read interface destination name '%s'", name.c_str());
        else {
            m_pDsn->m_mifRdDstList.insert(name);
            AddDst(m_pOpenMifRd, name, infoW, stgCnt, bMultiRd, memSrc, rdType);
        }

		m_pLex->GetNextTk();

    } else
        CPreProcess::ParseMsg(Error, "Expected an AddReadMem method");
}

void HtdFile::ParseMifWrMethods()
{
    if (m_pLex->GetTkString() == "AddSrc") {

        string name;
        string var;
        string field;
        bool bMultiWr=false;
        string srcIdx;
        string memDst="coproc";

        CParamList pavars[] = {
            { "name",		&name,		false,	eTkIdent,   0, 0 },
            { "var",		&var,		true,	eTkIdent,   0, 0 },
            { "field",		&field,		false,	eTkIdent,   0, 0 },
            { "multiWr",    &bMultiWr,  false,  eTkBoolean, 0, 0 },
            { "srcIdx",     &srcIdx,    false,  eTkIdent,   0, 0 },
            { "memDst",     &memDst,    false,  eTkIdent,   0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(pavars))
            CPreProcess::ParseMsg(Error, "expected AddWriteMem(...).AddSrc( name, var, field {, srcIdx }{, memDst=both } )");

        if (name.size() == 0)
            name = var;

        if (memDst != "coproc" && memDst != "host")
            CPreProcess::ParseMsg(Error, "invalid value for memDst (must be coproc or host)");

        if (!bMultiWr && srcIdx.size() > 0)
            CPreProcess::ParseMsg(Error, "Single quadword writes do not require srcIdx to be specified");

        if (srcIdx != "" && srcIdx != "varAddr1" && srcIdx != "varAddr2" && srcIdx != "varIdx1" && srcIdx != "varIdx2" && srcIdx != "fldIdx1" && srcIdx != "fldIdx2")
            CPreProcess::ParseMsg(Error, "invalid value for srcIdx (must be varAddr1, varAddr2, varIdx1, varIdx2, fldIdx1 or fldIdx2)");

        if (m_pDsn->m_mifWrSrcList.isInList(name))
            CPreProcess::ParseMsg(Error, "duplicate memory write interface source name '%s'", name.c_str());
        else {
            m_pDsn->m_mifWrSrcList.insert(name);
            AddSrc(m_pOpenMifWr, name, var, field, bMultiWr, srcIdx, memDst);
        }

		m_pLex->GetNextTk();

    } else
        CPreProcess::ParseMsg(Error, "Expected an AddWriteMem method");
}

void HtdFile::ParseIhmMethods()
{
    if (m_pLex->GetTkString() == "AddDst") {

        string var;
        string dataLsb="0";
        string addr1Lsb;
        string addr2Lsb;
        string idx1Lsb;
        string idx2Lsb;
        string field;
        string fldIdx1Lsb;
        string fldIdx2Lsb;
		bool bReadOnly = true;

        CParamList params[] = {
            { "var",		&var,		true,	eTkIdent , 0, 0 },
            { "dataLsb",	&dataLsb,	false,	eTkInteger , 0, 0 },
            { "addr1Lsb",	&addr1Lsb,	false,	eTkInteger , 0, 0 },
            { "addr2Lsb",	&addr2Lsb,	false,	eTkInteger , 0, 0 },
            { "idx1Lsb",	&idx1Lsb,	false,	eTkInteger , 0, 0 },
            { "idx2Lsb",	&idx2Lsb,	false,	eTkInteger , 0, 0 },
            { "field",		&field,		false,	eTkIdent , 0, 0 },
            { "fldIdx1Lsb",	&fldIdx1Lsb,	false,	eTkInteger, 0, 0 },
            { "fldIdx2Lsb",	&fldIdx2Lsb,	false,	eTkInteger , 0, 0 },
			{ "readOnly", &bReadOnly, false, eTkBoolean, 0, 0 },
            { 0, 0, 0, eTkUnknown, 0, 0}
        };

        if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected AddHostMsg(...).AddDst( var, {,dataLsb=0} {, addr1Lsb {, addr2Lsb }} {, idx1Lsb {, idx2Lsb }} {,readOnly=false})");

        AddMsgDst(m_pOpenIhm, var, dataLsb, addr1Lsb, addr2Lsb, idx1Lsb, idx2Lsb, field, fldIdx1Lsb, fldIdx2Lsb, bReadOnly);

		m_pLex->GetNextTk();

    } else
        CPreProcess::ParseMsg(Error, "Expected a AddHostMsg method");
}

bool HtdFile::ParseParameters(CParamList *params, bool * pNewGlobal)
{
    if (m_pLex->GetNextTk() != eTkLParen) {
        SkipTo(eTkRParen);
        return false;
    }

    bool bError = false;
    EToken tk;
    while ((tk = m_pLex->GetNextTk()) == eTkIdent) {
		if (pNewGlobal) *pNewGlobal = false;
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
		if (params[i].m_token == eTkString) {

	        tk = m_pLex->GetNextTk();
			if (tk != eTkString) {
				bError = true;
				CPreProcess::ParseMsg(Error, "expected a string for parameter '%s'", params[i].m_pName);
				break;
			}
            *(string *)params[i].m_pValue = m_pLex->GetTkString();
            tk = m_pLex->GetNextTk();

        } else if (params[i].m_token == eTkTypeStr) {

	        tk = m_pLex->GetNextTk();
			if (tk != eTkIdent) {
				bError = true;
				CPreProcess::ParseMsg(Error, "expected an type name for parameter '%s'", params[i].m_pName);
				break;
			}
			string typeStr = m_pLex->GetTkString();

			bool bStruct = typeStr == "union" || typeStr == "struct";

			if (bStruct) {
				tk = m_pLex->GetNextTk();
				if (tk != eTkIdent) {
					bError = true;
					CPreProcess::ParseMsg(Error, "expected an type name for parameter '%s'", params[i].m_pName);
					break;
				}
				typeStr = m_pLex->GetTkString();
			}
 
			while ((tk = m_pLex->GetNextTk()) == eTkIdent) {
				if (typeStr == "signed") {
					if (m_pLex->GetTkString() == "long" || m_pLex->GetTkString() == "int" || m_pLex->GetTkString() == "short" || m_pLex->GetTkString() == "char")
						typeStr = m_pLex->GetTkString();
					else {
						bError = true;
						CPreProcess::ParseMsg(Error, "unexpected value for parameter '%s'", params[i].m_pName);
					}
				} else if (typeStr == "unsigned") {
					if (m_pLex->GetTkString() == "long" || m_pLex->GetTkString() == "int" || m_pLex->GetTkString() == "short" || m_pLex->GetTkString() == "char")
						typeStr += " " + m_pLex->GetTkString();
					else {
						bError = true;
						CPreProcess::ParseMsg(Error, "unexpected value for parameter '%s'", params[i].m_pName);
					}
				} else if (typeStr == "long") {
					if (m_pLex->GetTkString() == "long" || m_pLex->GetTkString() == "unsigned")
						typeStr += " " + m_pLex->GetTkString();
					else {
						bError = true;
						CPreProcess::ParseMsg(Error, "unexpected value for parameter '%s'", params[i].m_pName);
					}
				} else if (typeStr == "unsigned long") {
					if (m_pLex->GetTkString() == "long")
						typeStr += " " + m_pLex->GetTkString();
					else {
						bError = true;
						CPreProcess::ParseMsg(Error, "unexpected value for parameter '%s'", params[i].m_pName);
					}
				} else if (typeStr == "long long") {
					if (m_pLex->GetTkString() == "unsigned")
						typeStr = "unsigned long long";
					else {
						bError = true;
						CPreProcess::ParseMsg(Error, "unexpected value for parameter '%s'", params[i].m_pName);
					}
				} else {
					bError = true;
					CPreProcess::ParseMsg(Error, "unexpected value for parameter '%s'", params[i].m_pName);
				}
			}

			if (typeStr == "char") {
				static bool bChar = false;
				if (!bChar) {
					bChar = true;
					CPreProcess::ParseMsg(Warning, "found type with compiler dependent width, replaced int8_t for all occurances of char");
				}
				typeStr = "int8_t";
			} else if (typeStr == "short") {
				static bool bShort = false;
				if (!bShort) {
					bShort = true;
					CPreProcess::ParseMsg(Warning, "found type with compiler dependent width, replaced int16_t for all occurances of short");
				}
				typeStr = "int16_t";
			} else if (typeStr == "int") {
				static bool bInt = false;
				if (!bInt) {
					bInt = true;
					CPreProcess::ParseMsg(Warning, "found type with compiler dependent width, replaced int32_t for all occurances of int");
				}
				typeStr = "int32_t";
			} else if (typeStr == "long") {
				static bool bLong = false;
				if (!bLong) {
					bLong = true;
					CPreProcess::ParseMsg(Warning, "found type with compiler dependent width, replaced int64_t for all occurances of long");
				}
				typeStr = "int64_t";
			} else if (typeStr == "long long") {
				static bool bLongLong = false;
				if (!bLongLong) {
					bLongLong = true;
					CPreProcess::ParseMsg(Warning, "found type with compiler dependent width, replaced int64_t for all occurances of long long");
				}
				typeStr = "int64_t";
			} else if (typeStr == "unsigned char") {
				static bool bUchar = false;
				if (!bUchar) {
					bUchar = true;
					CPreProcess::ParseMsg(Warning, "found type with compiler dependent width, replaced uint8_t for all occurances of unsigned char");
				}
				typeStr = "uint8_t";
			} else if (typeStr == "unsigned short") {
				static bool bUshort = false;
				if (!bUshort) {
					bUshort = true;
					CPreProcess::ParseMsg(Warning, "found type with compiler dependent width, replaced uint16_t for all occurances of unsigned short");
				}
				typeStr = "uint16_t";
			} else if (typeStr == "unsigned int") {
				static bool bUint = false;
				if (!bUint) {
					bUint = true;
					CPreProcess::ParseMsg(Warning, "found type with compiler dependent width, replaced uint32_t for all occurances of unsigned int");
				}
				typeStr = "uint32_t";
			} else if (typeStr == "unsigned long") {
				static bool bUlong = false;
				if (!bUlong) {
					bUlong = true;
					CPreProcess::ParseMsg(Warning, "found type with compiler dependent width, replaced uint64_t for all occurances of unsigned long");
				}
				typeStr = "uint64_t";
			} else if (typeStr == "unsigned long long") {
				static bool bUlongLong = false;
				if (!bUlongLong) {
					bUlongLong = true;
					CPreProcess::ParseMsg(Warning, "found type with compiler dependent width, replaced uint64_t for all occurances of unsigned long long");
				}
				typeStr = "uint64_t";
			}
			
			if (bStruct && (typeStr == "uint8_t" || typeStr == "uint16_t" || typeStr == "uint32_t" || typeStr == "uint64_t" ||
				typeStr == "int8_t" || typeStr == "int16_t" || typeStr == "int32_t" || typeStr == "int64_t" ||
				typeStr == "bool" || IsInTypeList(typeStr)))
			{
				CPreProcess::ParseMsg(Error, "unexpected struct/union with type identifier '%s'", typeStr.c_str());
			}

           *(string *)params[i].m_pValue = typeStr;

        } else if (params[i].m_token == eTkIdent) {

	        tk = m_pLex->GetNextTk();
			if (tk != eTkIdent && tk != eTkString) {
				bError = true;
				CPreProcess::ParseMsg(Error, "expected an identifier for parameter '%s'", params[i].m_pName);
				break;
			}
			if (tk == eTkString) {
				static bool bWarn = true;
				if (bWarn) {
					CPreProcess::ParseMsg(Warning, "use of quotes for an identifier is deprecated");
					bWarn = false;
				}
			}
            *(string *)params[i].m_pValue = m_pLex->GetTkString();
            tk = m_pLex->GetNextTk();

       } else if (params[i].m_token == eTkIdentRef) {

		    string paramStr = m_pLex->GetExprStr(',');
			char const * pPos = paramStr.c_str();
			while (isspace(*pPos)) pPos += 1;

			for (;;) {
				if (!isalpha(*pPos) && *pPos != '_') {
					bError = true;
					CPreProcess::ParseMsg(Error, "expected an identifier reference for parameter '%s'", params[i].m_pName);
					break;
				}

				// an identifier
				while (isalpha(*pPos) || isdigit(*pPos) || *pPos == '_') pPos += 1;

				while (isspace(*pPos)) pPos += 1;

				// check for an index
				while (*pPos == '[') {
					while (*pPos != ']' && *pPos != '\0') pPos++;
					if (*pPos != ']') {
						bError = true;
						CPreProcess::ParseMsg(Error, "expected an identifier index for parameter '%s'", params[i].m_pName);
						break;
					}
					pPos += 1;
					while (isspace(*pPos)) pPos += 1;
				}

				// check for period
				if (*pPos != '.')
					break;
				pPos += 1;
				while (isspace(*pPos)) pPos += 1;
			}

			if (*pPos != '\0')
				CPreProcess::ParseMsg(Error, "parameter format error, '%s'", params[i].m_pName);

			*(string *)params[i].m_pValue = paramStr;
            tk = m_pLex->GetNextTk();

        } else if (params[i].m_token == eTkInteger || params[i].m_token == eTkParamStr) {

			*(string *)params[i].m_pValue = m_pLex->GetExprStr(',');
            tk = m_pLex->GetNextTk();

        } else if (params[i].m_token == eTkIntList) {

			vector<int> *pIntList = (vector<int> *)params[i].m_pValue;

	        tk = m_pLex->GetNextTk();

			if (tk == eTkLBrace) {
				tk = m_pLex->GetNextTk();

				for (;;) {
					if (!ParseIntRange(pIntList)) {
						CPreProcess::ParseMsg(Error, "expected an integer or integer range for parameter '%s'", params[i].m_pName);
						break;
					}

					tk = m_pLex->GetTk();

					if (tk == eTkComma)
						tk = m_pLex->GetNextTk();
					else
						break;
				}

				if (tk != eTkRBrace)
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
        } else {
			assert(0);
            *(string *)params[i].m_pValue = m_pLex->GetTkString();
            tk = m_pLex->GetNextTk();
        }

        if (tk != eTkComma)
            break;
    }

    if (!bError && (pNewGlobal == 0 || *pNewGlobal == false)) {
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

    return true;
}

bool HtdFile::ParseIntRange(vector<int> * pIntList)
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

bool HtdFile::ParseRandomParameter(CParamList * pParam)
{
    EToken tk = m_pLex->GetNextTk();
	string valueStr;

	if (pParam->m_token == eTkIdent) {
		if (tk != eTkIdent && tk != eTkString) {
			CPreProcess::ParseMsg(Error, "expected an identifier for parameter '%s'", pParam->m_pName);
			SkipTo(eTkRParen);
			return false;
		}

		string defParam = m_pLex->GetTkString();

		if (m_pLex->GetNextTk() != eTkColon) {
			CPreProcess::ParseMsg(Error, "unexpected format for random test parameter '%s'", pParam->m_pName);
			SkipTo(eTkRParen);
			return false;
		}

		vector<string> rndParamList;

		for (;;) {
			tk = m_pLex->GetNextTk();

			if (tk != eTkIdent && tk != eTkString) {
				CPreProcess::ParseMsg(Error, "expected an identifier for parameter '%s'", pParam->m_pName);
				SkipTo(eTkRParen);
				return false;
			}

			rndParamList.push_back(m_pLex->GetTkString());

			tk = m_pLex->GetNextTk();

			if (tk != eTkVbar)
				break;
		}

		if (g_appArgs.IsRndTestEnabled()) {
			int rndIdx = g_appArgs.GetRtRndIdx(rndParamList.size());
			valueStr = rndParamList[rndIdx];
			*(string *)pParam->m_pValue = valueStr;
		} else
			*(string *)pParam->m_pValue = defParam;

	} else if (pParam->m_token == eTkInteger) {

		if (tk != eTkInteger) {
			CPreProcess::ParseMsg(Error, "expected an integer for parameter '%s'", pParam->m_pName);
			SkipTo(eTkRParen);
			return false;
		}

		string defParam = m_pLex->GetTkString();

		if (m_pLex->GetNextTk() != eTkColon) {
			CPreProcess::ParseMsg(Error, "unexpected format for random test parameter '%s'", pParam->m_pName);
			SkipTo(eTkRParen);
			return false;
		}

		vector<string> rndParamList;

		for (;;) {
			tk = m_pLex->GetNextTk();

			if (tk != eTkInteger) {
				CPreProcess::ParseMsg(Error, "expected an integer for parameter '%s'", pParam->m_pName);
				SkipTo(eTkRParen);
				return false;
			}

			string intStr = m_pLex->GetTkString();

			tk = m_pLex->GetNextTk();

			if (tk == eTkMinus) {
				tk = m_pLex->GetNextTk();

				if (tk != eTkInteger) {
					CPreProcess::ParseMsg(Error, "expected an integer for parameter '%s'", pParam->m_pName);
					SkipTo(eTkRParen);
					return false;
				}

				int n1 = atoi(intStr.c_str());
				int n2 = atoi(m_pLex->GetTkString().c_str());

				if (n1 > n2) {
					CPreProcess::ParseMsg(Error, "illegal integer range (%d-%d) for parameter '%s'", n1, n2, pParam->m_pName);
					SkipTo(eTkRParen);
					return false;
				}

				char buf[64];
				for(int i = n1; i <= n2; i += 1) {
					sprintf(buf, "%d", i);
					string str = buf;
					rndParamList.push_back(str);
				}

				tk = m_pLex->GetNextTk();
			} else
				rndParamList.push_back(m_pLex->GetTkString());

			if (tk != eTkVbar)
				break;
		}

		if (g_appArgs.IsRndTestEnabled()) {
			int rndIdx = g_appArgs.GetRtRndIdx(rndParamList.size());
			valueStr = rndParamList[rndIdx];
			*(string *)pParam->m_pValue = valueStr;
		} else
			*(string *)pParam->m_pValue = defParam;

    } else if (pParam->m_token == eTkBoolean) {

		if (tk != eTkIdent || m_pLex->GetTkString() != "true" && m_pLex->GetTkString() != "false") {
			CPreProcess::ParseMsg(Error, "expected true or false for parameter '%s'", pParam->m_pName);
			SkipTo(eTkRParen);
			return false;
		}

		bool bDefParam = m_pLex->GetTkString() == "true";

		if (m_pLex->GetNextTk() != eTkColon) {
			CPreProcess::ParseMsg(Error, "unexpected format for random test parameter '%s'", pParam->m_pName);
			SkipTo(eTkRParen);
			return false;
		}

		vector<bool> rndParamList;

		for (;;) {
			tk = m_pLex->GetNextTk();

			if (tk != eTkIdent || m_pLex->GetTkString() != "true" && m_pLex->GetTkString() != "false") {
				CPreProcess::ParseMsg(Error, "expected true or false for parameter '%s'", pParam->m_pName);
				SkipTo(eTkRParen);
				return false;
			}

			rndParamList.push_back(m_pLex->GetTkString() == "true");

			tk = m_pLex->GetNextTk();

			if (tk != eTkVbar)
				break;
		}

		if (g_appArgs.IsRndTestEnabled()) {
			int rndIdx = g_appArgs.GetRtRndIdx(rndParamList.size());
            *(bool *)pParam->m_pValue = rndParamList[rndIdx];
			valueStr = rndParamList[rndIdx] ? "true" : "false";
		} else
            *(bool *)pParam->m_pValue = bDefParam;
	}

	if (g_appArgs.IsRndTestEnabled())
		printf("   Rnd %s: %s\n", pParam->m_pName, valueStr.c_str());

	return true;
}
