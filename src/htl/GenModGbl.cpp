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

void CDsnInfo::InitOptNgv()
{
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];

		if (!pMod->m_bIsUsed || pMod->m_ngvList.size() == 0) continue;

		// create unique list of global variables for entire unit
		for (size_t mgvIdx = 0; mgvIdx < pMod->m_ngvList.size(); mgvIdx += 1) {
			CRam * pMgv = pMod->m_ngvList[mgvIdx];

			// verify that ram name is unique in module
			for (size_t mgv2Idx = mgvIdx + 1; mgv2Idx < pMod->m_ngvList.size(); mgv2Idx += 1) {
				CRam * pMgv2 = pMod->m_ngvList[mgv2Idx];

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

				m_ngvList[gvIdx]->m_modInfoList.push_back(CNgvModInfo(pMod, pMgv));
				pMgv->m_pNgvInfo = m_ngvList[gvIdx];

				if (pMgv->m_pNgvInfo->m_ngvReplCnt != pMod->m_instSet.GetReplCnt(0))
					ParseMsg(Error, pMgv->m_lineInfo, "global variable '%s' accessed from modules with inconsistent replication count", pMgv->m_gblName.c_str());

			} else {
				// add new global variable to unit list
				m_ngvList.push_back(new CNgvInfo());
				m_ngvList.back()->m_modInfoList.push_back(CNgvModInfo(pMod, pMgv));
				m_ngvList.back()->m_ramType = pMgv->m_ramType;
				pMgv->m_pNgvInfo = m_ngvList.back();

				pMgv->m_pNgvInfo->m_ngvReplCnt = pMod->m_instSet.GetReplCnt(0);
			}
		}
	}

	// determine which ngv can be optimized (i.e. without centralized module)
	for (size_t gvIdx = 0; gvIdx < m_ngvList.size(); gvIdx += 1) {
		CNgvInfo * pNgvInfo = m_ngvList[gvIdx];
		CRam * pGv0 = pNgvInfo->m_modInfoList[0].m_pNgv;

		vector<CNgvModInfo> &ngvModInfoList = m_ngvList[gvIdx]->m_modInfoList;

		// create ngv port list of instruction and memory writes
		pNgvInfo->m_bAllWrPortClk1x = true;
		bool bAllRdPortClk1x = true;
		pNgvInfo->m_rdPortCnt = 0;
		pNgvInfo->m_pRdMod = 0;
		pNgvInfo->m_rdModCnt = 0;
		pNgvInfo->m_bUserSpanningWrite = false;

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

			pNgvInfo->m_bUserSpanningWrite |= pModNgv->m_bSpanningWrite;
		}

		FindSpanningWriteFields(pNgvInfo);

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

		bool bNgvReg = pGv0->m_addrW == 0;

		// determine if this ram has an optimized implementation
		pNgvInfo->m_bOgv = (pNgvInfo->m_rdModCnt == 1 && !bNgvAtomic && bNgvReg) ||
			((pNgvInfo->m_bUserSpanningWrite || pNgvInfo->m_bAutoSpanningWrite) &&
			!bNgvAtomic && (pNgvInfo->m_wrPortList.size() == 1 || pNgvInfo->m_bAllWrPortClk1x && pNgvInfo->m_wrPortList.size() == 2) &&
			pNgvInfo->m_rdModCnt == 1 && (pNgvInfo->m_rdPortCnt == 1 || pNgvInfo->m_rdPortCnt == 2 && bAllRdPortClk1x));

		if (pNgvInfo->m_bOgv)
			FindMemoryReadSpanningFields(pNgvInfo);
	}
}

void CDsnInfo::InitAndValidateModNgv()
{
	for (size_t gvIdx = 0; gvIdx < m_ngvList.size(); gvIdx += 1) {
		CNgvInfo * pNgvInfo = m_ngvList[gvIdx];
		CRam * pGv0 = pNgvInfo->m_modInfoList[0].m_pNgv;

		pNgvInfo->m_ramType = eUnknownRam;

		for (size_t modIdx = 0; modIdx < pNgvInfo->m_modInfoList.size(); modIdx += 1) {
			CRam * pModNgv = pNgvInfo->m_modInfoList[modIdx].m_pNgv;

			if (pNgvInfo->m_ramType == eUnknownRam)
				pNgvInfo->m_ramType = pModNgv->m_ramType;
			else if (pNgvInfo->m_ramType != pModNgv->m_ramType)
				pNgvInfo->m_ramType = eAutoRam;
		}

		// determine type of ram
		bool bNgvReg = pGv0->m_addrW == 0;
		bool bNgvDist = !bNgvReg && pGv0->m_pNgvInfo->m_ramType != eBlockRam;
		bool bNgvBlock = !bNgvReg && !bNgvDist;

		int ngvPortCnt = (int)pNgvInfo->m_wrPortList.size();
		bool bNgvAtomicSlow = pNgvInfo->m_bNgvAtomicSlow;
		bool bNgvAtomic = pNgvInfo->m_bNgvAtomicFast || bNgvAtomicSlow;

		bool bNgvMaxSel = false;

		for (size_t modIdx = 0; modIdx < pNgvInfo->m_modInfoList.size(); modIdx += 1) {
			CRam * pModNgv = pNgvInfo->m_modInfoList[modIdx].m_pNgv;

			bNgvMaxSel |= pModNgv->m_bMaxIw || pModNgv->m_bMaxMw;
		}

		bNgvMaxSel &= bNgvDist && (!pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt > 1) && bNgvAtomicSlow ||
			bNgvBlock && (bNgvAtomic || pNgvInfo->m_ngvFieldCnt > 1);

		pNgvInfo->m_bNgvMaxSel = bNgvMaxSel;

		pNgvInfo->m_bNgvWrCompClk2x = bNgvReg && (!pNgvInfo->m_bAllWrPortClk1x && ngvPortCnt <= 2 || ngvPortCnt == 3) ||
			bNgvDist && ((!pNgvInfo->m_bAllWrPortClk1x && ngvPortCnt <= 2 || ngvPortCnt == 3) && !bNgvAtomicSlow ||
			(bNgvAtomicSlow && (!pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt > 1) && bNgvMaxSel)) ||
			bNgvBlock && ((!bNgvAtomic && pNgvInfo->m_ngvFieldCnt == 1) && (!pNgvInfo->m_bAllWrPortClk1x && ngvPortCnt <= 2 || ngvPortCnt == 3) ||
			(bNgvAtomic || pNgvInfo->m_ngvFieldCnt > 1) && bNgvMaxSel);

		pNgvInfo->m_bNgvWrDataClk2x = bNgvReg && (!pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt > 1) ||
			bNgvDist && ((!pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt > 1) && !bNgvAtomicSlow ||
			(bNgvAtomicSlow && (!pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt > 1) && bNgvMaxSel)) ||
			bNgvBlock && ((!bNgvAtomic && pNgvInfo->m_ngvFieldCnt == 1) && (!pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt > 1) ||
			(bNgvAtomic || pNgvInfo->m_ngvFieldCnt > 1) && bNgvMaxSel);

		pNgvInfo->m_bNeedQue = bNgvReg && (ngvPortCnt == 2 && !pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt >= 3) ||
			bNgvDist && ((ngvPortCnt == 2 && !pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt >= 3) && !bNgvAtomicSlow ||
			((ngvPortCnt >= 2 || !pNgvInfo->m_bAllWrPortClk1x) && bNgvAtomicSlow)) ||
			bNgvBlock && ((!bNgvAtomic && pNgvInfo->m_ngvFieldCnt == 1) && (ngvPortCnt == 2 && !pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt >= 3) ||
			(bNgvAtomic || pNgvInfo->m_ngvFieldCnt > 1));

		// 2x RR selection - one level, 2 or 3 ports, 2x wrData
		bool bRrSel2x = bNgvReg && (ngvPortCnt == 2 && !pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt == 3) ||
			bNgvDist && (ngvPortCnt == 2 && !pNgvInfo->m_bAllWrPortClk1x && !bNgvAtomicSlow || ngvPortCnt == 3 && !bNgvAtomicSlow) ||
			bNgvBlock && (ngvPortCnt == 2 && !pNgvInfo->m_bAllWrPortClk1x && !bNgvAtomic && pNgvInfo->m_ngvFieldCnt == 1
			|| ngvPortCnt == 3 && !bNgvAtomic && pNgvInfo->m_ngvFieldCnt == 1);

		// 1x RR selection - no phase select, 1x wrData
		bool bRrSel1x = bNgvDist && ngvPortCnt >= 2 && bNgvAtomicSlow && !pNgvInfo->m_bNgvMaxSel ||
			bNgvBlock && ngvPortCnt >= 2 && (bNgvAtomic || pNgvInfo->m_ngvFieldCnt >= 2) && !pNgvInfo->m_bNgvMaxSel;

		// 1x RR selection, ports split using phase, 2x wrData
		bool bRrSelAB = bNgvReg && ngvPortCnt >= 4 ||
			bNgvDist && (ngvPortCnt >= 4 && !bNgvAtomicSlow) ||
			bNgvBlock && (ngvPortCnt >= 4 && !bNgvAtomic && pNgvInfo->m_ngvFieldCnt == 1);

		bool bRrSelEO = bNgvDist && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel && (!pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt >= 2) ||
			bNgvBlock && (bNgvAtomic || pNgvInfo->m_ngvFieldCnt > 1) && pNgvInfo->m_bNgvMaxSel;

		bool bNeedAddrComp = bNgvDist && bNgvAtomicSlow && pNgvInfo->m_bNgvMaxSel && (!pNgvInfo->m_bAllWrPortClk1x || ngvPortCnt >= 2) ||
			bNgvBlock && (bNgvAtomic || pNgvInfo->m_ngvFieldCnt > 1);

		bool bNeedRamReg = bNgvDist && bNgvAtomicSlow && (ngvPortCnt >= 2 || !pNgvInfo->m_bAllWrPortClk1x) && pNgvInfo->m_bNgvMaxSel ||
			bNgvBlock && (bNgvAtomic || pNgvInfo->m_ngvFieldCnt > 1) && pNgvInfo->m_bNgvMaxSel;

		bool bNeedRamWrReg = bNgvDist && bNgvAtomicSlow && (ngvPortCnt >= 2 || !pNgvInfo->m_bAllWrPortClk1x) && pNgvInfo->m_bNgvMaxSel ||
			bNgvBlock && (bNgvAtomic || pNgvInfo->m_ngvFieldCnt > 1) && pNgvInfo->m_bNgvMaxSel;

		int stgIdx = 0;
		pNgvInfo->m_wrCompStg = -1;
		if (bRrSel1x) {
			stgIdx += 1;
			pNgvInfo->m_wrCompStg = 1;
		}
		else if (bRrSel2x) {
			if ((pNgvInfo->m_ngvFieldCnt != 1 || bNgvAtomic) && !bNgvReg) stgIdx += 1;
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
		else if (ngvPortCnt == 1 && pNgvInfo->m_ngvFieldCnt == 1 && !bNgvAtomic) {
		}
		else if (ngvPortCnt == 1 && (pNgvInfo->m_ngvFieldCnt > 1 || bNgvAtomic)) {
			if (bNeedAddrComp)
				pNgvInfo->m_wrCompStg = stgIdx + 1;
		}
		else if (ngvPortCnt == 2 && pNgvInfo->m_bAllWrPortClk1x) {
			if (bNeedAddrComp)
				pNgvInfo->m_wrCompStg = stgIdx + 1;
		}

		if (!(pNgvInfo->m_ngvFieldCnt > 1 || bNgvAtomic) && ngvPortCnt > 1) stgIdx += 1;

		if (pNgvInfo->m_ngvFieldCnt > 1 || bNgvAtomic) {
			if (bNgvBlock) stgIdx += 1;
			if (bNeedRamReg) stgIdx += 1;
		}
		if (bNeedRamWrReg) stgIdx += 1;
		if ((pNgvInfo->m_ngvFieldCnt > 1 || bNgvAtomic) && !bNeedRamWrReg) stgIdx += 1;

		pNgvInfo->m_wrDataStg = stgIdx;

#ifdef WIN32
		//printf("%s: m_bOgv = %d\n", pGv0->m_gblName.c_str(), pNgvInfo->m_bOgv);
		//if (pNgvInfo->m_bOgv) {
		//	printf("  Auto=%d, User=%d\n", pNgvInfo->m_bAutoSpanningWrite, pNgvInfo->m_bUserSpanningWrite);

		//	for (size_t idx1 = 0; idx1 < pNgvInfo->m_spanningFieldList.size(); idx1 += 1) {
		//		CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[idx1];

		//		printf("    %s   p=%d, w=%d, f=%d, i=%d s=%d\n", fld1.m_heirName.c_str(),
		//			fld1.m_pos, fld1.m_width,
		//			fld1.m_bForced, fld1.m_bIgnore, fld1.m_bSpanning);
		//	}
		//}

		//bool bPossibleOgv = !pNgvInfo->m_bOgv && pNgvInfo->m_bAutoSpanningWrite && 
		//	(!bNgvAtomic && (pNgvInfo->m_wrPortList.size() == 1 || pNgvInfo->m_bAllWrPortClk1x && pNgvInfo->m_wrPortList.size() == 2) &&
		//	pNgvInfo->m_rdModCnt == 1 && (pNgvInfo->m_rdPortCnt == 1 || pNgvInfo->m_rdPortCnt == 2 && bAllRdPortClk1x));

		//if (!pNgvInfo->m_bOgv && !bNgvAtomic)
		//{
		//	printf("** %s %s\n", pGv0->m_gblName.c_str(), bPossibleOgv ? "yes" : "no");
		//	printf("**   spanning fields %d\n", (int)pNgvInfo->m_spanningFieldList.size());

		//	for (size_t modIdx = 0; modIdx < ngvModInfoList.size(); modIdx += 1) {
		//		CModule * pMod = ngvModInfoList[modIdx].m_pMod;
		//		CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

		//		for (int imIdx = 0; imIdx < 2; imIdx += 1) {
		//			if (imIdx == 0 && !pModNgv->m_bWriteForInstrWrite) continue;
		//			if (imIdx == 1 && !pModNgv->m_bWriteForMifRead) continue;

		//			printf("**   Wr Mod %s, %s\n", pMod->m_modName.c_str(), pMod->m_clkRate == eClk1x ? "1x" : "2x");
		//		}

		//		for (int imIdx = 0; imIdx < 2; imIdx += 1) {
		//			if (imIdx == 0 && !pModNgv->m_bReadForInstrRead) continue;
		//			if (imIdx == 1 && !pModNgv->m_bReadForMifWrite) continue;

		//			printf("**   Rd Mod %s, %s\n", pMod->m_modName.c_str(), pMod->m_clkRate == eClk1x ? "1x" : "2x");
		//		}
		//	}
		//}
#endif
	}

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];

		if (!pMod->m_bIsUsed || pMod->m_ngvList.size() == 0) continue;

		pMod->m_bGvIwComp = false;
		pMod->m_gvIwCompStg = max(pMod->m_stage.m_execStg.AsInt(), pMod->m_stage.m_privWrStg.AsInt());

		// create unique list of global variables for entire unit
		for (size_t mgvIdx = 0; mgvIdx < pMod->m_ngvList.size(); mgvIdx += 1) {
			CRam * pMgv = pMod->m_ngvList[mgvIdx];

			int lastStg = 1;
			if (pMgv->m_wrStg.size() > 0)
				lastStg = pMgv->m_wrStg.AsInt();
			else {
				lastStg = pMod->m_stage.m_execStg.AsInt();
				pMgv->m_wrStg.SetValue(lastStg);
			}

			if (pMgv->m_pNgvInfo->m_bOgv) continue;

			pMod->m_bGvIwComp |= pMgv->m_bWriteForInstrWrite;

			pMod->m_gvIwCompStg = max(pMod->m_gvIwCompStg, lastStg);
		}
	}
}

struct CRange { int m_start, m_end; };

void CDsnInfo::FindSpanningWriteFields(CNgvInfo * pNgvInfo)
{
	CRam * pNgv = pNgvInfo->m_modInfoList[0].m_pNgv;

	if (pNgv->m_pType->IsInt()) {
		pNgvInfo->m_spanningFieldList.push_back(CSpanningField());

		CSpanningField & fld = pNgvInfo->m_spanningFieldList.back();
		fld.m_lineInfo = pNgv->m_lineInfo;
		fld.m_ramName = pNgv->m_gblName;
		fld.m_heirName = "";
		fld.m_pos = 0;
		fld.m_width = pNgv->m_pType->m_clangBitWidth;
		fld.m_bForced = false;
		fld.m_pType = pNgv->m_pType;
		fld.m_bSpanning = true;

		pNgvInfo->m_bAutoSpanningWrite = true;

		return;
	}

	pNgvInfo->m_bAutoSpanningWrite = false;

	{ // create list of all fields
		for (CStructElemIter iter(this, pNgv->m_pType); !iter.end(); iter++) {

			pNgvInfo->m_spanningFieldList.push_back(CSpanningField());

			CSpanningField & fld = pNgvInfo->m_spanningFieldList.back();
			fld.m_lineInfo = iter->m_lineInfo;
			fld.m_pField = &iter();
			fld.m_heirName = iter.GetHeirFieldName();
			fld.m_pos = iter.GetHeirFieldPos();
			fld.m_width = iter.GetWidth();
			fld.m_bForced = iter->m_bSpanningFieldForced;
			fld.m_pType = iter->m_pType;
		}
	}

	{ // first step is mark all fields where a forced spanning fields overlaps
		for (size_t idx1 = 0; idx1 < pNgvInfo->m_spanningFieldList.size(); idx1 += 1) {
			CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[idx1];

			if (!fld1.m_bForced) continue;

			int fp1 = fld1.m_pos;
			int fw1 = fld1.m_width;

			for (size_t idx2 = 0; idx2 < pNgvInfo->m_spanningFieldList.size(); idx2 += 1) {
				CSpanningField & fld2 = pNgvInfo->m_spanningFieldList[idx2];

				if (&fld1 == &fld2) continue;

				int fp2 = fld2.m_pos;
				int fw2 = fld2.m_width;

				if (fp1 + fw1 <= fp2 || fp2 + fw2 <= fp1)
					continue;

				// some overlap found
				if (fld2.m_bForced) {
					if (!pNgvInfo->m_bUserSpanningWrite) return;
					ParseMsg(Error, fld1.m_lineInfo, "Overlapping forced spanning fields");
					ParseMsg(Info, fld1.m_lineInfo, "  %s", fld1.m_heirName.c_str());
					ParseMsg(Info, fld2.m_lineInfo, "  %s", fld2.m_heirName.c_str());
					return;
				}

				fld2.m_bIgnore = true;
			}
		}
	}

	{ // second step is to mark spanning fields
		for (size_t idx1 = 0; idx1 < pNgvInfo->m_spanningFieldList.size(); idx1 += 1) {
			CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[idx1];

			int fp1 = fld1.m_pos;
			int fw1 = fld1.m_width;

			fld1.m_bSpanning = !fld1.m_bIgnore;

			if (fld1.m_bForced || fld1.m_bIgnore) continue;

			for (size_t idx2 = 0; idx2 < pNgvInfo->m_spanningFieldList.size(); idx2 += 1) {
				CSpanningField & fld2 = pNgvInfo->m_spanningFieldList[idx2];

				if (&fld1 == &fld2) continue;
				if (fld2.m_bIgnore) continue;

				int fp2 = fld2.m_pos;
				int fw2 = fld2.m_width;

				if (fp1 + fw1 <= fp2 || fp2 + fw2 <= fp1)
					continue;

				if (fld2.m_bForced) {
					fld1.m_bSpanning = false;
					break;
				}

				// mark spanning field
				if (fp1 == fp2 && fw1 == fw2) {
					// iter and iter2 have same bit ranges
					fld2.m_bDupRange = !fld1.m_bDupRange;
					fld2.m_pDupField = &fld1;
				} else if (fp1 <= fp2 && fp1 + fw1 >= fp2 + fw2) {
					// iter2 is subrange of iter
				} else if (fp2 <= fp1 && fp2 + fw2 >= fp1 + fw1) {
					// iter is subrange of iter2
					fld1.m_bSpanning = false;
				} else {
					// one field is not subrange of other
					fld1.m_bSpanning = false;
				}
			}
		}
	}

	{ // verify all fields are covered by one or more spanning fields
		vector<CRange> rangeList;

		// create list of bit ranges that spanning fields cover
		for (size_t idx1 = 0; idx1 < pNgvInfo->m_spanningFieldList.size(); idx1 += 1) {
			CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[idx1];

			if (!fld1.m_bSpanning) continue;

			int fs = fld1.m_pos;
			int fe = fs + fld1.m_width;

			bool bFound = false;
			for (size_t idx2 = 0; idx2 < rangeList.size(); idx2 += 1) {
				int rs = rangeList[idx2].m_start;
				int re = rangeList[idx2].m_end;

				if (rs <= fs && fe <= re) {
					bFound = true;
					break;
				}

				if (fs < rs) {
					rangeList.insert(rangeList.begin()+idx2, CRange());
					rangeList[idx2].m_start = fs;
					rangeList[idx2].m_end = fe;
					if (rangeList.size() - 1 > idx2 && rangeList[idx2 + 1].m_start == fe) {
						rangeList[idx2].m_end = rangeList[idx2 + 1].m_end;
						rangeList.erase(rangeList.begin() + idx2 + 1);
					}
					bFound = true;
					break;
				}

				if (re == fs) {
					rangeList[idx2].m_end = fe;
					if (rangeList.size() - 1 > idx2 && rangeList[idx2 + 1].m_start == fe) {
						rangeList[idx2].m_end = rangeList[idx2 + 1].m_end;
						rangeList.erase(rangeList.begin() + idx2 + 1);
					}
					bFound = true;
					break;
				}
			}

			if (!bFound) {
				rangeList.push_back(CRange());
				rangeList.back().m_start = fs;
				rangeList.back().m_end = fe;
			}
		}

		// verify that all fields are covered by range list
		bool bErrorPrinted = false;
		for (size_t idx1 = 0; idx1 < pNgvInfo->m_spanningFieldList.size(); idx1 += 1) {
			CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[idx1];

			int fs = fld1.m_pos;
			int fe = fs + fld1.m_width;

			bool bFound = false;
			for (size_t idx2 = 0; idx2 < rangeList.size(); idx2 += 1) {
				int rs = rangeList[idx2].m_start;
				int re = rangeList[idx2].m_end;

				if (rs <= fs && fe <= re) {
					bFound = true;
					break;
				}
			}

			if (!bFound) {
				if (!pNgvInfo->m_bUserSpanningWrite) return;
				if (!bErrorPrinted) {
					ParseMsg(Error, pNgv->m_lineInfo, "One or more fields within global variable were not covered by identified spanning fields");
					ParseMsg(Info, pNgv->m_lineInfo, "  List of spanning fields:");

					for (size_t idx3 = 0; idx3 < pNgvInfo->m_spanningFieldList.size(); idx3 += 1) {
						CSpanningField & fld3 = pNgvInfo->m_spanningFieldList[idx3];
						if (!fld3.m_bSpanning) continue;
						ParseMsg(Info, fld3.m_lineInfo, "    %s", fld3.m_heirName.c_str());
					}

					ParseMsg(Info, pNgv->m_lineInfo, "  List of fields not covered:");
				}

				ParseMsg(Info, fld1.m_lineInfo, "    %s", fld1.m_heirName.c_str());
			}
		}
	}

	{ // verify that elements of an array have same spanning flag value
		vector<CSpanningField *> fieldList;
		vector<bool> errorList;

		for (size_t idx1 = 0; idx1 < pNgvInfo->m_spanningFieldList.size(); idx1 += 1) {
			CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[idx1];

			size_t fldIdx;
			for (fldIdx = 0; fldIdx < fieldList.size(); fldIdx += 1) {
				if (fld1.m_pField == fieldList[fldIdx]->m_pField) {
					if (fld1.m_bSpanning != fieldList[fldIdx]->m_bSpanning && !errorList[fldIdx]) {
						if (!pNgvInfo->m_bUserSpanningWrite) return;
						ParseMsg(Error, fld1.m_lineInfo, "array elements have inconsistent write spanning");
						errorList[fldIdx] = true;
					}
					break;
				}
			}

			if (fldIdx == fieldList.size()) {
				fieldList.push_back(&fld1);
				errorList.push_back(false);
			}
		}
	}

	if (!pNgvInfo->m_bUserSpanningWrite && pNgvInfo->m_rdModCnt == 1) { // check for overlapping fields

		bool bOverlappingFields = false;

		for (size_t idx1 = 0; idx1 < pNgvInfo->m_spanningFieldList.size(); idx1 += 1) {
			CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[idx1];

			int fp1 = fld1.m_pos;
			int fw1 = fld1.m_width;

			for (size_t idx2 = 0; idx2 < pNgvInfo->m_spanningFieldList.size(); idx2 += 1) {
				CSpanningField & fld2 = pNgvInfo->m_spanningFieldList[idx2];

				if (&fld1 == &fld2) continue;

				int fp2 = fld2.m_pos;
				int fw2 = fld2.m_width;

				if (fp1 == fp2 && fw1 == fw2) continue;	// exact overlap is allowed
				if (fp1 + fw1 <= fp2 || fp2 + fw2 <= fp1) continue; // no overlap

				// some overlap found
				bOverlappingFields = true;
			}
		}

		pNgvInfo->m_bAutoSpanningWrite = !bOverlappingFields;

		// check if we can auto enable spanning writes
		if (false && !bOverlappingFields) {
			int rdModIdx;
			for (rdModIdx = 0; pNgvInfo->m_modInfoList[rdModIdx].m_pMod != pNgvInfo->m_pRdMod; rdModIdx += 1);
			ERamType ramType = pNgvInfo->m_modInfoList[rdModIdx].m_pNgv->m_ramType;

			if (ramType == eBlockRam) {
				// calculate number of block rams for both implementations (centralized and local)
				int bramWidth = 0;
				if (pNgv->m_addrW <= 9)
					bramWidth = 36;
				else if (pNgv->m_addrW == 10)
					bramWidth = 18;
				else
					bramWidth = 9;

				int centralizedCopies = pNgvInfo->m_rdPortCnt + 1;
				int centralizedBramCnt = centralizedCopies * ((pNgv->m_pType->GetPackedBitWidth() + bramWidth - 1) / bramWidth);

				int localBramCnt = 0;

				for (size_t idx1 = 0; idx1 < pNgvInfo->m_spanningFieldList.size(); idx1 += 1) {
					CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[idx1];

					if (!fld1.m_bSpanning) continue;

					int localCopies = pNgvInfo->m_rdPortCnt;
					localBramCnt += localCopies * ((fld1.m_width + bramWidth - 1) / bramWidth);
				}

				pNgvInfo->m_bAutoSpanningWrite = localBramCnt <= centralizedBramCnt;
			}

			if (ramType == eDistRam) {
				// calculate number of distributed rams for both implementations (centralized and local)
				int dramWidth = 0;
				if (pNgv->m_addrW <= 5)
					dramWidth = 6;
				else
					dramWidth = 3;

				int centralizedCopies = pNgvInfo->m_rdPortCnt + 1;
				int centralizedSliceCnt = centralizedCopies * ((pNgv->m_pType->GetPackedBitWidth() + dramWidth - 1) / dramWidth);

				int localSliceCnt = 0;

				for (size_t idx1 = 0; idx1 < pNgvInfo->m_spanningFieldList.size(); idx1 += 1) {
					CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[idx1];

					if (!fld1.m_bSpanning) continue;

					int localCopies = pNgvInfo->m_rdPortCnt;
					localSliceCnt += localCopies * ((fld1.m_width + dramWidth - 1) / dramWidth);
				}

				pNgvInfo->m_bAutoSpanningWrite = localSliceCnt <= centralizedSliceCnt;
			}
		}
	}

	// set ram name for each spanning field
	if (pNgvInfo->m_bUserSpanningWrite || pNgvInfo->m_bAutoSpanningWrite) {
		if (pNgvInfo->m_spanningFieldList.size() == 0) {
			HtlAssert(0);	// this should be handled at entry of routine
			pNgvInfo->m_spanningFieldList.push_back(CSpanningField());
			CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[0];
			fld1.m_ramName = pNgv->m_gblName;
			fld1.m_pType = pNgv->m_pType;
		} else if (pNgvInfo->m_spanningFieldList.size() == 1) {
			CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[0];
			fld1.m_ramName = pNgv->m_gblName;
		} else {
			int spanningFldIdx = 1;
			for (size_t idx1 = 0; idx1 < pNgvInfo->m_spanningFieldList.size(); idx1 += 1) {
				CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[idx1];
				if (!fld1.m_bSpanning) continue;
				fld1.m_ramName = VA("%sFld%d", pNgv->m_gblName.c_str(), spanningFldIdx++);
			}
		}
	}

	//for (size_t idx1 = 0; idx1 < pNgvInfo->m_spanningFieldList.size(); idx1 += 1) {
	//	CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[idx1];

	//	printf("  %s   p=%d, w=%d, f=%d, i=%d s=%d\n", fld1.m_heirName.c_str(),
	//		fld1.m_pos, fld1.m_width,
	//		fld1.m_bForced, fld1.m_bIgnore, fld1.m_bSpanning);
	//}
}

