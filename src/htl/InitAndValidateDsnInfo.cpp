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
#include <iostream>
#include <fstream>

void CDsnInfo::InitializeAndValidate()
{
	InitAndValidateTypes();

	InitNativeCTypes();

	// Cxr sets module used flag
	InitAndValidateModCxr();

	// Cxr sets mod_INSTR_W
	ValidateDesignInfo();

	// Initialize addr1W from addr1 and addr2W from addr2
	InitAddrWFromAddrName();

	InitPrivateAsGlobal();
	InitAndValidateModMif();

	InitOptNgv();	// determine if NGV ram has optimized implementation
	InitBramUsage();
	InitMifRamType();

	InitAndValidateModNgv();
	InitAndValidateModIpl();
	InitAndValidateModIhm();
	InitAndValidateModOhm();
	InitAndValidateModIhd();
	InitAndValidateModMsg();
	InitAndValidateModBar();
	InitAndValidateModStrm();
	InitAndValidateModStBuf();

	InitAndValidateHif();	// ModCxr must be called first

	if (CLex::GetParseErrorCnt() > 0)
		ParseMsg(Fatal, "Previous errors prevent generation of files");
}

void CDsnInfo::ValidateDesignInfo()
{
	// walk design and init all value fields
	//   At this point we don't know which modules are need, validate them all
	for (size_t typedefIdx = 0; typedefIdx < m_typedefList.size(); typedefIdx += 1) {
		CTypeDef &typeDef = m_typedefList[typedefIdx];

		typeDef.m_width.InitValue(typeDef.m_lineInfo, false, 0);
	}

	for (size_t recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
		CRecord * pRecord = m_recordList[recordIdx];

		InitFieldDimenValues(pRecord->m_fieldList);
	}

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		mod.m_stage.m_privWrStg.InitValue(mod.m_stage.m_lineInfo, false, 1);
		mod.m_stage.m_execStg.InitValue(mod.m_stage.m_lineInfo, false, 1);

		//if (mod.m_stage.m_privWrStg.AsInt() > 1 || mod.m_stage.m_execStg.AsInt() > 1)
		//	mod.m_stage.m_bStageNums = true;

		for (size_t callIdx = 0; callIdx < mod.m_cxrCallList.size(); callIdx += 1) {
			CCxrCall * pCxrCall = mod.m_cxrCallList[callIdx];

			pCxrCall->m_queueW.InitValue(pCxrCall->m_lineInfo);
		}

		if (mod.m_bHasThreads) {
			mod.m_threads.m_htIdW.InitValue(mod.m_threads.m_lineInfo);

			for (size_t prIdx = 0; prIdx < mod.m_threads.m_htPriv.m_fieldList.size(); prIdx += 1) {
				CField * pPriv = mod.m_threads.m_htPriv.m_fieldList[prIdx];

				pPriv->m_addr1W.InitValue(pPriv->m_lineInfo, false, 0);
				pPriv->m_addr2W.InitValue(pPriv->m_lineInfo, false, 0);
				pPriv->m_fieldWidth.InitValue(pPriv->m_lineInfo, false, 0);
				pPriv->InitDimen(pPriv->m_lineInfo);
			}
		}

		for (size_t shIdx = 0; shIdx < mod.m_shared.m_fieldList.size(); shIdx += 1) {
			CField * pShared = mod.m_shared.m_fieldList[shIdx];

			pShared->m_queueW.InitValue(pShared->m_lineInfo, false, 0);
			pShared->m_rdSelW.InitValue(pShared->m_lineInfo, false, 0);
			pShared->m_wrSelW.InitValue(pShared->m_lineInfo, false, 0);
			pShared->m_addr1W.InitValue(pShared->m_lineInfo, false, 0);
			pShared->m_addr2W.InitValue(pShared->m_lineInfo, false, 0);

			pShared->InitDimen(pShared->m_lineInfo);
		}

		for (size_t stgIdx = 0; stgIdx < mod.m_stage.m_fieldList.size(); stgIdx += 1) {
			CField * pField = mod.m_stage.m_fieldList[stgIdx];

			pField->m_rngLow.InitValue(pField->m_lineInfo);
			pField->m_rngHigh.InitValue(pField->m_lineInfo);
			pField->m_fieldWidth.InitValue(pField->m_lineInfo, false, 0);

			pField->InitDimen(pField->m_lineInfo);
		}

		for (size_t gvIdx = 0; gvIdx < mod.m_ngvList.size(); gvIdx += 1) {
			CRam * pNgv = mod.m_ngvList[gvIdx];

			pNgv->m_addr0W.InitValue(pNgv->m_lineInfo, false, 0);
			pNgv->m_addr1W.InitValue(pNgv->m_lineInfo, false, 0);
			pNgv->m_addr2W.InitValue(pNgv->m_lineInfo, false, 0);
			pNgv->m_rdStg.InitValue(pNgv->m_lineInfo, false, 1);
			pNgv->m_wrStg.InitValue(pNgv->m_lineInfo, false, 1);

			pNgv->InitDimen(pNgv->m_lineInfo);

			pNgv->m_addrW = pNgv->m_addr1W.AsInt() + pNgv->m_addr2W.AsInt();

			for (size_t fldIdx = 0; fldIdx < pNgv->m_fieldList.size(); fldIdx += 1) {
				CField * pField = pNgv->m_fieldList[fldIdx];

				pField->InitDimen(pField->m_lineInfo);
			}
		}

		CCoprocInfo const & coprocInfo = g_appArgs.GetCoprocInfo();

		// MIF values
		if (mod.m_mif.m_bMifWr) {
			CMifWr & mifWr = mod.m_mif.m_mifWr;
			mifWr.m_queueW.InitValue(mifWr.m_lineInfo);

			int defValue = FindLg2(512 / coprocInfo.GetMaxCoprocQwWriteCnt(), true, true);

			//int defValue = max(6, 9 - mod.m_threads.m_htIdW.AsInt());
			mifWr.m_rspCntW.InitValue(mifWr.m_lineInfo, false, defValue);

			if (mifWr.m_rspCntW.AsInt() < 2 || mifWr.m_rspCntW.AsInt() > 9)
				ParseMsg(Error, mifWr.m_lineInfo, "rspCntW out of range, value %d, allowed range 1-9",
				mifWr.m_rspCntW.AsInt());
		}

		if (mod.m_mif.m_bMifRd) {
			CMifRd & mifRd = mod.m_mif.m_mifRd;
			mifRd.m_queueW.InitValue(mifRd.m_lineInfo);

			int defValue = FindLg2(512 / coprocInfo.GetMaxCoprocQwReadCnt(), true, true);

			//int defValue = max(6, 9 - mod.m_threads.m_htIdW.AsInt());
			mifRd.m_rspCntW.InitValue(mifRd.m_lineInfo, false, defValue);

			if (mifRd.m_rspCntW.AsInt() < 2 || mifRd.m_rspCntW.AsInt() > 9)
				ParseMsg(Error, mifRd.m_lineInfo, "rspCntW out of range, value %d, allowed range 1-9",
				mifRd.m_rspCntW.AsInt());

			for (size_t rdDstIdx = 0; rdDstIdx < mifRd.m_rdDstList.size(); rdDstIdx += 1) {
				CMifRdDst &mifRdDst = mifRd.m_rdDstList[rdDstIdx];

				mifRdDst.m_infoW.InitValue(mifRdDst.m_lineInfo, mifRdDst.m_infoW.size() > 0, 0);
				mifRdDst.m_rsmDly.InitValue(mifRdDst.m_lineInfo, mifRdDst.m_rsmDly.size() > 0, 0);
			}
		}

		for (size_t ihmIdx = 0; ihmIdx < mod.m_hostMsgList.size(); ihmIdx += 1) {
			CHostMsg &hostMsg = mod.m_hostMsgList[ihmIdx];

			if (hostMsg.m_msgDir == Outbound) continue;

			for (size_t dstIdx = 0; dstIdx < hostMsg.m_msgDstList.size(); dstIdx += 1) {
				CMsgDst & msgDst = hostMsg.m_msgDstList[dstIdx];

				msgDst.m_dataLsb.InitValue(msgDst.m_lineInfo, false, 0);
				msgDst.m_addr1Lsb.InitValue(msgDst.m_lineInfo, false, 0);
				msgDst.m_addr2Lsb.InitValue(msgDst.m_lineInfo, false, 0);
				msgDst.m_idx1Lsb.InitValue(msgDst.m_lineInfo, false, 0);
				msgDst.m_idx2Lsb.InitValue(msgDst.m_lineInfo, false, 0);
			}
		}

	}
}

