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

		if (!mod.m_bIsUsed || mod.m_ngvList.size() == 0) continue;

		// create unique list of global variables for entire unit
		for (size_t mgvIdx = 0; mgvIdx < mod.m_ngvList.size(); mgvIdx += 1) {
			CRam * pMgv = mod.m_ngvList[mgvIdx];
		
			// verify that ram name is unique in module
			for (size_t mgv2Idx = mgvIdx + 1; mgv2Idx < mod.m_ngvList.size(); mgv2Idx += 1) {
				CRam * pMgv2 = mod.m_ngvList[mgv2Idx];

				if (pMgv->m_gblName == pMgv2->m_gblName) {
					ParseMsg(Error, pMgv2->m_lineInfo, "global variable '%s' redeclared", pMgv->m_gblName.c_str());
					ParseMsg(Info, pMgv->m_lineInfo, "previous declaration");
				}
			}

			if (pMgv->m_addrW == 0) {
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

				m_ngvList[gvIdx]->m_modInfoList.push_back(CNgvModInfo(&mod, pMgv));
				pMgv->m_pNgvInfo = m_ngvList[gvIdx];

				if (pMgv->m_pNgvInfo->m_ngvReplCnt != (int)mod.m_modInstList.size())
					ParseMsg(Error, pMgv->m_lineInfo, "global variable '%s' accessed from modules with inconsistent replication count", pMgv->m_gblName.c_str());

			} else {
				// add new global variable to unit list
				m_ngvList.push_back(new CNgvInfo());
				m_ngvList.back()->m_modInfoList.push_back(CNgvModInfo(&mod, pMgv));
				m_ngvList.back()->m_ramType = pMgv->m_ramType;
				pMgv->m_pNgvInfo = m_ngvList.back();

				pMgv->m_pNgvInfo->m_ngvReplCnt = (int)mod.m_modInstList.size();
			}
		}
	}

	for (size_t gvIdx = 0; gvIdx < m_ngvList.size(); gvIdx += 1) {
		CNgvInfo * pNgvInfo = m_ngvList[gvIdx];
		CRam * pGv0 = pNgvInfo->m_modInfoList[0].m_pNgv;

		vector<CNgvModInfo> &ngvModInfoList = m_ngvList[gvIdx]->m_modInfoList;

		// create ngv port list of instruction and memory writes
		pNgvInfo->m_bAllWrPortClk1x = true;
		bool bAllRdPortClk1x = true;
		bool bNgvMaxSel = false;
		pNgvInfo->m_rdPortCnt = 0;
		pNgvInfo->m_ramType = eUnknownRam;
		pNgvInfo->m_pRdMod = 0;
		pNgvInfo->m_rdModCnt = 0;
		for (size_t modIdx = 0; modIdx < ngvModInfoList.size(); modIdx += 1) {
			CModule * pMod = ngvModInfoList[modIdx].m_pMod;
			CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

			for (int imIdx = 0; imIdx < 2; imIdx += 1) {
				if (imIdx == 0 && !pModNgv->m_bWriteForInstrWrite) continue;
				if (imIdx == 1 && !pModNgv->m_bWriteForMifRead) continue;

				pNgvInfo->m_wrPortList.push_back(pair<int, int>(modIdx, imIdx));
				pNgvInfo->m_bAllWrPortClk1x &= pMod->m_clkRate == eClk1x;
			}

			for (int imIdx = 0; imIdx < 2; imIdx += 1) {
				if (imIdx == 0 && !pModNgv->m_bReadForInstrRead) continue;
				if (imIdx == 1 && !pModNgv->m_bReadForMifWrite) continue;

				pNgvInfo->m_rdPortCnt += 1;
				bAllRdPortClk1x &= pMod->m_clkRate == eClk1x;

				if (pMod != pNgvInfo->m_pRdMod) {
					pNgvInfo->m_pRdMod = pMod;
					pNgvInfo->m_rdModCnt += 1;
				}
			}

			bNgvMaxSel |= pModNgv->m_bMaxIw || pModNgv->m_bMaxMw;

			if (pNgvInfo->m_ramType == eUnknownRam)
				pNgvInfo->m_ramType = pModNgv->m_ramType;
			else if (pNgvInfo->m_ramType != pModNgv->m_ramType)
				pNgvInfo->m_ramType = eAutoRam;
		}

		// determine if a single field in ngv
		int ngvFieldCnt = 0;
		bool bNgvAtomicFast = false;
		bool bNgvAtomicSlow = false;
		for (CStructElemIter iter(this, pGv0->m_pType); !iter.end(); iter++) {
			if (iter.IsStructOrUnion()) continue;

			ngvFieldCnt += 1;
			if (pGv0->m_pType->m_eType != eRecord) continue;

			bNgvAtomicFast |= (iter->m_atomicMask & (ATOMIC_INC | ATOMIC_SET)) != 0;
			bNgvAtomicSlow |= (iter->m_atomicMask & ATOMIC_ADD) != 0;
		}
		bool bNgvAtomic = bNgvAtomicFast || bNgvAtomicSlow;

		pNgvInfo->m_ngvFieldCnt = ngvFieldCnt;
		pNgvInfo->m_bNgvAtomicFast = bNgvAtomicFast;
		pNgvInfo->m_bNgvAtomicSlow = bNgvAtomicSlow;

		// determine type of ram
		bool bNgvReg = pGv0->m_addrW == 0;
		bool bNgvDist = !bNgvReg && pGv0->m_pNgvInfo->m_ramType != eBlockRam;
		bool bNgvBlock = !bNgvReg && !bNgvDist;

		int ngvPortCnt = (int)pNgvInfo->m_wrPortList.size();

		bNgvMaxSel &= bNgvDist && (!pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt > 1) && bNgvAtomicSlow ||
			bNgvBlock && (bNgvAtomic || ngvFieldCnt > 1);

		pNgvInfo->m_bNgvMaxSel = bNgvMaxSel;

		pNgvInfo->m_bNgvWrCompClk2x = bNgvReg && (!pNgvInfo->m_bAllWrPortClk1x && ngvPortCnt <= 2 || ngvPortCnt == 3) ||
			bNgvDist && ((!pNgvInfo->m_bAllWrPortClk1x && ngvPortCnt <= 2 || ngvPortCnt == 3) && !bNgvAtomicSlow ||
			(bNgvAtomicSlow && (!pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt > 1) && bNgvMaxSel)) ||
			bNgvBlock && ((!bNgvAtomic && ngvFieldCnt == 1) && (!pNgvInfo->m_bAllWrPortClk1x && ngvPortCnt <= 2 || ngvPortCnt == 3) ||
			(bNgvAtomic || ngvFieldCnt > 1) && bNgvMaxSel);

		pNgvInfo->m_bNgvWrDataClk2x = bNgvReg && (!pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt > 1) ||
			bNgvDist && ((!pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt > 1) && !bNgvAtomicSlow ||
			(bNgvAtomicSlow && (!pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt > 1) && bNgvMaxSel)) ||
			bNgvBlock && ((!bNgvAtomic && ngvFieldCnt == 1) && (!pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt > 1) ||
			(bNgvAtomic || ngvFieldCnt > 1) && bNgvMaxSel);

		pNgvInfo->m_bNeedQue = bNgvReg && (ngvPortCnt == 2 && !pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt >= 3) ||
			bNgvDist && ((ngvPortCnt == 2 && !pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt >= 3) && !bNgvAtomicSlow ||
			((ngvPortCnt >= 2 || !pNgvInfo->m_bAllWrPortClk1x) && bNgvAtomicSlow)) ||
			bNgvBlock && ((!bNgvAtomic && ngvFieldCnt == 1) && (ngvPortCnt == 2 && !pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt >= 3) ||
			(bNgvAtomic || ngvFieldCnt > 1));

		// 2x RR selection - one level, 2 or 3 ports, 2x wrData
		bool bRrSel2x = bNgvReg && (ngvPortCnt == 2 && !pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt == 3) ||
			bNgvDist && (ngvPortCnt == 2 && !pNgvInfo->m_bAllWrPortClk1x && !bNgvAtomicSlow || ngvPortCnt == 3 && !bNgvAtomicSlow) ||
			bNgvBlock && (ngvPortCnt == 2 && !pNgvInfo->m_bAllWrPortClk1x && !bNgvAtomic && ngvFieldCnt == 1
			|| ngvPortCnt == 3 && !bNgvAtomic && ngvFieldCnt == 1);

		// 1x RR selection - no phase select, 1x wrData
		bool bRrSel1x = bNgvDist && ngvPortCnt >= 2 && bNgvAtomicSlow && !pNgvInfo->m_bNgvMaxSel ||
			bNgvBlock && ngvPortCnt >= 2 && (bNgvAtomic || ngvFieldCnt >= 2) && !pNgvInfo->m_bNgvMaxSel;

		// 1x RR selection, ports split using phase, 2x wrData
		bool bRrSelAB = bNgvReg && ngvPortCnt >= 4 ||
			bNgvDist && (ngvPortCnt >= 4 && !bNgvAtomicSlow) ||
			bNgvBlock && (ngvPortCnt >= 4 && !bNgvAtomic && ngvFieldCnt == 1);

		bool bRrSelEO = bNgvDist && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel && (!pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt >= 2) ||
			bNgvBlock && (bNgvAtomic || ngvFieldCnt > 1) && pNgvInfo->m_bNgvMaxSel;

		bool bNeedAddrComp = bNgvDist && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel && (!pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt >= 2) ||
			bNgvBlock && (bNgvAtomic || ngvFieldCnt > 1);

		bool bNeedRamReg = bNgvDist && bNgvAtomicSlow && (ngvPortCnt >= 2 || !pNgvInfo->m_bAllWrPortClk1x) && pNgvInfo->m_bNgvMaxSel ||
			bNgvBlock && (bNgvAtomic || ngvFieldCnt > 1) && pNgvInfo->m_bNgvMaxSel;

		bool bNeedRamWrReg = bNgvDist && bNgvAtomicSlow && (ngvPortCnt >= 2 || !pNgvInfo->m_bAllWrPortClk1x) && pNgvInfo->m_bNgvMaxSel ||
			bNgvBlock && (bNgvAtomic || ngvFieldCnt > 1) && pNgvInfo->m_bNgvMaxSel;

		int stgIdx = 0;
		pNgvInfo->m_wrCompStg = -1;
		if (bRrSel1x) {
			stgIdx += 1;
			pNgvInfo->m_wrCompStg = 1;
		}
		else if (bRrSel2x) {
			if ((ngvFieldCnt != 1 || bNgvAtomic) && !bNgvReg) stgIdx += 1;
			pNgvInfo->m_wrCompStg = 1;
		}
		else if (bRrSelAB) {
			stgIdx += 1;
			pNgvInfo->m_wrCompStg = 1;
		}
		else if (bRrSelEO) {
			if (ngvPortCnt != 1) stgIdx += 1;
			stgIdx += 1;
			pNgvInfo->m_wrCompStg = stgIdx;
		}
		else if (ngvPortCnt == 1 && ngvFieldCnt == 1 && !bNgvAtomic) {
		}
		else if (ngvPortCnt == 1 && (ngvFieldCnt > 1 || bNgvAtomic)) {
			if (bNeedAddrComp)
				pNgvInfo->m_wrCompStg = stgIdx + 1;
		}
		else if (ngvPortCnt == 2 && pNgvInfo->m_bAllWrPortClk1x) {
			if (bNeedAddrComp)
				pNgvInfo->m_wrCompStg = stgIdx + 1;
		}

		if (!(ngvFieldCnt > 1 || bNgvAtomic) && ngvPortCnt > 1) stgIdx += 1;

		if (ngvFieldCnt > 1 || bNgvAtomic) {
			if (bNgvBlock) stgIdx += 1;
			if (bNeedRamReg) stgIdx += 1;
		}
		if (bNeedRamWrReg) stgIdx += 1;
		if ((ngvFieldCnt > 1 || bNgvAtomic) && !bNeedRamWrReg) stgIdx += 1;

		pNgvInfo->m_wrDataStg = stgIdx;

		// determine if this ram has an optimized implementation
		int modCnt = pNgvInfo->m_modInfoList.size();

		pNgvInfo->m_bOgv = (pNgvInfo->m_rdModCnt == 1 && !bNgvAtomic && bNgvReg) ||
			(ngvFieldCnt == 1 && !bNgvAtomic && (pNgvInfo->m_wrPortList.size() == 1 || pNgvInfo->m_bAllWrPortClk1x && pNgvInfo->m_wrPortList.size() == 2) &&
			pNgvInfo->m_rdModCnt == 1 && (pNgvInfo->m_rdPortCnt == 1 || pNgvInfo->m_rdPortCnt == 2 && bAllRdPortClk1x));

#ifdef WIN32
		bool bPossibleOgv = !pNgvInfo->m_bOgv && ngvFieldCnt == 1 && !bNgvAtomic &&
			(pNgvInfo->m_bAllWrPortClk1x && pNgvInfo->m_wrPortList.size() <= 2 || pNgvInfo->m_wrPortList.size() == 1) &&
			pNgvInfo->m_rdModCnt == 1 && (bAllRdPortClk1x || pNgvInfo->m_rdPortCnt == 1);

		if (!pNgvInfo->m_bOgv && ngvFieldCnt == 1 && !bNgvAtomic)
		{
			printf("** %s %s\n", pGv0->m_gblName.c_str(), bPossibleOgv ? "yes" : "no");

			for (size_t modIdx = 0; modIdx < ngvModInfoList.size(); modIdx += 1) {
				CModule * pMod = ngvModInfoList[modIdx].m_pMod;
				CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

				for (int imIdx = 0; imIdx < 2; imIdx += 1) {
					if (imIdx == 0 && !pModNgv->m_bWriteForInstrWrite) continue;
					if (imIdx == 1 && !pModNgv->m_bWriteForMifRead) continue;

					printf("**   Wr Mod %s, %s\n", pMod->m_modName.c_str(), pMod->m_clkRate == eClk1x ? "1x" : "2x");
				}

				for (int imIdx = 0; imIdx < 2; imIdx += 1) {
					if (imIdx == 0 && !pModNgv->m_bReadForInstrRead) continue;
					if (imIdx == 1 && !pModNgv->m_bReadForMifWrite) continue;

					printf("**   Rd Mod %s, %s\n", pMod->m_modName.c_str(), pMod->m_clkRate == eClk1x ? "1x" : "2x");
				}
			}
		}
#endif
	}

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed || mod.m_ngvList.size() == 0) continue;

		mod.m_bGvIwComp = false;
		mod.m_gvIwCompStg = max(mod.m_stage.m_execStg.AsInt(), mod.m_stage.m_privWrStg.AsInt());

		// create unique list of global variables for entire unit
		for (size_t mgvIdx = 0; mgvIdx < mod.m_ngvList.size(); mgvIdx += 1) {
			CRam * pMgv = mod.m_ngvList[mgvIdx];

			int lastStg = 1;
			if (pMgv->m_wrStg.size() > 0)
				lastStg = pMgv->m_wrStg.AsInt();
			else {
				lastStg = mod.m_stage.m_execStg.AsInt();
				pMgv->m_wrStg.SetValue(lastStg);
			}

			if (pMgv->m_pNgvInfo->m_bOgv) continue;

			mod.m_bGvIwComp |= pMgv->m_bWriteForInstrWrite;

			mod.m_gvIwCompStg = max(mod.m_gvIwCompStg, lastStg);
		}
	}
}

