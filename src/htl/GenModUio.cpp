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

// Generate a module's user io interface code

void CDsnInfo::InitAndValidateModUio()
{
  	// initialize / match uiointf to uiointfconn
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t uioIdx = 0; uioIdx < mod.m_uioIntfList.size(); uioIdx += 1) {
			CUioIntf * pUioIntf = mod.m_uioIntfList[uioIdx];

			pUioIntf->m_dimen.InitValue(pUioIntf->m_lineInfo, false, 0);
			pUioIntf->m_queueW.InitValue(pUioIntf->m_lineInfo, false, 0);
			pUioIntf->m_reserve.InitValue(pUioIntf->m_lineInfo, false, 0);
			pUioIntf->m_instCnt = mod.m_instSet.GetInstCnt();
			for (int i = 0; i < pUioIntf->m_instCnt; i++)
				pUioIntf->m_replCntList.push_back(mod.m_instSet.GetReplCnt(i));

			pUioIntf->m_clkRate = mod.m_clkRate;

			pUioIntf->InitConnIntfList();

			int instMax  = pUioIntf->m_instCnt;
			int unitMax  = g_appArgs.GetAeUnitCnt();
			int dimenMax = (pUioIntf-> m_dimen.AsInt() != 0) ? pUioIntf->m_dimen.AsInt() : 1;

			// Try to find HTI match to HTD decl
			bool bMatch = false;
			for (size_t connIdx = 0; connIdx < GetUioIntfConnListSize(); connIdx += 1) {
				HtiFile::CUioIntfConn * pUioIntfConn = GetUioIntfConn(connIdx);

				for (size_t unitIdx = 0; unitIdx < GetUioIntfConnListSize(); unitIdx += 1) {
					int instIdx;
					if (HtiFile::IsUioPathMatch(pUioIntfConn->m_lineInfo, pUioIntfConn->m_uioIntf, mod, pUioIntf, instIdx) && pUioIntfConn->m_uioIntf.m_unitIdx == unitIdx) {
						int replMax = mod.m_instSet.GetReplCnt(instIdx);

						int replIdx = pUioIntfConn->m_uioIntf.m_replIdx;
						int dimenIdx = pUioIntfConn->m_uioIntf.m_msgIntfIdx.size() ? pUioIntfConn->m_uioIntf.m_msgIntfIdx.at(0) : 0;

						// Check dimen/repl
						if (replIdx > replMax-1)
							ParseMsg(Error, pUioIntfConn->m_lineInfo, "Replication Specifer (%d) was greater than the maximum replication index specified (%d)", replIdx, replMax-1);
						if (dimenIdx > dimenMax-1)
							ParseMsg(Error, pUioIntfConn->m_lineInfo, "Dimen Specifer (%d) was greater than the maximum dimension index specified (%d)", dimenIdx, dimenMax-1);

						pUioIntf->SetConnIntf(unitIdx, instIdx, replIdx, dimenIdx, pUioIntfConn);

						pUioIntfConn->m_uioIntf.m_pUioIntf = pUioIntf;
						pUioIntfConn->m_uioIntf.m_pMod = &mod;

						pUioIntfConn->m_pType = pUioIntf->m_pType;

						bMatch = true;
					}
				}
			}

			if (!bMatch) {
				ParseMsg(Error, pUioIntf->m_lineInfo, "htd AddUserIO declaration %s was not matched to a corresponding hti declaration", pUioIntf->m_name.c_str());
			}

			// Look for missing HTD->HTI conns
			for (int unitIdx = 0; unitIdx < unitMax; unitIdx++) {
				for (int instIdx = 0; instIdx < instMax; instIdx++) {
					int replMax = mod.m_instSet.GetReplCnt(instIdx);
					CInstance *pInst = mod.m_instSet.GetInst(instIdx);
					for (int replIdx = 0; replIdx < replMax; replIdx++) {
						for (int dimenIdx = 0; dimenIdx < dimenMax; dimenIdx++) {
						  if (pUioIntf->GetConnIntf(unitIdx, instIdx, replIdx, dimenIdx) == NULL) {
								ParseMsg(Error, pUioIntf->m_lineInfo, "Missing hti AddUserIOConn declaration for user io port %s", pUioIntf->m_name.c_str());
								ParseMsg(Info, pUioIntf->m_lineInfo, "  UnitId %d", unitIdx);
								if (instMax > 1)
									ParseMsg(Info, pUioIntf->m_lineInfo, "  InstId %d", instIdx);
								if (pInst->m_instParams.m_bExplicitRepl)
									ParseMsg(Info, pUioIntf->m_lineInfo, "  ReplId %d", replIdx);
								if (pUioIntf->m_dimen.AsInt() != 0)
									ParseMsg(Info, pUioIntf->m_lineInfo, "  Dimen %d", dimenIdx);
							}
						}
					}
				}
			}
		}
	}

	// Look for orphaned HTI conns
	for (size_t connIdx = 0; connIdx < GetUioIntfConnListSize(); connIdx += 1) {
		HtiFile::CUioIntfConn * pUioIntfConn = GetUioIntfConn(connIdx);
		if (pUioIntfConn->m_uioIntf.m_pUioIntf == NULL)
			ParseMsg(Error, pUioIntfConn->m_lineInfo, "hti AddUserIOConn declaration %s was never used", pUioIntfConn->m_uioIntf.m_msgIntfName.c_str());
	}

	bool bSimModUsed = false;
	CModule *pSimMod = 0;
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		if (bSimModUsed && mod.m_uioSimIntfList.size() > 0) {
			CUioIntf * pUioIntf = mod.m_uioSimIntfList[0];
			ParseMsg(Error, pUioIntf->m_lineInfo, "All htd AddUserIOSim declarations can only be attached to a single module (first different occurance: %s)", pUioIntf->m_name.c_str());
		} else if (mod.m_uioSimIntfList.size() > 0) {
			bSimModUsed = true;
			pSimMod = m_modList[modIdx];
		}

		for (size_t uioIdx = 0; uioIdx < mod.m_uioSimIntfList.size(); uioIdx += 1) {
			CUioIntf * pUioIntf = mod.m_uioSimIntfList[uioIdx];

			pUioIntf->m_dimen.InitValue(pUioIntf->m_lineInfo, false, 0);
			pUioIntf->m_queueW.InitValue(pUioIntf->m_lineInfo, false, 0);
			pUioIntf->m_reserve.InitValue(pUioIntf->m_lineInfo, false, 0);
			pUioIntf->m_instCnt = mod.m_instSet.GetInstCnt();
			if (pUioIntf->m_instCnt > 1) {
				ParseMsg(Error, pUioIntf->m_lineInfo, "htd AddUserIOSim is only supported on a module with a single instance; only one call chain can reach this module");
			}
			for (int i = 0; i < pUioIntf->m_instCnt; i++) {
				if (mod.m_instSet.GetReplCnt(i) > 1) {
					ParseMsg(Error, pUioIntf->m_lineInfo, "htd AddUserIOSim is only supported on a module without replication");
				}
				pUioIntf->m_replCntList.push_back(mod.m_instSet.GetReplCnt(i));
			}

			pUioIntf->m_clkRate = mod.m_clkRate;

			pUioIntf->InitConnIntfList();

			int dimenMax = (pUioIntf-> m_dimen.AsInt() != 0) ? pUioIntf->m_dimen.AsInt() : 1;

			// Try to find HTI match to HTD decl
			bool bMatch = false;
			for (size_t connIdx = 0; connIdx < GetUioSimIntfConnListSize(); connIdx += 1) {
				HtiFile::CUioIntfConn * pUioIntfConn = GetUioSimIntfConn(connIdx);

				int instIdx;
				if (HtiFile::IsUioPathMatch(pUioIntfConn->m_lineInfo, pUioIntfConn->m_uioIntf, mod, pUioIntf, instIdx)) {
					int dimenIdx = pUioIntfConn->m_uioIntf.m_msgIntfIdx.size() ? pUioIntfConn->m_uioIntf.m_msgIntfIdx.at(0) : 0;

					// Check dimen
					if (dimenIdx > dimenMax-1)
						ParseMsg(Error, pUioIntfConn->m_lineInfo, "Dimen Specifer (%d) was greater than the maximum dimension index specified (%d)", dimenIdx, dimenMax-1);

					pUioIntf->SetConnIntf(0, 0, 0, dimenIdx, pUioIntfConn);

					pUioIntfConn->m_uioIntf.m_pUioIntf = pUioIntf;
					pUioIntfConn->m_uioIntf.m_pMod = &mod;

					pUioIntfConn->m_pType = pUioIntf->m_pType;

					bMatch = true;
				}
			}

			if (!bMatch) {
				ParseMsg(Error, pUioIntf->m_lineInfo, "htd AddUserIOSim declaration %s was not matched to a corresponding hti declaration", pUioIntf->m_name.c_str());
			}

			// Look for missing HTD->HTI conns
			for (int dimenIdx = 0; dimenIdx < dimenMax; dimenIdx++) {
				if (pUioIntf->GetConnIntf(0, 0, 0, dimenIdx) == NULL) {
					ParseMsg(Error, pUioIntf->m_lineInfo, "Missing hti AddUserIOSimConn declaration for user io simport %s", pUioIntf->m_name.c_str());
					if (pUioIntf->m_dimen.AsInt() != 0)
						ParseMsg(Info, pUioIntf->m_lineInfo, "  Dimen %d", dimenIdx);
				}
			}
		}
	}

	// Look for orphaned HTI conns
	for (size_t connIdx = 0; connIdx < GetUioSimIntfConnListSize(); connIdx += 1) {
		HtiFile::CUioIntfConn * pUioIntfConn = GetUioSimIntfConn(connIdx);
		if (pUioIntfConn->m_uioIntf.m_pUioIntf == NULL)
			ParseMsg(Error, pUioIntfConn->m_lineInfo, "hti AddUserIOSimConn declaration %s was never used", pUioIntfConn->m_uioIntf.m_msgIntfName.c_str());
	}

	// check for each user io interface for port conflicts
	for (size_t conn0Idx = 0; conn0Idx < GetUioIntfConnListSize(); conn0Idx += 1) {
		HtiFile::CUioIntfConn * pUioIntfConn0 = GetUioIntfConn(conn0Idx);

		for (size_t recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
			CRecord * pRecord = m_recordList[recordIdx];

			if (pRecord->m_typeName == pUioIntfConn0->m_pType->m_typeName)
				pRecord->m_bNeedIntf = true;
		}

		// check that no other modules use this port
		for (size_t conn1Idx = conn0Idx; conn1Idx < GetUioIntfConnListSize(); conn1Idx += 1) {
			HtiFile::CUioIntfConn * pUioIntfConn1 = GetUioIntfConn(conn1Idx);

			if (conn0Idx == conn1Idx) continue;

			if (pUioIntfConn0->m_bInbound == pUioIntfConn1->m_bInbound && pUioIntfConn0->m_uioPort == pUioIntfConn1->m_uioPort) {
				ParseMsg(Error, pUioIntfConn1->m_lineInfo, "%sbound user io interface %s has conflicting port information with another user io interface",
					 (pUioIntfConn0->m_bInbound ? "In" : "Out"), pUioIntfConn0->m_uioIntf.m_msgIntfName.c_str());
				ParseMsg(Info, pUioIntfConn0->m_lineInfo, "  previous user io interface");
			}
		}
	}
	for (size_t conn0Idx = 0; conn0Idx < GetUioSimIntfConnListSize(); conn0Idx += 1) {
		HtiFile::CUioIntfConn * pUioIntfConn0 = GetUioSimIntfConn(conn0Idx);

		for (size_t recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
			CRecord * pRecord = m_recordList[recordIdx];

			if (pRecord->m_typeName == pUioIntfConn0->m_pType->m_typeName)
				pRecord->m_bNeedIntf = true;
		}

		// check that no other modules use this port
		for (size_t conn1Idx = conn0Idx; conn1Idx < GetUioSimIntfConnListSize(); conn1Idx += 1) {
			HtiFile::CUioIntfConn * pUioIntfConn1 = GetUioSimIntfConn(conn1Idx);

			if (conn0Idx == conn1Idx) continue;

			if (pUioIntfConn0->m_bInbound == pUioIntfConn1->m_bInbound && pUioIntfConn0->m_uioPort == pUioIntfConn1->m_uioPort) {
				ParseMsg(Error, pUioIntfConn1->m_lineInfo, "%sbound user io interface %s has conflicting port information with another user io interface",
					 (pUioIntfConn0->m_bInbound ? "In" : "Out"), pUioIntfConn0->m_uioIntf.m_msgIntfName.c_str());
				ParseMsg(Info, pUioIntfConn0->m_lineInfo, "  previous user io interface");
			}
		}
	}

	// Check bounds (do we exceed number of ports specified)
	int portCntMax = g_appArgs.GetUioPortCnt()-1;
	for (size_t connIdx = 0; connIdx < GetUioIntfConnListSize(); connIdx += 1) {
		HtiFile::CUioIntfConn * pConnIntf = GetUioIntfConn(connIdx);

		int portId = atoi(pConnIntf->m_uioPort.c_str());
		if (portId > portCntMax) {
			ParseMsg(Error, pConnIntf->m_lineInfo, "user io interface %s has a port id (%d) that exceeds the maximum port id index specified (%d)",
				 pConnIntf->m_uioIntf.m_pUioIntf->m_name.c_str(), portId, portCntMax);
		}
	}
	for (size_t connIdx = 0; connIdx < GetUioSimIntfConnListSize(); connIdx += 1) {
		HtiFile::CUioIntfConn * pConnIntf = GetUioSimIntfConn(connIdx);

		int portId = atoi(pConnIntf->m_uioPort.c_str());
		if (portId > portCntMax) {
			ParseMsg(Error, pConnIntf->m_lineInfo, "user io sim interface %s has a port id (%d) that exceeds the maximum port id index specified (%d)",
				 pConnIntf->m_uioIntf.m_pUioIntf->m_name.c_str(), portId, portCntMax);
		}
	}

	// Check matching widths
	int portWidthMax = g_appArgs.GetUioPortsWidth();
	for (size_t connIdx = 0; connIdx < GetUioIntfConnListSize(); connIdx += 1) {
		HtiFile::CUioIntfConn * pConnIntf = GetUioIntfConn(connIdx);

		int portWidth = pConnIntf->m_pType->GetPackedBitWidth();
		if (portWidth != portWidthMax) {
			ParseMsg(Error, pConnIntf->m_lineInfo, "user io interface %s has a port width (%d) that does not match the user io port width specified (%d)",
				 pConnIntf->m_uioIntf.m_pUioIntf->m_name.c_str(), portWidth, portWidthMax);
		}
	}
	for (size_t connIdx = 0; connIdx < GetUioSimIntfConnListSize(); connIdx += 1) {
		HtiFile::CUioIntfConn * pConnIntf = GetUioSimIntfConn(connIdx);

		int portWidth = pConnIntf->m_pType->GetPackedBitWidth();
		if (portWidth != portWidthMax) {
			ParseMsg(Error, pConnIntf->m_lineInfo, "user io sim interface %s has a port width (%d) that does not match the user io port width specified (%d)",
				 pConnIntf->m_uioIntf.m_pUioIntf->m_name.c_str(), portWidth, portWidthMax);
		}
	}

	// Check that only 1 csr interface is used... (and it must be in the same module as sim ports!)
	bool bSimCsrUsed = false;
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t uioIdx = 0; uioIdx < mod.m_uioCsrIntfList.size(); uioIdx += 1) {
			CUioCsrIntf * pUioCsrIntf = mod.m_uioCsrIntfList[uioIdx];

			// Check for >1 csr interface
			if (bSimCsrUsed) {
				ParseMsg(Error, pUioCsrIntf->m_lineInfo, "Only one user io sim csr interface per design is supported");
			} else {
				bSimCsrUsed = true;
			}

			// Check that interface is attached to the same module as sim ports
			if (m_modList[modIdx] != pSimMod) {
				ParseMsg(Error, pUioCsrIntf->m_lineInfo, "user io sim csr interface must be attached to the same module as the user io sim interface ports");
			}
		}
	}

	// Check to ensure each port has a matching sim port (SYSC_SIM ONLY)
	bool isSyscSim = false;
	for (int i = 0; i < g_appArgs.GetPreDefinedNameCnt(); i++) {
		if (g_appArgs.GetPreDefinedName(i) == "HT_SYSC") {
			isSyscSim = true;
			break;
		}
	}

	if (isSyscSim) {
		for (size_t connIdx = 0; connIdx < GetUioIntfConnListSize(); connIdx += 1) {
			HtiFile::CUioIntfConn * pConnIntf = GetUioIntfConn(connIdx);

			bool bFound = false;
			for (size_t connSimIdx = 0; connSimIdx < GetUioSimIntfConnListSize(); connSimIdx += 1) {
				HtiFile::CUioIntfConn * pConnSimIntf = GetUioSimIntfConn(connSimIdx);
			
				if (pConnIntf->m_uioPort == pConnSimIntf->m_uioPort && pConnIntf->m_bInbound != pConnSimIntf->m_bInbound) {
					bFound = true;
					pConnIntf->m_syscSimConn = pConnSimIntf;
					pConnSimIntf->m_syscSimConn = pConnIntf;
					break;
				}
			}

			if (!bFound)
				ParseMsg(Error, pConnIntf->m_lineInfo, "cannot find corresponsing user io sim interface for user io interface %s",
					 pConnIntf->m_uioIntf.m_pUioIntf->m_name.c_str());
		}
	}
		
	

	if (GetParseErrorCnt() > 0)
		ParseMsg(Fatal, "Previous errors prevent generation of files");
}

