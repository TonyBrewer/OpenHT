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

void CDsnInfo::GenerateCommonIncludeFile()
{
	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

	string fileName = g_appArgs.GetOutputFolder() + "/Pers" + unitNameUc + "Common.h";

	CHtFile incFile(fileName.c_str(), "w");

	GenerateBanner(incFile, fileName.c_str(), true);

	for (size_t i = 0; i < m_includeList.size(); i += 1) {
		// only print include if not in src directory
		string &fullPath = m_includeList[i].m_fullPath;
		size_t pos = fullPath.find_last_of("/\\");
		if (strncmp(fullPath.c_str() + pos - 3, "src", 3) != 0)
			fprintf(incFile, "#include \"%s\"\n", m_includeList[i].m_name.c_str());
	}

	if (m_includeList.size() > 0)
		fprintf(incFile, "\n");

	fprintf(incFile, "#ifndef _HTV\n");
	fprintf(incFile, "#include \"sysc/SyscMon.h\"\n");
	fprintf(incFile, "#endif\n");
	fprintf(incFile, "\n");

	fprintf(incFile, "//////////////////////////////////\n");
	fprintf(incFile, "// Common #define's\n");
	fprintf(incFile, "\n");

	fprintf(incFile, "#define HT_INVALID 0\n");
	fprintf(incFile, "#define HT_CONT 1\n");
	fprintf(incFile, "#define HT_CALL 2\n");
	fprintf(incFile, "#define HT_RTN 3\n");
	fprintf(incFile, "#define HT_JOIN 4\n");
	fprintf(incFile, "#define HT_RETRY 5\n");
	fprintf(incFile, "#define HT_TERM 6\n");
	fprintf(incFile, "#define HT_PAUSE 7\n");
	fprintf(incFile, "#define HT_JOIN_AND_CONT 8\n");
	fprintf(incFile, "\n");

	for (size_t i = 0; i< m_defineTable.size(); i += 1) {
		//	must be a unit or host
		if (m_defineTable[i].m_scope.compare("unit") == 0 && !m_defineTable[i].m_bPreDefined) {
			fprintf(incFile, "#define %s",
				m_defineTable[i].m_name.c_str());
			if (m_defineTable[i].m_bHasParamList) {
				fprintf(incFile, "(");
				char * pSeparator = "";
				for (size_t j = 0; j < m_defineTable[i].m_paramList.size(); j += 1) {
					fprintf(incFile, "%s%s", pSeparator, m_defineTable[i].m_paramList[j].c_str());
					pSeparator = ",";
				}
				fprintf(incFile, ")");
			}
			fprintf(incFile, " %s\n",
				m_defineTable[i].m_value.c_str());
		}
	}
	if (GetMacroTblSize() > 0 || m_defineTable.size() > 0)
		fprintf(incFile, "\n");

	fprintf(incFile, "//////////////////////////////////\n");
	fprintf(incFile, "// Common typedef's\n");
	fprintf(incFile, "\n");

	for (size_t typedefIdx = 0; typedefIdx < m_typedefList.size(); typedefIdx += 1) {
		CTypeDef &typeDef = m_typedefList[typedefIdx];
		if (typeDef.m_scope.compare("unit") != 0) {
			continue;							// host or module or auto
		}

		if (typeDef.m_width.size() == 0)
			fprintf(incFile, "typedef %s %s;\n", typeDef.m_type.c_str(), typeDef.m_name.c_str());

		else {
			int width = typeDef.m_width.AsInt();

			if (typeDef.m_name == "uint8_t" || typeDef.m_name == "uint16_t" || typeDef.m_name == "uint32_t" || typeDef.m_name == "uint64_t"
				|| typeDef.m_name == "int8_t" || typeDef.m_name == "int16_t" || typeDef.m_name == "int32_t" || typeDef.m_name == "int64_t")
				continue;

			if (width > 0) {
				if (typeDef.m_type.substr(0, 3) == "int")
					fprintf(incFile, "typedef %s<%s> %s;\n",
					width > 64 ? "sc_bigint" : "sc_int", typeDef.m_width.c_str(), typeDef.m_name.c_str());
				else if (typeDef.m_type.substr(0, 4) == "uint")
					fprintf(incFile, "typedef %s<%s> %s;\n",
					width > 64 ? "sc_biguint" : "sc_uint", typeDef.m_width.c_str(), typeDef.m_name.c_str());
				else
					ParseMsg(Fatal, typeDef.m_lineInfo, "unexpected type '%s'", typeDef.m_type.c_str());
			}
		}
	}
	fprintf(incFile, "\n");

	fprintf(incFile, "#ifdef _HTV\n");
	fprintf(incFile, "#define INT(a) a\n");
	fprintf(incFile, "#else\n");
	fprintf(incFile, "#define INT(a) int(a)\n");
	fprintf(incFile, "#define HT_CYC_1X ((long long)sc_time_stamp().value() / 10000)\n");
	fprintf(incFile, "#endif\n");
	fprintf(incFile, "\n");

	fprintf(incFile, "//////////////////////////////////\n");
	fprintf(incFile, "// Ht Assert interfaces\n");
	fprintf(incFile, "\n");

	fprintf(incFile, "#ifdef HT_ASSERT\n");
	fprintf(incFile, "#\tifdef _HTV\n");
	fprintf(incFile, "#\t\tdefine HtAssert(expression, info) \\\n");
	fprintf(incFile, "\t\tif (!c_htAssert.m_bAssert && !(expression)) {\\\n");
	fprintf(incFile, "\t\t\tc_htAssert.m_bAssert = true; \\\n");
	fprintf(incFile, "\t\t\tc_htAssert.m_module = HT_MOD_ID; \\\n");
	fprintf(incFile, "\t\t\tc_htAssert.m_lineNum = __LINE__; \\\n");
	fprintf(incFile, "\t\t\tc_htAssert.m_info = info;\\\n");
	fprintf(incFile, "\t\t}\n");
	fprintf(incFile, "#\telse\n");
	fprintf(incFile, "#\t\tdefine HtAssert(expression, info) \\\n");
	fprintf(incFile, "\t\tif (!(expression)) {\\\n");
	fprintf(incFile, "\t\t\tfprintf(stderr, \"HtAssert: file %%s, line %%d, info 0x%%x\\n\", __FILE__, __LINE__, (int)info);\\\n");
	fprintf(incFile, "\t\t\tif (Ht::g_vcdp) sc_close_vcd_trace_file(Ht::g_vcdp);\\\n");
	fprintf(incFile, "\t\t\tassert(expression);\\\n");
	fprintf(incFile, "\t\t\tuint32_t *pNull = 0; *pNull = 0;\\\n");
	fprintf(incFile, "\t\t}\n");
	fprintf(incFile, "#\t\tendif\n");
	fprintf(incFile, "#else\n");
	fprintf(incFile, "#\tdefine HtAssert(expression, info) assert(expression)\n");
	fprintf(incFile, "#endif\n");
	fprintf(incFile, "\n");

	CRecord htAssert;
	htAssert.AddStructField(&g_bool, "bAssert");
	htAssert.AddStructField(&g_bool, "bCollision");
	htAssert.AddStructField(FindHtIntType(eUnsigned, 8), "module");
	htAssert.AddStructField(FindHtIntType(eUnsigned, 16), "lineNum");
	htAssert.AddStructField(FindHtIntType(eUnsigned, 32), "info");
	GenIntfStruct(incFile, "CHtAssert", htAssert.m_fieldList, htAssert.m_bCStyle, false, false, false);

	fprintf(incFile, "#ifdef _HTV\n");
	fprintf(incFile, "#define assert_msg(bCond, ...)  HtAssert(bCond, 0)\n");
	fprintf(incFile, "#else\n");
	fprintf(incFile, "#define assert_msg(...) if (!assert_msg_(__VA_ARGS__)) assert(0)\n");
	fprintf(incFile, "\n");
	fprintf(incFile, "inline bool assert_msg_(bool bCond, char const * pFmt, ... ) {\n");
	fprintf(incFile, "\tif (bCond) return true;\n");
	fprintf(incFile, "\tva_list args;\n");
	fprintf(incFile, "\tva_start( args, pFmt );\n");
	fprintf(incFile, "\tvprintf(pFmt, args);\n");
	fprintf(incFile, "\tif (Ht::g_vcdp) sc_close_vcd_trace_file(Ht::g_vcdp);\n");
	fprintf(incFile, "\treturn false;\n");
	fprintf(incFile, "}\n");
	fprintf(incFile, "#endif\n");
	fprintf(incFile, "\n");

	fprintf(incFile, "//////////////////////////////////\n");
	fprintf(incFile, "// Common structs and unions\n");
	fprintf(incFile, "\n");

	for (size_t recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {

		if (m_recordList[recordIdx]->m_scope != eUnit && !m_recordList[recordIdx]->m_bInclude) continue;
		//if (!m_recordList[recordIdx]->m_bNeedIntf) continue;

		if (m_recordList[recordIdx]->m_bCStyle)
			GenIntfStruct(incFile, m_recordList[recordIdx]->m_typeName, m_recordList[recordIdx]->m_fieldList,
			m_recordList[recordIdx]->m_bCStyle, m_recordList[recordIdx]->m_bInclude,
			false, m_recordList[recordIdx]->m_bUnion);
		else
			GenUserStructs(incFile, m_recordList[recordIdx]);

		CHtCode htFile(incFile);
		GenUserStructBadData(htFile, true, m_recordList[recordIdx]->m_typeName,
			m_recordList[recordIdx]->m_fieldList, m_recordList[recordIdx]->m_bCStyle, "");
	}
	fprintf(incFile, "\n");

	if (m_ngvList.size() > 0) {
		fprintf(incFile, "//////////////////////////////////\n");
		fprintf(incFile, "// Global variable write structs\n");
		fprintf(incFile, "\n");

		vector<string> gvTypeList;

		for (size_t gvIdx = 0; gvIdx < m_ngvList.size(); gvIdx += 1) {
			CNgvInfo * pNgvInfo = m_ngvList[gvIdx];
			CRam * pNgv = pNgvInfo->m_modInfoList[0].m_pNgv;
			GenGlobalVarWriteTypes(incFile, pNgv->m_pType, pNgvInfo->m_atomicMask, gvTypeList, pNgv);
		}
	}

	fprintf(incFile, "//////////////////////////////////\n");
	fprintf(incFile, "// Host Control interfaces\n");
	fprintf(incFile, "\n");

	CRecord ctrlMsgIntf;
	ctrlMsgIntf.AddStructField(&g_uint64, "bValid", "1");
	ctrlMsgIntf.AddStructField(&g_uint64, "msgType", "7");
	ctrlMsgIntf.AddStructField(&g_uint64, "msgData", "56");
	GenIntfStruct(incFile, "CHostCtrlMsg", ctrlMsgIntf.m_fieldList, false, false, true, false);

	fprintf(incFile, "//////////////////////////////////\n");
	fprintf(incFile, "// Host Queue interfaces\n");
	fprintf(incFile, "\n");

	fprintf(incFile, "#define HT_DQ_NULL\t1\n");
	fprintf(incFile, "#define HT_DQ_DATA\t2\n");
	fprintf(incFile, "#define HT_DQ_FLSH\t3\n");
	fprintf(incFile, "#define HT_DQ_CALL\t6\n");
	fprintf(incFile, "#define HT_DQ_HALT\t7\n");


	CRecord hostDataQue;
	hostDataQue.AddStructField(FindHtIntType(eUnsigned, 64), "data");
	hostDataQue.AddStructField(FindHtIntType(eUnsigned, 3), "ctl");
	GenIntfStruct(incFile, "CHostDataQue", hostDataQue.m_fieldList, false, false, false, false);

	fprintf(incFile, "//////////////////////////////////\n");
	fprintf(incFile, "// Module Queue interfaces\n");
	fprintf(incFile, "\n");

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		for (size_t queIdx = 0; queIdx < mod.m_queIntfList.size(); queIdx += 1) {
			CQueIntf & queIntf = mod.m_queIntfList[queIdx];

			bool bHifQue = queIntf.m_modName.Lc() == "hif" || mod.m_modName.Lc() == "hif";

			if (!bHifQue) {
				char queIntfName[256];
				if (queIntf.m_queType == Pop)
					sprintf(queIntfName, "C%sTo%s_%sIntf", queIntf.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());
				else
					sprintf(queIntfName, "C%sTo%s_%sIntf", mod.m_modName.Uc().c_str(), queIntf.m_modName.Uc().c_str(), queIntf.m_queName.Lc().c_str());

				queIntf.m_typeName = queIntfName;
			} else
				queIntf.m_typeName = "CHostDataQueIntf";

			if (queIntf.m_queType == Push || bHifQue) continue;

			GenUserStructs(incFile, &queIntf);

			CHtCode htFile(incFile);
			GenUserStructBadData(htFile, true, queIntf.m_typeName, queIntf.m_fieldList, queIntf.m_bCStyle, "");
		}
	}

	fprintf(incFile, "//////////////////////////////////\n");
	fprintf(incFile, "// Command interfaces\n");
	fprintf(incFile, "\n");

	// generate interface struct for all inbound cxr interfaces (that are non-empty)
	vector<string> intfNameList;
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;

		for (int modInstIdx = 0; modInstIdx < mod.m_instSet.GetInstCnt(); modInstIdx += 1) {
			CInstance * pModInst = mod.m_instSet.GetInst(modInstIdx);

			for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

				if (pCxrIntf->m_cxrDir == CxrOut) continue;

				char intfName[256];
				sprintf(intfName, "C%s_%s", pCxrIntf->GetSrcToDstUc(), pCxrIntf->GetIntfName());

				size_t i;
				for (i = 0; i < intfNameList.size(); i += 1) {
					if (intfNameList[i] == intfName)
						break;
				}

				if (i < intfNameList.size())
					continue;

				intfNameList.push_back(intfName);

				pCxrIntf->m_fullFieldList = *pCxrIntf->m_pFieldList;
				CLineInfo lineInfo;

				if (pCxrIntf->m_cxrType == CxrCall) {

					if (pCxrIntf->m_pSrcGroup->m_htIdW.AsInt() > 0) {
						pCxrIntf->m_fullFieldList.push_back(new CField(pCxrIntf->m_pSrcGroup->m_pHtIdType, "rtnHtId"));
						pCxrIntf->m_fullFieldList.back()->InitDimen(lineInfo);
					}
					if (pCxrIntf->m_pSrcModInst->m_pMod->m_pInstrType) {
						pCxrIntf->m_fullFieldList.push_back(new CField(pCxrIntf->m_pSrcModInst->m_pMod->m_pInstrType, "rtnInstr"));
						pCxrIntf->m_fullFieldList.back()->InitDimen(lineInfo);
					}

				} else if (pCxrIntf->m_cxrType == CxrTransfer) { // else Transfer

					if (pCxrIntf->m_pSrcGroup->m_rtnSelW > 0) {
						pCxrIntf->m_fullFieldList.push_back(new CField(pCxrIntf->m_pSrcGroup->m_pRtnSelType, "rtnSel"));
						pCxrIntf->m_fullFieldList.back()->InitDimen(lineInfo);
					}
					if (pCxrIntf->m_pSrcGroup->m_rtnHtIdW > 0) {
						pCxrIntf->m_fullFieldList.push_back(new CField(pCxrIntf->m_pSrcGroup->m_pRtnHtIdType, "rtnHtId"));
						pCxrIntf->m_fullFieldList.back()->InitDimen(lineInfo);
					}
					if (pCxrIntf->m_pSrcGroup->m_rtnInstrW > 0) {
						pCxrIntf->m_fullFieldList.push_back(new CField(pCxrIntf->m_pSrcGroup->m_pRtnInstrType, "rtnInstr"));
						pCxrIntf->m_fullFieldList.back()->InitDimen(lineInfo);
					}

				} else { // CxrReturn

					if (pCxrIntf->m_pDstGroup->m_htIdW.AsInt() > 0) {
						pCxrIntf->m_fullFieldList.push_back(new CField(pCxrIntf->m_pDstGroup->m_pHtIdType, "rtnHtId"));
						pCxrIntf->m_fullFieldList.back()->InitDimen(lineInfo);
					}
					if (pCxrIntf->m_pDstModInst->m_pMod->m_pInstrType != 0) {
						pCxrIntf->m_fullFieldList.push_back(new CField(pCxrIntf->m_pDstModInst->m_pMod->m_pInstrType, "rtnInstr"));
						pCxrIntf->m_fullFieldList.back()->InitDimen(lineInfo);
					}

				}

				if (pCxrIntf->m_bRtnJoin) {
					pCxrIntf->m_fullFieldList.push_back(new CField(&g_bool, "rtnJoin"));
					pCxrIntf->m_fullFieldList.back()->InitDimen(lineInfo);
				}

				if (pCxrIntf->m_fullFieldList.size() == 0)
					continue;

				GenIntfStruct(incFile, intfName, pCxrIntf->m_fullFieldList, false, false, false, false);
			}
		}
	}

	incFile.FileClose();
}

