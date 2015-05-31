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

vector<CHtString> const g_nullHtStringVec; // used as default argument for GenModDecl

bool CDsnInfo::FindVariableWidth(CLineInfo const &lineInfo, CModule &mod, string varName, bool &bHtId, bool &bPrivate, bool &bShared, bool &bStage, int &varW)
{
	if (bHtId && varName == "htId") {
		varW = mod.m_threads.m_htIdW.AsInt();
		bPrivate = bShared = bStage = false;
		return true;
	}

	if (bPrivate && mod.m_bHasThreads && FindFieldRefWidth(lineInfo, varName, mod.m_threads.m_htPriv.m_fieldList, varW)) {
		bHtId = bShared = bStage = false;
		return true;
	}

	if (bShared && FindFieldRefWidth(lineInfo, varName, mod.m_shared.m_fieldList, varW)) {
		bHtId = bPrivate = bStage = false;
		return true;
	}

	if (bStage && FindFieldRefWidth(lineInfo, varName, mod.m_stage.m_fieldList, varW)) {
		bHtId = bPrivate = bShared = false;
		return true;
	}

	return false;
}

bool CDsnInfo::FindFieldRefWidth(CLineInfo const &lineInfo, string const &fieldRef, vector<CField *> const &fieldList, int &fieldW)
{
	// find field in fieldlist that matches fieldRef, return width of field
	//  need to parse fieldRef into individual identifiers

	string fieldName = fieldRef.substr(0, fieldRef.find_first_of('.'));
	string postName;
	if (fieldName.size() < fieldRef.size())
		postName = fieldRef.substr(fieldName.size() + 1);

	char const *pStr = fieldRef.c_str();
	int spaceCnt = 0;
	while (isspace(*pStr)) pStr += 1, spaceCnt += 1;

	int identCnt = 0;
	while (isalpha(*pStr) || isdigit(*pStr) || *pStr == '_') pStr += 1, identCnt += 1;

	// fieldRef name without leading white space or indexing
	string name = fieldRef.substr(spaceCnt, identCnt);

	for (size_t fldIdx = 0; fldIdx < fieldList.size(); fldIdx += 1) {
		CField const * pField = fieldList[fldIdx];

		if (pField->m_name.size() == 0) {
			if (FindFieldRefWidth(lineInfo, fieldRef, pField->m_pType->AsRecord()->m_fieldList, fieldW))
				return true;
		} 
		else if (pField->m_name == name) {
			// found field, check if native type or record
			if (pField->m_pType->IsRecord()) {
				return FindFieldRefWidth(lineInfo, postName, pField->m_pType->AsRecord()->m_fieldList, fieldW);
			}
			else if (postName.size() > 0) {
				ParseMsg(Error, lineInfo, "address variable can not be a struct or union type");
			}
			else {
				CHtString const fieldWidth = pField->m_fieldWidth;
				fieldW = fieldWidth.AsInt() > 0 ? fieldWidth.AsInt() : pField->m_pType->m_clangBitWidth;
				return true;
			}
		}

		if (pField->m_pType->IsRecord()) {	// CStyle anonamous struct/union

		}
	}

	return false;
}

void CDsnInfo::ListIgnoredModules()
{
	static bool bAlreadyCalled = false;
	if (bAlreadyCalled) return;
	bAlreadyCalled = true;

	// an error occured that may be due to a module being ignored, list the ignored modules
	bool bFoundOne = false;
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (mod.m_bIsUsed) continue;

		if (!bFoundOne) {
			ParseMsg(Info, "The following modules were ignored because they were not found in the call chain");
			bFoundOne = true;
		}

		ParseMsg(Info, "  %s", mod.m_modName.c_str());
	}
}

// Transfer #defines in HTD/HTI files to defineTable
void CDsnInfo::LoadDefineList()
{
	vector<string> nullParamList;
	for (int i = 0; i < g_appArgs.GetPreDefinedNameCnt(); i += 1) {
		m_defineTable.Insert(g_appArgs.GetPreDefinedName(i), nullParamList, g_appArgs.GetPreDefinedValue(i), true, false, string("auto"));
	}

	for (MacroMap_Iter iter = GetMacroTbl().begin(); iter != GetMacroTbl().end(); iter++) {
		CMacro const & macro = iter->second;

		bool bPreDefined = macro.GetName() == "__LINE__" || macro.GetName() == "_HTV";

		if (macro.IsFromIncludeFile())
			m_defineTable.Insert(macro.GetName(), macro.GetParamList(), macro.GetExpansion(), bPreDefined, macro.IsParenReqd(), string("auto"));
		else
			m_defineTable.Insert(macro.GetName(), macro.GetParamList(), macro.GetExpansion(), bPreDefined, macro.IsParenReqd(), string("unit"));

		if (macro.GetName() == "__PLATFORM__") {
			string str = macro.GetExpansion();
			if (str.size() > 2 && str[0] == '"' && str[str.size() - 1] == '"') {
				str.erase(0, 1);
				str.erase(str.size() - 1, 1);
			}
			if (!g_appArgs.SetHtCoproc(str.c_str())) {
				fprintf(stderr, "Unknown value for __PLATFORM__\n");
				exit(1);
			}
		}
	}
}

void
CDsnInfo::GenIntfStruct(FILE *incFp, string intfName, vector<CField *> &fieldList,
bool bCStyle, bool bInclude, bool bData64, bool bUnion)
{
	string m_ = bCStyle ? "" : "m_";

	if (!bCStyle) {
		fprintf(incFp, "#ifdef _HTV\n");
		fprintf(incFp, "%s %sIntf {\n", bUnion ? "union" : "struct", intfName.c_str());
		fprintf(incFp, "#else\n");
		fprintf(incFp, "%s %s {\n", bUnion ? "union" : "struct", intfName.c_str());
		fprintf(incFp, "#endif\n");

		char *pTabs = "\t";
		if (bData64) {
			fprintf(incFp, "\tunion {\n");
			fprintf(incFp, "\t\tstruct {\n");
			pTabs = "\t\t\t";
		}

		for (size_t fieldIdx = 0; fieldIdx < fieldList.size(); fieldIdx += 1) {
			CField * pField = fieldList[fieldIdx];

			if (pField->m_bIfDefHtv)
				fprintf(incFp, "#ifndef _HTV\n");

			if (IsBaseType(pField->m_type)) {
				CTypeDef *pTypeDef = FindTypeDef(pField->m_type);

				if (pTypeDef && pTypeDef->m_width.size() > 0)
					fprintf(incFp, "%s%s %s%s : %s;\n", pTabs, pTypeDef->m_type.c_str(), m_.c_str(), pField->m_name.c_str(), pTypeDef->m_width.c_str());
				else if (pTypeDef)
					fprintf(incFp, "%s%s %s%s;\n", pTabs, pTypeDef->m_type.c_str(), m_.c_str(), pField->m_name.c_str());
				else if (pField->m_fieldWidth.size() > 0)
					fprintf(incFp, "%s%s %s%s : %s;\n", pTabs, pField->m_pType->m_typeName.c_str(), m_.c_str(), pField->m_name.c_str(), pField->m_fieldWidth.c_str());
				else
					fprintf(incFp, "%s%s %s%s;\n", pTabs, pField->m_pType->m_typeName.c_str(), m_.c_str(), pField->m_name.c_str());
			} else
				fprintf(incFp, "%s%s %s%s;\n", pTabs, pField->m_pType->m_typeName.c_str(), m_.c_str(), pField->m_name.c_str());

			if (pField->m_bIfDefHtv)
				fprintf(incFp, "#endif\n");
		}

		if (bData64) {
			fprintf(incFp, "\t\t};\n");
			fprintf(incFp, "\t\tuint64_t\t\tm_data64;\n");
			fprintf(incFp, "\t};\n");
		}

		fprintf(incFp, "};\n");
		fprintf(incFp, "\n");
	}

	if (bCStyle) {
		if (bInclude) {
			fprintf(incFp, "#ifdef _HTV\n");
			fprintf(incFp, "#define %s__HTL_ %s\n", intfName.c_str(), intfName.c_str());
			fprintf(incFp, "#else\n");
		}
	} else {
		fprintf(incFp, "#ifdef _HTV\n");
		fprintf(incFp, "#define %s %sIntf\n", intfName.c_str(), intfName.c_str());
		fprintf(incFp, "#else\n");
	}

	if (bCStyle) {
		if (bInclude) {
			fprintf(incFp, "#define %s %s__HTL_\n", intfName.c_str(), intfName.c_str());
			fprintf(incFp, "%s %s {\n", bUnion ? "union" : "struct", intfName.c_str());
			//fprintf(incFp, "\t%s() {}\n", intfName.c_str());
		} else {
			fprintf(incFp, "%s %s {\n", bUnion ? "union" : "struct", intfName.c_str());
			fprintf(incFp, "#ifndef _HTV\n");
		}
	} else {
		fprintf(incFp, "%s %sIntf : %s {\n", bUnion ? "union" : "struct", intfName.c_str(), intfName.c_str());
		fprintf(incFp, "\t%sIntf() {}\n", intfName.c_str());
	}

	// compare for equal
	CHtCode htFile(incFp);
	const char *pTabs = "";
	const char *pStr = "";
	string prefixName;
	GenStructIsEqual(htFile, pTabs, prefixName, intfName, fieldList, bCStyle, pStr, eStructAll, true);

	// assignment to zero
	pStr = "";
	GenStructAssignToZero(htFile, pTabs, prefixName, intfName, fieldList, bCStyle, pStr, eStructAll, true);

	// signal tracing
	fprintf(incFp, "# ifdef HT_SYSC\n");

	pStr = "";
	GenStructScTrace(htFile, pTabs, prefixName, prefixName, intfName, fieldList, bCStyle, pStr, eStructAll, true);

	// signal printing
	pStr = "";
	GenStructStream(htFile, pTabs, prefixName, intfName, fieldList, bCStyle, pStr, eStructAll, true);

	fprintf(incFp, "#endif\n");

	if (bCStyle && !bInclude)
		fprintf(incFp, "#endif\n");

	if (bCStyle)
		GenUserStructFieldList(incFp, false, fieldList, bCStyle, FieldList, "");

	fprintf(incFp, "};\n");
	if (!bCStyle || bInclude)
		fprintf(incFp, "#endif\n");
	fprintf(incFp, "\n");
}

