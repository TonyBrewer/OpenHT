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

	for(size_t i=0; i< m_defineTable.size(); i += 1) {
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
		if(typeDef.m_scope.compare("unit") != 0) {
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
				if (typeDef.m_type.substr(0,3) == "int")
					fprintf(incFile, "typedef %s<%s> %s;\n",
						width > 64 ? "sc_bigint" : "sc_int", typeDef.m_width.c_str(), typeDef.m_name.c_str());
				else if (typeDef.m_type.substr(0,4) == "uint")
					fprintf(incFile, "typedef %s<%s> %s;\n",
						width > 64 ? "sc_biguint" : "sc_uint", typeDef.m_width.c_str(), typeDef.m_name.c_str());
				else
					ParseMsg(Fatal, typeDef.m_lineInfo, "unexpected type '%s'", typeDef.m_type.c_str());
			}
		}
	}
	fprintf(incFile, "\n");

	fprintf(incFile, "//////////////////////////////////\n");
	fprintf(incFile, "// Common structs and unions\n");
	fprintf(incFile, "\n");

	for (size_t structIdx = 0; structIdx < m_structList.size(); structIdx += 1) {

		if (m_structList[structIdx].m_scope != eUnit && !m_structList[structIdx].m_bInclude) continue;
		//if (!m_structList[structIdx].m_bNeedIntf) continue;

		if (m_structList[structIdx].m_bCStyle)
			GenIntfStruct(incFile, m_structList[structIdx].m_structName, m_structList[structIdx].m_fieldList,
				m_structList[structIdx].m_bCStyle, m_structList[structIdx].m_bInclude,
				false, m_structList[structIdx].m_bUnion);
		else
			GenUserStructs(incFile, m_structList[structIdx]);

		CHtCode htFile(incFile);
		GenUserStructBadData(htFile, true, m_structList[structIdx].m_structName,
			m_structList[structIdx].m_fieldList, m_structList[structIdx].m_bCStyle, "");
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
			GenGlobalVarWriteTypes(incFile, pNgv->m_type, pNgvInfo->m_atomicMask, gvTypeList, pNgv);
		}
	}

	fprintf(incFile, "#ifdef _HTV\n");
	fprintf(incFile, "#define INT(a) a\n");
	fprintf(incFile, "#else\n");
	fprintf(incFile, "#define INT(a) int(a)\n");
	fprintf(incFile, "#define HT_CYC_1X ((long long)sc_time_stamp().value()/10000)\n");
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

	CStruct htAssert;
	htAssert.AddStructField("bool", "bAssert");
	htAssert.AddStructField("bool", "bCollision");
	htAssert.AddStructField("ht_uint8", "module");
	htAssert.AddStructField("ht_uint16", "lineNum");
	htAssert.AddStructField("ht_uint32", "info");
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
	fprintf(incFile, "// Host Control interfaces\n");
	fprintf(incFile, "\n");

	CStruct ctrlMsgIntf;
	ctrlMsgIntf.AddStructField("ht_uint1", "bValid");
	ctrlMsgIntf.AddStructField("ht_uint7", "msgType");
	ctrlMsgIntf.AddStructField("ht_uint56", "msgData");
	GenIntfStruct(incFile, "CHostCtrlMsg", ctrlMsgIntf.m_fieldList, false, false, true, false);

	fprintf(incFile, "//////////////////////////////////\n");
	fprintf(incFile, "// Host Queue interfaces\n");
	fprintf(incFile, "\n");

	fprintf(incFile, "#define HT_DQ_NULL\t1\n");
	fprintf(incFile, "#define HT_DQ_DATA\t2\n");
	fprintf(incFile, "#define HT_DQ_FLSH\t3\n");
	fprintf(incFile, "#define HT_DQ_CALL\t6\n");
	fprintf(incFile, "#define HT_DQ_HALT\t7\n");


	CStruct hostDataQue;
	hostDataQue.AddStructField("ht_uint64", "data");
	hostDataQue.AddStructField("ht_uint3", "ctl");
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

				queIntf.m_structName = queIntfName;
			} else
				queIntf.m_structName = "CHostDataQueIntf";

			if (queIntf.m_queType == Push || bHifQue) continue;

			GenUserStructs(incFile, queIntf);

			CHtCode htFile(incFile);
			GenUserStructBadData(htFile, true, queIntf.m_structName, queIntf.m_fieldList, queIntf.m_bCStyle, "");
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

		int instId = -1;
		for (size_t modInstIdx = 0; modInstIdx < mod.m_modInstList.size(); modInstIdx += 1) {
			CModInst &modInst = mod.m_modInstList[modInstIdx];

			if (modInst.m_instParams.m_instId == instId)
				continue;

			instId = modInst.m_instParams.m_instId;

			for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

				if (cxrIntf.m_cxrDir == CxrOut) continue;

				char intfName[256];
				sprintf(intfName, "C%s_%s", cxrIntf.GetSrcToDstUc(), cxrIntf.GetIntfName());

				size_t i;
				for (i = 0; i < intfNameList.size(); i += 1) {
					if (intfNameList[i] == intfName)
						break;
				}

				if (i < intfNameList.size())
					continue;

				intfNameList.push_back(intfName);

				cxrIntf.m_fullFieldList = *cxrIntf.m_pFieldList;
				CLineInfo lineInfo;

				if (cxrIntf.m_cxrType == CxrCall) {

					if (cxrIntf.m_pSrcGroup->m_htIdW.AsInt() > 0) {
						cxrIntf.m_fullFieldList.push_back(CField(cxrIntf.m_pSrcGroup->m_htIdType, "rtnHtId"));
						cxrIntf.m_fullFieldList.back().DimenListInit(lineInfo);
					}
					if (cxrIntf.m_pSrcMod->m_instrType.size() > 0) {
						cxrIntf.m_fullFieldList.push_back(CField(cxrIntf.m_pSrcMod->m_instrType, "rtnInstr"));
						cxrIntf.m_fullFieldList.back().DimenListInit(lineInfo);
					}

				} else if (cxrIntf.m_cxrType == CxrTransfer) { // else Transfer

					if (cxrIntf.m_pSrcGroup->m_rtnSelW > 0) {
						cxrIntf.m_fullFieldList.push_back(CField(cxrIntf.m_pSrcGroup->m_rtnSelType, "rtnSel"));
						cxrIntf.m_fullFieldList.back().DimenListInit(lineInfo);
					}
					if (cxrIntf.m_pSrcGroup->m_rtnHtIdW > 0) {
						cxrIntf.m_fullFieldList.push_back(CField(cxrIntf.m_pSrcGroup->m_rtnHtIdType, "rtnHtId"));
						cxrIntf.m_fullFieldList.back().DimenListInit(lineInfo);
					}
					if (cxrIntf.m_pSrcGroup->m_rtnInstrW > 0) {
						cxrIntf.m_fullFieldList.push_back(CField(cxrIntf.m_pSrcGroup->m_rtnInstrType, "rtnInstr"));
						cxrIntf.m_fullFieldList.back().DimenListInit(lineInfo);
					}

				} else { // CxrReturn

					if (cxrIntf.m_pDstGroup->m_htIdW.AsInt() > 0) {
						cxrIntf.m_fullFieldList.push_back(CField(cxrIntf.m_pDstGroup->m_htIdType, "rtnHtId"));
						cxrIntf.m_fullFieldList.back().DimenListInit(lineInfo);
					}
					if (cxrIntf.m_pDstMod->m_instrType.size() > 0) {
						cxrIntf.m_fullFieldList.push_back(CField(cxrIntf.m_pDstMod->m_instrType, "rtnInstr"));
						cxrIntf.m_fullFieldList.back().DimenListInit(lineInfo);
					}

				}

				if (cxrIntf.m_bRtnJoin) {
					cxrIntf.m_fullFieldList.push_back(CField("bool", "rtnJoin"));
					cxrIntf.m_fullFieldList.back().DimenListInit(lineInfo);
				}

				if (cxrIntf.m_fullFieldList.size() == 0)
					continue;

				GenIntfStruct(incFile, intfName, cxrIntf.m_fullFieldList, false, false, false, false);
			}
		}
	}

	fprintf(incFile, "//////////////////////////////////\n");
	fprintf(incFile, "// Ram interfaces\n");
	fprintf(incFile, "\n");

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		// Generate ram interface structs
		for (size_t ramIdx = 0; ramIdx < mod.m_extRamList.size(); ramIdx += 1) {
			CRam & ram = mod.m_extRamList[ramIdx];

			bool bRead = false;
			bool bWrite = false;
			for (size_t fieldIdx = 0; fieldIdx < ram.m_fieldList.size(); fieldIdx += 1) {
				bRead = bRead | ram.m_fieldList[fieldIdx].m_bSrcRead | ram.m_fieldList[fieldIdx].m_bMifRead;
				bWrite = bWrite | ram.m_fieldList[fieldIdx].m_bSrcWrite | ram.m_fieldList[fieldIdx].m_bMifWrite;
			}

			if (bRead) {
				string intfName = "C" + ram.m_modName.Uc() + "To" + mod.m_modName.Uc() + "_" + ram.m_gblName.AsStr() + "RdData";
				GenRamIntfStruct(incFile, intfName, ram, eStructRamRdData);
			}

			if (bWrite) {
				string intfName = "C" + mod.m_modName.Uc() + "To" + ram.m_modName.Uc() + "_" + ram.m_gblName.AsStr() + "WrData";
				GenRamIntfStruct(incFile, intfName, ram, eGenRamWrData);
			}

			if (bWrite) {
				string intfName = "C" + mod.m_modName.Uc() + "To" + ram.m_modName.Uc() + "_" + ram.m_gblName.AsStr() + "WrEn";
				GenRamIntfStruct(incFile, intfName, ram, eGenRamWrEn);
			}
		}
	}

	incFile.FileClose();
}

