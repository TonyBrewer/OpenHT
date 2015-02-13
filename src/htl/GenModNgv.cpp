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

void CDsnInfo::InitAndValidateModNgv()
{
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed || mod.m_globalVarList.size() == 0) continue;

		// create unique list of global variables for entire unit
		for (size_t mgvIdx = 0; mgvIdx < mod.m_globalVarList.size(); mgvIdx += 1) {
			CRam * pMgv = mod.m_globalVarList[mgvIdx];

			if (pMgv->m_addr1W.AsInt() == 0) {
				pMgv->m_bMaxIw = false;
				pMgv->m_bMaxMw = false;
			}

			size_t gvIdx;
			for (gvIdx = 0; gvIdx < m_ngvList.size(); gvIdx += 1) {
				CNgvInfo * pNgvInfo = m_ngvList[gvIdx];
				CRam * pNgv = pNgvInfo->m_modInfoList[0].m_pNgv;
				if (pMgv->m_gblName == pNgv->m_gblName)
					break;
			}

			if (gvIdx < m_ngvList.size()) {
				CNgvInfo * pNgvInfo = m_ngvList[gvIdx];
				CRam * pNgv = pNgvInfo->m_modInfoList[0].m_pNgv;

				// verify global variables have same parameters
				if (pMgv->m_type != pNgv->m_type) {
					ParseMsg(Error, pMgv->m_lineInfo, "global variable '%s' declared with inconsistent types", pMgv->m_gblName.c_str());
					ParseMsg(Error, pNgv->m_lineInfo, "previous declaration");
				}

				if (pMgv->m_dimenList.size() != pNgv->m_dimenList.size() ||
					pMgv->m_dimenList.size() > 0 && pMgv->m_dimenList[0].AsInt() != pNgv->m_dimenList[0].AsInt() ||
					pMgv->m_dimenList.size() > 1 && pMgv->m_dimenList[1].AsInt() != pNgv->m_dimenList[1].AsInt()) {
					ParseMsg(Error, pMgv->m_lineInfo, "global variable '%s' declared with inconsistent dimensions", pMgv->m_gblName.c_str());
					ParseMsg(Error, pNgv->m_lineInfo, "previous declaration");
				}

				if (pMgv->m_addr1W.AsInt() != pNgv->m_addr1W.AsInt() || pMgv->m_addr2W.AsInt() != pNgv->m_addr2W.AsInt()) {
					ParseMsg(Error, pMgv->m_lineInfo, "global variable '%s' declared with inconsistent addressing", pMgv->m_gblName.c_str());
					ParseMsg(Error, pNgv->m_lineInfo, "previous declaration");
				}

				if (m_ngvList[gvIdx]->m_ramType == eAutoRam) {
					m_ngvList[gvIdx]->m_ramType = pMgv->m_ramType;
				} else if (pMgv->m_ramType != eAutoRam && m_ngvList[gvIdx]->m_ramType != pMgv->m_ramType) {
					ParseMsg(Error, pMgv->m_lineInfo, "global variable '%s' declared with inconsistent ramType", pMgv->m_gblName.c_str());
				}

				m_ngvList[gvIdx]->m_modInfoList.push_back(CNgvModInfo(&mod, pMgv));
				pMgv->m_pNgvInfo = m_ngvList[gvIdx];

			} else {
				// add new global variable to unit list
				m_ngvList.push_back(new CNgvInfo());
				m_ngvList.back()->m_modInfoList.push_back(CNgvModInfo(&mod, pMgv));
				m_ngvList.back()->m_ramType = pMgv->m_ramType;
				pMgv->m_pNgvInfo = m_ngvList.back();
			}
		}
	}

	for (size_t gvIdx = 0; gvIdx < m_ngvList.size(); gvIdx += 1) {
		CNgvInfo * pNgvInfo = m_ngvList[gvIdx];
		CRam * pGv = pNgvInfo->m_modInfoList[0].m_pNgv;

		vector<CNgvModInfo> &ngvModInfoList = m_ngvList[gvIdx]->m_modInfoList;

		// create ngv port list of instruction and memory writes
		vector<pair<int, int> > ngvPortList;
		bool bAllModClk1x = true;
		bool bNgvMaxSel = false;
		for (size_t modIdx = 0; modIdx < ngvModInfoList.size(); modIdx += 1) {
			CModule * pMod = ngvModInfoList[modIdx].m_pMod;
			CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

			for (int imIdx = 0; imIdx < 2; imIdx += 1) {
				if (imIdx == 1 && !pModNgv->m_bMifRead) continue;

				ngvPortList.push_back(pair<int, int>(modIdx, imIdx));
				bAllModClk1x &= pMod->m_clkRate == eClk1x;
			}

			bNgvMaxSel |= pModNgv->m_bMaxIw || pModNgv->m_bMaxMw;
		}

		// determine if a single field in ngv
		int ngvFieldCnt = 0;
		bool bNgvAtomicFast = false;
		bool bNgvAtomicSlow = false;
		for (CStructElemIter iter(this, pGv->m_type); !iter.end(); iter++) {
			if (iter.IsStructOrUnion()) continue;

			ngvFieldCnt += 1;
			bNgvAtomicFast |= (iter->m_atomicMask & (ATOMIC_INC | ATOMIC_SET)) != 0;
			bNgvAtomicSlow |= (iter->m_atomicMask & ATOMIC_ADD) != 0;
		}
		bool bNgvAtomic = bNgvAtomicFast || bNgvAtomicSlow;

		pNgvInfo->m_ngvFieldCnt = ngvFieldCnt;
		pNgvInfo->m_bNgvAtomicFast = bNgvAtomicFast;
		pNgvInfo->m_bNgvAtomicSlow = bNgvAtomicSlow;

		// determine type of ram
		bool bNgvReg = pGv->m_addr1W.AsInt() == 0;
		bool bNgvDist = !bNgvReg && pGv->m_pNgvInfo->m_ramType != eBlockRam;
		bool bNgvBlock = !bNgvReg && !bNgvDist;

		bNgvMaxSel &= bNgvDist && (!bAllModClk1x || ngvPortList.size() > 1) && bNgvAtomicSlow ||
			bNgvBlock && (bNgvAtomic || ngvFieldCnt > 1);

		pNgvInfo->m_bNgvMaxSel = bNgvMaxSel;

		pNgvInfo->m_bNgvWrCompClk2x = bNgvReg && (!bAllModClk1x && ngvPortList.size() <= 2 || ngvPortList.size() == 3) ||
			bNgvDist && ((!bAllModClk1x && ngvPortList.size() <= 2 || ngvPortList.size() == 3) && !bNgvAtomicSlow ||
			(bNgvAtomicSlow && (!bAllModClk1x || ngvPortList.size() > 1) && bNgvMaxSel)) ||
			bNgvBlock && ((!bNgvAtomic && ngvFieldCnt == 1) && (!bAllModClk1x && ngvPortList.size() <= 2 || ngvPortList.size() == 3) ||
			(bNgvAtomic || ngvFieldCnt > 1) && bNgvMaxSel);

		pNgvInfo->m_bNgvWrDataClk2x = bNgvReg && (!bAllModClk1x || ngvPortList.size() > 1) ||
			bNgvDist && ((!bAllModClk1x || ngvPortList.size() > 1) && !bNgvAtomicSlow ||
			(bNgvAtomicSlow && (!bAllModClk1x || ngvPortList.size() > 1) && bNgvMaxSel)) ||
			bNgvBlock && ((!bNgvAtomic && ngvFieldCnt == 1) && (!bAllModClk1x || ngvPortList.size() > 1) ||
			(bNgvAtomic || ngvFieldCnt > 1) && bNgvMaxSel);
	}
}