void CDsnInfo::GenRamIntfStruct(FILE *incFp, string intfName, CRam &ram, EStructType type)
{
	CHtCode htFile(incFp);

	GenRamIntfStruct(htFile, "", intfName, ram, type);
}

string CDsnInfo::GenIndexStr(bool bGen, char const * pFormat, int index)
{
	if (!bGen) return string();
	char buf[64];
	sprintf(buf, pFormat, index);
	return string(buf);
}

void
CDsnInfo::GenRamIntfStruct(CHtCode &code, char const * pTabs, string intfName, CRam &ram, EStructType type)
{
	// mode: 0-all, 1-read, 2-write
	code.Append("%sstruct %s {\n", pTabs, intfName.c_str());
	code.Append("%s\t#ifndef _HTV\n", pTabs);

	// compare for equal
	const char *pStr = "";
	string prefixName;
	GenStructIsEqual(code, pTabs, prefixName, intfName, ram.m_fieldList, ram.m_bCStyle, pStr, type);

	// assignment
	//pStr = "\t\t";
	//prefixName = "";
	//GenStructAssign(code, pTabs, prefixName, intfName, ram.m_fieldList, ram.m_bCStyle, pStr, type);

	// assignment to zero
	pStr = "";
	prefixName = "";
	GenStructAssignToZero(code, pTabs, prefixName, intfName, ram.m_fieldList, ram.m_bCStyle, pStr, type);

	// signal tracing
	pStr = "\t\t";
	prefixName = "";
	code.Append("%s\t#ifdef HT_SYSC\n", pTabs);

	GenStructScTrace(code, pTabs, prefixName, prefixName, intfName, ram.m_fieldList, ram.m_bCStyle, pStr, type);

	// signal printing
	GenStructStream(code, pTabs, prefixName, intfName, ram.m_fieldList, ram.m_bCStyle, pStr, type);

	code.Append("%s\t#endif // HT_SYSC\n", pTabs);
	code.Append("%s\t#endif // _HTV\n", pTabs);

	if (type == eGenRamWrEn)
		GenRamWrEn(code, pTabs, "", ram);

	else {
		for (size_t fieldIdx = 0; fieldIdx < ram.m_fieldList.size(); fieldIdx += 1) {
			CField * pField = ram.m_fieldList[fieldIdx];

			if (type == eStructRamRdData && (pField->m_bSrcRead || pField->m_bMifRead) || type != eStructRamRdData && (pField->m_bSrcWrite || pField->m_bMifWrite)) {
				CTypeDef *pTypeDef = FindTypeDef(pField->m_type);

				if (false && pField->m_dimenList.size() == 0) {
					if (pTypeDef && pTypeDef->m_width.size() > 0)
						code.Append("%s\t%s\tm_%s:%s;\n", pTabs, pTypeDef->m_type.c_str(), pField->m_name.c_str(), pTypeDef->m_width.c_str());
					else if (pTypeDef)
						code.Append("%s\t%s\tm_%s;\n", pTabs, pTypeDef->m_type.c_str(), pField->m_name.c_str());
					else
						code.Append("%s\t%s\tm_%s;\n", pTabs, pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());
				} else {
					if (pTypeDef && pTypeDef->m_type.substr(0, 3) == "int")
						code.Append("%s\tht_int%d\tm_%s%s;\n", pTabs, pTypeDef->m_width.AsInt(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());
					else if (pTypeDef && pTypeDef->m_type.substr(0, 4) == "uint")
						code.Append("%s\tht_uint%d\tm_%s%s;\n", pTabs, pTypeDef->m_width.AsInt(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());
					else if (pTypeDef)
						code.Append("%s\t%s\tm_%s%s;\n", pTabs, pTypeDef->m_type.c_str(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());
					else
						code.Append("%s\t%s\tm_%s%s;\n", pTabs, pField->m_pType->m_typeName.c_str(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());
				}
			}
		}
	}

	code.Append("%s};\n", pTabs);
	code.Append("\n");
}

void
CDsnInfo::GenStructIsEqual(CHtCode &htFile, char const * pTabs, string prefixName, string &typeName,
vector<CField *> &fieldList, bool bCStyle, const char *&pStr, EStructType structType, bool bHeader)
{
	string m_ = bCStyle ? "" : "m_";
	if (bHeader) {
		htFile.Append("%s\tbool operator == (const %s & rhs) const\n", pTabs, typeName.c_str());
		htFile.Append("%s\t{\n", pTabs);
	}

	for (size_t fieldIdx = 0; fieldIdx < fieldList.size(); fieldIdx += 1) {
		CField * pField = fieldList[fieldIdx];

		if (structType == eStructAll || structType == eStructRamRdData && (pField->m_bSrcRead || pField->m_bMifRead)
			|| structType != eStructRamRdData && (pField->m_bSrcWrite || pField->m_bMifWrite)) {

			size_t recordIdx;
			for (recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
				if (m_recordList[recordIdx]->m_typeName == pField->m_type)
					break;
			}
			if (structType != eGenRamWrEn && !bCStyle && recordIdx < m_recordList.size()) {
				size_t prefixLen = prefixName.size();

				if (pField->m_elemCnt <= 8) {
					vector<int> refList(pField->m_dimenList.size());

					do {
						string fldIdx = IndexStr(refList);

						if (pField->m_name.size() > 0)
							prefixName += m_ + pField->m_name + fldIdx + ".";

						GenStructIsEqual(htFile, pTabs, prefixName, typeName, m_recordList[recordIdx]->m_fieldList, m_recordList[recordIdx]->m_bCStyle, pStr, structType, false);

						prefixName.erase(prefixLen);

					} while (DimenIter(pField->m_dimenList, refList));

				} else {
					string tabs = pTabs;
					int idxCnt = 0;
					for (size_t i = 0; i < pField->m_dimenList.size(); i += 1) {
						idxCnt += 1;
						bool bIsSigned = m_defineTable.FindStringIsSigned(pField->m_dimenList[i].AsStr());
						htFile.Append("%s\t\tfor (%s idx%d = 0; idx%d < %d; idx%d += 1) {\n", tabs.c_str(),
							bIsSigned ? "int" : "unsigned", idxCnt, idxCnt, pField->m_dimenList[i].AsInt(), idxCnt);
						tabs += "\t";
					}

					if (pField->m_name.size() > 0)
						prefixName += m_ + pField->m_name + pField->m_dimenIndex + ".";

					GenStructIsEqual(htFile, tabs.c_str(), prefixName, typeName, m_recordList[recordIdx]->m_fieldList, m_recordList[recordIdx]->m_bCStyle, pStr, structType, false);

					prefixName.erase(prefixLen);

					for (size_t i = 0; i < pField->m_dimenList.size(); i += 1) {
						tabs = tabs.substr(1);
						htFile.Append("%s\t\t}\n", tabs.c_str());
					}
				}

			} else if (pField->m_pType->IsRecord() && pField->m_name.size() == 0) {

				CRecord * pRecord = pField->m_pType->AsRecord();
				GenStructIsEqual(htFile, pTabs, prefixName, typeName, pRecord->m_fieldList, pRecord->m_bCStyle, pStr, structType, false);

			} else {
				if (pField->m_elemCnt <= 8) {

					vector<int> refList(pField->m_dimenList.size());

					do {
						string fldIdx = IndexStr(refList);

						htFile.Append("%s\t\tif (!(%s%s%s%s == rhs.%s%s%s%s)) return false;\n", pTabs,
							prefixName.c_str(), m_.c_str(), pField->m_name.c_str(), fldIdx.c_str(),
							prefixName.c_str(), m_.c_str(), pField->m_name.c_str(), fldIdx.c_str());

					} while (DimenIter(pField->m_dimenList, refList));

				} else {
					GenRamIndexLoops(htFile, "\t\t", *pField);

					htFile.Append("%s\t\tif (!(%s%s%s%s == rhs.%s%s%s%s)) return false;\n", pTabs,
						prefixName.c_str(), m_.c_str(), pField->m_name.c_str(), pField->m_dimenIndex.c_str(),
						prefixName.c_str(), m_.c_str(), pField->m_name.c_str(), pField->m_dimenIndex.c_str());
				}
			}
		}
	}


	if (bHeader) {
		htFile.Append("%s\t\treturn true;\n", pTabs);
		htFile.Append("%s\t}\n", pTabs);
	}
}

void
CDsnInfo::GenStructAssign(CHtCode &htFile, char const * pTabs, string prefixName, string &typeName,
vector<CField *> &fieldList, bool bCStyle, const char *&pStr, EStructType structType, bool bHeader)
{
	string m_ = bCStyle ? "" : "m_";

	if (bHeader) {
		htFile.Append("%s\t%s & operator = (const %s & rhs)\n", pTabs, typeName.c_str(), typeName.c_str());
		htFile.Append("%s\t{\n", pTabs);
	}

	for (size_t fieldIdx = 0; fieldIdx < fieldList.size(); fieldIdx += 1) {
		CField * pField = fieldList[fieldIdx];

		if (structType == eStructAll || structType == eStructRamRdData && (pField->m_bSrcRead || pField->m_bMifRead)
			|| structType != eStructRamRdData && (pField->m_bSrcWrite || pField->m_bMifWrite)) {

			size_t recordIdx;
			for (recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
				if (m_recordList[recordIdx]->m_typeName == pField->m_type)
					break;
			}
			if (structType != eGenRamWrEn && recordIdx < m_recordList.size()) {
				size_t prefixLen = prefixName.size();

				if (pField->m_elemCnt <= 8) {
					vector<int> refList(pField->m_dimenList.size());

					do {
						string fldIdx = IndexStr(refList);

						if (pField->m_name.size() > 0)
							prefixName += m_ + pField->m_name + fldIdx + ".";

						GenStructAssign(htFile, pTabs, prefixName, typeName, m_recordList[recordIdx]->m_fieldList,
							m_recordList[recordIdx]->m_bCStyle, pStr, structType, false);

						prefixName.erase(prefixLen);

					} while (DimenIter(pField->m_dimenList, refList));

				} else {
					string tabs = pTabs;
					int idxCnt = 0;
					for (size_t i = 0; i < pField->m_dimenList.size(); i += 1) {
						idxCnt += 1;
						bool bIsSigned = m_defineTable.FindStringIsSigned(pField->m_dimenList[i].AsStr());
						htFile.Append("%s\t\tfor (%s idx%d = 0; idx%d < %d; idx%d += 1) {\n", tabs.c_str(),
							bIsSigned ? "int" : "unsigned", idxCnt, idxCnt, pField->m_dimenList[i].AsInt(), idxCnt);
						tabs += "\t";
					}

					if (pField->m_name.size() > 0)
						prefixName += m_ + pField->m_name + pField->m_dimenIndex + ".";

					GenStructAssign(htFile, tabs.c_str(), prefixName, typeName, m_recordList[recordIdx]->m_fieldList,
						m_recordList[recordIdx]->m_bCStyle, pStr, structType, false);

					prefixName.erase(prefixLen);

					for (size_t i = 0; i < pField->m_dimenList.size(); i += 1) {
						tabs = tabs.substr(1);
						htFile.Append("%s\t\t}\n", tabs.c_str());
					}
				}

			} else if (pField->m_pType->IsRecord() && pField->m_name.size() == 0) {

				CRecord * pRecord = pField->m_pType->AsRecord();
				GenStructAssign(htFile, pTabs, prefixName, typeName, pRecord->m_fieldList, bCStyle, pStr, structType, false);

			} else {
				vector<int> refList(pField->m_dimenList.size(), 0);

				do {
					string fldIdx = IndexStr(refList);

					htFile.Append("%s%s%s%s%s%s = rhs.%s%s%s%s;\n", pTabs, pStr,
						prefixName.c_str(), m_.c_str(), pField->m_name.c_str(), fldIdx.c_str(),
						prefixName.c_str(), m_.c_str(), pField->m_name.c_str(), fldIdx.c_str());

				} while (DimenIter(pField->m_dimenList, refList));
			}
		}
	}

	if (bHeader) {
		htFile.Append("%s\t\treturn *this;\n", pTabs);
		htFile.Append("%s\t}\n", pTabs);
	}
}

void
CDsnInfo::GenStructAssignToZero(CHtCode &htFile, char const * pTabs, string prefixName, string &typeName,
vector<CField *> &fieldList, bool bCStyle, const char *&pStr, EStructType structType, bool bHeader)
{
	string m_ = bCStyle ? "" : "m_";

	if (bHeader) {
		htFile.Append("%s\t%s & operator = (int zero)\n", pTabs, typeName.c_str());
		htFile.Append("%s\t{\n", pTabs);
		htFile.Append("%s\t\tassert(zero == 0);\n", pTabs);
	}

	for (size_t fieldIdx = 0; fieldIdx < fieldList.size(); fieldIdx += 1) {
		CField * pField = fieldList[fieldIdx];

		if (structType == eStructAll || structType == eStructRamRdData && (pField->m_bSrcRead || pField->m_bMifRead)
			|| structType != eStructRamRdData && (pField->m_bSrcWrite || pField->m_bMifWrite)) {

			size_t recordIdx;
			for (recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
				if (m_recordList[recordIdx]->m_typeName == pField->m_type)
					break;
			}
			if (structType != eGenRamWrEn && !bCStyle && recordIdx < m_recordList.size()) {
				size_t prefixLen = prefixName.size();

				if (pField->m_elemCnt <= 8) {
					vector<int> refList(pField->m_dimenList.size());

					do {
						string fldIdx = IndexStr(refList);

						if (pField->m_name.size() > 0)
							prefixName += m_ + pField->m_name + fldIdx + ".";

						GenStructAssignToZero(htFile, pTabs, prefixName, typeName, m_recordList[recordIdx]->m_fieldList,
							m_recordList[recordIdx]->m_bCStyle, pStr, structType, false);

						prefixName.erase(prefixLen);

					} while (DimenIter(pField->m_dimenList, refList));

				} else {
					string tabs = pTabs;
					int idxCnt = 0;
					for (size_t i = 0; i < pField->m_dimenList.size(); i += 1) {
						idxCnt += 1;
						bool bIsSigned = m_defineTable.FindStringIsSigned(pField->m_dimenList[i].AsStr());
						htFile.Append("%s\t\tfor (%s idx%d = 0; idx%d < %d; idx%d += 1) {\n", tabs.c_str(),
							bIsSigned ? "int" : "unsigned", idxCnt, idxCnt, pField->m_dimenList[i].AsInt(), idxCnt);
						tabs += "\t";
					}

					if (pField->m_name.size() > 0)
						prefixName += m_ + pField->m_name + pField->m_dimenIndex + ".";

					GenStructAssignToZero(htFile, tabs.c_str(), prefixName, typeName, m_recordList[recordIdx]->m_fieldList,
						m_recordList[recordIdx]->m_bCStyle, pStr, structType, false);

					prefixName.erase(prefixLen);

					for (size_t i = 0; i < pField->m_dimenList.size(); i += 1) {
						tabs = tabs.substr(1);
						htFile.Append("%s\t\t}\n", tabs.c_str());
					}
				}

				prefixName.erase(prefixLen);
			} else if (pField->m_pType->IsRecord() && pField->m_name.size() == 0) {

				CRecord * pRecord = pField->m_pType->AsRecord();
				GenStructAssignToZero(htFile, pTabs, prefixName, typeName, pRecord->m_fieldList, bCStyle, pStr, structType, false);

			} else {

				if (pField->m_elemCnt <= 8) {

					vector<int> refList(pField->m_dimenList.size());

					do {
						string fldIdx = IndexStr(refList);

						htFile.Append("\t\t%s%s%s%s%s%s = 0;\n", pTabs, pStr,
							prefixName.c_str(), m_.c_str(), pField->m_name.c_str(), fldIdx.c_str());

					} while (DimenIter(pField->m_dimenList, refList));

				} else {

					GenRamIndexLoops(htFile, "\t\t", *pField);
					htFile.Append("\t\t%s%s%s%s%s%s = 0;\n", pTabs, pStr,
						prefixName.c_str(), m_.c_str(), pField->m_name.c_str(), pField->m_dimenIndex.c_str());
				}
			}
		}
	}

	if (bHeader) {
		htFile.Append("%s\t\treturn *this;\n", pTabs);
		htFile.Append("%s\t}\n", pTabs);
	}
}

void
CDsnInfo::GenStructScTrace(CHtCode &htFile, char const * pTabs, string prefixName1, string prefixName2, string &typeName,
vector<CField *> &fieldList, bool bCStyle, const char *&pStr, EStructType structType, bool bHeader)
{
	string m_ = bCStyle ? "" : "m_";

	if (bHeader) {
		htFile.Append("%s\tfriend void sc_trace(sc_trace_file *tf, const %s & v, const std::string & NAME)\n", pTabs, typeName.c_str());
		htFile.Append("%s\t{\n", pTabs);
	}

	for (size_t fieldIdx = 0; fieldIdx < fieldList.size(); fieldIdx += 1) {
		CField * pField = fieldList[fieldIdx];

		if (structType == eStructAll || structType == eStructRamRdData && (pField->m_bSrcRead || pField->m_bMifRead)
			|| structType != eStructRamRdData && (pField->m_bSrcWrite || pField->m_bMifWrite)) {

			size_t recordIdx;
			for (recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
				if (m_recordList[recordIdx]->m_typeName == pField->m_type)
					break;
			}
			if (structType != eGenRamWrEn && !bCStyle && recordIdx < m_recordList.size()) {
				size_t prefixLen1 = prefixName1.size();
				size_t prefixLen2 = prefixName2.size();

				if (pField->m_elemCnt <= 8) {
					vector<int> refList(pField->m_dimenList.size());

					do {
						string fldIdx = IndexStr(refList);

						if (pField->m_name.size() > 0) {
							prefixName1 += m_ + pField->m_name + fldIdx + ".";
							prefixName2 += m_ + pField->m_name + fldIdx + ".";
						}

						GenStructScTrace(htFile, pTabs, prefixName1, prefixName2, typeName, m_recordList[recordIdx]->m_fieldList,
							m_recordList[recordIdx]->m_bCStyle, pStr, structType, false);

						prefixName1.erase(prefixLen1);
						prefixName2.erase(prefixLen2);

					} while (DimenIter(pField->m_dimenList, refList));

				} else {
					string loopIdx;
					string tabs = pTabs;
					int idxCnt = 0;
					for (size_t i = 0; i < pField->m_dimenList.size(); i += 1) {
						idxCnt += 1;
						bool bIsSigned = m_defineTable.FindStringIsSigned(pField->m_dimenList[i].AsStr());
						htFile.Append("%s\t\tfor (%s idx%d = 0; idx%d < %d; idx%d += 1) {\n", tabs.c_str(),
							bIsSigned ? "int" : "unsigned", idxCnt, idxCnt, pField->m_dimenList[i].AsInt(), idxCnt);
						tabs += "\t";
						loopIdx += VA("[\" + (std::string)HtStrFmt(\"%%d\", idx%d) + \"]", (int)i + 1);
					}

					if (pField->m_name.size() > 0) {
						prefixName1 += m_ + pField->m_name + pField->m_dimenIndex + ".";
						prefixName2 += m_ + pField->m_name + loopIdx + ".";
					}

					GenStructScTrace(htFile, tabs.c_str(), prefixName1, prefixName2, typeName, m_recordList[recordIdx]->m_fieldList,
						m_recordList[recordIdx]->m_bCStyle, pStr, structType, false);

					prefixName1.erase(prefixLen1);
					prefixName2.erase(prefixLen2);

					for (size_t i = 0; i < pField->m_dimenList.size(); i += 1) {
						tabs = tabs.substr(1);
						htFile.Append("%s\t\t}\n", tabs.c_str());
					}
				}

			} else if (pField->m_pType->IsRecord() && pField->m_name.size() == 0) {

				CRecord * pRecord = pField->m_pType->AsRecord();
				GenStructScTrace(htFile, pTabs, prefixName1, prefixName2, typeName, pRecord->m_fieldList, bCStyle, pStr, structType, false);

			} else {
				if (pField->m_elemCnt <= 8) {

					vector<int> refList(pField->m_dimenList.size());

					do {
						string fldIdx = IndexStr(refList);

						htFile.Append("%s\t\tsc_trace(tf, v.%s%s%s%s, NAME + \".%s%s%s%s\");\n", pTabs,
							prefixName1.c_str(), m_.c_str(), pField->m_name.c_str(), fldIdx.c_str(),
							prefixName2.c_str(), m_.c_str(), pField->m_name.c_str(), fldIdx.c_str());

					} while (DimenIter(pField->m_dimenList, refList));

				} else {

					string loopIdx = GenRamIndexLoops(htFile, "\t\t", *pField);

					htFile.Append("%s\t\tsc_trace(tf, v.%s%s%s%s, NAME + \".%s%s%s%s\");\n", pTabs,
						prefixName1.c_str(), m_.c_str(), pField->m_name.c_str(), pField->m_dimenIndex.c_str(),
						prefixName2.c_str(), m_.c_str(), pField->m_name.c_str(), loopIdx.c_str());
				}
			}
		}
	}

	if (bHeader)
		htFile.Append("%s\t}\n", pTabs);
}

void
CDsnInfo::GenStructStream(CHtCode &htFile, char const * pTabs, string prefixName, string &typeName,
vector<CField *> &fieldList, bool bCStyle, const char *&pStr, EStructType structType, bool bHeader)
{
	htFile.Append("%s\tfriend ostream& operator << (ostream& os, %s const & v) { return os; }\n", pTabs, typeName.c_str());
}

void
CDsnInfo::GenStructInit(FILE *fp, string &tabs, string prefixName, CField * pField, int idxCnt, bool bZero)
{
	size_t recordIdx;
	for (recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
		if (m_recordList[recordIdx]->m_typeName == pField->m_type)
			break;
	}
	bool bIsInStructList = recordIdx < m_recordList.size();
	if (bIsInStructList || pField->m_pType->IsRecord()) {
		size_t prefixLen = prefixName.size();

		CRecord * pRecord = pField->m_pType->AsRecord();

		vector<CField *> & fieldList = bIsInStructList
			? m_recordList[recordIdx]->m_fieldList : pRecord->m_fieldList;

		for (size_t i = 0; i < fieldList.size(); i += 1) {
			CField * pSubField = fieldList[i];

			if (pSubField->m_name.size() > 0)
				prefixName += "." + pSubField->m_name;

			if (pSubField->m_elemCnt <= 8) {

				vector<int> refList(pSubField->m_dimenList.size());

				do {
					string fldIdx = IndexStr(refList);

					GenStructInit(fp, tabs, prefixName + fldIdx, pSubField, idxCnt, bZero);

				} while (DimenIter(pSubField->m_dimenList, refList));

			} else {
				string idxStr;
				for (size_t i = 0; i < pSubField->m_dimenList.size(); i += 1) {
					idxCnt += 1;
					bool bIsSigned = m_defineTable.FindStringIsSigned(pSubField->m_dimenList[i].AsStr());
					fprintf(fp, "%sfor (%s idx%d = 0; idx%d < %d; idx%d += 1) {\n", tabs.c_str(),
						bIsSigned ? "int" : "unsigned", idxCnt, idxCnt, pSubField->m_dimenList[i].AsInt(), idxCnt);
					idxStr += VA("[idx%d]", idxCnt);
					tabs += "\t";
				}

				GenStructInit(fp, tabs, prefixName + idxStr, pSubField, idxCnt, bZero);

				for (size_t i = 0; i < pSubField->m_dimenList.size(); i += 1) {
					tabs = tabs.substr(1);
					fprintf(fp, "%s}\n", tabs.c_str());
				}
			}

			prefixName.erase(prefixLen);
		}

	} else if (bZero) {
		fprintf(fp, "%s%s = 0;\n",
			tabs.c_str(), prefixName.c_str());
	} else {
		int width = FindTypeWidth(pField);

		string mask = width == 64 ? "" : VA(" & 0x%llxULL", ((1ull << width) - 1));

		fprintf(fp, "%s%s = g_rndInit()%s;\n",
			tabs.c_str(), prefixName.c_str(), mask.c_str());
	}
}

void CDsnInfo::GenModDecl(EVcdType vcdType, CHtCode &htFile, string &modName, VA type, VA var, vector<CHtString> const & dimenList)
{
	string dimenStr;
	for (size_t i = 0; i < dimenList.size(); i += 1) {
		char dimenBuf[16];
		sprintf(dimenBuf, "[%d]", dimenList[i].AsInt());
		dimenStr += dimenBuf;
	}

	htFile.Append("\t%s %s%s;\n", type.c_str(), var.c_str(), dimenStr.c_str());

	vector<int> refList(dimenList.size(), 0);
	do {
		string idxStr = IndexStr(refList);
		GenModTrace(vcdType, modName,
			VA("%s%s", var.c_str(), idxStr.c_str()),
			VA("%s%s", var.c_str(), idxStr.c_str()));

	} while (DimenIter(dimenList, refList));
}

void CDsnInfo::GenVcdDecl(CHtCode &sosCode, EVcdType vcdType, CHtCode &declCode, string &modName, VA type, VA var, vector<CHtString> const & dimenList)
{
	string dimenStr;
	for (size_t i = 0; i < dimenList.size(); i += 1) {
		char dimenBuf[16];
		sprintf(dimenBuf, "[%d]", dimenList[i].AsInt());
		dimenStr += dimenBuf;
	}

	declCode.Append("\t%s %s%s;\n", type.c_str(), var.c_str(), dimenStr.c_str());

	vector<int> refList(dimenList.size(), 0);
	do {
		string idxStr = IndexStr(refList);
		GenVcdTrace(sosCode, vcdType, modName,
			VA("%s%s", var.c_str(), idxStr.c_str()),
			VA("%s%s", var.c_str(), idxStr.c_str()));

	} while (DimenIter(dimenList, refList));
}

void CDsnInfo::GenModVar(EVcdType vcdType, string &vcdModName, bool &bFirstModVar, string decl, string dimen, string name, string val, vector<CHtString> const & dimenList)
{
	m_iplDefines.Append("#define %s %s\n", name.c_str(), val.c_str());

	// variable tracing
	if (val[0] == 'r') {
		vector<int> refList(dimenList.size(), 0);
		do {
			string idxStr = IndexStr(refList);
			GenModTrace(vcdType, vcdModName,
				VA("%s%s", name.c_str(), idxStr.c_str()),
				VA("%s%s", val.c_str(), idxStr.c_str()));

		} while (DimenIter(dimenList, refList));
	}

	// variable reference
	if (dimen.size() > 0)
		m_iplRefDecl.Append("\t%s (& %s)%s;\n", decl.c_str(), name.c_str(), dimen.c_str());
	else
		m_iplRefDecl.Append("\t%s & %s;\n", decl.c_str(), name.c_str());

	if (bFirstModVar)
		m_iplRefInit.Append("\t\t: %s(%s)", name.c_str(), val.c_str());
	else
		m_iplRefInit.Append(",\n\t\t%s(%s)", name.c_str(), val.c_str());

	bFirstModVar = false;
}

string CDsnInfo::GenFieldType(CField * pField, bool bConst)
{
	string type;

	if (pField->m_rdSelW.AsInt() > 0) {

		type = VA("ht_mrd_block_ram<%s, %d, %d",
			pField->m_pType->m_typeName.c_str(), pField->m_rdSelW.AsInt(), pField->m_addr1W.AsInt());
		if (pField->m_addr2W.size() > 0)
			type += VA(", %d", pField->m_addr2W.AsInt());
		type += ">";

	} else if (pField->m_wrSelW.AsInt() > 0) {

		type = VA("ht_mwr_block_ram<%s, %d, %d",
			pField->m_pType->m_typeName.c_str(), pField->m_wrSelW.AsInt(), pField->m_addr1W.AsInt());
		if (pField->m_addr2W.size() > 0)
			type += VA(", %d", pField->m_addr2W.AsInt());
		type += ">";

	} else if (pField->m_addr1W.size() > 0) {

		const char *pScMem = pField->m_ramType == eDistRam ? "ht_dist_ram" : "ht_block_ram";
		if (pField->m_addr2W.size() > 0) {
			type = VA("%s<%s, %s, %s>", pScMem, pField->m_pType->m_typeName.c_str(),
				pField->m_addr1W.c_str(), pField->m_addr2W.c_str());
		} else {
			type = VA("%s<%s, %s>", pScMem, pField->m_pType->m_typeName.c_str(),
				pField->m_addr1W.c_str());
		}

	} else if (pField->m_queueW.size() > 0) {

		const char *pScMem = pField->m_ramType == eDistRam ? "ht_dist_que" : "ht_block_que";

		type = VA("%s<%s, %s>", pScMem, pField->m_pType->m_typeName.c_str(),
			pField->m_queueW.c_str());

	} else {

		type = pField->m_type;
	}

	if (bConst)
		type += " const";

	return type;
}

void CDsnInfo::GenModTrace(EVcdType vcdType, string &modName, VA name, VA val)
{
	if (vcdType == eVcdNone) return;

	if (g_appArgs.IsVcdUserEnabled() && vcdType == eVcdUser || g_appArgs.IsVcdAllEnabled()) {
		string vcdName = VA("%s.%s", modName.c_str(), name.c_str());
		if (g_appArgs.IsVcdFilterMatch(vcdName))
			m_vcdSos.Append("\t\tsc_trace(Ht::g_vcdp, %s, (std::string)name() + \".%s\");\n",
			val.c_str(), name.c_str());
	}
}

void CDsnInfo::GenVcdTrace(CHtCode &sosCode, EVcdType vcdType, string &modName, VA name, VA val)
{
	if (vcdType == eVcdNone) return;

	if (g_appArgs.IsVcdUserEnabled() && vcdType == eVcdUser || g_appArgs.IsVcdAllEnabled()) {
		string vcdName = VA("%s.%s", modName.c_str(), name.c_str());
		if (g_appArgs.IsVcdFilterMatch(vcdName))
			sosCode.Append("\t\tsc_trace(Ht::g_vcdp, %s, (std::string)name() + \".%s\");\n",
			val.c_str(), name.c_str());
	}
}

CTypeDef * CDsnInfo::FindTypeDef(string typeName)
{
	for (size_t typeIdx = 0; typeIdx < m_typedefList.size(); typeIdx += 1) {
		CTypeDef & typeDef = m_typedefList[typeIdx];
		if (typeDef.m_name == typeName) {
			if (typeDef.m_width.size() != 0)
				return &typeDef;
			else {
				// check for nested typedef's
				CTypeDef * pTypeDef2 = FindTypeDef(typeDef.m_type);
				return pTypeDef2 ? pTypeDef2 : &typeDef;
			}
		}
	}
	return 0;
}

CRecord * CDsnInfo::FindRecord(string recordName)
{
	CTypeDef * pTypeDef;
	do {
		pTypeDef = FindTypeDef(recordName);
		if (pTypeDef) recordName = pTypeDef->m_type;
	} while (pTypeDef);

	size_t recordIdx;
	for (recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
		if (m_recordList[recordIdx]->m_typeName == recordName)
			return m_recordList[recordIdx];
	}

	return 0;
}

CCxrCall &
CCxrCallList::GetCxrCall(size_t listIdx)
{
	return m_modIdxList[listIdx].m_pMod->m_cxrCallList[m_modIdxList[listIdx].m_idx];
}

void
CDsnInfo::GenUserStructFieldList(FILE *incFp, bool bIsHtPriv, vector<CField *> &fieldList, bool bCStyle, EFieldListMode mode, string tabs, bool bUnion)
{
	CHtCode	htFile(incFp);
	GenUserStructFieldList(htFile, bIsHtPriv, fieldList, bCStyle, mode, tabs, bUnion);
}

void
CDsnInfo::GenUserStructFieldList(CHtCode &htFile, bool bIsHtPriv, vector<CField *> &fieldList, bool bCStyle, EFieldListMode mode, string tabs, bool bUnion)
{
	string m_ = bCStyle ? "" : "m_";

	bUnion |= mode == Union;

	switch (mode) {
	case Struct: htFile.Append("%sstruct {\n", tabs.c_str()); break;
	case Union: htFile.Append("%sunion {\n", tabs.c_str()); break;
	default: break;
	}
	for (size_t fieldIdx = 0; fieldIdx < fieldList.size(); fieldIdx += 1) {
		CField * pField = fieldList[fieldIdx];

		size_t recordIdx;
		for (recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
			if (m_recordList[recordIdx]->m_typeName == pField->m_type)
				break;
		}

		if (recordIdx < m_recordList.size() && pField->m_name.size() == 0)
			GenUserStructFieldList(htFile, false, m_recordList[recordIdx]->m_fieldList, bCStyle, m_recordList[recordIdx]->m_bUnion ? Union : Struct, tabs + "\t", bUnion);
		else if (pField->m_name.size() == 0 && pField->m_pType->IsRecord()) {
			CRecord * pRecord = pField->m_pType->AsRecord();
			GenUserStructFieldList(htFile, false, pRecord->m_fieldList, pRecord->m_bCStyle, pRecord->m_bUnion ? Union : Struct, tabs + "\t", bUnion);
		} else {
			CTypeDef *pTypeDef = FindTypeDef(pField->m_type);

			if (bUnion && pTypeDef && pTypeDef->m_width.size() > 0 && pField->m_dimenList.size() > 0)
				ParseMsg(Error, pField->m_lineInfo, "unsupported capability, arrays of members within unions, where member has a specified field width");

			bool bStdInt = !pTypeDef || pTypeDef->m_width.AsInt() == 8 || pTypeDef->m_width.AsInt() == 16
				|| pTypeDef->m_width.AsInt() == 32 || pTypeDef->m_width.AsInt() == 64;

			//			m_iplRegDecl.Append("\t\t%s %s%s%s;\n", pField->m_pType->m_typeName.c_str(), m_.c_str(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());

			if (bCStyle || bIsHtPriv) {
				if (!bIsHtPriv && pField->m_fieldWidth.size() > 0)
					htFile.Append("%s\t%s %s%s : %d;\n", tabs.c_str(), pField->m_pType->m_typeName.c_str(), m_.c_str(), pField->m_name.c_str(), pField->m_fieldWidth.AsInt());
				else
					htFile.Append("%s\t%s %s%s%s;\n", tabs.c_str(), pField->m_pType->m_typeName.c_str(), m_.c_str(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());
			} else {
				if (pTypeDef && pTypeDef->m_width.size() > 0 && pField->m_dimenList.size() > 0) {
					if (pTypeDef->m_type.substr(0, 3) == "int") {
						if (bStdInt)
							htFile.Append("%s\tint%d_t %s%s%s;\n", tabs.c_str(), pTypeDef->m_width.AsInt(), m_.c_str(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());
						else
							htFile.Append("%s\tsc_int<%s> %s%s%s;\n", tabs.c_str(), pTypeDef->m_width.c_str(), m_.c_str(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());
					} else if (pTypeDef->m_type.substr(0, 4) == "uint") {
						if (bStdInt)
							htFile.Append("%s\tuint%d_t %s%s%s;\n", tabs.c_str(), pTypeDef->m_width.AsInt(), m_.c_str(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());
						else
							htFile.Append("%s\tsc_uint<%s> %s%s%s;\n", tabs.c_str(), pTypeDef->m_width.c_str(), m_.c_str(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());
					} else
						ParseMsg(Error, pField->m_lineInfo, "expected type to begin with 'int' or 'uint' for field with a specified width");
				} else if (pTypeDef && pTypeDef->m_width.size() > 0 && pField->m_base.size() > 0)
					htFile.Append("%s\t%s %s%s : %s;\n", tabs.c_str(), pField->m_base.c_str(), m_.c_str(), pField->m_name.c_str(), pTypeDef->m_width.c_str());
				else if (pTypeDef && pTypeDef->m_width.size() > 0)
					htFile.Append("%s\t%s %s%s : %s;\n", tabs.c_str(), pTypeDef->m_type.c_str(), m_.c_str(), pField->m_name.c_str(), pTypeDef->m_width.c_str());
				else if (pTypeDef)
					htFile.Append("%s\t%s %s%s%s;\n", tabs.c_str(), pTypeDef->m_type.c_str(), m_.c_str(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());
				else
					htFile.Append("%s\t%s %s%s%s;\n", tabs.c_str(), pField->m_pType->m_typeName.c_str(), m_.c_str(), pField->m_name.c_str(), pField->m_dimenDecl.c_str());
			}
		}
	}
	if (mode == Union || mode == Struct)
		htFile.Append("%s};\n", tabs.c_str());
}

void CDsnInfo::GenUserStructBadData(CHtCode &htFile, bool bHeader, string structName, vector<CField *> &fieldList, bool bCStyle, string tabs)
{
	string m_ = bCStyle ? "" : "m_";

	if (bHeader) {
		htFile.Append("\n");
		htFile.Append("#%sifndef _HTV\n", tabs.c_str());
		htFile.Append("%sinline %s ht_bad_data(%s const &)\n", tabs.c_str(), structName.c_str(), structName.c_str());
		htFile.Append("%s{\n", tabs.c_str());
		htFile.Append("%s\t%s x;\n", tabs.c_str(), structName.c_str());
		htFile.Append("%s\tx = 0;\n", tabs.c_str());
	}

	for (size_t fieldIdx = 0; fieldIdx < fieldList.size(); fieldIdx += 1) {
		CField * pField = fieldList[fieldIdx];

		size_t recordIdx;
		for (recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
			if (m_recordList[recordIdx]->m_typeName == pField->m_type)
				break;
		}

		if (recordIdx < m_recordList.size() && pField->m_name.size() == 0)
			GenUserStructBadData(htFile, false, structName, m_recordList[recordIdx]->m_fieldList, bCStyle, tabs);

		else if (pField->m_name.size() == 0 && pField->m_pType->IsRecord()) {

			CRecord * pRecord = pField->m_pType->AsRecord();
			GenUserStructBadData(htFile, false, structName, pRecord->m_fieldList, bCStyle, tabs);

		} else if (pField->m_dimenList.size() == 0) {
			htFile.Append("%s\tx.%s%s = ht_bad_data(x.%s%s);\n", tabs.c_str(), m_.c_str(), pField->m_name.c_str(), m_.c_str(), pField->m_name.c_str());

		} else {
			int idxCnt = 0;
			string index;
			string indexZero;
			for (size_t i = 0; i < pField->m_dimenList.size(); i += 1) {
				idxCnt += 1;
				bool bIsSigned = m_defineTable.FindStringIsSigned(pField->m_dimenList[i].AsStr());
				htFile.Append("%s\tfor (%s idx%d = 0; idx%d < %d; idx%d += 1)\n", tabs.c_str(),
					bIsSigned ? "int" : "unsigned", idxCnt, idxCnt, pField->m_dimenList[i].AsInt(), idxCnt);
				tabs += "\t";

				index += VA("[idx%d]", idxCnt);
				indexZero += "[0]";
			}

			htFile.Append("%s\tx.%s%s%s = ht_bad_data(x.%s%s%s);\n",
				tabs.c_str(), m_.c_str(), pField->m_name.c_str(), index.c_str(), m_.c_str(), pField->m_name.c_str(), indexZero.c_str());

			tabs.erase(0, pField->m_dimenList.size());
		}
	}

	if (bHeader) {
		htFile.Append("%s\treturn x;\n", tabs.c_str());
		htFile.Append("%s}\n", tabs.c_str());
		htFile.Append("#%sendif\n", tabs.c_str());
		htFile.Append("\n");
	}
}

void
CDsnInfo::GenUserStructs(FILE *incFp, CRecord * pUserRecord, char const * pTabs)
{
	CHtCode htFile(incFp);

	GenUserStructs(htFile, pUserRecord, pTabs);
	htFile.Write(incFp);
}

void
CDsnInfo::GenUserStructs(CHtCode &htFile, CRecord * pUserRecord, char const * pTabs)
{
	string structName = pUserRecord->m_typeName + (pUserRecord->m_bInclude ? "Intf" : "");

	// mode: 0-all, 1-read, 2-write
	if (pUserRecord->m_bUnion)
		htFile.Append("%sunion %s {\n", pTabs, structName.c_str());
	else
		htFile.Append("%sstruct %s {\n", pTabs, structName.c_str());

	htFile.Append("#%s\tifndef _HTV\n", pTabs);
	if (!pUserRecord->m_bUnion) {
		htFile.Append("%s\t%s() {}\n", pTabs, structName.c_str());
		htFile.Append("%s\t%s(int zero)\n", pTabs, structName.c_str());
		htFile.Append("%s\t{\n", pTabs);
		htFile.Append("%s\t\toperator= (0);\n", pTabs);
		htFile.Append("%s\t}\n", pTabs);
	}

	// compare for equal
	const char *pStr = "";
	string prefixName;
	GenStructIsEqual(htFile, pTabs, prefixName, structName, pUserRecord->m_fieldList, pUserRecord->m_bCStyle, pStr);

	// assignment to zero
	pStr = "";
	prefixName = "";
	GenStructAssignToZero(htFile, pTabs, prefixName, structName, pUserRecord->m_fieldList, pUserRecord->m_bCStyle, pStr);

	// signal tracing
	pStr = "\t\t";
	prefixName = "";
	htFile.Append("#%s\tifdef HT_SYSC\n", pTabs);

	GenStructScTrace(htFile, pTabs, prefixName, prefixName, structName, pUserRecord->m_fieldList, pUserRecord->m_bCStyle, pStr);

	// signal printing
	htFile.Append("%s\tfriend ostream& operator << (ostream& os, %s const & v) { return os; }\n", pTabs, structName.c_str());

	htFile.Append("#%s\tendif\n", pTabs);
	htFile.Append("#%s\tendif\n", pTabs);

	bool bIsHtPriv = structName == "CHtPriv";
	GenUserStructFieldList(htFile, bIsHtPriv, pUserRecord->m_fieldList, pUserRecord->m_bCStyle, FieldList, pTabs);

	htFile.Append("%s};\n", pTabs);
	htFile.Append("\n");
}

void
CDsnInfo::GenStruct(FILE *incFp, string intfName, CRecord &ram, EGenStructMode mode, bool bEmptyContructor)
{
	// mode: 0-all, 1-read, 2-write
	if (ram.m_bUnion)
		fprintf(incFp, "\tunion %s {\n", intfName.c_str());
	else
		fprintf(incFp, "\tstruct %s {\n", intfName.c_str());

	if (bEmptyContructor) {
		fprintf(incFp, "#\t\tifndef _HTV\n");
		fprintf(incFp, "\t\t%s() {} // avoid uninitialized error\n", intfName.c_str());
		fprintf(incFp, "#\t\tendif\n");
		fprintf(incFp, "\n");
	}

	for (size_t fieldIdx = 0; fieldIdx < ram.m_fieldList.size(); fieldIdx += 1) {
		CField * pField = ram.m_fieldList[fieldIdx];

		if (mode == eGenStruct || mode == eGenStructRdData && pField->m_bSrcRead || mode == eGenStructWrData && pField->m_bSrcWrite) {
			// find typedef
			CTypeDef *pTypeDef = FindTypeDef(pField->m_type);

			if (pTypeDef && pTypeDef->m_width.size() > 0)
				fprintf(incFp, "\t\t%s m_%s:%s;\n", pTypeDef->m_type.c_str(), pField->m_name.c_str(), pTypeDef->m_width.c_str());
			else if (pTypeDef)
				fprintf(incFp, "\t\t%s m_%s;\n", pTypeDef->m_type.c_str(), pField->m_name.c_str());
			else
				fprintf(incFp, "\t\t%s m_%s;\n", pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());

		} else if (mode == eGenStructWrEn && pField->m_bSrcWrite)
			fprintf(incFp, "\t\tbool m_%s;\n", pField->m_name.c_str());
	}
	fprintf(incFp, "\t};\n");
	fprintf(incFp, "\n");
}

void
CDsnInfo::GenRamWrEn(FILE *incFp, string intfName, CRecord &ram)
{
	// mode: 0-all, 1-read, 2-write
	fprintf(incFp, "\tstruct %s {\n", intfName.c_str());

	for (size_t fieldIdx = 0; fieldIdx < ram.m_fieldList.size(); fieldIdx += 1) {
		CField * pField = ram.m_fieldList[fieldIdx];

		if (pField->m_bSrcWrite || pField->m_bMifWrite) {
			fprintf(incFp, "\t\tbool m_%s%s;\n", pField->m_name.c_str(), pField->m_dimenDecl.c_str());
		}
	}
	fprintf(incFp, "\t};\n");
	fprintf(incFp, "\n");
}

void
CDsnInfo::GenSmInfo(FILE *incFp, CRecord &smInfo)
{
	for (size_t fieldIdx = 0; fieldIdx < smInfo.m_fieldList.size(); fieldIdx += 1) {
		CField * pField = smInfo.m_fieldList[fieldIdx];

		fprintf(incFp, "\t%s %s;\n", pField->m_pType->m_typeName.c_str(), pField->m_name.c_str());
	}
	fprintf(incFp, "\n");
}

void
CDsnInfo::GenSmCmd(FILE *incFp, CModule &mod)
{
	if (!mod.m_bHasThreads)
		return;

	fprintf(incFp, "\tstruct CSmCmd {\n");
	fprintf(incFp, "\t\tuint32_t m_state:%d;\n", mod.m_instrW);
	fprintf(incFp, "\t\tuint32_t m_htId:%s;\n", mod.m_threads.m_htIdW.c_str());
	fprintf(incFp, "\t};\n");
}

void
CDsnInfo::GenCmdInfo(FILE *incFp, CModule &mod)
{
	string smInfoName = "CCmdInfo";
	GenStruct(incFp, smInfoName, mod.m_threads.m_htPriv, eGenStruct, true);
}

//////////////////////////////////
// class CStructElemIter methods

CStructElemIter::CStructElemIter(CDsnInfo * pDsnInfo, CType *pType/*string typeName*/, bool bFollowNamedStruct, bool bDimenIter)
{
	m_pDsnInfo = pDsnInfo;
	m_bDimenIter = bDimenIter;
	m_bFollowNamedStruct = bFollowNamedStruct;
	m_pType = pType;

	if (pType->m_eType == eRecord) {
		m_stack.push_back(CStack(pType->AsRecord(), -1, 0));
		operator++(0);
	} else
		m_stack.push_back(CStack(0, -1, 0));
}

bool CStructElemIter::end()
{
	return m_stack.size() == 0;
}

CField & CStructElemIter::operator()()
{
	CStack &stack = m_stack.back();
	CField * pField = stack.m_pRecord->m_fieldList[stack.m_fieldIdx];
	return *pField;
}

CField * CStructElemIter::operator -> ()
{
	CStack &stack = m_stack.back();
	CField * pField = stack.m_pRecord->m_fieldList[stack.m_fieldIdx];
	return pField;
}

void CStructElemIter::operator ++ (int)
{
	// check if incrementing an array index is sufficient
	if (m_stack.back().m_fieldIdx >= 0) {
		CField * pField1 = m_stack.back().m_pRecord->m_fieldList[m_stack.back().m_fieldIdx];
		if (m_bDimenIter && m_pDsnInfo->DimenIter(pField1->m_dimenList, m_stack.back().m_refList)) {
			m_stack.back().m_elemBitPos += pField1->m_pType->m_clangBitWidth;
			return;
		}
	}

	// check if non structure type
	if (m_stack.size() == 1 && m_stack.back().m_pRecord == 0) {
		m_stack.pop_back();
		return;
	}

	// advance to next field
	m_stack.back().m_fieldIdx += 1;
	m_stack.back().m_elemBitPos = 0;

	// check if we are at end of field list, if so, pop stack
	for (;;) {

		if (m_stack.back().m_fieldIdx < (int)m_stack.back().m_pRecord->m_fieldList.size()) {
			CField * pField2 = m_stack.back().m_pRecord->m_fieldList[m_stack.back().m_fieldIdx];
			m_stack.back().m_refList = vector<int>(pField2->m_dimenList.size());
			break;
		}

		m_stack.pop_back();

		if (m_stack.empty())
			return;

		CField * pField = m_stack.back().m_pRecord->m_fieldList[m_stack.back().m_fieldIdx];
		if (m_bDimenIter && m_pDsnInfo->DimenIter(pField->m_dimenList, m_stack.back().m_refList)) {
			m_stack.back().m_elemBitPos += pField->m_pType->m_clangBitWidth;
			break;
		}

		m_stack.back().m_fieldIdx += 1;
		m_stack.back().m_elemBitPos = 0;
	}

	// check if next field is a structure
	for (;;) {
		CField * pField3 = m_stack.back().m_pRecord->m_fieldList[m_stack.back().m_fieldIdx];
		CRecord * pStruct = pField3->m_pType->IsRecord() ? pField3->m_pType->AsRecord() : 0;

		bool bFollowStruct = pStruct && (pField3->m_name == "" || m_bFollowNamedStruct);

		if (bFollowStruct) {
			string heirName = m_stack.back().m_heirName;
			if (pField3->m_name.size() > 0)
				heirName += "." + pField3->m_name;
			if (m_bDimenIter)
				heirName += m_pDsnInfo->IndexStr(m_stack.back().m_refList);

			int heirBitPos = GetHeirFieldPos();

			m_stack.push_back(CStack(pStruct, 0, 0));
			m_stack.back().m_heirName = heirName;
			m_stack.back().m_heirBitPos = heirBitPos;

			CField * pField5 = m_stack.back().m_pRecord->m_fieldList[m_stack.back().m_fieldIdx];
			m_stack.back().m_refList = vector<int>(pField5->m_dimenList.size());
		} else {
			break;
		}
	}
}

bool CStructElemIter::IsStructOrUnion()
{
	CStack &stack = m_stack.back();
	if (stack.m_pRecord == 0) return false;
	CField * pField = stack.m_pRecord->m_fieldList[stack.m_fieldIdx];

	return pField->m_pType->IsRecord();
}

string CStructElemIter::GetHeirFieldName(bool bAddPreDot)
{
	CStack &stack = m_stack.back();
	if (stack.m_pRecord == 0) return string();
	CField * pField = stack.m_pRecord->m_fieldList[stack.m_fieldIdx];

	string heirFieldName;
	if (stack.m_heirName.size() > 0 || bAddPreDot)
		heirFieldName = stack.m_heirName + ".";
	heirFieldName += pField->m_name + m_pDsnInfo->IndexStr(stack.m_refList);

	return heirFieldName;
}

int CStructElemIter::GetHeirFieldPos()
{
	CStack &stack = m_stack.back();
	if (stack.m_pRecord == 0) return 0;
	CField * pField = stack.m_pRecord->m_fieldList[stack.m_fieldIdx];

	return stack.m_heirBitPos + pField->m_clangBitPos + stack.m_elemBitPos;
}

int CStructElemIter::GetWidth()
{
	CStack &stack = m_stack.back();
	if (stack.m_pRecord == 0) return m_pType->m_clangBitWidth;
	CField * pField = stack.m_pRecord->m_fieldList[stack.m_fieldIdx];

	return m_pDsnInfo->FindTypeWidth(pField);
}

CType * CStructElemIter::GetType()
{
	CStack &stack = m_stack.back();
	if (stack.m_pRecord == 0) return m_pType;
	CField * pField = stack.m_pRecord->m_fieldList[stack.m_fieldIdx];

	return pField->m_pType;
}

int CStructElemIter::GetMinAlign()
{
	CStack &stack = m_stack.back();
	if (stack.m_pRecord == 0) return m_pType->m_clangMinAlign;
	CField * pField = stack.m_pRecord->m_fieldList[stack.m_fieldIdx];

	return pField->m_pType->m_clangMinAlign;
}

bool CStructElemIter::IsSigned()
{
	CStack &stack = m_stack.back();
	if (stack.m_pRecord == 0) return m_pType->AsInt()->m_eSign == eSigned;
	CField * pField = stack.m_pRecord->m_fieldList[stack.m_fieldIdx];

	return pField->m_pType->AsInt()->m_eSign == eSigned;
}

int CStructElemIter::GetAtomicMask()
{
	CStack &stack = m_stack.back();
	if (stack.m_pRecord == 0) return 0;
	CField * pField = stack.m_pRecord->m_fieldList[stack.m_fieldIdx];

	return pField->m_atomicMask;
}

string CDsnInfo::GenRamIndexLoops(CHtCode &ramCode, vector<int> & dimenList, bool bOpenParen)
{
	string tabs;
	for (size_t i = 0; i < dimenList.size(); i += 1) {
		tabs += "\t";
		ramCode.Append("\tfor (int idx%d = 0; idx%d < %d; idx%d += 1)%s\n%s",
			(int)i + 1, (int)i + 1, dimenList[i], (int)i + 1, bOpenParen && i == dimenList.size() - 1 ? " {" : "", tabs.c_str());
	}
	return tabs;
}

string CDsnInfo::GenRamIndexLoops(CHtCode &ramCode, const char *pTabs, CDimenList &dimenList, bool bOpenParen)
{
	string loopIdx;
	string indentTabs;
	for (size_t i = 0; i < dimenList.m_dimenList.size(); i += 1) {
		bool bIsSigned = m_defineTable.FindStringIsSigned(dimenList.m_dimenList[i].AsStr());
		ramCode.Append("%s\tfor (%s idx%d = 0; idx%d < %d; idx%d += 1)%s\n\t%s", pTabs,
			bIsSigned ? "int" : "unsigned", (int)i + 1, (int)i + 1, dimenList.m_dimenList[i].AsInt(), (int)i + 1,
			bOpenParen && dimenList.m_dimenList.size() == i + 1 ? " {" : "", indentTabs.c_str());
		indentTabs += "\t";
		loopIdx += VA("[\" + (std::string)HtStrFmt(\"%%d\", idx%d) + \"]", (int)i + 1);
	}
	return loopIdx;
}

void CDsnInfo::GenRamIndexLoops(FILE *fp, CDimenList &dimenList, const char *pTabs)
{
	string indentTabs;
	for (size_t i = 0; i < dimenList.m_dimenList.size(); i += 1) {
		bool bIsSigned = m_defineTable.FindStringIsSigned(dimenList.m_dimenList[i].AsStr());
		fprintf(fp, "%sfor (%s idx%d = 0; idx%d < %s; idx%d += 1)\n\t%s",
			pTabs, bIsSigned ? "int" : "unsigned", (int)i + 1, (int)i + 1, dimenList.m_dimenList[i].c_str(), (int)i + 1, indentTabs.c_str());
		indentTabs += "\t";
	}
}

void CDsnInfo::GenIndexLoops(CHtCode &htCode, string &tabs, vector<CHtString> & dimenList, bool bOpenParen)
{
	for (size_t i = 0; i < dimenList.size(); i += 1) {
		htCode.Append("%sfor (int idx%d = 0; idx%d < %d; idx%d += 1)%s\n",
			tabs.c_str(), (int)i + 1, (int)i + 1, dimenList[i].AsInt(), (int)i + 1,
			bOpenParen && i == dimenList.size() - 1 ? " {" : "", tabs.c_str());
		tabs += "\t";
	}
}

// Code genertion loop / unroll routines
CLoopInfo::CLoopInfo(CHtCode &htCode, string &tabs, vector<CHtString> & dimenList, int stmtCnt)
	: m_htCode(htCode), m_tabs(tabs), m_dimenList(dimenList),
	m_stmtCnt(stmtCnt), m_refList(dimenList.size()), m_unrollList(dimenList.size())
{
	for (size_t i = 0; i < m_dimenList.size(); i += 1) {
		m_unrollList[i] = false;// stmtCnt <= 3;
		if (m_unrollList[i])
			stmtCnt *= m_dimenList[i].AsInt();

		if (!m_unrollList[i]) {
			htCode.Append("%sfor (int idx%d = 0; idx%d < %d; idx%d += 1)%s\n",
				m_tabs.c_str(), (int)i + 1, (int)i + 1, m_dimenList[i].AsInt(), (int)i + 1, stmtCnt > 1 ? " {" : "");
			tabs += "\t";
		}
	}
}

string CLoopInfo::IndexStr(int startPos, int endPos, bool bParamStr)
{
	if (startPos == -1) {
		startPos = 0;
		endPos = (int)m_refList.size();
	}
	string index;
	for (int i = startPos; i < endPos; i += 1) {
		char buf[8];
		if (bParamStr) {
			if (i == startPos)
				sprintf(buf, "%d", m_refList[i]);
			else
				sprintf(buf, ", %d", m_refList[i]);
		} else {
			if (m_unrollList[i])
				sprintf(buf, "[%d]", m_refList[i]);
			else
				sprintf(buf, "[idx%d]", i + 1);
		}
		index += buf;
	}
	return index;
}

bool CLoopInfo::Iter(bool bNewLine)
{
	// refList must be same size as dimenList and all elements zero
	HtlAssert(m_dimenList.size() == m_refList.size());

	bool bMore;

	size_t i;
	for (i = 0; i < m_unrollList.size(); i += 1) {
		if (m_unrollList[i]) {
			m_refList[i] += 1;
			break;
		}
	}
	if (i == m_unrollList.size()) {
		LoopEnd(bNewLine);
		return false;
	}

	if (m_dimenList.size() > 0) {
		bMore = true;
		for (; i < m_dimenList.size(); i += 1) {
			if (m_unrollList[i] && m_refList[i] < m_dimenList[i].AsInt())
				break;
			else {
				m_refList[i] = 0;
				size_t j;
				for (j = i + 1; j < m_dimenList.size(); j += 1) {
					if (!m_unrollList[j]) continue;
					m_refList[j] += 1;
					i = j - 1;
					break;
				}
				if (j == m_dimenList.size())
					bMore = false;
			}
		}
	} else
		bMore = false;

	if (!bMore)
		LoopEnd(bNewLine);

	return bMore;
}

void CLoopInfo::LoopEnd(bool bNewLine)
{
	int stmtCnt = m_stmtCnt;
	for (size_t i = 0; i < m_dimenList.size(); i += 1) {
		if (m_unrollList[i])
			stmtCnt *= m_dimenList[i].AsInt();

		if (!m_unrollList[i]) {
			m_tabs.erase(0, 1);
			if (stmtCnt > 1)
				m_htCode.Append("%s}\n", m_tabs.c_str());
		}
	}
	if (bNewLine)
		m_htCode.NewLine();
}

bool CType::IsEmbeddedUnion() {
	// check if there is a union in the type
	if (IsRecord()) {
		CRecord * pRecord = AsRecord();
		if (pRecord->m_bUnion)
			return true;
		for (size_t i = 0; i < pRecord->m_fieldList.size(); i += 1) {
			CField * pField = pRecord->m_fieldList[i];
			if (pField->m_pType->IsEmbeddedUnion())
				return true;
		}
	}

	return false;
}