void CDsnInfo::GenModUioStatements(CInstance * pInst)
{
	//int instIdx = pInst->m_instId;
	CModule * pMod = pInst->m_pMod;

	if (pMod->m_uioIntfList.size() == 0)
		return;

	// message interface
	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

	bool bIn = false;
	for (size_t intfIdx = 0; intfIdx < pMod->m_uioIntfList.size(); intfIdx += 1) {
		CUioIntf * pUioIntf = pMod->m_uioIntfList[intfIdx];

		CHtCode & uioPreInstr = pMod->m_clkRate == eClk2x ? m_uioPreInstr2x : m_uioPreInstr1x;
		CHtCode & srcUioPostInstr = pUioIntf->m_clkRate == eClk2x ? m_uioPostInstr2x : m_uioPostInstr1x;
		CHtCode & uioReg = pMod->m_clkRate == eClk2x ? m_uioReg2x : m_uioReg1x;
		CHtCode & uioPostReg = pMod->m_clkRate == eClk2x ? m_uioPostReg2x : m_uioPostReg1x;
		CHtCode & uioOut = pMod->m_clkRate == eClk2x ? m_uioOut2x : m_uioOut1x;

		string reset = pMod->m_clkRate == eClk1x ? "r_reset1x" : "c_reset2x";

		int intfQueueW = pUioIntf->m_queueW.AsInt();

		if (pUioIntf->m_bInbound) {

			if (!bIn) {
				m_uioIoDecl.Append("\t// Inbound User IO interfaces\n");
				g_appArgs.GetDsnRpt().AddLevel("Inbound User IO\n");
				bIn = true;
			}

			string intfDecl;
			string intfParam;
			string intfParamIdx;
			vector<CHtString> dimenList;
			int intfDimenCnt = (pUioIntf->m_dimen.AsInt() != 0) ? pUioIntf->m_dimen.AsInt() : 1;
			if (pUioIntf->m_dimen.AsInt() != 0) {
				intfDecl = VA("[%d]", intfDimenCnt);
				intfParam = VA("ht_uint%d dimenIdx", FindLg2(intfDimenCnt - 1));
				intfParamIdx = "[dimenIdx]";
				dimenList.push_back(pUioIntf->m_dimen);
			}

			m_uioIoDecl.Append("\tsc_in<bool> i_portTo%s_%sUioRdy%s;\n",
				pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioIoDecl.Append("\tsc_in<%s> i_portTo%s_%sUioData%s;\n",
				pUioIntf->m_pType->m_typeName.c_str(), pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioIoDecl.Append("\tsc_out<bool> o_portTo%s_%sUioAFull%s;\n",
				pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());

			m_uioIoDecl.Append("\n");

			if (intfQueueW == 0) {
				m_uioRegDecl.Append("\tbool r_portTo%s_%sUioRdy%s;\n",
					pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
				m_uioRegDecl.Append("\t%s r_portTo%s_%sUioData%s;\n",
					pUioIntf->m_pType->m_typeName.c_str(), pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			} else {
				m_uioRegDecl.Append("\tht_%s_que<%s, %d> m_portTo%s_%sUioQue%s;\n",
					intfQueueW > 6 ? "block" : "dist",
					pUioIntf->m_pType->m_typeName.c_str(), intfQueueW,
					pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
				m_uioRegDecl.Append("\tbool r_portToQue_%sUioRdy%s;\n",
					pUioIntf->m_name.c_str(), intfDecl.c_str());
				m_uioRegDecl.Append("\t%s r_portToQue_%sUioData%s;\n",
					pUioIntf->m_pType->m_typeName.c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
				m_uioRegDecl.Append("\tbool r_portToQue_%sUioAFull%s;\n",
					pUioIntf->m_name.c_str(), intfDecl.c_str());

				m_uioRegDecl.Append("\tbool c_queTo%s_%sUioRdy%s;\n",
					pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
				m_uioRegDecl.Append("\t%s c_queTo%s_%sUioData%s;\n",
					pUioIntf->m_pType->m_typeName.c_str(), pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
				m_uioRegDecl.Append("\tbool c_queTo%s_%sUioPop%s;\n",
					pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			}

			m_uioRegDecl.Append("\n");

			if (intfQueueW == 0) {
				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioReg.Append("\tr_portTo%s_%sUioRdy%s = i_portTo%s_%sUioRdy%s;\n",
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioReg.Append("\tr_portTo%s_%sUioData%s = i_portTo%s_%sUioData%s;\n",
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}
				uioReg.Append("\n");

			} else {
				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioPreInstr.Append("\tc_queTo%s_%sUioPop%s = false;\n",
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioPreInstr.Append("\tc_queTo%s_%sUioRdy%s = !m_portTo%s_%sUioQue%s.empty();\n",
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

						uioPreInstr.Append("\tc_queTo%s_%sUioData%s = m_portTo%s_%sUioQue%s.front();\n",
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}
				uioPreInstr.Append("\n");

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioPreInstr.Append("\tif (r_portToQue_%sUioRdy%s)\n",
							pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pUioIntf->m_name.c_str(), intfIdx.c_str());
						uioPreInstr.Append("\t\tm_portTo%s_%sUioQue%s.push(r_portToQue_%sUioData%s);\n",
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}
				uioPreInstr.Append("\n");

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioReg.Append("\tr_portToQue_%sUioRdy%s = i_portTo%s_%sUioRdy%s;\n",
							pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioReg.Append("\tr_portToQue_%sUioData%s = i_portTo%s_%sUioData%s;\n",
							pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}
				uioReg.Append("\n");

				{
					int maxReserved = pUioIntf->m_reserve.AsInt();
					HtlAssert(maxReserved < (1 << intfQueueW));

					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioReg.Append("\tr_portToQue_%sUioAFull%s = m_portTo%s_%sUioQue%s.full(%d);\n",
							pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
							maxReserved + 6);

					} while (DimenIter(dimenList, refList));
				}
				uioReg.Append("\n");

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						srcUioPostInstr.Append("\tif (c_queTo%s_%sUioPop%s)\n",
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

						srcUioPostInstr.Append("\t\tm_portTo%s_%sUioQue%s.pop();\n",
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}
				srcUioPostInstr.Append("\n");

				{
					HtdFile::EClkRate srcClkRate = pUioIntf->m_clkRate;

					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						if (srcClkRate == pMod->m_clkRate)
							uioReg.Append("\tm_portTo%s_%sUioQue%s.clock(%s);\n",
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(), reset.c_str());
						else if (srcClkRate == eClk2x) {
							m_uioReg2x.Append("\tm_portTo%s_%sUioQue%s.push_clock(c_reset2x);\n",
								pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
							m_uioReg1x.Append("\tm_portTo%s_%sUioQue%s.pop_clock(r_reset1x);\n",
								pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
						} else {
							m_uioReg1x.Append("\tm_portTo%s_%sUioQue%s.push_clock(r_reset1x);\n",
								pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
							m_uioReg2x.Append("\tm_portTo%s_%sUioQue%s.pop_clock(c_reset2x);\n",
								pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
						}
					} while (DimenIter(dimenList, refList));
				}
			}
			uioReg.Append("\n");

			if (intfQueueW == 0) {
				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioOut.Append("\to_portTo%s_%sUioAFull%s = false;\n",
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}
			} else {
				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioOut.Append("\to_portTo%s_%sUioAFull%s = r_portToQue_%sUioAFull%s;\n",
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}
			}
			uioOut.Append("\n");

			string vcdModName = VA("Pers%s", pMod->m_modName.Uc().c_str());
			m_uioRegDecl.Append("\tbool ht_noload c_RecvUioBusy_%s%s;\n", pUioIntf->m_name.c_str(), intfDecl.c_str());

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);
					string intfParamIdx = IndexStr(refList, -1, 0, true);

					GenModTrace(eVcdUser, vcdModName, VA("RecvUioBusy_%s(%s)", pUioIntf->m_name.c_str(), intfParamIdx.c_str()),
						VA("c_RecvUioBusy_%s%s", pUioIntf->m_name.c_str(), intfIdx.c_str()));

				} while (DimenIter(dimenList, refList));
			}

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					if (intfQueueW == 0)
						uioPostReg.Append("\tc_RecvUioBusy_%s%s = !r_portTo%s_%sUioRdy%s;\n",
						pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
					else
						uioPostReg.Append("\tc_RecvUioBusy_%s%s = !c_queTo%s_%sUioRdy%s;\n",
						pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}

			g_appArgs.GetDsnRpt().AddItem("bool RecvUioBusy_%s(%s)\n",
				pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDecl.Append("\tbool RecvUioBusy_%s(%s);\n",
				pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("bool CPers%s::RecvUioBusy_%s(%s)\n",
				pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("{\n");

			if (pUioIntf->m_dimen.AsInt() > 0) {
				m_uioFuncDef.Append("\tassert_msg(dimenIdx < %d, \"Runtime check failed in CPers%s::RecvUioBusy_%s()"
					" - dimenIdx value (%%d) out of range (0-%d)\\n\", (int)dimenIdx);\n",
					pUioIntf->m_dimen.AsInt(), pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(),
					pUioIntf->m_dimen.AsInt() - 1);
			}

			m_uioFuncDef.Append("\treturn c_RecvUioBusy_%s%s;\n",
				pUioIntf->m_name.c_str(), intfParamIdx.c_str());
			m_uioFuncDef.Append("}\n");

			m_uioFuncDef.Append("\n");

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);
					string intfParamIdx = IndexStr(refList, -1, 0, true);

					if (intfQueueW == 0)
						GenModTrace(eVcdUser, vcdModName, VA("RecvUioReady_%s(%s)", pUioIntf->m_name.c_str(), intfParamIdx.c_str()),
						VA("r_portTo%s_%sUioRdy%s", pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str()));
					else
						GenModTrace(eVcdUser, vcdModName, VA("RecvUioReady_%s(%s)", pUioIntf->m_name.c_str(), intfParamIdx.c_str()),
						VA("c_queTo%s_%sUioRdy%s", pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str()));

				} while (DimenIter(dimenList, refList));
			}

			g_appArgs.GetDsnRpt().AddItem("bool RecvUioReady_%s(%s)\n",
				pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDecl.Append("\tbool RecvUioReady_%s(%s);\n",
				pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("bool CPers%s::RecvUioReady_%s(%s)\n",
				pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("{\n");

			if (pUioIntf->m_dimen.AsInt() > 0) {
				m_uioFuncDef.Append("\tassert_msg(dimenIdx < %d, \"Runtime check failed in CPers%s::RecvUioReady_%s()"
					" - dimenIdx value (%%d) out of range (0-%d)\\n\", (int)dimenIdx);\n",
					pUioIntf->m_dimen.AsInt(), pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(),
					pUioIntf->m_dimen.AsInt() - 1);
			}

			if (intfQueueW == 0)
				m_uioFuncDef.Append("\treturn r_portTo%s_%sUioRdy%s;\n",
					pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());
			else
				m_uioFuncDef.Append("\treturn c_queTo%s_%sUioRdy%s;\n",
					pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());

			m_uioFuncDef.Append("}\n");
			m_uioFuncDef.Append("\n");

			g_appArgs.GetDsnRpt().AddItem("%s PeekUio_%s(%s)\n",
				pUioIntf->m_pType->m_typeName.c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDecl.Append("\t%s PeekUio_%s(%s);\n",
				pUioIntf->m_pType->m_typeName.c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("%s CPers%s::PeekUio_%s(%s)\n",
				pUioIntf->m_pType->m_typeName.c_str(), 
				pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("{\n");

			if (pUioIntf->m_dimen.AsInt() > 0) {
				m_uioFuncDef.Append("\tassert_msg(dimenIdx < %d, \"Runtime check failed in CPers%s::PeekUio_%s()"
					" - dimenIdx value (%%d) out of range (0-%d)\\n\", (int)dimenIdx);\n",
					pUioIntf->m_dimen.AsInt(), pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(),
					pUioIntf->m_dimen.AsInt() - 1);
			}

			if (intfQueueW == 0)
				m_uioFuncDef.Append("\treturn r_portTo%s_%sUioData%s;\n",
					pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());
			else
				m_uioFuncDef.Append("\treturn c_queTo%s_%sUioData%s;\n",
					pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());

			m_uioFuncDef.Append("}\n");
			m_uioFuncDef.Append("\n");

			g_appArgs.GetDsnRpt().AddItem("%s RecvUioData_%s(%s)\n",
				pUioIntf->m_pType->m_typeName.c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDecl.Append("\t%s RecvUioData_%s(%s);\n",
				pUioIntf->m_pType->m_typeName.c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("%s CPers%s::RecvUioData_%s(%s)\n",
				pUioIntf->m_pType->m_typeName.c_str(),
				pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("{\n");

			if (pUioIntf->m_dimen.AsInt() > 0) {
				m_uioFuncDef.Append("\tassert_msg(dimenIdx < %d, \"Runtime check failed in CPers%s::RecvUioData_%s()"
					" - dimenIdx value (%%d) out of range (0-%d)\\n\", (int)dimenIdx);\n",
					pUioIntf->m_dimen.AsInt(), pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(),
					pUioIntf->m_dimen.AsInt() - 1);
			}


			if (intfQueueW == 0) {
				m_uioFuncDef.Append("\treturn r_portTo%s_%sUioData%s;\n",
					pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());
			} else {
				m_uioFuncDef.Append("\tc_queTo%s_%sUioPop%s = true;\n",
					pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());
				m_uioFuncDef.Append("\treturn c_queTo%s_%sUioData%s;\n",
					pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());
			}

			m_uioFuncDef.Append("}\n");
			m_uioFuncDef.Append("\n");
		}
	}

	if (bIn)
		g_appArgs.GetDsnRpt().EndLevel();


	bool bOut = false;
	for (size_t intfIdx = 0; intfIdx < pMod->m_uioIntfList.size(); intfIdx += 1) {
		CUioIntf * pUioIntf = pMod->m_uioIntfList[intfIdx];

		CHtCode & uioPreInstr = pMod->m_clkRate == eClk2x ? m_uioPreInstr2x : m_uioPreInstr1x;
		CHtCode & uioReg = pMod->m_clkRate == eClk2x ? m_uioReg2x : m_uioReg1x;
		CHtCode & uioOut = pMod->m_clkRate == eClk2x ? m_uioOut2x : m_uioOut1x;
		CHtCode & uioPostInstr = pMod->m_clkRate == eClk2x ? m_uioPostInstr2x : m_uioPostInstr1x;
		CHtCode & srcUioPostInstr = pUioIntf->m_clkRate == eClk2x ? m_uioPostInstr2x : m_uioPostInstr1x;

		int intfQueueW = pUioIntf->m_queueW.AsInt();

		string reset = pMod->m_clkRate == eClk1x ? "r_reset1x" : "c_reset2x";

		if (!pUioIntf->m_bInbound) {

			if (!bOut) {
				m_uioIoDecl.Append("\t// Outbound User IO interfaces\n");
				g_appArgs.GetDsnRpt().AddLevel("Outbound User IO\n");

				bOut = true;
			}

			string intfDecl;
			string intfParam;
			string intfParamNoload;
			string intfParamIdx;
			vector<CHtString> dimenList;
			if (pUioIntf->m_dimen.AsInt() > 0) {
				intfDecl = VA("[%d]", pUioIntf->m_dimen.AsInt());
				intfParam = VA("ht_uint%d dimenIdx", FindLg2(pUioIntf->m_dimen.AsInt() - 1));
				intfParamNoload = VA("ht_noload ht_uint%d dimenIdx", FindLg2(pUioIntf->m_dimen.AsInt() - 1));
				intfParamIdx = "[dimenIdx]";
				dimenList.push_back(pUioIntf->m_dimen);
			}

			m_uioIoDecl.Append("\tsc_out<bool> o_%sToPort_%sUioRdy%s;\n",
				pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioIoDecl.Append("\tsc_out<%s> o_%sToPort_%sUioData%s;\n",
				pUioIntf->m_pType->m_typeName.c_str(), pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioIoDecl.Append("\tsc_in<bool> i_%sToPort_%sUioAFull%s;\n",
				pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());

			m_uioIoDecl.Append("\n");

			if (intfQueueW == 0) {
				m_uioRegDecl.Append("\tbool c_%sToPort_%sUioRdy%s;\n",
					pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
				m_uioRegDecl.Append("\t%s c_%sToPort_%sUioData%s;\n",
					pUioIntf->m_pType->m_typeName.c_str(), pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
				m_uioRegDecl.Append("\tbool r_%sToPort_%sUioAFull%s;\n",
					pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			} else {
				m_uioRegDecl.Append("\tht_%s_que<%s, %d> m_%sToPort_%sUioQue%s;\n",
					intfQueueW > 6 ? "block" : "dist",
					pUioIntf->m_pType->m_typeName.c_str(), intfQueueW,
					pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
				m_uioRegDecl.Append("\tbool r_queToPort_%sUioRdy%s;\n",
					pUioIntf->m_name.c_str(), intfDecl.c_str());
				m_uioRegDecl.Append("\t%s r_queToPort_%sUioData%s;\n",
					pUioIntf->m_pType->m_typeName.c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
				m_uioRegDecl.Append("\tbool r_queToPort_%sUioAFull%s;\n",
					pUioIntf->m_name.c_str(), intfDecl.c_str());

				m_uioRegDecl.Append("\tbool c_%sToQue_%sUioRdy%s;\n",
					pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
				m_uioRegDecl.Append("\t%s c_%sToQue_%sUioData%s;\n",
					pUioIntf->m_pType->m_typeName.c_str(), pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
				m_uioRegDecl.Append("\tbool r_%sToQue_%sUioAFull%s;\n",
					pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			}

			m_uioRegDecl.Append("\n");

			if (intfQueueW == 0) {
				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioPreInstr.Append("\tc_%sToPort_%sUioRdy%s = false;\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
						uioPreInstr.Append("\tc_%sToPort_%sUioData%s = 0;\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}
				uioPreInstr.Append("\n");

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioReg.Append("\tr_%sToPort_%sUioAFull%s = i_%sToPort_%sUioAFull%s;\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}
				uioReg.Append("\n");

			} else {
				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioPreInstr.Append("\tc_%sToQue_%sUioRdy%s = false;\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
						uioPreInstr.Append("\tc_%sToQue_%sUioData%s = 0;\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}
				uioPreInstr.Append("\n");

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioReg.Append("\tr_queToPort_%sUioAFull%s = i_%sToPort_%sUioAFull%s;\n",
							pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}

				{

					int maxReserved = pUioIntf->m_reserve.AsInt();
					HtlAssert(maxReserved < (1 << intfQueueW));

					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioReg.Append("\tr_%sToQue_%sUioAFull%s = m_%sToPort_%sUioQue%s.full(%d);\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
							maxReserved + 6);

					} while (DimenIter(dimenList, refList));
				}
				uioReg.Append("\n");

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioPostInstr.Append("\tr_queToPort_%sUioRdy%s = (!m_%sToPort_%sUioQue%s.empty() && !r_queToPort_%sUioAFull%s);\n",
							pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioPostInstr.Append("\tr_queToPort_%sUioData%s = m_%sToPort_%sUioQue%s.front();\n",
							pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}
				uioPostInstr.Append("\n");

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						srcUioPostInstr.Append("\tif (r_queToPort_%sUioRdy%s)\n",
							pUioIntf->m_name.c_str(), intfIdx.c_str());
						srcUioPostInstr.Append("\t\tm_%sToPort_%sUioQue%s.pop();\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}

				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						srcUioPostInstr.Append("\tif (c_%sToQue_%sUioRdy%s)\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
						srcUioPostInstr.Append("\t\tm_%sToPort_%sUioQue%s.push(c_%sToQue_%sUioData%s);\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}
				srcUioPostInstr.Append("\n");

				{
					HtdFile::EClkRate srcClkRate = pUioIntf->m_clkRate;

					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						if (srcClkRate == pMod->m_clkRate)
							uioReg.Append("\tm_%sToPort_%sUioQue%s.clock(%s);\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(), reset.c_str());
						else if (srcClkRate == eClk2x) {
							m_uioReg2x.Append("\tm_%sToPort_%sUioQue%s.push_clock(c_reset2x);\n",
								pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
							m_uioReg1x.Append("\tm_%sToPort_%sUioQue%s.pop_clock(r_reset1x);\n",
								pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
						} else {
							m_uioReg1x.Append("\tm_%sToPort_%sUioQue%s.push_clock(r_reset1x);\n",
								pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
							m_uioReg2x.Append("\tm_%sToPort_%sUioQue%s.pop_clock(c_reset2x);\n",
								pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
						}
					} while (DimenIter(dimenList, refList));
				}
				uioReg.Append("\n");

			}


			if (intfQueueW == 0) {
				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioOut.Append("\to_%sToPort_%sUioRdy%s = c_%sToPort_%sUioRdy%s;\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
						uioOut.Append("\to_%sToPort_%sUioData%s = c_%sToPort_%sUioData%s;\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}
			} else {
				{
					vector<int> refList(dimenList.size());
					do {
						string intfIdx = IndexStr(refList);

						uioOut.Append("\to_%sToPort_%sUioRdy%s = r_queToPort_%sUioRdy%s;\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pUioIntf->m_name.c_str(), intfIdx.c_str());
						uioOut.Append("\to_%sToPort_%sUioData%s = r_queToPort_%sUioData%s;\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
							pUioIntf->m_name.c_str(), intfIdx.c_str());

					} while (DimenIter(dimenList, refList));
				}
			}
			uioOut.Append("\n");

			{
				string vcdModName = VA("Pers%s", pInst->m_instName.Uc().c_str());
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);
					string intfParamIdx = IndexStr(refList, -1, 0, true);


					if (intfQueueW == 0)
						GenModTrace(eVcdUser, vcdModName, VA("SendUioBusy_%s(%s)", pUioIntf->m_name.c_str(), intfParamIdx.c_str()),
							VA("r_%sToPort_%sUioAFull%s", pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str()));
					else
						GenModTrace(eVcdUser, vcdModName, VA("SendUioBusy_%s(%s)", pUioIntf->m_name.c_str(), intfParamIdx.c_str()),
							VA("r_%sToQue_%sUioAFull%s", pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str()));

				} while (DimenIter(dimenList, refList));
			}

			g_appArgs.GetDsnRpt().AddItem("bool SendUioBusy_%s(%s)\n",
				pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDecl.Append("\tbool SendUioBusy_%s(%s);\n",
				pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("bool CPers%s::SendUioBusy_%s(%s)\n",
				pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("{\n");

			if (pUioIntf->m_dimen.AsInt() > 0) {
				m_uioFuncDef.Append("\tassert_msg(dimenIdx < %d, \"Runtime check failed in CPers%s::SendUioBusy_%s()"
					" - dimenIdx value (%%d) out of range (0-%d)\\n\", (int)dimenIdx);\n",
					pUioIntf->m_dimen.AsInt(), pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(),
					pUioIntf->m_dimen.AsInt() - 1);
			}

			if (intfQueueW == 0)
				m_uioFuncDef.Append("\treturn r_%sToPort_%sUioAFull%s;\n",
					pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());
			else
				m_uioFuncDef.Append("\treturn r_%sToQue_%sUioAFull%s;\n",
					pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());

			m_uioFuncDef.Append("}\n");
			m_uioFuncDef.Append("\n");

			{
				string vcdModName = VA("Pers%s", pMod->m_modName.Uc().c_str());
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);
					string intfParamIdx = IndexStr(refList, -1, 0, true);

					if (intfQueueW == 0)
						GenModTrace(eVcdUser, vcdModName, VA("SendUioFull_%s(%s)", pUioIntf->m_name.c_str(), intfParamIdx.c_str()),
							VA("r_%sToPort_%sUioAFull%s", pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str()));
					else
						GenModTrace(eVcdUser, vcdModName, VA("SendUioFull_%s(%s)", pUioIntf->m_name.c_str(), intfParamIdx.c_str()),
							VA("r_%sToQue_%sUioAFull%s", pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str()));

				} while (DimenIter(dimenList, refList));
			}

			g_appArgs.GetDsnRpt().AddItem("bool SendUioFull_%s(%s)\n",
				pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDecl.Append("\tbool SendUioFull_%s(%s);\n",
				pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("bool CPers%s::SendUioFull_%s(%s)\n",
				pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("{\n");

			if (pUioIntf->m_dimen.AsInt() > 0) {
				m_uioFuncDef.Append("\tassert_msg(dimenIdx < %d, \"Runtime check failed in CPers%s::SendUioFull_%s()"
					" - dimenIdx value (%%d) out of range (0-%d)\\n\", (int)dimenIdx);\n",
					pUioIntf->m_dimen.AsInt(), pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(),
					pUioIntf->m_dimen.AsInt() - 1);
			}

			if (intfQueueW == 0)
				m_uioFuncDef.Append("\treturn r_%sToPort_%sUioAFull%s;\n",
					pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());
			else
				m_uioFuncDef.Append("\treturn r_%sToQue_%sUioAFull%s;\n",
					pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());

			m_uioFuncDef.Append("}\n");
			m_uioFuncDef.Append("\n");

			if (intfParam.size() > 0) intfParam += ", ";

			g_appArgs.GetDsnRpt().AddItem("void SendUioData_%s(%s%s data)\n",
				pUioIntf->m_name.c_str(), intfParam.c_str(), pUioIntf->m_pType->m_typeName.c_str());
			m_uioFuncDecl.Append("\tvoid SendUioData_%s(%s%s data);\n",
				pUioIntf->m_name.c_str(), intfParam.c_str(), pUioIntf->m_pType->m_typeName.c_str());
			m_uioFuncDef.Append("void CPers%s::SendUioData_%s(%s%s data)\n",
				pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParam.c_str(), pUioIntf->m_pType->m_typeName.c_str());
			m_uioFuncDef.Append("{\n");

			if (pUioIntf->m_dimen.AsInt() > 0) {
				m_uioFuncDef.Append("\tassert_msg(dimenIdx < %d, \"Runtime check failed in CPers%s::SendUioData_%s()"
					" - dimenIdx value (%%d) out of range (0-%d)\\n\", (int)dimenIdx);\n",
					pUioIntf->m_dimen.AsInt(), pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(),
					pUioIntf->m_dimen.AsInt() - 1);
			}

			if (intfQueueW == 0) {
				m_uioFuncDef.Append("\tc_%sToPort_%sUioRdy%s = true;\n",
					pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());
				m_uioFuncDef.Append("\tc_%sToPort_%sUioData%s = data;\n",
					pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());
			} else {
				m_uioFuncDef.Append("\tc_%sToQue_%sUioRdy%s = true;\n",
					pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());
				m_uioFuncDef.Append("\tc_%sToQue_%sUioData%s = data;\n",
					pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());
			}
			m_uioFuncDef.Append("}\n");
			m_uioFuncDef.Append("\n");
		}
	}

	if (bOut)
		g_appArgs.GetDsnRpt().EndLevel();
}

void CDsnInfo::GenModUioSimStatements(CInstance * pInst)
{
	//int instIdx = pInst->m_instId;
	CModule * pMod = pInst->m_pMod;

	if (pMod->m_uioSimIntfList.size() == 0)
		return;

	// message interface
	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

	bool bIn = false;
	for (size_t intfIdx = 0; intfIdx < pMod->m_uioSimIntfList.size(); intfIdx += 1) {
		CUioIntf * pUioIntf = pMod->m_uioSimIntfList[intfIdx];

		CHtCode & uioPreInstr = pMod->m_clkRate == eClk2x ? m_uioPreInstr2x : m_uioPreInstr1x;
		CHtCode & srcUioPostInstr = pUioIntf->m_clkRate == eClk2x ? m_uioPostInstr2x : m_uioPostInstr1x;
		CHtCode & uioReg = pMod->m_clkRate == eClk2x ? m_uioReg2x : m_uioReg1x;
		CHtCode & uioPostReg = pMod->m_clkRate == eClk2x ? m_uioPostReg2x : m_uioPostReg1x;
		CHtCode & uioOut = pMod->m_clkRate == eClk2x ? m_uioOut2x : m_uioOut1x;

		string reset = pMod->m_clkRate == eClk1x ? "r_reset1x" : "c_reset2x";

		int intfQueueW = pUioIntf->m_queueW.AsInt();

		if (pUioIntf->m_queueW.AsInt() == 0)
			ParseMsg(Error, pUioIntf->m_lineInfo, "user io sim interface cannot have queueW of 0 %s",
				 pUioIntf->m_name.c_str());

		if (pUioIntf->m_bInbound) {

			if (!bIn) {
				m_uioIoDecl.Append("\t// Inbound User IO Sim interfaces\n");
				g_appArgs.GetDsnRpt().AddLevel("Inbound User IO Sim\n");
				bIn = true;
			}

			string intfDecl;
			string intfParam;
			string intfParamIdx;
			vector<CHtString> dimenList;
			int intfDimenCnt = (pUioIntf->m_dimen.AsInt() != 0) ? pUioIntf->m_dimen.AsInt() : 1;
			if (pUioIntf->m_dimen.AsInt() != 0) {
				intfDecl = VA("[%d]", intfDimenCnt);
				intfParam = VA("ht_uint%d dimenIdx", FindLg2(intfDimenCnt - 1));
				intfParamIdx = "[dimenIdx]";
				dimenList.push_back(pUioIntf->m_dimen);
			}

			m_uioIoDecl.Append("\tsc_in<bool> i_portTo%s_%sUioRdy%s;\n",
				pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioIoDecl.Append("\tsc_in<%s> i_portTo%s_%sUioData%s;\n",
				pUioIntf->m_pType->m_typeName.c_str(), pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioIoDecl.Append("\tsc_out<bool> o_portTo%s_%sUioAFull%s;\n",
				pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());

			m_uioIoDecl.Append("\n");

			m_uioRegDecl.Append("\tht_%s_que<%s, %d> m_portTo%s_%sUioQue%s;\n",
				intfQueueW > 6 ? "block" : "dist",
				pUioIntf->m_pType->m_typeName.c_str(), intfQueueW,
				pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioRegDecl.Append("\tbool r_portToQue_%sUioRdy%s;\n",
				pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioRegDecl.Append("\t%s r_portToQue_%sUioData%s;\n",
				pUioIntf->m_pType->m_typeName.c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioRegDecl.Append("\tbool r_portToQue_%sUioAFull%s;\n",
				pUioIntf->m_name.c_str(), intfDecl.c_str());

			m_uioRegDecl.Append("\tbool c_queTo%s_%sUioRdy%s;\n",
				pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioRegDecl.Append("\t%s c_queTo%s_%sUioData%s;\n",
				pUioIntf->m_pType->m_typeName.c_str(), pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioRegDecl.Append("\tbool c_queTo%s_%sUioPop%s;\n",
				pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());


			m_uioRegDecl.Append("\n");

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					uioPreInstr.Append("\tc_queTo%s_%sUioPop%s = false;\n",
						pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					uioPreInstr.Append("\tc_queTo%s_%sUioRdy%s = !m_portTo%s_%sUioQue%s.empty();\n",
						pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					uioPreInstr.Append("\tc_queTo%s_%sUioData%s = m_portTo%s_%sUioQue%s.front();\n",
						pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}
			uioPreInstr.Append("\n");

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					uioPreInstr.Append("\tif (r_portToQue_%sUioRdy%s)\n",
						pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pUioIntf->m_name.c_str(), intfIdx.c_str());
					uioPreInstr.Append("\t\tm_portTo%s_%sUioQue%s.push(r_portToQue_%sUioData%s);\n",
						pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pUioIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}
			uioPreInstr.Append("\n");

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					uioReg.Append("\tr_portToQue_%sUioRdy%s = i_portTo%s_%sUioRdy%s;\n",
						pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					uioReg.Append("\tr_portToQue_%sUioData%s = i_portTo%s_%sUioData%s;\n",
						pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}
			uioReg.Append("\n");

			{
				int maxReserved = pUioIntf->m_reserve.AsInt();
				HtlAssert(maxReserved < (1 << intfQueueW));

				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					uioReg.Append("\tr_portToQue_%sUioAFull%s = m_portTo%s_%sUioQue%s.full(%d);\n",
						pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
						maxReserved + 6);

				} while (DimenIter(dimenList, refList));
			}
			uioReg.Append("\n");

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					srcUioPostInstr.Append("\tif (c_queTo%s_%sUioPop%s)\n",
						pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

					srcUioPostInstr.Append("\t\tm_portTo%s_%sUioQue%s.pop();\n",
						pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}
			srcUioPostInstr.Append("\n");

			{
				HtdFile::EClkRate srcClkRate = pUioIntf->m_clkRate;

				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					if (srcClkRate == pMod->m_clkRate)
						uioReg.Append("\tm_portTo%s_%sUioQue%s.clock(%s);\n",
						pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(), reset.c_str());
					else if (srcClkRate == eClk2x) {
						m_uioReg2x.Append("\tm_portTo%s_%sUioQue%s.push_clock(c_reset2x);\n",
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
						m_uioReg1x.Append("\tm_portTo%s_%sUioQue%s.pop_clock(r_reset1x);\n",
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
					} else {
						m_uioReg1x.Append("\tm_portTo%s_%sUioQue%s.push_clock(r_reset1x);\n",
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
						m_uioReg2x.Append("\tm_portTo%s_%sUioQue%s.pop_clock(c_reset2x);\n",
							pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
					}
				} while (DimenIter(dimenList, refList));
			}

			uioReg.Append("\n");

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					uioOut.Append("\to_portTo%s_%sUioAFull%s = r_portToQue_%sUioAFull%s;\n",
						pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pUioIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}
			uioOut.Append("\n");

			string vcdModName = VA("Pers%s", pMod->m_modName.Uc().c_str());
			m_uioRegDecl.Append("\tbool ht_noload c_RecvUioBusy_%s%s;\n", pUioIntf->m_name.c_str(), intfDecl.c_str());

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);
					string intfParamIdx = IndexStr(refList, -1, 0, true);

					GenModTrace(eVcdUser, vcdModName, VA("RecvUioBusy_%s(%s)", pUioIntf->m_name.c_str(), intfParamIdx.c_str()),
						VA("c_RecvUioBusy_%s%s", pUioIntf->m_name.c_str(), intfIdx.c_str()));

				} while (DimenIter(dimenList, refList));
			}

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					uioPostReg.Append("\tc_RecvUioBusy_%s%s = !c_queTo%s_%sUioRdy%s;\n",
					pUioIntf->m_name.c_str(), intfIdx.c_str(),
					pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}

			g_appArgs.GetDsnRpt().AddItem("bool RecvUioBusy_%s(%s)\n",
				pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDecl.Append("\tbool RecvUioBusy_%s(%s);\n",
				pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("bool CPers%s::RecvUioBusy_%s(%s)\n",
				pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("{\n");

			if (pUioIntf->m_dimen.AsInt() > 0) {
				m_uioFuncDef.Append("\tassert_msg(dimenIdx < %d, \"Runtime check failed in CPers%s::RecvUioBusy_%s()"
					" - dimenIdx value (%%d) out of range (0-%d)\\n\", (int)dimenIdx);\n",
					pUioIntf->m_dimen.AsInt(), pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(),
					pUioIntf->m_dimen.AsInt() - 1);
			}

			m_uioFuncDef.Append("\treturn c_RecvUioBusy_%s%s;\n",
				pUioIntf->m_name.c_str(), intfParamIdx.c_str());
			m_uioFuncDef.Append("}\n");

			m_uioFuncDef.Append("\n");

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);
					string intfParamIdx = IndexStr(refList, -1, 0, true);

					GenModTrace(eVcdUser, vcdModName, VA("RecvUioReady_%s(%s)", pUioIntf->m_name.c_str(), intfParamIdx.c_str()),
					VA("c_queTo%s_%sUioRdy%s", pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str()));

				} while (DimenIter(dimenList, refList));
			}

			g_appArgs.GetDsnRpt().AddItem("bool RecvUioReady_%s(%s)\n",
				pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDecl.Append("\tbool RecvUioReady_%s(%s);\n",
				pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("bool CPers%s::RecvUioReady_%s(%s)\n",
				pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("{\n");

			if (pUioIntf->m_dimen.AsInt() > 0) {
				m_uioFuncDef.Append("\tassert_msg(dimenIdx < %d, \"Runtime check failed in CPers%s::RecvUioReady_%s()"
					" - dimenIdx value (%%d) out of range (0-%d)\\n\", (int)dimenIdx);\n",
					pUioIntf->m_dimen.AsInt(), pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(),
					pUioIntf->m_dimen.AsInt() - 1);
			}

			m_uioFuncDef.Append("\treturn c_queTo%s_%sUioRdy%s;\n",
				pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());

			m_uioFuncDef.Append("}\n");
			m_uioFuncDef.Append("\n");

			g_appArgs.GetDsnRpt().AddItem("%s PeekUio_%s(%s)\n",
				pUioIntf->m_pType->m_typeName.c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDecl.Append("\t%s PeekUio_%s(%s);\n",
				pUioIntf->m_pType->m_typeName.c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("%s CPers%s::PeekUio_%s(%s)\n",
				pUioIntf->m_pType->m_typeName.c_str(), 
				pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("{\n");

			if (pUioIntf->m_dimen.AsInt() > 0) {
				m_uioFuncDef.Append("\tassert_msg(dimenIdx < %d, \"Runtime check failed in CPers%s::PeekUio_%s()"
					" - dimenIdx value (%%d) out of range (0-%d)\\n\", (int)dimenIdx);\n",
					pUioIntf->m_dimen.AsInt(), pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(),
					pUioIntf->m_dimen.AsInt() - 1);
			}

			m_uioFuncDef.Append("\treturn c_queTo%s_%sUioData%s;\n",
				pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());

			m_uioFuncDef.Append("}\n");
			m_uioFuncDef.Append("\n");

			g_appArgs.GetDsnRpt().AddItem("%s RecvUioData_%s(%s)\n",
				pUioIntf->m_pType->m_typeName.c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDecl.Append("\t%s RecvUioData_%s(%s);\n",
				pUioIntf->m_pType->m_typeName.c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("%s CPers%s::RecvUioData_%s(%s)\n",
				pUioIntf->m_pType->m_typeName.c_str(),
				pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("{\n");

			if (pUioIntf->m_dimen.AsInt() > 0) {
				m_uioFuncDef.Append("\tassert_msg(dimenIdx < %d, \"Runtime check failed in CPers%s::RecvUioData_%s()"
					" - dimenIdx value (%%d) out of range (0-%d)\\n\", (int)dimenIdx);\n",
					pUioIntf->m_dimen.AsInt(), pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(),
					pUioIntf->m_dimen.AsInt() - 1);
			}


			m_uioFuncDef.Append("\tc_queTo%s_%sUioPop%s = true;\n",
				pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());
			m_uioFuncDef.Append("\treturn c_queTo%s_%sUioData%s;\n",
				pMod->m_modName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());

			m_uioFuncDef.Append("}\n");
			m_uioFuncDef.Append("\n");
		}
	}

	if (bIn)
		g_appArgs.GetDsnRpt().EndLevel();


	bool bOut = false;
	for (size_t intfIdx = 0; intfIdx < pMod->m_uioSimIntfList.size(); intfIdx += 1) {
		CUioIntf * pUioIntf = pMod->m_uioSimIntfList[intfIdx];

		CHtCode & uioPreInstr = pMod->m_clkRate == eClk2x ? m_uioPreInstr2x : m_uioPreInstr1x;
		CHtCode & uioReg = pMod->m_clkRate == eClk2x ? m_uioReg2x : m_uioReg1x;
		CHtCode & uioOut = pMod->m_clkRate == eClk2x ? m_uioOut2x : m_uioOut1x;
		CHtCode & uioPostInstr = pMod->m_clkRate == eClk2x ? m_uioPostInstr2x : m_uioPostInstr1x;
		CHtCode & srcUioPostInstr = pUioIntf->m_clkRate == eClk2x ? m_uioPostInstr2x : m_uioPostInstr1x;

		int intfQueueW = pUioIntf->m_queueW.AsInt();

		string reset = pMod->m_clkRate == eClk1x ? "r_reset1x" : "c_reset2x";

		if (!pUioIntf->m_bInbound) {

			if (!bOut) {
				m_uioIoDecl.Append("\t// Outbound User IO Sim interfaces\n");
				g_appArgs.GetDsnRpt().AddLevel("Outbound User IO Sim\n");

				bOut = true;
			}

			string intfDecl;
			string intfParam;
			string intfParamNoload;
			string intfParamIdx;
			vector<CHtString> dimenList;
			if (pUioIntf->m_dimen.AsInt() > 0) {
				intfDecl = VA("[%d]", pUioIntf->m_dimen.AsInt());
				intfParam = VA("ht_uint%d dimenIdx", FindLg2(pUioIntf->m_dimen.AsInt() - 1));
				intfParamNoload = VA("ht_noload ht_uint%d dimenIdx", FindLg2(pUioIntf->m_dimen.AsInt() - 1));
				intfParamIdx = "[dimenIdx]";
				dimenList.push_back(pUioIntf->m_dimen);
			}

			m_uioIoDecl.Append("\tsc_out<bool> o_%sToPort_%sUioRdy%s;\n",
				pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioIoDecl.Append("\tsc_out<%s> o_%sToPort_%sUioData%s;\n",
				pUioIntf->m_pType->m_typeName.c_str(), pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioIoDecl.Append("\tsc_in<bool> i_%sToPort_%sUioAFull%s;\n",
				pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());

			m_uioIoDecl.Append("\n");

			m_uioRegDecl.Append("\tht_%s_que<%s, %d> m_%sToPort_%sUioQue%s;\n",
				intfQueueW > 6 ? "block" : "dist",
				pUioIntf->m_pType->m_typeName.c_str(), intfQueueW,
				pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioRegDecl.Append("\tbool r_queToPort_%sUioRdy%s;\n",
				pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioRegDecl.Append("\t%s r_queToPort_%sUioData%s;\n",
				pUioIntf->m_pType->m_typeName.c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioRegDecl.Append("\tbool r_queToPort_%sUioAFull%s;\n",
				pUioIntf->m_name.c_str(), intfDecl.c_str());

			m_uioRegDecl.Append("\tbool c_%sToQue_%sUioRdy%s;\n",
				pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioRegDecl.Append("\t%s c_%sToQue_%sUioData%s;\n",
				pUioIntf->m_pType->m_typeName.c_str(), pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			m_uioRegDecl.Append("\tbool r_%sToQue_%sUioAFull%s;\n",
				pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfDecl.c_str());
			
			m_uioRegDecl.Append("\n");

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					uioPreInstr.Append("\tc_%sToQue_%sUioRdy%s = false;\n",
						pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
					uioPreInstr.Append("\tc_%sToQue_%sUioData%s = 0;\n",
						pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}
			uioPreInstr.Append("\n");

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					uioReg.Append("\tr_queToPort_%sUioAFull%s = i_%sToPort_%sUioAFull%s;\n",
						pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}

			{

				int maxReserved = pUioIntf->m_reserve.AsInt();
				HtlAssert(maxReserved < (1 << intfQueueW));

				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					uioReg.Append("\tr_%sToQue_%sUioAFull%s = m_%sToPort_%sUioQue%s.full(%d);\n",
						pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
						maxReserved + 6);

				} while (DimenIter(dimenList, refList));
			}
			uioReg.Append("\n");

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					uioPostInstr.Append("\tr_queToPort_%sUioRdy%s = (!m_%sToPort_%sUioQue%s.empty() && !r_queToPort_%sUioAFull%s);\n",
						pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pUioIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					uioPostInstr.Append("\tr_queToPort_%sUioData%s = m_%sToPort_%sUioQue%s.front();\n",
						pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}
			uioPostInstr.Append("\n");

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					srcUioPostInstr.Append("\tif (r_queToPort_%sUioRdy%s)\n",
						pUioIntf->m_name.c_str(), intfIdx.c_str());
					srcUioPostInstr.Append("\t\tm_%sToPort_%sUioQue%s.pop();\n",
						pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					srcUioPostInstr.Append("\tif (c_%sToQue_%sUioRdy%s)\n",
						pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
					srcUioPostInstr.Append("\t\tm_%sToPort_%sUioQue%s.push(c_%sToQue_%sUioData%s);\n",
						pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}
			srcUioPostInstr.Append("\n");

			{
				HtdFile::EClkRate srcClkRate = pUioIntf->m_clkRate;

				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					if (srcClkRate == pMod->m_clkRate)
						uioReg.Append("\tm_%sToPort_%sUioQue%s.clock(%s);\n",
						pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(), reset.c_str());
					else if (srcClkRate == eClk2x) {
						m_uioReg2x.Append("\tm_%sToPort_%sUioQue%s.push_clock(c_reset2x);\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
						m_uioReg1x.Append("\tm_%sToPort_%sUioQue%s.pop_clock(r_reset1x);\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
					} else {
						m_uioReg1x.Append("\tm_%sToPort_%sUioQue%s.push_clock(r_reset1x);\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
						m_uioReg2x.Append("\tm_%sToPort_%sUioQue%s.pop_clock(c_reset2x);\n",
							pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str());
					}
				} while (DimenIter(dimenList, refList));
			}
			uioReg.Append("\n");

			{
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);

					uioOut.Append("\to_%sToPort_%sUioRdy%s = r_queToPort_%sUioRdy%s;\n",
						pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pUioIntf->m_name.c_str(), intfIdx.c_str());
					uioOut.Append("\to_%sToPort_%sUioData%s = r_queToPort_%sUioData%s;\n",
						pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str(),
						pUioIntf->m_name.c_str(), intfIdx.c_str());

				} while (DimenIter(dimenList, refList));
			}
			uioOut.Append("\n");

			{
				string vcdModName = VA("Pers%s", pInst->m_instName.Uc().c_str());
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);
					string intfParamIdx = IndexStr(refList, -1, 0, true);

					GenModTrace(eVcdUser, vcdModName, VA("SendUioBusy_%s(%s)", pUioIntf->m_name.c_str(), intfParamIdx.c_str()),
						VA("r_%sToQue_%sUioAFull%s", pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str()));

				} while (DimenIter(dimenList, refList));
			}

			g_appArgs.GetDsnRpt().AddItem("bool SendUioBusy_%s(%s)\n",
				pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDecl.Append("\tbool SendUioBusy_%s(%s);\n",
				pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("bool CPers%s::SendUioBusy_%s(%s)\n",
				pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("{\n");

			if (pUioIntf->m_dimen.AsInt() > 0) {
				m_uioFuncDef.Append("\tassert_msg(dimenIdx < %d, \"Runtime check failed in CPers%s::SendUioBusy_%s()"
					" - dimenIdx value (%%d) out of range (0-%d)\\n\", (int)dimenIdx);\n",
					pUioIntf->m_dimen.AsInt(), pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(),
					pUioIntf->m_dimen.AsInt() - 1);
			}

			m_uioFuncDef.Append("\treturn r_%sToQue_%sUioAFull%s;\n",
				pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());

			m_uioFuncDef.Append("}\n");
			m_uioFuncDef.Append("\n");

			{
				string vcdModName = VA("Pers%s", pMod->m_modName.Uc().c_str());
				vector<int> refList(dimenList.size());
				do {
					string intfIdx = IndexStr(refList);
					string intfParamIdx = IndexStr(refList, -1, 0, true);

					GenModTrace(eVcdUser, vcdModName, VA("SendUioFull_%s(%s)", pUioIntf->m_name.c_str(), intfParamIdx.c_str()),
						VA("r_%sToQue_%sUioAFull%s", pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfIdx.c_str()));

				} while (DimenIter(dimenList, refList));
			}

			g_appArgs.GetDsnRpt().AddItem("bool SendUioFull_%s(%s)\n",
				pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDecl.Append("\tbool SendUioFull_%s(%s);\n",
				pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("bool CPers%s::SendUioFull_%s(%s)\n",
				pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParam.c_str());
			m_uioFuncDef.Append("{\n");

			if (pUioIntf->m_dimen.AsInt() > 0) {
				m_uioFuncDef.Append("\tassert_msg(dimenIdx < %d, \"Runtime check failed in CPers%s::SendUioFull_%s()"
					" - dimenIdx value (%%d) out of range (0-%d)\\n\", (int)dimenIdx);\n",
					pUioIntf->m_dimen.AsInt(), pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(),
					pUioIntf->m_dimen.AsInt() - 1);
			}

			m_uioFuncDef.Append("\treturn r_%sToQue_%sUioAFull%s;\n",
				pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());

			m_uioFuncDef.Append("}\n");
			m_uioFuncDef.Append("\n");

			if (intfParam.size() > 0) intfParam += ", ";

			g_appArgs.GetDsnRpt().AddItem("void SendUioData_%s(%s%s data)\n",
				pUioIntf->m_name.c_str(), intfParam.c_str(), pUioIntf->m_pType->m_typeName.c_str());
			m_uioFuncDecl.Append("\tvoid SendUioData_%s(%s%s data);\n",
				pUioIntf->m_name.c_str(), intfParam.c_str(), pUioIntf->m_pType->m_typeName.c_str());
			m_uioFuncDef.Append("void CPers%s::SendUioData_%s(%s%s data)\n",
				pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(), intfParam.c_str(), pUioIntf->m_pType->m_typeName.c_str());
			m_uioFuncDef.Append("{\n");

			if (pUioIntf->m_dimen.AsInt() > 0) {
				m_uioFuncDef.Append("\tassert_msg(dimenIdx < %d, \"Runtime check failed in CPers%s::SendUioData_%s()"
					" - dimenIdx value (%%d) out of range (0-%d)\\n\", (int)dimenIdx);\n",
					pUioIntf->m_dimen.AsInt(), pInst->m_instName.Uc().c_str(), pUioIntf->m_name.c_str(),
					pUioIntf->m_dimen.AsInt() - 1);
			}

			m_uioFuncDef.Append("\tc_%sToQue_%sUioRdy%s = true;\n",
				pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());
			m_uioFuncDef.Append("\tc_%sToQue_%sUioData%s = data;\n",
				pMod->m_modName.Lc().c_str(), pUioIntf->m_name.c_str(), intfParamIdx.c_str());

			m_uioFuncDef.Append("}\n");
			m_uioFuncDef.Append("\n");
		}
	}

	if (bOut)
		g_appArgs.GetDsnRpt().EndLevel();
}

void CDsnInfo::GenModUioSimCsrStatements(CInstance * pInst)
{
	CModule * pMod = pInst->m_pMod;

	if (pMod->m_uioCsrIntfList.size() == 0)
		return;

	// message interface
	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

	for (size_t intfIdx = 0; intfIdx < pMod->m_uioCsrIntfList.size(); intfIdx += 1) {

		CHtCode & uioPreInstr = pMod->m_clkRate == eClk2x ? m_uioPreInstr2x : m_uioPreInstr1x;
		CHtCode & srcUioPostInstr = pMod->m_clkRate == eClk2x ? m_uioPostInstr2x : m_uioPostInstr1x;
		CHtCode & uioReg = pMod->m_clkRate == eClk2x ? m_uioReg2x : m_uioReg1x;
		CHtCode & uioPostReg = pMod->m_clkRate == eClk2x ? m_uioPostReg2x : m_uioPostReg1x;

		string reset = pMod->m_clkRate == eClk1x ? "r_reset1x" : "c_reset2x";


		m_uioIoDecl.Append("\t// User IO Sim Csr interface\n");
		g_appArgs.GetDsnRpt().AddLevel("User IO Sim Csr\n");


		m_uioRegDecl.Append("\tht_dist_que<uio_csr_rq_t, 2> m_rq_csrUioQue;\n");
		m_uioRegDecl.Append("\tht_dist_que<uio_csr_rs_t, 2> m_rs_csrUioQue;\n");

		m_uioRegDecl.Append("\tbool r_rq_queTo%s_csrUioRdy;\n", pMod->m_modName.Uc().c_str());
		m_uioRegDecl.Append("\tuio_csr_rq_t r_rq_queTo%s_csrUioData;\n", pMod->m_modName.Uc().c_str());
		m_uioRegDecl.Append("\tbool c_rq_queTo%s_csrUioPop;\n", pMod->m_modName.Uc().c_str());

		m_uioRegDecl.Append("\tbool c_rs_%sToQue_csrUioRdy;\n", pMod->m_modName.Lc().c_str());
		m_uioRegDecl.Append("\tuio_csr_rs_t c_rs_%sToQue_csrUioData;\n", pMod->m_modName.Lc().c_str());
		m_uioRegDecl.Append("\tbool r_rs_%sToQue_csrUioFull;\n", pMod->m_modName.Lc().c_str());

		m_uioRegDecl.Append("\n");

		{
			uioPreInstr.Append("\tc_rq_queTo%s_csrUioPop = false;\n",
				pMod->m_modName.Uc().c_str());
		}
		uioPreInstr.Append("\n");

		{
			uioPreInstr.Append("\tc_rs_%sToQue_csrUioRdy = false;\n",
				pMod->m_modName.Lc().c_str());
		}
		uioPreInstr.Append("\n");

		{
			uioPreInstr.Append("\tr_rq_queTo%s_csrUioRdy = !m_rq_csrUioQue.empty();\n",
				pMod->m_modName.Uc().c_str());

			uioPreInstr.Append("\tr_rq_queTo%s_csrUioData = m_rq_csrUioQue.front();\n",
				pMod->m_modName.Uc().c_str());
		}
		uioPreInstr.Append("\n");

		{
			uioPreInstr.Append("\tr_rs_%sToQue_csrUioFull = m_rs_csrUioQue.full();\n",
				pMod->m_modName.Lc().c_str());
		}
		uioPreInstr.Append("\n");

		{
			srcUioPostInstr.Append("\tif (c_rs_%sToQue_csrUioRdy)\n", pMod->m_modName.Lc().c_str());
			srcUioPostInstr.Append("\t\tm_rs_csrUioQue.push(c_rs_%sToQue_csrUioData);\n", pMod->m_modName.Lc().c_str());
		}
		srcUioPostInstr.Append("\n");

		{
			srcUioPostInstr.Append("\tif (c_rq_queTo%s_csrUioPop)\n", pMod->m_modName.Uc().c_str());
			srcUioPostInstr.Append("\t\tm_rq_csrUioQue.pop();\n");
		}
		srcUioPostInstr.Append("\n");

		{
			uioReg.Append("\tm_rq_csrUioQue.clock(%s);\n",
				reset.c_str());
			uioReg.Append("\tm_rs_csrUioQue.clock(%s);\n",
				reset.c_str());
		}
		uioReg.Append("\n");

		string vcdModName = VA("Pers%s", pMod->m_modName.Uc().c_str());

		m_uioFuncDecl.Append("\tbool UioCsrCmd(int cmd, uint64_t addr, uint64_t data);\n");
		m_uioFuncDef.Append("bool CPers%s::UioCsrCmd(int cmd, uint64_t addr, uint64_t data)\n",
			pInst->m_instName.Uc().c_str());
		m_uioFuncDef.Append("{\n");

		m_uioFuncDef.Append("\tuio_csr_rq_t tmpRq;\n");
		m_uioFuncDef.Append("\ttmpRq.cmd = cmd;\n");
		m_uioFuncDef.Append("\ttmpRq.addr = addr;\n");
		m_uioFuncDef.Append("\ttmpRq.data = data;\n");
		m_uioFuncDef.Append("\n");
		m_uioFuncDef.Append("\tif (m_rq_csrUioQue.full()) {\n");
		m_uioFuncDef.Append("\t\treturn false;\n");
		m_uioFuncDef.Append("\t} else {\n");
		m_uioFuncDef.Append("\t\tm_rq_csrUioQue.push(tmpRq);\n");
		m_uioFuncDef.Append("\t\treturn true;\n");
		m_uioFuncDef.Append("\t}\n");
		m_uioFuncDef.Append("}\n");
		m_uioFuncDef.Append("\n");

		m_uioFuncDecl.Append("#\tifndef _HTV\n");
		m_uioFuncDecl.Append("\tstatic bool ext_UioCsrCmd(void *self, int cmd, uint64_t addr, uint64_t data);\n");
		m_uioFuncDecl.Append("#\tendif\n");
		m_uioFuncDef.Append("bool CPers%s::ext_UioCsrCmd(void *self, int cmd, uint64_t addr, uint64_t data)\n",
			pInst->m_instName.Uc().c_str());
		m_uioFuncDef.Append("{\n");

		m_uioFuncDef.Append("\treturn static_cast<CPers%s *>(self)->UioCsrCmd(cmd, addr, data);\n", pInst->m_instName.Uc().c_str());
		m_uioFuncDef.Append("}\n");
		m_uioFuncDef.Append("\n");

		m_uioFuncDecl.Append("\tbool UioCsrRdRsp(uint64_t &data);\n");
		m_uioFuncDef.Append("bool CPers%s::UioCsrRdRsp(uint64_t &data)\n",
			pInst->m_instName.Uc().c_str());
		m_uioFuncDef.Append("{\n");

		m_uioFuncDef.Append("\tuio_csr_rs_t tmpRs;\n");
		m_uioFuncDef.Append("\tif (!m_rs_csrUioQue.empty()) {\n");
		m_uioFuncDef.Append("\t\ttmpRs = m_rs_csrUioQue.front();\n");
		m_uioFuncDef.Append("\t\tdata = tmpRs.data;\n");
		m_uioFuncDef.Append("\t\tm_rs_csrUioQue.pop();\n");
		m_uioFuncDef.Append("\n");
		m_uioFuncDef.Append("\t\treturn true;\n");
		m_uioFuncDef.Append("\t} else {\n");
		m_uioFuncDef.Append("\t\treturn false;\n");
		m_uioFuncDef.Append("\t}\n");
		m_uioFuncDef.Append("}\n");
		m_uioFuncDef.Append("\n");

		m_uioFuncDecl.Append("#\tifndef _HTV\n");
		m_uioFuncDecl.Append("\tstatic bool ext_UioCsrRdRsp(void *self, uint64_t &data);\n");
		m_uioFuncDecl.Append("#\tendif\n");
		m_uioFuncDef.Append("bool CPers%s::ext_UioCsrRdRsp(void *self, uint64_t &data)\n",
			pInst->m_instName.Uc().c_str());
		m_uioFuncDef.Append("{\n");

		m_uioFuncDef.Append("\treturn static_cast<CPers%s *>(self)->UioCsrRdRsp(data);\n", pInst->m_instName.Uc().c_str());
		m_uioFuncDef.Append("}\n");
		m_uioFuncDef.Append("\n");

		m_uioRegDecl.Append("\tbool ht_noload c_RecvUioCsrCmdBusy;\n");

		{
			GenModTrace(eVcdUser, vcdModName, VA("RecvUioCsrCmdBusy()"),
				VA("c_RecvUioCsrCmdBusy"));
		}

		{
			uioPostReg.Append("\tc_RecvUioCsrCmdBusy = !r_rq_queTo%s_csrUioRdy;\n", pMod->m_modName.Uc().c_str());
		}

		g_appArgs.GetDsnRpt().AddItem("bool RecvUioCsrCmdBusy()\n");
		m_uioFuncDecl.Append("\tbool RecvUioCsrCmdBusy();\n");
		m_uioFuncDef.Append("bool CPers%s::RecvUioCsrCmdBusy()\n",
			pInst->m_instName.Uc().c_str());
		m_uioFuncDef.Append("{\n");

		m_uioFuncDef.Append("\treturn c_RecvUioCsrCmdBusy;\n");
		m_uioFuncDef.Append("}\n");

		m_uioFuncDef.Append("\n");

		{
			GenModTrace(eVcdUser, vcdModName, VA("RecvUioCsrCmdReady()"),
				 VA("r_rq_queTo%s_csrUioRdy", pMod->m_modName.Uc().c_str()));

		}

		g_appArgs.GetDsnRpt().AddItem("bool RecvUioCsrCmdReady()\n");
		m_uioFuncDecl.Append("\tbool RecvUioCsrCmdReady();\n");
		m_uioFuncDef.Append("bool CPers%s::RecvUioCsrCmdReady()\n",
			pInst->m_instName.Uc().c_str());
		m_uioFuncDef.Append("{\n");

		m_uioFuncDef.Append("\treturn r_rq_queTo%s_csrUioRdy;\n", pMod->m_modName.Uc().c_str());

		m_uioFuncDef.Append("}\n");
		m_uioFuncDef.Append("\n");

		g_appArgs.GetDsnRpt().AddItem("uio_csr_rq_t RecvUioCsrCmd()\n");
		m_uioFuncDecl.Append("\tuio_csr_rq_t RecvUioCsrCmd();\n");
		m_uioFuncDef.Append("uio_csr_rq_t CPers%s::RecvUioCsrCmd()\n",
			pInst->m_instName.Uc().c_str());
		m_uioFuncDef.Append("{\n");


		m_uioFuncDef.Append("\tc_rq_queTo%s_csrUioPop = true;\n", pMod->m_modName.Uc().c_str());
		m_uioFuncDef.Append("\treturn r_rq_queTo%s_csrUioData;\n", pMod->m_modName.Uc().c_str());

		m_uioFuncDef.Append("}\n");
		m_uioFuncDef.Append("\n");

		{
		       	GenModTrace(eVcdUser, vcdModName, VA("SendUioCsrRdRspBusy()"),
				    VA("r_rs_%sToQue_csrUioFull", pMod->m_modName.Lc().c_str()));
		}

		g_appArgs.GetDsnRpt().AddItem("bool SendUioCsrRdRspBusy()\n");
		m_uioFuncDecl.Append("\tbool SendUioCsrRdRspBusy();\n");
		m_uioFuncDef.Append("bool CPers%s::SendUioCsrRdRspBusy()\n",
			pInst->m_instName.Uc().c_str());
		m_uioFuncDef.Append("{\n");

		m_uioFuncDef.Append("\treturn r_rs_%sToQue_csrUioFull;\n", pMod->m_modName.Lc().c_str());

		m_uioFuncDef.Append("}\n");
		m_uioFuncDef.Append("\n");

		{
			GenModTrace(eVcdUser, vcdModName, VA("SendUioCsrRdRspFull()"),
				    VA("r_rs_%sToQue_csrUioFull", pMod->m_modName.Lc().c_str()));
		}

		g_appArgs.GetDsnRpt().AddItem("bool SendUioCsrRdRspFull()\n");
		m_uioFuncDecl.Append("\tbool SendUioCsrRdRspFull();\n");
		m_uioFuncDef.Append("bool CPers%s::SendUioCsrRdRspFull()\n",
			pInst->m_instName.Uc().c_str());
		m_uioFuncDef.Append("{\n");

		m_uioFuncDef.Append("\treturn r_rs_%sToQue_csrUioFull;\n", pMod->m_modName.Lc().c_str());

		m_uioFuncDef.Append("}\n");
		m_uioFuncDef.Append("\n");

		g_appArgs.GetDsnRpt().AddItem("void SendUioCsrRdRsp(uint64_t data)\n");
		m_uioFuncDecl.Append("\tvoid SendUioCsrRdRsp(uint64_t data);\n");
		m_uioFuncDef.Append("void CPers%s::SendUioCsrRdRsp(uint64_t data)\n",
			pInst->m_instName.Uc().c_str());
		m_uioFuncDef.Append("{\n");

		m_uioFuncDef.Append("\tuio_csr_rs_t tmpRs;\n");
		m_uioFuncDef.Append("\ttmpRs.data = data;\n");
		m_uioFuncDef.Append("\n");
		m_uioFuncDef.Append("\tc_rs_%sToQue_csrUioRdy = true;\n", pMod->m_modName.Lc().c_str());
		m_uioFuncDef.Append("\tc_rs_%sToQue_csrUioData = tmpRs;\n", pMod->m_modName.Lc().c_str());

		m_uioFuncDef.Append("}\n");
		m_uioFuncDef.Append("\n");
	}

	g_appArgs.GetDsnRpt().EndLevel();
}

void CDsnInfo::GenerateUioStubFiles()
{

	bool isSyscSim = false;
	for (int i = 0; i < g_appArgs.GetPreDefinedNameCnt(); i++) {
		if (g_appArgs.GetPreDefinedName(i) == "HT_SYSC") {
			isSyscSim = true;
			break;
		}
	}
	bool isWx = false;
	if (strcasestr(g_appArgs.GetCoprocName(), "wx") != NULL) {
		isWx = true;
	}

	// Not needed in sysc sim or non-wx boards
	if (isSyscSim || !isWx || g_appArgs.GetUioPortCnt() == 0)
		return;

	if (GetUioIntfConnListSize() == 0)
		return;

	// Stub Files
	{
		HtiFile::CUioIntfConn * pUioIntfConn = GetUioIntfConn(0); // All Types are the same...so just grab the first conn ptr

		string fileName = g_appArgs.GetOutputFolder() + "/PersUioStub.h";
	
		CHtFile hFile(fileName, "w");
	
		GenerateBanner(hFile, fileName.c_str(), false);
	
		fprintf(hFile, "#pragma once\n");
		fprintf(hFile, "\n");
		fprintf(hFile, "#include \"Ht.h\"\n");
		fprintf(hFile, "\n");
		fprintf(hFile, "struct faketype_t {\n");
		fprintf(hFile, "\tht_uint1 foo[%d];\n", pUioIntfConn->m_pType->GetPackedBitWidth());
		fprintf(hFile, "};\n");
		fprintf(hFile, "\n");
	
		fprintf(hFile, "SC_MODULE(CPersUioStub) {\n");
	
		fprintf(hFile, "\t// User IO Interface\n");
		fprintf(hFile, "\tsc_in<bool> ht_noload o_stubOut_Rdy;\n");
		fprintf(hFile, "\tsc_in<faketype_t> ht_noload o_stubOut_Data;\n");
		fprintf(hFile, "\tsc_out<bool> i_stubOut_AFull;\n");
		fprintf(hFile, "\n");
	
		fprintf(hFile, "\tsc_out<bool> i_stubIn_Rdy;\n");
		fprintf(hFile, "\tsc_out<faketype_t> i_stubIn_Data;\n");
		fprintf(hFile, "\tsc_in<bool> ht_noload o_stubIn_AFull;\n");
		fprintf(hFile, "\n");
	
		fprintf(hFile, "\tvoid PersUioStub();\n");
		fprintf(hFile, "\n");
	
		fprintf(hFile, "\tSC_CTOR(CPersUioStub) {\n");
		fprintf(hFile, "\t\tSC_METHOD(PersUioStub);\n");
		fprintf(hFile, "\t}\n");
		fprintf(hFile, "\n");
	
		fprintf(hFile, "};\n");
	
		hFile.FileClose();
	
	
		fileName = g_appArgs.GetOutputFolder() + "/PersUioStub.cpp";
	
		CHtFile cppFile(fileName, "w");
	
		GenerateBanner(cppFile, fileName.c_str(), false);
	
		fprintf(cppFile, "#include \"Ht.h\"\n");
		fprintf(cppFile, "#include \"PersUioStub.h\"\n");
		fprintf(cppFile, "\n");
	
		fprintf(cppFile, "void CPersUioStub::PersUioStub()\n");
		fprintf(cppFile, "{\n");
	
		fprintf(cppFile, "\ti_stubOut_AFull = false;\n");
		fprintf(cppFile, "\n");
		fprintf(cppFile, "\ti_stubIn_Rdy = false;\n");
		fprintf(cppFile, "\ti_stubIn_Data = 0;\n");
		fprintf(cppFile, "\n");
	
		fprintf(cppFile, "}\n");
	
		cppFile.FileClose();
	}

	// SyscUio Files
	{
		HtiFile::CUioIntfConn * pUioIntfConn = GetUioIntfConn(0); // All Types are the same...so just grab the first conn ptr

		string fileName = g_appArgs.GetOutputFolder() + "/SyscUio.h";
	
		CHtFile hFile(fileName, "w");
	
		GenerateBanner(hFile, fileName.c_str(), false);
	
		fprintf(hFile, "#pragma once\n");
		fprintf(hFile, "\n");
	
		fprintf(hFile, "SC_MODULE(CSyscUio) {\n");
	
		fprintf(hFile, "\tsc_in<bool> i_clock1x;\n");
		fprintf(hFile, "\tsc_in<bool> i_reset;\n");
		fprintf(hFile, "\n");

		fprintf(hFile, "\t// User IO Interface\n");
		fprintf(hFile, "\tsc_out<bool> o_auToPort_uio_AFull;\n");
		fprintf(hFile, "\tsc_in<%s> i_auToPort_uio_Data;\n", pUioIntfConn->m_pType->m_typeName.c_str());
		fprintf(hFile, "\tsc_in<bool> i_auToPort_uio_Rdy;\n");
		fprintf(hFile, "\n");
	
		fprintf(hFile, "\tsc_in<bool> i_portToAu_uio_AFull;\n");
		fprintf(hFile, "\tsc_out<%s> o_portToAu_uio_Data;\n", pUioIntfConn->m_pType->m_typeName.c_str());
		fprintf(hFile, "\tsc_out<bool> o_portToAu_uio_Rdy;\n");
		fprintf(hFile, "\n");
	
		fprintf(hFile, "\tvoid SyscUio();\n");
		fprintf(hFile, "\n");
	
		fprintf(hFile, "\t// Constructor\n");
		fprintf(hFile, "\tSC_CTOR(CSyscUio) {\n");
		fprintf(hFile, "\t\tSC_METHOD(SyscUio);\n");
		fprintf(hFile, "\t\tsensitive << i_clock1x.pos();\n");
		fprintf(hFile, "\t}\n");
	
		fprintf(hFile, "};\n");
	
		hFile.FileClose();
	
		fileName = g_appArgs.GetOutputFolder() + "/SyscUio.cpp";
	
		CHtFile cppFile(fileName, "w");
	
		GenerateBanner(cppFile, fileName.c_str(), false);
	
		fprintf(cppFile, "#include \"Ht.h\"\n");
	
		cppFile.FileClose();
	}



}