void CDsnInfo::FindMemoryReadSpanningFields(CNgvInfo * pNgvInfo)
{
	// verify that fields that are the destination of a memory read are marked as spanning fields
	CModule * pMod = pNgvInfo->m_pRdMod;
	CRam * pNgv = pNgvInfo->m_modInfoList[0].m_pNgv;

	if (pNgv->m_addrW == 0) return;

	for (size_t wrSrcIdx = 0; wrSrcIdx < pMod->m_mif.m_mifWr.m_wrSrcList.size(); wrSrcIdx += 1) {
		CMifWrSrc & wrSrc = pMod->m_mif.m_mifWr.m_wrSrcList[wrSrcIdx];

		if (wrSrc.m_pGblVar != pNgv) continue;

		string dstName = wrSrc.m_var;

		// skip to first field name
		char const * pStr = dstName.c_str();
		while (*pStr != '.' && *pStr != '\0') pStr += 1;

		dstName = pStr;

		for (size_t idx1 = 0; idx1 < pNgvInfo->m_spanningFieldList.size(); idx1 += 1) {
			CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[idx1];

			string fldName = fld1.m_heirName;

			// compare strings for match
			char const * pDst = dstName.c_str();
			char const * pFld = fldName.c_str();

			while (*pDst && *pFld) {
				if (*pDst != *pFld)
					break;
				
				if (*pDst == '[') {
					pDst += 1; pFld += 1;

					if (*pDst == ']' || *pDst == '#') {
						// match until ']'
						while (*pDst != ']' && *pDst != '\0') pDst += 1;
						if (*pDst == ']') pDst += 1;
						while (*pFld != ']' && *pFld != '\0') pFld += 1;
						if (*pFld == ']') pFld += 1;
					} else {
						// exact match for integer constant index
						HtlAssert(isdigit(*pDst));
					}
					continue;
				}

				pDst += 1;
				pFld += 1;
			}

			if (*pDst != '\0') continue;

			// match - verify field is a spanning field and mark as a memory read destination
			if (!fld1.m_bSpanning) {
				ParseMsg(Error, wrSrc.m_lineInfo, "spanning write specified for global variable and destination not a spanning field");
				HtlAssert(pNgvInfo->m_bUserSpanningWrite);
			}

			fld1.m_bMrField = true;
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

	string gblRegReset = pMod->m_clkRate == eClk2x ? "c_reset2x" : "r_reset1x";

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
		if (pMod->m_stage.m_bStageNums || stgLast > 1)
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

		{
			string tabs = "\t";
			CLoopInfo loopInfo(gblPreInstr, tabs, pGv->m_dimenList, pGv->m_wrStg.AsInt() > 1 ? 2 : 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				for (int gvWrStg = pMod->m_tsStg; gvWrStg < pMod->m_tsStg + pGv->m_wrStg.AsInt(); gvWrStg += 1) {
					if (bPrivGblAndNoAddr && gvWrStg == pMod->m_tsStg) {
						if (pMod->m_threads.m_htIdW.AsInt() == 0) {
							gblPreInstr.Append("%sc_t%d_%sIwData%s.InitData(%s%sr__GBL__%s%s);\n", tabs.c_str(),
								gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
								htIdStr.c_str(), htIdStr.size() > 0 ? ", " : "",
								pGv->m_gblName.c_str(), dimIdx.c_str());
						} else {
							gblPreInstr.Append("%sc_t%d_%sIwData%s.InitData(%s%sr_t%d_%sIrData%s);\n", tabs.c_str(),
								gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
								htIdStr.c_str(), htIdStr.size() > 0 ? ", " : "",
								gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
						}
					} else if (!bPrivGblAndNoAddr && gvWrStg == pMod->m_tsStg) {
						gblPreInstr.Append("%sc_t%d_%sIwData%s.InitZero(%s);\n", tabs.c_str(),
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
							htIdStr.c_str());
					} else {
						gblPreInstr.Append("%sc_t%d_%sIwData%s = r_t%d_%sIwData%s;\n", tabs.c_str(),
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
					}
				}
			} while (loopInfo.Iter());
		}

		{
			string tabs = "\t";
			CLoopInfo loopInfo(gblReg, tabs, pGv->m_dimenList, pGv->m_wrStg.AsInt());
			do {
				string dimIdx = loopInfo.IndexStr();

				for (int gvWrStg = pMod->m_tsStg; gvWrStg < pMod->m_tsStg + pGv->m_wrStg.AsInt(); gvWrStg += 1) {
					gblReg.Append("%sr_t%d_%sIwData%s = c_t%d_%sIwData%s;\n", tabs.c_str(),
						gvWrStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
				}
			} while (loopInfo.Iter());
		}
	}

	if (pGv->m_bWriteForMifRead) {
		int rdRspStg = pMod->m_mif.m_mifRd.m_rdRspStg;

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

		{
			string tabs = "\t";
			CLoopInfo loopInfo(gblPreInstr, tabs, pGv->m_dimenList, 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				gblPreInstr.Append("%sc_m%d_%sMwData%s.InitZero(%s);\n", tabs.c_str(),
					rdRspStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
					htIdStr.c_str());
			} while (loopInfo.Iter());
		}

		{
			string tabs = "\t";
			CLoopInfo loopInfo(gblReg, tabs, pGv->m_dimenList, 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				gblReg.Append("%sr_m%d_%sMwData%s = c_m%d_%sMwData%s;\n", tabs.c_str(),
					rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
					rdRspStg, pGv->m_gblName.c_str(), dimIdx.c_str());

			} while (loopInfo.Iter());
		}
	}

	bool bFoundNonSpanning = false;
	for (size_t idx1 = 0; idx1 < pNgvInfo->m_spanningFieldList.size(); idx1 += 1) {
		CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[idx1];

		if (!fld1.m_bSpanning) {
			bFoundNonSpanning = true;
			break;
		}
	}

	if (bFoundNonSpanning && pGv->m_bWriteForInstrWrite && pGv->m_ramType != eRegRam) {
		string tabs = "\t";
		CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 3);
		do {
			string dimIdx = loopInfo.IndexStr();

			for (size_t idx1 = 0; idx1 < pNgvInfo->m_spanningFieldList.size(); idx1 += 1) {
				CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[idx1];

				if (fld1.m_bSpanning) continue;

				gblPostInstr.Append("%sassert_msg(!c_t%d_%sIwData%s%s.GetWrEn(), \"Runtime check failed in CPers%s::Pers%s_%dx() - instruction write to non-spanning field %s%s\");\n", tabs.c_str(),
					pMod->m_tsStg, pGv->m_gblName.c_str(), dimIdx.c_str(), fld1.m_heirName.c_str(),
					pMod->m_modName.Uc().c_str(), pMod->m_modName.Uc().c_str(), pMod->m_clkRate == eClk1x ? 1 : 2,
					pGv->m_gblName.c_str(), fld1.m_heirName.c_str());
			}
		} while (loopInfo.Iter());
	}

	if (bFoundNonSpanning && pGv->m_bWriteForMifRead && pGv->m_ramType != eRegRam) {
		int rdRspStg = pMod->m_mif.m_mifRd.m_rdRspStg;

		string tabs = "\t";
		CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 3);
		do {
			string dimIdx = loopInfo.IndexStr();

			for (size_t idx1 = 0; idx1 < pNgvInfo->m_spanningFieldList.size(); idx1 += 1) {
				CSpanningField & fld1 = pNgvInfo->m_spanningFieldList[idx1];

				if (fld1.m_bSpanning) continue;

				gblPostInstr.Append("%sassert_msg(!c_m%d_%sMwData%s%s.GetWrEn(), \"Runtime check failed in CPers%s::Pers%s_%dx() - memory response write to non-spanning field %s%s\");\n", tabs.c_str(),
					rdRspStg, pGv->m_gblName.c_str(), dimIdx.c_str(), fld1.m_heirName.c_str(),
					pMod->m_modName.Uc().c_str(), pMod->m_modName.Uc().c_str(), pMod->m_clkRate == eClk1x ? 1 : 2,
					pGv->m_gblName.c_str(), fld1.m_heirName.c_str());
			}
		} while (loopInfo.Iter());
	}

	// write outputs
	if (!pGv->m_bReadForInstrRead && !pGv->m_bReadForMifWrite) {
		if (pGv->m_bWriteForInstrWrite) {
			int gvWrStg = pMod->m_tsStg + pGv->m_wrStg.AsInt();

			m_gblIoDecl.Append("\tsc_out<CGW_%s> o_%sTo%s_%sIwData%s;\n",
				pGv->m_pNgvInfo->m_ngvWrType.c_str(),
				pMod->m_modName.Lc().c_str(), pRdMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

			string tabs = "\t";
			CLoopInfo loopInfo(gblOut, tabs, pGv->m_dimenList, 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				gblOut.Append("%so_%sTo%s_%sIwData%s = r_t%d_%sIwData%s;\n", tabs.c_str(),
					pMod->m_modName.Lc().c_str(), pRdMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(),
					gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());

			} while (loopInfo.Iter());
		}

		if (pGv->m_bWriteForMifRead) { // Memory
			int rdRspStg = pMod->m_mif.m_mifRd.m_rdRspStg;

			m_gblIoDecl.Append("\tsc_out<CGW_%s> o_%sTo%s_%sMwData%s;\n",
				pGv->m_pNgvInfo->m_ngvWrType.c_str(),
				pMod->m_modName.Lc().c_str(), pRdMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

			string tabs = "\t";
			CLoopInfo loopInfo(gblOut, tabs, pGv->m_dimenList, 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				gblOut.Append("%so_%sTo%s_%sMwData%s = r_m%d_%sMwData%s;\n", tabs.c_str(),
					pMod->m_modName.Lc().c_str(), pRdMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(),
					rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str());

			} while (loopInfo.Iter());
		}

		return;
	}

	if (pGv->m_addrW == 0) {
		m_gblRegDecl.Append("\t%s c__GBL__%s%s;\n",
			pGv->m_type.c_str(), pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

		GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, pGv->m_type.c_str(),
			VA("r__GBL__%s", pGv->m_gblName.c_str()), pGv->m_dimenList);

		{
			string tabs = "\t";
			CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				gblPostInstr.Append("%sc__GBL__%s%s = r__GBL__%s%s;\n", tabs.c_str(),
					pGv->m_gblName.c_str(), dimIdx.c_str(),
					pGv->m_gblName.c_str(), dimIdx.c_str());

			} while (loopInfo.Iter());
		}

		{
			string tabs = "\t";
			CLoopInfo loopInfo(gblReg, tabs, pGv->m_dimenList, 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				gblReg.Append("%sr__GBL__%s%s = c__GBL__%s%s;\n", tabs.c_str(),
					pGv->m_gblName.c_str(), dimIdx.c_str(),
					pGv->m_gblName.c_str(), dimIdx.c_str());

			} while (loopInfo.Iter());
		}
	} else {
		for (int imIdx = 0; imIdx < 2; imIdx += 1) {
			if (imIdx == 0 && !pGv->m_bReadForInstrRead) continue;
			if (imIdx == 1 && !pGv->m_bReadForMifWrite) continue;
			char const * pImStr = imIdx == 0 ? "Ir" : "Mr";

			for (size_t fldIdx = 0; fldIdx < pNgvInfo->m_spanningFieldList.size(); fldIdx += 1) {
				CSpanningField &fld = pNgvInfo->m_spanningFieldList[fldIdx];
				if (!fld.m_bSpanning || fld.m_bDupRange || imIdx == 1 && !fld.m_bMrField) continue;

				string typeName = fld.m_pType->m_typeName;
				if (fld.m_pType->IsInt()) {
					int fldWidth = fld.m_pType->AsInt()->m_fldWidth;
					if (fldWidth > 0 && fldWidth < fld.m_pType->m_clangBitWidth)
						typeName = VA("ht_%sint%d", fld.m_pType->AsInt()->m_eSign == eSigned ? "" : "u", fldWidth);
				}

				m_gblRegDecl.Append("\tht_%s_ram<%s, %d",
					pGv->m_ramType == eBlockRam ? "block" : "dist",
					typeName.c_str(), pGv->m_addrW);
				m_gblRegDecl.Append("> m__GBL__%s%s%s;\n", fld.m_ramName.c_str(), pImStr, pGv->m_dimenDecl.c_str());
			}

			EClkRate rdClk = pMod->m_clkRate;
			EClkRate wrClk = pNgvInfo->m_wrPortList.size() == 1 && pNgvInfo->m_bAllWrPortClk1x ? eClk1x : eClk2x;

			if (rdClk == wrClk) {

				string tabs = "\t";
				CLoopInfo loopInfo(gblReg, tabs, pGv->m_dimenList, 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					for (size_t fldIdx = 0; fldIdx < pNgvInfo->m_spanningFieldList.size(); fldIdx += 1) {
						CSpanningField &fld = pNgvInfo->m_spanningFieldList[fldIdx];

						if (!fld.m_bSpanning || fld.m_bDupRange || imIdx == 1 && !fld.m_bMrField) continue;

						gblReg.Append("%sm__GBL__%s%s%s.clock(%s);\n", tabs.c_str(),
							fld.m_ramName.c_str(), pImStr, dimIdx.c_str(),
							gblRegReset.c_str());
					}
				} while (loopInfo.Iter());

			} else if (rdClk == eClk1x) {
				{
					string tabs = "\t";
					CLoopInfo loopInfo(m_gblReg1x, tabs, pGv->m_dimenList, 3);
					do {
						string dimIdx = loopInfo.IndexStr();

						for (size_t fldIdx = 0; fldIdx < pNgvInfo->m_spanningFieldList.size(); fldIdx += 1) {
							CSpanningField &fld = pNgvInfo->m_spanningFieldList[fldIdx];

							if (!fld.m_bSpanning || fld.m_bDupRange || imIdx == 1 && !fld.m_bMrField) continue;

							m_gblReg1x.Append("%sm__GBL__%s%s%s.read_clock(r_reset1x);\n", tabs.c_str(),
								fld.m_ramName.c_str(), pImStr, dimIdx.c_str());
						}
					} while (loopInfo.Iter());
				}

				{
					string tabs = "\t";
					CLoopInfo loopInfo(m_gblReg2x, tabs, pGv->m_dimenList, 3);
					do {
						string dimIdx = loopInfo.IndexStr();

						for (size_t fldIdx = 0; fldIdx < pNgvInfo->m_spanningFieldList.size(); fldIdx += 1) {
							CSpanningField &fld = pNgvInfo->m_spanningFieldList[fldIdx];

							if (!fld.m_bSpanning || fld.m_bDupRange || imIdx == 1 && !fld.m_bMrField) continue;

							m_gblReg2x.Append("%sm__GBL__%s%s%s.write_clock(%s);\n", tabs.c_str(),
								fld.m_ramName.c_str(), pImStr, dimIdx.c_str(), gblRegReset.c_str());
						}
					} while (loopInfo.Iter());
				}
			} else {
				{
					string tabs = "\t";
					CLoopInfo loopInfo(m_gblReg2x, tabs, pGv->m_dimenList, 3);
					do {
						string dimIdx = loopInfo.IndexStr();

						for (size_t fldIdx = 0; fldIdx < pNgvInfo->m_spanningFieldList.size(); fldIdx += 1) {
							CSpanningField &fld = pNgvInfo->m_spanningFieldList[fldIdx];

							if (!fld.m_bSpanning || fld.m_bDupRange || imIdx == 1 && !fld.m_bMrField) continue;

							m_gblReg2x.Append("%sm__GBL__%s%s%s.read_clock(%s);\n", tabs.c_str(),
								fld.m_ramName.c_str(), pImStr, dimIdx.c_str(), gblRegReset.c_str());
						}
					} while (loopInfo.Iter());
				}

				{
					string tabs = "\t";
					CLoopInfo loopInfo(m_gblReg1x, tabs, pGv->m_dimenList, 3);
					do {
						string dimIdx = loopInfo.IndexStr();

						for (size_t fldIdx = 0; fldIdx < pNgvInfo->m_spanningFieldList.size(); fldIdx += 1) {
							CSpanningField &fld = pNgvInfo->m_spanningFieldList[fldIdx];

							if (!fld.m_bSpanning || fld.m_bDupRange || imIdx == 1 && !fld.m_bMrField) continue;

							m_gblReg1x.Append("%sm__GBL__%s%s%s.write_clock(r_reset1x);\n", tabs.c_str(),
							fld.m_ramName.c_str(), pImStr, dimIdx.c_str());
						}
					} while (loopInfo.Iter());
				}
			}
		}
	}

	if (pNgvInfo->m_modInfoList.size() > 1) {
		for (size_t i = 0; i < pNgvInfo->m_modInfoList.size(); i += 1) {
			CNgvModInfo * pModInfo = &pNgvInfo->m_modInfoList[i];
			if (pModInfo->m_pNgv->m_bWriteForInstrWrite && pModInfo->m_pMod != pMod) {
				m_gblIoDecl.Append("\tsc_in<CGW_%s> i_%sTo%s_%sIwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(),
					pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

				if (pGv->m_addrW == 0) {

					if (pModInfo->m_pMod->m_clkRate == eClk2x && pMod->m_clkRate == eClk1x) {

						GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("sc_signal<CGW_%s>", pGv->m_type.c_str()),
							VA("r_%sTo%s_%sIwData_2x", pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str()), pGv->m_dimenList);
					}

					bool bFoundReg2x = false;
					{
						string tabs = "\t";
						CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 3);
						do {
							string dimIdx = loopInfo.IndexStr();

							for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
								gblPostInstr.Append("%sif (i_%sTo%s_%sIwData%s.read()%s.GetWrEn())\n", tabs.c_str(),
									pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
								gblPostInstr.Append("%s\tc__GBL__%s%s%s = i_%sTo%s_%sIwData%s.read()%s.GetData();\n", tabs.c_str(),
									pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
									pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());

								if (pModInfo->m_pMod->m_clkRate == eClk2x && pMod->m_clkRate == eClk1x) {
									gblPostInstr.Append("%selse if (r_%sTo%s_%sIwData_2x%s.read()%s.GetWrEn())\n", tabs.c_str(),
										pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
									gblPostInstr.Append("%s\tc__GBL__%s%s%s = r_%sTo%s_%sIwData_2x%s.read()%s.GetData();\n", tabs.c_str(),
										pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
										pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());

									bFoundReg2x = true;
								}
							}
						} while (loopInfo.Iter());
					}

					if (bFoundReg2x) {
						string tabs = "\t";
						CLoopInfo loopInfo(m_gblReg2x, tabs, pGv->m_dimenList, 3);
						do {
							string dimIdx = loopInfo.IndexStr();

							for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {

								if (pModInfo->m_pMod->m_clkRate == eClk2x && pMod->m_clkRate == eClk1x) {

									m_gblReg2x.Append("%sr_%sTo%s_%sIwData_2x%s = i_%sTo%s_%sIwData%s;\n", tabs.c_str(),
										pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(),
										pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str());
								}
							}
						} while (loopInfo.Iter());
					}

				} else if (pNgvInfo->m_wrPortList.size() == 1) {

					bool bWrClk1x = pNgvInfo->m_wrPortList.size() == 1 && pNgvInfo->m_bAllWrPortClk1x;
					CHtCode & wrCode = bWrClk1x ? m_gblPostInstr1x : m_gblPostInstr2x;

					for (int imIdx = 0; imIdx < 2; imIdx += 1) {
						if (imIdx == 0 && !pGv->m_bReadForInstrRead) continue;
						if (imIdx == 1 && !pGv->m_bReadForMifWrite) continue;
						char const * pImStr = imIdx == 0 ? "Ir" : "Mr";

						string tabs = "\t";
						CLoopInfo loopInfo(wrCode, tabs, pGv->m_dimenList, 3);
						do {
							string dimIdx = loopInfo.IndexStr();

							for (size_t fldIdx = 0; fldIdx < pNgvInfo->m_spanningFieldList.size(); fldIdx += 1) {
								CSpanningField &fld = pNgvInfo->m_spanningFieldList[fldIdx];

								if (!fld.m_bSpanning) continue;

								string & ramName = fld.m_bDupRange ? fld.m_pDupField->m_ramName : fld.m_ramName;

								for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
									wrCode.Append("%sm__GBL__%s%s%s.write_addr(i_%sTo%s_%sIwData%s.read().GetAddr());\n", tabs.c_str(),
										ramName.c_str(), pImStr, dimIdx.c_str(),
										pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str());
									wrCode.Append("%sif (i_%sTo%s_%sIwData%s.read()%s.GetWrEn())\n", tabs.c_str(),
										pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
									wrCode.Append("%s\tm__GBL__%s%s%s.write_mem(i_%sTo%s_%sIwData%s.read().GetData()%s);\n", tabs.c_str(),
										ramName.c_str(), pImStr, dimIdx.c_str(),
										pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
								}
								wrCode.NewLine();

							}
						} while (loopInfo.Iter());
					}
				}
			}

			if (pModInfo->m_pNgv->m_bWriteForMifRead && pModInfo->m_pMod != pMod) { // Memory
				m_gblIoDecl.Append("\tsc_in<CGW_%s> i_%sTo%s_%sMwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(),
					pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

				if (pGv->m_addrW == 0) {
					string tabs = "\t";
					CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 3);
					do {
						string dimIdx = loopInfo.IndexStr();

						for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
							gblPostInstr.Append("%sif (i_%sTo%s_%sMwData%s.read()%s.GetWrEn())\n", tabs.c_str(),
								pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
							gblPostInstr.Append("%s\tc__GBL__%s%s%s = i_%sTo%s_%sMwData%s.read()%s.GetData();\n", tabs.c_str(),
								pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
								pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
						}
					} while (loopInfo.Iter());

				} else if (pNgvInfo->m_wrPortList.size() == 1) {

					bool bWrClk1x = pNgvInfo->m_wrPortList.size() == 1 && pNgvInfo->m_bAllWrPortClk1x;
					CHtCode & wrCode = bWrClk1x ? m_gblPostInstr1x : m_gblPostInstr2x;

					for (int imIdx = 0; imIdx < 2; imIdx += 1) {
						if (imIdx == 0 && !pGv->m_bReadForInstrRead) continue;
						if (imIdx == 1 && !pGv->m_bReadForMifWrite) continue;
						char const * pImStr = imIdx == 0 ? "Ir" : "Mr";

						string tabs = "\t";
						CLoopInfo loopInfo(wrCode, tabs, pGv->m_dimenList, 3);
						do {
							string dimIdx = loopInfo.IndexStr();

							for (size_t fldIdx = 0; fldIdx < pNgvInfo->m_spanningFieldList.size(); fldIdx += 1) {
								CSpanningField &fld = pNgvInfo->m_spanningFieldList[fldIdx];

								if (!fld.m_bSpanning) continue;

								string & ramName = fld.m_bDupRange ? fld.m_pDupField->m_ramName : fld.m_ramName;

								wrCode.Append("%sm__GBL__%s%s%s.write_addr(i_%sTo%s_%sMwData%s.read().GetAddr());\n", tabs.c_str(),
									ramName.c_str(), pImStr, dimIdx.c_str(),
									pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str());
								wrCode.Append("%sif (i_%sTo%s_%sMwData%s.read()%s.GetWrEn())\n", tabs.c_str(),
									pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(), fld.m_heirName.c_str());
								wrCode.Append("%s\tm__GBL__%s%s%s.write_mem(i_%sTo%s_%sMwData%s.read().GetData()%s);\n", tabs.c_str(),
									ramName.c_str(), pImStr, dimIdx.c_str(),
									pModInfo->m_pMod->m_modName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_gblName.c_str(), dimIdx.c_str(), fld.m_heirName.c_str());
							}
						} while (loopInfo.Iter());
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

		if (pGv->m_rdStg.AsInt() + addr0Skip + 1 <= lastStg) {
			string tabs = "\t";
			CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, lastStg - (pGv->m_rdStg.AsInt() + addr0Skip + 1) + 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				for (int gvRdStg = pGv->m_rdStg.AsInt() + addr0Skip + 1; gvRdStg <= lastStg; gvRdStg += 1) {

					gblPostInstr.Append("%sc_t%d_%sIrData%s = r_t%d_%sIrData%s;\n", tabs.c_str(),
						pMod->m_tsStg + gvRdStg - 2, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pMod->m_tsStg + gvRdStg - 2, pGv->m_gblName.c_str(), dimIdx.c_str());
				}
			} while (loopInfo.Iter());
		}

		if (pGv->m_rdStg.AsInt() + addr0Skip <= lastStg) {
			string tabs = "\t";
			CLoopInfo loopInfo(gblReg, tabs, pGv->m_dimenList, lastStg - (pGv->m_rdStg.AsInt() + addr0Skip) + 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				for (int gvRdStg = pGv->m_rdStg.AsInt() + addr0Skip; gvRdStg <= lastStg; gvRdStg += 1) {

					gblReg.Append("%sr_t%d_%sIrData%s = c_t%d_%sIrData%s;\n", tabs.c_str(),
						pMod->m_tsStg + gvRdStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pMod->m_tsStg + gvRdStg - 2, pGv->m_gblName.c_str(), dimIdx.c_str());
				}
			} while (loopInfo.Iter());
		}
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
				string tabs = "\t";
				CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblPostInstr.Append("%sc_t%d_%sIrData%s = r__GBL__%s%s;\n", tabs.c_str(),
						pMod->m_tsStg + pGv->m_rdStg.AsInt() - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}
		} else {
			int rdAddrStgNum = pMod->m_tsStg + pGv->m_rdStg.AsInt() - (pGv->m_ramType == eBlockRam ? 3 : 2);
			string rdAddrStg = (rdAddrStgNum == 1 || pGv->m_ramType == eBlockRam) ? VA("c_t%d", rdAddrStgNum) : VA("r_t%d", rdAddrStgNum);

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

			bool bNgvReg = pGv->m_addrW == 0;
			bool bNgvDist = !bNgvReg && pGv->m_ramType != eBlockRam;

			{
				string tabs = "\t";
				CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					for (size_t fldIdx = 0; fldIdx < pNgvInfo->m_spanningFieldList.size(); fldIdx += 1) {
						CSpanningField &fld = pNgvInfo->m_spanningFieldList[fldIdx];

						if (!fld.m_bSpanning || fld.m_bDupRange) continue;

						gblPostInstr.Append("%sm__GBL__%sIr%s.read_addr(c_t%d_%sRdAddr);\n", tabs.c_str(),
							fld.m_ramName.c_str(), dimIdx.c_str(),
							rdAddrStgNum, pGv->m_gblName.Lc().c_str());
					}
				} while (loopInfo.Iter());
			}

			{
				string tabs = "\t";
				CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					for (size_t fldIdx = 0; fldIdx < pNgvInfo->m_spanningFieldList.size(); fldIdx += 1) {
						CSpanningField &fld = pNgvInfo->m_spanningFieldList[fldIdx];

						if (!fld.m_bSpanning || fld.m_bDupRange) continue;

						gblPostInstr.Append("%sc_t%d_%sIrData%s%s = m__GBL__%sIr%s.read_mem();\n", tabs.c_str(),
							rdAddrStgNum + (bNgvDist ? 0 : 1), pGv->m_gblName.c_str(), dimIdx.c_str(), fld.m_heirName.c_str(),
							fld.m_ramName.c_str(), dimIdx.c_str());
					}
				} while (loopInfo.Iter());
			}
		}
	}

	if (pGv->m_bWriteForInstrWrite) {
		int gvWrStg = pMod->m_tsStg + pGv->m_wrStg.AsInt();

		if (pGv->m_addrW == 0) {
			string tabs = "\t";
			CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 3);
			do {
				string dimIdx = loopInfo.IndexStr();

				for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
					gblPostInstr.Append("%sif (r_t%d_%sIwData%s%s.GetWrEn())\n", tabs.c_str(),
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
					gblPostInstr.Append("%s\tc__GBL__%s%s%s = r_t%d_%sIwData%s%s.GetData();\n", tabs.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
				}
			} while (loopInfo.Iter());

		} else if (pNgvInfo->m_wrPortList.size() == 1) {

			for (int imIdx = 0; imIdx < 2; imIdx += 1) {
				if (imIdx == 0 && !pGv->m_bReadForInstrRead) continue;
				if (imIdx == 1 && !pGv->m_bReadForMifWrite) continue;
				char const * pImStr = imIdx == 0 ? "Ir" : "Mr";

				string tabs = "\t";
				CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					for (size_t fldIdx = 0; fldIdx < pNgvInfo->m_spanningFieldList.size(); fldIdx += 1) {
						CSpanningField &fld = pNgvInfo->m_spanningFieldList[fldIdx];

						if (!fld.m_bSpanning || imIdx == 1 && !fld.m_bMrField) continue;

						string & ramName = fld.m_bDupRange ? fld.m_pDupField->m_ramName : fld.m_ramName;

						gblPostInstr.Append("%sm__GBL__%s%s%s.write_addr(r_t%d_%sIwData%s.GetAddr());\n", tabs.c_str(),
							ramName.c_str(), pImStr, dimIdx.c_str(),
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
						gblPostInstr.Append("%sif (r_t%d_%sIwData%s%s.GetWrEn())\n", tabs.c_str(),
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(), fld.m_heirName.c_str());
						gblPostInstr.Append("%s\tm__GBL__%s%s%s.write_mem(r_t%d_%sIwData%s.GetData()%s);\n", tabs.c_str(),
							ramName.c_str(), pImStr, dimIdx.c_str(),
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(), fld.m_heirName.c_str());

						gblPostInstr.NewLine();
					}
				} while (loopInfo.Iter());
			}
		}
	}

	if (pGv->m_bWriteForMifRead) {
		int rdRspStg = pMod->m_mif.m_mifRd.m_rdRspStg;

		if (pGv->m_addrW == 0) {
			string tabs = "\t";
			CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 3);
			do {
				string dimIdx = loopInfo.IndexStr();

				for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
					gblPostInstr.Append("%sif (r_m%d_%sMwData%s%s.GetWrEn())\n", tabs.c_str(),
						rdRspStg + 1,
						pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
					gblPostInstr.Append("%s\tc__GBL__%s%s%s = r_m%d_%sMwData%s%s.GetData();\n", tabs.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
						rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
				}
			} while (loopInfo.Iter());

		} else if (pNgvInfo->m_wrPortList.size() == 1) {

			for (int imIdx = 0; imIdx < 2; imIdx += 1) {
				if (imIdx == 0 && !pGv->m_bReadForInstrRead) continue;
				if (imIdx == 1 && !pGv->m_bReadForMifWrite) continue;
				char const * pImStr = imIdx == 0 ? "Ir" : "Mr";

				string tabs = "\t";
				CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					for (size_t fldIdx = 0; fldIdx < pNgvInfo->m_spanningFieldList.size(); fldIdx += 1) {
						CSpanningField &fld = pNgvInfo->m_spanningFieldList[fldIdx];

						if (!fld.m_bSpanning || imIdx == 1 && !fld.m_bMrField) continue;

						string & ramName = fld.m_bDupRange ? fld.m_pDupField->m_ramName : fld.m_ramName;

						gblPostInstr.Append("%sm__GBL__%s%s%s.write_addr(r_m%d_%sMwData%s.GetAddr());\n", tabs.c_str(),
							ramName.c_str(), pImStr, dimIdx.c_str(),
							rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str());
						gblPostInstr.Append("%sif (r_m%d_%sMwData%s%s.GetWrEn())\n", tabs.c_str(),
							rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(), fld.m_heirName.c_str());

						gblPostInstr.Append("%s\tm__GBL__%s%s%s.write_mem(r_m%d_%sMwData%s.GetData()%s);\n", tabs.c_str(),
							ramName.c_str(), pImStr, dimIdx.c_str(),
							rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(), fld.m_heirName.c_str());

						gblPostInstr.NewLine();

					}
				} while (loopInfo.Iter());
			}
		}
	}

	if (pNgvInfo->m_wrPortList.size() == 2 && pGv->m_addrW > 0) {
		int rdRspStg = pMod->m_mif.m_mifRd.m_rdRspStg;
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

			{
				string tabs = "\t";
				CLoopInfo loopInfo(m_gblPostInstr2x, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					m_gblPostInstr2x.Append("%sc__GBL__%sPwData%s = r_phase ? %s : %s;\n", tabs.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						VA(wrPort[0].c_str(), dimIdx.c_str()).c_str(),
						VA(wrPort[1].c_str(), dimIdx.c_str()).c_str(),
						rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}

			{
				string tabs = "\t";
				CLoopInfo loopInfo(m_gblReg2x, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					m_gblReg2x.Append("%sr__GBL__%sPwData%s = c__GBL__%sPwData%s;\n", tabs.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}
		}

		{

			for (int imIdx = 0; imIdx < 2; imIdx += 1) {
				if (imIdx == 0 && !pGv->m_bReadForInstrRead) continue;
				if (imIdx == 1 && !pGv->m_bReadForMifWrite) continue;
				char const * pImStr = imIdx == 0 ? "Ir" : "Mr";

				string tabs = "\t";
				CLoopInfo loopInfo(m_gblPostInstr2x, tabs, pGv->m_dimenList, 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					for (size_t fldIdx = 0; fldIdx < pNgvInfo->m_spanningFieldList.size(); fldIdx += 1) {
						CSpanningField &fld = pNgvInfo->m_spanningFieldList[fldIdx];

						if (!fld.m_bSpanning || imIdx == 1 && !fld.m_bMrField) continue;

						string & ramName = fld.m_bDupRange ? fld.m_pDupField->m_ramName : fld.m_ramName;

						m_gblPostInstr2x.Append("%sm__GBL__%s%s%s.write_addr(r__GBL__%sPwData%s.GetAddr());\n", tabs.c_str(),
							ramName.c_str(), pImStr, dimIdx.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str());
						m_gblPostInstr2x.Append("%sif (r__GBL__%sPwData%s%s.GetWrEn())\n", tabs.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str(), fld.m_heirName.c_str());
						m_gblPostInstr2x.Append("%s\tm__GBL__%s%s%s.write_mem(r__GBL__%sPwData%s%s.GetData());\n", tabs.c_str(),
							ramName.c_str(), pImStr, dimIdx.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str(), fld.m_heirName.c_str());

						m_gblPostInstr2x.NewLine();

					}
				} while (loopInfo.Iter());
			}
		}
	}
}

void CDsnInfo::GenModNgvStatements(CInstance * pModInst)
{
	CModule * pMod = pModInst->m_pMod;

	if (pMod->m_ngvList.size() == 0)
		return;

	for (size_t gvIdx = 0; gvIdx < pMod->m_ngvList.size(); gvIdx += 1) {
		CRam * pGv = pMod->m_ngvList[gvIdx];
		CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;
		if (!pNgvInfo->m_bOgv) continue;

		GenModOptNgvStatements(pMod, pGv);
	}

	CHtCode & gblPreInstr = pMod->m_clkRate == eClk2x ? m_gblPreInstr2x : m_gblPreInstr1x;
	CHtCode & gblPostInstr = pMod->m_clkRate == eClk2x ? m_gblPostInstr2x : m_gblPostInstr1x;
	CHtCode & gblReg = pMod->m_clkRate == eClk2x ? m_gblReg2x : m_gblReg1x;
	CHtCode & gblOut = pMod->m_clkRate == eClk2x ? m_gblOut2x : m_gblOut1x;

	string gblRegReset = pMod->m_clkRate == eClk2x ? "c_reset2x" : "r_reset1x";

	string vcdModName = VA("Pers%s", pMod->m_modName.Uc().c_str());

	bool bInstrWrite = false;

	bool bFirstModVar = false;
	for (size_t gvIdx = 0; gvIdx < pMod->m_ngvList.size(); gvIdx += 1) {
		CRam * pGv = pMod->m_ngvList[gvIdx];
		CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;
		if (pNgvInfo->m_bOgv) continue;

		bInstrWrite |= pGv->m_bWriteForInstrWrite;

		int stgLast = max(pGv->m_wrStg.AsInt(), pMod->m_stage.m_privWrStg.AsInt());
		for (int stgIdx = 1; stgIdx <= stgLast; stgIdx += 1) {

			string varStg;
			if (/*pGv->m_rdStg.size() > 0 || pGv->m_wrStg.size() > 0 || */pMod->m_stage.m_bStageNums)
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

					GenModVar(eVcdUser, vcdModName, bFirstModVar,
						VA("%s const", pGv->m_type.c_str()),
						pGv->m_dimenDecl,
						VA("GR%s_%s", varStg.c_str(), pGv->m_gblName.c_str()),
						VA("r_t%d_%sIrData", pMod->m_tsStg + stgIdx - 1, pGv->m_gblName.c_str()),
						pGv->m_dimenList);
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
	}

	//CInstance * pModInst = pMod->m_instSet.GetInst(0);

	if (bInstrWrite) {
		// ht completion struct
		CRecord htComp;
		htComp.m_typeName = "CHtComp";
		htComp.m_bCStyle = true;

		if (pMod->m_threads.m_htIdW.AsInt() > 0)
			htComp.AddStructField(FindHtIntType(eUnsigned, pMod->m_threads.m_htIdW.AsInt()), "m_htId");

		htComp.AddStructField(&g_bool, "m_htCmdRdy");

		for (size_t gvIdx = 0; gvIdx < pMod->m_ngvList.size(); gvIdx += 1) {
			CRam * pGv = pMod->m_ngvList[gvIdx];
			CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;
			if (pNgvInfo->m_bOgv) continue;

			if (pGv->m_bWriteForInstrWrite)
				htComp.AddStructField(&g_bool, VA("m_%sIwComp", pGv->m_gblName.c_str()), "", "", pGv->m_dimenList);
		}

		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrIn) continue;
			if (pCxrIntf->m_pDstModInst->m_pMod->m_modName.AsStr() == "hif") continue;
			if (pCxrIntf->GetPortReplId() != 0) continue;

			htComp.AddStructField(&g_bool, VA("m_%sCompRdy", pCxrIntf->GetIntfName()), "", "", pCxrIntf->GetPortReplDimen());
		}

		if (pMod->m_threads.m_bCallFork && pMod->m_threads.m_htIdW.AsInt() > 2)
			htComp.AddStructField(&g_bool, VA("m_htPrivLkData"));

		GenUserStructs(m_gblRegDecl, &htComp, "\t");

		if (pMod->m_threads.m_htIdW.AsInt() == 0) {
			gblReg.Append("\tr_t%d_htCmdRdy = c_t%d_htCmdRdy;\n",
				pMod->m_wrStg, pMod->m_wrStg - 1);
			gblReg.NewLine();
		}
	}

	for (size_t gvIdx = 0; gvIdx < pMod->m_ngvList.size(); gvIdx += 1) {
		CRam * pGv = pMod->m_ngvList[gvIdx];
		CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;
		if (pNgvInfo->m_bOgv) continue;

		CHtCode & gblPostInstrWrData = pNgvInfo->m_bNgvWrDataClk2x ? m_gblPostInstr2x : m_gblPostInstr1x;
		CHtCode & gblPostInstrWrComp = pNgvInfo->m_bNgvWrCompClk2x ? m_gblPostInstr2x : m_gblPostInstr1x;

		CHtCode & gblRegWrData = pNgvInfo->m_bNgvWrDataClk2x ? m_gblReg2x : m_gblReg1x;
		CHtCode & gblRegWrComp = pNgvInfo->m_bNgvWrCompClk2x ? m_gblReg2x : m_gblReg1x;

		string gblRegWrDataReset = pNgvInfo->m_bNgvWrDataClk2x ? "c_reset2x" : "r_reset1x";
		string gblRegWrCompReset = pNgvInfo->m_bNgvWrCompClk2x ? "c_reset2x" : "r_reset1x";

		// global variable module I/O
		if (pGv->m_bWriteForInstrWrite) { // Instruction read
			int gvWrStg = pMod->m_tsStg + pGv->m_wrStg.AsInt();
			if (pMod->m_threads.m_htIdW.AsInt() > 0) {
				m_gblIoDecl.Append("\tsc_out<sc_uint<%s_HTID_W> > o_%sTo%s_iwHtId%s;\n",
					pMod->m_modName.Upper().c_str(),
					pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());

				string tabs = "\t";
				CLoopInfo loopInfo(gblOut, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblOut.Append("%so_%sTo%s_iwHtId%s = r_t%d_htId;\n", tabs.c_str(),
						pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						gvWrStg);

				} while (loopInfo.Iter());
			}

			{
				m_gblIoDecl.Append("\tsc_out<bool> o_%sTo%s_iwComp%s;\n",
					pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());

				string tabs = "\t";
				CLoopInfo loopInfo(gblOut, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblOut.Append("%so_%sTo%s_iwComp%s = r_t%d_%sIwComp%s;\n", tabs.c_str(),
						pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}

			{
				m_gblIoDecl.Append("\tsc_out<CGW_%s> o_%sTo%s_iwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(),
					pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());

				string tabs = "\t";
				CLoopInfo loopInfo(gblOut, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblOut.Append("%so_%sTo%s_iwData%s = r_t%d_%sIwData%s;\n", tabs.c_str(),
						pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}
		}

		if (pGv->m_bReadForInstrRead) { // Instruction read
			if (pNgvInfo->m_atomicMask != 0) {
				m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_ifWrEn%s;\n",
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				if (pMod->m_threads.m_htIdW.AsInt() > 0) {
					m_gblIoDecl.Append("\tsc_in<sc_uint<%s_HTID_W> > i_%sTo%s_ifHtId%s;\n",
						pMod->m_modName.Upper().c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				}
				m_gblIoDecl.Append("\tsc_in<%s> i_%sTo%s_ifData%s;\n",
					pGv->m_type.c_str(),
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}
		}

		if (pGv->m_bReadForInstrRead || pGv->m_bReadForMifWrite) {
			m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_wrEn%s;\n",
				pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			if (pGv->m_addrW > 0)
				m_gblIoDecl.Append("\tsc_in<ht_uint%d> i_%sTo%s_wrAddr%s;\n",
				pGv->m_addrW,
				pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			m_gblIoDecl.Append("\tsc_in<%s> i_%sTo%s_wrData%s;\n",
				pGv->m_type.c_str(),
				pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
		}

		if (pGv->m_bWriteForInstrWrite) {
			m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_iwCompRdy%s;\n",
				pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			if (pMod->m_threads.m_htIdW.AsInt() > 0) {
				m_gblIoDecl.Append("\tsc_in<sc_uint<%s_HTID_W> > i_%sTo%s_iwCompHtId%s;\n",
					pMod->m_modName.Upper().c_str(),
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}
			m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_iwCompData%s;\n",
				pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
		}

		if (pGv->m_bWriteForMifRead) { // Memory
			int rdRspStg = (pMod->m_mif.m_mifRd.m_bNeedRdRspInfoRam || pMod->m_mif.m_mifRd.m_bMultiQwRdRsp) ? 2 : 1;
			if (pMod->m_mif.m_mifRd.m_rspGrpW.AsInt() > 0) {
				m_gblIoDecl.Append("\tsc_out<sc_uint<%s_RD_GRP_ID_W> > o_%sTo%s_mwGrpId%s;\n",
					pModInst->m_instName.Upper().c_str(),
					pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());

				string tabs = "\t";
				CLoopInfo loopInfo(gblOut, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblOut.Append("%so_%sTo%s_mwGrpId%s = r_m%d_rdRspInfo.m_grpId;\n", tabs.c_str(),
						pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						rdRspStg + 1);

				} while (loopInfo.Iter());
			}

			{
				m_gblIoDecl.Append("\tsc_out<CGW_%s> o_%sTo%s_mwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(),
					pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());

				string tabs = "\t";
				CLoopInfo loopInfo(gblOut, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblOut.Append("%so_%sTo%s_mwData%s = r_m%d_%sMwData%s;\n", tabs.c_str(),
						pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}

			if (pGv->m_pNgvInfo->m_bNeedQue/* && (pMod->m_mif.m_mifRd.m_bMultiQwHostRdMif || pMod->m_mif.m_mifRd.m_bMultiQwCoprocRdMif)*/) {
				m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_mwFull%s;\n",
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}

			m_gblIoDecl.Append("\tsc_in<bool> i_%sTo%s_mwCompRdy%s;\n",
				pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			if (pMod->m_mif.m_mifRd.m_rspGrpW.AsInt() > 0) {
				m_gblIoDecl.Append("\tsc_in<sc_uint<%s_RD_GRP_ID_W> > i_%sTo%s_mwCompGrpId%s;\n",
					pModInst->m_instName.Upper().c_str(),
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}
		}

		if (pGv->m_addrW == 0) {
			if (pGv->m_bReadForInstrRead || pGv->m_bReadForMifWrite) {

				bool bFoundGblReg = false;
				{
					string tabs = "\t";
					CLoopInfo loopInfo(gblRegWrData, tabs, pGv->m_dimenList, 1);
					do {
						string dimIdx = loopInfo.IndexStr();


						if (pNgvInfo->m_bNgvWrDataClk2x && (pMod->m_clkRate == eClk1x)) {
							gblRegWrData.Append("%sif (r_%sTo%s_wrEn_2x%s)\n", tabs.c_str(),
								pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
							gblRegWrData.Append("%s\tr__GBL__%s_2x%s = r_%sTo%s_wrData_2x%s;\n", tabs.c_str(),
								pGv->m_gblName.c_str(), dimIdx.c_str(),
								pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());

							bFoundGblReg = true;
						} else {
							gblRegWrData.Append("%sif (r_%sTo%s_wrEn%s)\n", tabs.c_str(),
								pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
							gblRegWrData.Append("%s\tr__GBL__%s%s = r_%sTo%s_wrData%s;\n", tabs.c_str(),
								pGv->m_gblName.c_str(), dimIdx.c_str(),
								pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
						}
						gblRegWrData.NewLine();

					} while (loopInfo.Iter());
				}

				if (bFoundGblReg) {
					string tabs = "\t";
					CLoopInfo loopInfo(gblReg, tabs, pGv->m_dimenList, 1);
					do {
						string dimIdx = loopInfo.IndexStr();

						if (pNgvInfo->m_bNgvWrDataClk2x && (pMod->m_clkRate == eClk1x)) {
							gblReg.Append("%sr__GBL__%s%s = r__GBL__%s_2x%s;\n", tabs.c_str(),
								pGv->m_gblName.c_str(), dimIdx.c_str(),
								pGv->m_gblName.c_str(), dimIdx.c_str());
						}

					} while (loopInfo.Iter());
				}
			}
		} else {
			for (int imIdx = 0; imIdx < 2; imIdx += 1) {
				if (imIdx == 0 && !pGv->m_bReadForInstrRead) continue;
				if (imIdx == 1 && !pGv->m_bReadForMifWrite) continue;
				char const * pImStr = imIdx == 0 ? "Ir" : "Mr";

				string tabs = "\t";
				CLoopInfo loopInfo(gblPostInstrWrData, tabs, pGv->m_dimenList, 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblPostInstrWrData.Append("%sm__GBL__%s%s%s.write_addr(r_%sTo%s_wrAddr%s);\n", tabs.c_str(),
						pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrData.Append("%sif (r_%sTo%s_wrEn%s)\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrData.Append("%s\tm__GBL__%s%s%s.write_mem(r_%sTo%s_wrData%s);\n", tabs.c_str(),
						pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrData.NewLine();

				} while (loopInfo.Iter());
			}
		}

		if (pGv->m_bReadForInstrRead && pNgvInfo->m_atomicMask != 0) {
			if (pMod->m_threads.m_htIdW.AsInt() == 0) {
				if (pNgvInfo->m_bNgvWrDataClk2x == (pMod->m_clkRate == eClk2x)) {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, pGv->m_type,
						VA("r_%sIf", pGv->m_gblName.c_str()), pGv->m_dimenList);
				} else {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("sc_signal<%s>", pGv->m_type.c_str()),
						VA("r_%sIf", pGv->m_gblName.c_str()), pGv->m_dimenList);
				}
			} else {
				m_gblRegDecl.Append("\tht_dist_ram<%s, %s_HTID_W> m__GBL__%sIf%s;\n",
					pGv->m_type.c_str(),
					pMod->m_modName.Upper().c_str(),
					pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

				bool bFoundGblRegWrData = false;
				{
					string tabs = "\t";
					CLoopInfo loopInfo(gblReg, tabs, pGv->m_dimenList, 1);
					do {
						string dimIdx = loopInfo.IndexStr();

						if (pNgvInfo->m_bNgvWrDataClk2x == (pMod->m_clkRate == eClk2x)) {
							gblReg.Append("%sm__GBL__%sIf%s.clock(%s);\n", tabs.c_str(),
								pGv->m_gblName.c_str(), dimIdx.c_str(),
								gblRegReset.c_str());
						} else {
							gblReg.Append("%sm__GBL__%sIf%s.read_clock(%s);\n", tabs.c_str(),
								pGv->m_gblName.c_str(), dimIdx.c_str(),
								gblRegReset.c_str());

							bFoundGblRegWrData = true;
						}

					} while (loopInfo.Iter());
				}

				if (bFoundGblRegWrData) {
					string tabs = "\t";
					CLoopInfo loopInfo(gblRegWrData, tabs, pGv->m_dimenList, 1);
					do {
						string dimIdx = loopInfo.IndexStr();

						if (pNgvInfo->m_bNgvWrDataClk2x != (pMod->m_clkRate == eClk2x)) {
							gblRegWrData.Append("%sm__GBL__%sIf%s.write_clock(%s);\n", tabs.c_str(),
								pGv->m_gblName.c_str(), dimIdx.c_str(),
								gblRegWrDataReset.c_str());
						}

					} while (loopInfo.Iter());
				}
			}
		}

		if (pGv->m_addrW == 0) {
			if (pGv->m_bReadForInstrRead || pGv->m_bReadForMifWrite) {
				if (pNgvInfo->m_bNgvWrDataClk2x == (pMod->m_clkRate == eClk2x)) {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, pGv->m_type.c_str(),
						VA("r__GBL__%s", pGv->m_gblName.c_str()), pGv->m_dimenList);
				} else if (pMod->m_clkRate == eClk2x) {
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

				{
					string tabs = "\t";
					CLoopInfo loopInfo(gblReg, tabs, pGv->m_dimenList, 1);
					do {
						string dimIdx = loopInfo.IndexStr();

						if (pNgvInfo->m_bNgvWrDataClk2x == (pMod->m_clkRate == eClk2x)) {
							gblReg.Append("%sm__GBL__%s%s%s.clock(%s);\n", tabs.c_str(),
								pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
								gblRegReset.c_str());
						} else {
							gblReg.Append("%sm__GBL__%s%s%s.read_clock(%s);\n", tabs.c_str(),
								pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
								gblRegReset.c_str());
						}

					} while (loopInfo.Iter());
				}

				if (pNgvInfo->m_bNgvWrDataClk2x != (pMod->m_clkRate == eClk2x)) {
					string tabs = "\t";
					CLoopInfo loopInfo(gblRegWrData, tabs, pGv->m_dimenList, 1);
					do {
						string dimIdx = loopInfo.IndexStr();

						gblRegWrData.Append("%sm__GBL__%s%s%s.write_clock(%s);\n", tabs.c_str(),
							pGv->m_gblName.c_str(), pImStr, dimIdx.c_str(),
							gblRegWrDataReset.c_str());

					} while (loopInfo.Iter());
				}
			}
		}

		if (pGv->m_bWriteForInstrWrite) {
			if (pMod->m_threads.m_htIdW.AsInt() == 0) {

				if ((pMod->m_clkRate == eClk2x) != pGv->m_pNgvInfo->m_bNgvWrCompClk2x) {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "sc_signal<bool>",
						VA("r_%sIwComp", pGv->m_gblName.c_str()), pGv->m_dimenList);
				} else {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
						VA("r_%sIwComp", pGv->m_gblName.c_str()), pGv->m_dimenList);
				}

			} else {
				m_gblRegDecl.Append("\tht_dist_ram<bool, %s_HTID_W> m_%sIwComp%s[2];\n",
					pMod->m_modName.Upper().c_str(), pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

				CHtCode &gblRegMod = pMod->m_clkRate == eClk2x ? m_gblReg2x : m_gblReg1x;
				string gblRegModReset = pMod->m_clkRate == eClk2x ? "c_reset2x" : "r_reset1x";

				{
					string tabs = "\t";
					CLoopInfo loopInfo(gblRegWrComp, tabs, pGv->m_dimenList, 2);
					do {
						string dimIdx = loopInfo.IndexStr();

						if (pNgvInfo->m_bNgvWrCompClk2x == (pMod->m_clkRate == eClk2x)) {

							gblRegWrComp.Append("%sm_%sIwComp%s[0].clock(%s);\n", tabs.c_str(),
								pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegWrCompReset.c_str());
							gblRegWrComp.Append("%sm_%sIwComp%s[1].clock(%s);\n", tabs.c_str(),
								pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegWrCompReset.c_str());
						} else {
							gblRegWrComp.Append("%sm_%sIwComp%s[0].write_clock(%s);\n", tabs.c_str(),
								pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegWrCompReset.c_str());
							gblRegWrComp.Append("%sm_%sIwComp%s[1].write_clock(%s);\n", tabs.c_str(),
								pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegWrCompReset.c_str());
						}

					} while (loopInfo.Iter());
				}

				if (pNgvInfo->m_bNgvWrCompClk2x != (pMod->m_clkRate == eClk2x)) {
					string tabs = "\t";
					CLoopInfo loopInfo(gblRegMod, tabs, pGv->m_dimenList, 2);
					do {
						string dimIdx = loopInfo.IndexStr();

						gblRegMod.Append("%sm_%sIwComp%s[0].read_clock(%s);\n", tabs.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegModReset.c_str());
						gblRegMod.Append("%sm_%sIwComp%s[1].read_clock(%s);\n", tabs.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegModReset.c_str());

					} while (loopInfo.Iter());
				}

				{
					string tabs = "\t";
					CLoopInfo loopInfo(gblRegWrComp, tabs, pGv->m_dimenList, 2);
					do {
						string dimIdx = loopInfo.IndexStr();

						if (pNgvInfo->m_bNgvWrCompClk2x == (pMod->m_clkRate == eClk2x)) {

							gblRegWrComp.Append("%sm_%sIwComp%s[0].clock(%s);\n", tabs.c_str(),
								pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegWrCompReset.c_str());
							gblRegWrComp.Append("%sm_%sIwComp%s[1].clock(%s);\n", tabs.c_str(),
								pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegWrCompReset.c_str());
						} else {
							gblRegWrComp.Append("%sm_%sIwComp%s[0].write_clock(%s);\n", tabs.c_str(),
								pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegWrCompReset.c_str());
							gblRegWrComp.Append("%sm_%sIwComp%s[1].write_clock(%s);\n", tabs.c_str(),
								pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegWrCompReset.c_str());
						}

					} while (loopInfo.Iter());
				}

				if (pNgvInfo->m_bNgvWrCompClk2x != (pMod->m_clkRate == eClk2x)) {
					string tabs = "\t";
					CLoopInfo loopInfo(gblRegMod, tabs, pGv->m_dimenList, 2);
					do {
						string dimIdx = loopInfo.IndexStr();

						gblRegMod.Append("%sm_%sIwComp%s[0].read_clock(%s);\n", tabs.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegModReset.c_str());
						gblRegMod.Append("%sm_%sIwComp%s[1].read_clock(%s);\n", tabs.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str(), gblRegModReset.c_str());

					} while (loopInfo.Iter());
				}
			}
		}

		if (pGv->m_bReadForInstrRead && pNgvInfo->m_atomicMask != 0) {
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
				VA("r_%sTo%s_ifWrEn", pGv->m_gblName.c_str(), pMod->m_modName.Uc().c_str()), pGv->m_dimenList);

			string tabs = "\t";
			CLoopInfo loopInfo(gblRegWrData, tabs, pGv->m_dimenList, 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				gblRegWrData.Append("%sr_%sTo%s_ifWrEn%s = i_%sTo%s_ifWrEn%s;\n", tabs.c_str(),
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());

			} while (loopInfo.Iter());
		}

		if (pGv->m_bReadForInstrRead && pNgvInfo->m_atomicMask != 0 && pMod->m_threads.m_htIdW.AsInt() > 0) {
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("sc_uint<%s_HTID_W>", pMod->m_modName.Upper().c_str()),
				VA("r_%sTo%s_ifHtId", pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str()), pGv->m_dimenList);

			string tabs = "\t";
			CLoopInfo loopInfo(gblRegWrData, tabs, pGv->m_dimenList, 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				gblRegWrData.Append("%sr_%sTo%s_ifHtId%s = i_%sTo%s_ifHtId%s;\n", tabs.c_str(),
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());

			} while (loopInfo.Iter());
		}

		if (pGv->m_bReadForInstrRead && pNgvInfo->m_atomicMask != 0) {
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, pGv->m_type,
				VA("r_%sTo%s_ifData", pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str()), pGv->m_dimenList);

			string tabs = "\t";
			CLoopInfo loopInfo(gblRegWrData, tabs, pGv->m_dimenList, 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				gblRegWrData.Append("%sr_%sTo%s_ifData%s = i_%sTo%s_ifData%s;\n", tabs.c_str(),
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());

			} while (loopInfo.Iter());
		}

		if (pGv->m_bReadForInstrRead && pGv->m_ramType == eBlockRam) {

			bool bSignal = (pMod->m_clkRate == eClk2x) != pNgvInfo->m_bNgvWrDataClk2x;
			{
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, bSignal ? "sc_signal<bool>" : "bool",
					VA("r_g1_%sTo%s_wrEn", pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str()), pGv->m_dimenList);

				string tabs = "\t";
				CLoopInfo loopInfo(gblRegWrData, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblRegWrData.Append("%sr_g1_%sTo%s_wrEn%s = r_%sTo%s_wrEn%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}

			if (pGv->m_addrW > 0) {
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA(bSignal ? "sc_signal<ht_uint%d>" : "ht_uint%d", pGv->m_addrW),
					VA("r_g1_%sTo%s_wrAddr", pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str()), pGv->m_dimenList);

				string tabs = "\t";
				CLoopInfo loopInfo(gblRegWrData, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblRegWrData.Append("%sr_g1_%sTo%s_wrAddr%s = r_%sTo%s_wrAddr%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}

			{
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA(bSignal ? "sc_signal<%s>" : "%s", pGv->m_type.c_str()),
					VA("r_g1_%sTo%s_wrData", pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str()), pGv->m_dimenList);

				string tabs = "\t";
				CLoopInfo loopInfo(gblRegWrData, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblRegWrData.Append("%sr_g1_%sTo%s_wrData%s = r_%sTo%s_wrData%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}
		}

		if (pGv->m_bReadForInstrRead && pGv->m_ramType == eBlockRam) {
			int rdAddrStgNum = pMod->m_tsStg + pGv->m_rdStg.AsInt() - (pGv->m_ramType == eBlockRam ? 3 : 2);

			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addrW),
				VA("r_t%d_%sRdAddr", rdAddrStgNum + 1, pGv->m_gblName.Lc().c_str()));

			gblReg.Append("\tr_t%d_%sRdAddr = c_t%d_%sRdAddr;\n",
				rdAddrStgNum + 1, pGv->m_gblName.Lc().c_str(),
				rdAddrStgNum, pGv->m_gblName.Lc().c_str());
		}

		if (pGv->m_bReadForInstrRead || pGv->m_bReadForMifWrite) {
			;
			{
				if (pNgvInfo->m_bNgvWrDataClk2x && (pMod->m_clkRate == eClk1x) && pGv->m_addrW == 0) {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
						VA("r_%sTo%s_wrEn_2x", pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str()), pGv->m_dimenList);
				} else {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
						VA("r_%sTo%s_wrEn", pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str()), pGv->m_dimenList);
				}

				string tabs = "\t";
				CLoopInfo loopInfo(gblRegWrData, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					if (pNgvInfo->m_bNgvWrDataClk2x && (pMod->m_clkRate == eClk1x) && pGv->m_addrW == 0) {
						gblRegWrData.Append("%sr_%sTo%s_wrEn_2x%s = i_%sTo%s_wrEn%s;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
					} else {
						gblRegWrData.Append("%sr_%sTo%s_wrEn%s = i_%sTo%s_wrEn%s;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
					}

				} while (loopInfo.Iter());
			}

			if (pGv->m_addrW > 0) {
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("ht_uint%d", pGv->m_addrW),
					VA("r_%sTo%s_wrAddr", pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str()), pGv->m_dimenList);

				string tabs = "\t";
				CLoopInfo loopInfo(gblRegWrData, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblRegWrData.Append("%sr_%sTo%s_wrAddr%s = i_%sTo%s_wrAddr%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}

			{
				if (pNgvInfo->m_bNgvWrDataClk2x && (pMod->m_clkRate == eClk1x) && pGv->m_addrW == 0) {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, pGv->m_type,
						VA("r_%sTo%s_wrData_2x", pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str()), pGv->m_dimenList);
				} else {
					GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, pGv->m_type,
						VA("r_%sTo%s_wrData", pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str()), pGv->m_dimenList);
				}

				string tabs = "\t";
				CLoopInfo loopInfo(gblRegWrData, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					if (pNgvInfo->m_bNgvWrDataClk2x && (pMod->m_clkRate == eClk1x) && pGv->m_addrW == 0) {
						gblRegWrData.Append("%sr_%sTo%s_wrData_2x%s = i_%sTo%s_wrData%s;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
					} else {
						gblRegWrData.Append("%sr_%sTo%s_wrData%s = i_%sTo%s_wrData%s;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
					}

				} while (loopInfo.Iter());
			}
		}

		string compReset = pNgvInfo->m_bNgvWrCompClk2x ? "c_reset2x" : "r_reset1x";
		if (pGv->m_bWriteForInstrWrite) {
			if (pMod->m_threads.m_htIdW.AsInt() > 0) {
				m_gblRegDecl.Append("\tbool c_%sTo%s_iwCompRdy%s;\n",
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
				VA("r_%sTo%s_iwCompRdy", pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str()), pGv->m_dimenList);

			string tabs = "\t";
			CLoopInfo loopInfo(gblRegWrComp, tabs, pGv->m_dimenList, 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				if (pMod->m_threads.m_htIdW.AsInt() > 0) {
					gblRegWrComp.Append("%sr_%sTo%s_iwCompRdy%s = %s || c_%sTo%s_iwCompRdy%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
						compReset.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
				} else {
					gblRegWrComp.Append("%sr_%sTo%s_iwCompRdy%s = i_%sTo%s_iwCompRdy%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
				}

			} while (loopInfo.Iter());
		}

		if (pGv->m_bWriteForInstrWrite && pMod->m_threads.m_htIdW.AsInt() > 0) {
			m_gblRegDecl.Append("\tht_uint%d c_%sTo%s_iwCompHtId%s;\n",
				pMod->m_threads.m_htIdW.AsInt(),
				pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());

			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("sc_uint<%s_HTID_W>", pMod->m_modName.Upper().c_str()),
				VA("r_%sTo%s_iwCompHtId", pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str()), pGv->m_dimenList);

			string tabs = "\t";
			CLoopInfo loopInfo(gblRegWrComp, tabs, pGv->m_dimenList, 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				gblRegWrComp.Append("%sr_%sTo%s_iwCompHtId%s = %s ? (ht_uint%d)0 : c_%sTo%s_iwCompHtId%s;\n", tabs.c_str(),
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
					compReset.c_str(),
					pMod->m_threads.m_htIdW.AsInt(),
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());

			} while (loopInfo.Iter());
		}

		if (pGv->m_bWriteForInstrWrite) {
			if (pMod->m_threads.m_htIdW.AsInt() > 0) {
				m_gblRegDecl.Append("\tbool c_%sTo%s_iwCompData%s;\n",
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}

			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
				VA("r_%sTo%s_iwCompData", pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str()), pGv->m_dimenList);

			string tabs = "\t";
			CLoopInfo loopInfo(gblRegWrComp, tabs, pGv->m_dimenList, 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				if (pMod->m_threads.m_htIdW.AsInt() > 0) {
					gblRegWrComp.Append("%sr_%sTo%s_iwCompData%s = !%s && c_%sTo%s_iwCompData%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
						compReset.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
				} else {
					gblRegWrComp.Append("%sr_%sTo%s_iwCompData%s = i_%sTo%s_iwCompData%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
				}

			} while (loopInfo.Iter());
		}

		if (pGv->m_bWriteForInstrWrite && pMod->m_threads.m_htIdW.AsInt() > 0) {
			m_gblRegDecl.Append("\tbool c_%sTo%s_iwCompInit%s;\n",
				pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());

			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
				VA("r_%sTo%s_iwCompInit", pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str()), pGv->m_dimenList);

			string tabs = "\t";
			CLoopInfo loopInfo(gblRegWrComp, tabs, pGv->m_dimenList, 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				gblRegWrComp.Append("%sr_%sTo%s_iwCompInit%s = %s || c_%sTo%s_iwCompInit%s;\n", tabs.c_str(),
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
					compReset.c_str(),
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());

			} while (loopInfo.Iter());
		}

		if (pGv->m_bWriteForInstrWrite) {

			int gvWrStg = pMod->m_tsStg + pGv->m_wrStg.AsInt();

			string tabs = "\t";
			CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, pMod->m_threads.m_htIdW.AsInt() == 0 ? 1 : 2);
			do {
				string dimIdx = loopInfo.IndexStr();

				if (pMod->m_threads.m_htIdW.AsInt() == 0) {
					gblPostInstr.Append("%sc_t%d_%sIwComp%s = r_%sIwComp%s;\n", tabs.c_str(),
						gvWrStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				} else {
					gblPostInstr.Append("%sm_%sIwComp%s[0].read_addr(r_t%d_htId);\n", tabs.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(), gvWrStg - 1);
					gblPostInstr.Append("%sc_t%d_%sIwComp%s = m_%sIwComp%s[0].read_mem();\n", tabs.c_str(),
						gvWrStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());
				}

			} while (loopInfo.Iter());

			for (gvWrStg = pMod->m_tsStg + pGv->m_wrStg.AsInt(); gvWrStg <= pMod->m_gvIwCompStg; gvWrStg += 1) {
				m_gblRegDecl.Append("\tbool c_t%d_%sIwComp%s;\n",
					gvWrStg - 1, pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
					VA("r_t%d_%sIwComp", gvWrStg, pGv->m_gblName.c_str()), pGv->m_dimenList);
			}

			if (pMod->m_tsStg + pGv->m_wrStg.AsInt() <= pMod->m_gvIwCompStg) {
				string tabs = "\t";
				CLoopInfo loopInfo(gblReg, tabs, pGv->m_dimenList, pMod->m_tsStg + pGv->m_wrStg.AsInt() == pMod->m_gvIwCompStg ? 1 : 2);
				do {
					string dimIdx = loopInfo.IndexStr();

					for (gvWrStg = pMod->m_tsStg + pGv->m_wrStg.AsInt(); gvWrStg <= pMod->m_gvIwCompStg; gvWrStg += 1) {
						gblReg.Append("%sr_t%d_%sIwComp%s = c_t%d_%sIwComp%s;\n", tabs.c_str(),
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
							gvWrStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str());
					}
				} while (loopInfo.Iter());
			}

			if (pMod->m_tsStg + pGv->m_wrStg.AsInt() + 1 <= pMod->m_gvIwCompStg) {
				string tabs = "\t";
				CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, pMod->m_tsStg + pGv->m_wrStg.AsInt() + 1 == pMod->m_gvIwCompStg ? 1 : 2);
				do {
					string dimIdx = loopInfo.IndexStr();

					for (gvWrStg = pMod->m_tsStg + pGv->m_wrStg.AsInt() + 1; gvWrStg <= pMod->m_gvIwCompStg; gvWrStg += 1) {
						gblPostInstr.Append("%sc_t%d_%sIwComp%s = r_t%d_%sIwComp%s;\n", tabs.c_str(),
							gvWrStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
							gvWrStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str());
					}
				} while (loopInfo.Iter());
			}
		}

		if (pGv->m_bReadForInstrRead && pNgvInfo->m_atomicMask != 0) {
			m_gblRegDecl.Append("\t%s c_t%d_%sIfData%s;\n",
				pGv->m_type.c_str(), pMod->m_tsStg - 1, pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("%s ht_noload", pGv->m_type.c_str()),
				VA("r_t%d_%sIfData", pMod->m_tsStg, pGv->m_gblName.c_str()), pGv->m_dimenList);

			string tabs = "\t";
			CLoopInfo loopInfo(gblReg, tabs, pGv->m_dimenList, 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				gblReg.Append("%sr_t%d_%sIfData%s = c_t%d_%sIfData%s;\n", tabs.c_str(),
					pMod->m_tsStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
					pMod->m_tsStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str());

			} while (loopInfo.Iter());
		}

		bool bPrivGblAndNoAddr = pGv->m_bPrivGbl && pGv->m_addrW == pMod->m_threads.m_htIdW.AsInt();

		if (pGv->m_bReadForInstrRead) {
			int lastStg = pGv->m_bPrivGbl ? pMod->m_stage.m_privWrStg.AsInt() : max(pGv->m_wrStg.AsInt(), pMod->m_stage.m_privWrStg.AsInt());
			for (int gvRdStg = pGv->m_rdStg.AsInt(); gvRdStg <= lastStg; gvRdStg += 1) {
				m_gblRegDecl.Append("\t%s c_t%d_%sIrData%s;\n",
					pGv->m_type.c_str(), pMod->m_tsStg + gvRdStg - 2, pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("%s ht_noload", pGv->m_type.c_str()),
					VA("r_t%d_%sIrData", pMod->m_tsStg + gvRdStg - 1, pGv->m_gblName.c_str()), pGv->m_dimenList);
			}

			if (pGv->m_rdStg.AsInt() <= lastStg) {
				string tabs = "\t";
				CLoopInfo loopInfo(gblReg, tabs, pGv->m_dimenList, pGv->m_rdStg.AsInt() == lastStg ? 1 : 2);
				do {
					string dimIdx = loopInfo.IndexStr();

					for (int gvRdStg = pGv->m_rdStg.AsInt(); gvRdStg <= lastStg; gvRdStg += 1) {

						gblReg.Append("%sr_t%d_%sIrData%s = c_t%d_%sIrData%s;\n", tabs.c_str(),
							pMod->m_tsStg + gvRdStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
							pMod->m_tsStg + gvRdStg - 2, pGv->m_gblName.c_str(), dimIdx.c_str());
					}
				} while (loopInfo.Iter());
			}

			if (pGv->m_rdStg.AsInt() + 1 <= lastStg) {
				string tabs = "\t";
				CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, pGv->m_rdStg.AsInt() + 1 == lastStg ? 1 : 2);
				do {
					string dimIdx = loopInfo.IndexStr();

					for (int gvRdStg = pGv->m_rdStg.AsInt() + 1; gvRdStg <= lastStg; gvRdStg += 1) {

						gblPostInstr.Append("%sc_t%d_%sIrData%s = r_t%d_%sIrData%s;\n", tabs.c_str(),
							pMod->m_tsStg + gvRdStg - 2, pGv->m_gblName.c_str(), dimIdx.c_str(),
							pMod->m_tsStg + gvRdStg - 2, pGv->m_gblName.c_str(), dimIdx.c_str());
					}
				} while (loopInfo.Iter());
			}
		}

		if (pGv->m_bWriteForInstrWrite) {
			for (int gvWrStg = pMod->m_tsStg; gvWrStg < pMod->m_tsStg + pGv->m_wrStg.AsInt(); gvWrStg += 1) {
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
					VA("r_t%d_%sIwData", gvWrStg + 1, pGv->m_gblName.c_str()), pGv->m_dimenList);
				m_gblRegDecl.Append("\tCGW_%s c_t%d_%sIwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(), gvWrStg, pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
			}

			string htIdStr = (pMod->m_threads.m_htIdW.AsInt() > 0 &&
				(pGv->m_bPrivGbl || pGv->m_addr1Name == "htId" || pGv->m_addr2Name == "htId")) ? VA("r_t%d_htId", pMod->m_tsStg) : "";

			if (pMod->m_tsStg < pMod->m_tsStg + pGv->m_wrStg.AsInt()) {
				string tabs = "\t";
				CLoopInfo loopInfo(gblPreInstr, tabs, pGv->m_dimenList, pMod->m_tsStg + 1 == pMod->m_tsStg + pGv->m_wrStg.AsInt() ? 1 : 2);
				do {
					string dimIdx = loopInfo.IndexStr();

					for (int gvWrStg = pMod->m_tsStg; gvWrStg < pMod->m_tsStg + pGv->m_wrStg.AsInt(); gvWrStg += 1) {
						if (bPrivGblAndNoAddr && gvWrStg == pMod->m_tsStg) {
							gblPreInstr.Append("%sc_t%d_%sIwData%s.InitData(%s, r_t%d_%sIrData%s);\n", tabs.c_str(),
								gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
								htIdStr.c_str(),
								gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
						} else if (!bPrivGblAndNoAddr && gvWrStg == pMod->m_tsStg) {
							gblPreInstr.Append("%sc_t%d_%sIwData%s.InitZero(%s);\n", tabs.c_str(),
								gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
								htIdStr.c_str());
						} else {
							gblPreInstr.Append("%sc_t%d_%sIwData%s = r_t%d_%sIwData%s;\n", tabs.c_str(),
								gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
								gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
						}
					}
				} while (loopInfo.Iter());
			}

			if (pMod->m_tsStg < pMod->m_tsStg + pGv->m_wrStg.AsInt()) {
				string tabs = "\t";
				CLoopInfo loopInfo(gblReg, tabs, pGv->m_dimenList, pMod->m_tsStg + 1 == pMod->m_tsStg + pGv->m_wrStg.AsInt() ? 1 : 2);
				do {
					string dimIdx = loopInfo.IndexStr();

					for (int gvWrStg = pMod->m_tsStg; gvWrStg < pMod->m_tsStg + pGv->m_wrStg.AsInt(); gvWrStg += 1) {
						gblReg.Append("%sr_t%d_%sIwData%s = c_t%d_%sIwData%s;\n", tabs.c_str(),
							gvWrStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
					}
				} while (loopInfo.Iter());
			}
		}

		gblPostInstr.NewLine();

		if (pGv->m_bWriteForMifRead) {
			int rdRspStg = (pMod->m_mif.m_mifRd.m_bNeedRdRspInfoRam || pMod->m_mif.m_mifRd.m_bMultiQwRdRsp) ? 2 : 1;
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
				VA("r_m%d_%sMwData", rdRspStg + 1, pGv->m_gblName.c_str()), pGv->m_dimenList);
			m_gblRegDecl.Append("\tCGW_%s c_m%d_%sMwData%s;\n",
				pGv->m_pNgvInfo->m_ngvWrType.c_str(),
				rdRspStg,
				pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

			string htIdStr = (pMod->m_threads.m_htIdW.AsInt() > 0 &&
				(pGv->m_bPrivGbl || pGv->m_addr1Name == "htId" || pGv->m_addr2Name == "htId")) ? VA("r_m%d_rdRspInfo.m_htId", rdRspStg) : "";

			{
				string tabs = "\t";
				CLoopInfo loopInfo(gblPreInstr, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblPreInstr.Append("%sc_m%d_%sMwData%s.InitZero(%s);\n", tabs.c_str(),
						rdRspStg,
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						htIdStr.c_str());

				} while (loopInfo.Iter());
			}

			{
				string tabs = "\t";
				CLoopInfo loopInfo(gblReg, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblReg.Append("%sr_m%d_%sMwData%s = c_m%d_%sMwData%s;\n", tabs.c_str(),
						rdRspStg + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						rdRspStg, pGv->m_gblName.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}
		}

		if (pGv->m_bReadForInstrRead && pNgvInfo->m_atomicMask != 0) {
			if (pMod->m_threads.m_htIdW.AsInt() == 0) {

				string tabs = "\t";
				CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblPostInstr.Append("%sc_t%d_%sIfData%s = r_%sIf%s;\n", tabs.c_str(),
						pMod->m_tsStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());

			} else {
				int rdAddrStgNum = pGv->m_ramType == eBlockRam ? (pMod->m_tsStg - 2) : (pMod->m_tsStg - 1);
				string rdAddrStg = rdAddrStgNum == 1 ? "c_t1" : VA("r_t%d", rdAddrStgNum);

				string tabs = "\t";
				CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 2);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblPostInstr.Append("%sm__GBL__%sIf%s.read_addr(%s_htId);\n", tabs.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						rdAddrStg.c_str());
					gblPostInstr.Append("%sc_t%d_%sIfData%s = m__GBL__%sIf%s.read_mem();\n", tabs.c_str(),
						pMod->m_tsStg - 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}
			gblPostInstr.NewLine();
		}

		if (pGv->m_bReadForInstrRead && pNgvInfo->m_atomicMask != 0) {

			string tabs = "\t";
			CLoopInfo loopInfo(gblPostInstrWrData, tabs, pGv->m_dimenList, pMod->m_threads.m_htIdW.AsInt() == 0 ? 1 : 2);
			do {
				string dimIdx = loopInfo.IndexStr();

				if (pMod->m_threads.m_htIdW.AsInt() == 0) {
					gblPostInstrWrData.Append("%sif (r_%sTo%s_ifWrEn%s)\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrData.Append("%s\tr_%sIf%s = r_%sTo%s_ifData%s;\n", tabs.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
				} else {
					gblPostInstrWrData.Append("%sm__GBL__%sIf%s.write_addr(r_%sTo%s_ifHtId%s);\n", tabs.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrData.Append("%sif (r_%sTo%s_ifWrEn%s)\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
					gblPostInstrWrData.Append("%s\tm__GBL__%sIf%s.write_mem(r_%sTo%s_ifData%s);\n", tabs.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
				}
			} while (loopInfo.Iter());
		}
		gblPostInstrWrData.NewLine();

		if (pGv->m_bReadForInstrRead) {
			if (pGv->m_addrW == 0) {

				string tabs = "\t";
				CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList,1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblPostInstr.Append("%sc_t%d_%sIrData%s = r__GBL__%s%s;\n", tabs.c_str(),
						pMod->m_tsStg - 2 + pGv->m_rdStg.AsInt(), pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			} else {
				int rdAddrStgNum = pMod->m_tsStg + pGv->m_rdStg.AsInt() - (pGv->m_ramType == eBlockRam ? 3 : 2);
				string rdAddrStg = (rdAddrStgNum == 1 || pGv->m_ramType == eBlockRam) ? VA("c_t%d", rdAddrStgNum) : VA("r_t%d", rdAddrStgNum);

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

				string tabs = "\t";
				CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblPostInstr.Append("%sm__GBL__%sIr%s.read_addr(c_t%d_%sRdAddr);\n", tabs.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						rdAddrStgNum, pGv->m_gblName.Lc().c_str());

					bool bNgvReg = pGv->m_addrW == 0;
					bool bNgvDist = !bNgvReg && pGv->m_ramType != eBlockRam;

					if (bNgvDist) {
						gblPostInstr.Append("%sc_t%d_%sIrData%s = m__GBL__%sIr%s.read_mem();\n", tabs.c_str(),
							rdAddrStgNum, pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str());
					} else {

						gblPostInstr.Append("%sif (r_g1_%sTo%s_wrEn%s && r_g1_%sTo%s_wrAddr%s == r_t%d_%sRdAddr)\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
							rdAddrStgNum + 1, pGv->m_gblName.Lc().c_str());

						gblPostInstr.Append("%s\tc_t%d_%sIrData%s = r_g1_%sTo%s_wrData%s;\n", tabs.c_str(),
							rdAddrStgNum + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());

						gblPostInstr.Append("%selse\n", tabs.c_str());

						gblPostInstr.Append("%s\tc_t%d_%sIrData%s = m__GBL__%sIr%s.read_mem();\n", tabs.c_str(),
							rdAddrStgNum + 1, pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str());
					}
					gblPostInstr.NewLine();

				} while (loopInfo.Iter());
			}
		}

		if (pGv->m_bWriteForInstrWrite) {
			string typeStr;
			if (pMod->m_threads.m_htIdW.AsInt() == 0) {
				if (pGv->m_dimenList.size() == 0)
					typeStr = "bool ";
				else
					gblPostInstrWrComp.Append("\tbool c_%sIwComp%s;\n", pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());
			}

			{
				string tabs = "\t";
				CLoopInfo loopInfo(gblPostInstrWrComp, tabs, pGv->m_dimenList, pMod->m_threads.m_htIdW.AsInt() == 0 ? 1 : 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					if (pMod->m_threads.m_htIdW.AsInt() == 0) {
						gblPostInstrWrComp.Append("%s%sc_%sIwComp%s = r_%sTo%s_iwCompRdy%s ? !r_%sTo%s_iwCompData%s : r_%sIwComp%s;\n", tabs.c_str(),
							typeStr.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str());

					} else {
						gblPostInstrWrComp.Append("%sm_%sIwComp%s[0].write_addr(r_%sTo%s_iwCompHtId%s);\n", tabs.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
						gblPostInstrWrComp.Append("%sm_%sIwComp%s[1].write_addr(r_%sTo%s_iwCompHtId%s);\n", tabs.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
						gblPostInstrWrComp.Append("%sif (r_%sTo%s_iwCompRdy%s) {\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
						gblPostInstrWrComp.Append("%s\tm_%sIwComp%s[0].write_mem(!r_%sTo%s_iwCompData%s);\n", tabs.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
						gblPostInstrWrComp.Append("%s\tm_%sIwComp%s[1].write_mem(!r_%sTo%s_iwCompData%s);\n", tabs.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
						gblPostInstrWrComp.Append("%s}\n", tabs.c_str());
						gblPostInstrWrComp.NewLine();

						gblPostInstrWrComp.Append("%sif (r_%sTo%s_iwCompInit%s) {\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
						gblPostInstrWrComp.Append("%s\tc_%sTo%s_iwCompRdy%s = true;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
						gblPostInstrWrComp.Append("%s\tc_%sTo%s_iwCompHtId%s = r_%sTo%s_iwCompHtId%s + 1u;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
						gblPostInstrWrComp.Append("%s\tc_%sTo%s_iwCompData%s = false;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
						gblPostInstrWrComp.Append("%s\tc_%sTo%s_iwCompInit%s = r_%sTo%s_iwCompHtId%s != 0x%x;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
							(1 << pMod->m_threads.m_htIdW.AsInt()) - 2);

						gblPostInstrWrComp.Append("%s} else {\n", tabs.c_str());

						gblPostInstrWrComp.Append("%s\tc_%sTo%s_iwCompRdy%s = i_%sTo%s_iwCompRdy%s;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
						gblPostInstrWrComp.Append("%s\tc_%sTo%s_iwCompHtId%s = i_%sTo%s_iwCompHtId%s;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
						gblPostInstrWrComp.Append("%s\tc_%sTo%s_iwCompData%s = i_%sTo%s_iwCompData%s;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());
						gblPostInstrWrComp.Append("%s\tc_%sTo%s_iwCompInit%s = false;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str());

						gblPostInstrWrComp.Append("%s}\n", tabs.c_str());
					}
					gblPostInstrWrComp.NewLine();
				} while (loopInfo.Iter());
			}

			if (pMod->m_threads.m_htIdW.AsInt() == 0) {
				string tabs = "\t";
				CLoopInfo loopInfo(gblRegWrComp, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblRegWrComp.Append("%sr_%sIwComp%s = !r_reset1x && c_%sIwComp%s;\n", tabs.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}
		}

		int gvWrStg = pMod->m_tsStg + pGv->m_wrStg.AsInt();
		if (pGv->m_bWriteForInstrWrite) {

			string typeStr;
			if (pGv->m_dimenList.size() == 0)
				typeStr = "bool ";
			else {
				gblPostInstr.Append("\tbool c_t%d_%sTo%s_iwWrEn%s;\n", 
					gvWrStg, pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}

			string tabs = "\t";
			CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 1);
			do {
				string dimIdx = loopInfo.IndexStr();

				gblPostInstr.Append("%s%sc_t%d_%sTo%s_iwWrEn%s = ", tabs.c_str(),
					typeStr.c_str(),
					gvWrStg, pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str());

				string separator;
				for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
					if (iter.IsStructOrUnion()) continue;

					gblPostInstr.Append("%sr_t%d_%sIwData%s%s.GetWrEn()",
						separator.c_str(), gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
						iter.GetHeirFieldName().c_str());

					separator = VA(" ||\n%s\t", tabs.c_str());

					if (pGv->m_pType->m_eType != eRecord) continue;

					if (iter->m_atomicMask & ATOMIC_INC) {
						gblPostInstr.Append(" || r_t%d_%sIwData%s%s.GetIncEn()",
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
							iter.GetHeirFieldName().c_str());
					}
					if (iter->m_atomicMask & ATOMIC_SET) {
						gblPostInstr.Append(" || r_t%d_%sIwData%s%s.GetSetEn()",
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
							iter.GetHeirFieldName().c_str());
					}
					if (iter->m_atomicMask & ATOMIC_ADD) {
						gblPostInstr.Append(" || r_t%d_%sIwData%s%s.GetAddEn()",
							gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
							iter.GetHeirFieldName().c_str());
					}
				}

				gblPostInstr.Append(";\n");
			} while (loopInfo.Iter());

			if (pGv->m_addr1W.size() > 0 && pGv->m_addr1Name != "htId" || pGv->m_addr2W.size() > 0 && pGv->m_addr2Name != "htId") {
				string tabs = "\t";
				CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblPostInstr.Append("%sassert_msg(!c_t%d_%sTo%s_iwWrEn%s || r_t%d_%sIwData%s.IsAddrSet(),\n", tabs.c_str(),
						gvWrStg, pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						gvWrStg, pGv->m_gblName.c_str(), dimIdx.c_str());
					gblPostInstr.Append("%s\t\"%s variable %cW_%s%s was written but address was not set, use method .write_addr()\");\n", tabs.c_str(),
						pGv->m_bPrivGbl ? "private" : "global", pGv->m_bPrivGbl ? 'P' : 'G', pGv->m_gblName.Lc().c_str(), dimIdx.c_str());
				} while (loopInfo.Iter());
			}

			for (int gvIwStg = gvWrStg + 1; gvIwStg <= pMod->m_gvIwCompStg; gvIwStg += 1) {
				if (pGv->m_dimenList.size() > 0) {
					gblPostInstr.Append("\tbool c_t%d_%sTo%s_iwWrEn%s;\n",
						gvIwStg, pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				}
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool", VA("r_t%d_%sTo%s_iwWrEn", gvIwStg, pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str()), pGv->m_dimenList);

				{
					string tabs = "\t";
					CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 1);
					do {
						string dimIdx = loopInfo.IndexStr();

						gblPostInstr.Append("%s%sc_t%d_%sTo%s_iwWrEn%s = r_t%d_%sTo%s_iwWrEn%s;\n", tabs.c_str(),
							typeStr.c_str(),
							gvIwStg, pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
							gvIwStg, pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str());

					} while (loopInfo.Iter());
				}

				{
					string tabs = "\t";
					CLoopInfo loopInfo(gblReg, tabs, pGv->m_dimenList, 1);
					do {
						string dimIdx = loopInfo.IndexStr();

						gblReg.Append("%sr_t%d_%sTo%s_iwWrEn%s = c_t%d_%sTo%s_iwWrEn%s;\n", tabs.c_str(),
							gvIwStg, pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
							gvIwStg - 1, pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str());

					} while (loopInfo.Iter());
				}
			}
			gblPostInstr.NewLine();
		}
	}

	if (bInstrWrite) {
		int queDepthW = pMod->m_threads.m_htIdW.AsInt() <= 5 ? 5 : pMod->m_threads.m_htIdW.AsInt();
		m_gblRegDecl.Append("\tht_dist_que<CHtComp, %d> m_htCompQue;\n", queDepthW);
		gblReg.Append("\tm_htCompQue.clock(%s);\n", gblRegReset.c_str());

		if (pMod->m_threads.m_htIdW.AsInt() == 0) {
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

			for (size_t gvIdx = 0; gvIdx < pMod->m_ngvList.size(); gvIdx += 1) {
				CRam * pGv = pMod->m_ngvList[gvIdx];
				CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;
				if (pNgvInfo->m_bOgv) continue;

				if (!pGv->m_bWriteForInstrWrite) continue;

				m_gblRegDecl.Append("\tbool c_c1_%sIwComp%s;\n", pGv->m_gblName.c_str(), pGv->m_dimenDecl.c_str());

				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool",
					VA("r_c2_%sIwComp", pGv->m_gblName.c_str()), pGv->m_dimenList);

				string tabs = "\t";
				CLoopInfo loopInfo(gblReg, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblReg.Append("%sr_c2_%sIwComp%s = c_c1_%sIwComp%s;\n", tabs.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(), pGv->m_gblName.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}
		}

		m_gblRegDecl.Append("\tht_uint%d c_htCompQueAvlCnt;\n", queDepthW + 1);
		GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("ht_uint%d", queDepthW + 1), "r_htCompQueAvlCnt");

		m_iplEosChecks.Append("\t\tht_assert(r_htCompQueAvlCnt == %d);\n", 1 << queDepthW);

		gblReg.Append("\tr_htCompQueAvlCnt = r_reset1x ? (ht_uint%d)0x%x : c_htCompQueAvlCnt;\n", queDepthW + 1, 1 << queDepthW);

		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrIn) continue;
			if (pCxrIntf->m_pDstModInst->m_pMod->m_modName.AsStr() == "hif") continue;

			if (pCxrIntf->GetPortReplId() == 0) {
				m_gblRegDecl.Append("\tbool c_%s_%sCompRdy%s;\n", pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplDecl());
				GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool", VA("r_%s_%sCompRdy", pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName()), pCxrIntf->GetPortReplDimen());
			}

			gblReg.Append("\tr_%s_%sCompRdy%s = c_%s_%sCompRdy%s;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
		}

		if (pMod->m_threads.m_htIdW.AsInt() == 0) {
			m_gblRegDecl.Append("\tbool c_htCompRdy;\n");
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, "bool", "r_htCompRdy");

			gblReg.Append("\tr_htCompRdy = !r_reset1x && c_htCompRdy;\n");
		} else {
			m_gblRegDecl.Append("\tht_uint%d c_htCmdRdyCnt;\n",
				pMod->m_threads.m_htIdW.AsInt() + 1);
			GenModDecl(eVcdAll, m_gblRegDecl, vcdModName, VA("ht_uint%d", pMod->m_threads.m_htIdW.AsInt() + 1), "r_htCmdRdyCnt");

			gblReg.Append("\tr_htCmdRdyCnt = r_reset1x ? (ht_uint%d)0 : c_htCmdRdyCnt;\n", pMod->m_threads.m_htIdW.AsInt() + 1);
		}

		gblPostInstr.Append("\tif (r_t%d_htValid) {\n", pMod->m_gvIwCompStg);
		gblPostInstr.Append("\t\tCHtComp htComp;\n");

		if (pMod->m_threads.m_htIdW.AsInt() > 0) {
			gblPostInstr.Append("\t\thtComp.m_htId = r_t%d_htId;\n",
				pMod->m_gvIwCompStg);
		}

		for (size_t gvIdx = 0; gvIdx < pMod->m_ngvList.size(); gvIdx += 1) {
			CRam * pGv = pMod->m_ngvList[gvIdx];
			CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;
			if (pNgvInfo->m_bOgv) continue;

			if (pGv->m_bWriteForInstrWrite) {

				string tabs = "\t\t";
				CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 1);
				do {
					string dimIdx = loopInfo.IndexStr();

					gblPostInstr.Append("%shtComp.m_%sIwComp%s = c_t%d_%sTo%s_iwWrEn%s ? !r_t%d_%sIwComp%s : r_t%d_%sIwComp%s;\n", tabs.c_str(),
						pGv->m_gblName.c_str(), dimIdx.c_str(),
						pMod->m_gvIwCompStg, pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), dimIdx.c_str(),
						pMod->m_gvIwCompStg, pGv->m_gblName.c_str(), dimIdx.c_str(),
						pMod->m_gvIwCompStg, pGv->m_gblName.c_str(), dimIdx.c_str());

				} while (loopInfo.Iter());
			}
		}

		gblPostInstr.Append("\t\thtComp.m_htCmdRdy = r_t%d_htCmdRdy;\n", pMod->m_gvIwCompStg);

		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrIn) continue;

			string intfRdy;
			if (pMod->m_execStg + 1 == pMod->m_gvIwCompStg)
				intfRdy = VA("r_%s_%sRdy%s", pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			else
				intfRdy = VA("r_t%d_%s_%sRdy%s", pMod->m_gvIwCompStg, pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

			if (pCxrIntf->m_pDstModInst->m_pMod->m_modName.AsStr() == "hif") {
				gblPostInstr.Append("\t\tif (%s)\n", intfRdy.c_str());

				if (pMod->m_threads.m_htIdW.AsInt() > 0)
					gblPostInstr.Append("\t\t\tm_htIdPool.push(htComp.m_htId);\n");
				else
					gblPostInstr.Append("\t\t\tc_htBusy = false; \n");
			} else {
				gblPostInstr.Append("\t\thtComp.m_%sCompRdy%s = %s;\n",
					pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex(),
					intfRdy.c_str());
			}
		}

		if (pMod->m_threads.m_bCallFork && pMod->m_threads.m_htIdW.AsInt() > 2)
			gblPostInstr.Append("\t\thtComp.m_htPrivLkData = r_t%d_htPrivLkData;\n", pMod->m_gvIwCompStg);

		gblPostInstr.Append("\t\tm_htCompQue.push(htComp);\n");

		gblPostInstr.Append("\t}\n");
		gblPostInstr.Append("\n");

		for (size_t gvIdx = 0; gvIdx < pMod->m_ngvList.size(); gvIdx += 1) {
			CRam * pGv = pMod->m_ngvList[gvIdx];
			CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;
			if (pNgvInfo->m_bOgv) continue;

			if (pGv->m_bWriteForInstrWrite) {
				if (pMod->m_threads.m_htIdW.AsInt() > 0) {

					string tabs = "\t";
					CLoopInfo loopInfo(gblPostInstr, tabs, pGv->m_dimenList, 2);
					do {
						string dimIdx = loopInfo.IndexStr();


						gblPostInstr.Append("%sm_%sIwComp%s[1].read_addr(r_c1_htCompQueFront.m_htId);\n", tabs.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str());
						gblPostInstr.Append("%sc_c1_%sIwComp%s = m_%sIwComp%s[1].read_mem();\n", tabs.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str(),
							pGv->m_gblName.c_str(), dimIdx.c_str());

					} while (loopInfo.Iter());
				}
			}
		}

		string c2StgStr = pMod->m_threads.m_htIdW.AsInt() == 0 ? "" : "c2_";

		gblPostInstr.Append("\tbool c_%shtComp = r_%shtCompQueValid", c2StgStr.c_str(), c2StgStr.c_str());

		for (size_t gvIdx = 0; gvIdx < pMod->m_ngvList.size(); gvIdx += 1) {
			CRam * pGv = pMod->m_ngvList[gvIdx];
			CNgvInfo * pNgvInfo = pGv->m_pNgvInfo;
			if (pNgvInfo->m_bOgv) continue;

			if (pGv->m_bWriteForInstrWrite) {
				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);
					if (pMod->m_threads.m_htIdW.AsInt() == 0) {
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

		if (pMod->m_threads.m_htIdW.AsInt() == 0) {
			gblPostInstr.Append("\tbool c_htCompQueHold = r_htCompQueValid && !c_htComp;\n");
		} else {
			gblPostInstr.Append("\tbool c_c1_htCompQueReplay = !r_c2_htCompQueReplay && r_c2_htCompQueValid && !c_c2_htComp;\n");
			gblPostInstr.Append("\tbool c_htCompReplay = c_c1_htCompQueReplay || r_c2_htCompQueReplay;\n");
		}
		gblPostInstr.NewLine();

		if (pMod->m_bMultiThread && pMod->m_threads.m_bCallFork && pMod->m_bGvIwComp) {
			if (pMod->m_threads.m_htIdW.AsInt() <= 2) {
				// no code for registered version
			} else {
				for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
					CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

					if (pCxrIntf->m_cxrDir == CxrOut) continue;
					if (pCxrIntf->m_cxrType == CxrCall || pCxrIntf->m_cxrType == CxrTransfer) continue;

					gblPostInstr.Append("\tm_%s_%sPrivLk1%s.write_addr(r_c2_htCompQueFront.m_htId);\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
				gblPostInstr.Append("\tm_htCmdPrivLk1.write_addr(r_c2_htCompQueFront.m_htId);\n");

				if (pMod->m_rsmSrcCnt > 0)
					gblPostInstr.Append("\tm_rsmPrivLk1.write_addr(r_c2_htCompQueFront.m_htId);\n");

				gblPostInstr.Append("\n");
			}
		}

		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrIn) continue;
			if (pCxrIntf->m_pDstModInst->m_pMod->m_modName.AsStr() == "hif") continue;

			gblPostInstr.Append("\tc_%s_%sCompRdy%s = false;\n",
				pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
		}

		if (pMod->m_threads.m_htIdW.AsInt() == 0) {
			gblPostInstr.Append("\tif (c_htComp) {\n");
			gblPostInstr.Append("\t\tif (r_htCompQueFront.m_htCmdRdy)\n");
		} else {
			gblPostInstr.Append("\tif (!c_htCompReplay && c_c2_htComp) {\n");
			gblPostInstr.Append("\t\tif (r_c2_htCompQueFront.m_htCmdRdy)\n");
		}

		if (pMod->m_threads.m_htIdW.AsInt() == 0)
			gblPostInstr.Append("\t\t\tc_htCompRdy = true;\n");
		else
			gblPostInstr.Append("\t\t\tc_htCmdRdyCnt += 1u;\n");

		for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
			CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

			if (pCxrIntf->m_cxrDir == CxrIn) continue;
			if (pCxrIntf->m_pDstModInst->m_pMod->m_modName.AsStr() == "hif") continue;

			if (pCxrIntf->m_cxrType == CxrCall) {
				gblPostInstr.Append("\t\tif (r_%shtCompQueFront.m_%sCompRdy%s)\n",
					c2StgStr.c_str(),
					pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				gblPostInstr.Append("\t\t\tc_%s_%sCompRdy%s = true;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
			} else {
				gblPostInstr.Append("\t\tif (r_%shtCompQueFront.m_%sCompRdy%s) {\n",
					c2StgStr.c_str(),
					pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				gblPostInstr.Append("\t\t\tc_%s_%sCompRdy%s = true;\n",
					pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());

				if (pMod->m_threads.m_htIdW.AsInt() == 0) {
					gblPostInstr.Append("\t\t\tc_htBusy = false;\n");
				} else {
					gblPostInstr.Append("\t\t\tm_htIdPool.push(r_%shtCompQueFront.m_htId);\n",	c2StgStr.c_str());
				}
				gblPostInstr.Append("\t\t}\n");
			}
		}

		gblPostInstr.Append("\t\tc_htCompQueAvlCnt += 1;\n");

		if (pMod->m_bMultiThread && pMod->m_threads.m_bCallFork && pMod->m_bGvIwComp) {
			gblPostInstr.NewLine();
			if (pMod->m_threads.m_htIdW.AsInt() == 0)
				gblPostInstr.Append("\t\tc_htPrivLk = 0;\n");
			else if (pMod->m_threads.m_htIdW.AsInt() <= 2)
				gblPostInstr.Append("\t\tc_htPrivLk[INT(r_c2_htCompQueFront.m_htId)] = 0;\n");
			else {
				for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
					CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

					if (pCxrIntf->m_cxrDir == CxrOut) continue;
					if (pCxrIntf->m_cxrType == CxrCall || pCxrIntf->m_cxrType == CxrTransfer) continue;

					gblPostInstr.Append("\t\tm_%s_%sPrivLk1%s.write_mem(r_c2_htCompQueFront.m_htPrivLkData);\n",
						pCxrIntf->GetPortNameSrcToDstLc(), pCxrIntf->GetIntfName(), pCxrIntf->GetPortReplIndex());
				}
				gblPostInstr.Append("\t\tm_htCmdPrivLk1.write_mem(r_c2_htCompQueFront.m_htPrivLkData);\n");

				if (pMod->m_rsmSrcCnt > 0)
					gblPostInstr.Append("\t\tm_rsmPrivLk1.write_mem(r_c2_htCompQueFront.m_htPrivLkData);\n");
			}
		}

		gblPostInstr.Append("\t}\n");
		gblPostInstr.NewLine();

		if (pMod->m_threads.m_htIdW.AsInt() == 0) {
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

		string ngvRegWrDataReset = pNgvInfo->m_bNgvWrDataClk2x ? "c_reset2x" : "r_reset1x";

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
			CModule * pMod = ngvModInfoList[modIdx].m_pMod;
			CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

			if (pModNgv->m_bWriteForInstrWrite) {
				if (pMod->m_threads.m_htIdW.AsInt() > 0) {
					ngvIo.Append("\tsc_in<sc_uint<%s_HTID_W> > i_%sTo%s_iwHtId%s;\n",
						pMod->m_modName.Upper().c_str(), pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				}
				ngvIo.Append("\tsc_in<bool> i_%sTo%s_iwComp%s;\n",
					pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				ngvIo.Append("\tsc_in<CGW_%s> i_%sTo%s_iwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(), pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				ngvIo.Append("\n");
			}

			if (pModNgv->m_bReadForInstrRead) {
				if (pNgvInfo->m_atomicMask != 0) {
					ngvIo.Append("\tsc_out<bool> o_%sTo%s_ifWrEn%s;\n",
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
					if (pMod->m_threads.m_htIdW.AsInt() > 0) {
						ngvIo.Append("\tsc_out<sc_uint<%s_HTID_W> > o_%sTo%s_ifHtId%s;\n",
							pMod->m_modName.Upper().c_str(), pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
					}
					ngvIo.Append("\tsc_out<%s> o_%sTo%s_ifData%s;\n",
						pGv->m_type.c_str(), pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				}
			}

			if (pModNgv->m_bReadForInstrRead || pModNgv->m_bReadForMifWrite) {
				ngvIo.Append("\tsc_out<bool> o_%sTo%s_wrEn%s;\n",
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				if (pGv->m_addrW > 0)
					ngvIo.Append("\tsc_out<ht_uint%d> o_%sTo%s_wrAddr%s;\n",
					pGv->m_addrW, pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				ngvIo.Append("\tsc_out<%s> o_%sTo%s_wrData%s;\n",
					pGv->m_type.c_str(), pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
			}

			if (pModNgv->m_bWriteForInstrWrite) {
				ngvIo.Append("\tsc_out<bool> o_%sTo%s_iwCompRdy%s;\n",
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				if (pMod->m_threads.m_htIdW.AsInt() > 0) {
					ngvIo.Append("\tsc_out<sc_uint<%s_HTID_W> > o_%sTo%s_iwCompHtId%s;\n",
						pMod->m_modName.Upper().c_str(), pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				}
				ngvIo.Append("\tsc_out<bool> o_%sTo%s_iwCompData%s;\n",
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				ngvIo.Append("\n");
			}

			if (pModNgv->m_bWriteForMifRead) {
				if (pMod->m_mif.m_mifRd.m_rspGrpW.AsInt() > 0) {
					ngvIo.Append("\tsc_in<ht_uint%d> i_%sTo%s_mwGrpId%s;\n",
						pMod->m_mif.m_mifRd.m_rspGrpW.AsInt(), pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				}
				ngvIo.Append("\tsc_in<CGW_%s> i_%sTo%s_mwData%s;\n",
					pGv->m_pNgvInfo->m_ngvWrType.c_str(), pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				if (bNeedQue) {
					ngvIo.Append("\tsc_out<bool> o_%sTo%s_mwFull%s;\n",
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				}
				ngvIo.Append("\n");

				ngvIo.Append("\tsc_out<bool> o_%sTo%s_mwCompRdy%s;\n",
					pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				if (pMod->m_mif.m_mifRd.m_rspGrpW.AsInt() > 0) {
					ngvIo.Append("\tsc_out<ht_uint%d> o_%sTo%s_mwCompGrpId%s;\n",
						pMod->m_mif.m_mifRd.m_rspGrpW.AsInt(), pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), pGv->m_dimenDecl.c_str());
				}
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
				CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
				char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';

				ngvRegDecl.Append("\tbool c_t0_%s_%cwRrRdy%s%s;\n",
					pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());

				ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSel%s%s;\n",
					pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
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

					string rrSelReset = pNgvInfo->m_bNgvWrCompClk2x ? "c_reset2x" : "r_reset1x";

					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';

						if (bNgvBlock && (bNgvAtomic || ngvFieldCnt > 1)) {
							ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrRdy%s%s = r_%s_%cwWrEn%s%s && (!r_t1_gwWrEn%s%s || r_t1_gwData%s%s.GetAddr() != r_%s_%cwData%s%s.GetAddr());\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								ngvClkRrSel.c_str(), dimIdx.c_str(),
								ngvClkRrSel.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());
						} else {
							ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrRdy%s%s = r_%s_%cwWrEn%s%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());
						}

					}
					ngvPreRegRrSel.NewLine();

					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';

						ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrSel%s%s = c_t0_%s_%cwRrRdy%s%s && ((r_rrSel%s%s & 0x%lx) != 0 || !(\n", tabs.c_str(),
							pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
							pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
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

					string rrSelReset = pNgvInfo->m_bNgvWrCompClk2x ? "c_reset2x" : "r_reset1x";

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
					CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
					int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
					char imCh = imIdx == 0 ? 'i' : 'm';
					string idStr = imIdx == 0 ? "Ht" : "Grp";
					int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

					ngvRegDecl.Append("\tbool c_t0_%s_%cwRrRdy%s%s;\n",
						pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());

					ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSel%s%s;\n",
						pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
						VA("r_t1_%s_%cwWrEn%s", pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str()), pGv->m_dimenList);
					if (bNgvAtomic) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
							VA("r_t1_%s_%cwWrEn%s_sig", pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str()), pGv->m_dimenList);
					}

					if (idW > 0) {
						ngvRegDecl.Append("\tht_uint%d c_t0_%s_%cw%sId%s%s;\n",
							idW,
							pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", idW),
							VA("r_t1_%s_%cw%sId%s", pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkRrSel.c_str()), pGv->m_dimenList);
						if (bNgvAtomic) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<ht_uint%d>", idW),
								VA("r_t1_%s_%cw%sId%s_sig", pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkRrSel.c_str()), pGv->m_dimenList);
						}
					}

					if (imIdx == 0) {
						ngvRegDecl.Append("\tbool c_t0_%s_%cwComp%s%s;\n",
							pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
							VA("r_t1_%s_%cwComp%s", pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str()), pGv->m_dimenList);
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
							CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';

							ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrRdy%s%s = r_%s_%cwWrEn%s%s%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str());

						}
						ngvPreRegRrSel.NewLine();

						for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
							CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
							char imCh = imIdx == 0 ? 'i' : 'm';
							string idStr = imIdx == 0 ? "Ht" : "Grp";

							ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrSel%s%s = c_t0_%s_%cwRrRdy%s%s && ((r_rrSel%c%s & 0x%lx) != 0 || !(\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
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
							CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
							char imCh = imIdx == 0 ? 'i' : 'm';
							string idStr = imIdx == 0 ? "Ht" : "Grp";
							int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

							ngvRegRrSel.Append("%sr_t1_%s_%cwWrEn%s%s = c_t0_%s_%cwRrSel%s%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());

							if (bNgvAtomic) {
								ngvRegRrSel.Append("%sr_t1_%s_%cwWrEn%s_sig%s = c_t0_%s_%cwRrSel%s%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());
							}

							if (idW > 0) {
								ngvRegRrSel.Append("%sr_t1_%s_%cw%sId%s%s = c_t0_%s_%cw%sId%s%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkRrSel.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkRrSel.c_str(), dimIdx.c_str());
							}

							if (bNgvAtomic && idW > 0) {
								ngvRegRrSel.Append("%sr_t1_%s_%cw%sId%s_sig%s = c_t0_%s_%cw%sId%s%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkRrSel.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkRrSel.c_str(), dimIdx.c_str());
							}

							if (imIdx == 0) {
								ngvRegRrSel.Append("%sr_t1_%s_%cwComp%s%s = c_t0_%s_%cwComp%s%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, ngvClkRrSel.c_str(), dimIdx.c_str());
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
					CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
					int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
					char imCh = imIdx == 0 ? 'i' : 'm';
					string idStr = imIdx == 0 ? "Ht" : "Grp";
					int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

					ngvRegDecl.Append("\tbool c_t0_%s_%cwRrRdy%c%s%s;\n",
						pMod->m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());

					ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSel%c%s%s;\n",
						pMod->m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());

					if (idW > 0) {
						ngvRegDecl.Append("\tht_uint%d c_t0_%s_%cw%sId%c%s%s;\n",
							idW,
							pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
					}

					if (imIdx == 0) {
						ngvRegDecl.Append("\tbool c_t0_%s_%cwComp%c%s%s;\n",
							pMod->m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), pGv->m_dimenDecl.c_str());
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
							CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
							char imCh = imIdx == 0 ? 'i' : 'm';
							int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();
							bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? (pModNgv->m_bMaxIw && idW > 0) : pModNgv->m_bMaxMw);

							string maxWrStr = bMaxWr ? (eoIdx == 0 ? "E" : "O") : "";

							ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrRdy%c%s%s = r_%s_%cwWrEn%s%s%s && ", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, maxWrStr.c_str(), ngvClkRrSel.c_str(), dimIdx.c_str());

							if (!bMaxWr) {
								ngvPreRegRrSel.Append("(r_%s_%cwData%s.GetAddr() & 1) == %d && ", pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(), eoIdx);
							}

							ngvPreRegRrSel.Append("(!r_t1_gwWrEn%c_sig%s || r_%s_%cwData%s%s.GetAddr() != r_t1_gwData%c_sig%s.read().GetAddr());\n",
								eoCh, dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, maxWrStr.c_str(), dimIdx.c_str(),
								eoCh, dimIdx.c_str());

						}
						ngvPreRegRrSel.NewLine();

						for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
							CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';

							ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrSel%c%s%s = c_t0_%s_%cwRrRdy%c%s%s && ((r_rrSel%c%s & 0x%lx) != 0 || !(\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, eoCh, ngvClkRrSel.c_str(), dimIdx.c_str(),
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
				CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
				int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
				char imCh = imIdx == 0 ? 'i' : 'm';
				int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();
				bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? (pModNgv->m_bMaxIw/* && idW > 0*/) : pModNgv->m_bMaxMw);

				for (int eoIdx = 0; eoIdx < 2; eoIdx += 1) {
					string eoStr;
					if (bRrSelEO && bMaxWr) {
						eoStr = eoIdx == 0 ? "E" : "O";
					} else if (eoIdx == 1)
						continue;

					ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSel%s%s;\n", pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), pGv->m_dimenDecl.c_str());

					string tabs = "\t";
					CLoopInfo loopInfo(ngvPreRegRrSel, tabs, pGv->m_dimenList, 1);
					do {
						string dimIdx = loopInfo.IndexStr();

						if (bRrSelEO) {
							if (idW == 0) {
								ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrSel%s%s = r_%s_%cwWrEn%s%s && (r_%s_%cwData%s%s.read().GetAddr() & 1) == %d && (!r_t1_gwWrEn%s_sig%s || r_%s_%cwData%s%s.read().GetAddr() != r_t1_gwAddr%s_sig%s);\n",
									tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
									eoIdx,
									eoStr.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(),
									eoStr.c_str(), dimIdx.c_str());
							} else {
								ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrSel%s%s = r_%s_%cwWrEn%s%s%s && (!r_t1_gwWrEn%s_sig%s || r_%s_%cwData%s%s%s.read().GetAddr() != r_t1_gwAddr%s_sig%s);\n",
									tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
									eoStr.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), queRegDataSig.c_str(), dimIdx.c_str(),
									eoStr.c_str(), dimIdx.c_str());
							}
						} else if (bNeedAddrComp) {
							ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrSel%s = r_%s_%cwWrEn%s%s && (!r_t1_wrEn%s || r_%s_%cwData%s%s.GetAddr() != r_t1_gwData%s.GetAddr());\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
								dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(), 
								dimIdx.c_str());
						} else {
							ngvPreRegRrSel.Append("%sc_t0_%s_%cwRrSel%s = r_%s_%cwWrEn%s%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str());
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
				CModule * pMod = ngvModInfoList[modIdx].m_pMod;
				CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

				string ngvClkModIn = (pMod->m_clkRate == eClk2x || pNgvInfo->m_bNgvWrCompClk2x) ? "_2x" : "";
				string ngvClkWrComp = pNgvInfo->m_bNgvWrCompClk2x ? "_2x" : "";

				CHtCode & ngvPreRegModIn = (pMod->m_clkRate == eClk2x || pNgvInfo->m_bNgvWrCompClk2x) ? ngvPreReg_2x : ngvPreReg_1x;

				CHtCode & ngvRegSelGw = pNgvInfo->m_bNgvWrCompClk2x ? ngvReg_2x : ngvReg_1x;

				for (int imIdx = 0; imIdx < 2; imIdx += 1) {
					if (imIdx == 0 && !pModNgv->m_bWriteForInstrWrite) continue;
					if (imIdx == 1 && !pModNgv->m_bWriteForMifRead) continue;
					char imCh = imIdx == 0 ? 'i' : 'm';
					string idStr = imIdx == 0 ? "Ht" : "Grp";
					int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

					if (idW > 0) {
						ngvRegDecl.Append("\tht_uint%d c_%s_%cw%sId%s%s;\n",
							idW, pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());

						if (bNeedQueRegHtIdNonSig) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", idW),
								VA("r_%s_%cw%sId%s", pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
						}
						if (bNeedQueRegHtIdSig) {
							ngvRegDecl.Append("\tsc_signal<ht_uint%d> r_%s_%cw%sId%s_sig%s;\n",
								idW, pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkWrComp.c_str(), pGv->m_dimenDecl.c_str());
						}
					}

					if (imIdx == 0) {
						ngvRegDecl.Append("\tbool c_%s_%cwComp%s%s;\n",
							pMod->m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
							VA("r_%s_%cwComp%s", pMod->m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str()), pGv->m_dimenList);
					}

					ngvRegDecl.Append("\tCGW_%s c_%s_%cwData%s%s;\n",
						pGv->m_pNgvInfo->m_ngvWrType.c_str(),
						pMod->m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());
					if (bNeedQueRegDataSig) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<CGW_%s>", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
							VA("r_%s_%cwData%s_sig", pMod->m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str()), pGv->m_dimenList);
					} else {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
							VA("r_%s_%cwData%s", pMod->m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str()), pGv->m_dimenList);
					}
					ngvRegDecl.Append("\tbool c_%s_%cwWrEn%s%s;\n",
						pMod->m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());

					if (bNeedQueRegWrEnNonSig) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
							VA("r_%s_%cwWrEn%s", pMod->m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str()), pGv->m_dimenList);
					}
					if (bNeedQueRegWrEnSig) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
							VA("r_%s_%cwWrEn%s_sig", pMod->m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str()), pGv->m_dimenList);
					}

					ngvRegDecl.NewLine();

					{
						string tabs = "\t";
						CLoopInfo loopInfo(ngvPreRegModIn, tabs, pGv->m_dimenList, 4);
						do {
							string dimIdx = loopInfo.IndexStr();
							if (idW > 0) {
								ngvPreRegModIn.Append("%sc_%s_%cw%sId%s%s = i_%sTo%s_%cw%sId%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
							}
							if (imIdx == 0) {
								ngvPreRegModIn.Append("%sc_%s_%cwComp%s%s = i_%sTo%s_%cwComp%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());
							}
							ngvPreRegModIn.Append("%sc_%s_%cwData%s%s = i_%sTo%s_%cwData%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());

							ngvPreRegModIn.Append("%sc_%s_%cwWrEn%s%s = ", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());

							string separator;
							for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
								if (iter.IsStructOrUnion()) continue;

								ngvPreRegModIn.Append("%sc_%s_%cwData%s%s%s.GetWrEn()",
									separator.c_str(), pMod->m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
									iter.GetHeirFieldName().c_str());

								separator = VA(" ||\n%s\t", tabs.c_str());

								if (pGv->m_pType->m_eType != eRecord) continue;

								if ((iter->m_atomicMask & ATOMIC_INC) != 0 && imIdx == 0) {
									ngvPreRegModIn.Append(" || c_%s_%cwData%s%s%s.GetIncEn()",
										pMod->m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										iter.GetHeirFieldName().c_str());
								}
								if ((iter->m_atomicMask & ATOMIC_SET) != 0 && imIdx == 0) {
									ngvPreRegModIn.Append(" || c_%s_%cwData%s%s%s.GetSetEn()",
										pMod->m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										iter.GetHeirFieldName().c_str());
								}
								if ((iter->m_atomicMask & ATOMIC_ADD) != 0 && imIdx == 0) {
									ngvPreRegModIn.Append(" || c_%s_%cwData%s%s%s.GetAddEn()",
										pMod->m_modName.Lc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
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
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrCompClk.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrCompClk.c_str(), dimIdx.c_str());
								}
								if (bNeedQueRegHtIdSig) {
									ngvRegSelGw.Append("%sr_%s_%cw%sId%s_sig%s = c_%s_%cw%sId%s%s;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrCompClk.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrCompClk.c_str(), dimIdx.c_str());
								}
							}
							if (imIdx == 0) {
								ngvRegSelGw.Append("%sr_%s_%cwComp%s%s = c_%s_%cwComp%s%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str());
							}
							ngvRegSelGw.Append("%sr_%s_%cwData%s%s%s = c_%s_%cwData%s%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), queRegDataSig.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str());

							if (bNeedQueRegWrEnNonSig) {
								ngvRegSelGw.Append("%sr_%s_%cwWrEn%s%s = c_%s_%cwWrEn%s%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str());
							}

							if (bNeedQueRegWrEnSig) {
								ngvRegSelGw.Append("%sr_%s_%cwWrEn%s_sig%s = c_%s_%cwWrEn%s%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, ngvWrCompClk.c_str(), dimIdx.c_str());
							}

						} while (loopInfo.Iter());
					}
				}
			}
		} else {
			for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
				CRam * pModNgv = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pNgv;
				CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
				int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
				char imCh = imIdx == 0 ? 'i' : 'm';
				string idStr = imIdx == 0 ? "Ht" : "Grp";
				int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

				string ngvClkModIn = (pMod->m_clkRate == eClk2x || bNgvSelGwClk2x) ? "_2x" : "";
				string ngvClkWrComp = bNgvSelGwClk2x ? "_2x" : "";

				CHtCode & ngvPreRegModIn = (pMod->m_clkRate == eClk2x || bNgvSelGwClk2x) ? ngvPreReg_2x : ngvPreReg_1x;
				CHtCode & ngvPreRegWrComp = bNgvSelGwClk2x ? ngvPreReg_2x : ngvPreReg_1x;

				CHtCode & ngvRegModIn = (pMod->m_clkRate == eClk2x || bNgvSelGwClk2x) ? ngvReg_2x : ngvReg_1x;
				CHtCode & ngvRegSelGw = bNgvSelGwClk2x ? ngvReg_2x : ngvReg_1x;

				CHtCode & ngvOutModIn = (pMod->m_clkRate == eClk2x || bNgvSelGwClk2x) ? ngvOut_2x : ngvOut_1x;

				string ngvRegSelGwReset = pNgvInfo->m_bNgvWrCompClk2x ? "c_reset2x" : "r_reset1x";

				string preSig = pMod->m_clkRate == eClk2x && !bNgvSelGwClk2x && idW == 0 && imCh == 'i' ? "sc_signal<" : "";
				string postSig = pMod->m_clkRate == eClk2x && !bNgvSelGwClk2x && idW == 0 && imCh == 'i' ? ">" : "";

				if (idW > 0) {
					ngvRegDecl.Append("\tht_uint%d c_%sTo%s_%cw%sId%s%s;\n", 
						idW,
						pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("%sht_uint%d%s", preSig.c_str(), idW, postSig.c_str()),
						VA("r_%sTo%s_%cw%sId%s", pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str()), pGv->m_dimenList);
				}
				if (imIdx == 0) {
					ngvRegDecl.Append("\tbool c_%sTo%s_%cwComp%s%s;\n",
						pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("%sbool%s", preSig.c_str(), postSig.c_str()),
						VA("r_%sTo%s_%cwComp%s", pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str()), pGv->m_dimenList);
				}

				ngvRegDecl.Append("\tCGW_%s c_%sTo%s_%cwData%s%s;\n", 
					pGv->m_pNgvInfo->m_ngvWrType.c_str(),
					pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("%sCGW_%s%s", preSig.c_str(), pGv->m_pNgvInfo->m_ngvWrType.c_str(), postSig.c_str()),
					VA("r_%sTo%s_%cwData%s", pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str()), pGv->m_dimenList);

				ngvRegDecl.Append("\tbool c_%sTo%s_%cwWrEn%s%s;\n",
					pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("%sbool%s", preSig.c_str(), postSig.c_str()),
					VA("r_%sTo%s_%cwWrEn%s", pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str()), pGv->m_dimenList);

				if ((bNeedQue && imIdx == 1)) {
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
						VA("r_%sTo%s_%cwFull%s", pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), imCh, ngvClkModIn.c_str()), pGv->m_dimenList);
				}
				ngvRegDecl.NewLine();

				for (int eoIdx = 0; eoIdx < 2; eoIdx += 1) {
					string eoStr;
					bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? (pModNgv->m_bMaxIw && idW > 0) : pModNgv->m_bMaxMw);
					if (bRrSelEO && bMaxWr) {
						eoStr = eoIdx == 0 ? "E" : "O";
					} else if (eoIdx == 1)
						continue;

					bool bQueBypass = pMod->m_clkRate == eClk1x || bNgvSelGwClk2x;

					if (bMaxWr) {
						ngvRegDecl.Append("\tbool c_%sTo%s_%cwWrEn%s%s%s;\n",
							pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, eoStr.c_str(), ngvClkModIn.c_str(), pGv->m_dimenDecl.c_str());
						ngvRegDecl.NewLine();
					}
					if (idW > 0 || imCh == 'm') {
						string queueW = imIdx == 1 ? "5" : VA("%s_HTID_W", pMod->m_modName.Upper().c_str());
						if (idW > 0) {
							ngvRegDecl.Append("\tht_dist_que <ht_uint%d, %s> m_%s_%cw%sIdQue%s%s;\n",
								idW, queueW.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), pGv->m_dimenDecl.c_str());
						}
						if (imIdx == 0) {
							ngvRegDecl.Append("\tht_dist_que <bool, %s> m_%s_%cwCompQue%s%s;\n",
								queueW.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), pGv->m_dimenDecl.c_str());
						}
						ngvRegDecl.Append("\tht_dist_que <CGW_%s, %s> m_%s_%cwDataQue%s%s;\n",
							pGv->m_pNgvInfo->m_ngvWrType.c_str(), queueW.c_str(),
							pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), pGv->m_dimenDecl.c_str());
						ngvRegDecl.NewLine();
					}

					if (idW > 0) {
						ngvRegDecl.Append("\tht_uint%d c_%s_%cw%sId%s%s%s;\n",
							idW, pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str(), pGv->m_dimenDecl.c_str());

						if (bNeedQueRegHtIdSig) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<ht_uint%d>", idW),
								VA("r_%s_%cw%sId%s%s_sig", pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
						} else {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", idW),
								VA("r_%s_%cw%sId%s%s", pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
						}
					}

					if (imIdx == 0) {
						ngvRegDecl.Append("\tbool c_%s_%cwComp%s%s%s;\n",
							pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), pGv->m_dimenDecl.c_str());
						if (bNeedQueRegCompSig) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
								VA("r_%s_%cwComp%s%s_sig", pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
						} else {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
								VA("r_%s_%cwComp%s%s", pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
						}
					}
					ngvRegDecl.Append("\tCGW_%s c_%s_%cwData%s%s%s;\n",
						pGv->m_pNgvInfo->m_ngvWrType.c_str(),
						pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), pGv->m_dimenDecl.c_str());
					if (bNeedQueRegDataSig) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<CGW_%s>", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
							VA("r_%s_%cwData%s%s_sig", pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
					} else {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("CGW_%s", pGv->m_pNgvInfo->m_ngvWrType.c_str()),
							VA("r_%s_%cwData%s%s", pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
					}
					ngvRegDecl.Append("\tbool c_%s_%cwWrEn%s%s%s;\n",
						pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), pGv->m_dimenDecl.c_str());
					if (bNeedQueRegWrEnNonSig) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
							VA("r_%s_%cwWrEn%s%s", pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
					}
					if (bNeedQueRegWrEnSig) {
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
							VA("r_%s_%cwWrEn%s%s_sig", pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str()), pGv->m_dimenList);
					}
					ngvRegDecl.NewLine();

					if (eoIdx == 0) {
						string tabs = "\t";
						CLoopInfo loopInfo(ngvPreRegModIn, tabs, pGv->m_dimenList, 5);
						do {
							string dimIdx = loopInfo.IndexStr();

							ngvPreRegModIn.Append("%sc_%sTo%s_%cwWrEn%s%s = ", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());

							if (pMod->m_clkRate == eClk1x && bNgvSelGwClk2x)
								ngvPreRegModIn.Append("r_phase && (");

							string separator;
							for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
								if (iter.IsStructOrUnion()) continue;

								ngvPreRegModIn.Append("%si_%sTo%s_%cwData%s.read()%s.GetWrEn()",
									separator.c_str(), pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str(),
									iter.GetHeirFieldName().c_str());

								if ((iter.GetAtomicMask() & ATOMIC_INC) != 0 && imCh == 'i') {
									ngvPreRegModIn.Append(" || i_%sTo%s_%cwData%s.read()%s.GetIncEn()",
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str(),
										iter.GetHeirFieldName().c_str());
								}
								if ((iter.GetAtomicMask() & ATOMIC_SET) != 0 && imCh == 'i') {
									ngvPreRegModIn.Append(" || i_%sTo%s_%cwData%s.read()%s.GetSetEn()",
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str(),
										iter.GetHeirFieldName().c_str());
								}
								if ((iter.GetAtomicMask() & ATOMIC_ADD) != 0 && imCh == 'i') {
									ngvPreRegModIn.Append(" || i_%sTo%s_%cwData%s.read()%s.GetAddEn()",
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str(),
										iter.GetHeirFieldName().c_str());
								}
								separator = VA(" ||\n%s\t", tabs.c_str());
							}

							if (pMod->m_clkRate == eClk1x && bNgvSelGwClk2x)
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
									pMod->m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str());
							}
							else {
								portRrSel = VA("c_t0_%s_%cwRrSel%s%s%s",
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());
							}

							{
								if (pMod->m_clkRate == eClk2x && !bNgvSelGwClk2x && idW == 0 && imCh == 'i') {
									ngvPreRegModIn.Append("%sc_%sTo%s_%cwWrEn%s%s = (r_phase && r_%sTo%s_%cwWrEn%s%s) ?\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
									ngvPreRegModIn.Append("%s\tr_%sTo%s_%cwWrEn%s%s.read() : c_%sTo%s_%cwWrEn%s%s;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());

									if (idW > 0) {
										ngvPreRegModIn.Append("%sc_%sTo%s_%cw%sId%s%s = (r_phase && r_%sTo%s_%cwWrEn%s%s) ?\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
										ngvPreRegModIn.Append("%s\tr_%sTo%s_%cw%sId%s%s.read() : i_%sTo%s_%cw%sId%s.read()) ?\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
									}
									if (imIdx == 0) {
										ngvPreRegModIn.Append("%sc_%sTo%s_%cwComp%s%s = (r_phase && r_%sTo%s_%cwWrEn%s%s) ?\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
										ngvPreRegModIn.Append("%s\tr_%sTo%s_%cwComp%s%s.read() : i_%sTo%s_%cwComp%s.read();\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());
									}
									ngvPreRegModIn.Append("%sc_%sTo%s_%cwData%s%s = (r_phase && r_%sTo%s_%cwWrEn%s%s) ?\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
									ngvPreRegModIn.Append("%s\tr_%sTo%s_%cwData%s%s.read() : i_%sTo%s_%cwData%s.read();\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());
								}
								else {
									if (idW > 0) {
										ngvPreRegModIn.Append("%sc_%sTo%s_%cw%sId%s%s = i_%sTo%s_%cw%sId%s;\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
									}
									if (imIdx == 0) {
										ngvPreRegModIn.Append("%sc_%sTo%s_%cwComp%s%s = i_%sTo%s_%cwComp%s;\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());
									}
									ngvPreRegModIn.Append("%sc_%sTo%s_%cwData%s%s = i_%sTo%s_%cwData%s;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, dimIdx.c_str());
								}
								ngvPreRegModIn.NewLine();
							}

							if (idW > 0 || imCh == 'm') {
								char rcCh = 'r';
								if (bMaxWr) {
									rcCh = 'c';
									ngvPreRegModIn.Append("%sc_%sTo%s_%cwWrEn%s%s%s = r_%sTo%s_%cwWrEn%s%s && (r_%sTo%s_%cwData%s%s.GetAddr() & 1) == %d;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, eoStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
										eoIdx);
									ngvPreRegModIn.NewLine();
								}

								if (bQueBypass) {
									ngvPreRegModIn.Append("%sif (%c_%sTo%s_%cwWrEn%s%s%s && (r_%s_%cwWrEn%s%s%s%s && !%s || !m_%s_%cwDataQue%s%s.empty())) {\n", tabs.c_str(),
										rcCh, pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, eoStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
										portRrSel.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());
								}
								else {
									ngvPreRegModIn.Append("%sif (%c_%sTo%s_%cwWrEn%s%s%s) {\n", tabs.c_str(),
										rcCh, pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, eoStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str());
								}
								if (idW > 0) {
									ngvPreRegModIn.Append("%s\tm_%s_%cw%sIdQue%s%s.push(r_%sTo%s_%cw%sId%s%s);\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str());
								}
								if (imIdx == 0) {
									ngvPreRegModIn.Append("%s\tm_%s_%cwCompQue%s%s.push(r_%sTo%s_%cwComp%s%s);\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
								}
								ngvPreRegModIn.Append("%s\tm_%s_%cwDataQue%s%s.push(r_%sTo%s_%cwData%s%s);\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());

								ngvPreRegModIn.Append("%s}\n", tabs.c_str());
							}
						} while (loopInfo.Iter());

						if (bNeedQue && imIdx == 1) {
							string tabs = "\t";
							CLoopInfo loopInfo(ngvRegModIn, tabs, pGv->m_dimenList, 1);
							do {
								string dimIdx = loopInfo.IndexStr();

								ngvRegModIn.Append("%sr_%sTo%s_%cwFull%s%s = m_%s_%cwDataQue%s%s.size() > 24;\n", tabs.c_str(),
									pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

							} while (loopInfo.Iter());
							ngvRegModIn.NewLine();
						}

						if (bNeedQue && imIdx == 1) {
							string tabs = "\t";
							CLoopInfo loopInfo(ngvOutModIn, tabs, pGv->m_dimenList, 1);
							do {
								string dimIdx = loopInfo.IndexStr();

								ngvOutModIn.Append("%so_%sTo%s_%cwFull%s = r_%sTo%s_%cwFull%s%s;\n", tabs.c_str(),
									pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), imCh, dimIdx.c_str(),
									pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
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
									pMod->m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str());
							} else {
								portRrSel = VA("c_t0_%s_%cwRrSel%s%s%s",
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());
							}

							ngvPreRegWrComp.Append("%sif (r_%s_%cwWrEn%s%s%s%s && !%s) {\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
								portRrSel.c_str());

							if (idW > 0) {
								ngvPreRegWrComp.Append("%s\tc_%s_%cw%sId%s%s%s = r_%s_%cw%sId%s%s%s%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
							}
							if (imIdx == 0) {
								ngvPreRegWrComp.Append("%s\tc_%s_%cwComp%s%s%s = r_%s_%cwComp%s%s%s%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegCompSig.c_str(), dimIdx.c_str());
							}
							ngvPreRegWrComp.Append("%s\tc_%s_%cwData%s%s%s = r_%s_%cwData%s%s%s%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegDataSig.c_str(), dimIdx.c_str());

							ngvPreRegWrComp.Append("%s} else {\n", tabs.c_str());

							if (idW > 0 || imCh == 'm') {
								if (bQueBypass) {
									if (idW > 0) {
										ngvPreRegWrComp.Append("%s\tc_%s_%cw%sId%s%s%s = m_%s_%cw%sIdQue%s%s.empty() ? r_%sTo%s_%cw%sId%s%s : m_%s_%cw%sIdQue%s%s.front();\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), dimIdx.c_str());
									}
									if (imIdx == 0) {
										ngvPreRegWrComp.Append("%s\tc_%s_%cwComp%s%s%s = m_%s_%cwCompQue%s%s.empty() ? r_%sTo%s_%cwComp%s%s : m_%s_%cwCompQue%s%s.front();\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());
									}
									ngvPreRegWrComp.Append("%s\tc_%s_%cwData%s%s%s = m_%s_%cwDataQue%s%s.empty() ? r_%sTo%s_%cwData%s%s : m_%s_%cwDataQue%s%s.front();\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkWrComp.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());
								} else {
									if (idW > 0) {
										ngvPreRegWrComp.Append("%s\tc_%s_%cw%sId%s%s%s = m_%s_%cw%sIdQue%s%s.front();\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), dimIdx.c_str());
									}
									if (imIdx == 0) {
										ngvPreRegWrComp.Append("%s\tc_%s_%cwComp%s%s%s = m_%s_%cwCompQue%s%s.front();\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());
									}
									ngvPreRegWrComp.Append("%s\tc_%s_%cwData%s%s%s = m_%s_%cwDataQue%s%s.front();\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());
								}
							} else {
								if (imIdx == 0) {
									ngvPreRegWrComp.Append("%s\tc_%s_%cwComp%s%s%s = r_%sTo%s_%cwComp%s%s;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
								}
								ngvPreRegWrComp.Append("%s\tc_%s_%cwData%s%s%s = r_%sTo%s_%cwData%s%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
							}
							ngvPreRegWrComp.Append("%s}\n", tabs.c_str());

							if (idW > 0 || imCh == 'm') {
								if (bQueBypass) {
									char rcCh = eoStr.size() > 0 ? 'c' : 'r';
									ngvPreRegWrComp.Append("%sc_%s_%cwWrEn%s%s%s = !m_%s_%cwDataQue%s%s.empty() || %c_%sTo%s_%cwWrEn%s%s%s || r_%s_%cwWrEn%s%s%s%s && !%s;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
										rcCh, pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
										portRrSel.c_str());
								} else {
									ngvPreRegWrComp.Append("%sc_%s_%cwWrEn%s%s%s = !m_%s_%cwDataQue%s%s.empty() || r_%s_%cwWrEn%s%s%s%s && !%s;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
										portRrSel.c_str());
								}
							} else {
								ngvPreRegWrComp.Append("%sc_%s_%cwWrEn%s%s%s = r_%sTo%s_%cwWrEn%s%s%s || r_%s_%cwWrEn%s%s%s%s && !%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, eoStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
									portRrSel.c_str());
							}

							if (idW > 0 || imCh == 'm') {
								ngvPreRegWrComp.NewLine();
								ngvPreRegWrComp.Append("%sif ((!r_%s_%cwWrEn%s%s%s%s || %s) && !m_%s_%cwDataQue%s%s.empty()) {\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
									portRrSel.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

								if (idW > 0) {
									ngvPreRegWrComp.Append("%s\tm_%s_%cw%sIdQue%s%s.pop();\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), dimIdx.c_str());
								}
								if (imIdx == 0) {
									ngvPreRegWrComp.Append("%s\tm_%s_%cwCompQue%s%s.pop();\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());
								}
								ngvPreRegWrComp.Append("%s\tm_%s_%cwDataQue%s%s.pop();\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str());

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
								pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
							if (idW > 0) {
								ngvRegModIn.Append("%sr_%sTo%s_%cw%sId%s%s = c_%sTo%s_%cw%sId%s%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, idStr.c_str(), ngvClkModIn.c_str(), dimIdx.c_str());
							}
							if (imIdx == 0) {
								ngvRegModIn.Append("%sr_%sTo%s_%cwComp%s%s = c_%sTo%s_%cwComp%s%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());
							}
							ngvRegModIn.Append("%sr_%sTo%s_%cwData%s%s = c_%sTo%s_%cwData%s%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), pGv->m_gblName.Uc().c_str(), imCh, ngvClkModIn.c_str(), dimIdx.c_str());

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
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), dimIdx.c_str(),
										ngvRegSelGwReset.c_str());
								}
								if (imIdx == 0) {
									ngvRegSelGw.Append("%sm_%s_%cwCompQue%s%s.clock(%s);\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
										ngvRegSelGwReset.c_str());
								}
								ngvRegSelGw.Append("%sm_%s_%cwDataQue%s%s.clock(%s);\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
									ngvRegSelGwReset.c_str());
							} else {
								if (idW > 0) {
									ngvRegSelGw.Append("%sm_%s_%cw%sIdQue%s%s.pop_clock(%s);\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), dimIdx.c_str(),
										ngvRegSelGwReset.c_str());
								}
								if (imIdx == 0) {
									ngvRegSelGw.Append("%sm_%s_%cwCompQue%s%s.pop_clock(%s);\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
										ngvRegSelGwReset.c_str());
								}

								ngvRegSelGw.Append("%sm_%s_%cwDataQue%s%s.pop_clock(%s);\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
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
								ngvReg_2x.Append("%sm_%s_%cw%sIdQue%s%s.push_clock(%s);\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), dimIdx.c_str(),
									ngvRegSelGwReset.c_str());
							}
							if (imIdx == 0) {
								ngvReg_2x.Append("%sm_%s_%cwCompQue%s%s.push_clock(%s);\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
									ngvRegSelGwReset.c_str());
							}
							ngvReg_2x.Append("%sm_%s_%cwDataQue%s%s.push_clock(%s);\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), dimIdx.c_str(),
								ngvRegSelGwReset.c_str());

						} while (loopInfo.Iter());
					}

					{
						string tabs = "\t";
						CLoopInfo loopInfo(ngvRegSelGw, tabs, pGv->m_dimenList, 3);
						do {
							string dimIdx = loopInfo.IndexStr();

							if (idW > 0) {
								ngvRegSelGw.Append("%sr_%s_%cw%sId%s%s%s%s = c_%s_%cw%sId%s%s%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str(), queRegHtIdSig.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());
							}
							if (imIdx == 0) {
								ngvRegSelGw.Append("%sr_%s_%cwComp%s%s%s%s = c_%s_%cwComp%s%s%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegCompSig.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());
							}
							ngvRegSelGw.Append("%sr_%s_%cwData%s%s%s%s = c_%s_%cwData%s%s%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegDataSig.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());

							string wrCompReset = bNgvSelGwClk2x ? "c_reset2x" : "r_reset1x";

							ngvRegSelGw.Append("%sr_%s_%cwWrEn%s%s%s%s = !%s && c_%s_%cwWrEn%s%s%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
								wrCompReset.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), ngvClkWrComp.c_str(), dimIdx.c_str());

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
				CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
				int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
				char imCh = imIdx == 0 ? 'i' : 'm';
				string idStr = imIdx == 0 ? "Ht" : "Grp";
				int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

				ngvRegDecl.Append("\tbool c_t0_%s_%cwWrEn%s;\n",
					pMod->m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
					VA("r_t1_%s_%cwWrEn", pMod->m_modName.Lc().c_str(), imCh), pGv->m_dimenList);

				if (idW > 0) {
					ngvRegDecl.Append("\tht_uint%d c_t0_%s_%cw%sId%s;\n",
						idW,
						pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", idW),
						VA("r_t1_%s_%cw%sId", pMod->m_modName.Lc().c_str(), imCh, idStr.c_str()), pGv->m_dimenList);
				}

				if (imIdx == 0) {
					ngvRegDecl.Append("\tbool c_t0_%s_%cwComp%s;\n",
						pMod->m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
						VA("r_t1_%s_%cwComp", pMod->m_modName.Lc().c_str(), imCh), pGv->m_dimenList);
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
					ngvPreRegRrSel.Append("%sc_t0_gwData%s.InitZero();\n", tabs.c_str(), dimIdx.c_str());
					ngvPreRegRrSel.NewLine();

					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

						ngvPreRegRrSel.Append("%sc_t0_%s_%cwWrEn%s = c_t0_%s_%cwRrSel%s;\n", tabs.c_str(),
							pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
							pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						if (idW > 0) {
							ngvPreRegRrSel.Append("%sc_t0_%s_%cw%sId%s = r_%s_%cw%sId%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
						}
						if (imIdx == 0) {
							ngvPreRegRrSel.Append("%sc_t0_%s_%cwComp%s = r_%s_%cwComp%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());
						}
						ngvPreRegRrSel.NewLine();
					}

					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';

						ngvPreRegRrSel.Append("%sif (c_t0_%s_%cwRrSel%s) {\n", tabs.c_str(),
							pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						ngvPreRegRrSel.Append("%s\tc_rrSel%s = 0x%x;\n", tabs.c_str(),
							dimIdx.c_str(),
							1 << ((ngvIdx + 1) % ngvPortCnt));

						ngvPreRegRrSel.Append("%s\tc_t0_gwWrEn%s = true;\n", tabs.c_str(), dimIdx.c_str());

						ngvPreRegRrSel.Append("%s\tc_t0_gwData%s = r_%s_%cwData%s;\n", tabs.c_str(),
							dimIdx.c_str(),
							pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());

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
						CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

						ngvRegRrSel.Append("%sr_t1_%s_%cwWrEn%s = c_t0_%s_%cwWrEn%s;\n", tabs.c_str(),
							pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
							pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						if (idW > 0) {
							ngvRegRrSel.Append("%sr_t1_%s_%cw%sId%s = c_t0_%s_%cw%sId%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
						}
						if (imIdx == 0) {
							ngvRegRrSel.Append("%sr_t1_%s_%cwComp%s = c_t0_%s_%cwComp%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());
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
				CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
				int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
				char imCh = imIdx == 0 ? 'i' : 'm';
				string idStr = imIdx == 0 ? "Ht" : "Grp";
				int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

				GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
					VA("r_t1_%s_%cwWrEn%s", pMod->m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str()), pGv->m_dimenList);

				if (idW > 0) {
					ngvRegDecl.Append("\tht_uint%d c_t0_%s_%cw%sId_2x%s;\n",
						idW,
						pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", idW),
						VA("r_t1_%s_%cw%sId%s", pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrDataClk.c_str()), pGv->m_dimenList);
				}

				if (imIdx == 0) {
					ngvRegDecl.Append("\tbool c_t0_%s_%cwComp_2x%s;\n",
						pMod->m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
						VA("r_t1_%s_%cwComp%s", pMod->m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str()), pGv->m_dimenList);
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

					ngvPreReg_2x.Append("%sc_t%d_gwData_2x%s.InitZero();\n", tabs.c_str(), stgIdx, dimIdx.c_str());
					ngvPreReg_2x.NewLine();

					for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
						CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

						if (idW > 0) {
							ngvPreReg_2x.Append("%sc_t0_%s_%cw%sId_2x%s = r_%s_%cw%sId_2x%s%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
						}

						if (imIdx == 0) {
							ngvPreReg_2x.Append("%sc_t0_%s_%cwComp_2x%s = r_%s_%cwComp_2x%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());
						}
						ngvPreReg_2x.NewLine();
					}

					for (size_t ngvIdx = 0; ngvIdx < pNgvInfo->m_wrPortList.size(); ngvIdx += 1) {
						CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';

						ngvPreReg_2x.Append("%sif (c_t0_%s_%cwRrSel_2x%s) {\n", tabs.c_str(),
							pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());
						ngvPreReg_2x.Append("%s\tc_rrSel_2x%s = 0x%x;\n", tabs.c_str(),
							dimIdx.c_str(), 1 << (int)((ngvIdx + 1) % pNgvInfo->m_wrPortList.size()));
						ngvPreReg_2x.Append("%s\tc_t%d_gwWrEn_2x%s = true;\n", tabs.c_str(), stgIdx, dimIdx.c_str());

						ngvPreReg_2x.Append("%s\tc_t%d_gwData_2x%s = r_%s_%cwData_2x%s;\n", tabs.c_str(),
							stgIdx, dimIdx.c_str(),
							pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());

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
						CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

						ngvReg_2x.Append("%sr_t1_%s_%cwWrEn_2x%s = c_t0_%s_%cwRrSel_2x%s;\n", tabs.c_str(),
							pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
							pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());

						if (idW > 0) {
							ngvReg_2x.Append("%sr_t1_%s_%cw%sId_2x%s = c_t0_%s_%cw%sId_2x%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
						}

						if (imIdx == 0) {
							ngvReg_2x.Append("%sr_t1_%s_%cwComp_2x%s = c_t0_%s_%cwComp_2x%s;\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());
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
						ngvPreRegRrSel.Append("%sc_t0_gwData%c%s.InitZero();\n", tabs.c_str(), rngCh, dimIdx.c_str());
						ngvPreRegRrSel.NewLine();

						for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
							CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
							char imCh = imIdx == 0 ? 'i' : 'm';
							string idStr = imIdx == 0 ? "Ht" : "Grp";
							int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

							if (idW > 0) {
								ngvPreRegRrSel.Append("%sc_t0_%s_%cw%sId%s = r_%s_%cw%sId%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
							}
							if (imIdx == 0) {
								ngvPreRegRrSel.Append("%sc_t0_%s_%cwComp%s = r_%s_%cwComp%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());
							}
							ngvPreRegRrSel.NewLine();
						}

						for (int ngvIdx = ngvPortLo; ngvIdx < ngvPortHi; ngvIdx += 1) {
							CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';

							ngvPreRegRrSel.Append("%sif (c_t0_%s_%cwRrSel%s) {\n", tabs.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());

							ngvPreRegRrSel.Append("%s\tc_rrSel%c%s = 0x%x;\n", tabs.c_str(),
								rngCh, dimIdx.c_str(),
								1 << ((ngvIdx + 1 - ngvPortLo) % ngvPortRngCnt));

							ngvPreRegRrSel.Append("%s\tc_t0_gwWrEn%c%s = true;\n", tabs.c_str(), rngCh, dimIdx.c_str());
							ngvPreRegRrSel.Append("%s\tc_t0_gwData%c%s = r_%s_%cwData%s;\n", tabs.c_str(),
								rngCh, dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());

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
					CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
					int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
					char imCh = imIdx == 0 ? 'i' : 'm';
					int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

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
									pMod->m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str());

								if (pGv->m_addrW > 0) {
									ngvPreReg_1x.Append("%sc_t0_gwAddr%s%s = r_%s_%cwData%s%s.read().GetAddr();\n", tabs.c_str(),
										eoStr.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());
								}
							} else {
								ngvPreReg_1x.Append("%sc_t0_gwWrEn%s%s = r_%s_%cwWrEn%s%s%s;\n", tabs.c_str(),
									eoStr.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), queRegWrEnSig.c_str(), dimIdx.c_str());

								if (pGv->m_addrW > 0) {
									ngvPreReg_1x.Append("%sc_t0_gwAddr%s%s = r_%s_%cwData%s%s%s.read().GetAddr();\n", tabs.c_str(),
										eoStr.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoStr.c_str(), queRegDataSig.c_str(), dimIdx.c_str());
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
						CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						char imCh = pNgvInfo->m_wrPortList[ngvIdx].second == 0 ? 'i' : 'm';
						int idW = imCh == 'i' ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

						ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSelE_2x%s;\n",
							pMod->m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());

						ngvRegDecl.Append("\tbool c_t0_%s_%cwRrSelO_2x%s;\n",
							pMod->m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());

						string tabs = "\t";
						CLoopInfo loopInfo(ngvPreReg_2x, tabs, pGv->m_dimenList, 10);
						do {
							string dimIdx = loopInfo.IndexStr();

							if (idW == 0) {
								ngvPreReg_2x.Append("%sc_t0_%s_%cwRrSelE_2x%s = r_%s_%cwWrEn%s%s && (r_%s_%cwData%s%s.read().GetAddr() & 1) == 0 && (!r_t1_gwWrEnE_sig%s || r_%s_%cwData%s%s.read().GetAddr() != r_t1_gwAddrE_sig%s);\n", 
									tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
									dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(),
									dimIdx.c_str());

								ngvPreReg_2x.Append("%sc_t0_%s_%cwRrSelO_2x%s = r_%s_%cwWrEn%s%s && (r_%s_%cwData%s%s.read().GetAddr() & 1) == 1 && (!r_t1_gwWrEnO_sig%s || r_%s_%cwData%s%s.read().GetAddr() != r_t1_gwAddrO_sig%s);\n", 
									tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
									dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(),
									dimIdx.c_str());
							} else {
								ngvPreReg_2x.Append("%sc_t0_%s_%cwRrSelE_2x%s = r_%s_%cwWrEnE%s%s && (!r_t1_gwWrEnE_sig%s || r_%s_%cwDataE%s%s.read().GetAddr() != r_t1_gwAddrE_sig%s);\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
									dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(),
									dimIdx.c_str());

								ngvPreReg_2x.Append("%sc_t0_%s_%cwRrSelO_2x%s = r_%s_%cwWrEnO%s%s && (!r_t1_gwWrEnO_sig%s || r_%s_%cwDataO%s%s.read().GetAddr() != r_t1_gwAddrO_sig%s);\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str(),
									dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(),
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
						CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

						ngvRegDecl.Append("\tbool c_t0_%s_%cwWrEn_2x%s;\n",
							pMod->m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());

						if (idW > 0) {
							ngvRegDecl.Append("\tht_uint%d c_t0_%s_%cw%sId_2x%s;\n",
								idW,
								pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), pGv->m_dimenDecl.c_str());
						}

						if (imIdx == 0) {
							ngvRegDecl.Append("\tbool c_t0_%s_%cwComp_2x%s;\n",
								pMod->m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
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
								CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
								int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
								char imCh = imIdx == 0 ? 'i' : 'm';
								string idStr = imIdx == 0 ? "Ht" : "Grp";
								int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

								ngvPreReg_2x.Append("%s\tc_t0_gwWrEn_2x%s = c_t0_%s_%cwRrSelE_2x%s;\n", tabs.c_str(),
									dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());
								if (idW == 0) {
									ngvPreReg_2x.Append("%s\tc_t0_gwData_2x%s = r_%s_%cwData%s%s;\n", tabs.c_str(),
										dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());
								} else {
									ngvPreReg_2x.Append("%s\tc_t0_gwData_2x%s = r_%s_%cwDataE%s%s;\n", tabs.c_str(),
										dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());
								}
								ngvPreReg_2x.NewLine();

								ngvPreReg_2x.Append("%s\tc_t0_%s_%cwWrEn_2x%s = c_t0_%s_%cwRrSelE_2x%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());

								if (idW > 0) {
									ngvPreReg_2x.Append("%s\tc_t0_%s_%cw%sId_2x%s = r_%s_%cw%sIdE%s%s;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
								}
								if (imIdx == 0) {
									if (idW == 0) {
										ngvPreReg_2x.Append("%s\tc_t0_%s_%cwComp_2x%s = r_%s_%cwComp%s%s;\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, queRegCompSig.c_str(), dimIdx.c_str());
									} else {
										ngvPreReg_2x.Append("%s\tc_t0_%s_%cwComp_2x%s = r_%s_%cwCompE%s%s;\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, queRegCompSig.c_str(), dimIdx.c_str());
									}
								}
							}

							ngvPreReg_2x.Append("%s} else {\n", tabs.c_str());

							for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
								CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
								int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
								char imCh = imIdx == 0 ? 'i' : 'm';
								string idStr = imIdx == 0 ? "Ht" : "Grp";
								int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

								ngvPreReg_2x.Append("%s\tc_t0_gwWrEn_2x%s = c_t0_%s_%cwRrSelO_2x%s;\n", tabs.c_str(),
									dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());
								if (idW == 0) {
									ngvPreReg_2x.Append("%s\tc_t0_gwData_2x%s = r_%s_%cwData%s%s;\n", tabs.c_str(),
										dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());
								} else {
									ngvPreReg_2x.Append("%s\tc_t0_gwData_2x%s = r_%s_%cwDataO%s%s;\n", tabs.c_str(),
										dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());
								}
								ngvPreReg_2x.NewLine();

								ngvPreReg_2x.Append("%s\tc_t0_%s_%cwWrEn_2x%s = c_t0_%s_%cwRrSelO_2x%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());

								if (idW > 0) {
									ngvPreReg_2x.Append("%s\tc_t0_%s_%cw%sId_2x%s = r_%s_%cw%sIdO%s%s;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
								}
								if (imIdx == 0) {
									if (idW == 0) {
										ngvPreReg_2x.Append("%s\tc_t0_%s_%cwComp_2x%s = r_%s_%cwComp%s%s;\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, queRegCompSig.c_str(), dimIdx.c_str());
									} else {
										ngvPreReg_2x.Append("%s\tc_t0_%s_%cwComp_2x%s = r_%s_%cwCompO%s%s;\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, queRegCompSig.c_str(), dimIdx.c_str());
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
						CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
							VA("r_t1_%s_%cwWrEn_2x", pMod->m_modName.Lc().c_str(), imCh), pGv->m_dimenList);

						if (idW > 0) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", idW),
								VA("r_t1_%s_%cw%sId_2x", pMod->m_modName.Lc().c_str(), imCh, idStr.c_str()), pGv->m_dimenList);
						}

						if (imIdx == 0) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
								VA("r_t1_%s_%cwComp_2x", pMod->m_modName.Lc().c_str(), imCh), pGv->m_dimenList);
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
								CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
								int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
								char imCh = imIdx == 0 ? 'i' : 'm';
								string idStr = imIdx == 0 ? "Ht" : "Grp";
								int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

								ngvReg_2x.Append("%sr_t1_%s_%cwWrEn_2x%s = c_t0_%s_%cwWrEn_2x%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());

								if (idW > 0) {
									ngvReg_2x.Append("%sr_t1_%s_%cw%sId_2x%s = c_t0_%s_%cw%sId_2x%s;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
								}
								if (imIdx == 0) {
									ngvReg_2x.Append("%sr_t1_%s_%cwComp_2x%s = c_t0_%s_%cwComp_2x%s;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());
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
							ngvPreReg_1x.Append("%sc_t0_gwData%c%s.InitZero();\n", tabs.c_str(), eoCh, dimIdx.c_str());
							ngvPreReg_1x.NewLine();

							for (int ngvIdx = 0; ngvIdx < ngvPortCnt; ngvIdx += 1) {
								CRam * pModNgv = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pNgv;
								CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
								int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
								char imCh = imIdx == 0 ? 'i' : 'm';
								string idStr = imIdx == 0 ? "Ht" : "Grp";
								int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();
								bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? (pModNgv->m_bMaxIw && idW > 0) : pModNgv->m_bMaxMw);
								string maxWrStr = bMaxWr ? (eoIdx == 0 ? "E" : "O") : "";

								if (idW > 0) {
									ngvPreReg_1x.Append("%sc_t0_%s_%cw%sId%c%s = r_%s_%cw%sId%s%s;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoCh, dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), maxWrStr.c_str(), dimIdx.c_str());
								}

								if (imIdx == 0) {
									ngvPreReg_1x.Append("%sc_t0_%s_%cwComp%c%s = r_%s_%cwComp%s%s;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, maxWrStr.c_str(), dimIdx.c_str());
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
								CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
								int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
								char imCh = imIdx == 0 ? 'i' : 'm';
								int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();
								bool bMaxWr = pNgvInfo->m_bNgvMaxSel && (imCh == 'i' ? (pModNgv->m_bMaxIw && idW > 0) : pModNgv->m_bMaxMw);
								string maxWrStr = bMaxWr ? (eoIdx == 0 ? "E" : "O") : "";

								ngvPreReg_1x.Append("%sif (c_t0_%s_%cwRrSel%c%s) {\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());

								ngvPreReg_1x.Append("%s\tc_rrSel%c%s = 0x%x;\n", tabs.c_str(),
									eoCh, dimIdx.c_str(),
									1 << ((ngvIdx + 1) % ngvPortCnt));

								ngvPreReg_1x.Append("%s\tc_t0_gwWrEn%c%s = true;\n", tabs.c_str(),
									eoCh, dimIdx.c_str());

								ngvPreReg_1x.Append("%s\tc_t0_gwData%c%s = r_%s_%cwData%s%s;\n", tabs.c_str(),
									eoCh, dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, maxWrStr.c_str(), dimIdx.c_str());

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
						CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
							VA("r_t1_%s_%cwRrSel%c_sig", pMod->m_modName.Lc().c_str(), imCh, eoCh), pGv->m_dimenList);

						if (idW > 0) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_signal<ht_uint%d>", idW),
								VA("r_t1_%s_%cw%sId%c_sig", pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoCh), pGv->m_dimenList);
						}

						if (imIdx == 0) {
							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "sc_signal<bool>",
								VA("r_t1_%s_%cwComp%c_sig", pMod->m_modName.Lc().c_str(), imCh, eoCh), pGv->m_dimenList);
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
								CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
								int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
								char imCh = imIdx == 0 ? 'i' : 'm';
								string idStr = imIdx == 0 ? "Ht" : "Grp";
								int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

								ngvReg_1x.Append("%sr_t1_%s_%cwRrSel%c_sig%s = c_t0_%s_%cwRrSel%c%s;\n", tabs.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());

								if (idW > 0) {
									ngvReg_1x.Append("%sr_t1_%s_%cw%sId%c_sig%s = c_t0_%s_%cw%sId%c%s;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoCh, dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoCh, dimIdx.c_str());
								}

								if (imIdx == 0) {
									ngvReg_1x.Append("%sr_t1_%s_%cwComp%c_sig%s = c_t0_%s_%cwComp%c%s;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());
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
							CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
							char imCh = imIdx == 0 ? 'i' : 'm';
							string idStr = imIdx == 0 ? "Ht" : "Grp";
							int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

							ngvRegDecl.Append("\tbool c_t1_%s_%cwWrEn_2x%s;\n",
								pMod->m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());

							if (idW > 0) {
								ngvRegDecl.Append("\tht_uint%d c_t1_%s_%cw%sId_2x%s;\n",
									idW,
									pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), pGv->m_dimenDecl.c_str());
							}

							if (imIdx == 0) {
								ngvRegDecl.Append("\tbool c_t1_%s_%cwComp_2x%s;\n",
									pMod->m_modName.Lc().c_str(), imCh, pGv->m_dimenDecl.c_str());
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
									CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
									int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
									char imCh = imIdx == 0 ? 'i' : 'm';
									string idStr = imIdx == 0 ? "Ht" : "Grp";
									int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

									ngvPreReg_2x.Append("%s\tc_t1_%s_%cwWrEn_2x%s = r_t1_%s_%cwRrSel%c_sig%s;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());

									if (idW > 0) {
										ngvPreReg_2x.Append("%s\tc_t1_%s_%cw%sId_2x%s = r_t1_%s_%cw%sId%c_sig%s;\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), eoCh, dimIdx.c_str());
									}
									if (imIdx == 0) {
										ngvPreReg_2x.Append("%s\tc_t1_%s_%cwComp_2x%s = r_t1_%s_%cwComp%c_sig%s;\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, eoCh, dimIdx.c_str());
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
							CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
							int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
							char imCh = imIdx == 0 ? 'i' : 'm';
							string idStr = imIdx == 0 ? "Ht" : "Grp";
							int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
								VA("r_t2_%s_%cwWrEn_2x", pMod->m_modName.Lc().c_str(), imCh), pGv->m_dimenList);

							if (idW > 0) {
								GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", idW),
									VA("r_t2_%s_%cw%sId_2x", pMod->m_modName.Lc().c_str(), imCh, idStr.c_str()), pGv->m_dimenList);
							}

							if (imIdx == 0) {
								GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
									VA("r_t2_%s_%cwComp_2x", pMod->m_modName.Lc().c_str(), imCh), pGv->m_dimenList);
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
									CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
									int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
									char imCh = imIdx == 0 ? 'i' : 'm';
									string idStr = imIdx == 0 ? "Ht" : "Grp";
									int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

									ngvReg_2x.Append("%sr_t2_%s_%cwWrEn_2x%s = c_t1_%s_%cwWrEn_2x%s;\n", tabs.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());

									if (idW > 0) {
										ngvReg_2x.Append("%sr_t2_%s_%cw%sId_2x%s = c_t1_%s_%cw%sId_2x%s;\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
									}
									if (imIdx == 0) {
										ngvReg_2x.Append("%sr_t2_%s_%cwComp_2x%s = c_t1_%s_%cwComp_2x%s;\n", tabs.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());
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
			CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[0].first].m_pMod;
			char imCh = pNgvInfo->m_wrPortList[0].second == 0 ? 'i' : 'm';

			CHtCode &ngvPreRegMod = pMod->m_clkRate == eClk2x ? ngvPreReg_2x : ngvPreReg_1x;
			string ngvModClk = pMod->m_clkRate == eClk2x ? "_2x" : "";

			{
				string tabs = "\t";
				CLoopInfo loopInfo(ngvPreRegMod, tabs, pGv->m_dimenList, 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvPreRegMod.Append("%sc_t%d_wrEn%s%s = r_%s_%cwWrEn%s%s;\n", tabs.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						pMod->m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str());

					if (pGv->m_addrW > 0) {
						ngvPreRegMod.Append("%sc_t%d_wrAddr%s%s = r_%s_%cwData%s%s.GetAddr();\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							pMod->m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str());
					}

					for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
						if (iter.IsStructOrUnion()) continue;

						ngvPreRegMod.Append("%sc_t%d_wrData%s%s%s = r_%s_%cwData%s%s%s.GetData();\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
							pMod->m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
					}
				} while (loopInfo.Iter());
			}

		} else if (pNgvInfo->m_wrPortList.size() == 1 && (ngvFieldCnt > 1 || bNgvAtomic)) {
			CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[0].first].m_pMod;
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
							stgIdx, pMod->m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str());
						wrCompStg = VA("t%d_", stgIdx + 1);
					} else {
						ngvPreRegMod.Append("%sc_t%d_gwWrEn%s%s = r_%s_%cwWrEn%s%s;\n", tabs.c_str(),
							stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
							pMod->m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str());
					}
					ngvPreRegMod.Append("%sc_t%d_gwData%s%s = r_%s_%cwData%s%s;\n", tabs.c_str(),
						stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
						pMod->m_modName.Lc().c_str(), imCh, ngvModClk.c_str(), dimIdx.c_str());

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

				CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[phaseIdx].first].m_pMod;
				char imCh = pNgvInfo->m_wrPortList[phaseIdx].second == 0 ? 'i' : 'm';

				{
					string tabs = "\t\t";
					CLoopInfo loopInfo(ngvPreReg_2x, tabs, pGv->m_dimenList, 3);
					do {
						string dimIdx = loopInfo.IndexStr();

						if (ngvFieldCnt == 1 && !bNgvAtomic) {

							ngvPreReg_2x.Append("%sc_t%d_wrEn%s%s = r_%s_%cwWrEn%s%s;\n", tabs.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str());

						} else {
							if (bNeedAddrComp) {
								ngvPreReg_2x.Append("%sc_t%d_gwWrEn%s%s = c_t%d_%s_%cwRrSel%s%s;\n", tabs.c_str(),
									stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
									stgIdx, pMod->m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str());
								wrCompStg = VA("t%d_", stgIdx + 1);
							} else {
								ngvPreReg_2x.Append("%sc_t%d_gwWrEn%s%s = r_%s_%cwWrEn%s%s;\n", tabs.c_str(),
									stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), imCh, queRegWrEnSig.c_str(), dimIdx.c_str());
							}
						}

						if (pGv->m_addrW > 0) {

							ngvPreReg_2x.Append("%sc_t%d_wrAddr%s%s = r_%s_%cwData%s%s.read().GetAddr();\n", tabs.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());
						}

						if (ngvFieldCnt == 1 && !bNgvAtomic) {
							for (CStructElemIter iter(this, pGv->m_pType); !iter.end(); iter++) {
								if (iter.IsStructOrUnion()) continue;

								ngvPreReg_2x.Append("%sc_t%d_wrData%s%s%s = r_%s_%cwData%s%s.read()%s.GetData();\n", tabs.c_str(),
									stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str(),
									pMod->m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str(), iter.GetHeirFieldName().c_str());
							}

						} else {

							ngvPreReg_2x.Append("%sc_t%d_gwData%s%s = r_%s_%cwData%s%s;\n", tabs.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str(),
								pMod->m_modName.Lc().c_str(), imCh, queRegDataSig.c_str(), dimIdx.c_str());
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
						CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

						if (bNeedIfStg) {
							if (stgIdx > 0) {
								ngvRegDecl.Append("\tbool c_t%d_%s_%cwWrEn%s%s;\n",
									stgIdx, pMod->m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
							}

							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
								VA("r_t%d_%s_%cwWrEn%s", stgIdx + 1, pMod->m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str()), pGv->m_dimenList);
						}

						if (bNeedIfStg && idW > 0) {
							ngvRegDecl.Append("\tht_uint%d c_t%d_%s_%cw%sId%s%s;\n",
								idW,
								stgIdx, pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("ht_uint%d", idW),
								VA("r_t%d_%s_%cw%sId%s", stgIdx + 1, pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrDataClk.c_str()), pGv->m_dimenList);
						}

						if (bNeedCompStg && imIdx == 0) {
							ngvRegDecl.Append("\tbool c_t%d_%s_%cwComp%s%s;\n",
								stgIdx, pMod->m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());

							GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
								VA("r_t%d_%s_%cwComp%s", stgIdx + 1, pMod->m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str()), pGv->m_dimenList);
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
						CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
						int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
						char imCh = imIdx == 0 ? 'i' : 'm';
						string idStr = imIdx == 0 ? "Ht" : "Grp";
						int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

						if (bNeedIfStg && stgIdx > 0 || bNeedIfStg && idW > 0 || bNeedCompStg && imIdx == 0) {
							string tabs = "\t";
							CLoopInfo loopInfo(ngvPreRegWrData, tabs, pGv->m_dimenList, 3);
							do {
								string dimIdx = loopInfo.IndexStr();

								if (bNeedIfStg && stgIdx > 0) {
									ngvPreRegWrData.Append("%sc_t%d_%s_%cwWrEn%s%s = r_t%d_%s_%cwWrEn%s%s;\n", tabs.c_str(),
										stgIdx, pMod->m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str(),
										stgIdx, pMod->m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str());
								}

								if (bNeedIfStg && idW > 0) {
									if (stgIdx == 0) {
										ngvPreRegWrData.Append("%sc_t%d_%s_%cw%sId%s = r_%s_%cw%sId%s;\n", tabs.c_str(),
											stgIdx, pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), dimIdx.c_str());
									} else {
										ngvPreRegWrData.Append("%sc_t%d_%s_%cw%sId%s%s = r_t%d_%s_%cw%sId%s%s;\n", tabs.c_str(),
											stgIdx, pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
											stgIdx, pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
									}
								}

								if (bNeedCompStg && imIdx == 0) {
									if (stgIdx == 0) {
										ngvPreRegWrData.Append("%sc_t%d_%s_%cwComp%s = r_%s_%cwComp%s;\n", tabs.c_str(),
											stgIdx, pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());
									} else {
										ngvPreRegWrData.Append("%sc_t%d_%s_%cwComp%s%s = r_t%d_%s_%cwComp%s%s;\n", tabs.c_str(),
											stgIdx, pMod->m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str(),
											stgIdx, pMod->m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str());
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
								CModule * pMod = ngvModInfoList[pNgvInfo->m_wrPortList[ngvIdx].first].m_pMod;
								int imIdx = pNgvInfo->m_wrPortList[ngvIdx].second;
								char imCh = imIdx == 0 ? 'i' : 'm';
								string idStr = imIdx == 0 ? "Ht" : "Grp";
								int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

								if (bNeedIfStg) {
									if (stgIdx == 0) {
										ngvReg.Append("%sr_t%d_%s_%cwWrEn%s = c_t%d_%s_%cwRrSel%s;\n", tabs.c_str(),
											stgIdx + 1, pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str(),
											stgIdx, pMod->m_modName.Lc().c_str(), imCh, dimIdx.c_str());
									} else {
										ngvReg.Append("%sr_t%d_%s_%cwWrEn%s%s = c_t%d_%s_%cwWrEn%s%s;\n", tabs.c_str(),
											stgIdx + 1, pMod->m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str(),
											stgIdx, pMod->m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str());
									}

									if (idW > 0) {
										ngvReg.Append("%sr_t%d_%s_%cw%sId%s%s = c_t%d_%s_%cw%sId%s%s;\n", tabs.c_str(),
											stgIdx + 1, pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
											stgIdx, pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
									}
								}

								if (bNeedCompStg && imIdx == 0) {
									ngvReg.Append("%sr_t%d_%s_%cwComp%s%s = c_t%d_%s_%cwComp%s%s;\n", tabs.c_str(),
										stgIdx + 1, pMod->m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str(),
										stgIdx, pMod->m_modName.Lc().c_str(), imCh, ngvWrDataClk.c_str(), dimIdx.c_str());
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
			CModule * pMod = ngvModInfoList[modIdx].m_pMod;
			CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

			for (int imIdx = 0; imIdx < 2; imIdx += 1) {
				if (imIdx == 0 && !pModNgv->m_bWriteForInstrWrite) continue;
				if (imIdx == 1 && !pModNgv->m_bWriteForMifRead) continue;
				char imCh = imIdx == 0 ? 'i' : 'm';
				string idStr = imIdx == 0 ? "Ht" : "Grp";
				int idW = imIdx == 0 ? pMod->m_threads.m_htIdW.AsInt() : pMod->m_mif.m_mifRd.m_rspGrpW.AsInt();

				string tabs = "\t";
				CLoopInfo loopInfo(ngvWrCompOut, tabs, pGv->m_dimenList, 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvWrCompOut.Append("%so_%sTo%s_%cwCompRdy%s = r_%s%s_%cwWrEn%s%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), imCh, dimIdx.c_str(),
						wrCompStg.c_str(), pMod->m_modName.Lc().c_str(), imCh, ngvSelClk.c_str(), dimIdx.c_str());
					if (idW > 0) {
						ngvWrCompOut.Append("%so_%sTo%s_%cwComp%sId%s = r_%s%s_%cw%sId%s%s;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), imCh, idStr.c_str(), dimIdx.c_str(),
							wrCompStg.c_str(), pMod->m_modName.Lc().c_str(), imCh, idStr.c_str(), ngvSelClk.c_str(), dimIdx.c_str());
					}
					if (imIdx == 0) {
						ngvWrCompOut.Append("%so_%sTo%s_%cwCompData%s = r_%s%s_%cwComp%s%s;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), imCh, dimIdx.c_str(),
							wrCompStg.c_str(), pMod->m_modName.Lc().c_str(), imCh, ngvSelClk.c_str(), dimIdx.c_str());
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
			CModule * pMod = ngvModInfoList[modIdx].m_pMod;
			//CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

			int ifStgIdx = stgIdx + 1 + (bNgvBlock ? 1 : 0);

			if (pNgvInfo->m_atomicMask != 0) {
				string tabs = "\t";
				CLoopInfo loopInfo(ngvOut, tabs, pGv->m_dimenList, 3);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvOut.Append("%so_%sTo%s_ifWrEn%s = r_t%d_%s_ifWrEn%s%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
						ifStgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
					if (pMod->m_threads.m_htIdW.AsInt() > 0)
						ngvOut.Append("%so_%sTo%s_ifHtId%s = r_t%d_%s_ifHtId%s%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
						ifStgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
					ngvOut.Append("%so_%sTo%s_ifData%s = r_t%d_%s_ifData%s%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
						ifStgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
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
					CModule * pMod = ngvModInfoList[modIdx].m_pMod;
					//CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

					ngvRegDecl.Append("\tbool c_t%d_%s_ifWrEn%s%s;\n",
						stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, "bool",
						VA("r_t%d_%s_ifWrEn%s", stgIdx + 1, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str()), pGv->m_dimenList);

					if (pMod->m_threads.m_htIdW.AsInt() > 0) {
						ngvRegDecl.Append("\tsc_uint<%s_HTID_W> c_t%d_%s_ifHtId%s%s;\n",
							pMod->m_modName.Upper().c_str(),
							stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
						GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, VA("sc_uint<%s_HTID_W>", pMod->m_modName.Upper().c_str()),
							VA("r_t%d_%s_ifHtId%s", stgIdx + 1, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str()), pGv->m_dimenList);
					}

					ngvRegDecl.Append("\t%s c_t%d_%s_ifData%s%s;\n",
						pGv->m_type.c_str(),
						stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), pGv->m_dimenDecl.c_str());
					GenVcdDecl(ngvSosCode, eVcdAll, ngvRegDecl, vcdModName, pGv->m_type,
						VA("r_t%d_%s_ifData%s", stgIdx + 1, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str()), pGv->m_dimenList);

					ngvRegDecl.NewLine();

					string regStg = stgIdx == 0 ? "r" : VA("r_t%d", stgIdx);

					{
						string tabs = "\t";
						CLoopInfo loopInfo(ngvPreRegWrData, tabs, pGv->m_dimenList, 2);
						do {
							string dimIdx = loopInfo.IndexStr();

							if (bRrSelAB) {
								ngvPreRegWrData.Append("%sc_t%d_%s_ifWrEn%s%s = r_t1_%s_iwWrEn%s_sig%s && %sr_phase;\n", tabs.c_str(),
									stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
									pMod->m_modName.Lc().c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(), (int)modIdx < ngvPortCnt / 2 ? "" : "!");
							} else if (bRrSelEO && pNgvInfo->m_bNgvMaxSel) {
								ngvPreRegWrData.Append("%sc_t%d_%s_ifWrEn%s%s = r_t%d_%s_iwWrEn_2x%s;\n", tabs.c_str(),
									stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
									stgIdx, pMod->m_modName.Lc().c_str(), dimIdx.c_str());
							} else if (pNgvInfo->m_wrPortList.size() == 2 && pNgvInfo->m_bAllWrPortClk1x) {
								if (bNgvBlock) {
									ngvPreRegWrData.Append("%sc_t%d_%s_ifWrEn%s%s = r_t%d_%s_iwWrEn%s%s;\n", tabs.c_str(),
										stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										stgIdx, pMod->m_modName.Lc().c_str(), queRegWrEnSig.c_str(), dimIdx.c_str());
								} else if (bNgvAtomicSlow) {
									ngvPreRegWrData.Append("%sc_t%d_%s_ifWrEn%s%s = r_%s_iwWrEn%s%s;\n", tabs.c_str(),
										stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), queRegWrEnSig.c_str(), dimIdx.c_str());
								} else {
									ngvPreRegWrData.Append("%sc_t%d_%s_ifWrEn%s%s = r_%s_iwWrEn%s%s && %sr_phase;\n", tabs.c_str(),
										stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), queRegWrEnSig.c_str(), dimIdx.c_str(),
										modIdx == 0 ? "!" : "");
								}
							} else {
								ngvPreRegWrData.Append("%sc_t%d_%s_ifWrEn%s%s = %s_%s_iwWrEn%s%s;\n", tabs.c_str(),
									stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
									regStg.c_str(), pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
							}

							if (pMod->m_threads.m_htIdW.AsInt() > 0) {
								if (bRrSelAB) {
									ngvPreRegWrData.Append("%sc_t%d_%s_ifHtId%s%s = r_t1_%s_iwHtId%s_sig%s;\n", tabs.c_str(),
										stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										pMod->m_modName.Lc().c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
								} else if (bRrSelEO && pNgvInfo->m_bNgvMaxSel) {
									ngvPreRegWrData.Append("%sc_t%d_%s_ifHtId%s%s = r_t%d_%s_iwHtId_2x%s;\n", tabs.c_str(),
										stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										stgIdx, pMod->m_modName.Lc().c_str(), dimIdx.c_str());
								} else if (pNgvInfo->m_wrPortList.size() == 2 && pNgvInfo->m_bAllWrPortClk1x) {
									if (bNgvBlock) {
										ngvPreRegWrData.Append("%sc_t%d_%s_ifHtId%s%s = r_t%d_%s_iwHtId%s%s;\n", tabs.c_str(),
											stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
											stgIdx, pMod->m_modName.Lc().c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
									} else {
										ngvPreRegWrData.Append("%sc_t%d_%s_ifHtId%s%s = r_%s_iwHtId%s%s;\n", tabs.c_str(),
											stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
											pMod->m_modName.Lc().c_str(), queRegHtIdSig.c_str(), dimIdx.c_str());
									}
								} else {
									ngvPreRegWrData.Append("%sc_t%d_%s_ifHtId%s%s = %s_%s_iwHtId%s%s;\n", tabs.c_str(),
										stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
										regStg.c_str(), pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
								}
							}

							ngvPreRegWrData.Append("%sc_t%d_%s_ifData%s%s = c_t%d_wrData%s%s;\n", tabs.c_str(),
								stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, ngvWrDataClk.c_str(), dimIdx.c_str());

						} while (loopInfo.Iter());
					}

					{
						string tabs = "\t";
						CLoopInfo loopInfo(ngvRegWrData, tabs, pGv->m_dimenList, 10);
						do {
							string dimIdx = loopInfo.IndexStr();

							ngvRegWrData.Append("%sr_t%d_%s_ifWrEn%s%s = c_t%d_%s_ifWrEn%s%s;\n", tabs.c_str(),
								stgIdx + 1, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());

							if (pMod->m_threads.m_htIdW.AsInt() > 0) {
								ngvRegWrData.Append("%sr_t%d_%s_ifHtId%s%s = c_t%d_%s_ifHtId%s%s;\n", tabs.c_str(),
									stgIdx + 1, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
									stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
							}

							ngvRegWrData.Append("%sr_t%d_%s_ifData%s%s = c_t%d_%s_ifData%s%s;\n", tabs.c_str(),
								stgIdx + 1, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str(),
								stgIdx, pMod->m_modName.Lc().c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());

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
				CModule * pMod = ngvModInfoList[modIdx].m_pMod;
				CRam * pModNgv = ngvModInfoList[modIdx].m_pNgv;

				if (!pModNgv->m_bReadForInstrRead && !pModNgv->m_bReadForMifWrite) continue;

				string tabs = "\t";
				CLoopInfo loopInfo(ngvOut, tabs, pGv->m_dimenList, 10);
				do {
					string dimIdx = loopInfo.IndexStr();

					ngvOut.Append("%so_%sTo%s_wrEn%s = %s_wrEn%s%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
						varPrefix.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
					if (pGv->m_addrW > 0) {
						ngvOut.Append("%so_%sTo%s_wrAddr%s = %s_wrAddr%s%s;\n", tabs.c_str(),
							pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
							varPrefix.c_str(), ngvWrDataClk.c_str(), dimIdx.c_str());
					}
					ngvOut.Append("%so_%sTo%s_wrData%s = %s_wrData%s%s;\n", tabs.c_str(),
						pGv->m_gblName.Lc().c_str(), pMod->m_modName.Uc().c_str(), dimIdx.c_str(),
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
		if (bNeed2x) {
			ngvRegDecl.Append("\tbool ht_noload r_reset2x;\n");
			ngvRegDecl.Append("\tsc_signal<bool> ht_noload c_reset2x;\n");
		}
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
			fprintf(incFile, "\t\tc_reset2x = true;\n");
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
		fprintf(cppFile, "\n");

		if (bNeed2x) {
			fprintf(cppFile, "\tht_attrib(equivalent_register_removal, r_reset2x, \"no\");\n");
			fprintf(cppFile, "\tHtResetFlop(r_reset2x, i_reset.read());\n");
			fprintf(cppFile, "\tc_reset2x = r_reset1x;\n");
			fprintf(cppFile, "\n");
		}

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
			fprintf(cppFile, "\tr_phase = c_reset2x.read() || !r_phase;\n");
			fprintf(cppFile, "\n");
		}

		ngvOut_2x.Write(cppFile);

		if (bNeed2x)
			fprintf(cppFile, "}\n");

		cppFile.FileClose();
	}
}