void CDsnInfo::InitAddrWFromAddrName()
{
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		// check new global variables for specification of an addrName but not an addrW
		for (size_t ramIdx = 0; ramIdx < mod.m_ngvList.size(); ramIdx += 1) {
			CRam * pNgv = mod.m_ngvList[ramIdx];

			if (pNgv->m_addr1Name.size() > 0) {
				int addr1W;
				pNgv->m_addr1IsHtId = true;
				pNgv->m_addr1IsPrivate = true;
				pNgv->m_addr1IsStage = true;
				bool bShared = false;
				CField const * pRtnField;

				if (FindVariableWidth(pNgv->m_lineInfo, mod, pNgv->m_addr1Name, pNgv->m_addr1IsHtId,
					pNgv->m_addr1IsPrivate, bShared, pNgv->m_addr1IsStage, addr1W, pRtnField))
				{
					if (pNgv->m_addr1W.size() == 0) {
						pNgv->m_addr1W = CHtString(VA("%d", addr1W));
						pNgv->m_addr1W.SetValue(addr1W);
						pNgv->m_addrW = pNgv->m_addr0W.AsInt() + pNgv->m_addr1W.AsInt() + pNgv->m_addr2W.AsInt();
					} else if (pNgv->m_addr1W.AsInt() != addr1W)
						ParseMsg(Error, pNgv->m_lineInfo, "addr1Name (%d) and addr1W (%d) have inconsistent widths",
						addr1W, pNgv->m_addr1W.AsInt());

					if (pNgv->m_addr1IsStage && (pNgv->m_rdStg.AsInt() - 2 < pRtnField->m_rngLow.AsInt() || pNgv->m_rdStg.AsInt() - 2 > pRtnField->m_rngHigh.AsInt()))
						ParseMsg(Error, pNgv->m_lineInfo, "Global variable rdStg (%d) requires staging variable to be valid two stages earlier", pNgv->m_rdStg.AsInt());
				} else
					ParseMsg(Error, pNgv->m_lineInfo, "%s was not found as a private or stage variable", pNgv->m_addr1Name.c_str());
			}

			if (pNgv->m_addr2Name.size() > 0) {
				int addr2W;
				pNgv->m_addr2IsHtId = true;
				pNgv->m_addr2IsPrivate = true;
				pNgv->m_addr2IsStage = true;
				bool bShared = false;
				CField const * pRtnField;

				if (FindVariableWidth(pNgv->m_lineInfo, mod, pNgv->m_addr2Name, pNgv->m_addr2IsHtId,
					pNgv->m_addr2IsPrivate, bShared, pNgv->m_addr2IsStage, addr2W, pRtnField))
				{
					if (pNgv->m_addr2W.size() == 0) {
						pNgv->m_addr2W = CHtString(VA("%d", addr2W));
						pNgv->m_addr2W.SetValue(addr2W);
						pNgv->m_addrW = pNgv->m_addr0W.AsInt() + pNgv->m_addr1W.AsInt() + pNgv->m_addr2W.AsInt();
					} else if (pNgv->m_addr2W.AsInt() != addr2W) {
						ParseMsg(Error, pNgv->m_lineInfo, "addr2Name (%d) and addr2W (%d) have inconsistent widths",
							addr2W, pNgv->m_addr2W.AsInt());
					}

					if (pNgv->m_addr2IsStage && (pNgv->m_rdStg.AsInt() - 2 < pRtnField->m_rngLow.AsInt() || pNgv->m_rdStg.AsInt() - 2 > pRtnField->m_rngHigh.AsInt()))
						ParseMsg(Error, pNgv->m_lineInfo, "Global variable rdStg (%d) requires staging variable to be valid two stages earlier", pNgv->m_rdStg.AsInt());
				} else
					ParseMsg(Error, pNgv->m_lineInfo, "%s was not found as a private or stage variable", pNgv->m_addr2Name.c_str());
			}
		}

		// check private variables for specification of an addrName but not an addrW
		for (size_t prIdx = 0; prIdx < mod.m_threads.m_htPriv.m_fieldList.size(); prIdx += 1) {
			CField * pPriv = mod.m_threads.m_htPriv.m_fieldList[prIdx];

			if (pPriv->m_addr1W.size() == 0 && pPriv->m_addr1Name.size() > 0) {
				int addr1W;
				pPriv->m_addr1IsHtId = true;
				pPriv->m_addr1IsPrivate = true;
				pPriv->m_addr1IsStage = true;
				bool bShared = false;
				CField const * pRtnField;

				if (FindVariableWidth(pPriv->m_lineInfo, mod, pPriv->m_addr1Name, pPriv->m_addr1IsHtId,
					pPriv->m_addr1IsPrivate, bShared, pPriv->m_addr1IsStage, addr1W, pRtnField))
				{
					pPriv->m_addr1W = CHtString(VA("%d", addr1W));
					pPriv->m_addr1W.SetValue(addr1W);
				} else
					ParseMsg(Error, pPriv->m_lineInfo, "%s was not found as a private or stage variable", pPriv->m_addr1Name.c_str());
			}

			if (pPriv->m_addr2W.size() == 0 && pPriv->m_addr2Name.size() > 0) {
				int addr2W;
				pPriv->m_addr2IsHtId = true;
				pPriv->m_addr2IsPrivate = true;
				pPriv->m_addr2IsStage = true;
				bool bShared = false;
				CField const * pRtnField;

				if (FindVariableWidth(pPriv->m_lineInfo, mod, pPriv->m_addr2Name, pPriv->m_addr2IsHtId,
					pPriv->m_addr2IsPrivate, bShared, pPriv->m_addr2IsStage, addr2W, pRtnField))
				{
					pPriv->m_addr2W = CHtString(VA("%d", addr2W));
					pPriv->m_addr2W.SetValue(addr2W);
				} else
					ParseMsg(Error, pPriv->m_lineInfo, "%s was not found as a private or stage variable", pPriv->m_addr2Name.c_str());
			}
		}
	}
}