void CDsnInfo::GenModOptNgvStatements(CModule * pMod, CRam * pGv)
{
	CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;

	CHtCode & gblPreInstr = pMod->m_clkRate == eClk2x ? m_gblPreInstr2x : m_gblPreInstr1x;
	CHtCode & gblPostInstr = pMod->m_clkRate == eClk2x ? m_gblPostInstr2x : m_gblPostInstr1x;
	CHtCode & gblReg = pMod->m_clkRate == eClk2x ? m_gblReg2x : m_gblReg1x;
	CHtCode & gblOut = pMod->m_clkRate == eClk2x ? m_gblOut2x : m_gblOut1x;

	string gblRegReset = pMod->m_clkRate == eClk2x ? "c_reset1x" : "r_reset1x";

	string vcdModName = VA("Pers%s", pMod->m_modName.Uc().c_str());

	// find read module
	CModule * pRdMod = 0;
	for (size_t i = 0; i < pNgvInfo->m_modInfoList.size(); i += 1) {
		CNgvModInfo * pInfo = & pNgvInfo->m_modInfoList[i];
		if (pInfo->m_pNgv->m_bReadForInstrRead || pInfo->m_pNgv->m_bReadForMifWrite) {
			pRdMod = pInfo->m_pMod;
			break;
		}
	}
	HtlAssert(pRdMod != 0);

	bool bFirstModVar = false;
	int stgLast = max(pGv->m_wrStg.AsInt(), pMod->m_stage.m_privWrStg.AsInt());
	for (int stgIdx = 1; stgIdx <= stgLast; stgIdx += 1) {

		string varStg;
		if (pMod->m_stage.m_bStageNums)
			varStg = VA("%d", stgIdx);

		if (stgIdx >= pGv->m_rdStg.AsInt()) {
			if (!pGv->m_bPrivGbl && pGv->m_bReadForInstrRead) {
				if (pNgvInfo->m_atomicMask != 0) {
					GenModVar(eVcdUser, vcdModName, bFirstModVar,
						VA("%s const", pGv->m_type.c_str()),
						pGv->m_dimenDecl,
						VA("GF%s_%s", varStg.c_str(), pGv->m_gblName.c_str()),
						VA("r_t%d_%sIfData", pMod->m_tsStg + stgIdx - 1, pGv->m_gblName.c_str()),
						pGv->m_dimenList);
				}

				if (pGv->m_addrW == 0 && stgIdx == pGv->m_rdStg.AsInt()) {
					GenModVar(eVcdUser, vcdModName, bFirstModVar,
						VA("%s const", pGv->m_type.c_str()),
						pGv->m_dimenDecl,
						VA("GR%s_%s", varStg.c_str(), pGv->m_gblName.c_str()),
						VA("r__GBL__%s", pGv->m_gblName.c_str()),
						pGv->m_dimenList);
				} else {
					GenModVar(eVcdUser, vcdModName, bFirstModVar,
						VA("%s const", pGv->m_type.c_str()),
						pGv->m_dimenDecl,
						VA("GR%s_%s", varStg.c_str(), pGv->m_gblName.c_str()),
						VA("r_t%d_%sIrData", pMod->m_tsStg + stgIdx - 1, pGv->m_gblName.c_str()),
						pGv->m_dimenList);
				}
			}
		}

		if (stgIdx <= pGv->m_wrStg.AsInt()) {
			if (!pGv->m_bPrivGbl && pGv->m_bWriteForInstrWrite) {
				GenModVar(eVcdNone, vcdModName, bFirstModVar,
					VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
					pGv->m_dimenDecl,
					VA("GW%s_%s", varStg.c_str(), pGv->m_gblName.c_str()),
					VA("c_t%d_%sIwData", pMod->m_tsStg + stgIdx - 1, pGv->m_gblName.c_str()),
					pGv->m_dimenList);
			}
		}
	}

	bool bPrivGblAndNoAddr = pGv->m_bPrivGbl && pGv->m_addrW == pMod->m_threads.m_htIdW.AsInt();

	if (pGv->m_bWriteForInstrWrite) {
		for (int gvWrStg = pMod->m_tsStg; gvWrStg < pMod->m_tsStg + pGv->m_wrStg.AsInt(); gvWrStg += 1) {

			string typeName;
			if (gvWrStg == pMod->m_tsStg + pGv->m_wrStg.AsInt() - 1 &&
				pNgvInfo->m_wrPortList.size() > 1 && pGv->m_addrW > 0 && pGv->m_pNgvInfo->m_pRdMod == pMod)
			{
				typeName = VA("sc_signal<CGW_%s>", pGv->m_pNgvInfo->m_ngvWrType.c_str());
			} else
				typeName = VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str());

			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, typeName,
				VA("r_t%d_%sIwData", gvWrStg + 1, pGv->m_gblName.c_str()), pGv->m_dimenList);
			m_gblRegDecl.Append("\tCGW_%s c_t%d_%sIwData%s;\n",
				pGv->m_pNgvInfo->m_ngvWrType.c_str(), gvWrStg, pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
		}

		string htIdStr = (pMod->m_threads.m_htIdW.AsInt() > 0 &&
			(pGv->m_bPrivGbl || pGv->m_addr1Name == "htId" || pGv->m_addr2Name == "htId")) ? VA("r_t%d_htId", pMod->m_tsStg) : "";

		vector<int> refList(pGv->m_dimenList.size());
		do {
			string dimIdx = IndexStr(refList);

			for (int gvWrStg = pMod->m_tsStg; gvWrStg < pMod->m_tsStg + pGv->m_wrStg.AsInt(); gvWrStg += 1) {
				if (bPrivGblAndNoAddr && gvWrStg == pMod->m_tsStg) {
					if (pMod->m_threads.m_htIdW.AsInt() == 0) {
						gblPreInstr.Append("\tc_t%d_%sIwData%s.InitData(%s%sr__GBL__%s%s);\n",
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
							htIdStr.c_str(), htIdStr.size() > 0 ? ", " : "",
							pGv->m_gblName.c_str(), dimIdx.c_str());
					} else {
						gblPreInstr.Append("\tc_t%d_%sIwData%s.InitData(%s%sr_t%d_%sIrData%s);\n",
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
							htIdStr.c_str(), htIdStr.size() > 0 ? ", " : "",
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
					}
				} else if (!bPrivGblAndNoAddr && gvWrStg == pMod->m_tsStg) {
					gblPreInstr.Append("\tc_t%d_%sIwData%s.InitZero(%s);\n",
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
						htIdStr.c_str());
				} else {
					gblPreInstr.Append("\tc_t%d_%sIwData%s = r_t%d_%sIwData%s;\n",
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
				}
				gblReg.Append("\tr_t%d_%sIwData%s = c_t%d_%sIwData%s;\n",
					gvWrStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
					gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
			}
		} while (DimenIter(pGv->m_dimenList, refList));
	}

	if (pGv->m_bWriteForMifRead) {
		int rdRspStg = pMod->m_mif.m_mifRd.m_bNeedRdRspInfoRam ? 2 : 1;

		string typeName;
		if (pGv->m_bWriteForMifRead && pGv->m_bWriteForInstrWrite && pGv->m_addrW > 0 && pGv->m_pNgvInfo->m_pRdMod == pMod)
			typeName = VA("sc_signal<CGW_%s>", pGv->m_pNgvInfo->m_ngvWrType.c_str());
		else
			typeName = VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str());

		GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, typeName,
			VA("r_m%d_%sMwData", rdRspStg + 1, pGv->m_gblName.c_str()), pGv->m_dimenList);
		m_gblRegDecl.Append("\tCGW_%s c_m%d_%sMwData%s;\n",
			pGv->m_pNgvInfo->m_ngvWrType.c_str(),
			rdRspStg, pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

		string htIdStr = (pMod->m_threads.m_htIdW.AsInt() > 0 &&
			(pGv->m_bPrivGbl || pGv->m_addr1Name == "htId" || pGv->m_addr2Name == "htId")) ? VA("r_m%d_rdRspInfo.m_htId", rdRspStg) : "";

		vector<int> refList(pGv->m_dimenList.size());
		do {
			string dimIdx = IndexStr(refList);

			gblPreInstr.Append("\tc_m%d_%sMwData%s.InitZero(%s);\n",
				rdRspStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
				htIdStr.c_str());

			gblReg.Append("\tr_m%d_%sMwData%s = c_m%d_%sMwData%s;\n",
				rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
				rdRspStg, pGv->m_gblName.c_str(), dimIdx.c_str());

		} while (DimenIter(pGv->m_dimenList, refList));
	}

	// write outputs
	if (!pGv->m_bReadForInstrRead && !pGv->m_bReadForMifWrite) {
		if (pGv->m_bWriteForInstrWrite) {
			int gvWrStg = pMod->m_tsStg + pGv->m_wrStg.AsInt();

			{
				m_gblIoDecl.Append("\tsc_out<CGW_%s> o_%sTo%s_%sIwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(),
					pMod->m_modName.Lc().c_str(), pRdMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblOut.Append("\to_%sTo%s_%sIwData%s = r_t%d_%sIwData%s;\n",
						pMod->m_modName.Lc().c_str(), pRdMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(),
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}

		if (pGv->m_bWriteForMifRead) { // Memory
			int rdRspStg = pMod->m_mif.m_mifRd.m_bNeedRdRspInfoRam ? 2 : 1;

			{
				m_gblIoDecl.Append("\tsc_out<CGW_%s> o_%sTo%s_%sMwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(),
					pMod->m_modName.Lc().c_str(), pRdMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblOut.Append("\to_%sTo%s_%sMwData%s = r_m%d_%sMwData%s;\n",
						pMod->m_modName.Lc().c_str(), pRdMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(),
						rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}

		return;
	}

	if (pGv->m_addrW == 0) {
		m_gblRegDecl.Append("\t%s c__GBL__%s%s;\n",
			pGv->m_type.c_str(), pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

		GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, pGv->m_type.c_str(),
			VA("r__GBL__%s", pGv->m_gblName.c_str()), pGv->m_dimenList);

		vector<int> refList(pGv->m_dimenList.size());
		do {
			string dimIdx = IndexStr(refList);

			gblPostInstr.Append("\tc__GBL__%s%s = r__GBL__%s%s;\n",
				pGv->m_gblName.c_str(), dimIdx.c_str(),
				pGv->m_gblName.c_str(), dimIdx.c_str());

			gblReg.Append("\tr__GBL__%s%s = c__GBL__%s%s;\n",
				pGv->m_gblName.c_str(), dimIdx.c_str(),
				pGv->m_gblName.c_str(), dimIdx.c_str());

		} while (DimenIter(pGv->m_dimenList, refList));

	} else {
		for (int imIdx = 0; imIdx < 2; imIdx += 1) {
			if (imIdx == 0 && !pGv->m_bReadForInstrRead) continue;
			if (imIdx == 1 && !pGv->m_bReadForMifWrite) continue;
			char const * pImStr = imIdx == 0 ? "Ir" : "Mr";

			m_gblRegDecl.Append("\tht_%s_ram<%s, %d",
				pGv->m_ramType == eBlockRam ? "block" : "dist",
				pGv->m_type.c_str(), pGv->m_addrW);
			m_gblRegDecl.Append("> m__GBL__%s%s%s;\n", pGv->m_gblName.c_str(), pImStr, pGv->m_dimenDecl.c_str());

			EClkRate rdClk = pMod->m_clkRate;
			EClkRate wrClk = pNgvInfo->m_wrPortList.size() == 1 && pNgvInfo->m_bAllWrPortClk1x ? eClk1x : eClk2x;

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);

				if (rdClk == wrClk) {

					gblReg.Append("\tm__GBL__%s%s%s.clock(%s);\n",
						pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
						gblRegReset.c_str());

				} else if (rdClk == eClk1x) {

					m_gblReg1x.Append("\tm__GBL__%s%s%s.read_clock(r_reset1x);\n",
						pGv->m_gblName.c_str(), pImStr, dimIdx.c_str());
					m_gblReg2x.Append("\tm__GBL__%s%s%s.write_clock(c_reset1x);\n",
						pGv->m_gblName.c_str(), pImStr, dimIdx.c_str());

				} else {

					m_gblReg2x.Append("\tm__GBL__%s%s%s.read_clock(c_reset1x);\n",
						pGv->m_gblName.c_str(), pImStr, dimIdx.c_str());

					m_gblReg1x.Append("\tm__GBL__%s%s%s.write_clock(r_reset1x);\n",
						pGv->m_gblName.c_str(), pImStr, dimIdx.c_str());
				}
			} while (DimenIter(pGv->m_dimenList, refList));
		}
		m_gblReg1x.NewLine();
		m_gblReg2x.NewLine();
	}

	if (pNgvInfo->m_modInfoList.size() > 1) {
		for (size_t i = 0; i < pNgvInfo->m_modInfoList.size(); i += 1) {
			CNgvModInfo * pModInfo = &pNgvInfo->m_modInfoList[i];
			if (pModInfo->m_pNgv->m_bWriteForInstrWrite && pModInfo->m_pMod != pMod) {
				m_gblIoDecl.Append("\tsc_in<CGW_%s> i_%sTo%s_%sIwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(),
					pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

				if (pGv->m_addrW == 0) {
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
							gblPostInstr.Append("\tif (i_%sTo%s_%sIwData%s.read()%s.GetWrEn())\n",
								pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
							gblPostInstr.Append("\t\tc__GBL__%s%s%s = i_%sTo%s_%sIwData%s.read()%s.GetData();\n",
								pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
								pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
						}
						gblPostInstr.NewLine();

					} while (DimenIter(pGv->m_dimenList, refList));

				} else if (pNgvInfo->m_wrPortList.size() == 1) {

					bool bWrClk1x = pNgvInfo->m_wrPortList.size() == 1 && pNgvInfo->m_bAllWrPortClk1x;
					CHtCode & wrCode = bWrClk1x ? m_gblPostInstr1x : m_gblPostInstr2x;

					for (int imIdx = 0; imIdx < 2; imIdx += 1) {
						if (imIdx == 0 && !pGv->m_bReadForInstrRead) continue;
						if (imIdx == 1 && !pGv->m_bReadForMifWrite) continue;
						char const * pImStr = imIdx == 0 ? "Ir" : "Mr";

						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
								wrCode.Append("\tm__GBL__%s%s%s.write_addr(i_%sTo%s_%sIwData%s.read().GetAddr());\n",
									pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
									pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str());
								wrCode.Append("\tif (i_%sTo%s_%sIwData%s.read()%s.GetWrEn())\n",
									pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
								wrCode.Append("\t\tm__GBL__%s%s%s.write_mem(i_%sTo%s_%sIwData%s.read().GetData());\n",
									pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
									pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str());
							}
							wrCode.NewLine();

						} while (DimenIter(pGv->m_dimenList, refList));
					}
				}
			}

			if (pModInfo->m_pNgv->m_bWriteForMifRead && pModInfo->m_pMod != pMod) { // Memory
				m_gblIoDecl.Append("\tsc_in<CGW_%s> i_%sTo%s_%sMwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(),
					pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

				if (pGv->m_addrW == 0) {
					vector<int> refList(pGv->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
							gblPostInstr.Append("\tif (i_%sTo%s_%sMwData%s.read()%s.GetWrEn())\n",
								pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
							gblPostInstr.Append("\t\tc__GBL__%s%s%s = i_%sTo%s_%sMwData%s.read()%s.GetData();\n",
								pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
								pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
						}
						gblPostInstr.NewLine();

					} while (DimenIter(pGv->m_dimenList, refList));

				} else if (pNgvInfo->m_wrPortList.size() == 1) {

					bool bWrClk1x = pNgvInfo->m_wrPortList.size() == 1 && pNgvInfo->m_bAllWrPortClk1x;
					CHtCode & wrCode = bWrClk1x ? m_gblPostInstr1x : m_gblPostInstr2x;

					for (int imIdx = 0; imIdx < 2; imIdx += 1) {
						if (imIdx == 0 && !pGv->m_bReadForInstrRead) continue;
						if (imIdx == 1 && !pGv->m_bReadForMifWrite) continue;
						char const * pImStr = imIdx == 0 ? "Ir" : "Mr";

						vector<int> refList(pGv->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
								wrCode.Append("\tm__GBL__%s%s%s.write_addr(i_%sTo%s_%sMwData%s.read().GetAddr());\n",
									pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
									pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str());
								wrCode.Append("\tif (i_%sTo%s_%sMwData%s.read()%s.GetWrEn())\n",
									pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
								wrCode.Append("\t\tm__GBL__%s%s%s.write_mem(i_%sTo%s_%sMwData%s.read().GetData());\n",
									pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
									pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str());
							}
							gblPostInstr.NewLine();

						} while (DimenIter(pGv->m_dimenList, refList));
					}
				}
			}
		}
	}

	if (pGv->m_bReadForInstrRead) {
		int addr0Skip = pGv->m_addrW == 0 ? 1 : 0;
		int lastStg = pGv->m_bPrivGbl ? pMod->m_stage.m_privWrStg.AsInt() : max(pGv->m_wrStg.AsInt(), pMod->m_stage.m_privWrStg.AsInt());
		for (int gvRdStg = pGv->m_rdStg.AsInt() + addr0Skip; gvRdStg <= lastStg; gvRdStg += 1) {
			m_gblRegDecl.Append("\t%s c_t%d_%sIrData%s;\n",
				pGv->m_type.c_str(), pMod->m_tsStg + gvRdStg - 2, pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("%s ht_noload", pGv->m_type.c_str()),
				VA("r_t%d_%sIrData", pMod->m_tsStg + gvRdStg - 1, pGv->m_gblName.c_str()), pGv->m_dimenList);
		}
		vector<int> refList(pGv->m_dimenList.size());
		do {
			for (int gvRdStg = pGv->m_rdStg.AsInt() + addr0Skip; gvRdStg <= lastStg; gvRdStg += 1) {
				string dimIdx = IndexStr(refList);

				if (gvRdStg > pGv->m_rdStg.AsInt() + addr0Skip) {
					gblPostInstr.Append("\tc_t%d_%sIrData%s = r_t%d_%sIrData%s;\n",
						pMod->m_tsStg + gvRdStg - 2, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pMod->m_tsStg + gvRdStg - 2, pGv->m_gblName.c_str(), dimIdx.c_str());
				}

				gblReg.Append("\tr_t%d_%sIrData%s = c_t%d_%sIrData%s;\n",
					pMod->m_tsStg + gvRdStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
					pMod->m_tsStg + gvRdStg - 2, pGv->m_gblName.c_str(), dimIdx.c_str());
			}
		} while (DimenIter(pGv->m_dimenList, refList));
	}

	if (pNgvInfo->m_wrPortList.size() > 1 && pGv->m_addrW > 0) {

		m_gblRegDecl.Append("\tCGW_%s c__GBL__%sPwData%s;\n",
			pGv->m_pNgvInfo->m_ngvWrType.c_str(), pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
		GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
			VA("r__GBL__%sPwData", pGv->m_gblName.c_str()), pGv->m_dimenList);
	}

	gblPostInstr.NewLine();

	if (pGv->m_bReadForInstrRead) {
		int lastStg = pGv->m_bPrivGbl ? pMod->m_stage.m_privWrStg.AsInt() : max(pGv->m_wrStg.AsInt(), pMod->m_stage.m_privWrStg.AsInt());
		if (pGv->m_addrW == 0) {
			if (pGv->m_rdStg.AsInt() < lastStg) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblPostInstr.Append("\tc_t%d_%sIrData%s = r__GBL__%s%s;\n",
						pMod->m_tsStg + pGv->m_rdStg.AsInt() - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}
		} else {
			int rdAddrStgNum = pMod->m_tsStg + pGv->m_rdStg.AsInt() - (pGv->m_ramType == eBlockRam ? 3 : 2);
			string rdAddrStg = rdAddrStgNum == 1 ? "c_t1" : VA("r_t%d", rdAddrStgNum);

			string addr1Name;
			if (pGv->m_addr1IsHtId)
				addr1Name = VA("%s_htId", rdAddrStg.c_str());
			else if (pGv->m_addr1IsPrivate)
				addr1Name = VA("%s_htPriv.m_%s", rdAddrStg.c_str(), pGv->m_addr1Name.c_str());
			else if (pGv->m_addr1IsStage)
				addr1Name = VA("%s__STG__%s", rdAddrStg.c_str(), pGv->m_addr1Name.c_str());
			else if (pGv->m_addr1W.AsInt() > 0)
				HtlAssert(0);

			string addr2Name;
			if (pGv->m_addr2IsHtId)
				addr2Name = VA("%s_htId", rdAddrStg.c_str());
			else if (pGv->m_addr2IsPrivate)
				addr2Name = VA("%s_htPriv.m_%s", rdAddrStg.c_str(), pGv->m_addr2Name.c_str());
			else if (pGv->m_addr1IsStage)
				addr2Name = VA("%s__STG__%s", rdAddrStg.c_str(), pGv->m_addr2Name.c_str());
			else if (pGv->m_addr2W.AsInt() > 0)
				HtlAssert(0);

			bool bNeedParan = ((pGv->m_addr0W.AsInt() > 0 ? 1 : 0) + (pGv->m_addr1W.AsInt() > 0 ? 1 : 0) + (pGv->m_addr2W.AsInt() > 0 ? 1 : 0)) > 1;
			gblPostInstr.Append("\tht_uint%d c_t%d_%sRdAddr = %s", pGv->m_addrW, rdAddrStgNum, pGv->m_gblName.Lc().c_str(), bNeedParan ? "(" : "");
			if (pGv->m_addr0W.AsInt() > 0)
				gblPostInstr.Append("%s_htId", rdAddrStg.c_str());
			if (pGv->m_addr0W.AsInt() > 0 && pGv->m_addr1W.AsInt() > 0)
				gblPostInstr.Append(", ");
			if (pGv->m_addr1W.AsInt() > 0)
				gblPostInstr.Append("%s", addr1Name.c_str());
			if (pGv->m_addr2W.AsInt() > 0)
				gblPostInstr.Append(", %s", addr2Name.c_str());
			gblPostInstr.Append("%s;\n", bNeedParan ? ")" : "");

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);

				gblPostInstr.Append("\tm__GBL__%sIr%s.read_addr(c_t%d_%sRdAddr);\n",
					pGv->m_gblName.c_str(), dimIdx.c_str(),
					rdAddrStgNum, pGv->m_gblName.Lc().c_str());

				bool bNgvReg = pGv->m_addrW == 0;
				bool bNgvDist = !bNgvReg && pGv->m_ramType != eBlockRam;

				if (bNgvDist || pNgvInfo->m_ngvFieldCnt == 1) {

					gblPostInstr.Append("\tc_t%d_%sIrData%s = m__GBL__%sIr%s.read_mem();\n",
						rdAddrStgNum + (bNgvDist ? 0 : 1), pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				} else {

					gblPostInstr.Append("\tif (r_g1_%sTo%s_wrEn%s && r_g1_%sTo%s_wrAddr%s == r_t%d_%sRdAddr)\n",
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
						rdAddrStgNum + 1, pGv->m_gblName.Lc().c_str());

					gblPostInstr.Append("\t\tc_t%d_%sIrData%s = r_g1_%sTo%s_wrData%s;\n",
						rdAddrStgNum + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());

					gblPostInstr.Append("\telse\n");

					gblPostInstr.Append("\t\tc_t%d_%sIrData%s = m__GBL__%sIr%s.read_mem();\n",
						rdAddrStgNum + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				}
			} while (DimenIter(pGv->m_dimenList, refList));
		}
		gblPostInstr.NewLine();
	}

	if (pGv->m_bWriteForInstrWrite) {
		int gvWrStg = pMod->m_tsStg + pGv->m_wrStg.AsInt();

		if (pGv->m_addrW == 0) {
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);

				for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
					gblPostInstr.Append("\tif (r_t%d_%sIwData%s%s.GetWrEn())\n",
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
					gblPostInstr.Append("\t\tc__GBL__%s%s%s = r_t%d_%sIwData%s%s.GetData();\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
				}
				gblPostInstr.NewLine();

			} while (DimenIter(pGv->m_dimenList, refList));

		} else if (pNgvInfo->m_wrPortList.size() == 1) {

			for (int imIdx = 0; imIdx < 2; imIdx += 1) {
				if (imIdx == 0 && !pGv->m_bReadForInstrRead) continue;
				if (imIdx == 1 && !pGv->m_bReadForMifWrite) continue;
				char const * pImStr = imIdx == 0 ? "Ir" : "Mr";

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					gblPostInstr.Append("\tm__GBL__%s%s%s.write_addr(r_t%d_%sIwData%s.GetAddr());\n",
						pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());

					for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {

						gblPostInstr.Append("\tif (r_t%d_%sIwData%s%s.GetWrEn())\n",
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
						gblPostInstr.Append("\t\tm__GBL__%s%s%s.write_mem(r_t%d_%sIwData%s.GetData());\n",
							pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
					}

					gblPostInstr.NewLine();

				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}
	}

	if (pGv->m_bWriteForMifRead) {
		int rdRspStg = pMod->m_mif.m_mifRd.m_bNeedRdRspInfoRam ? 2 : 1;

		if (pGv->m_addrW == 0) {
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);

				for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
					gblPostInstr.Append("\tif (r_m%d_%sMwData%s%s.GetWrEn())\n",
						rdRspStg + 1,
						pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
					gblPostInstr.Append("\t\tc__GBL__%s%s%s = r_m%d_%sMwData%s%s.GetData();\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
						rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
				}
				gblPostInstr.NewLine();

			} while (DimenIter(pGv->m_dimenList, refList));

		} else if (pNgvInfo->m_wrPortList.size() == 1) {

			for (int imIdx = 0; imIdx < 2; imIdx += 1) {
				if (imIdx == 0 && !pGv->m_bReadForInstrRead) continue;
				if (imIdx == 1 && !pGv->m_bReadForMifWrite) continue;
				char const * pImStr = imIdx == 0 ? "Ir" : "Mr";


				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					gblPostInstr.Append("\tm__GBL__%s%s%s.write_addr(r_m%d_%sMwData%s.GetAddr());\n",
						pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
						rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str());

					for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {

						gblPostInstr.Append("\tif (r_m%d_%sMwData%s%s.GetWrEn())\n",
							rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
						gblPostInstr.Append("\t\tm__GBL__%s%s%s.write_mem(r_m%d_%sMwData%s.GetData());\n",
							pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
							rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str());
					}

					gblPostInstr.NewLine();

				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}
	}

	if (pNgvInfo->m_wrPortList.size() == 2 && pGv->m_addrW > 0) {
		int rdRspStg = pMod->m_mif.m_mifRd.m_bNeedRdRspInfoRam ? 2 : 1;
		int gvWrStg = pMod->m_tsStg + pGv->m_wrStg.AsInt();

		{
			string wrPort[2];

			for (size_t i = 0; i < pNgvInfo->m_wrPortList.size(); i += 1) {
				int modIdx = pNgvInfo->m_wrPortList[i].first;
				CModule * pWrMod = pNgvInfo->m_modInfoList[modIdx].m_pMod;
				int imIdx = pNgvInfo->m_wrPortList[i].second;

				if (pWrMod == pNgvInfo->m_pRdMod) {
					if (imIdx == 0) {
						wrPort[i] = VA("r_t%d_%sIwData%%s.read()", gvWrStg, pGv->m_gblName.c_str());
					} else {
						wrPort[i] = VA("r_m%d_%sMwData%%s.read()", rdRspStg + 1, pGv->m_gblName.c_str());
					}
				} else {
					if (imIdx == 0) {
						wrPort[i] = VA("i_%sTo%s_%sIwData%%s.read()",
							pWrMod->m_modName.Lc().c_str(), pNgvInfo->m_pRdMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str());
					} else {
						wrPort[i] = VA("i_%sTo%s_%sMwData%%s.read()", 
							pWrMod->m_modName.Lc().c_str(), pNgvInfo->m_pRdMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str());
					}
				}
			}

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);

				m_gblPostInstr2x.Append("\tc__GBL__%sPwData%s = r_phase ? %s : %s;\n",
					pGv->m_gblName.c_str(), dimIdx.c_str(),
					VA(wrPort[0].c_str(), dimIdx.c_str()).c_str(),
					VA(wrPort[1].c_str(), dimIdx.c_str()).c_str(),
					rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str());

				m_gblReg2x.Append("\tr__GBL__%sPwData%s = c__GBL__%sPwData%s;\n",
					pGv->m_gblName.c_str(), dimIdx.c_str(),
					pGv->m_gblName.c_str(), dimIdx.c_str());

			} while (DimenIter(pGv->m_dimenList, refList));

			m_gblPostInstr2x.NewLine();
			m_gblReg2x.NewLine();
		}

		{

			for (int imIdx = 0; imIdx < 2; imIdx += 1) {
				if (imIdx == 0 && !pGv->m_bReadForInstrRead) continue;
				if (imIdx == 1 && !pGv->m_bReadForMifWrite) continue;
				char const * pImStr = imIdx == 0 ? "Ir" : "Mr";

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					m_gblPostInstr2x.Append("\tm__GBL__%s%s%s.write_addr(r__GBL__%sPwData%s.GetAddr());\n",
						pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());

					for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {

						m_gblPostInstr2x.Append("\tif (r__GBL__%sPwData%s%s.GetWrEn())\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
						m_gblPostInstr2x.Append("\t\tm__GBL__%s%s%s.write_mem(r__GBL__%sPwData%s.GetData());\n",
							pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str());
					}

					gblPostInstr.NewLine();

				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}
	}
}

void CDsnInfo::GenModNgvStatements(CModule &mod)
{
	if (mod.m_ngvList.size() == 0)
		return;

	for (size_t gvIdx = 0; gvIdx < mod.m_ngvList.size(); gvIdx += 1) {
		CRam * pGv = mod.m_ngvList[gvIdx];
		CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;
		if (!pNgvInfo->m_bOgv) continue;

		GenModOptNgvStatements(&mod, pGv);
	}

	CHtCode & gblPreInstr = mod.m_clkRate == eClk2x ? m_gblPreInstr2x : m_gblPreInstr1x;
	CHtCode & gblPostInstr = mod.m_clkRate == eClk2x ? m_gblPostInstr2x : m_gblPostInstr1x;
	CHtCode & gblReg = mod.m_clkRate == eClk2x ? m_gblReg2x : m_gblReg1x;
	CHtCode & gblOut = mod.m_clkRate == eClk2x ? m_gblOut2x : m_gblOut1x;

	string gblRegReset = mod.m_clkRate == eClk2x ? "c_reset1x" : "r_reset1x";

	string vcdModName = VA("Pers%s", mod.m_modName.Uc().c_str());

	bool bInstrWrite = false;

	bool bFirstModVar = false;
	for (size_t gvIdx = 0; gvIdx < mod.m_ngvList.size(); gvIdx += 1) {
		CRam * pGv = mod.m_ngvList[gvIdx];
		CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;
		if (pNgvInfo->m_bOgv) continue;

		bInstrWrite |= pGv->m_bWriteForInstrWrite;

		int stgLast = max(pGv->m_wrStg.AsInt(), mod.m_stage.m_privWrStg.AsInt());
		for (int stgIdx = 1; stgIdx <= stgLast; stgIdx += 1) {

			string varStg;
			if (/*pGv->m_rdStg.size() > 0 || pGv->m_wrStg.size() > 0 || */mod.m_stage.m_bStageNums)
				varStg = VA("%d", stgIdx);

			if (stgIdx >= pGv->m_rdStg.AsInt()) {
				if (!pGv->m_bPrivGbl && pGv->m_bReadForInstrRead) {
					if (pNgvInfo->m_atomicMask != 0) {
						GenModVar(eVcdUser, vcdModName, bFirstModVar,
							VA("%s const", pGv->m_type.c_str()),
							pGv->m_dimenDecl,
							VA("GF%s_%s", varStg.c_str(), pGv->m_gblName.c_str()),
							VA("r_t%d_%sIfData", mod.m_tsStg + stgIdx - 1, pGv->m_gblName.c_str()),
							pGv->m_dimenList);
					}

					GenModVar(eVcdUser, vcdModName, bFirstModVar,
						VA("%s const", pGv->m_type.c_str()),
						pGv->m_dimenDecl,
						VA("GR%s_%s", varStg.c_str(), pGv->m_gblName.c_str()),
						VA("r_t%d_%sIrData", mod.m_tsStg + stgIdx - 1, pGv->m_gblName.c_str()),
						pGv->m_dimenList);
				}
			}

			if (stgIdx <= pGv->m_wrStg.AsInt()) {
				if (!pGv->m_bPrivGbl && pGv->m_bWriteForInstrWrite) {
					GenModVar(eVcdNone, vcdModName, bFirstModVar,
						VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
						pGv->m_dimenDecl,
						VA("GW%s_%s", varStg.c_str(), pGv->m_gblName.c_str()),
						VA("c_t%d_%sIwData", mod.m_tsStg + stgIdx - 1, pGv->m_gblName.c_str()),
						pGv->m_dimenList);
				}
			}
		}
	}

	CModInst & modInst = mod.m_modInstList[0];

	if (bInstrWrite) {
		// ht completion struct
		CRecord htComp;
		htComp.m_typeName = "CHtComp";
		htComp.m_bCStyle = true;

		if (mod.m_threads.m_htIdW.AsInt() > 0)
			htComp.AddStructField(FindHtIntType(eUnsigned, mod.m_threads.m_htIdW.AsInt()), "m_htId");

		htComp.AddStructField(&g_bool, "m_htCmdRdy");

		for (size_t gvIdx = 0; gvIdx < mod.m_ngvList.size(); gvIdx += 1) {
			CRam * pGv = mod.m_ngvList[gvIdx];
			CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;
			if (pNgvInfo->m_bOgv) continue;

			if (pGv->m_bWriteForInstrWrite)
				htComp.AddStructField(&g_bool, VA("m_%sIwComp", pGv->m_gblName.c_str()), "", "", pGv->m_dimenList);
		}

		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrIn) continue;
			if (cxrIntf.m_pDstMod->m_modName.AsStr() == "hif") continue;
			if (cxrIntf.GetPortReplId() != 0) continue;

			htComp.AddStructField(&g_bool, VA("m_%sCompRdy", cxrIntf.GetIntfName()), "", "", cxrIntf.GetPortReplDimen());
		}

		if (mod.m_threads.m_bCallFork && mod.m_threads.m_htIdW.AsInt() > 2)
			htComp.AddStructField(&g_bool, VA("m_htPrivLkData"));

		GenUserStructs(m_gblRegDecl, &htComp, "\t");

		if (mod.m_threads.m_htIdW.AsInt() == 0) {
			gblReg.Append("\tr_t%d_htCmdRdy = c_t%d_htCmdRdy;\n",
				mod.m_wrStg, mod.m_wrStg - 1);
			gblReg.NewLine();
		}
	}

	for (size_t gvIdx = 0; gvIdx < mod.m_ngvList.size(); gvIdx += 1) {
		CRam * pGv = mod.m_ngvList[gvIdx];
		CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;
		if (pNgvInfo->m_bOgv) continue;

		CHtCode & gblPostInstrWrData = pNgvInfo->m_bNgvWrDataClk2x ? m_gblPostInstr2x : m_gblPostInstr1x;
		CHtCode & gblPostInstrWrComp = pNgvInfo->m_bNgvWrCompClk2x ? m_gblPostInstr2x : m_gblPostInstr1x;

		CHtCode & gblRegWrData = pNgvInfo->m_bNgvWrDataClk2x ? m_gblReg2x : m_gblReg1x;
		CHtCode & gblRegWrComp = pNgvInfo->m_bNgvWrCompClk2x ? m_gblReg2x : m_gblReg1x;

		string gblRegWrDataReset = pNgvInfo->m_bNgvWrDataClk2x ? "c_reset1x" : "r_reset1x";
		string gblRegWrCompReset = pNgvInfo->m_bNgvWrCompClk2x ? "c_reset1x" : "r_reset1x";

		// global variable module I/O
		if (pGv->m_bWriteForInstrWrite) { // Instruction read
			int gvWrStg = mod.m_tsStg + pGv->m_wrStg.AsInt();
			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				m_gblIoDecl.Append("\tsc_out<sc_uint<%s_HTID_W> > o_%sTo%s_iwHtId%s;\n",
					mod.m_modName.Upper().c_str(),
					mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblOut.Append("\to_%sTo%s_iwHtId%s = r_t%d_htId;\n",
						mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						gvWrStg);
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
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
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
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}

		if (pGv->m_bReadForInstrRead) { // Instruction read
			if (pNgvInfo->m_atomicMask != 0) {
				m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_ifWrEn%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					m_gblIoDecl.Append("\tsc_in<sc_uint<%s_HTID_W> > i_%sTo%s_ifHtId%s;\n",
						mod.m_modName.Upper().c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				}
				m_gblIoDecl.Append("\tsc_in<%s> i_%sTo%s_ifData%s;\n",
					pGv->m_type.c_str(),
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}
		}

		if (pGv->m_bReadForInstrRead || pGv->m_bReadForMifWrite) {
			m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_wrEn%s;\n",
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			if (pGv->m_addrW > 0)
				m_gblIoDecl.Append("\tsc_in<ht_uint%d> i_%sTo%s_wrAddr%s;\n",
				pGv->m_addrW,
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			m_gblIoDecl.Append("\tsc_in<%s> i_%sTo%s_wrData%s;\n",
				pGv->m_type.c_str(),
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
		}

		if (pGv->m_bWriteForInstrWrite) {
			m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_iwCompRdy%s;\n",
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				m_gblIoDecl.Append("\tsc_in<sc_uint<%s_HTID_W> > i_%sTo%s_iwCompHtId%s;\n",
					mod.m_modName.Upper().c_str(),
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}
			m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_iwCompData%s;\n",
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
		}

		if (pGv->m_bWriteForMifRead) { // Memory
			int rdRspStg = mod.m_mif.m_mifRd.m_bNeedRdRspInfoRam ? 2 : 1;
			if (mod.m_mif.m_mifRd.m_rspGrpW.AsInt() > 0) {
				m_gblIoDecl.Append("\tsc_out<sc_uint<%s_RD_GRP_ID_W> > o_%sTo%s_mwGrpId%s;\n",
					mod.m_modName.Upper().c_str(),
					mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblOut.Append("\to_%sTo%s_mwGrpId%s = r_m%d_rdRspInfo.m_grpId;\n",
						mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						rdRspStg + 1);
				} while (DimenIter(pGv->m_dimenList, refList));
			}

			{
				m_gblIoDecl.Append("\tsc_out<CGW_%s> o_%sTo%s_mwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(),
					mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblOut.Append("\to_%sTo%s_mwData%s = r_m%d_%sMwData%s;\n",
						mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}

			if (pGv->m_pNgvInfo->m_bNeedQue/* && (mod.m_mif.m_mifRd.m_bMultiQwHostRdMif || mod.m_mif.m_mifRd.m_bMultiQwCoprocRdMif)*/) {
				m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_mwFull%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}

			m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_mwCompRdy%s;\n",
				pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			if (mod.m_mif.m_mifRd.m_rspGrpW.AsInt() > 0) {
				m_gblIoDecl.Append("\tsc_in<sc_uint<%s_RD_GRP_ID_W> > i_%sTo%s_mwCompGrpId%s;\n",
					mod.m_modName.Upper().c_str(),
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}
		}

		if (pGv->m_addrW == 0) {
			if (pGv->m_bReadForInstrRead || pGv->m_bReadForMifWrite) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					if (pNgvInfo->m_bNgvWrDataClk2x && (mod.m_clkRate == eClk1x)) {
						gblRegWrData.Append("\tif (r_%sTo%s_wrEn_2x%s)\n",
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
						gblRegWrData.Append("\t\tr__GBL__%s_2x%s = r_%sTo%s_wrData_2x%s;\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
						gblReg.Append("\tr__GBL__%s%s = r__GBL__%s_2x%s;\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str());
					} else {
						gblRegWrData.Append("\tif (r_%sTo%s_wrEn%s)\n",
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
						gblRegWrData.Append("\t\tr__GBL__%s%s = r_%sTo%s_wrData%s;\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					}
					gblRegWrData.NewLine();

				} while (DimenIter(pGv->m_dimenList, refList));
			}
		} else {
			for (int imIdx = 0; imIdx < 2; imIdx += 1) {
				if (imIdx == 0 && !pGv->m_bReadForInstrRead) continue;
				if (imIdx == 1 && !pGv->m_bReadForMifWrite) continue;
				char const * pImStr = imIdx == 0 ? "Ir" : "Mr";

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblPostInstrWrData.Append("\tm__GBL__%s%s%s.write_addr(r_%sTo%s_wrAddr%s);\n",
						pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrData.Append("\tif (r_%sTo%s_wrEn%s)\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrData.Append("\t\tm__GBL__%s%s%s.write_mem(r_%sTo%s_wrData%s);\n",
						pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrData.NewLine();
				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}

		if (pGv->m_bReadForInstrRead && pNgvInfo->m_atomicMask != 0) {
			if (mod.m_threads.m_htIdW.AsInt() == 0) {
				if (pNgvInfo->m_bNgvWrDataClk2x == (mod.m_clkRate == eClk2x)) {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, pGv->m_type,
						VA("r_%sIf", pGv->m_gblName.c_str()), pGv->m_dimenList);
				} else {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("sc_signal<%s>", pGv->m_type.c_str()),
						VA("r_%sIf", pGv->m_gblName.c_str()), pGv->m_dimenList);
				}
			} else {
				m_gblRegDecl.Append("\tht_dist_ram<%s, %s_HTID_W> m__GBL__%sIf%s;\n",
					pGv->m_type.c_str(),
					mod.m_modName.Upper().c_str(),
					pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					if (pNgvInfo->m_bNgvWrDataClk2x == (mod.m_clkRate == eClk2x)) {
						gblReg.Append("\tm__GBL__%sIf%s.clock(%s);\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							gblRegReset.c_str());
					} else {
						gblReg.Append("\tm__GBL__%sIf%s.read_clock(%s);\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							gblRegReset.c_str());
						gblRegWrData.Append("\tm__GBL__%sIf%s.write_clock(%s);\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							gblRegWrDataReset.c_str());
					}
				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}

		if (pGv->m_addrW == 0) {
			if (pGv->m_bReadForInstrRead || pGv->m_bReadForMifWrite) {
				if (pNgvInfo->m_bNgvWrDataClk2x == (mod.m_clkRate == eClk2x)) {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, pGv->m_type.c_str(),
						VA("r__GBL__%s", pGv->m_gblName.c_str()), pGv->m_dimenList);
				} else if (mod.m_clkRate == eClk2x) {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("sc_signal<%s>", pGv->m_type.c_str()),
						VA("r__GBL__%s", pGv->m_gblName.c_str()), pGv->m_dimenList);
				} else {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("sc_signal<%s>", pGv->m_type.c_str()),
						VA("r__GBL__%s_2x", pGv->m_gblName.c_str()), pGv->m_dimenList);
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, pGv->m_type.c_str(),
						VA("r__GBL__%s", pGv->m_gblName.c_str()), pGv->m_dimenList);
				}
			}
		} else {
			for (int imIdx = 0; imIdx < 2; imIdx += 1) {
				if (imIdx == 0 && !pGv->m_bReadForInstrRead) continue;
				if (imIdx == 1 && !pGv->m_bReadForMifWrite) continue;
				char const * pImStr = imIdx == 0 ? "Ir" : "Mr";

				m_gblRegDecl.Append("\tht_%s_ram<%s, %d",
					pGv->m_ramType == eBlockRam ? "block" : "dist",
					pGv->m_type.c_str(), pGv->m_addrW);
				m_gblRegDecl.Append("> m__GBL__%s%s%s;\n", pGv->m_gblName.c_str(), pImStr, pGv->m_dimenDecl.c_str());

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					if (pNgvInfo->m_bNgvWrDataClk2x == (mod.m_clkRate == eClk2x)) {
						gblReg.Append("\tm__GBL__%s%s%s.clock(%s);\n",
							pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
							gblRegReset.c_str());
					} 
					else {
						gblReg.Append("\tm__GBL__%s%s%s.read_clock(%s);\n",
							pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(), 
							gblRegReset.c_str());
						gblRegWrData.Append("\tm__GBL__%s%s%s.write_clock(%s);\n",
							pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(), 
							gblRegWrDataReset.c_str());
					}
				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}

		if (pGv->m_bWriteForInstrWrite) {
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

						gblRegWrComp.Append("\tm_%sIwComp%s[0].clock(%s);\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegWrCompReset.c_str());
						gblRegWrComp.Append("\tm_%sIwComp%s[1].clock(%s);\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegWrCompReset.c_str());
					} else {
						CHtCode &gblRegMod = mod.m_clkRate == eClk2x ? m_gblReg2x : m_gblReg1x;
						string gblRegModReset = mod.m_clkRate == eClk2x ? "c_reset1x" : "r_reset1x";

						gblRegMod.Append("\tm_%sIwComp%s[0].read_clock(%s);\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegModReset.c_str());
						gblRegMod.Append("\tm_%sIwComp%s[1].read_clock(%s);\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegModReset.c_str());
						gblRegWrComp.Append("\tm_%sIwComp%s[0].write_clock(%s);\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegWrCompReset.c_str());
						gblRegWrComp.Append("\tm_%sIwComp%s[1].write_clock(%s);\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegWrCompReset.c_str());
					}
				} while (DimenIter(pGv->m_dimenList, refList));

				if (pGv->m_dimenList.size() > 0) {
					gblReg.NewLine();
					gblRegWrComp.NewLine();
				}
			}
		}

		if (pGv->m_bReadForInstrRead && pNgvInfo->m_atomicMask != 0) {
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

		if (pGv->m_bReadForInstrRead && pNgvInfo->m_atomicMask != 0 && mod.m_threads.m_htIdW.AsInt() > 0) {
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

		if (pGv->m_bReadForInstrRead && pNgvInfo->m_atomicMask != 0) {
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

		if ((pGv->m_bReadForInstrRead || pGv->m_bReadForMifWrite) && pGv->m_ramType == eBlockRam) {

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

			if (pGv->m_addrW > 0) {
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA(bSignal ? "sc_signal<ht_uint%d>" : "ht_uint%d", pGv->m_addrW),
					VA("r_g1_%sTo%s_wrAddr", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblRegWrData.Append("\tr_g1_%sTo%s_wrAddr%s = r_%sTo%s_wrAddr%s;\n",
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

		if (pGv->m_bReadForInstrRead && pGv->m_ramType == eBlockRam) {
			int rdAddrStgNum = mod.m_tsStg + pGv->m_rdStg.AsInt() - (pGv->m_ramType == eBlockRam ? 3 : 2);

			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addrW),
				VA("r_t%d_%sRdAddr", rdAddrStgNum + 1, pGv->m_gblName.Lc().c_str()));

			gblReg.Append("\tr_t%d_%sRdAddr = c_t%d_%sRdAddr;\n",
				rdAddrStgNum + 1, pGv->m_gblName.Lc().c_str(),
				rdAddrStgNum, pGv->m_gblName.Lc().c_str());
		}

		if (pGv->m_bReadForInstrRead || pGv->m_bReadForMifWrite) {
			;
			{
				if (pNgvInfo->m_bNgvWrDataClk2x && (mod.m_clkRate == eClk1x) && pGv->m_addrW == 0) {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
						VA("r_%sTo%s_wrEn_2x", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);
				} else {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
						VA("r_%sTo%s_wrEn", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);
				}

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					if (pNgvInfo->m_bNgvWrDataClk2x && (mod.m_clkRate == eClk1x) && pGv->m_addrW == 0) {
						gblRegWrData.Append("\tr_%sTo%s_wrEn_2x%s = i_%sTo%s_wrEn%s;\n",
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					} else {
						gblRegWrData.Append("\tr_%sTo%s_wrEn%s = i_%sTo%s_wrEn%s;\n",
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					}
				} while (DimenIter(pGv->m_dimenList, refList));
			}

			if (pGv->m_addrW > 0) {
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addrW),
					VA("r_%sTo%s_wrAddr", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblRegWrData.Append("\tr_%sTo%s_wrAddr%s = i_%sTo%s_wrAddr%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}

			{
				if (pNgvInfo->m_bNgvWrDataClk2x && (mod.m_clkRate == eClk1x) && pGv->m_addrW == 0) {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, pGv->m_type,
						VA("r_%sTo%s_wrData_2x", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);
				} else {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, pGv->m_type,
						VA("r_%sTo%s_wrData", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);
				}

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					if (pNgvInfo->m_bNgvWrDataClk2x && (mod.m_clkRate == eClk1x) && pGv->m_addrW == 0) {
						gblRegWrData.Append("\tr_%sTo%s_wrData_2x%s = i_%sTo%s_wrData%s;\n",
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					} else {
						gblRegWrData.Append("\tr_%sTo%s_wrData%s = i_%sTo%s_wrData%s;\n",
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					}
				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}

		string compReset = pNgvInfo->m_bNgvWrCompClk2x ? "c_reset1x" : "r_reset1x";
		if (pGv->m_bWriteForInstrWrite) {
			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				m_gblRegDecl.Append("\tbool c_%sTo%s_iwCompRdy%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
				VA("r_%sTo%s_iwCompRdy", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), pGv->m_dimenList);

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					gblRegWrComp.Append("\tr_%sTo%s_iwCompRdy%s = %s || c_%sTo%s_iwCompRdy%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						compReset.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				} else {
					gblRegWrComp.Append("\tr_%sTo%s_iwCompRdy%s = i_%sTo%s_iwCompRdy%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				}
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		if (pGv->m_bWriteForInstrWrite && mod.m_threads.m_htIdW.AsInt() > 0) {
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

		if (pGv->m_bWriteForInstrWrite) {
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

		if (pGv->m_bWriteForInstrWrite && mod.m_threads.m_htIdW.AsInt() > 0) {
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

		if (pGv->m_bWriteForInstrWrite) {

			int gvWrStg = mod.m_tsStg + pGv->m_wrStg.AsInt();
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				if (mod.m_threads.m_htIdW.AsInt() == 0) {
					gblPostInstr.Append("\tc_t%d_%sIwComp%s = r_%sIwComp%s;\n",
						gvWrStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				}
				else {
					gblPostInstr.Append("\tm_%sIwComp%s[0].read_addr(r_t%d_htId);\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(), gvWrStg - 1);
					gblPostInstr.Append("\tc_t%d_%sIwComp%s = m_%sIwComp%s[0].read_mem();\n",
						gvWrStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				}
			} while (DimenIter(pGv->m_dimenList, refList));

			for (; gvWrStg <= mod.m_gvIwCompStg; gvWrStg += 1) {
				m_gblRegDecl.Append("\tbool c_t%d_%sIwComp%s;\n",
					gvWrStg - 1, pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
					VA("r_t%d_%sIwComp", gvWrStg, pGv->m_gblName.c_str()), pGv->m_dimenList);
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					if (gvWrStg > mod.m_tsStg + pGv->m_wrStg.AsInt()) {
						gblPostInstr.Append("\tc_t%d_%sIwComp%s = r_t%d_%sIwComp%s;\n",
							gvWrStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
							gvWrStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str());
					}
					gblReg.Append("\tr_t%d_%sIwComp%s = c_t%d_%sIwComp%s;\n",
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
						gvWrStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}

		if (pGv->m_bReadForInstrRead && pNgvInfo->m_atomicMask != 0) {
			m_gblRegDecl.Append("\t%s c_t%d_%sIfData%s;\n",
				pGv->m_type.c_str(), mod.m_tsStg - 1, pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("%s ht_noload", pGv->m_type.c_str()),
				VA("r_t%d_%sIfData", mod.m_tsStg, pGv->m_gblName.c_str()), pGv->m_dimenList);
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblReg.Append("\tr_t%d_%sIfData%s = c_t%d_%sIfData%s;\n",
					mod.m_tsStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
					mod.m_tsStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		bool bPrivGblAndNoAddr = pGv->m_bPrivGbl && pGv->m_addrW == mod.m_threads.m_htIdW.AsInt();

		if (pGv->m_bReadForInstrRead) {
			int lastStg = pGv->m_bPrivGbl ? mod.m_stage.m_privWrStg.AsInt() : max(pGv->m_wrStg.AsInt(), mod.m_stage.m_privWrStg.AsInt());
			for (int gvRdStg = pGv->m_rdStg.AsInt(); gvRdStg <= lastStg; gvRdStg += 1) {
				m_gblRegDecl.Append("\t%s c_t%d_%sIrData%s;\n",
					pGv->m_type.c_str(), mod.m_tsStg + gvRdStg - 2, pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("%s ht_noload", pGv->m_type.c_str()),
					VA("r_t%d_%sIrData", mod.m_tsStg + gvRdStg - 1, pGv->m_gblName.c_str()), pGv->m_dimenList);
			}
			vector<int> refList(pGv->m_dimenList.size());
			do {
				for (int gvRdStg = pGv->m_rdStg.AsInt(); gvRdStg <= lastStg; gvRdStg += 1) {
					string dimIdx = IndexStr(refList);

					if (gvRdStg > pGv->m_rdStg.AsInt()) {
						gblPostInstr.Append("\tc_t%d_%sIrData%s = r_t%d_%sIrData%s;\n",
							mod.m_tsStg + gvRdStg - 2, pGv->m_gblName.c_str(), dimIdx.c_str(),
							mod.m_tsStg + gvRdStg - 2, pGv->m_gblName.c_str(), dimIdx.c_str());
					}

					gblReg.Append("\tr_t%d_%sIrData%s = c_t%d_%sIrData%s;\n",
						mod.m_tsStg + gvRdStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						mod.m_tsStg + gvRdStg - 2, pGv->m_gblName.c_str(), dimIdx.c_str());
				}
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		if (pGv->m_bWriteForInstrWrite) {
			for (int gvWrStg = mod.m_tsStg; gvWrStg < mod.m_tsStg + pGv->m_wrStg.AsInt(); gvWrStg += 1) {
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
					VA("r_t%d_%sIwData", gvWrStg + 1, pGv->m_gblName.c_str()), pGv->m_dimenList);
				m_gblRegDecl.Append("\tCGW_%s c_t%d_%sIwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(), gvWrStg, pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
			}

			string htIdStr = (mod.m_threads.m_htIdW.AsInt() > 0 &&
				(pGv->m_bPrivGbl || pGv->m_addr1Name == "htId" || pGv->m_addr2Name == "htId")) ? VA("r_t%d_htId", mod.m_tsStg) : "";

			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);

				for (int gvWrStg = mod.m_tsStg; gvWrStg < mod.m_tsStg + pGv->m_wrStg.AsInt(); gvWrStg += 1) {
					if (bPrivGblAndNoAddr && gvWrStg == mod.m_tsStg) {
						gblPreInstr.Append("\tc_t%d_%sIwData%s.InitData(%s, r_t%d_%sIrData%s);\n",
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
							htIdStr.c_str(),
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
					} else if (!bPrivGblAndNoAddr && gvWrStg == mod.m_tsStg) {
						gblPreInstr.Append("\tc_t%d_%sIwData%s.InitZero(%s);\n",
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
							htIdStr.c_str());
					} else {
						gblPreInstr.Append("\tc_t%d_%sIwData%s = r_t%d_%sIwData%s;\n",
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
					}
					gblReg.Append("\tr_t%d_%sIwData%s = c_t%d_%sIwData%s;\n",
						gvWrStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
				}
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		gblPostInstr.NewLine();

		if (pGv->m_bWriteForMifRead) {
			int rdRspStg = mod.m_mif.m_mifRd.m_bNeedRdRspInfoRam ? 2 : 1;
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
				VA("r_m%d_%sMwData", rdRspStg + 1, pGv->m_gblName.c_str()), pGv->m_dimenList);
			m_gblRegDecl.Append("\tCGW_%s c_m%d_%sMwData%s;\n",
				pGv->m_pNgvInfo->m_ngvWrType.c_str(),
				rdRspStg,
				pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

			string htIdStr = (mod.m_threads.m_htIdW.AsInt() > 0 &&
				(pGv->m_bPrivGbl || pGv->m_addr1Name == "htId" || pGv->m_addr2Name == "htId")) ? VA("r_m%d_rdRspInfo.m_htId", rdRspStg) : "";
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblPreInstr.Append("\tc_m%d_%sMwData%s.InitZero(%s);\n",
					rdRspStg,
					pGv->m_gblName.c_str(), dimIdx.c_str(),
					htIdStr.c_str());
				gblReg.Append("\tr_m%d_%sMwData%s = c_m%d_%sMwData%s;\n",
					rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
					rdRspStg, pGv->m_gblName.c_str(), dimIdx.c_str());
			} while (DimenIter(pGv->m_dimenList, refList));
		}

		if (pGv->m_bReadForInstrRead && pNgvInfo->m_atomicMask != 0) {
			if (mod.m_threads.m_htIdW.AsInt() == 0) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblPostInstr.Append("\tc_t%d_%sIfData%s = r_%sIf%s;\n",
						mod.m_tsStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			} else {
				int rdAddrStgNum = pGv->m_ramType == eBlockRam ? (mod.m_tsStg - 2) : (mod.m_tsStg - 1);
				string rdAddrStg = rdAddrStgNum == 1 ? "c_t1" : VA("r_t%d", rdAddrStgNum);

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblPostInstr.Append("\tm__GBL__%sIf%s.read_addr(%s_htId);\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						rdAddrStg.c_str());
					gblPostInstr.Append("\tc_t%d_%sIfData%s = m__GBL__%sIf%s.read_mem();\n",
						mod.m_tsStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}
			gblPostInstr.NewLine();
		}

		if (pGv->m_bReadForInstrRead && pNgvInfo->m_atomicMask != 0) {
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
					gblPostInstrWrData.Append("\tm__GBL__%sIf%s.write_addr(r_%sTo%s_ifHtId%s);\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrData.Append("\tif (r_%sTo%s_ifWrEn%s)\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrData.Append("\t\tm__GBL__%sIf%s.write_mem(r_%sTo%s_ifData%s);\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
				}
				gblPostInstrWrData.NewLine();
			} while (DimenIter(pGv->m_dimenList, refList));
		}
		gblPostInstrWrData.NewLine();

		if (pGv->m_bReadForInstrRead) {
			if (pGv->m_addrW == 0) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblPostInstr.Append("\tc_t%d_%sIrData%s = r__GBL__%s%s;\n",
						mod.m_tsStg - 2 + pGv->m_rdStg.AsInt(), pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			} else {
				int rdAddrStgNum = mod.m_tsStg + pGv->m_rdStg.AsInt() - (pGv->m_ramType == eBlockRam ? 3 : 2);
				string rdAddrStg = rdAddrStgNum == 1 ? "c_t1" : VA("r_t%d", rdAddrStgNum);

				string addr1Name;
				if (pGv->m_addr1IsHtId)
					addr1Name = VA("%s_htId", rdAddrStg.c_str());
				else if (pGv->m_addr1IsPrivate)
					addr1Name = VA("%s_htPriv.m_%s", rdAddrStg.c_str(), pGv->m_addr1Name.c_str());
				else if (pGv->m_addr1IsStage)
					addr1Name = VA("%s__STG__%s", rdAddrStg.c_str(), pGv->m_addr1Name.c_str());
				else if (pGv->m_addr1W.AsInt() > 0)
					HtlAssert(0);

				string addr2Name;
				if (pGv->m_addr2IsHtId)
					addr2Name = VA("%s_htId", rdAddrStg.c_str());
				else if (pGv->m_addr2IsPrivate)
					addr2Name = VA("%s_htPriv.m_%s", rdAddrStg.c_str(), pGv->m_addr2Name.c_str());
				else if (pGv->m_addr1IsStage)
					addr2Name = VA("%s__STG__%s", rdAddrStg.c_str(), pGv->m_addr2Name.c_str());
				else if (pGv->m_addr2W.AsInt() > 0)
					HtlAssert(0);

				bool bNeedParan = ((pGv->m_addr0W.AsInt() > 0 ? 1 : 0) + (pGv->m_addr1W.AsInt() > 0 ? 1 : 0) + (pGv->m_addr2W.AsInt() > 0 ? 1 : 0)) > 1;
				gblPostInstr.Append("\tht_uint%d c_t%d_%sRdAddr = %s", pGv->m_addrW, rdAddrStgNum, pGv->m_gblName.Lc().c_str(), bNeedParan ? "(" : "");
				if (pGv->m_addr0W.AsInt() > 0)
					gblPostInstr.Append("%s_htId", rdAddrStg.c_str());
				if (pGv->m_addr0W.AsInt() > 0 && pGv->m_addr1W.AsInt() > 0)
					gblPostInstr.Append(", ");
				if (pGv->m_addr1W.AsInt() > 0)
					gblPostInstr.Append("%s", addr1Name.c_str());
				if (pGv->m_addr2W.AsInt() > 0)
					gblPostInstr.Append(", %s", addr2Name.c_str());
				gblPostInstr.Append("%s;\n", bNeedParan ? ")" : "");

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					gblPostInstr.Append("\tm__GBL__%sIr%s.read_addr(c_t%d_%sRdAddr);\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						rdAddrStgNum, pGv->m_gblName.Lc().c_str());

					bool bNgvReg = pGv->m_addrW == 0;
					bool bNgvDist = !bNgvReg && pGv->m_ramType != eBlockRam;

					if (bNgvDist) {
						gblPostInstr.Append("\tc_t%d_%sIrData%s = m__GBL__%sIr%s.read_mem();\n",
							rdAddrStgNum, pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str());
					} else {

						gblPostInstr.Append("\tif (r_g1_%sTo%s_wrEn%s && r_g1_%sTo%s_wrAddr%s == r_t%d_%sRdAddr)\n",
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							rdAddrStgNum + 1, pGv->m_gblName.Lc().c_str());

						gblPostInstr.Append("\t\tc_t%d_%sIrData%s = r_g1_%sTo%s_wrData%s;\n",
							rdAddrStgNum + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());

						gblPostInstr.Append("\telse\n");

						gblPostInstr.Append("\t\tc_t%d_%sIrData%s = m__GBL__%sIr%s.read_mem();\n",
							rdAddrStgNum + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str());
					}
					gblPostInstr.NewLine();

				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}

		if (pGv->m_bWriteForInstrWrite) {
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
					gblPostInstrWrComp.Append("\t%sc_%sIwComp%s = r_%sTo%s_iwCompRdy%s ? !r_%sTo%s_iwCompData%s : r_%sIwComp%s;\n",
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
					gblPostInstrWrComp.Append("\tif (r_%sTo%s_iwCompRdy%s) {\n",
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
					gblPostInstrWrComp.Append("\t\tc_%sTo%s_iwCompRdy%s = true;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrComp.Append("\t\tc_%sTo%s_iwCompHtId%s = r_%sTo%s_iwCompHtId%s + 1u;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrComp.Append("\t\tc_%sTo%s_iwCompData%s = false;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrComp.Append("\t\tc_%sTo%s_iwCompInit%s = r_%sTo%s_iwCompHtId%s != 0x%x;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						(1 << mod.m_threads.m_htIdW.AsInt()) - 2);

					gblPostInstrWrComp.Append("\t} else {\n");

					gblPostInstrWrComp.Append("\t\tc_%sTo%s_iwCompRdy%s = i_%sTo%s_iwCompRdy%s;\n",
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

		int gvWrStg = mod.m_tsStg + pGv->m_wrStg.AsInt();
		if (pGv->m_bWriteForInstrWrite) {

			string typeStr;
			if (pGv->m_dimenList.size() == 0)
				typeStr = "bool ";
			else {
				gblPostInstr.Append("\tbool c_t%d_%sTo%s_iwWrEn%s;\n", 
					gvWrStg, mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}
			vector<int> refList(pGv->m_dimenList.size());
			do {
				string dimIdx = IndexStr(refList);
				gblPostInstr.Append("\t%sc_t%d_%sTo%s_iwWrEn%s = ", typeStr.c_str(),
					gvWrStg, mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str());

				string separator;
				for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
					if (iter.IsStructOrUnion()) continue;

					gblPostInstr.Append("%sr_t%d_%sIwData%s%s.GetWrEn()",
						separator.c_str(), gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
						iter.GetHeirFieldName().c_str());

					separator = " ||\n\t\t";

					if (pGv->m_pType->m_eType != eRecord) continue;

					if (iter->m_atomicMask & ATOMIC_INC)
						gblPostInstr.Append(" || r_t%d_%sIwData%s%s.GetIncEn()",
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
						iter.GetHeirFieldName().c_str());

					if (iter->m_atomicMask & ATOMIC_SET)
						gblPostInstr.Append(" || r_t%d_%sIwData%s%s.GetSetEn()",
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
						iter.GetHeirFieldName().c_str());

					if (iter->m_atomicMask & ATOMIC_ADD)
						gblPostInstr.Append(" || r_t%d_%sIwData%s%s.GetAddEn()",
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
						iter.GetHeirFieldName().c_str());
				}

				gblPostInstr.Append(";\n");

				if (pGv->m_addr1W.size() > 0 && pGv->m_addr1Name != "htId" || pGv->m_addr2W.size() > 0 && pGv->m_addr2Name != "htId") {
					gblPostInstr.Append("\tassert_msg(!c_t%d_%sTo%s_iwWrEn%s || r_t%d_%sIwData%s.IsAddrSet(),\n",
						gvWrStg, mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
					gblPostInstr.Append("\t\t\"%s variable %cW_%s%s was written but address was not set, use method .write_addr()\");\n",
						pGv->m_bPrivGbl ? "private" : "global", pGv->m_bPrivGbl ? 'P' : 'G', pGv->m_gblName.Lc().c_str(), dimIdx.c_str());
				}
			} while (DimenIter(pGv->m_dimenList, refList));

			for (int gvIwStg = gvWrStg + 1; gvIwStg <= mod.m_gvIwCompStg; gvIwStg += 1) {
				if (pGv->m_dimenList.size() > 0) {
					gblPostInstr.Append("\tbool c_t%d_%sTo%s_iwWrEn%s;\n",
						gvIwStg, mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				}
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool", VA("r_t%d_%sTo%s_iwWrEn", gvIwStg, mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str()), pGv->m_dimenList);
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblPostInstr.Append("\t%sc_t%d_%sTo%s_iwWrEn%s = r_t%d_%sTo%s_iwWrEn%s;\n", typeStr.c_str(),
						gvIwStg, mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						gvIwStg, mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str());
					gblReg.Append("\tr_t%d_%sTo%s_iwWrEn%s = c_t%d_%sTo%s_iwWrEn%s;\n",
						gvIwStg, mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						gvIwStg - 1, mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}
			gblPostInstr.NewLine();
		}
	}

	if (bInstrWrite) {
		int queDepthW = mod.m_threads.m_htIdW.AsInt() <= 5 ? 5 : mod.m_threads.m_htIdW.AsInt();
		m_gblRegDecl.Append("\tht_dist_que<CHtComp, %d> m_htCompQue;\n", queDepthW);
		gblReg.Append("\tm_htCompQue.clock(%s);\n", gblRegReset.c_str());

		if (mod.m_threads.m_htIdW.AsInt() == 0) {
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "CHtComp", "r_htCompQueFront");
			gblReg.Append("\tr_htCompQueFront = c_htCompQueFront;\n");

			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool", "r_htCompQueValid");
			gblReg.Append("\tr_htCompQueValid = !r_reset1x && c_htCompQueValid;\n");
		} else {
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "CHtComp", "r_c1_htCompQueFront");
			gblReg.Append("\tr_c1_htCompQueFront = c_c0_htCompQueFront;\n");

			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool", "r_c1_htCompQueValid");
			gblReg.Append("\tr_c1_htCompQueValid = !r_reset1x && c_c0_htCompQueValid;\n");

			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "CHtComp", "r_c2_htCompQueFront");
			gblReg.Append("\tr_c2_htCompQueFront = c_c1_htCompQueFront;\n");

			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool", "r_c2_htCompQueValid");
			gblReg.Append("\tr_c2_htCompQueValid = !r_reset1x && c_c1_htCompQueValid;\n");

			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool", "r_c2_htCompQueReplay");
			gblReg.Append("\tr_c2_htCompQueReplay = c_c1_htCompQueReplay;\n");

			for (size_t gvIdx = 0; gvIdx < mod.m_ngvList.size(); gvIdx += 1) {
				CRam * pGv = mod.m_ngvList[gvIdx];
				CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;
				if (pNgvInfo->m_bOgv) continue;

				if (!pGv->m_bWriteForInstrWrite) continue;

				m_gblRegDecl.Append("\tbool c_c1_%sIwComp%s;\n", pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
					VA("r_c2_%sIwComp", pGv->m_gblName.c_str()), pGv->m_dimenList);

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblReg.Append("\tr_c2_%sIwComp%s = c_c1_%sIwComp%s;\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(), pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}

		m_gblRegDecl.Append("\tht_uint%d c_htCompQueAvlCnt;\n", queDepthW + 1);
		GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("ht_uint%d", queDepthW + 1), "r_htCompQueAvlCnt");

		m_iplEosChecks.Append("\t\tht_assert(r_htCompQueAvlCnt == %d);\n", 1 << queDepthW);

		gblReg.Append("\tr_htCompQueAvlCnt = r_reset1x ? (ht_uint%d)0x%x : c_htCompQueAvlCnt;\n", queDepthW + 1, 1 << queDepthW);

		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrIn) continue;
			if (cxrIntf.m_pDstMod->m_modName.AsStr() == "hif") continue;

			if (cxrIntf.GetPortReplId() == 0) {
				m_gblRegDecl.Append("\tbool c_%s_%sCompRdy%s;\n", cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplDecl());
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool", VA("r_%s_%sCompRdy", cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName()), cxrIntf.GetPortReplDimen());
			}

			gblReg.Append("\tr_%s_%sCompRdy%s = c_%s_%sCompRdy%s;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
		}

		if (mod.m_threads.m_htIdW.AsInt() == 0) {
			m_gblRegDecl.Append("\tbool c_htCompRdy;\n");
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool", "r_htCompRdy");

			gblReg.Append("\tr_htCompRdy = !r_reset1x && c_htCompRdy;\n");
		} else {
			m_gblRegDecl.Append("\tht_uint%d c_htCmdRdyCnt;\n",
				mod.m_threads.m_htIdW.AsInt() + 1);
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("ht_uint%d", mod.m_threads.m_htIdW.AsInt() + 1), "r_htCmdRdyCnt");

			gblReg.Append("\tr_htCmdRdyCnt = r_reset1x ? (ht_uint%d)0 : c_htCmdRdyCnt;\n", mod.m_threads.m_htIdW.AsInt() + 1);
		}

		gblPostInstr.Append("\tif (r_t%d_htValid) {\n", mod.m_gvIwCompStg);
		gblPostInstr.Append("\t\tCHtComp htComp;\n");

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			gblPostInstr.Append("\t\thtComp.m_htId = r_t%d_htId;\n",
				mod.m_gvIwCompStg);
		}

		for (size_t gvIdx = 0; gvIdx < mod.m_ngvList.size(); gvIdx += 1) {
			CRam * pGv = mod.m_ngvList[gvIdx];
			CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;
			if (pNgvInfo->m_bOgv) continue;

			if (pGv->m_bWriteForInstrWrite) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					gblPostInstr.Append("\t\thtComp.m_%sIwComp%s = c_t%d_%sTo%s_iwWrEn%s ? !r_t%d_%sIwComp%s : r_t%d_%sIwComp%s;\n",
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						mod.m_gvIwCompStg, mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						mod.m_gvIwCompStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
						mod.m_gvIwCompStg, pGv->m_gblName.c_str(), dimIdx.c_str());
				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}

		gblPostInstr.Append("\t\thtComp.m_htCmdRdy = r_t%d_htCmdRdy;\n", mod.m_gvIwCompStg);

		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrIn) continue;

			string intfRdy;
			if (mod.m_execStg + 1 == mod.m_gvIwCompStg)
				intfRdy = VA("r_%s_%sRdy%s", cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			else
				intfRdy = VA("r_t%d_%s_%sRdy%s", mod.m_gvIwCompStg, cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

			if (cxrIntf.m_pDstMod->m_modName.AsStr() == "hif") {
				gblPostInstr.Append("\t\tif (%s)\n", intfRdy.c_str());

				if (mod.m_threads.m_htIdW.AsInt() > 0)
					gblPostInstr.Append("\t\t\tm_htIdPool.push(htComp.m_htId);\n");
				else
					gblPostInstr.Append("\t\t\tc_htBusy = false; \n");
			} else {
				gblPostInstr.Append("\t\thtComp.m_%sCompRdy%s = %s;\n",
					cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex(),
					intfRdy.c_str());
			}
		}

		if (mod.m_threads.m_bCallFork && mod.m_threads.m_htIdW.AsInt() > 2)
			gblPostInstr.Append("\t\thtComp.m_htPrivLkData = r_t%d_htPrivLkData;\n", mod.m_gvIwCompStg);

		gblPostInstr.Append("\t\tm_htCompQue.push(htComp);\n");

		gblPostInstr.Append("\t}\n");
		gblPostInstr.Append("\n");

		for (size_t gvIdx = 0; gvIdx < mod.m_ngvList.size(); gvIdx += 1) {
			CRam * pGv = mod.m_ngvList[gvIdx];
			CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;
			if (pNgvInfo->m_bOgv) continue;

			if (pGv->m_bWriteForInstrWrite) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					if (mod.m_threads.m_htIdW.AsInt() > 0) {
						gblPostInstr.Append("\tm_%sIwComp%s[1].read_addr(r_c1_htCompQueFront.m_htId);\n",
							pGv->m_gblName.c_str(), dimIdx.c_str());
						gblPostInstr.Append("\tc_c1_%sIwComp%s = m_%sIwComp%s[1].read_mem();\n",
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str());
					}
				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}
		gblPostInstr.Append("\n");

		string c2StgStr = mod.m_threads.m_htIdW.AsInt() == 0 ? "" : "c2_";

		gblPostInstr.Append("\tbool c_%shtComp = r_%shtCompQueValid", c2StgStr.c_str(), c2StgStr.c_str());

		for (size_t gvIdx = 0; gvIdx < mod.m_ngvList.size(); gvIdx += 1) {
			CRam * pGv = mod.m_ngvList[gvIdx];
			CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;
			if (pNgvInfo->m_bOgv) continue;

			if (pGv->m_bWriteForInstrWrite) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					if (mod.m_threads.m_htIdW.AsInt() == 0) {
						gblPostInstr.Append("\n\t\t&& r_htCompQueFront.m_%sIwComp%s == r_%sIwComp%s",
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str());
					} else {
						gblPostInstr.Append("\n\t\t&& r_c2_htCompQueFront.m_%sIwComp%s == r_c2_%sIwComp%s",
							pGv->m_gblName.c_str(), dimIdx.c_str(), pGv->m_gblName.c_str(), dimIdx.c_str());
					}
				} while (DimenIter(pGv->m_dimenList, refList));
			}
		}
		gblPostInstr.Append(";\n");
		gblPostInstr.NewLine();

		if (mod.m_threads.m_htIdW.AsInt() == 0) {
			gblPostInstr.Append("\tbool c_htCompQueHold = r_htCompQueValid && !c_htComp;\n");
		} else {
			gblPostInstr.Append("\tbool c_c1_htCompQueReplay = !r_c2_htCompQueReplay && r_c2_htCompQueValid && !c_c2_htComp;\n");
			gblPostInstr.Append("\tbool c_htCompReplay = c_c1_htCompQueReplay || r_c2_htCompQueReplay;\n");
		}
		gblPostInstr.NewLine();

		if (mod.m_bMultiThread && mod.m_threads.m_bCallFork && mod.m_bGvIwComp) {
			if (mod.m_threads.m_htIdW.AsInt() <= 2) {
				// no code for registered version
			} else {
				for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
					CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

					if (cxrIntf.m_cxrDir == CxrOut) continue;
					if (cxrIntf.m_cxrType == CxrCall || cxrIntf.m_cxrType == CxrTransfer) continue;

					gblPostInstr.Append("\tm_%s_%sPrivLk1%s.write_addr(r_c2_htCompQueFront.m_htId);\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
				gblPostInstr.Append("\tm_htCmdPrivLk1.write_addr(r_c2_htCompQueFront.m_htId);\n");

				if (mod.m_rsmSrcCnt > 0)
					gblPostInstr.Append("\tm_rsmPrivLk1.write_addr(r_c2_htCompQueFront.m_htId);\n");

				gblPostInstr.Append("\n");
			}
		}

		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrIn) continue;
			if (cxrIntf.m_pDstMod->m_modName.AsStr() == "hif") continue;

			gblPostInstr.Append("\tc_%s_%sCompRdy%s = false;\n",
				cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
		}

		if (mod.m_threads.m_htIdW.AsInt() == 0) {
			gblPostInstr.Append("\tif (c_htComp) {\n");
			gblPostInstr.Append("\t\tif (r_htCompQueFront.m_htCmdRdy)\n");
		} else {
			gblPostInstr.Append("\tif (!c_htCompReplay && c_c2_htComp) {\n");
			gblPostInstr.Append("\t\tif (r_c2_htCompQueFront.m_htCmdRdy)\n");
		}

		if (mod.m_threads.m_htIdW.AsInt() == 0)
			gblPostInstr.Append("\t\t\tc_htCompRdy = true;\n");
		else
			gblPostInstr.Append("\t\t\tc_htCmdRdyCnt += 1u;\n");

		for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

			if (cxrIntf.m_cxrDir == CxrIn) continue;
			if (cxrIntf.m_pDstMod->m_modName.AsStr() == "hif") continue;

			if (cxrIntf.m_cxrType == CxrCall) {
				gblPostInstr.Append("\t\tif (r_%shtCompQueFront.m_%sCompRdy%s)\n",
					c2StgStr.c_str(),
					cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				gblPostInstr.Append("\t\t\tc_%s_%sCompRdy%s = true;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
			} else {
				gblPostInstr.Append("\t\tif (r_%shtCompQueFront.m_%sCompRdy%s) {\n",
					c2StgStr.c_str(),
					cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				gblPostInstr.Append("\t\t\tc_%s_%sCompRdy%s = true;\n",
					cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());

				if (mod.m_threads.m_htIdW.AsInt() == 0) {
					gblPostInstr.Append("\t\t\tc_htBusy = false;\n");
				} else {
					gblPostInstr.Append("\t\t\tm_htIdPool.push(r_%shtCompQueFront.m_htId);\n",	c2StgStr.c_str());
				}
				gblPostInstr.Append("\t\t}\n");
			}
		}

		gblPostInstr.Append("\t\tc_htCompQueAvlCnt += 1;\n");

		if (mod.m_bMultiThread && mod.m_threads.m_bCallFork && mod.m_bGvIwComp) {
			gblPostInstr.NewLine();
			if (mod.m_threads.m_htIdW.AsInt() == 0)
				gblPostInstr.Append("\t\tc_htPrivLk = 0;\n");
			else if (mod.m_threads.m_htIdW.AsInt() <= 2)
				gblPostInstr.Append("\t\tc_htPrivLk[INT(r_c2_htCompQueFront.m_htId)] = 0;\n");
			else {
				for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
					CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

					if (cxrIntf.m_cxrDir == CxrOut) continue;
					if (cxrIntf.m_cxrType == CxrCall || cxrIntf.m_cxrType == CxrTransfer) continue;

					gblPostInstr.Append("\t\tm_%s_%sPrivLk1%s.write_mem(r_c2_htCompQueFront.m_htPrivLkData);\n",
						cxrIntf.GetPortNameSrcToDstLc(), cxrIntf.GetIntfName(), cxrIntf.GetPortReplIndex());
				}
				gblPostInstr.Append("\t\tm_htCmdPrivLk1.write_mem(r_c2_htCompQueFront.m_htPrivLkData);\n");

				if (mod.m_rsmSrcCnt > 0)
					gblPostInstr.Append("\t\tm_rsmPrivLk1.write_mem(r_c2_htCompQueFront.m_htPrivLkData);\n");
			}
		}

		gblPostInstr.Append("\t}\n");
		gblPostInstr.NewLine();

		if (mod.m_threads.m_htIdW.AsInt() == 0) {
			gblPostInstr.Append("\tbool c_htCompQueValid = (!c_htCompQueHold && !m_htCompQue.empty()) || (c_htCompQueHold && r_htCompQueValid);\n");
			gblPostInstr.Append("\tCHtComp c_htCompQueFront = c_htCompQueHold ? r_htCompQueFront : m_htCompQue.front();\n");
			gblPostInstr.NewLine();

			gblPostInstr.Append("\tif (!m_htCompQue.empty() && (c_htComp || !r_htCompQueValid))\n");
			gblPostInstr.Append("\t\tm_htCompQue.pop();\n");
			gblPostInstr.NewLine();
		} else {
			gblPostInstr.Append("\tbool c_c0_htCompQueValid = (!c_htCompReplay && !m_htCompQue.empty()) || (c_htCompReplay && r_c2_htCompQueValid);\n");
			gblPostInstr.Append("\tCHtComp c_c0_htCompQueFront = c_htCompReplay ? r_c2_htCompQueFront : m_htCompQue.front();\n");
			gblPostInstr.NewLine();

			gblPostInstr.Append("\tbool c_c1_htCompQueValid = r_c1_htCompQueValid;\n");
			gblPostInstr.Append("\tCHtComp c_c1_htCompQueFront = r_c1_htCompQueFront;\n");
			gblPostInstr.NewLine();

			gblPostInstr.Append("\tif (!m_htCompQue.empty() && !c_htCompReplay && (c_c2_htComp || !r_c2_htCompQueValid))\n");
			gblPostInstr.Append("\t\tm_htCompQue.pop();\n");
			gblPostInstr.NewLine();
		}
	}
}

void CDsnInfo::GenerateNgvFiles()
{
	for (size_t gvIdx = 0; gvIdx < m_ngvList.size(); gvIdx += 1) {
		CNgvInfo * pNgvInfo = m_ngvList[gvIdx];
		if (pNgvInfo->m_bOgv) continue;
		CRam * pGv = pNgvInfo->m_modInfoList[0].m_pNgv;

		vector<CNgvModInfo> &ngvModInfoList = m_ngvList[gvIdx]->m_modInfoList;

		int ngvPortCnt = (int)pNgvInfo->m_wrPortList.size();

		int ngvFieldCnt = pNgvInfo->m_ngvFieldCnt;
		bool bNgvAtomicFast = pNgvInfo->m_bNgvAtomicFast;
		bool bNgvAtomicSlow = pNgvInfo->m_bNgvAtomicSlow;
		bool bNgvAtomic = bNgvAtomicFast || bNgvAtomicSlow;

		// determine type of ram
		bool bNgvReg = pGv->m_addrW == 0;
		bool bNgvDist = !bNgvReg && pNgvInfo->m_ramType != eBlockRam;
		bool bNgvBlock = !bNgvReg && !bNgvDist;

		bool bNgvSelGwClk2x = bNgvReg && (!pNgvInfo->m_bAllWrPortClk1x && ngvPortCnt <= 2 || ngvPortCnt == 3) ||
			bNgvDist && (!pNgvInfo->m_bAllWrPortClk1x && ngvPortCnt <= 2 || ngvPortCnt == 3) && !bNgvAtomicSlow ||
			bNgvBlock && ((!pNgvInfo->m_bAllWrPortClk1x && ngvPortCnt <= 2 || ngvPortCnt == 3) && !bNgvAtomic && ngvFieldCnt == 1);

		bool bNeed2x = pNgvInfo->m_bNgvWrDataClk2x || pNgvInfo->m_bNgvWrCompClk2x || !pNgvInfo->m_bAllWrPortClk1x || bNgvSelGwClk2x;

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

		string ngvRegWrDataReset = pNgvInfo->m_bNgvWrDataClk2x ? "c_reset1x" : "r_reset1x";

		string ngvWrDataClk = pNgvInfo->m_bNgvWrDataClk2x ? "_2x" : "";
		string ngvSelClk = pNgvInfo->m_bNgvWrCompClk2x ? "_2x" : "";
		string ngvWrCompClk = pNgvInfo->m_bNgvWrCompClk2x ? "_2x" : "";

		bool bNgvMaxSel = pNgvInfo->m_bNgvMaxSel;

		bool bNeedQue = bNgvReg && (pNgvInfo->m_wrPortList.size() == 2 && !pNgvInfo->m_bAllWrPortClk1x || pNgvInfo->m_wrPortList.size() >= 3) ||
			bNgvDist && ((pNgvInfo->m_wrPortList.size() == 2 && !pNgvInfo->m_bAllWrPortClk1x || pNgvInfo->m_wrPortList.size() >= 3) && !bNgvAtomicSlow ||
			((pNgvInfo->m_wrPortList.size() >= 2 || !pNgvInfo->m_bAllWrPortClk1x) && bNgvAtomicSlow)) ||
			bNgvBlock && ((!bNgvAtomic && ngvFieldCnt == 1) && (pNgvInfo->m_wrPortList.size() == 2 && !pNgvInfo->m_bAllWrPortClk1x || pNgvInfo->m_wrPortList.size() >= 3) ||
			(bNgvAtomic || ngvFieldCnt > 1));

		assert(bNeedQue == pNgvInfo->m_bNeedQue);

		bool bNeedQueRegWrEnSig = bNgvReg && (pNgvInfo->m_wrPortList.size() == 2 && pNgvInfo->m_bAllWrPortClk1x) ||
			bNgvDist && ((pNgvInfo->m_wrPortList.size() == 2 && pNgvInfo->m_bAllWrPortClk1x) && !bNgvAtomicSlow ||
			pNgvInfo->m_wrPortList.size() == 1 && !pNgvInfo->m_bAllWrPortClk1x && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel) ||
			bNgvBlock && ((pNgvInfo->m_wrPortList.size() == 2 && pNgvInfo->m_bAllWrPortClk1x) && (!bNgvAtomic && ngvFieldCnt == 1) ||
			ngvPortCnt == 1 && (ngvFieldCnt > 1 || bNgvAtomic) && pNgvInfo->m_bNgvMaxSel);

		bool bNeedQueRegWrEnNonSig = !bNeedQueRegWrEnSig || !bNgvSelGwClk2x && !pNgvInfo->m_bNgvWrCompClk2x;
		string queRegWrEnSig = bNeedQueRegWrEnSig ? "_sig" : "";

		bool bNeedQueRegHtIdSig = bNgvReg && (ngvPortCnt == 2 && pNgvInfo->m_bAllWrPortClk1x && bNgvAtomic) ||
			bNgvDist && (ngvPortCnt == 1 && !pNgvInfo->m_bAllWrPortClk1x && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel ||
			ngvPortCnt == 2 && pNgvInfo->m_bAllWrPortClk1x && bNgvAtomicFast) ||
			bNgvBlock && ngvPortCnt == 1 && (ngvFieldCnt > 1 || bNgvAtomic) && pNgvInfo->m_bNgvMaxSel;

		bool bNeedQueRegHtIdNonSig = !(bNgvReg && (ngvPortCnt == 2 && pNgvInfo->m_bAllWrPortClk1x && (bNgvAtomic || ngvFieldCnt >= 2)) ||
			bNgvDist && (ngvPortCnt == 1 && !pNgvInfo->m_bAllWrPortClk1x && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel) ||
			bNgvBlock && ngvPortCnt == 1 && (ngvFieldCnt > 1 || bNgvAtomic) && pNgvInfo->m_bNgvMaxSel) ||
			bNgvReg && (ngvPortCnt == 2 && pNgvInfo->m_bAllWrPortClk1x && (bNgvAtomic || ngvFieldCnt >= 2));

		string queRegHtIdSig = bNeedQueRegHtIdSig ? "_sig" : "";

		bool bNeedQueRegCompSig = bNgvDist && ngvPortCnt == 1 && !pNgvInfo->m_bAllWrPortClk1x && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel ||
			bNgvBlock && ngvPortCnt == 1 && (ngvFieldCnt > 1 || bNgvAtomic) && pNgvInfo->m_bNgvMaxSel;

		string queRegCompSig = bNeedQueRegCompSig ? "_sig" : "";

		bool bNeedQueRegDataSig = bNgvReg && (pNgvInfo->m_wrPortList.size() == 2 && pNgvInfo->m_bAllWrPortClk1x) ||
			bNgvDist && ((pNgvInfo->m_wrPortList.size() == 2 && pNgvInfo->m_bAllWrPortClk1x) && !bNgvAtomicSlow ||
			pNgvInfo->m_wrPortList.size() == 1 && !pNgvInfo->m_bAllWrPortClk1x && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel) ||
			bNgvBlock && ((pNgvInfo->m_wrPortList.size() == 2 && pNgvInfo->m_bAllWrPortClk1x) && (!bNgvAtomic && ngvFieldCnt == 1) ||
			ngvPortCnt == 1 && (ngvFieldCnt > 1 || bNgvAtomic) && pNgvInfo->m_bNgvMaxSel);

		bool bNeedAddrComp = bNgvDist && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel && (!pNgvInfo->m_bAllWrPortClk1x || pNgvInfo->m_wrPortList.size() >= 2) ||
			bNgvBlock && (bNgvAtomic || ngvFieldCnt > 1);

		string queRegDataSig = bNeedQueRegDataSig ? "_sig" : "";

		// Declare NGV I/O
		for (size_t modIdx = 0; modIdx < ngvModInfoList.size(); modIdx += 1) {
			CModule & mod = *ngvModInfoList[modIdx].m_pMod;
			CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

			if (pModNgv->m_bWriteForInstrWrite) {
				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					ngvIo.Append("\tsc_in<sc_uint<%s_HTID_W> > i_%sTo%s_iwHtId%s;\n",
						mod.m_modName.Upper().c_str(), mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				}
				ngvIo.Append("\tsc_in<bool> i_%sTo%s_iwComp%s;\n",
					mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				ngvIo.Append("\tsc_in<CGW_%s> i_%sTo%s_iwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(), mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				ngvIo.Append("\n");
			}

			if (pModNgv->m_bReadForInstrRead) {
				if (pNgvInfo->m_atomicMask != 0) {
					ngvIo.Append("\tsc_out<bool> o_%sTo%s_ifWrEn%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
					if (mod.m_threads.m_htIdW.AsInt() > 0) {
						ngvIo.Append("\tsc_out<sc_uint<%s_HTID_W> > o_%sTo%s_ifHtId%s;\n",
							mod.m_modName.Upper().c_str(), pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
					}
					ngvIo.Append("\tsc_out<%s> o_%sTo%s_ifData%s;\n",
						pGv->m_type.c_str(), pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				}
			}

			if (pModNgv->m_bReadForInstrRead || pModNgv->m_bReadForMifWrite) {
				ngvIo.Append("\tsc_out<bool> o_%sTo%s_wrEn%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				if (pGv->m_addrW > 0)
					ngvIo.Append("\tsc_out<ht_uint%d> o_%sTo%s_wrAddr%s;\n",
					pGv->m_addrW, pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				ngvIo.Append("\tsc_out<%s> o_%sTo%s_wrData%s;\n",
					pGv->m_type.c_str(), pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}

			if (pModNgv->m_bWriteForInstrWrite) {
				ngvIo.Append("\tsc_out<bool> o_%sTo%s_iwCompRdy%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					ngvIo.Append("\tsc_out<sc_uint<%s_HTID_W> > o_%sTo%s_iwCompHtId%s;\n",
						mod.m_modName.Upper().c_str(), pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				}
				ngvIo.Append("\tsc_out<bool> o_%sTo%s_iwCompData%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				ngvIo.Append("\n");
			}

			if (pModNgv->m_bWriteForMifRead) {
				if (mod.m_mif.m_mifRd.m_rspGrpW.AsInt() > 0) {
					ngvIo.Append("\tsc_in<ht_uint%d> i_%sTo%s_mwGrpId%s;\n",
						mod.m_mif.m_mifRd.m_rspGrpW.AsInt(), mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				}
				//ngvIo.Append("\tsc_in<bool> i_%sTo%s_mwComp%s;\n",
				//	mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				ngvIo.Append("\tsc_in<CGW_%s> i_%sTo%s_mwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(), mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				if (bNeedQue) {
					ngvIo.Append("\tsc_out<bool> o_%sTo%s_mwFull%s;\n",
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				}
				ngvIo.Append("\n");

				ngvIo.Append("\tsc_out<bool> o_%sTo%s_mwCompRdy%s;\n",
					pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				if (mod.m_mif.m_mifRd.m_rspGrpW.AsInt() > 0) {
					ngvIo.Append("\tsc_out<ht_uint%d> o_%sTo%s_mwCompGrpId%s;\n",
						mod.m_mif.m_mifRd.m_rspGrpW.AsInt(), pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				}
				//ngvIo.Append("\tsc_out<bool> o_%sTo%s_mwCompData%s;\n",
				//	pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				ngvIo.Append("\n");
			}
		}

		// 2x RR selection - one level, 2 or 3 ports, 2x wrData
		bool bRrSel2x = bNgvReg && (pNgvInfo->m_wrPortList.size() == 2 && !pNgvInfo->m_bAllWrPortClk1x || pNgvInfo->m_wrPortList.size() == 3) ||
			bNgvDist && (pNgvInfo->m_wrPortList.size() == 2 && !pNgvInfo->m_bAllWrPortClk1x && !bNgvAtomicSlow || pNgvInfo->m_wrPortList.size() == 3 && !bNgvAtomicSlow) ||
			bNgvBlock && (pNgvInfo->m_wrPortList.size() == 2 && !pNgvInfo->m_bAllWrPortClk1x && !bNgvAtomic && ngvFieldCnt == 1
			|| pNgvInfo->m_wrPortList.size() == 3 && !bNgvAtomic && ngvFieldCnt == 1);

		// 1x RR selection - no phase select, 1x wrData
		bool bRrSel1x = bNgvDist && ngvPortCnt >= 2 && bNgvAtomicSlow && !pNgvInfo->m_bNgvMaxSel ||
			bNgvBlock && ngvPortCnt >= 2 && (bNgvAtomic || ngvFieldCnt >= 2) && !pNgvInfo->m_bNgvMaxSel;

		// 1x RR selection, ports split using phase, 2x wrData
		bool bRrSelAB = bNgvReg && pNgvInfo->m_wrPortList.size() >= 4 ||
			bNgvDist && (pNgvInfo->m_wrPortList.size() >= 4 && !bNgvAtomicSlow) ||
			bNgvBlock && (pNgvInfo->m_wrPortList.size() >= 4 && !bNgvAtomic && ngvFieldCnt == 1);

		bool bRrSelEO = bNgvDist && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel && (!pNgvInfo->m_bAllWrPortClk1x || pNgvInfo->m_wrPortList.size() >= 2) ||
			bNgvBlock && (bNgvAtomic || ngvFieldCnt > 1) && pNgvInfo->m_bNgvMaxSel;

		//
		// generate RR selection signals
		//

		if (bRrSel1x || bRrSel2x) {
			string ngvClkRrSel = bNgvSelGwClk2x ? "_2x" : "";

			CHtCode & ngvPreRegRrSel = bNgvSelGwClk2x ? ngvPreReg_2x : ngvPreReg_1x;
			CHtCode & ngvRegRrSel = bNgvSelGwClk2x ? ngvReg_2x : ngvReg_1x;

			ngvRegDecl.Append("\tht_uint%d c_rrSel%s%s;\n",
				(int)pNgvInfo->m_wrPortList.size(), ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
			GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", (int)pNgvInfo->m_wrPortList.size()),
				VA("r_rrSel%s", ngvClkRrSel.c_str()), pGv->m_dimenList);
			ngvRegDecl.NewLine();

			for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
				CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
				char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';

				ngvRegDecl.Append("\tbool c_t0_%s_%cwRrRdy%s%s;\n",
					mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());

				ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSel%s%s;\n",
					mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
			}
			ngvRegDecl.NewLine();

			{
				string tabs = "\t";
				CLoopInfo loopInfo(ngvPreRegRrSel, tabs, pGv->m_dimenList, 5);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvPreRegRrSel.Append("%sc_rrSel%s%s = r_rrSel%s%s;\n", tabs.c_str(),
						ngvClkRrSel.c_str(), dimIdx.c_str(),
						ngvClkRrSel.c_str(), dimIdx.c_str());
					ngvPreRegRrSel.NewLine();

					string rrSelReset = pNgvInfo->m_bNgvWrCompClk2x ? "c_reset1x" : "r_reset1x";

					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';

						if (bNgvBlock && (bNgvAtomic || ngvFieldCnt > 1)) {
							ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrRdy%s%s = r_%s_%cwWrEn%s%s && (!r_t1_gwWrEn%s%s || r_t1_gwData%s%s.GetAddr() != r_%s_%cwData%s%s.GetAddr());\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								ngvClkRrSel.c_str(), dimIdx.c_str(),
								ngvClkRrSel.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());
						} else {
							ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrRdy%s%s = r_%s_%cwWrEn%s%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());
						}

					}
					ngvPreRegRrSel.NewLine();

					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';

						ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrSel%s%s = c_t0_%s_%cwRrRdy%s%s && ((r_rrSel%s%s & 0x%lx) != 0 || !(\n", tabs.c_str(),
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							ngvClkRrSel.c_str(), dimIdx.c_str(),
							1ul << ngvIdx);

						for (int j = 1; j < ngvPortCnt; j += 1) {
							int k = (ngvIdx + j) % ngvPortCnt;

							uint32_t mask1 = (1ul << ngvPortCnt) - 1;
							uint32_t mask2 = ((1ul << j) - 1) << (ngvIdx + 1);
							uint32_t mask3 = (mask2 & mask1) | (mask2 >> ngvPortCnt);

							char kCh = pNgvInfo->m_wrPortList[k].second == 0 ? 'i' : 'm';

							ngvPreRegRrSel.Append("%s\t(c_t0_%s_%cwRrRdy%s%s && (r_rrSel%s%s & 0x%x) != 0)%s\n", tabs.c_str(),
								ngvModInfoList[pNgvInfo->m_wrPortList[k].first].m_pMod->m_modName.Lc().c_str(), kCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								ngvClkRrSel.c_str(), dimIdx.c_str(),
								mask3,
								j == ngvPortCnt - 1 ? "));\n" : " ||");
						}
					}
				} while (loopInfo.Iter());
			}

			{
				string tabs = "\t";
				CLoopInfo loopInfo(ngvRegRrSel, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					string rrSelReset = pNgvInfo->m_bNgvWrCompClk2x ? "c_reset1x" : "r_reset1x";

					ngvRegRrSel.Append("%sr_rrSel%s%s = %s ? (ht_uint%d)0x1 : c_rrSel%s%s;\n", tabs.c_str(),
						ngvWrDataClk.c_str(), dimIdx.c_str(),
						rrSelReset.c_str(),
						(int)pNgvInfo->m_wrPortList.size(),
						ngvWrDataClk.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}

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
					CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
					int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
					char imCh = imIdx == 0 ? 'i' : 'm';
					string idStr = imIdx == 0 ? "Ht" : "Grp";
					int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

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

					if (idW > 0) {
						ngvRegDecl.Append("\tht_uint%d c_t0_%s_%cw%sId%s%s;\n",
							idW,
							mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", idW),
							VA("r_t1_%s_%cw%sId%s", mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkRrSel.c_str()), pGv->m_dimenList);
						if (bNgvAtomic) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<ht_uint%d>", idW),
								VA("r_t1_%s_%cw%sId%s_sig", mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkRrSel.c_str()), pGv->m_dimenList);
						}
					}

					if (imIdx == 0) {
						ngvRegDecl.Append("\tbool c_t0_%s_%cwComp%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
							VA("r_t1_%s_%cwComp%s", mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str()), pGv->m_dimenList);
					}

					ngvRegDecl.NewLine();
				}

				{
					string tabs = "\t";
					CLoopInfo loopInfo(ngvPreRegRrSel, tabs, pGv->m_dimenList, 5);
					do {
						string dimIdx = loopInfo.IndexStr();

						ngvPreRegRrSel.Append("%sc_rrSel%c%s%s = r_rrSel%c%s%s;\n", tabs.c_str(),
							rngCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							rngCh, ngvClkRrSel.c_str(), dimIdx.c_str());
						ngvPreRegRrSel.NewLine();

						for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
							CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';

							ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrRdy%s%s = r_%s_%cwWrEn%s%s%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str());

						}
						ngvPreRegRrSel.NewLine();

						for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
							CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
							char imCh = imIdx == 0 ? 'i' : 'm';
							string idStr = imIdx == 0 ? "Ht" : "Grp";

							ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrSel%s%s = c_t0_%s_%cwRrRdy%s%s && ((r_rrSel%c%s & 0x%lx) != 0 || !(\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								rngCh, dimIdx.c_str(),
								1ul << (ngvIdx - ngvPortLo));

							for (int j = 1; j < ngvPortRngCnt; j += 1) {
								int k = ngvPortLo + (ngvIdx - ngvPortLo + j) % ngvPortRngCnt;

								uint32_t mask1 = (1ul << ngvPortRngCnt) - 1;
								uint32_t mask2 = ((1ul << j) - 1) << (ngvIdx + 1 - ngvPortLo);
								uint32_t mask3 = (mask2 & mask1) | (mask2 >> ngvPortRngCnt);

								char kCh = pNgvInfo->m_wrPortList[k].second == 0 ? 'i' : 'm';

								ngvPreRegRrSel.Append("%s\t(c_t0_%s_%cwRrRdy%s%s && (r_rrSel%c%s & 0x%x) != 0)%s\n", tabs.c_str(),
									ngvModInfoList[pNgvInfo->m_wrPortList[k].first].m_pMod->m_modName.Lc().c_str(), kCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
									rngCh, dimIdx.c_str(),
									mask3,
									j == ngvPortRngCnt - 1 ? "));\n" : " ||");
							}
						}

					} while (loopInfo.Iter());
				}

				{
					string tabs = "\t";
					CLoopInfo loopInfo(ngvRegRrSel, tabs, pGv->m_dimenList, 5);
					do {
						string dimIdx = loopInfo.IndexStr();

						ngvRegRrSel.Append("%sr_rrSel%c%s%s = r_reset1x ? (ht_uint%d)0x1 : c_rrSel%c%s%s;\n", tabs.c_str(),
							rngCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							ngvPortRngCnt,
							rngCh, ngvClkRrSel.c_str(), dimIdx.c_str());
						ngvRegRrSel.NewLine();

						for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
							CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
							char imCh = imIdx == 0 ? 'i' : 'm';
							string idStr = imIdx == 0 ? "Ht" : "Grp";
							int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

							ngvRegRrSel.Append("%sr_t1_%s_%cwWrEn%s%s = c_t0_%s_%cwRrSel%s%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());

							if (bNgvAtomic) {
								ngvRegRrSel.Append("%sr_t1_%s_%cwWrEn%s_sig%s = c_t0_%s_%cwRrSel%s%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());
							}

							if (idW > 0) {
								ngvRegRrSel.Append("%sr_t1_%s_%cw%sId%s%s = c_t0_%s_%cw%sId%s%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkRrSel.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkRrSel.c_str(), dimIdx.c_str());
							}

							if (bNgvAtomic && idW > 0) {
								ngvRegRrSel.Append("%sr_t1_%s_%cw%sId%s_sig%s = c_t0_%s_%cw%sId%s%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkRrSel.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkRrSel.c_str(), dimIdx.c_str());
							}

							if (imIdx == 0) {
								ngvRegRrSel.Append("%sr_t1_%s_%cwComp%s%s = c_t0_%s_%cwComp%s%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());
							}
							ngvRegRrSel.NewLine();
						}

					} while (loopInfo.Iter());
				}
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
					CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
					int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
					char imCh = imIdx == 0 ? 'i' : 'm';
					string idStr = imIdx == 0 ? "Ht" : "Grp";
					int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

					ngvRegDecl.Append("\tbool c_t0_%s_%cwRrRdy%c%s%s;\n",
						mod.m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());

					ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSel%c%s%s;\n",
						mod.m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());

					if (idW > 0) {
						ngvRegDecl.Append("\tht_uint%d c_t0_%s_%cw%sId%c%s%s;\n",
							idW,
							mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
					}

					if (imIdx == 0) {
						ngvRegDecl.Append("\tbool c_t0_%s_%cwComp%c%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
					}
					ngvRegDecl.NewLine();
				}

				{
					string tabs = "\t";
					CLoopInfo loopInfo(ngvPreRegRrSel, tabs, pGv->m_dimenList, 5);
					do {
						string dimIdx = loopInfo.IndexStr();

						ngvPreRegRrSel.Append("%sc_rrSel%c%s%s = r_rrSel%c%s%s;\n", tabs.c_str(),
							eoCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							eoCh, ngvClkRrSel.c_str(), dimIdx.c_str());
						ngvPreRegRrSel.NewLine();

						for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
							CRam * pModNgv = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pNgv;
							CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
							char imCh = imIdx == 0 ? 'i' : 'm';
							int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();
							bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? (pModNgv->m_bMaxIw && idW > 0) : pModNgv->m_bMaxMw);

							string maxWrStr = bMaxWr ? (eoIdx == 0 ? "E" : "O") : "";

							ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrRdy%c%s%s = r_%s_%cwWrEn%s%s%s && ", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, maxWrStr.c_str(), ngvClkRrSel.c_str(), dimIdx.c_str());

							if (!bMaxWr) {
								ngvPreRegRrSel.Append("(r_%s_%cwData%s.GetAddr() & 1) == %d && ", mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(), eoIdx);
							}

							ngvPreRegRrSel.Append("(!r_t1_gwWrEn%c_sig%s || r_%s_%cwData%s%s.GetAddr() != r_t1_gwData%c_sig%s.read().GetAddr());\n",
								eoCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, maxWrStr.c_str(), dimIdx.c_str(),
								eoCh, dimIdx.c_str());

						}
						ngvPreRegRrSel.NewLine();

						for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
							CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';

							ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrSel%c%s%s = c_t0_%s_%cwRrRdy%c%s%s && ((r_rrSel%c%s & 0x%lx) != 0 || !(\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								eoCh, dimIdx.c_str(),
								1ul << (ngvIdx - ngvPortLo));

							for (int j = 1; j < ngvPortRngCnt; j += 1) {
								int k = ngvPortLo + (ngvIdx - ngvPortLo + j) % ngvPortRngCnt;

								uint32_t mask1 = (1ul << ngvPortRngCnt) - 1;
								uint32_t mask2 = ((1ul << j) - 1) << (ngvIdx + 1 - ngvPortLo);
								uint32_t mask3 = (mask2 & mask1) | (mask2 >> ngvPortRngCnt);

								char kCh = pNgvInfo->m_wrPortList[k].second == 0 ? 'i' : 'm';

								ngvPreRegRrSel.Append("%s\t(c_t0_%s_%cwRrRdy%c%s%s && (r_rrSel%c%s & 0x%x) != 0)%s\n", tabs.c_str(),
									ngvModInfoList[pNgvInfo->m_wrPortList[k].first].m_pMod->m_modName.Lc().c_str(), kCh, eoCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
									eoCh, dimIdx.c_str(),
									mask3,
									j == ngvPortRngCnt - 1 ? "));" : " ||");
							}

							ngvPreRegRrSel.NewLine();
						}

					} while (loopInfo.Iter());
				}

				{
					string tabs = "\t";
					CLoopInfo loopInfo(ngvRegRrSel, tabs, pGv->m_dimenList, 1);
					do {
						string dimIdx = loopInfo.IndexStr();

						ngvRegRrSel.Append("%sr_rrSel%c%s%s = r_reset1x ? (ht_uint%d)0x1 : c_rrSel%c%s%s;\n", tabs.c_str(),
							eoCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							ngvPortRngCnt,
							eoCh, ngvClkRrSel.c_str(), dimIdx.c_str());

					} while (loopInfo.Iter());
				}
			}
		} else if (bNeedQue) {

			string ngvClkRrSel = bNgvSelGwClk2x ? "_2x" : "";

			CHtCode & ngvPreRegRrSel = bNgvSelGwClk2x ? ngvPreReg_2x : ngvPreReg_1x;

			for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
				CRam * pModNgv = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pNgv;
				CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
				int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
				char imCh = imIdx == 0 ? 'i' : 'm';
				int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();
				bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? (pModNgv->m_bMaxIw/* && idW > 0*/) : pModNgv->m_bMaxMw);

				for (int eoIdx = 0; eoIdx < 2; eoIdx += 1) {
					string eoStr;
					if (bRrSelEO && bMaxWr) {
						eoStr = eoIdx == 0 ? "E" : "O";
					} else if (eoIdx == 1)
						continue;

					ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSel%s%s;\n", mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), pGv->m_dimenDecl.c_str());

					string tabs = "\t";
					CLoopInfo loopInfo(ngvPreRegRrSel, tabs, pGv->m_dimenList, 1);
					do {
						string dimIdx = loopInfo.IndexStr();

						if (bRrSelEO) {
							if (idW == 0) {
								ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrSel%s%s = r_%s_%cwWrEn%s%s && (r_%s_%cwData%s%s.read().GetAddr() & 1) == %d && (!r_t1_gwWrEn%s_sig%s || r_%s_%cwData%s%s.read().GetAddr() != r_t1_gwAddr%s_sig%s);\n",
									tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
									eoIdx,
									eoStr.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(),
									eoStr.c_str(), dimIdx.c_str());
							} else {
								ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrSel%s%s = r_%s_%cwWrEn%s%s%s && (!r_t1_gwWrEn%s_sig%s || r_%s_%cwData%s%s%s.read().GetAddr() != r_t1_gwAddr%s_sig%s);\n",
									tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
									eoStr.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), queRegDataSig.c_str(), dimIdx.c_str(),
									eoStr.c_str(), dimIdx.c_str());
							}
						} else if (bNeedAddrComp) {
							ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrSel%s = r_%s_%cwWrEn%s%s && (!r_t1_wrEn%s || r_%s_%cwData%s%s.GetAddr() != r_t1_gwData%s.GetAddr());\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
								dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(), 
								dimIdx.c_str());
						} else {
							ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrSel%s = r_%s_%cwWrEn%s%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str());
						}

					} while (loopInfo.Iter());
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
					if (imIdx == 0 && !pModNgv->m_bWriteForInstrWrite) continue;
					if (imIdx == 1 && !pModNgv->m_bWriteForMifRead) continue;
					char imCh = imIdx == 0 ? 'i' : 'm';
					string idStr = imIdx == 0 ? "Ht" : "Grp";
					int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

					if (idW > 0) {
						ngvRegDecl.Append("\tht_uint%d c_%s_%cw%sId%s%s;\n",
							idW, mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());

						if (bNeedQueRegHtIdNonSig) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", idW),
								VA("r_%s_%cw%sId%s", mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
						}
						if (bNeedQueRegHtIdSig) {
							ngvRegDecl.Append("\tsc_signal<ht_uint%d> r_%s_%cw%sId%s_sig%s;\n",
								idW, mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkWrComp.c_str(), pGv->m_dimenDecl.c_str());
						}
					}

					if (imIdx == 0) {
						ngvRegDecl.Append("\tbool c_%s_%cwComp%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
							VA("r_%s_%cwComp%s", mod.m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str()), pGv->m_dimenList);
					}

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

					{
						string tabs = "\t";
						CLoopInfo loopInfo(ngvPreRegModIn, tabs, pGv->m_dimenList, 4);
						do {
							string dimIdx = loopInfo.IndexStr();
							if (idW > 0) {
								ngvPreRegModIn.Append("%sc_%s_%cw%sId%s%s = i_%sTo%s_%cw%sId%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
							}
							if (imIdx == 0) {
								ngvPreRegModIn.Append("%sc_%s_%cwComp%s%s = i_%sTo%s_%cwComp%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());
							}
							ngvPreRegModIn.Append("%sc_%s_%cwData%s%s = i_%sTo%s_%cwData%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());

							ngvPreRegModIn.Append("%sc_%s_%cwWrEn%s%s = ", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());

							string separator;
							for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
								if (iter.IsStructOrUnion()) continue;

								ngvPreRegModIn.Append("%sc_%s_%cwData%s%s%s.GetWrEn()",
									separator.c_str(), mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
									iter.GetHeirFieldName().c_str());

								separator = VA(" ||\n%s\t", tabs.c_str());

								if (pGv->m_pType->m_eType != eRecord) continue;

								if ((iter->m_atomicMask & ATOMIC_INC) != 0 && imIdx == 0) {
									ngvPreRegModIn.Append(" || c_%s_%cwData%s%s%s.GetIncEn()",
										mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										iter.GetHeirFieldName().c_str());
								}
								if ((iter->m_atomicMask & ATOMIC_SET) != 0 && imIdx == 0) {
									ngvPreRegModIn.Append(" || c_%s_%cwData%s%s%s.GetSetEn()",
										mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										iter.GetHeirFieldName().c_str());
								}
								if ((iter->m_atomicMask & ATOMIC_ADD) != 0 && imIdx == 0) {
									ngvPreRegModIn.Append(" || c_%s_%cwData%s%s%s.GetAddEn()",
										mod.m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										iter.GetHeirFieldName().c_str());
								}
							}

							ngvPreRegModIn.Append(";\n");
						} while (loopInfo.Iter());
					}

					{
						string tabs = "\t";
						CLoopInfo loopInfo(ngvRegSelGw, tabs, pGv->m_dimenList, 4);
						do {
							string dimIdx = loopInfo.IndexStr();
							if (idW > 0) {
								if (bNeedQueRegHtIdNonSig) {
									ngvRegSelGw.Append("%sr_%s_%cw%sId%s%s = c_%s_%cw%sId%s%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrCompClk.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrCompClk.c_str(), dimIdx.c_str());
								}
								if (bNeedQueRegHtIdSig) {
									ngvRegSelGw.Append("%sr_%s_%cw%sId%s_sig%s = c_%s_%cw%sId%s%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrCompClk.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrCompClk.c_str(), dimIdx.c_str());
								}
							}
							if (imIdx == 0) {
								ngvRegSelGw.Append("%sr_%s_%cwComp%s%s = c_%s_%cwComp%s%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str());
							}
							ngvRegSelGw.Append("%sr_%s_%cwData%s%s%s = c_%s_%cwData%s%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), queRegDataSig.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str());

							if (bNeedQueRegWrEnNonSig) {
								ngvRegSelGw.Append("%sr_%s_%cwWrEn%s%s = c_%s_%cwWrEn%s%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str());
							}

							if (bNeedQueRegWrEnSig) {
								ngvRegSelGw.Append("%sr_%s_%cwWrEn%s_sig%s = c_%s_%cwWrEn%s%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str());
							}

						} while (loopInfo.Iter());
					}
				}
			}
		} else {
			for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
				CRam * pModNgv = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pNgv;
				CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
				int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
				char imCh = imIdx == 0 ? 'i' : 'm';
				string idStr = imIdx == 0 ? "Ht" : "Grp";
				int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

				string ngvClkModIn = (mod.m_clkRate == eClk2x || bNgvSelGwClk2x) ? "_2x" : "";
				string ngvClkWrComp = bNgvSelGwClk2x ? "_2x" : "";

				CHtCode & ngvPreRegModIn = (mod.m_clkRate == eClk2x || bNgvSelGwClk2x) ? ngvPreReg_2x : ngvPreReg_1x;
				CHtCode & ngvPreRegWrComp = bNgvSelGwClk2x ? ngvPreReg_2x : ngvPreReg_1x;

				CHtCode & ngvRegModIn = (mod.m_clkRate == eClk2x || bNgvSelGwClk2x) ? ngvReg_2x : ngvReg_1x;
				CHtCode & ngvRegSelGw = bNgvSelGwClk2x ? ngvReg_2x : ngvReg_1x;

				CHtCode & ngvOutModIn = (mod.m_clkRate == eClk2x || bNgvSelGwClk2x) ? ngvOut_2x : ngvOut_1x;

				string ngvRegSelGwReset = pNgvInfo->m_bNgvWrCompClk2x ? "c_reset1x" : "r_reset1x";

				string preSig = mod.m_clkRate == eClk2x && !bNgvSelGwClk2x && idW == 0 && imCh == 'i' ? "sc_signal<" : "";
				string postSig = mod.m_clkRate == eClk2x && !bNgvSelGwClk2x && idW == 0 && imCh == 'i' ? ">" : "";

				if (idW > 0) {
					ngvRegDecl.Append("\tht_uint%d c_%sTo%s_%cw%sId%s%s;\n", 
						idW,
						mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("%sht_uint%d%s", preSig.c_str(), idW, postSig.c_str()),
						VA("r_%sTo%s_%cw%sId%s", mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str()), pGv->m_dimenList);
				}
				if (imIdx == 0) {
					ngvRegDecl.Append("\tbool c_%sTo%s_%cwComp%s%s;\n",
						mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("%sbool%s", preSig.c_str(), postSig.c_str()),
						VA("r_%sTo%s_%cwComp%s", mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str()), pGv->m_dimenList);
				}

				ngvRegDecl.Append("\tCGW_%s c_%sTo%s_%cwData%s%s;\n", 
					pGv->m_pNgvInfo->m_ngvWrType.c_str(),
					mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("%sCGW_%s%s", preSig.c_str(), pGv->m_pNgvInfo->m_ngvWrType.c_str(), postSig.c_str()),
					VA("r_%sTo%s_%cwData%s", mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str()), pGv->m_dimenList);

				ngvRegDecl.Append("\tbool c_%sTo%s_%cwWrEn%s%s;\n",
					mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("%sbool%s", preSig.c_str(), postSig.c_str()),
					VA("r_%sTo%s_%cwWrEn%s", mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str()), pGv->m_dimenList);

				if ((bNeedQue && imIdx == 1)) {
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
						VA("r_%sTo%s_%cwFull%s", pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), imCh, ngvClkModIn.c_str()), pGv->m_dimenList);
				}
				ngvRegDecl.NewLine();

				for (int eoIdx = 0; eoIdx < 2; eoIdx += 1) {
					string eoStr;
					bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? (pModNgv->m_bMaxIw && idW > 0) : pModNgv->m_bMaxMw);
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
					if (idW > 0 || imCh == 'm') {
						string queueW = imIdx == 1 ? "5" : VA("%s_HTID_W", mod.m_modName.Upper().c_str());
						if (idW > 0) {
							ngvRegDecl.Append("\tht_dist_que <ht_uint%d, %s> m_%s_%cw%sIdQue%s%s;\n",
								idW, queueW.c_str(),
								mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), pGv->m_dimenDecl.c_str());
						}
						if (imIdx == 0) {
							ngvRegDecl.Append("\tht_dist_que <bool, %s> m_%s_%cwCompQue%s%s;\n",
								queueW.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), pGv->m_dimenDecl.c_str());
						}
						ngvRegDecl.Append("\tht_dist_que <CGW_%s, %s> m_%s_%cwDataQue%s%s;\n",
							pGv->m_pNgvInfo->m_ngvWrType.c_str(), queueW.c_str(),
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), pGv->m_dimenDecl.c_str());
						ngvRegDecl.NewLine();
					}

					if (idW > 0) {
						ngvRegDecl.Append("\tht_uint%d c_%s_%cw%sId%s%s%s;\n",
							idW, mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str(), pGv->m_dimenDecl.c_str());

						if (bNeedQueRegHtIdSig) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<ht_uint%d>", idW),
								VA("r_%s_%cw%sId%s%s_sig", mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
						} else {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", idW),
								VA("r_%s_%cw%sId%s%s", mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
						}
					}

					if (imIdx == 0) {
						ngvRegDecl.Append("\tbool c_%s_%cwComp%s%s%s;\n",
							mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), pGv->m_dimenDecl.c_str());
						if (bNeedQueRegCompSig) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
								VA("r_%s_%cwComp%s%s_sig", mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
						} else {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
								VA("r_%s_%cwComp%s%s", mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
						}
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

					if (eoIdx == 0) {
						string tabs = "\t";
						CLoopInfo loopInfo(ngvPreRegModIn, tabs, pGv->m_dimenList, 5);
						do {
							string dimIdx = loopInfo.IndexStr();

							ngvPreRegModIn.Append("%sc_%sTo%s_%cwWrEn%s%s = ", tabs.c_str(),
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());

							if (mod.m_clkRate == eClk1x && bNgvSelGwClk2x)
								ngvPreRegModIn.Append("r_phase && (");

							string separator;
							for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
								if (iter.IsStructOrUnion()) continue;

								ngvPreRegModIn.Append("%si_%sTo%s_%cwData%s.read()%s.GetWrEn()",
									separator.c_str(), mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str(),
									iter.GetHeirFieldName().c_str());

								if ((iter.GetAtomicMask() & ATOMIC_INC) != 0 && imCh == 'i') {
									ngvPreRegModIn.Append(" || i_%sTo%s_%cwData%s.read()%s.GetIncEn()",
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str(),
										iter.GetHeirFieldName().c_str());
								}
								if ((iter.GetAtomicMask() & ATOMIC_SET) != 0 && imCh == 'i') {
									ngvPreRegModIn.Append(" || i_%sTo%s_%cwData%s.read()%s.GetSetEn()",
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str(),
										iter.GetHeirFieldName().c_str());
								}
								if ((iter.GetAtomicMask() & ATOMIC_ADD) != 0 && imCh == 'i') {
									ngvPreRegModIn.Append(" || i_%sTo%s_%cwData%s.read()%s.GetAddEn()",
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str(),
										iter.GetHeirFieldName().c_str());
								}
								separator = VA(" ||\n%s\t", tabs.c_str());
							}

							if (mod.m_clkRate == eClk1x && bNgvSelGwClk2x)
								ngvPreRegModIn.Append(")");

							ngvPreRegModIn.Append(";\n");

						} while (loopInfo.Iter());

						ngvPreRegModIn.Append("\n");
					}

					if (eoIdx == 0 || idW > 0 || imCh == 'm') {
						string tabs = "\t";
						CLoopInfo loopInfo(ngvPreRegModIn, tabs, pGv->m_dimenList, 5);
						do {
							string dimIdx = loopInfo.IndexStr();

							string portRrSel;
							if (bRrSelEO && !bMaxWr) {
								portRrSel = VA("(c_t0_%s_%cwRrSelE%s%s || c_t0_%s_%cwRrSelO%s%s)",
									mod.m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str());
							}
							else {
								portRrSel = VA("c_t0_%s_%cwRrSel%s%s%s",
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());
							}

							{
								if (mod.m_clkRate == eClk2x && !bNgvSelGwClk2x && idW == 0 && imCh == 'i') {
									ngvPreRegModIn.Append("%sc_%sTo%s_%cwWrEn%s%s = (r_phase && r_%sTo%s_%cwWrEn%s%s) ?\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
									ngvPreRegModIn.Append("%s\tr_%sTo%s_%cwWrEn%s%s.read() : c_%sTo%s_%cwWrEn%s%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());

									if (idW > 0) {
										ngvPreRegModIn.Append("%sc_%sTo%s_%cw%sId%s%s = (r_phase && r_%sTo%s_%cwWrEn%s%s) ?\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
										ngvPreRegModIn.Append("%s\tr_%sTo%s_%cw%sId%s%s.read() : i_%sTo%s_%cw%sId%s.read()) ?\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
									}
									if (imIdx == 0) {
										ngvPreRegModIn.Append("%sc_%sTo%s_%cwComp%s%s = (r_phase && r_%sTo%s_%cwWrEn%s%s) ?\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
										ngvPreRegModIn.Append("%s\tr_%sTo%s_%cwComp%s%s.read() : i_%sTo%s_%cwComp%s.read();\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());
									}
									ngvPreRegModIn.Append("%sc_%sTo%s_%cwData%s%s = (r_phase && r_%sTo%s_%cwWrEn%s%s) ?\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
									ngvPreRegModIn.Append("%s\tr_%sTo%s_%cwData%s%s.read() : i_%sTo%s_%cwData%s.read();\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());
								}
								else {
									if (idW > 0) {
										ngvPreRegModIn.Append("%sc_%sTo%s_%cw%sId%s%s = i_%sTo%s_%cw%sId%s;\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
									}
									if (imIdx == 0) {
										ngvPreRegModIn.Append("%sc_%sTo%s_%cwComp%s%s = i_%sTo%s_%cwComp%s;\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());
									}
									ngvPreRegModIn.Append("%sc_%sTo%s_%cwData%s%s = i_%sTo%s_%cwData%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());
								}
								ngvPreRegModIn.NewLine();
							}

							if (idW > 0 || imCh == 'm') {
								char rcCh = 'r';
								if (bMaxWr) {
									rcCh = 'c';
									ngvPreRegModIn.Append("%sc_%sTo%s_%cwWrEn%s%s%s = r_%sTo%s_%cwWrEn%s%s && (r_%sTo%s_%cwData%s%s.GetAddr() & 1) == %d;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, eoStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										eoIdx);
									ngvPreRegModIn.NewLine();
								}

								if (bQueBypass) {
									ngvPreRegModIn.Append("%sif (%c_%sTo%s_%cwWrEn%s%s%s && (r_%s_%cwWrEn%s%s%s%s && !%s || !m_%s_%cwDataQue%s%s.empty())) {\n", tabs.c_str(),
										rcCh, mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, eoStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
										portRrSel.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());
								}
								else {
									ngvPreRegModIn.Append("%sif (%c_%sTo%s_%cwWrEn%s%s%s) {\n", tabs.c_str(),
										rcCh, mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, eoStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str());
								}
								if (idW > 0) {
									ngvPreRegModIn.Append("%s\tm_%s_%cw%sIdQue%s%s.push(r_%sTo%s_%cw%sId%s%s);\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str());
								}
								if (imIdx == 0) {
									ngvPreRegModIn.Append("%s\tm_%s_%cwCompQue%s%s.push(r_%sTo%s_%cwComp%s%s);\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
								}
								ngvPreRegModIn.Append("%s\tm_%s_%cwDataQue%s%s.push(r_%sTo%s_%cwData%s%s);\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());

								ngvPreRegModIn.Append("%s}\n", tabs.c_str());
							}
						} while (loopInfo.Iter());

						if (bNeedQue && imIdx == 1) {
							string tabs = "\t";
							CLoopInfo loopInfo(ngvRegModIn, tabs, pGv->m_dimenList, 5);
							do {
								string dimIdx = loopInfo.IndexStr();

								ngvRegModIn.Append("%sr_%sTo%s_%cwFull%s%s = m_%s_%cwDataQue%s%s.size() > 24;\n", tabs.c_str(),
									pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

							} while (loopInfo.Iter());
							ngvRegModIn.NewLine();
						}

						if (bNeedQue && imIdx == 1) {
							string tabs = "\t";
							CLoopInfo loopInfo(ngvOutModIn, tabs, pGv->m_dimenList, 5);
							do {
								string dimIdx = loopInfo.IndexStr();

								ngvOutModIn.Append("%so_%sTo%s_%cwFull%s = r_%sTo%s_%cwFull%s%s;\n", tabs.c_str(),
									pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), imCh, dimIdx.c_str(),
									pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
							} while (loopInfo.Iter());
							ngvOutModIn.NewLine();
						}

					}

					{
						string tabs = "\t";
						CLoopInfo loopInfo(ngvPreRegWrComp, tabs, pGv->m_dimenList, 4);
						do {
							string dimIdx = loopInfo.IndexStr();

							string portRrSel;
							if (bRrSelEO && !bMaxWr) {
								portRrSel = VA("(c_t0_%s_%cwRrSelE%s%s || c_t0_%s_%cwRrSelO%s%s)",
									mod.m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str());
							} else {
								portRrSel = VA("c_t0_%s_%cwRrSel%s%s%s",
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());
							}

							ngvPreRegWrComp.Append("%sif (r_%s_%cwWrEn%s%s%s%s && !%s) {\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
								portRrSel.c_str());

							if (idW > 0) {
								ngvPreRegWrComp.Append("%s\tc_%s_%cw%sId%s%s%s = r_%s_%cw%sId%s%s%s%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
							}
							if (imIdx == 0) {
								ngvPreRegWrComp.Append("%s\tc_%s_%cwComp%s%s%s = r_%s_%cwComp%s%s%s%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegCompSig.c_str(), dimIdx.c_str());
							}
							ngvPreRegWrComp.Append("%s\tc_%s_%cwData%s%s%s = r_%s_%cwData%s%s%s%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegDataSig.c_str(), dimIdx.c_str());

							ngvPreRegWrComp.Append("%s} else {\n", tabs.c_str());

							if (idW > 0 || imCh == 'm') {
								if (bQueBypass) {
									if (idW > 0) {
										ngvPreRegWrComp.Append("%s\tc_%s_%cw%sId%s%s%s = m_%s_%cw%sIdQue%s%s.empty() ? r_%sTo%s_%cw%sId%s%s : m_%s_%cw%sIdQue%s%s.front();\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), dimIdx.c_str());
									}
									if (imIdx == 0) {
										ngvPreRegWrComp.Append("%s\tc_%s_%cwComp%s%s%s = m_%s_%cwCompQue%s%s.empty() ? r_%sTo%s_%cwComp%s%s : m_%s_%cwCompQue%s%s.front();\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());
									}
									ngvPreRegWrComp.Append("%s\tc_%s_%cwData%s%s%s = m_%s_%cwDataQue%s%s.empty() ? r_%sTo%s_%cwData%s%s : m_%s_%cwDataQue%s%s.front();\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());
								} else {
									if (idW > 0) {
										ngvPreRegWrComp.Append("%s\tc_%s_%cw%sId%s%s%s = m_%s_%cw%sIdQue%s%s.front();\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), dimIdx.c_str());
									}
									if (imIdx == 0) {
										ngvPreRegWrComp.Append("%s\tc_%s_%cwComp%s%s%s = m_%s_%cwCompQue%s%s.front();\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());
									}
									ngvPreRegWrComp.Append("%s\tc_%s_%cwData%s%s%s = m_%s_%cwDataQue%s%s.front();\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());
								}
							} else {
								if (imIdx == 0) {
									ngvPreRegWrComp.Append("%s\tc_%s_%cwComp%s%s%s = r_%sTo%s_%cwComp%s%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
								}
								ngvPreRegWrComp.Append("%s\tc_%s_%cwData%s%s%s = r_%sTo%s_%cwData%s%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
							}
							ngvPreRegWrComp.Append("%s}\n", tabs.c_str());

							if (idW > 0 || imCh == 'm') {
								if (bQueBypass) {
									char rcCh = eoStr.size() > 0 ? 'c' : 'r';
									ngvPreRegWrComp.Append("%sc_%s_%cwWrEn%s%s%s = !m_%s_%cwDataQue%s%s.empty() || %c_%sTo%s_%cwWrEn%s%s%s || r_%s_%cwWrEn%s%s%s%s && !%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
										rcCh, mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
										portRrSel.c_str());
								} else {
									ngvPreRegWrComp.Append("%sc_%s_%cwWrEn%s%s%s = !m_%s_%cwDataQue%s%s.empty() || r_%s_%cwWrEn%s%s%s%s && !%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
										portRrSel.c_str());
								}
							} else {
								ngvPreRegWrComp.Append("%sc_%s_%cwWrEn%s%s%s = r_%sTo%s_%cwWrEn%s%s%s || r_%s_%cwWrEn%s%s%s%s && !%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, eoStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
									portRrSel.c_str());
							}

							if (idW > 0 || imCh == 'm') {
								ngvPreRegWrComp.NewLine();
								ngvPreRegWrComp.Append("%sif ((!r_%s_%cwWrEn%s%s%s%s || %s) && !m_%s_%cwDataQue%s%s.empty()) {\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
									portRrSel.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

								if (idW > 0) {
									ngvPreRegWrComp.Append("%s\tm_%s_%cw%sIdQue%s%s.pop();\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), dimIdx.c_str());
								}
								if (imIdx == 0) {
									ngvPreRegWrComp.Append("%s\tm_%s_%cwCompQue%s%s.pop();\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());
								}
								ngvPreRegWrComp.Append("%s\tm_%s_%cwDataQue%s%s.pop();\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

								ngvPreRegWrComp.Append("%s}\n", tabs.c_str());
							}
						} while (loopInfo.Iter());
					}

					if (eoIdx == 0) {
						string tabs = "\t";
						CLoopInfo loopInfo(ngvRegModIn, tabs, pGv->m_dimenList, 4);
						do {
							string dimIdx = loopInfo.IndexStr();

							ngvRegModIn.Append("%sr_%sTo%s_%cwWrEn%s%s = c_%sTo%s_%cwWrEn%s%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
							if (idW > 0) {
								ngvRegModIn.Append("%sr_%sTo%s_%cw%sId%s%s = c_%sTo%s_%cw%sId%s%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str());
							}
							if (imIdx == 0) {
								ngvRegModIn.Append("%sr_%sTo%s_%cwComp%s%s = c_%sTo%s_%cwComp%s%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
							}
							ngvRegModIn.Append("%sr_%sTo%s_%cwData%s%s = c_%sTo%s_%cwData%s%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());

						} while (loopInfo.Iter());
					}

					if (idW > 0 || imCh =='m') {
						string tabs = "\t";
						CLoopInfo loopInfo(ngvRegSelGw, tabs, pGv->m_dimenList, 4);
						do {
							string dimIdx = loopInfo.IndexStr();

							if (bQueBypass) {
								if (idW > 0) {
									ngvRegSelGw.Append("%sm_%s_%cw%sIdQue%s%s.clock(%s);\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), dimIdx.c_str(),
										ngvRegSelGwReset.c_str());
								}
								if (imIdx == 0) {
									ngvRegSelGw.Append("%sm_%s_%cwCompQue%s%s.clock(%s);\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
										ngvRegSelGwReset.c_str());
								}
								ngvRegSelGw.Append("%sm_%s_%cwDataQue%s%s.clock(%s);\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
									ngvRegSelGwReset.c_str());
							} else {
								if (idW > 0) {
									ngvRegSelGw.Append("%sm_%s_%cw%sIdQue%s%s.pop_clock(%s);\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), dimIdx.c_str(),
										ngvRegSelGwReset.c_str());
								}
								if (imIdx == 0) {
									ngvRegSelGw.Append("%sm_%s_%cwCompQue%s%s.pop_clock(%s);\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
										ngvRegSelGwReset.c_str());
								}

								ngvRegSelGw.Append("%sm_%s_%cwDataQue%s%s.pop_clock(%s);\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
									ngvRegSelGwReset.c_str());
							}

						} while (loopInfo.Iter());
					}

					if ((idW > 0 || imCh == 'm') && !bQueBypass) {
						string tabs = "\t";
						CLoopInfo loopInfo(ngvReg_2x, tabs, pGv->m_dimenList, 3);
						do {
							string dimIdx = loopInfo.IndexStr();

							if (idW > 0) {
								ngvReg_2x.Append("%sm_%s_%cw%sIdQue%s%s.push_clock(c_reset1x);\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), dimIdx.c_str());
							}
							if (imIdx == 0) {
								ngvReg_2x.Append("%sm_%s_%cwCompQue%s%s.push_clock(c_reset1x);\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());
							}
							ngvReg_2x.Append("%sm_%s_%cwDataQue%s%s.push_clock(c_reset1x);\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

						} while (loopInfo.Iter());
					}

					{
						string tabs = "\t";
						CLoopInfo loopInfo(ngvRegSelGw, tabs, pGv->m_dimenList, 3);
						do {
							string dimIdx = loopInfo.IndexStr();

							if (idW > 0) {
								ngvRegSelGw.Append("%sr_%s_%cw%sId%s%s%s%s = c_%s_%cw%sId%s%s%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str(), queRegHtIdSig.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());
							}
							if (imIdx == 0) {
								ngvRegSelGw.Append("%sr_%s_%cwComp%s%s%s%s = c_%s_%cwComp%s%s%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegCompSig.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());
							}
							ngvRegSelGw.Append("%sr_%s_%cwData%s%s%s%s = c_%s_%cwData%s%s%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegDataSig.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());

							string wrCompReset = bNgvSelGwClk2x ? "c_reset1x" : "r_reset1x";

							ngvRegSelGw.Append("%sr_%s_%cwWrEn%s%s%s%s = !%s && c_%s_%cwWrEn%s%s%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
								wrCompReset.c_str(),
								mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());

						} while (loopInfo.Iter());
					}
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
				CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
				int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
				char imCh = imIdx == 0 ? 'i' : 'm';
				string idStr = imIdx == 0 ? "Ht" : "Grp";
				int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

				ngvRegDecl.Append("\tbool c_t0_%s_%cwWrEn%s;\n",
					mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
					VA("r_t1_%s_%cwWrEn", mod.m_modName.Lc().c_str(), imCh), pGv->m_dimenList);

				if (idW > 0) {
					ngvRegDecl.Append("\tht_uint%d c_t0_%s_%cw%sId%s;\n",
						idW,
						mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", idW),
						VA("r_t1_%s_%cw%sId", mod.m_modName.Lc().c_str(), imCh, idStr.c_str()), pGv->m_dimenList);
				}

				if (imIdx == 0) {
					ngvRegDecl.Append("\tbool c_t0_%s_%cwComp%s;\n",
						mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
						VA("r_t1_%s_%cwComp", mod.m_modName.Lc().c_str(), imCh), pGv->m_dimenList);
				}
				ngvRegDecl.NewLine();
			}


			GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("CGW_%s", pNgvInfo->m_ngvWrType.c_str()),
				"r_t1_gwData", pGv->m_dimenList);
			ngvRegDecl.NewLine();

			{
				string tabs = "\t";
				CLoopInfo loopInfo(ngvPreRegRrSel, tabs, pGv->m_dimenList, 10);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvPreRegRrSel.Append("%sc_t0_gwWrEn%s = false;\n", tabs.c_str(), dimIdx.c_str());
					ngvPreRegRrSel.Append("%sc_t0_gwData%s = 0;\n", tabs.c_str(), dimIdx.c_str());
					ngvPreRegRrSel.NewLine();

					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

						ngvPreRegRrSel.Append("%sc_t0_%s_%cwWrEn%s = c_t0_%s_%cwRrSel%s;\n", tabs.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						if (idW > 0) {
							ngvPreRegRrSel.Append("%sc_t0_%s_%cw%sId%s = r_%s_%cw%sId%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
						}
						if (imIdx == 0) {
							ngvPreRegRrSel.Append("%sc_t0_%s_%cwComp%s = r_%s_%cwComp%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
						}
						ngvPreRegRrSel.NewLine();
					}

					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';

						ngvPreRegRrSel.Append("%sif (c_t0_%s_%cwRrSel%s) {\n", tabs.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						ngvPreRegRrSel.Append("%s\tc_rrSel%s = 0x%x;\n", tabs.c_str(),
							dimIdx.c_str(),
							1 << ((ngvIdx + 1) % ngvPortCnt));

						ngvPreRegRrSel.Append("%s\tc_t0_gwWrEn%s = true;\n", tabs.c_str(), dimIdx.c_str());

						ngvPreRegRrSel.Append("%s\tc_t0_gwData%s = r_%s_%cwData%s;\n", tabs.c_str(),
							dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						ngvPreRegRrSel.Append("%s}\n", tabs.c_str());
					}
					ngvPreRegRrSel.NewLine();

					ngvPreRegRrSel.Append("%sc_t1_gwWrEn%s = r_t1_gwWrEn%s;\n", tabs.c_str(),
						dimIdx.c_str(),
						dimIdx.c_str());

					ngvPreRegRrSel.Append("%sc_t1_gwData%s = r_t1_gwData%s;\n", tabs.c_str(),
						dimIdx.c_str(),
						dimIdx.c_str());

					ngvPreRegRrSel.NewLine();

				} while (loopInfo.Iter());
			}

			{
				string tabs = "\t";
				CLoopInfo loopInfo(ngvRegRrSel, tabs, pGv->m_dimenList, 10);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvRegRrSel.Append("%sr_t1_gwWrEn%s = c_t0_gwWrEn%s;\n", tabs.c_str(),
						dimIdx.c_str(),
						dimIdx.c_str());

					ngvRegRrSel.Append("%sr_t1_gwData%s = c_t0_gwData%s;\n", tabs.c_str(),
						dimIdx.c_str(),
						dimIdx.c_str());

					ngvRegRrSel.NewLine();

					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

						ngvRegRrSel.Append("%sr_t1_%s_%cwWrEn%s = c_t0_%s_%cwWrEn%s;\n", tabs.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						if (idW > 0) {
							ngvRegRrSel.Append("%sr_t1_%s_%cw%sId%s = c_t0_%s_%cw%sId%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
						}
						if (imIdx == 0) {
							ngvRegRrSel.Append("%sr_t1_%s_%cwComp%s = c_t0_%s_%cwComp%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
						}
						ngvRegRrSel.NewLine();
					}

				} while (loopInfo.Iter());
			}

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

			for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
				CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
				int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
				char imCh = imIdx == 0 ? 'i' : 'm';
				string idStr = imIdx == 0 ? "Ht" : "Grp";
				int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
					VA("r_t1_%s_%cwWrEn%s", mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str()), pGv->m_dimenList);

				if (idW > 0) {
					ngvRegDecl.Append("\tht_uint%d c_t0_%s_%cw%sId_2x%s;\n",
						idW,
						mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", idW),
						VA("r_t1_%s_%cw%sId%s", mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrDataClk.c_str()), pGv->m_dimenList);
				}

				if (imIdx == 0) {
					ngvRegDecl.Append("\tbool c_t0_%s_%cwComp_2x%s;\n",
						mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
						VA("r_t1_%s_%cwComp%s", mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str()), pGv->m_dimenList);
				}
				ngvRegDecl.NewLine();
			}

			if ((ngvFieldCnt != 1 || bNgvAtomic) && !bNgvReg) {

				ngvRegDecl.Append("\tbool c_t%d_gwWrEn%s%s;\n",
					stgIdx + 1, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

				ngvRegDecl.Append("\tCGW_%s c_t%d_gwData%s%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(),
					stgIdx + 1, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				ngvRegDecl.NewLine();
			}

			{
				string tabs = "\t";
				CLoopInfo loopInfo(ngvPreReg_2x, tabs, pGv->m_dimenList, 10);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvPreReg_2x.Append("%sc_t%d_gwWrEn_2x%s = false;\n", tabs.c_str(), stgIdx, dimIdx.c_str());

					ngvPreReg_2x.Append("%sc_t%d_gwData_2x%s = 0;\n", tabs.c_str(), stgIdx, dimIdx.c_str());
					ngvPreReg_2x.NewLine();

					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

						if (idW > 0) {
							ngvPreReg_2x.Append("%sc_t0_%s_%cw%sId_2x%s = r_%s_%cw%sId_2x%s%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
						}

						if (imIdx == 0) {
							ngvPreReg_2x.Append("%sc_t0_%s_%cwComp_2x%s = r_%s_%cwComp_2x%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
						}
						ngvPreReg_2x.NewLine();
					}

					for (size_t ngvIdx = 0; ngvIdx < pNgvInfo->m_wrPortList.size(); ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';

						ngvPreReg_2x.Append("%sif (c_t0_%s_%cwRrSel_2x%s) {\n", tabs.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
						ngvPreReg_2x.Append("%s\tc_rrSel_2x%s = 0x%x;\n", tabs.c_str(),
							dimIdx.c_str(), 1 << (int)((ngvIdx + 1) % pNgvInfo->m_wrPortList.size()));
						ngvPreReg_2x.Append("%s\tc_t%d_gwWrEn_2x%s = true;\n", tabs.c_str(), stgIdx, dimIdx.c_str());

						ngvPreReg_2x.Append("%s\tc_t%d_gwData_2x%s = r_%s_%cwData_2x%s;\n", tabs.c_str(),
							stgIdx, dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						ngvPreReg_2x.Append("%s}\n", tabs.c_str());
					}
					ngvPreReg_2x.NewLine();

					if (ngvFieldCnt == 1 && !bNgvAtomic) {
						ngvPreReg_2x.Append("%sc_t%d_wrEn_2x%s = c_t%d_gwWrEn_2x%s;\n", tabs.c_str(),
							stgIdx, dimIdx.c_str(),
							stgIdx, dimIdx.c_str());

						if (pGv->m_addrW > 0) {
							ngvPreReg_2x.Append("%sc_t%d_wrAddr_2x%s = c_t%d_gwData_2x%s.GetAddr();\n", tabs.c_str(),
								stgIdx, dimIdx.c_str(),
								stgIdx, dimIdx.c_str());
						}

						for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
							if (iter.IsStructOrUnion()) continue;

							ngvPreReg_2x.Append("%sc_t%d_wrData_2x%s%s = c_t%d_gwData_2x%s%s.GetData();\n", tabs.c_str(),
								stgIdx, dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
								stgIdx, dimIdx.c_str(), iter.GetHeirFieldName().c_str());
						}
					} else if (!bNgvReg) {

						ngvPreReg_2x.Append("%sc_t%d_gwWrEn_2x%s = r_t%d_gwWrEn_2x%s;\n", tabs.c_str(),
							stgIdx + 1, dimIdx.c_str(),
							stgIdx + 1, dimIdx.c_str());

						ngvPreReg_2x.Append("%sc_t%d_gwData_2x%s = r_t%d_gwData_2x%s;\n", tabs.c_str(),
							stgIdx + 1, dimIdx.c_str(),
							stgIdx + 1, dimIdx.c_str());
						ngvPreReg_2x.NewLine();
					}
				} while (loopInfo.Iter());
			}

			{
				string tabs = "\t";
				CLoopInfo loopInfo(ngvReg_2x, tabs, pGv->m_dimenList, 10);
				do {
					string dimIdx = loopInfo.IndexStr();

					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

						ngvReg_2x.Append("%sr_t1_%s_%cwWrEn_2x%s = c_t0_%s_%cwRrSel_2x%s;\n", tabs.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						if (idW > 0) {
							ngvReg_2x.Append("%sr_t1_%s_%cw%sId_2x%s = c_t0_%s_%cw%sId_2x%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
						}

						if (imIdx == 0) {
							ngvReg_2x.Append("%sr_t1_%s_%cwComp_2x%s = c_t0_%s_%cwComp_2x%s;\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
						}
						ngvReg_2x.NewLine();
					}

					if ((ngvFieldCnt != 1 || bNgvAtomic) && !bNgvReg) {

						ngvReg_2x.Append("%sr_t%d_gwWrEn_2x%s = c_t%d_gwWrEn_2x%s;\n", tabs.c_str(),
							stgIdx + 1, dimIdx.c_str(),
							stgIdx, dimIdx.c_str());

						ngvReg_2x.Append("%sr_t%d_gwData_2x%s = c_t%d_gwData_2x%s;\n", tabs.c_str(),
							stgIdx + 1, dimIdx.c_str(),
							stgIdx, dimIdx.c_str());
						ngvReg_2x.NewLine();
					}
				} while (loopInfo.Iter());
			}

			if ((ngvFieldCnt != 1 || bNgvAtomic) && !bNgvReg)
				stgIdx += 1;

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

				{
					string tabs = "\t";
					CLoopInfo loopInfo(ngvPreRegRrSel, tabs, pGv->m_dimenList, 10);
					do {
						string dimIdx = loopInfo.IndexStr();

						ngvPreRegRrSel.Append("%sc_t0_gwWrEn%c%s = false;\n", tabs.c_str(), rngCh, dimIdx.c_str());
						ngvPreRegRrSel.Append("%sc_t0_gwData%c%s = 0;\n", tabs.c_str(), rngCh, dimIdx.c_str());
						ngvPreRegRrSel.NewLine();

						for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
							CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
							char imCh = imIdx == 0 ? 'i' : 'm';
							string idStr = imIdx == 0 ? "Ht" : "Grp";
							int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

							if (idW > 0) {
								ngvPreRegRrSel.Append("%sc_t0_%s_%cw%sId%s = r_%s_%cw%sId%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
							}
							if (imIdx == 0) {
								ngvPreRegRrSel.Append("%sc_t0_%s_%cwComp%s = r_%s_%cwComp%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
							}
							ngvPreRegRrSel.NewLine();
						}

						for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
							CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';

							ngvPreRegRrSel.Append("%sif (c_t0_%s_%cwRrSel%s) {\n", tabs.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

							ngvPreRegRrSel.Append("%s\tc_rrSel%c%s = 0x%x;\n", tabs.c_str(),
								rngCh, dimIdx.c_str(),
								1 << ((ngvIdx + 1 - ngvPortLo) % ngvPortRngCnt));

							ngvPreRegRrSel.Append("%s\tc_t0_gwWrEn%c%s = true;\n", tabs.c_str(), rngCh, dimIdx.c_str());
							ngvPreRegRrSel.Append("%s\tc_t0_gwData%c%s = r_%s_%cwData%s;\n", tabs.c_str(),
								rngCh, dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

							ngvPreRegRrSel.Append("%s}\n", tabs.c_str());
						}
					} while (loopInfo.Iter());
				}

				{
					string tabs = "\t";
					CLoopInfo loopInfo(ngvRegRrSel, tabs, pGv->m_dimenList, 2);
					do {
						string dimIdx = loopInfo.IndexStr();
						ngvRegRrSel.Append("%sr_t1_gwWrEn%c_sig%s = c_t0_gwWrEn%c%s;\n", tabs.c_str(),
							rngCh, dimIdx.c_str(),
							rngCh, dimIdx.c_str());

						ngvRegRrSel.Append("%sr_t1_gwData%c_sig%s = c_t0_gwData%c%s;\n", tabs.c_str(),
							rngCh, dimIdx.c_str(),
							rngCh, dimIdx.c_str());

					} while (loopInfo.Iter());
				}
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

				string tabs = "\t\t";
				CLoopInfo loopInfo(ngvPreReg_2x, tabs, pGv->m_dimenList, 2);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvPreReg_2x.Append("%sc_t1_gwWrEn_2x%s = r_t1_gwWrEn%c_sig%s;\n", tabs.c_str(),
						dimIdx.c_str(),
						rngCh, dimIdx.c_str());

					ngvPreReg_2x.Append("%sc_t1_gwData_2x%s = r_t1_gwData%c_sig%s;\n", tabs.c_str(),
						dimIdx.c_str(),
						rngCh, dimIdx.c_str());

				} while (loopInfo.Iter(false));
			}
			ngvPreReg_2x.Append("\t}\n");
			ngvPreReg_2x.NewLine();

			if (ngvFieldCnt == 1 && !bNgvAtomic) {

				string tabs = "\t";
				CLoopInfo loopInfo(ngvPreReg_2x, tabs, pGv->m_dimenList, 10);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvPreReg_2x.Append("%sc_t1_wrEn_2x%s = c_t1_gwWrEn_2x%s;\n", tabs.c_str(),
						dimIdx.c_str(),
						dimIdx.c_str());

					if (pGv->m_addrW > 0) {
						ngvPreReg_2x.Append("%sc_t1_wrAddr_2x%s = c_t1_gwData_2x%s.GetAddr();\n", tabs.c_str(),
							dimIdx.c_str(),
							dimIdx.c_str());
					}

					for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
						if (iter.IsStructOrUnion()) continue;

						ngvPreReg_2x.Append("%sc_t1_wrData_2x%s%s = c_t1_gwData_2x%s%s.GetData();\n", tabs.c_str(),
							dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
							dimIdx.c_str(), iter.GetHeirFieldName().c_str());
					}

				} while (loopInfo.Iter());
			}

			stgIdx += 1;
			wrCompStg = "t1_";

		} else if (bRrSelEO) {
			if (pNgvInfo->m_wrPortList.size() == 1) {
				// no RR, just prepare for next stage inputs

				for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
					//CRam * pModNgv = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pNgv;
					CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
					int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
					char imCh = imIdx == 0 ? 'i' : 'm';
					int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

					for (int eoIdx = 0; eoIdx < 2; eoIdx += 1) {
						string eoStr;
						//bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? (pModNgv->m_bMaxIw && idW > 0) : pModNgv->m_bMaxMw);
						if (bRrSelEO /*&& bMaxWr*/) {
							eoStr = eoIdx == 0 ? "E" : "O";
						} else if (eoIdx == 1)
							continue;

						ngvRegDecl.Append("\tbool c_t0_gwWrEn%s%s;\n", eoStr.c_str(), pGv->m_dimenDecl.c_str());
						if (pGv->m_addrW > 0) {
							ngvRegDecl.Append("\tht_uint%d c_t0_gwAddr%s%s;\n",
								pGv->m_addrW, eoStr.c_str(), pGv->m_dimenDecl.c_str());
						}
						ngvRegDecl.NewLine();

						string tabs = "\t";
						CLoopInfo loopInfo(ngvPreReg_1x, tabs, pGv->m_dimenList, 10);
						do {
							string dimIdx = loopInfo.IndexStr();

							if (idW == 0) {
								ngvPreReg_1x.Append("%sc_t0_gwWrEn%s%s = r_%s_%cwWrEn%s%s;\n", tabs.c_str(),
									eoStr.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str());

								if (pGv->m_addrW > 0) {
									ngvPreReg_1x.Append("%sc_t0_gwAddr%s%s = r_%s_%cwData%s%s.read().GetAddr();\n", tabs.c_str(),
										eoStr.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());
								}
							} else {
								ngvPreReg_1x.Append("%sc_t0_gwWrEn%s%s = r_%s_%cwWrEn%s%s%s;\n", tabs.c_str(),
									eoStr.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str());

								if (pGv->m_addrW > 0) {
									ngvPreReg_1x.Append("%sc_t0_gwAddr%s%s = r_%s_%cwData%s%s%s.read().GetAddr();\n", tabs.c_str(),
										eoStr.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoStr.c_str(), queRegDataSig.c_str(), dimIdx.c_str());
								}
							}

						} while (loopInfo.Iter());
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
					if (pGv->m_addrW > 0) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<ht_uint%d>", pGv->m_addrW),
							VA("r_t1_gwAddr%s_sig", eoStr.c_str()), pGv->m_dimenList);
					}
					ngvRegDecl.NewLine();

					string tabs = "\t";
					CLoopInfo loopInfo(ngvReg_1x, tabs, pGv->m_dimenList, 10);
					do {
						string dimIdx = loopInfo.IndexStr();

						ngvReg_1x.Append("%sr_t1_gwWrEn%s_sig%s = c_t0_gwWrEn%s%s;\n", tabs.c_str(),
							eoStr.c_str(), dimIdx.c_str(),
							eoStr.c_str(), dimIdx.c_str());

						if (pGv->m_addrW > 0) {
							ngvReg_1x.Append("%sr_t1_gwAddr%s_sig%s = c_t0_gwAddr%s%s;\n", tabs.c_str(),
								eoStr.c_str(), dimIdx.c_str(),
								eoStr.c_str(), dimIdx.c_str());
						}

					} while (loopInfo.Iter());
				}

				// generate phase selection
				{
					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';
						int idW = imCh == 'i' ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

						ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSelE_2x%s;\n",
							mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());

						ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSelO_2x%s;\n",
							mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());

						string tabs = "\t";
						CLoopInfo loopInfo(ngvPreReg_2x, tabs, pGv->m_dimenList, 10);
						do {
							string dimIdx = loopInfo.IndexStr();

							if (idW == 0) {
								ngvPreReg_2x.Append("%sc_t0_%s_%cwRrSelE_2x%s = r_%s_%cwWrEn%s%s && (r_%s_%cwData%s%s.read().GetAddr() & 1) == 0 && (!r_t1_gwWrEnE_sig%s || r_%s_%cwData%s%s.read().GetAddr() != r_t1_gwAddrE_sig%s);\n", 
									tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
									dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(),
									dimIdx.c_str());

								ngvPreReg_2x.Append("%sc_t0_%s_%cwRrSelO_2x%s = r_%s_%cwWrEn%s%s && (r_%s_%cwData%s%s.read().GetAddr() & 1) == 1 && (!r_t1_gwWrEnO_sig%s || r_%s_%cwData%s%s.read().GetAddr() != r_t1_gwAddrO_sig%s);\n", 
									tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
									dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(),
									dimIdx.c_str());
							} else {
								ngvPreReg_2x.Append("%sc_t0_%s_%cwRrSelE_2x%s = r_%s_%cwWrEnE%s%s && (!r_t1_gwWrEnE_sig%s || r_%s_%cwDataE%s%s.read().GetAddr() != r_t1_gwAddrE_sig%s);\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
									dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(),
									dimIdx.c_str());

								ngvPreReg_2x.Append("%sc_t0_%s_%cwRrSelO_2x%s = r_%s_%cwWrEnO%s%s && (!r_t1_gwWrEnO_sig%s || r_%s_%cwDataO%s%s.read().GetAddr() != r_t1_gwAddrO_sig%s);\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
									dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(),
									dimIdx.c_str());
							}
						} while (loopInfo.Iter());
					}
					ngvPreReg_2x.NewLine();

					ngvRegDecl.Append("\tbool c_t0_gwWrEn_2x%s;\n",
						pGv->m_dimenDecl.c_str());

					ngvRegDecl.Append("\tCGW_%s c_t0_gwData_2x%s;\n",
						pNgvInfo->m_ngvWrType.c_str(), pGv->m_dimenDecl.c_str());

					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

						ngvRegDecl.Append("\tbool c_t0_%s_%cwWrEn_2x%s;\n",
							mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());

						if (idW > 0) {
							ngvRegDecl.Append("\tht_uint%d c_t0_%s_%cw%sId_2x%s;\n",
								idW,
								mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), pGv->m_dimenDecl.c_str());
						}

						if (imIdx == 0) {
							ngvRegDecl.Append("\tbool c_t0_%s_%cwComp_2x%s;\n",
								mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
						}
						ngvRegDecl.NewLine();
					}

					{
						string tabs = "\t";
						CLoopInfo loopInfo(ngvPreReg_2x, tabs, pGv->m_dimenList, 10);
						do {
							string dimIdx = loopInfo.IndexStr();

							ngvPreReg_2x.Append("%sif (r_phase) {\n", tabs.c_str());

							for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
								CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
								int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
								char imCh = imIdx == 0 ? 'i' : 'm';
								string idStr = imIdx == 0 ? "Ht" : "Grp";
								int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

								ngvPreReg_2x.Append("%s\tc_t0_gwWrEn_2x%s = c_t0_%s_%cwRrSelE_2x%s;\n", tabs.c_str(),
									dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
								if (idW == 0) {
									ngvPreReg_2x.Append("%s\tc_t0_gwData_2x%s = r_%s_%cwData%s%s;\n", tabs.c_str(),
										dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());
								} else {
									ngvPreReg_2x.Append("%s\tc_t0_gwData_2x%s = r_%s_%cwDataE%s%s;\n", tabs.c_str(),
										dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());
								}
								ngvPreReg_2x.NewLine();

								ngvPreReg_2x.Append("%s\tc_t0_%s_%cwWrEn_2x%s = c_t0_%s_%cwRrSelE_2x%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

								if (idW > 0) {
									ngvPreReg_2x.Append("%s\tc_t0_%s_%cw%sId_2x%s = r_%s_%cw%sIdE%s%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
								}
								if (imIdx == 0) {
									if (idW == 0) {
										ngvPreReg_2x.Append("%s\tc_t0_%s_%cwComp_2x%s = r_%s_%cwComp%s%s;\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, queRegCompSig.c_str(), dimIdx.c_str());
									} else {
										ngvPreReg_2x.Append("%s\tc_t0_%s_%cwComp_2x%s = r_%s_%cwCompE%s%s;\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, queRegCompSig.c_str(), dimIdx.c_str());
									}
								}
							}

							ngvPreReg_2x.Append("%s} else {\n", tabs.c_str());

							for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
								CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
								int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
								char imCh = imIdx == 0 ? 'i' : 'm';
								string idStr = imIdx == 0 ? "Ht" : "Grp";
								int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

								ngvPreReg_2x.Append("%s\tc_t0_gwWrEn_2x%s = c_t0_%s_%cwRrSelO_2x%s;\n", tabs.c_str(),
									dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
								if (idW == 0) {
									ngvPreReg_2x.Append("%s\tc_t0_gwData_2x%s = r_%s_%cwData%s%s;\n", tabs.c_str(),
										dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());
								} else {
									ngvPreReg_2x.Append("%s\tc_t0_gwData_2x%s = r_%s_%cwDataO%s%s;\n", tabs.c_str(),
										dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());
								}
								ngvPreReg_2x.NewLine();

								ngvPreReg_2x.Append("%s\tc_t0_%s_%cwWrEn_2x%s = c_t0_%s_%cwRrSelO_2x%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

								if (idW > 0) {
									ngvPreReg_2x.Append("%s\tc_t0_%s_%cw%sId_2x%s = r_%s_%cw%sIdO%s%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
								}
								if (imIdx == 0) {
									if (idW == 0) {
										ngvPreReg_2x.Append("%s\tc_t0_%s_%cwComp_2x%s = r_%s_%cwComp%s%s;\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, queRegCompSig.c_str(), dimIdx.c_str());
									} else {
										ngvPreReg_2x.Append("%s\tc_t0_%s_%cwComp_2x%s = r_%s_%cwCompO%s%s;\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, queRegCompSig.c_str(), dimIdx.c_str());
									}
								}
							}

							ngvPreReg_2x.Append("%s}\n", tabs.c_str());

						} while (loopInfo.Iter());
					}
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
						CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
							VA("r_t1_%s_%cwWrEn_2x", mod.m_modName.Lc().c_str(), imCh), pGv->m_dimenList);

						if (idW > 0) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", idW),
								VA("r_t1_%s_%cw%sId_2x", mod.m_modName.Lc().c_str(), imCh, idStr.c_str()), pGv->m_dimenList);
						}

						if (imIdx == 0) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
								VA("r_t1_%s_%cwComp_2x", mod.m_modName.Lc().c_str(), imCh), pGv->m_dimenList);
						}
						ngvRegDecl.NewLine();
					}

					{
						string tabs = "\t";
						CLoopInfo loopInfo(ngvReg_2x, tabs, pGv->m_dimenList, 10);
						do {
							string dimIdx = loopInfo.IndexStr();

							ngvReg_2x.Append("%sr_t1_gwWrEn_2x%s = c_t0_gwWrEn_2x%s;\n", tabs.c_str(),
								dimIdx.c_str(), dimIdx.c_str());

							ngvReg_2x.Append("%sr_t1_gwData_2x%s = c_t0_gwData_2x%s;\n", tabs.c_str(),
								dimIdx.c_str(), dimIdx.c_str());
							ngvReg_2x.NewLine();

							for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
								CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
								int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
								char imCh = imIdx == 0 ? 'i' : 'm';
								string idStr = imIdx == 0 ? "Ht" : "Grp";
								int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

								ngvReg_2x.Append("%sr_t1_%s_%cwWrEn_2x%s = c_t0_%s_%cwWrEn_2x%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

								if (idW > 0) {
									ngvReg_2x.Append("%sr_t1_%s_%cw%sId_2x%s = c_t0_%s_%cw%sId_2x%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
								}
								if (imIdx == 0) {
									ngvReg_2x.Append("%sr_t1_%s_%cwComp_2x%s = c_t0_%s_%cwComp_2x%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
								}
								ngvReg_2x.NewLine();
							}

						} while (loopInfo.Iter());
					}

					{
						string tabs = "\t";
						CLoopInfo loopInfo(ngvPreReg_2x, tabs, pGv->m_dimenList, 10);
						do {
							string dimIdx = loopInfo.IndexStr();

							ngvPreReg_2x.Append("%sc_t1_gwWrEn_2x%s = r_t1_gwWrEn_2x%s;\n", tabs.c_str(),
								dimIdx.c_str(), dimIdx.c_str());

							ngvPreReg_2x.Append("%sc_t1_gwData_2x%s = r_t1_gwData_2x%s;\n", tabs.c_str(),
								dimIdx.c_str(), dimIdx.c_str());

						} while (loopInfo.Iter());
					}
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
						string tabs = "\t";
						CLoopInfo loopInfo(ngvPreReg_1x, tabs, pGv->m_dimenList, 10);
						do {
							string dimIdx = loopInfo.IndexStr();

							ngvPreReg_1x.Append("%sc_t0_gwWrEn%c%s = false;\n", tabs.c_str(), eoCh, dimIdx.c_str());
							ngvPreReg_1x.Append("%sc_t0_gwData%c%s = 0;\n", tabs.c_str(), eoCh, dimIdx.c_str());
							ngvPreReg_1x.NewLine();

							for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
								CRam * pModNgv = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pNgv;
								CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
								int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
								char imCh = imIdx == 0 ? 'i' : 'm';
								string idStr = imIdx == 0 ? "Ht" : "Grp";
								int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();
								bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? (pModNgv->m_bMaxIw && idW > 0) : pModNgv->m_bMaxMw);
								string maxWrStr = bMaxWr ? (eoIdx == 0 ? "E" : "O") : "";

								if (idW > 0) {
									ngvPreReg_1x.Append("%sc_t0_%s_%cw%sId%c%s = r_%s_%cw%sId%s%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoCh, dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), maxWrStr.c_str(), dimIdx.c_str());
								}

								if (imIdx == 0) {
									ngvPreReg_1x.Append("%sc_t0_%s_%cwComp%c%s = r_%s_%cwComp%s%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, maxWrStr.c_str(), dimIdx.c_str());
								}
								ngvPreReg_1x.NewLine();
							}

						} while (loopInfo.Iter());
					}

					{
						string tabs = "\t";
						CLoopInfo loopInfo(ngvPreReg_1x, tabs, pGv->m_dimenList, 10);
						do {
							string dimIdx = loopInfo.IndexStr();

							for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
								CRam * pModNgv = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pNgv;
								CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
								int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
								char imCh = imIdx == 0 ? 'i' : 'm';
								int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();
								bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? (pModNgv->m_bMaxIw && idW > 0) : pModNgv->m_bMaxMw);
								string maxWrStr = bMaxWr ? (eoIdx == 0 ? "E" : "O") : "";

								ngvPreReg_1x.Append("%sif (c_t0_%s_%cwRrSel%c%s) {\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());

								ngvPreReg_1x.Append("%s\tc_rrSel%c%s = 0x%x;\n", tabs.c_str(),
									eoCh, dimIdx.c_str(),
									1 << ((ngvIdx + 1) % ngvPortCnt));

								ngvPreReg_1x.Append("%s\tc_t0_gwWrEn%c%s = true;\n", tabs.c_str(),
									eoCh, dimIdx.c_str());

								ngvPreReg_1x.Append("%s\tc_t0_gwData%c%s = r_%s_%cwData%s%s;\n", tabs.c_str(),
									eoCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, maxWrStr.c_str(), dimIdx.c_str());

								ngvPreReg_1x.Append("%s}\n", tabs.c_str());
							}

						} while (loopInfo.Iter());
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
						CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
							VA("r_t1_%s_%cwRrSel%c_sig", mod.m_modName.Lc().c_str(), imCh, eoCh), pGv->m_dimenList);

						if (idW > 0) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<ht_uint%d>", idW),
								VA("r_t1_%s_%cw%sId%c_sig", mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoCh), pGv->m_dimenList);
						}

						if (imIdx == 0) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
								VA("r_t1_%s_%cwComp%c_sig", mod.m_modName.Lc().c_str(), imCh, eoCh), pGv->m_dimenList);
						}
					}

					{
						string tabs = "\t";
						CLoopInfo loopInfo(ngvReg_1x, tabs, pGv->m_dimenList, 10);
						do {
							string dimIdx = loopInfo.IndexStr();

							ngvReg_1x.Append("%sr_t1_gwWrEn%c_sig%s = c_t0_gwWrEn%c%s;\n", tabs.c_str(),
								eoCh, dimIdx.c_str(),
								eoCh, dimIdx.c_str());

							ngvReg_1x.Append("%sr_t1_gwData%c_sig%s = c_t0_gwData%c%s;\n", tabs.c_str(),
								eoCh, dimIdx.c_str(),
								eoCh, dimIdx.c_str());
							ngvReg_1x.NewLine();

							for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
								CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
								int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
								char imCh = imIdx == 0 ? 'i' : 'm';
								string idStr = imIdx == 0 ? "Ht" : "Grp";
								int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

								ngvReg_1x.Append("%sr_t1_%s_%cwRrSel%c_sig%s = c_t0_%s_%cwRrSel%c%s;\n", tabs.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());

								if (idW > 0) {
									ngvReg_1x.Append("%sr_t1_%s_%cw%sId%c_sig%s = c_t0_%s_%cw%sId%c%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoCh, dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoCh, dimIdx.c_str());
								}

								if (imIdx == 0) {
									ngvReg_1x.Append("%sr_t1_%s_%cwComp%c_sig%s = c_t0_%s_%cwComp%c%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());
								}
								ngvReg_1x.NewLine();
							}

						} while (loopInfo.Iter());
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
							CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
							char imCh = imIdx == 0 ? 'i' : 'm';
							string idStr = imIdx == 0 ? "Ht" : "Grp";
							int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

							ngvRegDecl.Append("\tbool c_t1_%s_%cwWrEn_2x%s;\n",
								mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());

							if (idW > 0) {
								ngvRegDecl.Append("\tht_uint%d c_t1_%s_%cw%sId_2x%s;\n",
									idW,
									mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), pGv->m_dimenDecl.c_str());
							}

							if (imIdx == 0) {
								ngvRegDecl.Append("\tbool c_t1_%s_%cwComp_2x%s;\n",
									mod.m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
							}
							ngvRegDecl.NewLine();
						}

						string tabs = "\t";
						CLoopInfo loopInfo(ngvPreReg_2x, tabs, pGv->m_dimenList, 5);
						do {
							string dimIdx = loopInfo.IndexStr();

							for (int eoIdx = 0; eoIdx < 2; eoIdx += 1) {
								char eoCh = eoIdx == 0 ? 'E' : 'O';

								if (eoIdx == 0)
									ngvPreReg_2x.Append("%sif (r_phase) {\n", tabs.c_str());
								else
									ngvPreReg_2x.Append("%s} else {\n", tabs.c_str());

								ngvPreReg_2x.Append("%s\tc_t1_gwWrEn_2x%s = r_t1_gwWrEn%c_sig%s;\n", tabs.c_str(),
									dimIdx.c_str(),
									eoCh, dimIdx.c_str());
								ngvPreReg_2x.Append("%s\tc_t1_gwData_2x%s = r_t1_gwData%c_sig%s;\n", tabs.c_str(),
									dimIdx.c_str(),
									eoCh, dimIdx.c_str());
								ngvPreReg_2x.NewLine();

								for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
									CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
									int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
									char imCh = imIdx == 0 ? 'i' : 'm';
									string idStr = imIdx == 0 ? "Ht" : "Grp";
									int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

									ngvPreReg_2x.Append("%s\tc_t1_%s_%cwWrEn_2x%s = r_t1_%s_%cwRrSel%c_sig%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());

									if (idW > 0) {
										ngvPreReg_2x.Append("%s\tc_t1_%s_%cw%sId_2x%s = r_t1_%s_%cw%sId%c_sig%s;\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), eoCh, dimIdx.c_str());
									}
									if (imIdx == 0) {
										ngvPreReg_2x.Append("%s\tc_t1_%s_%cwComp_2x%s = r_t1_%s_%cwComp%c_sig%s;\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());
									}
									ngvPreReg_2x.NewLine();
								}
							}

							ngvPreReg_2x.Append("%s}\n", tabs.c_str());

						} while (loopInfo.Iter());
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
							CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
							char imCh = imIdx == 0 ? 'i' : 'm';
							string idStr = imIdx == 0 ? "Ht" : "Grp";
							int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
								VA("r_t2_%s_%cwWrEn_2x", mod.m_modName.Lc().c_str(), imCh), pGv->m_dimenList);

							if (idW > 0) {
								GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", idW),
									VA("r_t2_%s_%cw%sId_2x", mod.m_modName.Lc().c_str(), imCh, idStr.c_str()), pGv->m_dimenList);
							}

							if (imIdx == 0) {
								GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
									VA("r_t2_%s_%cwComp_2x", mod.m_modName.Lc().c_str(), imCh), pGv->m_dimenList);
							}
							ngvRegDecl.NewLine();
						}

						{
							string tabs = "\t";
							CLoopInfo loopInfo(ngvReg_2x, tabs, pGv->m_dimenList, 5);
							do {
								string dimIdx = loopInfo.IndexStr();

								ngvReg_2x.Append("%sr_t2_gwWrEn_2x%s = c_t1_gwWrEn_2x%s;\n", tabs.c_str(),
									dimIdx.c_str(), dimIdx.c_str());

								ngvReg_2x.Append("%sr_t2_gwData_2x%s = c_t1_gwData_2x%s;\n", tabs.c_str(),
									dimIdx.c_str(), dimIdx.c_str());
								ngvReg_2x.NewLine();

								for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
									CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
									int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
									char imCh = imIdx == 0 ? 'i' : 'm';
									string idStr = imIdx == 0 ? "Ht" : "Grp";
									int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

									ngvReg_2x.Append("%sr_t2_%s_%cwWrEn_2x%s = c_t1_%s_%cwWrEn_2x%s;\n", tabs.c_str(),
										mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());

									if (idW > 0) {
										ngvReg_2x.Append("%sr_t2_%s_%cw%sId_2x%s = c_t1_%s_%cw%sId_2x%s;\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
									}
									if (imIdx == 0) {
										ngvReg_2x.Append("%sr_t2_%s_%cwComp_2x%s = c_t1_%s_%cwComp_2x%s;\n", tabs.c_str(),
											mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
									}
									ngvReg_2x.NewLine();
								}
							} while (loopInfo.Iter());
						}

						{
							string tabs = "\t";
							CLoopInfo loopInfo(ngvPreReg_2x, tabs, pGv->m_dimenList, 2);
							do {
								string dimIdx = loopInfo.IndexStr();

								ngvPreReg_2x.Append("%sc_t2_gwWrEn_2x%s = r_t2_gwWrEn_2x%s;\n", tabs.c_str(),
									dimIdx.c_str(), dimIdx.c_str());

								ngvPreReg_2x.Append("%sc_t2_gwData_2x%s = r_t2_gwData_2x%s;\n", tabs.c_str(),
									dimIdx.c_str(), dimIdx.c_str());

							} while (loopInfo.Iter());
						}
					}
				}
			}

			stgIdx += 1;
			wrCompStg = VA("t%d_", stgIdx);

		} else if (pNgvInfo->m_wrPortList.size() == 1 && ngvFieldCnt == 1 && !bNgvAtomic) {
			CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[0].first].m_pMod;
			char imCh = pNgvInfo->m_wrPortList[0].second == 0 ? 'i' : 'm';

			CHtCode &ngvPreRegMod = mod.m_clkRate == eClk2x ? ngvPreReg_2x : ngvPreReg_1x;
			string ngvModClk = mod.m_clkRate == eClk2x ? "_2x" : "";

			{
				string tabs = "\t";
				CLoopInfo loopInfo(ngvPreRegMod, tabs, pGv->m_dimenList, 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvPreRegMod.Append("%sc_t%d_wrEn%s%s = r_%s_%cwWrEn%s%s;\n", tabs.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						mod.m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str());

					if (pGv->m_addrW > 0) {
						ngvPreRegMod.Append("%sc_t%d_wrAddr%s%s = r_%s_%cwData%s%s.GetAddr();\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str());
					}

					for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
						if (iter.IsStructOrUnion()) continue;

						ngvPreRegMod.Append("%sc_t%d_wrData%s%s%s = r_%s_%cwData%s%s%s.GetData();\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
							mod.m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
					}
				} while (loopInfo.Iter());
			}

		} else if (pNgvInfo->m_wrPortList.size() == 1 && (ngvFieldCnt > 1 || bNgvAtomic)) {
			CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[0].first].m_pMod;
			char imCh = pNgvInfo->m_wrPortList[0].second == 0 ? 'i' : 'm';

			CHtCode &ngvPreRegMod = pNgvInfo->m_bNgvWrDataClk2x ? ngvPreReg_2x : ngvPreReg_1x;
			string ngvModClk = pNgvInfo->m_bNgvWrDataClk2x ? "_2x" : "";

			ngvRegDecl.Append("\tbool c_t%d_gwWrEn%s%s;\n",
				stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

			ngvRegDecl.Append("\tCGW_%s c_t%d_gwData%s%s;\n",
				pGv->m_pNgvInfo->m_ngvWrType.c_str(),
				stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
			ngvRegDecl.NewLine();

			{
				string tabs = "\t";
				CLoopInfo loopInfo(ngvPreRegMod, tabs, pGv->m_dimenList, 2);
				do {
					string dimIdx = loopInfo.IndexStr();

					if (bNeedAddrComp) {
						ngvPreRegMod.Append("%sc_t%d_gwWrEn%s%s = c_t%d_%s_%cwRrSel%s%s;\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str());
						wrCompStg = VA("t%d_", stgIdx + 1);
					} else {
						ngvPreRegMod.Append("%sc_t%d_gwWrEn%s%s = r_%s_%cwWrEn%s%s;\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							mod.m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str());
					}
					ngvPreRegMod.Append("%sc_t%d_gwData%s%s = r_%s_%cwData%s%s;\n", tabs.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						mod.m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}

		} else if (pNgvInfo->m_wrPortList.size() == 2 && pNgvInfo->m_bAllWrPortClk1x) {

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

				CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[phaseIdx].first].m_pMod;
				char imCh = pNgvInfo->m_wrPortList[phaseIdx].second == 0 ? 'i' : 'm';

				{
					string tabs = "\t\t";
					CLoopInfo loopInfo(ngvPreReg_2x, tabs, pGv->m_dimenList, 3);
					do {
						string dimIdx = loopInfo.IndexStr();

						if (ngvFieldCnt == 1 && !bNgvAtomic) {

							ngvPreReg_2x.Append("%sc_t%d_wrEn%s%s = r_%s_%cwWrEn%s%s;\n", tabs.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str());

						} else {
							if (bNeedAddrComp) {
								ngvPreReg_2x.Append("%sc_t%d_gwWrEn%s%s = c_t%d_%s_%cwRrSel%s%s;\n", tabs.c_str(),
									stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
									stgIdx, mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str());
								wrCompStg = VA("t%d_", stgIdx + 1);
							} else {
								ngvPreReg_2x.Append("%sc_t%d_gwWrEn%s%s = r_%s_%cwWrEn%s%s;\n", tabs.c_str(),
									stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str());
							}
						}

						if (pGv->m_addrW > 0) {

							ngvPreReg_2x.Append("%sc_t%d_wrAddr%s%s = r_%s_%cwData%s%s.read().GetAddr();\n", tabs.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());
						}

						if (ngvFieldCnt == 1 && !bNgvAtomic) {
							for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
								if (iter.IsStructOrUnion()) continue;

								ngvPreReg_2x.Append("%sc_t%d_wrData%s%s%s = r_%s_%cwData%s%s.read()%s.GetData();\n", tabs.c_str(),
									stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
									mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
							}

						} else {

							ngvPreReg_2x.Append("%sc_t%d_gwData%s%s = r_%s_%cwData%s%s;\n", tabs.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
								mod.m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());
						}

					} while (loopInfo.Iter(false));
				}
			}
			ngvPreReg_2x.Append("\t}\n");
			ngvPreReg_2x.NewLine();
		}

		bool bNeedIfStg = bNgvBlock && (ngvPortCnt == 1 && ngvFieldCnt > 1 && !bNgvAtomic && !bNgvMaxSel
			|| bNgvAtomic);
		bool bNeedCompStg = bNgvBlock && (ngvPortCnt == 1 && ngvFieldCnt > 1 && !bNgvAtomic && !bNgvMaxSel
			|| ngvPortCnt == 1 && bNgvAtomic && !bNgvMaxSel);

		if (ngvFieldCnt > 1 || bNgvAtomic) {

			if (bNgvBlock) {
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
					VA("r_t%d_gwData%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				ngvRegDecl.NewLine();

				if (ngvFieldCnt > 1 || bNgvAtomic) {
					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

						if (bNeedIfStg) {
							if (stgIdx > 0) {
								ngvRegDecl.Append("\tbool c_t%d_%s_%cwWrEn%s%s;\n",
									stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
							}

							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
								VA("r_t%d_%s_%cwWrEn%s", stgIdx + 1, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str()), pGv->m_dimenList);
						}

						if (bNeedIfStg && idW > 0) {
							ngvRegDecl.Append("\tht_uint%d c_t%d_%s_%cw%sId%s%s;\n",
								idW,
								stgIdx, mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", idW),
								VA("r_t%d_%s_%cw%sId%s", stgIdx + 1, mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrDataClk.c_str()), pGv->m_dimenList);
						}

						if (bNeedCompStg && imIdx == 0) {
							ngvRegDecl.Append("\tbool c_t%d_%s_%cwComp%s%s;\n",
								stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
								VA("r_t%d_%s_%cwComp%s", stgIdx + 1, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str()), pGv->m_dimenList);
						}

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
				;
				if (ngvFieldCnt > 1 || bNgvAtomic) {
					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

						if (bNeedIfStg && stgIdx > 0 || bNeedIfStg && idW > 0 || bNeedCompStg && imIdx == 0) {
							string tabs = "\t";
							CLoopInfo loopInfo(ngvPreRegWrData, tabs, pGv->m_dimenList, 3);
							do {
								string dimIdx = loopInfo.IndexStr();

								if (bNeedIfStg && stgIdx > 0) {
									ngvPreRegWrData.Append("%sc_t%d_%s_%cwWrEn%s%s = r_t%d_%s_%cwWrEn%s%s;\n", tabs.c_str(),
										stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str(),
										stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str());
								}

								if (bNeedIfStg && idW > 0) {
									if (stgIdx == 0) {
										ngvPreRegWrData.Append("%sc_t%d_%s_%cw%sId%s = r_%s_%cw%sId%s;\n", tabs.c_str(),
											stgIdx, mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
									} else {
										ngvPreRegWrData.Append("%sc_t%d_%s_%cw%sId%s%s = r_t%d_%s_%cw%sId%s%s;\n", tabs.c_str(),
											stgIdx, mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
											stgIdx, mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
									}
								}

								if (bNeedCompStg && imIdx == 0) {
									if (stgIdx == 0) {
										ngvPreRegWrData.Append("%sc_t%d_%s_%cwComp%s = r_%s_%cwComp%s;\n", tabs.c_str(),
											stgIdx, mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
									} else {
										ngvPreRegWrData.Append("%sc_t%d_%s_%cwComp%s%s = r_t%d_%s_%cwComp%s%s;\n", tabs.c_str(),
											stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str(),
											stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str());
									}
								}
							} while (loopInfo.Iter());
						}
					}
				}

				{
					string tabs = "\t";
					CLoopInfo loopInfo(ngvReg, tabs, pGv->m_dimenList, 3);
					do {
						string dimIdx = loopInfo.IndexStr();

						ngvReg.Append("%sr_t%d_gwData%s%s = c_t%d_gwData%s%s;\n", tabs.c_str(),
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
						ngvReg.NewLine();

						if (ngvFieldCnt > 1 || bNgvAtomic) {
							for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
								CModule &mod = *ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
								int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
								char imCh = imIdx == 0 ? 'i' : 'm';
								string idStr = imIdx == 0 ? "Ht" : "Grp";
								int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

								if (bNeedIfStg) {
									if (stgIdx == 0) {
										ngvReg.Append("%sr_t%d_%s_%cwWrEn%s = c_t%d_%s_%cwRrSel%s;\n", tabs.c_str(),
											stgIdx + 1, mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											stgIdx, mod.m_modName.Lc().c_str(), imCh, dimIdx.c_str());
									} else {
										ngvReg.Append("%sr_t%d_%s_%cwWrEn%s%s = c_t%d_%s_%cwWrEn%s%s;\n", tabs.c_str(),
											stgIdx + 1, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str(),
											stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str());
									}

									if (idW > 0) {
										ngvReg.Append("%sr_t%d_%s_%cw%sId%s%s = c_t%d_%s_%cw%sId%s%s;\n", tabs.c_str(),
											stgIdx + 1, mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
											stgIdx, mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
									}
								}

								if (bNeedCompStg && imIdx == 0) {
									ngvReg.Append("%sr_t%d_%s_%cwComp%s%s = c_t%d_%s_%cwComp%s%s;\n", tabs.c_str(),
										stgIdx + 1, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str(),
										stgIdx, mod.m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str());
								}
							}
						}

					} while (loopInfo.Iter());
				}
			}
		} else {
			ngvRegDecl.Append("\tbool c_t%d_wrEn%s%s;\n",
				stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

			if (pNgvInfo->m_wrPortList.size() > 1) {
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
					VA("r_t%d_wrEn%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
			}

			if (pGv->m_addrW > 0) {
				ngvRegDecl.Append("\tht_uint%d c_t%d_wrAddr%s%s;\n",
					pGv->m_addrW,
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				if (pNgvInfo->m_wrPortList.size() > 1) {
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addrW),
						VA("r_t%d_wrAddr%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				}
			}

			ngvRegDecl.Append("\t%s c_t%d_wrData%s%s;\n",
				pGv->m_type.c_str(),
				stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
			if (pNgvInfo->m_wrPortList.size() > 1) {
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, pGv->m_type,
					VA("r_t%d_wrData%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
			}
			ngvRegDecl.NewLine();

			CHtCode & ngvReg = pNgvInfo->m_bNgvWrDataClk2x ? ngvReg_2x : ngvReg_1x;

			if (pNgvInfo->m_wrPortList.size() > 1) {
				string tabs = "\t";
				CLoopInfo loopInfo(ngvReg, tabs, pGv->m_dimenList, 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvReg.Append("%sr_t%d_wrEn%s%s = c_t%d_wrEn%s%s;\n", tabs.c_str(),
						stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					if (pGv->m_addrW > 0) {
						ngvReg.Append("%sr_t%d_wrAddr%s%s = c_t%d_wrAddr%s%s;\n", tabs.c_str(),
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					}

					ngvReg.Append("%sr_t%d_wrData%s%s = c_t%d_wrData%s%s;\n", tabs.c_str(),
						stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					ngvReg.NewLine();

				} while (loopInfo.Iter());

				stgIdx += 1;
			}
		}

		assert(pNgvInfo->m_wrCompStg < 0 && wrCompStg == "" || VA("t%d_", pNgvInfo->m_wrCompStg).operator std::string() == wrCompStg);

		// Write completion output
		for (size_t modIdx = 0; modIdx < ngvModInfoList.size(); modIdx += 1) {
			CModule & mod = *ngvModInfoList[modIdx].m_pMod;
			CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

			for (int imIdx = 0; imIdx < 2; imIdx += 1) {
				if (imIdx == 0 && !pModNgv->m_bWriteForInstrWrite) continue;
				if (imIdx == 1 && !pModNgv->m_bWriteForMifRead) continue;
				char imCh = imIdx == 0 ? 'i' : 'm';
				string idStr = imIdx == 0 ? "Ht" : "Grp";
				int idW = imIdx == 0 ? mod.m_threads.m_htIdW.AsInt() : mod.m_mif.m_mifRd.m_rspGrpW.AsInt();

				string tabs = "\t";
				CLoopInfo loopInfo(ngvWrCompOut, tabs, pGv->m_dimenList, 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvWrCompOut.Append("%so_%sTo%s_%cwCompRdy%s = r_%s%s_%cwWrEn%s%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), imCh, dimIdx.c_str(),
						wrCompStg.c_str(), mod.m_modName.Lc().c_str(), imCh, ngvSelClk.c_str(), dimIdx.c_str());
					if (idW > 0) {
						ngvWrCompOut.Append("%so_%sTo%s_%cwComp%sId%s = r_%s%s_%cw%sId%s%s;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
							wrCompStg.c_str(), mod.m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvSelClk.c_str(), dimIdx.c_str());
					}
					if (imIdx == 0) {
						ngvWrCompOut.Append("%so_%sTo%s_%cwCompData%s = r_%s%s_%cwComp%s%s;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), imCh, dimIdx.c_str(),
							wrCompStg.c_str(), mod.m_modName.Lc().c_str(), imCh, ngvSelClk.c_str(), dimIdx.c_str());
					}

				} while (loopInfo.Iter());
			}
		}
		ngvWrCompOut.NewLine();

		if (ngvFieldCnt > 1 || bNgvAtomic) {
			// declare global variable
			if (bNgvReg) {
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, pGv->m_type.c_str(),
					VA("r_%s", pGv->m_gblName.c_str()), pGv->m_dimenList);

				string tabs = "\t";
				CLoopInfo loopInfo(ngvRegWrData, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvRegWrData.Append("%sif (c_t%d_wrEn%s%s)\n", tabs.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					ngvRegWrData.Append("%s\tr_%s%s = c_t%d_wrData%s%s;\n", tabs.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			} else {
				ngvRegDecl.Append("\tht_%s_ram<%s, %d",
					pGv->m_pNgvInfo->m_ramType == eBlockRam ? "block" : "dist",
					pGv->m_type.c_str(), pGv->m_addrW);
				ngvRegDecl.Append("> m_%s%s;\n", pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

				{
					string tabs = "\t";
					CLoopInfo loopInfo(ngvRegWrData, tabs, pGv->m_dimenList, 1);
					do {
						string dimIdx = loopInfo.IndexStr();

						ngvRegWrData.Append("%sm_%s%s.clock(%s);\n", tabs.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str(), ngvRegWrDataReset.c_str());

					} while (loopInfo.Iter());
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
				string tabs = "\t";
				CLoopInfo loopInfo(ngvOut, tabs, pGv->m_dimenList, 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvOut.Append("%so_%sTo%s_ifWrEn%s = r_t%d_%s_ifWrEn%s%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						ifStgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
					if (mod.m_threads.m_htIdW.AsInt() > 0)
						ngvOut.Append("%so_%sTo%s_ifHtId%s = r_t%d_%s_ifHtId%s%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						ifStgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
					ngvOut.Append("%so_%sTo%s_ifData%s = r_t%d_%s_ifData%s%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						ifStgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
				} while (loopInfo.Iter());
			}
		}

		if (ngvFieldCnt > 1 || bNgvAtomic) {

			// read stage of RMW
			ngvRegDecl.Append("\tbool c_t%d_wrEn%s%s;\n",
				stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

			if (pGv->m_addrW > 0) {
				ngvRegDecl.Append("\tht_uint%d c_t%d_wrAddr%s%s;\n",
					pGv->m_addrW,
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
			}

			if (!bNgvBlock) {
				ngvRegDecl.Append("\t%s c_t%d_wrData%s%s;\n",
					pGv->m_type.c_str(),
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
			}
			ngvRegDecl.NewLine();

			CHtCode & ngvReg = pNgvInfo->m_bNgvWrDataClk2x ? ngvReg_2x : ngvReg_1x;

			{
				string tabs = "\t";
				CLoopInfo loopInfo(ngvPreRegWrData, tabs, pGv->m_dimenList, 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvPreRegWrData.Append("%sc_t%d_wrEn%s%s = c_t%d_gwWrEn%s%s;\n", tabs.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					if (pGv->m_addrW > 0) {
						ngvPreRegWrData.Append("%sc_t%d_wrAddr%s%s = c_t%d_gwData%s%s.GetAddr();\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					}

					if (bNgvReg) {
						ngvPreRegWrData.Append("%sc_t%d_wrData%s%s = r_%s%s;\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str());

					} else {
						ngvPreRegWrData.Append("%sm_%s%s.read_addr(c_t%d_wrAddr%s%s);\n", tabs.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					}
				} while (loopInfo.Iter());
			}

			if (bNgvDist) {
				string tabs = "\t";
				CLoopInfo loopInfo(ngvPreRegWrData, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvPreRegWrData.Append("%sc_t%d_wrData%s%s = m_%s%s.read_mem();\n", tabs.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());

			} else if (bNgvBlock) {
				ngvPreRegWrData.NewLine();

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
					VA("r_t%d_wrEn%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);

				if (pGv->m_addrW > 0) {
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addrW),
						VA("r_t%d_wrAddr%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				}
				ngvRegDecl.NewLine();

				{
					string tabs = "\t";
					CLoopInfo loopInfo(ngvReg, tabs, pGv->m_dimenList, 2);
					do {
						string dimIdx = loopInfo.IndexStr();

						ngvReg.Append("%sr_t%d_wrEn%s%s = c_t%d_wrEn%s%s;\n", tabs.c_str(),
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

						if (pGv->m_addrW > 0) {
							ngvReg.Append("%sr_t%d_wrAddr%s%s = c_t%d_wrAddr%s%s;\n", tabs.c_str(),
								stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
						}

					} while (loopInfo.Iter());
				}

				stgIdx += 1;

				ngvRegDecl.Append("\tbool c_t%d_wrEn%s%s;\n",
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

				if (pGv->m_addrW > 0) {
					ngvRegDecl.Append("\tht_uint%d c_t%d_wrAddr%s%s;\n",
						pGv->m_addrW,
						stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				}

				ngvRegDecl.Append("\t%s c_t%d_wrData%s%s;\n",
					pGv->m_type.c_str(),
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				ngvRegDecl.NewLine();

				{
					string tabs = "\t";
					CLoopInfo loopInfo(ngvPreRegWrData, tabs, pGv->m_dimenList, 1);
					do {
						string dimIdx = loopInfo.IndexStr();

						ngvPreRegWrData.Append("%sc_t%d_gwData%s%s = r_t%d_gwData%s%s;\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					} while (loopInfo.Iter());
				}

				{
					string tabs = "\t";
					CLoopInfo loopInfo(ngvPreRegWrData, tabs, pGv->m_dimenList, 3);
					do {
						string dimIdx = loopInfo.IndexStr();

						ngvPreRegWrData.Append("%sc_t%d_wrEn%s%s = r_t%d_wrEn%s%s;\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

						if (pGv->m_addrW > 0) {
							ngvPreRegWrData.Append("%sc_t%d_wrAddr%s%s = r_t%d_wrAddr%s%s;\n", tabs.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
						}

						ngvPreRegWrData.Append("%sc_t%d_wrData%s%s = m_%s%s.read_mem();\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str());

					} while (loopInfo.Iter());
				}
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
						string tabs = "\t";
						CLoopInfo loopInfo(ngvPreRegWrData, tabs, pGv->m_dimenList, 2);
						do {
							string dimIdx = loopInfo.IndexStr();

							if (bRrSelAB) {
								ngvPreRegWrData.Append("%sc_t%d_%s_ifWrEn%s%s = r_t1_%s_iwWrEn%s_sig%s && %sr_phase;\n", tabs.c_str(),
									stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
									mod.m_modName.Lc().c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(), (int)modIdx < ngvPortCnt / 2 ? "" : "!");
							} else if (bRrSelEO && pNgvInfo->m_bNgvMaxSel) {
								ngvPreRegWrData.Append("%sc_t%d_%s_ifWrEn%s%s = r_t%d_%s_iwWrEn_2x%s;\n", tabs.c_str(),
									stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
									stgIdx, mod.m_modName.Lc().c_str(), dimIdx.c_str());
							} else if (pNgvInfo->m_wrPortList.size() == 2 && pNgvInfo->m_bAllWrPortClk1x) {
								if (bNgvBlock) {
									ngvPreRegWrData.Append("%sc_t%d_%s_ifWrEn%s%s = r_t%d_%s_iwWrEn%s%s;\n", tabs.c_str(),
										stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										stgIdx, mod.m_modName.Lc().c_str(), queRegWrEnSig.c_str(), dimIdx.c_str());
								} else if (bNgvAtomicSlow) {
									ngvPreRegWrData.Append("%sc_t%d_%s_ifWrEn%s%s = r_%s_iwWrEn%s%s;\n", tabs.c_str(),
										stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), queRegWrEnSig.c_str(), dimIdx.c_str());
								} else {
									ngvPreRegWrData.Append("%sc_t%d_%s_ifWrEn%s%s = r_%s_iwWrEn%s%s && %sr_phase;\n", tabs.c_str(),
										stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
										modIdx == 0 ? "!" : "");
								}
							} else {
								ngvPreRegWrData.Append("%sc_t%d_%s_ifWrEn%s%s = %s_%s_iwWrEn%s%s;\n", tabs.c_str(),
									stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
									regStg.c_str(), mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
							}

							if (mod.m_threads.m_htIdW.AsInt() > 0) {
								if (bRrSelAB) {
									ngvPreRegWrData.Append("%sc_t%d_%s_ifHtId%s%s = r_t1_%s_iwHtId%s_sig%s;\n", tabs.c_str(),
										stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										mod.m_modName.Lc().c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
								} else if (bRrSelEO && pNgvInfo->m_bNgvMaxSel) {
									ngvPreRegWrData.Append("%sc_t%d_%s_ifHtId%s%s = r_t%d_%s_iwHtId_2x%s;\n", tabs.c_str(),
										stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										stgIdx, mod.m_modName.Lc().c_str(), dimIdx.c_str());
								} else if (pNgvInfo->m_wrPortList.size() == 2 && pNgvInfo->m_bAllWrPortClk1x) {
									if (bNgvBlock) {
										ngvPreRegWrData.Append("%sc_t%d_%s_ifHtId%s%s = r_t%d_%s_iwHtId%s%s;\n", tabs.c_str(),
											stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
											stgIdx, mod.m_modName.Lc().c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
									} else {
										ngvPreRegWrData.Append("%sc_t%d_%s_ifHtId%s%s = r_%s_iwHtId%s%s;\n", tabs.c_str(),
											stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
											mod.m_modName.Lc().c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
									}
								} else {
									ngvPreRegWrData.Append("%sc_t%d_%s_ifHtId%s%s = %s_%s_iwHtId%s%s;\n", tabs.c_str(),
										stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										regStg.c_str(), mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
								}
							}

							ngvPreRegWrData.Append("%sc_t%d_%s_ifData%s%s = c_t%d_wrData%s%s;\n", tabs.c_str(),
								stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

						} while (loopInfo.Iter());
					}

					{
						string tabs = "\t";
						CLoopInfo loopInfo(ngvRegWrData, tabs, pGv->m_dimenList, 10);
						do {
							string dimIdx = loopInfo.IndexStr();

							ngvRegWrData.Append("%sr_t%d_%s_ifWrEn%s%s = c_t%d_%s_ifWrEn%s%s;\n", tabs.c_str(),
								stgIdx + 1, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());

							if (mod.m_threads.m_htIdW.AsInt() > 0) {
								ngvRegWrData.Append("%sr_t%d_%s_ifHtId%s%s = c_t%d_%s_ifHtId%s%s;\n", tabs.c_str(),
									stgIdx + 1, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
									stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
							}

							ngvRegWrData.Append("%sr_t%d_%s_ifData%s%s = c_t%d_%s_ifData%s%s;\n", tabs.c_str(),
								stgIdx + 1, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, mod.m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());

						} while (loopInfo.Iter());
					}
				}
			}

			bool bNeedRamReg = bNgvDist && bNgvAtomicSlow && (pNgvInfo->m_wrPortList.size() >= 2 || !pNgvInfo->m_bAllWrPortClk1x) && pNgvInfo->m_bNgvMaxSel ||
				bNgvBlock && (bNgvAtomic || ngvFieldCnt > 1) && pNgvInfo->m_bNgvMaxSel;

			if (bNeedRamReg) {
				// register output of ram and other signals

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("CGW_%s", pNgvInfo->m_ngvWrType.c_str()),
					VA("r_t%d_gwData%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
					VA("r_t%d_wrEn%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);

				if (pGv->m_addrW > 0) {
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addrW),
						VA("r_t%d_wrAddr%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				}

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, pGv->m_type,
					VA("r_t%d_wrData%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				ngvRegDecl.NewLine();

				{
					string tabs = "\t";
					CLoopInfo loopInfo(ngvRegWrData, tabs, pGv->m_dimenList, 1);
					do {
						string dimIdx = loopInfo.IndexStr();

						ngvRegWrData.Append("%sr_t%d_gwData%s%s = c_t%d_gwData%s%s;\n", tabs.c_str(),
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					} while (loopInfo.Iter());
				}

				{
					string tabs = "\t";
					CLoopInfo loopInfo(ngvRegWrData, tabs, pGv->m_dimenList, 10);
					do {
						string dimIdx = loopInfo.IndexStr();

						ngvRegWrData.Append("%sr_t%d_wrEn%s%s = c_t%d_wrEn%s%s;\n", tabs.c_str(),
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

						if (pGv->m_addrW > 0) {
							ngvRegWrData.Append("%sr_t%d_wrAddr%s%s = c_t%d_wrAddr%s%s;\n", tabs.c_str(),
								stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
						}

						ngvRegWrData.Append("%sr_t%d_wrData%s%s = c_t%d_wrData%s%s;\n", tabs.c_str(),
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					} while (loopInfo.Iter());
				}

				stgIdx += 1;

				ngvRegDecl.Append("\tCGW_%s c_t%d_gwData%s%s;\n",
					pNgvInfo->m_ngvWrType.c_str(), stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

				ngvRegDecl.Append("\tbool c_t%d_wrEn%s%s;\n",
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

				if (pGv->m_addrW > 0) {
					ngvRegDecl.Append("\tht_uint%d c_t%d_wrAddr%s%s;\n",
						pGv->m_addrW, stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				}

				ngvRegDecl.Append("\t%s c_t%d_wrData%s%s;\n",
					pGv->m_type.c_str(), stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
				ngvRegDecl.NewLine();

				{
					string tabs = "\t";
					CLoopInfo loopInfo(ngvPreRegWrData, tabs, pGv->m_dimenList, 1);
					do {
						string dimIdx = loopInfo.IndexStr();

						ngvPreRegWrData.Append("%sc_t%d_gwData%s%s = r_t%d_gwData%s%s;\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					} while (loopInfo.Iter());
				}

				{
					string tabs = "\t";
					CLoopInfo loopInfo(ngvPreRegWrData, tabs, pGv->m_dimenList, 10);
					do {
						string dimIdx = loopInfo.IndexStr();

						ngvPreRegWrData.Append("%sc_t%d_wrEn%s%s = r_t%d_wrEn%s%s;\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

						if (pGv->m_addrW > 0) {
							ngvPreRegWrData.Append("%sc_t%d_wrAddr%s%s = r_t%d_wrAddr%s%s;\n", tabs.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
						}

						ngvPreRegWrData.Append("%sc_t%d_wrData%s%s = r_t%d_wrData%s%s;\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					} while (loopInfo.Iter());
				}
			}

			{
				// modify stage of RMW
				string tabs = "\t";
				CLoopInfo loopInfo(ngvPreRegWrData, tabs, pGv->m_dimenList, 10);
				do {
					string dimIdx = loopInfo.IndexStr();

					for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
						if (iter.IsStructOrUnion()) continue;

						ngvPreRegWrData.Append("%sif (c_t%d_gwData%s%s%s.GetWrEn())\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());

						ngvPreRegWrData.Append("%s\tc_t%d_wrData%s%s%s = c_t%d_gwData%s%s%s.GetData();\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());

						if ((iter->m_atomicMask & ATOMIC_INC) != 0) {

							ngvPreRegWrData.Append("%sif (c_t%d_gwData%s%s%s.GetIncEn())\n", tabs.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());

							ngvPreRegWrData.Append("%s\tc_t%d_wrData%s%s%s += 1u;\n", tabs.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
						}

						if ((iter->m_atomicMask & ATOMIC_ADD) != 0) {

							ngvPreRegWrData.Append("%sif (c_t%d_gwData%s%s%s.GetAddEn())\n", tabs.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());

							ngvPreRegWrData.Append("%s\tc_t%d_wrData%s%s%s += c_t%d_gwData%s%s%s.GetData();\n", tabs.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
						}
					}

				} while (loopInfo.Iter());
			}
		}

		bool bNeedRamWrReg = bNgvDist && bNgvAtomicSlow && (pNgvInfo->m_wrPortList.size() >= 2 || !pNgvInfo->m_bAllWrPortClk1x) && pNgvInfo->m_bNgvMaxSel ||
			bNgvBlock && (bNgvAtomic || ngvFieldCnt > 1) && pNgvInfo->m_bNgvMaxSel;

		if (bNeedRamWrReg) {

			GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
				VA("r_t%d_wrEn%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);

			if (pGv->m_addrW > 0) {
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addrW),
					VA("r_t%d_wrAddr%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
			}

			GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, pGv->m_type,
				VA("r_t%d_wrData%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
			ngvRegDecl.NewLine();

			{
				string tabs = "\t";
				CLoopInfo loopInfo(ngvRegWrData, tabs, pGv->m_dimenList, 10);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvRegWrData.Append("%sr_t%d_wrEn%s%s = c_t%d_wrEn%s%s;\n", tabs.c_str(),
						stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					if (pGv->m_addrW > 0) {
						ngvRegWrData.Append("%sr_t%d_wrAddr%s%s = c_t%d_wrAddr%s%s;\n", tabs.c_str(),
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					}

					ngvRegWrData.Append("%sr_t%d_wrData%s%s = c_t%d_wrData%s%s;\n", tabs.c_str(),
						stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}

			stgIdx += 1;

			ngvRegDecl.Append("\tbool c_t%d_wrEn%s%s;\n",
				stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

			if (pGv->m_addrW > 0) {
				ngvRegDecl.Append("\tht_uint%d c_t%d_wrAddr%s%s;\n",
					pGv->m_addrW,
					stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
			}

			ngvRegDecl.Append("\t%s c_t%d_wrData%s%s;\n",
				pGv->m_type.c_str(),
				stgIdx, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
			ngvRegDecl.NewLine();

			{
				string tabs = "\t";
				CLoopInfo loopInfo(ngvPreRegWrData, tabs, pGv->m_dimenList, 10);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvPreRegWrData.Append("%sc_t%d_wrEn%s%s = r_t%d_wrEn%s%s;\n", tabs.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					if (pGv->m_addrW > 0) {
						ngvPreRegWrData.Append("%sc_t%d_wrAddr%s%s = r_t%d_wrAddr%s%s;\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
					}

					ngvPreRegWrData.Append("%sc_t%d_wrData%s%s = r_t%d_wrData%s%s;\n", tabs.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}
		}

		// write stage of RMW
		if (ngvFieldCnt > 1 || bNgvAtomic) {

			if (bNgvDist || bNgvBlock) {
				string tabs = "\t";
				CLoopInfo loopInfo(ngvPreRegWrData, tabs, pGv->m_dimenList, 10);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvPreRegWrData.Append("%sm_%s%s.write_addr(c_t%d_wrAddr%s%s);\n", tabs.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					ngvPreRegWrData.Append("%sif (c_t%d_wrEn%s%s)\n", tabs.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					ngvPreRegWrData.Append("%s\tm_%s%s.write_mem(c_t%d_wrData%s%s);\n", tabs.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}

			if (!bNeedRamWrReg) {

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
					VA("r_t%d_wrEn%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);

				if (pGv->m_addrW > 0) {
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addrW),
						VA("r_t%d_wrAddr%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				}

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, pGv->m_type,
					VA("r_t%d_wrData%s", stgIdx + 1, ngvWrDataClk.c_str()), pGv->m_dimenList);
				ngvRegDecl.NewLine();

				{
					string tabs = "\t";
					CLoopInfo loopInfo(ngvRegWrData, tabs, pGv->m_dimenList, 10);
					do {
						string dimIdx = loopInfo.IndexStr();

						ngvRegWrData.Append("%sr_t%d_wrEn%s%s = c_t%d_wrEn%s%s;\n", tabs.c_str(),
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

						if (pGv->m_addrW > 0) {
							ngvRegWrData.Append("%sr_t%d_wrAddr%s%s = c_t%d_wrAddr%s%s;\n", tabs.c_str(),
								stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());
						}

						ngvRegWrData.Append("%sr_t%d_wrData%s%s = c_t%d_wrData%s%s;\n", tabs.c_str(),
							stgIdx + 1, ngvWrDataClk.c_str(), dimIdx.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

					} while (loopInfo.Iter());
				}

				stgIdx += 1;
			}
		}

		// Write data output
		{
			string varPrefix = (pNgvInfo->m_wrPortList.size() == 1 && ngvFieldCnt == 1 && !bNgvAtomic) ? VA("c_t%d", stgIdx) : VA("r_t%d", stgIdx);
			assert(pNgvInfo->m_wrDataStg == stgIdx);

			for (size_t modIdx = 0; modIdx < ngvModInfoList.size(); modIdx += 1) {
				CModule & mod = *ngvModInfoList[modIdx].m_pMod;
				CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

				if (!pModNgv->m_bReadForInstrRead && !pModNgv->m_bReadForMifWrite) continue;

				string tabs = "\t";
				CLoopInfo loopInfo(ngvOut, tabs, pGv->m_dimenList, 10);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvOut.Append("%so_%sTo%s_wrEn%s = %s_wrEn%s%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						varPrefix.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
					if (pGv->m_addrW > 0) {
						ngvOut.Append("%so_%sTo%s_wrAddr%s = %s_wrAddr%s%s;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							varPrefix.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
					}
					ngvOut.Append("%so_%sTo%s_wrData%s = %s_wrData%s%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
						varPrefix.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
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
