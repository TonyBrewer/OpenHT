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

void
CDsnInfo::GenerateModuleFiles(CModule &mod)
{
	for (int modInstIdx = 0; modInstIdx < mod.m_instSet.GetInstCnt(); modInstIdx += 1) {
		CModInst * pModInst = mod.m_instSet.GetInst(modInstIdx);

		g_appArgs.GetDsnRpt().AddLevel("Pers%s\n", pModInst->m_instName.c_str());

		// Generate statements seperate lists of statements for each module feature
		GenPrimStateStatements(mod);
		GenModTDSUStatements(mod);		// typedef/define/struct/union
		GenModIplStatements(pModInst);
		GenModIhmStatements(mod);
		GenModMsgStatements(mod);
		GenModBarStatements(mod);
		GenModOhmStatements(mod);
		GenModCxrStatements(mod, modInstIdx);
		GenModIhdStatements(mod);
		GenModOhdStatements(mod);
		GenModMifStatements(mod);
		GenModStrmStatements(mod);
		GenModStBufStatements(&mod);
		GenModNgvStatements(mod);

		bool bNeedClk2x = NeedClk2x();

		if (mod.m_instSet.GetInstCnt() > 1)
			pModInst->m_fileName = mod.m_modName == pModInst->m_instName ? mod.m_modName.AsStr() + "_" : pModInst->m_instName;
		else
			pModInst->m_fileName = mod.m_modName;

		WritePersCppFile(pModInst, bNeedClk2x);
		WritePersIncFile(pModInst, bNeedClk2x);

		g_appArgs.GetDsnRpt().EndLevel();
	}

	if (mod.m_instSet.GetInstCnt() > 1)
		GenModInstInc(mod);
}

void CDsnInfo::GenModInstInc(CModule &mod)
{
	// Generate .h file
	string incFileName = g_appArgs.GetOutputFolder() + "/Pers" + mod.m_modName.Uc() + ".h";

	CHtFile incFile(incFileName, "w");

	GenPersBanner(incFile, "", mod.m_modName.Uc().c_str(), true);

	for (int modInstIdx = 0; modInstIdx < mod.m_instSet.GetInstCnt(); modInstIdx += 1) {
		CModInst * pModInst = mod.m_instSet.GetInst(modInstIdx);

		if (modInstIdx == 0)
			fprintf(incFile, "#if defined(PERS_%s)\n", pModInst->m_instName.Upper().c_str());
		else
			fprintf(incFile, "#elif defined(PERS_%s)\n", pModInst->m_instName.Upper().c_str());

		fprintf(incFile, "\n");

		for (size_t prmIdx = 0; prmIdx < pModInst->m_callInstParamList.size(); prmIdx += 1) {
			CCallInstParam & instParam = pModInst->m_callInstParamList[prmIdx];

			fprintf(incFile, "#define %s %s\n", instParam.m_name.c_str(), instParam.m_value.c_str());
		}

		if (pModInst->m_callInstParamList.size() > 0)
			fprintf(incFile, "\n");

		if (pModInst->m_instName != mod.m_modName)
			fprintf(incFile, "#define CPers%s CPers%s\n", mod.m_modName.Uc().c_str(), pModInst->m_instName.Uc().c_str());

		fprintf(incFile, "#include \"Pers%s.h\"\n", pModInst->m_fileName.Uc().c_str());

		fprintf(incFile, "\n");
	}

	fprintf(incFile, "#else\n");
	fprintf(incFile, "#error \"Fatal error in Pers%s.h\"\n", mod.m_modName.Uc().c_str());
	fprintf(incFile, "#endif\n");

	incFile.FileClose();
}

bool CDsnInfo::NeedClk2x()
{
	bool bNeed2xClk = !m_gblPreInstr2x.Empty() || !m_gblReg2x.Empty() || !m_gblPostInstr2x.Empty() || !m_gblOut2x.Empty()
		|| !m_cxrT0Stage2x.Empty() || !m_cxrTsStage2x.Empty() || !m_cxrPostInstr2x.Empty() || !m_cxrReg2x.Empty() || !m_cxrOut2x.Empty()
		|| !m_iplT0Stg2x.Empty() || !m_iplT1Stg2x.Empty() || !m_iplT2Stg2x.Empty() || !m_iplTsStg2x.Empty()
		|| !m_iplPostInstr2x.Empty() || !m_iplReg2x.Empty() || !m_iplPostReg2x.Empty() || !m_iplOut2x.Empty()
		|| !m_msgPreInstr2x.Empty() || !m_msgReg2x.Empty() || !m_msgPostInstr2x.Empty();

	return bNeed2xClk;
}