void CDsnInfo::InitPrivateAsGlobal()
{
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		if (mod.m_ngvList.size() > 0 && mod.m_instSet.GetInstCnt() > 1)
			ParseMsg(Fatal, mod.m_ngvList[0]->m_lineInfo, "Module with multiple instances and global variable is not supported");

		// Move private variables with depth to global
		for (size_t prIdx = 0; prIdx < mod.m_threads.m_htPriv.m_fieldList.size(); prIdx += 1) {
			CField * pPriv = mod.m_threads.m_htPriv.m_fieldList[prIdx];

			if (pPriv->m_addr1W.AsInt() == 0) continue;

			// create a global variable
			string rdStg = "1";
			string wrStg = mod.m_stage.m_execStg.AsStr();
			string nullStr;

			bool bRead = true;
			bool bWrite = true;
			bool bMaxIw = true;
			bool bMaxMw = true;

			string ramName = mod.m_modName.AsStr() + "__" + pPriv->m_name;

			mod.m_ngvList.push_back(new CRam(pPriv->m_pType->m_typeName, ramName, pPriv->m_dimenList,
				pPriv->m_addr1Name, pPriv->m_addr2Name,
				pPriv->m_addr1W.AsStr(), pPriv->m_addr2W.AsStr(), rdStg, wrStg, bMaxIw,
				bMaxMw, pPriv->m_ramType, bRead, bWrite));

			CRam * pNgv = mod.m_ngvList.back();

			pNgv->m_addr1IsHtId = pPriv->m_addr1IsHtId;
			pNgv->m_addr1IsPrivate = pPriv->m_addr1IsPrivate;
			pNgv->m_addr1IsStage = pPriv->m_addr1IsStage;
			pNgv->m_addr2IsHtId = pPriv->m_addr2IsHtId;
			pNgv->m_addr2IsPrivate = pPriv->m_addr2IsPrivate;
			pNgv->m_addr2IsStage = pPriv->m_addr2IsStage;

			pNgv->m_bPrivGbl = true;
			pNgv->m_privName = pPriv->m_name;
			pNgv->m_pType = pPriv->m_pType;
			pNgv->m_addr0W.InitValue(pNgv->m_lineInfo, false, mod.m_threads.m_htIdW.AsInt());
			pNgv->m_addr1W.InitValue(pNgv->m_lineInfo, false, 0);
			pNgv->m_addr2W.InitValue(pNgv->m_lineInfo, false, 0);
			pNgv->m_rdStg.InitValue(pNgv->m_lineInfo, false, 1);
			pNgv->m_wrStg.InitValue(pNgv->m_lineInfo, false, 1);
			pNgv->InitDimen(pPriv->m_lineInfo);
			pNgv->m_addrW = pNgv->m_addr0W.AsInt() + pNgv->m_addr1W.AsInt() + pNgv->m_addr2W.AsInt();
			pNgv->m_wrStg.SetValue(mod.m_stage.m_privWrStg.AsInt());
			pNgv->m_ramType = eAutoRam;

			// now delete from private list
			mod.m_threads.m_htPriv.m_fieldList.erase(mod.m_threads.m_htPriv.m_fieldList.begin() + prIdx);
			prIdx -= 1;
		}

		// now check mif rdDst and wrSrc list
		for (size_t dstIdx = 0; dstIdx < mod.m_mif.m_mifRd.m_rdDstList.size(); dstIdx += 1) {
			CMifRdDst & rdDst = mod.m_mif.m_mifRd.m_rdDstList[dstIdx];

			char const * pStr = rdDst.m_var.c_str();
			while (isalnum(*pStr) || *pStr == '_') pStr += 1;
			string rdDstVar = string(rdDst.m_var.c_str(), pStr - rdDst.m_var.c_str());

			for (size_t prIdx = 0; prIdx < mod.m_threads.m_htPriv.m_fieldList.size(); prIdx += 1) {
				CField * pPriv = mod.m_threads.m_htPriv.m_fieldList[prIdx];

				if (rdDstVar == pPriv->m_name) {

					if (pPriv->m_bCxrParam)
						ParseMsg(Error, pPriv->m_lineInfo, "Call parameter that is also memory read destination is not supported");

					// create a global variable
					string rdStg = "1";
					string wrStg = mod.m_stage.m_execStg.AsStr();
					string nullStr;

					bool bRead = true;
					bool bWrite = true;
					bool bMaxIw = true;
					bool bMaxMw = true;

					string ramName = mod.m_modName.AsStr() + "__" + pPriv->m_name;

					mod.m_ngvList.push_back(new CRam(pPriv->m_pType->m_typeName, ramName, pPriv->m_dimenList,
						pPriv->m_addr1Name, pPriv->m_addr2Name,
						pPriv->m_addr1W.AsStr(), pPriv->m_addr2W.AsStr(), rdStg, wrStg, bMaxIw,
						bMaxMw, pPriv->m_ramType, bRead, bWrite));

					CRam * pNgv = mod.m_ngvList.back();

					pNgv->m_addr1IsHtId = pPriv->m_addr1IsHtId;
					pNgv->m_addr1IsPrivate = pPriv->m_addr1IsPrivate;
					pNgv->m_addr1IsStage = pPriv->m_addr1IsStage;
					pNgv->m_addr2IsHtId = pPriv->m_addr2IsHtId;
					pNgv->m_addr2IsPrivate = pPriv->m_addr2IsPrivate;
					pNgv->m_addr2IsStage = pPriv->m_addr2IsStage;

					pNgv->m_bPrivGbl = true;
					pNgv->m_privName = pPriv->m_name;
					pNgv->m_pType = pPriv->m_pType;
					pNgv->m_addr0W.InitValue(pNgv->m_lineInfo, false, mod.m_threads.m_htIdW.AsInt());
					pNgv->m_addr1W.InitValue(pNgv->m_lineInfo, false, 0);
					pNgv->m_addr2W.InitValue(pNgv->m_lineInfo, false, 0);
					pNgv->m_rdStg.InitValue(pNgv->m_lineInfo, false, 1);
					pNgv->m_wrStg.InitValue(pNgv->m_lineInfo, false, 1);
					pNgv->InitDimen(pPriv->m_lineInfo);
					pNgv->m_addrW = pNgv->m_addr0W.AsInt() + pNgv->m_addr1W.AsInt() + pNgv->m_addr2W.AsInt();
					pNgv->m_wrStg.SetValue(mod.m_stage.m_privWrStg.AsInt());
					pNgv->m_ramType = eAutoRam;

					// now delete from private list
					mod.m_threads.m_htPriv.m_fieldList.erase(mod.m_threads.m_htPriv.m_fieldList.begin() + prIdx);
					prIdx -= 1;
				}
			}
		}

		for (size_t wrSrcIdx = 0; wrSrcIdx < mod.m_mif.m_mifWr.m_wrSrcList.size(); wrSrcIdx += 1) {
			CMifWrSrc & wrSrc = mod.m_mif.m_mifWr.m_wrSrcList[wrSrcIdx];

			char const * pStr = wrSrc.m_var.c_str();
			while (isalnum(*pStr) || *pStr == '_') pStr += 1;
			string wrSrcVar = string(wrSrc.m_var.c_str(), pStr - wrSrc.m_var.c_str());

			for (size_t prIdx = 0; prIdx < mod.m_threads.m_htPriv.m_fieldList.size(); prIdx += 1) {
				CField * pPriv = mod.m_threads.m_htPriv.m_fieldList[prIdx];

				if (wrSrcVar == pPriv->m_name) {

					if (pPriv->m_addr1W.AsInt() == 0) continue;

					// create a global variable
					string rdStg = "1";
					string wrStg = mod.m_stage.m_execStg.AsStr();
					string nullStr;

					bool bRead = true;
					bool bWrite = true;
					bool bMaxIw = true;
					bool bMaxMw = true;

					string ramName = mod.m_modName.AsStr() + "__" + pPriv->m_name;

					mod.m_ngvList.push_back(new CRam(pPriv->m_pType->m_typeName, ramName, pPriv->m_dimenList,
						pPriv->m_addr1Name, pPriv->m_addr2Name,
						pPriv->m_addr1W.AsStr(), pPriv->m_addr2W.AsStr(), rdStg, wrStg, bMaxIw,
						bMaxMw, pPriv->m_ramType, bRead, bWrite));

					CRam * pNgv = mod.m_ngvList.back();

					pNgv->m_bPrivGbl = true;
					pNgv->m_privName = pPriv->m_name;
					pNgv->m_pType = pPriv->m_pType;
					pNgv->m_addr0W.InitValue(pNgv->m_lineInfo, false, mod.m_threads.m_htIdW.AsInt());
					pNgv->m_addr1W.InitValue(pNgv->m_lineInfo, false, 0);
					pNgv->m_addr2W.InitValue(pNgv->m_lineInfo, false, 0);
					pNgv->m_rdStg.InitValue(pNgv->m_lineInfo, false, 1);
					pNgv->m_wrStg.InitValue(pNgv->m_lineInfo, false, 1);
					pNgv->InitDimen(pPriv->m_lineInfo);
					pNgv->m_addrW = pNgv->m_addr0W.AsInt() + pNgv->m_addr1W.AsInt() + pNgv->m_addr2W.AsInt();
					pNgv->m_wrStg.SetValue(mod.m_stage.m_privWrStg.AsInt());
					pNgv->m_ramType = eAutoRam;

					// now delete from private list
					mod.m_threads.m_htPriv.m_fieldList.erase(mod.m_threads.m_htPriv.m_fieldList.begin() + prIdx);
					prIdx -= 1;
				}
			}
		}
	}
}

void CDsnInfo::InitFieldDimenValues(vector<CField *> &fieldList)
{
	for (size_t fldIdx = 0; fldIdx < fieldList.size(); fldIdx += 1) {
		CField * pField = fieldList[fldIdx];

		if (pField->m_pType->IsRecord()) {
			CRecord * pRecord = pField->m_pType->AsRecord();
			InitFieldDimenValues(pRecord->m_fieldList);
		} else {
			pField->InitDimen(pField->m_lineInfo);
		}
	}
}

bool BramTargetListCmp(const CBramTarget & elem1, const CBramTarget & elem2)
{
	return elem1.m_slicePerBramRatio > elem2.m_slicePerBramRatio;
}

