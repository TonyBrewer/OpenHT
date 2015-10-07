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


void CDsnInfo::InitAndValidateHif()
{
	CModule & hifMod = *m_modList[0];

	// Verify that call and return types are known to host
	CCxrCall & cxrCall = *hifMod.m_cxrCallList[0];
	CCxrEntry * pCxrEntry = cxrCall.m_pairedFunc.GetCxrEntry();// m_pModInst->m_pMod->m_cxrEntryList[cxrCall.m_pairedFunc.m_idx];
	CCxrReturn & cxrReturn = *cxrCall.m_pairedReturnList[0].GetCxrReturn();// m_pModInst->m_pMod->m_cxrReturnList[cxrCall.m_pairedReturnList[0].m_idx];

	for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
		CField * pParam = pCxrEntry->m_paramList[fldIdx];

		// error messages are generated in routine
		FindHostTypeWidth(pParam->m_name, pParam->m_hostType, pParam->m_fieldWidth, pCxrEntry->m_lineInfo);

		if (pParam->m_hostType != pParam->m_pType->m_typeName) {
			string hostStructName, unitStructName;
			bool bHostStruct = FindStructName(pParam->m_hostType, hostStructName);
			bool bUnitStruct = FindStructName(pParam->m_pType->m_typeName, unitStructName);

			if (bHostStruct != bUnitStruct || bHostStruct && hostStructName != unitStructName)
				ParseMsg(Error, pParam->m_lineInfo, "incompatible types specified for parameters type and hostType");
		}
	}

	for (size_t fldIdx = 0; fldIdx < cxrReturn.m_paramList.size(); fldIdx += 1) {
		CField * pParam = cxrReturn.m_paramList[fldIdx];

		FindHostTypeWidth(pParam->m_name, pParam->m_hostType, pParam->m_fieldWidth, pCxrEntry->m_lineInfo);

		if (pParam->m_hostType != pParam->m_pType->m_typeName) {
			string hostStructName, unitStructName;
			bool bHostStruct = FindStructName(pParam->m_hostType, hostStructName);
			bool bUnitStruct = FindStructName(pParam->m_pType->m_typeName, unitStructName);

			if (bHostStruct != bUnitStruct || bHostStruct && hostStructName != unitStructName)
				ParseMsg(Error, pParam->m_lineInfo, "incompatible types specified for parameters type and hostType");
		}
	}
}

