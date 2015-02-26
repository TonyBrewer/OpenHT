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

void CDsnInfo::GenerateUnitTopFile()
{
	string fileName = g_appArgs.GetOutputFolder() + "/Pers" + m_unitName.Uc() + "Top.sc";

	CHtFile scFile(fileName, "w");

	GenerateBanner(scFile, fileName.c_str(), false);

	fprintf(scFile, "#define TOPOLOGY_HEADER\n");

	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;

		if (mod.m_bHostIntf) {
			// Host interface is two modules: hif and hti
			fprintf(scFile, "#include \"Ht.h\"\n");
			fprintf(scFile, "#include \"PersHif.h\"\n");
			fprintf(scFile, "#include \"Pers%sHti.h\"\n", m_unitName.Uc().c_str());

		} else {

			int prevInstId = -1;
			for (size_t modInstIdx = 0; modInstIdx < mod.m_modInstList.size(); modInstIdx += 1) {
				CModInst &modInst = mod.m_modInstList[modInstIdx];

				if (modInst.m_instParams.m_instId == prevInstId)
					continue;

				prevInstId = modInst.m_instParams.m_instId;

				string instIdStr = GenIndexStr(mod.m_instIdCnt > 1, "%d", modInst.m_instParams.m_instId);

				fprintf(scFile, "#include \"Pers%s%s%s.h\"\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str());

				if (mod.m_barrierList.size() > 0 && mod.m_modInstList.size() > 1)
					fprintf(scFile, "#include \"Pers%s%sBarCtl%s.h\"\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), instIdStr.c_str());
			}
		}
	}
	for (size_t mifInstIdx = 0; mifInstIdx < m_mifInstList.size(); mifInstIdx += 1) {
		CMifInst &mifInst = m_mifInstList[mifInstIdx];

		fprintf(scFile, "#include \"Pers%sMif%d.h\"\n", m_unitName.Uc().c_str(), mifInst.m_mifId);
	}
	fprintf(scFile, "#include \"Pers%sHta.h\"\n", m_unitName.Uc().c_str());
	for (size_t gvIdx = 0; gvIdx < m_ngvList.size(); gvIdx += 1) {
		CNgvInfo * pNgvInfo = m_ngvList[gvIdx];
		CRam * pNgv = pNgvInfo->m_modInfoList[0].m_pNgv;
		fprintf(scFile, "#include \"PersGbl%s.h\"\n", pNgv->m_gblName.Uc().c_str());
	}
	fprintf(scFile, "\n");

	fprintf(scFile, "SC_MODULE(CPers%sTop) {\n", m_unitName.Uc().c_str());
	fprintf(scFile, "\n");
	fprintf(scFile, "\tht_attrib(keep_hierarchy, CPers%sTop, \"true\");\n", m_unitName.Uc().c_str());
	fprintf(scFile, "\n");

	///////////////////////////
	// declared modules

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;

		if (mod.m_bHostIntf) {
			// Host interface is two modules: hif and hti
			fprintf(scFile, "\tCPersHif * pPersHif;\n");
			fprintf(scFile, "\tCPers%sHti * pPers%sHti;\n", m_unitName.Uc().c_str(), m_unitName.Uc().c_str());

		} else {

			for (size_t modInstIdx = 0; modInstIdx < mod.m_modInstList.size(); modInstIdx += 1) {
				CModInst &modInst = mod.m_modInstList[modInstIdx];

				if (modInst.m_replId > 0) continue;

				string replDeclStr = modInst.m_replCnt <= 1 ? "" : VA("[%d]", modInst.m_replCnt);

				fprintf(scFile, "\tCPers%s * pPers%s%s;\n",
					modInst.m_pMod->m_modName.Uc().c_str(),
					modInst.m_pMod->m_modName.Uc().c_str(), replDeclStr.c_str());
			}

			if (mod.m_barrierList.size() > 0 && mod.m_modInstList.size() > 1)
				fprintf(scFile, "\tCPers%sBarCtl * pPers%sBarCtl;\n",
				mod.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str());
		}
	}

	for (size_t mifInstIdx = 0; mifInstIdx < m_mifInstList.size(); mifInstIdx += 1) {
		CMifInst &mifInst = m_mifInstList[mifInstIdx];

		fprintf(scFile, "\tCPers%sMif%d * pPers%sMif%d;\n",
			m_unitName.Uc().c_str(), mifInst.m_mifId, m_unitName.Uc().c_str(), mifInst.m_mifId);
	}
	fprintf(scFile, "\tCPers%sHta * pPers%sHta;\n",
		m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
	for (size_t gvIdx = 0; gvIdx < m_ngvList.size(); gvIdx += 1) {
		CNgvInfo * pNgvInfo = m_ngvList[gvIdx];
		CRam * pNgv = pNgvInfo->m_modInfoList[0].m_pNgv;
		string replStr = pNgvInfo->m_ngvReplCnt > 1 ? VA("[%d]", pNgvInfo->m_ngvReplCnt) : "";
		fprintf(scFile, "\tCPersGbl%s * pPersGbl%s%s;\n",
			pNgv->m_gblName.Uc().c_str(), pNgv->m_gblName.Uc().c_str(), replStr.c_str());
	}
	fprintf(scFile, "\n");

	///////////////////////////
	// declared signals

	for (size_t i = 0; i < m_instIdList.size(); i += 1)
		fprintf(scFile, "\tsc_signal<uint8_t> instId%d;\n", (int)m_instIdList[i]);
	fprintf(scFile, "\n");

	for (int i = 0; i < m_maxReplCnt; i += 1)
		fprintf(scFile, "\tsc_signal<uint8_t> replId%d;\n", i);
	fprintf(scFile, "\n");

	fprintf(scFile, "\tSC_CTOR(CPers%sTop) {\n", m_unitName.Uc().c_str());
	fprintf(scFile, "\n");

	for (size_t i = 0; i < m_instIdList.size(); i += 1)
		fprintf(scFile, "\t\tinstId%d = %d;\n", (int)m_instIdList[i], (int)m_instIdList[i]);
	fprintf(scFile, "\n");

	for (int i = 0; i < m_maxReplCnt; i += 1)
		fprintf(scFile, "\t\treplId%d = %d;\n", i, i);
	fprintf(scFile, "\n");

	///////////////////////////
	// instantiated modules

	bool bHifFlag = false;
	bool bHtiFlag = false;
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {

		if (m_modList[modIdx]->m_bHostIntf)
			bHifFlag = true;
		else if (bHifFlag) {
			bHifFlag = false;
			bHtiFlag = true;
			modIdx -= 1;
		} else
			bHtiFlag = false;

		CModule &mod = *m_modList[modIdx];
		if (!mod.m_bIsUsed) continue;

		string modName;
		if (bHifFlag)
			modName = "PersHif";
		else if (bHtiFlag)
			modName = "Pers" + m_unitName.Uc() + "Hti";
		else
			modName = "Pers" + unitNameUc + mod.m_modName.Uc();

		string instNum = mod.m_modInstList.size() == 1 ? "" : "[0]";
		for (size_t modInstIdx = 0; modInstIdx < mod.m_modInstList.size(); modInstIdx += 1) {
			CModInst & modInst = mod.m_modInstList[modInstIdx];

			fprintf(scFile, "\t\tp%s%s = new C%s(\"%s%s\");\n",
				modName.c_str(), instNum.c_str(), modName.c_str(), modName.c_str(), instNum.c_str());

			if (!bHifFlag && !bHtiFlag) {
				fprintf(scFile, "\t\tp%s%s->i_instId(instId%d);\n",
					modName.c_str(), instNum.c_str(), modInst.m_instParams.m_instId);

				fprintf(scFile, "\t\tp%s%s->i_replId(replId%d);\n",
					modName.c_str(), instNum.c_str(), modInst.m_replId);
			}

			if (bHifFlag)
				fprintf(scFile, "\t\tp%s%s->i_htaToHif_assert(htaToHif_assert);\n",
				modName.c_str(), instNum.c_str());
			else if (!bHtiFlag && mod.m_bHasThreads)
				fprintf(scFile, "\t\tp%s%s->o_%sToHta_assert(%sToHta_assert%s);\n",
				modName.c_str(), instNum.c_str(),
				mod.m_modName.Lc().c_str(),
				mod.m_modName.Lc().c_str(), instNum.c_str());

			// generate memory interface
			if (mod.m_memPortList.size() > 0) {
				vector<int> &instMemPortList = modInst.m_instParams.m_memPortList;
				string modInstIdxStr = GenIndexStr(mod.m_modInstList.size() > 1, "%d", modInstIdx);

				if (bHtiFlag) {
					fprintf(scFile, "\n");
					break;
				}

				HtlAssert(mod.m_memPortList.size() == instMemPortList.size());

				for (size_t memPortIdx = 0; memPortIdx < mod.m_memPortList.size(); memPortIdx += 1) {

					CModMemPort &modMemPort = *mod.m_memPortList[memPortIdx];

					fprintf(scFile, "\t\tp%s%s->o_%sP%dToMif_reqRdy(%s%sP%dToMif%d_reqRdy);\n",
						modName.c_str(), instNum.c_str(),
						mod.m_modName.Lc().c_str(), (int)memPortIdx,
						mod.m_modName.Lc().c_str(), modInstIdxStr.c_str(), (int)memPortIdx, instMemPortList[memPortIdx]);
					fprintf(scFile, "\t\tp%s%s->o_%sP%dToMif_req(%s%sP%dToMif%d_req);\n",
						modName.c_str(), instNum.c_str(),
						mod.m_modName.Lc().c_str(), (int)memPortIdx,
						mod.m_modName.Lc().c_str(), modInstIdxStr.c_str(), (int)memPortIdx, instMemPortList[memPortIdx]);
					fprintf(scFile, "\t\tp%s%s->i_mifTo%sP%d_reqAvl(mif%dTo%s%sP%d_reqAvl);\n",
						modName.c_str(), instNum.c_str(),
						mod.m_modName.Uc().c_str(), (int)memPortIdx,
						instMemPortList[memPortIdx], mod.m_modName.Uc().c_str(), modInstIdxStr.c_str(), (int)memPortIdx);

					if (modMemPort.m_bRead) {
						fprintf(scFile, "\t\tp%s%s->i_mifTo%sP%d_rdRspRdy(mif%dTo%s%sP%d_rdRspRdy);\n",
							modName.c_str(), instNum.c_str(),
							mod.m_modName.Uc().c_str(), (int)memPortIdx,
							instMemPortList[memPortIdx], mod.m_modName.Uc().c_str(), modInstIdxStr.c_str(), (int)memPortIdx);
						fprintf(scFile, "\t\tp%s%s->i_mifTo%sP%d_rdRsp(mif%dTo%s%sP%d_rdRsp);\n",
							modName.c_str(), instNum.c_str(),
							mod.m_modName.Uc().c_str(), (int)memPortIdx,
							instMemPortList[memPortIdx], mod.m_modName.Uc().c_str(), modInstIdxStr.c_str(), (int)memPortIdx);
						fprintf(scFile, "\t\tp%s%s->o_%sP%dToMif_rdRspFull(%s%sP%dToMif%d_rdRspFull);\n",
							modName.c_str(), instNum.c_str(),
							mod.m_modName.Lc().c_str(), (int)memPortIdx,
							mod.m_modName.Lc().c_str(), modInstIdxStr.c_str(), (int)memPortIdx, instMemPortList[memPortIdx]);
					}

					if (modMemPort.m_bWrite/*mif.m_bMifWr*/) {
						fprintf(scFile, "\t\tp%s%s->i_mifTo%sP%d_wrRspRdy(mif%dTo%s%sP%d_wrRspRdy);\n",
							modName.c_str(), instNum.c_str(),
							mod.m_modName.Uc().c_str(), (int)memPortIdx,
							instMemPortList[memPortIdx], mod.m_modName.Uc().c_str(), modInstIdxStr.c_str(), (int)memPortIdx);
						fprintf(scFile, "\t\tp%s%s->i_mifTo%sP%d_wrRspTid(mif%dTo%s%sP%d_wrRspTid);\n",
							modName.c_str(), instNum.c_str(),
							mod.m_modName.Uc().c_str(), (int)memPortIdx,
							instMemPortList[memPortIdx], mod.m_modName.Uc().c_str(), modInstIdxStr.c_str(), (int)memPortIdx);
					}
				}
			}

			// Generate call/transfer/return interfaces
			for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

				if (bHifFlag) break;

				string replIdxStr = cxrIntf.m_instCnt <= 1 ? "" : VA("[%d]", cxrIntf.m_instIdx);

				if (cxrIntf.m_cxrDir == CxrIn) {

					fprintf(scFile, "\t\tp%s%s->i_%s_%sRdy%s(%s_%sRdy%s);\n",
						modName.c_str(), instNum.c_str(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetSignalNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetSignalIndexSrcToDst());

					if (cxrIntf.m_bCxrIntfFields) {
						fprintf(scFile, "\t\tp%s%s->i_%s_%s%s(%s_%s%s);\n",
							modName.c_str(), instNum.c_str(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
							cxrIntf.GetSignalNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetSignalIndexSrcToDst());
					}

					fprintf(scFile, "\t\tp%s%s->o_%s_%sAvl%s(%s_%sAvl%s);\n",
						modName.c_str(), instNum.c_str(),
						cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetSignalNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetSignalIndexDstToSrc());

					if (cxrIntf.m_pSrcMod->m_bGvIwComp) {
						fprintf(scFile, "\t\tp%s%s->i_%s_%sCompRdy%s(%s_%sCompRdy%s);\n",
							modName.c_str(), instNum.c_str(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
							cxrIntf.GetSignalNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetSignalIndexSrcToDst());
					}

				} else {

					fprintf(scFile, "\t\tp%s%s->o_%s_%sRdy%s(%s_%sRdy%s);\n",
						modName.c_str(), instNum.c_str(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetSignalNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetSignalIndexSrcToDst());

					if (cxrIntf.m_bCxrIntfFields)
						fprintf(scFile, "\t\tp%s%s->o_%s_%s%s(%s_%s%s);\n",
						modName.c_str(), instNum.c_str(),
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetSignalNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetSignalIndexSrcToDst());

					fprintf(scFile, "\t\tp%s%s->i_%s_%sAvl%s(%s_%sAvl%s);\n",
						modName.c_str(), instNum.c_str(),
						cxrIntf.GetPortNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
						cxrIntf.GetSignalNameDstToSrcLc(), cxrIntf.GetIntfName(), cxrIntf.GetSignalIndexDstToSrc());

					if (mod.m_bGvIwComp && cxrIntf.m_pDstMod->m_modName.AsStr() != "hif") {
						fprintf(scFile, "\t\tp%s%s->o_%s_%sCompRdy%s(%s_%sCompRdy%s);\n",
							modName.c_str(), instNum.c_str(),
							cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
							cxrIntf.GetSignalNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetSignalIndexSrcToDst());
					}
				}
			}

			// generate new global ram interfaces
			for (size_t ngvIdx = 0; ngvIdx < mod.m_ngvList.size(); ngvIdx += 1) {
				CRam * pNgv = mod.m_ngvList[ngvIdx];
				CNgvInfo * pNgvInfo = pNgv->m_pNgvInfo;

				if (pNgvInfo->m_ngvReplCnt == 1) continue;

				vector<int> refList(pNgv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					// global variable module I/O
					if (pNgv->m_bWriteForInstrWrite) { // Instruction read
						if (mod.m_threads.m_htIdW.AsInt() > 0) {

							fprintf(scFile, "\t\tp%s%s->o_%sTo%s_iwHtId%s(%sTo%s_iwHtId%s%s);\n",
								modName.c_str(), instNum.c_str(),
								mod.m_modName.Lc().c_str(), pNgv->m_gblName.Uc().c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pNgv->m_gblName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());

						}

						fprintf(scFile, "\t\tp%s%s->o_%sTo%s_iwComp%s(%sTo%s_iwComp%s%s);\n",
							modName.c_str(), instNum.c_str(),
							mod.m_modName.Lc().c_str(), pNgv->m_gblName.Uc().c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), pNgv->m_gblName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());

						fprintf(scFile, "\t\tp%s%s->o_%sTo%s_iwData%s(%sTo%s_iwData%s%s);\n",
							modName.c_str(), instNum.c_str(),
							mod.m_modName.Lc().c_str(), pNgv->m_gblName.Uc().c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), pNgv->m_gblName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
					}

					if (pNgv->m_bReadForInstrRead) { // Instruction read
						if (pNgvInfo->m_atomicMask != 0) {

							fprintf(scFile, "\t\tp%s%s->i_%sTo%s_ifWrEn%s(%sTo%s_ifWrEn%s%s);\n",
								modName.c_str(), instNum.c_str(),
								pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());

							if (mod.m_threads.m_htIdW.AsInt() > 0) {

								fprintf(scFile, "\t\tp%s%s->i_%sTo%s_ifHtId%s(%sTo%s_ifHtId%s%s);\n",
									modName.c_str(), instNum.c_str(),
									pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
									pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
							}

							fprintf(scFile, "\t\tp%s%s->i_%sTo%s_ifData%s(%sTo%s_ifData%s%s);\n",
								modName.c_str(), instNum.c_str(),
								pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
						}
					}

					if (pNgv->m_bReadForInstrRead || pNgv->m_bReadForMifWrite) {

						fprintf(scFile, "\t\tp%s%s->i_%sTo%s_wrEn%s(%sTo%s_wrEn%s%s);\n",
							modName.c_str(), instNum.c_str(),
							pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());

						if (pNgv->m_addrW > 0) {
							fprintf(scFile, "\t\tp%s%s->i_%sTo%s_wrAddr%s(%sTo%s_wrAddr%s%s);\n",
								modName.c_str(), instNum.c_str(),
								pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
						}

						fprintf(scFile, "\t\tp%s%s->i_%sTo%s_wrData%s(%sTo%s_wrData%s%s);\n",
							modName.c_str(), instNum.c_str(),
							pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
					}

					if (pNgv->m_bWriteForInstrWrite) {

						fprintf(scFile, "\t\tp%s%s->i_%sTo%s_iwCompRdy%s(%sTo%s_iwCompRdy%s%s);\n",
							modName.c_str(), instNum.c_str(),
							pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());

						if (mod.m_threads.m_htIdW.AsInt() > 0) {
							fprintf(scFile, "\t\tp%s%s->i_%sTo%s_iwCompHtId%s(%sTo%s_iwCompHtId%s%s);\n",
								modName.c_str(), instNum.c_str(),
								pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
						}

						fprintf(scFile, "\t\tp%s%s->i_%sTo%s_iwCompData%s(%sTo%s_iwCompData%s%s);\n",
							modName.c_str(), instNum.c_str(),
							pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
					}

					if (pNgv->m_bWriteForMifRead) { // Memory
						if (mod.m_mif.m_mifRd.m_rspGrpW.AsInt() > 0) {
							fprintf(scFile, "\t\tp%s%s->o_%sTo%s_mwGrpId%s(%sTo%s_mwGrpId%s%s);\n",
								modName.c_str(), instNum.c_str(),
								mod.m_modName.Lc().c_str(), pNgv->m_gblName.Uc().c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pNgv->m_gblName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
						}

						fprintf(scFile, "\t\tp%s%s->o_%sTo%s_mwData%s(%sTo%s_mwData%s%s);\n",
							modName.c_str(), instNum.c_str(),
							mod.m_modName.Lc().c_str(), pNgv->m_gblName.Uc().c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), pNgv->m_gblName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());

						fprintf(scFile, "\t\tp%s%s->i_%sTo%s_mwCompRdy%s(%sTo%s_mwCompRdy%s%s);\n",
							modName.c_str(), instNum.c_str(),
							pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());

						if (mod.m_mif.m_mifRd.m_rspGrpW.AsInt() > 0) {
							fprintf(scFile, "\t\tp%s%s->i_%sTo%s_mwCompGrpId%s(%sTo%s_mwCompGrpId%s%s);\n",
								modName.c_str(), instNum.c_str(),
								pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								pNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
						}
					}
				} while (DimenIter(pNgv->m_dimenList, refList));
			}

			// generate messaging interfaces for auto connected
			for (size_t msgIdx = 0; msgIdx < mod.m_msgIntfList.size(); msgIdx += 1) {
				CMsgIntf * pMsgIntf = mod.m_msgIntfList[msgIdx];

				if (!pMsgIntf->m_bAutoConn) continue;

				vector<CHtString> dimenList;
				int fanCntIdx = -1;
				if (pMsgIntf->m_dimen.AsInt() > 0) {
					dimenList.push_back(pMsgIntf->m_dimen);
				}
				if (pMsgIntf->m_fanCnt.AsInt() > 0) {
					fanCntIdx = (int)dimenList.size();
					dimenList.push_back(pMsgIntf->m_fanCnt);
				}

				if (pMsgIntf->m_dir == "in") {

					vector<int> portRefList(dimenList.size());
					do {
						string portIdx = IndexStr(portRefList);
						vector<int> signalRefList;

						int refListIdx = 0;
						if (pMsgIntf->m_dimen.AsInt() > 0) {
							signalRefList.push_back(portRefList[0]);
							refListIdx += 1;
						}

						int dstReplCnt = mod.m_modInstList.size();
						int dstFanout = pMsgIntf->m_fanCnt.AsInt() > 0 ? pMsgIntf->m_fanCnt.AsInt() : 1;
						int dstReplIdx = modInstIdx;
						int dstIntfIdx = fanCntIdx < 0 ? 0 : portRefList[fanCntIdx];

						int srcFanout = pMsgIntf->m_srcFanout;

						int dstTotalCnt = dstFanout * dstReplCnt;
						int srcTotalCnt = pMsgIntf->m_srcFanout * pMsgIntf->m_srcReplCnt;

						if (srcTotalCnt > 1) {
							assert(dstTotalCnt % srcTotalCnt == 0);
							if (srcFanout == 1)
								signalRefList.push_back((dstReplIdx * dstFanout + dstIntfIdx) % srcTotalCnt);
							else
								signalRefList.push_back((dstIntfIdx * dstReplCnt + dstReplIdx) % srcTotalCnt);
						}

						string signalIdx = IndexStr(signalRefList);

						fprintf(scFile, "\t\tp%s%s->i_%sToAu_%sMsgRdy%s(%sToAu_%sMsgRdy%s);\n",
							modName.c_str(), instNum.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), signalIdx.c_str());

						fprintf(scFile, "\t\tp%s%s->i_%sToAu_%sMsg%s(%sToAu_%sMsg%s);\n",
							modName.c_str(), instNum.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
							pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), signalIdx.c_str());

						if (pMsgIntf->m_queueW.AsInt() > 0)
							fprintf(scFile, "\t\tp%s%s->o_auTo%s_%sMsgFull%s(auTo%s_%sMsgFull%s);\n",
							modName.c_str(), instNum.c_str(),
							pMsgIntf->m_outModName.Uc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
							pMsgIntf->m_outModName.Uc().c_str(), pMsgIntf->m_name.c_str(), signalIdx.c_str());

					} while (DimenIter(dimenList, portRefList));

				} else {
					// message output interface

					vector<int> portRefList(dimenList.size());
					do {
						string portIdx = IndexStr(portRefList);
						vector<int> signalRefList;

						int refListIdx = 0;
						if (pMsgIntf->m_dimen.AsInt() > 0) {
							signalRefList.push_back(portRefList[0]);
							refListIdx += 1;
						}

						int srcReplCnt = pMsgIntf->m_srcReplCnt;
						int srcFanout = pMsgIntf->m_fanCnt.AsInt() > 0 ? pMsgIntf->m_fanCnt.AsInt() : 1;
						int srcReplIdx = modInstIdx;
						int srcIntfIdx = fanCntIdx < 0 ? 0 : portRefList[fanCntIdx];

						if (srcReplCnt > 1 || srcFanout > 1)
							signalRefList.push_back(srcFanout * srcReplIdx + srcIntfIdx);

						string signalIdx = IndexStr(signalRefList);

						fprintf(scFile, "\t\tp%s%s->o_%sToAu_%sMsgRdy%s(%sToAu_%sMsgRdy%s);\n",
							modName.c_str(), instNum.c_str(),
							mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
							mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), signalIdx.c_str());

						fprintf(scFile, "\t\tp%s%s->o_%sToAu_%sMsg%s(%sToAu_%sMsg%s);\n",
							modName.c_str(), instNum.c_str(),
							mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
							mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), signalIdx.c_str());

						if (pMsgIntf->m_bInboundQueue)
							fprintf(scFile, "\t\tp%s%s->i_auTo%s_%sMsgFull%s(auTo%s_%sMsgFull%s);\n",
							modName.c_str(), instNum.c_str(),
							mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
							mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), signalIdx.c_str());

					} while (DimenIter(dimenList, portRefList));

				}
			}

			// generate messaging interfaces for HTI connected
			for (size_t msgIdx = 0; msgIdx < mod.m_msgIntfList.size(); msgIdx += 1) {
				CMsgIntf * pMsgIntf = mod.m_msgIntfList[msgIdx];

				if (pMsgIntf->m_bAutoConn) continue;

				vector<CHtString> dimenList;
				int fanCntIdx = -1;
				if (pMsgIntf->m_dimen.AsInt() > 0) {
					dimenList.push_back(pMsgIntf->m_dimen);
				}
				if (pMsgIntf->m_fanCnt.AsInt() > 0) {
					fanCntIdx = (int)dimenList.size();
					dimenList.push_back(pMsgIntf->m_fanCnt);
				}

				if (pMsgIntf->m_dir == "in") {

					vector<int> portRefList(dimenList.size());
					do {
						string portIdx = IndexStr(portRefList);
						vector<int> signalRefList;

						int refListIdx = 0;
						if (pMsgIntf->m_dimen.AsInt() > 0) {
							signalRefList.push_back(portRefList[0]);
							refListIdx += 1;
						}

						int dstReplCnt = mod.m_modInstList.size();
						int dstFanout = pMsgIntf->m_fanCnt.AsInt() > 0 ? pMsgIntf->m_fanCnt.AsInt() : 1;
						int dstReplIdx = modInstIdx;
						int dstIntfIdx = fanCntIdx < 0 ? 0 : portRefList[fanCntIdx];

						int srcFanout = pMsgIntf->m_srcFanout;

						int dstTotalCnt = dstFanout * dstReplCnt;
						int srcTotalCnt = pMsgIntf->m_srcFanout * pMsgIntf->m_srcReplCnt;

						if (srcTotalCnt > 1) {
							assert(dstTotalCnt % srcTotalCnt == 0);
							if (srcFanout == 1)
								signalRefList.push_back((dstReplIdx * dstFanout + dstIntfIdx) % srcTotalCnt);
							else
								signalRefList.push_back((dstIntfIdx * dstReplCnt + dstReplIdx) % srcTotalCnt);
						}

						string signalIdx = IndexStr(signalRefList);

						CHtString &modInstName = mod.m_modInstList[modInstIdx].m_instName;

						if (pMsgIntf->m_bAeConn) {

							fprintf(scFile, "\t\tp%s%s->i_%sToAu_%sMsgRdy%s(i_aeTo%s_%sMsgRdy%s);\n",
								modName.c_str(), instNum.c_str(),
								pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
								modInstName.Uc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str());

							fprintf(scFile, "\t\tp%s%s->i_%sToAu_%sMsg%s(i_aeTo%s_%sMsg%s);\n",
								modName.c_str(), instNum.c_str(),
								pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
								modInstName.Uc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str());

							if (pMsgIntf->m_queueW.AsInt() > 0)
								fprintf(scFile, "\t\tp%s%s->o_auTo%s_%sMsgFull%s(o_%sToAe_%sMsgFull%s);\n",
								modName.c_str(), instNum.c_str(),
								pMsgIntf->m_outModName.Uc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
								modInstName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str());
						} else {

							int unitIdx = 0;
							int msgDimenCnt = pMsgIntf->m_dimen.AsInt() > 0 ? pMsgIntf->m_dimen.AsInt() : 1;
							int msgDimenIdx = pMsgIntf->m_dimen.AsInt() > 0 ? portRefList[0] : 0;

							int msgIntfInstIdx = ((unitIdx * dstReplCnt + dstReplIdx) * dstFanout + dstIntfIdx) * msgDimenCnt + msgDimenIdx;

							CMsgIntfConn * pConn = pMsgIntf->m_msgIntfInstList[msgIntfInstIdx][0];
							int outModInstIdx = max(0, pConn->m_outMsgIntf.m_replIdx);
							CHtString & outModInstName = pConn->m_outMsgIntf.m_pMod->m_modInstList[outModInstIdx].m_instName;

							fprintf(scFile, "\t\tp%s%s->i_%sToAu_%sMsgRdy%s(%sToAu_%sMsgRdy%s);\n",
								modName.c_str(), instNum.c_str(),
								pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
								outModInstName.Lc().c_str(), pMsgIntf->m_name.c_str(), signalIdx.c_str());

							fprintf(scFile, "\t\tp%s%s->i_%sToAu_%sMsg%s(%sToAu_%sMsg%s);\n",
								modName.c_str(), instNum.c_str(),
								pMsgIntf->m_outModName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
								outModInstName.Lc().c_str(), pMsgIntf->m_name.c_str(), signalIdx.c_str());

							if (pMsgIntf->m_queueW.AsInt() > 0)
								fprintf(scFile, "\t\tp%s%s->o_auTo%s_%sMsgFull%s(%sToAu_%sMsgFull%s);\n",
								modName.c_str(), instNum.c_str(),
								pMsgIntf->m_outModName.Uc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
								outModInstName.Lc().c_str(), pMsgIntf->m_name.c_str(), signalIdx.c_str());

						}
					} while (DimenIter(dimenList, portRefList));

				} else {
					// message output interface

					vector<int> portRefList(dimenList.size());
					do {
						string portIdx = IndexStr(portRefList);
						vector<int> signalRefList;

						int refListIdx = 0;
						if (pMsgIntf->m_dimen.AsInt() > 0) {
							signalRefList.push_back(portRefList[0]);
							refListIdx += 1;
						}

						int srcReplCnt = pMsgIntf->m_srcReplCnt;
						int srcFanout = pMsgIntf->m_fanCnt.AsInt() > 0 ? pMsgIntf->m_fanCnt.AsInt() : 1;
						int srcReplIdx = modInstIdx;
						int srcIntfIdx = fanCntIdx < 0 ? 0 : portRefList[fanCntIdx];

						if (srcReplCnt > 1 || srcFanout > 1)
							signalRefList.push_back(srcFanout * srcReplIdx + srcIntfIdx);

						string signalIdx = IndexStr(signalRefList);

						CHtString &modInstName = mod.m_modInstList[modInstIdx].m_instName;

						if (pMsgIntf->m_bAeConn) {
							fprintf(scFile, "\t\tp%s%s->o_%sToAu_%sMsgRdy%s(o_%sToAe_%sMsgRdy%s);\n",
								modName.c_str(), instNum.c_str(),
								mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
								modInstName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str());

							fprintf(scFile, "\t\tp%s%s->o_%sToAu_%sMsg%s(o_%sToAe_%sMsg%s);\n",
								modName.c_str(), instNum.c_str(),
								mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
								modInstName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str());

							if (pMsgIntf->m_bInboundQueue)
								fprintf(scFile, "\t\tp%s%s->i_auTo%s_%sMsgFull%s(i_aeTo%s_%sMsgFull%s);\n",
								modName.c_str(), instNum.c_str(),
								mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
								modInstName.Uc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str());
						} else {
							fprintf(scFile, "\t\tp%s%s->o_%sToAu_%sMsgRdy%s(%sToAu_%sMsgRdy%s);\n",
								modName.c_str(), instNum.c_str(),
								mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
								modInstName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str());

							fprintf(scFile, "\t\tp%s%s->o_%sToAu_%sMsg%s(%sToAu_%sMsg%s);\n",
								modName.c_str(), instNum.c_str(),
								mod.m_modName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
								modInstName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str());

							if (pMsgIntf->m_bInboundQueue)
								fprintf(scFile, "\t\tp%s%s->i_auTo%s_%sMsgFull%s(%sToAu_%sMsgFull%s);\n",
								modName.c_str(), instNum.c_str(),
								mod.m_modName.Uc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str(),
								modInstName.Lc().c_str(), pMsgIntf->m_name.c_str(), portIdx.c_str());
						}
					} while (DimenIter(dimenList, portRefList));
				}
			}

			if (mod.m_barrierList.size() > 0 && mod.m_modInstList.size() > 1) {
				for (size_t barIdx = 0; barIdx < mod.m_barrierList.size(); barIdx += 1) {
					CBarrier * pBar = mod.m_barrierList[barIdx];

					fprintf(scFile, "\t\tp%s%s->o_%sToBarCtl_bar%sEnter(%sToBarCtl_bar%sEnter%s);\n",
						modName.c_str(), instNum.c_str(),
						mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(),
						mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(), instNum.c_str());
					if (pBar->m_barIdW.AsInt() > 0)
						fprintf(scFile, "\t\tp%s%s->o_%sToBarCtl_bar%sId(%sToBarCtl_bar%sId%s);\n",
						modName.c_str(), instNum.c_str(),
						mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(),
						mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(), instNum.c_str());
					fprintf(scFile, "\t\tp%s%s->o_%sToBarCtl_bar%sReleaseCnt(%sToBarCtl_bar%sReleaseCnt%s);\n",
						modName.c_str(), instNum.c_str(),
						mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(),
						mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(), instNum.c_str());
					fprintf(scFile, "\t\tp%s%s->i_barCtlTo%s_bar%sRelease(barCtlTo%s_bar%sRelease);\n",
						modName.c_str(), instNum.c_str(),
						mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str(),
						mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str());
					if (pBar->m_barIdW.AsInt() > 0)
						fprintf(scFile, "\t\tp%s%s->i_barCtlTo%s_bar%sId(barCtlTo%s_bar%sId);\n",
						modName.c_str(), instNum.c_str(),
						mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str(),
						mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str());
				}
			}

			fprintf(scFile, "\n");

			if (instNum.size() > 0)
				instNum[instNum.size() - 2] += 1;
		}

		if (mod.m_barrierList.size() > 0 && mod.m_modInstList.size() > 1) {

			fprintf(scFile, "\t\tp%sBarCtl = new C%sBarCtl(\"%sBarCtl\");\n",
				modName.c_str(), modName.c_str(), modName.c_str());

			for (size_t barIdx = 0; barIdx < mod.m_barrierList.size(); barIdx += 1) {
				CBarrier * pBar = mod.m_barrierList[barIdx];

				string instNum = mod.m_modInstList.size() == 1 ? "" : "[0]";
				for (size_t modInstIdx = 0; modInstIdx < mod.m_modInstList.size(); modInstIdx += 1) {

					fprintf(scFile, "\t\tp%sBarCtl->i_%sToBarCtl_bar%sEnter%s(%sToBarCtl_bar%sEnter%s);\n",
						modName.c_str(),
						mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(), instNum.c_str(),
						mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(), instNum.c_str());
					if (pBar->m_barIdW.AsInt() > 0)
						fprintf(scFile, "\t\tp%sBarCtl->i_%sToBarCtl_bar%sId%s(%sToBarCtl_bar%sId%s);\n",
						modName.c_str(),
						mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(), instNum.c_str(),
						mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(), instNum.c_str());
					fprintf(scFile, "\t\tp%sBarCtl->i_%sToBarCtl_bar%sReleaseCnt%s(%sToBarCtl_bar%sReleaseCnt%s);\n",
						modName.c_str(),
						mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(), instNum.c_str(),
						mod.m_modName.Lc().c_str(), pBar->m_name.Uc().c_str(), instNum.c_str());

					if (instNum.size() > 0)
						instNum[instNum.size() - 2] += 1;
				}

				fprintf(scFile, "\t\tp%sBarCtl->o_barCtlTo%s_bar%sRelease(barCtlTo%s_bar%sRelease);\n",
					modName.c_str(),
					mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str(),
					mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str());
				if (pBar->m_barIdW.AsInt() > 0)
					fprintf(scFile, "\t\tp%sBarCtl->o_barCtlTo%s_bar%sId(barCtlTo%s_bar%sId);\n",
					modName.c_str(),
					mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str(),
					mod.m_modName.Uc().c_str(), pBar->m_name.Uc().c_str());
			}

			fprintf(scFile, "\n");
		}
	}

	// generate memory interface module
	for (size_t mifInstIdx = 0; mifInstIdx < m_mifInstList.size(); mifInstIdx += 1) {
		CMifInst &mifInst = m_mifInstList[mifInstIdx];

		int instMifId = mifInst.m_mifId;

		char mifNum[16];
		sprintf(mifNum, "%d", instMifId);

		string modName = "Pers" + m_unitName.Uc() + "Mif" + mifNum;

		fprintf(scFile, "\t\tp%s = new C%s(\"%s\");\n",
			modName.c_str(), modName.c_str(), modName.c_str());

		//for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		//	CModule &mod = *m_modList[modIdx];
		//	if (!mod.m_bIsUsed) continue;

		//	if (mod.m_mif.m_bMif) {
		//		CMif &mif = mod.m_mif;

		//		string instNum = mod.m_modInstList.size() == 1 ? "" : "0";
		//		for (int modInstIdx = 0; modInstIdx < (int)mod.m_modInstList.size(); modInstIdx += 1) {
		//			CModInst &modInst = mod.m_modInstList[modInstIdx];

		//			size_t memPortIdx;
		//			for (memPortIdx = 0; memPortIdx < mod.m_memPortList.size(); memPortIdx += 1)
		//				if (modInst.m_instParams.m_memPortList[memPortIdx] == instMifId)
		//					break;

		//			if (memPortIdx == mod.m_memPortList.size())
		//				continue;

		//			string replIdStr = GenIndexStr(modInst.m_replCnt > 1, "%d", modInst.m_replId);
		//			string modInstIdxStr = GenIndexStr(mod.m_modInstList.size() > 1, "%d", modInstIdx);

		//			// generate memory interface
		//			fprintf(scFile, "\t\tp%s->i_%s%sToMif_reqRdy(%s%sToMif%d_reqRdy);\n",
		//				modName.c_str(),
		//				mod.m_modName.Lc().c_str(), replIdStr.c_str(),
		//				mod.m_modName.Lc().c_str(), modInstIdxStr.c_str(), instMifId);
		//			fprintf(scFile, "\t\tp%s->i_%s%sToMif_req(%s%sToMif%d_req);\n",
		//				modName.c_str(),
		//				mod.m_modName.Lc().c_str(), replIdStr.c_str(), 
		//				mod.m_modName.Lc().c_str(), modInstIdxStr.c_str(), instMifId);
		//			fprintf(scFile, "\t\tp%s->o_mifTo%s%s_reqAvl(mif%dTo%s%s_reqAvl);\n",
		//				modName.c_str(),
		//				mod.m_modName.Uc().c_str(), replIdStr.c_str(), 
		//				instMifId, mod.m_modName.Uc().c_str(), modInstIdxStr.c_str());

		//			if (mif.m_bMifRd) {
		//				fprintf(scFile, "\t\tp%s->o_mifTo%s%s_rdRspRdy(mif%dTo%s%s_rdRspRdy);\n",
		//					modName.c_str(),
		//					mod.m_modName.Uc().c_str(), replIdStr.c_str(), 
		//					instMifId, mod.m_modName.Uc().c_str(), modInstIdxStr.c_str());
		//				fprintf(scFile, "\t\tp%s->o_mifTo%s%s_rdRsp(mif%dTo%s%s_rdRsp);\n",
		//					modName.c_str(),
		//					mod.m_modName.Uc().c_str(), replIdStr.c_str(), 
		//					instMifId, mod.m_modName.Uc().c_str(), modInstIdxStr.c_str());
		//			}

		//			if (mif.m_bMifWr) {
		//				fprintf(scFile, "\t\tp%s->o_mifTo%s%s_wrRspRdy(mif%dTo%s%s_wrRspRdy);\n",
		//					modName.c_str(),
		//					mod.m_modName.Uc().c_str(), replIdStr.c_str(), 
		//					instMifId, mod.m_modName.Uc().c_str(), modInstIdxStr.c_str());
		//				fprintf(scFile, "\t\tp%s->o_mifTo%s%s_wrRspTid(mif%dTo%s%s_wrRspTid);\n",
		//					modName.c_str(),
		//					mod.m_modName.Uc().c_str(), replIdStr.c_str(), 
		//					instMifId, mod.m_modName.Uc().c_str(), modInstIdxStr.c_str());
		//			}

		//			if (instNum.size() > 0)
		//				instNum[instNum.size()-1] += 1;
		//		}
		//	}
		//}

		//// now xbar ports
		//fprintf(scFile, "\t\tp%s->i_xbar%dToMif_rdRsp(i_xbar%dToMif%d_rdRsp);\n",
		//	modName.c_str(), instMifId, instMifId, instMifId); 
		//fprintf(scFile, "\t\tp%s->i_xbar%dToMif_rdRspRdy(i_xbar%dToMif%d_rdRspRdy);\n",
		//	modName.c_str(), instMifId, instMifId, instMifId);
		//fprintf(scFile, "\t\tp%s->i_xbar%dToMif_reqFull(i_xbar%dToMif%d_reqFull);\n",
		//	modName.c_str(), instMifId, instMifId, instMifId);
		//fprintf(scFile, "\t\tp%s->i_xbar%dToMif_wrRspRdy(i_xbar%dToMif%d_wrRspRdy);\n",
		//	modName.c_str(), instMifId, instMifId, instMifId);
		//fprintf(scFile, "\t\tp%s->i_xbar%dToMif_wrRspTid(i_xbar%dToMif%d_wrRspTid);\n",
		//	modName.c_str(), instMifId, instMifId, instMifId);

		//fprintf(scFile, "\t\tp%s->o_mifToXbar%d_rdRspFull(o_mif%dToXbar%d_rdRspFull);\n",
		//	modName.c_str(), instMifId, instMifId, instMifId);
		//fprintf(scFile, "\t\tp%s->o_mifToXbar%d_req(o_mif%dToXbar%d_req);\n",
		//	modName.c_str(), instMifId, instMifId, instMifId);
		//fprintf(scFile, "\t\tp%s->o_mifToXbar%d_reqRdy(o_mif%dToXbar%d_reqRdy);\n",
		//	modName.c_str(), instMifId, instMifId, instMifId);
		//fprintf(scFile, "\t\tp%s->o_mifToXbar%d_wrRspFull(o_mif%dToXbar%d_wrRspFull);\n",
		//	modName.c_str(), instMifId, instMifId, instMifId);

		fprintf(scFile, "\n");
	}

	// generate Ht Assert module
	string modName = "Pers" + m_unitName.Uc() + "Hta";

	fprintf(scFile, "\t\tp%s = new C%s(\"%s\");\n",
		modName.c_str(), modName.c_str(), modName.c_str());

	for (size_t instIdx = 0; instIdx < m_dsnInstList.size(); instIdx += 1) {
		CModInst & modInst = m_dsnInstList[instIdx];
		if (!modInst.m_pMod->m_bIsUsed) continue;

		if (!modInst.m_pMod->m_bHasThreads && !modInst.m_pMod->m_bHostIntf)
			continue;

		string replIdxStr = modInst.m_replCnt <= 1 ? "" : VA("[%d]", modInst.m_replId);

		// generate memory interface
		if (modInst.m_pMod->m_bHostIntf)
			fprintf(scFile, "\t\tp%s->o_htaTo%s_assert%s(htaTo%s_assert%s);\n",
			modName.c_str(),
			modInst.m_pMod->m_modName.Uc().c_str(), replIdxStr.c_str(),
			modInst.m_pMod->m_modName.Uc().c_str(), replIdxStr.c_str());
		else
			fprintf(scFile, "\t\tp%s->i_%sToHta_assert%s(%sToHta_assert%s);\n",
			modName.c_str(),
			modInst.m_pMod->m_modName.Lc().c_str(), replIdxStr.c_str(),
			modInst.m_pMod->m_modName.Lc().c_str(), replIdxStr.c_str());
	}
	fprintf(scFile, "\n");

	for (size_t gvIdx = 0; gvIdx < m_ngvList.size(); gvIdx += 1) {
		vector<CNgvModInfo> &ngvModInfoList = m_ngvList[gvIdx]->m_modInfoList;
		CNgvInfo * pNgvInfo = m_ngvList[gvIdx];
		CRam * pNgv = pNgvInfo->m_modInfoList[0].m_pNgv;

		string modName = VA("PersGbl%s", pNgv->m_gblName.Uc().c_str());

		for (int replIdx = 0; replIdx < pNgvInfo->m_ngvReplCnt; replIdx += 1) {
			string instNum = pNgvInfo->m_ngvReplCnt > 1 ? VA("[%d]", replIdx) : "";

			fprintf(scFile, "\t\tpPersGbl%s%s = new CPersGbl%s(\"PersGbl%s%s\");\n",
				pNgv->m_gblName.Uc().c_str(), instNum.c_str(),
				pNgv->m_gblName.Uc().c_str(),
				pNgv->m_gblName.Uc().c_str(), instNum.c_str());

			if (pNgvInfo->m_ngvReplCnt <= 1) {
				fprintf(scFile, "\n");
				continue;
			}

			for (size_t modIdx = 0; modIdx < ngvModInfoList.size(); modIdx += 1) {
				CModule & mod = *ngvModInfoList[modIdx].m_pMod;
				CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

				vector<int> refList(pModNgv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					// global variable module I/O
					if (pModNgv->m_bWriteForInstrWrite) { // Instruction read
						if (mod.m_threads.m_htIdW.AsInt() > 0) {

							fprintf(scFile, "\t\tp%s%s->i_%sTo%s_iwHtId%s(%sTo%s_iwHtId%s%s);\n",
								modName.c_str(), instNum.c_str(),
								mod.m_modName.Lc().c_str(), pModNgv->m_gblName.Uc().c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pModNgv->m_gblName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());

						}

						fprintf(scFile, "\t\tp%s%s->i_%sTo%s_iwComp%s(%sTo%s_iwComp%s%s);\n",
							modName.c_str(), instNum.c_str(),
							mod.m_modName.Lc().c_str(), pModNgv->m_gblName.Uc().c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), pModNgv->m_gblName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());

						fprintf(scFile, "\t\tp%s%s->i_%sTo%s_iwData%s(%sTo%s_iwData%s%s);\n",
							modName.c_str(), instNum.c_str(),
							mod.m_modName.Lc().c_str(), pModNgv->m_gblName.Uc().c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), pModNgv->m_gblName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
					}

					if (pModNgv->m_bReadForInstrRead) { // Instruction read
						if (pNgvInfo->m_atomicMask != 0) {

							fprintf(scFile, "\t\tp%s%s->o_%sTo%s_ifWrEn%s(%sTo%s_ifWrEn%s%s);\n",
								modName.c_str(), instNum.c_str(),
								pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());

							if (mod.m_threads.m_htIdW.AsInt() > 0) {

								fprintf(scFile, "\t\tp%s%s->o_%sTo%s_ifHtId%s(%sTo%s_ifHtId%s%s);\n",
									modName.c_str(), instNum.c_str(),
									pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
									pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
							}

							fprintf(scFile, "\t\tp%s%s->o_%sTo%s_ifData%s(%sTo%s_ifData%s%s);\n",
								modName.c_str(), instNum.c_str(),
								pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
						}
					}

					if (pModNgv->m_bReadForInstrRead || pModNgv->m_bReadForMifWrite) {

						fprintf(scFile, "\t\tp%s%s->o_%sTo%s_wrEn%s(%sTo%s_wrEn%s%s);\n",
							modName.c_str(), instNum.c_str(),
							pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());

						if (pModNgv->m_addrW > 0) {
							fprintf(scFile, "\t\tp%s%s->o_%sTo%s_wrAddr%s(%sTo%s_wrAddr%s%s);\n",
								modName.c_str(), instNum.c_str(),
								pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
						}

						fprintf(scFile, "\t\tp%s%s->o_%sTo%s_wrData%s(%sTo%s_wrData%s%s);\n",
							modName.c_str(), instNum.c_str(),
							pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
					}

					if (pModNgv->m_bWriteForInstrWrite) {

						fprintf(scFile, "\t\tp%s%s->o_%sTo%s_iwCompRdy%s(%sTo%s_iwCompRdy%s%s);\n",
							modName.c_str(), instNum.c_str(),
							pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());

						if (mod.m_threads.m_htIdW.AsInt() > 0) {
							fprintf(scFile, "\t\tp%s%s->o_%sTo%s_iwCompHtId%s(%sTo%s_iwCompHtId%s%s);\n",
								modName.c_str(), instNum.c_str(),
								pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
						}

						fprintf(scFile, "\t\tp%s%s->o_%sTo%s_iwCompData%s(%sTo%s_iwCompData%s%s);\n",
							modName.c_str(), instNum.c_str(),
							pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
					}

					if (pModNgv->m_bWriteForMifRead) { // Memory
						if (mod.m_mif.m_mifRd.m_rspGrpW.AsInt() > 0) {
							fprintf(scFile, "\t\tp%s%s->i_%sTo%s_mwGrpId%s(%sTo%s_mwGrpId%s%s);\n",
								modName.c_str(), instNum.c_str(),
								mod.m_modName.Lc().c_str(), pModNgv->m_gblName.Uc().c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pModNgv->m_gblName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
						}

						fprintf(scFile, "\t\tp%s%s->i_%sTo%s_mwData%s(%sTo%s_mwData%s%s);\n",
							modName.c_str(), instNum.c_str(),
							mod.m_modName.Lc().c_str(), pModNgv->m_gblName.Uc().c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), pModNgv->m_gblName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());

						fprintf(scFile, "\t\tp%s%s->o_%sTo%s_mwCompRdy%s(%sTo%s_mwCompRdy%s%s);\n",
							modName.c_str(), instNum.c_str(),
							pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());

						if (mod.m_mif.m_mifRd.m_rspGrpW.AsInt() > 0) {
							fprintf(scFile, "\t\tp%s%s->o_%sTo%s_mwCompGrpId%s(%sTo%s_mwCompGrpId%s%s);\n",
								modName.c_str(), instNum.c_str(),
								pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								pModNgv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), instNum.c_str(), dimIdx.c_str());
						}
					}
				} while (DimenIter(pModNgv->m_dimenList, refList));
			}
			fprintf(scFile, "\n");
		}
	}

	fprintf(scFile, "\t}\n");
	fprintf(scFile, "};\n");

	scFile.FileClose();
}