void CDsnInfo::InitBramUsage()
{
	// determine which dist rams should be implemented as block rams.
	// On entry:
	//   Required design modules have been marked
	//   Module instance lists have been initialized

	// Create list of potential bram targets
	//  Targets include:
	//    Entire set of private variables per group
	//    Individual fields of global rams

	for (size_t ngvIdx = 0; ngvIdx < m_ngvList.size(); ngvIdx += 1) {
		CNgvInfo * pNgvInfo = m_ngvList[ngvIdx];
		CRam * pRam = pNgvInfo->m_modInfoList[0].m_pNgv;

		if (pRam->m_pNgvInfo->m_bOgv) continue;

		CBramTarget target;
		target.m_name = pRam->m_gblName;
		target.m_pRamType = &pNgvInfo->m_ramType;
		target.m_depth = 1 << pRam->m_addrW;
		target.m_width = pRam->m_pType->m_clangBitWidth;
		target.m_copies = pNgvInfo->m_ngvReplCnt;
		target.m_copies *= pRam->m_elemCnt;
		target.m_slices = FindSliceCnt(target.m_depth, target.m_width);
		target.m_brams = FindBramCnt(target.m_depth, target.m_width);
		target.m_slicePerBramRatio = (float)target.m_slices / (float)target.m_brams;
		target.m_varType = pRam->m_bPrivGbl ? "Private" : "Global";
		target.m_modName = "";

		m_bramTargetList.push_back(target);
	}

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		// add group private to target list
		if (mod.m_bHasThreads && mod.m_threads.m_htPriv.m_fieldList.size() > 0) {

			CBramTarget target;
			target.m_name = "";
			target.m_pRamType = &mod.m_threads.m_ramType;
			target.m_depth = 1 << mod.m_threads.m_htIdW.AsInt();
			target.m_width = 0;
			for (size_t i = 0; i < mod.m_threads.m_htPriv.m_fieldList.size(); i += 1) {
				target.m_width += mod.m_threads.m_htPriv.m_fieldList[i]->m_pType->GetPackedBitWidth() * mod.m_threads.m_htPriv.m_fieldList[i]->m_elemCnt;
			}
			target.m_copies = mod.m_instSet.GetTotalCnt();
			target.m_slices = FindSliceCnt(target.m_depth, target.m_width);
			target.m_brams = FindBramCnt(target.m_depth, target.m_width);
			target.m_slicePerBramRatio = (float)target.m_slices / (float)target.m_brams;
			target.m_varType = "Private";
			target.m_modName = mod.m_modName.AsStr();

			m_bramTargetList.push_back(target);
		}

		for (size_t ngvIdx = 0; ngvIdx < mod.m_ngvList.size(); ngvIdx += 1) {
			CRam * pRam = mod.m_ngvList[ngvIdx];

			CBramTarget target;
			if (pRam->m_pNgvInfo->m_bOgv && pRam->m_addrW > 0) {
				target.m_name = pRam->m_gblName;
				target.m_pRamType = &pRam->m_ramType;
				target.m_depth = 1 << pRam->m_addrW;
				target.m_width = pRam->m_pType->GetPackedBitWidth();
				target.m_copies = mod.m_instSet.GetTotalCnt();
				target.m_copies *= pRam->m_elemCnt;
				target.m_copies *= (pRam->m_bReadForInstrRead ? 1 : 0) + (pRam->m_bReadForMifWrite ? 1 : 0);

				target.m_slices = 0;
				target.m_brams = 0;
				int fldWidths = 0;
				for (size_t idx1 = 0; idx1 < pRam->m_pNgvInfo->m_spanningFieldList.size(); idx1 += 1) {
					CSpanningField & fld1 = pRam->m_pNgvInfo->m_spanningFieldList[idx1];
					if (!fld1.m_bSpanning || fld1.m_bDupRange) continue;
					target.m_slices += FindSliceCnt(target.m_depth, fld1.m_width);
					target.m_brams += FindBramCnt(target.m_depth, fld1.m_width);
					fldWidths += fld1.m_width;
				}
				HtlAssert(fldWidths == target.m_width);

				target.m_slicePerBramRatio = (float)target.m_slices / (float)target.m_brams;
				target.m_varType = pRam->m_bPrivGbl ? "Private" : "Global";
				target.m_modName = mod.m_modName.AsStr();
			} else {
				target.m_name = pRam->m_gblName;
				target.m_pRamType = &pRam->m_ramType;
				target.m_depth = 1 << pRam->m_addrW;
				target.m_width = pRam->m_pType->GetPackedBitWidth();
				target.m_copies = mod.m_instSet.GetTotalCnt();
				target.m_copies *= pRam->m_elemCnt;
				target.m_copies *= (pRam->m_bReadForInstrRead ? 1 : 0) + (pRam->m_bReadForMifWrite ? 1 : 0);
				target.m_slices = FindSliceCnt(target.m_depth, target.m_width);
				target.m_brams = FindBramCnt(target.m_depth, target.m_width);
				target.m_slicePerBramRatio = (float)target.m_slices / (float)target.m_brams;
				target.m_varType = pRam->m_bPrivGbl ? "Private" : "Global";
				target.m_modName = mod.m_modName.AsStr();
			}

			m_bramTargetList.push_back(target);
		}

		for (size_t shIdx = 0; shIdx < mod.m_shared.m_fieldList.size(); shIdx += 1) {
			CField * pShared = mod.m_shared.m_fieldList[shIdx];

			CBramTarget target;
			target.m_name = pShared->m_name;
			target.m_pRamType = &pShared->m_ramType;

			if (pShared->m_addr1W.AsInt() == 0)
				target.m_depth = 0;
			if (pShared->m_addr1W.AsInt() > 0)
				target.m_depth = 1 << (pShared->m_addr1W.AsInt() + pShared->m_addr2W.AsInt());
			else
				target.m_depth = 1 << pShared->m_queueW.AsInt();

			target.m_width = pShared->m_pType->GetPackedBitWidth();
			target.m_width *= pShared->m_elemCnt;
			target.m_copies = mod.m_instSet.GetTotalCnt();
			target.m_copies *= pShared->m_elemCnt;
			target.m_slices = FindSliceCnt(target.m_depth, target.m_width);
			target.m_brams = FindBramCnt(target.m_depth, target.m_width);
			target.m_slicePerBramRatio = (float)target.m_slices / (float)target.m_brams;
			target.m_varType = "Shared";
			target.m_modName = mod.m_modName.AsStr();

			m_bramTargetList.push_back(target);
		}
	}

	// sort vector based on slicePerBramRatio
	sort(m_bramTargetList.begin(), m_bramTargetList.end(), BramTargetListCmp);

	// Now decide which rams to implement with block rams
	int brams18KbAvailCnt = g_appArgs.GetMax18KbBramPerUnit();

	// first check if depth is too great for distributed rams
	for (size_t targetIdx = 0; targetIdx < m_bramTargetList.size(); targetIdx += 1) {
		CBramTarget & target = m_bramTargetList[targetIdx];

		if (target.m_depth == 1)
			*target.m_pRamType = eRegRam;

		if (target.m_varType == "Shared")
			continue;

		if (target.m_depth > 1024) {
			*target.m_pRamType = eBlockRam;
			brams18KbAvailCnt -= target.m_brams * target.m_copies;
		}
	}

	// now use slicePerBramRatio for converting to brams
	for (size_t targetIdx = 0; targetIdx < m_bramTargetList.size(); targetIdx += 1) {
		CBramTarget & target = m_bramTargetList[targetIdx];

		if (*target.m_pRamType != eAutoRam) continue;
		if (target.m_varType == "Shared") continue;

		if (target.m_slicePerBramRatio > g_appArgs.GetMinSliceToBramRatio() && target.m_brams * target.m_copies <= brams18KbAvailCnt) {
			*target.m_pRamType = eBlockRam;
			brams18KbAvailCnt -= target.m_brams * target.m_copies;
		} else
			*target.m_pRamType = eDistRam;
	}
}

float CDsnInfo::FindSlicePerBramRatio(int depth, int width)
{
	float ratio;
	if (depth <= 64)
		// 3 bits per slice
		ratio = width / 3.0f;
	else
		// 1 bit per slice
		ratio = (float)((depth + 127) / 128 * width);

	return ratio;
}

int CDsnInfo::FindSliceCnt(int depth, int width)
{
	int slices;
	if (depth <= 32)
		slices = (width + 5) / 6;
	else
		slices = (width + 2) / 3 * ((depth + 63) / 64);

	return slices;
}

int CDsnInfo::FindBramCnt(int depth, int width)
{
	int brams;
	if (width <= 1)
		brams = (depth + 16 * 1024 - 1) / (16 * 1024);
	else if (width <= 2)
		brams = (depth + 8 * 1024 - 1) / (8 * 1024);
	else if (width <= 4)
		brams = (depth + 4 * 1024 - 1) / (4 * 1024);
	else if (width <= 9)
		brams = (depth + 2 * 1024 - 1) / (2 * 1024);
	else if (width <= 18)
		brams = (depth + 1024 - 1) / 1024;
	else
		brams = (depth + 512 - 1) / 512 * ((width + 35) / 36);

	return brams;
}

bool CDsnInfo::FindStructName(string const &structType, string &structName)
{
	// first check if typeName was defined as a typedef
	for (size_t i = 0; i < m_typedefList.size(); i += 1) {
		CTypeDef & typeDef = m_typedefList[i];

		if (typeDef.m_name == structType)
			return FindStructName(typeDef.m_type, structName);
	}

	// not found as typedef, check user structs
	for (size_t i = 0; i < m_recordList.size(); i += 1) {
		CRecord * pRecord = m_recordList[i];

		if (pRecord->m_typeName != structType) continue;

		// found it, now determine with of struct/union
		structName = structType;
		return true;
	}

	return false;
}

struct CCIntTypes {
	const char * m_pName;
	int m_width;
	bool m_bSigned;
} cIntTypes[] = {
		{ "char", 8, true },
		{ "short", 16, true },
		{ "int", 32, true },
		{ "bool", 1, false },
		{ "uint8_t", 8, false },
		{ "uint16_t", 16, false },
		{ "uint32_t", 32, false },
		{ "uint64_t", 64, false },
		{ "int8_t", 8, true },
		{ "int16_t", 16, true },
		{ "int32_t", 32, true },
		{ "int64_t", 64, true },
		{ 0, 0, false }
};

bool CDsnInfo::FindCIntType(string name, int & width, bool & bSigned)
{
	for (int i = 0; cIntTypes[i].m_pName; i += 1) {
		if (name == cIntTypes[i].m_pName) {
			width = cIntTypes[i].m_width;
			bSigned = cIntTypes[i].m_bSigned;
			return true;
		}
	}
	return false;
}

int CDsnInfo::FindHostTypeWidth(CField const * pField)
{
	return FindHostTypeWidth(pField->m_name, pField->m_hostType, pField->m_fieldWidth, pField->m_lineInfo);
}