void
CDsnInfo::WritePersCppFile(CModInst * pModInst, bool bNeedClk2x)
{
	CModule * pMod = pModInst->m_pMod;

	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

	string cppFileName = g_appArgs.GetOutputFolder() + "/Pers" + unitNameUc + pModInst->m_fileName.Uc() + ".cpp";

	CHtFile cppFile(cppFileName, "w");

	GenPersBanner(cppFile, unitNameUc.c_str(), pModInst->m_instName.Uc().c_str(), false, pMod->m_modName.Uc().c_str());

	m_moduleFileList.push_back(VA("Pers%s_src.cpp", pMod->m_modName.Uc().c_str()));
	m_moduleFileList.push_back(cppFileName);

	fprintf(cppFile, "#define HT_MOD_ID 0x%x\n", m_htModId++);
	fprintf(cppFile, "#include \"Pers%s%s_src.cpp\"\n", unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
	fprintf(cppFile, "#undef HT_MOD_ID\n");
	fprintf(cppFile, "\n");
	fprintf(cppFile, "#define HT_MOD_ID 0x%x\n", m_htModId++);
	fprintf(cppFile, "\n");

	if (g_appArgs.IsInstrTraceEnabled()) {
		fprintf(cppFile, "#ifndef _HTV\n");
		fprintf(cppFile, "extern FILE *Ht::g_instrTraceFp;\n");
		fprintf(cppFile, "#endif\n");
		fprintf(cppFile, "\n");
	}

	// Write statement lists to module file in appropriate order
	m_iplMacros.Write(cppFile);
	m_gblMacros.Write(cppFile);
	m_mifMacros.Write(cppFile);
	m_ihdMacros.Write(cppFile);
	m_ohdMacros.Write(cppFile);
	m_cxrMacros.Write(cppFile);
	m_ohmMacros.Write(cppFile);
	m_msgFuncDef.Write(cppFile);
	m_barFuncDef.Write(cppFile);
	m_strmFuncDef.Write(cppFile);
	m_stBufFuncDef.Write(cppFile);

	fprintf(cppFile, "void CPers%s%s::Pers%s%s_1x()\n",
		unitNameUc.c_str(), pModInst->m_instName.Uc().c_str(),
		unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
	fprintf(cppFile, "{\n");

	fprintf(cppFile, "\tc_aeId = i_aeId.read();\n");
	fprintf(cppFile, "\tc_unitId = (uint8_t)(i_aeId.read() * HT_UNIT_CNT + i_aeUnitId.read());\n");
	fprintf(cppFile, "\tc_replId = i_replId.read();\n\n");

	m_cxrT0Stage1x.Write(cppFile);
	m_iplT0Stg1x.Write(cppFile);
	m_iplT1Stg1x.Write(cppFile);
	m_iplT2Stg1x.Write(cppFile);
	m_iplTsStg1x.Write(cppFile);

	m_cxrTsStage1x.Write(cppFile);

	m_ohmPreInstr1x.Write(cppFile);
	m_ihdPreInstr1x.Write(cppFile);
	m_ohdPreInstr1x.Write(cppFile);
	m_mifPreInstr1x.Write(cppFile);
	m_gblPreInstr1x.Write(cppFile);
	m_msgPreInstr1x.Write(cppFile);
	m_barPreInstr1x.Write(cppFile);
	m_strmPreInstr1x.Write(cppFile);
	m_stBufPreInstr1x.Write(cppFile);
	m_cxrPreInstr1x.Write(cppFile);

	m_iplPostInstr1x.Write(cppFile);
	m_ihmPostInstr1x.Write(cppFile);
	m_ohmPostInstr1x.Write(cppFile);
	m_cxrPostInstr1x.Write(cppFile);
	m_gblPostInstr1x.Write(cppFile);
	m_ihdPostInstr1x.Write(cppFile);
	m_ohdPostInstr1x.Write(cppFile);
	m_mifPostInstr1x.Write(cppFile);
	m_msgPostInstr1x.Write(cppFile);
	m_barPostInstr1x.Write(cppFile);
	m_strmPostInstr1x.Write(cppFile);
	m_stBufPostInstr1x.Write(cppFile);

	if (!m_iplReset1x.Empty() || !m_mifReset1x.Empty()) {
		fprintf(cppFile, "\tif (r_reset1x) {\n");
		m_iplReset1x.Write(cppFile);
		m_mifReset1x.Write(cppFile);
		fprintf(cppFile, "\t}\n");
		fprintf(cppFile, "\n");
	}

	fprintf(cppFile, "\t//////////////////////////////////////////////////////\n");
	fprintf(cppFile, "\t// Register assignments\n");
	fprintf(cppFile, "\n");

	m_ihdRamClock1x.Write(cppFile);
	m_mifRamClock1x.Write(cppFile);

	m_iplReg1x.Write(cppFile);
	m_ihmReg1x.Write(cppFile);
	m_ohmReg1x.Write(cppFile);
	m_gblReg1x.Write(cppFile);
	m_ihdReg1x.Write(cppFile);
	m_ohdReg1x.Write(cppFile);
	m_mifReg1x.Write(cppFile);
	m_cxrReg1x.Write(cppFile);
	m_msgReg1x.Write(cppFile);
	m_barReg1x.Write(cppFile);
	m_strmReg1x.Write(cppFile);
	m_stBufReg1x.Write(cppFile);

	if (pMod->m_bHasThreads && pMod->m_clkRate == eClk1x)
		fprintf(cppFile, "\tr_userReset = r_reset1x;\n\n");

	fprintf(cppFile, "\tht_attrib(equivalent_register_removal, r_reset1x, \"no\");\n");
	fprintf(cppFile, "\tHtResetFlop(r_reset1x, i_reset.read());\n");
	fprintf(cppFile, "\n");

	if (bNeedClk2x) {
		fprintf(cppFile, "\tht_attrib(equivalent_register_removal, r_reset2x, \"no\");\n");
		fprintf(cppFile, "\tHtResetFlop(r_reset2x, i_reset.read());\n");
		fprintf(cppFile, "\tc_reset2x = r_reset2x;\n");
		fprintf(cppFile, "\n");
	}

	int phaseCnt = CountPhaseResetFanout(pModInst);
	phaseCnt = pModInst->m_pMod->m_phaseCnt;

	char phaseStr[4] = { 'A' - 1, '\0', '\0', '\0' };

	for (int i = 0; i < phaseCnt; i += 1) {
		if (phaseStr[0] < 'Z')
			phaseStr[0] += 1;
		else {
			phaseStr[0] = 'A';
			if (phaseStr[1] == '\0')
				phaseStr[1] = 'A';
			else
				phaseStr[1] += 1;
		}

		fprintf(cppFile, "\tht_attrib(equivalent_register_removal, r_phaseReset%s, \"no\");\n", phaseStr);
		fprintf(cppFile, "\tHtResetFlop(r_phaseReset%s, i_reset.read());\n", phaseStr);
		fprintf(cppFile, "\n");
	}

	m_gblPostReg1x.Write(cppFile);
	m_iplPostReg1x.Write(cppFile);
	m_mifPostReg1x.Write(cppFile);
	m_msgPostReg1x.Write(cppFile);
	m_ohmPostReg1x.Write(cppFile);
	m_ihdPostReg1x.Write(cppFile);
	m_ohdPostReg1x.Write(cppFile);
	m_cxrPostReg1x.Write(cppFile);

	fprintf(cppFile, "\t//////////////////////////////////////////////////////\n");
	fprintf(cppFile, "\t// Output assignments\n");
	fprintf(cppFile, "\n");

	m_iplOut1x.Write(cppFile);
	m_ohmOut1x.Write(cppFile);
	m_gblOut1x.Write(cppFile);
	m_ihdOut1x.Write(cppFile);
	m_ohdOut1x.Write(cppFile);
	m_mifOut1x.Write(cppFile);
	m_cxrOut1x.Write(cppFile);
	m_msgOut1x.Write(cppFile);
	m_barOut1x.Write(cppFile);
	m_strmOut1x.Write(cppFile);

	fprintf(cppFile, "}\n");

	//HtlAssert(pMod->m_bNeed2xClk == bNeed2xClk);

	if (bNeedClk2x) {

		fprintf(cppFile, "\n");
		fprintf(cppFile, "void CPers%s%s::Pers%s%s_2x()\n",
			unitNameUc.c_str(), pModInst->m_instName.Uc().c_str(),
			unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
		fprintf(cppFile, "{\n");

		m_cxrT0Stage2x.Write(cppFile);
		m_iplT0Stg2x.Write(cppFile);
		m_iplT1Stg2x.Write(cppFile);
		m_iplT2Stg2x.Write(cppFile);
		m_iplTsStg2x.Write(cppFile);

		m_cxrTsStage2x.Write(cppFile);

		m_ohmPreInstr2x.Write(cppFile);
		m_ihdPreInstr2x.Write(cppFile);
		m_ohdPreInstr2x.Write(cppFile);
		m_mifPreInstr2x.Write(cppFile);
		m_gblPreInstr2x.Write(cppFile);
		m_msgPreInstr2x.Write(cppFile);
		m_barPreInstr2x.Write(cppFile);
		m_strmPreInstr2x.Write(cppFile);
		m_stBufPreInstr2x.Write(cppFile);
		m_cxrPreInstr2x.Write(cppFile);

		m_iplPostInstr2x.Write(cppFile);
		m_ihmPostInstr2x.Write(cppFile);
		m_ohmPostInstr2x.Write(cppFile);
		m_cxrPostInstr2x.Write(cppFile);
		m_gblPostInstr2x.Write(cppFile);
		m_msgPostInstr2x.Write(cppFile);
		m_barPostInstr2x.Write(cppFile);
		m_strmPostInstr2x.Write(cppFile);
		m_stBufPostInstr2x.Write(cppFile);
		m_ihdPostInstr2x.Write(cppFile);
		m_ohdPostInstr2x.Write(cppFile);
		m_mifPostInstr2x.Write(cppFile);

		if (!m_iplReset2x.Empty() || !m_mifReset2x.Empty()) {
			fprintf(cppFile, "\tif (c_reset2x) {\n");
			m_iplReset2x.Write(cppFile);
			m_mifReset2x.Write(cppFile);
			fprintf(cppFile, "\t}\n");
			fprintf(cppFile, "\n");
		}

		fprintf(cppFile, "\t//////////////////////////////////////////////////////\n");
		fprintf(cppFile, "\t// Register assignments\n");
		fprintf(cppFile, "\n");

		m_ihdRamClock2x.Write(cppFile);
		m_mifRamClock2x.Write(cppFile);

		m_iplReg2x.Write(cppFile);
		//m_ihmReg2x.Write(cppFile);
		m_ohmReg2x.Write(cppFile);
		m_gblReg2x.Write(cppFile);
		m_ihdReg2x.Write(cppFile);
		m_ohdReg2x.Write(cppFile);
		m_mifReg2x.Write(cppFile);
		m_cxrReg2x.Write(cppFile);
		m_msgReg2x.Write(cppFile);
		m_barReg2x.Write(cppFile);
		m_strmReg2x.Write(cppFile);
		m_stBufReg2x.Write(cppFile);

		if (pMod->m_bHasThreads && pMod->m_clkRate == eClk2x)
			fprintf(cppFile, "\tr_userReset = c_reset2x;\n\n");

		fprintf(cppFile, "\tht_attrib(equivalent_register_removal, r_phase, \"no\");\n");
		fprintf(cppFile, "\tr_phase = c_reset2x.read() || !r_phase;\n");
		fprintf(cppFile, "\n");

		m_gblPostReg2x.Write(cppFile);
		m_iplPostReg2x.Write(cppFile);
		m_mifPostReg2x.Write(cppFile);
		m_msgPostReg2x.Write(cppFile);
		m_ohmPostReg2x.Write(cppFile);
		m_ihdPostReg2x.Write(cppFile);
		m_ohdPostReg2x.Write(cppFile);
		m_cxrPostReg2x.Write(cppFile);

		fprintf(cppFile, "\t//////////////////////////////////////////////////////\n");
		fprintf(cppFile, "\t// Output assignments\n");
		fprintf(cppFile, "\n");

		m_iplOut2x.Write(cppFile);
		m_ohmOut2x.Write(cppFile);
		m_gblOut2x.Write(cppFile);
		m_ihdOut2x.Write(cppFile);
		m_ohdOut2x.Write(cppFile);
		m_mifOut2x.Write(cppFile);
		m_cxrOut2x.Write(cppFile);
		m_msgOut2x.Write(cppFile);
		m_barOut2x.Write(cppFile);
		m_strmOut2x.Write(cppFile);

		fprintf(cppFile, "}\n");
	}

	if (!m_gblOutCont.Empty()) {

		pMod->m_bContAssign = true;

		fprintf(cppFile, "\n");
		fprintf(cppFile, "void CPers%s%s::Pers%s%sCont()\n",
			unitNameUc.c_str(), pModInst->m_instName.Uc().c_str(),
			unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
		fprintf(cppFile, "{\n");

		m_gblPreCont.Write(cppFile);

		fprintf(cppFile, "\t//////////////////////////////////////////////////////\n");
		fprintf(cppFile, "\t// Output assignments\n");
		fprintf(cppFile, "\n");

		m_gblOutCont.Write(cppFile);

		fprintf(cppFile, "}\n");
	}

	cppFile.FileClose();
}

void
CDsnInfo::WritePersIncFile(CModInst * pModInst, bool bNeedClk2x)
{
	CModule * pMod = pModInst->m_pMod;

	string vcdModName = VA("Pers%s", pModInst->m_instName.Uc().c_str());

	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

	// Generate .h file
	string incFileName = g_appArgs.GetOutputFolder() + "/Pers" + unitNameUc + pModInst->m_fileName.Uc() + ".h";

	CHtFile incFile(incFileName, "w");
	CHtCode incCode(incFile);

	GenPersBanner(incFile, unitNameUc.c_str(), pModInst->m_instName.Uc().c_str(), true);

	bool bStateMachine = pMod->m_bHasThreads;

	if (bStateMachine) {
		bool bFirstModVar = false;
		GenModVar(eVcdUser, vcdModName, bFirstModVar, "bool const", "", "GR_htReset", "r_userReset");
		GenModVar(eVcdUser, vcdModName, bFirstModVar, "uint8_t const", "", "SR_aeId", "c_aeId");
		GenModVar(eVcdUser, vcdModName, bFirstModVar, "uint8_t const", "", "SR_unitId", "c_unitId");
		GenModVar(eVcdUser, vcdModName, bFirstModVar, "uint8_t const", "", "SR_replId", "c_replId");
	}
	if (!pMod->m_bHtId)
		m_iplDefines.Append("#endif\n");
	m_iplDefines.Append("#endif\n");
	m_iplDefines.Append("\n");

	//////////////////////////////////////////////////
	// Generate include file

	fprintf(incFile, "\n");

	fprintf(incFile, "#ifndef _HTV\n");
	fprintf(incFile, "extern MTRand_int32 g_rndRetry;\n");
	fprintf(incFile, "extern MTRand_int64 g_rndInit;\n");
	fprintf(incFile, "#endif\n");
	fprintf(incFile, "\n");

	m_htSelDef.Write(incFile);
	fprintf(incFile, "\n");

	m_tdsuDefines.Write(incFile);
	m_psInclude.Write(incFile);
	m_mifDefines.Write(incFile);
	m_cxrDefines.Write(incFile);
	m_iplDefines.Write(incFile);
	m_gblDefines.Write(incFile);

	fprintf(incFile, "SC_MODULE(CPers%s%s)\n", unitNameUc.c_str(), pModInst->m_instName.Uc().c_str());
	fprintf(incFile, "{\n");
	fprintf(incFile, "\tht_attrib(keep_hierarchy, CPers%s%s, \"true\");\n",
		unitNameUc.c_str(), pModInst->m_instName.Uc().c_str());
	fprintf(incFile, "\n");
	fprintf(incFile, "\t//////////////////////////////////\n");
	fprintf(incFile, "\t// Module %s typedefs\n", pModInst->m_instName.c_str());

	for (size_t i = 0; i < m_typedefList.size(); i += 1) {
		//	must be a module and match current module
		if (m_typedefList[i].m_scope.compare("module") != 0
			|| m_typedefList[i].m_modName.compare(pMod->m_modName.c_str()) != 0) {
			continue;
		}

		CTypeDef &typeDef = m_typedefList[i];
		if (typeDef.m_width.size() == 0) {
			fprintf(incFile, "\ttypedef %s\t\t%s;\n", typeDef.m_type.c_str(), typeDef.m_name.c_str());
		} else {
			int width = typeDef.m_width.AsInt();

			if (typeDef.m_name == "uint8_t" || typeDef.m_name == "uint16_t" || typeDef.m_name == "uint32_t" || typeDef.m_name == "uint64_t"
				|| typeDef.m_name == "int8_t" || typeDef.m_name == "int16_t" || typeDef.m_name == "int32_t" || typeDef.m_name == "int64_t")
				continue;

			if (width > 0) {
				if (typeDef.m_type.substr(0, 3) == "int")
					fprintf(incFile, "\ttypedef %s<%s>\t\t%s;\n",
					width > 64 ? "sc_bigint" : "sc_int", typeDef.m_width.c_str(), typeDef.m_name.c_str());
				else if (typeDef.m_type.substr(0, 4) == "uint")
					fprintf(incFile, "\ttypedef %s<%s>\t\t%s;\n",
					width > 64 ? "sc_biguint" : "sc_uint", typeDef.m_width.c_str(), typeDef.m_name.c_str());
				else
					ParseMsg(Fatal, typeDef.m_lineInfo, "unexpected type '%s'", typeDef.m_type.c_str());
			}
		}
	}
	fprintf(incFile, "\n\n");
	fprintf(incFile, "\t//////////////////////////////////\n");
	fprintf(incFile, "\t// Module %s structs and unions\n", pModInst->m_instName.c_str());
	CHtCode fStructUnion;
	for (size_t recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
		if (m_recordList[recordIdx]->m_scope != eModule)
			continue;	// module only
		GenUserStructs(fStructUnion, m_recordList[recordIdx]);

		string structName = VA("CPers%s::%s", pModInst->m_instName.Uc().c_str(), m_recordList[recordIdx]->m_typeName.c_str());

		GenUserStructBadData(m_iplBadDecl, true, structName,
			m_recordList[recordIdx]->m_fieldList, m_recordList[recordIdx]->m_bCStyle, "");
	}
	fStructUnion.Write(incFile, "\t");
	fprintf(incFile, "\n\n");

	GenModDecl(eVcdUser, incCode, vcdModName, "sc_in<bool>", "i_clock1x");
	GenModDecl(eVcdUser, incCode, vcdModName, "sc_in<bool> ht_noload", "i_clock2x");

	if (bStateMachine || NeedClk2x())
		fprintf(incFile, "\tsc_in<bool> i_reset;\n");

	fprintf(incFile, "\tsc_in<uint8_t> i_aeId;\n");
	fprintf(incFile, "\tsc_in<uint8_t> i_aeUnitId;\n");
	fprintf(incFile, "\tsc_in<uint8_t> i_replId;\n");

	fprintf(incFile, "\n");

	if (pMod->m_bHasThreads) {
		fprintf(incFile, "\t// HT assert interface\n");
		fprintf(incFile, "\tsc_out<CHtAssertIntf> o_%sToHta_assert;\n", pModInst->m_instName.Lc().c_str());
		fprintf(incFile, "\n");
	}

	m_msgIoDecl.Write(incFile);
	m_barIoDecl.Write(incFile);
	m_ihmIoDecl.Write(incFile);
	m_ohmIoDecl.Write(incFile);
	m_cxrIoDecl.Write(incFile);
	m_gblIoDecl.Write(incFile);
	m_ihdIoDecl.Write(incFile);
	m_ohdIoDecl.Write(incFile);
	m_mifIoDecl.Write(incFile);
	m_strmIoDecl.Write(incFile);

	fprintf(incFile, "\t////////////////////////////////////////////\n");
	fprintf(incFile, "\t// References to user defined variables\n");
	fprintf(incFile, "\n");
	fprintf(incFile, "#\tif !defined(_HTV)\n");

	m_iplRefDecl.Write(incFile);
	m_gblRefDecl.Write(incFile);

	fprintf(incFile, "#\tendif\n\n");

	fprintf(incFile, "\tuint8_t ht_noload c_aeId;\n");
	fprintf(incFile, "\tuint8_t ht_noload c_unitId;\n");
	fprintf(incFile, "\tuint8_t ht_noload c_replId;\n");
	fprintf(incFile, "\n");

	fprintf(incFile, "\t////////////////////////////////////////////\n");
	fprintf(incFile, "\t// Internal State\n");
	fprintf(incFile, "\n");

	m_gblRegDecl.Write(incFile);
	m_ihmRegDecl.Write(incFile);
	m_ohmRegDecl.Write(incFile);
	m_msgRegDecl.Write(incFile);
	m_cxrRegDecl.Write(incFile);
	m_iplRegDecl.Write(incFile);
	//m_msgRegDecl.Write(incFile);
	m_barRegDecl.Write(incFile);
	m_strmRegDecl.Write(incFile);
	m_stBufRegDecl.Write(incFile);

	if (pMod->m_bHasThreads) {
		if (pMod->m_clkRate == eClk1x)
			fprintf(incFile, "\tCHtAssertIntf r_%sToHta_assert;\n", pModInst->m_instName.Lc().c_str());
		else {
			fprintf(incFile, "\tsc_signal<CHtAssertIntf> r_%sToHta_assert;\n", pModInst->m_instName.Lc().c_str());
			fprintf(incFile, "\tCHtAssertIntf r_%sToHta_assert_1x;\n", pModInst->m_instName.Lc().c_str());
		}
		fprintf(incFile, "\n");
	}

	m_ihdRamDecl.Write(incFile);
	m_ihdRegDecl.Write(incFile);
	m_ohdRegDecl.Write(incFile);

	m_mifRamDecl.Write(incFile);

	m_mifDecl.Write(incFile);

	if (bStateMachine) {
		if (bNeedClk2x)
			fprintf(incFile, "\tbool r_phase;\n");
		fprintf(incFile, "\tbool ht_noload r_reset1x;\n");
		if (bNeedClk2x) {
			fprintf(incFile, "\tbool r_reset2x;\n");
			fprintf(incFile, "\tsc_signal<bool> c_reset2x;\n");
		}

		fprintf(incFile, "\tbool ht_noload r_userReset;\n");
		fprintf(incFile, "\n");
	}

	m_psDecl.Write(incFile);

	fprintf(incFile, "\t////////////////////////////////////////////\n");
	fprintf(incFile, "\t// Methods\n");
	fprintf(incFile, "\n");

	fprintf(incFile, "\tvoid Pers%s%s();\n",
		unitNameUc.c_str(), pMod->m_modName.Uc().c_str());

	fprintf(incFile, "\tvoid Pers%s%s_1x();\n",
		unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
	if (bNeedClk2x)
		fprintf(incFile, "\tvoid Pers%s%s_2x();\n",
		unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
	if (pMod->m_bContAssign)
		fprintf(incFile, "\tvoid Pers%s%sCont();\n",
		unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
	fprintf(incFile, "\n");

	m_iplFuncDecl.Write(incFile);
	m_mifFuncDecl.Write(incFile);
	m_cxrFuncDecl.Write(incFile);
	m_ohmFuncDecl.Write(incFile);
	m_ihdFuncDecl.Write(incFile);
	m_ohdFuncDecl.Write(incFile);
	m_msgFuncDecl.Write(incFile);
	m_barFuncDecl.Write(incFile);
	m_strmFuncDecl.Write(incFile);
	m_stBufFuncDecl.Write(incFile);

	if (pMod->m_htFuncList.size() > 0) {
		for (size_t funcIdx = 0; funcIdx < pMod->m_htFuncList.size(); funcIdx += 1) {
			CFunc &func = pMod->m_htFuncList[funcIdx];

			fprintf(incFile, "\t%s %s;\n", func.m_rtnType.c_str(), func.m_funcName.c_str());
		}
		fprintf(incFile, "\n");
	}

	if (pMod->m_funcList.size() > 0) {
		for (size_t funcIdx = 0; funcIdx < pMod->m_funcList.size(); funcIdx += 1) {
			CFunction &func = pMod->m_funcList[funcIdx];

			fprintf(incFile, "\t%s %s(", func.m_pType->m_typeName.c_str(), func.m_name.c_str());

			if (func.m_paramList.size() == 0) {
				fprintf(incFile, ");\n");
				continue;
			} else {
				char * pSeparator = "\n";
				for (size_t paramIdx = 0; paramIdx < func.m_paramList.size(); paramIdx += 1) {
					CField * pParam = func.m_paramList[paramIdx];

					bool bInput = pParam->m_dir == "input";

					fprintf(incFile, "%s\t\t%s %s& %s", pSeparator, pParam->m_pType->m_typeName.c_str(),
						bInput ? "const " : "", pParam->m_name.c_str());
					pSeparator = ",\n";
				}
				fprintf(incFile, "\n\t);\n\n");
			}
		}
		fprintf(incFile, "\n");
	}

	// random initialization for shared rams
	if (m_bRndInit) {
		bool bFirst = true;

		for (size_t shIdx = 0; shIdx < pMod->m_shared.m_fieldList.size(); shIdx += 1) {
			CField * pShared = pMod->m_shared.m_fieldList[shIdx];

			if (pShared->m_queueW.AsInt() > 0) continue;

			if (bFirst) {
				fprintf(incFile, "#\tifndef _HTV\n");
				bFirst = false;
			}

			if (pShared->m_reset == "false" || pShared->m_reset == "" && !pMod->m_bResetShared) {
				fprintf(incFile, "\tvoid RndInit_S_%s()\n", pShared->m_name.c_str());
				fprintf(incFile, "\t{\n");
			} else {
				fprintf(incFile, "\tvoid ZeroInit_S_%s()\n", pShared->m_name.c_str());
				fprintf(incFile, "\t{\n");
			}

			string tabs = "\t\t";

			if (pShared->m_addr1W.AsInt() > 0) {
				bool bIsSigned = m_defineTable.FindStringIsSigned(pShared->m_addr1W.AsStr());
				fprintf(incFile, "\t\tfor (%s wrAddr1 = 0; wrAddr1 < (1 << (%s)); wrAddr1 += 1) {\n",
					bIsSigned ? "int" : "unsigned", pShared->m_addr1W.c_str());
				tabs += "\t";

				if (pShared->m_addr2W.AsInt() > 0) {
					bool bIsSigned = m_defineTable.FindStringIsSigned(pShared->m_addr2W.AsStr());
					fprintf(incFile, "\t\t\tfor (%s wrAddr2 = 0; wrAddr2 < (1 << (%s)); wrAddr2 += 1) {\n",
						bIsSigned ? "int" : "unsigned", pShared->m_addr2W.c_str());
					tabs += "\t";
				}

				string idxStr;
				int idxCnt = (int)pShared->m_dimenList.size();
				for (int i = 0; i < idxCnt; i += 1) {
					fprintf(incFile, "%sfor (int idx%d = 0; idx%d < %d; idx%d += 1) {\n", tabs.c_str(),
						i + 1, i + 1, pShared->m_dimenList[i].AsInt(), i + 1);
					idxStr += VA("[idx%d]", i + 1);
					tabs += "\t";
				}

				if (pShared->m_rdSelW.AsInt() > 0 || pShared->m_wrSelW.AsInt() > 0) {
					fprintf(incFile, "%sfor (int wrSel = 0; wrSel < %d; wrSel += 1) {\n", tabs.c_str(),
						1 << (pShared->m_rdSelW.AsInt() + pShared->m_wrSelW.AsInt()));
					tabs += "\t";
				}

				fprintf(incFile, "%s%s tmp;\n", tabs.c_str(), pShared->m_pType->m_typeName.c_str());

				GenStructInit(incFile, tabs, "tmp", pShared, idxCnt, false);

				if (pShared->m_rdSelW.AsInt() > 0 || pShared->m_wrSelW.AsInt() > 0)
					fprintf(incFile, "%sm__SHR__%s%s.write_mem_debug(wrSel, wrAddr1", tabs.c_str(), pShared->m_name.c_str(), idxStr.c_str());
				else
					fprintf(incFile, "%sm__SHR__%s%s.write_mem_debug(wrAddr1", tabs.c_str(), pShared->m_name.c_str(), idxStr.c_str());
				if (pShared->m_addr2W.AsInt() > 0)
					fprintf(incFile, ", wrAddr2");
				fprintf(incFile, ") = tmp;\n");

				if (pShared->m_rdSelW.AsInt() > 0 || pShared->m_wrSelW.AsInt() > 0) {
					tabs.erase(0, 1);
					fprintf(incFile, "%s}\n", tabs.c_str());
				}

				for (size_t i = 0; i < pShared->m_dimenList.size(); i += 1) {
					tabs.erase(0, 1);
					fprintf(incFile, "%s}\n", tabs.c_str());
				}

				if (pShared->m_addr2W.AsInt() > 0)
					fprintf(incFile, "\t\t\t}\n");

				fprintf(incFile, "\t\t}\n");
			} else {
				string idxStr;
				int idxCnt = (int)pShared->m_dimenList.size();
				for (int i = 0; i < idxCnt; i += 1) {
					fprintf(incFile, "%sfor (int idx%d = 0; idx%d < %d; idx%d += 1) {\n", tabs.c_str(),
						i + 1, i + 1, pShared->m_dimenList[i].AsInt(), i + 1);
					idxStr += VA("[idx%d]", i + 1);
					tabs += "\t";
				}

				bool bZero = !(pShared->m_reset == "false" || pShared->m_reset == "" && !pMod->m_bResetShared);
				GenStructInit(incFile, tabs, VA("r__SHR__%s%s", pShared->m_name.c_str(), idxStr.c_str()), pShared, idxCnt, bZero);

				for (size_t i = 0; i < pShared->m_dimenList.size(); i += 1) {
					tabs.erase(0, 1);
					fprintf(incFile, "%s}\n", tabs.c_str());
				}
			}
			fprintf(incFile, "\t}\n");
		}

		if (!bFirst)
			fprintf(incFile, "#\tendif\n\n");
	}

	fprintf(incFile, "\n");
	fprintf(incFile, "#\tifndef _HTV\n");
	fprintf(incFile, "\tstatic char const * m_pInstrNames[];\n");

	if (g_appArgs.IsInstrTraceEnabled())
		fprintf(incFile, "\tstatic char const * m_pHtCtrlNames[];\n");

	fprintf(incFile, "\tint m_htMonModId;\n");
	fprintf(incFile, "#\tendif\n");

	fprintf(incFile, "\n");
	fprintf(incFile, "\tSC_CTOR(CPers%s%s)\n",
		unitNameUc.c_str(), pModInst->m_instName.Uc().c_str());

	fprintf(incFile, "#\t\tif !defined(_HTV)\n");

	m_iplRefInit.Write(incFile);

	fprintf(incFile, "\n#\t\tendif\n");
	fprintf(incFile, "\t{\n");

	fprintf(incFile, "\t\tSC_METHOD(Pers%s%s_1x);\n",
		unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
	fprintf(incFile, "\t\tsensitive << i_clock1x.pos();\n");

	if (bNeedClk2x) {
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tSC_METHOD(Pers%s%s_2x);\n",
			unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
		fprintf(incFile, "\t\tsensitive << i_clock2x.pos();\n");
	}

	if (pMod->m_bContAssign) {
		fprintf(incFile, "\n");
		fprintf(incFile, "\t\tSC_METHOD(Pers%s%sCont);\n",
			unitNameUc.c_str(), pMod->m_modName.Uc().c_str());
		m_gblSenCont.Write(incFile);
	}

	if (bStateMachine) {
		fprintf(incFile, "\n");
		fprintf(incFile, "#\t\tifndef _HTV\n");
		if (bNeedClk2x)
			fprintf(incFile, "\t\tc_reset2x = true;\n");
		fprintf(incFile, "\t\tr_userReset = true;\n");
		for (int stgIdx = pMod->m_tsStg; stgIdx < pMod->m_wrStg; stgIdx += 1) {
			if (pMod->m_bHasThreads)
				fprintf(incFile, "\t\tr_t%d_htValid = false;\n", stgIdx);
		}
		m_iplCtorInit.Write(incFile);
		m_mifCtorInit.Write(incFile);
		m_strmCtorInit.Write(incFile);
		m_ihdCtorInit.Write(incFile);
		fprintf(incFile, "#\t\tendif\n");
	}

	if (m_bRndInit) {
		fprintf(incFile, "\n");
		fprintf(incFile, "#\t\tifndef _HTV\n");
		fprintf(incFile, "\t\t// Random variable initialization (-ri)\n");

		for (size_t shIdx = 0; shIdx < pMod->m_shared.m_fieldList.size(); shIdx += 1) {
			CField * pShared = pMod->m_shared.m_fieldList[shIdx];

			if (pShared->m_queueW.AsInt() > 0) continue;

			bool bZero = !(pShared->m_reset == "false" || pShared->m_reset == "" && !pMod->m_bResetShared);

			fprintf(incFile, "\t\t%sInit_S_%s();\n", bZero ? "Zero" : "Rnd", pShared->m_name.c_str());
		}

		//for (size_t ramIdx = 0; ramIdx < pMod->m_intGblList.size(); ramIdx += 1) {
		//	CRam &intRam = *pMod->m_intGblList[ramIdx];

		//	if (intRam.m_addr1W.AsInt() == 0) continue;

		//	fprintf(incFile, "\t\tRndInit_G_%s();\n", intRam.m_gblName.c_str());
		//}

		fprintf(incFile, "#\t\tendif\n");
	}

	string path("");
	if (pMod->m_instSet.GetInstCnt() > 0 && pMod->m_instSet.GetInst(0)->m_modPaths.size()) {
		size_t pos;
		path = pMod->m_instSet.GetInst(0)->m_modPaths[0];
		pos = path.find_last_of("/");
		if (pos < path.size()) path.erase(pos, path.size() - pos);
		pos = path.find_first_of(":/");
		if (pos < path.size()) path.erase(0, pos + 2);
		if (path.size()) path.append("/");
	}

	fprintf(incFile, "\n");
	fprintf(incFile, "#\t\tifndef _HTV\n");
	fprintf(incFile, "\t\tm_htMonModId = Ht::g_syscMon.RegisterModule(\"%s\", name(), %s_HTID_W, %s_INSTR_W, m_pInstrNames, %d);\n",
		(path + pMod->m_modName.Upper()).c_str(), pMod->m_modName.Upper().c_str(), pMod->m_modName.Upper().c_str(), (int)pMod->m_memPortList.size());
	fprintf(incFile, "#\t\tendif\n");

	fprintf(incFile, "\t}\n");

	if (g_appArgs.IsVcdUserEnabled() || g_appArgs.IsVcdAllEnabled()) {
		fprintf(incFile, "\n");
		fprintf(incFile, "#\tifndef _HTV\n");
		fprintf(incFile, "\tvoid start_of_simulation()\n");
		fprintf(incFile, "\t{\n");

		m_vcdSos.Write(incFile);

		fprintf(incFile, "\t}\n");
		fprintf(incFile, "#\tendif\n");
	}

	fprintf(incFile, "\n");
	fprintf(incFile, "#\tifndef _HTV\n");
	fprintf(incFile, "\tvoid end_of_simulation()\n");
	fprintf(incFile, "\t{\n");

	m_iplEosChecks.Write(incFile);
	m_mifAvlCntChk.Write(incFile);
	m_cxrAvlCntChk.Write(incFile);
	m_ohmAvlCntChk.Write(incFile);
	m_ohdAvlCntChk.Write(incFile);
	m_strmAvlCntChk.Write(incFile);

	fprintf(incFile, "\t}\n");
	fprintf(incFile, "#\tendif\n");

	fprintf(incFile, "};\n");

	m_iplBadDecl.Write(incFile);
	m_mifBadDecl.Write(incFile);
	m_strmBadDecl.Write(incFile);

	incFile.FileClose();
}

string
CDsnInfo::GenRamAddr(CModInst & modInst, CRam &ram, CHtCode *pCode, string accessSelW, string accessSelName, const char *pInStg, const char *pOutStg, bool bWrite, bool bNoSelectAssign)
{
	// construct address for ram access
	//   simple case is when there is a single thread group
	//   next case is multiple thread groups but only one has the private address variable
	//   complex case is multiple groups each having a private variable with the address name

	if (bWrite && ram.m_addr1Name.size() == 0)
		return string("0");

	//if (ram.m_addr1Name == "htId" && ram.m_addr2W.size() == 0)
	//	return string(pInStg) + "_htId";

	CModule &mod = *modInst.m_pMod;

	string accessName0;
	if (ram.m_addr0W.AsInt() > 0)
		accessName0 = string(pInStg) + "_htId";

	string accessName1;
	int cnt1 = 0;
	if (ram.m_addr1W.size() > 0) {
		if (ram.m_addr1Name == "htId") {
			if (ram.m_addr1W.AsInt() > 0)
				accessName1 = string(pInStg) + "_htId";
		} else {
			string fullName;
			CField const * pBaseField, *pLastField;
			if (mod.m_bHasThreads) {
				if (IsInFieldList(ram.m_lineInfo, ram.m_addr1Name, mod.m_threads.m_htPriv.m_fieldList, false, true,
					pBaseField, pLastField, &fullName)) {
					cnt1 += 1;
					accessName1 = string(pInStg) + "_htPriv." + fullName;//ram.m_addr1Name;
				}
			}

			if (IsInFieldList(ram.m_lineInfo, ram.m_addr1Name, mod.m_stage.m_fieldList, false, true, pBaseField, pLastField, 0)) {
				cnt1 += 1;
				accessName1 = "0";
			}

			if (cnt1 == 0) {
				ParseMsg(Error, ram.m_lineInfo, "did not find ram address name in private or stage variables, '%s'\n",
					ram.m_addr1Name.c_str());
				return "0";
			}
		}
	}

	string accessName;
	string accessName2;
	int cnt2 = 0;
	if (ram.m_addr2W.size() > 0) {
		if (ram.m_addr2Name.size() == 0) {
			ParseMsg(Error, ram.m_lineInfo, "addr2 parameter is missing\n");
			return "0";
		} else if (ram.m_addr2Name == "htId") {
			if (ram.m_addr2W.AsInt() > 0)
				accessName2 = string(pInStg) + "_htId";
		} else {
			CField const * pBaseField, *pLastField;
			if (mod.m_bHasThreads) {
				string fullName;
				if (IsInFieldList(ram.m_lineInfo, ram.m_addr2Name, mod.m_threads.m_htPriv.m_fieldList, false, true,
					pBaseField, pLastField, &fullName)) {
					cnt2 += 1;
					accessName2 = string(pInStg) + "_htPriv." + fullName;
				}
			}

			if (IsInFieldList(ram.m_lineInfo, ram.m_addr2Name, mod.m_stage.m_fieldList, false, true,
				pBaseField, pLastField, 0)) {
				cnt2 += 1;
				accessName2 = "0";
			}

			if (cnt2 == 0) {
				ParseMsg(Error, ram.m_lineInfo, "did not find addr2 name in private or stage variables, '%s'\n",
					ram.m_addr2Name.c_str());
				accessName = "0";
			}

			if (ram.m_addr1Name != "htId" && ram.m_addr2Name != "htId" && cnt1 != cnt2) {
				ParseMsg(Error, ram.m_lineInfo, "both addresses of ram must be in each thread group\n");
				accessName = "0";
			}
		}
	}

	if (accessName0.size() > 0) {
		if (accessName1.size() > 0 && accessName2.size() > 0)
			accessName = "(" + accessName0 + ", " + accessName1 + ", " + accessName2 + ")";

		else if (accessName1.size() > 0)
			accessName = "(" + accessName0 + ", " + accessName1 + ")";

		else if (accessName2.size() > 0)
			accessName = "(" + accessName0 + ", " + accessName2 + ")";

		else
			accessName = accessName0;

	} else {
		if (accessName1 == "0")
			accessName = accessName1;

		else if (accessName1.size() > 0 && accessName2.size() > 0)
			accessName = "(" + accessName1 + ", " + accessName2 + ")";

		else if (accessName1.size() > 0)
			accessName = accessName1;

		else if (accessName2.size() > 0)
			accessName = accessName2;
	}

	if (accessName.size() > 0) {
		if (bNoSelectAssign) {
			if (accessSelW.size() > 0)
				pCode->Append("\tsc_uint<%s> %s = %s;\n", accessSelW.c_str(), accessSelName.c_str(), accessName.c_str());
			else
				pCode->Append("\t%s = %s;\n", accessSelName.c_str(), accessName.c_str());
		}
		return accessName;
	}

	HtlAssert(0);

	return accessSelName;
}

int CDsnInfo::CountPhaseResetFanout(CModInst * pModInst)
{
	//CModule &mod = *modInst.m_pMod;

	int phaseCnt = 0;

	return phaseCnt;
}

void CDsnInfo::GenRamWrEn(CHtCode &code, char const * pTabs, string intfName, CRecord &ram)
{
	// mode: 0-all, 1-read, 2-write
	code.Append("%s\tstruct %s {\n", pTabs, intfName.c_str());

	for (size_t fieldIdx = 0; fieldIdx < ram.m_fieldList.size(); fieldIdx += 1) {
		CField * pField = ram.m_fieldList[fieldIdx];

		if (pField->m_bSrcWrite || pField->m_bMifWrite) {
			code.Append("%s\t\tbool\tm_%s%s;\n", pTabs, pField->m_name.c_str(), pField->m_dimenDecl.c_str());
		}
	}
	code.Append("%s\t};\n", pTabs);
	code.Append("\n");
}

bool
CDsnInfo::DimenIter(vector<CHtString> const &dimenList, vector<int> &refList)
{
	// refList must be same size as dimenList and all elements zero
	HtlAssert(dimenList.size() == refList.size());

	bool bMore;

	//if (dimenList.size() > 0) {
	//	bMore = true;
	//	refList[0] += 1;
	//	for (size_t i = 0; i < dimenList.size(); i += 1) {
	//		if (refList[i] < dimenList[i].AsInt())
	//			break;
	//		else {
	//			refList[i] = 0;
	//			if (i + 1 < dimenList.size())
	//				refList[i + 1] += 1;
	//			else
	//				bMore = false;
	//		}
	//	}
	//}
	//else
	//	bMore = false;

	if (dimenList.size() > 0) {
		int dimens = (int)dimenList.size();
		bMore = true;
		refList[dimens - 1] += 1;
		for (int i = dimens - 1; i >= 0; i -= 1) {
			if (refList[i] < dimenList[i].AsInt())
				break;
			else {
				refList[i] = 0;
				if (i - 1 >= 0)
					refList[i - 1] += 1;
				else
					bMore = false;
			}
		}
	}
	else
		bMore = false;

	return bMore;
}

string
CDsnInfo::IndexStr(vector<int> &refList, int startPos, int endPos, bool bParamStr)
{
	if (startPos == -1) {
		startPos = 0;
		endPos = (int)refList.size();
	}
	string index;
	for (int i = startPos; i < endPos; i += 1) {
		char buf[8];
		if (bParamStr) {
			if (i == startPos)
				sprintf(buf, "%d", refList[i]);
			else
				sprintf(buf, ", %d", refList[i]);
		} else
			sprintf(buf, "[%d]", refList[i]);
		index += buf;
	}
	return index;
}

void
CDsnInfo::GenPrimStateStatements(CModule &mod)
{
	if (mod.m_primStateList.size() == 0)
		return;

	m_psDecl.Append("\t// Statement for clocked primitives\n");

	for (size_t i = 0; i < mod.m_primStateList.size(); i += 1) {

		m_psInclude.Append("\n#include \"%s\"\n",
			mod.m_primStateList[i].m_include.c_str());

		m_psDecl.Append("\t%s %s;\n",
			mod.m_primStateList[i].m_type.c_str(),
			mod.m_primStateList[i].m_name.c_str());
	}

	m_psDecl.Append("\n");
}

void
CDsnInfo::GenModTDSUStatements(CModule &mod) // typedef/define/struct/union
{
	bool bFound = false;

	for (size_t i = 0; i < m_defineTable.size(); i += 1) {
		//	must be a module and match current module
		if (m_defineTable[i].m_scope.compare("module") == 0
			&& m_defineTable[i].m_modName.compare(mod.m_modName.c_str()) == 0) {

			if (!bFound) {
				bFound = true;

				m_tdsuDefines.Append("#if !defined(TOPOLOGY_HEADER)\n");
				m_tdsuDefines.Append("//////////////////////////////////\n");
				m_tdsuDefines.Append("// Module defines\n");
			}

			m_tdsuDefines.Append("\n#define\t%s\t\t%s",
				m_defineTable[i].m_name.c_str(),
				m_defineTable[i].m_value.c_str());
		}
	}

	if (bFound)
		m_tdsuDefines.Append("#endif\t\t// !TOPOLOGY_HEADER\n");
}