void CDsnInfo::MarkNeededIncludeStructs()
{
	for (size_t structIdx = 0; structIdx < m_structList.size(); structIdx += 1) {
		CStruct & struct_ = m_structList[structIdx];

		if (struct_.m_scope != eUnit && !struct_.m_bInclude) continue;
		if (!struct_.m_bNeedIntf) continue;

		// walk struct and see it if has embedded structures
		MarkNeededIncludeStructs(struct_);
	}
}

void CDsnInfo::MarkNeededIncludeStructs(CStruct & struct_)
{
	for (size_t fieldIdx = 0; fieldIdx < struct_.m_fieldList.size(); fieldIdx += 1) {
		CField &field = struct_.m_fieldList[fieldIdx];

		for (size_t struct2Idx = 0; struct2Idx < m_structList.size(); struct2Idx += 1) {
			CStruct & struct2 = m_structList[struct2Idx];

			if (struct2.m_structName == field.m_type && !struct2.m_bNeedIntf) {
				struct2.m_bNeedIntf = true;
				MarkNeededIncludeStructs(struct2);
			}
		}
	}
}

void CDsnInfo::GenGlobalVarWriteTypes(CHtFile & htFile, string &typeName, int &atomicMask, vector<string> &gvTypeList, CRam * pGv)
{
	string ramTypeName = typeName;
	if (pGv) {
		ramTypeName = VA("%s_A%d_%d", typeName.c_str(), pGv->m_addr1W.AsInt(), pGv->m_addr2W.AsInt());
		pGv->m_pNgvInfo->m_ngvWrType = ramTypeName;
	}

	string wrTypeName = "CGW_" + ramTypeName;
	switch (atomicMask) {
	case ATOMIC_INC: wrTypeName += "_I"; break;
	case ATOMIC_SET: wrTypeName += "_S"; break;
	case ATOMIC_ADD: wrTypeName += "_A"; break;
	default: break;
	}

	CStruct * pStruct = FindStruct(typeName);

	// check if type already generated
	for (size_t i = 0; i < gvTypeList.size(); i += 1) {
		if (gvTypeList[i] == wrTypeName) {
			if (pStruct)
				atomicMask |= pStruct->m_atomicMask;
			return;
		}
	}

	gvTypeList.push_back(wrTypeName);

	if (pStruct) {
		for (size_t fieldIdx = 0; fieldIdx < pStruct->m_fieldList.size(); fieldIdx += 1) {
			CField &field = pStruct->m_fieldList[fieldIdx];

			GenGlobalVarWriteTypes(htFile, field.m_type, field.m_atomicMask, gvTypeList);

			pStruct->m_atomicMask |= field.m_atomicMask;
		}
	}

	fprintf(htFile, "struct %s {\n", wrTypeName.c_str());
	fprintf(htFile, "#\tifndef _HTV\n");
	fprintf(htFile, "\tbool operator == (const %s & rhs) const {\n", wrTypeName.c_str());

	if (pStruct) {
		if (pGv && pGv->m_addr1W.AsInt() > 0) {
			fprintf(htFile, "\t\tif (!(m_bAddr == rhs.m_bAddr)) return false;\n");
			if (pGv->m_addr1W.AsInt() > 0)
				fprintf(htFile, "\t\tif (!(m_addr1 == rhs.m_addr1)) return false;\n");
			if (pGv->m_addr2W.AsInt() > 0)
				fprintf(htFile, "\t\tif (!(m_addr2 == rhs.m_addr2)) return false;\n");
		}

		for (size_t fieldIdx = 0; fieldIdx < pStruct->m_fieldList.size(); fieldIdx += 1) {
			CField &field = pStruct->m_fieldList[fieldIdx];

			vector<int> refList(field.m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);

				fprintf(htFile, "\t\tif (!(%s%s == rhs.%s%s)) return false;\n",
					field.m_name.c_str(), dimIdx.c_str(), field.m_name.c_str(), dimIdx.c_str());

			} while (DimenIter(field.m_dimenList, refList));
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

	if (pStruct) {
		fprintf(htFile, "\tvoid operator = (int zero) {\n");
		fprintf(htFile, "\t\tassert(zero == 0);\n");

		if (pStruct) {
			if (pGv && pGv->m_addr1W.AsInt() > 0) {
				fprintf(htFile, "\t\tm_bAddr = false;\n");
				if (pGv->m_addr1W.AsInt() > 0)
					fprintf(htFile, "\t\tm_addr1 = 0;\n");
				if (pGv->m_addr2W.AsInt() > 0)
					fprintf(htFile, "\t\tm_addr2 = 0;\n");
			}

			for (size_t fieldIdx = 0; fieldIdx < pStruct->m_fieldList.size(); fieldIdx += 1) {
				CField &field = pStruct->m_fieldList[fieldIdx];

				vector<int> refList(field.m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					fprintf(htFile, "\t\t%s%s = 0;\n", field.m_name.c_str(), dimIdx.c_str());

				} while (DimenIter(field.m_dimenList, refList));
			}
		} else {
			fprintf(htFile, "\t\tm_bWrite = false;\n");
			switch (atomicMask) {
			case ATOMIC_INC: fprintf(htFile, "\t\tm_bInc = false;\n"); break;
			case ATOMIC_SET: fprintf(htFile, "\t\tm_bSet = false;\n"); break;
			case ATOMIC_ADD: fprintf(htFile, "\t\tm_bAdd = false;\n"); break;
			default: break;
			}
			fprintf(htFile, "\t\tm_data = 0;\n");
		}

		fprintf(htFile, "\t}\n");
	}
	fprintf(htFile, "#\tifdef HT_SYSC\n");
	fprintf(htFile, "\tfriend void sc_trace(sc_trace_file *tf, const %s & v, const std::string & NAME) {\n", wrTypeName.c_str());

	if (pStruct) {
		if (pGv && pGv->m_addr1W.AsInt() > 0) {
			fprintf(htFile, "\t\tsc_trace(tf, v.m_bAddr, NAME + \".m_bAddr\");\n");
			if (pGv->m_addr1W.AsInt() > 0)
				fprintf(htFile, "\t\tsc_trace(tf, v.m_addr1, NAME + \".m_addr1\");\n");
			if (pGv->m_addr2W.AsInt() > 0)
				fprintf(htFile, "\t\tsc_trace(tf, v.m_addr2, NAME + \".m_addr2\");\n");
		}

		for (size_t fieldIdx = 0; fieldIdx < pStruct->m_fieldList.size(); fieldIdx += 1) {
			CField &field = pStruct->m_fieldList[fieldIdx];

			vector<int> refList(field.m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);

				fprintf(htFile, "\t\tsc_trace(tf, v.%s%s, NAME + \".%s%s\");\n", 
					field.m_name.c_str(), dimIdx.c_str(), field.m_name.c_str(), dimIdx.c_str());

			} while (DimenIter(field.m_dimenList, refList));
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

	if (pStruct) {
		fprintf(htFile, "\tvoid init() {\n");
		if (pGv && pGv->m_addr1W.AsInt() > 0) {
			fprintf(htFile, "\t\tm_bAddr = false;\n");
			fprintf(htFile, "\t\tm_addr1 = 0;\n");
			if (pGv->m_addr2W.AsInt() > 0)
				fprintf(htFile, "\t\tm_addr2 = 0;\n");
		}
		CHtCode htCode(htFile);
		for (size_t fieldIdx = 0; fieldIdx < pStruct->m_fieldList.size(); fieldIdx += 1) {
			CField &field = pStruct->m_fieldList[fieldIdx];

			string tabs = "\t\t";
			GenIndexLoops(htCode, tabs, field.m_dimenList);
			fprintf(htFile, "%s%s%s.init();\n", tabs.c_str(), field.m_name.c_str(), field.m_dimenIndex.c_str());
		}
		fprintf(htFile, "\t}\n");

		if (pGv && pGv->m_addr1W.AsInt() > 0) {
			fprintf(htFile, "\tvoid write_addr(ht_uint%d addr1%s) {\n", pGv->m_addr1W.AsInt(),
				VA(pGv->m_addr2W.AsInt() == 0 ? "" : ", ht_uint%d addr2", pGv->m_addr2W.AsInt()).c_str());
			if (pGv && pGv->m_addr1W.AsInt() > 0) {
				fprintf(htFile, "\t\tm_bAddr = true;\n");
				fprintf(htFile, "\t\tm_addr1 = addr1;\n");
				if (pGv->m_addr2W.AsInt() > 0)
					fprintf(htFile, "\t\tm_addr2 = addr2;\n");
			}
			fprintf(htFile, "\t}\n");
		}

		fprintf(htFile, "\tvoid operator = (%s rhs) {\n", typeName.c_str());
		for (size_t fieldIdx = 0; fieldIdx < pStruct->m_fieldList.size(); fieldIdx += 1) {
			CField &field = pStruct->m_fieldList[fieldIdx];

			string tabs = "\t\t";
			GenIndexLoops(htCode, tabs, field.m_dimenList);
			fprintf(htFile, "%s%s%s = rhs.%s%s;\n", tabs.c_str(), 
				field.m_name.c_str(), field.m_dimenIndex.c_str(),
				field.m_name.c_str(), field.m_dimenIndex.c_str());
		}
		fprintf(htFile, "\t}\n");

		if (pGv && pGv->m_addr1W.AsInt() > 0) {
			fprintf(htFile, "\tbool m_bAddr;\n");
			fprintf(htFile, "\tht_uint%d m_addr1;\n", pGv->m_addr1W.AsInt());
			if (pGv->m_addr2W.AsInt() > 0)
				fprintf(htFile, "\tht_uint%d m_addr2;\n", pGv->m_addr2W.AsInt());
		}

		for (size_t fieldIdx = 0; fieldIdx < pStruct->m_fieldList.size(); fieldIdx += 1) {
			CField &field = pStruct->m_fieldList[fieldIdx];

			CStruct * pFieldStruct = FindStruct(field.m_type);

			string fieldTypeName = "CGW_" + field.m_type;

			if (!pFieldStruct) {
				switch (field.m_atomicMask) {
				case ATOMIC_INC: fieldTypeName += "_I"; break;
				case ATOMIC_SET: fieldTypeName += "_S"; break;
				case ATOMIC_ADD: fieldTypeName += "_A"; break;
				default: break;
				}
			}

			fprintf(htFile, "\t%s %s%s;\n", fieldTypeName.c_str(), field.m_name.c_str(), field.m_dimenDecl.c_str());
		}
	} else {
		fprintf(htFile, "\tvoid init() {\n");
		fprintf(htFile, "\t\tm_bWrite = false;\n");
		switch (atomicMask) {
		case ATOMIC_INC: fprintf(htFile, "\t\tm_bInc = false;\n"); break;
		case ATOMIC_SET: fprintf(htFile, "\t\tm_bSet = false;\n"); break;
		case ATOMIC_ADD: fprintf(htFile, "\t\tm_bAdd = false;\n"); break;
		default: break;
		}
		fprintf(htFile, "\t\tm_data = 0;\n");
		fprintf(htFile, "\t}\n");

		fprintf(htFile, "\tvoid operator = (%s rhs) {\n", typeName.c_str());
		fprintf(htFile, "\t\tm_bWrite = true;\n");
		fprintf(htFile, "\t\tm_data = rhs;\n");
		fprintf(htFile, "\t}\n");

		switch (atomicMask) {
		case ATOMIC_INC:
			fprintf(htFile, "\tvoid AtomicInc() {\n");
			fprintf(htFile, "\t\tm_bInc = true;\n");
			fprintf(htFile, "\t}\n");
			break;
		case ATOMIC_SET:
			fprintf(htFile, "\tvoid AtomicSet() {\n");
			fprintf(htFile, "\t\tm_bSet = true;\n");
			fprintf(htFile, "\t}\n");
			break;
		case ATOMIC_ADD:
			fprintf(htFile, "\tvoid AtomicAdd(%s rhs) {\n", typeName.c_str());
			fprintf(htFile, "\t\tm_bAdd = true;\n");
			fprintf(htFile, "\t\tm_data = rhs;\n");
			fprintf(htFile, "\t}\n");
			break;
		default: break;
		}

		fprintf(htFile, "\tbool m_bWrite;\n");
		switch (atomicMask) {
		case ATOMIC_INC: fprintf(htFile, "\tbool m_bInc;\n"); break;
		case ATOMIC_SET: fprintf(htFile, "\tbool m_bSet;\n"); break;
		case ATOMIC_ADD: fprintf(htFile, "\tbool m_bAdd;\n"); break;
		default: break;
		}
		fprintf(htFile, "\t%s m_data;\n", typeName.c_str());
	}

	fprintf(htFile, "};\n");
	fprintf(htFile, "\n");

	if (pStruct)
		atomicMask |= pStruct->m_atomicMask;
}
