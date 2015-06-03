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

#define MIF_CHK_STATE g_appArgs.IsVcdAllEnabled()

void CDsnInfo::InitAndValidateModMif()
{
	CCoprocInfo const & coprocInfo = g_appArgs.GetCoprocInfo();

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		CMif &mif = mod.m_mif;

		if (!mif.m_bMif) continue;

		if (mif.m_mifRd.m_bPause && mif.m_mifWr.m_bPause)
			mod.m_rsmSrcCnt += 3;
		else if (mif.m_mifRd.m_bPause || mif.m_mifWr.m_bPause)
			mod.m_rsmSrcCnt += 2;
		else if (mif.m_mifRd.m_bPoll || mif.m_mifWr.m_bPoll)
			mod.m_rsmSrcCnt += 1;

		if (mif.m_bMifRd) {
			mif.m_mifRd.m_queueW.InitValue(mif.m_mifRd.m_lineInfo);
			mif.m_mifRd.m_rspGrpW.InitValue(mif.m_mifRd.m_lineInfo, false, mod.m_threads.m_htIdW.AsInt());
			mif.m_mifRd.m_rspGrpIdW = mif.m_mifRd.m_rspGrpW.AsInt();
			mif.m_mifRd.m_rspCntW.InitValue(mif.m_mifRd.m_lineInfo);
			mif.m_queueW = mif.m_mifRd.m_queueW.AsInt();
		}

		if (mif.m_bMifWr) {
			mif.m_mifWr.m_queueW.InitValue(mif.m_mifWr.m_lineInfo);
			mif.m_mifWr.m_rspGrpW.InitValue(mif.m_mifWr.m_lineInfo, false, mod.m_threads.m_htIdW.AsInt());
			mif.m_mifWr.m_rspGrpIdW = mif.m_mifWr.m_rspGrpW.AsInt();
			mif.m_mifWr.m_rspCntW.InitValue(mif.m_mifWr.m_lineInfo);
			mif.m_queueW = mif.m_mifWr.m_queueW.AsInt();
		}

		if (mif.m_bMifRd && mif.m_bMifWr) {
			if (mif.m_mifRd.m_queueW.AsInt() != mif.m_mifWr.m_queueW.AsInt())
				ParseMsg(Error, mif.m_mifRd.m_lineInfo, "expected queueW parameter for AddReadMem and AddWriteMem to have the same value");
		}

		mif.m_mifReqStgCnt = 0;
		mif.m_maxElemCnt = 1;

		int maxRdElemQwCnt = 1;

		if (mif.m_bMifRd) {

			mif.m_mifRd.m_maxRsmDly = 0;
			for (size_t rdDstIdx = 0; rdDstIdx < mif.m_mifRd.m_rdDstList.size(); rdDstIdx += 1) {
				CMifRdDst & rdDst = mif.m_mifRd.m_rdDstList[rdDstIdx];

				rdDst.m_elemCntW.InitValue(rdDst.m_lineInfo, false);

				char const * pStr = rdDst.m_var.c_str();
				char const * pFldName = pStr;
				while (isalpha(*pStr) || isdigit(*pStr) || *pStr == '_') pStr += 1;
				string varName(pFldName, pStr - pFldName);

				if (rdDst.m_infoW.size() > 0) {
					mif.m_mifRd.m_maxRsmDly = max(mif.m_mifRd.m_maxRsmDly, rdDst.m_rsmDly.AsInt());
					rdDst.m_pDstType = &g_uint64;
					rdDst.m_varAddr0W = 0;
					rdDst.m_varAddr1W = -1;
					rdDst.m_varAddr2W = -1;
					rdDst.m_memSize = 64;

					int rdRspInfoW = 20;	// need to calculate this value

					if (rdDst.m_bMultiQwRdReq || rdRspInfoW > 0)
						mif.m_mifReqStgCnt = max(mif.m_mifReqStgCnt, 1);

					mif.m_mifRd.m_bRdRspCallBack = true;

					continue;	// function call
				}

				// first check if rdDst name is unique
				bool bFoundShared = false;
				bool bFoundPrivate = false;
				bool bFoundNewGbl = false;
				CRam * pRam = 0;
				CField * pField = 0;

				for (size_t ngvIdx = 0; ngvIdx < mod.m_ngvList.size(); ngvIdx += 1) {
					CRam * pNgv = mod.m_ngvList[ngvIdx];

					if (pNgv->m_gblName == varName) {
						bFoundNewGbl = true;
						pRam = pNgv;
					}
				}

				for (size_t shIdx = 0; shIdx < mod.m_shared.m_fieldList.size(); shIdx += 1) {
					CField * pShared = mod.m_shared.m_fieldList[shIdx];

					if (pShared->m_name == varName) {
						bFoundShared = true;
						pField = pShared;
					}
				}

				for (size_t prIdx = 0; prIdx < mod.m_threads.m_htPriv.m_fieldList.size(); prIdx += 1) {
					CField * pPriv = mod.m_threads.m_htPriv.m_fieldList[prIdx];

					if (pPriv->m_name == varName) {
						bFoundPrivate = true;
						pField = pPriv;
					}
				}

				string ramName = mod.m_modName.AsStr() + "__" + varName;

				// check for private variable converted to a global ram
				for (size_t ngvIdx = 0; ngvIdx < mod.m_ngvList.size(); ngvIdx += 1) {
					CRam * pNgv = mod.m_ngvList[ngvIdx];

					if (pNgv->m_gblName == ramName) {
						bFoundNewGbl = true;
						pRam = pNgv;
					}
				}

				if (bFoundNewGbl + bFoundShared + bFoundPrivate == 0) {

					ParseMsg(Fatal, rdDst.m_lineInfo, "unable to find private, shared or global destination variable, '%s'", varName.c_str());

				}
				else if (bFoundNewGbl + bFoundShared + bFoundPrivate > 1) {
					string msg;
					if (bFoundNewGbl)
						msg += "global";
					if (bFoundShared) {
						if (msg.size() > 0)
						if (bFoundPrivate)
							msg += ", ";
						else
							msg += " or ";
						msg += "shared";
					}
					if (bFoundPrivate) {
						if (msg.size() > 0)
							msg += " or ";
						msg += "private";
					}

					ParseMsg(Error, rdDst.m_lineInfo, "read destination name is ambiguous, could be %s variable", msg.c_str());
				}

				// Find the width of the address for the destination ram

				if (bFoundNewGbl) {

					rdDst.m_pGblVar = pRam;
					rdDst.m_pGblVar->m_bWriteForMifRead = true;

					rdDst.m_varAddr0W = 0;
					rdDst.m_varAddr1W = pRam->m_addr1W.size() == 0 ? -1 : pRam->m_addr1W.AsInt();
					rdDst.m_varAddr2W = pRam->m_addr2W.size() == 0 ? -1 : pRam->m_addr2W.AsInt();
					rdDst.m_varAddr1IsHtId = pRam->m_addr1Name == "htId";
					rdDst.m_varAddr2IsHtId = pRam->m_addr2Name == "htId";
					rdDst.m_varDimen1 = pRam->GetDimen(0);
					rdDst.m_varDimen2 = pRam->GetDimen(1);

					vector<CVarAddr> varAddrList;
					if (pRam->m_addr1W.size() > 0) varAddrList.push_back(CVarAddr(pRam->m_addr1W.AsInt(), rdDst.m_varAddr1IsHtId));
					if (pRam->m_addr2W.size() > 0) varAddrList.push_back(CVarAddr(pRam->m_addr2W.AsInt(), rdDst.m_varAddr2IsHtId));

					// check if ram has field specified in AddRdDst
					ParseVarRefSpec(rdDst.m_lineInfo, pRam->m_gblName, pRam->m_dimenList, varAddrList,
						pRam->m_type, rdDst.m_var, rdDst.m_fieldRefList, rdDst.m_pDstType);

				}
				else if (bFoundShared) {

					// found a shared variable
					rdDst.m_pSharedVar = pField;
					rdDst.m_varAddr0W = 0;
					rdDst.m_varAddr1W = pField->m_addr1W.size() == 0 ? -1 : pField->m_addr1W.AsInt();
					rdDst.m_varAddr2W = pField->m_addr2W.size() == 0 ? -1 : pField->m_addr2W.AsInt();
					rdDst.m_varDimen1 = pField->GetDimen(0);
					rdDst.m_varDimen2 = pField->GetDimen(1);

					vector<CVarAddr> varAddrList;
					if (pField->m_addr1W.size() > 0) varAddrList.push_back(CVarAddr(pField->m_addr1W.AsInt(), rdDst.m_varAddr1IsHtId));
					if (pField->m_addr2W.size() > 0) varAddrList.push_back(CVarAddr(pField->m_addr2W.AsInt(), rdDst.m_varAddr2IsHtId));

					// check if ram has field specified in AddDst
					ParseVarRefSpec(rdDst.m_lineInfo, pField->m_name, pField->m_dimenList, varAddrList,
						pField->m_type, rdDst.m_var, rdDst.m_fieldRefList, rdDst.m_pDstType);

					if (pField->m_queueW.AsInt() > 0)
						ParseMsg(Error, pField->m_lineInfo, "memory read to shared queue variable not supported");

					if (rdDst.m_pSharedVar->m_ramType == eBlockRam && rdDst.m_varAddr1W >= 0 &&
						rdDst.m_pDstType->m_clangMinAlign != 1 && rdDst.m_pDstType->m_clangBitWidth != rdDst.m_pDstType->m_clangMinAlign)
						ParseMsg(Error, pField->m_lineInfo, "memory read to shared variable with read type that requires multiple block ram writes not supported");

					if (rdDst.m_pSharedVar->m_ramType == eBlockRam && rdDst.m_varAddr1W >= 0 &&
						rdDst.m_pDstType->m_clangBitWidth != rdDst.m_pSharedVar->m_pType->m_clangBitWidth)
						ParseMsg(Error, pField->m_lineInfo, "memory read to shared variable implemented as a block ram with read type that requires a partial write not supported");
				}
				else if (bFoundPrivate) {

					// found a private variable
					rdDst.m_pPrivVar = pField;
					rdDst.m_varAddr0W = 0;
					rdDst.m_varAddr1W = pField->m_addr1W.size() == 0 ? -1 : pField->m_addr1W.AsInt();
					rdDst.m_varAddr2W = pField->m_addr2W.size() == 0 ? -1 : pField->m_addr2W.AsInt();
					rdDst.m_varDimen1 = pField->GetDimen(0);
					rdDst.m_varDimen2 = pField->GetDimen(1);

					vector<CVarAddr> varAddrList;
					if (pField->m_addr1W.size() > 0) varAddrList.push_back(CVarAddr(pField->m_addr1W.AsInt(), rdDst.m_varAddr1IsHtId));
					if (pField->m_addr2W.size() > 0) varAddrList.push_back(CVarAddr(pField->m_addr2W.AsInt(), rdDst.m_varAddr2IsHtId));

					// check if ram has field specified in AddDst
					ParseVarRefSpec(rdDst.m_lineInfo, pField->m_name, pField->m_dimenList, varAddrList,
						pField->m_type, rdDst.m_var, rdDst.m_fieldRefList, rdDst.m_pDstType);

				}
				else
					HtlAssert(0);

				if (rdDst.m_pRdType)
					rdDst.m_memSize = rdDst.m_pRdType->m_clangBitWidth;
				else if (rdDst.m_pDstType) {
					rdDst.m_memSize = rdDst.m_pDstType->m_clangMinAlign;
					if (rdDst.m_memSize == 1) {
						if (rdDst.m_pDstType->m_clangBitWidth == 8 || rdDst.m_pDstType->m_clangBitWidth == 16 ||
							rdDst.m_pDstType->m_clangBitWidth == 32 || rdDst.m_pDstType->m_clangBitWidth == 64) {
							rdDst.m_memSize = rdDst.m_pDstType->m_clangBitWidth;
						}
						else
							ParseMsg(Error, rdDst.m_lineInfo, "rdType parameter must be specified when type is an ht_uint or ht_int");
					}
				}
				else
					rdDst.m_memSize = 64;

				int rdDstSize = 1;
				if (rdDst.m_fieldRefList.size() > 0) {
					for (size_t i = 0; i < rdDst.m_fieldRefList.back().m_refDimenList.size(); i += 1)
						rdDstSize *= rdDst.m_fieldRefList.back().m_refDimenList[i].m_size;
				}

				rdDst.m_bMultiElemRd = (rdDst.m_elemCntW.size() == 0 || rdDst.m_elemCntW.AsInt() > 0) &&
					(rdDst.m_fieldRefList.size() == 1 && rdDst.m_varAddr1W > 0 ||
					rdDstSize > 1);

				rdDst.m_bMultiQwRdReq = rdDst.m_bMultiElemRd || rdDst.m_pDstType->m_clangMinAlign == 64 &&
					rdDst.m_pDstType->m_clangBitWidth > rdDst.m_pDstType->m_clangMinAlign;
				rdDst.m_bMultiQwHostRdReq = rdDst.m_bMultiQwRdReq && (rdDst.m_memSrc.size() == 0 || rdDst.m_memSrc == "host");
				rdDst.m_bMultiQwCoprocRdReq = rdDst.m_bMultiQwRdReq && (rdDst.m_memSrc.size() == 0 || rdDst.m_memSrc == "coproc");

				rdDst.m_bMultiQwHostRdMif = rdDst.m_bMultiQwHostRdReq && coprocInfo.GetMaxHostQwReadCnt() > 1 && rdDst.m_memSize == 64;
				rdDst.m_bMultiQwCoprocRdMif = rdDst.m_bMultiQwCoprocRdReq && coprocInfo.GetMaxCoprocQwReadCnt() > 1 && rdDst.m_memSize == 64;

				mif.m_mifRd.m_bMultiQwRdReq |= rdDst.m_bMultiQwRdReq;
				mif.m_mifRd.m_bHostRdReq |= (rdDst.m_memSrc.size() == 0 || rdDst.m_memSrc == "host");
				mif.m_mifRd.m_bCoprocRdReq |= (rdDst.m_memSrc.size() == 0 || rdDst.m_memSrc == "coproc");

				mif.m_mifRd.m_bMultiQwHostRdReq |= rdDst.m_bMultiQwHostRdReq;
				mif.m_mifRd.m_bMultiQwCoprocRdReq |= rdDst.m_bMultiQwCoprocRdReq;

				mif.m_mifRd.m_bMultiQwHostRdMif |= rdDst.m_bMultiQwHostRdMif;
				mif.m_mifRd.m_bMultiQwCoprocRdMif |= rdDst.m_bMultiQwCoprocRdMif;

				int rdRspInfoW = 20;	// need to calculate this value

				if (rdDst.m_bMultiQwRdReq || rdRspInfoW > 0)
					mif.m_mifReqStgCnt = max(mif.m_mifReqStgCnt, 1);

				// find maxRdElemQwCnt and maxElemCnt
				int elemQwCnt = rdDst.m_pDstType->m_clangMinAlign == 1 ? 1 :
					((rdDst.m_pDstType->m_clangBitWidth + rdDst.m_pDstType->m_clangMinAlign - 1) / rdDst.m_pDstType->m_clangMinAlign);
				maxRdElemQwCnt = max(maxRdElemQwCnt, elemQwCnt);

				rdDst.m_maxElemCnt = 1;
				if (rdDst.m_fieldRefList.size() == 1 && rdDst.m_varAddr1W > 0) {
					rdDst.m_maxElemCnt *= 1 << (rdDst.m_varAddr1W >= 0 ? rdDst.m_varAddr1W : 0);
					rdDst.m_maxElemCnt *= 1 << (rdDst.m_varAddr2W >= 0 ? rdDst.m_varAddr2W : 0);
				}
				else if (rdDst.m_fieldRefList.back().m_refDimenList.size() > 0) {
					vector<CRefDimen> & refDimen = rdDst.m_fieldRefList.back().m_refDimenList;
					for (size_t i = 0; i < refDimen.size(); i += 1)
						rdDst.m_maxElemCnt *= refDimen[i].m_size;
				}
				if (rdDst.m_elemCntW.size() > 0)
					rdDst.m_maxElemCnt = min(rdDst.m_maxElemCnt, 1 << rdDst.m_elemCntW.AsInt());
				mif.m_maxElemCnt = max(mif.m_maxElemCnt, rdDst.m_maxElemCnt);

				// generate common list of vIdx widths
				bool bMultiElemRd = rdDst.m_infoW.size() == 0 && (rdDst.m_elemCntW.size() == 0 || rdDst.m_elemCntW.AsInt() > 0) &&
					(rdDst.m_fieldRefList.size() == 1 && rdDst.m_varAddr1W > 0 ||
					rdDst.m_fieldRefList.back().m_refDimenList.size() > 0);

				if (bMultiElemRd) {
					if (rdDst.m_fieldRefList.size() == 1 && rdDst.m_varAddr1W > 0) {
						int addr1W = rdDst.m_varAddr1IsHtId ? 0 : rdDst.m_varAddr1W;
						if (mif.m_vIdxWList.size() <= 0)
							mif.m_vIdxWList.push_back(addr1W);
						else
							mif.m_vIdxWList[0] = max(mif.m_vIdxWList[0], addr1W);

						int addr2W = rdDst.m_varAddr2IsHtId ? 0 : rdDst.m_varAddr2W;
						if (rdDst.m_varAddr2W > 0) {
							if (mif.m_vIdxWList.size() <= 1)
								mif.m_vIdxWList.push_back(addr2W);
							else
								mif.m_vIdxWList[1] = max(mif.m_vIdxWList[1], addr2W);
						}
					}
					else {
						for (size_t dimIdx = 0; dimIdx < rdDst.m_fieldRefList.back().m_refDimenList.size(); dimIdx += 1) {
							CRefDimen & refDimen = rdDst.m_fieldRefList.back().m_refDimenList[dimIdx];
							int vIdxW = refDimen.m_value >= 0 ? 0 : FindLg2(refDimen.m_size - 1, false);
							if (mif.m_vIdxWList.size() <= dimIdx)
								mif.m_vIdxWList.push_back(vIdxW);
							else
								mif.m_vIdxWList[dimIdx] = max(mif.m_vIdxWList[dimIdx], vIdxW);
						}
					}
				}
			}
		}

		int maxWrElemQwCnt = 1;

		if (mif.m_bMifWr) {

			for (size_t wrSrcIdx = 0; wrSrcIdx < mif.m_mifWr.m_wrSrcList.size(); wrSrcIdx += 1) {
				CMifWrSrc & wrSrc = mif.m_mifWr.m_wrSrcList[wrSrcIdx];

				if (wrSrc.m_pType != 0) {

					wrSrc.m_pSrcType = wrSrc.m_pType;

					wrSrc.m_varAddr1W = -1;
					wrSrc.m_varAddr2W = -1;
					if (wrSrc.m_pSrcType->m_clangMinAlign == 1) {
						if (wrSrc.m_pWrType)
							wrSrc.m_memSize = wrSrc.m_pWrType->m_clangBitWidth;
						else
							ParseMsg(Error, wrSrc.m_lineInfo, "wrType parameter must be specified when type is an ht_uint or ht_int");
					}
					else
						wrSrc.m_memSize = wrSrc.m_pSrcType->m_clangMinAlign;

					wrSrc.m_bMultiElemWr = false;

					wrSrc.m_bMultiQwWrReq = wrSrc.m_pSrcType->m_clangBitWidth > 64 && wrSrc.m_pSrcType->m_clangMinAlign == 64;
					mif.m_mifWr.m_bMultiQwWrReq |= wrSrc.m_bMultiQwWrReq;

					mif.m_mifReqStgCnt = max(mif.m_mifReqStgCnt, 1);

					wrSrc.m_maxElemCnt = 1;

					continue;
				}

				wrSrc.m_elemCntW.InitValue(wrSrc.m_lineInfo, false);

				char const * pStr = wrSrc.m_var.c_str();
				char const * pFldName = pStr;
				while (isalpha(*pStr) || isdigit(*pStr) || *pStr == '_') pStr += 1;
				string varName(pFldName, pStr - pFldName);

				// first check if wrSrc name is unique
				bool bFoundShared = false;
				bool bFoundPrivate = false;
				bool bFoundNewGbl = false;
				CRam * pRam = 0;
				CField * pField = 0;

				for (size_t ngvIdx = 0; ngvIdx < mod.m_ngvList.size(); ngvIdx += 1) {
					CRam * pNgv = mod.m_ngvList[ngvIdx];

					if (pNgv->m_gblName == varName) {
						bFoundNewGbl = true;
						pRam = pNgv;
					}
				}

				for (size_t shIdx = 0; shIdx < mod.m_shared.m_fieldList.size(); shIdx += 1) {
					CField * pShared = mod.m_shared.m_fieldList[shIdx];

					if (pShared->m_name == varName) {
						bFoundShared = true;
						pField = pShared;
					}
				}

				for (size_t prIdx = 0; prIdx < mod.m_threads.m_htPriv.m_fieldList.size(); prIdx += 1) {
					CField * pPriv = mod.m_threads.m_htPriv.m_fieldList[prIdx];

					if (pPriv->m_name == varName) {
						bFoundPrivate = true;
						pField = pPriv;
					}
				}

				string ramName = mod.m_modName.AsStr() + "__" + varName;

				// check for private variable converted to a global ram
				for (size_t ngvIdx = 0; ngvIdx < mod.m_ngvList.size(); ngvIdx += 1) {
					CRam * pNgv = mod.m_ngvList[ngvIdx];

					if (pNgv->m_gblName == ramName) {
						bFoundNewGbl = true;
						pRam = pNgv;
					}
				}

				if (bFoundNewGbl + bFoundShared + bFoundPrivate == 0) {

					ParseMsg(Fatal, wrSrc.m_lineInfo, "unable to find private, shared or global variable, '%s'", varName.c_str());

				}
				else if (bFoundNewGbl + bFoundShared + bFoundPrivate > 1) {
					string msg;
					if (bFoundNewGbl)
						msg += "global";
					if (bFoundShared) {
						if (msg.size() > 0)
						if (bFoundPrivate)
							msg += ", ";
						else
							msg += " or ";
						msg += "shared";
					}
					if (bFoundPrivate) {
						if (msg.size() > 0)
							msg += " or ";
						msg += "private";
					}

					ParseMsg(Error, wrSrc.m_lineInfo, "write source variable name is ambiguous, could be %s variable", msg.c_str());
				}

				// Find the width of the address for the destination ram

				if (bFoundNewGbl) {

					wrSrc.m_pGblVar = pRam;
					wrSrc.m_pGblVar->m_bReadForMifWrite = !pRam->m_bPrivGbl || pRam->m_addrW > mod.m_threads.m_htIdW.AsInt();

					wrSrc.m_varAddr0W = 0;
					wrSrc.m_varAddr1W = pRam->m_addr1W.size() == 0 ? -1 : pRam->m_addr1W.AsInt();
					wrSrc.m_varAddr2W = pRam->m_addr2W.size() == 0 ? -1 : pRam->m_addr2W.AsInt();
					wrSrc.m_varAddr1IsHtId = pRam->m_addr1Name == "htId";
					wrSrc.m_varAddr2IsHtId = pRam->m_addr2Name == "htId";
					wrSrc.m_varDimen1 = pRam->GetDimen(0);
					wrSrc.m_varDimen2 = pRam->GetDimen(1);

					vector<CVarAddr> varAddrList;
					if (pRam->m_addr1W.size() > 0) varAddrList.push_back(CVarAddr(pRam->m_addr1W.AsInt(), wrSrc.m_varAddr1IsHtId));
					if (pRam->m_addr2W.size() > 0) varAddrList.push_back(CVarAddr(pRam->m_addr2W.AsInt(), wrSrc.m_varAddr2IsHtId));

					// check if ram has field specified in AddSrc
					ParseVarRefSpec(wrSrc.m_lineInfo, pRam->m_gblName, pRam->m_dimenList, varAddrList,
						pRam->m_type, wrSrc.m_var, wrSrc.m_fieldRefList, wrSrc.m_pSrcType);

				}
				else if (bFoundShared) {

					// found a shared variable
					wrSrc.m_pSharedVar = pField;
					wrSrc.m_varAddr0W = 0;
					wrSrc.m_varAddr1W = pField->m_addr1W.size() == 0 ? -1 : pField->m_addr1W.AsInt();
					wrSrc.m_varAddr2W = pField->m_addr2W.size() == 0 ? -1 : pField->m_addr2W.AsInt();
					wrSrc.m_varDimen1 = pField->GetDimen(0);
					wrSrc.m_varDimen2 = pField->GetDimen(1);

					vector<CVarAddr> varAddrList;
					if (pField->m_addr1W.size() > 0) varAddrList.push_back(CVarAddr(pField->m_addr1W.AsInt(), wrSrc.m_varAddr1IsHtId));
					if (pField->m_addr2W.size() > 0) varAddrList.push_back(CVarAddr(pField->m_addr2W.AsInt(), wrSrc.m_varAddr2IsHtId));

					// check if ram has field specified in AddSrc
					ParseVarRefSpec(wrSrc.m_lineInfo, pField->m_name, pField->m_dimenList, varAddrList,
						pField->m_type, wrSrc.m_var, wrSrc.m_fieldRefList, wrSrc.m_pSrcType);

				}
				else if (bFoundPrivate) {

					// found a shared variable
					wrSrc.m_pPrivVar = pField;
					wrSrc.m_varAddr0W = 0;
					wrSrc.m_varAddr1W = pField->m_addr1W.size() == 0 ? -1 : pField->m_addr1W.AsInt();
					wrSrc.m_varAddr2W = pField->m_addr2W.size() == 0 ? -1 : pField->m_addr2W.AsInt();
					wrSrc.m_varDimen1 = pField->GetDimen(0);
					wrSrc.m_varDimen2 = pField->GetDimen(1);

					vector<CVarAddr> varAddrList;
					if (pField->m_addr1W.size() > 0) varAddrList.push_back(CVarAddr(pField->m_addr1W.AsInt(), wrSrc.m_varAddr1IsHtId));
					if (pField->m_addr2W.size() > 0) varAddrList.push_back(CVarAddr(pField->m_addr2W.AsInt(), wrSrc.m_varAddr2IsHtId));

					// check if ram has field specified in AddSrc
					ParseVarRefSpec(wrSrc.m_lineInfo, pField->m_name, pField->m_dimenList, varAddrList,
						pField->m_type, wrSrc.m_var, wrSrc.m_fieldRefList, wrSrc.m_pSrcType);
				}
				else
					HtlAssert(0);

				if (wrSrc.m_pWrType)
					wrSrc.m_memSize = wrSrc.m_pWrType->m_clangBitWidth;
				else if (wrSrc.m_pSrcType) {
					wrSrc.m_memSize = wrSrc.m_pSrcType->m_clangMinAlign;
					if (wrSrc.m_memSize == 1)
						ParseMsg(Error, wrSrc.m_lineInfo, "AddSrc type of ht_uint or ht_int not supported");
				}
				else
					wrSrc.m_memSize = 64;

				int wrSrcSize = 1;
				if (wrSrc.m_fieldRefList.size() > 0) {
					for (size_t i = 0; i < wrSrc.m_fieldRefList.back().m_refDimenList.size(); i += 1)
						wrSrcSize *= wrSrc.m_fieldRefList.back().m_refDimenList[i].m_size;
				}

				ERamType ramType = eRegRam;

				if (wrSrc.m_pGblVar) {
					bool bPrivGblAndNoAddr = wrSrc.m_pGblVar->m_bPrivGbl && wrSrc.m_pGblVar->m_addrW == mod.m_threads.m_htIdW.AsInt();
					if (!bPrivGblAndNoAddr)
						ramType = wrSrc.m_pGblVar->m_ramType;
				} else if (wrSrc.m_pSharedVar)
					ramType = wrSrc.m_pSharedVar->m_ramType;

				wrSrc.m_bMultiElemWr = (wrSrc.m_elemCntW.size() == 0 || wrSrc.m_elemCntW.AsInt() > 0) &&
					(wrSrc.m_fieldRefList.size() == 1 && wrSrc.m_varAddr1W > 0 ||
					wrSrcSize > 1);

				wrSrc.m_bMultiQwWrReq = wrSrc.m_bMultiElemWr || wrSrc.m_pSrcType &&
					wrSrc.m_pSrcType->m_clangMinAlign != 1 && wrSrc.m_pSrcType->m_clangBitWidth > wrSrc.m_pSrcType->m_clangMinAlign;
				wrSrc.m_bMultiQwHostWrReq = wrSrc.m_bMultiQwWrReq && (wrSrc.m_memDst.size() == 0 || wrSrc.m_memDst == "host");
				wrSrc.m_bMultiQwCoprocWrReq = wrSrc.m_bMultiQwWrReq && (wrSrc.m_memDst.size() == 0 || wrSrc.m_memDst == "coproc");

				wrSrc.m_bMultiQwHostWrMif = wrSrc.m_bMultiQwHostWrReq && coprocInfo.GetMaxHostQwWriteCnt() > 1 && wrSrc.m_memSize == 64;
				wrSrc.m_bMultiQwCoprocWrMif = wrSrc.m_bMultiQwCoprocWrReq && coprocInfo.GetMaxCoprocQwWriteCnt() > 1 && wrSrc.m_memSize == 64;

				mif.m_mifWr.m_bMultiQwWrReq |= wrSrc.m_bMultiQwWrReq;
				mif.m_mifWr.m_bHostWrReq |= (wrSrc.m_memDst.size() == 0 || wrSrc.m_memDst == "host");
				mif.m_mifWr.m_bCoprocWrReq |= (wrSrc.m_memDst.size() == 0 || wrSrc.m_memDst == "coproc");

				mif.m_mifWr.m_bMultiQwHostWrReq |= wrSrc.m_bMultiQwHostWrReq;
				mif.m_mifWr.m_bMultiQwCoprocWrReq |= wrSrc.m_bMultiQwCoprocWrReq;

				mif.m_mifWr.m_bMultiQwHostWrMif |= wrSrc.m_bMultiQwHostWrMif;
				mif.m_mifWr.m_bMultiQwCoprocWrMif |= wrSrc.m_bMultiQwCoprocWrMif;

				mif.m_mifWr.m_bDistRamAccessReq |= wrSrc.m_varAddr1W > 0 && ramType == eDistRam;
				mif.m_mifWr.m_bBlockRamAccessReq |= wrSrc.m_varAddr1W > 0 && ramType == eBlockRam;

				if (wrSrc.m_varAddr1W > 0)
					mif.m_mifReqStgCnt = max(mif.m_mifReqStgCnt, 2);
				else
					mif.m_mifReqStgCnt = max(mif.m_mifReqStgCnt, 1);

				// find maxWrelemQwCnt and maxElemCnt
				int elemQwCnt = wrSrc.m_pSrcType->m_clangMinAlign == 1 ? 1 :
					((wrSrc.m_pSrcType->m_clangBitWidth + wrSrc.m_pSrcType->m_clangMinAlign - 1) / wrSrc.m_pSrcType->m_clangMinAlign);
				maxWrElemQwCnt = max(maxWrElemQwCnt, elemQwCnt);

				wrSrc.m_maxElemCnt = 1;
				if (wrSrc.m_pType != 0) {
					;
				}
				else if (wrSrc.m_fieldRefList.size() == 1 && wrSrc.m_varAddr1W > 0) {
					wrSrc.m_maxElemCnt *= 1 << (wrSrc.m_varAddr1W >= 0 ? wrSrc.m_varAddr1W : 0);
					wrSrc.m_maxElemCnt *= 1 << (wrSrc.m_varAddr2W >= 0 ? wrSrc.m_varAddr2W : 0);
				}
				else if (wrSrc.m_fieldRefList.back().m_refDimenList.size() > 0) {
					vector<CRefDimen> & refDimen = wrSrc.m_fieldRefList.back().m_refDimenList;
					for (size_t i = 0; i < refDimen.size(); i += 1)
						wrSrc.m_maxElemCnt *= refDimen[i].m_size;
				}
				if (wrSrc.m_elemCntW.size() > 0)
					wrSrc.m_maxElemCnt = min(wrSrc.m_maxElemCnt, 1 << wrSrc.m_elemCntW.AsInt());
				mif.m_maxElemCnt = max(mif.m_maxElemCnt, wrSrc.m_maxElemCnt);

				// generate common list of vIdx widths
				bool bMultiElemWr = (wrSrc.m_pType == 0) &&
					(wrSrc.m_elemCntW.size() == 0 || wrSrc.m_elemCntW.AsInt() > 0) &&
					(wrSrc.m_fieldRefList.size() == 1 && wrSrc.m_varAddr1W > 0 ||
					wrSrc.m_fieldRefList.back().m_refDimenList.size() > 0);

				if (bMultiElemWr) {
					if (wrSrc.m_fieldRefList.size() == 1 && wrSrc.m_varAddr1W > 0) {
						int addr1W = wrSrc.m_varAddr1IsHtId ? 0 : wrSrc.m_varAddr1W;
						if (mif.m_vIdxWList.size() <= 0)
							mif.m_vIdxWList.push_back(addr1W);
						else
							mif.m_vIdxWList[0] = max(mif.m_vIdxWList[0], addr1W);

						int addr2W = wrSrc.m_varAddr2IsHtId ? 0 : wrSrc.m_varAddr2W;
						if (wrSrc.m_varAddr2W > 0) {
							if (mif.m_vIdxWList.size() <= 1)
								mif.m_vIdxWList.push_back(addr2W);
							else
								mif.m_vIdxWList[1] = max(mif.m_vIdxWList[1], addr2W);
						}
					}
					else {
						for (size_t dimIdx = 0; dimIdx < wrSrc.m_fieldRefList.back().m_refDimenList.size(); dimIdx += 1) {
							CRefDimen & refDimen = wrSrc.m_fieldRefList.back().m_refDimenList[dimIdx];
							int vIdxW = refDimen.m_value >= 0 ? 0 : FindLg2(refDimen.m_size - 1, false);
							if (mif.m_vIdxWList.size() <= dimIdx)
								mif.m_vIdxWList.push_back(vIdxW);
							else
								mif.m_vIdxWList[dimIdx] = max(mif.m_vIdxWList[dimIdx], vIdxW);
						}
					}
				}
			}
		}

		if (mif.m_bMifRd) {

			int maxElemQwCnt = max(maxRdElemQwCnt, maxWrElemQwCnt);
			int maxQwCnt = mif.m_maxElemCnt * maxElemQwCnt;

			// per dst fields
			vector<CRecord> dstRecordList;
			mif.m_mifRd.m_maxRdRspInfoW = 0;
			for (size_t rdDstIdx = 0; rdDstIdx < mif.m_mifRd.m_rdDstList.size(); rdDstIdx += 1) {
				CMifRdDst &rdDst = mif.m_mifRd.m_rdDstList[rdDstIdx];

				if (mif.m_mifRd.m_bMultiQwRdReq || mif.m_mifWr.m_bMultiQwWrReq) {

					int rdDstSize = 1;
					if (rdDst.m_fieldRefList.size() > 0) {
						for (size_t i = 0; i < rdDst.m_fieldRefList.back().m_refDimenList.size(); i += 1) {
							if (rdDst.m_fieldRefList.back().m_refDimenList[i].m_value < 0)
								rdDstSize *= rdDst.m_fieldRefList.back().m_refDimenList[i].m_size;
						}
					}

					bool bMultiElemDst = rdDst.m_infoW.size() == 0 &&
						(rdDst.m_elemCntW.size() == 0 || rdDst.m_elemCntW.AsInt() > 0) &&
						(rdDst.m_fieldRefList.size() == 1 && rdDst.m_varAddr1W > 0 ||
						rdDst.m_fieldRefList.back().m_refDimenList.size() > 0 && rdDstSize > 1);

					if (rdDst.m_varAddr1W > 0 && bMultiElemDst && rdDst.m_fieldRefList.size() == 1)
						rdDst.m_varAddr1IsIdx = true;

					if (rdDst.m_varAddr2W > 0 && bMultiElemDst && rdDst.m_fieldRefList.size() == 1) 
						rdDst.m_varAddr2IsIdx = true;

					if (rdDst.m_varAddr1W <= 0 || rdDst.m_fieldRefList.size() > 1) {
						for (size_t fldIdx = 0; fldIdx < rdDst.m_fieldRefList.size(); fldIdx += 1) {
							vector<CRefDimen> & indexRangeList = rdDst.m_fieldRefList[fldIdx].m_refDimenList;
							for (size_t dimIdx = 0; dimIdx < indexRangeList.size(); dimIdx += 1) {
								if (bMultiElemDst && fldIdx + 1 == rdDst.m_fieldRefList.size()) {
									if (rdDst.m_fieldRefList[fldIdx].m_refDimenList[dimIdx].m_value < 0)
										rdDst.m_fieldRefList[fldIdx].m_refDimenList[dimIdx].m_isIdx = true;
								}
							}
						}
					}
				}

				int rdDstInfoW = 0;

				if (rdDst.m_infoW.AsInt() > 0)
					rdDstInfoW += rdDst.m_infoW.AsInt();

				if (rdDst.m_varAddr1W > 0 && !rdDst.m_varAddr1IsHtId && !rdDst.m_varAddr1IsIdx)
					rdDstInfoW += rdDst.m_varAddr1W;

				if (rdDst.m_varAddr2W > 0 && !rdDst.m_varAddr2IsHtId && !rdDst.m_varAddr2IsIdx)
					rdDstInfoW += rdDst.m_varAddr2W;

				for (size_t fldIdx = 0; fldIdx < rdDst.m_fieldRefList.size(); fldIdx += 1) {
					for (size_t dimIdx = 0; dimIdx < rdDst.m_fieldRefList[fldIdx].m_refDimenList.size(); dimIdx += 1) {
						CRefDimen & refDimen = rdDst.m_fieldRefList[fldIdx].m_refDimenList[dimIdx];

						if (refDimen.m_value < 0 && refDimen.m_size > 1 && !refDimen.m_isIdx) {
							int varDimenW = FindLg2(rdDst.m_fieldRefList[fldIdx].m_refDimenList[dimIdx].m_size - 1);
							rdDstInfoW += varDimenW;
						}
					}
				}

				mif.m_mifRd.m_maxRdRspInfoW = max(mif.m_mifRd.m_maxRdRspInfoW, rdDstInfoW);
			}

			if (mod.m_threads.m_htIdW.AsInt() > 0)
				mif.m_mifRd.m_maxRdRspInfoW += mod.m_threads.m_htIdW.AsInt();

			if (mif.m_mifRd.m_rspGrpW.size() > 0 && mif.m_mifRd.m_rspGrpW.AsInt() > 0)
				mif.m_mifRd.m_maxRdRspInfoW += mif.m_mifRd.m_rspGrpW.AsInt();

			int dstW = FindLg2(mif.m_mifRd.m_rdDstList.size() - 1, false);
			if (dstW > 0)
				mif.m_mifRd.m_maxRdRspInfoW += dstW;

			if (mif.m_mifRd.m_bMultiQwHostRdMif || mif.m_mifRd.m_bMultiQwCoprocRdMif)
				mif.m_mifRd.m_maxRdRspInfoW += 7;

			bool bNeedCntM1 = false;
			for (int idx = 0; idx < (int)mif.m_vIdxWList.size(); idx += 1) {
				if (mif.m_vIdxWList[idx] > 0) {
					mif.m_mifRd.m_maxRdRspInfoW += mif.m_vIdxWList[idx];
					if (bNeedCntM1)
						mif.m_mifRd.m_maxRdRspInfoW += mif.m_vIdxWList[idx];
					bNeedCntM1 = true;
				}
			}

			if (maxQwCnt > 1) {
				if (maxElemQwCnt > 1) {
					int elemQwIdxW = FindLg2(maxElemQwCnt - 1);
					mif.m_mifRd.m_maxRdRspInfoW += elemQwIdxW;
					if (bNeedCntM1)
						mif.m_mifRd.m_maxRdRspInfoW += elemQwIdxW;
				}
			}

			mif.m_mifRd.m_bNeedRdRspInfoRam = mif.m_mifRd.m_maxRdRspInfoW > 24;
		}

		if (mif.m_bMifWr) {

			for (size_t wrSrcIdx = 0; wrSrcIdx < mif.m_mifWr.m_wrSrcList.size(); wrSrcIdx += 1) {
				CMifWrSrc & wrSrc = mif.m_mifWr.m_wrSrcList[wrSrcIdx];

				wrSrc.m_wrDataTypeName = wrSrc.m_pSrcType->m_typeName;
				vector<CHtString> dimenList;
				wrSrc.m_bConstDimenList = true;
				string varIdx;

				if (wrSrc.m_pPrivVar && (wrSrc.m_fieldRefList.size() == 1 && wrSrc.m_fieldRefList[0].m_refAddrList.size() == 0 ||
					wrSrc.m_fieldRefList.size() > 1))
				{
					vector<CRefDimen> & refDimenList = wrSrc.m_fieldRefList.back().m_refDimenList;

					for (size_t i = 0; i < refDimenList.size(); i += 1) {
						varIdx += VA("_I%d", refDimenList[i].m_size);
						CHtString dimStr = refDimenList[i].m_size;
						dimenList.push_back(dimStr);

						if (refDimenList[i].m_value < 0 && refDimenList[i].m_size > 1)
							wrSrc.m_bConstDimenList = false;
					}
				}

				if (!wrSrc.m_bConstDimenList)
					wrSrc.m_wrDataTypeName += varIdx;
			}
		}

		mif.m_mifReqStgCnt = 0;

		if (mif.m_mifRd.m_bRdRspCallBack || mif.m_mifRd.m_bNeedRdRspInfoRam)
			mif.m_mifReqStgCnt = max(mif.m_mifReqStgCnt, 1);

		if (mif.m_mifRd.m_bMultiQwRdReq)
			mif.m_mifReqStgCnt = max(mif.m_mifReqStgCnt, 1);

		if (mif.m_mifWr.m_bBlockRamAccessReq || mif.m_mifWr.m_bDistRamAccessReq && mif.m_mifWr.m_bMultiQwWrReq)
			mif.m_mifReqStgCnt = max(mif.m_mifReqStgCnt, 2);
		else if (mif.m_mifWr.m_bDistRamAccessReq || mif.m_mifWr.m_bMultiQwWrReq)
			mif.m_mifReqStgCnt = max(mif.m_mifReqStgCnt, 1);

		mod.m_memPortList[0]->m_queueW = mif.m_queueW;
		mod.m_memPortList[0]->m_bMultiQwRdReq |= mif.m_mifRd.m_bMultiQwRdReq;
		mod.m_memPortList[0]->m_bMultiQwWrReq |= mif.m_mifWr.m_bMultiQwWrReq;

		if ((coprocInfo.GetMaxHostQwReadCnt() > 1 || coprocInfo.GetMaxCoprocQwReadCnt() > 1)
			&& mif.m_mifRd.m_bMultiQwRdReq)
		{
			if (mif.m_bMifRd && mif.m_mifRd.m_rspCntW.AsInt() < 4)
				ParseMsg(Error, mif.m_mifRd.m_lineInfo, "AddReadMem rspCntW parameter must be greater or equal to 4 for multi-cycle memory requests");
			if (mif.m_bMifRd && mif.m_mifRd.m_queueW.AsInt() < 4)
				ParseMsg(Error, mif.m_mifRd.m_lineInfo, "AddReadMem queueW parameter must be greater or equal to 4 for multi-cycle memory requests");
		} else if (mif.m_bMifRd && mif.m_mifRd.m_rspCntW.AsInt() < 3)
			ParseMsg(Error, mif.m_mifRd.m_lineInfo, "AddReadMem rspCntW parameter must be greater or equal to 3");

		if ((coprocInfo.GetMaxHostQwWriteCnt() > 1 || coprocInfo.GetMaxCoprocQwWriteCnt() > 1)
			&& mif.m_mifWr.m_bMultiQwWrReq) 
		{
			if (mif.m_bMifWr && mif.m_mifWr.m_rspCntW.AsInt() < 4)
				ParseMsg(Error, mif.m_mifWr.m_lineInfo, "AddWriteMem rspCntW parameter must be greater or equal to 4 for multi-cycle memory requests");
			if (mif.m_bMifWr && mif.m_mifWr.m_queueW.AsInt() < 4)
				ParseMsg(Error, mif.m_mifWr.m_lineInfo, "AddWriteMem queueW parameter must be greater or equal to 4 for multi-cycle memory requests");
		} else if (mif.m_bMifWr && mif.m_mifWr.m_rspCntW.AsInt() < 3)
			ParseMsg(Error, mif.m_mifWr.m_lineInfo, "AddWriteMem rspCntW parameter must be greater or equal to 3");
	}

	// fill unit wide mifInstList
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t modInstIdx = 0; modInstIdx < mod.m_modInstList.size(); modInstIdx += 1) {
			CModInst & modInst = mod.m_modInstList[modInstIdx];

			for (size_t memPortIdx = 0; memPortIdx < modInst.m_instParams.m_memPortList.size(); memPortIdx += 1) {
				int instMemPort = modInst.m_instParams.m_memPortList[memPortIdx];

				size_t mifInstIdx;
				for (mifInstIdx = 0; mifInstIdx < m_mifInstList.size(); mifInstIdx += 1) {
					if (m_mifInstList[mifInstIdx].m_mifId == instMemPort)
						break;
				}

				if (mifInstIdx < m_mifInstList.size()) {
					m_mifInstList[mifInstIdx].m_bMifWrite |= mod.m_memPortList[memPortIdx]->m_bWrite;
					m_mifInstList[mifInstIdx].m_bMifRead |= mod.m_memPortList[memPortIdx]->m_bRead;
					continue;
				}

				m_mifInstList.push_back(CMifInst(instMemPort, mod.m_memPortList[memPortIdx]->m_bRead, mod.m_memPortList[memPortIdx]->m_bWrite));
			}
		}
	}

	if (m_mifInstList.size() * g_appArgs.GetAeUnitCnt() > 16) {
		ParseMsg(Error, "required memory interface ports (%d) for AE is greater then number available (16)",
			(int)(m_mifInstList.size() * g_appArgs.GetAeUnitCnt()));
		ParseMsg(Info, "   required memory interface ports is product of Unit count (%d) and memory ports per Unit (%d)",
			(int)g_appArgs.GetAeUnitCnt(), (int)m_mifInstList.size());
	}
}

CField * CDsnInfo::FindField(CLineInfo & lineInfo, CRecord * pRecord, string & fieldName)
{
	vector<CField *> & fieldList = pRecord->m_fieldList;

	size_t fldIdx;
	for (fldIdx = 0; fldIdx < fieldList.size(); fldIdx += 1) {
		if (fieldList[fldIdx]->m_pType->IsRecord() && fieldList[fldIdx]->m_name == "") {

			CField * pField = FindField(lineInfo, fieldList[fldIdx]->m_pType->AsRecord(), fieldName);
			if (pField)
				return pField;

		} else if (fieldList[fldIdx]->m_name == fieldName)
			return fieldList[fldIdx];
	}

	return 0;
}

void CDsnInfo::ParseVarRefSpec(CLineInfo & lineInfo, string const & name,
	vector<CHtString> const & dimenList, vector<CVarAddr> const & varAddrList,
	string const & baseType, string & varRefSpec, vector<CFieldRef> & varRefList, CType * &pDstType)
{
	pDstType = 0;
	char const * pStr = varRefSpec.c_str();
	if (*pStr == '\0')
		return;

	string fldType = baseType;
	for (;;) {

		// parse next field name
		varRefList.push_back(CFieldRef());
		CFieldRef & fieldRef = varRefList.back();

		char const * pFldName = pStr;
		while (isalpha(*pStr) || isdigit(*pStr) || *pStr == '_') pStr += 1;
		fieldRef.m_fieldName = string(pFldName, pStr - pFldName);

		vector<CHtString> const * pDimenList;
		if (varRefList.size() == 1) {
			pDimenList = &dimenList;
		} else {
			// find field name in struct list
			CRecord * pStruct = FindRecord(fldType);

			CField * pField = FindField(lineInfo, pStruct, fieldRef.m_fieldName);

			if (pField == 0) {
				ParseMsg(Error, lineInfo, "field %s was not found", fieldRef.m_fieldName.c_str());
				return;
			}
			pDimenList = &pField->m_dimenList;

			fldType = pField->m_type;
		}

		size_t dimenCnt = 0;
		while (*pStr == ' ') pStr += 1;
		while (*pStr == '[') {
			pStr += 1; while (*pStr == ' ') pStr += 1;

			if (pDimenList->size() <= dimenCnt) {
				ParseMsg(Error, lineInfo, "field %s has %d dimensions", fieldRef.m_fieldName.c_str(), (int)pDimenList->size());
				return;
			}

			if (*pStr == ']') {
				fieldRef.m_refDimenList.push_back(CRefDimen(-1, (*pDimenList)[dimenCnt].AsInt()));

			} else {
				char * pEnd;
				int value = strtol(pStr, &pEnd, 10);
				if (pEnd == pStr) {
					ParseMsg(Error, lineInfo, "field %s has invalid dimension", fieldRef.m_fieldName.c_str());
					return;
				}
				pStr = pEnd; while (*pStr == ' ') pStr += 1;
				if (*pStr != ']') {
					ParseMsg(Error, lineInfo, "field %s has invalid dimension", fieldRef.m_fieldName.c_str());
					return;
				}
				if (value >= (*pDimenList)[dimenCnt].AsInt()) {
					ParseMsg(Error, lineInfo, "field %s has index (%d) out of range (0-%d)",
						fieldRef.m_fieldName.c_str(), value, (*pDimenList)[dimenCnt].AsInt()-1);
					return;
				}
				fieldRef.m_refDimenList.push_back(CRefDimen(value, (*pDimenList)[dimenCnt].AsInt()));
			}
			pStr += 1; while (isspace(*pStr)) pStr += 1;
			dimenCnt += 1;
		}

		if (pDimenList->size() != dimenCnt) {
			ParseMsg(Error, lineInfo, "variable/field %s has invalid dimension count, expected %d",
				fieldRef.m_fieldName.c_str(), (int)pDimenList->size());
			return;
		}

		if (varRefList.size() == 1 && *pStr == '(') {
			size_t addrCnt = 0;
			do {
				pStr += 1; while (isspace(*pStr)) pStr += 1;

				if (varAddrList.size() <= addrCnt) {
					ParseMsg(Error, lineInfo, "variable %s has %d required addresses", fieldRef.m_fieldName.c_str(), (int)varAddrList.size());
					return;
				}

				if (*pStr == ',' || *pStr == ')') {
					fieldRef.m_refAddrList.push_back(CRefAddr(-1, varAddrList[addrCnt].m_sizeW, varAddrList[addrCnt].m_isHtId));
				} else {
					char * pEnd;
					int value = strtol(pStr, &pEnd, 10);
					if (pEnd == pStr) {
						ParseMsg(Error, lineInfo, "field %s has invalid address value", fieldRef.m_fieldName.c_str());
						return;
					}
					pStr = pEnd; while (isspace(*pStr)) pStr += 1;
					if (*pStr != ',' && *pStr != ')') {
						ParseMsg(Error, lineInfo, "field %s has invalid address value", fieldRef.m_fieldName.c_str());
						return;
					}
					fieldRef.m_refAddrList.push_back(CRefAddr(value, varAddrList[addrCnt].m_sizeW, varAddrList[addrCnt].m_isHtId));
				}
				addrCnt += 1;
			} while (*pStr != ')');
			pStr += 1;
		}

		if (*pStr != '\0' && *pStr != '.')
			ParseMsg(Error, lineInfo, "unexpected char (%c) after field %s", *pStr, fieldRef.m_fieldName.c_str());

		if (*pStr == '.')
			pStr += 1;
		else
			break;
		while (*pStr == ' ') pStr += 1;
	}

	pDstType = FindType(fldType, lineInfo);
}

void CDsnInfo::GenModMifStatements(CModule &mod)
{
	if (!mod.m_mif.m_bMif)
		return;

	bool bNeedMifContQue = false;
	bool bNeedMifPollQue = false;
	bool bNeedMifRdRsmQue = false;
	bool bNeedMifWrRsmQue = false;

	g_appArgs.GetDsnRpt().AddLevel("Memory Interface\n");

	string vcdModName = VA("Pers%s", mod.m_modName.Uc().c_str());

	CMif &mif = mod.m_mif;

	m_mifIoDecl.Append("\t// Memory Interface\n");
	GenModDecl(eVcdUser, m_mifIoDecl, vcdModName, "sc_out<bool>", VA("o_%sP0ToMif_reqRdy", mod.m_modName.Lc().c_str()));
	GenModDecl(eVcdAll, m_mifIoDecl, vcdModName, "sc_out<CMemRdWrReqIntf>", VA("o_%sP0ToMif_req", mod.m_modName.Lc().c_str()));

	m_mifIoDecl.Append("\tsc_in<bool> i_mifTo%sP0_reqAvl;\n", mod.m_modName.Uc().c_str());
	m_mifIoDecl.Append("\n");

	if (mif.m_bMifRd) {
		GenModDecl(eVcdUser, m_mifIoDecl, vcdModName, "sc_in<bool>", VA("i_mifTo%sP0_rdRspRdy", mod.m_modName.Uc().c_str()));
		GenModDecl(eVcdUser, m_mifIoDecl, vcdModName, "sc_in<CMemRdRspIntf>", VA("i_mifTo%sP0_rdRsp", mod.m_modName.Uc().c_str()));
		GenModDecl(eVcdUser, m_mifIoDecl, vcdModName, "sc_out<bool>", VA("o_%sP0ToMif_rdRspFull", mod.m_modName.Lc().c_str()));
		m_mifIoDecl.Append("\n");
	}

	if (mif.m_bMifWr) {
		GenModDecl(eVcdUser, m_mifIoDecl, vcdModName, "sc_in<bool>", VA("i_mifTo%sP0_wrRspRdy", mod.m_modName.Uc().c_str()));
		if (mod.m_mif.m_mifWr.m_rspGrpW.AsInt() == 0)
			GenModDecl(eVcdAll, m_mifIoDecl, vcdModName, "sc_in<sc_uint<MIF_TID_W> > ht_noload", VA("i_mifTo%sP0_wrRspTid", mod.m_modName.Uc().c_str()));
		else
			GenModDecl(eVcdAll, m_mifIoDecl, vcdModName, "sc_in<sc_uint<MIF_TID_W> >", VA("i_mifTo%sP0_wrRspTid", mod.m_modName.Uc().c_str()));
		m_mifIoDecl.Append("\n");
	}

	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

	CHtCode	& mifPreInstr = mod.m_clkRate == eClk1x ? m_mifPreInstr1x : m_mifPreInstr2x;
	CHtCode	& mifPostInstr = mod.m_clkRate == eClk1x ? m_mifPostInstr1x : m_mifPostInstr2x;
	CHtCode	& mifReg = mod.m_clkRate == eClk1x ? m_mifReg1x : m_mifReg2x;
	CHtCode	& mifPostReg = mod.m_clkRate == eClk1x ? m_mifPostReg1x : m_mifPostReg2x;
	CHtCode	& mifOut = mod.m_clkRate == eClk1x ? m_mifOut1x : m_mifOut2x;
	CHtCode & mifReset = mod.m_clkRate == eClk1x ? m_mifReset1x : m_mifReset2x;

	CCoprocInfo const & coprocInfo = g_appArgs.GetCoprocInfo();

	bool bMultiQwReq = mif.m_mifRd.m_bMultiQwRdReq || mif.m_mifWr.m_bMultiQwWrReq;

	bool bMultiQwHostRdMif = mif.m_mifRd.m_bMultiQwHostRdMif;
	bool bMultiQwCoprocRdMif = mif.m_mifRd.m_bMultiQwCoprocRdMif;
	bool bMultiQwHostWrMif = mif.m_mifWr.m_bMultiQwHostWrMif;
	bool bMultiQwCoprocWrMif = mif.m_mifWr.m_bMultiQwCoprocWrMif;

	// instruction has multi QW requestm
	bool bMultiQwMif = bMultiQwHostRdMif || bMultiQwCoprocRdMif || bMultiQwHostWrMif || bMultiQwCoprocWrMif;

	// generate multi QW request to mif
	bool bMultiQwRdMif = bMultiQwHostRdMif || bMultiQwCoprocRdMif;

	// list of write types
	vector<string> wrDataTypeList;
	vector<CType *> wrSrcTypeList;

	int maxRdElemQwCnt = 1;

	bool bRdDstNonNgv = false;
	vector<CRam *> rdDstNgvRamList;
	int rdDstRdyCnt = 0;
	bool bNgvWrCompClk2x = false;

	bool bSingleMemSize = true;
	int modMemSize = 0;

	for (size_t rdDstIdx = 0; rdDstIdx < mif.m_mifRd.m_rdDstList.size(); rdDstIdx += 1) {
		CMifRdDst &rdDst = mif.m_mifRd.m_rdDstList[rdDstIdx];

		if (rdDst.m_infoW.size() > 0) {
			if (!bRdDstNonNgv)
				rdDstRdyCnt += 1;
			bRdDstNonNgv = true;
			continue;
		}

		int elemQwCnt = rdDst.m_pDstType->m_clangMinAlign == 1 ? 1 :
			((rdDst.m_pDstType->m_clangBitWidth + rdDst.m_pDstType->m_clangMinAlign - 1) / rdDst.m_pDstType->m_clangMinAlign);
		maxRdElemQwCnt = max(maxRdElemQwCnt, elemQwCnt);

		int elemCnt = 1;
		if (rdDst.m_fieldRefList.size() == 1 && rdDst.m_varAddr1W > 0) {
			elemCnt *= 1 << (rdDst.m_varAddr1W >= 0 ? rdDst.m_varAddr1W : 0);
			elemCnt *= 1 << (rdDst.m_varAddr2W >= 0 ? rdDst.m_varAddr2W : 0);
		} else if (rdDst.m_fieldRefList.back().m_refDimenList.size() > 0) {
			vector<CRefDimen> & refDimen = rdDst.m_fieldRefList.back().m_refDimenList;
			for (size_t i = 0; i < refDimen.size(); i += 1)
				elemCnt *= refDimen[i].m_size;
		}
		if (rdDst.m_elemCntW.size() > 0)
			elemCnt = min(elemCnt, 1 << rdDst.m_elemCntW.AsInt());

		assert(rdDst.m_maxElemCnt == elemCnt);

		// check for req size
		if (modMemSize == 0)
			modMemSize = rdDst.m_memSize;
		else if (modMemSize != rdDst.m_memSize)
			bSingleMemSize = false;

		if (rdDst.m_pGblVar == 0 || rdDst.m_pGblVar->m_pNgvInfo->m_bOgv) {
			if (!bRdDstNonNgv)
				rdDstRdyCnt += 1;
			bRdDstNonNgv = true;
			continue;
		}

		size_t i;
		for (i = 0; i < rdDstNgvRamList.size(); i += 1) {
			if (rdDstNgvRamList[i] == rdDst.m_pGblVar)
				break;
		}

		if (i == rdDstNgvRamList.size()) {
			rdDstNgvRamList.push_back(rdDst.m_pGblVar);
			rdDstRdyCnt += rdDst.m_pGblVar->m_elemCnt;
		}

		if (rdDst.m_pGblVar->m_pNgvInfo->m_bNgvWrCompClk2x)
			bNgvWrCompClk2x = true;
	}

	int maxWrElemQwCnt = 1;

	for (size_t wrSrcIdx = 0; wrSrcIdx < mif.m_mifWr.m_wrSrcList.size(); wrSrcIdx += 1) {
		CMifWrSrc &wrSrc = mif.m_mifWr.m_wrSrcList[wrSrcIdx];

		int elemQwCnt = wrSrc.m_pSrcType->m_clangMinAlign == 1 ? 1 : 
			((wrSrc.m_pSrcType->m_clangBitWidth + wrSrc.m_pSrcType->m_clangMinAlign - 1) / wrSrc.m_pSrcType->m_clangMinAlign);
		maxWrElemQwCnt = max(maxWrElemQwCnt, elemQwCnt);

		int elemCnt = 1;
		if (wrSrc.m_pType != 0) {
			;
		} else if (wrSrc.m_fieldRefList.size() == 1 && wrSrc.m_varAddr1W > 0) {
			elemCnt *= 1 << (wrSrc.m_varAddr1W >= 0 ? wrSrc.m_varAddr1W : 0);
			elemCnt *= 1 << (wrSrc.m_varAddr2W >= 0 ? wrSrc.m_varAddr2W : 0);
		} else if (wrSrc.m_fieldRefList.back().m_refDimenList.size() > 0) {
			vector<CRefDimen> & refDimen = wrSrc.m_fieldRefList.back().m_refDimenList;
			for (size_t i = 0; i < refDimen.size(); i += 1)
				elemCnt *= refDimen[i].m_size;
		}
		if (wrSrc.m_elemCntW.size() > 0)
			elemCnt = min(elemCnt, 1 << wrSrc.m_elemCntW.AsInt());

		assert(wrSrc.m_maxElemCnt == elemCnt);
	}
	int maxElemQwCnt = max(maxRdElemQwCnt, maxWrElemQwCnt);
	int maxQwCnt = mif.m_maxElemCnt * maxElemQwCnt;

	bool bNeedElemQwCntM1 = false;
	for (int idx = 0; idx < (int)mif.m_vIdxWList.size(); idx += 1) {
		if (mif.m_vIdxWList[idx] > 0)
			bNeedElemQwCntM1 = true;
	}

	bool bRdRspGrpIdIsHtId = mif.m_mifRd.m_rspGrpW.size() == 0;
	int rdRspGrpIdW = 0;

	if (MIF_CHK_STATE)
		m_mifDefines.Append("#define %s_CHK_GRP_ID 0\n", mod.m_modName.Upper().c_str());

	if (mif.m_bMifRd) {
		rdRspGrpIdW = mif.m_mifRd.m_rspGrpW.AsInt();

		m_mifDefines.Append("#define %s_RD_RSP_CNT_W %d\n",
			mod.m_modName.Upper().c_str(), mif.m_mifRd.m_rspCntW.AsInt());

		if (bRdRspGrpIdIsHtId) {
			m_mifDefines.Append("#define %s_RD_GRP_ID_W %s_HTID_W\n",
				mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());
		} else {
			m_mifDefines.Append("#define %s_RD_GRP_ID_W %d\n",
				mod.m_modName.Upper().c_str(), mif.m_mifRd.m_rspGrpW.AsInt());
		}
	}

	bool bWrRspGrpIdIsHtId = mif.m_mifWr.m_rspGrpW.size() == 0;
	int wrRspGrpIdW = 0;

	if (mif.m_bMifWr) {
		wrRspGrpIdW = mif.m_mifWr.m_rspGrpW.AsInt();

		m_mifDefines.Append("#define %s_WR_RSP_CNT_W %d\n",
			mod.m_modName.Upper().c_str(), mif.m_mifWr.m_rspCntW.AsInt());

		if (bWrRspGrpIdIsHtId) {
			m_mifDefines.Append("#define %s_WR_GRP_ID_W %s_HTID_W\n",
				mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());
		} else {
			m_mifDefines.Append("#define %s_WR_GRP_ID_W %d\n",
				mod.m_modName.Upper().c_str(), mif.m_mifWr.m_rspGrpW.AsInt());
		}
	}

	m_mifDefines.NewLine();

	string rdRspGrpName;
	if (mif.m_bMifRd) {
		m_mifMacros.Append("// Memory Read Interface Routines\n");

		if (mif.m_mifRd.m_rspGrpId.size() == 0)
			rdRspGrpName = VA("r_t%d_htId", mod.m_execStg);
		else if (mif.m_mifRd.m_bRspGrpIdPriv) {
			rdRspGrpName = VA("r_t%d_htPriv.m_%s",
				mod.m_execStg, mif.m_mifRd.m_rspGrpId.AsStr().c_str());
		} else
			rdRspGrpName = VA("r_%s", mif.m_mifRd.m_rspGrpId.AsStr().c_str());

		///////////////////////////////////////////////////////////
		// generate ReadMemBusy routine

		g_appArgs.GetDsnRpt().AddLevel("bool ReadMemBusy()\n");
		g_appArgs.GetDsnRpt().EndLevel();

		m_mifFuncDecl.Append("\tbool ReadMemBusy();\n");
		m_mifMacros.Append("bool CPers%s%s::ReadMemBusy()\n",
			unitNameUc.c_str(), mod.m_modName.Uc().c_str());
		m_mifMacros.Append("{\n");

		string rdMemBusy;
		if (bMultiQwReq)
			rdMemBusy = VA("r_t%d_memReqBusy", mod.m_execStg);
		else
			rdMemBusy = VA("r_rdGrpRspCntBusy || r_%sToMif_reqAvlCntBusy", mod.m_modName.Lc().c_str());

		mifPostReg.Append("#\tifdef _HTV\n");
		mifPostReg.Append("\tc_t%d_bReadMemBusy = %s;\n",
			mod.m_execStg, rdMemBusy.c_str());

		mifPostReg.Append("#\telse\n");

		mifPostReg.Append("\tc_t%d_bReadMemBusy = %s ||\n",
			mod.m_execStg, rdMemBusy.c_str());
		mifPostReg.Append("\t\t(%s_RND_RETRY && !!(g_rndRetry() & 1));\n", mod.m_modName.Upper().c_str());
		mifPostReg.Append("#\tendif\n");

		m_mifMacros.Append("\tc_t%d_bReadMemAvail = !c_t%d_bReadMemBusy;\n", mod.m_execStg, mod.m_execStg);
		m_mifMacros.Append("\treturn c_t%d_bReadMemBusy;\n", mod.m_execStg);
		m_mifMacros.Append("}\n");
		m_mifMacros.NewLine();

		///////////////////////////////////////////////////////////
		// generate ReadMem or ReadMem_var routines

		for (int rdDstIdx = 0; rdDstIdx < (int)mif.m_mifRd.m_rdDstList.size(); rdDstIdx += 1) {
			CMifRdDst &rdDst = mif.m_mifRd.m_rdDstList[rdDstIdx];

			char dstName[64] = "";
			if (rdDst.m_name.size() > 0)
				sprintf(dstName, "_%s", rdDst.m_name.c_str());

			if (true || g_appArgs.IsMemTraceEnabled()) {
				m_mifFuncDecl.Append("#\tifdef _HTV\n");
				m_mifMacros.Append("#ifdef _HTV\n");
			}

			char * pReadType = 0;
			char * pMemOpName = 0;
			if (rdDst.m_atomic == "read") {
				pMemOpName = "Read";
				pReadType = "MEM_REQ_RD";
			} else if (rdDst.m_atomic == "setBit63") {
				pMemOpName = "SetBit63";
				pReadType = "MEM_REQ_SET_BIT_63";
			} else
				HtlAssert(0);

			g_appArgs.GetDsnRpt().AddLevel("void %sMem%s(", pMemOpName, dstName);

			m_mifFuncDecl.Append("\tvoid %sMem%s(", pMemOpName, dstName);

			m_mifMacros.Append("void CPers%s%s::%sMem%s(",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(),
				pMemOpName, dstName);

			if (!bRdRspGrpIdIsHtId && rdRspGrpIdW > 0) {
				g_appArgs.GetDsnRpt().AddText("sc_uint<%s_RD_GRP_ID_W> rdGrpId, ", mod.m_modName.Upper().c_str());
				m_mifFuncDecl.Append("sc_uint<%s_RD_GRP_ID_W> rdGrpId, ", mod.m_modName.Upper().c_str());
				m_mifMacros.Append("sc_uint<%s_RD_GRP_ID_W> rdGrpId, ", mod.m_modName.Upper().c_str());
			}

			g_appArgs.GetDsnRpt().AddText("sc_uint<MEM_ADDR_W> memAddr");
			m_mifFuncDecl.Append("sc_uint<MEM_ADDR_W> memAddr");
			m_mifMacros.Append("sc_uint<MEM_ADDR_W> memAddr");

			if (rdDst.m_varAddr1W >= 0 && !rdDst.m_varAddr1IsHtId) {
				int W = max(1, rdDst.m_varAddr1W);
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varAddr1", W);
				m_mifFuncDecl.Append(", ht_uint%d varAddr1", W);
				m_mifMacros.Append(", ht_uint%d %svarAddr1", W, rdDst.m_varAddr1W == 0 ? "ht_noload " : "");
			}

			if (rdDst.m_varAddr2W >= 0 && !rdDst.m_varAddr2IsHtId) {
				int W = max(1, rdDst.m_varAddr2W);
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varAddr2", W);
				m_mifFuncDecl.Append(", ht_uint%d varAddr2", W);
				m_mifMacros.Append(", ht_uint%d %svarAddr2", W, rdDst.m_varAddr2W ? "ht_noload " : "");
			}

			for (size_t fldIdx = 0; fldIdx < rdDst.m_fieldRefList.size(); fldIdx += 1) {
				string fldName = fldIdx == 0 ? "var" : VA("fld%d", (int)fldIdx);
				for (size_t dimIdx = 0; dimIdx < rdDst.m_fieldRefList[fldIdx].m_refDimenList.size(); dimIdx += 1) {
					CRefDimen & refDimen = rdDst.m_fieldRefList[fldIdx].m_refDimenList[dimIdx];

					if (refDimen.m_value < 0) {
						int varDimenW = FindLg2(rdDst.m_fieldRefList[fldIdx].m_refDimenList[dimIdx].m_size - 1, true);
						g_appArgs.GetDsnRpt().AddText(", ht_uint%d %sIdx%d", varDimenW, fldName.c_str(), (int)dimIdx + 1);
						m_mifFuncDecl.Append(", ht_uint%d ht_noload %sIdx%d", varDimenW, fldName.c_str(), (int)dimIdx + 1);
						m_mifMacros.Append(", ht_uint%d ht_noload %sIdx%d", varDimenW, fldName.c_str(), (int)dimIdx + 1);
					}
				}
			}

			if (rdDst.m_infoW.AsInt() > 0) {
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d info", rdDst.m_infoW.AsInt());
				m_mifFuncDecl.Append(", ht_uint%d info", rdDst.m_infoW.AsInt());
				m_mifMacros.Append(", ht_uint%d info", rdDst.m_infoW.AsInt());
			}

			int rdDstSize = 1;
			bool bNeedElemParam = false;
			if (rdDst.m_fieldRefList.size() > 0) {
				for (size_t i = 0; i < rdDst.m_fieldRefList.back().m_refDimenList.size(); i += 1) {
					if (rdDst.m_fieldRefList.back().m_refDimenList[i].m_value < 0)
						rdDstSize *= rdDst.m_fieldRefList.back().m_refDimenList[i].m_size;
					bNeedElemParam = rdDst.m_fieldRefList.back().m_refDimenList[i].m_value < 0;
				}
			}

			bool bMultiElemDstParam = rdDst.m_infoW.size() == 0 && 
				(rdDst.m_elemCntW.size() == 0 || rdDst.m_elemCntW.AsInt() > 0) &&
				(rdDst.m_fieldRefList.size() == 1 && rdDst.m_varAddr1W > 0 || bNeedElemParam);

			bool bMultiElemDst = rdDst.m_infoW.size() == 0 && 
				(rdDst.m_elemCntW.size() == 0 || rdDst.m_elemCntW.AsInt() > 0) &&
				(rdDst.m_fieldRefList.size() == 1 && rdDst.m_varAddr1W > 0 ||
				rdDst.m_fieldRefList.back().m_refDimenList.size() > 0 && rdDstSize > 1);

			int elemCntW = 0;
			int elemCnt = 1;
			if (bMultiElemDstParam) {
				if (rdDst.m_varAddr1W > 0) {
					if (!rdDst.m_varAddr1IsHtId)
						elemCnt *= 1 << rdDst.m_varAddr1W;
					if (rdDst.m_varAddr2W > 0 && !rdDst.m_varAddr2IsHtId)
						elemCnt *= 1 << rdDst.m_varAddr2W;
				} else {
					elemCnt = rdDstSize;
				}
				elemCntW = FindLg2(elemCnt);

				if (rdDst.m_elemCntW.size() > 0 && rdDst.m_elemCntW.AsInt() < elemCntW)
					elemCntW = rdDst.m_elemCntW.AsInt();

				g_appArgs.GetDsnRpt().AddText(", ht_uint%d elemCnt", elemCntW);
				m_mifFuncDecl.Append(", ht_uint%d ht_noload elemCnt", elemCntW);
				m_mifMacros.Append(", ht_uint%d ht_noload elemCnt", elemCntW);
			}

			if (rdDst.m_memSrc.size() == 0 && bMultiElemDstParam) {
				g_appArgs.GetDsnRpt().AddText(", bool bHost = false");
				m_mifFuncDecl.Append(", bool ht_noload bHost = false");
				m_mifMacros.Append(", bool ht_noload bHost");
			}

			g_appArgs.GetDsnRpt().AddText(", bool orderChk = true)\n");
			m_mifFuncDecl.Append(", bool ht_noload orderChk = true);\n");
			m_mifMacros.Append(", bool ht_noload orderChk)\n");

			g_appArgs.GetDsnRpt().EndLevel();

			if (true || g_appArgs.IsMemTraceEnabled()) {
				m_mifFuncDecl.Append("#\telse\n");
				m_mifMacros.Append("#else\n");

				m_mifFuncDecl.Append("#\tdefine %sMem%s(", pMemOpName, dstName);
				m_mifFuncDecl.Append("...) %sMem%s_(", pMemOpName, dstName);
				m_mifFuncDecl.Append("(char *)__FILE__, __LINE__, __VA_ARGS__)\n");
				m_mifFuncDecl.Append("\tvoid %sMem%s_(char *file, int line, ",
					pMemOpName, dstName);

				m_mifMacros.Append("void CPers%s%s::%sMem%s_(char *file, int line, ",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(),
					pMemOpName, dstName);

				if (!bRdRspGrpIdIsHtId && rdRspGrpIdW > 0) {
					m_mifFuncDecl.Append("sc_uint<%s_RD_GRP_ID_W> rdGrpId, ", mod.m_modName.Upper().c_str());
					m_mifMacros.Append("sc_uint<%s_RD_GRP_ID_W> rdGrpId, ", mod.m_modName.Upper().c_str());
				}

				m_mifFuncDecl.Append("sc_uint<MEM_ADDR_W> memAddr");
				m_mifMacros.Append("sc_uint<MEM_ADDR_W> memAddr");

				if (rdDst.m_varAddr1W >= 0 && !rdDst.m_varAddr1IsHtId) {
					int W = max(1, rdDst.m_varAddr1W);
					m_mifFuncDecl.Append(", ht_uint%d varAddr1", W);
					m_mifMacros.Append(", ht_uint%d varAddr1", W);
				}

				if (rdDst.m_varAddr2W >= 0 && !rdDst.m_varAddr2IsHtId) {
					int W = max(1, rdDst.m_varAddr2W);
					m_mifFuncDecl.Append(", ht_uint%d varAddr2", W);
					m_mifMacros.Append(", ht_uint%d varAddr2", W);
				}

				for (size_t fldIdx = 0; fldIdx < rdDst.m_fieldRefList.size(); fldIdx += 1) {
					string fldName = fldIdx == 0 ? "var" : VA("fld%d", (int)fldIdx);
					for (size_t dimIdx = 0; dimIdx < rdDst.m_fieldRefList[fldIdx].m_refDimenList.size(); dimIdx += 1) {
						CRefDimen & refDimen = rdDst.m_fieldRefList[fldIdx].m_refDimenList[dimIdx];

						if (refDimen.m_value < 0) {
							int varDimenW = FindLg2(rdDst.m_fieldRefList[fldIdx].m_refDimenList[dimIdx].m_size - 1);
							m_mifFuncDecl.Append(", ht_uint%d ht_noload %sIdx%d", varDimenW, fldName.c_str(), (int)dimIdx + 1);
							m_mifMacros.Append(", ht_uint%d ht_noload %sIdx%d", varDimenW, fldName.c_str(), (int)dimIdx + 1);
						}
					}
				}

				if (rdDst.m_infoW.AsInt() > 0) {
					m_mifFuncDecl.Append(", ht_uint%d info", rdDst.m_infoW.AsInt());
					m_mifMacros.Append(", ht_uint%d info", rdDst.m_infoW.AsInt());
				}

				if (bMultiElemDstParam) {
					m_mifFuncDecl.Append(", ht_uint%d ht_noload elemCnt", elemCntW);
					m_mifMacros.Append(", ht_uint%d ht_noload elemCnt", elemCntW);
				}

				if (rdDst.m_memSrc.size() == 0 && bMultiElemDstParam) {
					m_mifFuncDecl.Append(", bool ht_noload bHost = false");
					m_mifMacros.Append(", bool ht_noload bHost");
				}

				m_mifFuncDecl.Append(", bool orderChk = true);\n");
				m_mifMacros.Append(", bool orderChk)\n");

				m_mifFuncDecl.Append("#\tendif\n");
				m_mifMacros.Append("#endif\n");
			}

			m_mifMacros.Append("{\n");

			m_mifMacros.Append("\tassert_msg(c_t%d_bReadMemAvail, \"Runtime check failed in CPers%s::%sMem%s(...) - ReadMemBusy was not called\\n\");\n",
				mod.m_execStg, mod.m_modName.Uc().c_str(), pMemOpName, dstName);

			if (rdRspGrpIdW == 0) {
				m_mifMacros.Append("\t//assert_msg(!c_rdGrpState.m_pause, \"Runtime check failed in CPers%s::%sMem%s(...) - expected ReadMem_%s() to be called before ReadMemPause()\\n\");\n",
					mod.m_modName.Uc().c_str(), pMemOpName, dstName, dstName);
			} else if (rdRspGrpIdW <= 2) {
				m_mifMacros.Append("\t//assert_msg(!c_rdGrpState[%s].m_pause, \"Runtime check failed in CPers%s::%sMem%s(...) - expected ReadMem_%s() to be called before ReadMemPause()\\n\");\n",
					rdRspGrpName.c_str(), mod.m_modName.Uc().c_str(), pMemOpName, dstName, dstName);
			} else {
				;
			}

			for (size_t fldIdx = 0; fldIdx < rdDst.m_fieldRefList.size(); fldIdx += 1) {
				string fldName = fldIdx == 0 ? "var" : VA("fld%d", (int)fldIdx);
				for (size_t dimIdx = 0; dimIdx < rdDst.m_fieldRefList[fldIdx].m_refDimenList.size(); dimIdx += 1) {
					CRefDimen & refDimen = rdDst.m_fieldRefList[fldIdx].m_refDimenList[dimIdx];

					if (refDimen.m_value < 0 && (refDimen.m_size == 1 || !IsPowerTwo(refDimen.m_size))) {

						m_mifMacros.Append("\tassert_msg(%sIdx%d < %d, \"Runtime check failed in CPers%s%s::%sMem%s(...) - %sIdx%d (%%d) out of range (0-%d)\\n\", (uint32_t)%sIdx%d);\n",
							fldName.c_str(), (int)dimIdx + 1,
							refDimen.m_size,
							unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMemOpName, dstName,
							fldName.c_str(), (int)dimIdx + 1,
							refDimen.m_size - 1,
							fldName.c_str(), (int)dimIdx + 1);
					}
				}
			}

			switch (rdDst.m_memSize) {
			case 8:
				break;
			case 16:
				m_mifMacros.Append("\tassert_msg((memAddr & 1) == 0, \"Runtime check failed in CPers%s%s::%sMem%s(...) - memAddr must be uint16 aligned\\n\");\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMemOpName, dstName);
				break;
			case 32:
				m_mifMacros.Append("\tassert_msg((memAddr & 3) == 0, \"Runtime check failed in CPers%s%s::%sMem%s(...) - memAddr must be uint32 aligned\\n\");\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMemOpName, dstName);
				break;
			case 64:
				m_mifMacros.Append("\tassert_msg((memAddr & 7) == 0, \"Runtime check failed in CPers%s%s::%sMem%s(...) - memAddr must be uint64 aligned\\n\");\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMemOpName, dstName);
				break;
			default:
				HtlAssert(0);
			}

			for (size_t fldIdx = 0; fldIdx < rdDst.m_fieldRefList.size(); fldIdx += 1) {
				string fldName = fldIdx == 0 ? "var" : VA("fld%d", (int)fldIdx);
				for (size_t dimIdx = 0; dimIdx < rdDst.m_fieldRefList[fldIdx].m_refDimenList.size(); dimIdx += 1) {
					CRefDimen & refDimen = rdDst.m_fieldRefList[fldIdx].m_refDimenList[dimIdx];

					if (refDimen.m_value < 0 && (refDimen.m_size == 1 || !IsPowerTwo(refDimen.m_size))) {

						m_mifMacros.Append("\tassert_msg(%sIdx%d < %d, \"Runtime check failed in CPers%s%s::%sMem%s(...) - %sIdx%d (%%d) out of range (0-%d)\\n\", (uint32_t)%sIdx%d);\n",
							fldName.c_str(), (int)dimIdx + 1,
							refDimen.m_size,
							unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMemOpName, dstName,
							fldName.c_str(), (int)dimIdx + 1,
							refDimen.m_size - 1,
							fldName.c_str(), (int)dimIdx + 1);
					}
				}
			}

			if (bMultiElemDstParam) {
				m_mifMacros.Append("\tassert_msg(");
				
				if (rdDst.m_varAddr1W > 0 && rdDst.m_fieldRefList.size() == 1) {
					if (!rdDst.m_varAddr1IsHtId) {
						m_mifMacros.Append("varAddr1");
						if (rdDst.m_varAddr2W > 0)
							m_mifMacros.Append(" * %d + ", 1 << rdDst.m_varAddr2W);
					}
					if (rdDst.m_varAddr2W > 0 && !rdDst.m_varAddr2IsHtId)
						m_mifMacros.Append("varAddr2");
				} else {
					bool bNeedMul = false;
					size_t fldIdx = rdDst.m_fieldRefList.size() - 1;
					for (size_t dimIdx = 0; dimIdx < rdDst.m_fieldRefList.back().m_refDimenList.size(); dimIdx += 1) {
						string fldName = fldIdx == 0 ? "var" : VA("fld%d", (int)fldIdx);
						CRefDimen & refDimen = rdDst.m_fieldRefList.back().m_refDimenList[dimIdx];
						if (bNeedMul) {
							m_mifMacros.Append(" * %d + ", refDimen.m_size);
						}
						if (refDimen.m_value < 0) {
							m_mifMacros.Append("%sIdx%d", fldName.c_str(), (int)dimIdx + 1);
							bNeedMul = true;
						}
					}
				}
				m_mifMacros.Append(" + elemCnt <= %d, ", rdDst.m_maxElemCnt);
				m_mifMacros.Append(" \"Runtime check failed in CPers%s%s::%sMem%s(...) - elemCnt range check failed\\n\");\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMemOpName, dstName);
			}
			m_mifMacros.NewLine();

			int qwCnt = rdDst.m_pDstType->m_clangMinAlign == 1 ? 1 : 
				((rdDst.m_pDstType->m_clangBitWidth + rdDst.m_pDstType->m_clangMinAlign - 1) / rdDst.m_pDstType->m_clangMinAlign);

			if (mif.m_mifReqStgCnt > 0)
				m_mifMacros.Append("\tc_t%d_memReq.m_valid = %s;\n", mod.m_execStg, bMultiElemDstParam ? "elemCnt > 0" : "true");

			m_mifMacros.Append("\tc_t%d_memReq.m_rdReq = %s;\n", mod.m_execStg, bMultiElemDstParam ? "elemCnt > 0" : "true");

			if (bMultiQwReq) {
				if (bMultiElemDstParam)
					m_mifMacros.Append("\tc_t%d_memReq.m_qwRem = (ht_uint%d)(%d * elemCnt);\n", mod.m_execStg, FindLg2(maxQwCnt), qwCnt);
				else
					m_mifMacros.Append("\tc_t%d_memReq.m_qwRem = %d;\n", mod.m_execStg, qwCnt);

				if (bMultiQwMif)
					m_mifMacros.Append("\tc_t%d_memReq.m_reqQwRem = 0;\n", mod.m_execStg);

				bool bNeedCntM1 = false;
				if (rdDst.m_varAddr1W > 0) {
					if (bMultiElemDst && rdDst.m_fieldRefList.size() == 1) {
						if (!rdDst.m_varAddr1IsHtId) {
							m_mifMacros.Append("\tc_t%d_memReq.m_vIdx1 = varAddr1;\n",
								mod.m_execStg);
						}
						rdDst.m_varAddr1IsIdx = true;
						bNeedCntM1 = !rdDst.m_varAddr1IsHtId;
					}
				}

				if (rdDst.m_varAddr2W > 0) {
					if (bMultiElemDst && rdDst.m_fieldRefList.size() == 1) {
						if (!rdDst.m_varAddr2IsHtId) {
							m_mifMacros.Append("\tc_t%d_memReq.m_vIdx2 = varAddr2;\n",
								mod.m_execStg);
							if (bNeedCntM1)
								m_mifMacros.Append("\tc_t%d_memReq.m_vIdx2CntM1 = %d;\n", mod.m_execStg, (1 << rdDst.m_varAddr2W) - 1);
						}
						rdDst.m_varAddr2IsIdx = true;
						bNeedCntM1 |= !rdDst.m_varAddr2IsHtId;
					}
				}

				if (rdDst.m_varAddr1W <= 0 || rdDst.m_fieldRefList.size() > 1) {
					for (size_t fldIdx = 0; fldIdx < rdDst.m_fieldRefList.size(); fldIdx += 1) {
						string fldName = fldIdx == 0 ? "var" : VA("fld%d", (int)fldIdx);
						vector<CRefDimen> & indexRangeList = rdDst.m_fieldRefList[fldIdx].m_refDimenList;
						for (size_t dimIdx = 0; dimIdx < indexRangeList.size(); dimIdx += 1) {
							if (bMultiElemDst && fldIdx + 1 == rdDst.m_fieldRefList.size()) {
								if (mif.m_vIdxWList.size() > dimIdx && mif.m_vIdxWList[dimIdx] > 0 &&
									rdDst.m_fieldRefList[fldIdx].m_refDimenList[dimIdx].m_value < 0)
								{
									m_mifMacros.Append("\tc_t%d_memReq.m_vIdx%d = %sIdx%d;\n",
										mod.m_execStg, dimIdx + 1, fldName.c_str(), (int)dimIdx + 1);
									rdDst.m_fieldRefList[fldIdx].m_refDimenList[dimIdx].m_isIdx = true;
									if (bNeedCntM1) {
										m_mifMacros.Append("\tc_t%d_memReq.m_vIdx%dCntM1 = %d;\n",
											mod.m_execStg, dimIdx + 1, rdDst.m_fieldRefList[fldIdx].m_refDimenList[dimIdx].m_size - 1);
									}
									bNeedCntM1 = true;
								}
							}
						}
					}
				}

				if (maxQwCnt > 1) {
					if (maxElemQwCnt > 1) {
						m_mifMacros.Append("\tc_t%d_memReq.m_elemQwIdx = 0;\n", mod.m_execStg);
						if (bNeedElemQwCntM1)
							m_mifMacros.Append("\tc_t%d_memReq.m_elemQwCntM1 = %d;\n", mod.m_execStg, qwCnt - 1);
					}
				}

				m_mifMacros.NewLine();
			}

			if (rdRspGrpIdW == 0) {
				// rspGrpId == 0
			} else if (bRdRspGrpIdIsHtId) {
				m_mifMacros.Append("\tc_t%d_memReq.m_rdGrpId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tc_t%d_rdRspInfo.m_grpId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);
			} else {
				m_mifMacros.Append("\tc_t%d_memReq.m_rdGrpId = rdGrpId;\n", mod.m_execStg);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tc_t%d_rdRspInfo.m_grpId = rdGrpId;\n", mod.m_execStg);
			}

			if (mif.m_mifRd.m_rdDstList.size() > 1)
				m_mifMacros.Append("\tc_t%d_rdRspInfo.m_dst = %d;\n", mod.m_execStg, rdDstIdx);

			if (rdDst.m_infoW.AsInt() > 0)
				m_mifMacros.Append("\tc_t%d_rdRspInfo.m_d%d_info = info;\n", mod.m_execStg, rdDstIdx);

			if (rdDst.m_varAddr1W > 0 && !rdDst.m_varAddr1IsIdx && !rdDst.m_varAddr1IsHtId) {
				m_mifMacros.Append("\tc_t%d_rdRspInfo.m_d%d_f1Addr1 = varAddr1;\n", mod.m_execStg, rdDstIdx);
			}

			if (rdDst.m_varAddr2W > 0 && !rdDst.m_varAddr2IsIdx && !rdDst.m_varAddr2IsHtId) {
				m_mifMacros.Append("\tc_t%d_rdRspInfo.m_d%d_f1Addr2 = varAddr2;\n", mod.m_execStg, rdDstIdx);
			}

			for (size_t fldIdx = 0; fldIdx < rdDst.m_fieldRefList.size(); fldIdx += 1) {
				string fldName = fldIdx == 0 ? "var" : VA("fld%d", (int)fldIdx);
				for (size_t dimIdx = 0; dimIdx < rdDst.m_fieldRefList[fldIdx].m_refDimenList.size(); dimIdx += 1) {
					CRefDimen & refDimen = rdDst.m_fieldRefList[fldIdx].m_refDimenList[dimIdx];

					if (refDimen.m_value < 0 && refDimen.m_size > 1 && !refDimen.m_isIdx) {
						m_mifMacros.Append("\tc_t%d_rdRspInfo.m_d%d_f%dIdx%d = %sIdx%d;\n",
							mod.m_execStg, rdDstIdx, (int)fldIdx + 1, dimIdx + 1, fldName.c_str(), dimIdx + 1);
					}
				}
			}

			m_mifMacros.NewLine();

			if (mif.m_mifReqStgCnt == 0) {
				m_mifMacros.Append("\tc_t%d_%sToMif_reqRdy = true;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
			}

			if (rdDst.m_bMultiQwRdReq) {
				if (rdDst.m_memSrc.size() == 0 && bMultiElemDst)
					m_mifMacros.Append("\tc_t%d_%sToMif_req.m_host = bHost;\n", mod.m_execStg, mod.m_modName.Lc().c_str());
				else {
					m_mifMacros.Append("\tc_t%d_%sToMif_req.m_host = %s;\n", mod.m_execStg, mod.m_modName.Lc().c_str(),
						rdDst.m_memSrc == "host" ? "true" : "false");
				}
			}

			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_type = %s;\n", mod.m_execStg, mod.m_modName.Lc().c_str(), pReadType);

			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_addr = memAddr;\n", mod.m_execStg, mod.m_modName.Lc().c_str());

			char const * pReqSize = "";
			switch (rdDst.m_memSize) {
			case 8: pReqSize = "MEM_REQ_U8"; break;
			case 16: pReqSize = "MEM_REQ_U16"; break;
			case 32: pReqSize = "MEM_REQ_U32"; break;
			case 64: pReqSize = "MEM_REQ_U64"; break;
			default: HtlAssert(0);
			}

			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_size = %s;\n", mod.m_execStg, mod.m_modName.Lc().c_str(), pReqSize);

			if (mif.m_mifReqStgCnt > 0 || mif.m_mifRd.m_maxRdRspInfoW == 0)
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_tid = 0;\n", mod.m_execStg, mod.m_modName.Lc().c_str());
			else {
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_tid = c_t%d_rdRspInfo.m_uint32;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str(),
					mod.m_execStg);
			}

			if (true || g_appArgs.IsMemTraceEnabled()) {
				m_mifMacros.Append("#\tifndef _HTV\n");
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_file = file;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_line = line;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_time = sc_time_stamp().value();\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_orderChk = orderChk;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				m_mifMacros.Append("#\tendif\n");
			}

			m_mifMacros.Append("}\n");
			m_mifMacros.NewLine();
		}

		///////////////////////////////////////////////////////////
		// generate ReadMemPause routine

		if (mif.m_mifRd.m_bPause) {

			g_appArgs.GetDsnRpt().AddLevel("void ReadMemPause(");

			m_mifFuncDecl.Append("\tvoid ReadMemPause(");
			m_mifMacros.Append("void CPers%s%s::ReadMemPause(",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str());

			if (!bRdRspGrpIdIsHtId && rdRspGrpIdW > 0) {
				g_appArgs.GetDsnRpt().AddText("sc_uint<%s_RD_GRP_ID_W> rdGrpId, ", mod.m_modName.Upper().c_str());
				m_mifFuncDecl.Append("sc_uint<%s_RD_GRP_ID_W> rdGrpId, ", mod.m_modName.Upper().c_str());
				m_mifMacros.Append("sc_uint<%s_RD_GRP_ID_W> rdGrpId, ", mod.m_modName.Upper().c_str());
			}

			g_appArgs.GetDsnRpt().AddText("ht_uint%d rsmInstr)\n", mod.m_instrW);
			m_mifFuncDecl.Append("ht_uint%d rsmInstr);\n", mod.m_instrW);
			m_mifMacros.Append("ht_uint%d rsmInstr)\n", mod.m_instrW);

			g_appArgs.GetDsnRpt().EndLevel();
			m_mifMacros.Append("{\n");

			m_mifMacros.Append("\tassert_msg(c_t%d_bReadMemAvail, \"Runtime check failed in CPers%s::ReadMemPause()"
				" - expected ReadMemBusy() to have been called and not busy\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());

			m_mifMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::ReadMemPause()"
				" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());
			m_mifMacros.NewLine();

			string rdGrpId;
			if (rdRspGrpIdW == 0) {
				// rspGrpId == 0
			} else if (bRdRspGrpIdIsHtId)
				rdGrpId = VA("r_t%d_htId", mod.m_execStg);
			else
				rdGrpId = "rdGrpId";

			if (rdRspGrpIdW == 0) {

				m_mifMacros.Append("\tc_t%d_memReq.m_valid = true;\n", mod.m_execStg);
				m_mifMacros.Append("\tc_t%d_memReq.m_rdPause = true;\n", mod.m_execStg);
				m_mifMacros.Append("\tc_t%d_memReq.m_rsmInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.NewLine();

				if (mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\tc_t%d_memReq.m_rsmHtId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);

				m_mifMacros.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);

			} else if (rdRspGrpIdW <= 2) {

				m_mifMacros.Append("\tc_t%d_rdRspInfo.m_grpId = %s;\n",
					mod.m_execStg, rdGrpId.c_str());
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tc_t%d_memReq.m_valid = true;\n", mod.m_execStg);
				m_mifMacros.Append("\tc_t%d_memReq.m_rdPause = true;\n", mod.m_execStg);
				if (mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\tc_t%d_memReq.m_rsmHtId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);
				if (!bRdRspGrpIdIsHtId)
					m_mifMacros.Append("\tc_t%d_memReq.m_rdGrpId = %s;\n", mod.m_execStg, rdGrpId.c_str());

				m_mifMacros.Append("\tc_t%d_memReq.m_rsmInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);

			} else {

				m_mifMacros.Append("\tc_t%d_rdRspInfo.m_grpId = %s;\n",
					mod.m_execStg, rdGrpId.c_str());
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tc_t%d_memReq.m_valid = true;\n", mod.m_execStg);
				m_mifMacros.Append("\tc_t%d_memReq.m_rdPause = true;\n", mod.m_execStg);

				if (mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\tc_t%d_memReq.m_rsmHtId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);
				if (!bRdRspGrpIdIsHtId)
					m_mifMacros.Append("\tc_t%d_memReq.m_rdGrpId = %s;\n", mod.m_execStg, rdGrpId.c_str());
				m_mifMacros.Append("\tc_t%d_memReq.m_rsmInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
			}

			m_mifMacros.Append("}\n");
			m_mifMacros.NewLine();
		}

		///////////////////////////////////////////////////////////
		// generate ReadMemPoll routine

		if (mif.m_mifRd.m_bPoll) {

			g_appArgs.GetDsnRpt().AddLevel("void ReadMemPoll(");

			m_mifFuncDecl.Append("\tvoid ReadMemPoll(");
			m_mifMacros.Append("void CPers%s%s::ReadMemPoll(",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str());

			if (!bRdRspGrpIdIsHtId && rdRspGrpIdW > 0) {
				g_appArgs.GetDsnRpt().AddText("sc_uint<%s_RD_GRP_ID_W> rdGrpId, ", mod.m_modName.Upper().c_str());
				m_mifFuncDecl.Append("sc_uint<%s_RD_GRP_ID_W> rdGrpId, ", mod.m_modName.Upper().c_str());
				m_mifMacros.Append("sc_uint<%s_RD_GRP_ID_W> rdGrpId, ", mod.m_modName.Upper().c_str());
			}

			g_appArgs.GetDsnRpt().AddText("ht_uint%d rsmInstr)\n", mod.m_instrW);
			m_mifFuncDecl.Append("ht_uint%d rsmInstr);\n", mod.m_instrW);
			m_mifMacros.Append("ht_uint%d rsmInstr)\n", mod.m_instrW);

			g_appArgs.GetDsnRpt().EndLevel();
			m_mifMacros.Append("{\n");

			m_mifMacros.Append("\tassert_msg(c_t%d_bReadMemAvail, \"Runtime check failed in CPers%s::ReadMemPoll()"
				" - expected ReadMemBusy() to have been called and not busy\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());

			m_mifMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::ReadMemPoll()"
				" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());
			m_mifMacros.NewLine();

			string rdGrpId;
			if (rdRspGrpIdW == 0) {
				// rspGrpId == 0
			} else if (bRdRspGrpIdIsHtId)
				rdGrpId = VA("r_t%d_htId", mod.m_execStg);
			else
				rdGrpId = "rdGrpId";

			if (rdRspGrpIdW == 0) {

				m_mifMacros.Append("\tc_t%d_memReq.m_rdPoll = true;\n", mod.m_execStg);
				m_mifMacros.Append("\tc_t%d_memReq.m_rsmInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.Append("\tc_t%d_memReq.m_pollInstr = r_t%d_htInstr;\n", mod.m_execStg, mod.m_execStg);
				m_mifMacros.NewLine();

				if (mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\tc_t%d_memReq.m_rsmHtId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);

				m_mifMacros.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);

			} else if (rdRspGrpIdW <= 2) {

				m_mifMacros.Append("\tc_t%d_rdRspInfo.m_grpId = %s;\n",
					mod.m_execStg, rdGrpId.c_str());
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tc_t%d_memReq.m_rdPoll = true;\n", mod.m_execStg);
				if (mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\tc_t%d_memReq.m_rsmHtId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);
				if (!bRdRspGrpIdIsHtId)
					m_mifMacros.Append("\tc_t%d_memReq.m_rdGrpId = %s;\n", mod.m_execStg, rdGrpId.c_str());

				m_mifMacros.Append("\tc_t%d_memReq.m_rsmInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.Append("\tc_t%d_memReq.m_pollInstr = r_t%d_htInstr;\n", mod.m_execStg, mod.m_execStg);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);

			} else {

				m_mifMacros.Append("\tc_t%d_rdRspInfo.m_grpId = %s;\n",
					mod.m_execStg, rdGrpId.c_str());
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tc_t%d_memReq.m_rdPoll = true;\n", mod.m_execStg);
				if (mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\tc_t%d_memReq.m_rsmHtId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);
				if (!bRdRspGrpIdIsHtId)
					m_mifMacros.Append("\tc_t%d_memReq.m_rdGrpId = %s;\n", mod.m_execStg, rdGrpId.c_str());
				m_mifMacros.Append("\tc_t%d_memReq.m_rsmInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.Append("\tc_t%d_memReq.m_pollInstr = r_t%d_htInstr;\n", mod.m_execStg, mod.m_execStg);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
			}

			m_mifMacros.Append("}\n");
			m_mifMacros.NewLine();
		}
	}

	string wrGrpId;
	if (mif.m_bMifWr) {
		m_mifMacros.Append("// Memory Write Interface Routines\n");

		if (wrRspGrpIdW == 0) {
			// rspGrpId == 0
		} else if (bWrRspGrpIdIsHtId)
			wrGrpId = VA("r_t%d_htId", mod.m_execStg);
		else
			wrGrpId = "wrGrpId";

		///////////////////////////////////////////////////////////
		// generate WriteMemBusy routine

		g_appArgs.GetDsnRpt().AddLevel("bool WriteMemBusy()\n");
		g_appArgs.GetDsnRpt().EndLevel();

		m_mifFuncDecl.Append("\tbool WriteMemBusy();\n");
		m_mifMacros.Append("bool CPers%s%s::WriteMemBusy()\n",
			unitNameUc.c_str(), mod.m_modName.Uc().c_str());
		m_mifMacros.Append("{\n");

		string wrMemBusy;
		if (bMultiQwReq)
			wrMemBusy = VA("r_t%d_memReqBusy", mod.m_execStg);
		else {
			wrMemBusy = VA("r_wrGrpRspCntBusy || r_%sToMif_reqAvlCntBusy", mod.m_modName.Lc().c_str());
		}

		mifPostReg.Append("#\tifdef _HTV\n");
		mifPostReg.Append("\tc_t%d_bWriteMemBusy = %s;\n",
			mod.m_execStg, wrMemBusy.c_str());

		mifPostReg.Append("#\telse\n");

		mifPostReg.Append("\tc_t%d_bWriteMemBusy = %s ||\n",
			mod.m_execStg, wrMemBusy.c_str());
		mifPostReg.Append("\t\t(%s_RND_RETRY && !!(g_rndRetry() & 1));\n", mod.m_modName.Upper().c_str());
		mifPostReg.Append("#\tendif\n");

		m_mifMacros.Append("\tc_t%d_bWriteMemAvail = !c_t%d_bWriteMemBusy;\n", mod.m_execStg, mod.m_execStg);
		m_mifMacros.Append("\treturn c_t%d_bWriteMemBusy;\n", mod.m_execStg);
		m_mifMacros.Append("}\n");
		m_mifMacros.NewLine();

		///////////////////////////////////////////////////////////
		// generate WriteMemPoll routine

		if (mif.m_mifWr.m_bPoll) {

			g_appArgs.GetDsnRpt().AddLevel("void WriteMemPoll(");

			m_mifFuncDecl.Append("\tvoid WriteMemPoll(");
			m_mifMacros.Append("void CPers%s%s::WriteMemPoll(",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str());

			if (!bWrRspGrpIdIsHtId && wrRspGrpIdW > 0) {
				g_appArgs.GetDsnRpt().AddText("sc_uint<%s_WR_GRP_ID_W> wrGrpId, ", mod.m_modName.Upper().c_str());
				m_mifFuncDecl.Append("sc_uint<%s_WR_GRP_ID_W> wrGrpId, ", mod.m_modName.Upper().c_str());
				m_mifMacros.Append("sc_uint<%s_WR_GRP_ID_W> wrGrpId, ", mod.m_modName.Upper().c_str());
			}

			g_appArgs.GetDsnRpt().AddText("ht_uint%d rsmInstr)\n", mod.m_instrW);
			m_mifFuncDecl.Append("ht_uint%d rsmInstr);\n", mod.m_instrW);
			m_mifMacros.Append("ht_uint%d rsmInstr)\n", mod.m_instrW);

			g_appArgs.GetDsnRpt().EndLevel();
			m_mifMacros.Append("{\n");

			m_mifMacros.Append("\tassert_msg(c_t%d_bWriteMemAvail, \"Runtime check failed in CPers%s::WriteMemPoll()"
				" - expected WriteMemBusy() to have been called and not busy\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());

			m_mifMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::WriteMemPoll()"
				" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());
			m_mifMacros.NewLine();

			string wrGrpId;
			if (wrRspGrpIdW == 0) {
				// rspGrpId == 0
			} else if (bWrRspGrpIdIsHtId)
				wrGrpId = VA("r_t%d_htId", mod.m_execStg);
			else
				wrGrpId = "wrGrpId";

			if (wrRspGrpIdW == 0) {

				m_mifMacros.Append("\tc_t%d_memReq.m_wrPoll = true;\n", mod.m_execStg);
				m_mifMacros.Append("\tc_t%d_memReq.m_rsmInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.Append("\tc_t%d_memReq.m_pollInstr = r_t%d_htInstr;\n", mod.m_execStg, mod.m_execStg);
				m_mifMacros.NewLine();

				if (mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\tc_t%d_memReq.m_rsmHtId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);

				m_mifMacros.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);

			} else if (wrRspGrpIdW <= 2) {

				m_mifMacros.Append("\tc_t%d_memReq.m_wrPoll = true;\n", mod.m_execStg);
				if (mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\tc_t%d_memReq.m_rsmHtId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);
				if (!bWrRspGrpIdIsHtId)
					m_mifMacros.Append("\tc_t%d_memReq.m_wrGrpId = %s;\n", mod.m_execStg, wrGrpId.c_str());

				m_mifMacros.Append("\tc_t%d_memReq.m_rsmInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.Append("\tc_t%d_memReq.m_pollInstr = r_t%d_htInstr;\n", mod.m_execStg, mod.m_execStg);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);

			} else {

				m_mifMacros.Append("\tc_t%d_memReq.m_wrPoll = true;\n", mod.m_execStg);
				if (mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\tc_t%d_memReq.m_rsmHtId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);
				if (!bWrRspGrpIdIsHtId)
					m_mifMacros.Append("\tc_t%d_memReq.m_wrGrpId = %s;\n", mod.m_execStg, wrGrpId.c_str());
				m_mifMacros.Append("\tc_t%d_memReq.m_rsmInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.Append("\tc_t%d_memReq.m_pollInstr = r_t%d_htInstr;\n", mod.m_execStg, mod.m_execStg);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
			}

			m_mifMacros.Append("}\n");
			m_mifMacros.NewLine();
		}

		///////////////////////////////////////////////////////////
		// generate WriteMem_var routines

		string wrGrpIdDecl;
		if (!bWrRspGrpIdIsHtId && wrRspGrpIdW > 0)
			wrGrpIdDecl = VA("ht_uint%d wrGrpId, ", mif.m_mifWr.m_rspGrpW.AsInt());

		for (size_t wrSrcIdx = 0; wrSrcIdx < mif.m_mifWr.m_wrSrcList.size(); wrSrcIdx += 1) {
			CMifWrSrc &wrSrc = mif.m_mifWr.m_wrSrcList[wrSrcIdx];

			char srcName[64] = "";
			if (wrSrc.m_name.size() > 0)
				sprintf(srcName, "_%s", wrSrc.m_name.c_str());

			if (true || g_appArgs.IsMemTraceEnabled()) {
				m_mifFuncDecl.Append("#\tifdef _HTV\n");
				m_mifMacros.Append("#ifdef _HTV\n");
			}

			g_appArgs.GetDsnRpt().AddLevel("void WriteMem%s(",
				srcName);

			m_mifFuncDecl.Append("\tvoid WriteMem%s(",
				srcName);

			m_mifMacros.Append("void CPers%s%s::WriteMem%s(",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(),
				srcName);

			if (!bWrRspGrpIdIsHtId && wrRspGrpIdW > 0) {
				g_appArgs.GetDsnRpt().AddText("sc_uint<%s_WR_GRP_ID_W> wrGrpId, ", mod.m_modName.Upper().c_str());
				m_mifFuncDecl.Append("sc_uint<%s_WR_GRP_ID_W> wrGrpId, ", mod.m_modName.Upper().c_str());
				m_mifMacros.Append("sc_uint<%s_WR_GRP_ID_W> wrGrpId, ", mod.m_modName.Upper().c_str());
			}

			g_appArgs.GetDsnRpt().AddText("sc_uint<MEM_ADDR_W> memAddr");
			m_mifFuncDecl.Append("sc_uint<MEM_ADDR_W> memAddr");
			m_mifMacros.Append("sc_uint<MEM_ADDR_W> memAddr");

			if (wrSrc.m_pType != 0) {
				g_appArgs.GetDsnRpt().AddText(", %s data", wrSrc.m_pType->m_typeName.c_str());
				m_mifFuncDecl.Append(", %s data", wrSrc.m_pType->m_typeName.c_str());
				m_mifMacros.Append(", %s data", wrSrc.m_pType->m_typeName.c_str());
			}

			if (wrSrc.m_varAddr1W >= 0 && !wrSrc.m_varAddr1IsHtId) {
				int W = max(1, wrSrc.m_varAddr1W);
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varAddr1", W);
				m_mifFuncDecl.Append(", ht_uint%d varAddr1", W);
				m_mifMacros.Append(", ht_uint%d %svarAddr1", W, wrSrc.m_varAddr1W == 0 ? "ht_noload " : "");
			}

			if (wrSrc.m_varAddr2W >= 0 && !wrSrc.m_varAddr2IsHtId) {
				int W = max(1, wrSrc.m_varAddr2W);
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varAddr2", W);
				m_mifFuncDecl.Append(", ht_uint%d varAddr2", W);
				m_mifMacros.Append(", ht_uint%d %svarAddr2", W, wrSrc.m_varAddr2W ? "ht_noload " : "");
			}

			for (size_t fldIdx = 0; fldIdx < wrSrc.m_fieldRefList.size(); fldIdx += 1) {
				string fldName = fldIdx == 0 ? "var" : VA("fld%d", (int)fldIdx);
				for (size_t dimIdx = 0; dimIdx < wrSrc.m_fieldRefList[fldIdx].m_refDimenList.size(); dimIdx += 1) {
					CRefDimen & refDimen = wrSrc.m_fieldRefList[fldIdx].m_refDimenList[dimIdx];

					if (refDimen.m_value < 0) {
						int varDimenW = FindLg2(wrSrc.m_fieldRefList[fldIdx].m_refDimenList[dimIdx].m_size - 1, true);
						g_appArgs.GetDsnRpt().AddText(", ht_uint%d %sIdx%d", varDimenW, fldName.c_str(), (int)dimIdx + 1);
						m_mifFuncDecl.Append(", ht_uint%d ht_noload %sIdx%d", varDimenW, fldName.c_str(), (int)dimIdx + 1);
						m_mifMacros.Append(", ht_uint%d ht_noload %sIdx%d", varDimenW, fldName.c_str(), (int)dimIdx + 1);
					}
				}
			}

			int wrSrcSize = 1;
			bool bNeedElemParam = false;
			if (wrSrc.m_fieldRefList.size() > 0) {
				for (size_t i = 0; i < wrSrc.m_fieldRefList.back().m_refDimenList.size(); i += 1) {
					if (wrSrc.m_fieldRefList.back().m_refDimenList[i].m_value < 0)
						wrSrcSize *= wrSrc.m_fieldRefList.back().m_refDimenList[i].m_size;
					bNeedElemParam = wrSrc.m_fieldRefList.back().m_refDimenList[i].m_value < 0;
				}
			}

			bool bMultiElemDstParam = (wrSrc.m_pType == 0) &&
				(wrSrc.m_elemCntW.size() == 0 || wrSrc.m_elemCntW.AsInt() > 0) &&
				(wrSrc.m_fieldRefList.size() == 1 && wrSrc.m_varAddr1W > 0 || bNeedElemParam);

			bool bMultiElemDst = (wrSrc.m_pType == 0) &&
				(wrSrc.m_elemCntW.size() == 0 || wrSrc.m_elemCntW.AsInt() > 0) &&
				(wrSrc.m_fieldRefList.size() == 1 && wrSrc.m_varAddr1W > 0 ||
				wrSrc.m_fieldRefList.back().m_refDimenList.size() > 0 && wrSrcSize > 1);

			int elemCntW = 0;
			if (bMultiElemDstParam) {
				if (wrSrc.m_elemCntW.AsInt() > 0)
					elemCntW = wrSrc.m_elemCntW.AsInt();
				else if (wrSrc.m_varAddr1W > 0) {
					elemCntW = wrSrc.m_varAddr1W + 1;
					if (wrSrc.m_varAddr2W > 0)
						elemCntW += wrSrc.m_varAddr2W;
				} else {
					elemCntW = FindLg2(wrSrcSize, true);
				}
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d elemCnt", elemCntW);
				m_mifFuncDecl.Append(", ht_uint%d ht_noload elemCnt", elemCntW);
				m_mifMacros.Append(", ht_uint%d ht_noload elemCnt", elemCntW);
			}

			if (wrSrc.m_memDst.size() == 0 && bMultiElemDstParam) {
				g_appArgs.GetDsnRpt().AddText(", bool bHost = false");
				m_mifFuncDecl.Append(", bool ht_noload bHost = false");
				m_mifMacros.Append(", bool ht_noload bHost");
			}

			g_appArgs.GetDsnRpt().AddText(", bool orderChk = true);\n");
			m_mifFuncDecl.Append(", bool ht_noload orderChk = true);\n");
			m_mifMacros.Append(", bool ht_noload orderChk)\n");

			g_appArgs.GetDsnRpt().EndLevel();

			if (true || g_appArgs.IsMemTraceEnabled()) {
				m_mifFuncDecl.Append("#\telse\n");
				m_mifMacros.Append("#else\n");

				m_mifFuncDecl.Append("#\tdefine WriteMem%s(", srcName);
				m_mifFuncDecl.Append("...) WriteMem%s_(", srcName);
				m_mifFuncDecl.Append("(char *)__FILE__, __LINE__, __VA_ARGS__)\n");
				m_mifFuncDecl.Append("\tvoid WriteMem%s_(char *file, int line, ",
					srcName);

				m_mifMacros.Append("void CPers%s%s::WriteMem%s_(char *file, int line, ",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(),
					srcName);

				if (!bWrRspGrpIdIsHtId && wrRspGrpIdW > 0) {
					m_mifFuncDecl.Append("sc_uint<%s_WR_GRP_ID_W> wrGrpId, ", mod.m_modName.Upper().c_str());
					m_mifMacros.Append("sc_uint<%s_WR_GRP_ID_W> wrGrpId, ", mod.m_modName.Upper().c_str());
				}

				m_mifFuncDecl.Append("sc_uint<MEM_ADDR_W> memAddr");
				m_mifMacros.Append("sc_uint<MEM_ADDR_W> memAddr");

				if (wrSrc.m_pType != 0) {
					m_mifFuncDecl.Append(", %s data", wrSrc.m_pType->m_typeName.c_str());
					m_mifMacros.Append(", %s data", wrSrc.m_pType->m_typeName.c_str());
				}

				if (wrSrc.m_varAddr1W >= 0 && !wrSrc.m_varAddr1IsHtId) {
					int W = max(1, wrSrc.m_varAddr1W);
					m_mifFuncDecl.Append(", ht_uint%d varAddr1", W);
					m_mifMacros.Append(", ht_uint%d varAddr1", W);
				}

				if (wrSrc.m_varAddr2W >= 0 && !wrSrc.m_varAddr2IsHtId) {
					int W = max(1, wrSrc.m_varAddr2W);
					m_mifFuncDecl.Append(", ht_uint%d varAddr2", W);
					m_mifMacros.Append(", ht_uint%d varAddr2", W);
				}

				for (size_t fldIdx = 0; fldIdx < wrSrc.m_fieldRefList.size(); fldIdx += 1) {
					string fldName = fldIdx == 0 ? "var" : VA("fld%d", (int)fldIdx);
					for (size_t dimIdx = 0; dimIdx < wrSrc.m_fieldRefList[fldIdx].m_refDimenList.size(); dimIdx += 1) {
						CRefDimen & refDimen = wrSrc.m_fieldRefList[fldIdx].m_refDimenList[dimIdx];

						if (refDimen.m_value < 0) {
							int varDimenW = FindLg2(wrSrc.m_fieldRefList[fldIdx].m_refDimenList[dimIdx].m_size - 1);
							m_mifFuncDecl.Append(", ht_uint%d ht_noload %sIdx%d", varDimenW, fldName.c_str(), (int)dimIdx + 1);
							m_mifMacros.Append(", ht_uint%d ht_noload %sIdx%d", varDimenW, fldName.c_str(), (int)dimIdx + 1);
						}
					}
				}

				if (bMultiElemDstParam) {
					m_mifFuncDecl.Append(", ht_uint%d ht_noload elemCnt", elemCntW);
					m_mifMacros.Append(", ht_uint%d ht_noload elemCnt", elemCntW);
				}

				if (wrSrc.m_memDst.size() == 0 && bMultiElemDstParam) {
					m_mifFuncDecl.Append(", bool ht_noload bHost = false");
					m_mifMacros.Append(", bool ht_noload bHost");
				}

				m_mifFuncDecl.Append(", bool orderChk = true);\n");
				m_mifMacros.Append(", bool orderChk)\n");

				m_mifFuncDecl.Append("#\tendif\n");
				m_mifMacros.Append("#endif\n");
			}

			m_mifMacros.Append("{\n");

			string routineName = VA("WriteMem%s", srcName);

			m_mifMacros.Append("\tassert_msg(c_t%d_bWriteMemAvail, \"Runtime check failed in CPers%s::%s()"
				" - expected WriteMemBusy() to have been called and not busy\");\n",
				mod.m_execStg, mod.m_modName.Uc().c_str(), routineName.c_str());

			if (wrRspGrpIdW == 0) {
				m_mifMacros.Append("\t//assert_msg(!c_wrGrpState.m_pause, \"Runtime check failed in CPers%s::%s() - expected %s() to be called before WriteMemPause()\");\n",
					mod.m_modName.Uc().c_str(), routineName.c_str(), routineName.c_str());
			} else if (wrRspGrpIdW <= 2) {
				m_mifMacros.Append("\t//assert_msg(!c_wrGrpState[%s].m_pause, \"Runtime check failed in CPers%s::%s() - expected %s() to be called before WriteMemPause()\");\n",
					wrGrpId.c_str(), mod.m_modName.Uc().c_str(), routineName.c_str(), routineName.c_str());

			} else {
				m_mifMacros.Append("\t//assert_msg(c_t%d_wrGrpReqState.m_pause == c_t%d_wrGrpRspState.m_pause, \"Runtime check failed in CPers%s::%s() - expected %s() to be called before WriteMemPause()\");\n",
					mod.m_execStg, mod.m_execStg, mod.m_modName.Uc().c_str(), routineName.c_str(), routineName.c_str());
			}

			for (size_t fldIdx = 0; fldIdx < wrSrc.m_fieldRefList.size(); fldIdx += 1) {
				string fldName = fldIdx == 0 ? "var" : VA("fld%d", (int)fldIdx);
				for (size_t dimIdx = 0; dimIdx < wrSrc.m_fieldRefList[fldIdx].m_refDimenList.size(); dimIdx += 1) {
					CRefDimen & refDimen = wrSrc.m_fieldRefList[fldIdx].m_refDimenList[dimIdx];

					if (refDimen.m_value < 0 && (refDimen.m_size == 1 || !IsPowerTwo(refDimen.m_size))) {

						m_mifMacros.Append("\tassert_msg(%sIdx%d < %d, \"Runtime check failed in CPers%s%s::WriteMem%s(...) - %sIdx%d (%%d) out of range (0-%d)\\n\", (uint32_t)%sIdx%d);\n",
							fldName.c_str(), (int)dimIdx + 1,
							refDimen.m_size,
							unitNameUc.c_str(), mod.m_modName.Uc().c_str(), srcName,
							fldName.c_str(), (int)dimIdx + 1,
							refDimen.m_size - 1,
							fldName.c_str(), (int)dimIdx + 1);
					}
				}
			}

			if (bMultiElemDstParam) {
				m_mifMacros.Append("\tassert_msg(");

				if (wrSrc.m_varAddr1W > 0 && wrSrc.m_fieldRefList.size() == 1) {
					if (!wrSrc.m_varAddr1IsHtId) {
						m_mifMacros.Append("varAddr1");
						if (wrSrc.m_varAddr2W > 0)
							m_mifMacros.Append(" * %d + ", 1 << wrSrc.m_varAddr2W);
					}
					if (wrSrc.m_varAddr2W > 0 && !wrSrc.m_varAddr2IsHtId)
						m_mifMacros.Append("varAddr2");
				} else {
					bool bNeedMul = false;
					size_t fldIdx = wrSrc.m_fieldRefList.size() - 1;
					for (size_t dimIdx = 0; dimIdx < wrSrc.m_fieldRefList.back().m_refDimenList.size(); dimIdx += 1) {
						string fldName = fldIdx == 0 ? "var" : VA("fld%d", (int)fldIdx);
						CRefDimen & refDimen = wrSrc.m_fieldRefList.back().m_refDimenList[dimIdx];
						if (bNeedMul) {
							m_mifMacros.Append(" * %d + ", refDimen.m_size);
						}
						if (refDimen.m_value < 0) {
							m_mifMacros.Append("%sIdx%d", fldName.c_str(), (int)dimIdx + 1);
							bNeedMul = true;
						}
					}
				}
				m_mifMacros.Append(" + elemCnt <= %d, ", wrSrc.m_maxElemCnt);
				m_mifMacros.Append(" \"Runtime check failed in CPers%s%s::WriteMem%s(...) - elemCnt range check failed\\n\");\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), srcName);
			}
			m_mifMacros.NewLine();

			switch (wrSrc.m_pSrcType->m_clangMinAlign) {
			case 8:
				break;
			case 16:
				m_mifMacros.Append("\tassert_msg((memAddr & 1) == 0, \"Runtime check failed in CPers%s::WriteMem%s()"
					" - expected address to be uint16 aligned\");\n", mod.m_modName.Uc().c_str(), srcName);
				break;
			case 32:
				m_mifMacros.Append("\tassert_msg((memAddr & 3) == 0, \"Runtime check failed in CPers%s::WriteMem%s()"
					" - expected address to be uint32 aligned\");\n", mod.m_modName.Uc().c_str(), srcName);
				break;
			case 64:
				m_mifMacros.Append("\tassert_msg((memAddr & 7) == 0, \"Runtime check failed in CPers%s::WriteMem%s()"
					" - expected address to be uint64 aligned\");\n", mod.m_modName.Uc().c_str(), srcName);
				break;
			default:
				break;
			}

			if (wrSrc.m_bMultiQwWrReq && wrSrc.m_pGblVar == 0) {
				m_mifMacros.Append("\t//assert_msg(qwCnt > 0 && qwCnt <= 8, \"Runtime check failed in CPers%s::WriteMem%s()"
					" - expected multi-qw access to be within 64B memory line\");\n", mod.m_modName.Uc().c_str(), srcName);
				m_mifMacros.Append("\t//assert_msg(((memAddr >> 3) & 7) + qwCnt <= 8, \"Runtime check failed in CPers%s::WriteMem%s()"
					" - expected multi-qw access to be within 64B memory line\");\n", mod.m_modName.Uc().c_str(), srcName);
				m_mifMacros.NewLine();
			}
			m_mifMacros.NewLine();

			int qwCnt = wrSrc.m_pSrcType->m_clangMinAlign == 1 ? 1 :
				((wrSrc.m_pSrcType->m_clangBitWidth + wrSrc.m_pSrcType->m_clangMinAlign - 1) / wrSrc.m_pSrcType->m_clangMinAlign);

			if (mif.m_mifReqStgCnt > 0)
				m_mifMacros.Append("\tc_t%d_memReq.m_valid = %s;\n", mod.m_execStg, bMultiElemDstParam ? "elemCnt > 0" : "true");

			m_mifMacros.Append("\tc_t%d_memReq.m_wrReq = %s;\n", mod.m_execStg, bMultiElemDstParam ? "elemCnt > 0" : "true");

			if (bMultiQwReq) {
				if (bMultiElemDstParam)
					m_mifMacros.Append("\tc_t%d_memReq.m_qwRem = (ht_uint%d)(%d * elemCnt);\n", mod.m_execStg, FindLg2(maxQwCnt), qwCnt);
				else
					m_mifMacros.Append("\tc_t%d_memReq.m_qwRem = %d;\n", mod.m_execStg, qwCnt);

				if (bMultiQwMif)
					m_mifMacros.Append("\tc_t%d_memReq.m_reqQwRem = 0;\n", mod.m_execStg);

				bool bNeedCntM1 = false;
				if (wrSrc.m_varAddr1W > 0) {
					if (bMultiElemDst && wrSrc.m_fieldRefList.size() == 1) {
						m_mifMacros.Append("\tc_t%d_memReq.m_vIdx1 = %s;\n",
							mod.m_execStg,
							wrSrc.m_varAddr1IsHtId ? VA("r_t%d_htId", mod.m_execStg).c_str() : "varAddr1");
						wrSrc.m_varAddr1IsIdx = true;
						bNeedCntM1 = true;
					}
				}

				if (wrSrc.m_varAddr2W > 0) {
					if (bMultiElemDst && wrSrc.m_fieldRefList.size() == 1) {
						m_mifMacros.Append("\tc_t%d_memReq.m_vIdx2 = %s;\n",
							mod.m_execStg,
							wrSrc.m_varAddr2IsHtId ? VA("r_t%d_htId", mod.m_execStg).c_str() : "varAddr2");
						if (bNeedCntM1)
							m_mifMacros.Append("\tc_t%d_memReq.m_vIdx2CntM1 = %d;\n", mod.m_execStg, (1 << wrSrc.m_varAddr2W) - 1);
						wrSrc.m_varAddr2IsIdx = true;
						bNeedCntM1 = true;
					}
				}

				if (wrSrc.m_varAddr1W <= 0 || wrSrc.m_fieldRefList.size() > 1) {
					for (size_t fldIdx = 0; fldIdx < wrSrc.m_fieldRefList.size(); fldIdx += 1) {
						string fldName = fldIdx == 0 ? "var" : VA("fld%d", (int)fldIdx);
						for (size_t dimIdx = 0; dimIdx < wrSrc.m_fieldRefList[fldIdx].m_refDimenList.size(); dimIdx += 1) {
							if (bMultiElemDst && fldIdx + 1 == wrSrc.m_fieldRefList.size()) {
								if (mif.m_vIdxWList.size() > dimIdx && mif.m_vIdxWList[dimIdx] > 0 &&
									wrSrc.m_fieldRefList[fldIdx].m_refDimenList[dimIdx].m_value < 0)
								{
									m_mifMacros.Append("\tc_t%d_memReq.m_vIdx%d = %sIdx%d;\n",
										mod.m_execStg, dimIdx + 1, fldName.c_str(), (int)dimIdx + 1);
									wrSrc.m_fieldRefList[fldIdx].m_refDimenList[dimIdx].m_isIdx = true;
									if (bNeedCntM1) {
										m_mifMacros.Append("\tc_t%d_memReq.m_vIdx%dCntM1 = %d;\n",
											mod.m_execStg, dimIdx + 1, wrSrc.m_fieldRefList[fldIdx].m_refDimenList[dimIdx].m_size - 1);
									}
									bNeedCntM1 = true;
								}
							}
						}
					}
				}

				if (maxElemQwCnt > 1) {
					m_mifMacros.Append("\tc_t%d_memReq.m_elemQwIdx = 0;\n", mod.m_execStg);
					if (bNeedElemQwCntM1)
						m_mifMacros.Append("\tc_t%d_memReq.m_elemQwCntM1 = %d;\n", mod.m_execStg, qwCnt - 1);
				}
			}

			if (wrRspGrpIdW == 0) {
				// rspGrpId == 0
			} else if (bWrRspGrpIdIsHtId) {
				m_mifMacros.Append("\tc_t%d_memReq.m_wrGrpId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);
			} else {
				m_mifMacros.Append("\tc_t%d_memReq.m_wrGrpId = wrGrpId;\n", mod.m_execStg);
			}

			if (mif.m_mifWr.m_wrSrcList.size() > 0)
				m_mifMacros.Append("\tc_t%d_memReq.m_wrSrc = %d;\n", mod.m_execStg, wrSrcIdx);

			if (mif.m_mifReqStgCnt > 0) {
				if (wrSrc.m_pType != 0)
					m_mifMacros.Append("\tc_t%d_memReq.m_wrData.m_%s = data;\n", mod.m_execStg, wrSrc.m_pType->m_typeName.c_str());

				else if (wrSrc.m_pPrivVar || wrSrc.m_pGblVar && wrSrc.m_pGblVar->m_bPrivGbl &&
					wrSrc.m_pGblVar->m_addrW == mod.m_threads.m_htIdW.AsInt())
				{

					string varName;
					for (int fldIdx = 0; fldIdx < (int)wrSrc.m_fieldRefList.size(); fldIdx += 1) {
						CFieldRef & fieldRef = wrSrc.m_fieldRefList[fldIdx];

						if (fldIdx > 0) {
							varName += '.';
							varName += fieldRef.m_fieldName;
						}

						string fldName = fldIdx == 0 ? "var" : VA("fld%d", (int)fldIdx);

						if (wrSrc.m_pPrivVar && !wrSrc.m_bConstDimenList && fldIdx == (int)wrSrc.m_fieldRefList.size() - 1)
							break;

						for (int dimIdx = 0; dimIdx < (int)fieldRef.m_refDimenList.size(); dimIdx += 1) {
							CRefDimen & refDimen = fieldRef.m_refDimenList[dimIdx];

							string idxStr;
							if (refDimen.m_value >= 0)
								idxStr = VA("[%d]", refDimen.m_value);

							else if (refDimen.m_size == 1)
								idxStr = "[0]";

							else
								idxStr = VA("[INT(%sIdx%d)]", fldName.c_str(), dimIdx + 1);

							varName += idxStr;
						}
					}

					if (wrSrc.m_pPrivVar) {
						vector<CHtString> dimenList;

						if (!wrSrc.m_bConstDimenList && (wrSrc.m_fieldRefList.size() == 1 && wrSrc.m_fieldRefList[0].m_refAddrList.size() == 0 ||
							wrSrc.m_fieldRefList.size() > 1))
						{
							vector<CRefDimen> & refDimenList = wrSrc.m_fieldRefList.back().m_refDimenList;
							for (size_t i = 0; i < refDimenList.size(); i += 1)
								dimenList.push_back(CHtString(refDimenList[i].m_size));
						}
						string tabs = "\t";
						CLoopInfo loopInfo(m_mifMacros, tabs, dimenList, 1);
						do {
							string dimIdx = loopInfo.IndexStr();
							m_mifMacros.Append("%sc_t%d_memReq.m_wrData.m_%s%s = r_t%d_htPriv.m_%s%s%s;\n", tabs.c_str(),
								mod.m_execStg, wrSrc.m_wrDataTypeName.c_str(), dimIdx.c_str(),
								mod.m_execStg, wrSrc.m_pPrivVar->m_name.c_str(), varName.c_str(), dimIdx.c_str());
						} while (loopInfo.Iter());
					}
					else {
						m_mifMacros.Append("\tc_t%d_memReq.m_wrData.m_%s = r_t%d_%sI%cData%s;\n",
							mod.m_execStg, wrSrc.m_pSrcType->m_typeName.c_str(),
							mod.m_execStg, wrSrc.m_pGblVar->m_gblName.c_str(), mod.m_execStg > mod.m_tsStg ? 'w' : 'r', varName.c_str());
					}
				}
			}

			if (wrSrc.m_varAddr1W > 0 && !wrSrc.m_varAddr1IsIdx && !wrSrc.m_varAddr1IsHtId) {
				m_mifMacros.Append("\tc_t%d_memReq.m_s%d_f1Addr1 = varAddr1;\n", mod.m_execStg, wrSrcIdx);
			}

			if (wrSrc.m_varAddr2W > 0 && !wrSrc.m_varAddr2IsIdx && !wrSrc.m_varAddr2IsHtId) {
				m_mifMacros.Append("\tc_t%d_memReq.m_s%d_f1Addr2 = varAddr2;\n", mod.m_execStg, wrSrcIdx);
			}

			for (size_t fldIdx = 0; fldIdx < wrSrc.m_fieldRefList.size(); fldIdx += 1) {
				string fldName = fldIdx == 0 ? "var" : VA("fld%d", (int)fldIdx);
				for (size_t dimIdx = 0; dimIdx < wrSrc.m_fieldRefList[fldIdx].m_refDimenList.size(); dimIdx += 1) {
					CRefDimen & refDimen = wrSrc.m_fieldRefList[fldIdx].m_refDimenList[dimIdx];

					if (refDimen.m_value < 0 && refDimen.m_size > 1 && !refDimen.m_isIdx) {
						m_mifMacros.Append("\tc_t%d_memReq.m_s%d_f%dIdx%d = %sIdx%d;\n",
							mod.m_execStg, wrSrcIdx, (int)fldIdx + 1, dimIdx + 1, fldName.c_str(), dimIdx + 1);
					}
				}
			}

			m_mifMacros.NewLine();

			if (mif.m_mifReqStgCnt == 0) {
				m_mifMacros.Append("\tc_t%d_%sToMif_reqRdy = true;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
			}

			if (wrSrc.m_bMultiQwWrReq) {
				if (wrSrc.m_memDst.size() == 0 && bMultiElemDst)
					m_mifMacros.Append("\tc_t%d_%sToMif_req.m_host = bHost;\n", mod.m_execStg, mod.m_modName.Lc().c_str());
				else {
					m_mifMacros.Append("\tc_t%d_%sToMif_req.m_host = %s;\n", mod.m_execStg, mod.m_modName.Lc().c_str(),
						wrSrc.m_memDst == "host" ? "true" : "false");
				}
			}

			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_type = MEM_REQ_WR;\n", mod.m_execStg, mod.m_modName.Lc().c_str());

			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_addr = memAddr;\n", mod.m_execStg, mod.m_modName.Lc().c_str());

			char const * pReqSize = "";
			switch (wrSrc.m_memSize) {
			case 8: pReqSize = "MEM_REQ_U8"; break;
			case 16: pReqSize = "MEM_REQ_U16"; break;
			case 32: pReqSize = "MEM_REQ_U32"; break;
			case 64: pReqSize = "MEM_REQ_U64"; break;
			default: HtlAssert(0);
			}

			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_size = %s;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str(), pReqSize);

			if (wrRspGrpIdW == 0) {
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_tid = 0;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
			} else if (bWrRspGrpIdIsHtId) {
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_tid = r_t%d_htId;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str(), mod.m_execStg);
			} else {
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_tid = wrGrpId;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
			}

			if (mif.m_mifReqStgCnt == 0) {
				string varName;
				for (int fldIdx = 0; fldIdx < (int)wrSrc.m_fieldRefList.size(); fldIdx += 1) {
					CFieldRef & fieldRef = wrSrc.m_fieldRefList[fldIdx];

					if (fldIdx > 0) {
						varName += '.';
						varName += fieldRef.m_fieldName;
					}

					string fldName = fldIdx == 0 ? "var" : VA("fld%d", (int)fldIdx);

					for (int dimIdx = 0; dimIdx < (int)fieldRef.m_refDimenList.size(); dimIdx += 1) {
						CRefDimen & refDimen = fieldRef.m_refDimenList[dimIdx];

						string idxStr;
						if (refDimen.m_value >= 0)
							idxStr = VA("[%d]", refDimen.m_value);

						else if (refDimen.m_size == 1)
							idxStr = "[0]";

						else
							idxStr = VA("[INT(%sIdx%d)]", fldName.c_str(), dimIdx + 1);

						varName += idxStr;
					}
				}

				if (wrSrc.m_pType != 0) {
					if (!wrSrc.m_pSrcType->IsRecord()) {
						m_mifMacros.Append("\tc_t%d_%sToMif_req.m_data = data;\n", mod.m_execStg, mod.m_modName.Lc().c_str());
					} else {
						for (CStructElemIter iter(this, wrSrc.m_pSrcType); !iter.end(); iter++) {
							m_mifMacros.Append("\tc_t%d_%sToMif_req.m_data |= (uint64_t)(data%s%s << %dLL);\n",
								mod.m_execStg, mod.m_modName.Lc().c_str(),
								varName.c_str(), iter.GetHeirFieldName().c_str(),
								iter.GetHeirFieldPos());
						}
					}
				} else if (wrSrc.m_pPrivVar || wrSrc.m_pGblVar && wrSrc.m_pGblVar->m_bPrivGbl &&
					wrSrc.m_pGblVar->m_addrW == mod.m_threads.m_htIdW.AsInt() || wrSrc.m_pGblVar && wrSrc.m_pGblVar->m_addrW == 0 ||
					wrSrc.m_pSharedVar)
				{
					if (wrSrc.m_pPrivVar) {
						if (!wrSrc.m_pSrcType->IsRecord()) {
							m_mifMacros.Append("\tc_t%d_%sToMif_req.m_data = c_t%d_htPriv.m_%s%s;\n",
								mod.m_execStg, mod.m_modName.Lc().c_str(),
								mod.m_execStg, wrSrc.m_pPrivVar->m_name.c_str(), varName.c_str());
						} else {
							for (CStructElemIter iter(this, wrSrc.m_pSrcType); !iter.end(); iter++) {
								m_mifMacros.Append("\tc_t%d_%sToMif_req.m_data |= (uint64_t)(c_t%d_htPriv.m_%s%s%s << %dLL);\n",
									mod.m_execStg, mod.m_modName.Lc().c_str(),
									mod.m_execStg, wrSrc.m_pPrivVar->m_name.c_str(), varName.c_str(), iter.GetHeirFieldName().c_str(),
									iter.GetHeirFieldPos());
							}
						}
					}
					else if (wrSrc.m_pSharedVar) {
						if (!wrSrc.m_pSrcType->IsRecord()) {
							m_mifMacros.Append("\tc_t%d_%sToMif_req.m_data = r__SHR__%s%s;\n",
							mod.m_execStg, mod.m_modName.Lc().c_str(),
							wrSrc.m_pSharedVar->m_name.c_str(), varName.c_str());
						} else {
							for (CStructElemIter iter(this, wrSrc.m_pSrcType); !iter.end(); iter++) {
								m_mifMacros.Append("\tc_t%d_%sToMif_req.m_data |= (uint64_t)(r__SHR__%s%s%s << %dLL);\n",
									mod.m_execStg, mod.m_modName.Lc().c_str(),
									wrSrc.m_pSharedVar->m_name.c_str(), varName.c_str(), iter.GetHeirFieldName().c_str(),
									iter.GetHeirFieldPos());
							}
						}
					}
					else if (wrSrc.m_pGblVar && wrSrc.m_pGblVar->m_bPrivGbl) {
						if (!wrSrc.m_pSrcType->IsRecord()) {
							m_mifMacros.Append("\tc_t%d_%sToMif_req.m_data = r_t%d_%sI%cData%s;\n",
							mod.m_execStg, mod.m_modName.Lc().c_str(),
							mod.m_execStg, wrSrc.m_pGblVar->m_gblName.c_str(), mod.m_execStg > mod.m_tsStg ? 'w' : 'r', varName.c_str());
						} else {
							for (CStructElemIter iter(this, wrSrc.m_pSrcType); !iter.end(); iter++) {
								m_mifMacros.Append("\tc_t%d_%sToMif_req.m_data |= (uint64_t)(r_t%d_%sI%cData%s%s << %dLL);\n",
									mod.m_execStg, mod.m_modName.Lc().c_str(),
									mod.m_execStg, wrSrc.m_pGblVar->m_gblName.c_str(), mod.m_execStg > mod.m_tsStg ? 'w' : 'r', varName.c_str(), iter.GetHeirFieldName().c_str(),
									iter.GetHeirFieldPos());
							}
						}
					}
					else {
						if (!wrSrc.m_pSrcType->IsRecord()) {
							m_mifMacros.Append("\tc_t%d_%sToMif_req.m_data = r_%s%s;\n",
							mod.m_execStg, mod.m_modName.Lc().c_str(),
							wrSrc.m_pGblVar->m_gblName.c_str(), varName.c_str());
						} else {
							for (CStructElemIter iter(this, wrSrc.m_pSrcType); !iter.end(); iter++) {
								m_mifMacros.Append("\tc_t%d_%sToMif_req.m_data |= (uint64_t)(r_%s%s%s << %dLL);\n",
									mod.m_execStg, mod.m_modName.Lc().c_str(),
									wrSrc.m_pGblVar->m_gblName.c_str(), varName.c_str(), iter.GetHeirFieldName().c_str(),
									iter.GetHeirFieldPos());
							}
						}
					}
				}
			}

			m_mifMacros.Append("#\tifndef _HTV\n");
			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_file = file;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str());
			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_line = line;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str());
			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_time = sc_time_stamp().value();\n",
				mod.m_execStg, mod.m_modName.Lc().c_str());
			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_orderChk = orderChk;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str());
			m_mifMacros.Append("#\tendif\n");
			m_mifMacros.NewLine();

			m_mifMacros.Append("}\n");
			m_mifMacros.NewLine();
		}
		///////////////////////////////////////////////////////////
		// generate WriteMemPause routine

		if (mif.m_mifWr.m_bPause) {

			g_appArgs.GetDsnRpt().AddLevel("WriteMemPause(%sht_uint%d rsmInstr)\n", wrGrpIdDecl.c_str(), mod.m_instrW);
			g_appArgs.GetDsnRpt().EndLevel();

			m_mifFuncDecl.Append("\tvoid WriteMemPause(%sht_uint%d rsmInstr);\n", wrGrpIdDecl.c_str(), mod.m_instrW);
			m_mifMacros.Append("void CPers%s%s::WriteMemPause(%sht_uint%d rsmInstr)\n",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(), wrGrpIdDecl.c_str(), mod.m_instrW);
			m_mifMacros.Append("{\n");

			m_mifMacros.Append("\tassert_msg(c_t%d_bWriteMemAvail, \"Runtime check failed in CPers%s::WriteMemPause()"
				" - expected WriteMemBusy() to have been called and not busy\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());

			m_mifMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::WriteMemPause()"
				" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());
			m_mifMacros.NewLine();

			if (wrRspGrpIdW == 0) {

				m_mifMacros.Append("\tc_t%d_memReq.m_valid = true;\n", mod.m_execStg);
				m_mifMacros.Append("\tc_t%d_memReq.m_wrPause = true;\n", mod.m_execStg);

				if (mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\tc_t%d_memReq.m_rsmHtId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);

				m_mifMacros.Append("\tc_t%d_memReq.m_rsmInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);

			} else if (wrRspGrpIdW <= 2) {

				m_mifMacros.Append("\tc_t%d_memReq.m_valid = true;\n", mod.m_execStg);
				m_mifMacros.Append("\tc_t%d_memReq.m_wrPause = true;\n", mod.m_execStg);

				if (mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\tc_t%d_memReq.m_rsmHtId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);

				if (wrRspGrpIdW == 0) {
					// rspGrpId == 0
				} else if (bWrRspGrpIdIsHtId)
					m_mifMacros.Append("\tc_t%d_memReq.m_wrGrpId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);
				else
					m_mifMacros.Append("\tc_t%d_memReq.m_wrGrpId = wrGrpId;\n", mod.m_execStg);

				m_mifMacros.Append("\tc_t%d_memReq.m_rsmInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);

			} else {

				m_mifMacros.Append("\tc_t%d_memReq.m_valid = true;\n", mod.m_execStg);
				m_mifMacros.Append("\tc_t%d_memReq.m_wrPause = true;\n", mod.m_execStg);

				if (mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\tc_t%d_memReq.m_rsmHtId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);

				if (wrRspGrpIdW == 0) {
					// rspGrpId == 0
				} else if (bWrRspGrpIdIsHtId)
					m_mifMacros.Append("\tc_t%d_memReq.m_wrGrpId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);
				else
					m_mifMacros.Append("\tc_t%d_memReq.m_wrGrpId = wrGrpId;\n", mod.m_execStg);

				m_mifMacros.Append("\tc_t%d_memReq.m_rsmInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
			}

			m_mifMacros.Append("}\n");
			m_mifMacros.NewLine();
		}

		///////////////////////////////////////////////////////////
		// generate WriteReqPause routine

		if (mif.m_mifWr.m_bReqPause) {

			g_appArgs.GetDsnRpt().AddLevel("WriteReqPause(ht_uint%d rsmInstr)\n", mod.m_instrW);
			g_appArgs.GetDsnRpt().EndLevel();

			m_mifFuncDecl.Append("\tvoid WriteReqPause(ht_uint%d rsmInstr);\n", mod.m_instrW);
			m_mifMacros.Append("void CPers%s%s::WriteReqPause(ht_uint%d rsmInstr)\n",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
			m_mifMacros.Append("{\n");

			m_mifMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::WriteReqPause()"
				" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());
			m_mifMacros.NewLine();

			m_mifMacros.Append("\tc_t%d_memReq.m_valid = true;\n", mod.m_execStg);
			m_mifMacros.Append("\tc_t%d_memReq.m_reqPause = true;\n", mod.m_execStg);

			if (mod.m_threads.m_htIdW.AsInt() > 0)
				m_mifMacros.Append("\tc_t%d_memReq.m_rsmHtId = r_t%d_htId;\n", mod.m_execStg, mod.m_execStg);

			m_mifMacros.Append("\tc_t%d_memReq.m_rsmInstr = rsmInstr;\n", mod.m_execStg);
			m_mifMacros.NewLine();

			m_mifMacros.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);

			m_mifMacros.Append("}\n");
			m_mifMacros.NewLine();
		}
	}

	g_appArgs.GetDsnRpt().EndLevel();

	m_mifRamDecl.Append("\t// Memory Interface State\n");

	string reset = mod.m_clkRate == eClk2x ? "c_reset1x" : "r_reset1x";

	bool bNeedRdRspInfo = false;
	bool bNeedRdRspInfoRam = false;
	if (mif.m_bMifRd) {

		CRecord rdRspInfo;
		rdRspInfo.m_typeName = "CRdRspInfo";
		rdRspInfo.m_bCStyle = true;
		rdRspInfo.m_bUnion = true;

		int dstW = FindLg2(mif.m_mifRd.m_rdDstList.size() - 1, false);
		int baseW = 0;

		CHtInt * pUint = mif.m_mifRd.m_maxRdRspInfoW > 32 ? &g_uint64 : &g_uint32;

		CRecord unnamed1;
		{
			unnamed1.m_typeName = "";
			unnamed1.m_bCStyle = true;
			unnamed1.m_bUnion = false;

			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				unnamed1.AddStructField(pUint, "m_htId", VA("%d", mod.m_threads.m_htIdW.AsInt()));
				baseW += mod.m_threads.m_htIdW.AsInt();
			}

			if (!bRdRspGrpIdIsHtId && rdRspGrpIdW > 0) {
				unnamed1.AddStructField(pUint, "m_grpId", VA("%d", rdRspGrpIdW));
				baseW += rdRspGrpIdW;
			}

			if (dstW > 0) {
				unnamed1.AddStructField(pUint, "m_dst", VA("%d", dstW));
				baseW += dstW;
			}

			if (bMultiQwRdMif) {
				unnamed1.AddStructField(pUint, "m_multiQwReq", "1");
				unnamed1.AddStructField(pUint, "m_qwFst", "3");
				unnamed1.AddStructField(pUint, "m_qwLst", "3");
				baseW += 7;
			}

			bool bNeedCntM1 = false;
			for (int idx = 0; idx < (int)mif.m_vIdxWList.size(); idx += 1) {
				if (mif.m_vIdxWList[idx] > 0) {
					unnamed1.AddStructField(pUint, VA("m_vIdx%d", idx + 1), VA("%d", mif.m_vIdxWList[idx]));
					baseW += mif.m_vIdxWList[idx];
					if (bNeedCntM1) {
						unnamed1.AddStructField(pUint, VA("m_vIdx%dCntM1", idx + 1), VA("%d", mif.m_vIdxWList[idx]));
						baseW += mif.m_vIdxWList[idx];
					}
					bNeedCntM1 = true;
				}
			}

			if (maxElemQwCnt > 1) {
				int elemQwIdxW = FindLg2(maxElemQwCnt - 1);
				unnamed1.AddStructField(pUint, "m_elemQwIdx", VA("%d", elemQwIdxW));
				baseW += elemQwIdxW;
				if (bNeedCntM1) {
					unnamed1.AddStructField(pUint, "m_elemQwCntM1", VA("%d", elemQwIdxW));
					baseW += elemQwIdxW;
				}
			}

			if (unnamed1.m_fieldList.size() > 0) {
				rdRspInfo.AddStructField(&unnamed1, "");
			}
		}

		CRecord unnamed2;
		if (bRdRspGrpIdIsHtId && rdRspGrpIdW > 0) {
			// common fields
			unnamed2.m_typeName = "";
			unnamed2.m_bCStyle = true;
			unnamed2.m_bUnion = false;

			unnamed2.AddStructField(pUint, "m_grpId", VA("%d", rdRspGrpIdW));

			rdRspInfo.AddStructField(&unnamed2, "");
		}

		int maxRdDstInfoW = baseW;

		// per dst fields
		vector<CRecord> dstRecordList;
		for (size_t rdDstIdx = 0; rdDstIdx < mif.m_mifRd.m_rdDstList.size(); rdDstIdx += 1) {
			CMifRdDst &rdDst = mif.m_mifRd.m_rdDstList[rdDstIdx];

			CRecord dstRecord;
			dstRecord.m_typeName = "";
			dstRecord.m_bCStyle = true;
			dstRecord.m_bUnion = false;

			int rdDstInfoW = 0;

			if (baseW > 0) {
				dstRecord.AddStructField(pUint, VA("m_d%d_pad", rdDstIdx), VA("%d", baseW));
				rdDstInfoW += baseW;
			}

			if (rdDst.m_infoW.AsInt() > 0) {
				dstRecord.AddStructField(pUint, VA("m_d%d_info", rdDstIdx), VA("%d", rdDst.m_infoW.AsInt()));
				rdDstInfoW += rdDst.m_infoW.AsInt();
			}

			if (rdDst.m_varAddr1W > 0 && !rdDst.m_varAddr1IsHtId && !rdDst.m_varAddr1IsIdx) {
				dstRecord.AddStructField(pUint, VA("m_d%d_f1Addr1", rdDstIdx), VA("%d", rdDst.m_varAddr1W));
				rdDstInfoW += rdDst.m_varAddr1W;
			}

			if (rdDst.m_varAddr2W > 0 && !rdDst.m_varAddr2IsHtId && !rdDst.m_varAddr2IsIdx) {
				dstRecord.AddStructField(pUint, VA("m_d%d_f1Addr2", rdDstIdx), VA("%d", rdDst.m_varAddr2W));
				rdDstInfoW += rdDst.m_varAddr2W;
			}

			for (size_t fldIdx = 0; fldIdx < rdDst.m_fieldRefList.size(); fldIdx += 1) {
				for (size_t dimIdx = 0; dimIdx < rdDst.m_fieldRefList[fldIdx].m_refDimenList.size(); dimIdx += 1) {
					CRefDimen & refDimen = rdDst.m_fieldRefList[fldIdx].m_refDimenList[dimIdx];

					if (refDimen.m_value < 0 && refDimen.m_size > 1 && !refDimen.m_isIdx) {
						int varDimenW = FindLg2(rdDst.m_fieldRefList[fldIdx].m_refDimenList[dimIdx].m_size - 1);
						dstRecord.AddStructField(pUint, VA("m_d%d_f%dIdx%d",
							rdDstIdx, (int)fldIdx + 1, (int)dimIdx + 1), VA("%d", varDimenW));
						rdDstInfoW += varDimenW;
					}
				}
			}

			if ((int)dstRecord.m_fieldList.size() > (baseW == 0 ? 0 : 1)) {
				dstRecordList.push_back(dstRecord);
				maxRdDstInfoW = max(maxRdDstInfoW, rdDstInfoW);
			}
		}

		for (size_t listIdx = 0; listIdx < dstRecordList.size(); listIdx += 1) {
			rdRspInfo.AddStructField(&dstRecordList[listIdx], "");
		}

		if (rdRspInfo.m_fieldList.size() > 0) {
			if (!mif.m_mifRd.m_bNeedRdRspInfoRam)
				rdRspInfo.AddStructField(&g_uint32, "m_uint32", VA("%d", mif.m_mifRd.m_maxRdRspInfoW));

			GenUserStructs(m_mifDecl, &rdRspInfo, "\t");

			string fullName = VA("CPers%s::CRdRspInfo", mod.m_modName.Uc().c_str());

			GenUserStructBadData(m_mifBadDecl, true, fullName, rdRspInfo.m_fieldList, rdRspInfo.m_bCStyle, "");

			bNeedRdRspInfo = true;
		}

		bNeedRdRspInfoRam = maxRdDstInfoW > 24;

		assert(mif.m_mifRd.m_maxRdRspInfoW == maxRdDstInfoW);
		assert(mif.m_mifRd.m_bNeedRdRspInfoRam == bNeedRdRspInfoRam);
	}

	{
		CRecord memReq;
		memReq.m_typeName = "CMemReq";
		memReq.m_bCStyle = true;

		memReq.AddStructField(&g_bool, "m_valid");

		bool bNeedCntM1 = false;
		for (int idx = 0; idx < (int)mif.m_vIdxWList.size(); idx += 1) {
			if (mif.m_vIdxWList[idx] > 0) {
				memReq.AddStructField(FindHtIntType(eUnsigned, mif.m_vIdxWList[idx]), VA("m_vIdx%d", idx + 1));
				if (bNeedCntM1)
					memReq.AddStructField(FindHtIntType(eUnsigned, mif.m_vIdxWList[idx]), VA("m_vIdx%dCntM1", idx + 1));
				bNeedCntM1 = true;
			}
		}

		if (maxElemQwCnt > 1) {
			memReq.AddStructField(FindHtIntType(eUnsigned, FindLg2(maxElemQwCnt - 1)), "m_elemQwIdx");
			if (bNeedCntM1)
				memReq.AddStructField(FindHtIntType(eUnsigned, FindLg2(maxElemQwCnt - 1)), "m_elemQwCntM1");
		}

		if (maxQwCnt > 1) {
			memReq.AddStructField(FindHtIntType(eUnsigned, FindLg2(maxQwCnt)), "m_qwRem");

			if (bMultiQwMif) {
				memReq.AddStructField(FindHtIntType(eUnsigned, 3), "m_reqQwRem");
			}
		}

		if (mif.m_bMifRd) {
			memReq.AddStructField(&g_bool, "m_rdReq");
			if (mif.m_mifRd.m_bPause)
				memReq.AddStructField(&g_bool, "m_rdPause");
			if (mif.m_mifRd.m_bPoll)
				memReq.AddStructField(&g_bool, "m_rdPoll");
		}

		if (mif.m_bMifWr) {
			memReq.AddStructField(&g_bool, "m_wrReq");
			if (mif.m_mifWr.m_bPause)
				memReq.AddStructField(&g_bool, "m_wrPause");
			if (mif.m_mifWr.m_bPoll)
				memReq.AddStructField(&g_bool, "m_wrPoll");
			if (mif.m_mifWr.m_bReqPause)
				memReq.AddStructField(&g_bool, "m_reqPause");

			int wrSrcW = mif.m_mifWr.m_wrSrcList.size() == 0 ? 0 : FindLg2(mif.m_mifWr.m_wrSrcList.size(), false);
			if (wrSrcW > 0)
				memReq.AddStructField(&g_uint32, "m_wrSrc", VA("%d", wrSrcW));
		}

		CRecord htIdUnion;
		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			if (bRdRspGrpIdIsHtId || bWrRspGrpIdIsHtId) {
				htIdUnion.m_typeName = "";
				htIdUnion.m_bCStyle = true;
				htIdUnion.m_bUnion = true;

				htIdUnion.AddStructField(&g_uint32, "m_rsmHtId", VA("%d", mod.m_threads.m_htIdW.AsInt()));

				if (bRdRspGrpIdIsHtId && rdRspGrpIdW > 0)
					htIdUnion.AddStructField(&g_uint32, "m_rdGrpId", VA("%d", rdRspGrpIdW));
				if (bWrRspGrpIdIsHtId && wrRspGrpIdW > 0)
					htIdUnion.AddStructField(&g_uint32, "m_wrGrpId", VA("%d", wrRspGrpIdW));

				memReq.AddStructField(&htIdUnion, "");
			} else {
				memReq.AddStructField(&g_uint32, "m_rsmHtId", VA("%d", mod.m_threads.m_htIdW.AsInt()));
			}
		}
		if (!bRdRspGrpIdIsHtId && rdRspGrpIdW > 0)
			memReq.AddStructField(FindHtIntType(eUnsigned, rdRspGrpIdW), "m_rdGrpId");
		if (!bWrRspGrpIdIsHtId && wrRspGrpIdW > 0)
			memReq.AddStructField(FindHtIntType(eUnsigned, wrRspGrpIdW), "m_wrGrpId");

		memReq.AddStructField(FindHtIntType(eUnsigned, mod.m_instrW), "m_rsmInstr");

		if (mif.m_mifRd.m_bPoll || mif.m_mifWr.m_bPoll)
			memReq.AddStructField(FindHtIntType(eUnsigned, mod.m_instrW), "m_pollInstr");

		CRecord wrUnion;
		CRecord wrTypeUnion;
		vector<CRecord> srcRecordList;

		if (mif.m_bMifWr) {

			wrUnion.m_typeName = "";
			wrUnion.m_bCStyle = true;
			wrUnion.m_bUnion = true;

			// per wrSrc fields
			for (size_t wrSrcIdx = 0; wrSrcIdx < mif.m_mifWr.m_wrSrcList.size(); wrSrcIdx += 1) {
				CMifWrSrc &wrSrc = mif.m_mifWr.m_wrSrcList[wrSrcIdx];

				CRecord srcRecord;
				srcRecord.m_typeName = "";
				srcRecord.m_bCStyle = true;
				srcRecord.m_bUnion = false;

				if (wrSrc.m_varAddr1W > 0 && !wrSrc.m_varAddr1IsIdx && !wrSrc.m_varAddr1IsHtId)
					srcRecord.AddStructField(&g_uint32, VA("m_s%d_f1Addr1", wrSrcIdx), VA("%d", wrSrc.m_varAddr1W));

				if (wrSrc.m_varAddr2W > 0 && !wrSrc.m_varAddr2IsIdx && !wrSrc.m_varAddr2IsHtId)
					srcRecord.AddStructField(&g_uint32, VA("m_s%d_f1Addr2", wrSrcIdx), VA("%d", wrSrc.m_varAddr2W));

				for (size_t fldIdx = 0; fldIdx < wrSrc.m_fieldRefList.size(); fldIdx += 1) {
					for (size_t dimIdx = 0; dimIdx < wrSrc.m_fieldRefList[fldIdx].m_refDimenList.size(); dimIdx += 1) {
						CRefDimen & refDimen = wrSrc.m_fieldRefList[fldIdx].m_refDimenList[dimIdx];

						if (refDimen.m_value < 0 && refDimen.m_size > 1 && !refDimen.m_isIdx) {
							int varDimenW = FindLg2(wrSrc.m_fieldRefList[fldIdx].m_refDimenList[dimIdx].m_size - 1);
							srcRecord.AddStructField(&g_uint32, VA("m_s%d_f%dIdx%d",
								wrSrcIdx, (int)fldIdx + 1, (int)dimIdx + 1), VA("%d", varDimenW));
						}
					}
				}

				if (srcRecord.m_fieldList.size() > 0)
					srcRecordList.push_back(srcRecord);
			}

			for (size_t listIdx = 0; listIdx < srcRecordList.size(); listIdx += 1) {
				wrUnion.AddStructField(&srcRecordList[listIdx], "");
			}

			if (wrUnion.m_fieldList.size() > 0) {
				memReq.AddStructField(&wrUnion, "");
			}

			wrTypeUnion.m_typeName = "CMifWrData";
			wrTypeUnion.m_bCStyle = true;
			wrTypeUnion.m_bUnion = true;

			// union of storage for write types
			for (size_t wrSrcIdx = 0; wrSrcIdx < mif.m_mifWr.m_wrSrcList.size(); wrSrcIdx += 1) {
				CMifWrSrc &wrSrc = mif.m_mifWr.m_wrSrcList[wrSrcIdx];

				vector<CHtString> dimenList;
				bool bConstDimenList = true;

				if (wrSrc.m_pPrivVar && !wrSrc.m_bConstDimenList && 
					(wrSrc.m_fieldRefList.size() == 1 && wrSrc.m_fieldRefList[0].m_refAddrList.size() == 0 ||
					wrSrc.m_fieldRefList.size() > 1))
				{
					vector<CRefDimen> & refDimenList = wrSrc.m_fieldRefList.back().m_refDimenList;

					for (size_t i = 0; i < refDimenList.size(); i += 1) {
						CHtString dimStr = refDimenList[i].m_size;
						dimenList.push_back(dimStr);

						if (refDimenList[i].m_value < 0)
							bConstDimenList = false;
					}
				}

				if (bConstDimenList)
					dimenList.clear();

				size_t wrSrcTypeIdx;
				for (wrSrcTypeIdx = 0; wrSrcTypeIdx < wrDataTypeList.size(); wrSrcTypeIdx += 1) {
					if (wrSrc.m_wrDataTypeName == wrDataTypeList[wrSrcTypeIdx])
						break;
				}

				if (wrSrcTypeIdx < wrDataTypeList.size()) continue;

				wrDataTypeList.push_back(wrSrc.m_wrDataTypeName);

				for (wrSrcTypeIdx = 0; wrSrcTypeIdx < wrSrcTypeList.size(); wrSrcTypeIdx += 1) {
					if (wrSrc.m_pSrcType == wrSrcTypeList[wrSrcTypeIdx])
						break;
				}

				if (wrSrcTypeIdx == wrSrcTypeList.size())
					wrSrcTypeList.push_back(wrSrc.m_pSrcType);

				if (mif.m_mifReqStgCnt == 2 && wrSrc.m_wrDataTypeName != wrSrc.m_pSrcType->m_typeName) {

					if (wrSrc.m_pSrcType->m_clangMinAlign == 1)
						wrTypeUnion.AddStructField(&g_uint64, VA("m_%s", wrSrc.m_pSrcType->m_typeName.c_str()), VA("%d", wrSrc.m_pSrcType->m_clangBitWidth));
					else
						wrTypeUnion.AddStructField(wrSrc.m_pSrcType, VA("m_%s", wrSrc.m_pSrcType->m_typeName.c_str()));

					wrDataTypeList.push_back(wrSrc.m_pSrcType->m_typeName);
				}

				if (wrSrc.m_pSrcType->m_clangMinAlign == 1)
					wrTypeUnion.AddStructField(&g_uint64, VA("m_%s", wrSrc.m_wrDataTypeName.c_str()), VA("%d", wrSrc.m_pSrcType->m_clangBitWidth));
				else
					wrTypeUnion.AddStructField(wrSrc.m_pSrcType, VA("m_%s", wrSrc.m_wrDataTypeName.c_str()), "", "", dimenList);
			}

			if (wrTypeUnion.m_fieldList.size() > 0)
				memReq.AddStructField(&wrTypeUnion, "m_wrData");
		}

		if (memReq.m_fieldList.size() == 0)
			memReq.AddStructField(&g_uint32, "m_pad", "1");

		if (wrTypeUnion.m_fieldList.size() > 0)
			GenUserStructs(m_mifDecl, &wrTypeUnion, "\t");

		GenUserStructs(m_mifDecl, &memReq, "\t");
	}

	if (bMultiQwReq) {
		GenModDecl(eVcdAll, m_mifDecl, vcdModName, "bool", "r_memReqHold");
		mifReg.Append("\tr_memReqHold = !%s && c_memReqHold;\n", reset.c_str());
	}

	{
		GenModDecl(eVcdAll, m_mifDecl, vcdModName, VA("ht_uint%d", mif.m_queueW + 1), VA("r_%sToMif_reqAvlCnt", mod.m_modName.Lc().c_str()));
		mifReg.Append("\tr_%sToMif_reqAvlCnt = %s ? (ht_uint%d)%d : c_%sToMif_reqAvlCnt;\n",
			mod.m_modName.Lc().c_str(),
			reset.c_str(),
			mif.m_queueW + 1,
			1 << mif.m_queueW,
			mod.m_modName.Lc().c_str());

		m_mifAvlCntChk.Append("\t\tht_assert(r_%sToMif_reqAvlCnt == %d);\n",
			mod.m_modName.Lc().c_str(), 1 << mif.m_queueW);
	}

	{
		GenModDecl(eVcdAll, m_mifDecl, vcdModName, "bool", VA("r_%sToMif_reqAvlCntBusy", mod.m_modName.Lc().c_str()));
		mifReg.Append("\tr_%sToMif_reqAvlCntBusy = !%s && c_%sToMif_reqAvlCntBusy;\n",
			mod.m_modName.Lc().c_str(),
			reset.c_str(),
			mod.m_modName.Lc().c_str());
		mifReg.NewLine();
	}

	if (bMultiQwReq) {
		GenModDecl(eVcdAll, m_mifDecl, vcdModName, "bool", VA("r_t%d_memReqBusy", mod.m_execStg));
		mifReg.Append("\tr_t%d_memReqBusy = !%s && c_t%d_memReqBusy;\n",
			mod.m_execStg,
			reset.c_str(),
			mod.m_execStg - 1);
	}
	m_mifDecl.NewLine();

	if (bMultiQwReq) {
		GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CMemReq", VA("r_t%d_memReqLas", mod.m_execStg + 1));
		mifReg.Append("\tr_t%d_memReqLas = c_t%d_memReqLas;\n",
			mod.m_execStg + 1,
			mod.m_execStg);
	}

	{
		m_mifDecl.Append("\tCMemReq c_t%d_memReq;\n", mod.m_execStg, mod.m_modName.Lc().c_str());
		mifPreInstr.Append("\tc_t%d_memReq = 0;\n",
			mod.m_execStg);
		if (mif.m_bMifRd) {
			m_mifCtorInit.Append("\t\tr_t%d_memReq.m_rdReq = false;\n", mod.m_execStg + 1);
			if (mif.m_mifRd.m_bPause)
				m_mifCtorInit.Append("\t\tr_t%d_memReq.m_rdPause = false;\n", mod.m_execStg + 1);
			if (mif.m_mifRd.m_bPoll)
				m_mifCtorInit.Append("\t\tr_t%d_memReq.m_rdPoll = false;\n", mod.m_execStg + 1);
		}
		if (mif.m_bMifWr) {
			m_mifCtorInit.Append("\t\tr_t%d_memReq.m_wrReq = false;\n", mod.m_execStg + 1);
			if (mif.m_mifWr.m_bPause)
				m_mifCtorInit.Append("\t\tr_t%d_memReq.m_wrPause = false;\n", mod.m_execStg + 1);
			if (mif.m_mifWr.m_bPoll)
				m_mifCtorInit.Append("\t\tr_t%d_memReq.m_wrPoll = false;\n", mod.m_execStg + 1);
		}
		mifPreInstr.NewLine();

		GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CMemReq", VA("r_t%d_memReq", mod.m_execStg + 1));
		mifReg.Append("\tr_t%d_memReq = c_t%d_memReq;\n",
			mod.m_execStg + 1,
			mod.m_execStg);
	}

	if (mif.m_mifReqStgCnt == 2 || bMultiQwReq && mif.m_mifReqStgCnt >= 1) {
		GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CMemReq", VA("r_t%d_memReq", mod.m_execStg + 2));
		mifReg.Append("\tr_t%d_memReq = c_t%d_memReq;\n",
			mod.m_execStg + 2,
			mod.m_execStg + 1);
		mifReg.NewLine();
	}

	if (bMultiQwReq && mif.m_bMifRd && bNeedRdRspInfo) {
		GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CRdRspInfo", VA("r_t%d_rdRspInfoLas", mod.m_execStg + 1));
		mifReg.Append("\tr_t%d_rdRspInfoLas = c_t%d_rdRspInfoLas;\n",
			mod.m_execStg + 1,
			mod.m_execStg);
	}

	if (mif.m_bMifRd && bNeedRdRspInfo) {
		m_mifDecl.Append("\tCRdRspInfo c_t%d_rdRspInfo;\n", mod.m_execStg, mod.m_modName.Lc().c_str());
		if (bMultiQwRdMif) {
			mifPreInstr.Append("\tc_t%d_rdRspInfo = r_t%d_rdRspInfo;\n",
				mod.m_execStg,
				mod.m_execStg + 1);
		} else {
			mifPreInstr.Append("\tc_t%d_rdRspInfo = 0;\n",
				mod.m_execStg);
		}
		mifPreInstr.NewLine();

		if (mif.m_mifReqStgCnt > 0 || mif.m_mifRd.m_rspGrpIdW > 0) {
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CRdRspInfo", VA("r_t%d_rdRspInfo", mod.m_execStg + 1));
			mifReg.Append("\tr_t%d_rdRspInfo = c_t%d_rdRspInfo;\n",
				mod.m_execStg + 1,
				mod.m_execStg);
			mifReg.NewLine();
		}
	}
	mifReg.NewLine();
	m_mifDecl.NewLine();

	if (bMultiQwReq) {
		mifPreInstr.NewLine();

		GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CMemRdWrReqIntf", VA("r_t%d_%sToMif_reqLas", mod.m_execStg + 1, mod.m_modName.Lc().c_str()));
		mifReg.Append("\tr_t%d_%sToMif_reqLas = c_t%d_%sToMif_reqLas;\n",
			mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
			mod.m_execStg, mod.m_modName.Lc().c_str());
	}

	for (int stg = 0; stg <= mif.m_mifReqStgCnt; stg += 1) {
		if (stg == 0 && mif.m_mifReqStgCnt == 0) {
			m_mifDecl.Append("\tbool c_t%d_%sToMif_reqRdy;\n",
				mod.m_execStg + stg, mod.m_modName.Lc().c_str());

			mifPreInstr.Append("\tc_t%d_%sToMif_reqRdy = false;\n",
				mod.m_execStg + stg, mod.m_modName.Lc().c_str());
		}
		if (stg > 0 || mif.m_mifReqStgCnt == 0) {
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "bool", VA("r_t%d_%sToMif_reqRdy", mod.m_execStg + 1 + stg, mod.m_modName.Lc().c_str()));
			mifReg.Append("\tr_t%d_%sToMif_reqRdy = !%s && c_t%d_%sToMif_reqRdy;\n",
				mod.m_execStg + 1 + stg, mod.m_modName.Lc().c_str(),
				reset.c_str(),
				mod.m_execStg + stg, mod.m_modName.Lc().c_str());
		}
		if (stg == 0) {
			m_mifDecl.Append("\tCMemRdWrReqIntf c_t%d_%sToMif_req;\n",
				mod.m_execStg + stg, mod.m_modName.Lc().c_str());
			mifPreInstr.Append("\tc_t%d_%sToMif_req = 0;\n",
				mod.m_execStg + stg, mod.m_modName.Lc().c_str());
		}
		mifPreInstr.NewLine();

		GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CMemRdWrReqIntf", VA("r_t%d_%sToMif_req", mod.m_execStg + 1 + stg, mod.m_modName.Lc().c_str()));
		mifReg.Append("\tr_t%d_%sToMif_req = c_t%d_%sToMif_req;\n",
			mod.m_execStg + 1 + stg, mod.m_modName.Lc().c_str(),
			mod.m_execStg + stg, mod.m_modName.Lc().c_str());
	}

	if (mif.m_bMifRd) {
		GenModDecl(eVcdUser, m_mifDecl, vcdModName, "bool", VA("r_%sToMif_rdRspFull", mod.m_modName.Lc().c_str()));

		mifPreInstr.Append("\tbool c_%sToMif_rdRspFull = ", mod.m_modName.Lc().c_str());

		string separator;
		//if (mod.m_threads.m_htIdW.AsInt() > 0 || bMultiQwRdMif) {
			for (size_t gvIdx = 0; gvIdx < mod.m_ngvList.size(); gvIdx += 1) {
				CRam * pGv = mod.m_ngvList[gvIdx];

				if (!pGv->m_bWriteForMifRead) continue;
				if (!pGv->m_pNgvInfo->m_bNeedQue) continue;

				vector<int> refList(pGv->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					mifPreInstr.Append("%si_%sTo%s_mwFull%s",
						separator.c_str(), pGv->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());

					separator = " || ";
				} while (DimenIter(pGv->m_dimenList, refList));
			}
		//}

		if (separator.size() == 0)
			mifPreInstr.Append("false");
		mifPreInstr.Append(";\n");

		mifReg.Append("\tr_%sToMif_rdRspFull = c_%sToMif_rdRspFull;\n",
			mod.m_modName.Lc().c_str(),
			mod.m_modName.Lc().c_str());
	}
	mifReg.NewLine();
	m_mifDecl.NewLine();

	if (mif.m_bMifRd) {
		// Memory read state declaration
		int rdRspStg = mod.m_mif.m_mifRd.m_bNeedRdRspInfoRam ? 2 : 1;

		if (rdRspGrpIdW <= 2) {

			CRecord reqState;
			reqState.m_typeName = "CRdGrpState";
			reqState.m_bCStyle = true;

			if (mif.m_mifRd.m_bPause) {
				reqState.AddStructField(&g_bool, "m_pause");
				reqState.m_fieldList.back()->InitDimen(mod.m_threads.m_lineInfo);
			}

			if (!bRdRspGrpIdIsHtId && mod.m_threads.m_htIdW.AsInt() > 0)
				reqState.AddStructField(&g_uint32, "m_rsmHtId", VA("%d", mod.m_threads.m_htIdW.AsInt()));

			if (mod.m_instrW > 0) {
				reqState.AddStructField(FindHtIntType(eUnsigned, mod.m_instrW), "m_rsmInstr");
				reqState.m_fieldList.back()->InitDimen(mod.m_threads.m_lineInfo);
			}

			reqState.AddStructField(FindHtIntType(eUnsigned, mod.m_mif.m_mifRd.m_rspCntW.AsInt()), "m_cnt");

			GenUserStructs(m_mifDecl, &reqState, "\t");

		} else {
			;
			{
				CRecord reqState;
				reqState.m_typeName = "CRdGrpReqState";
				reqState.m_bCStyle = true;

				if (mif.m_mifRd.m_bPause) {
					reqState.AddStructField(&g_bool, "m_pause");
					reqState.m_fieldList.back()->InitDimen(mod.m_threads.m_lineInfo);
				}

				if (!bRdRspGrpIdIsHtId && mod.m_threads.m_htIdW.AsInt() > 0)
					reqState.AddStructField(&g_uint32, "m_rsmHtId", VA("%d", mod.m_threads.m_htIdW.AsInt()));

				if (mod.m_instrW > 0) {
					reqState.AddStructField(FindHtIntType(eUnsigned, mod.m_instrW), "m_rsmInstr");
					reqState.m_fieldList.back()->InitDimen(mod.m_threads.m_lineInfo);
				}

				reqState.AddStructField(FindHtIntType(eUnsigned, mif.m_mifRd.m_rspCntW.AsInt()), "m_cnt");

				GenUserStructs(m_mifDecl, &reqState, "\t");
			}

			{
				CRecord rspState;
				rspState.m_typeName = "CRdGrpRspState";
				rspState.m_bCStyle = true;

				if (mif.m_mifRd.m_bPause) {
					rspState.AddStructField(&g_bool, "m_pause");
					rspState.m_fieldList.back()->InitDimen(mod.m_threads.m_lineInfo);
				}

				rspState.AddStructField(FindHtIntType(eUnsigned, mif.m_mifRd.m_rspCntW.AsInt()), "m_cnt");

				GenUserStructs(m_mifDecl, &rspState, "\t");
			}
		}

		if (rdRspGrpIdW > 2) {
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, VA("sc_uint<%s_RD_GRP_ID_W + 1>", mod.m_modName.Upper().c_str()),
				"r_rdRspGrpInitCnt");
			mifReg.Append("\tr_rdRspGrpInitCnt = %s ? (sc_uint<%s_RD_GRP_ID_W + 1>)0 : c_rdRspGrpInitCnt;\n",
				reset.c_str(),
				mod.m_modName.Upper().c_str());
		}

		if (bNeedRdRspInfoRam) {
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, VA("sc_uint<%s_RD_RSP_CNT_W + 1>", mod.m_modName.Upper().c_str()),
				"r_rdRspIdInit");
			mifReg.Append("\tr_rdRspIdInit = %s ? (sc_uint<%s_RD_RSP_CNT_W + 1>)0 : c_rdRspIdInit;\n",
				reset.c_str(),
				mod.m_modName.Upper().c_str());
		}

		{
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "bool", "r_rdGrpRspCntBusy");
			mifReg.Append("\tr_rdGrpRspCntBusy = c_rdGrpRspCntBusy;\n");
		}

		{
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "bool", "r_m1_rdRspRdy");
			mifReg.Append("\tr_m1_rdRspRdy = !%s && c_m0_rdRspRdy;\n",
				reset.c_str());

			m_mifCtorInit.Append("\t\tr_m1_rdRspRdy = false;\n");
		}

		{
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CMemRdRspIntf", "r_m1_rdRsp");
			mifReg.Append("\tr_m1_rdRsp = c_m0_rdRsp;\n");
		}

		if (bNeedRdRspInfoRam) {
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "bool", "r_m2_rdRspRdy");
			mifReg.Append("\tr_m2_rdRspRdy = c_m1_rdRspRdy;\n");

			m_mifCtorInit.Append("\t\tr_m2_rdRspRdy = false;\n");
		}

		if (bNeedRdRspInfoRam) {
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CMemRdRspIntf", "r_m2_rdRsp");
			mifReg.Append("\tr_m2_rdRsp = c_m1_rdRsp;\n");
		}

		if (bNeedRdRspInfo) {
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CRdRspInfo", VA("r_m%d_rdRspInfo", rdRspStg));
			mifReg.Append("\tr_m%d_rdRspInfo = c_m%d_rdRspInfo;\n", rdRspStg, rdRspStg - 1);
		}

		if (rdRspGrpIdW > 0 && rdDstNgvRamList.size() > 0 && bNeedRdRspInfo) {
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CRdRspInfo", VA("r_m%d_rdRspInfo", rdRspStg + 1));
			mifReg.Append("\tr_m%d_rdRspInfo = c_m%d_rdRspInfo;\n", rdRspStg + 1, rdRspStg);
		}

		if (bMultiQwReq) {
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "ht_uint3", VA("r_m%d_rdRspQwIdx", rdRspStg));
			mifReg.Append("\tr_m%d_rdRspQwIdx = %s ? (ht_uint3)0 : c_m%d_rdRspQwIdx;\n", rdRspStg, reset.c_str(), rdRspStg - 1);
		}
		mifReg.NewLine();

		if (rdDstNgvRamList.size() > 0) {
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "bool", "r_rdCompRdy");
			mifReg.Append("\tr_rdCompRdy = c_rdCompRdy;\n");
		}

		if (rdDstNgvRamList.size() > 0 && rdRspGrpIdW > 0) {
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, VA("ht_uint%d", mif.m_mifRd.m_rspGrpW.AsInt()), "r_rdCompGrpId");
			mifReg.Append("\tr_rdCompGrpId = c_rdCompGrpId;\n");
		}

		if (rdDstRdyCnt > 1 || mod.m_clkRate == eClk1x && bNgvWrCompClk2x) {
			if (rdRspGrpIdW == 0) {

				for (size_t i = 0; i < rdDstNgvRamList.size(); i += 1) {
					m_mifDecl.Append("\tht_uint%d c_%sTo%s_mwCompCnt%s;\n",
						FindLg2(rdDstRdyCnt + 1),
						rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), rdDstNgvRamList[i]->m_dimenDecl.c_str());

					GenModDecl(eVcdAll, m_mifDecl, vcdModName, VA("ht_uint%d", FindLg2(rdDstRdyCnt + 1)), VA("r_%sTo%s_mwCompCnt",
						rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), rdDstNgvRamList[i]->m_dimenList);

					if (mod.m_clkRate == eClk1x && rdDstNgvRamList[i]->m_pNgvInfo->m_bNgvWrCompClk2x) {
						m_mifDecl.Append("\tht_uint%d c_%sTo%s_mwCompCnt_2x%s;\n",
							FindLg2(rdDstRdyCnt + 1),
							rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), rdDstNgvRamList[i]->m_dimenDecl.c_str());

						GenModDecl(eVcdAll, m_mifDecl, vcdModName, VA("sc_signal<ht_uint%d>", FindLg2(rdDstRdyCnt + 1)), VA("r_%sTo%s_mwCompCnt_2x",
							rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str()), rdDstNgvRamList[i]->m_dimenList);
					}

					vector<int> refList(rdDstNgvRamList[i]->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						mifReg.Append("\tr_%sTo%s_mwCompCnt%s = c_%sTo%s_mwCompCnt%s;\n",
							rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());

						mifReset.Append("\t\tc_%sTo%s_mwCompCnt%s = 0;\n",
							rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());

						if (mod.m_clkRate == eClk1x && rdDstNgvRamList[i]->m_pNgvInfo->m_bNgvWrCompClk2x) {
							m_mifReg2x.Append("\tr_%sTo%s_mwCompCnt_2x%s = c_%sTo%s_mwCompCnt_2x%s;\n",
								rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());

							m_mifReset2x.Append("\t\tc_%sTo%s_mwCompCnt_2x%s = 0;\n",
								rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
						}

					} while (DimenIter(rdDstNgvRamList[i]->m_dimenList, refList));
				}
			} else {
				mifReg.NewLine();
				for (size_t i = 0; i < rdDstNgvRamList.size(); i += 1) {
					CHtCode & mifRegMwComp = rdDstNgvRamList[i]->m_pNgvInfo->m_bNgvWrCompClk2x ? m_mifReg2x : m_mifReg1x;
					string resetMwComp = rdDstNgvRamList[i]->m_pNgvInfo->m_bNgvWrCompClk2x ? "c_reset1x" : "r_reset1x";

					m_mifDecl.Append("\tht_dist_que<ht_uint%d, 5> m_%sTo%s_mwCompQue%s;\n",
						mif.m_mifRd.m_rspGrpW.AsInt(),
						rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), rdDstNgvRamList[i]->m_dimenDecl.c_str());

					vector<int> refList(rdDstNgvRamList[i]->m_dimenList.size());
					do {
						string dimIdx = IndexStr(refList);

						if (mod.m_clkRate == eClk1x && rdDstNgvRamList[i]->m_pNgvInfo->m_bNgvWrCompClk2x) {
							mifReg.Append("\tm_%sTo%s_mwCompQue%s.pop_clock(%s);\n",
								rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								reset.c_str());

							mifRegMwComp.Append("\tm_%sTo%s_mwCompQue%s.push_clock(%s);\n",
								rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								resetMwComp.c_str());
						} else {
							mifReg.Append("\tm_%sTo%s_mwCompQue%s.clock(%s);\n",
								rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								reset.c_str());
						}
					} while (DimenIter(rdDstNgvRamList[i]->m_dimenList, refList));
				}
			}
		}
		mifReg.NewLine();

		if (rdRspGrpIdW == 0) {

			m_mifDecl.Append("\tCRdGrpState c_rdGrpState;\n");
			mifPreInstr.Append("\tc_rdGrpState = r_rdGrpState;\n");

			if (mif.m_mifRd.m_bPause)
				mifReset.Append("\t\tc_rdGrpState.m_pause = false;\n");
			mifReset.Append("\t\tc_rdGrpState.m_cnt = 0;\n");

			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CRdGrpState", "r_rdGrpState");
			mifReg.Append("\tr_rdGrpState = c_rdGrpState;\n");
			mifReg.NewLine();

		} else if (rdRspGrpIdW <= 2) {

			m_mifDecl.Append("\tsc_uint<%s_RD_RSP_CNT_W> c_rdGrpBusyCnt;\n", mod.m_modName.Upper().c_str());
			mifPreInstr.Append("\tc_rdGrpBusyCnt = r_rdGrpBusyCnt;\n");
			mifPreInstr.NewLine();

			GenModDecl(eVcdAll, m_mifDecl, vcdModName, VA("sc_uint<%s_RD_RSP_CNT_W>", mod.m_modName.Upper().c_str()), "r_rdGrpBusyCnt");
			mifReg.Append("\tr_rdGrpBusyCnt = %s ? (sc_uint<%s_RD_RSP_CNT_W>)0 : c_rdGrpBusyCnt;\n",
				reset.c_str(), mod.m_modName.Upper().c_str());
			mifReg.NewLine();

			vector<CHtString> dimVec;
			CHtString dimStr = 1 << rdRspGrpIdW;
			dimVec.push_back(dimStr);

			m_mifDecl.Append("\tCRdGrpState c_rdGrpState[%d];\n",
				1 << rdRspGrpIdW);
			for (int idx = 0; idx < (1 << rdRspGrpIdW); idx += 1)
				mifPreInstr.Append("\tc_rdGrpState[%d] = r_rdGrpState[%d];\n", idx, idx);

			for (int idx = 0; idx < (1 << rdRspGrpIdW); idx += 1) {
				if (mif.m_mifRd.m_bPause)
					mifReset.Append("\t\tc_rdGrpState[%d].m_pause = false;\n", idx);
				mifReset.Append("\t\tc_rdGrpState[%d].m_cnt = 0;\n", idx);
			}

			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CRdGrpState", "r_rdGrpState", dimVec);
			for (int idx = 0; idx < (1 << rdRspGrpIdW); idx += 1) {
				mifReg.Append("\tr_rdGrpState[%d] = c_rdGrpState[%d];\n",
					idx,
					idx);
			}
			mifReg.NewLine();

		} else {

			m_mifDecl.Append("\tsc_uint<%s_RD_RSP_CNT_W> c_rdGrpBusyCnt;\n", mod.m_modName.Upper().c_str());
			mifPreInstr.Append("\tc_rdGrpBusyCnt = r_rdGrpBusyCnt;\n");

			GenModDecl(eVcdAll, m_mifDecl, vcdModName, VA("sc_uint<%s_RD_RSP_CNT_W>", mod.m_modName.Upper().c_str()), "r_rdGrpBusyCnt");
			mifReg.Append("\tr_rdGrpBusyCnt = %s ? (sc_uint<%s_RD_RSP_CNT_W>)0 : c_rdGrpBusyCnt;\n",
				reset.c_str(), mod.m_modName.Upper().c_str());
			mifReg.NewLine();

			m_mifDecl.Append("\tht_dist_ram<CRdGrpReqState, %s_RD_GRP_ID_W> m_rdGrpReqState0;\n",
				mod.m_modName.Upper().c_str());
			if (mif.m_mifRd.m_bPause) {
				m_mifDecl.Append("\tht_dist_ram<CRdGrpReqState, %s_RD_GRP_ID_W> m_rdGrpReqState1;\n",
					mod.m_modName.Upper().c_str());
			}
			m_mifDecl.Append("\tht_dist_ram<CRdGrpRspState, %s_RD_GRP_ID_W> m_rdGrpRspState0;\n",
				mod.m_modName.Upper().c_str());
			m_mifDecl.Append("\tht_dist_ram<CRdGrpRspState, %s_RD_GRP_ID_W> m_rdGrpRspState1;\n",
				mod.m_modName.Upper().c_str());

			mifReg.Append("\tm_rdGrpReqState0.clock();\n");
			if (mif.m_mifRd.m_bPause)
				mifReg.Append("\tm_rdGrpReqState1.clock();\n");
			mifReg.Append("\tm_rdGrpRspState0.clock();\n");
			mifReg.Append("\tm_rdGrpRspState1.clock();\n");
			mifReg.NewLine();
		}

		if (bNeedRdRspInfoRam) {
			m_mifDecl.Append("\tht_block_que<sc_uint<%s_RD_RSP_CNT_W>, %s_RD_RSP_CNT_W> m_rdRspIdPool;\n",
				mod.m_modName.Upper().c_str(),
				mod.m_modName.Upper().c_str());
			m_mifDecl.Append("\tht_block_ram<CRdRspInfo, %s_RD_RSP_CNT_W> m_rdRspInfo;\n",
				mod.m_modName.Upper().c_str());

			mifReg.Append("\tm_rdRspIdPool.clock(%s);\n", reset.c_str());
			mifReg.Append("\tm_rdRspInfo.clock();\n");
			mifReg.NewLine();
		}
	}


	if (mif.m_bMifWr) {
		;// Memory write state declaration
		if (wrRspGrpIdW <= 2) {

			CRecord reqState;
			reqState.m_typeName = "CWrGrpState";
			reqState.m_bCStyle = true;

			if (mif.m_mifWr.m_bPause) {
				reqState.AddStructField(&g_bool, "m_pause");
				reqState.m_fieldList.back()->InitDimen(mod.m_threads.m_lineInfo);
			}

			if (mod.m_instrW > 0) {
				reqState.AddStructField(FindHtIntType(eUnsigned, mod.m_instrW), "m_rsmInstr");
				reqState.m_fieldList.back()->InitDimen(mod.m_threads.m_lineInfo);
			}

			if (!bWrRspGrpIdIsHtId && mod.m_threads.m_htIdW.AsInt() > 0)
				reqState.AddStructField(&g_uint32, "m_rsmHtId", VA("%d", mod.m_threads.m_htIdW.AsInt()));

			reqState.AddStructField(FindHtIntType(eUnsigned, mod.m_mif.m_mifWr.m_rspCntW.AsInt()), "m_cnt");

			GenUserStructs(m_mifDecl, &reqState, "\t");

		} else {
			;
			{
				CRecord reqState;
				reqState.m_typeName = "CWrGrpReqState";
				reqState.m_bCStyle = true;

				if (mif.m_mifWr.m_bPause) {
					reqState.AddStructField(&g_bool, "m_pause");
					reqState.m_fieldList.back()->InitDimen(mod.m_threads.m_lineInfo);
				}

				if (mod.m_instrW > 0) {
					reqState.AddStructField(FindHtIntType(eUnsigned, mod.m_instrW), "m_rsmInstr");
					reqState.m_fieldList.back()->InitDimen(mod.m_threads.m_lineInfo);
				}

				if (!bWrRspGrpIdIsHtId && mod.m_threads.m_htIdW.AsInt() > 0)
					reqState.AddStructField(&g_uint32, "m_rsmHtId", VA("%d", mod.m_threads.m_htIdW.AsInt()));

				reqState.AddStructField(FindHtIntType(eUnsigned, mod.m_mif.m_mifWr.m_rspCntW.AsInt()), "m_cnt");

				GenUserStructs(m_mifDecl, &reqState, "\t");
			}

			{
				CRecord rspState;
				rspState.m_typeName = "CWrGrpRspState";
				rspState.m_bCStyle = true;

				if (mif.m_mifWr.m_bPause) {
					rspState.AddStructField(&g_bool, "m_pause");
					rspState.m_fieldList.back()->InitDimen(mod.m_threads.m_lineInfo);
				}

				rspState.AddStructField(FindHtIntType(eUnsigned, mod.m_mif.m_mifWr.m_rspCntW.AsInt()), "m_cnt");

				GenUserStructs(m_mifDecl, &rspState, "\t");
			}
		}

		if (wrRspGrpIdW > 2) {
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, VA("sc_uint<%s_WR_GRP_ID_W + 1>", mod.m_modName.Upper().c_str()),
				"r_wrRspGrpInitCnt");
			mifReg.Append("\tr_wrRspGrpInitCnt = %s ? (sc_uint<%s_WR_GRP_ID_W + 1>)0 : c_wrRspGrpInitCnt;\n",
				reset.c_str(),
				mod.m_modName.Upper().c_str());
		}

		{
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "bool", "r_wrGrpRspCntBusy");
			mifReg.Append("\tr_wrGrpRspCntBusy = c_wrGrpRspCntBusy;\n");
		}

		{
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "bool", "r_m1_wrRspRdy");
			mifReg.Append("\tr_m1_wrRspRdy = !%s && c_m0_wrRspRdy;\n",
				reset.c_str());

			m_mifCtorInit.Append("\t\tr_m1_wrRspRdy = false;\n");
		}

		if (wrRspGrpIdW > 0) {
			GenModDecl(eVcdAll, m_mifDecl, vcdModName, VA("sc_uint<%s_WR_GRP_ID_W>", mod.m_modName.Upper().c_str()), "r_m1_wrGrpId");
			mifReg.Append("\tr_m1_wrGrpId = c_m0_wrGrpId;\n");
			mifReg.NewLine();
		}

		if (wrRspGrpIdW == 0) {

			m_mifDecl.Append("\tCWrGrpState c_wrGrpState;\n");
			mifPreInstr.Append("\tc_wrGrpState = r_wrGrpState;\n");

			if (mif.m_mifWr.m_bPause)
				mifReset.Append("\t\tc_wrGrpState.m_pause = false;\n");
			mifReset.Append("\t\tc_wrGrpState.m_cnt = 0;\n");

			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CWrGrpState", "r_wrGrpState");
			mifReg.Append("\tr_wrGrpState = c_wrGrpState;\n");
			mifReg.NewLine();

		} else if (wrRspGrpIdW <= 2) {

			m_mifDecl.Append("\tsc_uint<%s_WR_RSP_CNT_W> c_wrGrpBusyCnt;\n", mod.m_modName.Upper().c_str());
			mifPreInstr.Append("\tc_wrGrpBusyCnt = r_wrGrpBusyCnt;\n");
			mifPreInstr.NewLine();

			GenModDecl(eVcdAll, m_mifDecl, vcdModName, VA("sc_uint<%s_WR_RSP_CNT_W>", mod.m_modName.Upper().c_str()), "r_wrGrpBusyCnt");
			mifReg.Append("\tr_wrGrpBusyCnt = %s ? (sc_uint<%s_WR_RSP_CNT_W>)0 : c_wrGrpBusyCnt;\n",
				reset.c_str(), mod.m_modName.Upper().c_str());
			mifReg.NewLine();

			m_mifDecl.Append("\tCWrGrpState c_wrGrpState[%d];\n",
				1 << wrRspGrpIdW);
			for (int idx = 0; idx < (1 << wrRspGrpIdW); idx += 1)
				mifPreInstr.Append("\tc_wrGrpState[%d] = r_wrGrpState[%d];\n", idx, idx);

			for (int idx = 0; idx < (1 << wrRspGrpIdW); idx += 1) {
				if (mif.m_mifWr.m_bPause)
					mifReset.Append("\t\tc_wrGrpState[%d].m_pause = false;\n", idx);
				mifReset.Append("\t\tc_wrGrpState[%d].m_cnt = 0;\n", idx);
			}

			vector<CHtString> dimVec;
			CHtString dimStr = 1 << wrRspGrpIdW;
			dimVec.push_back(dimStr);

			GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CWrGrpState", "r_wrGrpState", dimVec);
			for (int idx = 0; idx < (1 << wrRspGrpIdW); idx += 1) {
				mifReg.Append("\tr_wrGrpState[%d] = c_wrGrpState[%d];\n",
					idx,
					idx);
			}
			mifReg.NewLine();

		} else {

			m_mifDecl.Append("\tsc_uint<%s_WR_RSP_CNT_W> c_wrGrpBusyCnt;\n", mod.m_modName.Upper().c_str());
			mifPreInstr.Append("\tc_wrGrpBusyCnt = r_wrGrpBusyCnt;\n");

			GenModDecl(eVcdAll, m_mifDecl, vcdModName, VA("sc_uint<%s_WR_RSP_CNT_W>", mod.m_modName.Upper().c_str()), "r_wrGrpBusyCnt");
			mifReg.Append("\tr_wrGrpBusyCnt = %s ? (sc_uint<%s_WR_RSP_CNT_W>)0 : c_wrGrpBusyCnt;\n",
				reset.c_str(), mod.m_modName.Upper().c_str());
			mifReg.NewLine();

			m_mifDecl.Append("\tht_dist_ram<CWrGrpReqState, %s_WR_GRP_ID_W> m_wrGrpReqState0;\n",
				mod.m_modName.Upper().c_str());
			if (mif.m_mifWr.m_bPause) {
				m_mifDecl.Append("\tht_dist_ram<CWrGrpReqState, %s_WR_GRP_ID_W> m_wrGrpReqState1;\n",
					mod.m_modName.Upper().c_str());
			}
			m_mifDecl.Append("\tht_dist_ram<CWrGrpRspState, %s_WR_GRP_ID_W> m_wrGrpRspState0;\n",
				mod.m_modName.Upper().c_str());
			m_mifDecl.Append("\tht_dist_ram<CWrGrpRspState, %s_WR_GRP_ID_W> m_wrGrpRspState1;\n",
				mod.m_modName.Upper().c_str());

			mifReg.Append("\tm_wrGrpReqState0.clock();\n");
			if (mif.m_mifWr.m_bPause)
				mifReg.Append("\tm_wrGrpReqState1.clock();\n");
			mifReg.Append("\tm_wrGrpRspState0.clock();\n");
			mifReg.Append("\tm_wrGrpRspState1.clock();\n");
			mifReg.NewLine();
		}
	}

	string or_memReqHold;
	string andNot_memReqHold;
	if (bMultiQwReq) {
		or_memReqHold = " || r_memReqHold";
		andNot_memReqHold = " && !r_memReqHold";
	}

	if (mif.m_bMifRd) {
		mifPostInstr.Append("\tassert_msg(c_reset1x || !c_t%d_memReq.m_valid || !c_t%d_memReq.m_rdReq%s || c_t%d_bReadMemAvail,"
			" \"Runtime check failed in CPers%s::Pers%s_%s()"
			" - expected ReadMemBusy() to have been called and not busy\");\n",
			mod.m_execStg,
			mod.m_execStg,
			or_memReqHold.c_str(),
			mod.m_execStg,
			mod.m_modName.Uc().c_str(),
			mod.m_modName.Uc().c_str(), mod.m_clkRate == eClk1x ? "1x" : "2x");
	}

	if (mif.m_bMifWr) {
		mifPostInstr.Append("\tassert_msg(c_reset1x || !c_t%d_memReq.m_valid || !c_t%d_memReq.m_wrReq%s || c_t%d_bWriteMemAvail,"
			" \"Runtime check failed in CPers%s::Pers%s_%s()"
			" - expected WriteMemBusy() to have been called and not busy\");\n",
			mod.m_execStg,
			mod.m_execStg,
			or_memReqHold.c_str(),
			mod.m_execStg,
			mod.m_modName.Uc().c_str(),
			mod.m_modName.Uc().c_str(), mod.m_clkRate == eClk1x ? "1x" : "2x");
	}
	mifPostInstr.NewLine();

	if (mif.m_bMifRd) {
		if (rdRspGrpIdW == 0) {
			mifPostInstr.Append("\tbool c_rdGrpRspCntBusy = r_rdGrpState.m_cnt >= ((1ul << %s_RD_RSP_CNT_W) - %d);\n",
				mod.m_modName.Upper().c_str(),
				bMultiQwMif ? 11 : 4);
		} else {
			mifPostInstr.Append("\tbool c_rdGrpRspCntBusy = r_rdGrpBusyCnt >= ((1ul << %s_RD_RSP_CNT_W) - %d);\n",
				mod.m_modName.Upper().c_str(),
				bMultiQwMif ? 11 : 4);
		}
		mifPostInstr.NewLine();
	}

	if (mif.m_bMifWr) {
		if (wrRspGrpIdW == 0) {
			mifPostInstr.Append("\tbool c_wrGrpRspCntBusy = r_wrGrpState.m_cnt >= ((1ul << %s_WR_RSP_CNT_W) - %d);\n",
				mod.m_modName.Upper().c_str(),
				bMultiQwMif ? 11 : 4);
		} else {
			mifPostInstr.Append("\tbool c_wrGrpRspCntBusy = r_wrGrpBusyCnt >= ((1ul << %s_WR_RSP_CNT_W) - %d);\n",
				mod.m_modName.Upper().c_str(),
				bMultiQwMif ? 11 : 4);
		}
		mifPostInstr.NewLine();
	}

	string reqValid;
	if (mif.m_bMifWr) reqValid += VA("r_t%d_memReq.m_wrReq", mod.m_execStg + 1);
	if (mif.m_bMifRd && mif.m_bMifWr) reqValid += " || ";
	if (mif.m_bMifRd) reqValid += VA("r_t%d_memReq.m_rdReq", mod.m_execStg + 1);

	if (bMultiQwReq) {
		mifPostInstr.Append("\tbool c_t%d_memReqHold = r_t%d_memReq.m_valid && (r_t%d_memReq.m_qwRem > 1 || r_memReqHold);\n",
			mod.m_execStg + 1,
			mod.m_execStg + 1,
			mod.m_execStg + 1);
		mifPostInstr.NewLine();

		mifPostInstr.Append("\tbool c_t%d_memReqLasHold = r_t%d_memReqLas.m_valid && c_t%d_memReqHold;\n",
			mod.m_execStg + 1,
			mod.m_execStg + 1,
			mod.m_execStg + 1);

		mifPostInstr.Append("\tCMemReq c_t%d_memReqLas = c_t%d_memReqLasHold ? r_t%d_memReqLas : c_t%d_memReq;\n",
			mod.m_execStg, mod.m_execStg + 1, mod.m_execStg + 1, mod.m_execStg);

		mifPostInstr.Append("\tc_t%d_memReqLas.m_valid = c_t%d_memReqLasHold || c_t%d_memReqHold && c_t%d_memReq.m_valid;\n",
			mod.m_execStg,
			mod.m_execStg + 1,
			mod.m_execStg + 1,
			mod.m_execStg);

		if (mif.m_bMifRd && bNeedRdRspInfo) {
			mifPostInstr.Append("\tCRdRspInfo c_t%d_rdRspInfoLas = c_t%d_memReqLasHold ? r_t%d_rdRspInfoLas : c_t%d_rdRspInfo;\n",
				mod.m_execStg, mod.m_execStg + 1, mod.m_execStg + 1, mod.m_execStg);
		}
		mifPostInstr.Append("\tCMemRdWrReqIntf c_t%d_%sToMif_reqLas = c_t%d_memReqLasHold ? r_t%d_%sToMif_reqLas : c_t%d_%sToMif_req;\n",
			mod.m_execStg, mod.m_modName.Lc().c_str(),
			mod.m_execStg + 1,
			mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
			mod.m_execStg, mod.m_modName.Lc().c_str());
		mifPostInstr.NewLine();

		mifPostInstr.Append("\tif (c_t%d_memReqHold) {\n",
			mod.m_execStg + 1);
		mifPostInstr.Append("\t\tc_t%d_%sToMif_req = r_t%d_%sToMif_req;\n",
			mod.m_execStg, mod.m_modName.Lc().c_str(),
			mod.m_execStg + 1, mod.m_modName.Lc().c_str());
		mifPostInstr.Append("\t\tc_t%d_memReq = r_t%d_memReq;\n",
			mod.m_execStg,
			mod.m_execStg + 1);
		if (mif.m_bMifRd && bNeedRdRspInfo) {
			mifPostInstr.Append("\t\tc_t%d_rdRspInfo = r_t%d_rdRspInfo;\n",
				mod.m_execStg,
				mod.m_execStg + 1);
		}
		mifPostInstr.Append("\t} else if (r_t%d_memReqLas.m_valid) {\n",
			mod.m_execStg + 1);
		mifPostInstr.Append("\t\tc_t%d_%sToMif_req = r_t%d_%sToMif_reqLas;\n",
			mod.m_execStg, mod.m_modName.Lc().c_str(),
			mod.m_execStg + 1, mod.m_modName.Lc().c_str());
		mifPostInstr.Append("\t\tc_t%d_memReq = r_t%d_memReqLas;\n",
			mod.m_execStg,
			mod.m_execStg + 1);
		if (mif.m_bMifRd && bNeedRdRspInfo) {
			mifPostInstr.Append("\t\tc_t%d_rdRspInfo = r_t%d_rdRspInfoLas;\n",
				mod.m_execStg,
				mod.m_execStg + 1);
		}
		mifPostInstr.Append("\t}\n");
		mifPostInstr.NewLine();

		string rspCntBusy;
		if (mif.m_bMifRd) rspCntBusy += "r_rdGrpRspCntBusy";
		if (rspCntBusy.size() > 0 && mif.m_bMifWr) rspCntBusy += " || ";
		if (mif.m_bMifWr) rspCntBusy += "r_wrGrpRspCntBusy";

		string reqQwRemEqZ;
		if (bMultiQwMif) reqQwRemEqZ = VA(" && r_t%d_memReq.m_reqQwRem == 0", mod.m_execStg + mif.m_mifReqStgCnt);

		mifPostInstr.Append("\tbool c_memReqHold = r_memReqHold && (%s || r_%sToMif_reqAvlCntBusy) ||\n",
			rspCntBusy.c_str(), mod.m_modName.Lc().c_str());
		mifPostInstr.Append("\t\t!r_memReqHold && r_t%d_memReq.m_valid%s && (%s || r_%sToMif_reqAvlCntBusy);\n",
			mod.m_execStg + mif.m_mifReqStgCnt,
			reqQwRemEqZ.c_str(),
			rspCntBusy.c_str(),
			mod.m_modName.Lc().c_str());
		mifPostInstr.NewLine();

		mifPostInstr.Append("\tbool c_t%d_memReqBusy = c_t%d_memReqLas.m_valid;\n",
			mod.m_execStg - 1, mod.m_execStg, mod.m_modName.Lc().c_str());
		mifPostInstr.NewLine();

		string reqValidQwRemEqZ = reqValid;
		if (mif.m_bMifRd && mif.m_bMifWr) reqValid = string("(") + reqValid + ")";
		if (bMultiQwMif && mif.m_bMifRd) {
			reqValidQwRemEqZ = VA("(%s && r_t%d_memReq.m_reqQwRem == 0)", reqValidQwRemEqZ.c_str(), mod.m_execStg + 1);
		}

		if (mif.m_mifReqStgCnt == 1) {
			mifPostInstr.Append("\tbool c_t%d_%sToMif_reqRdy = !c_memReqHold && (%s || r_memReqHold);\n",
				mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
				reqValidQwRemEqZ.c_str());
		} else {
			HtlAssert(mif.m_mifReqStgCnt == 2);
			mifPostInstr.Append("\tbool c_t%d_%sToMif_reqRdy = r_memReqHold ? r_t%d_%sToMif_reqRdy : %s;\n",
				mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
				mod.m_execStg + 2, mod.m_modName.Lc().c_str(),
				reqValidQwRemEqZ.c_str());
		}
		mifPostInstr.Append("\tCMemRdWrReqIntf c_t%d_%sToMif_req = r_memReqHold ? r_t%d_%sToMif_req : r_t%d_%sToMif_req;\n",
			mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
			mod.m_execStg + 2, mod.m_modName.Lc().c_str(),
			mod.m_execStg + 1, mod.m_modName.Lc().c_str());
		mifPostInstr.Append("\tCMemReq c_t%d_memReq = r_memReqHold ? r_t%d_memReq : r_t%d_memReq;\n",
			mod.m_execStg + 1,
			mod.m_execStg + 2,
			mod.m_execStg + 1);
		mifPostInstr.NewLine();
	}

	if (!bMultiQwReq && mif.m_mifReqStgCnt >= 1) {
		mifPostInstr.Append("\tbool c_t%d_%sToMif_reqRdy = %s;\n",
			mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
			reqValid.c_str());
		mifPostInstr.Append("\tCMemRdWrReqIntf c_t%d_%sToMif_req = r_t%d_%sToMif_req;\n",
			mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
			mod.m_execStg + 1, mod.m_modName.Lc().c_str());
		if (mif.m_mifReqStgCnt == 2) {
			mifPostInstr.Append("\tCMemReq c_t%d_memReq = r_t%d_memReq;\n",
				mod.m_execStg + 1,
				mod.m_execStg + 1);
		}
		mifPostInstr.NewLine();
	}

	if (mif.m_bMifRd && bNeedRdRspInfoRam) {
		mifPostInstr.Append("\tsc_uint<%s_RD_RSP_CNT_W + 1> c_rdRspIdInit = r_rdRspIdInit;\n",
			mod.m_modName.Upper().c_str());
		mifPostInstr.NewLine();
	}

	if (mif.m_bMifRd && !bMultiQwReq && bNeedRdRspInfo && mif.m_mifReqStgCnt > 0) {

		mifPostInstr.Append("\tif (r_t%d_memReq.m_rdReq) {\n",
			mod.m_execStg + 1);
		mifPostInstr.NewLine();

		if (bNeedRdRspInfoRam) {
			mifPostInstr.Append("\t\tsc_uint<%s_RD_RSP_CNT_W> c_rdRspId = 0;\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.NewLine();

			mifPostInstr.Append("\t\tif (r_rdRspIdInit[%s_RD_RSP_CNT_W]) {\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.Append("\t\t\tc_rdRspId = m_rdRspIdPool.front();\n");
			mifPostInstr.Append("\t\t\tm_rdRspIdPool.pop();\n");
			mifPostInstr.Append("\t\t} else {\n");
			mifPostInstr.Append("\t\t\tc_rdRspId = r_rdRspIdInit(%s_RD_RSP_CNT_W - 1, 0);\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.Append("\t\t\tc_rdRspIdInit = r_rdRspIdInit + 1;\n");
			mifPostInstr.Append("\t\t}\n");
			mifPostInstr.NewLine();
		}

		if (bNeedRdRspInfoRam) {
			mifPostInstr.Append("\t\tc_t%d_%sToMif_req.m_tid = c_rdRspId;\n",
				mod.m_execStg + 1, mod.m_modName.Lc().c_str());
		} else {
			mifPostInstr.Append("\t\tc_t%d_%sToMif_req.m_tid = r_t%d_rdRspInfo.m_uint32;\n",
				mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
				mod.m_execStg + 1);
		}
		mifPostInstr.NewLine();

		if (bNeedRdRspInfoRam) {
			mifPostInstr.Append("\t\tm_rdRspInfo.write_addr(c_rdRspId);\n");
			mifPostInstr.Append("\t\tm_rdRspInfo.write_mem(r_t%d_rdRspInfo);\n",
				mod.m_execStg + 1);
		}
		mifPostInstr.Append("\t}\n");
		mifPostInstr.NewLine();
	}

	if (bMultiQwReq) {
		mifPostInstr.Append("\tif (!r_memReqHold) {\n");

		if (bMultiQwMif) {
			string checkReqSize;
			if (!bSingleMemSize)
				checkReqSize = VA(" && r_t%d_%sToMif_req.m_size == 3", mod.m_execStg + 1, mod.m_modName.Lc().c_str());

			mifPostInstr.Append("\t\tbool bMultiQwReq = ");
			string separator;
			if (bMultiQwHostRdMif) {
				mifPostInstr.Append("%s(r_t%d_%sToMif_req.m_host && r_t%d_%sToMif_req.m_type == MEM_REQ_RD%s\n\t\t\t&& (r_t%d_%sToMif_req.m_addr & 0x38) != 0x38 && r_t%d_memReq.m_qwRem >= 2)", separator.c_str(),
					mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
					mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
					checkReqSize.c_str(),
					mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
					mod.m_execStg + 1);
				separator = "\n\t\t\t|| ";
			}
			if (bMultiQwCoprocRdMif) {
				mifPostInstr.Append("%s(!r_t%d_%sToMif_req.m_host && r_t%d_%sToMif_req.m_type == MEM_REQ_RD%s\n\t\t\t&& (r_t%d_%sToMif_req.m_addr & 0x38) != 0x38 && r_t%d_memReq.m_qwRem >= 2)", separator.c_str(),
					mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
					mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
					checkReqSize.c_str(),
					mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
					mod.m_execStg + 1);
				separator = "\n\t\t\t|| ";
			}
			if (bMultiQwHostWrMif) {
				mifPostInstr.Append("%s(r_t%d_%sToMif_req.m_host && r_t%d_%sToMif_req.m_type == MEM_REQ_WR%s\n\t\t\t&& (r_t%d_%sToMif_req.m_addr & 0x38) %s && r_t%d_memReq.m_qwRem >= %d)", separator.c_str(),
					mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
					mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
					checkReqSize.c_str(),
					mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
					coprocInfo.IsVarQwReqCnt() ? "!= 0x38" : " == 0",
					mod.m_execStg + 1,
					coprocInfo.IsVarQwReqCnt() ? 2 : 8);
				separator = "\n\t\t\t|| ";
			}
			if (bMultiQwCoprocWrMif) {
				mifPostInstr.Append("%s(!r_t%d_%sToMif_req.m_host && r_t%d_%sToMif_req.m_type == MEM_REQ_WR%s\n\t\t\t&& (r_t%d_%sToMif_req.m_addr & 0x38) %s && r_t%d_memReq.m_qwRem >= %d)", separator.c_str(),
					mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
					mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
					checkReqSize.c_str(),
					mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
					coprocInfo.IsVarQwReqCnt() ? "!= 0x38" : " == 0",
					mod.m_execStg + 1,
					coprocInfo.IsVarQwReqCnt() ? 2 : 8);
				separator = "\n\t\t\t|| ";
			}
			mifPostInstr.Append(";\n");
			mifPostInstr.NewLine();

			mifPostInstr.Append("\t\tht_uint3 qwFst = (r_t%d_%sToMif_req.m_addr >> 3) & 0x7;\n",
				mod.m_execStg + 1, mod.m_modName.Lc().c_str());
			mifPostInstr.Append("\t\tbool bSpanMemLines = (qwFst + r_t%d_memReq.m_qwRem) >= 0x8;\n",
				mod.m_execStg + 1);
			mifPostInstr.Append("\t\tht_uint3 qwLst = bSpanMemLines ? 7 : ((qwFst + r_t%d_memReq.m_qwRem - 1) & 0x7);\n",
				mod.m_execStg + 1);

			if (mif.m_bMifRd && mif.m_bMifWr) {
				mifPostInstr.Append("\t\tht_uint3 lineQwCntM1 = r_t%d_memReq.m_rdReq ? 7 : (qwLst - qwFst);\n",
					mod.m_execStg + 1);
			} else if (mif.m_bMifRd)
				mifPostInstr.Append("\t\tht_uint3 lineQwCntM1 = 7;\n");
			else
				mifPostInstr.Append("\t\tht_uint3 lineQwCntM1 = qwLst - qwFst;\n");
			mifPostInstr.NewLine();
		}

		mifPostInstr.Append("\t\tif (r_t%d_memReq.m_valid && r_t%d_memReq.m_qwRem > 1) {\n",
			mod.m_execStg + 1,
			mod.m_execStg + 1);

		mifPostInstr.NewLine();

		mifPostInstr.Append("\t\t\tc_t%d_memReq.m_qwRem = r_t%d_memReq.m_qwRem - 1;\n",
			mod.m_execStg, mod.m_execStg + 1);
		mifPostInstr.NewLine();

		bool bNeedElemQwIdx = maxElemQwCnt > 1;
		bool bNeedVIdx1 = mif.m_vIdxWList.size() > 0 && mif.m_vIdxWList[0] > 0;
		bool bNeedVIdx2 = mif.m_vIdxWList.size() > 1 && mif.m_vIdxWList[1] > 0;
		bool bNeedElemQwCntM1 = bNeedElemQwIdx && (bNeedVIdx1 || bNeedVIdx2);
		bool bNeedVIdx2CntM1 = bNeedVIdx2 && bNeedVIdx1;

		if (bNeedElemQwIdx) {
			string tabs;
			if (bNeedElemQwCntM1) {
				mifPostInstr.Append("\t\t\tif (r_t%d_memReq.m_elemQwIdx < r_t%d_memReq.m_elemQwCntM1) {\n",
					mod.m_execStg + 1, mod.m_execStg + 1);
				tabs = "\t";
			}
			mifPostInstr.Append("%s\t\t\tc_t%d_memReq.m_elemQwIdx = r_t%d_memReq.m_elemQwIdx + 1u;\n",
				tabs.c_str(), mod.m_execStg, mod.m_execStg + 1);

			if (bNeedElemQwCntM1)
				mifPostInstr.Append("\t\t\t}");
		}

		if (bNeedVIdx2) {
			if (bNeedElemQwIdx)
				mifPostInstr.Append(" else ");
			else
				mifPostInstr.Append("\t\t\t");

			string tabs;
			if (bNeedVIdx2CntM1) {
				mifPostInstr.Append("if (r_t%d_memReq.m_vIdx2 < r_t%d_memReq.m_vIdx2CntM1) {\n",
					mod.m_execStg + 1, mod.m_execStg + 1);
				tabs = "\t";
			} else if (bNeedElemQwIdx) {
				mifPostInstr.Append("{\n");
				mifPostInstr.Append("\t\t\t\tc_t%d_memReq.m_elemQwIdx = 0;\n",
					mod.m_execStg);
				tabs = "\t";
			}

			mifPostInstr.Append("%s\t\t\tc_t%d_memReq.m_vIdx2 = r_t%d_memReq.m_vIdx2 + 1u;\n",
				tabs.c_str(), mod.m_execStg, mod.m_execStg + 1);

			if (bNeedVIdx2CntM1 || bNeedElemQwIdx) {
				if (bNeedVIdx1)
					mifPostInstr.Append("\t\t\t}");
				else
					mifPostInstr.Append("\t\t\t}\n");
			}
		}

		if (bNeedVIdx1) {
			string tabs;
			if (bNeedElemQwCntM1 || bNeedVIdx2CntM1) {
				mifPostInstr.Append(" else {\n");
				tabs = "\t";
			}

			if (bNeedElemQwIdx) {
				mifPostInstr.Append("%s\t\t\tc_t%d_memReq.m_elemQwIdx = 0;\n",
					tabs.c_str(), mod.m_execStg);
			}

			if (bNeedVIdx2) {
				mifPostInstr.Append("%s\t\t\tc_t%d_memReq.m_vIdx2 = 0;\n",
					tabs.c_str(), mod.m_execStg);
			}

			mifPostInstr.Append("%s\t\t\tc_t%d_memReq.m_vIdx1 = r_t%d_memReq.m_vIdx1 + 1u;\n",
				tabs.c_str(), mod.m_execStg, mod.m_execStg + 1);

			if (bNeedElemQwIdx || bNeedVIdx2CntM1)
				mifPostInstr.Append("\t\t\t}\n");
		}

		mifPostInstr.NewLine();

		mifPostInstr.Append("\t\t\tc_t%d_%sToMif_req.m_addr = r_t%d_%sToMif_req.m_addr + ((ht_uint4)1 << r_t%d_%sToMif_req.m_size);\n",
			mod.m_execStg, mod.m_modName.Lc().c_str(),
			mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
			mod.m_execStg + 1, mod.m_modName.Lc().c_str());
		mifPostInstr.NewLine();

		if (bMultiQwMif) {
			mifPostInstr.Append("\t\t\tif (r_t%d_memReq.m_reqQwRem > 0) {\n", mod.m_execStg + 1);
			mifPostInstr.Append("\t\t\t\tc_t%d_memReq.m_reqQwRem = r_t%d_memReq.m_reqQwRem - 1;\n",
				mod.m_execStg, mod.m_execStg + 1);

			mifPostInstr.Append("\t\t\t} else if (bMultiQwReq) {\n");

			mifPostInstr.Append("\t\t\t\tc_t%d_memReq.m_reqQwRem = qwLst - qwFst;\n", mod.m_execStg);

			mifPostInstr.Append("\t\t\t} else {\n");
			mifPostInstr.Append("\t\t\t\tc_t%d_memReq.m_reqQwRem = 0;\n", mod.m_execStg);
			mifPostInstr.Append("\t\t\t}\n");
		}

		mifPostInstr.Append("\t\t}\n");
		mifPostInstr.NewLine();

		if (mif.m_bMifRd && bNeedRdRspInfo) {

			string reqQwRemEqZ;
			if (bMultiQwMif) reqQwRemEqZ = VA(" && r_t%d_memReq.m_reqQwRem == 0", mod.m_execStg + 1);

			mifPostInstr.Append("\t\tif (r_t%d_memReq.m_rdReq%s) {\n",
				mod.m_execStg + 1, reqQwRemEqZ.c_str());

			if (bNeedRdRspInfoRam) {
				mifPostInstr.Append("\t\t\tsc_uint<%s_RD_RSP_CNT_W> c_rdRspId = 0;\n",
					mod.m_modName.Upper().c_str());
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\t\tif (r_rdRspIdInit[%s_RD_RSP_CNT_W]) {\n",
					mod.m_modName.Upper().c_str());
				mifPostInstr.Append("\t\t\t\tc_rdRspId = m_rdRspIdPool.front();\n");
				mifPostInstr.Append("\t\t\t\tm_rdRspIdPool.pop();\n");
				mifPostInstr.Append("\t\t\t} else {\n");
				mifPostInstr.Append("\t\t\t\tc_rdRspId = r_rdRspIdInit(%s_RD_RSP_CNT_W - 1, 0);\n",
					mod.m_modName.Upper().c_str());
				mifPostInstr.Append("\t\t\t\tc_rdRspIdInit = r_rdRspIdInit + 1;\n");
				mifPostInstr.Append("\t\t\t}\n");
			}

			mifPostInstr.Append("\t\t\tCRdRspInfo c_t%d_rdRspInfo = r_t%d_rdRspInfo;\n",
				mod.m_execStg + 1,
				mod.m_execStg + 1);
			if (bMultiQwRdMif) {
				mifPostInstr.Append("\t\t\tc_t%d_rdRspInfo.m_multiQwReq = bMultiQwReq;\n",
					mod.m_execStg + 1);
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\t\tc_t%d_rdRspInfo.m_qwFst = qwFst;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tc_t%d_rdRspInfo.m_qwLst = qwLst;\n", mod.m_execStg + 1);
			}
			mifPostInstr.NewLine();

			bool bNeedCntM1 = false;
			for (int idx = 0; idx < (int)mif.m_vIdxWList.size(); idx += 1) {
				if (mif.m_vIdxWList[idx] > 0) {
					mifPostInstr.Append("\t\t\tc_t%d_rdRspInfo.m_vIdx%d = r_t%d_memReq.m_vIdx%d;\n",
						mod.m_execStg + 1, idx + 1,
						mod.m_execStg + 1, idx + 1);
					if (bNeedCntM1) {
						mifPostInstr.Append("\t\t\tc_t%d_rdRspInfo.m_vIdx%dCntM1 = r_t%d_memReq.m_vIdx%dCntM1;\n",
							mod.m_execStg + 1, idx + 1,
							mod.m_execStg + 1, idx + 1);
					}
					bNeedCntM1 = true;
				}
			}

			if (maxElemQwCnt > 1) {
				mifPostInstr.Append("\t\t\tc_t%d_rdRspInfo.m_elemQwIdx = r_t%d_memReq.m_elemQwIdx;\n",
					mod.m_execStg + 1,
					mod.m_execStg + 1);
				if (bNeedCntM1) {
					mifPostInstr.Append("\t\t\tc_t%d_rdRspInfo.m_elemQwCntM1 = r_t%d_memReq.m_elemQwCntM1;\n",
						mod.m_execStg + 1,
						mod.m_execStg + 1);
				}
			}
		}

		if (mif.m_bMifRd && bNeedRdRspInfo) {
			mifPostInstr.NewLine();

			if (bNeedRdRspInfoRam) {
				mifPostInstr.Append("\t\t\tc_t%d_%sToMif_req.m_tid = c_rdRspId;\n",
					mod.m_execStg + 1, mod.m_modName.Lc().c_str());
			} else {
				mifPostInstr.Append("\t\t\tc_t%d_%sToMif_req.m_tid = c_t%d_rdRspInfo.m_uint32;\n",
					mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
					mod.m_execStg + 1);
			}
			mifPostInstr.NewLine();

			if (bNeedRdRspInfoRam) {
				mifPostInstr.Append("\t\t\tm_rdRspInfo.write_addr(c_rdRspId);\n");
				mifPostInstr.Append("\t\t\tm_rdRspInfo.write_mem(c_t%d_rdRspInfo);\n",
					mod.m_execStg + 1);
			}
			mifPostInstr.Append("\t\t}\n");
			if (bMultiQwMif)
				mifPostInstr.NewLine();
		}

		if (bMultiQwMif) {
			mifPostInstr.Append("\t\tht_uint3 tidQwCntM1 = (r_t%d_memReq.m_reqQwRem > 0 || bMultiQwReq) ? lineQwCntM1 : (ht_uint3)0;\n", mod.m_execStg + 1);
			mifPostInstr.Append("\t\tif (r_t%d_memReq.m_reqQwRem == 0)\n", mod.m_execStg + 1);
			mifPostInstr.Append("\t\t\tc_t%d_%sToMif_req.m_tid |= tidQwCntM1 << 29;\n",
				mod.m_execStg + 1, mod.m_modName.Lc().c_str());
			mifPostInstr.Append("\t\telse\n");
			mifPostInstr.Append("\t\t\tc_t%d_%sToMif_req.m_tid = r_t%d_%sToMif_req.m_tid;\n",
				mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
				mod.m_execStg + 2, mod.m_modName.Lc().c_str());

			if (mif.m_bMifRd) {
				if (mif.m_bMifWr) {
					mifPostInstr.Append("\t\tc_t%d_%sToMif_req.m_addr &= (bMultiQwReq && r_t%d_memReq.m_rdReq) ? ~0x3fLL : ~0x0LL;\n",
						mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
						mod.m_execStg + 1);
				} else {
					mifPostInstr.Append("\t\tc_t%d_%sToMif_req.m_addr &= bMultiQwReq ? ~0x3fLL : ~0x0LL;\n",
						mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
						mod.m_execStg + 1);
				}
			}
		}
		mifPostInstr.Append("\t}\n");
		mifPostInstr.NewLine();
	}

	if (mif.m_bMifWr) {
		if (mif.m_mifReqStgCnt >= 1) {
			// generate ram as register read code

			string tabs;
			if (bMultiQwReq) {
				mifPostInstr.Append("\tif (!r_memReqHold) {\n");
				tabs += "\t";
			} else {
				mifPostInstr.Append("\tc_t%d_%sToMif_req.m_data = 0;\n",
					mod.m_execStg + 1, mod.m_modName.Lc().c_str());
			}

			if (mif.m_mifWr.m_wrSrcList.size() > 0)
				mifPostInstr.Append("%s\tswitch (r_t%d_memReq.m_wrSrc) {\n", tabs.c_str(), mod.m_execStg + 1);

			for (size_t wrSrcIdx = 0; wrSrcIdx < mif.m_mifWr.m_wrSrcList.size(); wrSrcIdx += 1) {
				CMifWrSrc & wrSrc = mif.m_mifWr.m_wrSrcList[wrSrcIdx];

				string varName;
				string varIdx;
				ERamType ramType = eRegRam;
				string addrVar;
				string addrFld;
				bool bPrivGblAndNoAddr = false;
				bool bPrivVarIdx = false;

				if (wrSrc.m_pGblVar) {
					bPrivGblAndNoAddr = wrSrc.m_pGblVar->m_bPrivGbl && wrSrc.m_pGblVar->m_addrW == mod.m_threads.m_htIdW.AsInt();

					if (bPrivGblAndNoAddr) {
						varName = VA("r_t%d_memReq.m_wrData.m_%s",
							mod.m_execStg + 1, wrSrc.m_pSrcType->m_typeName.c_str());
					} else {
						ramType = wrSrc.m_pGblVar->m_ramType;
						varName = VA("%s%s%s", ramType == eRegRam ? "r_" : "", wrSrc.m_pGblVar->m_gblName.c_str(), ramType == eRegRam ? "" : "Mr");
						addrVar = VA("%s%s", wrSrc.m_pGblVar->m_gblName.c_str(), wrSrc.m_pGblVar->m_addrW > 0 ? "Mr" : "");
					}
				} else if (wrSrc.m_pSharedVar) {
					ramType = wrSrc.m_pSharedVar->m_ramType;
					varName = VA("%s_SHR__%s", ramType == eRegRam ? "r_" : "", wrSrc.m_pSharedVar->m_name.c_str());
					addrVar = VA("_SHR__%s", wrSrc.m_pSharedVar->m_name.c_str());
				} else if (wrSrc.m_pPrivVar) {

					string varIdx;
					if (wrSrc.m_pPrivVar && !wrSrc.m_bConstDimenList && wrSrc.m_fieldRefList.size() > 0) {
						int fldIdx = wrSrc.m_fieldRefList.size() - 1;
						CFieldRef & fieldRef = wrSrc.m_fieldRefList[fldIdx];

						for (int dimIdx = 0; dimIdx < (int)fieldRef.m_refDimenList.size(); dimIdx += 1) {
							CRefDimen & refDimen = fieldRef.m_refDimenList[dimIdx];

							if (refDimen.m_value >= 0)
								varIdx += VA("[%d]", refDimen.m_value);

							else if (refDimen.m_size == 1)
								varIdx += "[0]";

							else {
								varIdx += VA("[INT(r_t%d_memReq.m_vIdx%d(%d, 0))]", mod.m_execStg + 1, dimIdx + 1, FindLg2(refDimen.m_size - 1) - 1);
								bPrivVarIdx = true;
							}
						}
					}

					varName = VA("r_t%d_memReq.m_wrData.m_%s%s",
						mod.m_execStg + 1, wrSrc.m_wrDataTypeName.c_str(), varIdx.c_str());
				} else if (wrSrc.m_pType != 0) {
					varName = VA("r_t%d_memReq.m_wrData.m_%s",
						mod.m_execStg + 1, wrSrc.m_pType->m_typeName.c_str());
				} else
					HtlAssert(0);

				if ((bPrivGblAndNoAddr || wrSrc.m_pPrivVar && !bPrivVarIdx || wrSrc.m_pType != 0) && mif.m_mifReqStgCnt == 2) continue;

				mifPostInstr.Append("%s\tcase %d:\n", tabs.c_str(), wrSrcIdx);
				tabs += "\t";

				if (!(wrSrc.m_pPrivVar || wrSrc.m_pGblVar && wrSrc.m_pGblVar->m_bPrivGbl &&
					wrSrc.m_pGblVar->m_addrW == mod.m_threads.m_htIdW.AsInt()))
				{
					for (int fldIdx = 0; fldIdx < (int)wrSrc.m_fieldRefList.size(); fldIdx += 1) {
						CFieldRef & fieldRef = wrSrc.m_fieldRefList[fldIdx];

						string identStr;

						if (fldIdx > 0) {
							identStr += '.';
							identStr += fieldRef.m_fieldName;
						}

						for (int dimIdx = 0; dimIdx < (int)fieldRef.m_refDimenList.size(); dimIdx += 1) {
							CRefDimen & refDimen = fieldRef.m_refDimenList[dimIdx];

							if (refDimen.m_value >= 0)
								identStr += VA("[%d]", refDimen.m_value);

							else if (refDimen.m_size == 1)
								identStr += "[0]";

							else if (refDimen.m_isIdx) {
								identStr += VA("[INT(r_t%d_memReq.m_vIdx%d(%d, 0))]",
									mod.m_execStg + 1, dimIdx + 1, FindLg2(refDimen.m_size - 1) - 1);
							}
							else {
								identStr += VA("[INT(r_t%d_memReq.m_s%d_f%dIdx%d)]",
									mod.m_execStg + 1, wrSrcIdx, fldIdx + 1, dimIdx + 1);
							}
						}

						varName += identStr;

						if (fldIdx == 0)
							addrVar += identStr;
						else
							addrFld += identStr;
					}
				}

				if ((ramType == eRegRam || wrSrc.m_pType != 0) && mif.m_mifReqStgCnt == 1) {
					int elemWidth = wrSrc.m_pSrcType->m_clangBitWidth;
					int reqSize = wrSrc.m_pSrcType->m_clangMinAlign == 1 ? wrSrc.m_pSrcType->m_clangBitWidth : wrSrc.m_pSrcType->m_clangMinAlign;
					int elemQwCnt = (elemWidth + reqSize - 1) / reqSize;

					if (elemQwCnt > 1) {
						mifPostInstr.Append("%s\tswitch (r_t%d_memReq.m_elemQwIdx) {\n", tabs.c_str(),
							mod.m_execStg + 1);
					}

					for (int qwIdx = 0; qwIdx < elemQwCnt; qwIdx += 1) {

						if (elemQwCnt > 1) {
							mifPostInstr.Append("%s\tcase %d:\n", tabs.c_str(), qwIdx);
							tabs += "\t";
						}

						for (CStructElemIter iter(this, wrSrc.m_pSrcType); !iter.end(); iter++) {
							if (iter.IsStructOrUnion()) continue;

							int pos = iter.GetHeirFieldPos();
							int width = iter.GetWidth();

							if (pos < qwIdx * reqSize || pos >= (qwIdx + 1) * reqSize) continue;

							if (width < 64) {
								mifPostInstr.Append("%s\tc_t%d_%sToMif_req.m_data |= (uint64_t)((%s%s & ((1LL << %d) - 1)) << %d);\n", tabs.c_str(),
									mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
									varName.c_str(), iter.GetHeirFieldName().c_str(),
									width, pos % reqSize);
							} else {
								mifPostInstr.Append("%s\tc_t%d_%sToMif_req.m_data |= %s%s;\n", tabs.c_str(),
									mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
									varName.c_str(), iter.GetHeirFieldName().c_str());
							}
						}

						if (elemQwCnt > 1) {
							mifPostInstr.Append("%s\tbreak;\n", tabs.c_str());
							tabs.erase(0, 1);
						}
					}

					if (elemQwCnt > 1) {
						mifPostInstr.Append("%s\tdefault:\n", tabs.c_str());
						mifPostInstr.Append("%s\t\tbreak;\n", tabs.c_str());
						mifPostInstr.Append("%s\t}\n", tabs.c_str());
					}
				} else if (ramType == eRegRam) {
					mifPostInstr.Append("%s\tc_t%d_memReq.m_wrData.m_%s = %s%s;\n", tabs.c_str(),
						mod.m_execStg + 1, wrSrc.m_pSrcType->m_typeName.c_str(),
						varName.c_str(), varIdx.c_str());
				} else {

					if (wrSrc.m_varAddr2W <= 0) {
						if (wrSrc.m_varAddr1IsIdx) {
							mifPostInstr.Append("%s\tm_%s%s.read_addr(r_t%d_memReq.m_vIdx1(%d, 0));\n", tabs.c_str(),
								addrVar.c_str(), varIdx.c_str(),
								mod.m_execStg + 1, wrSrc.m_varAddr1W - 1);
						} else {
							mifPostInstr.Append("%s\tm_%s%s.read_addr(r_t%d_memReq.m_s%d_f1Addr1);\n", tabs.c_str(),
								addrVar.c_str(), varIdx.c_str(),
								mod.m_execStg + 1, wrSrcIdx);
						}
					}
					else if (wrSrc.m_pSharedVar) {
						mifPostInstr.Append("%s\tm_%s%s.read_addr(", tabs.c_str(),
							addrVar.c_str(), varIdx.c_str());

						if (wrSrc.m_varAddr1IsIdx) {
							mifPostInstr.Append("r_t%d_memReq.m_vIdx1(%d, 0), ",
								mod.m_execStg + 1, wrSrc.m_varAddr1W - 1);
						}
						else {
							mifPostInstr.Append("r_t%d_memReq.m_s%d_f1Addr1, ",
								mod.m_execStg + 1, wrSrcIdx);
						}

						if (wrSrc.m_varAddr2IsIdx) {
							mifPostInstr.Append("r_t%d_memReq.m_vIdx2(%d, 0));\n",
								mod.m_execStg + 1, wrSrc.m_varAddr2W - 1);
						}
						else {
							mifPostInstr.Append("r_t%d_memReq.m_s%d_f1Addr2);\n",
								mod.m_execStg + 1, wrSrcIdx);
						}
					}
					else {
						mifPostInstr.Append("%s\tm_%s%s.read_addr((", tabs.c_str(),
							addrVar.c_str(), varIdx.c_str());

						if (wrSrc.m_varAddr1IsIdx) {
							mifPostInstr.Append("r_t%d_memReq.m_vIdx1(%d, 0), ",
								mod.m_execStg + 1, wrSrc.m_varAddr1W - 1);
						}
						else {
							mifPostInstr.Append("(ht_uint%d)r_t%d_memReq.m_s%d_f1Addr1, ",
								wrSrc.m_varAddr1W, mod.m_execStg + 1, wrSrcIdx);
						}

						if (wrSrc.m_varAddr2IsIdx) {
							mifPostInstr.Append("r_t%d_memReq.m_vIdx2(%d, 0)));\n",
								mod.m_execStg + 1, wrSrc.m_varAddr2W - 1);
						}
						else {
							mifPostInstr.Append("(ht_uint%d)r_t%d_memReq.m_s%d_f1Addr2));\n",
								wrSrc.m_varAddr2W, mod.m_execStg + 1, wrSrcIdx);
						}
					}

					if (ramType == eDistRam) {
						if (mif.m_mifReqStgCnt < 2) {
							mifPostInstr.Append("%s\tc_t%d_%sToMif_req.m_data = m_%s%s.read_mem()%s;\n", tabs.c_str(),
								mod.m_execStg + 1, mod.m_modName.Lc().c_str(),
								addrVar.c_str(), varIdx.c_str(), addrFld.c_str());
						} else {
							mifPostInstr.Append("%s\tc_t%d_memReq.m_wrData.m_%s = m_%s%s.read_mem()%s;\n", tabs.c_str(),
								mod.m_execStg + 1, wrSrc.m_pSrcType->m_typeName.c_str(),
								addrVar.c_str(), varIdx.c_str(), addrFld.c_str());
						}
					}
				}

				mifPostInstr.Append("%s\tbreak;\n", tabs.c_str());
				tabs.erase(0, 1);
			}

			mifPostInstr.Append("%s\tdefault:\n", tabs.c_str());
			mifPostInstr.Append("%s\t\tbreak;\n", tabs.c_str());
			mifPostInstr.Append("%s\t}\n", tabs.c_str());
			tabs.erase(0, 1);

			if (bMultiQwReq)
				mifPostInstr.Append("\t}\n");

			mifPostInstr.NewLine();
		}
	}

	if (mif.m_mifReqStgCnt == 2) {
		if (bMultiQwReq) {
			mifPostInstr.Append("\tbool c_t%d_%sToMif_reqRdy = !c_memReqHold && (r_t%d_%sToMif_reqRdy || r_memReqHold);\n",
				mod.m_execStg + 2, mod.m_modName.Lc().c_str(),
				mod.m_execStg + 2, mod.m_modName.Lc().c_str());
			mifPostInstr.Append("\tCMemRdWrReqIntf c_t%d_%sToMif_req = r_memReqHold ? r_t%d_%sToMif_req : r_t%d_%sToMif_req;\n",
				mod.m_execStg + 2, mod.m_modName.Lc().c_str(),
				mod.m_execStg + 3, mod.m_modName.Lc().c_str(),
				mod.m_execStg + 2, mod.m_modName.Lc().c_str());
			mifPostInstr.NewLine();
		}
		else {
			mifPostInstr.Append("\tbool c_t%d_%sToMif_reqRdy = r_t%d_%sToMif_reqRdy;\n",
				mod.m_execStg + 2, mod.m_modName.Lc().c_str(),
				mod.m_execStg + 2, mod.m_modName.Lc().c_str());
			mifPostInstr.Append("\tCMemRdWrReqIntf c_t%d_%sToMif_req = r_t%d_%sToMif_req;\n",
				mod.m_execStg + 2, mod.m_modName.Lc().c_str(),
				mod.m_execStg + 2, mod.m_modName.Lc().c_str());
			mifPostInstr.NewLine();
		}
	}

	if (mif.m_bMifWr) {
		if (mif.m_mifReqStgCnt == 2) {
			// generate write data code
			string tabs;
			if (bMultiQwReq) {
				mifPostInstr.Append("\tif (!r_memReqHold) {\n");
				tabs += "\t";
			}

			mifPostInstr.Append("%s\tc_t%d_%sToMif_req.m_data = 0;\n", tabs.c_str(),
				mod.m_execStg + 2, mod.m_modName.Lc().c_str());

			mifPostInstr.Append("%s\tswitch (r_t%d_memReq.m_wrSrc) {\n", tabs.c_str(),
				mod.m_execStg + 2);

			// generate for non block ram reads
			for (size_t wrSrcTypeIdx = 0; wrSrcTypeIdx < wrSrcTypeList.size(); wrSrcTypeIdx += 1) {
				CType * pWrSrcType = wrSrcTypeList[wrSrcTypeIdx];

				int caseCnt = 0;
				for (size_t wrSrcIdx = 0; wrSrcIdx < mif.m_mifWr.m_wrSrcList.size(); wrSrcIdx += 1) {
					CMifWrSrc & wrSrc = mif.m_mifWr.m_wrSrcList[wrSrcIdx];

					if (wrSrc.m_pSrcType != pWrSrcType) continue;

					ERamType ramType = eRegRam;

					if (wrSrc.m_pGblVar)
						ramType = wrSrc.m_pGblVar->m_ramType;
					else if (wrSrc.m_pSharedVar)
						ramType = wrSrc.m_pSharedVar->m_ramType;

					if (ramType == eBlockRam) continue;

					mifPostInstr.Append("%s\tcase %d:\n", tabs.c_str(), wrSrcIdx);
					caseCnt += 1;
				}

				if (caseCnt == 0) continue;

				tabs += "\t";

				int elemWidth = pWrSrcType->m_clangBitWidth;
				int reqSize = pWrSrcType->m_clangMinAlign == 1 ? pWrSrcType->m_clangBitWidth : pWrSrcType->m_clangMinAlign;
				int elemQwCnt = (elemWidth + reqSize - 1) / reqSize;

				if (elemQwCnt > 1)
					mifPostInstr.Append("%s\tswitch (r_t%d_memReq.m_elemQwIdx) {\n", tabs.c_str(),
						mod.m_execStg + 2);

				for (int qwIdx = 0; qwIdx < elemQwCnt; qwIdx += 1) {

					if (elemQwCnt > 1) {
						mifPostInstr.Append("%s\tcase %d:\n", tabs.c_str(), qwIdx);
						tabs += "\t";
					}

					for (CStructElemIter iter(this, pWrSrcType); !iter.end(); iter++) {
						if (iter.IsStructOrUnion()) continue;

						int pos = iter.GetHeirFieldPos();
						int width = iter.GetWidth();

						if (pos < qwIdx * reqSize || pos >= (qwIdx + 1) * reqSize) continue;

						if (width < 64) {
							mifPostInstr.Append("%s\tc_t%d_%sToMif_req.m_data |= (uint64_t)((r_t%d_memReq.m_wrData.m_%s%s & ((1LL << %d) - 1)) << %d);\n",
								tabs.c_str(),
								mod.m_execStg + 2, mod.m_modName.Lc().c_str(),
								mod.m_execStg + 2, pWrSrcType->m_typeName.c_str(), iter.GetHeirFieldName().c_str(),
								width, pos % reqSize);
						} else {
							mifPostInstr.Append("%s\tc_t%d_%sToMif_req.m_data |= r_t%d_memReq.m_wrData.m_%s%s;\n",
								tabs.c_str(),
								mod.m_execStg + 2, mod.m_modName.Lc().c_str(),
								mod.m_execStg + 2, pWrSrcType->m_typeName.c_str(), iter.GetHeirFieldName().c_str());
						}
					}

					if (elemQwCnt > 1) {
						mifPostInstr.Append("%s\tbreak;\n", tabs.c_str());
						tabs.erase(0, 1);
					}
				}

				if (elemQwCnt > 1) {
					mifPostInstr.Append("%s\tdefault:\n", tabs.c_str());
					mifPostInstr.Append("%s\t\tbreak;\n", tabs.c_str());
					mifPostInstr.Append("%s\t}\n", tabs.c_str());
				}

				mifPostInstr.Append("%s\tbreak;\n", tabs.c_str());
				tabs.erase(0, 1);
			}

			// generate for block ram reads
			for (size_t wrSrcIdx = 0; wrSrcIdx < mif.m_mifWr.m_wrSrcList.size(); wrSrcIdx += 1) {
				CMifWrSrc & wrSrc = mif.m_mifWr.m_wrSrcList[wrSrcIdx];

				ERamType ramType = eRegRam;
				string varIdx;
				string addrVar;
				string addrFld;

				if (wrSrc.m_pGblVar) {
					ramType = wrSrc.m_pGblVar->m_ramType;
					addrVar = VA("%s%s", wrSrc.m_pGblVar->m_gblName.c_str(), wrSrc.m_pGblVar->m_addrW > 0 ? "Mr" : "");
				} else if (wrSrc.m_pSharedVar) {
					ramType = wrSrc.m_pSharedVar->m_ramType;
					addrVar = VA("_SHR__%s", wrSrc.m_pSharedVar->m_name.c_str());
				}

				if (ramType != eBlockRam) continue;

				mifPostInstr.Append("%s\tcase %d:\n", tabs.c_str(), wrSrcIdx);
				tabs += "\t";

				if (!(wrSrc.m_pPrivVar || wrSrc.m_pGblVar && wrSrc.m_pGblVar->m_bPrivGbl &&
					wrSrc.m_pGblVar->m_addrW == mod.m_threads.m_htIdW.AsInt())) {

					for (int fldIdx = 0; fldIdx < (int)wrSrc.m_fieldRefList.size(); fldIdx += 1) {
						CFieldRef & fieldRef = wrSrc.m_fieldRefList[fldIdx];

						string identStr;

						if (fldIdx > 0) {
							identStr += '.';
							identStr += fieldRef.m_fieldName;
						}

						for (int dimIdx = 0; dimIdx < (int)fieldRef.m_refDimenList.size(); dimIdx += 1) {
							CRefDimen & refDimen = fieldRef.m_refDimenList[dimIdx];

							if (refDimen.m_value >= 0)
								identStr += VA("[%d]", refDimen.m_value);

							else if (refDimen.m_size == 1)
								identStr += "[0]";

							else if (refDimen.m_isIdx) {
								identStr += VA("[INT(r_t%d_memReq.m_vIdx%d(%d, 0))]",
									mod.m_execStg + 2, dimIdx + 1, FindLg2(refDimen.m_size - 1) - 1);
							}
							else {
								identStr += VA("[INT(r_t%d_memReq.m_s%d_f%dIdx%d)]",
									mod.m_execStg + 2, wrSrcIdx, fldIdx + 1, dimIdx + 1);
							}
						}

						//varName += identStr;

						if (fldIdx == 0)
							addrVar += identStr;
						else
							addrFld += identStr;
					}
				}

				int elemWidth = wrSrc.m_pSrcType->m_clangBitWidth;
				int reqSize = wrSrc.m_pSrcType->m_clangMinAlign == 1 ? wrSrc.m_pSrcType->m_clangBitWidth : wrSrc.m_pSrcType->m_clangMinAlign;
				int elemQwCnt = (elemWidth + reqSize - 1) / reqSize;

				if (elemQwCnt > 1)
					mifPostInstr.Append("%s\tswitch (r_t%d_memReq.m_elemQwIdx) {\n", tabs.c_str(),
						mod.m_execStg + 2);

				for (int qwIdx = 0; qwIdx < elemQwCnt; qwIdx += 1) {

					if (elemQwCnt > 1) {
						mifPostInstr.Append("%s\tcase %d:\n", tabs.c_str(), qwIdx);
						tabs += "\t";
					}

					for (CStructElemIter iter(this, wrSrc.m_pSrcType); !iter.end(); iter++) {
						if (iter.IsStructOrUnion()) continue;

						int pos = iter.GetHeirFieldPos();
						int width = iter.GetWidth();

						if (pos < qwIdx * reqSize || pos >= (qwIdx + 1) * reqSize) continue;

						if (width < 64) {
							mifPostInstr.Append("%s\tc_t%d_%sToMif_req.m_data |= (uint64_t)((m_%s%s.read_mem()%s%s & ((1LL << %d) - 1)) << %d);\n",
								tabs.c_str(),
								mod.m_execStg + 2, mod.m_modName.Lc().c_str(),
								addrVar.c_str(), varIdx.c_str(), addrFld.c_str(), iter.GetHeirFieldName().c_str(),
								width, pos % reqSize);
						} else {
							mifPostInstr.Append("%s\tc_t%d_%sToMif_req.m_data |= m_%s%s.read_mem()%s%s;\n",
								tabs.c_str(),
								mod.m_execStg + 2, mod.m_modName.Lc().c_str(),
								addrVar.c_str(), varIdx.c_str(), addrFld.c_str(), iter.GetHeirFieldName().c_str());
						}
					}

					if (elemQwCnt > 1) {
						mifPostInstr.Append("%s\tbreak;\n", tabs.c_str());
						tabs.erase(0, 1);
					}
				}

				if (elemQwCnt > 1) {
					mifPostInstr.Append("%s\tdefault:\n", tabs.c_str());
					mifPostInstr.Append("%s\t\tbreak;\n", tabs.c_str());
					mifPostInstr.Append("%s\t}\n", tabs.c_str());
				}

				mifPostInstr.Append("%s\tbreak;\n", tabs.c_str());
				tabs.erase(0, 1);
			}

			mifPostInstr.Append("%s\tdefault:\n", tabs.c_str());
			mifPostInstr.Append("%s\t\tbreak;\n", tabs.c_str());
			mifPostInstr.Append("%s\t}\n", tabs.c_str());

			if (bMultiQwReq)
				mifPostInstr.Append("\t}\n");
			mifPostInstr.NewLine();
		}
	}

	mifPostInstr.Append("\tht_uint%d c_%sToMif_reqAvlCnt = r_%sToMif_reqAvlCnt;\n",
		mif.m_queueW + 1,
		mod.m_modName.Lc().c_str(),
		mod.m_modName.Lc().c_str());
	mifPostInstr.Append("\tbool c_%sToMif_reqAvlCntBusy = r_%sToMif_reqAvlCntBusy;\n",
		mod.m_modName.Lc().c_str(),
		mod.m_modName.Lc().c_str());
	mifPostInstr.Append("\tif (r_t%d_%sToMif_reqRdy != i_mifTo%sP0_reqAvl) {\n",
		mod.m_execStg + 1 + mod.m_mif.m_mifReqStgCnt, mod.m_modName.Lc().c_str(),
		mod.m_modName.Uc().c_str());

	if (mif.m_queueW > 0) {
		mifPostInstr.Append("\t\tc_%sToMif_reqAvlCnt = r_%sToMif_reqAvlCnt + (r_t%d_%sToMif_reqRdy ? -1 : 1);\n",
			mod.m_modName.Lc().c_str(),
			mod.m_modName.Lc().c_str(),
			mod.m_execStg + 1 + mod.m_mif.m_mifReqStgCnt, mod.m_modName.Lc().c_str());
		mifPostInstr.Append("\t\tc_%sToMif_reqAvlCntBusy = r_%sToMif_reqAvlCnt < %d;\n",
			mod.m_modName.Lc().c_str(),
			mod.m_modName.Lc().c_str(),
			bMultiQwMif ? 11 : 4);
	} else {
		mifPostInstr.Append("\t\tc_%sToMif_reqAvlCnt = r_%sToMif_reqAvlCnt + 1ul;\n",
			mod.m_modName.Lc().c_str(),
			mod.m_modName.Lc().c_str());
	}
	mifPostInstr.Append("\t}\n");
	mifPostInstr.NewLine();
		
	if (mif.m_bMifRd) {
		int rdRspStg = mod.m_mif.m_mifRd.m_bNeedRdRspInfoRam ? 2 : 1;

		mifPostInstr.Append("\tbool c_m0_rdRspRdy = i_mifTo%sP0_rdRspRdy;\n",
			mod.m_modName.Uc().c_str());
		mifPostInstr.Append("\tCMemRdRspIntf c_m0_rdRsp = i_mifTo%sP0_rdRsp;\n",
			mod.m_modName.Uc().c_str());
		if (rdRspStg > 1) {
			mifPostInstr.Append("\tbool c_m1_rdRspRdy = r_m1_rdRspRdy;\n");
			mifPostInstr.Append("\tCMemRdRspIntf c_m1_rdRsp = r_m1_rdRsp;\n");
		}
		mifPostInstr.NewLine();

		if (bNeedRdRspInfo) {
			if (bNeedRdRspInfoRam) {
				mifPostInstr.Append("\tsc_uint<%s_RD_RSP_CNT_W> c_m0_rdRspId = i_mifTo%sP0_rdRsp.read().m_tid & ((1 << %s_RD_RSP_CNT_W) - 1);\n",
					mod.m_modName.Upper().c_str(),
					mod.m_modName.Uc().c_str(),
					mod.m_modName.Upper().c_str());
				mifPostInstr.Append("\tm_rdRspInfo.read_addr(c_m0_rdRspId);\n");
				mifPostInstr.Append("\tCRdRspInfo c_m1_rdRspInfo = m_rdRspInfo.read_mem();\n");
			}
			else if (bNeedRdRspInfo && bMultiQwReq) {
				mifPostInstr.Append("\tCRdRspInfo c_m%d_rdRspInfo = r_m%d_rdRspInfo;\n", rdRspStg - 1, rdRspStg);
			}
			else {
				mifPostInstr.Append("\tCRdRspInfo c_m%d_rdRspInfo;\n", rdRspStg - 1);
				mifPostInstr.Append("\tc_m%d_rdRspInfo.m_uint32 = c_m%d_rdRsp.m_tid & 0x%x;\n", rdRspStg - 1, rdRspStg - 1, (0x1 << mif.m_mifRd.m_maxRdRspInfoW) - 1);
			}

			if (rdRspGrpIdW > 0 && rdDstNgvRamList.size() > 0)
				mifPostInstr.Append("\tCRdRspInfo c_m%d_rdRspInfo = r_m%d_rdRspInfo;\n", rdRspStg, rdRspStg);
			mifPostInstr.NewLine();
		}

		if (bMultiQwReq) {
			mifPostInstr.Append("\tht_uint3 c_m%d_rdRspQwIdx = r_m%d_rdRspQwIdx;\n", rdRspStg - 1, rdRspStg);
			mifPostInstr.NewLine();
		}

		if (bMultiQwRdMif) {
			mifPostInstr.Append("\tbool c_m%d_bWrData = !r_m%d_rdRspInfo.m_multiQwReq ||\n", rdRspStg, rdRspStg);
			mifPostInstr.Append("\t\tr_m%d_rdRspInfo.m_qwFst <= r_m%d_rdRspQwIdx && r_m%d_rdRspQwIdx <= r_m%d_rdRspInfo.m_qwLst;\n",
				rdRspStg, rdRspStg, rdRspStg, rdRspStg);
			mifPostInstr.NewLine();
		}

		mifPostInstr.Append("\t// write read response to ram\n");
		mifPostInstr.Append("\tif (r_m%d_rdRspRdy) {\n", rdRspStg);

		if (bNeedRdRspInfoRam) {
			string tabs;
			if (bMultiQwReq) {
				mifPostInstr.Append("\t\tif (r_m%d_rdRspQwIdx == 0) {\n", rdRspStg);
				tabs = "\t";
			}
			mifPostInstr.Append("\t\t%ssc_uint<%s_RD_RSP_CNT_W> c_m%d_rdRspId = r_m%d_rdRsp.m_tid & ((1 << %s_RD_RSP_CNT_W) - 1);\n",
				tabs.c_str(),
				mod.m_modName.Upper().c_str(), 
				rdRspStg, rdRspStg,
				mod.m_modName.Upper().c_str());
			mifPostInstr.Append("\t\t%sm_rdRspIdPool.push(c_m%d_rdRspId);\n", tabs.c_str(), rdRspStg);
			if (bMultiQwReq)
				mifPostInstr.Append("\t\t}\n");
			mifPostInstr.NewLine();
		}

		if (bMultiQwRdMif) {
			mifPostInstr.Append("\t\tif (r_m%d_rdRspInfo.m_multiQwReq) {\n", rdRspStg);
			mifPostInstr.Append("\t\t\tc_m%d_rdRspQwIdx += 1u;\n", rdRspStg - 1);
			mifPostInstr.NewLine();

			string tabs;
			if (bMultiQwMif) {
				mifPostInstr.Append("\t\t\tif (r_m%d_rdRspQwIdx >= r_m%d_rdRspInfo.m_qwFst) {\n", rdRspStg, rdRspStg);
				tabs += "\t";
			}

			bool bNeedElemQwIdx = maxElemQwCnt > 1;
			bool bNeedVIdx1 = mif.m_vIdxWList.size() > 0 && mif.m_vIdxWList[0] > 0;
			bool bNeedVIdx2 = mif.m_vIdxWList.size() > 1 && mif.m_vIdxWList[1] > 0;
			bool bNeedElemQwCntM1 = bNeedElemQwIdx && (bNeedVIdx1 || bNeedVIdx2);
			bool bNeedVIdx2CntM1 = bNeedVIdx2 && bNeedVIdx1;

			if (bNeedElemQwIdx) {
				if (bNeedElemQwCntM1) {
					mifPostInstr.Append("%s\t\t\tif (r_m%d_rdRspInfo.m_elemQwIdx < r_m%d_rdRspInfo.m_elemQwCntM1) {\n\t", 
						tabs.c_str(), rdRspStg, rdRspStg);
				}
				mifPostInstr.Append("%s\t\t\tc_m%d_rdRspInfo.m_elemQwIdx = r_m%d_rdRspInfo.m_elemQwIdx + 1u;\n",
					tabs.c_str(), rdRspStg - 1, rdRspStg);

				if (bNeedElemQwCntM1)
					mifPostInstr.Append("%s\t\t\t}", tabs.c_str());
			}

			if (bNeedVIdx2) {
				if (bNeedElemQwIdx)
					mifPostInstr.Append(" else ");
				else
					mifPostInstr.Append("%s\t\t\t", tabs.c_str());

				if (bNeedVIdx2CntM1) {
					mifPostInstr.Append("if (r_m%d_rdRspInfo.m_vIdx2 < r_m%d_rdRspInfo.m_vIdx2CntM1) {\n\t", rdRspStg, rdRspStg);
				} else if (bNeedElemQwIdx) {
					mifPostInstr.Append("{\n");
					mifPostInstr.Append("%s\t\t\t\tc_m%d_rdRspInfo.m_elemQwIdx = 0;\n\t", tabs.c_str(), rdRspStg - 1);
				}
				mifPostInstr.Append("%s\t\t\tc_m%d_rdRspInfo.m_vIdx2 = r_m%d_rdRspInfo.m_vIdx2 + 1u;\n", tabs.c_str(), rdRspStg - 1, rdRspStg);

				if (bNeedVIdx2CntM1 || bNeedElemQwIdx) {
					if (bNeedVIdx1)
						mifPostInstr.Append("%s\t\t\t}", tabs.c_str());
					else
						mifPostInstr.Append("%s\t\t\t}\n", tabs.c_str());
				}
			}

			if (bNeedVIdx1) {
				if (bNeedElemQwCntM1 || bNeedVIdx2CntM1)
					mifPostInstr.Append(" else {\n\t");
				else
					mifPostInstr.Append("%s\t\t\t", tabs.c_str());

				if (bNeedElemQwIdx)
					mifPostInstr.Append("%s\t\t\tc_m%d_rdRspInfo.m_elemQwIdx = 0;\n\t", tabs.c_str(), rdRspStg - 1);

				if (bNeedVIdx2)
					mifPostInstr.Append("%s\t\t\tc_m%d_rdRspInfo.m_vIdx2 = 0;\n\t", tabs.c_str(), rdRspStg - 1);

				mifPostInstr.Append("%s\t\t\tc_m%d_rdRspInfo.m_vIdx1 = r_m%d_rdRspInfo.m_vIdx1 + 1u;\n", tabs.c_str(), rdRspStg - 1, rdRspStg);

				if (bNeedElemQwCntM1 || bNeedVIdx2CntM1)
					mifPostInstr.Append("%s\t\t\t}\n", tabs.c_str());
			}

			if (bMultiQwMif)
				mifPostInstr.Append("\t\t\t}\n");

			mifPostInstr.Append("\t\t}\n");
			mifPostInstr.NewLine();
		}

		mifPostInstr.Append("\t\tht_uint64 c_m%d_rdRspData = r_m%d_rdRsp.m_data;\n",
			rdRspStg, rdRspStg);
		mifPostInstr.NewLine();

		string tabs;
		if (bMultiQwRdMif) {
			mifPostInstr.Append("\t\tif (c_m%d_bWrData) {\n", rdRspStg);
			tabs += "\t";
		}

		if (mif.m_mifRd.m_rdDstList.size() > 1) {
			mifPostInstr.Append("%s\t\tswitch (r_m%d_rdRspInfo.m_dst) {\n", tabs.c_str(), rdRspStg);
			tabs += "\t";
		}

		for (int rdDstIdx = 0; rdDstIdx < (int)mif.m_mifRd.m_rdDstList.size(); rdDstIdx += 1) {
			CMifRdDst &rdDst = mif.m_mifRd.m_rdDstList[rdDstIdx];

			if (mif.m_mifRd.m_rdDstList.size() > 1) {
				tabs.erase(0, 1);
				mifPostInstr.Append("%s\t\tcase %d:\n", tabs.c_str(), rdDstIdx);
				tabs += "\t";
			}

			if (rdDst.m_infoW.size() > 0) {
				// function call

				//if (rdDst.m_infoW.AsInt() > 0) {
				//	mifPostInstr.Append("%ssc_uint<%s_MIF_DST_%s_INFO_W> c_m1_rdRsp_info = (c_m1_rdRsp_dst\n", pTabs,
				//		mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());
				//	mifPostInstr.Append("%s\t>> %s_MIF_DST_%s_INFO_SHF) & %s_MIF_DST_%s_INFO_MSK;\n", pTabs,
				//		mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(),
				//		mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());
				//}
				//mifPostInstr.NewLine();

				//if (mifRdDst.m_bMultiRd && !is_wx) {
				//	mifPostInstr.Append("%ssc_uint<%s_MIF_DST_%s_RSP_IDX_W> c_m1_rspIdx = (c_m1_rdRsp_dst\n", pTabs,
				//		mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());
				//	mifPostInstr.Append("%s\t>> %s_MIF_DST_%s_RSP_IDX_SHF) & %s_MIF_DST_%s_RSP_IDX_MSK;\n", pTabs,
				//		mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(),
				//		mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());

				//	//if (bNeed_rdRspQwIdx)
				//	//	mifPostInstr.Append("%sc_m1_rspIdx += c_m1_rdRspQwIdx;\n", pTabs);
				//	mifPostInstr.NewLine();
				//}

				//if (is_wx && mifRdDst.m_bMultiRd)
				//	mifPostInstr.Append("%sif (c_m1_varWrEn)\n\t", pTabs);

				mifPostInstr.Append("\t\t%sReadMemResp_%s(", tabs.c_str(), rdDst.m_name.c_str());
				m_mifFuncDecl.Append("\tvoid ReadMemResp_%s(", rdDst.m_name.c_str());

				//if (mifRdDst.m_bMultiRd) {
				//	if (is_wx)
				//		mifPostInstr.Append("c_m1_rdRspQwIdx - c_m1_rdRspQwFirst, ");
				//	else
				//		mifPostInstr.Append("c_m1_rspIdx, ");
				//	m_mifFuncDecl.Append("ht_uint3 rspIdx, ");
				//}

				if (mif.m_mifRd.m_rspGrpIdW > 0) {
					mifPostInstr.Append("r_m%d_rdRspInfo.m_grpId, ", rdRspStg);
					m_mifFuncDecl.Append("sc_uint<%s_RD_GRP_ID_W> rdRspGrpId, ", mod.m_modName.Upper().c_str());
				}

				if (rdDst.m_infoW.AsInt() > 0) {
					mifPostInstr.Append("r_m%d_rdRspInfo.m_d%d_info, ", rdRspStg, rdDstIdx);
					m_mifFuncDecl.Append("sc_uint<%s> rdRsp_info, ", rdDst.m_infoW.c_str());
				}

				mifPostInstr.Append("c_m%d_rdRspData);\n", rdRspStg);
				m_mifFuncDecl.Append("ht_uint64 rdRspData);\n");

			} else {

				CType * pDstType = 0;
				string baseVar;

				ERamType ramType = eRegRam;
				bool bMemVar = false;
				bool bQueVar = false;
				if (rdDst.m_pGblVar) {
					pDstType = rdDst.m_pDstType;
					baseVar = VA("c_m%d_%sMwData", rdRspStg, rdDst.m_pGblVar->m_gblName.c_str());
					ramType = rdDst.m_pGblVar->m_ramType;
				}
				else if (rdDst.m_pSharedVar) {
					pDstType = rdDst.m_pDstType;
					if (rdDst.m_pSharedVar->m_addr1W.AsInt() > 0) {
						baseVar = VA("m__SHR__%s", rdDst.m_pSharedVar->m_name.c_str());
						bMemVar = true;
					} else if (rdDst.m_pSharedVar->m_queueW.AsInt() > 0) {
						baseVar = VA("m__SHR__%s", rdDst.m_pSharedVar->m_name.c_str());
						bQueVar = true;
					} else {
						baseVar = VA("c__SHR__%s", rdDst.m_pSharedVar->m_name.c_str());
					}
					ramType = rdDst.m_pSharedVar->m_ramType;
				}
				else if (rdDst.m_pPrivVar) {
					pDstType = rdDst.m_pDstType;
					baseVar = VA("c_htPriv.m_%s", rdDst.m_pPrivVar->m_name.c_str());
				} else
					HtlAssert(0);

				string addrVar = baseVar;
				string addrFld;
				for (int fldIdx = 0; fldIdx < (int)rdDst.m_fieldRefList.size(); fldIdx += 1) {
					CFieldRef & fieldRef = rdDst.m_fieldRefList[fldIdx];

					string identStr;

					if (fldIdx > 0) {
						identStr += '.';
						identStr += fieldRef.m_fieldName;
					}

					for (int dimIdx = 0; dimIdx < (int)fieldRef.m_refDimenList.size(); dimIdx += 1) {
						CRefDimen & refDimen = fieldRef.m_refDimenList[dimIdx];

						if (refDimen.m_value >= 0)
							identStr += VA("[%d]", refDimen.m_value);

						else if (refDimen.m_size == 1)
							identStr += "[0]";

						else if (refDimen.m_isIdx) {
							identStr += VA("[INT(r_m%d_rdRspInfo.m_vIdx%d & 0x%x)]", rdRspStg,
								dimIdx + 1, (1 << FindLg2(refDimen.m_size - 1)) - 1);
						} else {
							identStr += VA("[INT(r_m%d_rdRspInfo.m_d%d_f%dIdx%d)]", rdRspStg,
								rdDstIdx, fldIdx + 1, dimIdx + 1);
						}
					}

					baseVar += identStr;
					if (fldIdx == 0)
						addrVar += identStr;
					else
						addrFld += identStr;
				}

				if (rdDst.m_varAddr1W > 0) {

					if (rdDst.m_varAddr1W > 0 && !rdDst.m_varAddr1IsHtId) {
						if (rdDst.m_varAddr1IsIdx) {
							mifPostInstr.Append("\t\t%s%s.write_addr(r_m%d_rdRspInfo.m_vIdx1 & 0x%x",
								tabs.c_str(),
								addrVar.c_str(),
								rdRspStg,
								(1 << rdDst.m_varAddr1W) - 1);
						} else {
							mifPostInstr.Append("\t\t%s%s.write_addr(r_m%d_rdRspInfo.m_d%d_f1Addr1",
								tabs.c_str(),
								addrVar.c_str(),
								rdRspStg,
								rdDstIdx);
						}
					} else if (rdDst.m_varAddr2W > 0 && !rdDst.m_varAddr2IsHtId) {
						mifPostInstr.Append("\t\t%s%s.write_addr(",
							tabs.c_str(),
							addrVar.c_str());
					}

					if (rdDst.m_varAddr1W > 0 && !rdDst.m_varAddr1IsHtId && rdDst.m_varAddr2W > 0 && !rdDst.m_varAddr2IsHtId)
						mifPostInstr.Append(", ");

					if (rdDst.m_varAddr2W > 0 && !rdDst.m_varAddr2IsHtId) {
						if (rdDst.m_varAddr2IsIdx)
							mifPostInstr.Append("r_m%d_rdRspInfo.m_vIdx2 & 0x%x", rdRspStg,
							(1 << rdDst.m_varAddr2W) - 1);
						else
							mifPostInstr.Append("r_m%d_rdRspInfo.m_d%d_f1Addr2", rdRspStg, rdDstIdx);
					}

					if (rdDst.m_varAddr1W > 0 && !rdDst.m_varAddr1IsHtId || rdDst.m_varAddr2W > 0 && !rdDst.m_varAddr2IsHtId)
						mifPostInstr.Append(");\n");
				}

				int reqSize = rdDst.m_pRdType ? (rdDst.m_pRdType->m_clangMinAlign == 1 ? rdDst.m_pRdType->m_clangBitWidth : rdDst.m_pRdType->m_clangMinAlign)
					: (pDstType->m_clangMinAlign == 1 ? pDstType->m_clangBitWidth : pDstType->m_clangMinAlign);

				if (rdDst.m_pRdType == 0 && pDstType->m_clangBitWidth > reqSize) {
					mifPostInstr.Append("%s\t\tswitch (r_m%d_rdRspInfo.m_elemQwIdx & 0x%x) {\n",
						tabs.c_str(), rdRspStg, (1 << FindLg2((pDstType->m_clangBitWidth + reqSize - 1) / reqSize - 1)) - 1);
					tabs += "\t";
				}

				for (int bitPos = 0; bitPos < pDstType->m_clangBitWidth; bitPos += reqSize) {
					if (rdDst.m_pRdType == 0 && pDstType->m_clangBitWidth > reqSize) {
						tabs.erase(0, 1);
						mifPostInstr.Append("%s\t\tcase %d:\n", tabs.c_str(), bitPos / reqSize);
						tabs += "\t";
					}

					for (CStructElemIter iter(this, pDstType); !iter.end(); iter++) {

						int pos = iter.GetHeirFieldPos();
						int width = iter.GetWidth();
						bool bIsSigned = iter.IsSigned();
						int minAlign = iter.GetMinAlign();

						if (pos < bitPos || pos >= (bitPos + reqSize)) continue;

						int rdTypeWidth = rdDst.m_pRdType ? rdDst.m_pRdType->m_clangBitWidth : 64;
						int dstTypeWidth = width;

						string typeCast;
						if (rdTypeWidth < dstTypeWidth)
						//if (rdDst.m_pRdType)
							typeCast = rdDst.m_pRdType->m_typeName;
						else if (width < 64 || bIsSigned) {
							if (minAlign != width) {
								if (bIsSigned)
									typeCast = VA("ht_int%d", width);
								else
									typeCast = VA("ht_uint%d", width);
							} else 
								typeCast = iter.GetType()->m_typeName;
						}

						if (width < 64) {
							if (bMemVar) {
								if (ramType == eDistRam) {
									mifPostInstr.Append("\t\t%s%s.write_mem()%s%s = (%s)(c_m%d_rdRspData >> %d);\n",
										tabs.c_str(),
										addrVar.c_str(),
										addrFld.c_str(),
										iter.GetHeirFieldName().c_str(),
										typeCast.c_str(),
										rdRspStg,
										pos % reqSize);
								} else {
									mifPostInstr.Append("\t\t%s%s%s.write_mem((%s)(c_m%d_rdRspData >> %d));\n",
										tabs.c_str(),
										baseVar.c_str(),
										iter.GetHeirFieldName().c_str(),
										typeCast.c_str(),
										rdRspStg,
										pos % reqSize);
								}
							} else {
								mifPostInstr.Append("\t\t%s%s%s = (%s)(c_m%d_rdRspData >> %d);\n",
									tabs.c_str(),
									baseVar.c_str(),
									iter.GetHeirFieldName().c_str(),
									typeCast.c_str(),
									rdRspStg,
									pos % reqSize);
							}
						} else {
							if (bMemVar) {
								if (ramType == eDistRam) {
									mifPostInstr.Append("\t\t%s%s.write_mem()%s%s = c_m%d_rdRspData;\n",
										tabs.c_str(),
										addrVar.c_str(),
										addrFld.c_str(),
										iter.GetHeirFieldName().c_str(),
										rdRspStg);
								}
								else {
									mifPostInstr.Append("\t\t%s%s%s.write_mem(c_m%d_rdRspData);\n",
										tabs.c_str(),
										baseVar.c_str(),
										iter.GetHeirFieldName().c_str(),
										rdRspStg);
								}
							} else if (bQueVar) {
								mifPostInstr.Append("\t\t%s%s%s.push(c_m%d_rdRspData);\n",
									tabs.c_str(),
									baseVar.c_str(),
									iter.GetHeirFieldName().c_str(),
									rdRspStg);
							} else if (typeCast.size() == 0) {
								mifPostInstr.Append("\t\t%s%s%s = c_m%d_rdRspData;\n",
									tabs.c_str(),
									baseVar.c_str(),
									iter.GetHeirFieldName().c_str(),
									rdRspStg);
							} else {
								mifPostInstr.Append("%s\t\t%s%s = (%s)c_m%d_rdRspData;\n",
									tabs.c_str(),
									baseVar.c_str(),
									iter.GetHeirFieldName().c_str(),
									typeCast.c_str(),
									rdRspStg);
							}
						}
					}

					if (rdDst.m_pRdType == 0 && pDstType->m_clangBitWidth > reqSize) {
						mifPostInstr.Append("%s\t\tbreak;\n", tabs.c_str());
					}
				}

				if (rdDst.m_pRdType == 0 && pDstType->m_clangBitWidth > reqSize) {
					tabs.erase(0, 1);
					mifPostInstr.Append("%s\t\tdefault:\n", tabs.c_str());
					mifPostInstr.Append("%s\t\t\tht_assert(0);\n", tabs.c_str());
					mifPostInstr.Append("%s\t\t}\n", tabs.c_str());
				}
			}

			if (mif.m_mifRd.m_rdDstList.size() > 1) {
				mifPostInstr.Append("%s\t\tbreak;\n", tabs.c_str());
			}
		}

		if (mif.m_mifRd.m_rdDstList.size() > 1) {
			tabs.erase(0, 1);
			mifPostInstr.Append("%s\t\tdefault:\n", tabs.c_str());
			mifPostInstr.Append("%s\t\t\tht_assert(0);\n", tabs.c_str());
			mifPostInstr.Append("%s\t\t}\n", tabs.c_str());
		}

		if (bMultiQwRdMif) {
			tabs.erase(0, 1);
			mifPostInstr.Append("%s\t\t}\n", tabs.c_str());
		}

		mifPostInstr.Append("\t}\n");
		mifPostInstr.NewLine();

		if (bNeedRdRspInfo && bMultiQwReq) {
			mifPostInstr.Append("\tif (c_m%d_rdRspQwIdx == 0)\n", rdRspStg - 1);
			if (bNeedRdRspInfoRam) {
				mifPostInstr.Append("\t\tc_m%d_rdRspInfo = m_rdRspInfo.read_mem();\n", rdRspStg - 1);
			} else {
				mifPostInstr.Append("\t\tc_m%d_rdRspInfo.m_uint32 = c_m%d_rdRsp.m_tid & 0x%x;\n",
					rdRspStg - 1, rdRspStg - 1, (0x1 << mif.m_mifRd.m_maxRdRspInfoW) - 1);
			}
			mifPostInstr.NewLine();
		}
	}

	if (mif.m_bMifRd) {
		int rdRspStg = mod.m_mif.m_mifRd.m_bNeedRdRspInfoRam ? 2 : 1;

		string rdReqRdy;
		if (bMultiQwReq) {
			rdReqRdy = VA("r_t%d_memReq.m_rdReq && !r_memReqHold", mod.m_execStg + 1);
		} else {
			rdReqRdy = VA("r_t%d_memReq.m_rdReq", mod.m_execStg + 1);
		}

		string rdRspRdy;
		string rdRspRdyWithParan;
		string rdRspGrpId;
		if (rdDstNgvRamList.size() == 0) {
			rdRspGrpId = VA("r_m%d_rdRspInfo.m_grpId", rdRspStg);

			if (bMultiQwRdMif) {
				rdRspRdy = VA("r_m%d_rdRspRdy && c_m%d_bWrData", rdRspStg, rdRspStg);
				rdRspRdyWithParan = "(" + rdRspRdy + ")";
			} else {
				rdRspRdy = VA("r_m%d_rdRspRdy", rdRspStg);
				rdRspRdyWithParan = rdRspRdy;
			}

		} else {

			rdRspGrpId = "r_rdCompGrpId";

			rdRspRdy = "r_rdCompRdy";
			rdRspRdyWithParan = rdRspRdy;
			}

		if (rdRspGrpIdW == 0) {

			mifPostInstr.Append("\tif ((%s) != %s)\n", rdReqRdy.c_str(), rdRspRdyWithParan.c_str());

			mifPostInstr.Append("\t\tc_rdGrpState.m_cnt = r_rdGrpState.m_cnt + (%s ? -1 : 1);\n", rdRspRdyWithParan.c_str());

		} else if (rdRspGrpIdW <= 2) {

			mifPostInstr.Append("\tfor (int grpId = 0; grpId < (1 << %s_RD_GRP_ID_W); grpId += 1) {\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.Append("\t\tif ((%s && r_t%d_rdRspInfo.m_grpId == grpId) != (%s && %s == grpId))\n",
				rdReqRdy.c_str(),
				mod.m_execStg + 1,
				rdRspRdyWithParan.c_str(), rdRspGrpId.c_str());

			mifPostInstr.Append("\t\t\tc_rdGrpState[grpId].m_cnt = r_rdGrpState[grpId].m_cnt + ((%s && %s == grpId) ? -1 : 1);\n", rdRspRdyWithParan.c_str(), rdRspGrpId.c_str());

			mifPostInstr.Append("\t}\n");
			mifPostInstr.NewLine();

			mifPostInstr.Append("\tif ((%s) != %s)\n", rdReqRdy.c_str(), rdRspRdyWithParan.c_str());

			mifPostInstr.Append("\t\tc_rdGrpBusyCnt = r_rdGrpBusyCnt + (%s ? -1 : 1);\n", rdRspRdyWithParan.c_str());

		} else {

			mifPostInstr.Append("\tsc_uint<%s_RD_GRP_ID_W> c_t%d_rdGrpId = r_t%d_rdRspInfo.m_grpId;\n",
				mod.m_modName.Upper().c_str(), mod.m_execStg + 1, mod.m_execStg + 1);
			mifPostInstr.Append("\tm_rdGrpReqState0.read_addr(c_t%d_rdGrpId);\n", mod.m_execStg + 1);
			mifPostInstr.Append("\tm_rdGrpRspState1.read_addr(c_t%d_rdGrpId);\n", mod.m_execStg + 1);
			mifPostInstr.Append("\tm_rdGrpReqState0.write_addr(c_t%d_rdGrpId);\n", mod.m_execStg + 1);
			if (mif.m_mifRd.m_bPause)
				mifPostInstr.Append("\tm_rdGrpReqState1.write_addr(c_t%d_rdGrpId);\n", mod.m_execStg + 1);
			mifPostInstr.NewLine();

			mifPostInstr.Append("\tCRdGrpReqState c_t%d_rdGrpReqState = m_rdGrpReqState0.read_mem();\n", mod.m_execStg + 1);
			mifPostInstr.Append("\tCRdGrpReqState c_t%d_rdGrpReqStateWr = c_t%d_rdGrpReqState;\n", mod.m_execStg + 1, mod.m_execStg + 1);
			mifPostInstr.Append("\tCRdGrpRspState c_t%d_rdGrpRspState = m_rdGrpRspState1.read_mem();\n", mod.m_execStg + 1);
			mifPostInstr.NewLine();

			mifPostInstr.Append("\tif (%s) {\n", rdReqRdy.c_str(), rdRspRdy.c_str());

			mifPostInstr.Append("\t\tc_rdGrpBusyCnt = r_rdGrpBusyCnt + 1;\n");
			mifPostInstr.Append("\t\tc_t%d_rdGrpReqStateWr.m_cnt = c_t%d_rdGrpReqState.m_cnt + 1;\n",
				mod.m_execStg + 1,
				mod.m_execStg + 1);
			mifPostInstr.Append("\t}\n");
		}
		mifPostInstr.NewLine();

		if (mif.m_mifRd.m_bPoll) {
			mifPostInstr.Append("\tif (r_t%d_memReq.m_rdPoll%s) {\n", mod.m_execStg + 1, andNot_memReqHold.c_str());

			if (rdRspGrpIdW == 0) {
				mifPostInstr.Append("\t\tif (r_rdGrpState.m_cnt == 0 && !r_t%d_memReq.m_rdReq) {\n", mod.m_execStg + 1);
			} else if (rdRspGrpIdW <= 2) {
				mifPostInstr.Append("\t\tif (r_rdGrpState[r_t%d_rdRspInfo.m_grpId].m_cnt == 0 && !r_t%d_memReq.m_rdReq) {\n",
					mod.m_execStg + 1, mod.m_execStg + 1);
			} else {
				mifPostInstr.Append("\t\tif (c_t%d_rdGrpReqStateWr.m_cnt == c_t%d_rdGrpRspState.m_cnt && !r_t%d_memReq.m_rdReq) {\n",
					mod.m_execStg + 1, mod.m_execStg + 1, mod.m_execStg + 1);
			}

			if (mod.m_rsmSrcCnt > 1 && mod.m_threads.m_htIdW.AsInt() > 0) {
				bNeedMifContQue = true;
				mifPostInstr.Append("\t\t\tCHtCmd mifCont;\n");
				mifPostInstr.Append("\t\t\tmifCont.m_htId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tmifCont.m_htInstr = r_t%d_memReq.m_rsmInstr;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tm_mifContQue.push(mifCont);\n");
			} else {
				mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
				if (mod.m_threads.m_htIdW.AsInt() > 0)
					mifPostInstr.Append("\t\t\tc_t0_rsmHtId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = r_t%d_memReq.m_rsmInstr;\n", mod.m_execStg + 1);
			}

			mifPostInstr.Append("\t\t} else {\n");

			if (mod.m_threads.m_htIdW.AsInt() > 2) {
				bNeedMifPollQue = true;
				mifPostInstr.Append("\t\t\tCHtCmd mifCont;\n");
				mifPostInstr.Append("\t\t\tmifCont.m_htId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tmifCont.m_htInstr = r_t%d_memReq.m_pollInstr;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tm_mifPollQue.push(mifCont);\n");

			} else if (mod.m_rsmSrcCnt > 1 && mod.m_threads.m_htIdW.AsInt() > 0) {
				bNeedMifContQue = true;
				mifPostInstr.Append("\t\t\tCHtCmd mifCont;\n");
				mifPostInstr.Append("\t\t\tmifCont.m_htId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tmifCont.m_htInstr = r_t%d_memReq.m_pollInstr;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tm_mifContQue.push(mifCont);\n");
			} else {
				mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
				if (mod.m_threads.m_htIdW.AsInt() > 0)
					mifPostInstr.Append("\t\t\tc_t0_rsmHtId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = r_t%d_memReq.m_pollInstr;\n", mod.m_execStg + 1);
			}

			mifPostInstr.Append("\t\t}\n");
			mifPostInstr.Append("\t}\n");
			mifPostInstr.NewLine();
		}

		if (mif.m_mifRd.m_bPause) {
			if (rdRspGrpIdW == 0) {

				mifPostInstr.Append("\tif (r_t%d_memReq.m_rdPause%s) {\n", mod.m_execStg + 1, andNot_memReqHold.c_str());
				mifPostInstr.Append("\t\tif (r_rdGrpState.m_cnt == 0 && !r_t%d_memReq.m_rdReq) {\n", mod.m_execStg + 1);

				if (mod.m_threads.m_htIdW.AsInt() == 0) {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = r_t%d_memReq.m_rsmInstr;\n",
						mod.m_execStg + 1);

				} else {
					bNeedMifContQue = true;
					mifPostInstr.Append("\t\t\tCHtCmd mifCont;\n");
					mifPostInstr.Append("\t\t\tmifCont.m_htId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
					mifPostInstr.Append("\t\t\tmifCont.m_htInstr = r_t%d_memReq.m_rsmInstr;\n", mod.m_execStg + 1);
					mifPostInstr.Append("\t\t\tm_mifContQue.push(mifCont);\n");
				}

				mifPostInstr.Append("\t\t} else {\n");
				mifPostInstr.Append("\t\t\tc_rdGrpState.m_pause = true;\n",
					mod.m_execStg + 1, mod.m_execStg + 1);

				if (!bRdRspGrpIdIsHtId && mod.m_threads.m_htIdW.AsInt() > 0) {
					mifPostInstr.Append("\t\t\tc_rdGrpState.m_rsmHtId = r_t%d_memReq.m_rsmHtId;\n",
						mod.m_execStg + 1);
				}
				mifPostInstr.Append("\t\t\tc_rdGrpState.m_rsmInstr = r_t%d_memReq.m_rsmInstr;\n",
					mod.m_execStg + 1);
				mifPostInstr.Append("\t\t}\n");
				mifPostInstr.Append("\t}\n");
				mifPostInstr.NewLine();

			} else if (rdRspGrpIdW <= 2) {

				mifPostInstr.Append("\tif (r_t%d_memReq.m_rdPause%s) {\n", mod.m_execStg + 1, andNot_memReqHold.c_str());
				mifPostInstr.Append("\t\tif (r_rdGrpState[r_t%d_rdRspInfo.m_grpId].m_cnt == 0 && !r_t%d_memReq.m_rdReq) {\n",
					mod.m_execStg + 1, mod.m_execStg + 1);

				if (mod.m_threads.m_htIdW.AsInt() == 0) {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = r_t%d_memReq.m_rsmInstr;\n",
						mod.m_execStg + 1);

				} else {
					bNeedMifContQue = true;
					mifPostInstr.Append("\t\t\tCHtCmd mifCont;\n");
					mifPostInstr.Append("\t\t\tmifCont.m_htId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
					mifPostInstr.Append("\t\t\tmifCont.m_htInstr = r_t%d_memReq.m_rsmInstr;\n", mod.m_execStg + 1);
					mifPostInstr.Append("\t\t\tm_mifContQue.push(mifCont);\n");
				}

				mifPostInstr.Append("\t\t} else {\n");
				mifPostInstr.Append("\t\t\tc_rdGrpState[r_t%d_rdRspInfo.m_grpId].m_pause = true;\n",
					mod.m_execStg + 1);

				if (!bRdRspGrpIdIsHtId && mod.m_threads.m_htIdW.AsInt() > 0) {
					mifPostInstr.Append("\t\t\tc_rdGrpState[r_t%d_rdRspInfo.m_grpId].m_rsmHtId = r_t%d_memReq.m_rsmHtId;\n",
						mod.m_execStg + 1, mod.m_execStg + 1);
				}
				mifPostInstr.Append("\t\t\tc_rdGrpState[r_t%d_rdRspInfo.m_grpId].m_rsmInstr = r_t%d_memReq.m_rsmInstr;\n",
					mod.m_execStg + 1, mod.m_execStg + 1);
				mifPostInstr.Append("\t\t}\n");
				mifPostInstr.Append("\t}\n");
				mifPostInstr.NewLine();

			} else {

				mifPostInstr.Append("\tif (r_t%d_memReq.m_rdPause%s) {\n", mod.m_execStg + 1, andNot_memReqHold.c_str());
				mifPostInstr.Append("\t\tif (c_t%d_rdGrpReqStateWr.m_cnt == c_t%d_rdGrpRspState.m_cnt && !r_t%d_memReq.m_rdReq) {\n",
					mod.m_execStg + 1, mod.m_execStg + 1, mod.m_execStg + 1);

				if (mod.m_threads.m_htIdW.AsInt() == 0) {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = r_t%d_memReq.m_rsmInstr;\n",
						mod.m_execStg + 1);

				} else {
					bNeedMifContQue = true;
					mifPostInstr.Append("\t\t\tCHtCmd mifCont;\n");
					mifPostInstr.Append("\t\t\tmifCont.m_htId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
					mifPostInstr.Append("\t\t\tmifCont.m_htInstr = r_t%d_memReq.m_rsmInstr;\n", mod.m_execStg + 1);
					mifPostInstr.Append("\t\t\tm_mifContQue.push(mifCont);\n");
				}

				mifPostInstr.Append("\t\t} else {\n");
				mifPostInstr.Append("\t\t\tc_t%d_rdGrpReqStateWr.m_pause = c_t%d_rdGrpRspState.m_pause ^ 1;\n",
					mod.m_execStg + 1, mod.m_execStg + 1);

				if (!bRdRspGrpIdIsHtId && mod.m_threads.m_htIdW.AsInt() > 0) {
					mifPostInstr.Append("\t\t\tc_t%d_rdGrpReqStateWr.m_rsmHtId = r_t%d_memReq.m_rsmHtId;\n",
						mod.m_execStg + 1, mod.m_execStg + 1);
				}
				mifPostInstr.Append("\t\t\tc_t%d_rdGrpReqStateWr.m_rsmInstr = r_t%d_memReq.m_rsmInstr;\n",
					mod.m_execStg + 1, mod.m_execStg + 1);
				mifPostInstr.Append("\t\t}\n");
				mifPostInstr.Append("\t}\n");
				mifPostInstr.NewLine();
			}
		}

		if (rdRspGrpIdW > 2) {
			mifPostInstr.Append("\tm_rdGrpReqState0.write_mem(c_t%d_rdGrpReqStateWr);\n",
				mod.m_execStg + 1);
			if (mif.m_mifRd.m_bPause) {
				mifPostInstr.Append("\tm_rdGrpReqState1.write_mem(c_t%d_rdGrpReqStateWr);\n",
					mod.m_execStg + 1);
			}
			mifPostInstr.NewLine();

			if (MIF_CHK_STATE) {
				GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CRdGrpReqState ht_noload", "x_rdGrpReqState");

				mifPostInstr.Append("\tif (c_t%d_rdGrpId == %s_CHK_GRP_ID)\n", mod.m_execStg + 1, mod.m_modName.Upper().c_str());
				mifPostInstr.Append("\t\tx_rdGrpReqState = c_t%d_rdGrpReqStateWr;\n", mod.m_execStg + 1);
				mifPostInstr.NewLine();
			}
		}

		if (rdRspGrpIdW > 2) {

			if (mif.m_mifRd.m_bPause)
				mifPostInstr.Append("\tm_rdGrpReqState1.read_addr(%s);\n", rdRspGrpId.c_str());
			mifPostInstr.Append("\tm_rdGrpRspState0.read_addr(%s);\n", rdRspGrpId.c_str());
			mifPostInstr.Append("\tm_rdGrpRspState0.write_addr(%s);\n", rdRspGrpId.c_str());
			mifPostInstr.Append("\tm_rdGrpRspState1.write_addr(%s);\n", rdRspGrpId.c_str());
			mifPostInstr.NewLine();

			mifPostInstr.Append("\tCRdGrpRspState c_m%d_rdGrpRspState = m_rdGrpRspState0.read_mem();\n", rdRspStg);
			mifPostInstr.Append("\tCRdGrpRspState c_m%d_rdGrpRspStateWr = c_m%d_rdGrpRspState;\n", rdRspStg, rdRspStg);
			if (mif.m_mifRd.m_bPause) {
				mifPostInstr.Append("\tCRdGrpReqState c_m%d_rdGrpReqState = m_rdGrpReqState1.read_mem();\n", rdRspStg);
			}
			mifPostInstr.NewLine();
		}

		if (mif.m_mifRd.m_bPause) {
			if (rdRspGrpIdW == 0) {

				mifPostInstr.Append("\tif (%s) {\n", rdRspRdy.c_str());
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tht_assert(%s || r_rdGrpState.m_cnt != 0);\n",
					reset.c_str());
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tbool c_rdGrpCollision = r_t%d_memReq.m_rdPause && !r_t%d_memReq.m_rdReq%s;\n",
					mod.m_execStg + 1, mod.m_execStg + 1, andNot_memReqHold.c_str());
				mifPostInstr.Append("\t\tbool c_rdGrpResume = r_rdGrpState.m_pause || c_rdGrpCollision;\n");
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tif (c_rdGrpState.m_cnt == 0 && c_rdGrpResume) {\n");
				mifPostInstr.Append("\t\t\t// last read response arrived, resume thread\n");
				mifPostInstr.NewLine();

				if (mif.m_mifRd.m_bPause)
					mifPostInstr.Append("\t\t\tc_rdGrpState.m_pause = false;\n");
				mifPostInstr.NewLine();

				if (mod.m_threads.m_htIdW.AsInt() == 0) {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = r_rdGrpState.m_rsmInstr;\n");

				} else {

					bNeedMifRdRsmQue = true;
					mifPostInstr.Append("\t\t\tCHtCmd rdRsm;\n");
					mifPostInstr.Append("\t\t\trdRsm.m_htId = r_rdGrpState.m_rsmHtId;\n");
					mifPostInstr.Append("\t\t\trdRsm.m_htInstr = r_rdGrpState.m_rsmInstr;\n");
					mifPostInstr.Append("\t\t\tm_mifRdRsmQue.push(rdRsm);\n");
				}

				mifPostInstr.Append("\t\t}\n");
				mifPostInstr.Append("\t}\n");
				mifPostInstr.NewLine();

			} else if (rdRspGrpIdW <= 2) {

				mifPostInstr.Append("\tif (%s) {\n", rdRspRdy.c_str());
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tht_assert(%s || r_rdGrpState[%s].m_cnt != 0);\n",
					reset.c_str(), rdRspGrpId.c_str());
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tbool c_rdGrpCollision = r_t%d_memReq.m_rdPause && !r_t%d_memReq.m_rdReq%s && r_t%d_memReq.m_rdGrpId == %s;\n",
					mod.m_execStg + 1, mod.m_execStg + 1, andNot_memReqHold.c_str(), mod.m_execStg + 1, rdRspGrpId.c_str());
				mifPostInstr.Append("\t\tbool c_rdGrpResume = r_rdGrpState[%s].m_pause || c_rdGrpCollision;\n",
					rdRspGrpId.c_str());
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tif (c_rdGrpState[%s].m_cnt == 0 && c_rdGrpResume) {\n", rdRspGrpId.c_str());
				mifPostInstr.Append("\t\t\t// last read response arrived, resume thread\n");
				mifPostInstr.NewLine();

				if (mif.m_mifRd.m_bPause)
					mifPostInstr.Append("\t\t\tc_rdGrpState[%s].m_pause = false;\n", rdRspGrpId.c_str());
				mifPostInstr.NewLine();

				if (mod.m_threads.m_htIdW.AsInt() == 0) {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = r_rdGrpState[%s].m_rsmInstr;\n", rdRspGrpId.c_str());

				} else {

					bNeedMifRdRsmQue = true;
					mifPostInstr.Append("\t\t\tCHtCmd rdRsm;\n");

					if (bRdRspGrpIdIsHtId) {
						mifPostInstr.Append("\t\t\trdRsm.m_htId = %s;\n", rdRspGrpId.c_str());
					} else {
						if (mod.m_threads.m_htIdW.AsInt() > 0)
							mifPostInstr.Append("\t\t\trdRsm.m_htId = r_rdGrpState[%s].m_rsmHtId;\n", rdRspGrpId.c_str());
					}

					mifPostInstr.Append("\t\t\trdRsm.m_htInstr = r_rdGrpState[%s].m_rsmInstr;\n", rdRspGrpId.c_str());
					mifPostInstr.Append("\t\t\tm_mifRdRsmQue.push(rdRsm);\n");
				}

				mifPostInstr.Append("\t\t}\n");
				mifPostInstr.Append("\t}\n");
				mifPostInstr.NewLine();

			} else {

				mifPostInstr.Append("\tif (%s) {\n", rdRspRdy.c_str());
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tht_assert(r_reset1x || c_m%d_rdGrpRspStateWr.m_cnt != c_m%d_rdGrpReqState.m_cnt);\n", rdRspStg, rdRspStg);
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tc_m%d_rdGrpRspStateWr.m_cnt = c_m%d_rdGrpRspState.m_cnt + 1;\n", rdRspStg, rdRspStg);
				mifPostInstr.Append("\t\tc_rdGrpBusyCnt = c_rdGrpBusyCnt - 1;\n");
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tbool c_m%d_bRdGrpRspCntZero = c_m%d_rdGrpRspStateWr.m_cnt == c_m%d_rdGrpReqState.m_cnt;\n", rdRspStg, rdRspStg, rdRspStg);
				mifPostInstr.NewLine();

				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tbool c_rdGrpCollision = r_t%d_memReq.m_rdPause && !r_t%d_memReq.m_rdReq%s && r_t%d_memReq.m_rdGrpId == %s;\n",
					mod.m_execStg + 1, mod.m_execStg + 1, andNot_memReqHold.c_str(), mod.m_execStg + 1, rdRspGrpId.c_str());
				mifPostInstr.Append("\t\tbool c_rdGrpResume = c_m%d_rdGrpRspState.m_pause != c_m%d_rdGrpReqState.m_pause ||\n", rdRspStg, rdRspStg);
				mifPostInstr.Append("\t\t\tc_rdGrpCollision;\n");
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tif (c_m%d_bRdGrpRspCntZero && c_rdGrpResume) {\n", rdRspStg);
				mifPostInstr.Append("\t\t\t// last read response arrived, resume thread\n");
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\t\tc_m%d_rdGrpRspStateWr.m_pause = c_m%d_rdGrpReqState.m_pause ^ (c_rdGrpCollision ? 1u : 0u);\n", rdRspStg, rdRspStg);
				mifPostInstr.NewLine();

				if (mod.m_threads.m_htIdW.AsInt() == 0) {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = c_m%d_rdGrpReqState.m_rsmInstr;\n", rdRspStg);

				} else {

					bNeedMifRdRsmQue = true;
					mifPostInstr.Append("\t\t\tCHtCmd rdRsm;\n");

					if (bRdRspGrpIdIsHtId) {
						mifPostInstr.Append("\t\t\trdRsm.m_htId = %s;\n", rdRspGrpId.c_str());
					} else {
						if (mod.m_threads.m_htIdW.AsInt() > 0)
							mifPostInstr.Append("\t\t\trdRsm.m_htId = c_m%d_rdGrpReqState.m_rsmHtId;\n", rdRspStg);
					}

					mifPostInstr.Append("\t\t\trdRsm.m_htInstr = c_m%d_rdGrpReqState.m_rsmInstr;\n", rdRspStg);
					mifPostInstr.Append("\t\t\tm_mifRdRsmQue.push(rdRsm);\n");
				}
				mifPostInstr.Append("\t\t}\n");
				mifPostInstr.Append("\t}\n");
				mifPostInstr.NewLine();
			}
		}

		if (!mif.m_mifRd.m_bPause && rdRspGrpIdW > 2) {

			mifPostInstr.Append("\tif (%s) {\n", rdRspRdy.c_str());
			mifPostInstr.NewLine();

			mifPostInstr.Append("\t\tc_m%d_rdGrpRspStateWr.m_cnt = c_m%d_rdGrpRspState.m_cnt + 1;\n", rdRspStg, rdRspStg);
			mifPostInstr.Append("\t\tc_rdGrpBusyCnt = c_rdGrpBusyCnt - 1;\n");
			mifPostInstr.Append("\t}\n");
			mifPostInstr.NewLine();
		}

		if (rdRspGrpIdW > 2) {

			mifPostInstr.Append("\tm_rdGrpRspState0.write_mem(c_m%d_rdGrpRspStateWr);\n", rdRspStg);
			mifPostInstr.Append("\tm_rdGrpRspState1.write_mem(c_m%d_rdGrpRspStateWr);\n", rdRspStg);
			mifPostInstr.NewLine();

			if (MIF_CHK_STATE) {
				GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CRdGrpRspState ht_noload", "x_rdGrpRspState");

				mifPostInstr.Append("\tif (r_m%d_rdRspInfo.m_grpId == %s_CHK_GRP_ID)\n", rdRspStg, mod.m_modName.Upper().c_str());
				mifPostInstr.Append("\t\tx_rdGrpRspState = c_m%d_rdGrpRspStateWr;\n", rdRspStg);
				mifPostInstr.NewLine();
			}
		}
	}

	if (mif.m_bMifRd) {
		int rdRspStg = mod.m_mif.m_mifRd.m_bNeedRdRspInfoRam ? 2 : 1;
		if (rdDstNgvRamList.size() > 0) {
			string gblVarList;
			for (size_t rdDstIdx = 0; rdDstIdx < mif.m_mifRd.m_rdDstList.size(); rdDstIdx += 1) {
				CMifRdDst &rdDst = mif.m_mifRd.m_rdDstList[rdDstIdx];

				if (rdDst.m_pGblVar == 0) continue;

				gblVarList += VA(" && r_m%d_rdRspInfo.m_dst != %d", rdRspStg, (int)rdDstIdx);
			}

			string rdRspRdy;
			if (bMultiQwRdMif) {
				rdRspRdy = VA("r_m%d_rdRspRdy && c_m%d_bWrData", rdRspStg, rdRspStg);
			} else {
				rdRspRdy = VA("r_m%d_rdRspRdy", rdRspStg);
			}

			if (rdRspGrpIdW > 0) {
				if (bRdDstNonNgv) {
					mifPostInstr.Append("\tbool c_rdCompRdy = %s%s;\n",
						rdRspRdy.c_str(), gblVarList.c_str());
					mifPostInstr.Append("\tht_uint%d c_rdCompGrpId = r_m%d_rdRspInfo.m_grpId;\n",
						mif.m_mifRd.m_rspGrpW.AsInt(), rdRspStg);
					mifPostInstr.NewLine();
				} else if (rdDstRdyCnt == 1 && !(mod.m_clkRate == eClk1x && bNgvWrCompClk2x)) {

					string varIdx;
					for (size_t i = 0; i < rdDstNgvRamList[0]->m_dimenList.size(); i += 1)
						varIdx += "[0]";

					string phase;
					if (mod.m_clkRate == eClk2x && !rdDstNgvRamList[0]->m_pNgvInfo->m_bNgvWrCompClk2x)
						phase = " & r_phase";

					mifPostInstr.Append("\tbool c_rdCompRdy = i_%sTo%s_mwCompRdy%s.read()%s;\n",
						rdDstNgvRamList[0]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), varIdx.c_str(), phase.c_str());
					mifPostInstr.Append("\tht_uint%d c_rdCompGrpId = i_%sTo%s_mwCompGrpId%s;\n",
						mif.m_mifRd.m_rspGrpW.AsInt(),
						rdDstNgvRamList[0]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), varIdx.c_str());
					mifPostInstr.NewLine();
				} else {
					mifPostInstr.Append("\tbool c_rdCompRdy = false;\n");
					mifPostInstr.Append("\tht_uint%d c_rdCompGrpId = 0;\n", mif.m_mifRd.m_rspGrpW.AsInt());
					mifPostInstr.NewLine();
				}
			} else {
				if (bRdDstNonNgv) {
					mifPostInstr.Append("\tbool c_rdCompRdy = %s%s;\n", rdRspRdy.c_str(), gblVarList.c_str());

				} else if (rdDstRdyCnt == 1 && !(mod.m_clkRate == eClk1x && bNgvWrCompClk2x)) {

					string varIdx;
					for (size_t i = 0; i < rdDstNgvRamList[0]->m_dimenList.size(); i += 1)
						varIdx += "[0]";

					string phase;
					if (mod.m_clkRate == eClk2x && !rdDstNgvRamList[0]->m_pNgvInfo->m_bNgvWrCompClk2x)
						phase = " & r_phase";

					int wrCompDly = 0;
					if (mod.m_clkRate == eClk2x && !rdDstNgvRamList[0]->m_pNgvInfo->m_bNgvWrCompClk2x && !rdDstNgvRamList[0]->m_pNgvInfo->m_bNgvWrDataClk2x)
						wrCompDly = (rdDstNgvRamList[0]->m_pNgvInfo->m_wrDataStg - rdDstNgvRamList[0]->m_pNgvInfo->m_wrCompStg - 1) * 2;

					if (wrCompDly > 0) {
						for (int stg = 0; stg < wrCompDly; stg += 1) {
							GenModDecl(eVcdAll, m_mifDecl, vcdModName, "bool",
								VA("r_c%d_%s_mwCompRdy%s", stg + 1, rdDstNgvRamList[0]->m_gblName.Lc().c_str(), varIdx.c_str()));

							mifReg.Append("\tr_c%d_%s_mwCompRdy%s = c_c%d_%s_mwCompRdy%s;\n",
								stg + 1, rdDstNgvRamList[0]->m_gblName.Lc().c_str(), varIdx.c_str(),
								stg, rdDstNgvRamList[0]->m_gblName.Lc().c_str(), varIdx.c_str());
						}

						mifPostInstr.Append("\tbool c_c0_%s_mwCompRdy%s = i_%sTo%s_mwCompRdy%s.read()%s;\n",
							rdDstNgvRamList[0]->m_gblName.Lc().c_str(), varIdx.c_str(),
							rdDstNgvRamList[0]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), varIdx.c_str(), phase.c_str());

						for (int stg = 1; stg < wrCompDly; stg += 1) {
							mifPostInstr.Append("\tbool c_c%d_%s_mwCompRdy%s = r_c%d_%s_mwCompRdy%s;\n",
								stg, rdDstNgvRamList[0]->m_gblName.Lc().c_str(), varIdx.c_str(),
								stg, rdDstNgvRamList[0]->m_gblName.Lc().c_str(), varIdx.c_str());
						}

						mifPostInstr.Append("\tbool c_rdCompRdy = r_c%d_%s_mwCompRdy%s;\n",
							wrCompDly, rdDstNgvRamList[0]->m_gblName.Lc().c_str(), varIdx.c_str());
					}
					else {
						mifPostInstr.Append("\tbool c_rdCompRdy = i_%sTo%s_mwCompRdy%s.read()%s;\n",
							rdDstNgvRamList[0]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), varIdx.c_str(), phase.c_str());
					}
				} else {
					mifPostInstr.Append("\tbool c_rdCompRdy = false;\n");
				}

				if (rdDstRdyCnt > 1 || mod.m_clkRate == eClk1x && bNgvWrCompClk2x) {
					for (size_t i = 0; i < rdDstNgvRamList.size(); i += 1) {
						vector<int> refList(rdDstNgvRamList[i]->m_dimenList.size());
						do {
							string dimIdx = IndexStr(refList);

							mifPostInstr.Append("\tc_%sTo%s_mwCompCnt%s = r_%sTo%s_mwCompCnt%s;\n",
								rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());

							if (mod.m_clkRate == eClk1x && rdDstNgvRamList[i]->m_pNgvInfo->m_bNgvWrCompClk2x) {
								m_mifPostInstr2x.Append("\tc_%sTo%s_mwCompCnt_2x%s = r_%sTo%s_mwCompCnt_2x%s;\n",
									rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
									rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
							}

						} while (DimenIter(rdDstNgvRamList[i]->m_dimenList, refList));

						mifPostInstr.NewLine();
						if (mod.m_clkRate == eClk1x && rdDstNgvRamList[i]->m_pNgvInfo->m_bNgvWrCompClk2x)
							m_mifPostInstr2x.NewLine();
					}
				}
			}
		}

		if (rdRspGrpIdW > 2) {

			mifPostInstr.Append("\tsc_uint<%s_RD_GRP_ID_W + 1> c_rdRspGrpInitCnt = r_rdRspGrpInitCnt;\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.Append("\tif (r_rdRspGrpInitCnt < (1 << %s_RD_GRP_ID_W)) {\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.NewLine();

			mifPostInstr.Append("\t\tCRdGrpReqState rdGrpReqState = 0;\n");
			mifPostInstr.Append("\t\tm_rdGrpReqState0.write_mem(rdGrpReqState);\n");
			if (mif.m_mifRd.m_bPause)
				mifPostInstr.Append("\t\tm_rdGrpReqState1.write_mem(rdGrpReqState);\n");
			mifPostInstr.NewLine();

			mifPostInstr.Append("\t\tCRdGrpRspState rdGrpRspState = 0;\n");
			mifPostInstr.Append("\t\tm_rdGrpRspState0.write_mem(rdGrpRspState);\n");
			mifPostInstr.Append("\t\tm_rdGrpRspState1.write_mem(rdGrpRspState);\n");
			mifPostInstr.NewLine();

			mifPostInstr.Append("\t\tc_rdRspGrpInitCnt = r_rdRspGrpInitCnt + 1u;\n");
			mifPostInstr.NewLine();

			mifPostInstr.Append("\t\tc_t%d_rdRspInfo.m_grpId = c_rdRspGrpInitCnt & ((1 << %s_RD_GRP_ID_W) - 1);\n",
				mod.m_execStg, mod.m_modName.Upper().c_str());
			mifReset.Append("\t\tc_t%d_rdRspInfo.m_grpId = 0;\n", mod.m_execStg);

			if (bRdDstNonNgv) {

				mifPostInstr.Append("\t\tc_m%d_rdRspInfo.m_grpId = c_rdRspGrpInitCnt & ((1 << %s_RD_GRP_ID_W) - 1);\n",
					rdRspStg - 1, mod.m_modName.Upper().c_str());

				mifReset.Append("\t\tc_m%d_rdRspInfo.m_grpId = 0;\n", rdRspStg - 1);
			} else {
				mifPostInstr.Append("\t\tc_rdCompGrpId = c_rdRspGrpInitCnt & ((1 << %s_RD_GRP_ID_W) - 1);\n",
					mod.m_modName.Upper().c_str());

				mifReset.Append("\t\tc_rdCompGrpId = 0;\n");
			}

			mifPostInstr.Append("\t}\n");
			mifPostInstr.NewLine();
		}

		if (rdDstRdyCnt > 1 || mod.m_clkRate == eClk1x && bNgvWrCompClk2x) {
			for (size_t i = 0; i < rdDstNgvRamList.size(); i += 1) {
				vector<int> refList(rdDstNgvRamList[i]->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					if (rdRspGrpIdW > 0) {
						mifPostInstr.Append("\tif (!c_rdCompRdy && !m_%sTo%s_mwCompQue%s.empty()) {\n",
							rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					} else {
						if (mod.m_clkRate == eClk1x && rdDstNgvRamList[i]->m_pNgvInfo->m_bNgvWrCompClk2x) {
							mifPostInstr.Append("\tif (!c_rdCompRdy && r_%sTo%s_mwCompCnt%s != r_%sTo%s_mwCompCnt_2x%s) {\n",
								rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
								rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
						}
						else {
							mifPostInstr.Append("\tif (!c_rdCompRdy && r_%sTo%s_mwCompCnt%s > 0) {\n",
								rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
						}
					}
					mifPostInstr.Append("\t\tc_rdCompRdy = true;\n");
					if (rdRspGrpIdW > 0) {
						mifPostInstr.Append("\t\tc_rdCompGrpId = m_%sTo%s_mwCompQue%s.front();\n",
							rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
						mifPostInstr.Append("\t\tm_%sTo%s_mwCompQue%s.pop();\n",
							rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					} else {
						if (mod.m_clkRate == eClk1x && rdDstNgvRamList[i]->m_pNgvInfo->m_bNgvWrCompClk2x) {
							mifPostInstr.Append("\t\tc_%sTo%s_mwCompCnt%s += 1u;\n",
								rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
						}
						else {
							mifPostInstr.Append("\t\tc_%sTo%s_mwCompCnt%s -= 1u;\n",
								rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
						}
					}
					mifPostInstr.Append("\t}\n");

				} while (DimenIter(rdDstNgvRamList[i]->m_dimenList, refList));
			}
			mifPostInstr.NewLine();

			for (size_t i = 0; i < rdDstNgvRamList.size(); i += 1) {
				CHtCode & mifPostInstrMwComp = rdDstNgvRamList[i]->m_pNgvInfo->m_bNgvWrCompClk2x ? m_mifPostInstr2x : m_mifPostInstr1x;
				vector<int> refList(rdDstNgvRamList[i]->m_dimenList.size());
				do {
					string dimIdx = IndexStr(refList);

					mifPostInstrMwComp.Append("\tif (i_%sTo%s_mwCompRdy%s)\n",
						rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					if (rdRspGrpIdW > 0) {
						mifPostInstrMwComp.Append("\t\tm_%sTo%s_mwCompQue%s.push(i_%sTo%s_mwCompGrpId%s);\n",
							rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str(),
							rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
					} else {
						if (mod.m_clkRate == eClk1x && rdDstNgvRamList[i]->m_pNgvInfo->m_bNgvWrCompClk2x) {
							mifPostInstrMwComp.Append("\t\tc_%sTo%s_mwCompCnt_2x%s += 1u;\n",
								rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
						}
						else {
							mifPostInstrMwComp.Append("\t\tc_%sTo%s_mwCompCnt%s += 1u;\n",
								rdDstNgvRamList[i]->m_gblName.Lc().c_str(), mod.m_modName.Uc().c_str(), dimIdx.c_str());
						}
					}
				} while (DimenIter(rdDstNgvRamList[i]->m_dimenList, refList));
			}
			mifPostInstr.NewLine();
		}
	}

	if (mif.m_bMifWr) {

		mifPostInstr.Append("\tbool c_m0_wrRspRdy = i_mifTo%sP0_wrRspRdy.read();\n",
			mod.m_modName.Uc().c_str());

		if (wrRspGrpIdW > 0) {
			mifPostInstr.Append("\tsc_uint<%s_WR_GRP_ID_W> c_m0_wrGrpId;\n", mod.m_modName.Upper().c_str());
			mifPostInstr.Append("\tc_m0_wrGrpId = i_mifTo%sP0_wrRspTid.read() & ((1 << %s_WR_GRP_ID_W) - 1);\n",
				mod.m_modName.Uc().c_str(),
				mod.m_modName.Upper().c_str());
		}
		mifPostInstr.NewLine();

		string wrReqRdy;
		if (bMultiQwReq) {

			string wrGrpInc;
			if (mif.m_bMifWr && bMultiQwMif)
				wrGrpInc = VA("r_t%d_memReq.m_reqQwRem == 0 && ", mod.m_execStg + 1);

			wrReqRdy = VA("r_t%d_memReq.m_wrReq && %s!r_memReqHold",
				mod.m_execStg + 1, wrGrpInc.c_str());
		} else {
			wrReqRdy = VA("r_t%d_memReq.m_wrReq", mod.m_execStg + 1);
		}

		string wrRspRdy = "r_m1_wrRspRdy";
		string wrRspGrpId = "r_m1_wrGrpId";

		if (wrRspGrpIdW == 0) {

			mifPostInstr.Append("\tif ((%s) != %s)\n", wrReqRdy.c_str(), wrRspRdy.c_str());

			mifPostInstr.Append("\t\tc_wrGrpState.m_cnt = r_wrGrpState.m_cnt + (%s ? -1 : 1);\n", wrRspRdy.c_str());

		} else if (wrRspGrpIdW <= 2) {

			mifPostInstr.Append("\tfor (int grpId = 0; grpId < (1 << %s_WR_GRP_ID_W); grpId += 1) {\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.Append("\t\tif ((%s && r_t%d_memReq.m_wrGrpId == grpId) != (%s && %s == grpId))\n",
				wrReqRdy.c_str(),
				mod.m_execStg + 1,
				wrRspRdy.c_str(), wrRspGrpId.c_str());

			mifPostInstr.Append("\t\t\tc_wrGrpState[grpId].m_cnt = r_wrGrpState[grpId].m_cnt + ((%s && %s == grpId) ? -1 : 1);\n", wrRspRdy.c_str(), wrRspGrpId.c_str());

			mifPostInstr.Append("\t}\n");

			mifPostInstr.Append("\tif ((%s) != %s)\n", wrReqRdy.c_str(), wrRspRdy.c_str());

			mifPostInstr.Append("\t\tc_wrGrpBusyCnt = r_wrGrpBusyCnt + (%s ? -1 : 1);\n", wrRspRdy.c_str());

		} else {

			mifPostInstr.Append("\tsc_uint<%s_WR_GRP_ID_W> c_t%d_wrGrpId = r_t%d_memReq.m_wrGrpId;\n",
				mod.m_modName.Upper().c_str(), mod.m_execStg + 1, mod.m_execStg + 1);
			mifPostInstr.Append("\tm_wrGrpReqState0.read_addr(c_t%d_wrGrpId);\n", mod.m_execStg + 1);
			mifPostInstr.Append("\tm_wrGrpRspState1.read_addr(c_t%d_wrGrpId);\n", mod.m_execStg + 1);
			mifPostInstr.Append("\tm_wrGrpReqState0.write_addr(c_t%d_wrGrpId);\n", mod.m_execStg + 1);
			if (mif.m_mifWr.m_bPause)
				mifPostInstr.Append("\tm_wrGrpReqState1.write_addr(c_t%d_wrGrpId);\n", mod.m_execStg + 1);
			mifPostInstr.NewLine();

			mifPostInstr.Append("\tCWrGrpReqState c_t%d_wrGrpReqState = m_wrGrpReqState0.read_mem();\n", mod.m_execStg + 1);
			mifPostInstr.Append("\tCWrGrpReqState c_t%d_wrGrpReqStateWr = c_t%d_wrGrpReqState;\n", mod.m_execStg + 1, mod.m_execStg + 1);
			mifPostInstr.Append("\tCWrGrpRspState c_t%d_wrGrpRspState = m_wrGrpRspState1.read_mem();\n", mod.m_execStg + 1);
			mifPostInstr.NewLine();

			mifPostInstr.Append("\tif (%s) {\n", wrReqRdy.c_str());

			mifPostInstr.Append("\t\tc_wrGrpBusyCnt = r_wrGrpBusyCnt + 1;\n");
			mifPostInstr.Append("\t\tc_t%d_wrGrpReqStateWr.m_cnt = c_t%d_wrGrpReqState.m_cnt + 1;\n",
				mod.m_execStg + 1,
				mod.m_execStg + 1);
			mifPostInstr.Append("\t}\n");
		}
		mifPostInstr.NewLine();

		if (mif.m_mifWr.m_bPoll) {
			mifPostInstr.Append("\tif (r_t%d_memReq.m_wrPoll%s) {\n", mod.m_execStg + 1, andNot_memReqHold.c_str());

			if (wrRspGrpIdW == 0) {
				mifPostInstr.Append("\t\tif (r_wrGrpState.m_cnt == 0 && !r_t%d_memReq.m_wrReq) {\n", mod.m_execStg + 1);
			} else if (wrRspGrpIdW <= 2) {
				mifPostInstr.Append("\t\tif (r_wrGrpState[r_t%d_memReq.m_wrGrpId].m_cnt == 0 && !r_t%d_memReq.m_wrReq) {\n",
					mod.m_execStg + 1, mod.m_execStg + 1);
			} else {
				mifPostInstr.Append("\t\tif (c_t%d_wrGrpReqStateWr.m_cnt == c_t%d_wrGrpRspState.m_cnt && !r_t%d_memReq.m_wrReq) {\n",
					mod.m_execStg + 1, mod.m_execStg + 1, mod.m_execStg + 1);
			}

			if (mod.m_rsmSrcCnt > 1 && mod.m_threads.m_htIdW.AsInt() > 0) {
				bNeedMifContQue = true;
				mifPostInstr.Append("\t\t\tCHtCmd mifCont;\n");
				mifPostInstr.Append("\t\t\tmifCont.m_htId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tmifCont.m_htInstr = r_t%d_memReq.m_rsmInstr;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tm_mifContQue.push(mifCont);\n");
			} else {
				mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
				if (mod.m_threads.m_htIdW.AsInt() > 0)
					mifPostInstr.Append("\t\t\tc_t0_rsmHtId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = r_t%d_memReq.m_rsmInstr;\n", mod.m_execStg + 1);
			}

			mifPostInstr.Append("\t\t} else {\n");

			if (mod.m_threads.m_htIdW.AsInt() > 2) {
				bNeedMifPollQue = true;
				mifPostInstr.Append("\t\t\tCHtCmd mifCont;\n");
				mifPostInstr.Append("\t\t\tmifCont.m_htId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tmifCont.m_htInstr = r_t%d_memReq.m_pollInstr;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tm_mifPollQue.push(mifCont);\n");

			} else if (mod.m_rsmSrcCnt > 1 && mod.m_threads.m_htIdW.AsInt() > 0) {
				bNeedMifContQue = true;
				mifPostInstr.Append("\t\t\tCHtCmd mifCont;\n");
				mifPostInstr.Append("\t\t\tmifCont.m_htId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tmifCont.m_htInstr = r_t%d_memReq.m_pollInstr;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tm_mifContQue.push(mifCont);\n");

			} else {
				mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
				if (mod.m_threads.m_htIdW.AsInt() > 0)
					mifPostInstr.Append("\t\t\tc_t0_rsmHtId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = r_t%d_memReq.m_pollInstr;\n", mod.m_execStg + 1);
			}

			mifPostInstr.Append("\t\t}\n");
			mifPostInstr.Append("\t}\n");
			mifPostInstr.NewLine();
		}

		if (mif.m_mifWr.m_bPause) {
			if (wrRspGrpIdW == 0) {

				mifPostInstr.Append("\tif (r_t%d_memReq.m_wrPause%s) {\n", mod.m_execStg + 1, andNot_memReqHold.c_str());
				mifPostInstr.Append("\t\tif (r_wrGrpState.m_cnt == 0 && !r_t%d_memReq.m_wrReq) {\n", mod.m_execStg + 1);

				if (mod.m_rsmSrcCnt == 1 || mod.m_threads.m_htIdW.AsInt() == 0) {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = r_t%d_memReq.m_rsmInstr;\n",
						mod.m_execStg + 1);
				} else {
					bNeedMifContQue = true;
					mifPostInstr.Append("\t\t\tCHtCmd wrCont;\n");
					mifPostInstr.Append("\t\t\twrCont.m_htId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
					mifPostInstr.Append("\t\t\twrCont.m_htInstr = r_t%d_memReq.m_rsmInstr;\n", mod.m_execStg + 1);
					mifPostInstr.Append("\t\t\tm_mifContQue.push(wrCont);\n");
				}

				mifPostInstr.Append("\t\t} else {\n");
				mifPostInstr.Append("\t\t\tc_wrGrpState.m_pause = true;\n");

				if (!bWrRspGrpIdIsHtId && mod.m_threads.m_htIdW.AsInt() > 0) {
					mifPostInstr.Append("\t\t\tc_wrGrpState.m_rsmHtId = r_t%d_memReq.m_rsmHtId;\n",
						mod.m_execStg + 1);
				}
				mifPostInstr.Append("\t\t\tc_wrGrpState.m_rsmInstr = r_t%d_memReq.m_rsmInstr;\n",
					mod.m_execStg + 1);
				mifPostInstr.Append("\t\t}\n");
				mifPostInstr.Append("\t}\n");
				mifPostInstr.NewLine();

			} else if (wrRspGrpIdW <= 2) {

				mifPostInstr.Append("\tif (r_t%d_memReq.m_wrPause%s) {\n", mod.m_execStg + 1, andNot_memReqHold.c_str());
				mifPostInstr.Append("\t\tif (r_wrGrpState[r_t%d_memReq.m_wrGrpId].m_cnt == 0 && !r_t%d_memReq.m_wrReq) {\n",
					mod.m_execStg + 1, mod.m_execStg + 1);

				if (mod.m_rsmSrcCnt == 1 || mod.m_threads.m_htIdW.AsInt() == 0) {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = r_t%d_memReq.m_rsmInstr;\n",
						mod.m_execStg + 1);
				} else {
					bNeedMifContQue = true;
					mifPostInstr.Append("\t\t\tCHtCmd wrCont;\n");
					mifPostInstr.Append("\t\t\twrCont.m_htId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
					mifPostInstr.Append("\t\t\twrCont.m_htInstr = r_t%d_memReq.m_rsmInstr;\n", mod.m_execStg + 1);
					mifPostInstr.Append("\t\t\tm_mifContQue.push(wrCont);\n");
				}

				mifPostInstr.Append("\t\t} else {\n");
				mifPostInstr.Append("\t\t\tc_wrGrpState[r_t%d_memReq.m_wrGrpId].m_pause = true;\n",
					mod.m_execStg + 1);

				if (!bWrRspGrpIdIsHtId && mod.m_threads.m_htIdW.AsInt() > 0) {
					mifPostInstr.Append("\t\t\tc_wrGrpState[r_t%d_memReq.m_wrGrpId].m_rsmHtId = r_t%d_memReq.m_rsmHtId;\n",
						mod.m_execStg + 1, mod.m_execStg + 1);
				}
				mifPostInstr.Append("\t\t\tc_wrGrpState[r_t%d_memReq.m_wrGrpId].m_rsmInstr = r_t%d_memReq.m_rsmInstr;\n",
					mod.m_execStg + 1, mod.m_execStg + 1);
				mifPostInstr.Append("\t\t}\n");
				mifPostInstr.Append("\t}\n");
				mifPostInstr.NewLine();

			} else {

				mifPostInstr.Append("\tif (r_t%d_memReq.m_wrPause%s) {\n", mod.m_execStg + 1, andNot_memReqHold.c_str());
				mifPostInstr.Append("\t\tif (c_t%d_wrGrpReqStateWr.m_cnt == c_t%d_wrGrpRspState.m_cnt && !r_t%d_memReq.m_wrReq) {\n",
					mod.m_execStg + 1, mod.m_execStg + 1, mod.m_execStg + 1);

				if (mod.m_rsmSrcCnt == 1 || mod.m_threads.m_htIdW.AsInt() == 0) {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = r_t%d_memReq.m_rsmInstr;\n",
						mod.m_execStg + 1);
				} else {
					bNeedMifContQue = true;
					mifPostInstr.Append("\t\t\tCHtCmd wrCont;\n");
					mifPostInstr.Append("\t\t\twrCont.m_htId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
					mifPostInstr.Append("\t\t\twrCont.m_htInstr = r_t%d_memReq.m_rsmInstr;\n", mod.m_execStg + 1);
					mifPostInstr.Append("\t\t\tm_mifContQue.push(wrCont);\n");
				}

				mifPostInstr.Append("\t\t} else {\n");
				mifPostInstr.Append("\t\t\tc_t%d_wrGrpReqStateWr.m_pause = c_t%d_wrGrpRspState.m_pause ^ 1;\n",
					mod.m_execStg + 1, mod.m_execStg + 1);

				if (!bWrRspGrpIdIsHtId && mod.m_threads.m_htIdW.AsInt() > 0) {
					mifPostInstr.Append("\t\t\tc_t%d_wrGrpReqStateWr.m_rsmHtId = r_t%d_memReq.m_rsmHtId;\n",
						mod.m_execStg + 1, mod.m_execStg + 1);
				}
				mifPostInstr.Append("\t\t\tc_t%d_wrGrpReqStateWr.m_rsmInstr = r_t%d_memReq.m_rsmInstr;\n",
					mod.m_execStg + 1, mod.m_execStg + 1);
				mifPostInstr.Append("\t\t}\n");
				mifPostInstr.Append("\t}\n");
				mifPostInstr.NewLine();
			}
		}

		if (mif.m_mifWr.m_bReqPause) {
			string andNot_memReqHold;
			if (bMultiQwReq)
				andNot_memReqHold = VA(" && !c_t%d_memReqHold", mod.m_execStg + 1);
			mifPostInstr.Append("\tif (r_t%d_memReq.m_reqPause%s) {\n", mod.m_execStg + 1, andNot_memReqHold.c_str());

			if (mod.m_rsmSrcCnt == 1 || mod.m_threads.m_htIdW.AsInt() == 0) {
				mifPostInstr.Append("\t\tc_t0_rsmHtRdy = true;\n");
				mifPostInstr.Append("\t\tc_t0_rsmHtInstr = r_t%d_memReq.m_rsmInstr;\n",
					mod.m_execStg + 1);
			} else {
				bNeedMifContQue = true;
				mifPostInstr.Append("\t\tCHtCmd reqCont;\n");
				mifPostInstr.Append("\t\treqCont.m_htId = r_t%d_memReq.m_rsmHtId;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\treqCont.m_htInstr = r_t%d_memReq.m_rsmInstr;\n", mod.m_execStg + 1);
				mifPostInstr.Append("\t\tm_mifContQue.push(reqCont);\n");
			}

			mifPostInstr.Append("\t}\n");
			mifPostInstr.NewLine();
		}

		if (wrRspGrpIdW > 2) {
			mifPostInstr.Append("\tm_wrGrpReqState0.write_mem(c_t%d_wrGrpReqStateWr);\n",
				mod.m_execStg + 1);
			if (mif.m_mifWr.m_bPause) {
				mifPostInstr.Append("\tm_wrGrpReqState1.write_mem(c_t%d_wrGrpReqStateWr);\n",
					mod.m_execStg + 1);
			}
			mifPostInstr.NewLine();

			if (MIF_CHK_STATE) {
				GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CWrGrpReqState ht_noload", "x_wrGrpReqState");

				mifPostInstr.Append("\tif (r_t%d_memReq.m_wrGrpId == %s_CHK_GRP_ID)\n", mod.m_execStg + 1, mod.m_modName.Upper().c_str());
				mifPostInstr.Append("\t\tx_wrGrpReqState = c_t%d_wrGrpReqStateWr;\n", mod.m_execStg + 1);
				mifPostInstr.NewLine();
			}

			if (mif.m_mifWr.m_bPause)
				mifPostInstr.Append("\tm_wrGrpReqState1.read_addr(r_m1_wrGrpId);\n");
			mifPostInstr.Append("\tm_wrGrpRspState0.read_addr(r_m1_wrGrpId);\n");
			mifPostInstr.Append("\tm_wrGrpRspState0.write_addr(r_m1_wrGrpId);\n");
			mifPostInstr.Append("\tm_wrGrpRspState1.write_addr(r_m1_wrGrpId);\n");
			mifPostInstr.NewLine();

			mifPostInstr.Append("\tCWrGrpRspState c_m1_wrGrpRspState = m_wrGrpRspState0.read_mem();\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.Append("\tCWrGrpRspState c_m1_wrGrpRspStateWr = c_m1_wrGrpRspState;\n",
				mod.m_modName.Upper().c_str());
			if (mif.m_mifWr.m_bPause) {
				mifPostInstr.Append("\tCWrGrpReqState c_m1_wrGrpReqState = m_wrGrpReqState1.read_mem();\n",
					mod.m_modName.Upper().c_str());
			}
			mifPostInstr.NewLine();
		}

		if (mif.m_mifWr.m_bPause) {
			if (wrRspGrpIdW == 0) {

				mifPostInstr.Append("\tif (r_m1_wrRspRdy) {\n");
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tht_assert(%s || r_wrGrpState.m_cnt != 0);\n",
					reset.c_str());
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tbool c_wrGrpCollision = r_t%d_memReq.m_wrPause && !r_t%d_memReq.m_wrReq%s;\n",
					mod.m_execStg + 1, mod.m_execStg + 1, andNot_memReqHold.c_str());
				mifPostInstr.Append("\t\tbool c_wrGrpResume = r_wrGrpState.m_pause || c_wrGrpCollision;\n");
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tif (c_wrGrpState.m_cnt == 0 && c_wrGrpResume) {\n");
				mifPostInstr.Append("\t\t\t// last write response arrived, resume thread\n");
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\t\tc_wrGrpState.m_pause = false;\n");
				mifPostInstr.NewLine();

				if (mod.m_threads.m_htIdW.AsInt() == 0) {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = r_wrGrpState.m_rsmInstr;\n");

				} else {
					bNeedMifWrRsmQue = true;

					mifPostInstr.Append("\t\t\tCHtCmd wrRsm;\n");

					if (bWrRspGrpIdIsHtId) {
						mifPostInstr.Append("\t\t\twrRsm.m_htId = r_m1_wrGrpId;\n");
					} else {
						if (mod.m_threads.m_htIdW.AsInt() > 0)
							mifPostInstr.Append("\t\t\twrRsm.m_htId = r_wrGrpState.m_rsmHtId;\n");
					}

					mifPostInstr.Append("\t\t\twrRsm.m_htInstr = r_wrGrpState.m_rsmInstr;\n");
					mifPostInstr.Append("\t\t\tm_mifWrRsmQue.push(wrRsm);\n");
				}

				mifPostInstr.Append("\t\t}\n");
				mifPostInstr.Append("\t}\n");
				mifPostInstr.NewLine();

			} else if (wrRspGrpIdW <= 2) {

				mifPostInstr.Append("\tif (r_m1_wrRspRdy) {\n");
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tht_assert(%s || r_wrGrpState[r_m1_wrGrpId].m_cnt != 0);\n",
					reset.c_str());
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tbool c_wrGrpCollision = r_t%d_memReq.m_wrPause && !r_t%d_memReq.m_wrReq%s && r_t%d_memReq.m_wrGrpId == r_m1_wrGrpId;\n",
					mod.m_execStg + 1, mod.m_execStg + 1, andNot_memReqHold.c_str(), mod.m_execStg + 1);
				mifPostInstr.Append("\t\tbool c_wrGrpResume = r_wrGrpState[r_m1_wrGrpId].m_pause || c_wrGrpCollision;\n");
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tif (c_wrGrpState[r_m1_wrGrpId].m_cnt == 0 && c_wrGrpResume) {\n");
				mifPostInstr.Append("\t\t\t// last read response arrived, resume thread\n");
				mifPostInstr.NewLine();

				if (mif.m_mifWr.m_bPause)
					mifPostInstr.Append("\t\t\tc_wrGrpState[r_m1_wrGrpId].m_pause = false;\n");
				mifPostInstr.NewLine();

				if (mod.m_threads.m_htIdW.AsInt() == 0) {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = r_wrGrpState[r_m1_wrGrpId].m_rsmInstr;\n");

				} else {
					bNeedMifWrRsmQue = true;

					mifPostInstr.Append("\t\t\tCHtCmd wrRsm;\n");

					if (bWrRspGrpIdIsHtId) {
						mifPostInstr.Append("\t\t\twrRsm.m_htId = r_m1_wrGrpId;\n");
					} else {
						if (mod.m_threads.m_htIdW.AsInt() > 0)
							mifPostInstr.Append("\t\t\twrRsm.m_htId = r_wrGrpState[r_m1_wrGrpId].m_rsmHtId;\n");
					}

					mifPostInstr.Append("\t\t\twrRsm.m_htInstr = r_wrGrpState[r_m1_wrGrpId].m_rsmInstr;\n");
					mifPostInstr.Append("\t\t\tm_mifWrRsmQue.push(wrRsm);\n");
				}

				mifPostInstr.Append("\t\t}\n");
				mifPostInstr.Append("\t}\n");
				mifPostInstr.NewLine();

			} else {

				mifPostInstr.Append("\tif (r_m1_wrRspRdy) {\n");
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tht_assert(%s || c_m1_wrGrpRspStateWr.m_cnt != c_m1_wrGrpReqState.m_cnt);\n",
					reset.c_str());
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tc_m1_wrGrpRspStateWr.m_cnt = c_m1_wrGrpRspState.m_cnt + 1;\n");
				mifPostInstr.Append("\t\tc_wrGrpBusyCnt = c_wrGrpBusyCnt - 1;\n");
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tbool c_m1_bWrGrpRspCntZero = c_m1_wrGrpRspStateWr.m_cnt == c_m1_wrGrpReqState.m_cnt;\n");
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tbool c_wrGrpCollision = r_t%d_memReq.m_wrPause && !r_t%d_memReq.m_wrReq%s && r_t%d_memReq.m_wrGrpId == r_m1_wrGrpId;\n",
					mod.m_execStg + 1, mod.m_execStg + 1, andNot_memReqHold.c_str(), mod.m_execStg + 1);
				mifPostInstr.Append("\t\tbool c_wrGrpResume = c_m1_wrGrpRspState.m_pause != c_m1_wrGrpReqState.m_pause ||\n");
				mifPostInstr.Append("\t\t\tc_wrGrpCollision;\n");
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tif (c_m1_bWrGrpRspCntZero && c_wrGrpResume) {\n");
				mifPostInstr.Append("\t\t\t// last write response arrived, resume thread\n");
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\t\tc_m1_wrGrpRspStateWr.m_pause = c_m1_wrGrpReqState.m_pause ^ (c_wrGrpCollision ? 1u : 0u);\n");
				mifPostInstr.NewLine();

				bNeedMifWrRsmQue = true;
				mifPostInstr.Append("\t\t\tCHtCmd wrRsm;\n");

				if (bWrRspGrpIdIsHtId) {
					if (mod.m_threads.m_htIdW.AsInt() > 0)
						mifPostInstr.Append("\t\t\twrRsm.m_htId = %s;\n", wrRspGrpId.c_str());
				} else {
					mifPostInstr.Append("\t\t\twrRsm.m_htId = c_m1_wrGrpReqState.m_rsmHtId;\n",
						mod.m_execStg);
				}

				mifPostInstr.Append("\t\t\twrRsm.m_htInstr = c_m1_wrGrpReqState.m_rsmInstr;\n",
					mod.m_execStg);

				mifPostInstr.Append("\t\t\tm_mifWrRsmQue.push(wrRsm);\n");

				mifPostInstr.Append("\t\t}\n");
				mifPostInstr.Append("\t}\n");
				mifPostInstr.NewLine();
			}
		}

		if (!mif.m_mifWr.m_bPause && wrRspGrpIdW > 2) {

			mifPostInstr.Append("\tif (r_m1_wrRspRdy) {\n");
			mifPostInstr.NewLine();

			mifPostInstr.Append("\t\tc_m1_wrGrpRspStateWr.m_cnt = c_m1_wrGrpRspState.m_cnt + 1;\n");
			mifPostInstr.Append("\t\tc_wrGrpBusyCnt = c_wrGrpBusyCnt - 1;\n");

			mifPostInstr.Append("\t}\n");
			mifPostInstr.NewLine();
		}

		if (wrRspGrpIdW > 2) {

			mifPostInstr.Append("\tm_wrGrpRspState0.write_mem(c_m1_wrGrpRspStateWr);\n");
			mifPostInstr.Append("\tm_wrGrpRspState1.write_mem(c_m1_wrGrpRspStateWr);\n");
			mifPostInstr.NewLine();

			if (MIF_CHK_STATE) {
				GenModDecl(eVcdAll, m_mifDecl, vcdModName, "CWrGrpRspState ht_noload", "x_wrGrpRspState");

				mifPostInstr.Append("\tif (r_m1_wrGrpId == %s_CHK_GRP_ID)\n", mod.m_modName.Upper().c_str());
				mifPostInstr.Append("\t\tx_wrGrpRspState = c_m1_wrGrpRspStateWr;\n");
				mifPostInstr.NewLine();
			}

			mifPostInstr.Append("\tsc_uint<%s_WR_GRP_ID_W + 1> c_wrRspGrpInitCnt = r_wrRspGrpInitCnt;\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.Append("\tif (r_wrRspGrpInitCnt < (1 << %s_WR_GRP_ID_W)) {\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.NewLine();

			mifPostInstr.Append("\t\tCWrGrpReqState wrGrpReqState = 0;\n");
			mifPostInstr.Append("\t\tm_wrGrpReqState0.write_mem(wrGrpReqState);\n");
			if (mif.m_mifWr.m_bPause)
				mifPostInstr.Append("\t\tm_wrGrpReqState1.write_mem(wrGrpReqState);\n");
			mifPostInstr.NewLine();

			mifPostInstr.Append("\t\tCWrGrpRspState wrGrpRspState = 0;\n");
			mifPostInstr.Append("\t\tm_wrGrpRspState0.write_mem(wrGrpRspState);\n");
			mifPostInstr.Append("\t\tm_wrGrpRspState1.write_mem(wrGrpRspState);\n");
			mifPostInstr.NewLine();

			mifPostInstr.Append("\t\tc_wrRspGrpInitCnt = r_wrRspGrpInitCnt + 1u;\n");
			mifPostInstr.NewLine();

			mifPostInstr.Append("\t\tc_t%d_memReq.m_wrGrpId = c_wrRspGrpInitCnt & ((1 << %s_WR_GRP_ID_W) - 1);\n",
				mod.m_execStg, mod.m_modName.Upper().c_str());
			mifReset.Append("\t\tc_t%d_memReq.m_wrGrpId = 0;\n", mod.m_execStg);

			mifPostInstr.Append("\t\tc_m0_wrGrpId = c_wrRspGrpInitCnt & ((1 << %s_WR_GRP_ID_W) - 1);\n",
				mod.m_modName.Upper().c_str());
			mifReset.Append("\t\tc_m0_wrGrpId = 0;\n");

			mifPostInstr.Append("\t}\n");
			mifPostInstr.NewLine();
		}
	}

	if (bNeedMifPollQue) {
		m_mifDecl.Append("\tht_dist_que<CHtCmd, %s_HTID_W> m_mifPollQue;\n", mod.m_modName.Upper().c_str());

		mifReg.Append("\tm_mifPollQue.clock(%s);\n", reset.c_str());
	}

	if (bNeedMifContQue) {
		m_mifDecl.Append("\tht_dist_que<CHtCmd, %s_HTID_W> m_mifContQue;\n", mod.m_modName.Upper().c_str());

		mifReg.Append("\tm_mifContQue.clock(%s);\n", reset.c_str());

		mifPostInstr.Append("\tif (!c_t0_rsmHtRdy && !m_mifContQue.empty()) {\n");
		mifPostInstr.Append("\t\tc_t0_rsmHtRdy = true;\n");
		mifPostInstr.Append("\t\tc_t0_rsmHtId = m_mifContQue.front().m_htId;\n");
		mifPostInstr.Append("\t\tc_t0_rsmHtInstr = m_mifContQue.front().m_htInstr;\n");
		mifPostInstr.Append("\t\tm_mifContQue.pop();\n");
		mifPostInstr.Append("\t}\n");
		mifPostInstr.NewLine();
	}

	if (bNeedMifRdRsmQue) {
		m_mifDecl.Append("\tht_dist_que<CHtCmd, %s_HTID_W> m_mifRdRsmQue;\n", mod.m_modName.Upper().c_str());

		mifReg.Append("\tm_mifRdRsmQue.clock(%s);\n", reset.c_str());

		mifPostInstr.Append("\tif (!c_t0_rsmHtRdy && !m_mifRdRsmQue.empty()) {\n");
		mifPostInstr.Append("\t\tc_t0_rsmHtRdy = true;\n");
		mifPostInstr.Append("\t\tc_t0_rsmHtId = m_mifRdRsmQue.front().m_htId;\n");
		mifPostInstr.Append("\t\tc_t0_rsmHtInstr = m_mifRdRsmQue.front().m_htInstr;\n");
		mifPostInstr.Append("\t\tm_mifRdRsmQue.pop();\n");
		mifPostInstr.Append("\t}\n");
		mifPostInstr.NewLine();
	}

	if (bNeedMifWrRsmQue) {
		m_mifDecl.Append("\tht_dist_que<CHtCmd, %s_HTID_W> m_mifWrRsmQue;\n", mod.m_modName.Upper().c_str());

		mifReg.Append("\tm_mifWrRsmQue.clock(%s);\n", reset.c_str());

		mifPostInstr.Append("\tif (!c_t0_rsmHtRdy && !m_mifWrRsmQue.empty()) {\n");
		mifPostInstr.Append("\t\tc_t0_rsmHtRdy = true;\n");
		mifPostInstr.Append("\t\tc_t0_rsmHtId = m_mifWrRsmQue.front().m_htId;\n");
		mifPostInstr.Append("\t\tc_t0_rsmHtInstr = m_mifWrRsmQue.front().m_htInstr;\n");
		mifPostInstr.Append("\t\tm_mifWrRsmQue.pop();\n");
		mifPostInstr.Append("\t}\n");
		mifPostInstr.NewLine();
	}

	mifReg.NewLine();

	// outputs
	mifOut.Append("\to_%sP0ToMif_reqRdy = r_t%d_%sToMif_reqRdy;\n",
		mod.m_modName.Lc().c_str(), mod.m_execStg + 1 + mif.m_mifReqStgCnt, mod.m_modName.Lc().c_str());
	mifOut.Append("\to_%sP0ToMif_req = r_t%d_%sToMif_req;\n",
		mod.m_modName.Lc().c_str(), mod.m_execStg + 1 + mif.m_mifReqStgCnt, mod.m_modName.Lc().c_str());
	if (mif.m_bMifRd) {
		mifOut.Append("\to_%sP0ToMif_rdRspFull = r_%sToMif_rdRspFull;\n",
			mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());
	}
}