int CDsnInfo::FindHostTypeWidth(string const &varName, string const &typeName, CHtString const &bitWidth, CLineInfo const &lineInfo)
{
	// first remove trailing astricks
	bool bPointer = false;
	int i;
	for (i = (int)typeName.size() - 1; i >= 0; i -= 1) {
		if (typeName[i] == '*')
			bPointer = true;
		else if (typeName[i] != ' ')
			break;
	}

	string baseName = typeName.substr(0, i + 1);

	// first check if typeName was defined as a typedef
	for (size_t i = 0; i < m_typedefList.size(); i += 1) {
		CTypeDef & typeDef = m_typedefList[i];

		if (typeDef.m_name == baseName) {
			HtlAssert(bitWidth.size() == 0);
			int width = FindHostTypeWidth(varName, typeDef.m_type, typeDef.m_width, lineInfo);
			return width == 0 ? 0 : (bPointer ? 64 : width);
		}
	}

	// not found as typedef, check user structs
	for (size_t i = 0; i < m_recordList.size(); i += 1) {
		CRecord * pRecord = m_recordList[i];

		if (pRecord->m_typeName != baseName) continue;

		// found it, now determine width of struct/union

		ValidateHostType(pRecord, lineInfo);

		HtlAssert(bitWidth.size() == 0);
		return bPointer ? 64 : pRecord->GetClangBitWidth();
	}

	// not found as typedef, must be a basic type
	for (size_t i = 0; i < m_nativeCTypeList.size(); i += 1) {
		CBasicType & basicType = m_nativeCTypeList[i];

		if (baseName == basicType.m_name) {
			if (bitWidth.size() > 0) {
				if (bitWidth.AsInt() > basicType.m_width)
					ParseMsg(Error, lineInfo, "%s bit field width (%d) too large for base type %s", varName.c_str(), bitWidth.AsInt(), typeName.c_str());
				return bitWidth.AsInt();
			}
			return bPointer ? 64 : basicType.m_width;
		}
	}

	if (baseName != "void" || !bPointer) {
		ParseMsg(Error, lineInfo, "type (%s) not supported for host call/return parameter", typeName.c_str());
		return 0;
	} else
		return 64;
}

bool CDsnInfo::FindFieldInStruct(CLineInfo const &lineInfo, string const &type,
	string const &fldName, bool bIndexCheck, bool &bCStyle, CField const * &pBaseField, CField const * &pLastField)
{
	size_t recordIdx;
	for (recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
		CRecord * pRecord = m_recordList[recordIdx];

		if (pRecord->m_typeName == type)
			break;
	}
	if (recordIdx == m_recordList.size()) {
		ParseMsg(Error, lineInfo, "did not find type in design struct/unions, '%s'", type.c_str());
		return false;
	}
	CRecord * pRecord = m_recordList[recordIdx];

	bCStyle = pRecord->m_bCStyle;

	return IsInFieldList(lineInfo, fldName, pRecord->m_fieldList, pRecord->m_bCStyle, bIndexCheck, pBaseField, pLastField, 0);
}

bool CDsnInfo::IsInFieldList(CLineInfo const &lineInfo, string const &fldName, vector<CField *> const &fieldList,
	bool bCStyle, bool bIndexCheck, CField const * &pBaseField, CField const * &pLastField, string * pFullName)
{
	string identRef = fldName.substr(0, fldName.find_first_of('.'));
	string postName;
	if (identRef.size() < fldName.size())
		postName = fldName.substr(identRef.size() + 1);

	char const *pStr = identRef.c_str();
	int spaceCnt = 0;
	while (isspace(*pStr)) pStr += 1, spaceCnt += 1;

	int identCnt = 0;
	while (isalpha(*pStr) || isdigit(*pStr) || *pStr == '_') pStr += 1, identCnt += 1;

	// ident name without leading white space or indexing
	string name = identRef.substr(spaceCnt, identCnt);

	for (size_t fldIdx = 0; fldIdx < fieldList.size(); fldIdx += 1) {
		CField const * pField = fieldList[fldIdx];

		if (pField->m_pType->IsRecord()) {	// CStyle anonamous struct/union
			size_t entryLen = pFullName ? pFullName->size() : 0;

			CRecord * pRecord = pField->m_pType->AsRecord();
			if (IsInFieldList(lineInfo, postName, pRecord->m_fieldList, pRecord->m_bCStyle, true, pBaseField, pLastField, pFullName)) {
				pBaseField = pField;
				return true;
			}

			if (pFullName)
				pFullName->erase(entryLen);

		} else if (pField->m_name == "") {	// AddStruct/AddUnion anonamous struct/union
			bool bCStyle;
			if (FindFieldInStruct(lineInfo, pField->m_pType->m_typeName, fldName, true, bCStyle, pBaseField, pLastField)) {
				pBaseField = pField;
				return true;
			}

		} else {
			if (pField->m_name != name)
				continue;

			if (pFullName) {
				if (pFullName->size() > 0)
					*pFullName += ".";
				if (!bCStyle)
					*pFullName += "m_";
				*pFullName += name;
			}

			// now handle indexing if present
			while (isspace(*pStr)) pStr += 1;

			size_t indexCnt = 0;
			while (*pStr == '[') {
				pStr += 1;
				indexCnt += 1;
				if (bIndexCheck && pField->m_dimenList.size() < indexCnt)
					ParseMsg(Error, lineInfo, "unexpected variable/field index, %s", identRef.c_str());

				bool bSigned;
				int indexValue;
				m_defineTable.FindStringValue(lineInfo, pStr, indexValue, bSigned);

				if (*(pStr - 1) != ']') {
					ParseMsg(Error, lineInfo, "variable/field index format error, %s", identRef.c_str());
					return false;
				}

				if (bIndexCheck && indexCnt <= pField->m_dimenList.size() && indexValue >= pField->m_dimenList[indexCnt - 1].AsInt())
					ParseMsg(Error, lineInfo, "variable/field index value out of range, %s", identRef.c_str());

				while (isspace(*pStr)) pStr += 1;
			}

			if (bIndexCheck && indexCnt != pField->m_dimenList.size())
				ParseMsg(Error, lineInfo, "variable/field has missing index, %s", identRef.c_str());

			if (postName.size() == 0) {
				pBaseField = pField;
				pLastField = pField;
				return true;
			}

			size_t recordIdx;
			for (recordIdx = 0; recordIdx < m_recordList.size(); recordIdx += 1) {
				CRecord * pRecord = m_recordList[recordIdx];

				if (pRecord->m_typeName == pField->m_pType->m_typeName)
					break;
			}

			if (recordIdx == m_recordList.size())
				return false;

			if (IsInFieldList(lineInfo, postName, m_recordList[recordIdx]->m_fieldList, m_recordList[recordIdx]->m_bCStyle, true,
				pBaseField, pLastField, pFullName)) {
				pBaseField = pField;
				return true;
			}
		}
	}

	return false;
}

void CDsnInfo::ValidateHostType(CType * pType, CLineInfo const & lineInfo)
{
	if (pType->IsRecord()) {
		CRecord * pRecord = pType->AsRecord();
		for (size_t i = 0; i < pRecord->m_fieldList.size(); i += 1) {
			ValidateHostType(pRecord->m_fieldList[i]->m_pType, lineInfo);
		}
	} else {
		CHtInt * pHtInt = pType->AsInt();
		if (pHtInt->m_clangMinAlign == 1) {
			ParseMsg(Error, lineInfo, "unsupported hostType, %s", pHtInt->m_typeName.c_str());
		}
	}
}

void CDsnInfo::InitNativeCTypes()
{
	m_nativeCTypeList.push_back(CBasicType("bool", 1, false));
	m_nativeCTypeList.push_back(CBasicType("char", 8, true));
	m_nativeCTypeList.push_back(CBasicType("signed char", 8, true));
	m_nativeCTypeList.push_back(CBasicType("unsigned char", 8, false));
	m_nativeCTypeList.push_back(CBasicType("short", 16, true));
	m_nativeCTypeList.push_back(CBasicType("signed short", 16, true));
	m_nativeCTypeList.push_back(CBasicType("unsigned short", 16, false));
	m_nativeCTypeList.push_back(CBasicType("int", 32, true));
	m_nativeCTypeList.push_back(CBasicType("signed int", 32, true));
	m_nativeCTypeList.push_back(CBasicType("unsigned int", 32, false));
	m_nativeCTypeList.push_back(CBasicType("long", sizeof(long) * 8, true));
	m_nativeCTypeList.push_back(CBasicType("signed long", sizeof(long) * 8, true));
	m_nativeCTypeList.push_back(CBasicType("unsigned long", sizeof(long) * 8, false));
	m_nativeCTypeList.push_back(CBasicType("long long", 64, true));
	m_nativeCTypeList.push_back(CBasicType("signed long long", 64, true));
	m_nativeCTypeList.push_back(CBasicType("unsigned long long", 64, false));

	m_nativeCTypeList.push_back(CBasicType("uint8_t", 8, false));
	m_nativeCTypeList.push_back(CBasicType("uint16_t", 16, false));
	m_nativeCTypeList.push_back(CBasicType("uint32_t", 32, false));
	m_nativeCTypeList.push_back(CBasicType("uint64_t", 64, false));

	m_nativeCTypeList.push_back(CBasicType("int8_t", 8, true));
	m_nativeCTypeList.push_back(CBasicType("int16_t", 16, true));
	m_nativeCTypeList.push_back(CBasicType("int32_t", 32, true));
	m_nativeCTypeList.push_back(CBasicType("int64_t", 64, true));
}

