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
					m_pParseModule = m_pOpenMod->m_pModule;	// make current module accessable to editor
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
	m_pOpenRecord = m_pDsnInfo->AddStruct(structName, true, bUnion, scope, !bIsInputFile, "");

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
		bool bSpanningFieldForced = false;

		if (m_pLex->GetTk() == eTkPound && m_pLex->GetNextTk() == eTkIdent && m_pLex->GetTkString() == "pragma") {
			if (m_pLex->GetNextTk() != eTkIdent || m_pLex->GetTkString() != "htl" ||
				m_pLex->GetNextTk() != eTkIdent ||
				!((bAtomicInc = (m_pLex->GetTkString() == "atomic_inc")) ||
				(bAtomicSet = (m_pLex->GetTkString() == "atomic_set")) ||
				(bAtomicAdd = (m_pLex->GetTkString() == "atomic_add")) ||
				(bSpanningFieldForced = (m_pLex->GetTkString() == "spanning_field"))) ||
				m_pLex->GetNextTk() != eTkPound) {
				CPreProcess::ParseMsg(Error, "expected #pragma htl {atomic_inc | atomic_set | atomic_add | spanning_field}");
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

			// anonymous struct/union

			m_pLex->GetNextTk();

			CRecord * pSavedOpenRecord = m_pOpenRecord;

			CField * pField = m_pOpenRecord->AddStructField(new CRecord("", bUnion), "");

			// Ensure spanning field can't be used on anonymous struct/union
			if (bSpanningFieldForced) {
				CPreProcess::ParseMsg(Error, "spanning_field pragma is invalid on anonymous struct/union members");
			}

			m_pOpenRecord = pField->m_pType->AsRecord();

			ParseFieldList();

			m_pOpenRecord = pSavedOpenRecord;

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

			for (;;) {
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
					dimenList.push_back(ParseArrayDimen());

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
					CType * foundType = m_pDsnInfo->FindType(type);
					if (foundType->IsRecord() && (foundType->AsRecord()->m_bUnion && bSpanningFieldForced)) {
						CPreProcess::ParseMsg(Error, "spanning_field pragma is invalid on unions themselves");
					}
					// NOTE! Found an issue where this is not supported (although it's technically possible)
					//   We're not using it now, so I'm catching this as an error, though it could be implemented in the future
					//   CB 4/11/17
					if (foundType->IsRecord() && (!foundType->AsRecord()->m_bUnion && bSpanningFieldForced)) {
						CPreProcess::ParseMsg(Error, "spanning_field pragma is invalid on structs themselves");
					}
					m_pOpenRecord->AddStructField(foundType, name, bitWidth, base, dimenList,
						true, true, false, false, HtdFile::eDistRam, atomicMask, bSpanningFieldForced);
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

	string dimen = string(pStr, pEnd - pStr);

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

	if (name == "struct" || name == "union") {
		string recordType = name;
		if (m_pLex->GetNextTk() != eTkIdent) {
			CPreProcess::ParseMsg(Error, "expected builtin type, typedef name or struct/union name");
			SkipTo(eTkSemi);
			m_pLex->GetNextTk();
			return errorStr;
		}

		name = m_pLex->GetTkString();

		m_pLex->GetNextTk();

		return name;
	}

	if (name == "void" || IsInTypeList(name) || IsInStructList(name)) {
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

		m_pLex->SetLinePos(pStr + 1);

		m_pLex->GetNextTk();
		return typeStr;
	}

	if (strncmp(name.c_str(), "ht_uint", 7) == 0) {
		char * pEnd;
		int width = strtol(name.c_str() + 7, &pEnd, 10);

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
		int width = strtol(name.c_str() + 6, &pEnd, 10);

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

	while (m_pLex->GetTk() == eTkIdent) {

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
			break;
		}

		if (idx < 12)
			continue;

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

		if (typeStr.size() == 0 && !bType) {
			bType = true;
			typeStr = name;
			m_pLex->GetNextTk();
		}

		break;
	}

	if (typeStr.size() == 0 || !bType) {
		CPreProcess::ParseMsg(Error, "expected builtin type, typedef name or struct/union name");
		SkipTo(eTkSemi);
		m_pLex->GetNextTk();
		return errorStr;
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

	return typeStr;
}

void HtdFile::ParseDsnInfoMethods()
{
	if (m_pLex->GetTkString() == "AddModule") {
		string name;
		string clock = "1x";
		string htIdW = "0";
		string reset;
		bool bPause = false;

		CParamList params[] = {
				{ "name", &name, true, ePrmIdent, 0, 0 },
				{ "clock", &clock, false, ePrmIdent, 0, 0 },
				{ "htIdW", &htIdW, false, ePrmInteger, 0, 0 },
				{ "reset", &reset, false, ePrmIdent, 0, 0 },
				{ "pause", &bPause, false, ePrmBoolean, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected dsnInfo.AddModule( name, {, clock=1x }{, htIdW=0 }{, pause=false}{, reset=\"\" })");

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
			m_pOpenMod->m_pModule = m_pDsnInfo->AddModule(name, clkRate);
			m_pParseMethod = &HtdFile::ParseModuleMethods;
			m_pDsn->m_modInstParamList.clear();

			AddThreads(m_pOpenMod->m_pModule, htIdW, reset, bPause);
		}

		// Insert name_HTID_W for current module if it does not exist
		CHtString modName = name;
		string htIdMacro = modName.Upper() + "_HTID_W";

		if (m_pLex->GetMacroTbl().Find(htIdMacro) == 0) {
			CPreProcess::CMacro * pDefine = m_pLex->GetMacroTbl().Insert(htIdMacro, htIdW);
			pDefine->SetIsFromIncludeFile(false);
			pDefine->SetLineInfo(CPreProcess::m_lineInfo);
		}

		m_pLex->GetNextTk();

	} else
		CPreProcess::ParseMsg(Error, "Expected DsnInfo method");
}

void HtdFile::ParseModuleMethods()
{
	if (m_pLex->GetTkString() == "AddInstParam") {

		string name;
		string default_;

		CParamList params[] = {
			{ "name", &name, true, ePrmParamStr, 0, 0 },
			{ "default", &default_, false, ePrmParamStr, 0, 0 },
			{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddInstParam( name=<name>, default=<value> )");

		if (m_pDsn->m_modInstParamList.isInList(name))
			CPreProcess::ParseMsg(Error, "duplicate instParam name '%s'", name.c_str());
		else {
			m_pDsn->m_modInstParamList.insert(name);
			m_pDsnInfo->AddModInstParam(m_pOpenMod->m_pModule, name, default_);
		}

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddReadMem") {

		string queueW;
		string rspGrpId;
		string rspCntW;
		string rspGrpW;
		bool bMaxBw = false;
		string pause;
		string poll = "false";

		CParamList params[] = {
				{ "maxBw", &bMaxBw, false, ePrmBoolean, 0, 0 },
				{ "rspGrpW", &rspGrpW, false, ePrmInteger, 0, 0 },
				{ "pause", &pause, false, ePrmIdent, 0, 0 },
				{ "poll", &poll, false, ePrmIdent, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddReadMem( {maxBw=false, }{ rspGrpW } {, pause=true } {, poll=false } )");

		// force depricated parameters
		queueW = "5";
		rspCntW = "";

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

		m_pOpenMifRd = m_pDsnInfo->AddReadMem(m_pOpenMod->m_pModule, queueW, rspGrpId, rspGrpW, rspCntW, bMaxBw, bPause, bPoll);
		m_pParseMethod = &HtdFile::ParseMifRdMethods;
		m_pDsn->m_mifRdDstList.clear();

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddWriteMem") {

		string queueW;
		string rspGrpId;
		string rspCntW;
		string rspGrpW;
		bool bMaxBw = false;
		bool bPause = true;
		bool bReqPause = false;
		bool bPoll = false;

		CParamList params[] = {
				{ "rspGrpW", &rspGrpW, false, ePrmInteger, 0, 0 },
				{ "pause", &bPause, false, ePrmBoolean, 0, 0 },
				{ "reqPause", &bReqPause, false, ePrmBoolean, 0, 0 },
				{ "poll", &bPoll, false, ePrmBoolean, 0, 0 },
				{ "maxBw", &bMaxBw, false, ePrmBoolean, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddWriteMem( {, rspGrpW } {, pause=true} {, poll=false} {, reqPause=false}{, maxBw=false} )");

		// force depricated parameters
		queueW = "5";
		rspCntW = "";

		m_pOpenMifWr = m_pDsnInfo->AddWriteMem(m_pOpenMod->m_pModule, queueW, rspGrpId, rspGrpW, rspCntW, bMaxBw, bPause, bPoll, bReqPause);

		{
			// Add uint64_t wrSrc
			string name;
			CType * pType = &g_uint64;
			string var;
			string memDst;

			m_pDsn->m_mifWrSrcList.insert(name);
			m_pDsnInfo->AddSrc(m_pOpenMifWr, name, pType, var, memDst, 0);
		}

		m_pParseMethod = &HtdFile::ParseMifWrMethods;
		m_pDsn->m_mifWrSrcList.clear();

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddCall") {

		string modEntry;
		string instName;
		string callName;
		string call;
		string fork = "false";
		string queueW = "5";
		string dest = "auto";
		vector<pair<string, string> > paramPairList;

		CParamList params[] = {
			{ "func", &modEntry, false, ePrmIdent, 0, 0, 1 },
			{ "modEntry", &modEntry, true, ePrmIdent, 0, 0 },
			{ "instName", &instName, false, ePrmIdent, 0, 0 },
			{ "callName", &callName, false, ePrmIdent, 0, 0 },
			{ "call", &call, false, ePrmIdent, 0, 0 },
			{ "fork", &fork, false, ePrmIdent, 0, 0 },
			{ "queueW", &queueW, false, ePrmInteger, 0, 0 },
			{ "dest", &dest, false, ePrmIdent, 0, 0 },
			{ "", &paramPairList, false, ePrmParamLst, 0, 0, -1 },
			{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddCall( func {, callName=<name>} {, instName=<name>} {, call=true} {, fork=false} {, queueW=5 } {, dest=auto } )");

		if (callName.size() == 0)
			callName = modEntry;

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

		if (dest != "auto" && dest != "user")
			CPreProcess::ParseMsg(Error, "unsupported parameter value for dest");

		if (m_pOpenMod->m_callList.isInList(callName))
			CPreProcess::ParseMsg(Error, "duplicate call name '%s'", callName.c_str());
		else {
			m_pOpenMod->m_callList.insert(callName);
			m_pOpenCall = m_pDsnInfo->AddCall(m_pOpenMod->m_pModule, modEntry, callName, instName, bCall, bFork, queueW, dest, paramPairList);
			m_pDsn->m_callInstParamList.clear();
		}

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddTransfer") {

		string modEntry;
		string instName;
		string callName;
		string queueW = "5";

		CParamList params[] = {
			{ "func", &modEntry, false, ePrmIdent, 0, 0, 1 },
			{ "modEntry", &modEntry, true, ePrmIdent, 0, 0 },
			{ "instName", &instName, false, ePrmIdent, 0, 0 },
			{ "xferName", &callName, false, ePrmIdent, 0, 0 },
			{ "queueW", &queueW, false, ePrmInteger, 0, 0 },
			{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddTransfer( func {, xferName=<name>} {, instName=<name>} {, queueW=5 } )");

		if (callName.size() == 0)
			callName = modEntry;

		if (m_pOpenMod->m_callList.isInList(callName))
			CPreProcess::ParseMsg(Error, "duplicate call name '%s'", modEntry.c_str());
		else {
			m_pOpenMod->m_callList.insert(callName);
			m_pDsnInfo->AddXfer(m_pOpenMod->m_pModule, modEntry, callName, instName, queueW);
		}

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddEntry") {

		string func;
		string inst;
		string reserve;
		bool bHost = false;
		static bool bAddEntryHost = false;

		CParamList params[] = {
				{ "func", &func, true, ePrmIdent, 0, 0 },
				{ "instr", &inst, true, ePrmIdent, 0, 0 },
				{ "host", &bHost, false, ePrmBoolean, 0, 0 },
				{ "reserve", &reserve, false, ePrmInteger, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <mod>.AddEntry( func, instr {, reserve }{, host=false } )");

		if (bHost && g_appArgs.GetEntryName().size() > 0)
			CPreProcess::ParseMsg(Error, "specifing both htl option -en and AddEntry(host=true) not supported");

		if (bHost && bAddEntryHost)
			CPreProcess::ParseMsg(Error, "AddEntry(host=true) was specified on multiple AddEntry commands");

		bAddEntryHost |= bHost;

		if (m_pOpenMod->m_entryList.isInList(func))
			CPreProcess::ParseMsg(Error, "duplicate entry function name '%s'", func.c_str());
		else {
			m_pOpenMod->m_entryList.insert(func);
			m_pOpenEntry = m_pDsnInfo->AddEntry(m_pOpenMod->m_pModule, func, inst, reserve, bHost);
			m_pParseMethod = &HtdFile::ParseEntryMethods;
			m_pDsn->m_entryParamList.clear();
		}

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddReturn") {

		string func;

		CParamList params[] = {
				{ "func", &func, true, ePrmIdent, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddReturn( func )");

		if (m_pOpenMod->m_returnList.isInList(func))
			CPreProcess::ParseMsg(Error, "duplicate return function name '%s'", func.c_str());
		else {
			m_pOpenMod->m_returnList.insert(func);
			m_pOpenReturn = m_pDsnInfo->AddReturn(m_pOpenMod->m_pModule, func);
			m_pParseMethod = &HtdFile::ParseReturnMethods;
			m_pDsn->m_returnParamList.clear();
		}

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddGlobal") {

		CParamList params[] = {
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params)) {
			CPreProcess::ParseMsg(Error, "expected <mod>.AddGlobal()");
		}

		m_pOpenGlobal = m_pOpenMod->m_pModule->AddGlobal();
		m_pParseMethod = &HtdFile::ParseGlobalMethods;

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddInstr") {

		string name;

		CParamList params[] = {
				{ "name", &name, true, ePrmIdent, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <mod>.AddInstr( name=\"\" )");

		if (m_pOpenMod->m_instrList.isInList(name))
			CPreProcess::ParseMsg(Error, "duplicate instruction name '%s'", name.c_str());
		else {
			m_pOpenMod->m_instrList.insert(name);
			AddInstr(m_pOpenMod->m_pModule, name);
		}

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddBarrier") {

		string name;
		string barIdW;

		CParamList params[] = {
				{ "name", &name, false, ePrmIdent, 0, 0 },
				{ "barIdW", &barIdW, false, ePrmInteger, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddBarrier( { name=<barrierName>, } { barIdW=0 } );");

		string barrierName = name.size() == 0 ? "unnamed barrier" : name;

		if (m_pOpenMod->m_barrierList.isInList(barrierName))
			CPreProcess::ParseMsg(Error, "duplicate barrier name '%s'", barrierName.c_str());
		else {
			m_pOpenMod->m_barrierList.insert(barrierName);
			AddBarrier(m_pOpenMod->m_pModule, name, barIdW);
		}

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddReadStream") {

		string name;
		CType * pType;
		string strmBw;
		string elemCntW = "32";
		string strmCnt;
		string memSrc = "coproc";
		vector<int> memPort;
		string access = "all";
		bool paired = false;
		bool bClose = false;
		CType * pTag = 0;
		string rspGrpW;

		CParamList params[] = {
				{ "name", &name, false, ePrmIdent, 0, 0 },
				{ "type", &pType, true, ePrmType, 0, 0 },
				{ "strmBw", &strmBw, false, ePrmInteger, 0, 0 },
				{ "elemCntW", &elemCntW, false, ePrmInteger, 0, 0 },
				{ "strmCnt", &strmCnt, false, ePrmInteger, 0, 0 },
				{ "close", &bClose, false, ePrmBoolean, 0, 0 },
				{ "memSrc", &memSrc, false, ePrmIdent, 0, 0 },
				{ "memPort", &memPort, false, ePrmIntList, 0, 0 },
				{ "tag", &pTag, false, ePrmType, 0, 0 },
				//{ "rspGrpW", &rspGrpW, false, ePrmInteger, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
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
			m_pDsnInfo->AddStream(m_pOpenMod->m_pModule, bRead, name, pType, strmBw, elemCntW,
				strmCnt, memSrc, memPort, access, pipeLen, paired, bClose, pTag, rspGrpW);
		}

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddWriteStream") {

		string name;
		CType * pType;
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
				{ "name", &name, false, ePrmIdent, 0, 0 },
				{ "type", &pType, true, ePrmType, 0, 0 },
				{ "elemCntW", &elemCntW, false, ePrmInteger, 0, 0 },
				{ "strmCnt", &strmCnt, false, ePrmInteger, 0, 0 },
				{ "close", &bClose, false, ePrmBoolean, 0, 0 },
				{ "memDst", &memDst, false, ePrmIdent, 0, 0 },
				{ "memPort", &memPort, false, ePrmIntList, 0, 0 },
				{ "reserve", &reserve, false, ePrmInteger, 0, 0 },
				{ "rspGrpW", &rspGrpW, false, ePrmInteger, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
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
			CType * pTag = 0;
			m_pDsnInfo->AddStream(m_pOpenMod->m_pModule, bRead, name, pType, strmBw, elemCntW,
				strmCnt, memDst, memPort, access, reserve, paired, bClose, pTag, rspGrpW);
		}

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddStencilBuffer") {

		string name;
		CType * pType;
		vector<int> gridSize;
		vector<int> stencilSize;
		string pipeLen;

		CParamList params[] = {
				{ "name", &name, false, ePrmIdent, 0, 0 },
				{ "type", &pType, true, ePrmType, 0, 0 },
				{ "gridSize", &gridSize, true, ePrmIntList, 0, 0 },
				{ "stencilSize", &stencilSize, true, ePrmIntList, 0, 0 },
				{ "pipeLen", &pipeLen, false, ePrmInteger, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddStencilBuffer( {name,} type, gridSize, stencilSize {, pipeLen}");

		if (m_pOpenMod->m_stencilBufferList.isInList(name))
			CPreProcess::ParseMsg(Error, "duplicate stencil buffer name '%s'", name.c_str());
		else {
			m_pOpenMod->m_stencilBufferList.insert(name);
			m_pDsnInfo->AddStencilBuffer(m_pOpenMod->m_pModule, name, pType, gridSize, stencilSize, pipeLen);
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

		m_pDsnInfo->AddStruct(inStructName, true, false, eModule, false, "");
		m_pDsnInfo->AddStruct(outStructName, true, false, eModule, false, "");

	} else if (m_pLex->GetTkString() == "AddMsgIntf") {

		string dir;
		string name;
		CType * pType;
		string dimen;
		string fanin;
		string fanout;
		string queueW = "0";
		string reserve;
		bool autoConn = true;

		CParamList params[] = {
				{ "dir", &dir, true, ePrmIdent, 0, 0 },
				{ "name", &name, true, ePrmIdent, 0, 0 },
				{ "type", &pType, true, ePrmType, 0, 0 },
				{ "dimen", &dimen, false, ePrmInteger, 0, 0 },
				{ "fanin", &fanin, false, ePrmInteger, 0, 0 },
				{ "fanout", &fanout, false, ePrmInteger, 0, 0 },
				{ "queueW", &queueW, false, ePrmInteger, 0, 0 },
				{ "reserve", &reserve, false, ePrmInteger, 0, 0 },
				{ "autoConn", &autoConn, false, ePrmBoolean, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
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
			m_pDsnInfo->AddMsgIntf(m_pOpenMod->m_pModule, name, dir, pType, dimen, fan, queueW, reserve, autoConn);
		}

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddHostMsg") {

		string dir;
		string name;

		CParamList params[] = {
				{ "dir", &dir, true, ePrmIdent, 0, 0 },
				{ "name", &name, true, ePrmIdent, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
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
				m_pOpenIhm = AddHostMsg(m_pOpenMod->m_pModule, eMsgDir, name);
				m_pParseMethod = &HtdFile::ParseIhmMethods;
			} else if (eMsgDir == Outbound) {
				AddHostMsg(m_pOpenMod->m_pModule, eMsgDir, name);
			}
		}

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddHostData") {

		string dir;
		bool bMaxBw = false;

		CParamList params[] = {
				{ "dir", &dir, true, ePrmIdent, 0, 0 },
				{ "maxBw", &bMaxBw, false, ePrmBoolean, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
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
			AddHostData(m_pOpenMod->m_pModule, eMsgDir, bMaxBw);

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddPrivate") {

		CParamList params[] = {
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddPrivate( )");

		m_pOpenRecord = m_pOpenMod->m_pModule->AddPrivate();
		m_pParseMethod = &HtdFile::ParsePrivateMethods;
		m_pDsn->m_privateFieldList.clear();

		m_pOpenMod = 0;

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddStage") {

		string privWrStg;
		string execStg = "1";

		CParamList params[] = {
				{ "privWrStg", &privWrStg, false, ePrmInteger, 0, 0 },
				{ "execStg", &execStg, false, ePrmInteger, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddStage( { execStg=1 } {, privWrStg=<execStg> } )");

		if (privWrStg.size() == 0)
			privWrStg = execStg;

		m_pOpenStage = m_pDsnInfo->AddStage(m_pOpenMod->m_pModule, privWrStg, execStg);
		m_pParseMethod = &HtdFile::ParseStageMethods;

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddShared") {

		string reset = "true";

		CParamList params[] = {
				{ "reset", &reset, false, ePrmIdent, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddShared( { reset=true } )");

		if (reset != "true" && reset != "false")
			CPreProcess::ParseMsg(Error, "expected reset parameter to be true or false");

		m_pOpenRecord = m_pDsnInfo->AddShared(m_pOpenMod->m_pModule, reset == "true");
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
				{ "groupW", &groupW, true, ePrmInteger, 1, 0 },
				{ "reset", &reset, false, ePrmIdent, 0, 0 },
				{ "pause", &bPause, false, ePrmBoolean, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddGroup( {, groupW=5 }{, pause=false }{, reset=\"\" } )");

		AddThreads(m_pOpenMod->m_pModule, groupW, reset, bPause);

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddFunction") {

		string name;
		CType * pType;

		CParamList params[] = {
				{ "name", &name, true, ePrmIdent, 0, 0 },
				{ "type", &pType, true, ePrmType, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddFunction( type, name )");
		else {
			if (m_pOpenMod->m_functionList.isInList(name))
				CPreProcess::ParseMsg(Error, "duplicate function name '%s'", name.c_str());
			else {
				m_pOpenMod->m_functionList.insert(name);
				m_pOpenFunction = m_pDsnInfo->AddFunction(m_pOpenMod->m_pModule, pType, name);
				m_pParseMethod = &HtdFile::ParseFunctionMethods;
				m_pDsn->m_functionParamList.clear();
			}

			m_pLex->GetNextTk();
		}

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

		AddScPrim(m_pOpenMod->m_pModule, scPrimDecl, scPrimCall);

	} else if (m_pLex->GetTkString() == "AddTrace") {

		string name;

		CParamList params[] = {
				{ "name", &name, true, ePrmIdent, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddTrace( name )");

		if (m_pOpenMod->m_traceList.isInList(name))
			CPreProcess::ParseMsg(Error, "duplicate trace name '%s'", name.c_str());
		else {
			m_pOpenMod->m_traceList.insert(name);
			AddTrace(m_pOpenMod->m_pModule, name);
		}

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddDefine") {	// define in module

		static bool bWarn = true;
		if (bWarn == true) {
			CPreProcess::ParseMsg(Warning, "AddDefine has been deprecated, use C/C++ #define within HTD file");
			bWarn = false;
		}

		string name;
		string value;
		string scope = "unit";

		CParamList params[] = {
				{ "name", &name, true, ePrmIdent, 0, 0 },
				{ "value", &value, true, ePrmInteger, 0, 0 },
				{ "scope", &scope, false, ePrmIdent, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected dsnInfo.AddDefine( name, value {, scope } )");

		if (scope.compare("host") != 0 && scope.compare("unit") != 0)
			CPreProcess::ParseMsg(Error, "invalid scope '%s'   only "
			"'host' and 'unit' allowed\n", scope.c_str());
		else if (m_pOpenMod->m_defineList.isInList(name))
			CPreProcess::ParseMsg(Error, "duplicate define name '%s'", name.c_str());
		else {
			m_pOpenMod->m_defineList.insert(name);
			AddDefine(m_pOpenMod->m_pModule, name, value, scope, m_pOpenMod->m_name);
		}

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddPrimState") {

		string type;
		string name;
		string include;

		CParamList params[] = {
				{ "type", &type, true, ePrmIdent, 0, 0 },
				{ "name", &name, true, ePrmIdent, 0, 0 },
				{ "include", &include, true, ePrmString, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected <module>.AddPrimState(type=<type>, name=<name>, include=\"<file>\")");

		if (m_pOpenMod->m_primStateList.isInList(name))
			CPreProcess::ParseMsg(Error, "duplicate prim state name '%s'", name.c_str());
		else {
			m_pOpenMod->m_primStateList.insert(name);
			AddPrimstate(m_pOpenMod->m_pModule, type, name, include);
		}

		m_pLex->GetNextTk();

	} else
		CPreProcess::ParseMsg(Error, "Unknown module capability %s", m_pLex->GetTkString().c_str());
}

void HtdFile::ParseFunctionMethods()
{
	if (m_pLex->GetTkString() == "AddParam") {

		string dir;
		CType * pType;
		string name;

		CParamList params[] = {
				{ "dir", &dir, true, ePrmIdent, 0, 0 },
				{ "type", &pType, true, ePrmType, 0, 0 },
				{ "name", &name, true, ePrmIdent, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected AddFunction(...).AddParam( dir, type, name )");

		if (dir != "input" && dir != "output" && dir != "inout")
			CPreProcess::ParseMsg(Error, "expected dir={input|output|inout}");

		if (m_pDsn->m_functionParamList.isInList(name))
			CPreProcess::ParseMsg(Error, "duplicate function parameter name '%s'", name.c_str());
		else {
			m_pDsn->m_functionParamList.insert(name);
			m_pDsnInfo->AddFunctionParam(m_pOpenFunction, dir, pType, name);
		}

		m_pLex->GetNextTk();

	} else
		CPreProcess::ParseMsg(Error, "Expected an AddFunction method");
}

void HtdFile::ParseEntryMethods()
{
	if (m_pLex->GetTkString() == "AddParam") {

		string hostType;
		CType * pType;
		string name;
		bool bUnused = false;

		CParamList params[] = {
				{ "hostType", &hostType, false, ePrmParamStr, 0, 0 },
				{ "type", &pType, true, ePrmType, 0, 0 },
				{ "name", &name, true, ePrmIdent, 0, 0 },
				{ "unused", &bUnused, false, ePrmBoolean, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected AddEntry(...).AddParam( { hostType, } type, name {, unused=false } )");

		if (hostType.size() == 0)
			hostType = pType->m_typeName;

		if (m_pDsn->m_entryParamList.isInList(name))
			CPreProcess::ParseMsg(Error, "duplicate entry parameter name '%s'", name.c_str());
		else {
			m_pDsn->m_entryParamList.insert(name);
			m_pDsnInfo->AddEntryParam(m_pOpenEntry, hostType, pType, name, !bUnused);
		}

		m_pLex->GetNextTk();

	} else
		CPreProcess::ParseMsg(Error, "Expected an AddEntry method");
}

void HtdFile::ParseReturnMethods()
{
	if (m_pLex->GetTkString() == "AddParam") {

		string hostType;
		CType * pType;
		string name;
		bool bUnused = false;

		CParamList params[] = {
				{ "hostType", &hostType, false, ePrmParamStr, 0, 0 },
				{ "type", &pType, true, ePrmType, 0, 0 },
				{ "name", &name, true, ePrmIdent, 0, 0 },
				{ "unused", &bUnused, false, ePrmBoolean, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected AddReturn(...).AddParam( { hostType,} type, name {, unused=false } )");

		if (hostType.size() == 0)
			hostType = pType->m_typeName;

		if (m_pDsn->m_returnParamList.isInList(name))
			CPreProcess::ParseMsg(Error, "duplicate return parameter name '%s'", name.c_str());
		else {
			m_pDsn->m_returnParamList.insert(name);
			m_pDsnInfo->AddReturnParam(m_pOpenReturn, hostType, pType, name, !bUnused);
		}

		m_pLex->GetNextTk();

	} else
		CPreProcess::ParseMsg(Error, "Expected an AddReturn method");
}

void HtdFile::ParseGlobalMethods()
{
	if (m_pLex->GetTkString() == "AddVar") {

		CType * pType;
		string name;
		string dimen1;
		string dimen2;
		string addr1W;
		string addr2W;
		string addr1;
		string addr2;
		string rdStg;
		string wrStg;
		bool bMaxIw = false;
		bool bMaxMw = false;
		string blockRam;
		bool bRead = false;
		bool bWrite = false;
		bool bNonInstrWrite = false;
		bool bSpanningWrite = false;

		CParamList params[] =
		{
			{ "type", &pType, true, ePrmType, 0, 0 },
			{ "name", &name, true, ePrmIdent, 0, 0 },
			{ "dimen1", &dimen1, false, ePrmInteger, 0, 0 },
			{ "dimen2", &dimen2, false, ePrmInteger, 0, 0 },
			{ "addr1W", &addr1W, false, ePrmInteger, 0, 0 },
			{ "addr2W", &addr2W, false, ePrmInteger, 0, 0 },
			{ "addr1", &addr1, false, ePrmIdentRef, 0, 0 },
			{ "addr2", &addr2, false, ePrmIdentRef, 0, 0 },
			{ "rdStg", &rdStg, false, ePrmInteger, 0, 0 },
			{ "wrStg", &wrStg, false, ePrmInteger, 0, 0 },
			{ "maxIw", &bMaxIw, false, ePrmBoolean, 0, 0 },
			{ "maxMw", &bMaxMw, false, ePrmBoolean, 0, 0 },
			{ "blockRam", &blockRam, false, ePrmIdent, 0, 0 },
			{ "instrRead", &bRead, false, ePrmBoolean, 0, 0 },
			{ "instrWrite", &bWrite, false, ePrmBoolean, 0, 0 },
			{ "nonInstrWrite", &bNonInstrWrite, false, ePrmBoolean, 0, 0 },
			{ "spanningWrite", &bSpanningWrite, false, ePrmBoolean, 0, 0 },
			{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params)) {
			CPreProcess::ParseMsg(Error, "expected .AddVar( type, name {, dimen1=0 {, dimen2=0 }} {, addr1W=0 {, addr2W=0 }} ");
			CPreProcess::ParseMsg(Info, "      {, addr1=\"\" {, addr2=\"\" }} {, rdStg=1 } {, wrStg=1 } {, maxIw=false} {, maxMw=false}");
			CPreProcess::ParseMsg(Info, "      {, blockRam=\"\" } {, instrRead=false } {, instrWrite=false } {, nonInstrWrite=false } )");
			CPreProcess::ParseMsg(Info, "      {, spanningWrite=false } )");
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

		if ((addr2W.size() > 0 || addr2.size() > 0) && addr1W.size() == 0 && addr1.size() == 0)
			CPreProcess::ParseMsg(Error, "unsupported parameter combination, addr2/addr2W is specified but addr1/addr1W is not");

		if (bRead && addr1W.size() > 0 && addr1.size() == 0)
			CPreProcess::ParseMsg(Error, "addr1W is specified but a shared or private variable for addr1 was not specified");

		if (bRead && addr2W.size() > 0 && addr2.size() == 0)
			CPreProcess::ParseMsg(Error, "addr2W is specified but a shared or private variable for addr2 was not specified");

		m_pDsnInfo->AddGlobalVar(m_pOpenGlobal, pType, name, dimen1, dimen2, addr1W, addr2W,
			addr1, addr2, rdStg, wrStg, bMaxIw, bMaxMw, ramType, bRead, bWrite, bNonInstrWrite, bSpanningWrite);

		m_pLex->GetNextTk();

	} else
		CPreProcess::ParseMsg(Error, "Expected an AddGlobal method");
}

void HtdFile::ParsePrivateMethods()
{
	if (m_pLex->GetTkString() == "AddVar") {

		CType * pType;
		string name;
		string dimen1;
		string dimen2;
		string addr1W;
		string addr2W;
		string addr1;
		string addr2;

		CParamList params[] = {
				{ "type", &pType, true, ePrmType, 0, 0 },
				{ "name", &name, true, ePrmIdent, 0, 0 },
				{ "dimen1", &dimen1, false, ePrmInteger, 0, 0 },
				{ "dimen2", &dimen2, false, ePrmInteger, 0, 0 },
				{ "addr1W", &addr1W, false, ePrmInteger, 0, 0 },
				{ "addr2W", &addr2W, false, ePrmInteger, 0, 0 },
				{ "addr1", &addr1, false, ePrmIdent, 0, 0 },
				{ "addr2", &addr2, false, ePrmIdent, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params)) {
			CPreProcess::ParseMsg(Error, "expected AddVar( type, name {, dimen1 {, dimen2 }} {, addr1W {, addr2W }}");
			CPreProcess::ParseMsg(Info, "{, addr1 {, addr2 }} )");
		}

		if ((addr2W.size() > 0 || addr2.size() > 0) && addr1W.size() == 0 && addr1.size() == 0)
			CPreProcess::ParseMsg(Error, "unsupported parameter combination, addr2/addr2W is specified but addr1/addr1W is not");

		if (addr1W.size() > 0 && addr1.size() == 0)
			CPreProcess::ParseMsg(Error, "addr1W is specified but a private variable for addr1 was not specified");

		if (addr2W.size() > 0 && addr2.size() == 0)
			CPreProcess::ParseMsg(Error, "addr2W is specified but a private variable for addr2 was not specified");

		if (m_pDsn->m_privateFieldList.isInList(name))
			CPreProcess::ParseMsg(Error, "duplicate private field name '%s'", name.c_str());
		else {
			m_pDsn->m_privateFieldList.insert(name);
			m_pOpenRecord->AddPrivateField(pType, name, dimen1, dimen2, addr1W, addr2W, addr1, addr2);
		}

		m_pLex->GetNextTk();

	} else
		CPreProcess::ParseMsg(Error, "Expected an AddPrivate method");
}

void HtdFile::ParseStageMethods()
{
	if (m_pLex->GetTkString() == "AddVar") {

		CType * pType;
		string name;
		string dimen1;
		string dimen2;
		string range[2];
		bool bInit = false;
		bool bConn = true;
		bool bReset = false;
		bool bPrimOut = false;

		CParamList params[] = {
				{ "type", &pType, true, ePrmType, 0, 0 },
				{ "name", &name, true, ePrmIdent, 0, 0 },
				{ "dimen1", &dimen1, false, ePrmInteger, 0, 0 },
				{ "dimen2", &dimen2, false, ePrmInteger, 0, 0 },
				{ "range", range, true, ePrmIntRange, 0, 0 },
				{ "init", &bInit, false, ePrmBoolean, 0, 0 },
				{ "conn", &bConn, false, ePrmBoolean, 0, 0 },
				{ "reset", &bReset, false, ePrmBoolean, 0, 0 },
				{ "primOut", &bPrimOut, false, ePrmBoolean, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected AddVar( type, name {, dimen1=0 {, dimen2=0 }}, range {, init=false } {, conn=false } {, reset=false } )");

		if (dimen2.size() > 0 && dimen1.size() == 0)
			CPreProcess::ParseMsg(Error, "unsupported parameter conbination, dimen2 is specified but dimen1 is not");

		if (m_pOpenMod->m_stageList.isInList(name))
			CPreProcess::ParseMsg(Error, "duplicate stage variable name '%s'", name.c_str());
		else {
			m_pOpenMod->m_stageList.insert(name);
			m_pDsnInfo->AddStageField(m_pOpenStage, pType, name, dimen1, dimen2, range, bInit, bConn, bReset, !bPrimOut);
		}

		m_pLex->GetNextTk();

	} else
		CPreProcess::ParseMsg(Error, "Expected an AddVar method");
}

void HtdFile::ParseSharedMethods()
{
	if (m_pLex->GetTkString() == "AddVar") {

		CType * pType;
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
				{ "type", &pType, false, ePrmType, 0, 0 },
				{ "name", &name, true, ePrmIdent, 0, 0 },
				{ "dimen1", &dimen1, false, ePrmInteger, 0, 0 },
				{ "dimen2", &dimen2, false, ePrmInteger, 0, 0 },
				{ "rdSelW", &rdSelW, false, ePrmInteger, 0, 0 },
				{ "wrSelW", &wrSelW, false, ePrmInteger, 0, 0 },
				{ "addr1W", &addr1W, false, ePrmInteger, 0, 0 },
				{ "addr2W", &addr2W, false, ePrmInteger, 0, 0 },
				{ "queueW", &queueW, false, ePrmInteger, 0, 0 },
				{ "blockRam", &blockRam, false, ePrmIdent, 0, 0 },
				{ "reset", &reset, false, ePrmIdent, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected AddVar( type, name {, dimen1 {, dimen2 }} {, rdSelW}{, wrSelW}{, addr1W {, addr2W }} {, queueW } {, blockRam=false } {, reset=true } )");

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
			m_pOpenRecord->AddSharedField(pType, name, dimen1, dimen2, rdSelW, wrSelW, addr1W, addr2W, queueW, ramType, reset);
		}

		m_pLex->GetNextTk();

	} else
		CPreProcess::ParseMsg(Error, "Expected an AddShared method");
}

void HtdFile::ParseMifRdMethods()
{
	if (m_pLex->GetTkString() == "AddDst") {

		string name;
		string var;
		string memSrc;// = "coproc";
		string atomic = "read";
		CType * pRdType = 0;

		CParamList pavars[] = {
				{ "name", &name, false, ePrmIdent, 0, 0 },
				{ "var", &var, true, ePrmIdentRef, 0, 0 },
				{ "memSrc", &memSrc, false, ePrmIdent, 0, 0 },
				{ "rdType", &pRdType, false, ePrmType, 0, 0 },
				{ "atomic", &atomic, false, ePrmIdent, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(pavars))
			CPreProcess::ParseMsg(Error, "expected AddReadMem(...).AddDst( name, var {, memSrc=host|coproc }{, rdType=<int type> }");

		if (pRdType && pRdType != &g_uint64 && pRdType != &g_uint32 && pRdType != &g_uint16 && pRdType != &g_uint8 &&
			pRdType != &g_int64 && pRdType != &g_int32 && pRdType != &g_int16 && pRdType != &g_int8)
			CPreProcess::ParseMsg(Error, "expected AddDst rdType parameter to be uint64_t, uint32_t, uint16_t, uint8_t, int64_t, int32_t, int16_t or int8_t");

		if (atomic != "read" && atomic != "setBit63")
			CPreProcess::ParseMsg(Error, "invalid value for atomic (must be read or setBit63)");

		if (name.size() == 0) {
			char const * pStr = var.c_str();
			while (isalnum(*pStr) || *pStr == '_') pStr += 1;
			name = string(var.c_str(), pStr - var.c_str());
		}

		if (memSrc.size() > 0 && memSrc != "coproc" && memSrc != "host")
			CPreProcess::ParseMsg(Error, "invalid value for memSrc (must be coproc or host)");

		if (m_pDsn->m_mifRdDstList.isInList(name))
			CPreProcess::ParseMsg(Error, "duplicate memory read interface destination name '%s'", name.c_str());
		else {
			m_pDsn->m_mifRdDstList.insert(name);
			m_pDsnInfo->AddDst(m_pOpenMifRd, name, var, memSrc, atomic, pRdType);
		}

		m_pLex->GetNextTk();

	} else if (m_pLex->GetTkString() == "AddFunction") {

		string name;
		string infoW;
		string stgCnt = "0";
		string elemCntW;
		string memSrc = "coproc";
		CType * pRdType = &g_uint64;

		CParamList params[] = {
				{ "name", &name, true, ePrmIdent, 0, 0 },
				{ "rspInfoW", &infoW, true, ePrmInteger, 0, 0 },
				{ "rsmDly", &stgCnt, false, ePrmInteger, 0, 0 },
				{ "elemCntW", &elemCntW, false, ePrmInteger, 0, 0 },
				{ "memSrc", &memSrc, false, ePrmIdent, 0, 0 },
				{ "rdType", &pRdType, false, ePrmType, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected AddFunction( name, rspInfoW, rsmDly=0 {, elemCntW=0}, {, memSrc=\"coproc\"}{, rdType=uint64 } )");

		if (pRdType	!= &g_uint64 && pRdType != &g_uint32 && pRdType != &g_uint16 && pRdType != &g_uint8 &&
			pRdType != &g_int64 && pRdType != &g_int32 && pRdType != &g_int16 && pRdType != &g_int8)
			CPreProcess::ParseMsg(Error, "expected AddFunction rdType parameter to be uint64_t, uint32_t, uint16_t, uint8_t, int64_t, int32_t, int16_t or int8_t");

		if (memSrc != "coproc" && memSrc != "host")
			CPreProcess::ParseMsg(Error, "invalid value for memSrc (must be coproc or host)");

		if (m_pDsn->m_mifRdDstList.isInList(name))
			CPreProcess::ParseMsg(Error, "duplicate memory read interface destination name '%s'", name.c_str());
		else {
			m_pDsn->m_mifRdDstList.insert(name);
			m_pDsnInfo->AddDst(m_pOpenMifRd, name, infoW, stgCnt, elemCntW, memSrc, pRdType);
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
		string memDst;
		CType * pWrType = 0;
		CType * pType = 0;

		CParamList pavars[] = {
				{ "name", &name, false, ePrmIdent, 0, 0 },
				{ "type", &pType, false, ePrmType, 0, 0 },
				{ "var", &var, false, ePrmIdentRef, 0, 0 },
				{ "memDst", &memDst, false, ePrmIdent, 0, 0 },
				{ "wrType", &pWrType, false, ePrmType, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(pavars))
			CPreProcess::ParseMsg(Error, "expected AddWriteMem(...).AddSrc( name, var, {, memDst=host|coproc } )");

		if ((pType == 0) == (var.size() == 0))
			CPreProcess::ParseMsg(Error, "either var or type must be specified (but not both)");

		if (name.size() == 0) {
			if (pType != 0)
				name = pType->m_typeName;
			else {
				char const * pStr = var.c_str();
				while (isalnum(*pStr) || *pStr == '_') pStr += 1;
				name = string(var.c_str(), pStr - var.c_str());
			}
		}

		if (memDst.size() != 0 && memDst != "coproc" && memDst != "host")
			CPreProcess::ParseMsg(Error, "invalid value for memDst (must be coproc or host)");

		if (m_pDsn->m_mifWrSrcList.isInList(name))
			CPreProcess::ParseMsg(Error, "duplicate memory write interface source name '%s'", name.c_str());
		else {
			m_pDsn->m_mifWrSrcList.insert(name);
			m_pDsnInfo->AddSrc(m_pOpenMifWr, name, pType, var, memDst, pWrType);
		}

		m_pLex->GetNextTk();

	} else
		CPreProcess::ParseMsg(Error, "Expected an AddWriteMem method");
}

void HtdFile::ParseIhmMethods()
{
	if (m_pLex->GetTkString() == "AddDst") {

		string var;
		string dataLsb = "0";
		string addr1Lsb;
		string addr2Lsb;
		string idx1Lsb;
		string idx2Lsb;
		string field;
		string fldIdx1Lsb;
		string fldIdx2Lsb;
		bool bReadOnly = true;

		CParamList params[] = {
				{ "var", &var, true, ePrmIdent, 0, 0 },
				{ "dataLsb", &dataLsb, false, ePrmInteger, 0, 0 },
				{ "addr1Lsb", &addr1Lsb, false, ePrmInteger, 0, 0 },
				{ "addr2Lsb", &addr2Lsb, false, ePrmInteger, 0, 0 },
				{ "idx1Lsb", &idx1Lsb, false, ePrmInteger, 0, 0 },
				{ "idx2Lsb", &idx2Lsb, false, ePrmInteger, 0, 0 },
				{ "field", &field, false, ePrmIdent, 0, 0 },
				{ "fldIdx1Lsb", &fldIdx1Lsb, false, ePrmInteger, 0, 0 },
				{ "fldIdx2Lsb", &fldIdx2Lsb, false, ePrmInteger, 0, 0 },
				{ "readOnly", &bReadOnly, false, ePrmBoolean, 0, 0 },
				{ 0, 0, 0, ePrmUnknown, 0, 0 }
		};

		if (!ParseParameters(params))
			CPreProcess::ParseMsg(Error, "expected AddHostMsg(...).AddDst( var, {,dataLsb=0} {, addr1Lsb {, addr2Lsb }} {, idx1Lsb {, idx2Lsb }} {,readOnly=false})");

		AddMsgDst(m_pOpenIhm, var, dataLsb, addr1Lsb, addr2Lsb, idx1Lsb, idx2Lsb, field, fldIdx1Lsb, fldIdx2Lsb, bReadOnly);

		m_pLex->GetNextTk();

	} else
		CPreProcess::ParseMsg(Error, "Expected a AddHostMsg method");
}

bool HtdFile::ParseParameters(CParamList *params)
{
	if (m_pLex->GetNextTk() != eTkLParen) {
		SkipTo(eTkRParen);
		return false;
	}

	bool bError = false;
	EToken tk;
	while ((tk = m_pLex->GetNextTk()) == eTkIdent) {
		int i;
		for (i = 0; params[i].m_pName && params[i].m_pName[0] != '\0'; i += 1)
			if (m_pLex->GetTkString() == params[i].m_pName)
				break;

		string paramName = m_pLex->GetTkString();

		if (params[i].m_pName == 0) {
			bError = true;
			CPreProcess::ParseMsg(Error, "unexpected parameter name '%s'", m_pLex->GetTkString().c_str());
			break;
		}

		int dupIdx = params[i].m_dupIdx > 0 ? params[i].m_dupIdx : i;

		if (params[dupIdx].m_bPresent && params[dupIdx].m_pName[0] != '\0') {
			bError = true;
			CPreProcess::ParseMsg(Error, "duplicate parameters '%s'", m_pLex->GetTkString().c_str());
			break;
		}

		params[dupIdx].m_bPresent = true;

		if (m_pLex->GetNextTk() != eTkEqual) {
			bError = true;
			CPreProcess::ParseMsg(Error, "expected '=' after parameter '%s'", params[i].m_pName);
			break;
		}

		bool bValue = true;
		if (params[i].m_paramType == ePrmString) {

			tk = m_pLex->GetNextTk();
			if (tk != eTkString) {
				bError = true;
				CPreProcess::ParseMsg(Error, "expected a string for parameter '%s'", params[i].m_pName);
				break;
			}
			*(string *)params[i].m_pValue = m_pLex->GetTkString();
			tk = m_pLex->GetNextTk();

		} else if (params[i].m_paramType == ePrmType) {

			string typeParamStr;

			tk = m_pLex->GetNextTk();

			int builtinWidth;
			bool bSignedBuiltin;
			typeParamStr = ParseType(builtinWidth, bSignedBuiltin);

			CType * pType = m_pDsnInfo->FindType(typeParamStr, -1, CPreProcess::m_lineInfo);

			if (pType && pType->IsRecord())
				m_pDsnInfo->InitAndValidateRecord(pType->AsRecord());

			*(CType **)params[i].m_pValue = pType;

			tk = m_pLex->GetTk();

		} else if (params[i].m_paramType == ePrmIdent) {

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

		} else if (params[i].m_paramType == ePrmIdentRef) {

			string paramStr = m_pLex->GetExprStr(',');
			char const * pPos = paramStr.c_str();
			while (isspace(*pPos)) pPos += 1;

			for (int fldIdx = 0; ; fldIdx += 1) {
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
						CPreProcess::ParseMsg(Error, "end of line reached while looking for ']' in parameter '%s'", params[i].m_pName);
						break;
					}
					pPos += 1;
					while (isspace(*pPos)) pPos += 1;
				}

				if (fldIdx == 0 && *pPos == '(') {
					while (*pPos != ')' && *pPos != '\0') pPos++;
					if (*pPos != ')') {
						bError = true;
						CPreProcess::ParseMsg(Error, "end of line reached while looking for ')' in parameter '%s'", params[i].m_pName);
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

		} else if (params[i].m_paramType == ePrmInteger || params[i].m_paramType == ePrmParamStr) {

			string & intParamStr = *(string *)params[i].m_pValue;
			int defValue = 0;

			intParamStr = m_pLex->GetExprStr(',');
			tk = m_pLex->GetNextTk();

			if (params[i].m_paramType == ePrmInteger) {
				CHtIntParam intParam;
				intParam.SetValue(CPreProcess::m_lineInfo, intParamStr, params[i].m_bRequired, defValue);
			}

		} else if (params[i].m_paramType == ePrmInt) {

			string intParamStr = m_pLex->GetExprStr(',');
			tk = m_pLex->GetNextTk();

			int defValue = 0;
			CHtIntParam intParam;
			intParam.SetValue(CPreProcess::m_lineInfo, intParamStr, true, defValue);

			*(int*)params[i].m_pValue = intParam.AsInt();

		} else if (params[i].m_paramType == ePrmIntList) {

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

		} else if (params[i].m_paramType == ePrmIntRange) {

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

		} else if (params[i].m_paramType == ePrmBoolean) {

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

		} else if (params[i].m_paramType == ePrmParamLst) {

			vector<pair<string, string> > & paramPairList = *(vector<pair<string, string> > *)params[i].m_pValue;
			pair<string, string> param;

			param.first = paramName;
			param.second = m_pLex->GetExprStr(',');

			paramPairList.push_back(param);

			tk = m_pLex->GetNextTk();

		} else {
			assert(0);
			*(string *)params[i].m_pValue = m_pLex->GetTkString();
			tk = m_pLex->GetNextTk();
		}

		if (tk != eTkComma)
			break;
	}

	if (!bError) {
		for (int i = 0; params[i].m_pName; i += 1) {
			if (params[i].m_dupIdx > 0) continue;

			if (params[i].m_bDeprecated && params[i].m_bPresent) {
				CPreProcess::ParseMsg(Warning, "use of deprecated parameter name '%s'",
					params[i].m_pName);
			}

			if (params[i].m_bRequired && !params[i].m_bPresent) {
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

	if (pParam->m_paramType == ePrmIdent) {
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

	} else if (pParam->m_paramType == ePrmInteger) {

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
				for (int i = n1; i <= n2; i += 1) {
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

	} else if (pParam->m_paramType == ePrmBoolean) {

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