void CDsnInfo::MarkNeededIncludeStructs()
{
	for (size_t recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
		CRecord * pRecord = m_recordList[recordIdx];

		if (pRecord->m_scope != eUnit && !pRecord->m_bInclude) continue;
		if (!pRecord->m_bNeedIntf) continue;

		// walk struct and see it if has embedded structures
		MarkNeededIncludeStructs(pRecord);
	}
}

void CDsnInfo::MarkNeededIncludeStructs(CRecord * pRecord)
{
	for (size_t fieldIdx = 0; fieldIdx < pRecord->m_fieldList.size(); fieldIdx += 1) {
		CField * pField = pRecord->m_fieldList[fieldIdx];

		for (size_t struct2Idx = 0; struct2Idx < m_recordList.size(); struct2Idx += 1) {
			CRecord * pRecord2 = m_recordList[struct2Idx];

			if (pRecord2->m_typeName == pField->m_pType->m_typeName && !pRecord2->m_bNeedIntf) {
				pRecord2->m_bNeedIntf = true;
				MarkNeededIncludeStructs(pRecord2);
			}
		}
	}
}

void CDsnInfo::GenGlobalVarWriteTypes(CHtFile & htFile, CType * pType, int &atomicMask, vector<string> &gvTypeList, CRam * pGv)
{
	string ramTypeName = pType->m_typeName;
	if (pGv) {
		if (pGv->m_addr0W.AsInt() > 0)
			ramTypeName += VA("_H%d", pGv->m_addr0W.AsInt());
		if (pGv->m_addr1W.AsInt() > 0)
			ramTypeName += VA("_%c%d", pGv->m_addr1Name == "htId" ? 'H' : 'A', pGv->m_addr1W.AsInt());
		if (pGv->m_addr2W.AsInt() > 0)
			ramTypeName += VA("_%c%d", pGv->m_addr2Name == "htId" ? 'H' : 'A', pGv->m_addr2W.AsInt());

		pGv->m_pNgvInfo->m_ngvWrType = ramTypeName;
	}

	string wrTypeName = "CGW_" + ramTypeName;
	switch (atomicMask) {
	case ATOMIC_INC: wrTypeName += "_I"; break;
	case ATOMIC_SET: wrTypeName += "_S"; break;
	case ATOMIC_ADD: wrTypeName += "_A"; break;
	default: break;
	}

	CRecord * pRecord = pType->AsRecord();

	// check if type already generated
	for (size_t i = 0; i < gvTypeList.size(); i += 1) {
		if (gvTypeList[i] == wrTypeName) {
			if (pRecord)
				atomicMask |= pRecord->m_atomicMask;
			return;
		}
	}

	gvTypeList.push_back(wrTypeName);

	if (pRecord) {
		for (CStructElemIter iter(this, pRecord, false, false); !iter.end(); iter++) {
			CField & field = iter();

			CType * pFieldType = 0;
			if (field.m_fieldWidth.size() > 0) {
				int width;
				bool bSigned;
				bool bFound = FindCIntType(field.m_pType->m_typeName, width, bSigned);
				HtlAssert(bFound);

				pFieldType = FindHtIntType(bSigned ? eSigned : eUnsigned, field.m_fieldWidth.AsInt());

			} else {
				pFieldType = field.m_pType;
			}

			GenGlobalVarWriteTypes(htFile, pFieldType, field.m_atomicMask, gvTypeList);

			pRecord->m_atomicMask |= field.m_atomicMask;
		}
	}

	fprintf(htFile, "struct %s {\n", wrTypeName.c_str());
	fprintf(htFile, "#\tifndef _HTV\n");
	fprintf(htFile, "\tbool operator == (const %s & rhs) const\n", wrTypeName.c_str());
	fprintf(htFile, "\t{\n");

	if (pGv && pGv->m_addrW > 0) {
		fprintf(htFile, "\t\tif (!(m_bAddr == rhs.m_bAddr)) return false;\n");
		fprintf(htFile, "\t\tif (!(m_addr == rhs.m_addr)) return false;\n");
	}

	if (pRecord) {
		for (CStructElemIter iter(this, pRecord, false); !iter.end(); iter++) {
			fprintf(htFile, "\t\tif (!(%s == rhs.%s)) return false;\n",
				iter.GetHeirFieldName(false).c_str(), iter.GetHeirFieldName(false).c_str());
		}
	} else {
		fprintf(htFile, "\t\tif (!(m_bWrite == rhs.m_bWrite)) return false;\n");
		switch (atomicMask) {
		case ATOMIC_INC: fprintf(htFile, "\t\tif (!(m_bInc == rhs.m_bInc)) return false;\n"); break;
		case ATOMIC_SET: fprintf(htFile, "\t\tif (!(m_bSet == rhs.m_bSet)) return false;\n"); break;
		case ATOMIC_ADD: fprintf(htFile, "\t\tif (!(m_bAdd == rhs.m_bAdd)) return false;\n"); break;
		default: break;
		}
		fprintf(htFile, "\t\tif (!(m_data == rhs.m_data)) return false;\n");
	}

	fprintf(htFile, "\t\treturn true;\n");
	fprintf(htFile, "\t}\n");

	bool bInitBAddrFalse = false;
	if (pGv) {
		bool bAddr1EqHtId = pGv->m_addr1Name == "htId";
		bool bAddr2EqHtId = pGv->m_addr2Name == "htId";
		bInitBAddrFalse = pGv->m_addr1W.size() > 0 && !bAddr1EqHtId || pGv->m_addr2W.size() > 0 && !bAddr2EqHtId;
	}

	if (pRecord) {
		fprintf(htFile, "\tvoid operator = (int zero) {\n");
		fprintf(htFile, "\t\tassert(zero == 0);\n");

		if (pGv && pGv->m_addrW > 0) {
			fprintf(htFile, "\t\tm_bAddr = %s;\n", bInitBAddrFalse ? "false" : "true");
			fprintf(htFile, "\t\tm_addr = 0;\n");
		}

		for (CStructElemIter iter(this, pRecord, false); !iter.end(); iter++) {
			fprintf(htFile, "\t\t%s = 0;\n", iter.GetHeirFieldName(false).c_str());
		}

		fprintf(htFile, "\t}\n");
	}
	fprintf(htFile, "#\tifdef HT_SYSC\n");
	fprintf(htFile, "\tfriend void sc_trace(sc_trace_file *tf, const %s & v, const std::string & NAME)\n", wrTypeName.c_str());
	fprintf(htFile, "\t{\n");

	if (pGv && pGv->m_addrW > 0) {
		fprintf(htFile, "\t\tsc_trace(tf, v.m_bAddr, NAME + \".m_bAddr\");\n");
		fprintf(htFile, "\t\tsc_trace(tf, v.m_addr, NAME + \".m_addr\");\n");
	}

	if (pRecord) {
		for (CStructElemIter iter(this, pRecord/*->m_structName*/, false); !iter.end(); iter++) {
			fprintf(htFile, "\t\tsc_trace(tf, v.%s, NAME + \".%s\");\n",
				iter.GetHeirFieldName(false).c_str(), iter.GetHeirFieldName(false).c_str());
		}
	} else {
		fprintf(htFile, "\t\tsc_trace(tf, v.m_bWrite, NAME + \".m_bWrite\");\n");
		switch (atomicMask) {
		case ATOMIC_INC: fprintf(htFile, "\t\tsc_trace(tf, v.m_bInc, NAME + \".m_bInc\");\n"); break;
		case ATOMIC_SET: fprintf(htFile, "\t\tsc_trace(tf, v.m_bSet, NAME + \".m_bSet\");\n"); break;
		case ATOMIC_ADD: fprintf(htFile, "\t\tsc_trace(tf, v.m_bAdd, NAME + \".m_bAdd\");\n"); break;
		default: break;
		}
		fprintf(htFile, "\t\tsc_trace(tf, v.m_data, NAME + \".m_data\");\n");
	}

	fprintf(htFile, "\t}\n");
	fprintf(htFile, "\tfriend ostream & operator << (ostream & os, %s const & v) { return os; }\n", wrTypeName.c_str());
	fprintf(htFile, "#\tendif\n");
	fprintf(htFile, "#\tendif\n");

	if (pRecord) {
		if (pGv) {
			int htIdW = 0;
			if (pGv->m_addr0W.AsInt() > 0)
				htIdW = pGv->m_addr0W.AsInt();
			else if (pGv->m_addr1Name == "htId")
				htIdW = pGv->m_addr1W.AsInt();
			else if (pGv->m_addr2Name == "htId")
				htIdW = pGv->m_addr2W.AsInt();

			if (htIdW > 0)
				fprintf(htFile, "\tvoid InitZero(ht_uint%d htId=0) {\n", htIdW);
			else
				fprintf(htFile, "\tvoid InitZero() {\n");

			if (pGv->m_addrW > 0) {
				fprintf(htFile, "\t\tm_bAddr = %s;\n", bInitBAddrFalse ? "false" : "true");
				fprintf(htFile, "\t\tm_addr = 0;\n");
			}

			if (htIdW > 0) {
				if (pGv->m_addr0W.AsInt() > 0)
					fprintf(htFile, "\t\tm_addr(%d, %d) = htId;\n", pGv->m_addrW - 1, pGv->m_addrW - pGv->m_addr0W.AsInt());
				if (pGv->m_addr1Name == "htId")
					fprintf(htFile, "\t\tm_addr(%d, %d) = htId;\n", pGv->m_addr1W.AsInt() + pGv->m_addr2W.AsInt() - 1, pGv->m_addr2W.AsInt());
				if (pGv->m_addr2Name == "htId")
					fprintf(htFile, "\t\tm_addr(%d, 0) = htId;\n", pGv->m_addr2W.AsInt() - 1);
			}
		} else
			fprintf(htFile, "\tvoid InitZero() {\n");

		CHtCode htCode(htFile);
		for (CStructElemIter iter(this, pRecord/*->m_structName*/, false); !iter.end(); iter++) {
			fprintf(htFile, "\t\t%s.InitZero();\n", iter.GetHeirFieldName(false).c_str());
		}
		fprintf(htFile, "\t}\n");

		if (pGv) {
			int htIdW = 0;
			if (pGv->m_addr0W.AsInt() > 0)
				htIdW = pGv->m_addr0W.AsInt();
			else if (pGv->m_addr1Name == "htId")
				htIdW = pGv->m_addr1W.AsInt();
			else if (pGv->m_addr2Name == "htId")
				htIdW = pGv->m_addr2W.AsInt();

			if (htIdW > 0) {
				fprintf(htFile, "\tvoid InitData(ht_uint%d htId, %s _data_)\n", htIdW, pType->m_typeName.c_str());
				fprintf(htFile, "\t{\n");

				if (pGv->m_addrW > 0) {
					fprintf(htFile, "\t\tm_bAddr = %s;\n", bInitBAddrFalse ? "false" : "true");
					fprintf(htFile, "\t\tm_addr = 0;\n");
				}
				if (pGv->m_addr0W.AsInt() > 0)
					fprintf(htFile, "\t\tm_addr(%d, %d) = htId;\n", pGv->m_addrW - 1, pGv->m_addrW - pGv->m_addr0W.AsInt());
				if (pGv->m_addr1Name == "htId")
					fprintf(htFile, "\t\tm_addr(%d, %d) = htId;\n", pGv->m_addr1W.AsInt() + pGv->m_addr2W.AsInt() - 1, pGv->m_addr2W.AsInt());
				if (pGv->m_addr2Name == "htId")
					fprintf(htFile, "\t\tm_addr(%d, 0) = htId;\n", pGv->m_addr2W.AsInt() - 1);
			} else {
				fprintf(htFile, "\tvoid InitData(%s _data_)\n", pType->m_typeName.c_str());
				fprintf(htFile, "\t{\n");

				if (pGv->m_addrW > 0) {
					fprintf(htFile, "\t\tm_bAddr = %s;\n", bInitBAddrFalse ? "false" : "true");
					fprintf(htFile, "\t\tm_addr = 0;\n");
				}
			}

			for (CStructElemIter iter(this, pRecord, false); !iter.end(); iter++) {
				fprintf(htFile, "\t\t%s.InitData(_data_.%s);\n", 
					iter.GetHeirFieldName(false).c_str(), iter.GetHeirFieldName(false).c_str());
			}
			fprintf(htFile, "\t}\n");
		} else {
			fprintf(htFile, "\tvoid InitData(%s _data_)\n", pType->m_typeName.c_str());
			fprintf(htFile, "\t{\n");

			for (CStructElemIter iter(this, pRecord, false); !iter.end(); iter++) {
				fprintf(htFile, "\t\t%s.InitData(_data_.%s);\n",
					iter.GetHeirFieldName(false).c_str(), iter.GetHeirFieldName(false).c_str());
			}
			fprintf(htFile, "\t}\n");
		}

		if (pGv) {
			bool bAddr1EqHtId = pGv->m_addr1Name == "htId";
			bool bAddr2EqHtId = pGv->m_addr2Name == "htId";
			bool bAddr1WEq0 = pGv->m_addr1W.AsInt() == 0;
			bool bAddr2WEq0 = pGv->m_addr2W.AsInt() == 0;

			if (pGv->m_addr1W.size() > 0 && !bAddr1EqHtId || pGv->m_addr2W.size() > 0 && !bAddr2EqHtId) {
				fprintf(htFile, "\tvoid write_addr(");
				if (pGv->m_addr1W.size() > 0 && !bAddr1EqHtId)
					fprintf(htFile, "ht_uint%d addr1", bAddr1WEq0 ? 1 : pGv->m_addr1W.AsInt());
				if (pGv->m_addr1W.size() > 0 && !bAddr1EqHtId && pGv->m_addr2W.size() > 0 && !bAddr2EqHtId)
					fprintf(htFile, ", ");
				if (pGv->m_addr2W.size() > 0 && !bAddr2EqHtId)
					fprintf(htFile, "ht_uint%d addr2", bAddr2WEq0 ? 1 : pGv->m_addr2W.AsInt());
				fprintf(htFile, ")\n");
				fprintf(htFile, "\t{\n");

				if (pGv->m_addr1W.size() > 0 && bAddr1WEq0)
					fprintf(htFile, "\t\tht_assert(addr1 == 0); // addr1 bounds check\n");

				if (pGv->m_addr2W.size() > 0 && bAddr2WEq0)
					fprintf(htFile, "\t\tht_assert(addr2 == 0); // addr2 bounds check\n");

				if (pGv->m_addrW > 0) {
					fprintf(htFile, "#ifndef _HTV\n");
					fprintf(htFile, "\t\tif (!assert_msg_(!m_bAddr, \"Runtime check failed in %s.write_addr() method - write_addr() was already called on this variable\\n\")) assert(0);\n", pGv->m_gblName.c_str());
					fprintf(htFile, "#endif\n");
					fprintf(htFile, "\t\tm_bAddr = true;\n");
				}

				string bitRange = pGv->m_addr0W.AsInt() > 0 ? VA("(%d, 0)", pGv->m_addr1W.AsInt() + pGv->m_addr2W.AsInt() - 1) : "";
				if (pGv->m_addr1W.AsInt() > 0 && !bAddr1EqHtId)
					fprintf(htFile, "\t\tm_addr(%d, %d) = addr1;\n", pGv->m_addr1W.AsInt() + pGv->m_addr2W.AsInt() - 1, pGv->m_addr2W.AsInt());
				if (pGv->m_addr2W.AsInt() > 0 && !bAddr2EqHtId)
					fprintf(htFile, "\t\tm_addr(%d, 0) = addr2;\n", pGv->m_addr2W.AsInt() - 1);

				fprintf(htFile, "\t}\n");
			}
		}

		fprintf(htFile, "\tvoid operator = (%s rhs) {\n", pType->m_typeName.c_str());
		for (CStructElemIter iter(this, pRecord/*->m_structName*/, false); !iter.end(); iter++) {
			fprintf(htFile, "\t\t%s = rhs.%s;\n",
				iter.GetHeirFieldName(false).c_str(), iter.GetHeirFieldName(false).c_str());
		}
		fprintf(htFile, "\t}\n");

		fprintf(htFile, "\toperator %s () const\n", pType->m_typeName.c_str());
		fprintf(htFile, "\t{\n");

		fprintf(htFile, "\t\t%s _data_;\n", pType->m_typeName.c_str());

		for (CStructElemIter iter(this, pRecord, false); !iter.end(); iter++) {
			fprintf(htFile, "\t\t_data_.%s = %s.GetData();\n",
				iter.GetHeirFieldName(false).c_str(), iter.GetHeirFieldName(false).c_str());
		}
		fprintf(htFile, "\t\treturn _data_;\n");
		fprintf(htFile, "\t}\n");

		fprintf(htFile, "\t%s GetData() const\n", pType->m_typeName.c_str());
		fprintf(htFile, "\t{\n");

		fprintf(htFile, "\t\t%s _data_;\n", pType->m_typeName.c_str());

		if (pRecord->m_bUnion) {
			fprintf(htFile, "\t\t_data_ = 0;\n");
			for (CStructElemIter iter(this, pRecord, true); !iter.end(); iter++) {
				fprintf(htFile, "\t\tif (%s.GetWrEn())\n",
					iter.GetHeirFieldName(false).c_str());
				fprintf(htFile, "\t\t\t_data_.%s = %s.GetData();\n",
					iter.GetHeirFieldName(false).c_str(), iter.GetHeirFieldName(false).c_str());
			}
		} else {
			for (CStructElemIter iter(this, pRecord, false); !iter.end(); iter++) {
				fprintf(htFile, "\t\t_data_.%s = %s.GetData();\n",
					iter.GetHeirFieldName(false).c_str(), iter.GetHeirFieldName(false).c_str());
			}
		}
		fprintf(htFile, "\t\treturn _data_;\n");
		fprintf(htFile, "\t}\n");

		if (pGv && pGv->m_addrW > 0) {
			fprintf(htFile, "\tbool IsAddrSet() const { return m_bAddr; }\n");
			fprintf(htFile, "\tht_uint%d GetAddr() const { return m_addr; }\n", pGv->m_addrW);
		}

		fprintf(htFile, "public:\n");

		for (CStructElemIter iter(this, pRecord, false, false); !iter.end(); iter++) {
			CField & field = iter();

			string fieldTypeName = "CGW_";
			if (field.m_fieldWidth.size() > 0) {
				int width;
				bool bSigned;
				bool bFound = FindCIntType(field.m_pType->m_typeName, width, bSigned);
				HtlAssert(bFound);

				if (bSigned)
					fieldTypeName += VA("ht_int%d", field.m_fieldWidth.AsInt());
				else
					fieldTypeName += VA("ht_uint%d", field.m_fieldWidth.AsInt());
			} else {
				fieldTypeName += field.m_pType->m_typeName;
			}

			if (!field.m_pType->IsRecord()) {
				switch (field.m_atomicMask) {
				case ATOMIC_INC: fieldTypeName += "_I"; break;
				case ATOMIC_SET: fieldTypeName += "_S"; break;
				case ATOMIC_ADD: fieldTypeName += "_A"; break;
				default: break;
				}
			}

			fprintf(htFile, "\t%s %s%s;\n", fieldTypeName.c_str(), field.m_name.c_str(), field.m_dimenDecl.c_str());
		}

		fprintf(htFile, "private:\n");

		if (pGv && pGv->m_addrW > 0) {
			fprintf(htFile, "\tbool m_bAddr;\n");
			fprintf(htFile, "\tht_uint%d m_addr;\n", pGv->m_addrW);
		}
	} else {
		if (pGv) {
			int htIdW = 0;
			if (pGv->m_addr0W.AsInt() > 0)
				htIdW = pGv->m_addr0W.AsInt();
			else if (pGv->m_addr1Name == "htId")
				htIdW = pGv->m_addr1W.AsInt();
			else if (pGv->m_addr2Name == "htId")
				htIdW = pGv->m_addr2W.AsInt();

			if (htIdW > 0) {
				fprintf(htFile, "\tvoid InitZero(ht_uint%d htId=0)\n", htIdW);
				fprintf(htFile, "\t{\n");
				if (pGv && pGv->m_addrW > 0) {
					fprintf(htFile, "\t\tm_bAddr = %s;\n", bInitBAddrFalse ? "false" : "true");
					fprintf(htFile, "\t\tm_addr = 0;\n");
				}
				if (pGv->m_addr0W.AsInt() > 0)
					fprintf(htFile, "\t\tm_addr(%d, %d) = htId;\n", pGv->m_addrW - 1, pGv->m_addrW - pGv->m_addr0W.AsInt());
				if (pGv->m_addr1Name == "htId")
					fprintf(htFile, "\t\tm_addr(%d, %d) = htId;\n", pGv->m_addr1W.AsInt() + pGv->m_addr2W.AsInt() - 1, pGv->m_addr2W.AsInt());
				if (pGv->m_addr2Name == "htId")
					fprintf(htFile, "\t\tm_addr(%d, 0) = htId;\n", pGv->m_addr2W.AsInt() - 1);
			} else {
				fprintf(htFile, "\tvoid InitZero()\n");
				fprintf(htFile, "\t{\n");
				if (pGv && pGv->m_addrW > 0) {
					fprintf(htFile, "\t\tm_bAddr = %s;\n", bInitBAddrFalse ? "false" : "true");
					fprintf(htFile, "\t\tm_addr = 0;\n");
				}
			}
		} else {
			fprintf(htFile, "\tvoid InitZero()\n");
			fprintf(htFile, "\t{\n");
		}

		fprintf(htFile, "\t\tm_bWrite = false;\n");
		switch (atomicMask) {
		case ATOMIC_INC: fprintf(htFile, "\t\tm_bInc = false;\n"); break;
		case ATOMIC_SET: fprintf(htFile, "\t\tm_bSet = false;\n"); break;
		case ATOMIC_ADD: fprintf(htFile, "\t\tm_bAdd = false;\n"); break;
		default: break;
		}
		fprintf(htFile, "\t\tm_data = 0;\n");
		fprintf(htFile, "\t}\n");

		if (pGv) {
			int htIdW = 0;
			if (pGv->m_addr0W.AsInt() > 0)
				htIdW = pGv->m_addr0W.AsInt();
			else if (pGv->m_addr1Name == "htId")
				htIdW = pGv->m_addr1W.AsInt();
			else if (pGv->m_addr2Name == "htId")
				htIdW = pGv->m_addr2W.AsInt();

			if (htIdW > 0) {
				fprintf(htFile, "\tvoid InitData(ht_uint%d htId, %s _data_)\n", htIdW, pType->m_typeName.c_str());
				fprintf(htFile, "\t{\n");
				if (pGv->m_addrW > 0) {
					fprintf(htFile, "\t\tm_bAddr = %s;\n", bInitBAddrFalse ? "false" : "true");
					fprintf(htFile, "\t\tm_addr = 0;\n");
				}
				if (pGv->m_addr0W.AsInt() > 0)
					fprintf(htFile, "\t\tm_addr(%d, %d) = htId;\n", pGv->m_addrW - 1, pGv->m_addrW - pGv->m_addr0W.AsInt());
				if (pGv->m_addr1Name == "htId")
					fprintf(htFile, "\t\tm_addr(%d, %d) = htId;\n", pGv->m_addr1W.AsInt() + pGv->m_addr2W.AsInt() - 1, pGv->m_addr2W.AsInt());
				if (pGv->m_addr2Name == "htId")
					fprintf(htFile, "\t\tm_addr(%d, 0) = htId;\n", pGv->m_addr2W.AsInt() - 1);
			} else {
				fprintf(htFile, "\tvoid InitData(%s _data_)\n", pType->m_typeName.c_str());
				fprintf(htFile, "\t{\n");

				if (pGv->m_addrW > 0) {
					fprintf(htFile, "\t\tm_bAddr = %s;\n", bInitBAddrFalse ? "false" : "true");
					fprintf(htFile, "\t\tm_addr = 0;\n");
				}
			}
			fprintf(htFile, "\t\tm_bWrite = false;\n");
			switch (atomicMask) {
			case ATOMIC_INC: fprintf(htFile, "\t\tm_bInc = false;\n"); break;
			case ATOMIC_SET: fprintf(htFile, "\t\tm_bSet = false;\n"); break;
			case ATOMIC_ADD: fprintf(htFile, "\t\tm_bAdd = false;\n"); break;
			default: break;
			}
			fprintf(htFile, "\t\tm_data = _data_;\n");
			fprintf(htFile, "\t}\n");
		} else {
			fprintf(htFile, "\tvoid InitData(%s _data_)\n", pType->m_typeName.c_str());
			fprintf(htFile, "\t{\n");
			fprintf(htFile, "\t\tm_bWrite = false;\n");
			switch (atomicMask) {
			case ATOMIC_INC: fprintf(htFile, "\t\tm_bInc = false;\n"); break;
			case ATOMIC_SET: fprintf(htFile, "\t\tm_bSet = false;\n"); break;
			case ATOMIC_ADD: fprintf(htFile, "\t\tm_bAdd = false;\n"); break;
			default: break;
			}
			fprintf(htFile, "\t\tm_data = _data_;\n");
			fprintf(htFile, "\t}\n");
		}

		if (pGv && pGv->m_addr1W.AsInt() > 0) {
			fprintf(htFile, "\tvoid write_addr(ht_uint%d addr1%s) {\n", pGv->m_addr1W.AsInt(),
				VA(pGv->m_addr2W.AsInt() == 0 ? "" : ", ht_uint%d addr2", pGv->m_addr2W.AsInt()).c_str());

			fprintf(htFile, "#ifndef _HTV\n");
			fprintf(htFile, "\t\tif (!assert_msg_(!m_bAddr, \"Runtime check failed in %s.write_addr() method - write_addr() was already called on this variable\\n\")) assert(0);\n", pGv->m_gblName.c_str());
			fprintf(htFile, "#endif\n");
			fprintf(htFile, "\t\tm_bAddr = true;\n");

			string bitRange = pGv->m_addr0W.AsInt() > 0 ? VA("(%d, 0)", pGv->m_addr1W.AsInt() + pGv->m_addr2W.AsInt() - 1) : "";
			if (pGv->m_addr2W.AsInt() == 0)
				fprintf(htFile, "\t\tm_addr%s = addr1;\n", bitRange.c_str());
			else
				fprintf(htFile, "\t\tm_addr%s = (addr1, addr2);\n", bitRange.c_str());

			fprintf(htFile, "\t}\n");
		}

		char * intOpList[] = { "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", 0 };
		char * booleanOpList[] = { "=", "&=", "|=", "^=", 0 };

		char ** opList = pType == &g_bool ? booleanOpList : intOpList;

		for (int i = 0; opList[i]; i += 1) {
			fprintf(htFile, "\tvoid operator %s (%s rhs)\n", opList[i], pType->m_typeName.c_str());
			fprintf(htFile, "\t{\n");
			fprintf(htFile, "\t\tm_bWrite = true;\n");
			fprintf(htFile, "\t\tm_data %s rhs;\n", opList[i]);
			fprintf(htFile, "\t}\n");
		}

		if (pType->IsInt() && pType->AsInt()->m_clangMinAlign == 1) {
			fprintf(htFile, "\tbool operator == (%s const &rhs) const { return m_data == rhs; }\n",
				pType->m_typeName.c_str());
			fprintf(htFile, "\tbool operator != (%s const &rhs) const { return m_data != rhs; }\n",
				pType->m_typeName.c_str());
		}

		switch (atomicMask) {
		case ATOMIC_INC:
			fprintf(htFile, "\tvoid AtomicInc() { m_bInc = true; }\n");
			fprintf(htFile, "\tbool GetIncEn() const { return m_bInc; }\n");
			break;
		case ATOMIC_SET:
			fprintf(htFile, "\tvoid AtomicSet() { m_bSet = true; }\n");
			fprintf(htFile, "\tbool GetSetEn() const { return m_bSet; }\n");
			break;
		case ATOMIC_ADD:
			fprintf(htFile, "\tvoid AtomicAdd(%s rhs) {\n", pType->m_typeName.c_str());
			fprintf(htFile, "\t\tm_bAdd = true;\n");
			fprintf(htFile, "\t\tm_data = rhs;\n");
			fprintf(htFile, "\t}\n");
			fprintf(htFile, "\tbool GetAddEn() const { return m_bAdd; }\n");
			break;
		default: break;
		}

		fprintf(htFile, "\tbool GetWrEn() const { return m_bWrite; }\n");
		if (pGv && pGv->m_addrW > 0) {
			fprintf(htFile, "\tbool IsAddrSet() const { return m_bAddr; }\n");
			fprintf(htFile, "\tht_uint%d GetAddr() const { return m_addr; }\n", pGv->m_addrW);
		}
		fprintf(htFile, "\t%s GetData() const { return m_data; }\n", pType->m_typeName.c_str());
		//fprintf(htFile, "\toperator %s () const { return m_data; }\n", pType->m_typeName.c_str());

		if (pType->IsInt()) {
			string typeName;
			if (pType->AsInt()->m_clangMinAlign == 1) {
				string typePrefix = pType->m_typeName.substr(0, 4);
				if (typePrefix == "ht_u")
					typeName = "uint64_t";
				else if (typePrefix == "ht_i")
					typeName = "int64_t";
				else
					HtlAssert(0);

				fprintf(htFile, "\toperator %s () const { return m_data; }\n", typeName.c_str());
			} else
				typeName = pType->m_typeName;

			fprintf(htFile, "\toperator %s () const { return m_data; }\n", pType->m_typeName.c_str());
		}

		fprintf(htFile, "private:\n");

		if (pGv && pGv->m_addrW > 0) {
			fprintf(htFile, "\tbool m_bAddr;\n");
			fprintf(htFile, "\tht_uint%d m_addr;\n", pGv->m_addrW);
		}

		fprintf(htFile, "\tbool m_bWrite;\n");
		switch (atomicMask) {
		case ATOMIC_INC: fprintf(htFile, "\tbool m_bInc;\n"); break;
		case ATOMIC_SET: fprintf(htFile, "\tbool m_bSet;\n"); break;
		case ATOMIC_ADD: fprintf(htFile, "\tbool m_bAdd;\n"); break;
		default: break;
		}
		fprintf(htFile, "\t%s m_data;\n", pType->m_typeName.c_str());
	}

	fprintf(htFile, "};\n");
	fprintf(htFile, "\n");

	if (pRecord)
		atomicMask |= pRecord->m_atomicMask;
}