// Scan all declared variables, validate and initialize used types
void CDsnInfo::InitAndValidateTypes()
{
	// scan declared struct/unions
	for (size_t idx = 0; idx < m_recordList.size(); idx += 1) {
		CRecord * pRecord = m_recordList[idx];

		InitAndValidateRecord(pRecord);
	}

	// Validate types in modules
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		// new global variables
		for (size_t mgvIdx = 0; mgvIdx < mod.m_ngvList.size(); mgvIdx += 1) {
			CRam * pNgv = mod.m_ngvList[mgvIdx];

			pNgv->m_pType = FindType(pNgv->m_type, -1, pNgv->m_lineInfo);
		}

		// shared variables
		for (size_t shIdx = 0; shIdx < mod.m_shared.m_fieldList.size(); shIdx += 1) {
			CField * pShared = mod.m_shared.m_fieldList[shIdx];

			pShared->m_pType = FindType(pShared->m_pType->m_typeName, -1, pShared->m_lineInfo);
		}

		// private variables
		for (size_t prIdx = 0; prIdx < mod.m_threads.m_htPriv.m_fieldList.size(); prIdx += 1) {
			CField * pPriv = mod.m_threads.m_htPriv.m_fieldList[prIdx];

			pPriv->m_pType = FindType(pPriv->m_pType->m_typeName, -1, pPriv->m_lineInfo);
		}

		// stage variables
		for (size_t stgIdx = 0; stgIdx < mod.m_stage.m_fieldList.size(); stgIdx += 1) {
			CField * pStage = mod.m_stage.m_fieldList[stgIdx];

			pStage->m_pType = FindType(pStage->m_pType->m_typeName, -1, pStage->m_lineInfo);
		}
	}
}

CType * CDsnInfo::FindType(string const & typeName, int fldWidth, CLineInfo const & lineInfo)
{
	CType * pType = 0;

	if (pType = FindClangType(typeName)) {
		if (fldWidth > 0) {
			if (fldWidth > pType->m_clangBitWidth) {
				ParseMsg(Error, lineInfo, "specified field width is greater than type width");
			} else {
				CHtInt * pIntType = pType->AsInt();
				pType = new CHtInt(pIntType->m_eSign, pIntType->m_typeName, pIntType->m_clangBitWidth, pIntType->m_clangMinAlign, fldWidth);
			}			
		}

		return pType;
	}

	HtlAssert(fldWidth <= 0);

	if (!(pType = FindRecord(typeName)) && !(pType = FindHtIntType(typeName, lineInfo)))
		ParseMsg(Error, lineInfo, "undeclared type, '%s'", typeName.c_str());

	return pType;
}

void CDsnInfo::InitAndValidateRecord(CRecord * pRecord)
{
	// check if already initialized
	if (pRecord->m_clangBitWidth >= 0)
		return;

	int clangBitPos = 0;
	int clangBitWidth = 0;
	int clangMinAlign = 8;
	int prevTypeWidth = 0;

	for (size_t fldIdx = 0; fldIdx < pRecord->m_fieldList.size(); fldIdx += 1) {
		CField * pField = pRecord->m_fieldList[fldIdx];

		CType * pType;
		int fieldWidth;
		//int packedFieldWidth;

		if (pField->m_pType->IsRecord()) {
			// unnamed struct/union
			InitAndValidateRecord(pField->m_pType->AsRecord());
			pType = pField->m_pType;
		} else {
			if (!FindType(pField->m_pType->m_typeName, -1, pField->m_lineInfo)) {
				HtlAssert(0);
				continue;
			}

			pType = pField->m_pType;
		}

		pField->InitDimen(pField->m_lineInfo);

		if (pField->m_fieldWidth.size() > 0) {
			// field bit width specified
			pField->m_fieldWidth.InitValue(pField->m_lineInfo);

			fieldWidth = pField->m_fieldWidth.AsInt();
			//packedFieldWidth = pField->m_fieldWidth.AsInt();

			if (prevTypeWidth != pType->m_clangBitWidth || clangBitPos + fieldWidth > clangBitWidth) {

				if (!pRecord->m_bUnion) {
					if (clangBitWidth % pType->m_clangMinAlign > 0)
						clangBitPos = clangBitWidth = clangBitWidth + pType->m_clangMinAlign - clangBitWidth % pType->m_clangMinAlign;
					else
						clangBitPos = clangBitWidth;
				}

				if (pRecord->m_bUnion)
					clangBitWidth = max(clangBitWidth, pType->m_clangBitWidth * (int)pField->m_elemCnt);
				else
					clangBitWidth += pType->m_clangBitWidth * pField->m_elemCnt;
			}

			prevTypeWidth = pType->m_clangBitWidth;

		} else {
			fieldWidth = pType->m_clangBitWidth;
			//packedFieldWidth = pType->GetPackedBitWidth();
			prevTypeWidth = 0;

			if (!pRecord->m_bUnion) {
				if (clangBitWidth % pType->m_clangMinAlign > 0)
					clangBitPos = clangBitWidth = clangBitWidth + pType->m_clangMinAlign - clangBitWidth % pType->m_clangMinAlign;
				else
					clangBitPos = clangBitWidth;
			}

			if (pRecord->m_bUnion)
				clangBitWidth = max(clangBitWidth, pType->m_clangBitWidth * (int)pField->m_elemCnt);
			else
				clangBitWidth += pType->m_clangBitWidth * pField->m_elemCnt;
		}

		pField->m_clangBitPos = clangBitPos;
		if (!pRecord->m_bUnion)
			clangBitPos += fieldWidth * pField->m_elemCnt;

		clangMinAlign = max(clangMinAlign, pType->m_clangMinAlign);
	}

	if (clangBitWidth % clangMinAlign > 0)
		clangBitWidth = clangBitWidth + clangMinAlign - clangBitWidth % clangMinAlign;

	pRecord->m_clangBitWidth = clangBitWidth;
	pRecord->m_clangMinAlign = clangMinAlign;
}

// Check if type name is valid C language integer, if so return stdint type
CHtInt g_void(eUnsigned, "void", 0, 0);
CHtInt g_bool(eSigned, "bool", 1, 8);
CHtInt g_uint8(eUnsigned, "uint8_t", 8, 8);
CHtInt g_uint16(eUnsigned, "uint16_t", 16, 16);
CHtInt g_uint32(eUnsigned, "uint32_t", 32, 32);
CHtInt g_uint64(eUnsigned, "uint64_t", 64, 64);
CHtInt g_int8(eSigned, "int8_t", 8, 8);
CHtInt g_int16(eSigned, "int16_t", 16, 16);
CHtInt g_int32(eSigned, "int32_t", 32, 32);
CHtInt g_int64(eSigned, "int64_t", 64, 64);