// Check for valid host call/return parameter
// Allow native C/C++ types, pointer to native type
// and pointer to host visible structs.
bool CDsnInfo::IsNativeCType(string &type, bool bAllowHostPtr)
{
	vector<string> typeVec;

	char const * pType = type.c_str();
	for (;;) {
		while (*pType == ' ' || *pType == '\t') pType += 1;
		if (*pType == '\0') break;
		char const * pStart = pType++;
		if (*pStart != '*')
			while (*pType != ' ' && *pType != '\t' && *pType != '\0' && *pType != '*') pType += 1;
		typeVec.push_back(string(pStart, pType - pStart));
	}

	bool bHostVar = false;
	bool bNative = false;
	bool bPtr = false;

	for (size_t i = 0; i < typeVec.size(); i += 1) {
		string &type = typeVec[i];
		if (type == "bool" || type == "char" || type == "short" || type == "int" ||
			type == "uint8_t" || type == "uint16_t" || type == "uint32_t" || type == "uint64_t" ||
			type == "int8_t" || type == "int16_t" || type == "int32_t" || type == "int64_t") {
			if (bNative || bPtr || bHostVar) return false;
			bNative = true;
			continue;
		}
		if (type == "long") {
			if (bNative || bPtr || bHostVar) return false;
			bNative = true;
			if (typeVec.size() > i + 1 && typeVec[i + 1] == "long")
				i += 1;
			continue;
		}
		if (type == "unsigned") {
			if (bPtr || bHostVar) return false;
			continue;
		}

		if (type == "*") {
			if (!bAllowHostPtr) return false;
			bPtr = true;
			continue;
		}

		// pointer to void is allow for host parameters
		if (type == "void") {
			if (bNative || bPtr || bHostVar) return false;
			bHostVar = true;
			continue;
		}

		// non-native type, check if it is a host accessable structure
		size_t recordIdx;
		for (recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {

			if (m_recordList[recordIdx]->m_scope != eHost) continue;

			if (type == m_recordList[recordIdx]->m_typeName) {
				if (bNative || bPtr || bHostVar) return false;
				bHostVar = true;
				break;
			}
		}
		if (recordIdx < m_recordList.size())
			continue;

		return false;
	}

	return bNative || bAllowHostPtr && bHostVar && bPtr;
}

void CDsnInfo::GenerateHifFiles()
{
	vector<CHostMsg> hostMsgList;

	// check if any needed modules have a host message interface
	bool bInHostMsg = false;
	bool bOutHostMsg = false;
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule & mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;

		bInHostMsg |= mod.m_bInHostMsg;
		bOutHostMsg |= mod.m_ohm.m_bOutHostMsg;
	}

	bool b64B = g_appArgs.GetCoprocInfo().GetMaxHostQwReadCnt() > 1;

	CModule & hifMod = *m_modList[0];
	bool bInHostData = false;
	bool bOutHostData = false;
	bool bInHostDataMaxBw = false;

	for (size_t queIdx = 0; queIdx < hifMod.m_queIntfList.size(); queIdx += 1) {
		CQueIntf & queIntf = hifMod.m_queIntfList[queIdx];

		if (queIntf.m_queType == Push) {
			bInHostData = true;
			bInHostDataMaxBw = queIntf.m_bMaxBw;
		} else
			bOutHostData = true;
	}

	if (!g_appArgs.IsModelOnly()) {
		string fileName = g_appArgs.GetOutputFolder() + "/PersHif.h";

		CHtFile incFile(fileName, "w");

		CModule & modInfo = *m_modList[0];
		if (!modInfo.m_bHostIntf)
			ParseMsg(Fatal, "Expected first module to be host interface\n");

		if (modInfo.m_cxrCallList.size() == 0)
			ParseMsg(Fatal, "A host entry point was not specified (i.e. use AddEntry(host=true) )\n");

		if (modInfo.m_cxrCallList.size() > 1)
			ParseMsg(Fatal, "Multiple host entry points were specified (i.e. multiple AddEntry(host=true) )\n");

		GenPersBanner(incFile, m_unitName.Uc().c_str(), "Hif", true);

		fprintf(incFile, "#include \"HostIntf.h\"\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "// Control States\n");
		fprintf(incFile, "#define ICM_READ\t\t0u\n");
		fprintf(incFile, "#define ICM_RD_WAIT\t\t1u\n");
		fprintf(incFile, "#define ICM_WRITE\t\t2u\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "#define OCM_READ\t\t0u\n");
		fprintf(incFile, "#define OCM_RD_WAIT\t\t1u\n");
		fprintf(incFile, "#define OCM_WRITE\t\t2u\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "// Block States\n");
		fprintf(incFile, "#define IBK_IDLE\t\t0u\n");
		fprintf(incFile, "#define IBK_READ\t\t1u\n");
		fprintf(incFile, "#define IBK_WAIT\t\t2u\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "#define OBK_IDLE\t\t0u\n");
		fprintf(incFile, "#define OBK_POP\t\t1u\n");
		fprintf(incFile, "#define OBK_WR8\t\t2u\n");
		if (b64B)
			fprintf(incFile, "#define OBK_WR64\t\t3u\n");
		fprintf(incFile, "#define OBK_FLSH\t\t4u\n");
		fprintf(incFile, "#define OBK_SEND\t\t5u\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "#define HIF_TID_TYPE_W\t\t2\n");
		fprintf(incFile, "#define HIF_TID_TYPE_MSK\t((1u << HIF_TID_TYPE_W)-1)\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "#define IBK_FETCH_CNT_W\t\t%d\n", bInHostDataMaxBw ? 5 : 2);
		fprintf(incFile, "#define IBK_FETCH_CNT\t\t(1u << IBK_FETCH_CNT_W)\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "SC_MODULE(CPersHif) {\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tht_attrib(keep_hierarchy, CPersHif, \"true\");\n");
		fprintf(incFile, "\t//\n");
		fprintf(incFile, "\t// Ports\n");
		fprintf(incFile, "\t//\n");
		fprintf(incFile, "\tsc_in<bool>\t\t\t\ti_clock1x;\n");
		fprintf(incFile, "\tsc_in<bool>\t\t\t\ti_reset;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tsc_in<uint8_t>\t\t\ti_aeUnitId;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t// control queue base address and dispatch status\n");
		fprintf(incFile, "\tsc_in<sc_uint<MEM_ADDR_W> >\t\ti_dispToHif_ctlQueBase;\n");
		fprintf(incFile, "\tsc_in<bool>\t\t\t\ti_dispToHif_dispStart;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tsc_out<ht_uint1>\t\t\to_hifToDisp_busy;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t// inbound control queue interface\n");
		fprintf(incFile, "\tsc_out<bool>\t\t\t\to_hifToHti_ctlRdy;\n");
		fprintf(incFile, "\tsc_out<CHostCtrlMsgIntf>\t\to_hifToHti_ctl;\n");
		fprintf(incFile, "\tsc_in<bool>\t\t\t\ti_htiToHif_ctlAvl;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t// inbound block queue interface\n");
		fprintf(incFile, "\tsc_out<bool>\t\t\t\to_hifToHti_datRdy;\n");
		fprintf(incFile, "\tsc_out<CHostDataQueIntf>\t\to_hifToHti_dat;\n");
		fprintf(incFile, "\tsc_in<bool>\t\t\t\ti_htiToHif_datAvl;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t// outbound control queue interface\n");
		fprintf(incFile, "\tsc_in<bool>\t\t\t\ti_htiToHif_ctlRdy;\n");
		fprintf(incFile, "\tsc_in<CHostCtrlMsgIntf>\t\t\ti_htiToHif_ctl;\n");
		fprintf(incFile, "\tsc_out<bool>\t\t\t\to_hifToHti_ctlAvl;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t// outbound block queue interface\n");
		fprintf(incFile, "\tsc_in<bool>\t\t\t\ti_htiToHif_datRdy;\n");
		fprintf(incFile, "\tsc_in<CHostDataQueIntf>\t\t\ti_htiToHif_dat;\n");
		fprintf(incFile, "\tsc_out<bool>\t\t\t\to_hifToHti_datAvl;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t// memory interface(s)\n");
		fprintf(incFile, "\tsc_out<bool>\t\t\t\to_hifP0ToMif_reqRdy;\n");
		fprintf(incFile, "\tsc_out<CMemRdWrReqIntf>\t\t\to_hifP0ToMif_req;\n");
		fprintf(incFile, "\tsc_in<bool>\t\t\t\ti_mifToHifP0_reqAvl;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tsc_in<bool>\t\t\t\ti_mifToHifP0_rdRspRdy;\n");
		fprintf(incFile, "\tsc_in<CMemRdRspIntf>\t\t\ti_mifToHifP0_rdRsp;\n");
		fprintf(incFile, "\tsc_out<bool>\t\t\t\to_hifP0ToMif_rdRspFull;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tsc_in<bool>\t\t\t\ti_mifToHifP0_wrRspRdy;\n");
		fprintf(incFile, "\tsc_in<sc_uint<MIF_TID_W> >\t\ti_mifToHifP0_wrRspTid;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tsc_in<CHtAssertIntf>\t\t\ti_htaToHif_assert;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t//\n");
		fprintf(incFile, "\t// Internal State\n");
		fprintf(incFile, "\t//\n");
		fprintf(incFile, "\tht_dist_que<sc_uint<HIF_ARG_IBLK_SMAX_W+1>, HIF_ARG_IBLK_CNT_W> m_iBlkRdyQue;\n");
		fprintf(incFile, "\tht_%s_ram<ht_uint64, IBK_FETCH_CNT_W, 3> m_iBlkRdDat;\n", bInHostDataMaxBw ? "block" : "dist");
		fprintf(incFile, "\tht_dist_que<CHostDataQueIntf, 5> m_oBlkQue;\n");
		fprintf(incFile, "\tht_dist_que<ht_uint64, 3> m_oBlkWrDat;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tbool r_hifToDisp_busy;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tsc_uint<MEM_ADDR_W> r_CtlQueBase;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tsc_uint<HIF_TID_TYPE_W> r_mifReqSelRr;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tbool c_hifToMif_reqRdy;\n");
		fprintf(incFile, "\tbool r_hifToMif_reqRdy;\n");
		fprintf(incFile, "\tCMemRdWrReqIntf r_hifToMif_req;\n");
		fprintf(incFile, "\tht_uint6 r_hifToMif_reqAvlCnt;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tsc_uint<HIF_ARG_CTL_TIME_W> r_ctlTimeArg;\n");
		fprintf(incFile, "\tsc_uint<MEM_ADDR_W> r_iBlkAdrArg;\n");
		fprintf(incFile, "\tsc_uint<MEM_ADDR_W> r_oBlkAdrArg;\n");
		fprintf(incFile, "\tsc_uint<MEM_ADDR_W> r_oBlkFlshAdrArg;\n");
		fprintf(incFile, "\tsc_uint<HIF_ARG_IBLK_SIZE_W> r_iBlkSizeArg;\n");
		fprintf(incFile, "\tsc_uint<HIF_ARG_IBLK_CNT_W> r_iBlkCntArg;\n");
		fprintf(incFile, "\tsc_uint<HIF_ARG_OBLK_SIZE_W> r_oBlkSizeArg;\n");
		fprintf(incFile, "\tsc_uint<HIF_ARG_OBLK_CNT_W> r_oBlkCntArg;\n");
		fprintf(incFile, "\tsc_uint<HIF_ARG_OBLK_TIME_W> r_oBlkTimeArg;\n");
		fprintf(incFile, "\tsc_uint<HIF_ARG_OBLK_CNT_W+1> r_oBlkAvlCnt;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tbool r_oBlkFlshEn;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tbool r_hifToHti_ctlRdy;\n");
		fprintf(incFile, "\tCHostCtrlMsgIntf r_hifToHti_ctl;\n");
		fprintf(incFile, "\tht_uint6 r_hifToHti_ctlAvlCnt;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tsc_uint<HIF_ARG_CTL_TIME_W> r_iCtlTimer;\n");
		fprintf(incFile, "\tbool r_iCtlTimerWait;\n");
		fprintf(incFile, "\tsc_uint<2> r_iCtlState;\n");
		fprintf(incFile, "\tbool r_iCtlRdWait;\n");
		fprintf(incFile, "\tbool r_iCtlWrPend;\n");
		fprintf(incFile, "\tsc_uint<HIF_CTL_QUE_CNT_W> r_iCtlRdIdx;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tsc_uint<HIF_ARG_CTL_TIME_W> r_oCtlTimer;\n");
		fprintf(incFile, "\tbool r_oCtlTimerWait;\n");
		fprintf(incFile, "\tsc_uint<2> r_oCtlState;\n");
		fprintf(incFile, "\tbool r_oCtlRdWait;\n");
		fprintf(incFile, "\tbool r_oCtlWrPend;\n");
		fprintf(incFile, "\tsc_uint<HIF_CTL_QUE_CNT_W> r_oCtlWrIdx;\n");
		fprintf(incFile, "\tCHostCtrlMsgIntf r_htiToHif_ctl;\n");
		fprintf(incFile, "\tbool r_hifToHti_ctlAvl;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tCHostCtrlMsgIntf r_oCtlWrMsg;\n");
		fprintf(incFile, "\tbool r_oBlkSendRdy;\n");
		fprintf(incFile, "\tsc_uint<HIF_ARG_OBLK_SMAX_W+1> r_oBlkSize;\n");
		fprintf(incFile, "\tht_uint1 r_oBlkSendIdx;\n");
		fprintf(incFile, "\tbool r_iBlkSendAvl;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tCHostCtrlMsgIntf r_iCtlMsg;\n");
		fprintf(incFile, "\tCHostCtrlMsgIntf r_oCtlRdMsg;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tsc_uint<2> r_iBlkState;\n");
		fprintf(incFile, "\tsc_uint<HIF_ARG_IBLK_SMAX_W-2> r_iBlkRdCnt;\n");
		fprintf(incFile, "\tsc_uint<MEM_ADDR_W> r_iBlkRdAdr;\n");
		fprintf(incFile, "\tsc_uint<HIF_ARG_IBLK_CNT_W> r_iBlkRdIdx;\n");
		fprintf(incFile, "\tsc_uint<4> r_iBlkFetchCnt[IBK_FETCH_CNT];\n");
		fprintf(incFile, "\tsc_uint<4> r_iBlkFetchVal[IBK_FETCH_CNT];\n");
		fprintf(incFile, "\tsc_uint<IBK_FETCH_CNT_W> r_iBlkFetchIdx;\n");
		fprintf(incFile, "\tsc_uint<IBK_FETCH_CNT_W> r_iBlkPushIdx;\n");
		fprintf(incFile, "\tsc_uint<3> r_iBlkPushCnt;\n");
		fprintf(incFile, "\tbool r_iBlkPushing;\n");
		fprintf(incFile, "\tbool r_hifToHti_datHalt;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tbool r_hifToHti_datRdy;\n");
		fprintf(incFile, "\tCHostDataQueIntf r_hifToHti_dat;\n");
		fprintf(incFile, "\tht_uint6 r_hifToHti_datAvlCnt;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tsc_uint<3> r_oBlkState;\n");
		fprintf(incFile, "\tsc_uint<HIF_ARG_OBLK_SMAX_W-2> r_oBlkWrCnt;\n");
		fprintf(incFile, "\tsc_uint<HIF_ARG_OBLK_SMAX_W-2> r_oBlkWrCntEnd;\n");
		if (b64B) {
			fprintf(incFile, "\tsc_uint<HIF_ARG_OBLK_SMAX_W-2> r_oBlkWrCntEnd64;\n");
		}
		fprintf(incFile, "\tsc_uint<4> r_oBlkRdCnt;\n");
		fprintf(incFile, "\tsc_uint<MEM_ADDR_W> r_oBlkWrAdr;\n");
		fprintf(incFile, "\tsc_uint<HIF_ARG_OBLK_CNT_W> r_oBlkWrIdx;\n");
		fprintf(incFile, "\tsc_uint<HIF_ARG_OBLK_TIME_W> r_oBlkTimer;\n");
		fprintf(incFile, "\tbool r_oBlkExpired;\n");
		fprintf(incFile, "\tbool r_htiToHif_datHalt;\n");
		fprintf(incFile, "\tbool r_hifToHti_datAvl;\n");
		fprintf(incFile, "\tsc_uint<8> r_oBlkWrPendCnt[2];\n");
		fprintf(incFile, "\tsc_uint<4> r_oBlkRdPendCnt[2];\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tCHtAssertIntf r_htaToHif_assert;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tbool r_i_reset;\n");
		fprintf(incFile, "\tbool r_reset1x;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t//\n");
		fprintf(incFile, "\t// Methods\n");
		fprintf(incFile, "\t//\n");
		fprintf(incFile, "\tvoid PersHif();\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tSC_CTOR(CPersHif) {\n");
		fprintf(incFile, "\t\tSC_METHOD(PersHif);\n");
		fprintf(incFile, "\t\tsensitive << i_clock1x.pos();\n");
		fprintf(incFile, "\t}\n");
		fprintf(incFile, "};\n");

		incFile.FileClose();
	}

	{
		string fileName = g_appArgs.GetOutputFolder() + "/HostIntf.h";

		CHtFile incFile(fileName, "w");

		GenerateBanner(incFile, "HostIntf.h", true);

		fprintf(incFile, "#include <stdint.h>\n");
		fprintf(incFile, "\n");


		if (m_includeList.size() > 0) {
			fprintf(incFile, "// User include files in HTD files\n");

			for (size_t i = 0; i < m_includeList.size(); i += 1) {
				// only print include if in src directory
				string &fullPath = m_includeList[i].m_fullPath;
				size_t pos = fullPath.find_last_of("/\\");
				if (strncmp(fullPath.c_str() + pos - 3, "src", 3) == 0)
					fprintf(incFile, "#include \"%s\"\n", m_includeList[i].m_name.c_str());
			}

			fprintf(incFile, "\n");
		}

		bool bComment = true;
		for (size_t i = 0; i < m_defineTable.size(); i += 1) {
			//	must be a host
			if (m_defineTable[i].m_scope.compare("host") == 0) {
				if (bComment) {
					fprintf(incFile, "// Module generated defines/typedefs/structs/unions\n");
					bComment = false;
				}

				fprintf(incFile, "#define\t%s\t\t%s\n",
					m_defineTable[i].m_name.c_str(),
					m_defineTable[i].m_value.c_str());
			}
		}
		for (size_t i = 0; i < m_typedefList.size(); i += 1) {
			//	must be a host
			if (m_typedefList[i].m_scope.compare("host") == 0) {
				if (bComment) {
					fprintf(incFile, "// Module generated defines/typedefs/structs/unions\n");
					bComment = false;
				}

				fprintf(incFile, "typedef\t%s\t\t%s;\n",
					m_typedefList[i].m_type.c_str(),
					m_typedefList[i].m_name.c_str());
			}
		}
		if (!bComment)
			fprintf(incFile, "\n");

		fprintf(incFile, "#ifndef HT_AE_CNT\n");
		fprintf(incFile, "#define HT_AE_CNT %d\n", g_appArgs.GetAeCnt());
		fprintf(incFile, "#endif\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "#ifndef HT_UNIT_CNT\n");
		fprintf(incFile, "#define HT_UNIT_CNT %d\n", g_appArgs.GetAeUnitCnt());
		fprintf(incFile, "#endif\n");
		fprintf(incFile, "\n");

		//fprintf(incFile, "// HIF internal commands\n");

		//fprintf(incFile, "#define HIF_CMD_CTL_PARAM\t0\n");
		//fprintf(incFile, "#define HIF_CMD_IBLK_ADR\t1\n");
		//fprintf(incFile, "#define HIF_CMD_OBLK_ADR\t2\n");
		//fprintf(incFile, "#define HIF_CMD_IBLK_PARAM\t3\n");
		//fprintf(incFile, "#define HIF_CMD_OBLK_PARAM\t4\n");
		//fprintf(incFile, "#define HIF_CMD_IBLK_RDY\t5\n");
		//fprintf(incFile, "#define HIF_CMD_OBLK_AVL\t6\n");
		//fprintf(incFile, "#define HIF_CMD_OBLK_FLSH_ADR\t7\n");
		//fprintf(incFile, "\n");
		//fprintf(incFile, "#define HIF_CMD_OBLK_RDY\t8\n");
		//fprintf(incFile, "#define HIF_CMD_IBLK_AVL\t9\n");
		//fprintf(incFile, "#define HIF_CMD_ASSERT\t\t10\n");
		//fprintf(incFile, "#define HIF_CMD_ASSERT_COLL\t11\n");
		//fprintf(incFile, "\n");
		//fprintf(incFile, "// HIF argument widths\n");
		//fprintf(incFile, "#define HIF_ARG_CTL_TIME_W\t16\n");
		//fprintf(incFile, "\n");
		//fprintf(incFile, "#define HIF_ARG_IBLK_SIZE_W\t5\n");
		//fprintf(incFile, "#define HIF_ARG_IBLK_SMAX_W\t20\n");
		//fprintf(incFile, "#define HIF_ARG_IBLK_CNT_W\t4\n");
		//fprintf(incFile, "\n");
		//fprintf(incFile, "#define HIF_ARG_OBLK_SIZE_W\t5\n");
		//fprintf(incFile, "#define HIF_ARG_OBLK_SMAX_W\t20\n");
		//fprintf(incFile, "#define HIF_ARG_OBLK_CNT_W\t4\n");
		//fprintf(incFile, "#define HIF_ARG_OBLK_TIME_W\t28\n");
		//fprintf(incFile, "\n");
		//fprintf(incFile, "#define HIF_CTL_QUE_CNT_W\t9\n");
		//fprintf(incFile, "#define HIF_CTL_QUE_CNT\t\t(1u << HIF_CTL_QUE_CNT_W)\n");
		//fprintf(incFile, "\n");
		//fprintf(incFile, "// HIF configurable arguments\n");
		//fprintf(incFile, "#define HIF_CTL_QUE_TIME\t1000\t// 1k ns\n");
		//fprintf(incFile, "\n");
		//fprintf(incFile, "#define HIF_ARG_IBLK_SIZE_LG2\t16\n");
		//fprintf(incFile, "#define HIF_ARG_IBLK_SIZE\t(1u << HIF_ARG_IBLK_SIZE_LG2)\n");
		//fprintf(incFile, "#define HIF_ARG_IBLK_CNT_LG2\t4\n");
		//fprintf(incFile, "#define HIF_ARG_IBLK_CNT\t(1u << HIF_ARG_IBLK_CNT_LG2)\n");
		//fprintf(incFile, "\n");
		//fprintf(incFile, "#define HIF_ARG_OBLK_SIZE_LG2\t16\n");
		//fprintf(incFile, "#define HIF_ARG_OBLK_SIZE\t(1u << HIF_ARG_OBLK_SIZE_LG2)\n");
		//fprintf(incFile, "#define HIF_ARG_OBLK_CNT_LG2\t4\n");
		//fprintf(incFile, "#define HIF_ARG_OBLK_CNT\t(1u << HIF_ARG_OBLK_CNT_LG2)\n");
		//fprintf(incFile, "\n");
		//fprintf(incFile, "\n");
		//fprintf(incFile, "#ifndef HT_AE_CNT\n");
		//fprintf(incFile, "#define HT_AE_CNT\t%d\n", g_appArgs.GetAeCnt());
		//fprintf(incFile, "#endif\n");
		//fprintf(incFile, "\n");
		//fprintf(incFile, "#ifndef HT_UNIT_CNT\n");
		//fprintf(incFile, "#define HT_UNIT_CNT\t%d\n", g_appArgs.GetUnitCnt());
		//fprintf(incFile, "#endif\n");
		//fprintf(incFile, "\n");
		//fprintf(incFile, "#define HIF_DQ_NULL 1\n");
		//fprintf(incFile, "#define HIF_DQ_DATA 2\n");
		//fprintf(incFile, "#define HIF_DQ_FLSH 3\n");
		//fprintf(incFile, "#define HIF_DQ_CALL 6\n");
		//fprintf(incFile, "#define HIF_DQ_HALT 7\n");
		//fprintf(incFile, "\n");
		fprintf(incFile, "// Consolidated list of user defined host messages\n");

		// consolidate list
		for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
			CModule &mod = *m_modList[modIdx];
			if (!mod.m_bIsUsed) continue;

			// transfer module message to consolidated list
			for (size_t modMsgIdx = 0; modMsgIdx < mod.m_hostMsgList.size(); modMsgIdx += 1) {
				CHostMsg &modHostMsg = mod.m_hostMsgList[modMsgIdx];

				// first check for uniqueness
				bool bFound = false;
				for (size_t dsnMsgIdx = 0; dsnMsgIdx < hostMsgList.size(); dsnMsgIdx += 1) {
					CHostMsg &dsnHostMsg = hostMsgList[dsnMsgIdx];

					if (dsnHostMsg.m_msgName == modHostMsg.m_msgName) {
						bFound = true;
						if (dsnHostMsg.m_msgDir != modHostMsg.m_msgDir)
							dsnHostMsg.m_msgDir = HtdFile::InAndOutbound;
						break;
					}
				}

				if (!bFound)
					hostMsgList.push_back(CHostMsg(modHostMsg.m_msgDir, modHostMsg.m_msgName));
			}
		}

		// first handle message used for both inbound and outbound
		int msgId = 16;
		for (size_t msgIdx = 0; msgIdx < hostMsgList.size(); msgIdx += 1) {
			CHostMsg &msg = hostMsgList[msgIdx];

			if (msg.m_msgDir != InAndOutbound) continue;

			fprintf(incFile, "#define %s %d\n", msg.m_msgName.c_str(), msgId++);
		}

		// first handle Inbound only message
		int inMsgId = msgId;
		for (size_t msgIdx = 0; msgIdx < hostMsgList.size(); msgIdx += 1) {
			CHostMsg &msg = hostMsgList[msgIdx];

			if (msg.m_msgDir != Inbound) continue;

			fprintf(incFile, "#define %s %d\n", msg.m_msgName.c_str(), inMsgId++);
		}

		// first handle Outbound only message
		int outMsgId = msgId;
		for (size_t msgIdx = 0; msgIdx < hostMsgList.size(); msgIdx += 1) {
			CHostMsg &msg = hostMsgList[msgIdx];

			if (msg.m_msgDir != Outbound) continue;

			fprintf(incFile, "#define %s %d\n", msg.m_msgName.c_str(), outMsgId++);
		}
		fprintf(incFile, "\n");

		for (size_t recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {

			if (m_recordList[recordIdx]->m_scope != eHost) continue;
			if (m_recordList[recordIdx]->m_bInclude) continue;

			GenUserStructs(incFile, m_recordList[recordIdx]);

			CHtCode htFile(incFile);
			GenUserStructBadData(htFile, true, m_recordList[recordIdx]->m_typeName,
				m_recordList[recordIdx]->m_fieldList, m_recordList[recordIdx]->m_bCStyle, "");
		}
		fprintf(incFile, "\n");

		incFile.FileClose();
	}

	////////////////////////////////////////////////////////////////
	// Generate UnitIntf.cpp file

	{
		string fileName = g_appArgs.GetOutputFolder() + "/UnitIntf.cpp";

		CHtFile cppFile(fileName, "w");

		GenerateBanner(cppFile, "UnitIntf.cpp", false);

		fprintf(cppFile, "#include \"Ht.h\"\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "namespace Ht {\n");

		fprintf(cppFile, "\tchar const * CHt%sUnit::m_moduleNameList[] = {\n", m_unitName.Uc().c_str());
		if (m_moduleFileList.size() > 0) {
			for (size_t i = 0; i < m_moduleFileList.size(); i += 1)
				fprintf(cppFile, "\t\t\"%s\"%s\n", m_moduleFileList[i].c_str(), i < m_moduleFileList.size() - 1 ? "," : "");
		} else
			fprintf(cppFile, "\t\t0\n");
		fprintf(cppFile, "\t};\n");

		fprintf(cppFile, "\tint CHt%sUnit::m_moduleNameListSize = %d;\n", m_unitName.Uc().c_str(), (int)m_moduleFileList.size());
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\tchar const * CHt%sUnit::m_sendMsgNameList[] = {\n", m_unitName.Uc().c_str());

		// first handle message used for both inbound and outbound
		int sendMsgCnt = 0;
		char const * pSeparator = "\t\t";
		for (size_t msgIdx = 0; msgIdx < hostMsgList.size(); msgIdx += 1) {
			CHostMsg &msg = hostMsgList[msgIdx];

			if (msg.m_msgDir != InAndOutbound) continue;

			fprintf(cppFile, "%s\"%s\"", pSeparator, msg.m_msgName.c_str());
			pSeparator = ",\n\t\t";
			sendMsgCnt += 1;
		}

		// first handle Inbound only message
		for (size_t msgIdx = 0; msgIdx < hostMsgList.size(); msgIdx += 1) {
			CHostMsg &msg = hostMsgList[msgIdx];

			if (msg.m_msgDir != Inbound) continue;

			fprintf(cppFile, "%s\"%s\"", pSeparator, msg.m_msgName.c_str());
			pSeparator = ",\n\t\t";
			sendMsgCnt += 1;
		}
		if (sendMsgCnt > 0) {
			fprintf(cppFile, "\n");
		} else
			fprintf(cppFile, "\t\t0\n");

		fprintf(cppFile, "\t};\n");
		fprintf(cppFile, "\tint CHt%sUnit::m_sendMsgNameListSize = %d;\n", m_unitName.Uc().c_str(), sendMsgCnt);

		fprintf(cppFile, "\tchar const * CHt%sUnit::m_recvMsgNameList[] = {\n", m_unitName.Uc().c_str());

		// first handle message used for both inbound and outbound
		int recvMsgCnt = 0;
		pSeparator = "\t\t";
		for (size_t msgIdx = 0; msgIdx < hostMsgList.size(); msgIdx += 1) {
			CHostMsg &msg = hostMsgList[msgIdx];

			if (msg.m_msgDir != InAndOutbound) continue;

			fprintf(cppFile, "%s\"%s\"", pSeparator, msg.m_msgName.c_str());
			pSeparator = ",\n\t\t";
			recvMsgCnt += 1;
		}

		// first handle Inbound only message
		for (size_t msgIdx = 0; msgIdx < hostMsgList.size(); msgIdx += 1) {
			CHostMsg &msg = hostMsgList[msgIdx];

			if (msg.m_msgDir != Outbound) continue;

			fprintf(cppFile, "%s\"%s\"", pSeparator, msg.m_msgName.c_str());
			pSeparator = ",\n\t\t";
			recvMsgCnt += 1;
		}
		if (recvMsgCnt > 0) {
			fprintf(cppFile, "\n");
		} else
			fprintf(cppFile, "\t\t0\n");

		fprintf(cppFile, "\t};\n");
		fprintf(cppFile, "\tint CHt%sUnit::m_recvMsgNameListSize = %d;\n", m_unitName.Uc().c_str(), recvMsgCnt);
		fprintf(cppFile, "\n");

		fprintf(cppFile, "#\tifndef HT_MODEL_UNIT_CLASS\n");
		fprintf(cppFile, "#\tdefine HT_MODEL_UNIT_CLASS CHtModel%sUnit\n", m_unitName.Uc().c_str());
		fprintf(cppFile, "#\tendif\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "#\tif defined(HT_MODEL) && !defined(HT_MODEL_NO_UNIT_THREADS)\n");
		fprintf(cppFile, "\tvoid HtCoprocModel(CHtHifLibBase * pHtHifLibBase)\n");
		fprintf(cppFile, "\t{\n");
		fprintf(cppFile, "\t\tCHtModelHif *pModel = new CHtModelHif(pHtHifLibBase);\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tpModel->StartUnitThreads<HT_MODEL_UNIT_CLASS>();\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tpModel->WaitForUnitThreads();\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tdelete pModel;\n");
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "#\tendif\n");

		fprintf(cppFile, "}\n");

		cppFile.FileClose();
	}

	////////////////////////////////////////////////////////////////
	// Generate UnitIntf.h file

	{
		string fileName = g_appArgs.GetOutputFolder() + "/UnitIntf.h";

		CHtFile incFile(fileName, "w");

		GenerateBanner(incFile, "UnitIntf.h", true);

		// find entry point parameter list

		char * pSeparator;

		CCxrCall * pCxrCall = hifMod.m_cxrCallList[0];
		CCxrEntry * pCxrEntry = pCxrCall->m_pairedFunc.GetCxrEntry();// m_pModInst->m_pMod->m_cxrEntryList[pCxrCall->m_pairedFunc.m_idx];
		CCxrReturn * pCxrReturn = pCxrCall->m_pairedReturnList[0].GetCxrReturn();// m_pModInst->m_pMod->m_cxrReturnList[pCxrCall->m_pairedReturnList[0].m_idx];

#ifndef OLD_UNIT_INTF

		fprintf(incFile, "#ifndef _HTV\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "namespace Ht {\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tclass CHt%sUnit : public CHtUnitBase {\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\tpublic:\n");

		fprintf(incFile, "\t\tCHt%sUnit(CHtHifBase * pHtHifBase, int numaSet=-1) : CHtUnitBase(pHtHifBase, numaSet) {}\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\n");

		fprintf(incFile, "#\t\tifdef HT_USING_HOST_UNIT_THREAD\n");
		fprintf(incFile, "\t\t\tvirtual void UnitThread();\n");
		fprintf(incFile, "#\t\telse\n");
		fprintf(incFile, "\t\t\tvirtual void UnitThread() {}\n");
		fprintf(incFile, "#\t\tendif\n");

		fprintf(incFile, "\n");

		fprintf(incFile, "\t\tstatic void * StartUnitThread(void * pVoid) {\n");
		fprintf(incFile, "\t\t\treinterpret_cast<CHt%sUnit *>(pVoid)->UnitThread();\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\t\t\treturn 0;\n");
		fprintf(incFile, "\t\t}\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\t\tuint8_t * Get%sUnitMemBase() { return GetUnitMemBase(); }\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\n");

		fprintf(incFile, "\t\tstatic char const * m_moduleNameList[];\n");
		fprintf(incFile, "\t\tstatic int m_moduleNameListSize;\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\t\tchar const * GetModuleName(int modId) {\n");
		fprintf(incFile, "\t\t\tif (modId >= m_moduleNameListSize)\n");
		fprintf(incFile, "\t\t\t\treturn 0;\n");
		fprintf(incFile, "\t\t\treturn m_moduleNameList[modId];\n");
		fprintf(incFile, "\t\t}\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\t\tstatic char const * m_sendMsgNameList[];\n");
		fprintf(incFile, "\t\tstatic int m_sendMsgNameListSize;\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\t\tchar const * GetSendMsgName(int msgId) {\n");
		fprintf(incFile, "\t\t\tmsgId -= 16;\n");
		fprintf(incFile, "\t\t\tif (msgId < 0 || msgId >= m_sendMsgNameListSize)\n");
		fprintf(incFile, "\t\t\t\treturn \"Unknown\";\n");
		fprintf(incFile, "\t\t\treturn m_sendMsgNameList[msgId];\n");
		fprintf(incFile, "\t\t}\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\t\tstatic char const * m_recvMsgNameList[];\n");
		fprintf(incFile, "\t\tstatic int m_recvMsgNameListSize;\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\t\tchar const * GetRecvMsgName(int msgId) {\n");
		fprintf(incFile, "\t\t\tmsgId -= 16;\n");
		fprintf(incFile, "\t\t\tif (msgId < 0 || msgId >= m_recvMsgNameListSize)\n");
		fprintf(incFile, "\t\t\t\treturn \"Unknown\";\n");
		fprintf(incFile, "\t\t\treturn m_recvMsgNameList[msgId];\n");
		fprintf(incFile, "\t\t}\n");
		fprintf(incFile, "\n");

		if (bInHostMsg) {
			fprintf(incFile, "\t\tvoid SendHostMsg(uint8_t msgType, uint64_t msgData) {\n");
			fprintf(incFile, "\t\t\tCHtUnitBase::SendHostMsg(msgType, msgData);\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");
		}

		if (bOutHostMsg) {
			fprintf(incFile, "\t\tbool RecvHostMsg(uint8_t &msgType, uint64_t &msgData) {\n");
			fprintf(incFile, "\t\t\treturn CHtUnitBase::RecvHostMsg(msgType, msgData);\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");
		}

		if (bInHostData) {
			fprintf(incFile, "\t\tint SendHostData(int qwCnt, uint64_t * pData) {\n");
			fprintf(incFile, "\t\t\treturn CHtUnitBase::SendHostData(qwCnt, pData);\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\t\tbool SendHostDataMarker() {\n");
			fprintf(incFile, "\t\t\treturn CHtUnitBase::SendHostDataMarker();\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\t\tvoid FlushHostData() {\n");
			fprintf(incFile, "\t\t\tCHtUnitBase::FlushHostData();\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\t\tvoid SendHostDataFlush() {\n");
			fprintf(incFile, "\t\t\tCHtUnitBase::FlushHostData();\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");
		}

		if (bOutHostData) {
			fprintf(incFile, "\t\tbool RecvHostDataPeek(uint64_t & data) {\n");
			fprintf(incFile, "\t\t\treturn CHtUnitBase::PeekHostData(data);\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\t\tbool PeekHostData(uint64_t & data) {\n");
			fprintf(incFile, "\t\t\treturn CHtUnitBase::PeekHostData(data);\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\t\tint RecvHostData(int qwCnt, uint64_t * pData) {\n");
			fprintf(incFile, "\t\t\treturn CHtUnitBase::RecvHostData(qwCnt, pData);\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\t\tbool RecvHostDataMarker() {\n");
			fprintf(incFile, "\t\t\treturn CHtUnitBase::RecvHostDataMarker();\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");
		}

		fprintf(incFile, "\tpublic:\n");
		fprintf(incFile, "\t\tstruct CCallArgs_%s {\n", pCxrCall->m_modEntry.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "\t\t\t%s m_%s;\n", pParam->m_hostType.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\t\t};\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tprivate:\n");
		fprintf(incFile, "\t\tstruct CCallArgv_%s {\n", pCxrCall->m_modEntry.c_str());

		int argQwCnt = 0;
		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			int fldQwCnt = (FindHostTypeWidth(pParam) + 63) / 64;
			string declIdx = fldQwCnt == 1 ? "" : VA("[%d]", fldQwCnt);

			fprintf(incFile, "\t\t\tuint64_t m_%s%s;\n", pParam->m_name.c_str(), declIdx.c_str());

			argQwCnt += fldQwCnt;
		}

		fprintf(incFile, "\t\t};\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tpublic:\n");
		fprintf(incFile, "\t\tbool SendCall_%s(CCallArgs_%s &args) {\n",
			pCxrCall->m_modEntry.c_str(), pCxrCall->m_modEntry.c_str());

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\tint argc = %d;\n", (int)pCxrEntry->m_paramList.size());
		fprintf(incFile, "\t\t\tint argw[%d];\n", max(1, (int)pCxrEntry->m_paramList.size()));
		fprintf(incFile, "\t\t\tCCallArgv_%s argv;\n", pCxrCall->m_modEntry.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			int fldQwCnt = (FindHostTypeWidth(pParam) + 63) / 64;

			fprintf(incFile, "\n");
			fprintf(incFile, "\t\t\tassert((sizeof(%s) + 7)/8 == %d);\n", pParam->m_hostType.c_str(), fldQwCnt);
			fprintf(incFile, "\t\t\targw[%d] = %d;\n", (int)fldIdx, fldQwCnt);
			fprintf(incFile, "\t\t\tunion Args_%s {\n", pParam->m_name.c_str());
			fprintf(incFile, "\t\t\t\tuint64_t m_data[%d];\n", fldQwCnt);
			fprintf(incFile, "\t\t\t\t%s m_%s;\n", pParam->m_hostType.c_str(), pParam->m_name.c_str());
			fprintf(incFile, "\t\t\t} args_%s;\n", pParam->m_name.c_str());

			for (int fldQwIdx = 0; fldQwIdx < fldQwCnt; fldQwIdx += 1)
				fprintf(incFile, "\t\t\targs_%s.m_data[%d] = 0;\n", pParam->m_name.c_str(), fldQwIdx);

			fprintf(incFile, "\t\t\targs_%s.m_%s = args.m_%s;\n", pParam->m_name.c_str(), pParam->m_name.c_str(), pParam->m_name.c_str());

			if (fldQwCnt == 1)
				fprintf(incFile, "\t\t\targv.m_%s = args_%s.m_data[0];\n", pParam->m_name.c_str(), pParam->m_name.c_str());
			else
				for (int fldQwIdx = 0; fldQwIdx < fldQwCnt; fldQwIdx += 1)
					fprintf(incFile, "\t\t\targv.m_%s[%d] = args_%s.m_data[%d];\n", pParam->m_name.c_str(), fldQwIdx, pParam->m_name.c_str(), fldQwIdx);
		}

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\treturn CHtUnitBase::SendCall(argc, argw, (uint64_t *)&argv);\n");
		fprintf(incFile, "\t\t}\n");

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tbool SendCall_%s(", pCxrCall->m_modEntry.c_str());

		pSeparator = "";

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "%s%s %s", pSeparator, pParam->m_hostType.c_str(), pParam->m_name.c_str());

			pSeparator = ", ";
		}

		fprintf(incFile, ") {\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\tint argc = %d;\n", (int)pCxrEntry->m_paramList.size());
		fprintf(incFile, "\t\t\tint argw[%d];\n", max(1, (int)pCxrEntry->m_paramList.size()));
		fprintf(incFile, "\t\t\tCCallArgv_%s argv;\n", pCxrCall->m_modEntry.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			int fldQwCnt = (FindHostTypeWidth(pParam) + 63) / 64;

			fprintf(incFile, "\n");
			fprintf(incFile, "\t\t\tassert((sizeof(%s) + 7)/8 == %d);\n", pParam->m_hostType.c_str(), fldQwCnt);
			fprintf(incFile, "\t\t\targw[%d] = %d;\n", (int)fldIdx, fldQwCnt);
			fprintf(incFile, "\t\t\tunion Argv_%s {\n", pParam->m_name.c_str());
			fprintf(incFile, "\t\t\t\tuint64_t m_data[%d];\n", fldQwCnt);
			fprintf(incFile, "\t\t\t\t%s m_%s;\n", pParam->m_hostType.c_str(), pParam->m_name.c_str());
			fprintf(incFile, "\t\t\t} argv_%s;\n", pParam->m_name.c_str());

			for (int fldQwIdx = 0; fldQwIdx < fldQwCnt; fldQwIdx += 1)
				fprintf(incFile, "\t\t\targv_%s.m_data[%d] = 0;\n", pParam->m_name.c_str(), fldQwIdx);

			fprintf(incFile, "\t\t\targv_%s.m_%s = %s;\n", pParam->m_name.c_str(), pParam->m_name.c_str(), pParam->m_name.c_str());

			if (fldQwCnt == 1)
				fprintf(incFile, "\t\t\targv.m_%s = argv_%s.m_data[0];\n", pParam->m_name.c_str(), pParam->m_name.c_str());
			else
				for (int fldQwIdx = 0; fldQwIdx < fldQwCnt; fldQwIdx += 1)
					fprintf(incFile, "\t\t\targv.m_%s[%d] = argv_%s.m_data[%d];\n", pParam->m_name.c_str(), fldQwIdx, pParam->m_name.c_str(), fldQwIdx);
		}

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\treturn CHtUnitBase::SendCall(argc, argw, (uint64_t *)&argv);\n");
		fprintf(incFile, "\t\t}\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tpublic:\n");
		fprintf(incFile, "\t\tstruct CRtnArgs_%s {\n", pCxrCall->m_modEntry.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "\t\t\t%s m_%s;\n", pParam->m_hostType.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\t\t};\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tprivate:\n");
		fprintf(incFile, "\t\tstruct CRtnArgv_%s {\n", pCxrCall->m_modEntry.c_str());

		int rtnQwCnt = 0;
		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			int fldQwCnt = (FindHostTypeWidth(pParam) + 63) / 64;
			string declIdx = fldQwCnt == 1 ? "" : VA("[%d]", fldQwCnt);

			fprintf(incFile, "\t\t\tuint64_t m_%s%s;\n", pParam->m_name.c_str(), declIdx.c_str());

			rtnQwCnt += fldQwCnt;
		}

		fprintf(incFile, "\t\t};\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tpublic:\n");
		fprintf(incFile, "\t\tbool RecvReturn_%s(CRtnArgs_%s &args) {\n",
			pCxrCall->m_modEntry.c_str(), pCxrCall->m_modEntry.c_str());

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\tint argc = %d;\n", rtnQwCnt);
		fprintf(incFile, "\t\t\tCRtnArgv_%s argv;\n", pCxrCall->m_modEntry.c_str());
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\tif (!CHtUnitBase::RecvReturn(argc, (uint64_t *)&argv))\n");
		fprintf(incFile, "\t\t\t\treturn false;\n");

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			int fldQwCnt = (FindHostTypeWidth(pParam) + 63) / 64;

			fprintf(incFile, "\n");
			fprintf(incFile, "\t\t\tassert((sizeof(%s) + 7)/8 == %d);\n", pParam->m_hostType.c_str(), fldQwCnt);
			fprintf(incFile, "\t\t\tunion Args_%s {\n", pParam->m_name.c_str());
			fprintf(incFile, "\t\t\t\tuint64_t m_data[%d];\n", fldQwCnt);
			fprintf(incFile, "\t\t\t\t%s m_%s;\n", pParam->m_hostType.c_str(), pParam->m_name.c_str());
			fprintf(incFile, "\t\t\t} args_%s;\n", pParam->m_name.c_str());

			if (fldQwCnt == 1)
				fprintf(incFile, "\t\t\targs_%s.m_data[0] = argv.m_%s;\n", pParam->m_name.c_str(), pParam->m_name.c_str());
			else
				for (int fldQwIdx = 0; fldQwIdx < fldQwCnt; fldQwIdx += 1)
					fprintf(incFile, "\t\t\targs_%s.m_data[%d] = argv.m_%s[%d];\n", pParam->m_name.c_str(), fldQwIdx, pParam->m_name.c_str(), fldQwIdx);

			fprintf(incFile, "\t\t\targs.m_%s = args_%s.m_%s;\n", pParam->m_name.c_str(), pParam->m_name.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\treturn true;\n");
		fprintf(incFile, "\t\t}\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tbool RecvReturn_%s(", pCxrCall->m_modEntry.c_str());

		pSeparator = "";

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "%s%s &%s", pSeparator, pParam->m_hostType.c_str(), pParam->m_name.c_str());

			pSeparator = ", ";
		}

		fprintf(incFile, ") {\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\tint argc = %d;\n", rtnQwCnt);
		fprintf(incFile, "\t\t\tCRtnArgv_%s argv;\n", pCxrCall->m_modEntry.c_str());
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\tif (!CHtUnitBase::RecvReturn(argc, (uint64_t *)&argv))\n");
		fprintf(incFile, "\t\t\t\treturn false;\n");

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			int fldQwCnt = (FindHostTypeWidth(pParam) + 63) / 64;

			fprintf(incFile, "\n");
			fprintf(incFile, "\t\t\tassert((sizeof(%s) + 7)/8 == %d);\n", pParam->m_hostType.c_str(), fldQwCnt);
			fprintf(incFile, "\t\t\tunion Argv_%s {\n", pParam->m_name.c_str());
			fprintf(incFile, "\t\t\t\tuint64_t m_data[%d];\n", fldQwCnt);
			fprintf(incFile, "\t\t\t\t%s m_%s;\n", pParam->m_hostType.c_str(), pParam->m_name.c_str());
			fprintf(incFile, "\t\t\t} argv_%s;\n", pParam->m_name.c_str());

			if (fldQwCnt == 1)
				fprintf(incFile, "\t\t\targv_%s.m_data[0] = argv.m_%s;\n", pParam->m_name.c_str(), pParam->m_name.c_str());
			else
				for (int fldQwIdx = 0; fldQwIdx < fldQwCnt; fldQwIdx += 1)
					fprintf(incFile, "\t\t\targv_%s.m_data[%d] = argv.m_%s[%d];\n", pParam->m_name.c_str(), fldQwIdx, pParam->m_name.c_str(), fldQwIdx);

			fprintf(incFile, "\t\t\t%s = argv_%s.m_%s;\n", pParam->m_name.c_str(), pParam->m_name.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\treturn true;\n");
		fprintf(incFile, "\t\t}\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t};\n");
		fprintf(incFile, "\n");

#else

		fprintf(incFile, "#ifndef _HTV\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "class CHt%sUnit : public CHtUnitBase {\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\tfriend class CHtHif;\n");
		fprintf(incFile, "public:\n");

		fprintf(incFile, "\tuint8_t * Get%sUnitMemBase() { return GetUnitMemBase(); }\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\n");

		fprintf(incFile, "\tvoid SetCallback( void (* pCbFuncPtr)(CHt%sUnit *, CHt%sUnit::ERecvType) ) {\n",
			m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
		fprintf(incFile, "\t\tm_pCbFuncPtr = pCbFuncPtr;\n");
		fprintf(incFile, "\t\tSetUsingCallback(pCbFuncPtr != 0);\n");
		fprintf(incFile, "\t}\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tERecvType RecvPoll(bool bUseCb=true) {\n");
		fprintf(incFile, "\t\treturn RecvPollBase(bUseCb);\n");
		fprintf(incFile, "\t}\n");
		fprintf(incFile, "\n");

		if (bInHostMsg) {
			fprintf(incFile, "\tvoid SendHostMsg(uint8_t msgType, uint64_t msgData) {\n");
			fprintf(incFile, "\t\tSendHostMsgBase(msgType, msgData);\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");
		}

		if (bOutHostMsg) {
			fprintf(incFile, "\tbool RecvHostMsg(uint8_t &msgType, uint64_t &msgData) {\n");
			fprintf(incFile, "\t\treturn RecvHostMsgBase(msgType, msgData);\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");
		}

		if (bInHostData) {
			fprintf(incFile, "\tint SendHostData(int qwCnt, uint64_t * pData) {\n");
			fprintf(incFile, "\t\treturn SendHostDataBase(qwCnt, pData);\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\tbool SendHostDataMarker() {\n");
			fprintf(incFile, "\t\treturn SendHostDataMarkerBase();\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\tvoid FlushHostData() {\n");
			fprintf(incFile, "\t\tFlushHostDataBase();\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\tvoid SendHostDataFlush() {\n");
			fprintf(incFile, "\t\tFlushHostDataBase();\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");
		}

		if (bOutHostData) {
			fprintf(incFile, "\tbool RecvHostDataPeek(uint64_t & data) {\n");
			fprintf(incFile, "\t\treturn PeekHostDataBase(data);\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\tbool PeekHostData(uint64_t & data) {\n");
			fprintf(incFile, "\t\treturn PeekHostDataBase(data);\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\tint RecvHostData(int qwCnt, uint64_t * pData) {\n");
			fprintf(incFile, "\t\treturn RecvHostDataBase(qwCnt, pData);\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\tbool RecvHostDataMarker() {\n");
			fprintf(incFile, "\t\treturn RecvHostDataMarkerBase();\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");
		}

		fprintf(incFile, "public:\n");
		fprintf(incFile, "\tstruct CCallArgs_%s {\n", pCxrCall->m_modEntry.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "\t\t%s m_%s;\n", pParam->m_hostType.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\t};\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "private:\n");
		fprintf(incFile, "\tstruct CCallArgv_%s {\n", pCxrCall->m_modEntry.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "\t\tuint64_t m_%s;\n", pParam->m_name.c_str());
		}

		fprintf(incFile, "\t};\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "public:\n");
		fprintf(incFile, "\tbool SendCall_%s(CCallArgs_%s &args) {\n",
			pCxrCall->m_modEntry.c_str(), pCxrCall->m_modEntry.c_str());

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tint argc = %d;\n", (int)pCxrEntry->m_paramList.size());
		fprintf(incFile, "\t\tCCallArgv_%s argv;\n", pCxrCall->m_modEntry.c_str());
		fprintf(incFile, "\n");

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "\t\targv.m_%s = (uint64_t)args.m_%s;\n", pParam->m_name.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\treturn SendCallBase(argc, (uint64_t *)&argv);\n");
		fprintf(incFile, "\t}\n");

		fprintf(incFile, "\n");
		fprintf(incFile, "\tbool SendCall_%s(", pCxrCall->m_modEntry.c_str());

		pSeparator = "";

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "%s%s %s", pSeparator, pParam->m_hostType.c_str(), pParam->m_name.c_str());

			pSeparator = ", ";
		}

		fprintf(incFile, ") {\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tint argc = %d;\n", (int)pCxrEntry->m_paramList.size());
		fprintf(incFile, "\t\tCCallArgv_%s argv;\n", pCxrCall->m_modEntry.c_str());
		fprintf(incFile, "\n");

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "\t\targv.m_%s = (uint64_t)%s;\n", pParam->m_name.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\treturn SendCallBase(argc, (uint64_t *)&argv);\n");
		fprintf(incFile, "\t}\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "public:\n");
		fprintf(incFile, "\tstruct CRtnArgs_%s {\n", pCxrCall->m_modEntry.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "\t\t%s m_%s;\n", pParam->m_hostType.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\t};\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "private:\n");
		fprintf(incFile, "\tstruct CRtnArgv_%s {\n", pCxrCall->m_modEntry.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "\t\tuint64_t m_%s;\n", pParam->m_name.c_str());
		}

		fprintf(incFile, "\t};\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "public:\n");
		fprintf(incFile, "\tbool RecvReturn_%s(CRtnArgs_%s &args) {\n",
			pCxrCall->m_modEntry.c_str(), pCxrCall->m_modEntry.c_str());

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tint argc = %d;\n", (int)pCxrReturn->m_paramList.size());
		fprintf(incFile, "\t\tCRtnArgv_%s argv;\n", pCxrCall->m_modEntry.c_str());
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tif (!RecvReturnBase(argc, (uint64_t *)&argv))\n");
		fprintf(incFile, "\t\t\treturn false;\n");
		fprintf(incFile, "\n");

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "\t\targs.m_%s = (%s)argv.m_%s;\n", pParam->m_name.c_str(), pParam->m_hostType.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\treturn true;\n");
		fprintf(incFile, "\t}\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tbool RecvReturn_%s(", pCxrCall->m_modEntry.c_str());

		pSeparator = "";

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "%s%s &%s", pSeparator, pParam->m_hostType.c_str(), pParam->m_name.c_str());

			pSeparator = ", ";
		}

		fprintf(incFile, ") {\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tint argc = %d;\n", (int)pCxrReturn->m_paramList.size());
		fprintf(incFile, "\t\tCRtnArgv_%s argv;\n", pCxrCall->m_modEntry.c_str());
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tif (!RecvReturnBase(argc, (uint64_t *)&argv))\n");
		fprintf(incFile, "\t\t\treturn false;\n");
		fprintf(incFile, "\n");

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "\t\t%s = (%s)argv.m_%s;\n", pParam->m_name.c_str(), pParam->m_hostType.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\treturn true;\n");
		fprintf(incFile, "\t}\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "private:\n");
		fprintf(incFile, "\tCHt%sUnit() {}\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\tCHt%sUnit(CHtHifBase * pHtHif, int unitId) : CHtUnitBase(pHtHif, unitId) {}\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\tvirtual ~CHt%sUnit() {}\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\tvoid Callback(CHtUnitBase::ERecvType recvType);\n");
		fprintf(incFile, "\tvoid (* m_pCbFuncPtr)(CHt%sUnit *, CHt%sUnit::ERecvType);\n",
			m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
		fprintf(incFile, "};\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "class CHtHif : public CHtHifBase {\n");
		fprintf(incFile, "public:\n");
		fprintf(incFile, "\tCHtHif(CHtHifParams * pParams = 0) : CHtHifBase(pParams, sizeof(CHt%sUnit)) {};\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\tvirtual ~CHtHif();\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tint Get%sUnitCnt() { return GetUnitCnt(); }\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\tCHt%sUnit * const * AllocAll%sUnits() { return (CHt%sUnit * const *) AllocAllUnits(); }\n",
			m_unitName.Uc().c_str(), m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
		fprintf(incFile, "\tvoid FreeAll%sUnits() { FreeAllUnits(); }\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\tCHt%sUnit * Alloc%sUnit(int numaSet=-1) { return (CHt%sUnit *) AllocUnit(numaSet); }\n",
			m_unitName.Uc().c_str(), m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
		fprintf(incFile, "\tvoid Free%sUnit(CHt%sUnit * p%sUnit) { FreeUnit((CHtUnitBase *)p%sUnit); }\n",
			m_unitName.Uc().c_str(), m_unitName.Uc().c_str(), m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
		fprintf(incFile, "\n");
		fprintf(incFile, "\tvoid SendAll%sHostMsg(uint8_t msgType, uint64_t msgData);\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\n");
		fprintf(incFile, "private:\n");
		fprintf(incFile, "\tvoid InitUnit(int unitId);\n");
		fprintf(incFile, "};\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "inline void CHt%sUnit::Callback(CHtUnitBase::ERecvType recvType)\n", m_unitName.Uc().c_str());
		fprintf(incFile, "{\n");
		fprintf(incFile, "\t(* m_pCbFuncPtr)(this, recvType);\n");
		fprintf(incFile, "}\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "inline void CHtHif::SendAll%sHostMsg(uint8_t msgType, uint64_t msgData)\n", m_unitName.Uc().c_str());
		fprintf(incFile, "{\n");
		fprintf(incFile, "\tfor (int i = 0; i < Get%sUnitCnt(); i += 1) {\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\t\tif (m_ppHtUnits[i] == 0)\n");
		fprintf(incFile, "\t\t\tInitUnit(i);\n");
		fprintf(incFile, "\t\tm_ppHtUnits[i]->SendHostMsgBase(msgType, msgData);\n");
		fprintf(incFile, "\t}\n");
		fprintf(incFile, "}\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "inline void CHtHif::InitUnit(int unitId) {\n");
		fprintf(incFile, "\tassert(unitId < GetUnitCnt());\n");
		fprintf(incFile, "\tif (m_ppHtUnits[unitId] == 0)\n");
		fprintf(incFile, "\t\tm_ppHtUnits[unitId] = new (GetUnitMemBase(unitId)) CHt%sUnit(this, unitId);\n", m_unitName.Uc().c_str());
		fprintf(incFile, "}\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "inline CHtHif::~CHtHif()\n");
		fprintf(incFile, "{\n");
		fprintf(incFile, "\t// send shutdown message and free host resources\n");
		fprintf(incFile, "\tfor (int i = 0; i < GetUnitCnt(); i += 1) {\n");
		fprintf(incFile, "\t\tif (m_ppHtUnits[i] == 0)\n");
		fprintf(incFile, "\t\t\tInitUnit(i);\n");
		fprintf(incFile, "\t\twhile (!m_ppHtUnits[i]->SendHostHaltBase())\n");
		fprintf(incFile, "\t\t\tm_ppHtUnits[i]->HaltLoop();\n");
		fprintf(incFile, "\t}\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t// wait for coproc dispatch to complete\n");
		fprintf(incFile, "\twhile (m_bCoprocBusy) {\n");
		fprintf(incFile, "\t\tfor (int i = 0; i < GetUnitCnt(); i += 1)\n");
		fprintf(incFile, "\t\t\tm_ppHtUnits[i]->HaltLoop();\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tusleep(1);\n");
		fprintf(incFile, "\t}\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tm_bHaltSent = true;\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t// wait for timer thread to terminate\n");
		fprintf(incFile, "\twhile (m_htHifParams.m_iBlkTimerUSec > 0 && !m_bTimerHaltSeen);\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t// wait for monitor thread to terminate\n");
		fprintf(incFile, "\twhile (GetHtDebug() > 0 && !m_bMonitorHaltSeen);\n");
		fprintf(incFile, "}\n");
		fprintf(incFile, "\n");
#endif

#ifndef OLD_UNIT_INTF

		fprintf(incFile, "\t//////////////////////////////////////////////////////\n");
		fprintf(incFile, "\t// HT_MODEL Interface\n");
		fprintf(incFile, "#\tifdef HT_MODEL\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tclass CHtModel%sUnit : public CHtModelUnitBase {\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\tpublic:\n");
		fprintf(incFile, "\t\tCHtModel%sUnit(CHtModelHifBase * pHtModelHifBase) : CHtModelUnitBase(pHtModelHifBase) {}\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\n");

		fprintf(incFile, "#\t\tifndef HT_MODEL_NO_UNIT_THREADS\n");
		fprintf(incFile, "\t\tvirtual void UnitThread();\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\t\tstatic void * StartUnitThread(void * pVoid) {\n");
		fprintf(incFile, "\t\t\treinterpret_cast<CHtModel%sUnit *>(pVoid)->UnitThread();\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\t\t\treturn 0;\n");
		fprintf(incFile, "\t\t}\n");
		fprintf(incFile, "#\t\tendif\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tpublic:\n");
		fprintf(incFile, "\t\tERecvType RecvPoll(bool bUseCbCall=true, bool bUseCbMsg=true, bool bUseCbData=true) {\n");
		fprintf(incFile, "\t\t\treturn CHtModelUnitBase::RecvPoll(bUseCbCall, bUseCbMsg, bUseCbData);\n");
		fprintf(incFile, "\t\t}\n");
		fprintf(incFile, "\n");

		if (bOutHostMsg) {
			fprintf(incFile, "\t\tvoid SendHostMsg(uint8_t msgType, uint64_t msgData) {\n");
			fprintf(incFile, "\t\t\tCHtModelUnitBase::SendHostMsg(msgType, msgData);\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");
		}

		if (bInHostMsg) {
			fprintf(incFile, "\t\tbool RecvHostMsg(uint8_t &msgType, uint64_t &msgData) {\n");
			fprintf(incFile, "\t\t\treturn CHtModelUnitBase::RecvHostMsg(msgType, msgData);\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");
		}

		if (bOutHostData) {
			fprintf(incFile, "\t\tint SendHostData(int qwCnt, uint64_t * pData) {\n");
			fprintf(incFile, "\t\t\treturn CHtModelUnitBase::SendHostData(qwCnt, pData);\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\t\tbool SendHostDataMarker() {\n");
			fprintf(incFile, "\t\t\treturn CHtModelUnitBase::SendHostDataMarker();\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\t\tvoid FlushHostData() {\n");
			fprintf(incFile, "\t\t\tCHtModelUnitBase::FlushHostData();\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\t\tvoid SendHostDataFlush() {\n");
			fprintf(incFile, "\t\t\tCHtModelUnitBase::FlushHostData();\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");
		}

		if (bInHostData) {
			fprintf(incFile, "\t\tbool RecvHostDataPeek(uint64_t & data) {\n");
			fprintf(incFile, "\t\t\treturn CHtModelUnitBase::PeekHostData(data);\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\t\tbool PeekHostData(uint64_t & data) {\n");
			fprintf(incFile, "\t\t\treturn CHtModelUnitBase::PeekHostData(data);\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\t\tint RecvHostData(int qwCnt, uint64_t * pData) {\n");
			fprintf(incFile, "\t\t\treturn CHtModelUnitBase::RecvHostData(qwCnt, pData);\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\t\tbool RecvHostDataMarker() {\n");
			fprintf(incFile, "\t\t\treturn CHtModelUnitBase::RecvHostDataMarker();\n");
			fprintf(incFile, "\t\t}\n");
			fprintf(incFile, "\n");
		}

		fprintf(incFile, "\tpublic:\n");
		fprintf(incFile, "\t\tstruct CCallArgs_%s {\n", pCxrCall->m_modEntry.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "\t\t\t%s m_%s;\n", pParam->m_hostType.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\t\t};\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tprivate:\n");
		fprintf(incFile, "\t\tstruct CCallArgv_%s {\n", pCxrCall->m_modEntry.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "\t\t\tuint64_t m_%s;\n", pParam->m_name.c_str());
		}

		fprintf(incFile, "\t\t};\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tpublic:\n");
		fprintf(incFile, "\t\tbool RecvCall_%s(CCallArgs_%s &args) {\n",
			pCxrCall->m_modEntry.c_str(), pCxrCall->m_modEntry.c_str());

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\tint argc = %d;\n", (int)pCxrEntry->m_paramList.size());
		fprintf(incFile, "\t\t\tCCallArgv_%s argv;\n", pCxrCall->m_modEntry.c_str());
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\tif (!CHtModelUnitBase::RecvCall(argc, (uint64_t *)&argv))\n");
		fprintf(incFile, "\t\t\t\treturn false;\n");
		fprintf(incFile, "\n");

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "\t\t\targs.m_%s = (%s)argv.m_%s;\n", pParam->m_name.c_str(), pParam->m_hostType.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\treturn true;\n");
		fprintf(incFile, "\t\t}\n");

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tbool RecvCall_%s(", pCxrCall->m_modEntry.c_str());

		pSeparator = "";

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "%s%s &%s", pSeparator, pParam->m_hostType.c_str(), pParam->m_name.c_str());

			pSeparator = ", ";
		}

		fprintf(incFile, ") {\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\tint argc = %d;\n", (int)pCxrEntry->m_paramList.size());
		fprintf(incFile, "\t\t\tCCallArgv_%s argv;\n", pCxrCall->m_modEntry.c_str());
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\tif (!CHtModelUnitBase::RecvCall(argc, (uint64_t *)&argv))\n");
		fprintf(incFile, "\t\t\t\treturn false;\n");
		fprintf(incFile, "\n");

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "\t\t\t%s = (%s)argv.m_%s;\n", pParam->m_name.c_str(), pParam->m_hostType.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\treturn true;\n");
		fprintf(incFile, "\t\t}\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tpublic:\n");
		fprintf(incFile, "\t\tstruct CRtnArgs_%s {\n", pCxrCall->m_modEntry.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "\t\t\t%s m_%s;\n", pParam->m_hostType.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\t\t};\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tprivate:\n");
		fprintf(incFile, "\t\tstruct CRtnArgv_%s {\n", pCxrCall->m_modEntry.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "\t\t\tuint64_t m_%s;\n", pParam->m_name.c_str());
		}

		fprintf(incFile, "\t\t};\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tpublic:\n");
		fprintf(incFile, "\t\tbool SendReturn_%s(CRtnArgs_%s &args) {\n",
			pCxrCall->m_modEntry.c_str(), pCxrCall->m_modEntry.c_str());

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\tint argc = %d;\n", (int)pCxrReturn->m_paramList.size());
		fprintf(incFile, "\t\t\tCRtnArgv_%s argv;\n", pCxrCall->m_modEntry.c_str());
		fprintf(incFile, "\n");

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "\t\t\targv.m_%s = (uint64_t)args.m_%s;\n", pParam->m_name.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\t\t\treturn CHtModelUnitBase::SendReturn(argc, (uint64_t *)&argv);\n");
		fprintf(incFile, "\t\t}\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tbool SendReturn_%s(", pCxrCall->m_modEntry.c_str());

		pSeparator = "";

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "%s%s %s", pSeparator, pParam->m_hostType.c_str(), pParam->m_name.c_str());

			pSeparator = ", ";
		}

		fprintf(incFile, ") {\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\tint argc = %d;\n", (int)pCxrReturn->m_paramList.size());
		fprintf(incFile, "\t\t\tCRtnArgv_%s argv;\n", pCxrCall->m_modEntry.c_str());
		fprintf(incFile, "\n");

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "\t\t\targv.m_%s = (uint64_t)%s;\n", pParam->m_name.c_str(), pParam->m_name.c_str());
		}
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\t\treturn CHtModelUnitBase::SendReturn(argc, (uint64_t *)&argv);\n");
		fprintf(incFile, "\t\t}\n");
		fprintf(incFile, "\t};\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "#\tifndef HT_MODEL_NO_UNIT_THREADS\n");
		fprintf(incFile, "\tvoid HtCoprocModel(CHtHifLibBase * pHtHifLibBase);\n");
		fprintf(incFile, "#\tendif\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "#\tendif // HT_MODEL\n");
		fprintf(incFile, "} // namespace Ht\n");
		fprintf(incFile, "#endif // _HTV\n");

#else
		fprintf(incFile, "//////////////////////////////////////////////////////\n");
		fprintf(incFile, "// HT_MODEL Interface\n");
		fprintf(incFile, "#ifdef HT_MODEL\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "class CHtModel%sUnit;\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\n");
		fprintf(incFile, "class CHtModelHif : public CHtModelHifBase {\n");
		fprintf(incFile, "public:\n");
		fprintf(incFile, "\tCHtModelHif() : CHtModelHifBase() {}\n");
		fprintf(incFile, "\tvirtual ~CHtModelHif() {}\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tint Get%sUnitCnt() { return GetUnitCnt(); }\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\tCHtModel%sUnit * const * AllocAll%sUnits() { return (CHtModel%sUnit * const *) AllocAllUnits(); }\n",
			m_unitName.Uc().c_str(), m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
		fprintf(incFile, "\tvoid FreeAll%sUnits() { FreeAllUnits(); }\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\tCHtModel%sUnit * Alloc%sUnit() { return (CHtModel%sUnit *) AllocUnit(); }\n",
			m_unitName.Uc().c_str(), m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
		fprintf(incFile, "\tvoid Free%sUnit(CHtModel%sUnit * p%sUnit) { FreeUnit((CHtModelUnitBase *)p%sUnit); }\n",
			m_unitName.Uc().c_str(), m_unitName.Uc().c_str(), m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
		fprintf(incFile, "\n");
		fprintf(incFile, "private:\n");
		fprintf(incFile, "\tvoid InitUnit(int unitId);\n");
		fprintf(incFile, "};\n");
		fprintf(incFile, "\n");

		//*********************

		fprintf(incFile, "class CHtModel%sUnit : public CHtModelUnitBase {\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\tfriend class CHtModelHif;\n");
		fprintf(incFile, "public:\n");
		fprintf(incFile, "\tvoid SetCallback( void (* pCbFuncPtr)(CHtModel%sUnit *, CHtModel%sUnit::ERecvType) ) {\n",
			m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
		fprintf(incFile, "\t\tm_pCbFuncPtr = pCbFuncPtr;\n");
		fprintf(incFile, "\t\tSetUsingCallback(pCbFuncPtr != 0);\n");
		fprintf(incFile, "\t}\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "\tERecvType RecvPoll(bool bUseCbCall=true, bool bUseCbMsg=true, bool bUseCbData=true) {\n");
		fprintf(incFile, "\t\treturn RecvPollBase(bUseCbCall, bUseCbMsg, bUseCbData);\n");
		fprintf(incFile, "\t}\n");
		fprintf(incFile, "\n");

		if (bOutHostMsg) {
			fprintf(incFile, "\tvoid SendHostMsg(uint8_t msgType, uint64_t msgData) {\n");
			fprintf(incFile, "\t\tSendHostMsgBase(msgType, msgData);\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");
		}

		if (bInHostMsg) {
			fprintf(incFile, "\tbool RecvHostMsg(uint8_t &msgType, uint64_t &msgData) {\n");
			fprintf(incFile, "\t\treturn RecvHostMsgBase(msgType, msgData);\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");
		}

		if (bOutHostData) {
			fprintf(incFile, "\tint SendHostData(int qwCnt, uint64_t * pData) {\n");
			fprintf(incFile, "\t\treturn SendHostDataBase(qwCnt, pData);\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\tbool SendHostDataMarker() {\n");
			fprintf(incFile, "\t\treturn SendHostDataMarkerBase();\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\tvoid FlushHostData() {\n");
			fprintf(incFile, "\t\tFlushHostDataBase();\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\tvoid SendHostDataFlush() {\n");
			fprintf(incFile, "\t\tFlushHostDataBase();\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");
		}

		if (bInHostData) {
			fprintf(incFile, "\tbool RecvHostDataPeek(uint64_t & data) {\n");
			fprintf(incFile, "\t\treturn PeekHostDataBase(data);\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\tbool PeekHostData(uint64_t & data) {\n");
			fprintf(incFile, "\t\treturn PeekHostDataBase(data);\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\tint RecvHostData(int qwCnt, uint64_t * pData) {\n");
			fprintf(incFile, "\t\treturn RecvHostDataBase(qwCnt, pData);\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");

			fprintf(incFile, "\tbool RecvHostDataMarker() {\n");
			fprintf(incFile, "\t\treturn RecvHostDataMarkerBase();\n");
			fprintf(incFile, "\t}\n");
			fprintf(incFile, "\n");
		}

		fprintf(incFile, "public:\n");
		fprintf(incFile, "\tstruct CCallArgs_%s {\n", pCxrCall->m_modEntry.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "\t\t%s m_%s;\n", pParam->m_hostType.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\t};\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "private:\n");
		fprintf(incFile, "\tstruct CCallArgv_%s {\n", pCxrCall->m_modEntry.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "\t\tuint64_t m_%s;\n", pParam->m_name.c_str());
		}

		fprintf(incFile, "\t};\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "public:\n");
		fprintf(incFile, "\tbool RecvCall_%s(CCallArgs_%s &args) {\n",
			pCxrCall->m_modEntry.c_str(), pCxrCall->m_modEntry.c_str());

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tint argc = %d;\n", (int)pCxrEntry->m_paramList.size());
		fprintf(incFile, "\t\tCCallArgv_%s argv;\n", pCxrCall->m_modEntry.c_str());
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tif (!RecvCallBase(argc, (uint64_t *)&argv))\n");
		fprintf(incFile, "\t\t\treturn false;\n");
		fprintf(incFile, "\n");

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "\t\targs.m_%s = (%s)argv.m_%s;\n", pParam->m_name.c_str(), pParam->m_hostType.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\treturn true;\n");
		fprintf(incFile, "\t}\n");

		fprintf(incFile, "\n");
		fprintf(incFile, "\tbool RecvCall_%s(", pCxrCall->m_modEntry.c_str());

		pSeparator = "";

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "%s%s &%s", pSeparator, pParam->m_hostType.c_str(), pParam->m_name.c_str());

			pSeparator = ", ";
		}

		fprintf(incFile, ") {\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tint argc = %d;\n", (int)pCxrEntry->m_paramList.size());
		fprintf(incFile, "\t\tCCallArgv_%s argv;\n", pCxrCall->m_modEntry.c_str());
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tif (!RecvCallBase(argc, (uint64_t *)&argv))\n");
		fprintf(incFile, "\t\t\treturn false;\n");
		fprintf(incFile, "\n");

		for (size_t fldIdx = 0; fldIdx < pCxrEntry->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrEntry->m_paramList[fldIdx];

			fprintf(incFile, "\t\t%s = (%s)argv.m_%s;\n", pParam->m_name.c_str(), pParam->m_hostType.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\treturn true;\n");
		fprintf(incFile, "\t}\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "public:\n");
		fprintf(incFile, "\tstruct CRtnArgs_%s {\n", pCxrCall->m_modEntry.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "\t\t%s m_%s;\n", pParam->m_hostType.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\t};\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "private:\n");
		fprintf(incFile, "\tstruct CRtnArgv_%s {\n", pCxrCall->m_modEntry.c_str());

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "\t\tuint64_t m_%s;\n", pParam->m_name.c_str());
		}

		fprintf(incFile, "\t};\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "public:\n");
		fprintf(incFile, "\tbool SendReturn_%s(CRtnArgs_%s &args) {\n",
			pCxrCall->m_modEntry.c_str(), pCxrCall->m_modEntry.c_str());

		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tint argc = %d;\n", (int)pCxrReturn->m_paramList.size());
		fprintf(incFile, "\t\tCRtnArgv_%s argv;\n", pCxrCall->m_modEntry.c_str());
		fprintf(incFile, "\n");

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "\t\targv.m_%s = (uint64_t)args.m_%s;\n", pParam->m_name.c_str(), pParam->m_name.c_str());
		}

		fprintf(incFile, "\t\treturn SendReturnBase(argc, (uint64_t *)&argv);\n");
		fprintf(incFile, "\t}\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\tbool SendReturn_%s(", pCxrCall->m_modEntry.c_str());

		pSeparator = "";

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "%s%s %s", pSeparator, pParam->m_hostType.c_str(), pParam->m_name.c_str());

			pSeparator = ", ";
		}

		fprintf(incFile, ") {\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tint argc = %d;\n", (int)pCxrReturn->m_paramList.size());
		fprintf(incFile, "\t\tCRtnArgv_%s argv;\n", pCxrCall->m_modEntry.c_str());
		fprintf(incFile, "\n");

		for (size_t fldIdx = 0; fldIdx < pCxrReturn->m_paramList.size(); fldIdx += 1) {
			CField * pParam = pCxrReturn->m_paramList[fldIdx];

			fprintf(incFile, "\t\targv.m_%s = (uint64_t)%s;\n", pParam->m_name.c_str(), pParam->m_name.c_str());
		}
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\treturn SendReturnBase(argc, (uint64_t *)&argv);\n");
		fprintf(incFile, "\t}\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "private:\n");
		fprintf(incFile, "\tCHtModel%sUnit(CHtModelHif * pHtModelHif, int unitId) : CHtModelUnitBase(pHtModelHif, unitId) {}\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\tCHtModel%sUnit() : CHtModelUnitBase() {}\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\tvirtual ~CHtModel%sUnit() {}\n", m_unitName.Uc().c_str());
		fprintf(incFile, "\tvoid Callback(CHtModelUnitBase::ERecvType recvType);\n");
		fprintf(incFile, "\tvoid (* m_pCbFuncPtr)(CHtModel%sUnit *, CHtModel%sUnit::ERecvType);\n",
			m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
		fprintf(incFile, "};\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "inline void CHtModel%sUnit::Callback(CHtModelUnitBase::ERecvType recvType)\n", m_unitName.Uc().c_str());
		fprintf(incFile, "{\n");
		fprintf(incFile, "\t(* m_pCbFuncPtr)(this, recvType);\n");
		fprintf(incFile, "}\n");
		fprintf(incFile, "\n");

		fprintf(incFile, "inline void CHtModelHif::InitUnit(int unitId)\n");
		fprintf(incFile, "{\n");
		fprintf(incFile, "\tassert(unitId < GetUnitCnt());\n");
		fprintf(incFile, "\tif (m_ppHtModelUnits[unitId] == 0)\n");
		fprintf(incFile, "\t\tm_ppHtModelUnits[unitId] = new CHtModel%sUnit(this, unitId);\n", m_unitName.Uc().c_str());
		fprintf(incFile, "}\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "#endif // HT_MODEL\n");
		fprintf(incFile, "#endif\n");

#endif

		incFile.FileClose();
	}

	if (!g_appArgs.IsModelOnly()) {
		////////////////////////////////////////////////////////////////
		// Generate cpp file
		string fileName = g_appArgs.GetOutputFolder() + "/PersHif.cpp";

		CHtFile cppFile(fileName, "w");

		GenPersBanner(cppFile, "", "Hif", false);

		fprintf(cppFile, "void\n");
		fprintf(cppFile, "CPersHif::PersHif()\n");
		fprintf(cppFile, "{\n");
		fprintf(cppFile, "\t//\n");
		fprintf(cppFile, "\t// Memory interface\n");
		fprintf(cppFile, "\t//\n");
		fprintf(cppFile, "\tht_attrib(keep, c_hifToMif_reqRdy, \"true\");\n");
		fprintf(cppFile, "\tc_hifToMif_reqRdy = false;\n");
		fprintf(cppFile, "\tCMemRdWrReqIntf c_hifToMif_req;\n");
		fprintf(cppFile, "\tc_hifToMif_req.m_host = 1;\n");
		fprintf(cppFile, "\tc_hifToMif_req.m_tid  = 0;\n");
		fprintf(cppFile, "\tc_hifToMif_req.m_type = 0;\n");
		fprintf(cppFile, "\tc_hifToMif_req.m_addr = 0;\n");
		fprintf(cppFile, "\tc_hifToMif_req.m_data = 0;\n");
		fprintf(cppFile, "\tc_hifToMif_req.m_size = MEM_REQ_U64;\n");
		fprintf(cppFile, "#ifndef _HTV\n");
		fprintf(cppFile, "\tc_hifToMif_req.m_line = 0;\n");
		fprintf(cppFile, "\tc_hifToMif_req.m_file = (char *)(char *)__FILE__;\n");
		fprintf(cppFile, "\tc_hifToMif_req.m_time = sc_time_stamp().value();\n");
		fprintf(cppFile, "\tc_hifToMif_req.m_orderChk = true;\n");
		fprintf(cppFile, "#endif\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tbool c_mifBusy = r_hifToMif_reqAvlCnt < 2;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tsc_uint<MEM_ADDR_W> c_CtlQueBase = i_dispToHif_ctlQueBase.read()\n");
		fprintf(cppFile, "\t\t\t\t\t+ (i_aeUnitId.read()\n");
		fprintf(cppFile, "\t\t\t\t\t* sizeof(uint64_t)\n");
		fprintf(cppFile, "\t\t\t\t\t* 2 // In/Out\n");
		fprintf(cppFile, "\t\t\t\t\t* HIF_CTL_QUE_CNT);\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t//\n");
		fprintf(cppFile, "\t// Inbound control messages\n");
		fprintf(cppFile, "\t//\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_CTL_TIME_W> c_ctlTimeArg = r_ctlTimeArg;\n");
		fprintf(cppFile, "\tsc_uint<MEM_ADDR_W> c_iBlkAdrArg = r_iBlkAdrArg;\n");
		fprintf(cppFile, "\tsc_uint<MEM_ADDR_W> c_oBlkAdrArg = r_oBlkAdrArg;\n");
		fprintf(cppFile, "\tsc_uint<MEM_ADDR_W> c_oBlkFlshAdrArg = r_oBlkFlshAdrArg;\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_IBLK_SIZE_W> c_iBlkSizeArg = r_iBlkSizeArg;\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_IBLK_CNT_W> c_iBlkCntArg = r_iBlkCntArg;\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_OBLK_SIZE_W> c_oBlkSizeArg = r_oBlkSizeArg;\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_OBLK_CNT_W> c_oBlkCntArg = r_oBlkCntArg;\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_OBLK_TIME_W> c_oBlkTimeArg = r_oBlkTimeArg;\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_OBLK_CNT_W+1> c_oBlkAvlCnt = r_oBlkAvlCnt;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tbool c_oBlkFlshEn = r_oBlkFlshAdrArg != 0;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tbool c_hifToHti_ctlRdy = false;\n");
		fprintf(cppFile, "\tCHostCtrlMsgIntf c_hifToHti_ctl;\n");
		fprintf(cppFile, "\tc_hifToHti_ctl.m_data64 = 0;\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\tif (r_iCtlMsg.m_bValid) {\n");
		fprintf(cppFile, "\t\tif (r_iCtlMsg.m_msgType > 15) {\n");
		fprintf(cppFile, "\t\t\tc_hifToHti_ctlRdy = true;\n");
		fprintf(cppFile, "\t\t\tc_hifToHti_ctl = r_iCtlMsg;\n");
		fprintf(cppFile, "\t\t} else {\n");
		fprintf(cppFile, "\t\t\tuint64_t iCtlData = (uint64_t)r_iCtlMsg.m_msgData;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\t\tswitch (r_iCtlMsg.m_msgType) {\n");
		fprintf(cppFile, "\t\t\tcase HIF_CMD_CTL_PARAM:\n");
		fprintf(cppFile, "\t\t\t\tc_ctlTimeArg = (sc_uint<HIF_ARG_CTL_TIME_W>)iCtlData;\n");
		fprintf(cppFile, "\t\t\t\tbreak;\n");
		fprintf(cppFile, "\t\t\tcase HIF_CMD_IBLK_ADR:\n");
		fprintf(cppFile, "\t\t\t\tc_iBlkAdrArg = (sc_uint<MEM_ADDR_W>)iCtlData;\n");
		fprintf(cppFile, "\t\t\t\tbreak;\n");
		fprintf(cppFile, "\t\t\tcase HIF_CMD_OBLK_ADR:\n");
		fprintf(cppFile, "\t\t\t\tc_oBlkAdrArg = (sc_uint<MEM_ADDR_W>)iCtlData;\n");
		fprintf(cppFile, "\t\t\t\tbreak;\n");
		fprintf(cppFile, "\t\t\tcase HIF_CMD_OBLK_FLSH_ADR:\n");
		fprintf(cppFile, "\t\t\t\tc_oBlkFlshAdrArg = (sc_uint<MEM_ADDR_W>)iCtlData;\n");
		fprintf(cppFile, "\t\t\t\tbreak;\n");
		fprintf(cppFile, "\t\t\tcase HIF_CMD_IBLK_PARAM:\n");
		fprintf(cppFile, "\t\t\t\tc_iBlkSizeArg = (sc_uint<HIF_ARG_IBLK_SIZE_W>)iCtlData;\n");
		fprintf(cppFile, "\t\t\t\tc_iBlkCntArg  = (sc_uint<HIF_ARG_IBLK_CNT_W>)(iCtlData >> 8);\n");
		fprintf(cppFile, "\t\t\t\tbreak;\n");
		fprintf(cppFile, "\t\t\tcase HIF_CMD_OBLK_PARAM:\n");
		fprintf(cppFile, "\t\t\t\tc_oBlkSizeArg = (sc_uint<HIF_ARG_OBLK_SIZE_W>)iCtlData;\n");
		if (b64B) {
			fprintf(cppFile, "\t\t\t\tassert(c_oBlkSizeArg > 5);\n");
		} else {
			fprintf(cppFile, "\t\t\t\tassert(c_oBlkSizeArg > 2);\n");
		}
		fprintf(cppFile, "\t\t\t\tc_oBlkCntArg  = (sc_uint<HIF_ARG_OBLK_CNT_W>)(iCtlData >> 8);\n");
		fprintf(cppFile, "\t\t\t\tc_oBlkTimeArg = (sc_uint<HIF_ARG_OBLK_TIME_W>)(iCtlData >> 16);\n");
		fprintf(cppFile, "\t\t\t\tbreak;\n");
		fprintf(cppFile, "\t\t\tcase HIF_CMD_IBLK_RDY:\n");
		fprintf(cppFile, "\t\t\t\tm_iBlkRdyQue.push((sc_uint<HIF_ARG_IBLK_SMAX_W+1>)iCtlData);\n");
		fprintf(cppFile, "\t\t\t\tbreak;\n");
		fprintf(cppFile, "\t\t\tcase HIF_CMD_OBLK_AVL:\n");
		fprintf(cppFile, "\t\t\t\tc_oBlkAvlCnt += 1;\n");
		fprintf(cppFile, "\t\t\t\tbreak;\n");
		fprintf(cppFile, "\t\t\tdefault:\n");
		fprintf(cppFile, "\t\t\t\tassert(0);\n");
		fprintf(cppFile, "\t\t\t}\n");
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t// ICM state machine\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_CTL_TIME_W> c_iCtlTimer;\n");
		fprintf(cppFile, "\tc_iCtlTimer = r_iCtlTimer > 0 ?\n");
		fprintf(cppFile, "\t\t(sc_uint<HIF_ARG_CTL_TIME_W>)(r_iCtlTimer-1) :\n");
		fprintf(cppFile, "\t\tr_iCtlTimer;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tbool c_iCtlTimerWait = r_iCtlTimer > 0;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tsc_uint<2> c_iCtlState = r_iCtlState;\n");
		fprintf(cppFile, "\tbool c_iCtlRdWait = r_iCtlRdWait;\n");
		fprintf(cppFile, "\tbool c_iCtlWrPend = r_iCtlWrPend;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tsc_uint<HIF_CTL_QUE_CNT_W> c_iCtlRdIdx = r_iCtlRdIdx;\n");
		fprintf(cppFile, "\tsc_uint<MEM_ADDR_W> c_iCtlQueAdr = r_CtlQueBase\n");
		fprintf(cppFile, "\t\t+ (r_iCtlRdIdx * sizeof(uint64_t));\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_OBLK_CNT_W+1> c_oBlkNum;\n");
		fprintf(cppFile, "\tc_oBlkNum = (sc_uint<HIF_ARG_OBLK_CNT_W+1>)(1 << r_oBlkCntArg);\n");
		fprintf(cppFile, "\tbool c_oBlkCntIdle = r_oBlkAvlCnt == c_oBlkNum;\n");
		fprintf(cppFile, "\tbool c_oBlkCntHalt = r_oBlkAvlCnt >= (c_oBlkNum - 1);\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tswitch (r_iCtlState) {\n");
		fprintf(cppFile, "\tcase ICM_READ:\n");
		fprintf(cppFile, "\t\tc_iCtlRdWait = false;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (r_hifToHti_datHalt && c_oBlkCntIdle)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (r_iCtlTimerWait || r_hifToHti_ctlAvlCnt == 0)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (c_mifBusy || r_mifReqSelRr != 0)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tassert(!c_hifToMif_reqRdy);\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_hifToMif_reqRdy = true;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_type = MEM_REQ_RD;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_addr = c_iCtlQueAdr;\n");
		if (true || g_appArgs.IsMemTraceEnabled()) {
			fprintf(cppFile, "#ifndef _HTV\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_line = __LINE__;\n");
			fprintf(cppFile, "#endif\n");
		}
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_iCtlRdWait = true;\n");
		fprintf(cppFile, "\t\tc_iCtlState = ICM_RD_WAIT;\n");
		fprintf(cppFile, "\tbreak;\n");
		fprintf(cppFile, "\tcase ICM_RD_WAIT:\n");
		fprintf(cppFile, "\t\tif (r_iCtlRdWait)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (!r_iCtlMsg.m_bValid) {\n");
		fprintf(cppFile, "\t\t\t// rate limit when empty\n");
		fprintf(cppFile, "\t\t\tc_iCtlTimer = r_ctlTimeArg;\n");
		fprintf(cppFile, "\t\t\tc_iCtlState = ICM_READ;\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_iCtlState = ICM_WRITE;\n");
		fprintf(cppFile, "\tbreak;\n");
		fprintf(cppFile, "\tcase ICM_WRITE:\n");
		fprintf(cppFile, "\t\tif (r_iCtlWrPend)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (c_mifBusy || r_mifReqSelRr != 0)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tassert(!c_hifToMif_reqRdy);\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_hifToMif_reqRdy = true;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_type = MEM_REQ_WR;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_addr = c_iCtlQueAdr;\n");
		if (true || g_appArgs.IsMemTraceEnabled()) {
			fprintf(cppFile, "#ifndef _HTV\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_line = __LINE__;\n");
			fprintf(cppFile, "#endif\n");
		}
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_iCtlWrPend = true;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_iCtlRdIdx = (sc_uint<HIF_CTL_QUE_CNT_W>)(r_iCtlRdIdx + 1);\n");
		fprintf(cppFile, "\t\tc_iCtlState = ICM_READ;\n");
		fprintf(cppFile, "\tbreak;\n");
		fprintf(cppFile, "\tdefault:\n");
		fprintf(cppFile, "\t\tassert(0);\n");
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t//\n");
		fprintf(cppFile, "\t// Outbound control messages\n");
		fprintf(cppFile, "\t//\n");
		fprintf(cppFile, "\tCHostCtrlMsgIntf c_htiToHif_ctl = r_htiToHif_ctl;\n");
		fprintf(cppFile, "\tif (r_reset1x)\n");
		fprintf(cppFile, "\t\tc_htiToHif_ctl.m_bValid = false;\n");

		fprintf(cppFile, "\tif (i_htiToHif_ctlRdy.read())\n");
		fprintf(cppFile, "\t\tc_htiToHif_ctl = i_htiToHif_ctl.read();\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_CTL_TIME_W> c_oCtlTimer;\n");
		fprintf(cppFile, "\tc_oCtlTimer = r_oCtlTimer > 0 ?\n");
		fprintf(cppFile, "\t\t(sc_uint<HIF_ARG_CTL_TIME_W>)(r_oCtlTimer-1) :\n");
		fprintf(cppFile, "\t\tr_oCtlTimer;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tbool c_oCtlTimerWait = r_oCtlTimer > 0;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tsc_uint<2> c_oCtlState = r_oCtlState;\n");
		fprintf(cppFile, "\tbool c_oCtlRdWait = r_oCtlRdWait;\n");
		fprintf(cppFile, "\tbool c_oCtlWrPend = r_oCtlWrPend;\n");
		fprintf(cppFile, "\tbool c_oCtlMsgSent = false;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tsc_uint<HIF_CTL_QUE_CNT_W> c_oCtlWrIdx = r_oCtlWrIdx;\n");
		fprintf(cppFile, "\tsc_uint<MEM_ADDR_W> c_oCtlQueAdr = r_CtlQueBase\n");
		fprintf(cppFile, "\t\t\t\t\t+ (HIF_CTL_QUE_CNT * sizeof(uint64_t))\n");
		fprintf(cppFile, "\t\t\t\t\t+ (r_oCtlWrIdx * sizeof(uint64_t));\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tswitch (r_oCtlState) {\n");
		fprintf(cppFile, "\tcase OCM_READ:\n");
		fprintf(cppFile, "\t\t// check for open slot\n");
		fprintf(cppFile, "\t\tc_oCtlRdWait = false;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (r_htiToHif_datHalt && !r_oCtlWrMsg.m_bValid)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (r_oCtlTimerWait || c_mifBusy || r_mifReqSelRr != 1)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tassert(!c_hifToMif_reqRdy);\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_hifToMif_reqRdy = true;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_type = MEM_REQ_RD;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_addr = c_oCtlQueAdr;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_tid = 1;\n");
		if (true || g_appArgs.IsMemTraceEnabled()) {
			fprintf(cppFile, "#ifndef _HTV\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_line = __LINE__;\n");
			fprintf(cppFile, "#endif\n");
		}
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_oCtlRdWait = true;\n");
		fprintf(cppFile, "\t\tc_oCtlState = OCM_RD_WAIT;\n");
		fprintf(cppFile, "\tbreak;\n");
		fprintf(cppFile, "\tcase OCM_RD_WAIT:\n");
		fprintf(cppFile, "\t\tif (r_oCtlRdWait)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (r_oCtlRdMsg.m_bValid) {\n");
		fprintf(cppFile, "\t\t\t// rate limit when full\n");
		fprintf(cppFile, "\t\t\tc_oCtlTimer = r_ctlTimeArg;\n");
		fprintf(cppFile, "\t\t\tc_oCtlState = OCM_READ;\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_oCtlState = OCM_WRITE;\n");
		fprintf(cppFile, "\tbreak;\n");
		fprintf(cppFile, "\tcase OCM_WRITE:\n");
		fprintf(cppFile, "\t\tif (r_oCtlWrPend)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (!r_oCtlWrMsg.m_bValid)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (c_mifBusy || r_mifReqSelRr != 1)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tassert(!c_hifToMif_reqRdy);\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_hifToMif_reqRdy = true;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_type = MEM_REQ_WR;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_addr = c_oCtlQueAdr;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_tid = 1;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_data = r_oCtlWrMsg.m_data64;\n");
		if (true || g_appArgs.IsMemTraceEnabled()) {
			fprintf(cppFile, "#ifndef _HTV\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_line = __LINE__;\n");
			fprintf(cppFile, "#endif\n");
		}
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_oCtlWrIdx = (sc_uint<HIF_CTL_QUE_CNT_W>)(r_oCtlWrIdx + 1);\n");
		fprintf(cppFile, "\t\tc_oCtlState = OCM_READ;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_oCtlWrPend = true;\n");
		fprintf(cppFile, "\t\tc_oCtlMsgSent = true;\n");
		fprintf(cppFile, "\tbreak;\n");
		fprintf(cppFile, "\tdefault:\n");
		fprintf(cppFile, "\t\tassert(0);\n");
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t// Select outgoing control message\n");
		fprintf(cppFile, "\tCHostCtrlMsgIntf c_oCtlWrMsg = r_oCtlWrMsg;\n");
		fprintf(cppFile, "\tbool c_oBlkSendRdy = r_oBlkSendRdy;\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_OBLK_SMAX_W+1> c_oBlkSize = r_oBlkSize;\n");
		fprintf(cppFile, "\tht_uint1 c_oBlkSendIdx = r_oBlkSendIdx;\n");
		fprintf(cppFile, "\tbool c_iBlkSendAvl = r_iBlkSendAvl;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tif (r_reset1x || c_oCtlMsgSent) {\n");
		fprintf(cppFile, "\t\tc_oCtlWrMsg.m_bValid  = false;\n");
		fprintf(cppFile, "\t\tc_oCtlWrMsg.m_msgType = 0;\n");
		fprintf(cppFile, "\t\tc_oCtlWrMsg.m_msgData = 0;\n");
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tCHostDataQueIntf c_oBlkQueDat = m_oBlkQue.front();\n");
		fprintf(cppFile, "\tbool c_htiToHif_datHalt = r_htiToHif_datHalt;\n");
		fprintf(cppFile, "\tbool c_hifToHti_datAvl = false;\n");
		fprintf(cppFile, "\tbool c_hifToHti_ctlAvl = false;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tCHtAssertIntf c_htaToHif_assert = r_htaToHif_assert;\n");
		fprintf(cppFile, "\tif (i_htaToHif_assert.read().m_bAssert) {\n");
		fprintf(cppFile, "\t\tc_htaToHif_assert = i_htaToHif_assert.read();\n");
		fprintf(cppFile, "\t\tif (r_htaToHif_assert.m_bAssert)\n");
		fprintf(cppFile, "\t\t\tc_htaToHif_assert.m_bCollision = true;\n");
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tif (!c_oCtlWrMsg.m_bValid) {\n");
		fprintf(cppFile, "\t\tif (r_htaToHif_assert.m_bAssert) {\n");
		fprintf(cppFile, "\t\t\tc_oCtlWrMsg.m_bValid  = true;\n");
		fprintf(cppFile, "\t\t\tif (r_htaToHif_assert.m_bCollision) {\n");
		fprintf(cppFile, "\t\t\t\tc_oCtlWrMsg.m_msgType = HIF_CMD_ASSERT_COLL;\n");
		fprintf(cppFile, "\t\t\t\tc_oCtlWrMsg.m_msgData = 0;\n");
		fprintf(cppFile, "\t\t\t} else {\n");
		fprintf(cppFile, "\t\t\t\tc_oCtlWrMsg.m_msgType = HIF_CMD_ASSERT;\n");
		fprintf(cppFile, "\t\t\t\tc_oCtlWrMsg.m_msgData = (sc_uint<56>)(\n");
		fprintf(cppFile, "\t\t\t\t\t((uint64_t)r_htaToHif_assert.m_module  << 48) |\n");
		fprintf(cppFile, "\t\t\t\t\t((uint64_t)r_htaToHif_assert.m_lineNum << 32) |\n");
		fprintf(cppFile, "\t\t\t\t\t((uint64_t)r_htaToHif_assert.m_info));\n");
		fprintf(cppFile, "\t\t\t}\n");
		fprintf(cppFile, "\t\t\tc_htaToHif_assert.m_bAssert = false;\n");
		fprintf(cppFile, "\t\t} else if (r_oBlkSendRdy &&\n");
		fprintf(cppFile, "\t\t\tr_oBlkWrPendCnt[INT(r_oBlkSendIdx)] == 0 &&\n");
		fprintf(cppFile, "\t\t\t(r_oBlkRdPendCnt[INT(r_oBlkSendIdx)] == 0 || !r_oBlkFlshEn))\n");
		fprintf(cppFile, "\t\t{\n");
		fprintf(cppFile, "\t\t\tc_oCtlWrMsg.m_bValid  = true;\n");
		fprintf(cppFile, "\t\t\tc_oCtlWrMsg.m_msgType = HIF_CMD_OBLK_RDY;\n");
		fprintf(cppFile, "\t\t\tc_oCtlWrMsg.m_msgData = (sc_uint<56>)r_oBlkSize;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\t\tc_oBlkSendRdy = false;\n");
		fprintf(cppFile, "\t\t} else if (r_iBlkSendAvl) {\n");
		fprintf(cppFile, "\t\t\tc_oCtlWrMsg.m_bValid  = true;\n");
		fprintf(cppFile, "\t\t\tc_oCtlWrMsg.m_msgType = HIF_CMD_IBLK_AVL;\n");
		fprintf(cppFile, "\t\t\tc_oCtlWrMsg.m_msgData = 0;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\t\tc_iBlkSendAvl = false;\n");
		fprintf(cppFile, "\t\t} else if (r_htiToHif_ctl.m_bValid) {\n");
		fprintf(cppFile, "\t\t\tc_oCtlWrMsg = r_htiToHif_ctl;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\t\tc_hifToHti_ctlAvl = true;\n");
		fprintf(cppFile, "\t\t\tc_htiToHif_ctl.m_bValid = false;\n");
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t//\n");
		fprintf(cppFile, "\t// Inbound blocks\n");
		fprintf(cppFile, "\t//\n");
		fprintf(cppFile, "\tsc_uint<2> c_iBlkState = r_iBlkState;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_IBLK_SMAX_W-2> c_iBlkRdCnt = r_iBlkRdCnt;\n");
		fprintf(cppFile, "\tsc_uint<MEM_ADDR_W> c_iBlkRdAdr = r_iBlkRdAdr;\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_IBLK_CNT_W> c_iBlkRdIdx = r_iBlkRdIdx;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tsc_uint<4> c_iBlkFetchCnt[IBK_FETCH_CNT];\n");
		fprintf(cppFile, "\tsc_uint<4> c_iBlkFetchVal[IBK_FETCH_CNT];\n");
		fprintf(cppFile, "\tbool c_iBlkActive = false;\n");
		fprintf(cppFile, "\tfor (unsigned i=0; i<IBK_FETCH_CNT; i+=1) {\n");
		fprintf(cppFile, "\t\tc_iBlkFetchCnt[i] = r_iBlkFetchCnt[i];\n");
		fprintf(cppFile, "\t\tc_iBlkFetchVal[i] = r_iBlkFetchVal[i];\n");
		fprintf(cppFile, "\t\tc_iBlkActive |= r_iBlkFetchCnt[i] || r_iBlkFetchVal[i];\n");
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\tsc_uint<IBK_FETCH_CNT_W> c_iBlkFetchIdx = r_iBlkFetchIdx;\n");
		fprintf(cppFile, "\tbool c_iBlkRdBusy = r_iBlkFetchVal[r_iBlkFetchIdx] != 0;\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\tbool c_hifToHti_datRdy = false;\n");
		fprintf(cppFile, "\tCHostDataQueIntf c_hifToHti_dat;\n");
		fprintf(cppFile, "\tc_hifToHti_dat.m_data = 0;\n");
		fprintf(cppFile, "\tc_hifToHti_dat.m_ctl = HT_DQ_DATA;\n");
		fprintf(cppFile, "\n");


		fprintf(cppFile, "\tswitch (r_iBlkState) {\n");
		fprintf(cppFile, "\tcase IBK_IDLE:\n");
		fprintf(cppFile, "\t\tassert(m_iBlkRdyQue.empty() || !r_hifToHti_datHalt);\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\t\tif (m_iBlkRdyQue.empty() || r_iBlkSendAvl)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\t\tc_hifToHti_dat.m_data = (uint64_t)(m_iBlkRdyQue.front() >> 3);\n");
		fprintf(cppFile, "\t\tc_hifToHti_dat.m_ctl = m_iBlkRdyQue.front() %% 8u;\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\t\tc_iBlkRdIdx = (r_iBlkRdIdx + 1) & ((1 << r_iBlkCntArg)-1);\n");
		fprintf(cppFile, "\t\tc_iBlkRdCnt = (sc_uint<HIF_ARG_IBLK_SMAX_W-2>)\n");
		fprintf(cppFile, "\t\t\t(m_iBlkRdyQue.front()/8);\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_iBlkRdAdr = r_iBlkAdrArg + (r_iBlkRdIdx << r_iBlkSizeArg);\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\t\tif (m_iBlkRdyQue.front() %% 8u) {\n");
		fprintf(cppFile, "\t\t\t// special block\n");
		fprintf(cppFile, "\t\t\tc_hifToHti_datRdy = true;\n");
		fprintf(cppFile, "\t\t\tc_iBlkSendAvl = true;\n");
		fprintf(cppFile, "\t\t} else\n");
		fprintf(cppFile, "\t\t\tc_iBlkState = IBK_READ;\n");
		fprintf(cppFile, "\n");

		fprintf(cppFile, "\t\tm_iBlkRdyQue.pop();\n");
		fprintf(cppFile, "\tbreak;\n");
		fprintf(cppFile, "\tcase IBK_READ: {\n");
		fprintf(cppFile, "\t\tif (r_iBlkRdCnt == 0) {\n");
		fprintf(cppFile, "\t\t\tc_iBlkState = IBK_WAIT;\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (c_iBlkRdBusy)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (c_mifBusy || r_mifReqSelRr != 2)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tassert(!c_hifToMif_reqRdy);\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_hifToMif_reqRdy = true;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_type = MEM_REQ_RD;\n");
		if (b64B) {
			fprintf(cppFile, "\t\tc_hifToMif_req.m_addr = (sc_uint<48>)((r_iBlkRdAdr >> 6) << 6);\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_tid  = (sc_uint<MIF_TID_W>)(7 << MIF_TID_QWCNT_SHF);\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_tid |= (sc_uint<MIF_TID_W>)(r_iBlkFetchIdx << HIF_TID_TYPE_W);\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_tid |= 2;\n");
			fprintf(cppFile, "#ifndef _HTV\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_line = __LINE__;\n");
			fprintf(cppFile, "#endif\n");
			fprintf(cppFile, "\n");
			fprintf(cppFile, "\t\tc_iBlkFetchIdx += 1;\n");
			fprintf(cppFile, "\t\tc_iBlkFetchCnt[r_iBlkFetchIdx] = 8;\n");
			fprintf(cppFile, "\t\tif (r_iBlkRdCnt < 8) {\n");
			fprintf(cppFile, "\t\t\tc_iBlkFetchVal[r_iBlkFetchIdx] = (sc_uint<4>)r_iBlkRdCnt;\n");
			fprintf(cppFile, "\t\t\tc_iBlkRdCnt = 0;\n");
			fprintf(cppFile, "\t\t} else {\n");
			fprintf(cppFile, "\t\t\tc_iBlkFetchVal[r_iBlkFetchIdx] = 8;\n");
			fprintf(cppFile, "\t\t\tc_iBlkRdCnt -= 8;\n");
			fprintf(cppFile, "\t\t}\n");
			fprintf(cppFile, "\n");
			fprintf(cppFile, "\t\tc_iBlkRdAdr = c_hifToMif_req.m_addr + 64;\n");
		} else {
			fprintf(cppFile, "\t\tc_hifToMif_req.m_addr = (sc_uint<48>)((r_iBlkRdAdr >> 3) << 3);\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_tid  = (sc_uint<MIF_TID_W>)(7 << MIF_TID_QWCNT_SHF);\n");
			fprintf(cppFile, "\t\tuint32_t rdQw = (r_iBlkRdAdr >> 3) & 7;\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_tid |= (sc_uint<MIF_TID_W>)(rdQw << (IBK_FETCH_CNT_W+HIF_TID_TYPE_W));\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_tid |= (sc_uint<MIF_TID_W>)(r_iBlkFetchIdx << HIF_TID_TYPE_W);\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_tid |= 2;\n");
			fprintf(cppFile, "#ifndef _HTV\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_line = __LINE__;\n");
			fprintf(cppFile, "#endif\n");
			fprintf(cppFile, "\n");
			fprintf(cppFile, "\t\tc_iBlkFetchCnt[r_iBlkFetchIdx] += 1;\n");
			fprintf(cppFile, "\t\tif (r_iBlkRdCnt == 1) {\n");
			fprintf(cppFile, "\t\t\tc_iBlkFetchVal[r_iBlkFetchIdx] = (sc_uint<4>)(rdQw + 1);\n");
			fprintf(cppFile, "\t\t\tc_iBlkRdCnt = 0;\n");
			fprintf(cppFile, "\t\t\tc_iBlkFetchIdx += 1;\n");
			fprintf(cppFile, "\t\t} else {\n");
			fprintf(cppFile, "\t\t\tif (rdQw == 7) {\n");
			fprintf(cppFile, "\t\t\t\tc_iBlkFetchVal[r_iBlkFetchIdx] = 8;\n");
			fprintf(cppFile, "\t\t\t\tc_iBlkFetchIdx += 1;\n");
			fprintf(cppFile, "\t\t\t}\n");
			fprintf(cppFile, "\t\t\tc_iBlkRdCnt -= 1;\n");
			fprintf(cppFile, "\t\t}\n");
			fprintf(cppFile, "\n");
			fprintf(cppFile, "\t\tc_iBlkRdAdr = c_hifToMif_req.m_addr + 8;\n");
		}
		fprintf(cppFile, "\t} break;\n");
		fprintf(cppFile, "\tcase IBK_WAIT:\n");
		fprintf(cppFile, "\t\tif (!c_iBlkActive) {\n");
		fprintf(cppFile, "\t\t\tc_iBlkState = IBK_IDLE;\n");
		fprintf(cppFile, "\t\t\tc_iBlkSendAvl = true;\n");
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\tbreak;\n");
		fprintf(cppFile, "\tdefault:\n");
		fprintf(cppFile, "\t\tassert(0);\n");
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t//\n");
		fprintf(cppFile, "\t// Outbound blocks\n");
		fprintf(cppFile, "\t//\n");
		fprintf(cppFile, "\tif (i_htiToHif_datRdy.read())\n");
		fprintf(cppFile, "\t\tm_oBlkQue.push(i_htiToHif_dat);\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tsc_uint<3> c_oBlkState = r_oBlkState;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_OBLK_SMAX_W-2> c_oBlkWrCnt = r_oBlkWrCnt;\n");
		fprintf(cppFile, "\tsc_uint<4> c_oBlkRdCnt = r_oBlkRdCnt;\n");
		fprintf(cppFile, "\tsc_uint<MEM_ADDR_W> c_oBlkWrAdr = r_oBlkWrAdr;\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_OBLK_CNT_W> c_oBlkWrIdx = r_oBlkWrIdx;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_OBLK_SMAX_W-2> c_oBlkWrCntEnd;\n");
		fprintf(cppFile, "\tc_oBlkWrCntEnd = (sc_uint<HIF_ARG_OBLK_SMAX_W-2>)\n");
		fprintf(cppFile, "\t\t(((1 << r_oBlkSizeArg)/8)-1);\n");
		if (b64B) {
			fprintf(cppFile, "\tsc_uint<HIF_ARG_OBLK_SMAX_W-2> c_oBlkWrCntEnd64;\n");
			fprintf(cppFile, "\tc_oBlkWrCntEnd64 = (sc_uint<HIF_ARG_OBLK_SMAX_W-2>) r_oBlkWrCntEnd - 7;\n");
		}
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tsc_uint<8> c_oBlkWrPendCnt[2];\n");
		fprintf(cppFile, "\tsc_uint<4> c_oBlkRdPendCnt[2];\n");
		fprintf(cppFile, "\tht_uint1 c_oBlkWrTid = r_oBlkWrIdx & 1;\n");
		fprintf(cppFile, "\tbool c_oBlkWrPendFull = r_oBlkWrPendCnt[INT(c_oBlkWrTid)] > (1<<8)-16;\n");
		fprintf(cppFile, "\tbool c_oBlkWrPendBusy = r_oBlkWrPendCnt[INT(c_oBlkWrTid)] != 0;\n");
		fprintf(cppFile, "\tbool c_oBlkRdPendBusy = r_oBlkRdPendCnt[INT(c_oBlkWrTid)] != 0;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tfor (int i=0; i<2; i+=1) {\n");
		fprintf(cppFile, "\t\tc_oBlkWrPendCnt[i] = r_oBlkWrPendCnt[i];\n");
		fprintf(cppFile, "\t\tc_oBlkRdPendCnt[i] = r_oBlkRdPendCnt[i];\n");
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tsc_uint<HIF_ARG_OBLK_TIME_W> c_oBlkTimer;\n");
		fprintf(cppFile, "\tc_oBlkTimer = r_oBlkTimer > 0 ?\n");
		fprintf(cppFile, "\t\t(sc_uint<HIF_ARG_OBLK_TIME_W>)(r_oBlkTimer-1) :\n");
		fprintf(cppFile, "\t\tr_oBlkTimer;\n");
		fprintf(cppFile, "\tbool c_oBlkExpired = r_oBlkTimer == 0 && r_oBlkTimeArg != 0;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tswitch (r_oBlkState) {\n");
		fprintf(cppFile, "\tcase OBK_IDLE:\n");
		fprintf(cppFile, "\t\tc_oBlkTimer = r_oBlkTimeArg;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (!m_oBlkQue.empty() && c_oBlkQueDat.m_ctl == HT_DQ_FLSH &&\n");
		fprintf(cppFile, "\t\t\tm_oBlkWrDat.empty())\n");
		fprintf(cppFile, "\t\t{\n");
		fprintf(cppFile, "\t\t\tc_hifToHti_datAvl = true;\n");
		fprintf(cppFile, "\t\t\tm_oBlkQue.pop();\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (r_oBlkAvlCnt == 0)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (c_oBlkWrPendBusy || (c_oBlkRdPendBusy && r_oBlkFlshEn))\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (m_oBlkWrDat.empty() && m_oBlkQue.empty())\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (!m_oBlkWrDat.empty() ||\n");
		fprintf(cppFile, "\t\t\t(!m_oBlkQue.empty() &&\n");
		fprintf(cppFile, "\t\t\t\tc_oBlkQueDat.m_ctl == HT_DQ_DATA))\n");
		fprintf(cppFile, "\t\t\tc_oBlkState = OBK_POP;\n");
		fprintf(cppFile, "\t\telse\n");
		fprintf(cppFile, "\t\t\tc_oBlkState = OBK_SEND;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_oBlkAvlCnt -= 1;\n");
		fprintf(cppFile, "\t\tc_oBlkWrAdr = r_oBlkAdrArg + (r_oBlkWrIdx << r_oBlkSizeArg);\n");
		fprintf(cppFile, "\t\tc_oBlkRdCnt = 8;\n");
		fprintf(cppFile, "\tbreak;\n");
		fprintf(cppFile, "\tcase OBK_POP: {\n");
		fprintf(cppFile, "\t\tbool c_oBlkDirty = r_oBlkWrCnt != 0 || !m_oBlkWrDat.empty();\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (c_oBlkDirty && r_oBlkExpired) {\n");
		fprintf(cppFile, "\t\t\tc_oBlkState = m_oBlkWrDat.empty() ? OBK_FLSH : OBK_WR8;\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (m_oBlkQue.empty())\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (c_oBlkQueDat.m_ctl != HT_DQ_DATA) {\n");
		fprintf(cppFile, "\t\t\tc_oBlkState = m_oBlkWrDat.empty() ? OBK_FLSH : OBK_WR8;\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tm_oBlkWrDat.push(c_oBlkQueDat.m_data);\n");
		fprintf(cppFile, "\t\tm_oBlkQue.pop();\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (m_oBlkWrDat.size() == 7) {\n");
		if (b64B) {
			fprintf(cppFile, "\t\t\tif (r_oBlkWrCnt <= r_oBlkWrCntEnd64 &&\n");
			fprintf(cppFile, "\t\t\t\t!c_oBlkWrPendFull &&\n");
			fprintf(cppFile, "\t\t\t\t((r_oBlkWrAdr >> 3) & 0x7) == 0)\n");
			fprintf(cppFile, "\t\t\t\t\tc_oBlkState = OBK_WR64;\n");
			fprintf(cppFile, "\t\t\telse\n");
			fprintf(cppFile, "\t\t\t\tc_oBlkState = OBK_WR8;\n");
		} else
			fprintf(cppFile, "\t\t\tc_oBlkState = OBK_WR8;\n");
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\t} break;\n");
		fprintf(cppFile, "\tcase OBK_WR8:\n");
		fprintf(cppFile, "\t\tif (c_mifBusy || r_mifReqSelRr != 3 || c_oBlkWrPendFull)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tassert(!c_hifToMif_reqRdy);\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_hifToMif_reqRdy = true;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_type = MEM_REQ_WR;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_addr = r_oBlkWrAdr;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_tid = (sc_uint<MIF_TID_W>)(c_oBlkWrTid << HIF_TID_TYPE_W);\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_tid |= 3;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_data = m_oBlkWrDat.front();\n");
		fprintf(cppFile, "#ifndef _HTV\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_line = __LINE__;\n");
		fprintf(cppFile, "#endif\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_hifToHti_datAvl = true;\n");
		fprintf(cppFile, "\t\tm_oBlkWrDat.pop();\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_oBlkWrAdr = r_oBlkWrAdr + 8;\n");
		fprintf(cppFile, "\t\tc_oBlkWrCnt = r_oBlkWrCnt + 1;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_oBlkWrPendCnt[INT(c_oBlkWrTid)] += 1;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (r_oBlkWrCnt == r_oBlkWrCntEnd)\n");
		fprintf(cppFile, "\t\t\tc_oBlkState = OBK_FLSH;\n");
		fprintf(cppFile, "\t\telse if (m_oBlkWrDat.size() == 1)\n");
		fprintf(cppFile, "\t\t\tc_oBlkState = r_oBlkExpired ? OBK_FLSH : OBK_POP;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tbreak;\n");
		if (b64B) {
			fprintf(cppFile, "\tcase OBK_WR64:\n");
			fprintf(cppFile, "\t\tif (c_mifBusy || r_mifReqSelRr != 3)\n");
			fprintf(cppFile, "\t\t\tbreak;\n");
			fprintf(cppFile, "\n");
			fprintf(cppFile, "\t\tassert(!c_hifToMif_reqRdy);\n");
			fprintf(cppFile, "\n");
			fprintf(cppFile, "\t\tc_hifToMif_reqRdy = true;\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_type = MEM_REQ_WR;\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_addr = r_oBlkWrAdr;\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_tid = (sc_uint<MIF_TID_W>)(7 << MIF_TID_QWCNT_SHF);\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_tid |= (sc_uint<MIF_TID_W>)(c_oBlkWrTid << HIF_TID_TYPE_W);\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_tid |= 3;\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_data = m_oBlkWrDat.front();\n");
			fprintf(cppFile, "#ifndef _HTV\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_line = __LINE__;\n");
			fprintf(cppFile, "#endif\n");
			fprintf(cppFile, "\n");
			fprintf(cppFile, "\t\tc_hifToHti_datAvl = true;\n");
			fprintf(cppFile, "\t\tm_oBlkWrDat.pop();\n");
			fprintf(cppFile, "\n");
			fprintf(cppFile, "\t\tc_oBlkWrAdr = r_oBlkWrAdr + 8;\n");
			fprintf(cppFile, "\t\tc_oBlkWrCnt = r_oBlkWrCnt + 1;\n");
			fprintf(cppFile, "\n");
			fprintf(cppFile, "\t\tif (m_oBlkWrDat.size() == 1)\n");
			fprintf(cppFile, "\t\t\tc_oBlkWrPendCnt[INT(c_oBlkWrTid)] += 1;\n");
			fprintf(cppFile, "\n");
			fprintf(cppFile, "\t\tif (r_oBlkWrCnt == r_oBlkWrCntEnd)\n");
			fprintf(cppFile, "\t\t\tc_oBlkState = OBK_FLSH;\n");
			fprintf(cppFile, "\t\telse if (m_oBlkWrDat.size() == 1)\n");
			fprintf(cppFile, "\t\t\tc_oBlkState = OBK_POP;\n");
			fprintf(cppFile, "\n");
			fprintf(cppFile, "\t\tbreak;\n");
		}
		fprintf(cppFile, "\tcase OBK_FLSH:\n");
		fprintf(cppFile, "\t\tif (r_oBlkFlshEn && c_oBlkWrPendBusy)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (!r_oBlkFlshEn || r_oBlkRdCnt == 0) {\n");
		fprintf(cppFile, "\t\t\tc_oBlkState = OBK_SEND;\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (c_mifBusy || r_mifReqSelRr != 3)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tassert(!c_hifToMif_reqRdy);\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_hifToMif_reqRdy = true;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_type = MEM_REQ_RD;\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_addr = r_oBlkFlshAdrArg + ((r_oBlkRdCnt & 7) * 64);\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_tid = (sc_uint<MIF_TID_W>)(c_oBlkWrTid << HIF_TID_TYPE_W);\n");
		fprintf(cppFile, "\t\tc_hifToMif_req.m_tid |= 3;\n");
		if (true || g_appArgs.IsMemTraceEnabled()) {
			fprintf(cppFile, "#ifndef _HTV\n");
			fprintf(cppFile, "\t\tc_hifToMif_req.m_line = __LINE__;\n");
			fprintf(cppFile, "#endif\n");
		}
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_oBlkRdCnt -= 1;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_oBlkRdPendCnt[INT(c_oBlkWrTid)] += 1;\n");
		fprintf(cppFile, "\tbreak;\n");
		fprintf(cppFile, "\tcase OBK_SEND:\n");
		fprintf(cppFile, "\t\tif (r_oBlkSendRdy)\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (r_oBlkWrCnt == 0 &&\n");
		fprintf(cppFile, "\t\t\tc_oBlkQueDat.m_ctl == HT_DQ_HALT && !c_oBlkCntHalt)\n");
		fprintf(cppFile, "\t\t\t\tbreak;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_oBlkWrCnt = 0;\n");
		fprintf(cppFile, "\t\tc_oBlkWrIdx = (r_oBlkWrIdx + 1) & (c_oBlkNum-1);\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_oBlkSendRdy = true;\n");
		fprintf(cppFile, "\t\tc_oBlkSendIdx = c_oBlkWrTid;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tif (r_oBlkWrCnt != 0)\n");
		fprintf(cppFile, "\t\t\tc_oBlkSize = r_oBlkWrCnt*8;\n");
		fprintf(cppFile, "\t\telse {\n");
		fprintf(cppFile, "\t\t\tc_htiToHif_datHalt = c_oBlkQueDat.m_ctl == HT_DQ_HALT;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\t\tc_oBlkSize = (sc_uint<HIF_ARG_OBLK_SMAX_W+1>)c_oBlkQueDat.m_ctl;\n");
		fprintf(cppFile, "\t\t\tif (c_oBlkQueDat.m_ctl == HT_DQ_CALL)\n");
		fprintf(cppFile, "\t\t\t    c_oBlkSize |= (sc_uint<HIF_ARG_OBLK_SMAX_W+1>)(c_oBlkQueDat.m_data << 3);\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\t\tc_hifToHti_datAvl = true;\n");
		fprintf(cppFile, "\t\t\tm_oBlkQue.pop();\n");
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_oBlkTimer = r_oBlkTimeArg;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tc_oBlkState = OBK_IDLE;\n");
		fprintf(cppFile, "\tbreak;\n");
		fprintf(cppFile, "\tdefault:\n");
		fprintf(cppFile, "\t\tassert(0);\n");
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t//\n");
		fprintf(cppFile, "\t// Memory read responses\n");
		fprintf(cppFile, "\t//\n");
		fprintf(cppFile, "\tCHostCtrlMsgIntf c_iCtlMsg;\n");
		fprintf(cppFile, "\tCHostCtrlMsgIntf c_oCtlRdMsg;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tCMemRdRspIntf rdRsp = i_mifToHifP0_rdRsp;\n");
		fprintf(cppFile, "\n");
		if (b64B)
			fprintf(cppFile, "\tsc_uint<3> c_iBlkRdRspQw = (sc_uint<3>)(rdRsp.m_tid>>MIF_TID_QWCNT_SHF);\n");
		else
			fprintf(cppFile, "\tsc_uint<3> c_iBlkRdRspQw = (sc_uint<3>)(rdRsp.m_tid>>(IBK_FETCH_CNT_W+HIF_TID_TYPE_W));\n");
		fprintf(cppFile, "\tsc_uint<IBK_FETCH_CNT_W> c_iBlkRdRspIdx = (sc_uint<IBK_FETCH_CNT_W>)(rdRsp.m_tid>>HIF_TID_TYPE_W);\n");
		fprintf(cppFile, "\tm_iBlkRdDat.write_addr(c_iBlkRdRspIdx, c_iBlkRdRspQw);\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tc_iCtlMsg.m_bValid = false;\n");
		fprintf(cppFile, "\tc_oCtlRdMsg.m_bValid = false;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tif (i_mifToHifP0_rdRspRdy.read()) {\n");
		fprintf(cppFile, "\t\tswitch (rdRsp.m_tid & HIF_TID_TYPE_MSK) {\n");
		fprintf(cppFile, "\t\tcase 0:\n");
		fprintf(cppFile, "\t\t\tc_iCtlRdWait = false;\n");
		fprintf(cppFile, "\t\t\tc_iCtlMsg.m_data64 = rdRsp.m_data;\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\t\tcase 1:\n");
		fprintf(cppFile, "\t\t\tc_oCtlRdWait = false;\n");
		fprintf(cppFile, "\t\t\tc_oCtlRdMsg.m_data64 = rdRsp.m_data;\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\t\tcase 2:\n");
		fprintf(cppFile, "\t\t\tc_iBlkFetchCnt[c_iBlkRdRspIdx] -= 1;\n");
		fprintf(cppFile, "\t\t\tm_iBlkRdDat.write_mem(rdRsp.m_data);\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\t\tcase 3:\n");
		fprintf(cppFile, "\t\t\tc_oBlkRdPendCnt[(rdRsp.m_tid >> HIF_TID_TYPE_W) & 1] -= 1;\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\t\tdefault:\n");
		fprintf(cppFile, "\t\t\tassert(0);\n");
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t//\n");
		fprintf(cppFile, "\t// Memory write responses\n");
		fprintf(cppFile, "\t//\n");
		fprintf(cppFile, "\tif (i_mifToHifP0_wrRspRdy.read()) {\n");
		fprintf(cppFile, "\t\tsc_uint<MIF_TID_W> wrRspTid = i_mifToHifP0_wrRspTid;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\tswitch (wrRspTid & HIF_TID_TYPE_MSK) {\n");
		fprintf(cppFile, "\t\tcase 0:\n");
		fprintf(cppFile, "\t\t\tc_iCtlWrPend = false;\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\t\tcase 1:\n");
		fprintf(cppFile, "\t\t\tc_oCtlWrPend = false;\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\t\tcase 3:\n");
		fprintf(cppFile, "\t\t\tc_oBlkWrPendCnt[(wrRspTid >> HIF_TID_TYPE_W) & 1] -= 1;\n");
		fprintf(cppFile, "\t\t\tbreak;\n");
		fprintf(cppFile, "\t\tdefault:\n");
		fprintf(cppFile, "\t\t\tassert(0);\n");
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t//\n");
		fprintf(cppFile, "\t// iBlk queue\n");
		fprintf(cppFile, "\t//\n");
		if (!bInHostDataMaxBw) {
			fprintf(cppFile, "\tm_iBlkRdDat.read_addr(r_iBlkPushIdx, r_iBlkPushCnt);\n");
			fprintf(cppFile, "\n");
		}
		fprintf(cppFile, "\tsc_uint<IBK_FETCH_CNT_W> c_iBlkPushIdx = r_iBlkPushIdx;\n");
		fprintf(cppFile, "\tsc_uint<3> c_iBlkPushCnt = r_iBlkPushCnt;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tbool c_iBlkQueFull = r_hifToHti_datAvlCnt < 3;\n");
		fprintf(cppFile, "\tbool c_iBlkPushing = r_iBlkPushing;\n");
		fprintf(cppFile, "\tbool c_iBlkPushVal = !r_iBlkFetchCnt[r_iBlkPushIdx] &&\n");
		fprintf(cppFile, "\t\t\t\tr_iBlkFetchVal[r_iBlkPushIdx];\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tif (r_iBlkPushing && !c_iBlkQueFull) {\n");
		fprintf(cppFile, "\t\tif (r_iBlkPushCnt < r_iBlkFetchVal[r_iBlkPushIdx]) {\n");
		fprintf(cppFile, "\t\t\tc_hifToHti_datRdy = true;\n");
		fprintf(cppFile, "\t\t\tc_hifToHti_dat.m_data = m_iBlkRdDat.read_mem();\n");
		fprintf(cppFile, "\t\t\tc_hifToHti_dat.m_ctl = HT_DQ_DATA;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\t\tsc_uint<4> iBlkPushCnt = r_iBlkPushCnt + 1;\n");
		fprintf(cppFile, "\t\t\tc_iBlkPushCnt = (sc_uint<3>)iBlkPushCnt;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t\t\tif (iBlkPushCnt == r_iBlkFetchVal[r_iBlkPushIdx]) {\n");
		fprintf(cppFile, "\t\t\t\tc_iBlkPushIdx += 1;\n");
		fprintf(cppFile, "\t\t\t\tc_iBlkPushCnt = 0;\n");
		fprintf(cppFile, "\t\t\t\tc_iBlkFetchVal[r_iBlkPushIdx] = 0;\n");
		fprintf(cppFile, "\t\t\t\tc_iBlkPushing = false;\n");
		fprintf(cppFile, "\t\t\t}\n");
		fprintf(cppFile, "\t\t}\n");
		fprintf(cppFile, "\t} else if (!r_iBlkPushing)\n");
		fprintf(cppFile, "\t\tc_iBlkPushing = c_iBlkPushVal;\n");
		fprintf(cppFile, "\n");
		if (bInHostDataMaxBw) {
			fprintf(cppFile, "\tm_iBlkRdDat.read_addr(c_iBlkPushIdx, c_iBlkPushCnt);\n");
			fprintf(cppFile, "\n");
		}
		fprintf(cppFile, "\t//\n");
		fprintf(cppFile, "\t// Miscellaneous\n");
		fprintf(cppFile, "\t//\n");
		if (b64B) {
			fprintf(cppFile, "\tsc_uint<2> c_mifReqSel = r_mifReqSelRr +\n");
			fprintf(cppFile, "\t\t!(r_mifReqSelRr == 3 && r_oBlkState == OBK_WR64);\n");
		} else
			fprintf(cppFile, "\tsc_uint<2> c_mifReqSel = r_mifReqSelRr + 1;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tht_uint6 c_hifToMif_reqAvlCnt = r_hifToMif_reqAvlCnt\n");
		fprintf(cppFile, "\t\t\t\t\t+ i_mifToHifP0_reqAvl.read()\n");
		fprintf(cppFile, "\t\t\t\t\t- c_hifToMif_reqRdy;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tht_uint6 c_hifToHti_ctlAvlCnt = r_hifToHti_ctlAvlCnt\n");
		fprintf(cppFile, "\t\t\t\t\t+ i_htiToHif_ctlAvl.read()\n");
		fprintf(cppFile, "\t\t\t\t\t- c_hifToHti_ctlRdy;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tht_uint6 c_hifToHti_datAvlCnt = r_hifToHti_datAvlCnt\n");
		fprintf(cppFile, "\t\t\t\t\t+ i_htiToHif_datAvl.read()\n");
		fprintf(cppFile, "\t\t\t\t\t- c_hifToHti_datRdy;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tbool c_hifToHti_datHalt = r_hifToHti_datHalt;\n");
		fprintf(cppFile, "\tc_hifToHti_datHalt |= c_hifToHti_datRdy\n");
		fprintf(cppFile, "\t\t&& c_hifToHti_dat.m_ctl == HT_DQ_HALT;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tbool c_idle = r_hifToHti_datHalt && r_htiToHif_datHalt &&\n");
		fprintf(cppFile, "\t\tr_iCtlState == ICM_READ && !r_iCtlWrPend &&\n");
		fprintf(cppFile, "\t\tr_oCtlState == OCM_READ && !r_oCtlWrPend &&\n");
		fprintf(cppFile, "\t\t!r_oCtlWrMsg.m_bValid && c_oBlkCntIdle;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tbool c_reset = (r_reset1x && !i_dispToHif_dispStart.read()) || c_idle;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tbool c_hifToDisp_busy = !r_reset1x || i_dispToHif_dispStart.read();\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tif (r_reset1x) c_htaToHif_assert.m_bAssert = false;\n");
		fprintf(cppFile, "\n");
		if (m_bPerfMon) {
			fprintf(cppFile, "#ifndef _HTV\n");
			fprintf(cppFile, "\tif (c_hifToMif_reqRdy && !r_reset1x) {\n");
			fprintf(cppFile, "\t\tif (c_hifToMif_req.m_type == MEM_REQ_RD) {\n");
			fprintf(cppFile, "\t\t\tHt::g_syscMon.UpdateModuleMemReadBytes(0, 0, 8);\n");
			if (b64B) {
				fprintf(cppFile, "\t\t\tif (c_hifToMif_req.m_tid >> MIF_TID_QWCNT_SHF)\n");
				fprintf(cppFile, "\t\t\t\tHt::g_syscMon.UpdateModuleMemRead64s(0, 0, (c_hifToMif_req.m_addr & 0x3f) == 0 ? 1 : 0);\n");
				fprintf(cppFile, "\t\t\telse\n\t");
			}
			fprintf(cppFile, "\t\t\tHt::g_syscMon.UpdateModuleMemReads(0, 0, 1);\n");
			fprintf(cppFile, "\t\t}\n");
			fprintf(cppFile, "\t\tif (c_hifToMif_req.m_type == MEM_REQ_WR) {\n");
			fprintf(cppFile, "\t\t\tHt::g_syscMon.UpdateModuleMemWriteBytes(0, 0, 8);\n");
			if (b64B) {
				fprintf(cppFile, "\t\t\tif (c_hifToMif_req.m_tid >> MIF_TID_QWCNT_SHF)\n");
				fprintf(cppFile, "\t\t\t\tHt::g_syscMon.UpdateModuleMemWrite64s(0, 0, (c_hifToMif_req.m_addr & 0x3f) == 0 ? 1 : 0);\n");
				fprintf(cppFile, "\t\t\telse\n\t");
			}
			fprintf(cppFile, "\t\t\tHt::g_syscMon.UpdateModuleMemWrites(0, 0, 1);\n");
			fprintf(cppFile, "\t\t}\n");
			fprintf(cppFile, "\t}\n");
			fprintf(cppFile, "#endif\n");
		}
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t///////////////////////////////////////////////////////////////////////\n");
		fprintf(cppFile, "\t// Register assignments\n");
		fprintf(cppFile, "\t///////////////////////////////////////////////////////////////////////\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tm_iBlkRdyQue.clock(r_reset1x);\n");
		fprintf(cppFile, "\tm_iBlkRdDat.clock();\n");
		fprintf(cppFile, "\tm_oBlkQue.clock(r_reset1x);\n");
		fprintf(cppFile, "\tm_oBlkWrDat.clock(r_reset1x);\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tr_hifToDisp_busy = c_hifToDisp_busy;\n");
		fprintf(cppFile, "\tr_hifToHti_datHalt = r_reset1x ? false : c_hifToHti_datHalt;\n");
		fprintf(cppFile, "\tr_htiToHif_datHalt = r_reset1x ? false : c_htiToHif_datHalt;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tr_CtlQueBase = c_CtlQueBase;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tr_mifReqSelRr = r_reset1x ? (sc_uint<HIF_TID_TYPE_W>)0 : c_mifReqSel;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tr_hifToMif_reqRdy = r_reset1x ? false : c_hifToMif_reqRdy;\n");
		fprintf(cppFile, "\tr_hifToMif_req = c_hifToMif_req;\n");
		fprintf(cppFile, "\tr_hifToMif_reqAvlCnt = r_reset1x ? (ht_uint6)32 : c_hifToMif_reqAvlCnt;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tr_ctlTimeArg = r_reset1x ? (sc_uint<HIF_ARG_CTL_TIME_W>)0 : c_ctlTimeArg;\n");
		fprintf(cppFile, "\tr_iBlkAdrArg = c_iBlkAdrArg;\n");
		fprintf(cppFile, "\tr_oBlkAdrArg = c_oBlkAdrArg;\n");
		fprintf(cppFile, "\tr_oBlkFlshAdrArg = r_reset1x ? (sc_uint<MEM_ADDR_W>)0 : c_oBlkFlshAdrArg;\n");
		fprintf(cppFile, "\tr_oBlkFlshEn = c_oBlkFlshEn;\n");
		fprintf(cppFile, "\tr_iBlkSizeArg = c_iBlkSizeArg;\n");
		fprintf(cppFile, "\tr_iBlkCntArg = r_reset1x ? (sc_uint<HIF_ARG_IBLK_CNT_W>)0 : c_iBlkCntArg;\n");
		fprintf(cppFile, "\tr_oBlkSizeArg = c_oBlkSizeArg;\n");
		fprintf(cppFile, "\tr_oBlkCntArg = c_oBlkCntArg;\n");
		fprintf(cppFile, "\tr_oBlkTimeArg = r_reset1x ? (sc_uint<HIF_ARG_OBLK_TIME_W>)0 : c_oBlkTimeArg;\n");
		fprintf(cppFile, "\tr_oBlkAvlCnt = r_reset1x ? (sc_uint<HIF_ARG_OBLK_CNT_W+1>)0 : c_oBlkAvlCnt;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tr_iCtlTimer = r_reset1x ? (sc_uint<HIF_ARG_CTL_TIME_W>)0 : c_iCtlTimer;\n");
		fprintf(cppFile, "\tr_iCtlTimerWait = c_iCtlTimerWait;\n");
		fprintf(cppFile, "\tr_iCtlState = r_reset1x ? (sc_uint<2>)ICM_READ : c_iCtlState;\n");
		fprintf(cppFile, "\tr_iCtlRdWait = c_iCtlRdWait;\n");
		fprintf(cppFile, "\tr_iCtlWrPend = r_reset1x ? false : c_iCtlWrPend;\n");
		fprintf(cppFile, "\tr_iCtlRdIdx = r_reset1x ? (sc_uint<HIF_CTL_QUE_CNT_W>)0 : c_iCtlRdIdx;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tr_hifToHti_ctlRdy = r_reset1x ? false : c_hifToHti_ctlRdy;\n");
		fprintf(cppFile, "\tr_hifToHti_ctl = c_hifToHti_ctl;\n");
		fprintf(cppFile, "\tr_hifToHti_ctlAvlCnt = r_reset1x ? (ht_uint6)32 : c_hifToHti_ctlAvlCnt;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tr_oCtlTimer = r_reset1x ? (sc_uint<HIF_ARG_CTL_TIME_W>)0 : c_oCtlTimer;\n");
		fprintf(cppFile, "\tr_oCtlTimerWait = c_oCtlTimerWait;\n");
		fprintf(cppFile, "\tr_oCtlState = r_reset1x ? (sc_uint<2>)OCM_READ : c_oCtlState;\n");
		fprintf(cppFile, "\tr_oCtlRdWait = c_oCtlRdWait;\n");
		fprintf(cppFile, "\tr_oCtlWrPend = r_reset1x ? false : c_oCtlWrPend;\n");
		fprintf(cppFile, "\tr_oCtlWrIdx = r_reset1x ? (sc_uint<HIF_CTL_QUE_CNT_W>)0 : c_oCtlWrIdx;\n");
		fprintf(cppFile, "\tr_htiToHif_ctl = c_htiToHif_ctl;\n");
		fprintf(cppFile, "\tr_hifToHti_ctlAvl = c_hifToHti_ctlAvl;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tr_oCtlWrMsg = c_oCtlWrMsg;\n");
		fprintf(cppFile, "\tr_oBlkSendRdy = r_reset1x ? false : c_oBlkSendRdy;\n");
		fprintf(cppFile, "\tr_iBlkSendAvl = r_reset1x ? false : c_iBlkSendAvl;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tr_iCtlMsg = c_iCtlMsg;\n");
		fprintf(cppFile, "\tr_oCtlRdMsg = c_oCtlRdMsg;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tr_iBlkState = r_reset1x ? (sc_uint<2>)IBK_IDLE : c_iBlkState;\n");
		fprintf(cppFile, "\tr_iBlkRdCnt = c_iBlkRdCnt;\n");
		fprintf(cppFile, "\tr_iBlkRdAdr = c_iBlkRdAdr;\n");
		fprintf(cppFile, "\tr_iBlkRdIdx = r_reset1x ? (sc_uint<HIF_ARG_IBLK_CNT_W>)0 : c_iBlkRdIdx;\n");
		fprintf(cppFile, "\tfor (unsigned i=0; i<IBK_FETCH_CNT; i+=1) {\n");
		fprintf(cppFile, "\t\tr_iBlkFetchCnt[i] = r_reset1x ? (sc_uint<4>)0 : c_iBlkFetchCnt[i];\n");
		fprintf(cppFile, "\t\tr_iBlkFetchVal[i] = r_reset1x ? (sc_uint<4>)0 : c_iBlkFetchVal[i];\n");
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\tr_iBlkFetchIdx = r_reset1x ? (sc_uint<IBK_FETCH_CNT_W>)0 : c_iBlkFetchIdx;\n");
		fprintf(cppFile, "\tr_iBlkPushIdx = r_reset1x ? (sc_uint<IBK_FETCH_CNT_W>)0 : c_iBlkPushIdx;\n");
		fprintf(cppFile, "\tr_iBlkPushCnt = r_reset1x ? (sc_uint<3>)0 : (sc_uint<3>)c_iBlkPushCnt;\n");
		fprintf(cppFile, "\tr_iBlkPushing = r_reset1x ? false : c_iBlkPushing;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tr_hifToHti_datRdy = r_reset1x ? false : c_hifToHti_datRdy;\n");
		fprintf(cppFile, "\tr_hifToHti_dat = c_hifToHti_dat;\n");
		fprintf(cppFile, "\tr_hifToHti_datAvlCnt = r_reset1x ? (ht_uint6)32 : c_hifToHti_datAvlCnt;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tr_oBlkState = r_reset1x ? (sc_uint<3>)OBK_IDLE : c_oBlkState;\n");
		fprintf(cppFile, "\tr_oBlkSendIdx = c_oBlkSendIdx;\n");
		fprintf(cppFile, "\tr_oBlkSize = c_oBlkSize;\n");
		fprintf(cppFile, "\tr_oBlkTimer = c_oBlkTimer;\n");
		fprintf(cppFile, "\tr_oBlkExpired = c_oBlkExpired;\n");
		fprintf(cppFile, "\tr_oBlkWrAdr = c_oBlkWrAdr;\n");
		fprintf(cppFile, "\tr_oBlkWrCnt = r_reset1x ? (sc_uint<HIF_ARG_OBLK_SMAX_W-2>)0 : c_oBlkWrCnt;\n");
		fprintf(cppFile, "\tr_oBlkWrCntEnd = c_oBlkWrCntEnd;\n");
		if (b64B) {
			fprintf(cppFile, "\tr_oBlkWrCntEnd64 = c_oBlkWrCntEnd64;\n");
		}
		fprintf(cppFile, "\tr_oBlkWrIdx = r_reset1x ? (sc_uint<HIF_ARG_OBLK_CNT_W>)0 : c_oBlkWrIdx;\n");
		fprintf(cppFile, "\tr_oBlkRdCnt = c_oBlkRdCnt;\n");
		fprintf(cppFile, "\tr_hifToHti_datAvl = c_hifToHti_datAvl;\n");
		fprintf(cppFile, "\tfor (int i=0; i<2; i+=1) {\n");
		fprintf(cppFile, "\t\tr_oBlkWrPendCnt[i] = r_reset1x ? (sc_uint<8>)0 : c_oBlkWrPendCnt[i];\n");
		fprintf(cppFile, "\t\tr_oBlkRdPendCnt[i] = r_reset1x ? (sc_uint<4>)0 : c_oBlkRdPendCnt[i];\n");
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tr_htaToHif_assert = c_htaToHif_assert;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\tr_reset1x = r_i_reset ? true : c_reset;\n");
		fprintf(cppFile, "\tHtResetFlop(r_i_reset, i_reset.read());\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t// register properties\n");
		fprintf(cppFile, "\tht_attrib(keep, r_iCtlState, \"true\");\n");
		fprintf(cppFile, "\tht_attrib(keep, r_oCtlState, \"true\");\n");
		fprintf(cppFile, "\tht_attrib(keep, r_iBlkState, \"true\");\n");
		fprintf(cppFile, "\tht_attrib(keep, r_oBlkState, \"true\");\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\t///////////////////////////////////////////////////////////////////////\n");
		fprintf(cppFile, "\t// Output assignments\n");
		fprintf(cppFile, "\t///////////////////////////////////////////////////////////////////////\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\to_hifToDisp_busy = r_hifToDisp_busy;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\to_hifToHti_ctlRdy = r_hifToHti_ctlRdy;\n");
		fprintf(cppFile, "\to_hifToHti_ctl = r_hifToHti_ctl;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\to_hifToHti_datRdy = r_hifToHti_datRdy;\n");
		fprintf(cppFile, "\to_hifToHti_dat = r_hifToHti_dat;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\to_hifToHti_ctlAvl = r_hifToHti_ctlAvl;\n");
		fprintf(cppFile, "\to_hifToHti_datAvl = r_hifToHti_datAvl;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\to_hifP0ToMif_reqRdy = r_hifToMif_reqRdy;\n");
		fprintf(cppFile, "\to_hifP0ToMif_req = r_hifToMif_req;\n");
		fprintf(cppFile, "\to_hifP0ToMif_rdRspFull = false;\n");
		fprintf(cppFile, "}\n");

		cppFile.FileClose();
	}
}