void CDsnInfo::GenModNgvStatements(CModule &mod)
{
	if (mod.m_globalVarList.size() == 0)
		return;

	CHtCode & gblPreInstr = mod.m_clkRate == eClk2x ? m_gblPreInstr2x : m_gblPreInstr1x;
	CHtCode & gblPostInstr = mod.m_clkRate == eClk2x ? m_gblPostInstr2x : m_gblPostInstr1x;
	CHtCode & gblReg = mod.m_clkRate == eClk2x ? m_gblReg2x : m_gblReg1x;
	CHtCode & gblOut = mod.m_clkRate == eClk2x ? m_gblOut2x : m_gblOut1x;

	string vcdModName = VA("Pers%s", mod.m_modName.Uc().c_str());

	bool bFirstModVar = false;
	for (size_t gvIdx = 0; gvIdx < mod.m_globalVarList.size(); gvIdx += 1) {
		CRam * pGv = mod.m_globalVarList[gvIdx];
		CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;

		for (int privWrIdx = 1; privWrIdx <= mod.m_stage.m_privWrStg.AsInt(); privWrIdx += 1) {
			char privIdxStr[8];
			if (mod.m_stage.m_privWrStg.AsInt() > 1)
				sprintf(privIdxStr, "%d", privWrIdx);
			else
				privIdxStr[0] = '\0';

			if (pNgvInfo->m_atomicMask != 0)
				GenModVar(eVcdUser, vcdModName, bFirstModVar,
					VA("%s const", pGv->m_type.c_str()),
					pGv->m_dimenDecl,
					VA("GF%s_%s", privIdxStr, pGv->m_gblName.c_str()),
					VA("r_t%d_%sIfData", mod.m_tsStg+privWrIdx-1, pGv->m_gblName.c_str()),
					pGv->m_dimenList);

			GenModVar(eVcdUser, vcdModName, bFirstModVar,
				VA("%s const", pGv->m_type.c_str()),
				pGv->m_dimenDecl,
				VA("GR%s_%s", privIdxStr, pGv->m_gblName.c_str()),
				VA("r_t%d_%sIrData", mod.m_tsStg+privWrIdx-1, pGv->m_gblName.c_str()),
				pGv->m_dimenList);

			GenModVar(eVcdNone, vcdModName, bFirstModVar,
				VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
				pGv->m_dimenDecl,
				VA("GW%s_%s", privIdxStr, pGv->m_gblName.c_str()),
				VA("c_t%d_%sIwData", mod.m_tsStg+privWrIdx-1, pGv->m_gblName.c_str()),
				pGv->m_dimenList);
		}
	}

	// ht completion struct
	m_gblRegDecl.Append("\tstruct CHtComp {\n");

	CModInst & modInst = mod.m_modInstList[0];

	if (mod.m_threads.m_htIdW.AsInt() > 0)
		m_gblRegDecl.Append("\t\tsc_uint<%s_HTID_W> m_htId;\n", mod.m_modName.Upper().c_str());

	for (size_t gvIdx = 0; gvIdx < mod.m_globalVarList.size(); gvIdx += 1) {
		CRam * pGv = mod.m_globalVarList[gvIdx];

		m_gblRegDecl.Append("\t\tbool m_%sIwComp%s;\n", pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
	}
	m_gblRegDecl.Append("\t\tbool m_htCmdRdy;\n");

	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrIn) continue;
		if (cxrIntf.m_pDstMod->m_modName.AsStr() == "hif") continue;

		m_gblRegDecl.Append("\t\tbool m_%sCompRdy;\n", cxrIntf.GetIntfName());
	}

	m_gblRegDecl.Append("\t};\n");

	for (size_t gvIdx = 0; gvIdx < mod.m_globalVarList.size(); gvIdx += 1) {
		CRam * pGv = mod.m_globalVarList[gvIdx];
		CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;

		CHtCode & gblPostInstrWrData = pNgvInfo->m_bNgvWrDataClk2x ? m_gblPostInstr2x : m_gblPostInstr1x;
		CHtCode & gblPostInstrWrComp = pNgvInfo->m_bNgvWrCompClk2x ? m_gblPostInstr2x : m_gblPostInstr1x;

		CHtCode & gblRegWrData = pNgvInfo->m_bNgvWrDataClk2x ? m_gblReg2x : m_gblReg1x;
		CHtCode & gblRegWrComp = pNgvInfo->m_bNgvWrCompClk2x ? m_gblReg2x : m_gblReg1x;

		// global variable module I/O
		{ // Instruction
			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				m_gblIoDecl.Append("\tsc_out<sc_uint<%s_HTID_W> > o_%sTo%s_iwHtId%s;\n",
					mod.m_modName.Upper().c_str(),
					mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblOut.Append("\to_%sTo%s_iwHtId%s = r_t%d_htId;\n",
						mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						mod.m_tsStg + 1);
				} while (DimenIter(pGv->m_dimenList, refList));
			}

			{
				m_gblIoDecl.Append("\tsc_out<bool> o_%sTo%s_iwComp%s;\n",
					mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblOut.Append("\to_%sTo%s_iwComp%s = r_t%d_%sIwComp%s;\n",
						mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						mod.m_tsStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}

			{
				m_gblIoDecl.Append("\tsc_out<CGW_%s> o_%sTo%s_iwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(),
					mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblOut.Append("\to_%sTo%s_iwData%s = r_t%d_%sIwData%s;\n",
						mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						mod.m_tsStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}

			if (pNgvInfo->m_atomicMask != 0) {
				m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_ifWrEn%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				if (mod.m_threads.m_htIdW.AsInt() > 0)
					m_gblIoDecl.Append("\tsc_in<sc_uint<%s_HTID_W> > i_%sTo%s_ifHtId%s;\n",
					mod.m_modName.Upper().c_str(),
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				m_gblIoDecl.Append("\tsc_in<%s> i_%sTo%s_ifData%s;\n",
					pGv->m_type.c_str(),
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}

			m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_wrEn%s;\n",
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			if (pGv->m_addr1W.AsInt() > 0)
				m_gblIoDecl.Append("\tsc_in<ht_uint%d> i_%sTo%s_wrAddr1%s;\n",
				pGv->m_addr1W.AsInt(),
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			if (pGv->m_addr2W.AsInt() > 0)
				m_gblIoDecl.Append("\tsc_in<ht_uint%d> i_%sTo%s_wrAddr2%s;\n",
				pGv->m_addr2W.AsInt(),
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			m_gblIoDecl.Append("\tsc_in<%s> i_%sTo%s_wrData%s;\n",
				pGv->m_type.c_str(),
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_iwCompWrEn%s;\n",
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			if (mod.m_threads.m_htIdW.AsInt() > 0)
				m_gblIoDecl.Append("\tsc_in<sc_uint<%s_HTID_W> > i_%sTo%s_iwCompHtId%s;\n",
				mod.m_modName.Upper().c_str(),
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_iwCompData%s;\n",
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
		}

		if (pGv->m_bMifRead) { // Memory
			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				m_gblIoDecl.Append("\tsc_out<sc_uint<%s_HTID_W> > o_%sTo%s_mwHtId%s;\n",
					mod.m_modName.Upper().c_str(),
					mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblOut.Append("\to_%sTo%s_mwHtId%s = 0;//r_t%d_htId;\n",
						mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						mod.m_tsStg + 1);
				} while (DimenIter(pGv->m_dimenList, refList));
			}

			{
				m_gblIoDecl.Append("\tsc_out<bool> o_%sTo%s_mwComp%s;\n",
					mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblOut.Append("\to_%sTo%s_mwComp%s = 0;//r_m%d_%sMwComp%s;\n",
						mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						mod.m_tsStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}

			{
				m_gblIoDecl.Append("\tsc_out<CGW_%s> o_%sTo%s_mwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(),
					mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblOut.Append("\to_%sTo%s_mwData%s = r_m2_%sMwData%s;\n",
						mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}

			m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_mwCompWrEn%s;\n",
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			if (mod.m_threads.m_htIdW.AsInt() > 0)
				m_gblIoDecl.Append("\tsc_in<sc_uint<%s_HTID_W> > i_%sTo%s_mwCompHtId%s;\n",
				mod.m_modName.Upper().c_str(),
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_mwCompData%s;\n",
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
		}

		if (pGv->m_addr1W.AsInt() == 0) {
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblRegWrData.Append("\tif (r_%sTo%s_wrEn%s)\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());

				if (pNgvInfo->m_bNgvWrDataClk2x && (mod.m_clkRate == eClk1x)) {
					gblRegWrData.Append("\t\tr_%s_2x%s = r_%sTo%s_wrData%s;\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblReg.Append("\tr_%s%s = r_%s_2x%s;\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				} else {
					gblRegWrData.Append("\t\tr_%s%s = r_%sTo%s_wrData%s;\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				}

			} while (DimenIter(pGv->m_dimenList, refList));
			gblRegWrData.NewLine();
		} else {
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblPostInstrWrData.Append("\tm_%s%s.write_addr(r_%sTo%s_wrAddr1%s%s);\n",
					pGv->m_gblName.c_str(), dimIdx.c_str(),
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
					VA(pGv->m_addr2W.AsInt() == 0 ? "" : ", r_%sTo%s_wrAddr2%s",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str()).c_str());
				gblPostInstrWrData.Append("\tif (r_%sTo%s_wrEn%s)\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				gblPostInstrWrData.Append("\t\tm_%s%s.write_mem(r_%sTo%s_wrData%s);\n",
					pGv->m_gblName.c_str(), dimIdx.c_str(),
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				gblPostInstrWrData.NewLine();
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		if (pNgvInfo->m_atomicMask != 0) {
			if (mod.m_threads.m_htIdW.AsInt() == 0) {
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("sc_signal<%s>", pGv->m_type.c_str()),
					VA("r_%sIf", pGv->m_gblName.c_str()), pGv->m_dimenList);
			} else {
				m_gblRegDecl.Append("\tht_dist_ram<%s, %s_HTID_W> m_%sIf%s;\n",
					pGv->m_type.c_str(),
					mod.m_modName.Upper().c_str(),
					pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					if (pNgvInfo->m_bNgvWrDataClk2x == (mod.m_clkRate == eClk2x)) {
					gblReg.Append("\tm_%sIf%s.clock();\n",
						pGv->m_gblName.c_str(), dimIdx.c_str());
					} else {
						gblReg.Append("\tm_%sIf%s.read_clock();\n",
							pGv->m_gblName.c_str(), dimIdx.c_str());
						gblRegWrData.Append("\tm_%sIf%s.write_clock();\n",
							pGv->m_gblName.c_str(), dimIdx.c_str());
					}
				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}

		if (pGv->m_addr1W.AsInt() == 0) {
			if (pNgvInfo->m_bNgvWrDataClk2x == (mod.m_clkRate == eClk2x)) {
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, pGv->m_type.c_str(),
					VA("r_%s", pGv->m_gblName.c_str()), pGv->m_dimenList);
			} else if (mod.m_clkRate == eClk2x) {
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("sc_signal<%s>", pGv->m_type.c_str()),
					VA("r_%s", pGv->m_gblName.c_str()), pGv->m_dimenList);
			} else {
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("sc_signal<%s>", pGv->m_type.c_str()),
					VA("r_%s_2x", pGv->m_gblName.c_str()), pGv->m_dimenList);
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, pGv->m_type.c_str(),
					VA("r_%s", pGv->m_gblName.c_str()), pGv->m_dimenList);
			}

		} else {
			m_gblRegDecl.Append("\tht_%s_ram<%s, %d",
				pGv->m_pNgvInfo->m_ramType == eBlockRam ? "block" : "dist",
				pGv->m_type.c_str(), pGv->m_addr1W.AsInt());
			if (pGv->m_addr2W.AsInt() > 0)
				m_gblRegDecl.Append(", %d", pGv->m_addr2W.AsInt());
			m_gblRegDecl.Append("> m_%s%s;\n", pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);

				if (pNgvInfo->m_bNgvWrDataClk2x == (mod.m_clkRate == eClk2x))
					gblReg.Append("\tm_%s%s.clock();\n",
					pGv->m_gblName.c_str(), dimIdx.c_str());
				else {
					gblReg.Append("\tm_%s%s.read_clock();\n",
						pGv->m_gblName.c_str(), dimIdx.c_str());
					gblRegWrData.Append("\tm_%s%s.write_clock();\n",
						pGv->m_gblName.c_str(), dimIdx.c_str());
				}
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		if (mod.m_threads.m_htIdW.AsInt() == 0) {

			if ((mod.m_clkRate == eClk2x) != pGv->m_pNgvInfo->m_bNgvWrCompClk2x) {
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "sc_signal<bool>",
					VA("r_%sIwComp", pGv->m_gblName.c_str()), pGv->m_dimenList);
			} else {
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
					VA("r_%sIwComp", pGv->m_gblName.c_str()), pGv->m_dimenList);
			}

		} else {
			m_gblRegDecl.Append("\tht_dist_ram<bool, %s_HTID_W> m_%sIwComp%s[2];\n",
				mod.m_modName.Upper().c_str(), pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				if (pNgvInfo->m_bNgvWrCompClk2x == (mod.m_clkRate == eClk2x)) {
					gblRegWrComp.Append("\tm_%sIwComp%s[0].clock();\n",
						pGv->m_gblName.c_str(), dimIdx.c_str());
					gblRegWrComp.Append("\tm_%sIwComp%s[1].clock();\n",
						pGv->m_gblName.c_str(), dimIdx.c_str());
				} else {
					CHtCode &gblRegMod = mod.m_clkRate == eClk2x ? m_gblReg2x : m_gblReg1x;

					gblRegMod.Append("\tm_%sIwComp%s[0].read_clock();\n",
						pGv->m_gblName.c_str(), dimIdx.c_str());
					gblRegMod.Append("\tm_%sIwComp%s[1].read_clock();\n",
						pGv->m_gblName.c_str(), dimIdx.c_str());
					gblRegWrComp.Append("\tm_%sIwComp%s[0].write_clock();\n",
						pGv->m_gblName.c_str(), dimIdx.c_str());
					gblRegWrComp.Append("\tm_%sIwComp%s[1].write_clock();\n",
						pGv->m_gblName.c_str(), dimIdx.c_str());
				}
			} while (DimenIter(pGv->m_dimenList, refList));

			if (pGv->m_dimenList.size() > 0) {
				gblReg.NewLine();
				gblRegWrComp.NewLine();
			}
		}

		if (pNgvInfo->m_atomicMask != 0) {
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
				VA("r_%sTo%s_ifWrEn", pGv->m_gblName.c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblRegWrData.Append("\tr_%sTo%s_ifWrEn%s = i_%sTo%s_ifWrEn%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		if (pNgvInfo->m_atomicMask != 0 && mod.m_threads.m_htIdW.AsInt() > 0) {
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("sc_uint<%s_HTID_W>", mod.m_modName.Upper().c_str()),
				VA("r_%sTo%s_ifHtId", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblRegWrData.Append("\tr_%sTo%s_ifHtId%s = i_%sTo%s_ifHtId%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		if (pNgvInfo->m_atomicMask != 0) {
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, pGv->m_type,
				VA("r_%sTo%s_ifData", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblRegWrData.Append("\tr_%sTo%s_ifData%s = i_%sTo%s_ifData%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		if (pNgvInfo->m_ramType == eBlockRam) {

			bool bSignal = (mod.m_clkRate == eClk2x) != pNgvInfo->m_bNgvWrDataClk2x;
			{
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, bSignal ? "sc_signal<bool>" : "bool",
					VA("r_g1_%sTo%s_wrEn", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblRegWrData.Append("\tr_g1_%sTo%s_wrEn%s = r_%sTo%s_wrEn%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}

			if (pGv->m_addr1W.AsInt() > 0) {
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA(bSignal ? "sc_signal<ht_uint%d>" : "ht_uint%d", pGv->m_addr1W.AsInt()),
					VA("r_g1_%sTo%s_wrAddr1", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblRegWrData.Append("\tr_g1_%sTo%s_wrAddr1%s = r_%sTo%s_wrAddr1%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}

			if (pGv->m_addr2W.AsInt() > 0) {
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA(bSignal ? "sc_signal<ht_uint%d>" : "ht_uint%d", pGv->m_addr2W.AsInt()),
					VA("r_g1_%sTo%s_wrAddr2", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblRegWrData.Append("\tr_g1_%sTo%s_wrAddr2%s = r_%sTo%s_wrAddr2%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}

			{
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA(bSignal ? "sc_signal<%s>" : "%s", pGv->m_type.c_str()),
					VA("r_g1_%sTo%s_wrData", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblRegWrData.Append("\tr_g1_%sTo%s_wrData%s = r_%sTo%s_wrData%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}

		{
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
				VA("r_%sTo%s_wrEn", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblRegWrData.Append("\tr_%sTo%s_wrEn%s = i_%sTo%s_wrEn%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		if (pGv->m_addr1W.AsInt() > 0) {
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addr1W.AsInt()),
				VA("r_%sTo%s_wrAddr1", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblRegWrData.Append("\tr_%sTo%s_wrAddr1%s = i_%sTo%s_wrAddr1%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		if (pGv->m_addr2W.AsInt() > 0) {
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addr2W.AsInt()),
				VA("r_%sTo%s_wrAddr2", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblRegWrData.Append("\tr_%sTo%s_wrAddr2%s = i_%sTo%s_wrAddr2%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		{
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, pGv->m_type,
				VA("r_%sTo%s_wrData", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblRegWrData.Append("\tr_%sTo%s_wrData%s = i_%sTo%s_wrData%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		string compReset = pNgvInfo->m_bNgvWrCompClk2x ? "c_reset1x" : "r_reset1x";
		{
			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				m_gblRegDecl.Append("\tbool c_%sTo%s_iwCompWrEn%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
				VA("r_%sTo%s_iwCompWrEn", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					gblRegWrComp.Append("\tr_%sTo%s_iwCompWrEn%s = %s || c_%sTo%s_iwCompWrEn%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						compReset.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				} else {
					gblRegWrComp.Append("\tr_%sTo%s_iwCompWrEn%s = i_%sTo%s_iwCompWrEn%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				}
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			m_gblRegDecl.Append("\tht_uint%d c_%sTo%s_iwCompHtId%s;\n",
				mod.m_threads.m_htIdW.AsInt(),
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());

			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("sc_uint<%s_HTID_W>", mod.m_modName.Upper().c_str()),
				VA("r_%sTo%s_iwCompHtId", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblRegWrComp.Append("\tr_%sTo%s_iwCompHtId%s = %s ? (ht_uint%d)0 : c_%sTo%s_iwCompHtId%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
					compReset.c_str(),
					mod.m_threads.m_htIdW.AsInt(),
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		{
			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				m_gblRegDecl.Append("\tbool c_%sTo%s_iwCompData%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}

			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
				VA("r_%sTo%s_iwCompData", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					gblRegWrComp.Append("\tr_%sTo%s_iwCompData%s = !%s && c_%sTo%s_iwCompData%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						compReset.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				} else {
					gblRegWrComp.Append("\tr_%sTo%s_iwCompData%s = i_%sTo%s_iwCompData%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				}
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			m_gblRegDecl.Append("\tbool c_%sTo%s_iwCompInit%s;\n",
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());

			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
				VA("r_%sTo%s_iwCompInit", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblRegWrComp.Append("\tr_%sTo%s_iwCompInit%s = %s || c_%sTo%s_iwCompInit%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
					compReset.c_str(),
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		{
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
				VA("r_t%d_%sIwComp", mod.m_tsStg + 1, pGv->m_gblName.c_str()), pGv->m_dimenList);
			m_gblRegDecl.Append("\tbool c_t%d_%sIwComp%s;\n",
				mod.m_tsStg, pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblReg.Append("\tr_t%d_%sIwComp%s = c_t%d_%sIwComp%s;\n",
					mod.m_tsStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
					mod.m_tsStg, pGv->m_gblName.c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		if (mod.m_threads.m_htIdW.AsInt() == 0) {
			gblReg.Append("\tr_t%d_htCmdRdy = c_t%d_htCmdRdy;\n",
				mod.m_tsStg + 1,
				mod.m_tsStg);
		}

		if (pNgvInfo->m_atomicMask != 0) {
			m_gblRegDecl.Append("\t%s c_t%d_%sIfData%s;\n", 
				pGv->m_type.c_str(), mod.m_tsStg-1, pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("%s ht_noload", pGv->m_type.c_str()),
				VA("r_t%d_%sIfData", mod.m_tsStg, pGv->m_gblName.c_str()), pGv->m_dimenList);
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblReg.Append("\tr_t%d_%sIfData%s = c_t%d_%sIfData%s;\n",
					mod.m_tsStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
					mod.m_tsStg-1, pGv->m_gblName.c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		{
			m_gblRegDecl.Append("\t%s c_t%d_%sIrData%s;\n", 
				pGv->m_type.c_str(), mod.m_tsStg-1, pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("%s ht_noload", pGv->m_type.c_str()),
				VA("r_t%d_%sIrData", mod.m_tsStg, pGv->m_gblName.c_str()), pGv->m_dimenList);
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblReg.Append("\tr_t%d_%sIrData%s = c_t%d_%sIrData%s;\n",
					mod.m_tsStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
					mod.m_tsStg-1, pGv->m_gblName.c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		{
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
				VA("r_t%d_%sIwData", mod.m_tsStg+1, pGv->m_gblName.c_str()), pGv->m_dimenList);
			m_gblRegDecl.Append("\tCGW_%s c_t%d_%sIwData%s;\n", 
				pGv->m_pNgvInfo->m_ngvWrType.c_str(), mod.m_tsStg, pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblPreInstr.Append("\tc_t%d_%sIwData%s.init();\n",
					mod.m_tsStg, pGv->m_gblName.c_str(), dimIdx.c_str());
				gblReg.Append("\tr_t%d_%sIwData%s = c_t%d_%sIwData%s;\n",
					mod.m_tsStg+1, pGv->m_gblName.c_str(), dimIdx.c_str(),
					mod.m_tsStg, pGv->m_gblName.c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		gblPostInstr.NewLine();

		if (pGv->m_bMifRead) {
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
				VA("r_m2_%sMwData", pGv->m_gblName.c_str()), pGv->m_dimenList);
			m_gblRegDecl.Append("\tCGW_%s c_m1_%sMwData%s;\n",
				pGv->m_pNgvInfo->m_ngvWrType.c_str(), pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblPreInstr.Append("\tc_m1_%sMwData%s.init();\n",
					pGv->m_gblName.c_str(), dimIdx.c_str());
				gblReg.Append("\tr_m2_%sMwData%s = c_m1_%sMwData%s;\n",
					pGv->m_gblName.c_str(), dimIdx.c_str(),
					pGv->m_gblName.c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		if (pNgvInfo->m_atomicMask != 0) {
			if (mod.m_threads.m_htIdW.AsInt() == 0) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblPostInstr.Append("\tc_t%d_%sIfData%s = r_%sIf%s;\n",
						mod.m_tsStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			} else {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblPostInstr.Append("\tm_%sIf%s.read_addr(r_t%d_htId);\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						mod.m_tsStg - 1);
					gblPostInstr.Append("\tc_t%d_%sIfData%s = m_%sIf%s.read_mem();\n",
						mod.m_tsStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}
			gblPostInstr.NewLine();
		}

		if (pNgvInfo->m_atomicMask != 0) {
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				if (mod.m_threads.m_htIdW.AsInt() == 0) {
					gblPostInstrWrData.Append("\tif (r_%sTo%s_ifWrEn%s)\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrData.Append("\t\tr_%sIf%s = r_%sTo%s_ifData%s;\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				} else {
					gblPostInstrWrData.Append("\tm_%sIf%s.write_addr(r_%sTo%s_ifHtId%s);\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrData.Append("\tif (r_%sTo%s_ifWrEn%s)\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrData.Append("\t\tm_%sIf%s.write_mem(r_%sTo%s_ifData%s);\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				}
				gblPostInstrWrData.NewLine();
			} while (DimenIter(pGv->m_dimenList, refList));
		}
		gblPostInstrWrData.NewLine();

		if (pGv->m_addr1W.AsInt() == 0) {
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblPostInstr.Append("\tc_t%d_%sIrData%s = r_%s%s;\n",
					mod.m_tsStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
					pGv->m_gblName.c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		} else {
			int rdAddrStg = pNgvInfo->m_ramType == eBlockRam ? (mod.m_tsStg - 2) : (mod.m_tsStg - 1);
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblPostInstr.Append("\tm_%s%s.read_addr(r_t%d_htPriv.m_%s%s);\n",
					pGv->m_gblName.c_str(), dimIdx.c_str(),
					rdAddrStg, pGv->m_addr1Name.c_str(),
					VA(pGv->m_addr2W.AsInt() == 0 ? "" : ", r_t%d_htPriv.m_%s", rdAddrStg, pGv->m_addr2Name.c_str()).c_str());

				bool bNgvReg = pGv->m_addr1W.AsInt() == 0;
				bool bNgvDist = !bNgvReg && pGv->m_pNgvInfo->m_ramType != eBlockRam;

				if (bNgvDist) {
					gblPostInstr.Append("\tc_t%d_%sIrData%s = m_%s%s.read_mem();\n",
						mod.m_tsStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				} else {
					gblPostInstr.Append("\tif (r_g1_%sTo%s_wrEn%s && r_g1_%sTo%s_wrAddr1%s == r_t%d_htPriv.m_%s%s",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						mod.m_tsStg - 1, pGv->m_addr1Name.c_str(), dimIdx.c_str());

					if (pGv->m_addr2W.AsInt() > 0) {
						gblPostInstr.Append(" && r_g1_%sTo%s_wrAddr2%s == r_t%d_htPriv.m_%s%s)\n",
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							mod.m_tsStg - 1, pGv->m_addr2Name.c_str(), dimIdx.c_str());
					} else {
						gblPostInstr.Append(")\n");
					}

					gblPostInstr.Append("\t\tc_t%d_%sIrData%s = r_g1_%sTo%s_wrData%s;\n",
						mod.m_tsStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());

					gblPostInstr.Append("\telse\n");

					gblPostInstr.Append("\t\tc_t%d_%sIrData%s = m_%s%s.read_mem();\n",
						mod.m_tsStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				}
				gblPostInstr.NewLine();

			} while (DimenIter(pGv->m_dimenList, refList));
		}

		{
			string typeStr;
			if (mod.m_threads.m_htIdW.AsInt() == 0) {
				if (pGv->m_dimenList.size() == 0)
					typeStr = "bool ";
				else
					gblPostInstrWrComp.Append("\tbool c_%sIwComp%s;\n", pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
			}

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				if (mod.m_threads.m_htIdW.AsInt() == 0) {
					gblPostInstrWrComp.Append("\t%sc_%sIwComp%s = r_%sTo%s_iwCompWrEn%s ? !r_%sTo%s_iwCompData%s : r_%sIwComp%s;\n",
						typeStr.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());

					gblRegWrComp.Append("\tr_%sIwComp%s = !r_reset1x && c_%sIwComp%s;\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				} else {
					gblPostInstrWrComp.Append("\tm_%sIwComp%s[0].write_addr(r_%sTo%s_iwCompHtId%s);\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrComp.Append("\tm_%sIwComp%s[1].write_addr(r_%sTo%s_iwCompHtId%s);\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrComp.Append("\tif (r_%sTo%s_iwCompWrEn%s) {\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrComp.Append("\t\tm_%sIwComp%s[0].write_mem(!r_%sTo%s_iwCompData%s);\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrComp.Append("\t\tm_%sIwComp%s[1].write_mem(!r_%sTo%s_iwCompData%s);\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrComp.Append("\t}\n");
					gblPostInstrWrComp.NewLine();

					gblPostInstrWrComp.Append("\tif (r_%sTo%s_iwCompInit%s) {\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrComp.Append("\t\tc_%sTo%s_iwCompWrEn%s = true;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrComp.Append("\t\tc_%sTo%s_iwCompHtId%s = r_%sTo%s_iwCompHtId%s + 1u;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrComp.Append("\t\tc_%sTo%s_iwCompData%s = false;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrComp.Append("\t\tc_%sTo%s_iwCompInit%s = r_%sTo%s_iwCompHtId%s != 0x%x;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						(1 << mod.m_threads.m_htIdW.AsInt())-2);

					gblPostInstrWrComp.Append("\t} else {\n");

					gblPostInstrWrComp.Append("\t\tc_%sTo%s_iwCompWrEn%s = i_%sTo%s_iwCompWrEn%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrComp.Append("\t\tc_%sTo%s_iwCompHtId%s = i_%sTo%s_iwCompHtId%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrComp.Append("\t\tc_%sTo%s_iwCompData%s = i_%sTo%s_iwCompData%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrComp.Append("\t\tc_%sTo%s_iwCompInit%s = false;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());

					gblPostInstrWrComp.Append("\t}\n");
				}
				gblPostInstrWrComp.NewLine();
			} while (DimenIter(pGv->m_dimenList, refList));

			gblRegWrComp.NewLine();
		}

		{
			string typeStr;
			if (pGv->m_dimenList.size() == 0)
				typeStr = "bool ";
			else {
				gblPostInstr.Append("\tbool c_%sTo%s_iwWrEn%s;\n", mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblPostInstr.Append("\t%sc_%sTo%s_iwWrEn%s = ", typeStr.c_str(), mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str());

				string separator;
				for (CStructElemIter iter(this, pGv->m_type); !iter.end(); iter++) {
					if (iter.IsStructOrUnion()) continue;

					gblPostInstr.Append("%sr_t%d_%sIwData%s%s.m_bWrite", 
						separator.c_str(), mod.m_tsStg+1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						iter.GetHeirFieldName().c_str());

					if (iter->m_atomicMask & ATOMIC_INC)
						gblPostInstr.Append(" || r_t%d_%sIwData%s%s.m_bInc", 
							mod.m_tsStg+1, pGv->m_gblName.c_str(), dimIdx.c_str(),
							iter.GetHeirFieldName().c_str());

					if (iter->m_atomicMask & ATOMIC_SET)
						gblPostInstr.Append(" || r_t%d_%sIwData%s%s.m_bSet",
						mod.m_tsStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						iter.GetHeirFieldName().c_str());

					if (iter->m_atomicMask & ATOMIC_ADD)
						gblPostInstr.Append(" || r_t%d_%sIwData%s%s.m_bAdd",
						mod.m_tsStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						iter.GetHeirFieldName().c_str());

					separator = " ||\n\t\t";
				}
				
				gblPostInstr.Append(";\n");
			} while (DimenIter(pGv->m_dimenList, refList));
			gblPostInstr.NewLine();
		}

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblPostInstr.Append("\tm_%sIwComp%s[0].read_addr(r_t%d_htId);\n",
					pGv->m_gblName.c_str(), dimIdx.c_str(), mod.m_tsStg);
				gblPostInstr.Append("\tc_t%d_%sIwComp%s = m_%sIwComp%s[0].read_mem();\n",
					mod.m_tsStg, pGv->m_gblName.c_str(), dimIdx.c_str(), 
					pGv->m_gblName.c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
			gblPostInstr.NewLine();
		}
	}

	int queDepthW = mod.m_threads.m_htIdW.AsInt() <= 5 ? 5 : mod.m_threads.m_htIdW.AsInt();
	m_gblRegDecl.Append("\tht_dist_que<CHtComp, %d> m_htCompQue;\n", queDepthW);
	gblReg.Append("\tm_htCompQue.clock(r_reset1x);\n");

	m_gblRegDecl.Append("\tht_uint%d c_htCompQueAvlCnt;\n", queDepthW + 1);
	GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("ht_uint%d", queDepthW + 1), "r_htCompQueAvlCnt");

	gblReg.Append("\tr_htCompQueAvlCnt = r_reset1x ? (ht_uint%d)0x%x : c_htCompQueAvlCnt;\n", queDepthW + 1, 1 << queDepthW);

	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrIn) continue;
		if (cxrIntf.m_pDstMod->m_modName.AsStr() == "hif") continue;

		m_gblRegDecl.Append("\tbool c_%s_%sCompRdy;\n", cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
		GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool", VA("r_%s_%sCompRdy", cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName()));

		gblReg.Append("\tr_%s_%sCompRdy = c_%s_%sCompRdy;\n",
			cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(),
			cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
	}

	if (mod.m_threads.m_htIdW.AsInt() == 0) {
		m_gblRegDecl.Append("\tbool c_htCompRdy;\n");
		GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool", "r_htCompRdy");

		gblReg.Append("\tr_htCompRdy = c_htCompRdy;\n", mod.m_modName.Upper().c_str());
	} else {
		m_gblRegDecl.Append("\tsc_uint<%s_HTID_W> c_htCmdRdyCnt;\n",
			mod.m_modName.Upper().c_str());
		GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("sc_uint<%s_HTID_W>", mod.m_modName.Upper().c_str()), "r_htCmdRdyCnt");

		gblReg.Append("\tr_htCmdRdyCnt = r_reset1x ? (sc_uint<%s_HTID_W>)0 : c_htCmdRdyCnt;\n", mod.m_modName.Upper().c_str());
	}

	gblPostInstr.Append("\tif (r_t%d_htValid) {\n", mod.m_execStg+1);
	gblPostInstr.Append("\t\tCHtComp htComp;\n");

	for (size_t gvIdx = 0; gvIdx < mod.m_globalVarList.size(); gvIdx += 1) {
		CRam * pGv = mod.m_globalVarList[gvIdx];

		vector<int> refList(pGv->m_dimenList.size());
		do {
			string dimIdx = IndexStr(refList);
			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				gblPostInstr.Append("\t\thtComp.m_htId = r_t%d_htId;\n",
					mod.m_tsStg + 1);
			}
			gblPostInstr.Append("\t\thtComp.m_%sIwComp%s = c_%sTo%s_iwWrEn%s ? !r_t%d_%sIwComp%s : r_t%d_%sIwComp%s;\n",
				pGv->m_gblName.c_str(), dimIdx.c_str(),
				mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
				mod.m_tsStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
				mod.m_tsStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str());
		} while (DimenIter(pGv->m_dimenList, refList));
	}

	gblPostInstr.Append("\t\thtComp.m_htCmdRdy = r_t%d_htCmdRdy;\n", mod.m_execStg + 1);

	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrIn) continue;
		if (cxrIntf.m_pDstMod->m_modName.AsStr() == "hif") {
			gblPostInstr.Append("\t\tif (r_%s_%sRdy)\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());

			if (mod.m_threads.m_htIdW.AsInt() > 0)
				gblPostInstr.Append("\t\t\tm_htIdPool.push(htComp.m_htId);\n");
			else
				gblPostInstr.Append("\t\t\tc_htBusy = false; \n");
		} else {
			gblPostInstr.Append("\t\thtComp.m_%sCompRdy = r_%s_%sRdy;\n",
				cxrIntf.GetIntfName(),
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
		}
	}

	gblPostInstr.Append("\t\tm_htCompQue.push(htComp);\n");

	gblPostInstr.Append("\t}\n");
	gblPostInstr.Append("\n");

	gblPostInstr.Append("\tbool c_htCompQueEmpty = m_htCompQue.empty();\n");
	gblPostInstr.Append("\tCHtComp c_htCompQueFront = m_htCompQue.front();\n");

	for (size_t gvIdx = 0; gvIdx < mod.m_globalVarList.size(); gvIdx += 1) {
		CRam * pGv = mod.m_globalVarList[gvIdx];

		vector<int> refList(pGv->m_dimenList.size());
		do {
			string dimIdx = IndexStr(refList);
			if (mod.m_threads.m_htIdW.AsInt() == 0) {
				gblPostInstr.Append("\tc_t%d_%sIwComp%s = r_%sIwComp%s;\n",
					mod.m_execStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
					pGv->m_gblName.c_str(), dimIdx.c_str());
			} else {
				gblPostInstr.Append("\tm_%sIwComp%s[1].read_addr(c_htCompQueFront.m_htId);\n",
					pGv->m_gblName.c_str(), dimIdx.c_str());
			}
		} while (DimenIter(pGv->m_dimenList, refList));
	}
	gblPostInstr.Append("\n");

	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrIn) continue;
		if (cxrIntf.m_pDstMod->m_modName.AsStr() == "hif") continue;

		gblPostInstr.Append("\tc_%s_%sCompRdy = false;\n", cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
	}

	gblPostInstr.Append("\tif (!c_htCompQueEmpty");

	for (size_t gvIdx = 0; gvIdx < mod.m_globalVarList.size(); gvIdx += 1) {
		CRam * pGv = mod.m_globalVarList[gvIdx];

		vector<int> refList(pGv->m_dimenList.size());
		do {
			string dimIdx = IndexStr(refList);
			if (mod.m_threads.m_htIdW.AsInt() == 0) {
				gblPostInstr.Append("\n\t\t&& c_htCompQueFront.m_%sIwComp%s == r_%sIwComp%s",
					pGv->m_gblName.c_str(), dimIdx.c_str(),
					pGv->m_gblName.c_str(), dimIdx.c_str());
			} else {
				gblPostInstr.Append("\n\t\t&& c_htCompQueFront.m_%sIwComp%s == m_%sIwComp%s[1].read_mem()",
					pGv->m_gblName.c_str(), dimIdx.c_str(), pGv->m_gblName.c_str(), dimIdx.c_str());
			}
		} while (DimenIter(pGv->m_dimenList, refList));
	}
	gblPostInstr.Append(")\n\t{\n");

	gblPostInstr.Append("\t\tif (c_htCompQueFront.m_htCmdRdy)\n");
	if (mod.m_threads.m_htIdW.AsInt() == 0)
		gblPostInstr.Append("\t\t\tc_htCompRdy = true;\n");
	else
		gblPostInstr.Append("\t\t\tc_htCmdRdyCnt += 1u;\n");

	for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
		CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

		if (cxrIntf.m_cxrDir == CxrIn) continue;
		if (cxrIntf.m_pDstMod->m_modName.AsStr() == "hif") continue;

		gblPostInstr.Append("\t\tif (c_htCompQueFront.m_%sCompRdy) {\n", cxrIntf.GetIntfName());
		gblPostInstr.Append("\t\t\tc_%s_%sCompRdy = true;\n", cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName());
		if (mod.m_threads.m_htIdW.AsInt() == 0) {
			gblPostInstr.Append("\t\t\tc_htBusy = false;\n");
		} else {
			gblPostInstr.Append("\t\t\tm_htIdPool.push(c_htCompQueFront.m_htId);\n");
		}
		gblPostInstr.Append("\t\t}\n");
	}

	gblPostInstr.Append("\t\tc_htCompQueAvlCnt += 1;\n");
	gblPostInstr.Append("\t\tm_htCompQue.pop();\n");
	gblPostInstr.Append("\t}\n");
}

void CDsnInfo::GenerateNgvFiles()
{
	for (size_t gvIdx = 0; gvIdx < m_ngvList.size(); gvIdx += 1) {
		CNgvInfo * pNgvInfo = m_ngvList[gvIdx];
		CRam * pGv = pNgvInfo->m_modInfoList[0].m_pNgv;

		vector<CNgvModInfo> &ngvModInfoList = m_ngvList[gvIdx]->m_modInfoList;

		// create ngv port list of instruction and memory writes
		vector<pair<int, int> > ngvPortList;
		bool bAllModClk1x = true;
		for (size_t modIdx = 0; modIdx < ngvModInfoList.size(); modIdx += 1) {
			CModule * pMod = ngvModInfoList[modIdx].m_pMod;
			CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

			for (int imIdx = 0; imIdx < 2; imIdx += 1) {
				if (imIdx == 1 && !pModNgv->m_bMifRead) continue;

				ngvPortList.push_back(pair<int, int>(modIdx, imIdx));
				bAllModClk1x &= pMod->m_clkRate == eClk1x;
			}
		}
		int ngvPortCnt = (int)ngvPortList.size();

		int ngvFieldCnt = pNgvInfo->m_ngvFieldCnt;
		bool bNgvAtomicFast = pNgvInfo->m_bNgvAtomicFast;
		bool bNgvAtomicSlow = pNgvInfo->m_bNgvAtomicSlow;
		bool bNgvAtomic = bNgvAtomicFast || bNgvAtomicSlow;

		// determine type of ram
		bool bNgvReg = pGv->m_addr1W.AsInt() == 0;
		bool bNgvDist = !bNgvReg && pGv->m_pNgvInfo->m_ramType != eBlockRam;
		bool bNgvBlock = !bNgvReg && !bNgvDist;

		bool bNgvSelGwClk2x = bNgvReg && (!bAllModClk1x && ngvPortCnt <= 2 || ngvPortCnt == 3) ||
			bNgvDist && (!bAllModClk1x && ngvPortCnt <= 2 || ngvPortCnt == 3) && !bNgvAtomicSlow ||
			bNgvBlock && ((!bAllModClk1x && ngvPortCnt <= 2 || ngvPortCnt == 3) && !bNgvAtomic && ngvFieldCnt == 1);

		bool bNeed2x = pNgvInfo->m_bNgvWrDataClk2x || pNgvInfo->m_bNgvWrCompClk2x || !bAllModClk1x || bNgvSelGwClk2x;

		string incName = VA("PersGbl%s.h", pGv->m_gblName.Uc().c_str());
		string incPath = g_appArgs.GetOutputFolder() + "/" + incName;

		CHtFile incFile(incPath, "w");

		string vcdModName = "";
		CHtCode ngvIo;
		CHtCode ngvRegDecl;
		CHtCode ngvSosCode;
		CHtCode ngvPreReg_1x;
		CHtCode ngvPreReg_2x;
		CHtCode ngvReg_1x;
		CHtCode ngvReg_2x;
		CHtCode ngvOut_1x;
		CHtCode ngvOut_2x;

		CHtCode &ngvPreRegWrData = pNgvInfo->m_bNgvWrDataClk2x ? ngvPreReg_2x : ngvPreReg_1x;
		CHtCode &ngvRegWrData = pNgvInfo->m_bNgvWrDataClk2x ? ngvReg_2x : ngvReg_1x;

		CHtCode &ngvOut = pNgvInfo->m_bNgvWrDataClk2x ? ngvOut_2x : ngvOut_1x;
		CHtCode &ngvWrCompOut = pNgvInfo->m_bNgvWrCompClk2x ? ngvOut_2x : ngvOut_1x;

		string ngvWrDataClk = pNgvInfo->m_bNgvWrDataClk2x ? "_2x" : "";
		string ngvSelClk = pNgvInfo->m_bNgvWrCompClk2x ? "_2x" : "";
		string ngvWrCompClk = pNgvInfo->m_bNgvWrCompClk2x ? "_2x" : "";

		bool bNeedQueRegWrEnSig = bNgvReg && (ngvPortList.size() == 2 && bAllModClk1x) ||
			bNgvDist && ((ngvPortList.size() == 2 && bAllModClk1x) && !bNgvAtomicSlow ||
			ngvPortList.size() == 1 && !bAllModClk1x && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel) ||
			bNgvBlock && ((ngvPortList.size() == 2 && bAllModClk1x) && (!bNgvAtomic && ngvFieldCnt == 1) ||
			ngvPortCnt == 1 && (ngvFieldCnt > 1 || bNgvAtomic) && pNgvInfo->m_bNgvMaxSel);

		bool bNeedQueRegWrEnNonSig = !bNeedQueRegWrEnSig || !bNgvSelGwClk2x && !pNgvInfo->m_bNgvWrCompClk2x;
		string queRegWrEnSig = bNeedQueRegWrEnSig ? "_sig" : "";

		bool bNeedQueRegHtIdSig = bNgvReg && (ngvPortCnt == 2 && bAllModClk1x && bNgvAtomic) ||
			bNgvDist && (ngvPortCnt == 1 && !bAllModClk1x && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel ||
			ngvPortCnt == 2 && bAllModClk1x && bNgvAtomicFast) ||
			bNgvBlock && ngvPortCnt == 1 && (ngvFieldCnt > 1 || bNgvAtomic) && pNgvInfo->m_bNgvMaxSel;

		bool bNeedQueRegHtIdNonSig = !(bNgvReg && (ngvPortCnt == 2 && bAllModClk1x && (bNgvAtomic || ngvFieldCnt >= 2)) ||
			bNgvDist && (ngvPortCnt == 1 && !bAllModClk1x && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel) ||
			bNgvBlock && ngvPortCnt == 1 && (ngvFieldCnt > 1 || bNgvAtomic) && pNgvInfo->m_bNgvMaxSel) ||
			bNgvReg && (ngvPortCnt == 2 && bAllModClk1x && (bNgvAtomic || ngvFieldCnt >= 2));

		string queRegHtIdSig = bNeedQueRegHtIdSig ? "_sig" : "";

		bool bNeedQueRegCompSig = bNgvDist && ngvPortCnt == 1 && !bAllModClk1x && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel ||
			bNgvBlock && ngvPortCnt == 1 && (ngvFieldCnt > 1 || bNgvAtomic) && pNgvInfo->m_bNgvMaxSel;

		string queRegCompSig = bNeedQueRegCompSig ? "_sig" : "";

		bool bNeedQueRegDataSig = bNgvReg && (ngvPortList.size() == 2 && bAllModClk1x) ||
			bNgvDist && ((ngvPortList.size() == 2 && bAllModClk1x) && !bNgvAtomicSlow ||
			ngvPortList.size() == 1 && !bAllModClk1x && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel) ||
			bNgvBlock && ((ngvPortList.size() == 2 && bAllModClk1x) && (!bNgvAtomic && ngvFieldCnt == 1) ||
			ngvPortCnt == 1 && (ngvFieldCnt > 1 || bNgvAtomic) && pNgvInfo->m_bNgvMaxSel);

		string queRegDataSig = bNeedQueRegDataSig ? "_sig" : "";

		// Declare NGV I/O
		for (size_t modIdx = 0; modIdx < ngvModInfoList.size(); modIdx += 1) {
			CModule & mod = *ngvModInfoList[modIdx].m_pMod;
			CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

			if (mod.m_threads.m_htIdW.AsInt() > 0)
				ngvIo.Append("\tsc_in<sc_uint<%s_HTID_W> > i_%sTo%s_iwHtId%s;\n",
				mod.m_modName.Upper().c_str(), mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			ngvIo.Append("\tsc_in<bool> i_%sTo%s_iwComp%s;\n",
				mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			ngvIo.Append("\tsc_in<CGW_%s> i_%sTo%s_iwData%s;\n",
				pGv->m_pNgvInfo->m_ngvWrType.c_str(), mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			ngvIo.Append("\n");

			if (pNgvInfo->m_atomicMask != 0) {
				ngvIo.Append("\tsc_out<bool> o_%sTo%s_ifWrEn%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				if (mod.m_threads.m_htIdW.AsInt() > 0)
					ngvIo.Append("\tsc_out<sc_uint<%s_HTID_W> > o_%sTo%s_ifHtId%s;\n",
					mod.m_modName.Upper().c_str(), pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				ngvIo.Append("\tsc_out<%s> o_%sTo%s_ifData%s;\n",
					pGv->m_type.c_str(), pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}

			ngvIo.Append("\tsc_out<bool> o_%sTo%s_wrEn%s;\n",
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			if (pGv->m_addr1W.AsInt() > 0)
				ngvIo.Append("\tsc_out<ht_uint%d> o_%sTo%s_wrAddr1%s;\n",
				pGv->m_addr1W.AsInt(), pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			if (pGv->m_addr2W.AsInt() > 0)
				ngvIo.Append("\tsc_out<ht_uint%d> o_%sTo%s_wrAddr2%s;\n",
				pGv->m_addr2W.AsInt(), pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			ngvIo.Append("\tsc_out<%s> o_%sTo%s_wrData%s;\n",
				pGv->m_type.c_str(), pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			ngvIo.Append("\tsc_out<bool> o_%sTo%s_iwCompWrEn%s;\n",
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			if (mod.m_threads.m_htIdW.AsInt() > 0)
				ngvIo.Append("\tsc_out<sc_uint<%s_HTID_W> > o_%sTo%s_iwCompHtId%s;\n",
				mod.m_modName.Upper().c_str(), pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			ngvIo.Append("\tsc_out<bool> o_%sTo%s_iwCompData%s;\n",
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			ngvIo.Append("\n");

			if (pModNgv->m_bMifRead) {
				if (mod.m_threads.m_htIdW.AsInt() > 0)
					ngvIo.Append("\tsc_in<sc_uint<%s_HTID_W> > i_%sTo%s_mwHtId%s;\n",
					mod.m_modName.Upper().c_str(), mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				ngvIo.Append("\tsc_in<bool> i_%sTo%s_mwComp%s;\n",
					mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				ngvIo.Append("\tsc_in<CGW_%s> i_%sTo%s_mwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(), mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				ngvIo.Append("\n");

				ngvIo.Append("\tsc_out<bool> o_%sTo%s_mwCompWrEn%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				if (mod.m_threads.m_htIdW.AsInt() > 0)
					ngvIo.Append("\tsc_out<sc_uint<%s_HTID_W> > o_%sTo%s_mwCompHtId%s;\n",
					mod.m_modName.Upper().c_str(), pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				ngvIo.Append("\tsc_out<bool> o_%sTo%s_mwCompData%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				ngvIo.Append("\n");
			}
		}

		// 2x RR selection - one level, 2 or 3 ports, 2x wrData
		bool bRrSel2x = bNgvReg && (ngvPortList.size() == 2 && !bAllModClk1x || ngvPortList.size() == 3) ||
			bNgvDist && (ngvPortList.size() == 2 && !bAllModClk1x && !bNgvAtomicSlow || ngvPortList.size() == 3 && !bNgvAtomicSlow) ||
			bNgvBlock && (ngvPortList.size() == 2 && !bAllModClk1x && !bNgvAtomic && ngvFieldCnt == 1
			|| ngvPortList.size() == 3 && !bNgvAtomic && ngvFieldCnt == 1);

		// 1x RR selection - no phase select, 1x wrData
		bool bRrSel1x = bNgvDist && ngvPortCnt >= 2 && bNgvAtomicSlow && !pNgvInfo->m_bNgvMaxSel ||
			bNgvBlock && ngvPortCnt >= 2 && (bNgvAtomic || ngvFieldCnt >= 2) && !pNgvInfo->m_bNgvMaxSel;

		// 1x RR selection, ports split using phase, 2x wrData
		bool bRrSelAB = bNgvReg && ngvPortList.size() >= 4 ||
			bNgvDist && (ngvPortList.size() >= 4 && !bNgvAtomicSlow) ||
			bNgvBlock && (ngvPortList.size() >= 4 && !bNgvAtomic && ngvFieldCnt == 1);

		bool bNeedQue = bNgvReg && (ngvPortList.size() == 2 && !bAllModClk1x || ngvPortList.size() >= 3) ||
			bNgvDist && ((ngvPortList.size() == 2 && !bAllModClk1x || ngvPortList.size() >= 3) && !bNgvAtomicSlow ||
			((ngvPortList.size() >= 2 || !bAllModClk1x) && bNgvAtomicSlow)) ||
			bNgvBlock && ((!bNgvAtomic && ngvFieldCnt == 1) && (ngvPortList.size() == 2 && !bAllModClk1x || ngvPortList.size() >= 3) ||
			(bNgvAtomic || ngvFieldCnt > 1));

		bool bRrSelEO = bNgvDist && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel && (!bAllModClk1x || ngvPortList.size() >= 2) ||
			bNgvBlock && (bNgvAtomic || ngvFieldCnt > 1) && pNgvInfo->m_bNgvMaxSel;

		//
		// generate RR selection signals
		//

		if (bRrSel1x || bRrSel2x) {
			string ngvClkRrSel = bNgvSelGwClk2x ? "_2x" : "";

			CHtCode & ngvPreRegRrSel = bNgvSelGwClk2x ? ngvPreReg_2x : ngvPreReg_1x;
			CHtCode & ngvRegRrSel = bNgvSelGwClk2x ? ngvReg_2x : ngvReg_1x;

			ngvRegDecl.Append("\tht_uint%d c_rrSel%s%s;\n",
				(int)ngvPortList.size(), ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
			GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", (int)ngvPortList.size()),
				VA("r_rrSel%s", ngvClkRrSel.c_str()), pGv->m_dimenList);
			ngvRegDecl.NewLine();

			for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
				CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
				char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

				ngvRegDecl.Append("\tbool c_t0_%s_%cwRrRdy%s%s;\n",
					mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());

				ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSel%s%s;\n",
					mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
			}
			ngvRegDecl.NewLine();

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);

				ngvPreRegRrSel.Append("\tc_rrSel%s%s = r_rrSel%s%s;\n",
					ngvClkRrSel.c_str(), dimIdx.c_str(),
					ngvClkRrSel.c_str(), dimIdx.c_str());
				ngvPreRegRrSel.NewLine();

				string rrSelReset = pNgvInfo->m_bNgvWrCompClk2x ? "c_reset1x" : "r_reset1x";

				ngvRegRrSel.Append("\tr_rrSel%s%s = %s ? (ht_uint%d)0x1 : c_rrSel%s%s;\n",
					ngvWrDataClk.c_str(), dimIdx.c_str(),
					rrSelReset.c_str(),
					(int)ngvPortList.size(),
					ngvWrDataClk.c_str(), dimIdx.c_str());
				ngvRegRrSel.NewLine();

				for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
					CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
					char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

					if (bNgvBlock && (bNgvAtomic || ngvFieldCnt > 1)) {
						ngvPreRegRrSel.Append("\tc_t0_%s_%cwRrRdy%s%s = r_%s_%cwWrEn%s%s && (!r_t1_gwWrEn%s%s || r_t1_gwData%s%s.m_addr1 != r_%s_%cwData%s%s.m_addr1);\n",
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							ngvClkRrSel.c_str(), dimIdx.c_str(),
							ngvClkRrSel.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());
					} else {
						ngvPreRegRrSel.Append("\tc_t0_%s_%cwRrRdy%s%s = r_%s_%cwWrEn%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());
					}

				}
				ngvPreRegRrSel.NewLine();

				for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
					CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
					char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

					ngvPreRegRrSel.Append("\tc_t0_%s_%cwRrSel%s%s = c_t0_%s_%cwRrRdy%s%s && ((r_rrSel%s%s & 0x%lx) != 0 || !(\n",
						mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
						mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
						ngvClkRrSel.c_str(), dimIdx.c_str(),
						1ul << ngvIdx);

					for (int j = 1; j < ngvPortCnt; j += 1) {
						int k = (ngvIdx + j) % ngvPortCnt;

						uint32_t mask1 = (1ul << ngvPortCnt) - 1;
						uint32_t mask2 = ((1ul << j) - 1) << (ngvIdx + 1);
						uint32_t mask3 = (mask2 & mask1) | (mask2 >> ngvPortCnt);

						char kCh = ngvPortList[k].second == 0 ? 'i' : 'm';

						ngvPreRegRrSel.Append("\t\t(c_t0_%s_%cwRrRdy%s%s && (r_rrSel%s%s & 0x%x) != 0)%s\n",
							ngvModInfoList[ngvPortList[k].first].m_pMod->m_modName.Lc().c_str(), kCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							ngvClkRrSel.c_str(), dimIdx.c_str(),
							mask3,
							j == ngvPortCnt - 1 ? "));\n" : " ||");
					}
				}
				ngvPreRegRrSel.NewLine();

			} while (DimenIter(pGv->m_dimenList, refList));

		} else if (bRrSelAB) {
			string ngvClkRrSel = "";

			CHtCode & ngvPreRegRrSel = ngvPreReg_1x;
			CHtCode & ngvRegRrSel = ngvReg_1x;

			for (int rngIdx = 0; rngIdx < 2; rngIdx += 1) {
				char rngCh = 'A' + rngIdx;

				int ngvPortLo = rngIdx == 0 ? 0 : ngvPortCnt / 2;
				int ngvPortHi = rngIdx == 0 ? ngvPortCnt / 2 : ngvPortCnt;
				int ngvPortRngCnt = ngvPortHi - ngvPortLo;

				ngvRegDecl.Append("\tht_uint%d c_rrSel%c%s%s;\n",
					ngvPortRngCnt, rngCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", ngvPortRngCnt),
					VA("r_rrSel%c%s", rngCh, ngvClkRrSel.c_str()), pGv->m_dimenList);
				ngvRegDecl.NewLine();

				for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
					CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
					char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

					ngvRegDecl.Append("\tbool c_t0_%s_%cwRrRdy%s%s;\n",
						mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());

					ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSel%s%s;\n",
						mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
						VA("r_t1_%s_%cwWrEn%s", mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str()), pGv->m_dimenList);
					if (bNgvAtomic) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
							VA("r_t1_%s_%cwWrEn%s_sig", mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str()), pGv->m_dimenList);
					}

					if (mod.m_threads.m_htIdW.AsInt() > 0) {
						ngvRegDecl.Append("\tht_uint%d c_t0_%s_%cwHtId%s%s;\n",
							mod.m_threads.m_htIdW.AsInt(),
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", mod.m_threads.m_htIdW.AsInt()),
							VA("r_t1_%s_%cwHtId%s", mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str()), pGv->m_dimenList);
						if (bNgvAtomic) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<ht_uint%d>", mod.m_threads.m_htIdW.AsInt()),
								VA("r_t1_%s_%cwHtId%s_sig", mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str()), pGv->m_dimenList);
						}
					}

					ngvRegDecl.Append("\tbool c_t0_%s_%cwComp%s%s;\n",
						mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
						VA("r_t1_%s_%cwComp%s", mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str()), pGv->m_dimenList);

					ngvRegDecl.NewLine();
				}

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvPreRegRrSel.Append("\tc_rrSel%c%s%s = r_rrSel%c%s%s;\n",
						rngCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
						rngCh, ngvClkRrSel.c_str(), dimIdx.c_str());
					ngvPreRegRrSel.NewLine();

					ngvRegRrSel.Append("\tr_rrSel%c%s%s = r_reset1x ? (ht_uint%d)0x1 : c_rrSel%c%s%s;\n",
						rngCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
						ngvPortRngCnt,
						rngCh, ngvClkRrSel.c_str(), dimIdx.c_str());
					ngvRegRrSel.NewLine();

					for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
						char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

						ngvPreRegRrSel.Append("\tc_t0_%s_%cwRrRdy%s%s = r_%s_%cwWrEn%s%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str());

					}
					ngvPreRegRrSel.NewLine();

					for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
						char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

						ngvPreRegRrSel.Append("\tc_t0_%s_%cwRrSel%s%s = c_t0_%s_%cwRrRdy%s%s && ((r_rrSel%c%s & 0x%lx) != 0 || !(\n",
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							rngCh, dimIdx.c_str(),
							1ul << (ngvIdx - ngvPortLo));

						for (int j = 1; j < ngvPortRngCnt; j += 1) {
							int k = ngvPortLo + (ngvIdx - ngvPortLo + j) % ngvPortRngCnt;

							uint32_t mask1 = (1ul << ngvPortRngCnt) - 1;
							uint32_t mask2 = ((1ul << j) - 1) << (ngvIdx + 1 - ngvPortLo);
							uint32_t mask3 = (mask2 & mask1) | (mask2 >> ngvPortRngCnt);

							char kCh = ngvPortList[k].second == 0 ? 'i' : 'm';

							ngvPreRegRrSel.Append("\t\t(c_t0_%s_%cwRrRdy%s%s && (r_rrSel%c%s & 0x%x) != 0)%s\n",
								ngvModInfoList[ngvPortList[k].first].m_pMod->m_modName.Lc().c_str(), kCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								rngCh, dimIdx.c_str(),
								mask3,
								j == ngvPortRngCnt - 1 ? "));\n" : " ||");
						}

						//ngvPreRegRrSel.Append("\tc_t0_%s_%cwHtId%s%s = r_%s_%cwHtId%s%s%s;\n",   xxx
						//	mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
						//	mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());

						//ngvPreRegRrSel.Append("\tc_t0_%s_%cwComp%s%s = r_%s_%cwComp%s%s%s;\n",
						//	mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
						//	mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), queRegCompSig.c_str(), dimIdx.c_str());
						//ngvPreRegRrSel.NewLine();

						ngvRegRrSel.Append("\tr_t1_%s_%cwWrEn%s%s = c_t0_%s_%cwRrSel%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());

						if (bNgvAtomic) {
							ngvRegRrSel.Append("\tr_t1_%s_%cwWrEn%s_sig%s = c_t0_%s_%cwRrSel%s%s;\n",
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());
						}

						if (mod.m_threads.m_htIdW.AsInt() > 0) {
							ngvRegRrSel.Append("\tr_t1_%s_%cwHtId%s%s = c_t0_%s_%cwHtId%s%s;\n",
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());
						}

						if (bNgvAtomic && mod.m_threads.m_htIdW.AsInt() > 0) {
							ngvRegRrSel.Append("\tr_t1_%s_%cwHtId%s_sig%s = c_t0_%s_%cwHtId%s%s;\n",
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());
						}

						ngvRegRrSel.Append("\tr_t1_%s_%cwComp%s%s = c_t0_%s_%cwComp%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());
						ngvRegRrSel.NewLine();
					}

				} while (DimenIter(pGv->m_dimenList, refList));
			}
		} else if (bRrSelEO && ngvPortCnt > 1) {
			string ngvClkRrSel = "";

			CHtCode & ngvPreRegRrSel = ngvPreReg_1x;
			CHtCode & ngvRegRrSel = ngvReg_1x;

			for (int eoIdx = 0; eoIdx < 2; eoIdx += 1) {
				char eoCh = eoIdx == 0 ? 'E' : 'O';

				int ngvPortLo = 0;
				int ngvPortHi = ngvPortCnt;
				int ngvPortRngCnt = ngvPortHi - ngvPortLo;

				ngvRegDecl.Append("\tht_uint%d c_rrSel%c%s%s;\n",
					ngvPortRngCnt, eoCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", ngvPortRngCnt),
					VA("r_rrSel%c%s", eoCh, ngvClkRrSel.c_str()), pGv->m_dimenList);
				ngvRegDecl.NewLine();

				for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
					CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
					char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

					ngvRegDecl.Append("\tbool c_t0_%s_%cwRrRdy%c%s%s;\n",
						mod.m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());

					ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSel%c%s%s;\n",
						mod.m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());

					if (mod.m_threads.m_htIdW.AsInt() > 0) {
						ngvRegDecl.Append("\tsc_uint<%s_HTID_W> c_t0_%s_%cwHtId%c%s%s;\n",
							mod.m_modName.Upper().c_str(),
							mod.m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
					}

					ngvRegDecl.Append("\tbool c_t0_%s_%cwComp%c%s%s;\n",
						mod.m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());

					ngvRegDecl.NewLine();
				}

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvPreRegRrSel.Append("\tc_rrSel%c%s%s = r_rrSel%c%s%s;\n",
						eoCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
						eoCh, ngvClkRrSel.c_str(), dimIdx.c_str());
					ngvPreRegRrSel.NewLine();

					ngvRegRrSel.Append("\tr_rrSel%c%s%s = r_reset1x ? (ht_uint%d)0x1 : c_rrSel%c%s%s;\n",
						eoCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
						ngvPortRngCnt,
						eoCh, ngvClkRrSel.c_str(), dimIdx.c_str());
					ngvRegRrSel.NewLine();

					for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
						CRam * pModNgv = ngvModInfoList[ngvPortList[ngvIdx].first].m_pNgv;
						CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
						char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';
						bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? pModNgv->m_bMaxIw : pModNgv->m_bMaxMw);

						string maxWrStr = bMaxWr ? (eoIdx == 0 ? "E" : "O") : "";

						ngvPreRegRrSel.Append("\tc_t0_%s_%cwRrRdy%c%s%s = r_%s_%cwWrEn%s%s%s && ",
							mod.m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, maxWrStr.c_str(), ngvClkRrSel.c_str(), dimIdx.c_str());

						if (!bMaxWr) {
							ngvPreRegRrSel.Append("(r_%s_%cwData%s.m_addr1 & 1) == %d && ", mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(), eoIdx);
						}

						ngvPreRegRrSel.Append("(!r_t1_gwWrEn%c_sig%s || r_%s_%cwData%s%s.m_addr1 != r_t1_gwData%c_sig%s.read().m_addr1);\n",
							eoCh, dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, maxWrStr.c_str(), dimIdx.c_str(),
							eoCh, dimIdx.c_str());

					}
					ngvPreRegRrSel.NewLine();

					for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
						char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

						ngvPreRegRrSel.Append("\tc_t0_%s_%cwRrSel%c%s%s = c_t0_%s_%cwRrRdy%c%s%s && ((r_rrSel%c%s & 0x%lx) != 0 || !(\n",
							mod.m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							eoCh, dimIdx.c_str(),
							1ul << (ngvIdx - ngvPortLo));

						for (int j = 1; j < ngvPortRngCnt; j += 1) {
							int k = ngvPortLo + (ngvIdx - ngvPortLo + j) % ngvPortRngCnt;

							uint32_t mask1 = (1ul << ngvPortRngCnt) - 1;
							uint32_t mask2 = ((1ul << j) - 1) << (ngvIdx + 1 - ngvPortLo);
							uint32_t mask3 = (mask2 & mask1) | (mask2 >> ngvPortRngCnt);

							char kCh = ngvPortList[k].second == 0 ? 'i' : 'm';

							ngvPreRegRrSel.Append("\t\t(c_t0_%s_%cwRrRdy%c%s%s && (r_rrSel%c%s & 0x%x) != 0)%s\n",
								ngvModInfoList[ngvPortList[k].first].m_pMod->m_modName.Lc().c_str(), kCh, eoCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								eoCh, dimIdx.c_str(),
								mask3,
								j == ngvPortRngCnt - 1 ? "));" : " ||");
						}

						ngvPreRegRrSel.NewLine();
					}

				} while (DimenIter(pGv->m_dimenList, refList));
			}
		} else if (bNeedQue) {

			string ngvClkRrSel = bNgvSelGwClk2x ? "_2x" : "";

			CHtCode & ngvPreRegRrSel = bNgvSelGwClk2x ? ngvPreReg_2x : ngvPreReg_1x;

			for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
				CRam * pModNgv = ngvModInfoList[ngvPortList[ngvIdx].first].m_pNgv;
				CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
				char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

				for (int eoIdx = 0; eoIdx < 2; eoIdx += 1) {
					string eoStr;
					bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? pModNgv->m_bMaxIw : pModNgv->m_bMaxMw);
					if (bRrSelEO && bMaxWr) {
						eoStr = eoIdx == 0 ? "E" : "O";
					} else if (eoIdx == 1)
						continue;

					ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSel%s%s;\n", mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), pGv->m_dimenDecl.c_str());

					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						if (bRrSelEO) {
							ngvPreRegRrSel.Append("\tc_t0_%s_%cwRrSel%s%s = r_%s_%cwWrEn%s%s%s && (!r_t1_gwWrEn%s_sig%s || r_%s_%cwData%s%s%s.read().m_addr1 != r_t1_gwAddr1%s_sig%s);\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
								eoStr.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), queRegDataSig.c_str(), dimIdx.c_str(),
								eoStr.c_str(), dimIdx.c_str());
						} else {
							ngvPreRegRrSel.Append("\tc_t0_%s_%cwRrSel%s = r_%s_%cwWrEn%s%s;\n",
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str());
						}

					} while (DimenIter(pGv->m_dimenList, refList));
				}
				ngvRegDecl.NewLine();
			}
			ngvPreRegRrSel.NewLine();
		}

		//
		// input port staging / queuing
		//

		if (!bNeedQue) {
			for (size_t modIdx = 0; modIdx < ngvModInfoList.size(); modIdx += 1) {
				CModule & mod = *ngvModInfoList[modIdx].m_pMod;
				CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

				string ngvClkModIn = (mod.m_clkRate == eClk2x || pNgvInfo->m_bNgvWrCompClk2x) ? "_2x" : "";
				string ngvClkWrComp = pNgvInfo->m_bNgvWrCompClk2x ? "_2x" : "";

				CHtCode & ngvPreRegModIn = (mod.m_clkRate == eClk2x || pNgvInfo->m_bNgvWrCompClk2x) ? ngvPreReg_2x : ngvPreReg_1x;

				CHtCode & ngvRegSelGw = pNgvInfo->m_bNgvWrCompClk2x ? ngvReg_2x : ngvReg_1x;

				for (int imIdx = 0; imIdx < 2; imIdx += 1) {
					if (imIdx == 1 && !pModNgv->m_bMifRead) continue;
					char imCh = imIdx == 0 ? 'i' : 'm';

					if (mod.m_threads.m_htIdW.AsInt() > 0) {
						ngvRegDecl.Append("\tsc_uint<%s_HTID_W> c_%s_%cwHtId%s%s;\n",
							mod.m_modName.Upper().c_str(), mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());

						if (bNeedQueRegHtIdNonSig) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_uint<%s_HTID_W>", mod.m_modName.Upper().c_str()),
								VA("r_%s_%cwHtId%s", mod.m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str()), pGv->m_dimenList);
						}
						if (bNeedQueRegHtIdSig) {
							ngvRegDecl.Append("\tsc_signal<sc_uint<%s_HTID_W> > r_%s_%cwHtId%s_sig%s;\n",
								mod.m_modName.Upper().c_str(), mod.m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str(), pGv->m_dimenDecl.c_str());
						}
					}

					ngvRegDecl.Append("\tbool c_%s_%cwComp%s%s;\n",
						mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
						VA("r_%s_%cwComp%s", mod.m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str()), pGv->m_dimenList);

					ngvRegDecl.Append("\tCGW_%s c_%s_%cwData%s%s;\n",
						pGv->m_pNgvInfo->m_ngvWrType.c_str(),
						mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());
					if (bNeedQueRegDataSig) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<CGW_%s>", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
							VA("r_%s_%cwData%s_sig", mod.m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str()), pGv->m_dimenList);
					} else {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
							VA("r_%s_%cwData%s", mod.m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str()), pGv->m_dimenList);
					}
					ngvRegDecl.Append("\tbool c_%s_%cwWrEn%s%s;\n",
						mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());

					if (bNeedQueRegWrEnNonSig) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
							VA("r_%s_%cwWrEn%s", mod.m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str()), pGv->m_dimenList);
					}
					if (bNeedQueRegWrEnSig) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
							VA("r_%s_%cwWrEn%s_sig", mod.m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str()), pGv->m_dimenList);
					}

					ngvRegDecl.NewLine();

					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);
						if (mod.m_threads.m_htIdW.AsInt() > 0) {
							ngvPreRegModIn.Append("\tc_%s_%cwHtId%s%s = i_%sTo%s_%cwHtId%s;\n",
								mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());
						}
						ngvPreRegModIn.Append("\tc_%s_%cwComp%s%s = i_%sTo%s_%cwComp%s;\n",
							mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());
						ngvPreRegModIn.Append("\tc_%s_%cwData%s%s = i_%sTo%s_%cwData%s;\n",
							mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());

						ngvPreRegModIn.Append("\tc_%s_%cwWrEn%s%s = ", mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());

						string separator;
						for (CStructElemIter iter(this, pGv->m_type); !iter.end(); iter++) {
							if (iter.IsStructOrUnion()) continue;

							ngvPreRegModIn.Append("%sc_%s_%cwData%s%s%s.m_bWrite",
								separator.c_str(), mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
								iter.GetHeirFieldName().c_str());

							if ((iter->m_atomicMask & ATOMIC_INC) != 0 && imIdx == 0) {
								ngvPreRegModIn.Append(" || c_%s_%cwData%s%s%s.m_bInc",
									mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
									iter.GetHeirFieldName().c_str());
							}
							if ((iter->m_atomicMask & ATOMIC_SET) != 0 && imIdx == 0) {
								ngvPreRegModIn.Append(" || c_%s_%cwData%s%s%s.m_bSet",
									mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
									iter.GetHeirFieldName().c_str());
							}
							if ((iter->m_atomicMask & ATOMIC_ADD) != 0 && imIdx == 0) {
								ngvPreRegModIn.Append(" || c_%s_%cwData%s%s%s.m_bAdd",
									mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
									iter.GetHeirFieldName().c_str());
							}
							separator = " ||\n\t\t";
						}

						ngvPreRegModIn.Append(";\n");
						ngvPreRegModIn.Append("\n");

						if (mod.m_threads.m_htIdW.AsInt() > 0) {
							if (bNeedQueRegHtIdNonSig) {
								ngvRegSelGw.Append("\tr_%s_%cwHtId%s%s = c_%s_%cwHtId%s%s;\n",
									mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str());
							}
							if (bNeedQueRegHtIdSig) {
								ngvRegSelGw.Append("\tr_%s_%cwHtId%s_sig%s = c_%s_%cwHtId%s%s;\n",
									mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str());
							}
						}
						ngvRegSelGw.Append("\tr_%s_%cwComp%s%s = c_%s_%cwComp%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str());

						ngvRegSelGw.Append("\tr_%s_%cwData%s%s%s = c_%s_%cwData%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), queRegDataSig.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str());

						if (bNeedQueRegWrEnNonSig) {
							ngvRegSelGw.Append("\tr_%s_%cwWrEn%s%s = c_%s_%cwWrEn%s%s;\n",
								mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str());
						}

						if (bNeedQueRegWrEnSig) {
							ngvRegSelGw.Append("\tr_%s_%cwWrEn%s_sig%s = c_%s_%cwWrEn%s%s;\n",
								mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str());
						}

						ngvRegSelGw.NewLine();

					} while (DimenIter(pGv->m_dimenList, refList));
				}
			}
		} else {
			for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
				CRam * pModNgv = ngvModInfoList[ngvPortList[ngvIdx].first].m_pNgv;
				CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
				char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

				string ngvClkModIn = (mod.m_clkRate == eClk2x || bNgvSelGwClk2x) ? "_2x" : "";
				string ngvClkWrComp = bNgvSelGwClk2x ? "_2x" : "";

				CHtCode & ngvPreRegModIn = (mod.m_clkRate == eClk2x || bNgvSelGwClk2x) ? ngvPreReg_2x : ngvPreReg_1x;
				CHtCode & ngvPreRegWrComp = bNgvSelGwClk2x ? ngvPreReg_2x : ngvPreReg_1x;

				CHtCode & ngvRegModIn = (mod.m_clkRate == eClk2x || bNgvSelGwClk2x) ? ngvReg_2x : ngvReg_1x;
				CHtCode & ngvRegSelGw = bNgvSelGwClk2x ? ngvReg_2x : ngvReg_1x;

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_uint<%s_HTID_W>", mod.m_modName.Upper().c_str()),
					VA("r_%sTo%s_%cwHtId%s", mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str()), pGv->m_dimenList);

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
					VA("r_%sTo%s_%cwComp%s", mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str()), pGv->m_dimenList);

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
					VA("r_%sTo%s_%cwData%s", mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str()), pGv->m_dimenList);

				ngvRegDecl.Append("\tbool c_%sTo%s_%cwWrEn%s%s;\n",
					mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());
				ngvRegDecl.NewLine();

				for (int eoIdx = 0; eoIdx < 2; eoIdx += 1) {
					string eoStr;
					bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? pModNgv->m_bMaxIw : pModNgv->m_bMaxMw);
					if (bRrSelEO && bMaxWr) {
						eoStr = eoIdx == 0 ? "E" : "O";
					} else if (eoIdx == 1)
						continue;

					bool bQueBypass = mod.m_clkRate == eClk1x || bNgvSelGwClk2x;

					if (bMaxWr) {
						ngvRegDecl.Append("\tbool c_%sTo%s_%cwWrEn%s%s%s;\n",
							mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, eoStr.c_str(), ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());
						ngvRegDecl.NewLine();
					}
					ngvRegDecl.Append("\tht_dist_que <sc_uint<%s_HTID_W>, %s_HTID_W> m_%s_%cwHtIdQue%s%s;\n",
						mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str(),
						mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), pGv->m_dimenDecl.c_str());

					ngvRegDecl.Append("\tht_dist_que <bool, %s_HTID_W> m_%s_%cwCompQue%s%s;\n",
						mod.m_modName.Upper().c_str(),
						mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), pGv->m_dimenDecl.c_str());

					ngvRegDecl.Append("\tht_dist_que <CGW_%s, %s_HTID_W> m_%s_%cwDataQue%s%s;\n",
						pGv->m_pNgvInfo->m_ngvWrType.c_str(), mod.m_modName.Upper().c_str(),
						mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), pGv->m_dimenDecl.c_str());
					ngvRegDecl.NewLine();

					if (mod.m_threads.m_htIdW.AsInt() > 0) {
						ngvRegDecl.Append("\tsc_uint<%s_HTID_W> c_%s_%cwHtId%s%s%s;\n",
							mod.m_modName.Upper().c_str(), mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), pGv->m_dimenDecl.c_str());

						if (bNeedQueRegHtIdSig) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<sc_uint<%s_HTID_W> >", mod.m_modName.Upper().c_str()),
								VA("r_%s_%cwHtId%s%s_sig", mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
						} else {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_uint<%s_HTID_W>", mod.m_modName.Upper().c_str()),
								VA("r_%s_%cwHtId%s%s", mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
						}
					}

					ngvRegDecl.Append("\tbool c_%s_%cwComp%s%s%s;\n",
						mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), pGv->m_dimenDecl.c_str());
					if (bNeedQueRegCompSig) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
							VA("r_%s_%cwComp%s%s_sig", mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
					} else {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
							VA("r_%s_%cwComp%s%s", mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
					}

					ngvRegDecl.Append("\tCGW_%s c_%s_%cwData%s%s%s;\n",
						pGv->m_pNgvInfo->m_ngvWrType.c_str(),
						mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), pGv->m_dimenDecl.c_str());
					if (bNeedQueRegDataSig) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<CGW_%s>", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
							VA("r_%s_%cwData%s%s_sig", mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
					} else {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
							VA("r_%s_%cwData%s%s", mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
					}
					ngvRegDecl.Append("\tbool c_%s_%cwWrEn%s%s%s;\n",
						mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), pGv->m_dimenDecl.c_str());
					if (bNeedQueRegWrEnNonSig) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
							VA("r_%s_%cwWrEn%s%s", mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
					}
					if (bNeedQueRegWrEnSig) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
							VA("r_%s_%cwWrEn%s%s_sig", mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
					}
					ngvRegDecl.NewLine();

					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						string portRrSel;
						if (bRrSelEO && !bMaxWr) {
							portRrSel = VA("(c_t0_%s_%cwRrSelE%s%s || c_t0_%s_%cwRrSelO%s%s)",
								mod.m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str());
						} else {
							portRrSel = VA("c_t0_%s_%cwRrSel%s%s%s",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());
						}

						if (eoIdx == 0) {
							ngvPreRegModIn.Append("\tc_%sTo%s_%cwWrEn%s%s = ",
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());

							if (mod.m_clkRate == eClk1x && bNgvSelGwClk2x)
								ngvPreRegModIn.Append("r_phase && (");

							string separator;
							for (CStructElemIter iter(this, pGv->m_type); !iter.end(); iter++) {
								if (iter.IsStructOrUnion()) continue;

								ngvPreRegModIn.Append("%sr_%sTo%s_%cwData%s%s%s.m_bWrite",
									separator.c_str(), mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
									iter.GetHeirFieldName().c_str());

								if ((iter->m_atomicMask & ATOMIC_INC) != 0 && imCh == 'i') {
									ngvPreRegModIn.Append(" || r_%sTo%s_%cwData%s%s%s.m_bInc",
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										iter.GetHeirFieldName().c_str());
								}
								if ((iter->m_atomicMask & ATOMIC_SET) != 0 && imCh == 'i') {
									ngvPreRegModIn.Append(" || r_%sTo%s_%cwData%s%s%s.m_bSet",
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										iter.GetHeirFieldName().c_str());
								}
								if ((iter->m_atomicMask & ATOMIC_ADD) != 0 && imCh == 'i') {
									ngvPreRegModIn.Append(" || r_%sTo%s_%cwData%s%s%s.m_bAdd",
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										iter.GetHeirFieldName().c_str());
								}
								separator = " ||\n\t\t";
							}

							if (mod.m_clkRate == eClk1x && bNgvSelGwClk2x)
								ngvPreRegModIn.Append(")");

							ngvPreRegModIn.Append(";\n");
							ngvPreRegModIn.Append("\n");
						}

						if (bMaxWr) {
							ngvPreRegModIn.Append("\tc_%sTo%s_%cwWrEn%s%s%s = c_%sTo%s_%cwWrEn%s%s && (r_%sTo%s_%cwData%s%s.m_addr1 & 1) == %d;\n",
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, eoStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
								eoIdx);
							ngvPreRegModIn.NewLine();
						}
						if (bQueBypass) {
							ngvPreRegModIn.Append("\tif (c_%sTo%s_%cwWrEn%s%s%s && (r_%s_%cwWrEn%s%s%s%s && !%s || !m_%s_%cwDataQue%s%s.empty())) {\n",
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, eoStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
								portRrSel.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());
						} else {
							ngvPreRegModIn.Append("\tif (c_%sTo%s_%cwWrEn%s%s%s) {\n",
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, eoStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str());
						}
						ngvPreRegModIn.Append("\t\tm_%s_%cwHtIdQue%s%s.push(r_%sTo%s_%cwHtId%s%s);\n",
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());

						ngvPreRegModIn.Append("\t\tm_%s_%cwCompQue%s%s.push(r_%sTo%s_%cwComp%s%s);\n",
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());

						ngvPreRegModIn.Append("\t\tm_%s_%cwDataQue%s%s.push(r_%sTo%s_%cwData%s%s);\n",
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());

						ngvPreRegModIn.Append("\t}\n");
						ngvPreRegModIn.NewLine();

						ngvPreRegWrComp.Append("\tif (r_%s_%cwWrEn%s%s%s%s && !%s) {\n",
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
							portRrSel.c_str());

						ngvPreRegWrComp.Append("\t\tc_%s_%cwHtId%s%s%s = r_%s_%cwHtId%s%s%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());

						ngvPreRegWrComp.Append("\t\tc_%s_%cwComp%s%s%s = r_%s_%cwComp%s%s%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegCompSig.c_str(), dimIdx.c_str());

						ngvPreRegWrComp.Append("\t\tc_%s_%cwData%s%s%s = r_%s_%cwData%s%s%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegDataSig.c_str(), dimIdx.c_str());

						ngvPreRegWrComp.Append("\t} else {\n");

						if (bQueBypass) {
							ngvPreRegWrComp.Append("\t\tc_%s_%cwHtId%s%s%s = m_%s_%cwHtIdQue%s%s.empty() ? r_%sTo%s_%cwHtId%s%s : m_%s_%cwHtIdQue%s%s.front();\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

							ngvPreRegWrComp.Append("\t\tc_%s_%cwComp%s%s%s = m_%s_%cwCompQue%s%s.empty() ? r_%sTo%s_%cwComp%s%s : m_%s_%cwCompQue%s%s.front();\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

							ngvPreRegWrComp.Append("\t\tc_%s_%cwData%s%s%s = m_%s_%cwDataQue%s%s.empty() ? r_%sTo%s_%cwData%s%s : m_%s_%cwDataQue%s%s.front();\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());
						} else {
							ngvPreRegWrComp.Append("\t\tc_%s_%cwHtId%s%s%s = m_%s_%cwHtIdQue%s%s.front();\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

							ngvPreRegWrComp.Append("\t\tc_%s_%cwComp%s%s%s = m_%s_%cwCompQue%s%s.front();\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

							ngvPreRegWrComp.Append("\t\tc_%s_%cwData%s%s%s = m_%s_%cwDataQue%s%s.front();\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());
						}
						ngvPreRegWrComp.Append("\t}\n");

						if (bQueBypass) {
							ngvPreRegWrComp.Append("\tc_%s_%cwWrEn%s%s%s = !m_%s_%cwDataQue%s%s.empty() || c_%sTo%s_%cwWrEn%s%s%s || r_%s_%cwWrEn%s%s%s%s && !%s;\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
								portRrSel.c_str());
						} else {
							ngvPreRegWrComp.Append("\tc_%s_%cwWrEn%s%s%s = !m_%s_%cwDataQue%s%s.empty() || r_%s_%cwWrEn%s%s%s%s && !%s;\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
								portRrSel.c_str());
						}
						ngvPreRegWrComp.NewLine();

						ngvPreRegWrComp.Append("\tif ((!r_%s_%cwWrEn%s%s%s%s || %s) && !m_%s_%cwDataQue%s%s.empty()) {\n",
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
							portRrSel.c_str(),
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

						ngvPreRegWrComp.Append("\t\tm_%s_%cwHtIdQue%s%s.pop();\n",
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

						ngvPreRegWrComp.Append("\t\tm_%s_%cwCompQue%s%s.pop();\n",
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

						ngvPreRegWrComp.Append("\t\tm_%s_%cwDataQue%s%s.pop();\n",
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

						ngvPreRegWrComp.Append("\t}\n");
						ngvPreRegWrComp.NewLine();

						if (eoIdx == 0) {
							ngvRegModIn.Append("\tr_%sTo%s_%cwHtId%s%s = i_%sTo%s_%cwHtId%s;\n",
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());

							ngvRegModIn.Append("\tr_%sTo%s_%cwComp%s%s = i_%sTo%s_%cwComp%s;\n",
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());

							ngvRegModIn.Append("\tr_%sTo%s_%cwData%s%s = i_%sTo%s_%cwData%s;\n",
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());
							ngvRegModIn.NewLine();
						}

						string resetWrComp = bNgvSelGwClk2x ? "c_reset1x" : "r_reset1x";
						if (bQueBypass) {
							ngvRegSelGw.Append("\tm_%s_%cwHtIdQue%s%s.clock(%s);\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
								resetWrComp.c_str());

							ngvRegSelGw.Append("\tm_%s_%cwCompQue%s%s.clock(%s);\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
								resetWrComp.c_str());

							ngvRegSelGw.Append("\tm_%s_%cwDataQue%s%s.clock(%s);\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
								resetWrComp.c_str());
						} else {
							ngvRegSelGw.Append("\tm_%s_%cwHtIdQue%s%s.pop_clock(c_reset1x);\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

							ngvRegSelGw.Append("\tm_%s_%cwCompQue%s%s.pop_clock(c_reset1x);\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

							ngvRegSelGw.Append("\tm_%s_%cwDataQue%s%s.pop_clock(c_reset1x);\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

							ngvReg_2x.Append("\tm_%s_%cwHtIdQue%s%s.push_clock(c_reset1x);\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

							ngvReg_2x.Append("\tm_%s_%cwCompQue%s%s.push_clock(c_reset1x);\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

							ngvReg_2x.Append("\tm_%s_%cwDataQue%s%s.push_clock(c_reset1x);\n",
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

							ngvReg_2x.NewLine();
						}
						ngvRegSelGw.NewLine();

						ngvRegSelGw.Append("\tr_%s_%cwHtId%s%s%s%s = c_%s_%cwHtId%s%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegHtIdSig.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());

						ngvRegSelGw.Append("\tr_%s_%cwComp%s%s%s%s = c_%s_%cwComp%s%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegCompSig.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());

						ngvRegSelGw.Append("\tr_%s_%cwData%s%s%s%s = c_%s_%cwData%s%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegDataSig.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());

						string wrCompReset = bNgvSelGwClk2x ? "c_reset1x" : "r_reset1x";

						ngvRegSelGw.Append("\tr_%s_%cwWrEn%s%s%s%s = !%s && c_%s_%cwWrEn%s%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
							wrCompReset.c_str(),
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());
						ngvRegSelGw.NewLine();

					} while (DimenIter(pGv->m_dimenList, refList));
				}
			}
		}

		int stgIdx = 0;	// Input stage/queue reg is at stage 0 (i.e. read from c_t0, write to r_t1)
		string wrCompStg;

		//
		// generate RR muxing
		//

		if (bRrSel1x) {
			string ngvClkRrSel = "";
			CHtCode & ngvPreRegRrSel = ngvPreReg_1x;
			CHtCode & ngvRegRrSel = ngvReg_1x;

			ngvRegDecl.Append("\tbool c_t0_gwWrEn%s;\n",
				pGv->m_dimenDecl.c_str());
			GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
				"r_t1_gwWrEn", pGv->m_dimenList);

			ngvRegDecl.Append("\tCGW_%s c_t0_gwData%s;\n",
				pNgvInfo->m_ngvWrType.c_str(),
				pGv->m_dimenDecl.c_str());

			for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
				CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
				char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

				ngvRegDecl.Append("\tbool c_t0_%s_%cwWrEn%s;\n",
					mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
					VA("r_t1_%s_%cwWrEn", mod.m_modName.Lc().c_str(), imCh), pGv->m_dimenList);

				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					ngvRegDecl.Append("\tht_uint%d c_t0_%s_%cwHtId%s;\n",
						mod.m_threads.m_htIdW.AsInt(),
						mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", mod.m_threads.m_htIdW.AsInt()),
						VA("r_t1_%s_%cwHtId", mod.m_modName.Lc().c_str(), imCh), pGv->m_dimenList);
				}

				ngvRegDecl.Append("\tbool c_t0_%s_%cwComp%s;\n",
					mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
					VA("r_t1_%s_%cwComp", mod.m_modName.Lc().c_str(), imCh), pGv->m_dimenList);

				ngvRegDecl.NewLine();
			}


			GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("CGW_%s", pNgvInfo->m_ngvWrType.c_str()),
				"r_t1_gwData", pGv->m_dimenList);
			ngvRegDecl.NewLine();

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);

				ngvPreRegRrSel.Append("\tc_t0_gwWrEn%s = false;\n", dimIdx.c_str());
				ngvPreRegRrSel.Append("\tc_t0_gwData%s = 0;\n", dimIdx.c_str());
				ngvPreRegRrSel.NewLine();

				for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
					CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
					char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

					ngvPreRegRrSel.Append("\tc_t0_%s_%cwWrEn%s = c_t0_%s_%cwRrSel%s;\n",
						mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
						mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

					if (mod.m_threads.m_htIdW.AsInt() > 0) {
						ngvPreRegRrSel.Append("\tc_t0_%s_%cwHtId%s = r_%s_%cwHtId%s;\n",
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
					}
					ngvPreRegRrSel.Append("\tc_t0_%s_%cwComp%s = r_%s_%cwComp%s;\n",
						mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
						mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

					ngvPreRegRrSel.NewLine();
				}

				for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
					CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
					char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

					ngvPreRegRrSel.Append("\tif (c_t0_%s_%cwRrSel%s) {\n",
						mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

					ngvPreRegRrSel.Append("\t\tc_rrSel%s = 0x%x;\n",
						dimIdx.c_str(),
						1 << ((ngvIdx + 1) % ngvPortCnt));

					ngvPreRegRrSel.Append("\t\tc_t0_gwWrEn%s = true;\n", dimIdx.c_str());

					ngvPreRegRrSel.Append("\t\tc_t0_gwData%s = r_%s_%cwData%s;\n",
						dimIdx.c_str(),
						mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

					ngvPreRegRrSel.Append("\t}\n");
				}
				ngvPreRegRrSel.NewLine();

				ngvRegRrSel.Append("\tr_t1_gwWrEn%s = c_t0_gwWrEn%s;\n",
					dimIdx.c_str(),
					dimIdx.c_str());

				ngvRegRrSel.Append("\tr_t1_gwData%s = c_t0_gwData%s;\n",
					dimIdx.c_str(),
					dimIdx.c_str());

				ngvRegRrSel.NewLine();

				for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
					CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
					char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

					ngvRegRrSel.Append("\tr_t1_%s_%cwWrEn%s = c_t0_%s_%cwWrEn%s;\n",
						mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
						mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

					if (mod.m_threads.m_htIdW.AsInt() > 0) {
						ngvRegRrSel.Append("\tr_t1_%s_%cwHtId%s = c_t0_%s_%cwHtId%s;\n",
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
					}
					ngvRegRrSel.Append("\tr_t1_%s_%cwComp%s = c_t0_%s_%cwComp%s;\n",
						mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
						mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

					ngvRegRrSel.NewLine();
				}

				ngvPreRegRrSel.Append("\tc_t1_gwWrEn%s = r_t1_gwWrEn%s;\n",
					dimIdx.c_str(),
					dimIdx.c_str());

				ngvPreRegRrSel.Append("\tc_t1_gwData%s = r_t1_gwData%s;\n",
					dimIdx.c_str(),
					dimIdx.c_str());

				ngvPreRegRrSel.NewLine();

			} while (DimenIter(pGv->m_dimenList, refList));
		
			{
				ngvRegDecl.Append("\tbool c_t1_gwWrEn%s;\n",
					pGv->m_dimenDecl.c_str());

				ngvRegDecl.Append("\tCGW_%s c_t1_gwData%s;\n",
					pNgvInfo->m_ngvWrType.c_str(),
					pGv->m_dimenDecl.c_str());
				ngvRegDecl.NewLine();
			}

			stgIdx += 1;
			wrCompStg = "t1_";

		} else if (bRrSel2x) {
			// 2x RR select

			ngvRegDecl.Append("\tbool c_t%d_gwWrEn%s%s;\n",
				stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
			if (!bNgvReg && (ngvFieldCnt > 1 || bNgvAtomic)) {
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
					VA("r_t%d_gwWrEn%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
			}

			ngvRegDecl.Append("\tCGW_%s c_t%d_gwData%s%s;\n",
				pGv->m_pNgvInfo->m_ngvWrType.c_str(),
				stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
			if (!bNgvReg && (ngvFieldCnt > 1 || bNgvAtomic)) {
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
					VA("r_t%d_gwData%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
			}
			ngvRegDecl.NewLine();

			//if (bNgvAtomic && !bNgvReg) {
				for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
					CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
					char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

					//ngvRegDecl.Append("\tbool c_t0_%s_%cwWrEn_2x%s;\n",
					//	mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
						VA("r_t1_%s_%cwWrEn%s", mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str()), pGv->m_dimenList);

					if (mod.m_threads.m_htIdW.AsInt() > 0) {
						ngvRegDecl.Append("\tht_uint%d c_t0_%s_%cwHtId_2x%s;\n",
							mod.m_threads.m_htIdW.AsInt(),
							mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", mod.m_threads.m_htIdW.AsInt()),
							VA("r_t1_%s_%cwHtId%s", mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str()), pGv->m_dimenList);
					}

					ngvRegDecl.Append("\tbool c_t0_%s_%cwComp_2x%s;\n",
						mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
						VA("r_t1_%s_%cwComp%s", mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str()), pGv->m_dimenList);

					ngvRegDecl.NewLine();
				}
			//}

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);

				ngvPreReg_2x.Append("\tc_t%d_gwWrEn_2x%s = false;\n", stgIdx, dimIdx.c_str());

				ngvPreReg_2x.Append("\tc_t%d_gwData_2x%s = 0;\n", stgIdx, dimIdx.c_str());
				ngvPreReg_2x.NewLine();

				//if (bNgvAtomic && !bNgvReg) {
					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
						char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

						if (mod.m_threads.m_htIdW.AsInt() > 0) {
							ngvPreReg_2x.Append("\tc_t0_%s_%cwHtId_2x%s = r_%s_%cwHtId_2x%s%s;\n",
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegHtIdSig.c_str(), dimIdx.c_str());
						}

						ngvPreReg_2x.Append("\tc_t0_%s_%cwComp_2x%s = r_%s_%cwComp_2x%s;\n",
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						ngvPreReg_2x.NewLine();

						ngvReg_2x.Append("\tr_t1_%s_%cwWrEn_2x%s = c_t0_%s_%cwRrSel_2x%s;\n",
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						if (mod.m_threads.m_htIdW.AsInt() > 0) {
							ngvReg_2x.Append("\tr_t1_%s_%cwHtId_2x%s = c_t0_%s_%cwHtId_2x%s;\n",
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
						}

						ngvReg_2x.Append("\tr_t1_%s_%cwComp_2x%s = c_t0_%s_%cwComp_2x%s;\n",
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						ngvReg_2x.NewLine();
					}
				//}

				for (size_t ngvIdx = 0; ngvIdx < ngvPortList.size(); ngvIdx += 1) {
					CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
					char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

					ngvPreReg_2x.Append("\tif (c_t0_%s_%cwRrSel_2x%s) {\n",
						mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
					ngvPreReg_2x.Append("\t\tc_rrSel_2x%s = 0x%x;\n",
						dimIdx.c_str(), 1 << (int)((ngvIdx + 1) % ngvPortList.size()));
					ngvPreReg_2x.Append("\t\tc_t%d_gwWrEn_2x%s = true;\n", stgIdx, dimIdx.c_str());

					ngvPreReg_2x.Append("\t\tc_t%d_gwData_2x%s = r_%s_%cwData_2x%s;\n",
						stgIdx, dimIdx.c_str(),
						mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

					ngvPreReg_2x.Append("\t}\n");
				}
				ngvPreReg_2x.NewLine();

				if (ngvFieldCnt == 1 && !bNgvAtomic) {
					ngvPreReg_2x.Append("\tc_t%d_wrEn_2x%s = c_t%d_gwWrEn_2x%s;\n",
						stgIdx, dimIdx.c_str(),
						stgIdx, dimIdx.c_str());

					if (pGv->m_addr1W.AsInt() > 0) {
						ngvPreReg_2x.Append("\tc_t%d_wrAddr1_2x%s = c_t%d_gwData_2x%s.m_addr1;\n",
							stgIdx, dimIdx.c_str(),
							stgIdx, dimIdx.c_str());
					}
					if (pGv->m_addr2W.AsInt() > 0) {
						ngvPreReg_2x.Append("\tc_t%d_wrAddr2_2x%s = c_t%d_gwData_2x%s.m_addr2;\n",
							stgIdx, dimIdx.c_str(),
							stgIdx, dimIdx.c_str());
					}

					for (CStructElemIter iter(this, pGv->m_type); !iter.end(); iter++) {
						if (iter.IsStructOrUnion()) continue;

						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							ngvPreReg_2x.Append("\tc_t%d_wrData_2x%s%s = c_t%d_gwData_2x%s%s.m_data;\n",
								stgIdx, dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
								stgIdx, dimIdx.c_str(), iter.GetHeirFieldName().c_str());

						} while (DimenIter(pGv->m_dimenList, refList));
					}
				} else if (!bNgvReg) {

					ngvReg_2x.Append("\tr_t%d_gwWrEn_2x%s = c_t%d_gwWrEn_2x%s;\n",
						stgIdx + 1, dimIdx.c_str(),
						stgIdx, dimIdx.c_str());

					ngvReg_2x.Append("\tr_t%d_gwData_2x%s = c_t%d_gwData_2x%s;\n",
						stgIdx + 1, dimIdx.c_str(),
						stgIdx, dimIdx.c_str());
					ngvReg_2x.NewLine();

					stgIdx += 1;

					ngvRegDecl.Append("\tbool c_t%d_gwWrEn%s%s;\n",
						stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

					ngvRegDecl.Append("\tCGW_%s c_t%d_gwData%s%s;\n",
						pGv->m_pNgvInfo->m_ngvWrType.c_str(),
						stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
					ngvRegDecl.NewLine();

					ngvPreReg_2x.Append("\tc_t%d_gwWrEn_2x%s = r_t%d_gwWrEn_2x%s;\n",
						stgIdx, dimIdx.c_str(),
						stgIdx, dimIdx.c_str());

					ngvPreReg_2x.Append("\tc_t%d_gwData_2x%s = r_t%d_gwData_2x%s;\n",
						stgIdx, dimIdx.c_str(),
						stgIdx, dimIdx.c_str());
					ngvPreReg_2x.NewLine();
				}
			} while (DimenIter(pGv->m_dimenList, refList));

			wrCompStg = "t1_";

		} else if (bRrSelAB) {
			string ngvClkRrSel = "";
			CHtCode & ngvPreRegRrSel = ngvPreReg_1x;
			CHtCode & ngvRegRrSel = ngvReg_1x;

			for (int rngIdx = 0; rngIdx < 2; rngIdx += 1) {
				char rngCh = 'A' + rngIdx;

				int ngvPortLo = rngIdx == 0 ? 0 : ngvPortCnt / 2;
				int ngvPortHi = rngIdx == 0 ? ngvPortCnt / 2 : ngvPortCnt;
				int ngvPortRngCnt = ngvPortHi - ngvPortLo;

				ngvRegDecl.Append("\tbool c_t0_gwWrEn%c%s;\n",
					rngCh, pGv->m_dimenDecl.c_str());
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
					VA("r_t1_gwWrEn%c_sig", rngCh), pGv->m_dimenList);

				ngvRegDecl.Append("\tCGW_%s c_t0_gwData%c%s;\n",
					pNgvInfo->m_ngvWrType.c_str(),
					rngCh, pGv->m_dimenDecl.c_str());
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<CGW_%s>", pNgvInfo->m_ngvWrType.c_str()),
					VA("r_t1_gwData%c_sig", rngCh), pGv->m_dimenList);
				ngvRegDecl.NewLine();

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvPreRegRrSel.Append("\tc_t0_gwWrEn%c%s = false;\n", rngCh, dimIdx.c_str());
					ngvPreRegRrSel.Append("\tc_t0_gwData%c%s = 0;\n", rngCh, dimIdx.c_str());
					ngvPreRegRrSel.NewLine();

					for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
						char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

						//ngvPreRegRrSel.Append("\tc_t0_%s_%cwWrEn%s = c_t0_%s_%cwRrSel%s;\n",
						//	mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
						//	mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						if (mod.m_threads.m_htIdW.AsInt() > 0) {
							ngvPreRegRrSel.Append("\tc_t0_%s_%cwHtId%s = r_%s_%cwHtId%s;\n",
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
						}
						ngvPreRegRrSel.Append("\tc_t0_%s_%cwComp%s = r_%s_%cwComp%s;\n",
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						ngvPreRegRrSel.NewLine();
					}

					for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
						char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

						ngvPreRegRrSel.Append("\tif (c_t0_%s_%cwRrSel%s) {\n",
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						ngvPreRegRrSel.Append("\t\tc_rrSel%c%s = 0x%x;\n",
							rngCh, dimIdx.c_str(),
							1 << ((ngvIdx + 1 - ngvPortLo) % ngvPortRngCnt));

						ngvPreRegRrSel.Append("\t\tc_t0_gwWrEn%c%s = true;\n", rngCh, dimIdx.c_str());
						ngvPreRegRrSel.Append("\t\tc_t0_gwData%c%s = r_%s_%cwData%s;\n",
							rngCh, dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						ngvPreRegRrSel.Append("\t}\n");
					}
					ngvPreRegRrSel.NewLine();

					ngvRegRrSel.Append("\tr_t1_gwWrEn%c_sig%s = c_t0_gwWrEn%c%s;\n",
						rngCh, dimIdx.c_str(),
						rngCh, dimIdx.c_str());

					ngvRegRrSel.Append("\tr_t1_gwData%c_sig%s = c_t0_gwData%c%s;\n",
						rngCh, dimIdx.c_str(),
						rngCh, dimIdx.c_str());

					ngvRegRrSel.NewLine();

				} while (DimenIter(pGv->m_dimenList, refList));
			}

			{
				ngvRegDecl.Append("\tbool c_t1_gwWrEn_2x%s;\n",
					pGv->m_dimenDecl.c_str());

				ngvRegDecl.Append("\tCGW_%s c_t1_gwData_2x%s;\n",
					pNgvInfo->m_ngvWrType.c_str(),
					pGv->m_dimenDecl.c_str());
				ngvRegDecl.NewLine();
			}

			for (int rngIdx = 0; rngIdx < 2; rngIdx += 1) {
				char rngCh = 'A' + rngIdx;

				if (rngIdx == 0)
					ngvPreReg_2x.Append("\tif (r_phase) {\n");
				else
					ngvPreReg_2x.Append("\t} else {\n");

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvPreReg_2x.Append("\t\tc_t1_gwWrEn_2x%s = r_t1_gwWrEn%c_sig%s;\n",
						dimIdx.c_str(),
						rngCh, dimIdx.c_str());

					ngvPreReg_2x.Append("\t\tc_t1_gwData_2x%s = r_t1_gwData%c_sig%s;\n",
						dimIdx.c_str(),
						rngCh, dimIdx.c_str());

				} while (DimenIter(pGv->m_dimenList, refList));
			}
			ngvPreReg_2x.Append("\t}\n");
			ngvPreReg_2x.NewLine();

			if (ngvFieldCnt == 1 && !bNgvAtomic) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvPreReg_2x.Append("\tc_t1_wrEn_2x%s = c_t1_gwWrEn_2x%s;\n",
						dimIdx.c_str(),
						dimIdx.c_str());

					if (pGv->m_addr1W.AsInt() > 0) {
						ngvPreReg_2x.Append("\tc_t1_wrAddr1_2x%s = c_t1_gwData_2x%s.m_addr1;\n",
							dimIdx.c_str(),
							dimIdx.c_str());
					}
					if (pGv->m_addr2W.AsInt() > 0) {
						ngvPreReg_2x.Append("\tc_t1_wrAddr2_2x%s = c_t1_gwData_2x%s.m_addr2;\n",
							dimIdx.c_str(),
							dimIdx.c_str());
					}

					for (CStructElemIter iter(this, pGv->m_type); !iter.end(); iter++) {
						if (iter.IsStructOrUnion()) continue;

						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							ngvPreReg_2x.Append("\tc_t1_wrData_2x%s%s = c_t1_gwData_2x%s%s.m_data;\n",
								dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
								dimIdx.c_str(), iter.GetHeirFieldName().c_str());

						} while (DimenIter(pGv->m_dimenList, refList));
					}
					ngvPreReg_2x.NewLine();

				} while (DimenIter(pGv->m_dimenList, refList));
			}

			stgIdx += 1;
			wrCompStg = "t1_";

		} else if (bRrSelEO) {
			if (ngvPortList.size() == 1) {
				// no RR, just prepare for next stage inputs

				for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
					CRam * pModNgv = ngvModInfoList[ngvPortList[ngvIdx].first].m_pNgv;
					CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
					char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

					for (int eoIdx = 0; eoIdx < 2; eoIdx += 1) {
						string eoStr;
						bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? pModNgv->m_bMaxIw : pModNgv->m_bMaxMw);
						if (bRrSelEO && bMaxWr) {
							eoStr = eoIdx == 0 ? "E" : "O";
						} else if (eoIdx == 1)
							continue;

						ngvRegDecl.Append("\tbool c_t0_gwWrEn%s%s;\n", eoStr.c_str(), pGv->m_dimenDecl.c_str());
						if (pGv->m_addr1W.AsInt() > 0) {
							ngvRegDecl.Append("\tht_uint%d c_t0_gwAddr1%s%s;\n",
								pGv->m_addr1W.AsInt(), eoStr.c_str(), pGv->m_dimenDecl.c_str());
						}
						if (pGv->m_addr2W.AsInt() > 0) {
							ngvRegDecl.Append("\tht_uint%d c_t0_gwAddr2%s%s;\n",
								pGv->m_addr2W.AsInt(), eoStr.c_str(), pGv->m_dimenDecl.c_str());
						}
						ngvRegDecl.NewLine();

						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							ngvPreReg_1x.Append("\tc_t0_gwWrEn%s%s = r_%s_%cwWrEn%s%s%s;\n",
								eoStr.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str());

							if (pGv->m_addr1W.AsInt() > 0) {
								ngvPreReg_1x.Append("\tc_t0_gwAddr1%s%s = r_%s_%cwData%s%s%s.read().m_addr1;\n",
									eoStr.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), queRegDataSig.c_str(), dimIdx.c_str());
							}
							if (pGv->m_addr2W.AsInt() > 0) {
								ngvPreReg_1x.Append("\tc_t0_gwAddr2%s%s = r_%s_%cwData%s%s%s.read().m_addr2;\n",
									eoStr.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), queRegDataSig.c_str(), dimIdx.c_str());
							}

						} while (DimenIter(pGv->m_dimenList, refList));
						ngvPreReg_1x.NewLine();
					}
				}

				for (int eoIdx = 0; eoIdx < 2; eoIdx += 1) {
					string eoStr;
					if (bRrSelEO) {
						eoStr = eoIdx == 0 ? "E" : "O";
					} else if (eoIdx == 1)
						continue;

					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
						VA("r_t1_gwWrEn%s_sig", eoStr.c_str()), pGv->m_dimenList);
					if (pGv->m_addr1W.AsInt() > 0) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<ht_uint%d>", pGv->m_addr1W.AsInt()),
							VA("r_t1_gwAddr1%s_sig", eoStr.c_str()), pGv->m_dimenList);
					}
					if (pGv->m_addr2W.AsInt() > 0) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<ht_uint%d>", pGv->m_addr2W.AsInt()),
							VA("r_t1_gwAddr2%s_sig", eoStr.c_str()), pGv->m_dimenList);
					}
					ngvRegDecl.NewLine();

					{
						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							ngvReg_1x.Append("\tr_t1_gwWrEn%s_sig%s = c_t0_gwWrEn%s%s;\n",
								eoStr.c_str(), dimIdx.c_str(),
								eoStr.c_str(), dimIdx.c_str());

							if (pGv->m_addr1W.AsInt() > 0) {
								ngvReg_1x.Append("\tr_t1_gwAddr1%s_sig%s = c_t0_gwAddr1%s%s;\n",
									eoStr.c_str(), dimIdx.c_str(),
									eoStr.c_str(), dimIdx.c_str());
							}
							if (pGv->m_addr2W.AsInt() > 0) {
								ngvReg_1x.Append("\tr_t1_gwAddr2%s_sig%s = c_t0_gwAddr2%s%s;\n",
									eoStr.c_str(), dimIdx.c_str(),
									eoStr.c_str(), dimIdx.c_str());
							}

						} while (DimenIter(pGv->m_dimenList, refList));
						ngvReg_1x.NewLine();
					}
				}

				// generate phase selection
				{
					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
						char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

						ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSelE_2x%s;\n",
							mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());

						ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSelO_2x%s;\n",
							mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());

						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							ngvPreReg_2x.Append("\tc_t0_%s_%cwRrSelE_2x%s = r_%s_%cwWrEnE%s%s && (!r_t1_gwWrEnE_sig%s || r_%s_%cwDataE%s%s.read().m_addr1 != r_t1_gwAddr1E_sig%s);\n",
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
								dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(),
								dimIdx.c_str());

							ngvPreReg_2x.Append("\tc_t0_%s_%cwRrSelO_2x%s = r_%s_%cwWrEnO%s%s && (!r_t1_gwWrEnO_sig%s || r_%s_%cwDataO%s%s.read().m_addr1 != r_t1_gwAddr1O_sig%s);\n",
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
								dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(),
								dimIdx.c_str());
						} while (DimenIter(pGv->m_dimenList, refList));
					}
					ngvPreReg_2x.NewLine();

					ngvRegDecl.Append("\tbool c_t0_gwWrEn_2x%s;\n",
						pGv->m_dimenDecl.c_str());

					ngvRegDecl.Append("\tCGW_%s c_t0_gwData_2x%s;\n",
						pNgvInfo->m_ngvWrType.c_str(), pGv->m_dimenDecl.c_str());

					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
						char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

						ngvRegDecl.Append("\tbool c_t0_%s_%cwWrEn_2x%s;\n",
							mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());

						if (mod.m_threads.m_htIdW.AsInt() > 0) {
							ngvRegDecl.Append("\tht_uint%d c_t0_%s_%cwHtId_2x%s;\n",
								mod.m_threads.m_htIdW.AsInt(),
								mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
						}

						ngvRegDecl.Append("\tbool c_t0_%s_%cwComp_2x%s;\n",
							mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());

						ngvRegDecl.NewLine();
					}

					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvPreReg_2x.Append("\tif (r_phase) {\n");

						for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
							CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
							char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

							ngvPreReg_2x.Append("\t\tc_t0_gwWrEn_2x%s = c_t0_%s_%cwRrSelE_2x%s;\n",
								dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
							ngvPreReg_2x.Append("\t\tc_t0_gwData_2x%s = r_%s_%cwDataE%s%s;\n",
								dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());
							ngvPreReg_2x.NewLine();

							ngvPreReg_2x.Append("\t\tc_t0_%s_%cwWrEn_2x%s = c_t0_%s_%cwRrSelE_2x%s;\n",
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

							if (mod.m_threads.m_htIdW.AsInt() > 0) {
								ngvPreReg_2x.Append("\t\tc_t0_%s_%cwHtId_2x%s = r_%s_%cwHtIdE%s%s;\n",
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegHtIdSig.c_str(), dimIdx.c_str());
							}
							ngvPreReg_2x.Append("\t\tc_t0_%s_%cwComp_2x%s = r_%s_%cwCompE%s%s;\n",
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegCompSig.c_str(), dimIdx.c_str());
						}

						ngvPreReg_2x.Append("\t} else {\n");

						for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
							CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
							char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

							ngvPreReg_2x.Append("\t\tc_t0_gwWrEn_2x%s = c_t0_%s_%cwRrSelO_2x%s;\n",
								dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
							ngvPreReg_2x.Append("\t\tc_t0_gwData_2x%s = r_%s_%cwDataO%s%s;\n",
								dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());
							ngvPreReg_2x.NewLine();

							ngvPreReg_2x.Append("\t\tc_t0_%s_%cwWrEn_2x%s = c_t0_%s_%cwRrSelO_2x%s;\n",
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

							if (mod.m_threads.m_htIdW.AsInt() > 0) {
								ngvPreReg_2x.Append("\t\tc_t0_%s_%cwHtId_2x%s = r_%s_%cwHtIdO%s%s;\n",
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegHtIdSig.c_str(), dimIdx.c_str());
							}
							ngvPreReg_2x.Append("\t\tc_t0_%s_%cwComp_2x%s = r_%s_%cwCompO%s%s;\n",
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegCompSig.c_str(), dimIdx.c_str());
						}

						ngvPreReg_2x.Append("\t}\n");

					} while (DimenIter(pGv->m_dimenList, refList));
					ngvPreReg_2x.NewLine();
				}

				{
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
						"r_t1_gwWrEn_2x", pGv->m_dimenList);

					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("CGW_%s", pNgvInfo->m_ngvWrType.c_str()),
						"r_t1_gwData_2x", pGv->m_dimenList);

					ngvRegDecl.Append("\tbool c_t1_gwWrEn_2x%s;\n",
						pGv->m_dimenDecl.c_str());

					ngvRegDecl.Append("\tCGW_%s c_t1_gwData_2x%s;\n",
						pNgvInfo->m_ngvWrType.c_str(), pGv->m_dimenDecl.c_str());

					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
						char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
							VA("r_t1_%s_%cwWrEn_2x", mod.m_modName.Lc().c_str(), imCh), pGv->m_dimenList);

						if (mod.m_threads.m_htIdW.AsInt() > 0) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", mod.m_threads.m_htIdW.AsInt()),
								VA("r_t1_%s_%cwHtId_2x", mod.m_modName.Lc().c_str(), imCh), pGv->m_dimenList);
						}

						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
							VA("r_t1_%s_%cwComp_2x", mod.m_modName.Lc().c_str(), imCh), pGv->m_dimenList);

						ngvRegDecl.NewLine();
					}

					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvReg_2x.Append("\tr_t1_gwWrEn_2x%s = c_t0_gwWrEn_2x%s;\n",
							dimIdx.c_str(), dimIdx.c_str());

						ngvReg_2x.Append("\tr_t1_gwData_2x%s = c_t0_gwData_2x%s;\n",
							dimIdx.c_str(), dimIdx.c_str());
						ngvReg_2x.NewLine();

						for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
							CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
							char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

							ngvReg_2x.Append("\tr_t1_%s_%cwWrEn_2x%s = c_t0_%s_%cwWrEn_2x%s;\n",
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

							if (mod.m_threads.m_htIdW.AsInt() > 0) {
								ngvReg_2x.Append("\tr_t1_%s_%cwHtId_2x%s = c_t0_%s_%cwHtId_2x%s;\n",
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
							}
							ngvReg_2x.Append("\tr_t1_%s_%cwComp_2x%s = c_t0_%s_%cwComp_2x%s;\n",
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

							ngvReg_2x.NewLine();
						}

						ngvPreReg_2x.Append("\tc_t1_gwWrEn_2x%s = r_t1_gwWrEn_2x%s;\n",
							dimIdx.c_str(), dimIdx.c_str());

						ngvPreReg_2x.Append("\tc_t1_gwData_2x%s = r_t1_gwData_2x%s;\n",
							dimIdx.c_str(), dimIdx.c_str());

					} while (DimenIter(pGv->m_dimenList, refList));
					ngvPreReg_2x.NewLine();
				}

			} else {
				// Gen EO RR muxing

				for (int eoIdx = 0; eoIdx < 2; eoIdx += 1) {
					char eoCh = eoIdx == 0 ? 'E' : 'O';

					ngvRegDecl.Append("\tbool c_t0_gwWrEn%c%s;\n", eoCh, pGv->m_dimenDecl.c_str());
					ngvRegDecl.Append("\tCGW_%s c_t0_gwData%c%s;\n",
						pNgvInfo->m_ngvWrType.c_str(),
						eoCh, pGv->m_dimenDecl.c_str());
					ngvRegDecl.NewLine();

					{
						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							ngvPreReg_1x.Append("\tc_t0_gwWrEn%c%s = false;\n", eoCh, dimIdx.c_str());
							ngvPreReg_1x.Append("\tc_t0_gwData%c%s = 0;\n", eoCh, dimIdx.c_str());
							ngvPreReg_1x.NewLine();

							for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
								CRam * pModNgv = ngvModInfoList[ngvPortList[ngvIdx].first].m_pNgv;
								CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
								char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';
								bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? pModNgv->m_bMaxIw : pModNgv->m_bMaxMw);
								string maxWrStr = bMaxWr ? (eoIdx == 0 ? "E" : "O") : "";

								if (mod.m_threads.m_htIdW.AsInt() > 0) {
									ngvPreReg_1x.Append("\tc_t0_%s_%cwHtId%c%s = r_%s_%cwHtId%s%s;\n",
										mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, maxWrStr.c_str(), dimIdx.c_str());
								}

								ngvPreReg_1x.Append("\tc_t0_%s_%cwComp%c%s = r_%s_%cwComp%s%s;\n",
									mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, maxWrStr.c_str(), dimIdx.c_str());
								ngvPreReg_1x.NewLine();
							}

						} while (DimenIter(pGv->m_dimenList, refList));
					}

					{
						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
								CRam * pModNgv = ngvModInfoList[ngvPortList[ngvIdx].first].m_pNgv;
								CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
								char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';
								bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? pModNgv->m_bMaxIw : pModNgv->m_bMaxMw);
								string maxWrStr = bMaxWr ? (eoIdx == 0 ? "E" : "O") : "";

								ngvPreReg_1x.Append("\tif (c_t0_%s_%cwRrSel%c%s) {\n",
									mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());

								ngvPreReg_1x.Append("\t\tc_rrSel%c%s = 0x%x;\n", 
									eoCh, dimIdx.c_str(),
									1 << ((ngvIdx + 1) % ngvPortCnt));

								ngvPreReg_1x.Append("\t\tc_t0_gwWrEn%c%s = true;\n",
									eoCh, dimIdx.c_str());

								ngvPreReg_1x.Append("\t\tc_t0_gwData%c%s = r_%s_%cwData%s%s;\n",
									eoCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, maxWrStr.c_str(), dimIdx.c_str());

								ngvPreReg_1x.Append("\t}\n");
							}
							ngvPreReg_1x.NewLine();

						} while (DimenIter(pGv->m_dimenList, refList));
					}
				}

				stgIdx += 1;

				for (int eoIdx = 0; eoIdx < 2; eoIdx += 1) {
					char eoCh = eoIdx == 0 ? 'E' : 'O';

					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
						VA("r_t1_gwWrEn%c_sig", eoCh), pGv->m_dimenList);
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<CGW_%s>", pNgvInfo->m_ngvWrType.c_str()),
						VA("r_t1_gwData%c_sig", eoCh), pGv->m_dimenList);
					ngvRegDecl.NewLine();

					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
						char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
							VA("r_t1_%s_%cwRrSel%c_sig", mod.m_modName.Lc().c_str(), imCh, eoCh), pGv->m_dimenList);

						if (mod.m_threads.m_htIdW.AsInt() > 0) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<ht_uint%d>", mod.m_threads.m_htIdW.AsInt()),
								VA("r_t1_%s_%cwHtId%c_sig", mod.m_modName.Lc().c_str(), imCh, eoCh), pGv->m_dimenList);
						}

						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
							VA("r_t1_%s_%cwComp%c_sig", mod.m_modName.Lc().c_str(), imCh, eoCh), pGv->m_dimenList);
					}

					{
						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							ngvReg_1x.Append("\tr_t1_gwWrEn%c_sig%s = c_t0_gwWrEn%c%s;\n",
								eoCh, dimIdx.c_str(),
								eoCh, dimIdx.c_str());

							ngvReg_1x.Append("\tr_t1_gwData%c_sig%s = c_t0_gwData%c%s;\n",
								eoCh, dimIdx.c_str(),
								eoCh, dimIdx.c_str());
							ngvReg_1x.NewLine();

							for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
								CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
								char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

								ngvReg_1x.Append("\tr_t1_%s_%cwRrSel%c_sig%s = c_t0_%s_%cwRrSel%c%s;\n",
									mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());

								if (mod.m_threads.m_htIdW.AsInt() > 0) {
									ngvReg_1x.Append("\tr_t1_%s_%cwHtId%c_sig%s = c_t0_%s_%cwHtId%c%s;\n",
										mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());
								}

								ngvReg_1x.Append("\tr_t1_%s_%cwComp%c_sig%s = c_t0_%s_%cwComp%c%s;\n",
									mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());
								ngvReg_1x.NewLine();
							}

						} while (DimenIter(pGv->m_dimenList, refList));
					}
				}

				// generate phase selection
				{
					{
						ngvRegDecl.Append("\tbool c_t1_gwWrEn_2x%s;\n",
							pGv->m_dimenDecl.c_str());

						ngvRegDecl.Append("\tCGW_%s c_t1_gwData_2x%s;\n",
							pNgvInfo->m_ngvWrType.c_str(), pGv->m_dimenDecl.c_str());

						for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
							CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
							char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

							ngvRegDecl.Append("\tbool c_t1_%s_%cwWrEn_2x%s;\n",
								mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());

							if (mod.m_threads.m_htIdW.AsInt() > 0) {
								ngvRegDecl.Append("\tht_uint%d c_t1_%s_%cwHtId_2x%s;\n",
									mod.m_threads.m_htIdW.AsInt(),
									mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
							}

							ngvRegDecl.Append("\tbool c_t1_%s_%cwComp_2x%s;\n",
								mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());

							ngvRegDecl.NewLine();
						}

						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							for (int eoIdx = 0; eoIdx < 2; eoIdx += 1) {
								char eoCh = eoIdx == 0 ? 'E' : 'O';

								if (eoIdx == 0)
									ngvPreReg_2x.Append("\tif (r_phase) {\n");
								else
									ngvPreReg_2x.Append("\t} else {\n");

								ngvPreReg_2x.Append("\t\tc_t1_gwWrEn_2x%s = r_t1_gwWrEn%c_sig%s;\n",
									dimIdx.c_str(),
									eoCh, dimIdx.c_str());
								ngvPreReg_2x.Append("\t\tc_t1_gwData_2x%s = r_t1_gwData%c_sig%s;\n",
									dimIdx.c_str(),
									eoCh, dimIdx.c_str());
								ngvPreReg_2x.NewLine();

								for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
									CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
									char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

									ngvPreReg_2x.Append("\t\tc_t1_%s_%cwWrEn_2x%s = r_t1_%s_%cwRrSel%c_sig%s;\n",
										mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());

									if (mod.m_threads.m_htIdW.AsInt() > 0) {
										ngvPreReg_2x.Append("\t\tc_t1_%s_%cwHtId_2x%s = r_t1_%s_%cwHtId%c_sig%s;\n",
											mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());
									}
									ngvPreReg_2x.Append("\t\tc_t1_%s_%cwComp_2x%s = r_t1_%s_%cwComp%c_sig%s;\n",
										mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());

									ngvPreReg_2x.NewLine();
								}
							}

							ngvPreReg_2x.Append("\t}\n");

						} while (DimenIter(pGv->m_dimenList, refList));
						ngvPreReg_2x.NewLine();
					}

					{
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
							"r_t2_gwWrEn_2x", pGv->m_dimenList);

						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("CGW_%s", pNgvInfo->m_ngvWrType.c_str()),
							"r_t2_gwData_2x", pGv->m_dimenList);

						ngvRegDecl.Append("\tbool c_t2_gwWrEn_2x%s;\n",
							pGv->m_dimenDecl.c_str());

						ngvRegDecl.Append("\tCGW_%s c_t2_gwData_2x%s;\n",
							pNgvInfo->m_ngvWrType.c_str(), pGv->m_dimenDecl.c_str());

						for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
							CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
							char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
								VA("r_t2_%s_%cwWrEn_2x", mod.m_modName.Lc().c_str(), imCh), pGv->m_dimenList);

							if (mod.m_threads.m_htIdW.AsInt() > 0) {
								GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", mod.m_threads.m_htIdW.AsInt()),
									VA("r_t2_%s_%cwHtId_2x", mod.m_modName.Lc().c_str(), imCh), pGv->m_dimenList);
							}

							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
								VA("r_t2_%s_%cwComp_2x", mod.m_modName.Lc().c_str(), imCh), pGv->m_dimenList);

							ngvRegDecl.NewLine();
						}

						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							ngvReg_2x.Append("\tr_t2_gwWrEn_2x%s = c_t1_gwWrEn_2x%s;\n",
								dimIdx.c_str(), dimIdx.c_str());

							ngvReg_2x.Append("\tr_t2_gwData_2x%s = c_t1_gwData_2x%s;\n",
								dimIdx.c_str(), dimIdx.c_str());
							ngvReg_2x.NewLine();

							for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
								CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
								char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

								ngvReg_2x.Append("\tr_t2_%s_%cwWrEn_2x%s = c_t1_%s_%cwWrEn_2x%s;\n",
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

								if (mod.m_threads.m_htIdW.AsInt() > 0) {
									ngvReg_2x.Append("\tr_t2_%s_%cwHtId_2x%s = c_t1_%s_%cwHtId_2x%s;\n",
										mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
								}
								ngvReg_2x.Append("\tr_t2_%s_%cwComp_2x%s = c_t1_%s_%cwComp_2x%s;\n",
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

								ngvReg_2x.NewLine();
							}

							ngvPreReg_2x.Append("\tc_t2_gwWrEn_2x%s = r_t2_gwWrEn_2x%s;\n",
								dimIdx.c_str(), dimIdx.c_str());

							ngvPreReg_2x.Append("\tc_t2_gwData_2x%s = r_t2_gwData_2x%s;\n",
								dimIdx.c_str(), dimIdx.c_str());

						} while (DimenIter(pGv->m_dimenList, refList));
						ngvPreReg_2x.NewLine();
					}
				}
			}

			stgIdx += 1;
			wrCompStg = VA("t%d_", stgIdx);

		} else if (ngvPortList.size() == 1 && ngvFieldCnt == 1 && !bNgvAtomic) {
			CModule &mod = *ngvModInfoList[ngvPortList[0].first].m_pMod;
			char imCh = ngvPortList[0].second == 0 ? 'i' : 'm';

			CHtCode &ngvPreRegMod = mod.m_clkRate == eClk2x ? ngvPreReg_2x : ngvPreReg_1x;
			string ngvModClk = mod.m_clkRate == eClk2x ? "_2x" : "";

			{
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvPreRegMod.Append("\tc_t%d_wrEn%s%s = r_%s_%cwWrEn%s%s;\n",
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						mod.m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str());

				} while (DimenIter(pGv->m_dimenList, refList));
			}

			if (pGv->m_addr1W.AsInt() > 0) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvPreRegMod.Append("\tc_t%d_wrAddr1%s%s = r_%s_%cwData%s%s.m_addr1;\n",
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						mod.m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str());

				} while (DimenIter(pGv->m_dimenList, refList));
			}

			if (pGv->m_addr2W.AsInt() > 0) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvPreRegMod.Append("\tc_t%d_wrAddr2%s%s = r_%s_%cwData%s%s.m_addr2;\n",
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						mod.m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str());

				} while (DimenIter(pGv->m_dimenList, refList));
			}

			for (CStructElemIter iter(this, pGv->m_type); !iter.end(); iter++) {
				if (iter.IsStructOrUnion()) continue;

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvPreRegMod.Append("\tc_t%d_wrData%s%s%s = r_%s_%cwData%s%s%s.m_data;\n",
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
						mod.m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());

				} while (DimenIter(pGv->m_dimenList, refList));
			}
			ngvPreRegMod.NewLine();

		} else if (ngvPortList.size() == 1 && (ngvFieldCnt > 1 || bNgvAtomic)) {
			CModule &mod = *ngvModInfoList[ngvPortList[0].first].m_pMod;
			char imCh = ngvPortList[0].second == 0 ? 'i' : 'm';

			CHtCode &ngvPreRegMod = pNgvInfo->m_bNgvWrDataClk2x ? ngvPreReg_2x : ngvPreReg_1x;
			string ngvModClk = pNgvInfo->m_bNgvWrDataClk2x ? "_2x" : "";

			ngvRegDecl.Append("\tbool c_t%d_gwWrEn%s%s;\n",
				stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

			ngvRegDecl.Append("\tCGW_%s c_t%d_gwData%s%s;\n",
				pGv->m_pNgvInfo->m_ngvWrType.c_str(),
				stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
			ngvRegDecl.NewLine();

			{
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvPreRegMod.Append("\tc_t%d_gwWrEn%s%s = r_%s_%cwWrEn%s%s;\n",
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						mod.m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str());

				} while (DimenIter(pGv->m_dimenList, refList));
			}

				{
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvPreRegMod.Append("\tc_t%d_gwData%s%s = r_%s_%cwData%s%s;\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str());

					} while (DimenIter(pGv->m_dimenList, refList));
				}
			ngvPreRegMod.NewLine();

		} else if (ngvPortList.size() == 2 && bAllModClk1x) {

			if (ngvFieldCnt > 1 || bNgvAtomic) {
				ngvRegDecl.Append("\tbool c_t%d_gwWrEn%s%s;\n",
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

				ngvRegDecl.Append("\tCGW_%s c_t%d_gwData%s%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(),
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				ngvRegDecl.NewLine();
			}

			for (int phaseIdx = 0; phaseIdx < 2; phaseIdx += 1) {
				if (phaseIdx == 0)
					ngvPreReg_2x.Append("\tif (r_phase == false) {\n");
				else
					ngvPreReg_2x.Append("\t} else {\n");

				CModule &mod = *ngvModInfoList[ngvPortList[phaseIdx].first].m_pMod;
				char imCh = ngvPortList[phaseIdx].second == 0 ? 'i' : 'm';

				{
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						if (ngvFieldCnt == 1 && !bNgvAtomic) {

							ngvPreReg_2x.Append("\t\tc_t%d_wrEn%s%s = r_%s_%cwWrEn%s%s;\n",
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str());

						} else {
							ngvPreReg_2x.Append("\t\tc_t%d_gwWrEn%s%s = r_%s_%cwWrEn%s%s;\n",
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str());
						}

					} while (DimenIter(pGv->m_dimenList, refList));
				}

				if (pGv->m_addr1W.AsInt() > 0) {
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvPreReg_2x.Append("\t\tc_t%d_wrAddr1%s%s = r_%s_%cwData%s%s.read().m_addr1;\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());

					} while (DimenIter(pGv->m_dimenList, refList));
				}

				if (pGv->m_addr2W.AsInt() > 0) {
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvPreReg_2x.Append("\t\tc_t%d_wrAddr2%s%s = r_%s_%cwData%s%s.read().m_addr2;\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());

					} while (DimenIter(pGv->m_dimenList, refList));
				}

				if (ngvFieldCnt == 1 && !bNgvAtomic) {
					for (CStructElemIter iter(this, pGv->m_type); !iter.end(); iter++) {
						if (iter.IsStructOrUnion()) continue;

						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							ngvPreReg_2x.Append("\t\tc_t%d_wrData%s%s%s = r_%s_%cwData%s%s.read()%s.m_data;\n",
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());

						} while (DimenIter(pGv->m_dimenList, refList));
					}
				} else {
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvPreReg_2x.Append("\t\tc_t%d_gwData%s%s = r_%s_%cwData%s%s;\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());

					} while (DimenIter(pGv->m_dimenList, refList));
				}
			}
			ngvPreReg_2x.Append("\t}\n");
			ngvPreReg_2x.NewLine();

		}

		if (ngvFieldCnt > 1 || bNgvAtomic) {

			//if (pGv->m_ramType == eBlockRam) {
			//	GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
			//		VA("r_t%d_gwWrEn%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
			//}

			if (bNgvBlock) {
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
					VA("r_t%d_gwData%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				ngvRegDecl.NewLine();

				if (bNgvAtomic) {
					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
						char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

						if (stgIdx > 0) {
							ngvRegDecl.Append("\tbool c_t%d_%s_%cwWrEn%s%s;\n",
								stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
						}

						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
							VA("r_t%d_%s_%cwWrEn%s", stgIdx + 1, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str()), pGv->m_dimenList);

						if (mod.m_threads.m_htIdW.AsInt() > 0) {
							ngvRegDecl.Append("\tht_uint%d c_t%d_%s_%cwHtId%s%s;\n",
								mod.m_threads.m_htIdW.AsInt(),
								stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", mod.m_threads.m_htIdW.AsInt()),
								VA("r_t%d_%s_%cwHtId%s", stgIdx + 1, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str()), pGv->m_dimenList);
						}

						//GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
						//	VA("r_t2_%s_%cwComp_2x", mod.m_modName.Lc().c_str(), imCh), pGv->m_dimenList);

						ngvRegDecl.NewLine();
					}
				}

				ngvRegDecl.Append("\tCGW_%s c_t%d_gwData%s%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(),
					stgIdx + 1, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				ngvRegDecl.NewLine();
			}

			CHtCode & ngvReg = pNgvInfo->m_bNgvWrDataClk2x ? ngvReg_2x : ngvReg_1x;

			if (bNgvBlock) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					//ngvReg.Append("\tr_t%d_gwWrEn%s%s = c_t%d_gwWrEn%s%s;\n",
					//	stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
					//	stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					ngvReg.Append("\tr_t%d_gwData%s%s = c_t%d_gwData%s%s;\n",
						stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					ngvReg.NewLine();

					if (bNgvAtomic) {
						for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
							CModule &mod = *ngvModInfoList[ngvPortList[ngvIdx].first].m_pMod;
							char imCh = ngvPortList[ngvIdx].second == 0 ? 'i' : 'm';

							if (stgIdx == 0) {
								ngvReg.Append("\tr_t%d_%s_%cwWrEn%s = c_t%d_%s_%cwRrSel%s;\n",
									stgIdx + 1, mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									stgIdx, mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
							} else {
								ngvPreRegWrData.Append("\tc_t%d_%s_%cwWrEn%s%s = r_t%d_%s_%cwWrEn%s%s;\n",
									stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str(),
									stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str());

								ngvReg.Append("\tr_t%d_%s_%cwWrEn%s%s = c_t%d_%s_%cwWrEn%s%s;\n",
									stgIdx + 1, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str(),
									stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str());
							}

							if (mod.m_threads.m_htIdW.AsInt() > 0) {

								if (stgIdx == 0) {
									ngvPreRegWrData.Append("\tc_t%d_%s_%cwHtId%s = r_%s_%cwHtId%s;\n",
										stgIdx, mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
								} else {
									ngvPreRegWrData.Append("\tc_t%d_%s_%cwHtId%s%s = r_t%d_%s_%cwHtId%s%s;\n",
										stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str(),
										stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str());
								}
								ngvPreReg_1x.NewLine();

								ngvReg.Append("\tr_t%d_%s_%cwHtId%s%s = c_t%d_%s_%cwHtId%s%s;\n",
									stgIdx + 1, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str(),
									stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str());
							}

							ngvReg.NewLine();
						}
					}

				} while (DimenIter(pGv->m_dimenList, refList));
			}
		} else {
			ngvRegDecl.Append("\tbool c_t%d_wrEn%s%s;\n",
				stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

			if (ngvPortList.size() > 1) {
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
					VA("r_t%d_wrEn%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
			}

			if (pGv->m_addr1W.AsInt() > 0) {
				ngvRegDecl.Append("\tht_uint%d c_t%d_wrAddr1%s%s;\n",
					pGv->m_addr1W.AsInt(),
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				if (ngvPortList.size() > 1) {
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addr1W.AsInt()),
						VA("r_t%d_wrAddr1%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				}
			}
			if (pGv->m_addr2W.AsInt() > 0) {
				ngvRegDecl.Append("\tht_uint%d c_t%d_wrAddr2%s%s;\n",
					pGv->m_addr2W.AsInt(),
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				if (ngvPortList.size() > 1) {
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addr2W.AsInt()),
						VA("r_t%d_wrAddr2%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				}
			}

			ngvRegDecl.Append("\t%s c_t%d_wrData%s%s;\n",
				pGv->m_type.c_str(),
				stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
			if (ngvPortList.size() > 1) {
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, pGv->m_type,
					VA("r_t%d_wrData%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
			}
			ngvRegDecl.NewLine();

			CHtCode & ngvReg = pNgvInfo->m_bNgvWrDataClk2x ? ngvReg_2x : ngvReg_1x;

			if (ngvPortList.size() > 1) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvReg.Append("\tr_t%d_wrEn%s%s = c_t%d_wrEn%s%s;\n",
						stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					if (pGv->m_addr1W.AsInt() > 0) {
						ngvReg.Append("\tr_t%d_wrAddr1%s%s = c_t%d_wrAddr1%s%s;\n",
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					}
					if (pGv->m_addr2W.AsInt() > 0) {
						ngvReg.Append("\tr_t%d_wrAddr2%s%s = c_t%d_wrAddr2%s%s;\n",
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					}

					ngvReg.Append("\tr_t%d_wrData%s%s = c_t%d_wrData%s%s;\n",
						stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					ngvReg.NewLine();

				} while (DimenIter(pGv->m_dimenList, refList));

				stgIdx += 1;
			}
		}

		// Write completion output
		for (size_t modIdx = 0; modIdx < ngvModInfoList.size(); modIdx += 1) {
			CModule & mod = *ngvModInfoList[modIdx].m_pMod;
			CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

			for (int imIdx = 0; imIdx < 2; imIdx += 1) {
				if (imIdx == 1 && !pModNgv->m_bMifRead) continue;
				char imCh = imIdx == 0 ? 'i' : 'm';
				//bool bMaxGw = imIdx == 0 ? pModNgv->m_bMaxIw : pModNgv->m_bMaxMw;

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvWrCompOut.NewLine();
					ngvWrCompOut.Append("\to_%sTo%s_%cwCompWrEn%s = r_%s%s_%cwWrEn%s%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), imCh, dimIdx.c_str(),
						wrCompStg.c_str(), mod.m_modName.Lc().c_str(), imCh, ngvSelClk.c_str(), dimIdx.c_str());
					if (mod.m_threads.m_htIdW.AsInt() > 0)
						ngvWrCompOut.Append("\to_%sTo%s_%cwCompHtId%s = r_%s%s_%cwHtId%s%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), imCh, dimIdx.c_str(),
						wrCompStg.c_str(), mod.m_modName.Lc().c_str(), imCh, ngvSelClk.c_str(), dimIdx.c_str());
					ngvWrCompOut.Append("\to_%sTo%s_%cwCompData%s = r_%s%s_%cwComp%s%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), imCh, dimIdx.c_str(),
						wrCompStg.c_str(), mod.m_modName.Lc().c_str(), imCh, ngvSelClk.c_str(), dimIdx.c_str());

				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}
		ngvWrCompOut.NewLine();

		if (ngvFieldCnt > 1 || bNgvAtomic) {
			// declare global variable
			if (bNgvReg) {
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, pGv->m_type.c_str(),
					VA("r_%s", pGv->m_gblName.c_str()), pGv->m_dimenList);

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvRegWrData.Append("\tif (c_t%d_wrEn%s%s)\n",
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					ngvRegWrData.Append("\t\tr_%s%s = c_t%d_wrData%s%s;\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

				} while (DimenIter(pGv->m_dimenList, refList));
			} else {
				ngvRegDecl.Append("\tht_%s_ram<%s, %d",
					pGv->m_pNgvInfo->m_ramType == eBlockRam ? "block" : "dist",
					pGv->m_type.c_str(), pGv->m_addr1W.AsInt());
				if (pGv->m_addr2W.AsInt() > 0)
					ngvRegDecl.Append(", %d", pGv->m_addr2W.AsInt());
				ngvRegDecl.Append("> m_%s%s;\n", pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

				{
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvRegWrData.Append("\tm_%s%s.clock();\n",
							pGv->m_gblName.c_str(), dimIdx.c_str());

					} while (DimenIter(pGv->m_dimenList, refList));
				}
			}
			ngvRegDecl.NewLine();
			ngvRegWrData.NewLine();
		}

		// Instruction fetch outputs
		for (size_t modIdx = 0; modIdx < ngvModInfoList.size(); modIdx += 1) {
			CModule & mod = *ngvModInfoList[modIdx].m_pMod;
			//CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

			int ifStgIdx = stgIdx + 1 + (bNgvBlock ? 1 : 0);

			if (pNgvInfo->m_atomicMask != 0) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					ngvOut.Append("\to_%sTo%s_ifWrEn%s = r_t%d_%s_ifWrEn%s%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						ifStgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
					if (mod.m_threads.m_htIdW.AsInt() > 0)
						ngvOut.Append("\to_%sTo%s_ifHtId%s = r_t%d_%s_ifHtId%s%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						ifStgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
					ngvOut.Append("\to_%sTo%s_ifData%s = r_t%d_%s_ifData%s%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						ifStgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
				ngvOut.NewLine();
			}
		}

		if (ngvFieldCnt > 1 || bNgvAtomic) {

			// read stage of RMW
			ngvRegDecl.Append("\tbool c_t%d_wrEn%s%s;\n",
				stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
			//GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
			//	VA("r_t%d_wrEn%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);

			if (pGv->m_addr1W.AsInt() > 0) {
				ngvRegDecl.Append("\tht_uint%d c_t%d_wrAddr1%s%s;\n",
					pGv->m_addr1W.AsInt(),
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				//GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addr1W.AsInt()),
				//	VA("r_t%d_wrAddr1%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
			}
			if (pGv->m_addr2W.AsInt() > 0) {
				ngvRegDecl.Append("\tht_uint%d c_t%d_wrAddr2%s%s;\n",
					pGv->m_addr2W.AsInt(),
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				//GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addr2W.AsInt()),
				//	VA("r_t%d_wrAddr2%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
			}

			if (!bNgvBlock) {
				ngvRegDecl.Append("\t%s c_t%d_wrData%s%s;\n",
					pGv->m_type.c_str(),
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
			}
			//GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, pGv->m_type,
			//	VA("r_t%d_wrData%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
			ngvRegDecl.NewLine();

			CHtCode & ngvReg = pNgvInfo->m_bNgvWrDataClk2x ? ngvReg_2x : ngvReg_1x;

			{
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvPreRegWrData.Append("\tc_t%d_wrEn%s%s = c_t%d_gwWrEn%s%s;\n",
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

				} while (DimenIter(pGv->m_dimenList, refList));
			}
			if (pGv->m_addr1W.AsInt() > 0) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);


					ngvPreRegWrData.Append("\tc_t%d_wrAddr1%s%s = c_t%d_gwData%s%s.m_addr1;\n",
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

				} while (DimenIter(pGv->m_dimenList, refList));
			}
			if (pGv->m_addr2W.AsInt() > 0) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvPreRegWrData.Append("\tc_t%d_wrAddr2%s%s = c_t%d_gwData%s%s.m_addr2;\n",
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

				} while (DimenIter(pGv->m_dimenList, refList));
			}
			if (bNgvReg) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvPreRegWrData.Append("\tc_t%d_wrData%s%s = r_%s%s;\n",
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());

				} while (DimenIter(pGv->m_dimenList, refList));
			} else {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					if (pGv->m_addr2W.AsInt() == 0) {
						ngvPreRegWrData.Append("\tm_%s%s.read_addr(c_t%d_wrAddr1%s%s);\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					} else {
						ngvPreRegWrData.Append("\tm_%s%s.read_addr(c_t%d_wrAddr1%s%s, c_t%d_wrAddr2%s%s);\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					}

				} while (DimenIter(pGv->m_dimenList, refList));
			}

			if (bNgvBlock) {
				ngvPreRegWrData.NewLine();

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
					VA("r_t%d_wrEn%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);

				if (pGv->m_addr1W.AsInt() > 0) {
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addr1W.AsInt()),
						VA("r_t%d_wrAddr1%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				}
				if (pGv->m_addr2W.AsInt() > 0) {
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addr2W.AsInt()),
						VA("r_t%d_wrAddr2%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				}
				ngvRegDecl.NewLine();

				{
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvReg.Append("\tr_t%d_wrEn%s%s = c_t%d_wrEn%s%s;\n",
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

						if (pGv->m_addr1W.AsInt() > 0) {
							ngvReg.Append("\tr_t%d_wrAddr1%s%s = c_t%d_wrAddr1%s%s;\n",
								stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
						}
						if (pGv->m_addr2W.AsInt() > 0) {
							ngvReg.Append("\tr_t%d_wrAddr2%s%s = c_t%d_wrAddr2%s%s;\n",
								stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
						}
						ngvReg.NewLine();

					} while (DimenIter(pGv->m_dimenList, refList));
				}

				stgIdx += 1;

				ngvRegDecl.Append("\tbool c_t%d_wrEn%s%s;\n",
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

				if (pGv->m_addr1W.AsInt() > 0) {
					ngvRegDecl.Append("\tht_uint%d c_t%d_wrAddr1%s%s;\n",
						pGv->m_addr1W.AsInt(),
						stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				}
				if (pGv->m_addr2W.AsInt() > 0) {
					ngvRegDecl.Append("\tht_uint%d c_t%d_wrAddr2%s%s;\n",
						pGv->m_addr2W.AsInt(),
						stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				}

				ngvRegDecl.Append("\t%s c_t%d_wrData%s%s;\n",
					pGv->m_type.c_str(),
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				ngvRegDecl.NewLine();

				{
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvPreRegWrData.Append("\tc_t%d_gwData%s%s = r_t%d_gwData%s%s;\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					} while (DimenIter(pGv->m_dimenList, refList));
				}
				ngvPreRegWrData.NewLine();

				{
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvPreRegWrData.Append("\tc_t%d_wrEn%s%s = r_t%d_wrEn%s%s;\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					} while (DimenIter(pGv->m_dimenList, refList));
				}
				if (pGv->m_addr1W.AsInt() > 0) {
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);


						ngvPreRegWrData.Append("\tc_t%d_wrAddr1%s%s = r_t%d_wrAddr1%s%s;\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					} while (DimenIter(pGv->m_dimenList, refList));
				}
				if (pGv->m_addr2W.AsInt() > 0) {
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvPreRegWrData.Append("\tc_t%d_wrAddr2%s%s = r_t%d_wrAddr2%s%s;\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					} while (DimenIter(pGv->m_dimenList, refList));
				}
			}

			if (bNgvDist || bNgvBlock) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvPreRegWrData.Append("\tc_t%d_wrData%s%s = m_%s%s.read_mem();\n",
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());

				} while (DimenIter(pGv->m_dimenList, refList));
			}

			ngvPreRegWrData.NewLine();

			if (bNgvAtomic) {
				// register stage for instruction fetch (IF)
				for (size_t modIdx = 0; modIdx < ngvModInfoList.size(); modIdx += 1) {
					CModule & mod = *ngvModInfoList[modIdx].m_pMod;
					//CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

					ngvRegDecl.Append("\tbool c_t%d_%s_ifWrEn%s%s;\n",
						stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
						VA("r_t%d_%s_ifWrEn%s", stgIdx + 1, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str()), pGv->m_dimenList);

					if (mod.m_threads.m_htIdW.AsInt() > 0) {
						ngvRegDecl.Append("\tsc_uint<%s_HTID_W> c_t%d_%s_ifHtId%s%s;\n",
							mod.m_modName.Upper().c_str(),
							stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_uint<%s_HTID_W>", mod.m_modName.Upper().c_str()),
							VA("r_t%d_%s_ifHtId%s", stgIdx + 1, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str()), pGv->m_dimenList);
					}

					ngvRegDecl.Append("\t%s c_t%d_%s_ifData%s%s;\n",
						pGv->m_type.c_str(),
						stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, pGv->m_type,
						VA("r_t%d_%s_ifData%s", stgIdx + 1, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str()), pGv->m_dimenList);

					ngvRegDecl.NewLine();

					string regStg = stgIdx == 0 ? "r" : VA("r_t%d", stgIdx);

					{
						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							if (bRrSelAB) {
								ngvPreRegWrData.Append("\tc_t%d_%s_ifWrEn%s%s = r_t1_%s_iwWrEn%s_sig%s && %sr_phase;\n",
									stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(), (int)modIdx < ngvPortCnt / 2 ? "" : "!");
							} else if (bRrSelEO && pNgvInfo->m_bNgvMaxSel) {
								ngvPreRegWrData.Append("\tc_t%d_%s_ifWrEn%s%s = r_t%d_%s_iwWrEn_2x%s;\n",
									stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
									stgIdx, mod.m_modName.Lc().c_str(), dimIdx.c_str());
							} else if (ngvPortList.size() == 2 && bAllModClk1x) {
								if (bNgvBlock) {
									ngvPreRegWrData.Append("\tc_t%d_%s_ifWrEn%s%s = r_t%d_%s_iwWrEn%s%s;\n",
										stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										stgIdx, mod.m_modName.Lc().c_str(), queRegWrEnSig.c_str(), dimIdx.c_str());
								} else if (bNgvAtomicSlow) {
									ngvPreRegWrData.Append("\tc_t%d_%s_ifWrEn%s%s = r_%s_iwWrEn%s%s;\n",
										stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), queRegWrEnSig.c_str(), dimIdx.c_str());
								} else {
									ngvPreRegWrData.Append("\tc_t%d_%s_ifWrEn%s%s = r_%s_iwWrEn%s%s && %sr_phase;\n",
										stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
										modIdx == 0 ? "!" : "");
								}
							} else {
								ngvPreRegWrData.Append("\tc_t%d_%s_ifWrEn%s%s = %s_%s_iwWrEn%s%s;\n",
									stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
									regStg.c_str(), mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
							}
						} while (DimenIter(pGv->m_dimenList, refList));
					}

					if (mod.m_threads.m_htIdW.AsInt() > 0) {
						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							if (bRrSelAB) {
								ngvPreRegWrData.Append("\tc_t%d_%s_ifHtId%s%s = r_t1_%s_iwHtId%s_sig%s;\n",
									stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
							} else if (bRrSelEO && pNgvInfo->m_bNgvMaxSel) {
								ngvPreRegWrData.Append("\tc_t%d_%s_ifHtId%s%s = r_t%d_%s_iwHtId_2x%s;\n",
									stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
									stgIdx, mod.m_modName.Lc().c_str(), dimIdx.c_str());
							} else if (ngvPortList.size() == 2 && bAllModClk1x) {
								if (bNgvBlock) {
									ngvPreRegWrData.Append("\tc_t%d_%s_ifHtId%s%s = r_t%d_%s_iwHtId%s%s;\n",
										stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										stgIdx, mod.m_modName.Lc().c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
								} else {
									ngvPreRegWrData.Append("\tc_t%d_%s_ifHtId%s%s = r_%s_iwHtId%s%s;\n",
										stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
								}
							} else {
								ngvPreRegWrData.Append("\tc_t%d_%s_ifHtId%s%s = %s_%s_iwHtId%s%s;\n",
									stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
									regStg.c_str(), mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
							}
						} while (DimenIter(pGv->m_dimenList, refList));
					}

					{
						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							ngvPreRegWrData.Append("\tc_t%d_%s_ifData%s%s = c_t%d_wrData%s%s;\n",
								stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

						} while (DimenIter(pGv->m_dimenList, refList));
					}
					ngvPreRegWrData.NewLine();

					{
						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							ngvRegWrData.Append("\tr_t%d_%s_ifWrEn%s%s = c_t%d_%s_ifWrEn%s%s;\n",
								stgIdx + 1, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());

						} while (DimenIter(pGv->m_dimenList, refList));
					}

					if (mod.m_threads.m_htIdW.AsInt() > 0) {
						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							ngvRegWrData.Append("\tr_t%d_%s_ifHtId%s%s = c_t%d_%s_ifHtId%s%s;\n",
								stgIdx + 1, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());

						} while (DimenIter(pGv->m_dimenList, refList));
					}

					{
						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							ngvRegWrData.Append("\tr_t%d_%s_ifData%s%s = c_t%d_%s_ifData%s%s;\n",
								stgIdx + 1, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());

						} while (DimenIter(pGv->m_dimenList, refList));
					}
					ngvRegWrData.NewLine();
				}
			}

			bool bNeedRamReg = bNgvDist && bNgvAtomicSlow && (ngvPortList.size() >= 2 || !bAllModClk1x) && pNgvInfo->m_bNgvMaxSel ||
				bNgvBlock && (bNgvAtomic || ngvFieldCnt > 1) && pNgvInfo->m_bNgvMaxSel;

			if (bNeedRamReg) {
				// register output of ram and other signals

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("CGW_%s", pNgvInfo->m_ngvWrType.c_str()),
					VA("r_t%d_gwData%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
					VA("r_t%d_wrEn%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);

				if (pGv->m_addr1W.AsInt() > 0) {
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addr1W.AsInt()),
						VA("r_t%d_wrAddr1%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				}
				if (pGv->m_addr2W.AsInt() > 0) {
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addr2W.AsInt()),
						VA("r_t%d_wrAddr2%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				}

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, pGv->m_type,
					VA("r_t%d_wrData%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				ngvRegDecl.NewLine();

				{
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvRegWrData.Append("\tr_t%d_gwData%s%s = c_t%d_gwData%s%s;\n",
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					} while (DimenIter(pGv->m_dimenList, refList));
					ngvRegWrData.NewLine();
				}

				{
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvRegWrData.Append("\tr_t%d_wrEn%s%s = c_t%d_wrEn%s%s;\n",
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

						if (pGv->m_addr1W.AsInt() > 0) {
							ngvRegWrData.Append("\tr_t%d_wrAddr1%s%s = c_t%d_wrAddr1%s%s;\n",
								stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
						}
						if (pGv->m_addr2W.AsInt() > 0) {
							ngvRegWrData.Append("\tr_t%d_wrAddr2%s%s = c_t%d_wrAddr2%s%s;\n",
								stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
						}

						ngvRegWrData.Append("\tr_t%d_wrData%s%s = c_t%d_wrData%s%s;\n",
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					} while (DimenIter(pGv->m_dimenList, refList));
					ngvRegWrData.NewLine();
				}

				stgIdx += 1;

				ngvRegDecl.Append("\tCGW_%s c_t%d_gwData%s%s;\n",
					pNgvInfo->m_ngvWrType.c_str(), stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

				ngvRegDecl.Append("\tbool c_t%d_wrEn%s%s;\n", 
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

				if (pGv->m_addr1W.AsInt() > 0) {
					ngvRegDecl.Append("\tht_uint%d c_t%d_wrAddr1%s%s;\n",
						pGv->m_addr1W.AsInt(), stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				}
				if (pGv->m_addr2W.AsInt() > 0) {
					ngvRegDecl.Append("\tht_uint%d c_t%d_wrAddr2%s%s;\n",
						pGv->m_addr2W.AsInt(), stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				}

				ngvRegDecl.Append("\t%s c_t%d_wrData%s%s;\n",
					pGv->m_type.c_str(), stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				ngvRegDecl.NewLine();

				{
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvPreRegWrData.Append("\tc_t%d_gwData%s%s = r_t%d_gwData%s%s;\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					} while (DimenIter(pGv->m_dimenList, refList));
					ngvPreRegWrData.NewLine();
				}

				{
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvPreRegWrData.Append("\tc_t%d_wrEn%s%s = r_t%d_wrEn%s%s;\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

						if (pGv->m_addr1W.AsInt() > 0) {
							ngvPreRegWrData.Append("\tc_t%d_wrAddr1%s%s = r_t%d_wrAddr1%s%s;\n",
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
						}
						if (pGv->m_addr2W.AsInt() > 0) {
							ngvPreRegWrData.Append("\tc_t%d_wrAddr2%s%s = r_t%d_wrAddr2%s%s;\n",
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
						}

						ngvPreRegWrData.Append("\tc_t%d_wrData%s%s = r_t%d_wrData%s%s;\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					} while (DimenIter(pGv->m_dimenList, refList));
					ngvPreRegWrData.NewLine();
				}
			}

			// modify stage of RMW
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);

				for (CStructElemIter iter(this, pGv->m_type); !iter.end(); iter++) {
					if (iter.IsStructOrUnion()) continue;

					ngvPreRegWrData.Append("\tif (c_t%d_gwData%s%s%s.m_bWrite)\n",
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());

					ngvPreRegWrData.Append("\t\tc_t%d_wrData%s%s%s = c_t%d_gwData%s%s%s.m_data;\n",
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());

					if ((iter->m_atomicMask & ATOMIC_INC) != 0) {

						ngvPreRegWrData.Append("\tif (c_t%d_gwData%s%s%s.m_bInc)\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());

						ngvPreRegWrData.Append("\t\tc_t%d_wrData%s%s%s += 1u;\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
					}

					if ((iter->m_atomicMask & ATOMIC_ADD) != 0) {

						ngvPreRegWrData.Append("\tif (c_t%d_gwData%s%s%s.m_bAdd)\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());

						ngvPreRegWrData.Append("\t\tc_t%d_wrData%s%s%s += c_t%d_gwData%s%s%s.m_data;\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
					}
				}

			} while (DimenIter(pGv->m_dimenList, refList));
			ngvPreRegWrData.NewLine();
		}

		bool bNeedRamWrReg = bNgvDist && bNgvAtomicSlow && (ngvPortList.size() >= 2 || !bAllModClk1x) && pNgvInfo->m_bNgvMaxSel ||
			bNgvBlock && (bNgvAtomic || ngvFieldCnt > 1) && pNgvInfo->m_bNgvMaxSel;

		if (bNeedRamWrReg) {

			GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
				VA("r_t%d_wrEn%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);

			if (pGv->m_addr1W.AsInt() > 0) {
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addr1W.AsInt()),
					VA("r_t%d_wrAddr1%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
			}
			if (pGv->m_addr2W.AsInt() > 0) {
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addr2W.AsInt()),
					VA("r_t%d_wrAddr2%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
			}

			GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, pGv->m_type,
				VA("r_t%d_wrData%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
			ngvRegDecl.NewLine();

			{
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvRegWrData.Append("\tr_t%d_wrEn%s%s = c_t%d_wrEn%s%s;\n",
						stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					if (pGv->m_addr1W.AsInt() > 0) {
						ngvRegWrData.Append("\tr_t%d_wrAddr1%s%s = c_t%d_wrAddr1%s%s;\n",
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					}
					if (pGv->m_addr2W.AsInt() > 0) {
						ngvRegWrData.Append("\tr_t%d_wrAddr2%s%s = c_t%d_wrAddr2%s%s;\n",
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					}

					ngvRegWrData.Append("\tr_t%d_wrData%s%s = c_t%d_wrData%s%s;\n",
						stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

				} while (DimenIter(pGv->m_dimenList, refList));
				ngvRegWrData.NewLine();
			}

			stgIdx += 1;

			ngvRegDecl.Append("\tbool c_t%d_wrEn%s%s;\n",
				stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

			if (pGv->m_addr1W.AsInt() > 0) {
				ngvRegDecl.Append("\tht_uint%d c_t%d_wrAddr1%s%s;\n", 
					pGv->m_addr1W.AsInt(), 
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
			}
			if (pGv->m_addr2W.AsInt() > 0) {
				ngvRegDecl.Append("\tht_uint%d c_t%d_wrAddr2%s%s;\n",
					pGv->m_addr2W.AsInt(),
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
			}

			ngvRegDecl.Append("\t%s c_t%d_wrData%s%s;\n",
				pGv->m_type.c_str(),
				stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
			ngvRegDecl.NewLine();

			{
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					ngvPreRegWrData.Append("\tc_t%d_wrEn%s%s = r_t%d_wrEn%s%s;\n",
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					if (pGv->m_addr1W.AsInt() > 0) {
						ngvPreRegWrData.Append("\tc_t%d_wrAddr1%s%s = r_t%d_wrAddr1%s%s;\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					}
					if (pGv->m_addr2W.AsInt() > 0) {
						ngvPreRegWrData.Append("\tc_t%d_wrAddr2%s%s = r_t%d_wrAddr2%s%s;\n",
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					}

					ngvPreRegWrData.Append("\tc_t%d_wrData%s%s = r_t%d_wrData%s%s;\n",
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

				} while (DimenIter(pGv->m_dimenList, refList));
				ngvPreRegWrData.NewLine();
			}
		}

		// write stage of RMW
		if (ngvFieldCnt > 1 || bNgvAtomic) {

			if (bNgvDist || bNgvBlock) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					if (pGv->m_addr2W.AsInt() == 0) {
						ngvPreRegWrData.Append("\tm_%s%s.write_addr(c_t%d_wrAddr1%s%s);\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					} else {
						ngvPreRegWrData.Append("\tm_%s%s.write_addr(c_t%d_wrAddr1%s%s, c_t%d_wrAddr2%s%s);\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					}
					
					ngvPreRegWrData.Append("\tif (c_t%d_wrEn%s%s)\n",
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					ngvPreRegWrData.Append("\t\tm_%s%s.write_mem(c_t%d_wrData%s%s);\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

				} while (DimenIter(pGv->m_dimenList, refList));
			}

			if (!bNeedRamWrReg) {

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
					VA("r_t%d_wrEn%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);

				if (pGv->m_addr1W.AsInt() > 0) {
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addr1W.AsInt()),
						VA("r_t%d_wrAddr1%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				}
				if (pGv->m_addr2W.AsInt() > 0) {
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addr2W.AsInt()),
						VA("r_t%d_wrAddr2%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				}

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, pGv->m_type,
					VA("r_t%d_wrData%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				ngvRegDecl.NewLine();

				{
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvRegWrData.Append("\tr_t%d_wrEn%s%s = c_t%d_wrEn%s%s;\n",
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					} while (DimenIter(pGv->m_dimenList, refList));
				}
				{
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						if (pGv->m_addr1W.AsInt() > 0) {
							ngvRegWrData.Append("\tr_t%d_wrAddr1%s%s = c_t%d_wrAddr1%s%s;\n",
								stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
						}
					} while (DimenIter(pGv->m_dimenList, refList));
				}
				{
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						if (pGv->m_addr2W.AsInt() > 0) {
							ngvRegWrData.Append("\tr_t%d_wrAddr2%s%s = c_t%d_wrAddr2%s%s;\n",
								stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
						}
					} while (DimenIter(pGv->m_dimenList, refList));
				}
				{
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						ngvRegWrData.Append("\tr_t%d_wrData%s%s = c_t%d_wrData%s%s;\n",
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
						ngvRegWrData.NewLine();

					} while (DimenIter(pGv->m_dimenList, refList));
				}

				stgIdx += 1;
			}
		}

		// Write data output
		{
			string varPrefix = (ngvPortList.size() == 1 && ngvFieldCnt == 1 && !bNgvAtomic) ? VA("c_t%d", stgIdx) : VA("r_t%d", stgIdx);

			for (size_t modIdx = 0; modIdx < ngvModInfoList.size(); modIdx += 1) {
				CModule & mod = *ngvModInfoList[modIdx].m_pMod;
				CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

				for (int imIdx = 0; imIdx < 2; imIdx += 1) {
					if (imIdx == 1 && !pModNgv->m_bMifRead) continue;
					//char imCh = imIdx == 0 ? 'i' : 'm';
					//bool bMaxGw = imIdx == 0 ? pModNgv->m_bMaxIw : pModNgv->m_bMaxMw;

					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);
						if (imIdx == 0) {
							ngvOut.Append("\to_%sTo%s_wrEn%s = %s_wrEn%s%s;\n",
								pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								varPrefix.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
							if (pGv->m_addr1W.AsInt() > 0)
								ngvOut.Append("\to_%sTo%s_wrAddr1%s = %s_wrAddr1%s%s;\n",
								pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								varPrefix.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
							if (pGv->m_addr2W.AsInt() > 0)
								ngvOut.Append("\to_%sTo%s_wrAddr2%s = %s_wrAddr2%s%s;\n",
								pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								varPrefix.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
							ngvOut.Append("\to_%sTo%s_wrData%s = %s_wrData%s%s;\n",
								pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								varPrefix.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
						}
						ngvOut.NewLine();

					} while (DimenIter(pGv->m_dimenList, refList));
				}
			}
		}

		GenerateBanner(incFile, incName.c_str(), false);

		fprintf(incFile, "#pragma once\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "#include \"PersCommon.h\"\n");
		fprintf(incFile, "\n");
		fprintf(incFile, "SC_MODULE(CPersGbl%s)\n", pGv->m_gblName.Uc().c_str());
		fprintf(incFile, "{\n");
		fprintf(incFile, "\tht_attrib(keep_hierarchy, CPersGbl%s, \"true\");\n", pGv->m_gblName.Uc().c_str());
		fprintf(incFile, "\n");
		fprintf(incFile, "\tsc_in<bool> i_clock1x;\n");
		if (bNeed2x)
			fprintf(incFile, "\tsc_in<bool> i_clock2x;\n");
		fprintf(incFile, "\tsc_in<bool> i_reset;\n");
		fprintf(incFile, "\n");

		ngvIo.Write(incFile);

		if (bNeed2x)
			ngvRegDecl.Append("\tbool ht_noload r_phase;\n");
		ngvRegDecl.Append("\tbool ht_noload r_reset1x;\n");
		if (bNeed2x)
			ngvRegDecl.Append("\tsc_signal<bool> ht_noload c_reset1x;\n");
		ngvRegDecl.NewLine();

		ngvRegDecl.Write(incFile);

		fprintf(incFile, "\tvoid PersGbl%s_1x();\n", pGv->m_gblName.Uc().c_str());
		if (bNeed2x)
			fprintf(incFile, "\tvoid PersGbl%s_2x();\n", pGv->m_gblName.Uc().c_str());
		fprintf(incFile, "\n");

		fprintf(incFile, "\tSC_CTOR(CPersGbl%s)\n", pGv->m_gblName.Uc().c_str());
		fprintf(incFile, "\t{\n");
		fprintf(incFile, "\t\tSC_METHOD(PersGbl%s_1x);\n", pGv->m_gblName.Uc().c_str());
		fprintf(incFile, "\t\tsensitive << i_clock1x.pos();\n");
		fprintf(incFile, "\n");

		if (bNeed2x) {
			fprintf(incFile, "\t\tSC_METHOD(PersGbl%s_2x);\n", pGv->m_gblName.Uc().c_str());
			fprintf(incFile, "\t\tsensitive << i_clock2x.pos();\n");
			fprintf(incFile, "\n");
		}

		if (bNeed2x) {
			fprintf(incFile, "#\t\tifndef _HTV\n");
			fprintf(incFile, "\t\tc_reset1x = true;\n");
			fprintf(incFile, "#\t\tendif\n");
		}
		fprintf(incFile, "\t}\n");

		fprintf(incFile, "#\tifndef _HTV\n");
		fprintf(incFile, "\tvoid start_of_simulation()\n");
		fprintf(incFile, "\t{\n");

		ngvSosCode.Write(incFile);

		fprintf(incFile, "\t}\n");
		fprintf(incFile, "#\tendif\n");

		fprintf(incFile, "};\n");

		incFile.FileClose();

		/////////////////////////////////////////////////////

		string cppName = VA("PersGbl%s.cpp", pGv->m_gblName.Uc().c_str());
		string cppPath = g_appArgs.GetOutputFolder() + "/" + cppName;

		CHtFile cppFile(cppPath, "w");

		GenerateBanner(cppFile, cppName.c_str(), false);

		fprintf(cppFile, "#include \"Ht.h\"\n");
		fprintf(cppFile, "#include \"PersGbl%s.h\"\n", pGv->m_gblName.Uc().c_str());
		fprintf(cppFile, "\n");

		fprintf(cppFile, "void CPersGbl%s::PersGbl%s_1x()\n", 
			pGv->m_gblName.Uc().c_str(), pGv->m_gblName.Uc().c_str());
		fprintf(cppFile, "{\n");

		ngvPreReg_1x.Write(cppFile);
		ngvReg_1x.Write(cppFile);

		fprintf(cppFile, "\tht_attrib(equivalent_register_removal, r_reset1x, \"no\");\n");
		fprintf(cppFile, "\tHtResetFlop(r_reset1x, i_reset.read());\n");
		if (bNeed2x)
			fprintf(cppFile, "\tc_reset1x = r_reset1x;\n");
		fprintf(cppFile, "\n");

		ngvOut_1x.Write(cppFile);

		fprintf(cppFile, "}\n");
		fprintf(cppFile, "\n");

		if (bNeed2x) {
			fprintf(cppFile, "void CPersGbl%s::PersGbl%s_2x()\n", pGv->m_gblName.Uc().c_str(), pGv->m_gblName.Uc().c_str());
			fprintf(cppFile, "{\n");
		}

		ngvPreReg_2x.Write(cppFile);
		ngvReg_2x.Write(cppFile);

		if (bNeed2x) {
			fprintf(cppFile, "\tht_attrib(equivalent_register_removal, r_phase, \"no\");\n");
			fprintf(cppFile, "\tr_phase = c_reset1x.read() || !r_phase;\n");
			fprintf(cppFile, "\n");
		}

		ngvOut_2x.Write(cppFile);

		if (bNeed2x)
			fprintf(cppFile, "}\n");

		cppFile.FileClose();
	}
}