CType * CDsnInfo::FindClangType(string typeName)
{
	CTypeDef * pTypeDef;
	do {
		pTypeDef = FindTypeDef(typeName);
		if (pTypeDef) typeName = pTypeDef->m_type;
	} while (pTypeDef);

	bool bAllowHostPtr = false;
	vector<string> typeVec;

	// parse typeName into individual symbols
	char const * pType = typeName.c_str();
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

#define C_BOOL 0x0001
#define C_CHAR 0x0002
#define C_SHORT 0x0004
#define C_INT 0x0008
#define C_LONG 0x0010
#define C_LONG_LONG 0x0020
#define C_UNSIGNED 0x0040
#define C_VOID 0x0080
#define C_UINT8 0x0100
#define C_UINT16 0x0200
#define C_UINT32 0x0400
#define C_UINT64 0x0800
#define C_SINT8 0x1000
#define C_SINT16 0x2000
#define C_SINT32 0x4000
#define C_SINT64 0x8000

	uint16_t typeMask = 0;

	for (size_t i = 0; i < typeVec.size(); i += 1) {
		string &type = typeVec[i];

		if (type == "bool") {
			typeMask |= C_BOOL;
			if (bNative || bPtr || bHostVar) return 0;
			bNative = true;
			continue;
		}

		if (type == "char") {
			typeMask |= C_CHAR;
			if (bNative || bPtr || bHostVar) return 0;
			bNative = true;
			continue;
		}

		if (type == "short") {
			typeMask |= C_SHORT;
			if (bNative || bPtr || bHostVar) return 0;
			bNative = true;
			continue;
		}

		if (type == "int") {
			typeMask |= C_INT;
			if (bNative || bPtr || bHostVar) return 0;
			bNative = true;
			continue;
		}

		if (type == "uint8_t") {
			typeMask |= C_UINT8;
			if (bNative || bPtr || bHostVar) return 0;
			bNative = true;
			continue;
		}

		if (type == "uint16_t") {
			typeMask |= C_UINT16;
			if (bNative || bPtr || bHostVar) return 0;
			bNative = true;
			continue;
		}

		if (type == "uint32_t") {
			typeMask |= C_UINT32;
			if (bNative || bPtr || bHostVar) return 0;
			bNative = true;
			continue;
		}

		if (type == "uint64_t") {
			typeMask |= C_UINT64;
			if (bNative || bPtr || bHostVar) return 0;
			bNative = true;
			continue;
		}

		if (type == "int8_t") {
			typeMask |= C_SINT8;
			if (bNative || bPtr || bHostVar) return 0;
			bNative = true;
			continue;
		}

		if (type == "int16_t") {
			typeMask |= C_SINT16;
			if (bNative || bPtr || bHostVar) return 0;
			bNative = true;
			continue;
		}

		if (type == "int32_t") {
			typeMask |= C_SINT32;
			if (bNative || bPtr || bHostVar) return 0;
			bNative = true;
			continue;
		}

		if (type == "int64_t") {
			typeMask |= C_SINT64;
			if (bNative || bPtr || bHostVar) return 0;
			bNative = true;
			continue;
		}

		if (type == "long") {
			if (bNative || bPtr || bHostVar) return 0;
			bNative = true;
			if (typeVec.size() > i + 1 && typeVec[i + 1] == "long") {
				i += 1;
				typeMask |= C_LONG_LONG;
			} else
				typeMask |= C_LONG;
			continue;
		}

		if (type == "unsigned") {
			typeMask |= C_UNSIGNED;
			if (bPtr || bHostVar) return 0;
			continue;
		}

		if (type == "*") {
			if (!bAllowHostPtr) return 0;
			bPtr = true;
			continue;
		}

		// pointer to void is allow for host parameters
		if (type == "void") {
			if (bNative || bPtr || bHostVar) return 0;
			bHostVar = true;
			typeMask |= C_VOID;
			continue;
		}

		return 0;
	}

	// type is valid
	switch (typeMask) {
	case C_VOID: return &g_void;
	case C_BOOL: return &g_bool;
	case C_CHAR: return &g_int8;
	case C_SHORT: return &g_int16;
	case C_INT: return &g_int32;
	case C_LONG: return &g_int64;
	case C_LONG_LONG: return &g_int64;

		//case C_UNSIGNED | C_BOOL: return g_pBool;
	case C_UNSIGNED | C_CHAR: return &g_uint8;
	case C_UNSIGNED | C_SHORT: return &g_uint16;
	case C_UNSIGNED | C_INT: return &g_uint32;
	case C_UNSIGNED | C_LONG: return &g_uint64;
	case C_UNSIGNED | C_LONG_LONG: return &g_uint64;

	case C_UINT8: return &g_uint8;
	case C_UINT16: return &g_uint16;
	case C_UINT32: return &g_uint32;
	case C_UINT64: return &g_uint64;
	case C_SINT8: return &g_int8;
	case C_SINT16: return &g_int16;
	case C_SINT32: return &g_int32;
	case C_SINT64: return &g_int64;

	default:
		HtlAssert(0);
	}

	return 0;// bNative || bAllowHostPtr && bHostVar && bPtr;
}

CType * CDsnInfo::FindHtIntType(string typeName, CLineInfo const & lineInfo)
{
	CTypeDef * pTypeDef;
	do {
		pTypeDef = FindTypeDef(typeName);
		if (pTypeDef) typeName = pTypeDef->m_type;
	} while (pTypeDef);

	bool bScUint = false;
	bool bScInt = false;
	bool bHtUint = false;
	bool bHtInt = false;
	bool bScBigUint = false;
	bool bScBigInt = false;
	int width = 0;

	char const * pStr = typeName.c_str();
	if (strncmp(pStr, "sc_uint", 7) == 0) {
		bScUint = true;
		pStr += 7;
	} else if (strncmp(pStr, "sc_int", 6) == 0) {
		bScInt = true;
		pStr += 6;
	} else if (strncmp(pStr, "ht_uint", 7) == 0) {
		bHtUint = true;
		pStr += 7;
	} else if (strncmp(pStr, "ht_int", 6) == 0) {
		bHtInt = true;
		pStr += 6;
	} else if (strncmp(pStr, "sc_biguint", 10) == 0) {
		bScBigUint = true;
		pStr += 10;
	} else if (strncmp(pStr, "sc_bigint", 9) == 0) {
		bScBigInt = true;
		pStr += 9;
	} else
		return 0;

	if (bScUint || bScInt || bScBigUint || bScBigInt) {
		// find template parameter
		while (*pStr == ' ' || *pStr == '\t') pStr += 1;

		if (*pStr++ != '<') {
			ParseMsg(Error, lineInfo, "unrecognized type");
			return 0;
		}

		// find next '>' not within parens
		bool bErr;
		CHtString param = ParseTemplateParam(lineInfo, pStr, bErr);
		if (bErr) return 0;

		pStr += 1;
		while (*pStr == ' ' || *pStr == '\t') pStr += 1;

		if (*pStr != '\0') {
			ParseMsg(Error, lineInfo, "unrecognized type");
			return 0;
		}

		param.InitValue(lineInfo);
		width = param.AsInt();

		if (width <= 0)
			return 0;

	} else if (bHtUint || bHtInt) {
		char * pEnd;
		width = strtol(pStr, &pEnd, 10);

		if (pEnd == pStr) {
			ParseMsg(Error, lineInfo, "unrecognized type");
			return 0;
		}

		pStr = pEnd;
		while (*pStr == ' ' || *pStr == '\t') pStr += 1;

		if (*pStr != '\0') {
			ParseMsg(Error, lineInfo, "unrecognized type");
			return 0;
		}
	}

	ESign sign = (bHtInt || bScInt || bScBigInt) ? eSigned : eUnsigned;

	if (bScBigUint || bScBigInt)
		return FindScBigIntType(sign, width);
	else
		return FindHtIntType(sign, width);
}

CType * CDsnInfo::FindHtIntType(ESign sign, int width)
{
	for (size_t i = 0; i < m_htIntList.size(); i += 1) {
		CHtInt * pHtInt = m_htIntList[i];

		if (pHtInt->m_clangBitWidth == width && pHtInt->m_eSign == sign)
			return pHtInt;
	}

	string typeStr = sign == eSigned ? VA("ht_int%d", width) : VA("ht_uint%d", width);
	CHtInt * pHtInt = new CHtInt(sign, typeStr, width, 1);
	m_htIntList.push_back(pHtInt);

	return pHtInt;
}

CType * CDsnInfo::FindScBigIntType(ESign sign, int width)
{
	for (size_t i = 0; i < m_scBigIntList.size(); i += 1) {
		CHtInt * pHtInt = m_scBigIntList[i];

		if (pHtInt->m_clangBitWidth == width && pHtInt->m_eSign == sign)
			return pHtInt;
	}

	string typeStr = sign == eSigned ? VA("sc_bigint<%d>", width) : VA("sc_biguint<%d>", width);
	CHtInt * pHtInt = new CHtInt(sign, typeStr, width, 1);
	m_scBigIntList.push_back(pHtInt);

	return pHtInt;
}

int CDsnInfo::FindClangWidthAndMinAlign(CField * pField, int & minAlign)
{

	return 0;
}

string CDsnInfo::ParseTemplateParam(CLineInfo const & lineInfo, char const * &pStr, bool & bErr)
{
	// find next '>' not within parens
	bErr = false;
	char const * pStart = pStr;
	int parenCnt = 0;
	while (parenCnt > 0 || !(*pStr == '>' && pStr[1] != '>')) {
		if (*pStr == '\0') {
			ParseMsg(Error, lineInfo, "unrecognized type");
			bErr = true;
			return string();
		}

		if (*pStr == '(')
			parenCnt += 1;
		else if (*pStr == ')') {
			if (parenCnt == 0) {
				ParseMsg(Error, lineInfo, "unrecognized type");
				bErr = true;
				return string();
			}
			parenCnt -= 1;
		}
		pStr += 1;
	}

	if (parenCnt != 0) {
		CPreProcess::ParseMsg(Error, "template argument has unbalanced parenthesis");
		bErr = true;
		return string();
	}

	if (*pStr != '>') {
		CPreProcess::ParseMsg(Error, "expected template argument '< >'");
		bErr = true;
		return string();
	}
	return string(pStart, pStr - pStart);
}

void CDsnInfo::ReportRamFieldUsage()
{
	if (!m_bramTargetList.size()) return;

	FILE * fp = g_appArgs.GetDsnRpt().GetFp();

	const char *ws = "        ";

	fprintf(fp, "%s<h1><b>Generated RAMs</b></h1>\n", ws);
	fprintf(fp, "%s  <dl>\n", ws);
	fprintf(fp, "%s    <dt><b>Summary</b></dt>\n", ws);
	fprintf(fp, "%s      <br/>\n", ws);
	fprintf(fp, "%s      <dd>BRAMs per %s AE: %d</dd>\n", ws, g_appArgs.GetCoprocName(), g_appArgs.GetBramsPerAE());
	fprintf(fp, "%s      <dd>BRAM usage limit: 50%%</dd>\n", ws);
	fprintf(fp, "%s      <dd>Max 18Kb BRAMs per Unit (use -ub to override): %d (%d BRAMs * 50%% / %d units)</dd>\n\n",
		ws, g_appArgs.GetMax18KbBramPerUnit(), g_appArgs.GetBramsPerAE(), g_appArgs.GetAeUnitCnt());
	fprintf(fp, "%s      <dd>Min Slice to BRAM ratio<sup>1</sup> (use -sb to override): %d</dd>\n",
		ws, g_appArgs.GetMinSliceToBramRatio());
	fprintf(fp, "%s      <dd><dl><dd><sup>1</sup> The ratio is used to determine how many FPGA slices are required\n", ws);
	fprintf(fp, "%s      when implementing a memory as a BRAM versus a distributed ram.</dd></dl></dd>\n", ws);
	fprintf(fp, "%s    <br/>\n", ws);
	fprintf(fp, "%s    <dt><b>Arrayed Variable(s)</b></dt>\n", ws);
	fprintf(fp, "%s    <br/>\n", ws);
	fprintf(fp, "%s    <dl><dd><table border=\"1\">\n", ws);
	fprintf(fp, "%s      <tr>\n", ws);
	fprintf(fp, "%s        <th rowspan=2>Module Name</th>\n", ws);
	fprintf(fp, "%s        <th rowspan=2>Variable Type</th>\n", ws);
	fprintf(fp, "%s        <th rowspan=2>Ram/Field</th>\n", ws);
	fprintf(fp, "%s        <th rowspan=2>Depth</th>\n", ws);
	fprintf(fp, "%s        <th rowspan=2>Width</th>\n", ws);
	fprintf(fp, "%s        <th rowspan=2>Copies</th>\n", ws);
	fprintf(fp, "%s        <th colspan=2>Distributed</th>\n", ws);
	fprintf(fp, "%s        <th colspan=2>Block</th>\n", ws);
	fprintf(fp, "%s        <th rowspan=2>Ratio</th>\n", ws);
	fprintf(fp, "%s        <th rowspan=2>Selected</th>\n", ws);
	fprintf(fp, "%s      </tr>\n", ws);
	fprintf(fp, "%s      <tr>\n", ws);
	fprintf(fp, "%s        <th># Slices</th>\n", ws);
	fprintf(fp, "%s        <th>Total</th>\n", ws);
	fprintf(fp, "%s        <th># 18Kb</th>\n", ws);
	fprintf(fp, "%s        <th>Total</th>\n", ws);
	fprintf(fp, "%s      </tr>\n", ws);

	for (size_t targetIdx = 0; targetIdx < m_bramTargetList.size(); targetIdx += 1) {
		CBramTarget & target = m_bramTargetList[targetIdx];

		if (target.m_depth == 1) continue;

		fprintf(fp, "%s      <tr>\n", ws);
		fprintf(fp, "%s        <td>%-17s</td><td>%-17s</td><td>%-16s</td><td>%2d</td><td>%2d</td><td>%2d</td><td>%2d</td><td>%2d</td><td>%2d</td><td>%2d</td><td>%6.2f</td><td>%s</td>\n",
			ws, target.m_modName.c_str(), target.m_varType.c_str(),
			target.m_name.c_str(), target.m_depth, target.m_width,
			target.m_copies, target.m_slices, target.m_slices * target.m_copies,
			target.m_brams, target.m_brams * target.m_copies,
			(double)target.m_slicePerBramRatio,
			*target.m_pRamType == eDistRam ? "Dist" : "Block");
		fprintf(fp, "%s      </tr>\n", ws);
	}

	fprintf(fp, "%s    </dd></dl></table>\n", ws);

	fprintf(fp, "%s  </dl>\n", ws);
}

//--------------------------------------------------------------------
// Dump the modules into Graphviz/dot format so that the design can be
// visualized graphically.
// To view:
//   % dot -Tps file.dot -o file.ps
//
// We first draw the overall module call/cmd relationships, followed by
// each individual module.
//--------------------------------------------------------------------
void CDsnInfo::DrawModuleCmdRelationships()
{
	fstream fs;
	fs.open("HtCallGraph.dot", fstream::out | fstream::trunc);
	fs << "digraph module_capabilities {\n";

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		int modInstCnt = mod.m_instSet.GetReplCnt(0);
		HtlAssert(modInstCnt <= 8 && "bad replication count");
		string rankstr = "";
		for (int rep = 0; rep < modInstCnt; rep++) {
			string modName = mod.m_modName.Uc();
			string mtext = modName;
			string modRepStr = "";
			if (modInstCnt > 1) {
				modRepStr = "01234567"[rep];
				modName += modRepStr;
				mtext += ("." + modRepStr);
				rankstr += ("n" + modName + " ");
			}
			fs << "n" << modName << " [shape=\"box\", label=\"" << mtext << "\\n\"];\n";

			// cxr
			CInstance * pModInst = mod.m_instSet.GetInst(0, rep);
			map<string, bool> cxr_seen;
			for (size_t intfIdx = 0; intfIdx < pModInst->m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf * pCxrIntf = pModInst->m_cxrIntfList[intfIdx];

				string key = pCxrIntf->GetIntfName();
				if (cxr_seen.find(key) != cxr_seen.end()) continue;
				cxr_seen.insert(pair<string, bool>(key, true));

				int destUnitCnt = (int)pCxrIntf->m_pDstModInst->m_replCnt;
				int numArcs = 1;
				if (destUnitCnt > modInstCnt) {
					numArcs = destUnitCnt - modInstCnt + 1;
				}
				for (int arc = 0; arc < numArcs; arc++) {

					string destName = pCxrIntf->m_pDstModInst->m_replInstName.Uc();

					string label = " [label=\"";
					label += pCxrIntf->m_modEntry.c_str();
					label += "\"";
					label += pCxrIntf->IsCall() ? " color=red" :
						pCxrIntf->IsXfer() ? " color=deeppink" : " color=green";
					label += "]";

					if (pCxrIntf->m_cxrDir == CxrOut) {
						fs << "n" << modName << " -> n" << destName;
						fs << label << ";\n";
					}
				}
			}

			// msg
			for (size_t outIdx = 0; outIdx < mod.m_msgIntfList.size(); outIdx += 1) {
				CMsgIntf * pOutMsgIntf = mod.m_msgIntfList[outIdx];
				if (pOutMsgIntf->m_bInBound) continue;
				for (size_t modIdx2 = 0; modIdx2 < m_modList.size(); modIdx2 += 1) {
					CModule &mod2 = *m_modList[modIdx2];
					if (!mod2.m_bIsUsed) continue;
					for (size_t msgIdx = 0; msgIdx < mod2.m_msgIntfList.size(); msgIdx += 1) {
						CMsgIntf * pMsgIntf = mod2.m_msgIntfList[msgIdx];
						if (!pMsgIntf->m_bInBound) continue;
						if (pMsgIntf->m_name != pOutMsgIntf->m_name) continue;

						int destUnitCnt = (int)mod2.m_instSet.GetReplCnt(0);
						int numArcs = 1;
						if (destUnitCnt > modInstCnt) {
							numArcs = destUnitCnt - modInstCnt + 1;
						}
						for (int arc = 0; arc < numArcs; arc++) {
							string destRepStr = "";
							if (destUnitCnt == 1)
								destRepStr = "";
							else if (destUnitCnt > modInstCnt)
								destRepStr = "01234567"[arc];
							else
								destRepStr = "01234567"[rep];

							string destName = mod2.m_modName.Uc() + destRepStr;

							string label = " [label=\"";
							label += pMsgIntf->m_name.c_str();
							label += "\"]";

							fs << "n" << modName << " -> n" << destName;
							fs << label << " [style=dotted] [weight=50];\n";
						}
					}
				}
			}

			// host msg
			string ihm = "", ohm = "";
			for (size_t hmIdx = 0; hmIdx < mod.m_hostMsgList.size(); hmIdx += 1) {
				CHostMsg &hostMsg = mod.m_hostMsgList[hmIdx];
				if (hostMsg.m_msgDir == Outbound) {
					if (ohm.size() > 0) ohm.append("\\n");
					ohm.append(hostMsg.m_msgName.c_str());
				} else {
					if (ihm.size() > 0) ihm.append("\\n");
					ihm.append(hostMsg.m_msgName.c_str());
				}
			}
			if (ihm.size()) {
				fs << "nHif -> n" << modName;
				fs << " [label=\"" << ihm << "\"]";
				fs << " [style=dotted] [weight=100];\n";
			}
			if (ohm.size()) {
				fs << "n" << modName << " -> nHif";
				fs << " [label=\"" << ohm << "\"]";
				fs << " [style=dotted] [weight=100];\n";
			}
		}
		if (modInstCnt > 1) {
			fs << "{ rank=same; " << rankstr << "};\n";
		}
	}

	fs << "}\n\n";
	fs.close();
}
