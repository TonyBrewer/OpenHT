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

void CDsnInfo::InitAndValidateModMif()
{
	// foreach MIF find the maximum dst ram address width (number of bits needed in response TID).
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];
		CMif &mif = mod.m_mif;

		if (!mif.m_bMif) continue;

		mod.m_rsmSrcCnt += (mif.m_bMifRd && mod.m_mif.m_mifRd.m_bPause) ? 1 : 0;
		mod.m_rsmSrcCnt += (mif.m_bMifWr && mod.m_mif.m_mifWr.m_bPause) ? 1 : 0;

		if (mif.m_bMifRd) {
			mif.m_mifRd.m_queueW.InitValue( mif.m_mifRd.m_lineInfo );
			mif.m_mifRd.m_rspCntW.InitValue( mif.m_mifRd.m_lineInfo );
			mif.m_queueW = mif.m_mifRd.m_queueW.AsInt();
		}

		if (mif.m_bMifWr) {
			mif.m_mifWr.m_queueW.InitValue( mif.m_mifWr.m_lineInfo );
			mif.m_mifWr.m_rspCntW.InitValue( mif.m_mifWr.m_lineInfo );
			mif.m_queueW = mif.m_mifWr.m_queueW.AsInt();
		}

		if (mif.m_bMifRd && mif.m_bMifWr) {
			if (mif.m_mifRd.m_queueW.AsInt() != mif.m_mifWr.m_queueW.AsInt())
				ParseMsg(Error, mif.m_mifRd.m_lineInfo, "expected queueW parameter for AddReadMem and AddWriteMem to have the same value");
		}

		// Find m_mifRd.m_rspGrpIdW value
		if (mif.m_bMifRd) {
			if (mif.m_mifRd.m_rspGrpId.size() == 0)
				mif.m_mifRd.m_rspGrpIdW = mod.m_threads.m_htIdW.AsInt();
			else {
				if (!mod.m_bHasThreads)
					ParseMsg(Fatal, mif.m_mifRd.m_lineInfo, "private variable for rspGrpId not supported when multiple groups are defined");

				// find private variable with specified name
				size_t prIdx;
				for (prIdx = 0; prIdx < mod.m_threads.m_htPriv.m_fieldList.size(); prIdx += 1) {
					CField & priv = mod.m_threads.m_htPriv.m_fieldList[prIdx];

					if (priv.m_name != mif.m_mifRd.m_rspGrpId.AsStr()) continue;

					// found a private variable
					mif.m_mifRd.m_rspGrpIdW = FindTypeWidth(priv.m_name, priv.m_type, priv.m_bitWidth, mif.m_mifRd.m_lineInfo);
					mif.m_mifRd.m_bRspGrpIdPriv = true;

					break;
				}

				if (prIdx == mod.m_threads.m_htPriv.m_fieldList.size()) {
					// private variable not found, check shared variables
					size_t shIdx;
					for (shIdx = 0; shIdx < mod.m_shared.m_fieldList.size(); shIdx += 1) {
						CField & shared = mod.m_shared.m_fieldList[shIdx];

						if (shared.m_name != mif.m_mifRd.m_rspGrpId.AsStr()) continue;

						// found a private variable
						mif.m_mifRd.m_rspGrpIdW = FindTypeWidth(shared.m_name, shared.m_type, shared.m_bitWidth, mif.m_mifRd.m_lineInfo);
						mif.m_mifRd.m_bRspGrpIdPriv = false;

						break;
					}

					if (shIdx == mod.m_shared.m_fieldList.size())
						ParseMsg(Fatal, mif.m_mifRd.m_lineInfo, "unable to find rspGrpId as private or shared variable, '%s'", mif.m_mifRd.m_rspGrpId.AsStr().c_str());
				}
			}
		}

		// Find m_mifWd.m_rspGrpIdW value
		if (mif.m_bMifWr) {
			if (mif.m_mifWr.m_rspGrpId.size() == 0)
				mif.m_mifWr.m_rspGrpIdW = mod.m_threads.m_htIdW.AsInt();
			else {
				if (!mod.m_bHasThreads)
					ParseMsg(Fatal, mif.m_mifWr.m_lineInfo, "private variable for rspGrpId not supported when multiple groups are defined");

				// find private variable with specified name
				size_t prIdx;
				for (prIdx = 0; prIdx < mod.m_threads.m_htPriv.m_fieldList.size(); prIdx += 1) {
					CField & priv = mod.m_threads.m_htPriv.m_fieldList[prIdx];

					if (priv.m_name != mif.m_mifWr.m_rspGrpId.AsStr()) continue;

					// found a private variable
					mif.m_mifWr.m_rspGrpIdW = FindTypeWidth(priv.m_name, priv.m_type, priv.m_bitWidth, mif.m_mifWr.m_lineInfo);
					mif.m_mifWr.m_bRspGrpIdPriv = true;

					break;
				}

				if (prIdx == mod.m_threads.m_htPriv.m_fieldList.size()) {
					// private variable not found, check shared variables
					size_t shIdx;
					for (shIdx = 0; shIdx < mod.m_shared.m_fieldList.size(); shIdx += 1) {
						CField & shared = mod.m_shared.m_fieldList[shIdx];

						if (shared.m_name != mif.m_mifWr.m_rspGrpId.AsStr()) continue;

						if (shared.m_rdSelW.AsInt() > 0 || shared.m_wrSelW.AsInt() > 0)
							ParseMsg(Error, mif.m_mifWr.m_lineInfo, "unable to use shared asymetrical block ram for source variable");

						// found a private variable
						mif.m_mifWr.m_rspGrpIdW = FindTypeWidth(shared.m_name, shared.m_type, shared.m_bitWidth, mif.m_mifWr.m_lineInfo);
						mif.m_mifWr.m_bRspGrpIdPriv = false;

						break;
					}

					if (shIdx == mod.m_shared.m_fieldList.size())
						ParseMsg(Fatal, mif.m_mifWr.m_lineInfo, "unable to find rspGrpId private variable, '%s'", mif.m_mifWr.m_rspGrpId.AsStr().c_str());
				}
			}
		}

		if (mif.m_bMifRd) {

			mif.m_mifRd.m_maxRsmDly = 0;
			for (size_t rdDstIdx = 0; rdDstIdx < mif.m_mifRd.m_rdDstList.size(); rdDstIdx += 1) {
				CMifRdDst & rdDst = mif.m_mifRd.m_rdDstList[rdDstIdx];

				if (rdDst.m_bMultiRd)
					mif.m_mifRd.m_bMultiRd = true;

				if (rdDst.m_rdType[0] == 'i')
					sscanf(rdDst.m_rdType.c_str()+3, "%d", &rdDst.m_memSize);
				else
					sscanf(rdDst.m_rdType.c_str()+4, "%d", &rdDst.m_memSize);

				if (rdDst.m_infoW.size() > 0) {
					mif.m_mifRd.m_maxRsmDly = max(mif.m_mifRd.m_maxRsmDly, rdDst.m_rsmDly.AsInt());
					continue;	// function call
				}

				rdDst.m_dataLsb.InitValue(rdDst.m_lineInfo);

				// first check if rdDst name is unique
				bool bFoundIntGbl = false;
				bool bFoundExtGbl = false;
				bool bFoundShared = false;
				bool bFoundPrivate = false;
				bool bFoundNewGbl = false;
				int ramIdx = 0;

				for (size_t ngvIdx = 0; ngvIdx < mod.m_globalVarList.size(); ngvIdx += 1) {
					CRam * pNgv = mod.m_globalVarList[ngvIdx];

					if (pNgv->m_gblName == rdDst.m_var) {
						bFoundNewGbl = true;
						ramIdx = ngvIdx;
					}
				}

				for (size_t igvIdx = 0; igvIdx < mod.m_intGblList.size(); igvIdx += 1) {
					CRam &intRam = *mod.m_intGblList[igvIdx];

					if (intRam.m_gblName == rdDst.m_var) {
						bFoundIntGbl = true;
						ramIdx = igvIdx;
					}
				}

				for (size_t egvIdx = 0; egvIdx < mod.m_extRamList.size(); egvIdx += 1) {
					CRam &extRam = mod.m_extRamList[egvIdx];

					if (extRam.m_gblName == rdDst.m_var) {
						bFoundExtGbl = true;
						ramIdx = egvIdx;
					}
				}

				for (size_t shIdx = 0; shIdx < mod.m_shared.m_fieldList.size(); shIdx += 1) {
					CField & shared = mod.m_shared.m_fieldList[shIdx];

					if (shared.m_name == rdDst.m_var) {
						bFoundShared = true;
						ramIdx = shIdx;
					}
				}

				for (size_t prIdx = 0; prIdx < mod.m_threads.m_htPriv.m_fieldList.size(); prIdx += 1) {
					CField & priv = mod.m_threads.m_htPriv.m_fieldList[prIdx];

					if (priv.m_name == rdDst.m_var) {
						bFoundPrivate = true;
						ramIdx = prIdx;
					}
				}

				if (bFoundNewGbl + bFoundIntGbl + bFoundExtGbl + bFoundShared + bFoundPrivate == 0) {

					ParseMsg(Fatal, rdDst.m_lineInfo, "unable to find private, shared or global destination variable, '%s'", rdDst.m_var.c_str());

				} else if (bFoundNewGbl + bFoundIntGbl + bFoundExtGbl + bFoundShared + bFoundPrivate > 1) {
					string msg;
					if (bFoundNewGbl)
						msg += "new global";
					if (bFoundIntGbl || bFoundExtGbl)
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
					CRam * pNgv = mod.m_globalVarList[ramIdx];

					rdDst.m_varAddr0W = 0;
					rdDst.m_varAddr1W = pNgv->m_addr1W.AsInt();
					rdDst.m_varAddr2W = pNgv->m_addr2W.AsInt();
					rdDst.m_varDimen1 = pNgv->GetDimen(0);
					rdDst.m_varDimen2 = pNgv->GetDimen(1);

					// check if ram has field specified in AddRdDst
					CStructElemIter iter(this, pNgv->m_type);
					for ( ; !iter.end(); iter++) {
						if (iter.IsStructOrUnion()) continue;

						if (iter.GetHeirFieldName(false) == rdDst.m_field)
							break;
					}

					if (iter.end()) {
						ParseMsg(Error, rdDst.m_lineInfo, "field '%s' does not exist for variable '%s'",
							rdDst.m_field.c_str(), rdDst.m_var.c_str());

					}  else {
						CField & field = iter();

						pNgv->m_bMifWrite = true;
						field.m_bMifWrite = true;

						rdDst.m_pNgvRam = pNgv;
						rdDst.m_fldDimen1 = field.GetDimen(0);
						rdDst.m_fldDimen2 = field.GetDimen(1);

						// verify that fieldW + dataLsb <= 64
						rdDst.m_fldW = FindTypeWidth(field);
						rdDst.m_memSize = rdDst.m_fldW;

						if (rdDst.m_fldW != 8 && rdDst.m_fldW != 16 && rdDst.m_fldW != 32 && rdDst.m_fldW != 64)
							ParseMsg(Error, rdDst.m_lineInfo, "field width (%d) must be 8, 16, 32 or 64 bits", rdDst.m_fldW);

						if (rdDst.m_fldW + rdDst.m_dataLsb.AsInt() > rdDst.m_memSize)
							ParseMsg(Error, rdDst.m_lineInfo, "sum of dst field width (%d) and dataLsb (%d) must be less than rdType width (%d)",
							rdDst.m_fldW, rdDst.m_dataLsb.AsInt(), rdDst.m_memSize);
					}


				} else if (bFoundIntGbl) {
					CRam &intRam = *mod.m_intGblList[ramIdx];

					if (intRam.m_gblName == rdDst.m_var) {

						rdDst.m_varAddr0W = 0;
						rdDst.m_varAddr1W = intRam.m_addr1W.AsInt();
						rdDst.m_varAddr2W = intRam.m_addr2W.AsInt();
						rdDst.m_varDimen1 = intRam.GetDimen(0);
						rdDst.m_varDimen2 = intRam.GetDimen(1);

						// check if ram has field specified in AddRdDst
						size_t fldIdx;
						for (fldIdx = 0; fldIdx < intRam.m_fieldList.size(); fldIdx += 1) {
							CField & field = intRam.m_fieldList[fldIdx];

							if (field.m_name == rdDst.m_field)
								break;
						}

						if (fldIdx == intRam.m_fieldList.size())
							ParseMsg(Error, rdDst.m_lineInfo, "field '%s' does not exist for variable '%s'",
							rdDst.m_field.c_str(), rdDst.m_var.c_str());

						else {
							CField & field = intRam.m_fieldList[fldIdx];

							intRam.m_bMifWrite = true;
							field.m_bMifWrite = true;

							rdDst.m_pIntGbl = &intRam;
							rdDst.m_fldDimen1 = field.GetDimen(0);
							rdDst.m_fldDimen2 = field.GetDimen(1);

							// verify that fieldW + dataLsb <= 64
							rdDst.m_fldW = FindTypeWidth(field);

							if (rdDst.m_fldW + rdDst.m_dataLsb.AsInt() > rdDst.m_memSize)
								ParseMsg(Error, rdDst.m_lineInfo, "sum of dst field width (%d) and dataLsb (%d) must be less than rdType width (%d)",
								rdDst.m_fldW, rdDst.m_dataLsb.AsInt(), rdDst.m_memSize);
						}
					}
				} else if (bFoundExtGbl) {

					CRam &extRam = mod.m_extRamList[ramIdx];

					if (extRam.m_gblName == rdDst.m_var) {

						rdDst.m_varAddr0W = 0;
						rdDst.m_varAddr1W = extRam.m_addr1W.AsInt();
						rdDst.m_varAddr2W = extRam.m_addr2W.AsInt();
						rdDst.m_varDimen1 = extRam.GetDimen(0);
						rdDst.m_varDimen2 = extRam.GetDimen(1);

						// check if ram has field specified in AddRdDst
						size_t fldIdx;
						for (fldIdx = 0; fldIdx < extRam.m_fieldList.size(); fldIdx += 1) {
							CField & field = extRam.m_fieldList[fldIdx];

							if (field.m_name == rdDst.m_field)
								break;
						}

						if (fldIdx == extRam.m_fieldList.size())
							ParseMsg(Error, rdDst.m_lineInfo, "field '%s' does not exist for variable '%s'",
							rdDst.m_field.c_str(), rdDst.m_var.c_str());

						else {
							CField & field = extRam.m_fieldList[fldIdx];

							extRam.m_bMifWrite = true;
							field.m_bMifWrite = true;

							rdDst.m_pExtRam = &extRam;
							rdDst.m_fldDimen1 = field.GetDimen(0);
							rdDst.m_fldDimen2 = field.GetDimen(1);

							// verify that fieldW + dataLsb <= 64
							rdDst.m_fldW = FindTypeWidth(field);

							if (rdDst.m_fldW + rdDst.m_dataLsb.AsInt() > 64)
								ParseMsg(Error, rdDst.m_lineInfo, "sum of field width (%d) and dataLsb (%d) must be less than 64",
								rdDst.m_fldW, rdDst.m_dataLsb.AsInt());
						}
					}
				} else if (bFoundShared) {

					CField & shared = mod.m_shared.m_fieldList[ramIdx];

					if (shared.m_name != rdDst.m_var) continue;

					// found a shared variable
					rdDst.m_pSharedVar = &shared;
					rdDst.m_varAddr0W = 0;
					rdDst.m_varAddr1W = shared.m_addr1W.AsInt();
					rdDst.m_varAddr2W = shared.m_addr2W.AsInt();
					rdDst.m_varDimen1 = shared.GetDimen(0);
					rdDst.m_varDimen2 = shared.GetDimen(1);

					if (rdDst.m_field.size() == 0)
						rdDst.m_fldW = FindTypeWidth(shared);

					else {
						// check if shared variable has field specified in AddDst
						bool bCStyle;
						CField const * pBaseField, *pLastField;
						bool bFldFound = FindFieldInStruct(rdDst.m_lineInfo, shared.m_type, rdDst.m_field, false,
							bCStyle, pBaseField, pLastField);

						if (!bCStyle)
							rdDst.m_field = "m_" + rdDst.m_field;

						if (bFldFound) {
							rdDst.m_fldDimen1 = pBaseField->GetDimen(0);
							rdDst.m_fldDimen2 = pBaseField->GetDimen(1);

							// verify that fieldW + dataLsb <= 64
							rdDst.m_fldW = FindTypeWidth(*pLastField);

							if (rdDst.m_fldW + rdDst.m_dataLsb.AsInt() > 64)
								ParseMsg(Error, rdDst.m_lineInfo, "sum of field width (%d) and dataLsb (%d) must be less than 64",
								rdDst.m_fldW, rdDst.m_dataLsb.AsInt());

						} else
							ParseMsg(Error, rdDst.m_lineInfo, "field '%s' does not exist for shared variable '%s'",
							rdDst.m_field.c_str(), rdDst.m_var.c_str());
					}

					if (rdDst.m_bMultiRd && shared.m_queueW.AsInt() == 0 && rdDst.m_dstIdx.size() == 0)
						ParseMsg(Error, "Multi-quadword reads require a dstIdx to be specified");

					if (rdDst.m_varAddr1W > 0 && rdDst.m_dstIdx.substr(0, 3) == "fld")
						ParseMsg(Error, rdDst.m_lineInfo, "AddDst field indexing on a shared variable with depth is not supported");

				} else if (bFoundPrivate) {

					CField & priv = mod.m_threads.m_htPriv.m_fieldList[ramIdx];

					if (priv.m_name != rdDst.m_var) continue;

					if (mod.m_clkRate == eClk2x)
						ParseMsg(Fatal, rdDst.m_lineInfo, "private variables with memory read or write within clk2x module not supported");

					// found a private variable
					if (priv.m_pPrivGbl == 0) {
						// create a global variable
						string rdStg = "1";
						string wrStg = mod.m_stage.m_execStg.AsStr();
						string nullStr;

						priv.m_pPrivGbl = &mod.AddIntRam(priv.m_name, priv.m_addr1Name, priv.m_addr2Name,
							priv.m_addr1W.AsStr(), priv.m_addr2W.AsStr(), nullStr,
							nullStr, rdStg, wrStg);
						priv.m_pPrivGbl->m_addr0W = mod.m_threads.m_htIdW;

						priv.m_pPrivGbl->m_bPrivGbl = true;
						priv.m_pPrivGbl->m_addr0W.InitValue(priv.m_pPrivGbl->m_lineInfo, false, 0);
						priv.m_pPrivGbl->m_addr1W.InitValue(priv.m_pPrivGbl->m_lineInfo, false, 0);
						priv.m_pPrivGbl->m_addr2W.InitValue(priv.m_pPrivGbl->m_lineInfo, false, 0);
						priv.m_pPrivGbl->DimenListInit(priv.m_pPrivGbl->m_lineInfo);
						priv.m_pPrivGbl->m_rdStg.InitValue(priv.m_pPrivGbl->m_lineInfo, false, 1);
						priv.m_pPrivGbl->m_wrStg.InitValue(priv.m_pPrivGbl->m_lineInfo, false, 1);
						priv.m_pPrivGbl->InitDimen();

						priv.m_pPrivGbl->AddGlobalField(priv.m_type, "data");
						CField & field = priv.m_pPrivGbl->m_fieldList.back();

						field.m_dimenList = priv.m_dimenList;
						field.DimenListInit(priv.m_pPrivGbl->m_lineInfo);
						field.InitDimen();
					}

					rdDst.m_pIntGbl = priv.m_pPrivGbl;
					rdDst.m_varAddr0W = mod.m_threads.m_htIdW.AsInt();
					rdDst.m_varAddr1W = priv.m_addr1W.AsInt();
					rdDst.m_varAddr2W = priv.m_addr2W.AsInt();
					rdDst.m_field = "data";

					CField *pField = &priv.m_pPrivGbl->m_fieldList[0];
					HtlAssert(pField->m_name == rdDst.m_field);

					pField->m_bMifWrite = true;

					rdDst.m_fldDimen1 = pField->GetDimen(0);
					rdDst.m_fldDimen2 = pField->GetDimen(1);

					if (rdDst.m_dstIdx == "varIdx1")
						rdDst.m_dstIdx = "fldIdx1";
					else if (rdDst.m_dstIdx == "varIdx2")
						rdDst.m_dstIdx = "fldIdx2";

					// verify that fieldW + dataLsb <= 64
					rdDst.m_fldW = FindTypeWidth(*pField);

					if (rdDst.m_fldW + rdDst.m_dataLsb.AsInt() > 64)
						ParseMsg(Error, rdDst.m_lineInfo, "sum of field width (%d) and dataLsb (%d) must be less than 64",
						rdDst.m_fldW, rdDst.m_dataLsb.AsInt());

				}

				if (rdDst.m_dstIdx == "varAddr1" && rdDst.m_varAddr1W == 0)
					ParseMsg(Error, rdDst.m_lineInfo, "varAddr1 indexing specified but variable has addr1W == 0");

				if (rdDst.m_dstIdx == "varAddr2" && rdDst.m_varAddr2W == 0)
					ParseMsg(Error, rdDst.m_lineInfo, "varAddr2 indexing specified but variable has addr2W == 0");

				if (rdDst.m_dstIdx == "varIdx1" && rdDst.m_varDimen1 == 0)
					ParseMsg(Error, rdDst.m_lineInfo, "varIdx1 indexing specified but variable has dimen1 == 0");

				if (rdDst.m_dstIdx == "varIdx2" && rdDst.m_varDimen2 == 0)
					ParseMsg(Error, rdDst.m_lineInfo, "varIdx2 indexing specified but variable has dimen2 == 0");

				if (rdDst.m_dstIdx == "fldIdx1" && rdDst.m_fldDimen1 == 0)
					ParseMsg(Error, rdDst.m_lineInfo, "fldIdx1 indexing specified but variable field '%s' has dimen1 == 0",
					rdDst.m_field.c_str());

				if (rdDst.m_dstIdx == "fldIdx2" && rdDst.m_fldDimen2 == 0)
					ParseMsg(Error, rdDst.m_lineInfo, "fldIdx2 indexing specified but variable field '%s' has dimen2 == 0",
					rdDst.m_field.c_str());
			}

			if (mif.m_mifRd.m_bMultiRd && mif.m_mifRd.m_rspCntW.AsInt() < 4)
				ParseMsg(Error, mif.m_mifRd.m_lineInfo, "multi-quadword reads requires rspCntW >= 4");

			if (mif.m_mifRd.m_bMultiRd && mif.m_mifRd.m_queueW.AsInt() < 4)
				ParseMsg(Error, mif.m_mifRd.m_lineInfo, "multi-quadword reads requires queueW >= 4");
		}

		if (mif.m_bMifWr) {

			for (size_t wrSrcIdx = 0; wrSrcIdx < mif.m_mifWr.m_wrSrcList.size(); wrSrcIdx += 1) {
				CMifWrSrc & wrSrc = mif.m_mifWr.m_wrSrcList[wrSrcIdx];

				if (wrSrc.m_bMultiWr)
					mif.m_mifWr.m_bMultiWr = true;

				// first check if rdDst name is unique
				bool bFoundIntGbl = false;
				bool bFoundExtGbl = false;
				bool bFoundShared = false;
				bool bFoundPrivate = false;
				bool bFoundNewGbl = false;
				int ramIdx = 0;

				for (size_t ngvIdx = 0; ngvIdx < mod.m_globalVarList.size(); ngvIdx += 1) {
					CRam * pNgv = mod.m_globalVarList[ngvIdx];

					if (pNgv->m_gblName == wrSrc.m_var) {
						bFoundNewGbl = true;
						ramIdx = ngvIdx;
					}
				}

				for (size_t igvIdx = 0; igvIdx < mod.m_intGblList.size(); igvIdx += 1) {
					CRam &intRam = *mod.m_intGblList[igvIdx];

					if (intRam.m_gblName == wrSrc.m_var && !intRam.m_bPrivGbl) {
						bFoundIntGbl = true;
						ramIdx = igvIdx;
					}
				}

				for (size_t egvIdx = 0; egvIdx < mod.m_extRamList.size(); egvIdx += 1) {
					CRam &extRam = mod.m_extRamList[egvIdx];

					if (extRam.m_gblName == wrSrc.m_var) {
						bFoundExtGbl = true;
						ramIdx = egvIdx;
					}
				}

				for (size_t shIdx = 0; shIdx < mod.m_shared.m_fieldList.size(); shIdx += 1) {
					CField & shared = mod.m_shared.m_fieldList[shIdx];

					if (shared.m_name == wrSrc.m_var) {
						bFoundShared = true;
						ramIdx = shIdx;
					}
				}

				for (size_t prIdx = 0; prIdx < mod.m_threads.m_htPriv.m_fieldList.size(); prIdx += 1) {
					CField & priv = mod.m_threads.m_htPriv.m_fieldList[prIdx];

					if (priv.m_name == wrSrc.m_var) {
						bFoundPrivate = true;
						ramIdx = prIdx;
					}
				}

				if (bFoundNewGbl + bFoundIntGbl + bFoundExtGbl + bFoundShared + bFoundPrivate == 0) {

					ParseMsg(Fatal, wrSrc.m_lineInfo, "unable to find private, shared or global destination variable, '%s'", wrSrc.m_var.c_str());

				} else if (bFoundNewGbl + bFoundIntGbl + bFoundExtGbl + bFoundShared + bFoundPrivate > 1) {
					string msg;
					if (bFoundNewGbl)
						msg += "new global";
					if (bFoundIntGbl || bFoundExtGbl)
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

					ParseMsg(Error, wrSrc.m_lineInfo, "write source name is ambiguous, could be %s variable", msg.c_str());
				}

				if (bFoundIntGbl + bFoundExtGbl + bFoundShared + bFoundPrivate > 1) {
					string msg;
					if (bFoundIntGbl || bFoundExtGbl)
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

					ParseMsg(Error, wrSrc.m_lineInfo, "write source name is ambiguous, could be %s variable", msg.c_str());
				}

				// Find the width of the address for the destination ram - check internal rams
				if (bFoundNewGbl) {
					CRam * pNgv = mod.m_globalVarList[ramIdx];

					wrSrc.m_varAddr0W = 0;
					wrSrc.m_varAddr1W = pNgv->m_addr1W.AsInt();
					wrSrc.m_varAddr2W = pNgv->m_addr2W.AsInt();
					wrSrc.m_varDimen1 = pNgv->GetDimen(0);
					wrSrc.m_varDimen2 = pNgv->GetDimen(1);

					// check if ram has field specified in AddRdDst
					CStructElemIter iter(this, pNgv->m_type);
					for (; !iter.end(); iter++) {
						if (iter.IsStructOrUnion()) continue;

						if (iter.GetHeirFieldName(false) == wrSrc.m_field)
							break;
					}

					if (iter.end()) {
						ParseMsg(Error, wrSrc.m_lineInfo, "field '%s' does not exist for variable '%s'",
							wrSrc.m_field.c_str(), wrSrc.m_var.c_str());

					} else {
						CField & field = iter();

						pNgv->m_bMifRead = true;
						field.m_bMifRead = true;

						wrSrc.m_pField = &field;
						wrSrc.m_pNgvRam = pNgv;
						wrSrc.m_fldDimen1 = field.GetDimen(0);
						wrSrc.m_fldDimen2 = field.GetDimen(1);

						// verify that fieldW + dataLsb <= 64
						wrSrc.m_fldW = FindTypeWidth(field);

						if (wrSrc.m_fldW != 8 && wrSrc.m_fldW != 16 && wrSrc.m_fldW != 32 && wrSrc.m_fldW != 64)
							ParseMsg(Error, wrSrc.m_lineInfo, "field width (%d) must be 8, 16, 32 or 64 bits",
							wrSrc.m_fldW);
					}

				} else if (bFoundIntGbl) {
					CRam &intRam = *mod.m_intGblList[ramIdx];

					wrSrc.m_varAddr0W = 0;
					wrSrc.m_varAddr1W = intRam.m_addr1W.AsInt();
					wrSrc.m_varAddr2W = intRam.m_addr2W.AsInt();
					wrSrc.m_varDimen1 = intRam.GetDimen(0);
					wrSrc.m_varDimen2 = intRam.GetDimen(1);

					// find field
					size_t fldIdx;
					for (fldIdx = 0; fldIdx < intRam.m_fieldList.size(); fldIdx += 1) {
						CField & field = intRam.m_fieldList[fldIdx];

						if (field.m_name == wrSrc.m_field)
							break;
					}

					if (fldIdx == intRam.m_fieldList.size())
						ParseMsg(Error, wrSrc.m_lineInfo, "field '%s' does not exist for variable '%s'",
						wrSrc.m_field.c_str(), wrSrc.m_var.c_str());

					else {
						CField & field = intRam.m_fieldList[fldIdx];

						intRam.m_bMifRead = true;
						field.m_bMifRead = true;

						wrSrc.m_pField = &field;
						wrSrc.m_pIntGbl = &intRam;
						wrSrc.m_fldDimen1 = field.GetDimen(0);
						wrSrc.m_fldDimen2 = field.GetDimen(1);

						// verify that fieldW + dataLsb <= 64
						wrSrc.m_fldW = FindTypeWidth(field);

						if (wrSrc.m_fldW != 64)
							ParseMsg(Error, wrSrc.m_lineInfo, "field width (%d) must be 64 bits",
							wrSrc.m_fldW);
					}

				} else if (bFoundExtGbl) {

					// Find the width of the address for the destination ram - check external rams
					CRam &extRam = mod.m_extRamList[ramIdx];

					wrSrc.m_varAddr0W = 0;
					wrSrc.m_varAddr1W = extRam.m_addr1W.AsInt();
					wrSrc.m_varAddr2W = extRam.m_addr2W.AsInt();
					wrSrc.m_varDimen1 = extRam.GetDimen(0);
					wrSrc.m_varDimen2 = extRam.GetDimen(1);

					// check if ram has field specified in AddRdDst
					size_t fldIdx;
					for (fldIdx = 0; fldIdx < extRam.m_fieldList.size(); fldIdx += 1) {
						CField & field = extRam.m_fieldList[fldIdx];

						if (field.m_name == wrSrc.m_field)
							break;
					}

					if (fldIdx == extRam.m_fieldList.size())
						ParseMsg(Error, wrSrc.m_lineInfo, "field '%s' does not exist for variable '%s'",
						wrSrc.m_field.c_str(), wrSrc.m_var.c_str());

					else {
						CField & field = extRam.m_fieldList[fldIdx];

						extRam.m_bMifRead = true;
						field.m_bMifRead = true;

						wrSrc.m_pExtRam = &extRam;
						wrSrc.m_fldDimen1 = field.GetDimen(0);
						wrSrc.m_fldDimen2 = field.GetDimen(1);

						// verify that fieldW + dataLsb <= 64
						wrSrc.m_fldW = FindTypeWidth(field);

						if (wrSrc.m_fldW != 64)
							ParseMsg(Error, wrSrc.m_lineInfo, "field width (%d) must be 64 bits",
							wrSrc.m_fldW);
					}
						
					ParseMsg(Error, wrSrc.m_lineInfo, "multi-quadword write to global variable is not supported");
					continue;

				} if (bFoundShared) {
					CField & shared = mod.m_shared.m_fieldList[ramIdx];

					// found a shared variable
					wrSrc.m_pSharedVar = &shared;
					wrSrc.m_varAddr1W = shared.m_addr1W.AsInt();
					wrSrc.m_varAddr2W = shared.m_addr2W.AsInt();
					wrSrc.m_varDimen1 = shared.GetDimen(0);
					wrSrc.m_varDimen2 = shared.GetDimen(1);

					if (wrSrc.m_field.size() == 0)
						wrSrc.m_fldW = FindTypeWidth(shared);

					else {
						// check if shared variable has field specified in AddDst
						bool bCStyle;
						CField const * pBaseField, *pLastField;
						bool bFldFound = FindFieldInStruct(shared.m_lineInfo, shared.m_type, wrSrc.m_field, false,
							bCStyle, pBaseField, pLastField);

						if (!bCStyle)
							wrSrc.m_field = "m_" + wrSrc.m_field;

						if (bFldFound) {
							wrSrc.m_fldDimen1 = pBaseField->GetDimen(0);
							wrSrc.m_fldDimen2 = pBaseField->GetDimen(1);

							// verify that fieldW + dataLsb <= 64
							wrSrc.m_fldW = FindTypeWidth(*pLastField);

							if (wrSrc.m_fldW != 64)
								ParseMsg(Error, wrSrc.m_lineInfo, "field width (%d) must be 64 bits",
								wrSrc.m_fldW);

						} else
							ParseMsg(Error, wrSrc.m_lineInfo, "field '%s' does not exist for shared variable '%s'",
							wrSrc.m_field.c_str(), wrSrc.m_var.c_str());
					}

					if (wrSrc.m_bMultiWr && shared.m_queueW.AsInt() == 0 && wrSrc.m_srcIdx.size() == 0)
						ParseMsg(Error, "Multi-quadword writes require a srcIdx to be specified");

					if (wrSrc.m_varAddr1W > 0 && wrSrc.m_srcIdx.substr(0, 3) == "fld")
						ParseMsg(Error, wrSrc.m_lineInfo, "AddSrc field indexing on a shared variable with depth is not supported");

				} else if (bFoundPrivate) {

					CField & priv = mod.m_threads.m_htPriv.m_fieldList[ramIdx];

					if (mod.m_clkRate == eClk2x)
						ParseMsg(Fatal, wrSrc.m_lineInfo, "private variables with memory read or write within clk2x module not supported");

					// found a private variable
					if (priv.m_pPrivGbl == 0) {
						// create a global variable
						string rdStg = "1";
						string wrStg = mod.m_stage.m_execStg.AsStr();
						string nullStr;

						priv.m_pPrivGbl = &mod.AddIntRam(priv.m_name, priv.m_addr1Name, priv.m_addr2Name,
							priv.m_addr1W.AsStr(), priv.m_addr2W.AsStr(), nullStr,
							nullStr, rdStg, wrStg);
						priv.m_pPrivGbl->m_addr0W = mod.m_threads.m_htIdW;

						priv.m_pPrivGbl->m_bPrivGbl = true;
						priv.m_pPrivGbl->m_addr0W.InitValue(priv.m_pPrivGbl->m_lineInfo, false, 0);
						priv.m_pPrivGbl->m_addr1W.InitValue(priv.m_pPrivGbl->m_lineInfo, false, 0);
						priv.m_pPrivGbl->m_addr2W.InitValue(priv.m_pPrivGbl->m_lineInfo, false, 0);
						priv.m_pPrivGbl->DimenListInit(priv.m_pPrivGbl->m_lineInfo);
						priv.m_pPrivGbl->m_rdStg.InitValue(priv.m_pPrivGbl->m_lineInfo, false, 1);
						priv.m_pPrivGbl->m_wrStg.InitValue(priv.m_pPrivGbl->m_lineInfo, false, 1);
						priv.m_pPrivGbl->InitDimen();

						priv.m_pPrivGbl->AddGlobalField(priv.m_type, "data");
						CField & field = priv.m_pPrivGbl->m_fieldList.back();
						field.m_gblFieldIdx = 0;
						field.m_dimenList = priv.m_dimenList;
						field.DimenListInit(priv.m_pPrivGbl->m_lineInfo);
						field.InitDimen();
					}

					wrSrc.m_pIntGbl = priv.m_pPrivGbl;
					wrSrc.m_varAddr0W = mod.m_threads.m_htIdW.AsInt();
					wrSrc.m_varAddr1W = priv.m_addr1W.AsInt();
					wrSrc.m_varAddr2W = priv.m_addr2W.AsInt();
					wrSrc.m_field = "data";

					CField *pField = &priv.m_pPrivGbl->m_fieldList[0];
					HtlAssert(pField->m_name == wrSrc.m_field);

					pField->m_bMifRead = true;

					wrSrc.m_pField = pField;
					wrSrc.m_fldDimen1 = pField->GetDimen(0);
					wrSrc.m_fldDimen2 = pField->GetDimen(1);

					if (wrSrc.m_srcIdx == "varIdx1")
						wrSrc.m_srcIdx = "fldIdx1";
					else if (wrSrc.m_srcIdx == "varIdx2")
						wrSrc.m_srcIdx = "fldIdx2";

					// verify that fieldW + dataLsb <= 64
					wrSrc.m_fldW = FindTypeWidth(*pField);

					if (wrSrc.m_fldW != 64)
						ParseMsg(Error, wrSrc.m_lineInfo, "field width (%d) must be 64 bits",
						wrSrc.m_fldW);
				}

				if (wrSrc.m_srcIdx == "varAddr1" && wrSrc.m_varAddr1W == 0)
					ParseMsg(Error, wrSrc.m_lineInfo, "varAddr1 indexing specified but variable has addr1W == 0");

				if (wrSrc.m_srcIdx == "varAddr2" && wrSrc.m_varAddr2W == 0)
					ParseMsg(Error, wrSrc.m_lineInfo, "varAddr2 indexing specified but variable has addr2W == 0");

				if (wrSrc.m_srcIdx == "varIdx1" && wrSrc.m_varDimen1 == 0)
					ParseMsg(Error, wrSrc.m_lineInfo, "varIdx1 indexing specified but variable has dimen1 == 0");

				if (wrSrc.m_srcIdx == "varIdx2" && wrSrc.m_varDimen2 == 0)
					ParseMsg(Error, wrSrc.m_lineInfo, "varIdx2 indexing specified but variable has dimen2 == 0");

				if (wrSrc.m_srcIdx == "fldIdx1" && wrSrc.m_fldDimen1 == 0)
					ParseMsg(Error, wrSrc.m_lineInfo, "fldIdx1 indexing specified but variable field '%s' has dimen1 == 0",
					wrSrc.m_field.c_str());

				if (wrSrc.m_srcIdx == "fldIdx2" && wrSrc.m_fldDimen2 == 0)
					ParseMsg(Error, wrSrc.m_lineInfo, "fldIdx2 indexing specified but variable field '%s' has dimen2 == 0",
					wrSrc.m_field.c_str());
			}

			if (mif.m_mifWr.m_bMultiWr && mif.m_mifWr.m_rspCntW.AsInt() < 4)
				ParseMsg(Error, mif.m_mifWr.m_lineInfo, "multi-quadword write requires rspCntW >= 4");

			if (mif.m_mifWr.m_bMultiWr && mif.m_mifWr.m_queueW.AsInt() < 4)
				ParseMsg(Error, mif.m_mifWr.m_lineInfo, "multi-quadword write requires queueW >= 4");
		}

		mod.m_memPortList[0]->m_queueW = mif.m_queueW;
		mod.m_memPortList[0]->m_bMultiRd |= mif.m_mifRd.m_bMultiRd;
		mod.m_memPortList[0]->m_bMultiWr |= mif.m_mifWr.m_bMultiWr;
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
			(int)(m_mifInstList.size() * g_appArgs.GetAeUnitCnt()) );
		ParseMsg(Info, "   required memory interface ports is product of Unit count (%d) and memory ports per Unit (%d)",
			(int)g_appArgs.GetAeUnitCnt(), (int)m_mifInstList.size());
	}
}

void CDsnInfo::GenModMifStatements(CModule &mod)
{
	if (!mod.m_mif.m_bMif)
		return;

	g_appArgs.GetDsnRpt().AddLevel("Memory Interface\n");

	string vcdModName = VA("Pers%s", mod.m_modName.Uc().c_str());

	if (mod.m_mif.m_bMif) {
		CMif &mif = mod.m_mif;

		m_mifIoDecl.Append("\t// Memory Interface\n");
		GenModDecl(eVcdUser, m_mifIoDecl, vcdModName, "sc_out<bool>", VA("o_%sP0ToMif_reqRdy", mod.m_modName.Lc().c_str()));
		GenModDecl(eVcdAll, m_mifIoDecl, vcdModName, "sc_out<CMemRdWrReqIntf>", VA("o_%sP0ToMif_req", mod.m_modName.Lc().c_str()));

		m_mifIoDecl.Append("\tsc_in<bool> i_mifTo%sP0_reqAvl;\n", mod.m_modName.Uc().c_str());
		m_mifIoDecl.Append("\n");

		if (mif.m_bMifRd) {
			GenModDecl(eVcdUser, m_mifIoDecl, vcdModName, "sc_in<bool>", VA("i_mifTo%sP0_rdRspRdy", mod.m_modName.Uc().c_str()));
			GenModDecl(eVcdUser, m_mifIoDecl, vcdModName, "sc_in<CMemRdRspIntf>", VA("i_mifTo%sP0_rdRsp", mod.m_modName.Uc().c_str()));
			m_mifIoDecl.Append("\n");
		}

		if (mif.m_bMifWr) {
			GenModDecl(eVcdUser, m_mifIoDecl, vcdModName, "sc_in<bool>", VA("i_mifTo%sP0_wrRspRdy", mod.m_modName.Uc().c_str()));
			if (mod.m_threads.m_htIdW.AsInt() == 0)
				GenModDecl(eVcdAll, m_mifIoDecl, vcdModName, "sc_in<sc_uint<MIF_TID_W> > ht_noload", VA("i_mifTo%sP0_wrRspTid", mod.m_modName.Uc().c_str()));
			else
				GenModDecl(eVcdAll, m_mifIoDecl, vcdModName, "sc_in<sc_uint<MIF_TID_W> >", VA("i_mifTo%sP0_wrRspTid", mod.m_modName.Uc().c_str()));
			m_mifIoDecl.Append("\n");
		}
	}

	string unitNameUc = !g_appArgs.IsModuleUnitNamesEnabled() ? m_unitName.Uc() : "";

	CHtCode	& mifPreInstr = mod.m_clkRate == eClk1x ? m_mifPreInstr1x : m_mifPreInstr2x;
	CHtCode	& mifPostInstr = mod.m_clkRate == eClk1x ? m_mifPostInstr1x : m_mifPostInstr2x;
	CHtCode	& mifRamClock = mod.m_clkRate == eClk1x ? m_mifRamClock1x : m_mifRamClock2x;
	CHtCode	& mifReg = mod.m_clkRate == eClk1x ? m_mifReg1x : m_mifReg2x;
	CHtCode	& mifPostReg = mod.m_clkRate == eClk1x ? m_mifPostReg1x : m_mifPostReg2x;
	CHtCode	& mifOut = mod.m_clkRate == eClk1x ? m_mifOut1x : m_mifOut2x;

	CMif & mif = mod.m_mif;
	int rdGrpBankCnt = mif.m_mifRd.m_bMaxBw ? 4 : 2;
	int wrGrpBankCnt = mif.m_mifWr.m_bMaxBw ? 4 : 2;

	bool is_hc1 = g_appArgs.GetCoproc() == hc1 ||
		g_appArgs.GetCoproc() == hc1ex;
	bool is_hc2 = g_appArgs.GetCoproc() == hc2 ||
		g_appArgs.GetCoproc() == hc2ex;
	bool is_wx = g_appArgs.GetCoproc() == wx690 ||
		g_appArgs.GetCoproc() == wx2k;

	mif.m_bSingleReqMode = (!mif.m_bMifRd || mif.m_mifRd.m_rspCntW.AsInt() == 0)
		&& (!mif.m_bMifWr || mif.m_mifWr.m_rspCntW.AsInt() == 0);

	bool bNeed_reqQwCnt = mod.m_mif.m_mifWr.m_bMultiWr || mod.m_mif.m_mifRd.m_bMultiRd;

	bool bNeed_reqQwIdx = mod.m_mif.m_mifWr.m_bMultiWr
		|| !is_wx && mod.m_mif.m_mifRd.m_bMultiRd;

	bool bNeed_reqQwSplit = is_hc2
		&& (mod.m_mif.m_mifWr.m_bMultiWr || mod.m_mif.m_mifRd.m_bMultiRd);

	bool bNeed_reqBusy = mod.m_mif.m_mifWr.m_bMultiWr
		|| !is_wx && mod.m_mif.m_mifRd.m_bMultiRd;

	bool bNeed_rdRspQwIdx = mod.m_mif.m_mifRd.m_bMultiRd && (is_hc2 || is_wx);

	// thread resume logic
	m_mifDefines.Append("#define %s_MIF_TYPE_W 2\n\n", mod.m_modName.Upper().c_str());

	int dstIdW = mif.m_mifRd.m_rdDstList.size() <= 1 ? 0 : FindLg2(mif.m_mifRd.m_rdDstList.size()-1);

	mif.m_maxDstW = 0;
	for (size_t dstIdx = 0; dstIdx < mif.m_mifRd.m_rdDstList.size(); dstIdx += 1) {
		CMifRdDst & rdDst = mif.m_mifRd.m_rdDstList[dstIdx];

		// tid format:
		//   dstId - least significant bits
		//   fIdx
		//   lIdx
		//   varAddr1
		//   varAddr2
		//   varIdx1
		//   varIdx2
		//   fldIdx1
		//   fldIdx2 - most significant bits of rdDst
		//   qwCntM1 - <28:26>

		int multiRdW = rdDst.m_bMultiRd ? 6 : 0;

		if (rdDst.m_pIntGbl && rdDst.m_pIntGbl->m_bPrivGbl) {
			m_mifDefines.Append("#define %s_MIF_DST_%s_VAR_ADDR0_W %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), rdDst.m_varAddr0W);
			m_mifDefines.Append("#define %s_MIF_DST_%s_VAR_ADDR0_SHF %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), dstIdW + multiRdW);
			m_mifDefines.Append("#define %s_MIF_DST_%s_VAR_ADDR0_MSK 0x%x\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), (1ul << rdDst.m_varAddr0W)-1);
		}

		if (rdDst.m_varAddr1W > 0) {
			m_mifDefines.Append("#define %s_MIF_DST_%s_VAR_ADDR1_W %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), rdDst.m_varAddr1W);
			m_mifDefines.Append("#define %s_MIF_DST_%s_VAR_ADDR1_SHF %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), dstIdW + multiRdW + rdDst.m_varAddr0W);
			m_mifDefines.Append("#define %s_MIF_DST_%s_VAR_ADDR1_MSK 0x%x\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), (1ul << rdDst.m_varAddr1W)-1);
		}

		if (rdDst.m_varAddr2W > 0) {
			m_mifDefines.Append("#define %s_MIF_DST_%s_VAR_ADDR2_W %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), rdDst.m_varAddr2W);
			m_mifDefines.Append("#define %s_MIF_DST_%s_VAR_ADDR2_SHF %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), dstIdW + multiRdW + rdDst.m_varAddr0W + rdDst.m_varAddr1W);
			m_mifDefines.Append("#define %s_MIF_DST_%s_VAR_ADDR2_MSK 0x%x\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), (1ul << rdDst.m_varAddr2W)-1);
		}

		int varDimen1W = 0;
		if (rdDst.m_varDimen1 > 0) {
			varDimen1W = FindLg2(rdDst.m_varDimen1-1);
			m_mifDefines.Append("#define %s_MIF_DST_%s_VAR_IDX1_W %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), varDimen1W);
			m_mifDefines.Append("#define %s_MIF_DST_%s_VAR_IDX1_SHF %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), dstIdW + multiRdW + rdDst.m_varAddr0W + rdDst.m_varAddr1W + rdDst.m_varAddr2W);
			m_mifDefines.Append("#define %s_MIF_DST_%s_VAR_IDX1_MSK 0x%x\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), (1ul << varDimen1W)-1);
		}

		int varDimen2W = 0;
		if (rdDst.m_varDimen2 > 0) {
			varDimen2W = FindLg2(rdDst.m_varDimen2-1);
			m_mifDefines.Append("#define %s_MIF_DST_%s_VAR_IDX2_W %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), varDimen2W);
			m_mifDefines.Append("#define %s_MIF_DST_%s_VAR_IDX2_SHF %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(),
				dstIdW + multiRdW + rdDst.m_varAddr0W + rdDst.m_varAddr1W + rdDst.m_varAddr2W + varDimen1W);
			m_mifDefines.Append("#define %s_MIF_DST_%s_VAR_IDX2_MSK 0x%x\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), (1ul << varDimen2W)-1);
		}

		int fldDimen1W = 0;
		if (rdDst.m_fldDimen1 > 0) {
			fldDimen1W = FindLg2(rdDst.m_fldDimen1-1);
			m_mifDefines.Append("#define %s_MIF_DST_%s_FLD_IDX1_W %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), fldDimen1W);
			m_mifDefines.Append("#define %s_MIF_DST_%s_FLD_IDX1_SHF %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(),
				dstIdW + multiRdW + rdDst.m_varAddr0W + rdDst.m_varAddr1W + rdDst.m_varAddr2W + varDimen1W + varDimen2W);
			m_mifDefines.Append("#define %s_MIF_DST_%s_FLD_IDX1_MSK 0x%x\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), (1ul << fldDimen1W)-1);
		}

		int fldDimen2W = 0;
		if (rdDst.m_fldDimen2 > 0) {
			fldDimen2W = FindLg2(rdDst.m_fldDimen2-1);
			m_mifDefines.Append("#define %s_MIF_DST_%s_FLD_IDX2_W %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), fldDimen2W);
			m_mifDefines.Append("#define %s_MIF_DST_%s_FLD_IDX2_SHF %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(),
				dstIdW + multiRdW + rdDst.m_varAddr0W + rdDst.m_varAddr1W + rdDst.m_varAddr2W + varDimen1W + varDimen2W + fldDimen1W);
			m_mifDefines.Append("#define %s_MIF_DST_%s_FLD_IDX2_MSK 0x%x\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), (1ul << fldDimen2W)-1);
		}

		int infoW = rdDst.m_infoW.AsInt();
		if (infoW > 0) {
			m_mifDefines.Append("#define %s_MIF_DST_%s_INFO_W %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), infoW);
			m_mifDefines.Append("#define %s_MIF_DST_%s_INFO_SHF %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), dstIdW + multiRdW);
			m_mifDefines.Append("#define %s_MIF_DST_%s_INFO_MSK 0x%x\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), (1ul << infoW)-1);
		}

		if (infoW > 0 && rdDst.m_bMultiRd && !is_wx) {
			m_mifDefines.Append("#define %s_MIF_DST_%s_RSP_IDX_W 3\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str());
			m_mifDefines.Append("#define %s_MIF_DST_%s_RSP_IDX_SHF %d\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(), dstIdW + multiRdW + infoW);
			m_mifDefines.Append("#define %s_MIF_DST_%s_RSP_IDX_MSK 0x7\n",
				mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str());
		}

		rdDst.m_dstW = dstIdW + multiRdW + rdDst.m_varAddr0W + rdDst.m_varAddr1W + rdDst.m_varAddr2W + varDimen1W + varDimen2W + fldDimen1W + fldDimen2W + infoW;
		mif.m_maxDstW = max(mif.m_maxDstW, rdDst.m_dstW);

		if (rdDst.m_dstW > 25)
			ParseMsg(Error, "ReadMem response info field exceeds 25 bits, '%s'\n", rdDst.m_name.c_str());
	}

	m_mifDefines.Append("#define %s_MIF_DST_IDX_W %d\n",
		mod.m_modName.Upper().c_str(), dstIdW);
	m_mifDefines.Append("#define %s_MIF_DST_IDX_SHF %d\n",
		mod.m_modName.Upper().c_str(), 0);
	m_mifDefines.Append("#define %s_MIF_DST_IDX_MSK 0x%x\n",
		mod.m_modName.Upper().c_str(), (1ul << dstIdW) - 1);

	if (mif.m_mifRd.m_bMultiRd && is_wx) {
		m_mifDefines.Append("#define %s_MIF_DST_QW_FIRST_W 3\n", mod.m_modName.Upper().c_str());
		m_mifDefines.Append("#define %s_MIF_DST_QW_FIRST_SHF %d\n", mod.m_modName.Upper().c_str(), dstIdW);
		m_mifDefines.Append("#define %s_MIF_DST_QW_FIRST_MSK 0x7\n", mod.m_modName.Upper().c_str());
		m_mifDefines.Append("#define %s_MIF_DST_QW_LAST_W 3\n", mod.m_modName.Upper().c_str());
		m_mifDefines.Append("#define %s_MIF_DST_QW_LAST_SHF %d\n", mod.m_modName.Upper().c_str(), dstIdW+3);
		m_mifDefines.Append("#define %s_MIF_DST_QW_LAST_MSK 0x7\n", mod.m_modName.Upper().c_str());
	}

	m_mifDefines.Append("#define %s_MIF_DST_W %d\n\n",
		mod.m_modName.Upper().c_str(), mif.m_maxDstW);

	int srcIdW = mif.m_mifWr.m_wrSrcList.size()+1 <= 1 ? 0 : FindLg2(mif.m_mifWr.m_wrSrcList.size());

	mif.m_maxSrcW = 0;
	for (size_t srcIdx = 0; srcIdx < mif.m_mifWr.m_wrSrcList.size(); srcIdx += 1) {
		CMifWrSrc & wrSrc = mif.m_mifWr.m_wrSrcList[srcIdx];

		// tid format:
		//   srcId - least significant bits
		//   fIdx
		//   lIdx
		//   varAddr1
		//   varAddr2
		//   varIdx1
		//   varIdx2
		//   fldIdx1
		//   fldIdx2 - most significant bits of wrSrc
		//   qwCntM1 - <28:26>

		int multiWrW = /*wrSrc.m_bMultiWr ? 6 :*/ 0;

		if (wrSrc.m_pIntGbl && wrSrc.m_pIntGbl->m_bPrivGbl) {
			m_mifDefines.Append("#define %s_MIF_SRC_%s_VAR_ADDR0_W %d\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), wrSrc.m_varAddr0W);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_VAR_ADDR0_SHF %d\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), srcIdW + multiWrW);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_VAR_ADDR0_MSK 0x%x\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), (1ul << wrSrc.m_varAddr0W)-1);
		}

		if (wrSrc.m_varAddr1W > 0) {
			m_mifDefines.Append("#define %s_MIF_SRC_%s_VAR_ADDR1_W %d\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), wrSrc.m_varAddr1W);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_VAR_ADDR1_SHF %d\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), srcIdW + multiWrW + wrSrc.m_varAddr0W);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_VAR_ADDR1_MSK 0x%x\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), (1ul << wrSrc.m_varAddr1W)-1);
		}

		if (wrSrc.m_varAddr2W > 0) {
			m_mifDefines.Append("#define %s_MIF_SRC_%s_VAR_ADDR2_W %d\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), wrSrc.m_varAddr2W);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_VAR_ADDR2_SHF %d\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), srcIdW + multiWrW + wrSrc.m_varAddr0W + wrSrc.m_varAddr1W);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_VAR_ADDR2_MSK 0x%x\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), (1ul << wrSrc.m_varAddr2W)-1);
		}

		int varDimen1W = 0;
		if (wrSrc.m_varDimen1 > 0) {
			varDimen1W = FindLg2(wrSrc.m_varDimen1-1);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_VAR_IDX1_W %d\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), varDimen1W);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_VAR_IDX1_SHF %d\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), srcIdW + multiWrW + wrSrc.m_varAddr0W + wrSrc.m_varAddr1W + wrSrc.m_varAddr2W);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_VAR_IDX1_MSK 0x%x\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), (1ul << varDimen1W)-1);
		}

		int varDimen2W = 0;
		if (wrSrc.m_varDimen2 > 0) {
			varDimen2W = FindLg2(wrSrc.m_varDimen2-1);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_VAR_IDX2_W %d\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), varDimen2W);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_VAR_IDX2_SHF %d\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(),
				srcIdW + multiWrW + wrSrc.m_varAddr0W + wrSrc.m_varAddr1W + wrSrc.m_varAddr2W + varDimen1W);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_VAR_IDX2_MSK 0x%x\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), (1ul << varDimen2W)-1);
		}

		int fldDimen1W = 0;
		if (wrSrc.m_fldDimen1 > 0) {
			fldDimen1W = FindLg2(wrSrc.m_fldDimen1-1);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_FLD_IDX1_W %d\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), fldDimen1W);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_FLD_IDX1_SHF %d\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(),
				srcIdW + multiWrW + wrSrc.m_varAddr0W + wrSrc.m_varAddr1W + wrSrc.m_varAddr2W + varDimen1W + varDimen2W);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_FLD_IDX1_MSK 0x%x\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), (1ul << fldDimen1W)-1);
		}

		int fldDimen2W = 0;
		if (wrSrc.m_fldDimen2 > 0) {
			fldDimen2W = FindLg2(wrSrc.m_fldDimen2-1);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_FLD_IDX2_W %d\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), fldDimen2W);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_FLD_IDX2_SHF %d\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(),
				srcIdW + multiWrW + wrSrc.m_varAddr0W + wrSrc.m_varAddr1W + wrSrc.m_varAddr2W + varDimen1W + varDimen2W + fldDimen1W);
			m_mifDefines.Append("#define %s_MIF_SRC_%s_FLD_IDX2_MSK 0x%x\n",
				mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(), (1ul << fldDimen2W)-1);
		}

		wrSrc.m_srcW = srcIdW + multiWrW + wrSrc.m_varAddr0W + wrSrc.m_varAddr1W + wrSrc.m_varAddr2W + varDimen1W + varDimen2W + fldDimen1W + fldDimen2W;
		mif.m_maxSrcW = max(mif.m_maxSrcW, wrSrc.m_srcW);

		if (wrSrc.m_srcW > 25)
			ParseMsg(Error, "WriteMem response info field exceeds 25 bits, '%s'\n", wrSrc.m_name.c_str());
	}

	m_mifDefines.Append("#define %s_MIF_SRC_IDX_W %d\n",
		mod.m_modName.Upper().c_str(), srcIdW);
	m_mifDefines.Append("#define %s_MIF_SRC_IDX_SHF %d\n",
		mod.m_modName.Upper().c_str(), 0);
	m_mifDefines.Append("#define %s_MIF_SRC_IDX_MSK 0x%x\n",
		mod.m_modName.Upper().c_str(), (1ul << srcIdW) - 1);

	m_mifDefines.Append("#define %s_MIF_SRC_W %d\n\n",
		mod.m_modName.Upper().c_str(), mif.m_maxSrcW);

	if (mif.m_bMifRd && mif.m_mifRd.m_rspCntW.AsInt() > 0) {
		m_mifDefines.Append("#define %s_MIF_RLEN_W %d\n", mod.m_modName.Upper().c_str(), mif.m_mifRd.m_rspCntW.AsInt());
		m_mifDefines.Append("#define %s_MIF_RLEN (1 << %s_MIF_RLEN_W)\n",
			mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());
	}

	if (mif.m_bMifWr && mif.m_mifWr.m_rspCntW.AsInt() > 0) {
		m_mifDefines.Append("#define %s_MIF_WLEN_W %d\n", mod.m_modName.Upper().c_str(), mif.m_mifWr.m_rspCntW.AsInt());
		m_mifDefines.Append("#define %s_MIF_WLEN (1 << %s_MIF_WLEN_W)\n",
			mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());
		m_mifDefines.NewLine();
	}

	if (mif.m_bMifRd) {
		m_mifDefines.Append("#define %s_RD_RSP_CNT_W %d\n",
			mod.m_modName.Upper().c_str(), mif.m_mifRd.m_rspCntW.AsInt());

		if (mif.m_mifRd.m_rspGrpId.size() == 0)
			m_mifDefines.Append("#define %s_RD_GRP_ID_W %s_HTID_W\n",
			mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());
		else
			m_mifDefines.Append("#define %s_RD_GRP_ID_W %d\n",
			mod.m_modName.Upper().c_str(), mif.m_mifRd.m_rspGrpIdW);
	}

	if (mif.m_bMifWr) {
		m_mifDefines.Append("#define %s_WR_RSP_CNT_W %d\n",
			mod.m_modName.Upper().c_str(), mif.m_mifWr.m_rspCntW.AsInt());

		if (mif.m_mifWr.m_rspGrpId.size() == 0)
			m_mifDefines.Append("#define %s_WR_GRP_ID_W %s_HTID_W\n",
			mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());
		else
			m_mifDefines.Append("#define %s_WR_GRP_ID_W %d\n",
			mod.m_modName.Upper().c_str(), mif.m_mifWr.m_rspGrpIdW);
	}

	m_mifDefines.NewLine();

	const char *pRdGrpIdMask = (mif.m_mifRd.m_rspGrpIdW == 2 || mif.m_mifRd.m_bMaxBw) ? "3" : "1";
	const char *pRdGrpShfAmt = (mif.m_mifRd.m_rspGrpIdW == 2 || mif.m_mifRd.m_bMaxBw) ? "2" : "1";

	char rdRspGrpName[32];
	if (mif.m_bMifRd) {
		m_mifMacros.Append("// Memory Read Interface Routines\n");

		if (mif.m_mifRd.m_rspGrpId.size() == 0)
			sprintf(rdRspGrpName, "r_t%d_htId", mod.m_execStg);
		else if (mif.m_mifRd.m_bRspGrpIdPriv)
			sprintf(rdRspGrpName, "r_t%d_htPriv.m_%s",
			mod.m_execStg, mif.m_mifRd.m_rspGrpId.AsStr().c_str());
		else
			sprintf(rdRspGrpName, "r_%s", mif.m_mifRd.m_rspGrpId.AsStr().c_str());

		///////////////////////////////////////////////////////////
		// generate ReadMemBusy routine

		g_appArgs.GetDsnRpt().AddLevel("bool ReadMemBusy()\n");
		g_appArgs.GetDsnRpt().EndLevel();

		m_mifFuncDecl.Append("\tbool ReadMemBusy();\n");
		m_mifMacros.Append("bool CPers%s%s::ReadMemBusy()\n",
			unitNameUc.c_str(), mod.m_modName.Uc().c_str());
		m_mifMacros.Append("{\n");

		char reqBusy[32];
		if (bNeed_reqBusy)
			sprintf(reqBusy, " || r_t%d_reqBusy", mod.m_execStg);
		else
			reqBusy[0] = '\0';

		if (mif.m_mifRd.m_rspGrpIdW <= 2) { // using registers

			mifPostReg.Append("#\tifdef _HTV\n");
			mifPostReg.Append("\tc_t%d_bReadMemBusy = r_%sToMif_reqAvlCntBusy || r_t%d_rdRspBusyCntMax%s;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str(), mod.m_execStg, reqBusy);

			mifPostReg.Append("#\telse\n");

			mifPostReg.Append("\tc_t%d_bReadMemBusy = r_%sToMif_reqAvlCntBusy || r_t%d_rdRspBusyCntMax%s ||\n",
				mod.m_execStg, mod.m_modName.Lc().c_str(), mod.m_execStg, reqBusy);

		} else {

			mifPostReg.Append("#\tifdef _HTV\n");
			mifPostReg.Append("\tc_t%d_bReadMemBusy = r_t%d_bReadRspBusy || r_%sToMif_reqAvlCntBusy || r_t%d_rdRspBusyCntMax%s;\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), mod.m_execStg, reqBusy);

			mifPostReg.Append("#\telse\n");

			mifPostReg.Append("\tc_t%d_bReadMemBusy = r_t%d_bReadRspBusy || r_%sToMif_reqAvlCntBusy || r_t%d_rdRspBusyCntMax%s ||\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), mod.m_execStg, reqBusy);
		}

		mifPostReg.Append("\t\t(%s_RND_RETRY && !!(g_rndRetry() & 1));\n", mod.m_modName.Upper().c_str());
		mifPostReg.Append("#\tendif\n");

		m_mifMacros.Append("\tc_t%d_bReadMemAvail = !c_t%d_bReadMemBusy;\n", mod.m_execStg, mod.m_execStg);
		m_mifMacros.Append("\treturn c_t%d_bReadMemBusy;\n", mod.m_execStg);
		m_mifMacros.Append("}\n");
		m_mifMacros.NewLine();

		///////////////////////////////////////////////////////////
		// generate ReadMemPoll routine

		if (mif.m_mifRd.m_bPoll) {

			string paramStr;
			if (mif.m_mifRd.m_rspGrpId.size() > 0)
				paramStr = VA("sc_uint<%s_RD_GRP_ID_W> rdRspGrpId", mod.m_modName.Upper().c_str());

			g_appArgs.GetDsnRpt().AddLevel("bool ReadMemPoll(%s)\n", paramStr.c_str());
			g_appArgs.GetDsnRpt().EndLevel();

			m_mifFuncDecl.Append("\tbool ReadMemPoll(%s);\n", paramStr.c_str());
			m_mifMacros.Append("bool CPers%s%s::ReadMemPoll(%s)\n",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(), paramStr.c_str());
			m_mifMacros.Append("{\n");

			if (mif.m_mifRd.m_rspGrpId.size() == 0 || mif.m_mifRd.m_rspGrpIdW == 0) {
				m_mifMacros.Append("\tassert_msg(c_t%d_bReadMemAvail, \"Runtime check failed in CPers%s::ReadMemPoll()"
					" - expected ReadMemBusy() to have been called and not busy\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());
				m_mifMacros.NewLine();

				m_mifMacros.Append("\treturn !r_t%d_rdRspPollCntZero;\n", mod.m_execStg);

			} else if (mif.m_mifRd.m_rspGrpIdW <= 2) {

				m_mifMacros.Append("\treturn !r_rdGrpRspCntZero[rdRspGrpId];\n");

			} else {

				const char *pRdGrpIdMask = (mif.m_mifRd.m_rspGrpIdW == 2 || mif.m_mifRd.m_bMaxBw) ? "3" : "1";
				const char *pRdGrpShfAmt = (mif.m_mifRd.m_rspGrpIdW == 2 || mif.m_mifRd.m_bMaxBw) ? "2" : "1";

				m_mifMacros.Append("\tm_rdGrpRspCntZero[rdRspGrpId & %s].read_addr(rdRspGrpId >> %s);\n", pRdGrpIdMask, pRdGrpShfAmt);
				m_mifMacros.Append("\treturn !m_rdGrpRspCntZero[rdRspGrpId & %s].read_mem();\n", pRdGrpIdMask);
			}

			m_mifMacros.Append("}\n");
			m_mifMacros.NewLine();
		}

		///////////////////////////////////////////////////////////
		// generate ReadMem or ReadMem_var routines

		for (size_t dstIdx = 0; dstIdx < mif.m_mifRd.m_rdDstList.size(); dstIdx += 1) {
			CMifRdDst &rdDst = mif.m_mifRd.m_rdDstList[dstIdx];

			char dstName[64] = "";
			if (rdDst.m_name.size() > 0)
				sprintf(dstName, "_%s", rdDst.m_name.c_str());

			if (true || g_appArgs.IsMemTraceEnabled()) {
				m_mifFuncDecl.Append("\t#ifdef _HTV\n");
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

			g_appArgs.GetDsnRpt().AddLevel("void %sMem%s(sc_uint<MEM_ADDR_W> memAddr", pMemOpName, dstName);

			m_mifFuncDecl.Append("\tvoid %sMem%s(sc_uint<MEM_ADDR_W> memAddr",
				pMemOpName, dstName);

			m_mifMacros.Append("void CPers%s%s::%sMem%s(sc_uint<MEM_ADDR_W> memAddr",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(),
				pMemOpName, dstName);

			int argCnt = 1;
			if (rdDst.m_pIntGbl && (rdDst.m_pIntGbl->m_addr1W.size() > 0 || rdDst.m_pIntGbl->m_addr1Name.size() > 0)) {
				argCnt += 1;
				int varAddr1W = rdDst.m_pIntGbl->m_addr1W.AsInt() == 0 ? 1 : rdDst.m_pIntGbl->m_addr1W.AsInt();
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varAddr1", varAddr1W);
				m_mifFuncDecl.Append(", ht_uint%d varAddr1", varAddr1W);
				m_mifMacros.Append(", ht_uint%d %svarAddr1", varAddr1W, rdDst.m_pIntGbl->m_addr1W.AsInt() == 0 ? "ht_noload " : "");

			} else if (rdDst.m_pExtRam && (rdDst.m_pExtRam->m_addr1W.size() > 0 || rdDst.m_pExtRam->m_addr1Name.size() > 0)) {
				argCnt += 1;
				int varAddr1W = rdDst.m_pExtRam->m_addr1W.AsInt() == 0 ? 1 : rdDst.m_pExtRam->m_addr1W.AsInt();
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varAddr1", varAddr1W);
				m_mifFuncDecl.Append(", ht_uint%d varAddr1", varAddr1W);
				m_mifMacros.Append(", ht_uint%d %svarAddr1", varAddr1W, rdDst.m_pExtRam->m_addr1W.AsInt() == 0 ? "ht_noload" : "");

			} else if (rdDst.m_pSharedVar && rdDst.m_pSharedVar->m_addr1W.size() > 0) {
				argCnt += 1;
				int varAddr1W = rdDst.m_pSharedVar->m_addr1W.AsInt() == 0 ? 1 : rdDst.m_pSharedVar->m_addr1W.AsInt();
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varAddr1", varAddr1W);
				m_mifFuncDecl.Append(", ht_uint%d varAddr1", varAddr1W);
				m_mifMacros.Append(", ht_uint%d %svarAddr1", varAddr1W, rdDst.m_pSharedVar->m_addr1W.AsInt() == 0 ? "ht_noload" : "");
			}

			if (rdDst.m_pIntGbl && (rdDst.m_pIntGbl->m_addr2W.size() > 0 || rdDst.m_pIntGbl->m_addr2Name.size() > 0)) {
				argCnt += 1;
				int varAddr2W = rdDst.m_pIntGbl->m_addr2W.AsInt() == 0 ? 1 : rdDst.m_pIntGbl->m_addr2W.AsInt();
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varAddr2", varAddr2W);
				m_mifFuncDecl.Append(", ht_uint%d varAddr2", varAddr2W);
				m_mifMacros.Append(", ht_uint%d %svarAddr2", varAddr2W, rdDst.m_pIntGbl->m_addr2W.AsInt() == 0 ? "ht_noload" : "");

			} else if (rdDst.m_pExtRam && (rdDst.m_pExtRam->m_addr2W.size() > 0 || rdDst.m_pExtRam->m_addr2Name.size() > 0)) {
				argCnt += 1;
				int varAddr2W = rdDst.m_pExtRam->m_addr2W.AsInt() == 0 ? 1 : rdDst.m_pExtRam->m_addr2W.AsInt();
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varAddr2", varAddr2W);
				m_mifFuncDecl.Append(", ht_uint%d varAddr2", varAddr2W);
				m_mifMacros.Append(", ht_uint%d %svarAddr2", varAddr2W, rdDst.m_pExtRam->m_addr2W.AsInt() == 0 ? "ht_noload" : "");

			} else if (rdDst.m_pSharedVar && rdDst.m_pSharedVar->m_addr2W.size() > 0) {
				argCnt += 1;
				int varAddr2W = rdDst.m_pSharedVar->m_addr2W.AsInt() == 0 ? 1 : rdDst.m_pSharedVar->m_addr2W.AsInt();
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varAddr2", varAddr2W);
				m_mifFuncDecl.Append(", ht_uint%d varAddr2", varAddr2W);
				m_mifMacros.Append(", ht_uint%d %svarAddr2", varAddr2W, rdDst.m_pSharedVar->m_addr2W.AsInt() == 0 ? "ht_noload" : "");
			}

			if (rdDst.m_varDimen1 > 0) {
				argCnt += 1;
				int varDimen1W = FindLg2(rdDst.m_varDimen1-1);
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varIdx1", varDimen1W);
				m_mifFuncDecl.Append(", ht_uint%d varIdx1", varDimen1W);
				m_mifMacros.Append(", ht_uint%d varIdx1", varDimen1W);
			}
			if (rdDst.m_varDimen2 > 0) {
				argCnt += 1;
				int varDimen2W = FindLg2(rdDst.m_varDimen2-1);
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varIdx2", varDimen2W);
				m_mifFuncDecl.Append(", ht_uint%d varIdx2", varDimen2W);
				m_mifMacros.Append(", ht_uint%d varIdx2", varDimen2W);
			}

			if (rdDst.m_fldDimen1 > 0) {
				argCnt += 1;
				int fldDimen1W = FindLg2(rdDst.m_fldDimen1-1);
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d fldIdx1", fldDimen1W);
				m_mifFuncDecl.Append(", ht_uint%d fldIdx1", fldDimen1W);
				m_mifMacros.Append(", ht_uint%d fldIdx1", fldDimen1W);
			}
			if (rdDst.m_fldDimen2 > 0) {
				argCnt += 1;
				int fldDimen2W = FindLg2(rdDst.m_fldDimen2-1);
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d fldIdx2", fldDimen2W);
				m_mifFuncDecl.Append(", ht_uint%d fldIdx2", fldDimen2W);
				m_mifMacros.Append(", ht_uint%d fldIdx2", fldDimen2W);
			}
			if (rdDst.m_infoW.AsInt() > 0) {
				argCnt += 1;
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d info", rdDst.m_infoW.AsInt());
				m_mifFuncDecl.Append(", ht_uint%d info", rdDst.m_infoW.AsInt());
				m_mifMacros.Append(", ht_uint%d info", rdDst.m_infoW.AsInt());
			}
			if (rdDst.m_bMultiRd) {
				argCnt += 1;
				g_appArgs.GetDsnRpt().AddText(", sc_uint<4> qwCnt");
				m_mifFuncDecl.Append(", sc_uint<4> qwCnt");
				m_mifMacros.Append(", sc_uint<4> qwCnt");
			}

			g_appArgs.GetDsnRpt().AddText(", bool orderChk=true)\n");
			m_mifFuncDecl.Append(", bool ht_noload orderChk=true);\n");
			m_mifMacros.Append(", bool ht_noload orderChk)\n");

			g_appArgs.GetDsnRpt().EndLevel();

			if (true || g_appArgs.IsMemTraceEnabled()) {
				m_mifFuncDecl.Append("\t#else\n");
				m_mifMacros.Append("#else\n");

				m_mifFuncDecl.Append("\t#define %sMem%s(", pMemOpName, dstName);
				m_mifFuncDecl.Append("...) %sMem%s_(", pMemOpName, dstName);
				m_mifFuncDecl.Append("(char *)__FILE__, __LINE__, __VA_ARGS__)\n");
				m_mifFuncDecl.Append("\tvoid %sMem%s_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr",
					pMemOpName, dstName);

				m_mifMacros.Append("void CPers%s%s::%sMem%s_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(),
					pMemOpName, dstName);

				if (rdDst.m_pIntGbl && (rdDst.m_pIntGbl->m_addr1W.size() > 0 || rdDst.m_pIntGbl->m_addr1Name.size() > 0)) {

					int varAddr1W = rdDst.m_pIntGbl->m_addr1W.AsInt() == 0 ? 1 : rdDst.m_pIntGbl->m_addr1W.AsInt();
					m_mifFuncDecl.Append(", ht_uint%d varAddr1", varAddr1W);
					m_mifMacros.Append(", ht_uint%d varAddr1", varAddr1W);

				} else if (rdDst.m_pExtRam && (rdDst.m_pExtRam->m_addr1W.size() > 0 || rdDst.m_pExtRam->m_addr1Name.size() > 0)) {

					int varAddr1W = rdDst.m_pExtRam->m_addr1W.AsInt() == 0 ? 1 : rdDst.m_pExtRam->m_addr1W.AsInt();
					m_mifFuncDecl.Append(", ht_uint%d varAddr1", varAddr1W);
					m_mifMacros.Append(", ht_uint%d varAddr1", varAddr1W);

				} else if (rdDst.m_pSharedVar && rdDst.m_pSharedVar->m_addr1W.size() > 0) {

					int varAddr1W = rdDst.m_pSharedVar->m_addr1W.AsInt() == 0 ? 1 : rdDst.m_pSharedVar->m_addr1W.AsInt();
					m_mifFuncDecl.Append(", ht_uint%d varAddr1", varAddr1W);
					m_mifMacros.Append(", ht_uint%d varAddr1", varAddr1W);
				}

				if (rdDst.m_pIntGbl && (rdDst.m_pIntGbl->m_addr2W.size() > 0 || rdDst.m_pIntGbl->m_addr2Name.size() > 0)) {

					int varAddr2W = rdDst.m_pIntGbl->m_addr2W.AsInt() == 0 ? 1 : rdDst.m_pIntGbl->m_addr2W.AsInt();
					m_mifFuncDecl.Append(", ht_uint%d varAddr2", varAddr2W);
					m_mifMacros.Append(", ht_uint%d varAddr2", varAddr2W);

				} else if (rdDst.m_pExtRam && (rdDst.m_pExtRam->m_addr2W.size() > 0 || rdDst.m_pExtRam->m_addr2Name.size() > 0)) {

					int varAddr2W = rdDst.m_pExtRam->m_addr2W.AsInt() == 0 ? 1 : rdDst.m_pExtRam->m_addr2W.AsInt();
					m_mifFuncDecl.Append(", ht_uint%d varAddr2", varAddr2W);
					m_mifMacros.Append(", ht_uint%d varAddr2", varAddr2W);

				} else if (rdDst.m_pSharedVar && rdDst.m_pSharedVar->m_addr2W.size() > 0) {

					int varAddr2W = rdDst.m_pSharedVar->m_addr2W.AsInt() == 0 ? 1 : rdDst.m_pSharedVar->m_addr2W.AsInt();
					m_mifFuncDecl.Append(", ht_uint%d varAddr2", varAddr2W);
					m_mifMacros.Append(", ht_uint%d varAddr2", varAddr2W);
				}

				if (rdDst.m_varDimen1 > 0) {
					int varDimen1W = FindLg2(rdDst.m_varDimen1-1);
					m_mifFuncDecl.Append(", ht_uint%d varIdx1", varDimen1W);
					m_mifMacros.Append(", ht_uint%d varIdx1", varDimen1W);
				}
				if (rdDst.m_varDimen2 > 0) {
					int varDimen2W = FindLg2(rdDst.m_varDimen2-1);
					m_mifFuncDecl.Append(", ht_uint%d varIdx2", varDimen2W);
					m_mifMacros.Append(", ht_uint%d varIdx2", varDimen2W);
				}

				if (rdDst.m_fldDimen1 > 0) {
					int fldDimen1W = FindLg2(rdDst.m_fldDimen1-1);
					m_mifFuncDecl.Append(", ht_uint%d fldIdx1", fldDimen1W);
					m_mifMacros.Append(", ht_uint%d fldIdx1", fldDimen1W);
				}
				if (rdDst.m_fldDimen2 > 0) {
					int fldDimen2W = FindLg2(rdDst.m_fldDimen2-1);
					m_mifFuncDecl.Append(", ht_uint%d fldIdx2", fldDimen2W);
					m_mifMacros.Append(", ht_uint%d fldIdx2", fldDimen2W);
				}
				if (rdDst.m_infoW.AsInt() > 0) {
					m_mifFuncDecl.Append(", ht_uint%d info", rdDst.m_infoW.AsInt());
					m_mifMacros.Append(", ht_uint%d info", rdDst.m_infoW.AsInt());
				}
				if (rdDst.m_bMultiRd) {
					m_mifFuncDecl.Append(", sc_uint<4> qwCnt");
					m_mifMacros.Append(", sc_uint<4> qwCnt");
				}

				m_mifFuncDecl.Append(", bool orderChk=true);\n");
				m_mifMacros.Append(", bool orderChk)\n");

				m_mifFuncDecl.Append("\t#endif\n");
				m_mifMacros.Append("#endif\n");
			}

			m_mifMacros.Append("{\n");

			m_mifMacros.Append("\tassert_msg(c_t%d_bReadMemAvail, \"Runtime check failed in CPers%s::%sMem%s(...) - ReadMemBusy was not called\\n\");\n",
				mod.m_execStg, mod.m_modName.Uc().c_str(), pMemOpName, dstName);

			if (rdDst.m_varDimen1 > 0 && (!IsPowerTwo(rdDst.m_varDimen1) || rdDst.m_varDimen1 == 1))
				m_mifMacros.Append("\tassert_msg(varIdx1 < %d, \"Runtime check failed in CPers%s%s::%sMem%s(...) - varIdx1 (%%d) out of range (0-%d)\\n\", (uint32_t)varIdx1);\n",
					rdDst.m_varDimen1, unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMemOpName, dstName, rdDst.m_varDimen1-1);

			if (rdDst.m_varDimen2 > 0 && (!IsPowerTwo(rdDst.m_varDimen2) || rdDst.m_varDimen2 == 1))
				m_mifMacros.Append("\tassert_msg(varIdx2 < %d, \"Runtime check failed in CPers%s%s::%sMem%s(...) - varIdx2 (%%d) out of range (0-%d)\\n\", (uint32_t)varIdx2);\n",
					rdDst.m_varDimen2, unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMemOpName, dstName, rdDst.m_varDimen2-1);

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
			m_mifMacros.NewLine();

			if (rdDst.m_bMultiRd) {
				m_mifMacros.Append("\tassert_msg(qwCnt > 0 && qwCnt <= 8,"
					" \"Runtime check failed in CPers%s%s::%sMem%s(...) - "
					"expected multi-qw access to be within 64B memory line\\n\");\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMemOpName, dstName);
				m_mifMacros.Append("\tassert_msg(((memAddr >> 3) & 7) + qwCnt <= 8,"
					" \"Runtime check failed in CPers%s%s::%sMem%s(...) - "
					"expected multi-qw access to be within 64B memory line\\n\");\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), pMemOpName, dstName);
				m_mifMacros.NewLine();
			}

			char const * pTidQwCntM1 = "(qwCnt-1)";
			char tidQwCntBuf[64];
			if (bNeed_reqQwSplit && rdDst.m_bMultiRd && rdDst.m_memSrc == "host") {
				m_mifMacros.Append("\tht_uint3 tidQwCntM1 = qwCnt != 8 ? 0 : 7;\n\n");
				pTidQwCntM1 = "tidQwCntM1";
			} else if (is_wx && rdDst.m_bMultiRd) {
				sprintf(tidQwCntBuf, "(c_t%d_reqQwCnt-1)", mod.m_execStg);
				pTidQwCntM1 = tidQwCntBuf;
			}

			if (mif.m_bMifWr)
				m_mifMacros.Append("\tc_t%d_bReadMem = true;\n", mod.m_execStg);
			if (bNeed_reqQwCnt) {
				if (rdDst.m_bMultiRd) {
					if (is_wx) {
						m_mifMacros.Append("\tc_t%d_reqQwCnt = qwCnt == 1 ? 1 : 8;\n", mod.m_execStg);
						m_mifMacros.Append("\tht_uint3 c_t%d_reqQwFirst = (qwCnt == 1) ? 0 : (memAddr >> 3) & 0x7;\n", mod.m_execStg);
						m_mifMacros.Append("\tht_uint3 c_t%d_reqQwLast = (ht_uint3)(c_t%d_reqQwFirst + qwCnt - 1);\n",
							mod.m_execStg, mod.m_execStg);
					} else
						m_mifMacros.Append("\tc_t%d_reqQwCnt = qwCnt;\n", mod.m_execStg);
				} else
					m_mifMacros.Append("\tc_t%d_reqQwCnt = 1;\n", mod.m_execStg);
			}
			if (bNeed_reqQwSplit) {
				if (rdDst.m_memSrc == "coproc" || !rdDst.m_bMultiRd)
					m_mifMacros.Append("\tc_t%d_reqQwSplit = true;\n", mod.m_execStg);
				else
					m_mifMacros.Append("\tc_t%d_reqQwSplit = qwCnt != 8;\n", mod.m_execStg);
			}
			m_mifMacros.Append("\tc_t%d_%sToMif_reqRdy = true;\n", mod.m_execStg, mod.m_modName.Lc().c_str());

			if (rdDst.m_bMultiRd)
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_host = %s;\n", mod.m_execStg, mod.m_modName.Lc().c_str(),
				rdDst.m_memSrc == "host" ? "true" : "false");

			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_type = %s;\n", mod.m_execStg, mod.m_modName.Lc().c_str(), pReadType);

			if (rdDst.m_bMultiRd && is_wx)
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_addr = memAddr & (qwCnt == 1 ? ~0x0LL : ~0x3fLL);\n", mod.m_execStg, mod.m_modName.Lc().c_str());
			else
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

			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_tid = (sc_uint<MIF_TID_W>)(\n", mod.m_execStg, mod.m_modName.Lc().c_str());

			char const * pRdRspGrpW = mif.m_mifRd.m_rspGrpId.size() == 0 ? "_HTID_W" : "_RD_GRP_ID_W";

			if (rdDst.m_bMultiRd && (is_wx || rdDst.m_memSrc == "host" && is_hc2))
			{
				m_mifMacros.Append("\t\t((%s & MIF_TID_QWCNT_MSK) << MIF_TID_QWCNT_SHF) |\n",
					pTidQwCntM1);
			}

			if (rdDst.m_dstW > 0) {

				if (rdDst.m_infoW.size() > 0) {
					if (rdDst.m_infoW.AsInt() > 0) {
						m_mifMacros.Append("\t\t(info << (%s_MIF_DST_%s_INFO_SHF + 1 + %s%s)) |\n",
							mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(),
							mod.m_modName.Upper().c_str(), pRdRspGrpW);
					}
				} else {
					if (rdDst.m_varAddr0W > 0) {
						m_mifMacros.Append("\t\t(r_t%d_htId << (%s_MIF_DST_%s_VAR_ADDR0_SHF + 1 + %s%s)) |\n",
							mod.m_execStg,
							mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(),
							mod.m_modName.Upper().c_str(), pRdRspGrpW);
					}

					if (rdDst.m_pIntGbl && rdDst.m_pIntGbl->m_addr1W.AsInt() > 0
						|| rdDst.m_pExtRam && rdDst.m_pExtRam->m_addr1W.AsInt() > 0
						|| rdDst.m_pSharedVar && rdDst.m_pSharedVar->m_addr1W.AsInt() > 0)
					{
						if (rdDst.m_pIntGbl && rdDst.m_pIntGbl->m_addr1W.AsInt() > 0 &&
							(!g_appArgs.IsGlobalWriteHtidEnabled() && rdDst.m_pIntGbl->m_addr1Name == "htId"))

							m_mifMacros.Append("\t\t(r_t%d_htId << (%s_MIF_DST_%s_VAR_ADDR1_SHF + 1 + %s%s)) |\n",
								mod.m_execStg,
								mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(),
								mod.m_modName.Upper().c_str(), pRdRspGrpW);

						else if (rdDst.m_pExtRam && rdDst.m_pExtRam->m_addr1W.AsInt() > 0 &&
							(!g_appArgs.IsGlobalWriteHtidEnabled() && rdDst.m_pExtRam->m_addr1Name == "htId"))

							m_mifMacros.Append("\t\t(r_t%d_htId << (%s_MIF_DST_%s_VAR_ADDR1_SHF + 1 + %s%s)) |\n",
								mod.m_execStg,
								mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(),
								mod.m_modName.Upper().c_str(), pRdRspGrpW);

						else
							m_mifMacros.Append("\t\t(varAddr1 << (%s_MIF_DST_%s_VAR_ADDR1_SHF + 1 + %s%s)) |\n",
								mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(),
								mod.m_modName.Upper().c_str(), pRdRspGrpW);
					}

					if (rdDst.m_pIntGbl && rdDst.m_pIntGbl->m_addr2W.AsInt() > 0
						|| rdDst.m_pExtRam && rdDst.m_pExtRam->m_addr2W.AsInt() > 0
						|| rdDst.m_pSharedVar && rdDst.m_pSharedVar->m_addr2W.AsInt() > 0)
					{
						if (rdDst.m_pIntGbl && rdDst.m_pIntGbl->m_addr2W.AsInt() > 0 &&
							(!g_appArgs.IsGlobalWriteHtidEnabled() && rdDst.m_pIntGbl->m_addr2Name == "htId"))

							m_mifMacros.Append("\t\t(r_t%d_htId << (%s_MIF_DST_%s_VAR_ADDR2_SHF + 1 + %s%s)) |\n",
								mod.m_execStg,
								mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(),
								mod.m_modName.Upper().c_str(), pRdRspGrpW);

						else if (rdDst.m_pExtRam && rdDst.m_pExtRam->m_addr2W.AsInt() > 0 &&
							(!g_appArgs.IsGlobalWriteHtidEnabled() && rdDst.m_pExtRam->m_addr2Name == "htId"))

							m_mifMacros.Append("\t\t(r_t%d_htId << (%s_MIF_DST_%s_VAR_ADDR2_SHF + 1 + %s%s)) |\n",
								mod.m_execStg,
								mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(),
								mod.m_modName.Upper().c_str(), pRdRspGrpW);

						else
							m_mifMacros.Append("\t\t(varAddr2 << (%s_MIF_DST_%s_VAR_ADDR2_SHF + 1 + %s%s)) |\n",
								mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(),
								mod.m_modName.Upper().c_str(), pRdRspGrpW);
					}

					if (rdDst.m_varDimen1 > 0)
						m_mifMacros.Append("\t\t(varIdx1 << (%s_MIF_DST_%s_VAR_IDX1_SHF + 1 + %s%s)) |\n",
						mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), pRdRspGrpW);

					if (rdDst.m_varDimen2 > 0)
						m_mifMacros.Append("\t\t(varIdx2 << (%s_MIF_DST_%s_VAR_IDX2_SHF + 1 + %s%s)) |\n",
						mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), pRdRspGrpW);

					if (rdDst.m_fldDimen1 > 0)
						m_mifMacros.Append("\t\t(fldIdx1 << (%s_MIF_DST_%s_FLD_IDX1_SHF + 1 + %s%s)) |\n",
						mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), pRdRspGrpW);

					if (rdDst.m_fldDimen2 > 0)
						m_mifMacros.Append("\t\t(fldIdx2 << (%s_MIF_DST_%s_FLD_IDX2_SHF + 1 + %s%s)) |\n",
						mod.m_modName.Upper().c_str(), rdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), pRdRspGrpW);
				}
			}

			if (is_wx && rdDst.m_bMultiRd) {
				m_mifMacros.Append("\t\t(c_t%d_reqQwFirst << (%s_MIF_DST_QW_FIRST_SHF + 1 + %s%s)) |\n",
					mod.m_execStg, mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str(), pRdRspGrpW);
				m_mifMacros.Append("\t\t(c_t%d_reqQwLast << (%s_MIF_DST_QW_LAST_SHF + 1 + %s%s)) |\n",
					mod.m_execStg, mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str(), pRdRspGrpW);
			}

			m_mifMacros.Append("\t\t(%d << (1 + %s%s)) |\n",
				dstIdx, mod.m_modName.Upper().c_str(), pRdRspGrpW);

			m_mifMacros.Append("\t\t(0 << %s%s)",
				mod.m_modName.Upper().c_str(), pRdRspGrpW);

			if (mif.m_mifRd.m_rspGrpIdW == 0)
				m_mifMacros.Append(");\n");
			else if (mif.m_mifRd.m_rspGrpId.size() == 0)
				m_mifMacros.Append(" |\n\t\tr_t%d_htId);\n", mod.m_execStg);
			else if (mif.m_mifRd.m_bRspGrpIdPriv)
				m_mifMacros.Append(" |\n\t\tr_t%d_htPriv.m_%s);\n",
				mod.m_execStg, mif.m_mifRd.m_rspGrpId.AsStr().c_str());
			else
				m_mifMacros.Append(" |\n\t\tr_%s);\n", mif.m_mifRd.m_rspGrpId.AsStr().c_str());

			if (true || g_appArgs.IsMemTraceEnabled()) {
				m_mifMacros.Append("\t#ifndef _HTV\n");
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_file = file;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_line = line;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_time = sc_time_stamp().value();\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_orderChk = orderChk;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				m_mifMacros.Append("\t#endif\n");
			}
			m_mifMacros.Append("}\n");
			m_mifMacros.NewLine();
		}

		///////////////////////////////////////////////////////////
		// generate ReadMemPause routine

		if (mif.m_mifRd.m_bPause) {

			g_appArgs.GetDsnRpt().AddLevel("void ReadMemPause(ht_uint%d rsmInstr)\n",
				mod.m_instrW);
			g_appArgs.GetDsnRpt().EndLevel();

			m_mifFuncDecl.Append("\tvoid ReadMemPause(ht_uint%d rsmInstr);\n",
				mod.m_instrW);
			m_mifMacros.Append("void CPers%s%s::ReadMemPause(ht_uint%d rsmInstr)\n",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
			m_mifMacros.Append("{\n");

			m_mifMacros.Append("\tassert_msg(c_t%d_bReadMemAvail, \"Runtime check failed in CPers%s::ReadMemPause()"
				" - expected ReadMemBusy() to have been called and not busy\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());
			m_mifMacros.NewLine();

			m_mifMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::ReadMemPause()"
				" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());
			m_mifMacros.NewLine();

			if (mif.m_mifRd.m_rspGrpIdW == 0) {

				m_mifMacros.Append("\tc_t%d_memReadPause = true;\n", mod.m_execStg);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tif (r_t%d_rdRspPauseCntZero && !(c_t%d_%sToMif_reqRdy",
					mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str());

				if (mod.m_mif.m_bMifWr)
					m_mifMacros.Append(" && c_t%d_bReadMem))\n", mod.m_execStg);
				else
					m_mifMacros.Append("))\n");

				m_mifMacros.Append("\t\tHtContinue(rsmInstr);\n");
				m_mifMacros.Append("\telse {\n");
				m_mifMacros.Append("\t\t// should not occur - verify waiting flag not already set\n");
				m_mifMacros.Append("\t\tassert(c_rdGrpRsmWait == false);\n");
				m_mifMacros.Append("\t\tc_rdGrpRsmWait = true;\n");
				m_mifMacros.Append("\t\tc_rdGrpRsmInstr = rsmInstr;\n");

				if (mif.m_mifRd.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\t\tc_rdGrpRsmHtId = r_t%d_htId;\n", mod.m_execStg);

				m_mifMacros.Append("\t\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
				m_mifMacros.Append("\t\tc_t%d_htNextInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.Append("\t}\n");

			} else if (mif.m_mifRd.m_rspGrpIdW <= 2) {

				m_mifMacros.Append("\tc_t%d_memReadPause = true;\n", mod.m_execStg);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tif (r_t%d_rdRspPauseCntZero && !(c_t%d_%sToMif_reqRdy",
					mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str());

				if (mod.m_mif.m_bMifWr)
					m_mifMacros.Append(" && c_t%d_bReadMem))\n", mod.m_execStg);
				else
					m_mifMacros.Append("))\n");

				m_mifMacros.Append("\t\tHtContinue(rsmInstr);\n");
				m_mifMacros.Append("\telse {\n");
				m_mifMacros.Append("\t\t// should not occur - verify waiting flag not already set\n");
				m_mifMacros.Append("\t\tassert(c_rdGrpRsmWait[INT(%s)] == false);\n", rdRspGrpName);
				m_mifMacros.Append("\t\tc_rdGrpRsmWait[INT(%s)] = true;\n", rdRspGrpName);
				m_mifMacros.Append("\t\tc_rdGrpRsmInstr[INT(%s)] = rsmInstr;\n", rdRspGrpName);

				if (mif.m_mifRd.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\t\tc_rdGrpRsmHtId[INT(%s)] = r_t%d_htId;\n",
					rdRspGrpName, mod.m_execStg);

				m_mifMacros.Append("\t\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
				m_mifMacros.Append("\t\tc_t%d_htNextInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.Append("\t}\n");

			} else {
				m_mifMacros.Append("\tm_rdGrpRsmWait[%s & %s].write_addr(%s >> %s);\n",
					rdRspGrpName, pRdGrpIdMask, rdRspGrpName, pRdGrpShfAmt);
				m_mifMacros.Append("\tm_rdGrpRsmInstr[%s & %s].write_addr(%s >> %s);\n",
					rdRspGrpName, pRdGrpIdMask, rdRspGrpName, pRdGrpShfAmt);
				if (mif.m_mifRd.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\tm_rdGrpRsmHtId[%s & %s].write_addr(%s >> %s);\n",
					rdRspGrpName, pRdGrpIdMask, rdRspGrpName, pRdGrpShfAmt);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tif (r_t%d_rdRspPauseCntZero && !(c_t%d_%sToMif_reqRdy",
					mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str());

				if (mod.m_mif.m_bMifWr)
					m_mifMacros.Append(" && c_t%d_bReadMem))\n", mod.m_execStg);
				else
					m_mifMacros.Append("))\n");

				m_mifMacros.Append("\t\tHtContinue(rsmInstr);\n");
				m_mifMacros.Append("\telse {\n");
				m_mifMacros.Append("\t\tm_rdGrpRsmWait[%s & %s].write_mem(true);\n", rdRspGrpName, pRdGrpIdMask);
				m_mifMacros.Append("\t\tm_rdGrpRsmInstr[%s & %s].write_mem(rsmInstr);\n", rdRspGrpName, pRdGrpIdMask);
				if (mif.m_mifRd.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\t\tm_rdGrpRsmHtId[%s & %s].write_mem(r_t%d_htId);\n",
					rdRspGrpName, pRdGrpIdMask, mod.m_execStg);
				m_mifMacros.Append("\t\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
				m_mifMacros.Append("\t\tc_t%d_htNextInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.Append("\t}\n");
			}

			m_mifMacros.Append("}\n");
			m_mifMacros.NewLine();
		}
	}

	const char *pWrGrpIdMask = (mif.m_mifWr.m_rspGrpIdW == 2 || mif.m_mifWr.m_bMaxBw) ? "3" : "1";
	const char *pWrGrpShfAmt = (mif.m_mifWr.m_rspGrpIdW == 2 || mif.m_mifWr.m_bMaxBw) ? "2" : "1";

	char wrRspGrpName[64];
	if (mif.m_bMifWr) {
		// write busy macros
		m_mifMacros.Append("// Memory Write Interface Macros\n");

		if (mif.m_mifWr.m_rspGrpId.size() == 0)
			sprintf(wrRspGrpName, "r_t%d_htId", mod.m_execStg);
		else if (mif.m_mifWr.m_bRspGrpIdPriv)
			sprintf(wrRspGrpName, "r_t%d_htPriv.m_%s",
			mod.m_execStg, mif.m_mifWr.m_rspGrpId.AsStr().c_str());
		else
			sprintf(wrRspGrpName, "r_%s", mif.m_mifWr.m_rspGrpId.AsStr().c_str());

		///////////////////////////////////////////////////////////
		// generate WriteMemBusy routine

		g_appArgs.GetDsnRpt().AddLevel("bool WriteMemBusy()\n");
		g_appArgs.GetDsnRpt().EndLevel();

		m_mifFuncDecl.Append("\tbool WriteMemBusy();\n");
		m_mifMacros.Append("bool CPers%s%s::WriteMemBusy()\n",
			unitNameUc.c_str(), mod.m_modName.Uc().c_str());
		m_mifMacros.Append("{\n");

		string rspGrpId = mif.m_mifWr.m_rspGrpId.size() == 0 ? "" : "rspGrpId";

		char reqBusy[32];
		if (bNeed_reqBusy)
			sprintf(reqBusy, " || r_t%d_reqBusy", mod.m_execStg);
		else
			reqBusy[0] = '\0';

		if (mif.m_mifWr.m_rspGrpIdW <= 2) {

			mifPostReg.Append("#\tifdef _HTV\n");
			mifPostReg.Append("\tc_t%d_bWriteMemBusy = r_%sToMif_reqAvlCntBusy || r_t%d_wrGrpRspCntMax%s;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str(), mod.m_execStg, reqBusy);

			mifPostReg.Append("#\telse\n");

			mifPostReg.Append("\tc_t%d_bWriteMemBusy = r_%sToMif_reqAvlCntBusy || r_t%d_wrGrpRspCntMax%s ||\n",
				mod.m_execStg, mod.m_modName.Lc().c_str(), mod.m_execStg, reqBusy);

		} else {
			mifPostReg.Append("#\tifdef _HTV\n");
			mifPostReg.Append("\tc_t%d_bWriteMemBusy = r_t%d_bWriteRspBusy || r_%sToMif_reqAvlCntBusy || r_t%d_wrGrpRspCntMax%s;\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), mod.m_execStg, reqBusy);

			mifPostReg.Append("#\telse\n");

			mifPostReg.Append("\tc_t%d_bWriteMemBusy = r_t%d_bWriteRspBusy || r_%sToMif_reqAvlCntBusy || r_t%d_wrGrpRspCntMax%s ||\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), mod.m_execStg, reqBusy);
		}

		mifPostReg.Append("\t\t(%s_RND_RETRY && !!(g_rndRetry() & 1));\n", mod.m_modName.Upper().c_str());
		mifPostReg.Append("#\tendif\n");

		m_mifMacros.Append("\tc_t%d_bWriteMemAvail = !c_t%d_bWriteMemBusy;\n", mod.m_execStg, mod.m_execStg);
		m_mifMacros.Append("\treturn c_t%d_bWriteMemBusy;\n", mod.m_execStg);
		m_mifMacros.Append("}\n");
		m_mifMacros.NewLine();

		///////////////////////////////////////////////////////////
		// generate WriteMemPoll routine

		if (mif.m_mifWr.m_bPoll) {

			g_appArgs.GetDsnRpt().AddLevel("bool WriteMemPoll()\n");
			g_appArgs.GetDsnRpt().EndLevel();

			m_mifFuncDecl.Append("\tbool WriteMemPoll();\n");
			m_mifMacros.Append("bool CPers%s%s::WriteMemPoll()\n",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str());
			m_mifMacros.Append("{\n");

			m_mifMacros.Append("\tassert_msg(c_t%d_bWriteMemAvail, \"Runtime check failed in CPers%s::WriteMemPoll()"
				" - expected WriteMemBusy() to have been called and not busy\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());
			m_mifMacros.NewLine();

			m_mifMacros.Append("\treturn !r_t%d_wrGrpRspCntZero;\n", mod.m_execStg);

			m_mifMacros.Append("}\n");
			m_mifMacros.NewLine();
		}

		///////////////////////////////////////////////////////////
		// generate WriteMem routine

		if (true || g_appArgs.IsMemTraceEnabled()) {
			m_mifFuncDecl.Append("\t#ifdef _HTV\n");
			m_mifFuncDecl.Append("\tvoid WriteMem(sc_uint<MEM_ADDR_W> memAddr, uint64_t memData, bool ht_noload orderChk=true);\n", mod.m_instrW);
			m_mifFuncDecl.Append("\tvoid WriteMem_uint8(sc_uint<MEM_ADDR_W> memAddr, uint8_t memData, bool ht_noload orderChk=true);\n", mod.m_instrW);
			m_mifFuncDecl.Append("\tvoid WriteMem_uint16(sc_uint<MEM_ADDR_W> memAddr, uint16_t memData, bool ht_noload orderChk=true);\n", mod.m_instrW);
			m_mifFuncDecl.Append("\tvoid WriteMem_uint32(sc_uint<MEM_ADDR_W> memAddr, uint32_t memData, bool ht_noload orderChk=true);\n", mod.m_instrW);
			m_mifFuncDecl.Append("\tvoid WriteMem_uint64(sc_uint<MEM_ADDR_W> memAddr, uint64_t memData, bool ht_noload orderChk=true);\n", mod.m_instrW);
			m_mifFuncDecl.Append("\tvoid WriteMem_int8(sc_uint<MEM_ADDR_W> memAddr, int8_t memData, bool ht_noload orderChk=true);\n", mod.m_instrW);
			m_mifFuncDecl.Append("\tvoid WriteMem_int16(sc_uint<MEM_ADDR_W> memAddr, int16_t memData, bool ht_noload orderChk=true);\n", mod.m_instrW);
			m_mifFuncDecl.Append("\tvoid WriteMem_int32(sc_uint<MEM_ADDR_W> memAddr, int32_t memData, bool ht_noload orderChk=true);\n", mod.m_instrW);
			m_mifFuncDecl.Append("\tvoid WriteMem_int64(sc_uint<MEM_ADDR_W> memAddr, int64_t memData, bool ht_noload orderChk=true);\n", mod.m_instrW);
			m_mifFuncDecl.Append("\t#else\n");
			m_mifFuncDecl.Append("\t#define WriteMem_uint8(...) WriteMem_uint8_((char *)__FILE__, __LINE__, __VA_ARGS__)\n");
			m_mifFuncDecl.Append("\tvoid WriteMem_uint8_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr, uint8_t memData, bool orderChk=true);\n",
				mod.m_instrW);
			m_mifFuncDecl.Append("\t#define WriteMem_uint16(...) WriteMem_uint16_((char *)__FILE__, __LINE__, __VA_ARGS__)\n");
			m_mifFuncDecl.Append("\tvoid WriteMem_uint16_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr, uint16_t memData, bool orderChk=true);\n",
				mod.m_instrW);
			m_mifFuncDecl.Append("\t#define WriteMem_uint32(...) WriteMem_uint32_((char *)__FILE__, __LINE__, __VA_ARGS__)\n");
			m_mifFuncDecl.Append("\tvoid WriteMem_uint32_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr, uint32_t memData, bool orderChk=true);\n",
				mod.m_instrW);
			m_mifFuncDecl.Append("\t#define WriteMem(...) WriteMem_uint64_((char *)__FILE__, __LINE__, __VA_ARGS__)\n");
			m_mifFuncDecl.Append("\t#define WriteMem_uint64(...) WriteMem_uint64_((char *)__FILE__, __LINE__, __VA_ARGS__)\n");
			m_mifFuncDecl.Append("\tvoid WriteMem_uint64_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr, uint64_t memData, bool orderChk=true);\n",
				mod.m_instrW);

			m_mifFuncDecl.Append("\t#define WriteMem_int8(...) WriteMem_int8_((char *)__FILE__, __LINE__, __VA_ARGS__)\n");
			m_mifFuncDecl.Append("\tvoid WriteMem_int8_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr, int8_t memData, bool orderChk=true);\n",
				mod.m_instrW);
			m_mifFuncDecl.Append("\t#define WriteMem_int16(...) WriteMem_int16_((char *)__FILE__, __LINE__, __VA_ARGS__)\n");
			m_mifFuncDecl.Append("\tvoid WriteMem_int16_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr, int16_t memData, bool orderChk=true);\n",
				mod.m_instrW);
			m_mifFuncDecl.Append("\t#define WriteMem_int32(...) WriteMem_int32_((char *)__FILE__, __LINE__, __VA_ARGS__)\n");
			m_mifFuncDecl.Append("\tvoid WriteMem_int32_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr, int32_t memData, bool orderChk=true);\n",
				mod.m_instrW);
			m_mifFuncDecl.Append("\t#define WriteMem_int64(...) WriteMem_int64_((char *)__FILE__, __LINE__, __VA_ARGS__)\n");
			m_mifFuncDecl.Append("\tvoid WriteMem_int64_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr, int64_t memData, bool orderChk=true);\n",
				mod.m_instrW);

			m_mifFuncDecl.Append("\t#endif\n");
		}

		for (int memWrSize = 0; memWrSize < 8; memWrSize += 1) {
			m_mifMacros.Append("#ifdef _HTV\n");

			string routineName;

			switch (memWrSize) {
			case 0:
				routineName = "WriteMem_uint8";

				g_appArgs.GetDsnRpt().AddLevel("void WriteMem_uint8(sc_uint<MEM_ADDR_W> memAddr, uint8_t memData, bool orderChk=true)\n");
				g_appArgs.GetDsnRpt().EndLevel();

				m_mifMacros.Append("void CPers%s%s::WriteMem_uint8(sc_uint<MEM_ADDR_W> memAddr, uint8_t memData, bool ht_noload orderChk)\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				break;
			case 1:
				routineName = "WriteMem_uint16";

				g_appArgs.GetDsnRpt().AddLevel("void WriteMem_uint16(sc_uint<MEM_ADDR_W> memAddr, uint8_t memData, bool orderChk=true)\n");
				g_appArgs.GetDsnRpt().EndLevel();

				m_mifMacros.Append("void CPers%s%s::WriteMem_uint16(sc_uint<MEM_ADDR_W> memAddr, uint16_t memData, bool ht_noload orderChk)\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				break;
			case 2:
				routineName = "WriteMem_uint32";

				g_appArgs.GetDsnRpt().AddLevel("void WriteMem_uint32(sc_uint<MEM_ADDR_W> memAddr, uint32_t memData, bool orderChk=true)\n");
				g_appArgs.GetDsnRpt().EndLevel();

				m_mifMacros.Append("void CPers%s%s::WriteMem_uint32(sc_uint<MEM_ADDR_W> memAddr, uint32_t memData, bool ht_noload orderChk)\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				break;
			case 3:
				routineName = "WriteMem";

				g_appArgs.GetDsnRpt().AddLevel("void WriteMem(sc_uint<MEM_ADDR_W> memAddr, uint64_t memData, bool orderChk=true)\n");
				g_appArgs.GetDsnRpt().EndLevel();

				m_mifMacros.Append("void CPers%s%s::WriteMem(sc_uint<MEM_ADDR_W> memAddr, uint64_t memData, bool ht_noload orderChk) {\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				m_mifMacros.Append("\tWriteMem_uint64(memAddr, memData, orderChk);\n");
				m_mifMacros.Append("}\n");

				m_mifMacros.Append("void CPers%s%s::WriteMem_uint64(sc_uint<MEM_ADDR_W> memAddr, uint64_t memData, bool ht_noload orderChk)\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				break;
			case 4:
				routineName = "WriteMem_int8";

				g_appArgs.GetDsnRpt().AddLevel("void WriteMem_int8(sc_uint<MEM_ADDR_W> memAddr, int8_t memData, bool orderChk=true)\n");
				g_appArgs.GetDsnRpt().EndLevel();

				m_mifMacros.Append("void CPers%s%s::WriteMem_int8(sc_uint<MEM_ADDR_W> memAddr, int8_t memData, bool ht_noload orderChk)\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				break;
			case 5:
				routineName = "WriteMem_int16";

				g_appArgs.GetDsnRpt().AddLevel("void WriteMem_int16(sc_uint<MEM_ADDR_W> memAddr, int16_t memData, bool orderChk=true)\n");
				g_appArgs.GetDsnRpt().EndLevel();

				m_mifMacros.Append("void CPers%s%s::WriteMem_int16(sc_uint<MEM_ADDR_W> memAddr, int16_t memData, bool ht_noload orderChk)\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				break;
			case 6:
				routineName = "WriteMem_int32";

				g_appArgs.GetDsnRpt().AddLevel("void WriteMem_int32(sc_uint<MEM_ADDR_W> memAddr, int32_t memData, bool orderChk=true)\n");
				g_appArgs.GetDsnRpt().EndLevel();

				m_mifMacros.Append("void CPers%s%s::WriteMem_int32(sc_uint<MEM_ADDR_W> memAddr, int32_t memData, bool ht_noload orderChk)\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				break;
			case 7:
				routineName = "WriteMem_int64";

				g_appArgs.GetDsnRpt().AddLevel("void WriteMem_int64(sc_uint<MEM_ADDR_W> memAddr, int64_t memData, bool orderChk=true)\n");
				g_appArgs.GetDsnRpt().EndLevel();

				m_mifMacros.Append("void CPers%s%s::WriteMem_int64(sc_uint<MEM_ADDR_W> memAddr, int64_t memData, bool ht_noload orderChk)\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				break;
			}
			m_mifMacros.Append("#else\n");

			switch (memWrSize) {
			case 0:
				m_mifMacros.Append("void CPers%s%s::WriteMem_uint8_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr, uint8_t memData, bool orderChk)\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				break;
			case 1:
				m_mifMacros.Append("void CPers%s%s::WriteMem_uint16_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr, uint16_t memData, bool orderChk)\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				break;
			case 2:
				m_mifMacros.Append("void CPers%s%s::WriteMem_uint32_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr, uint32_t memData, bool orderChk)\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				break;
			case 3:
				m_mifMacros.Append("void CPers%s%s::WriteMem_uint64_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr, uint64_t memData, bool orderChk)\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				break;
			case 4:
				m_mifMacros.Append("void CPers%s%s::WriteMem_int8_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr, int8_t memData, bool orderChk)\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				break;
			case 5:
				m_mifMacros.Append("void CPers%s%s::WriteMem_int16_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr, int16_t memData, bool orderChk)\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				break;
			case 6:
				m_mifMacros.Append("void CPers%s%s::WriteMem_int32_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr, int32_t memData, bool orderChk)\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				break;
			case 7:
				m_mifMacros.Append("void CPers%s%s::WriteMem_int64_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr, int64_t memData, bool orderChk)\n",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
				break;
			}

			m_mifMacros.Append("#endif\n");
			m_mifMacros.Append("{\n");

			m_mifMacros.Append("\tassert_msg(c_t%d_bWriteMemAvail, \"Runtime check failed in CPers%s::%s()"
				" - expected WriteMemBusy() to have been called and not busy\");\n",
				mod.m_execStg, mod.m_modName.Uc().c_str(), routineName.c_str());
			m_mifMacros.NewLine();

			switch (memWrSize) {
			case 0:
			case 4:
				break;
			case 1:
			case 5:
				m_mifMacros.Append("\tassert_msg((memAddr & 1) == 0, \"Runtime check failed in CPers%s::%s()"
					" - expected address to be uint16 aligned\");\n", mod.m_modName.Uc().c_str(), routineName.c_str());
				break;
			case 2:
			case 6:
				m_mifMacros.Append("\tassert_msg((memAddr & 3) == 0, \"Runtime check failed in CPers%s::%s()"
					" - expected address to be uint32 aligned\");\n", mod.m_modName.Uc().c_str(), routineName.c_str());
				break;
			case 3:
			case 7:
				m_mifMacros.Append("\tassert_msg((memAddr & 7) == 0, \"Runtime check failed in CPers%s::%s()"
					" - expected address to be uint64 aligned\");\n", mod.m_modName.Uc().c_str(), routineName.c_str());
				break;
			}

			m_mifMacros.NewLine();

			if (mif.m_bMifRd)
				m_mifMacros.Append("\tc_t%d_bReadMem = false;\n", mod.m_execStg);
			if (bNeed_reqQwCnt)
				m_mifMacros.Append("\tc_t%d_reqQwCnt = 1;\n", mod.m_execStg);
			if (bNeed_reqQwSplit)
				m_mifMacros.Append("\tc_t%d_reqQwSplit = true;\n", mod.m_execStg);
			m_mifMacros.Append("\tc_t%d_%sToMif_reqRdy = true;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str());

			if (mif.m_bMifRd)
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_type = MEM_REQ_WR;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str());

			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_addr = memAddr;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str());
			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_data = memData;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str());

			switch (memWrSize) {
			case 0:
			case 4:
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_size = MEM_REQ_U8;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				break;
			case 1:
			case 5:
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_size = MEM_REQ_U16;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				break;
			case 2:
			case 6:
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_size = MEM_REQ_U32;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				break;
			case 3:
			case 7:
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_size = MEM_REQ_U64;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				break;
			}

			if (true || g_appArgs.IsMemTraceEnabled()) {
				m_mifMacros.Append("\t#ifndef _HTV\n");
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_file = file;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_line = line;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_time = sc_time_stamp().value();\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_orderChk = orderChk;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				m_mifMacros.Append("\t#endif\n");
			}
			char wrRspGrpId[32];
			if (mif.m_mifWr.m_rspGrpId.size() == 0)
				sprintf(wrRspGrpId, "r_t%d_htId", mod.m_execStg);
			else if (mif.m_mifWr.m_bRspGrpIdPriv)
				sprintf(wrRspGrpId, "r_t%d_htPriv.m_%s",
				mod.m_execStg, mif.m_mifWr.m_rspGrpId.AsStr().c_str());
			else
				sprintf(wrRspGrpId, "r_%s", mif.m_mifWr.m_rspGrpId.AsStr().c_str());

			if (mif.m_mifWr.m_rspGrpIdW > 0)
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_tid = (sc_uint<MIF_TID_W>)%s;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str(), wrRspGrpId);

			m_mifMacros.Append("}\n");
			m_mifMacros.NewLine();
		}

		///////////////////////////////////////////////////////////
		// generate WriteMem_var routines

		for (size_t srcIdx = 0; srcIdx < mif.m_mifWr.m_wrSrcList.size(); srcIdx += 1) {
			CMifWrSrc &wrSrc = mif.m_mifWr.m_wrSrcList[srcIdx];

			char srcName[64] = "";
			if (wrSrc.m_name.size() > 0)
				sprintf(srcName, "_%s", wrSrc.m_name.c_str());

			if (true || g_appArgs.IsMemTraceEnabled()) {
				m_mifFuncDecl.Append("\t#ifdef _HTV\n");
				m_mifMacros.Append("#ifdef _HTV\n");
			}

			g_appArgs.GetDsnRpt().AddLevel("void WriteMem%s(sc_uint<MEM_ADDR_W> memAddr",
				srcName);

			m_mifFuncDecl.Append("\tvoid WriteMem%s(sc_uint<MEM_ADDR_W> memAddr",
				srcName);

			m_mifMacros.Append("void CPers%s%s::WriteMem%s(sc_uint<MEM_ADDR_W> memAddr",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(),
				srcName);

			int argCnt = 1;
			if (wrSrc.m_pIntGbl && wrSrc.m_pIntGbl->m_addr1W.AsInt() > 0 && 
				(g_appArgs.IsGlobalWriteHtidEnabled() || wrSrc.m_pIntGbl->m_addr1Name != "htId"))
			{
				argCnt += 1;
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varAddr1", wrSrc.m_pIntGbl->m_addr1W.AsInt());
				m_mifFuncDecl.Append(", ht_uint%d varAddr1", wrSrc.m_pIntGbl->m_addr1W.AsInt());
				m_mifMacros.Append(", ht_uint%d varAddr1", wrSrc.m_pIntGbl->m_addr1W.AsInt());

			} else if (wrSrc.m_pExtRam && wrSrc.m_pExtRam->m_addr1W.AsInt() > 0 &&
				(g_appArgs.IsGlobalWriteHtidEnabled() || wrSrc.m_pExtRam->m_addr1Name != "htId"))
			{
				argCnt += 1;
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varAddr1", wrSrc.m_pExtRam->m_addr1W.AsInt());
				m_mifFuncDecl.Append(", ht_uint%d varAddr1", wrSrc.m_pExtRam->m_addr1W.AsInt());
				m_mifMacros.Append(", ht_uint%d varAddr1", wrSrc.m_pExtRam->m_addr1W.AsInt());

			} else if (wrSrc.m_pSharedVar && wrSrc.m_pSharedVar->m_addr1W.AsInt() > 0) {
				argCnt += 1;
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varAddr1", wrSrc.m_pSharedVar->m_addr1W.AsInt());
				m_mifFuncDecl.Append(", ht_uint%d varAddr1", wrSrc.m_pSharedVar->m_addr1W.AsInt());
				m_mifMacros.Append(", ht_uint%d varAddr1", wrSrc.m_pSharedVar->m_addr1W.AsInt());
			}

			if (wrSrc.m_pIntGbl && wrSrc.m_pIntGbl->m_addr2W.AsInt() > 0 &&
				(g_appArgs.IsGlobalWriteHtidEnabled() || wrSrc.m_pIntGbl->m_addr2Name != "htId"))
			{
				argCnt += 1;
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varAddr2", wrSrc.m_pIntGbl->m_addr2W.AsInt());
				m_mifFuncDecl.Append(", ht_uint%d varAddr2", wrSrc.m_pIntGbl->m_addr2W.AsInt());
				m_mifMacros.Append(", ht_uint%d varAddr2", wrSrc.m_pIntGbl->m_addr2W.AsInt());

			} else if (wrSrc.m_pExtRam && wrSrc.m_pExtRam->m_addr2W.AsInt() > 0 &&
				(g_appArgs.IsGlobalWriteHtidEnabled() || wrSrc.m_pExtRam->m_addr2Name != "htId"))
			{
				argCnt += 1;
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varAddr2", wrSrc.m_pExtRam->m_addr2W.AsInt());
				m_mifFuncDecl.Append(", ht_uint%d varAddr2", wrSrc.m_pExtRam->m_addr2W.AsInt());
				m_mifMacros.Append(", ht_uint%d varAddr2", wrSrc.m_pExtRam->m_addr2W.AsInt());

			} else if (wrSrc.m_pSharedVar && wrSrc.m_pSharedVar->m_addr2W.AsInt() > 0) {
				argCnt += 1;
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varAddr2", wrSrc.m_pSharedVar->m_addr2W.AsInt());
				m_mifFuncDecl.Append(", ht_uint%d varAddr2", wrSrc.m_pSharedVar->m_addr2W.AsInt());
				m_mifMacros.Append(", ht_uint%d varAddr2", wrSrc.m_pSharedVar->m_addr2W.AsInt());
			}

			if (wrSrc.m_varDimen1 > 0) {
				argCnt += 1;
				int varDimen1W = FindLg2(wrSrc.m_varDimen1-1);
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varIdx1", varDimen1W);
				m_mifFuncDecl.Append(", ht_uint%d varIdx1", varDimen1W);
				m_mifMacros.Append(", ht_uint%d varIdx1", varDimen1W);
			}
			if (wrSrc.m_varDimen2 > 0) {
				argCnt += 1;
				int varDimen2W = FindLg2(wrSrc.m_varDimen2-1);
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d varIdx2", varDimen2W);
				m_mifFuncDecl.Append(", ht_uint%d varIdx2", varDimen2W);
				m_mifMacros.Append(", ht_uint%d varIdx2", varDimen2W);
			}

			if (wrSrc.m_fldDimen1 > 0) {
				argCnt += 1;
				int fldDimen1W = FindLg2(wrSrc.m_fldDimen1-1);
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d fldIdx1", fldDimen1W);
				m_mifFuncDecl.Append(", ht_uint%d fldIdx1", fldDimen1W);
				m_mifMacros.Append(", ht_uint%d fldIdx1", fldDimen1W);
			}
			if (wrSrc.m_fldDimen2 > 0) {
				argCnt += 1;
				int fldDimen2W = FindLg2(wrSrc.m_fldDimen2-1);
				g_appArgs.GetDsnRpt().AddText(", ht_uint%d fldIdx2", fldDimen2W);
				m_mifFuncDecl.Append(", ht_uint%d fldIdx2", fldDimen2W);
				m_mifMacros.Append(", ht_uint%d fldIdx2", fldDimen2W);
			}
			if (wrSrc.m_bMultiWr) {
				argCnt += 1;
				g_appArgs.GetDsnRpt().AddText(", sc_uint<4> qwCnt");
				m_mifFuncDecl.Append(", sc_uint<4> qwCnt");
				m_mifMacros.Append(", sc_uint<4> qwCnt");
			}

			g_appArgs.GetDsnRpt().AddText(", bool orderChk=true);\n");
			m_mifFuncDecl.Append(", bool ht_noload orderChk=true);\n");
			m_mifMacros.Append(", bool ht_noload orderChk)\n");

			g_appArgs.GetDsnRpt().EndLevel();

			if (true || g_appArgs.IsMemTraceEnabled()) {
				m_mifFuncDecl.Append("\t#else\n");
				m_mifMacros.Append("#else\n");

				m_mifFuncDecl.Append("\t#define WriteMem%s(", srcName);
				m_mifFuncDecl.Append("...) WriteMem%s_(", srcName);
				m_mifFuncDecl.Append("(char *)__FILE__, __LINE__, __VA_ARGS__)\n");
				m_mifFuncDecl.Append("\tvoid WriteMem%s_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr",
					srcName);

				m_mifMacros.Append("void CPers%s%s::WriteMem%s_(char *file, int line, sc_uint<MEM_ADDR_W> memAddr",
					unitNameUc.c_str(), mod.m_modName.Uc().c_str(),
					srcName);

				if (wrSrc.m_pIntGbl && wrSrc.m_pIntGbl->m_addr1W.AsInt() > 0 &&
					(g_appArgs.IsGlobalWriteHtidEnabled() || wrSrc.m_pIntGbl->m_addr1Name != "htId"))
				{
					m_mifFuncDecl.Append(", ht_uint%d varAddr1", wrSrc.m_pIntGbl->m_addr1W.AsInt());
					m_mifMacros.Append(", ht_uint%d varAddr1", wrSrc.m_pIntGbl->m_addr1W.AsInt());

				} else if (wrSrc.m_pExtRam && wrSrc.m_pExtRam->m_addr1W.AsInt() > 0 &&
					(g_appArgs.IsGlobalWriteHtidEnabled() || wrSrc.m_pExtRam->m_addr1Name != "htId"))
				{
					m_mifFuncDecl.Append(", ht_uint%d varAddr1", wrSrc.m_pExtRam->m_addr1W.AsInt());
					m_mifMacros.Append(", ht_uint%d varAddr1", wrSrc.m_pExtRam->m_addr1W.AsInt());

				} else if (wrSrc.m_pSharedVar && wrSrc.m_pSharedVar->m_addr1W.AsInt() > 0) {
					m_mifFuncDecl.Append(", ht_uint%d varAddr1", wrSrc.m_pSharedVar->m_addr1W.AsInt());
					m_mifMacros.Append(", ht_uint%d varAddr1", wrSrc.m_pSharedVar->m_addr1W.AsInt());
				}

				if (wrSrc.m_pIntGbl && wrSrc.m_pIntGbl->m_addr2W.AsInt() > 0 && 
					(g_appArgs.IsGlobalWriteHtidEnabled() || wrSrc.m_pIntGbl->m_addr2Name != "htId"))
				{
					m_mifFuncDecl.Append(", ht_uint%d varAddr2", wrSrc.m_pIntGbl->m_addr2W.AsInt());
					m_mifMacros.Append(", ht_uint%d varAddr2", wrSrc.m_pIntGbl->m_addr2W.AsInt());

				} else if (wrSrc.m_pExtRam && wrSrc.m_pExtRam->m_addr2W.AsInt() > 0 &&
					(g_appArgs.IsGlobalWriteHtidEnabled() || wrSrc.m_pExtRam->m_addr2Name != "htId"))
				{
					m_mifFuncDecl.Append(", ht_uint%d varAddr2", wrSrc.m_pExtRam->m_addr2W.AsInt());
					m_mifMacros.Append(", ht_uint%d varAddr2", wrSrc.m_pExtRam->m_addr2W.AsInt());

				} else if (wrSrc.m_pSharedVar && wrSrc.m_pSharedVar->m_addr2W.AsInt() > 0) {
					m_mifFuncDecl.Append(", ht_uint%d varAddr2", wrSrc.m_pSharedVar->m_addr2W.AsInt());
					m_mifMacros.Append(", ht_uint%d varAddr2", wrSrc.m_pSharedVar->m_addr2W.AsInt());
				}

				if (wrSrc.m_varDimen1 > 0) {
					int varDimen1W = FindLg2(wrSrc.m_varDimen1-1);
					m_mifFuncDecl.Append(", ht_uint%d varIdx1", varDimen1W);
					m_mifMacros.Append(", ht_uint%d varIdx1", varDimen1W);
				}
				if (wrSrc.m_varDimen2 > 0) {
					int varDimen2W = FindLg2(wrSrc.m_varDimen2-1);
					m_mifFuncDecl.Append(", ht_uint%d varIdx2", varDimen2W);
					m_mifMacros.Append(", ht_uint%d varIdx2", varDimen2W);
				}

				if (wrSrc.m_fldDimen1 > 0) {
					int fldDimen1W = FindLg2(wrSrc.m_fldDimen1-1);
					m_mifFuncDecl.Append(", ht_uint%d fldIdx1", fldDimen1W);
					m_mifMacros.Append(", ht_uint%d fldIdx1", fldDimen1W);
				}
				if (wrSrc.m_fldDimen2 > 0) {
					int fldDimen2W = FindLg2(wrSrc.m_fldDimen2-1);
					m_mifFuncDecl.Append(", ht_uint%d fldIdx2", fldDimen2W);
					m_mifMacros.Append(", ht_uint%d fldIdx2", fldDimen2W);
				}
				if (wrSrc.m_bMultiWr) {
					m_mifFuncDecl.Append(", sc_uint<4> qwCnt");
					m_mifMacros.Append(", sc_uint<4> qwCnt");
				}

				m_mifFuncDecl.Append(", bool orderChk=true);\n");
				m_mifMacros.Append(", bool orderChk)\n");

				m_mifFuncDecl.Append("\t#endif\n");
				m_mifMacros.Append("#endif\n");
			}

			m_mifMacros.Append("{\n");

			m_mifMacros.Append("\tassert_msg(c_t%d_bWriteMemAvail, \"Runtime check failed in CPers%s::WriteMem%s()"
				" - expected WriteMemBusy() to have been called and not busy\");\n",
				mod.m_execStg, mod.m_modName.Uc().c_str(), srcName);
			m_mifMacros.NewLine();

			if (wrSrc.m_pNgvRam == 0) {
				m_mifMacros.Append("\tassert_msg((memAddr & 7) == 0, \"Runtime check failed in CPers%s::WriteMem%s()"
					" - expected address to be uint64 aligned\");\n", mod.m_modName.Uc().c_str(), srcName);
			} else {
				switch (wrSrc.m_fldW) {
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
			}
			m_mifMacros.NewLine();

			if (wrSrc.m_bMultiWr) {
				m_mifMacros.Append("\tassert_msg(qwCnt > 0 && qwCnt <= 8, \"Runtime check failed in CPers%s::WriteMem%s()"
					" - expected multi-qw access to be within 64B memory line\");\n", mod.m_modName.Uc().c_str(), srcName);
				m_mifMacros.Append("\tassert_msg(((memAddr >> 3) & 7) + qwCnt <= 8, \"Runtime check failed in CPers%s::WriteMem%s()"
					" - expected multi-qw access to be within 64B memory line\");\n", mod.m_modName.Uc().c_str(), srcName);
				m_mifMacros.NewLine();
			}

			char const * pTidQwCntM1 = "(qwCnt-1)";
			if (bNeed_reqQwSplit && wrSrc.m_bMultiWr && wrSrc.m_memDst == "host") {
				m_mifMacros.Append("\tht_uint3 tidQwCntM1 = qwCnt != 8 ? 0 : 7;\n\n");
				pTidQwCntM1 = "tidQwCntM1";
			}

			if (mif.m_bMifRd)
				m_mifMacros.Append("\tc_t%d_bReadMem = false;\n", mod.m_execStg);
			if (bNeed_reqQwCnt)
				m_mifMacros.Append("\tc_t%d_reqQwCnt = qwCnt;\n", mod.m_execStg);
			if (bNeed_reqQwSplit)
				m_mifMacros.Append("\tc_t%d_reqQwSplit = true;\n", mod.m_execStg);
			m_mifMacros.Append("\tc_t%d_%sToMif_reqRdy = true;\n", mod.m_execStg, mod.m_modName.Lc().c_str());

			if (wrSrc.m_bMultiWr)
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_host = %s;\n", mod.m_execStg, mod.m_modName.Lc().c_str(),
				wrSrc.m_memDst == "host" ? "true" : "false");

			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_type = MEM_REQ_WR;\n", mod.m_execStg, mod.m_modName.Lc().c_str());

			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_addr = memAddr;\n", mod.m_execStg, mod.m_modName.Lc().c_str());

			if (wrSrc.m_pNgvRam == 0) {
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_size = MEM_REQ_U64;\n", mod.m_execStg, mod.m_modName.Lc().c_str());
			} else {
				switch (wrSrc.m_fldW) {
				case 8:
					m_mifMacros.Append("\tc_t%d_%sToMif_req.m_size = MEM_REQ_U8;\n", mod.m_execStg, mod.m_modName.Lc().c_str());
					break;
				case 16:
					m_mifMacros.Append("\tc_t%d_%sToMif_req.m_size = MEM_REQ_U16;\n", mod.m_execStg, mod.m_modName.Lc().c_str());
					break;
				case 32:
					m_mifMacros.Append("\tc_t%d_%sToMif_req.m_size = MEM_REQ_U32;\n", mod.m_execStg, mod.m_modName.Lc().c_str());
					break;
				case 64:
					m_mifMacros.Append("\tc_t%d_%sToMif_req.m_size = MEM_REQ_U64;\n", mod.m_execStg, mod.m_modName.Lc().c_str());
					break;
				default:
					break;
				}
			}

			m_mifMacros.Append("\tc_t%d_%sToMif_req.m_tid = (sc_uint<MIF_TID_W>)(\n", mod.m_execStg, mod.m_modName.Lc().c_str());

			char const * pWrRspGrpW = mif.m_mifWr.m_rspGrpId.size() == 0 ? "_HTID_W" : "_WR_GRP_ID_W";

			if (wrSrc.m_bMultiWr && (is_wx || wrSrc.m_memDst == "host" && is_hc2)) {

				m_mifMacros.Append("\t\t((%s & MIF_TID_QWCNT_MSK) << MIF_TID_QWCNT_SHF) |\n",
					pTidQwCntM1);
			}

			if (wrSrc.m_srcW > 0) {

				if (wrSrc.m_varAddr0W > 0) {
					m_mifMacros.Append("\t\t(r_t%d_htId << (%s_MIF_SRC_%s_VAR_ADDR0_SHF + 1 + %s%s)) |\n",
						mod.m_execStg,
						mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), pWrRspGrpW);
				}

				if (wrSrc.m_pIntGbl && wrSrc.m_pIntGbl->m_addr1W.AsInt() > 0
					|| wrSrc.m_pExtRam && wrSrc.m_pExtRam->m_addr1W.AsInt() > 0
					|| wrSrc.m_pSharedVar && wrSrc.m_pSharedVar->m_addr1W.AsInt() > 0)
				{
					if (wrSrc.m_pIntGbl && wrSrc.m_pIntGbl->m_addr1W.AsInt() > 0 &&
						(!g_appArgs.IsGlobalWriteHtidEnabled() && wrSrc.m_pIntGbl->m_addr1Name == "htId"))

						m_mifMacros.Append("\t\t(r_t%d_htId << (%s_MIF_SRC_%s_VAR_ADDR1_SHF + 1 + %s%s)) |\n",
							mod.m_execStg,
							mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(),
							mod.m_modName.Upper().c_str(), pWrRspGrpW);

					else if (wrSrc.m_pExtRam && wrSrc.m_pExtRam->m_addr1W.AsInt() > 0 &&
						(!g_appArgs.IsGlobalWriteHtidEnabled() && wrSrc.m_pExtRam->m_addr1Name == "htId"))

						m_mifMacros.Append("\t\t(r_t%d_htId << (%s_MIF_SRC_%s_VAR_ADDR1_SHF + 1 + %s%s)) |\n",
							mod.m_execStg,
							mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(),
							mod.m_modName.Upper().c_str(), pWrRspGrpW);

					else
						m_mifMacros.Append("\t\t(varAddr1 << (%s_MIF_SRC_%s_VAR_ADDR1_SHF + 1 + %s%s)) |\n",
							mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(),
							mod.m_modName.Upper().c_str(), pWrRspGrpW);
				}

				if (wrSrc.m_pIntGbl && wrSrc.m_pIntGbl->m_addr2W.AsInt() > 0
					|| wrSrc.m_pExtRam && wrSrc.m_pExtRam->m_addr2W.AsInt() > 0
					|| wrSrc.m_pSharedVar && wrSrc.m_pSharedVar->m_addr2W.AsInt() > 0)
				{
					if (wrSrc.m_pIntGbl && wrSrc.m_pIntGbl->m_addr2W.AsInt() > 0 &&
						(!g_appArgs.IsGlobalWriteHtidEnabled() && wrSrc.m_pIntGbl->m_addr2Name == "htId"))

						m_mifMacros.Append("\t\t(r_t%d_htId << (%s_MIF_SRC_%s_VAR_ADDR2_SHF + 1 + %s%s)) |\n",
							mod.m_execStg,
							mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(),
							mod.m_modName.Upper().c_str(), pWrRspGrpW);

					else if (wrSrc.m_pExtRam && wrSrc.m_pExtRam->m_addr2W.AsInt() > 0 && 
						(!g_appArgs.IsGlobalWriteHtidEnabled() && wrSrc.m_pExtRam->m_addr2Name == "htId"))

						m_mifMacros.Append("\t\t(r_t%d_htId << (%s_MIF_SRC_%s_VAR_ADDR2_SHF + 1 + %s%s)) |\n",
							mod.m_execStg,
							mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(),
							mod.m_modName.Upper().c_str(), pWrRspGrpW);

					else
						m_mifMacros.Append("\t\t(varAddr2 << (%s_MIF_SRC_%s_VAR_ADDR2_SHF + 1 + %s%s)) |\n",
							mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(),
							mod.m_modName.Upper().c_str(), pWrRspGrpW);
				}

				if (wrSrc.m_varDimen1 > 0)
					m_mifMacros.Append("\t\t(varIdx1 << (%s_MIF_SRC_%s_VAR_IDX1_SHF + 1 + %s%s)) |\n",
					mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(),
					mod.m_modName.Upper().c_str(), pWrRspGrpW);

				if (wrSrc.m_varDimen2 > 0)
					m_mifMacros.Append("\t\t(varIdx2 << (%s_MIF_SRC_%s_VAR_IDX2_SHF + 1 + %s%s)) |\n",
					mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(),
					mod.m_modName.Upper().c_str(), pWrRspGrpW);

				if (wrSrc.m_fldDimen1 > 0)
					m_mifMacros.Append("\t\t(fldIdx1 << (%s_MIF_SRC_%s_FLD_IDX1_SHF + 1 + %s%s)) |\n",
					mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(),
					mod.m_modName.Upper().c_str(), pWrRspGrpW);

				if (wrSrc.m_fldDimen2 > 0)
					m_mifMacros.Append("\t\t(fldIdx2 << (%s_MIF_SRC_%s_FLD_IDX2_SHF + 1 + %s%s)) |\n",
					mod.m_modName.Upper().c_str(), wrSrc.m_name.Upper().c_str(),
					mod.m_modName.Upper().c_str(), pWrRspGrpW);
			}

			m_mifMacros.Append("\t\t(%d << (%s_MIF_SRC_IDX_SHF + 1 + %s%s)) |\n",
				srcIdx+1, mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str(), pWrRspGrpW);

			m_mifMacros.Append("\t\t(0 << %s%s)",
				mod.m_modName.Upper().c_str(), pWrRspGrpW);

			if (mif.m_mifWr.m_rspGrpIdW == 0)
				m_mifMacros.Append(");\n");
			else if (mif.m_mifWr.m_rspGrpId.size() == 0)
				m_mifMacros.Append(" |\n\t\tr_t%d_htId);\n", mod.m_execStg);
			else if (mif.m_mifWr.m_bRspGrpIdPriv)
				m_mifMacros.Append(" |\n\t\tr_t%d_htPriv.m_%s);\n",
				mod.m_execStg, mif.m_mifWr.m_rspGrpId.AsStr().c_str());
			else
				m_mifMacros.Append(" |\n\t\tr_%s);\n", mif.m_mifWr.m_rspGrpId.AsStr().c_str());

			if (true || g_appArgs.IsMemTraceEnabled()) {
				m_mifMacros.Append("\t#ifndef _HTV\n");
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_file = file;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_line = line;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_time = sc_time_stamp().value();\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				m_mifMacros.Append("\tc_t%d_%sToMif_req.m_orderChk = orderChk;\n",
					mod.m_execStg, mod.m_modName.Lc().c_str());
				m_mifMacros.Append("\t#endif\n");
			}
			m_mifMacros.Append("}\n");
			m_mifMacros.NewLine();
		}

		///////////////////////////////////////////////////////////
		// generate WriteMemPause routine

		if (mif.m_mifWr.m_bPause) {

			g_appArgs.GetDsnRpt().AddLevel("WriteMemPause(ht_uint%d rsmInstr)\n", mod.m_instrW);
			g_appArgs.GetDsnRpt().EndLevel();

			m_mifFuncDecl.Append("\tvoid WriteMemPause(ht_uint%d rsmInstr);\n", mod.m_instrW);
			m_mifMacros.Append("void CPers%s%s::WriteMemPause(ht_uint%d rsmInstr)\n",
				unitNameUc.c_str(), mod.m_modName.Uc().c_str(), mod.m_instrW);
			m_mifMacros.Append("{\n");

			m_mifMacros.Append("\tassert_msg(c_t%d_bWriteMemAvail, \"Runtime check failed in CPers%s::WriteMemPause()"
				" - expected WriteMemBusy() to have been called and not busy\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());
			m_mifMacros.NewLine();

			m_mifMacros.Append("\tassert_msg(c_t%d_htCtrl == HT_INVALID, \"Runtime check failed in CPers%s::WriteMemPause()"
				" - an Ht control routine was already called\");\n", mod.m_execStg, mod.m_modName.Uc().c_str());
			m_mifMacros.NewLine();

			if (mif.m_mifWr.m_rspGrpIdW == 0) {

				m_mifMacros.Append("\tc_t%d_memWritePause = true;\n", mod.m_execStg);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tif (r_t%d_wrGrpRspCntZero && !(c_t%d_%sToMif_reqRdy",
					mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str());
				if (mod.m_mif.m_bMifRd)
					m_mifMacros.Append(" && !c_t%d_bReadMem))\n", mod.m_execStg);
				else
					m_mifMacros.Append("))\n");

				m_mifMacros.Append("\t\tHtContinue(rsmInstr);\n");
				m_mifMacros.Append("\telse {\n");
				m_mifMacros.Append("\t\t// Should not occur - verify waiting flag not already set\n");
				m_mifMacros.Append("\t\tassert(c_wrGrpRsmWait == false);\n");
				m_mifMacros.Append("\t\tc_wrGrpRsmWait = true;\n");
				m_mifMacros.Append("\t\tc_wrGrpRsmInstr = rsmInstr;\n");

				if (mif.m_mifWr.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\t\tc_wrGrpRsmHtId = r_t%d_htId;\n", mod.m_execStg);

				m_mifMacros.Append("\t\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
				m_mifMacros.Append("\t\tc_t%d_htNextInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.Append("\t}\n");

			} else if (mif.m_mifWr.m_rspGrpIdW <= 2) {

				m_mifMacros.Append("\tc_t%d_memWritePause = true;\n", mod.m_execStg);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tif (r_t%d_wrGrpRspCntZero && !(c_t%d_%sToMif_reqRdy",
					mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str());

				if (mod.m_mif.m_bMifRd)
					m_mifMacros.Append(" && !c_t%d_bReadMem))\n", mod.m_execStg);
				else
					m_mifMacros.Append("))\n");

				m_mifMacros.Append("\t\tHtContinue(rsmInstr);\n");
				m_mifMacros.Append("\telse {\n");
				m_mifMacros.Append("\t\t// should not occur - verify waiting flag not already set\n");
				m_mifMacros.Append("\t\tassert(c_wrGrpRsmWait[INT(%s)] == false);\n", wrRspGrpName);
				m_mifMacros.Append("\t\tc_wrGrpRsmWait[INT(%s)] = true;\n", wrRspGrpName);
				m_mifMacros.Append("\t\tc_wrGrpRsmInstr[INT(%s)] = rsmInstr;\n", wrRspGrpName);

				if (mif.m_mifWr.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifMacros.Append("\t\tc_wrGrpRsmHtId[INT(%s)] = r_t%d_htId;\n",
					wrRspGrpName, mod.m_execStg);

				m_mifMacros.Append("\t\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
				m_mifMacros.Append("\t\tc_t%d_htNextInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.Append("\t}\n");

			} else {
				m_mifMacros.Append("\tm_wrGrpRsmWait[%s & %s].write_addr(%s >> %s);\n",
					wrRspGrpName, pWrGrpIdMask, wrRspGrpName, pWrGrpShfAmt);
				m_mifMacros.Append("\tm_wrGrpRsmInstr[%s & %s].write_addr(%s >> %s);\n",
					wrRspGrpName, pWrGrpIdMask, wrRspGrpName, pWrGrpShfAmt);
				if (mif.m_mifWr.m_rspGrpId.size() != 0)
					m_mifMacros.Append("\tm_wrGrpRsmHtId[%s & %s].write_addr(%s >> %s);\n",
					wrRspGrpName, pWrGrpIdMask, wrRspGrpName, pWrGrpShfAmt);
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tif (r_t%d_wrGrpRspCntZero && !(c_t%d_%sToMif_reqRdy",
					mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str());

				if (mod.m_mif.m_bMifRd)
					m_mifMacros.Append(" && !c_t%d_bReadMem))\n", mod.m_execStg);
				else
					m_mifMacros.Append("))\n");

				m_mifMacros.Append("\t\tHtContinue(rsmInstr);\n");
				m_mifMacros.Append("\telse {\n");
				m_mifMacros.Append("\t\tm_wrGrpRsmWait[%s & %s].write_mem(true);\n", wrRspGrpName, pWrGrpIdMask);
				m_mifMacros.Append("\t\tm_wrGrpRsmInstr[%s & %s].write_mem(rsmInstr);\n", wrRspGrpName, pWrGrpIdMask);
				if (mif.m_mifWr.m_rspGrpId.size() != 0)
					m_mifMacros.Append("\t\tm_wrGrpRsmHtId[%s & %s].write_mem(r_t%d_htId);\n",
					wrRspGrpName, pWrGrpIdMask, mod.m_execStg);
				m_mifMacros.Append("\t\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
				m_mifMacros.Append("\t\tc_t%d_htNextInstr = rsmInstr;\n", mod.m_execStg);
				m_mifMacros.Append("\t}\n");
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

			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				m_mifMacros.Append("\tCHtCmd c_wrpCmd;\n");
				m_mifMacros.Append("\tc_wrpCmd.m_htId = r_t%d_htId;\n", mod.m_execStg);
				m_mifMacros.Append("\tc_wrpCmd.m_htInstr = rsmInstr;\n");
				m_mifMacros.NewLine();

				m_mifMacros.Append("\tm_wrpRsmQue.push(c_wrpCmd);\n");
				m_mifMacros.NewLine();
			}

			m_mifMacros.Append("\tc_t%d_htCtrl = HT_PAUSE;\n", mod.m_execStg);
			m_mifMacros.Append("\tc_t%d_bWrpRsm = true;\n", mod.m_execStg);

			if (mod.m_threads.m_htIdW.AsInt() == 0)
				m_mifMacros.Append("\t\tc_t%d_htNextInstr = rsmInstr;\n", mod.m_execStg);

			m_mifMacros.Append("}\n");
			m_mifMacros.NewLine();
		}
	}

	g_appArgs.GetDsnRpt().EndLevel();

	bool bMifWr = false;
	int mifRsmCnt = 0;

	if (mod.m_threads.m_bPause)
		mifRsmCnt += 1;

	if (mif.m_bMifRd && mif.m_mifRd.m_bPause) {
		mifRsmCnt += 1;
	}
	if (mif.m_bMifWr && mif.m_mifWr.m_bPause) {
		bMifWr = true;
		mifRsmCnt += 1;
	}

	m_mifRamDecl.Append("\n\t// Memory Interface State\n");

	if (!mif.m_bSingleReqMode) {

		m_mifRamDecl.Append("\t#ifndef _HTV\n");
		m_mifRamDecl.Append("\tint m_mif_rdReqFirstCnt[1 << %s_HTID_W];\n",
			mod.m_modName.Upper().c_str());
		m_mifRamDecl.Append("\tint m_mif_rdPrefReqCnt[1 << %s_HTID_W];\n",
			mod.m_modName.Upper().c_str());
		m_mifRamDecl.Append("\tint m_mif_rdPrefMemLength[1 << %s_HTID_W];\n",
			mod.m_modName.Upper().c_str());
		m_mifRamDecl.Append("\t#endif\n");
		m_mifRamDecl.NewLine();

		m_mifRamDecl.Append("\tstruct CMifReq {\n");
		m_mifRamDecl.Append("\t\t#ifndef _HTV\n");
		m_mifRamDecl.Append("\t\tCMifReq() {}\n");
		m_mifRamDecl.Append("\t\t#endif\n");
		if (mod.m_threads.m_htIdW.AsInt() > 0)
			m_mifRamDecl.Append("\t\tuint64_t\tm_htId : %s_HTID_W;\n", mod.m_modName.Upper().c_str());
		m_mifRamDecl.Append("\t\tuint64_t\tm_type : 2;\n");
		m_mifRamDecl.Append("\t\tuint64_t\tm_addr : MEM_ADDR_W;\n");
		if (mif.m_bMifWr)
			m_mifRamDecl.Append("\t\tuint64_t\tm_data : MEM_DATA_W;\n");
		if (mif.m_bMifRd && mif.m_mifRd.m_rdDstList.size() > 1)
			m_mifRamDecl.Append("\t\tuint64_t\tm_dst : %s_MIF_DST_W;\n", mod.m_modName.Upper().c_str());

		m_mifRamDecl.Append("\t};\n");
		m_mifRamDecl.NewLine();
	}

	if (mif.m_bMifRd && mif.m_bMifWr) {
		m_mifDecl.Append("\tbool c_t%d_bReadMem;\n", mod.m_execStg);
		mifPreInstr.Append("\tc_t%d_bReadMem = false;\n", mod.m_execStg);
	}

	// Initialize memory request fields to zero
	m_mifDecl.Append("\tbool c_t%d_%sToMif_reqRdy;\n", mod.m_execStg, mod.m_modName.Lc().c_str());
	m_mifDecl.Append("\tCMemRdWrReqIntf c_t%d_%sToMif_req;\n",
		mod.m_execStg, mod.m_modName.Lc().c_str());
	if (mif.m_mifRd.m_bMultiRd || mif.m_mifWr.m_bMultiWr) {
		m_mifDecl.Append("\tht_uint4 c_t%d_reqQwCnt;\n", mod.m_execStg);
		if (bNeed_reqQwSplit)
			m_mifDecl.Append("\tbool c_t%d_reqQwSplit;\n", mod.m_execStg);

		mifPreInstr.Append("\t// Memory write request\n");
		if (bNeed_reqBusy) {
			mifPreInstr.Append("\tif (r_t%d_reqBusy) {\n", mod.m_execStg);
			mifPreInstr.Append("\t\tc_t%d_%sToMif_reqRdy = true;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str());
			mifPreInstr.Append("\t\tc_t%d_%sToMif_req = r_t%d_%sToMif_req;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str(), mod.m_execStg+1, mod.m_modName.Lc().c_str());
			if (bNeed_reqQwCnt)
				mifPreInstr.Append("\t\tc_t%d_reqQwCnt = r_t%d_reqQwCnt;\n", mod.m_execStg, mod.m_execStg+1);
			if (bNeed_reqQwSplit)
				mifPreInstr.Append("\t\tc_t%d_reqQwSplit = r_t%d_reqQwSplit;\n", mod.m_execStg, mod.m_execStg+1);
			if (mif.m_mifWr.m_bReqPause) {
				m_mifDecl.Append("\tbool c_t%d_bWrpRsm;\n", mod.m_execStg);
				mifPreInstr.Append("\t\tc_t%d_bWrpRsm = r_t%d_bWrpRsm;\n", mod.m_execStg, mod.m_execStg+1);
			}
			mifPreInstr.Append("\t} else {\n");
			mifPreInstr.Append("\t\tc_t%d_%sToMif_reqRdy = false;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str());
			mifPreInstr.Append("\t\tc_t%d_%sToMif_req = 0;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str());
			if (bNeed_reqQwCnt)
				mifPreInstr.Append("\t\tc_t%d_reqQwCnt = 0;\n", mod.m_execStg);
			if (bNeed_reqQwSplit)
				mifPreInstr.Append("\t\tc_t%d_reqQwSplit = false;\n", mod.m_execStg);
			if (mif.m_mifWr.m_bReqPause)
				mifPreInstr.Append("\t\tc_t%d_bWrpRsm = false;\n", mod.m_execStg);
			mifPreInstr.Append("\t}\n");
		} else {
			mifPreInstr.Append("\tc_t%d_%sToMif_reqRdy = false;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str());
			mifPreInstr.Append("\tc_t%d_%sToMif_req = 0;\n",
				mod.m_execStg, mod.m_modName.Lc().c_str());
			if (bNeed_reqQwCnt)
				mifPreInstr.Append("\tc_t%d_reqQwCnt = 0;\n", mod.m_execStg);
			if (bNeed_reqQwSplit)
				mifPreInstr.Append("\tc_t%d_reqQwSplit = false;\n", mod.m_execStg);
		}

		if (bNeed_reqQwIdx)
			mifPreInstr.Append("\tht_uint4 c_t%d_reqQwCnt = r_t%d_reqQwCnt;\n", mod.m_execStg+1, mod.m_execStg+1);
		if (bNeed_reqQwSplit)
			mifPreInstr.Append("\tbool c_t%d_reqQwSplit = r_t%d_reqQwSplit;\n", mod.m_execStg+1, mod.m_execStg+1);
		if (mif.m_mifWr.m_bReqPause)
			mifPreInstr.Append("bool\tc_t%d_bWrpRsm = r_t%d_bWrpRsm;\n", mod.m_execStg+1, mod.m_execStg+1);
	} else {
		mifPreInstr.Append("\tc_t%d_%sToMif_reqRdy = false;\n", mod.m_execStg, mod.m_modName.Lc().c_str());
		mifPreInstr.Append("\tc_t%d_%sToMif_req = 0;\n", mod.m_execStg, mod.m_modName.Lc().c_str());
	}
	mifPreInstr.NewLine();

	m_mifAvlCntChk.Append("\t\tassert(r_%sToMif_reqAvlCnt == %d);\n",
		mod.m_modName.Lc().c_str(), 1 << mif.m_queueW);

	m_mifDecl.Append("\tht_uint%d r_%sToMif_reqAvlCnt;\n",
		mif.m_queueW+1, mod.m_modName.Lc().c_str());
	m_mifDecl.Append("\tbool r_%sToMif_reqAvlCntBusy;\n",
		mod.m_modName.Lc().c_str());
	m_mifDecl.NewLine();

	char reqBusy[32];
	if (bNeed_reqBusy)
		sprintf(reqBusy, " || r_t%d_reqBusy", mod.m_execStg);
	else
		reqBusy[0] = '\0';


	if (mif.m_bMifRd) {
		string mifRdMemBusy = mif.m_bMifWr ? VA(" !c_t%d_bReadMem ||", mod.m_execStg) : "";

		mifPostInstr.Append("\tassert_msg(c_reset1x || !c_t%d_%sToMif_reqRdy%s ||%s c_t%d_bReadMemAvail,"
			" \"Runtime check failed in CPers%s::Pers%s_%s()"
			" - expected ReadMemBusy() to have been called and not busy\");\n",
			mod.m_execStg, mod.m_modName.Lc().c_str(), reqBusy, mifRdMemBusy.c_str(), mod.m_execStg, mod.m_modName.Uc().c_str(),
			mod.m_modName.Uc().c_str(), mod.m_clkRate == eClk1x ? "1x" : "2x");
	}
	
	if (mif.m_bMifWr) {
		string mifWrMemBusy = mif.m_bMifRd ? VA(" c_t%d_bReadMem ||", mod.m_execStg) : "";

		mifPostInstr.Append("\tassert_msg(c_reset1x || !c_t%d_%sToMif_reqRdy%s ||%s c_t%d_bWriteMemAvail,"
			" \"Runtime check failed in CPers%s::Pers%s_%s()"
			" - expected WriteMemBusy() to have been called and not busy\");\n",
			mod.m_execStg, mod.m_modName.Lc().c_str(), reqBusy, mifWrMemBusy.c_str(), mod.m_execStg, mod.m_modName.Uc().c_str(),
			mod.m_modName.Uc().c_str(), mod.m_clkRate == eClk1x ? "1x" : "2x");
	}

	mifPostInstr.NewLine();

	if (mif.m_mifWr.m_bReqPause) {
		if (mif.m_bMifRd)
			mifPostInstr.Append("\tassert_msg(c_reset1x || !c_t%d_bWrpRsm || c_t%d_%sToMif_reqRdy && !c_t%d_bReadMem,\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), mod.m_execStg);
		else
			mifPostInstr.Append("\tassert_msg(c_reset1x || !c_t%d_bWrpRsm || c_t%d_%sToMif_reqRdy,\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str());

		mifPostInstr.Append("\t\t\"Runtime check failed in CPers%s::Pers%s_%s()"
			" - expected WriteMemPause() to be called in an Ht Instruction with a memory write call\");\n",
			mod.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), mod.m_clkRate == eClk1x ? "1x" : "2x");

		mifPostInstr.NewLine();
	}

	char notReqBusy[32];
	if (bNeed_reqBusy)
		sprintf(notReqBusy, " && !r_t%d_reqBusy", mod.m_execStg);
	else
		notReqBusy[0] = '\0';

	mifPostInstr.Append("\tht_uint%d\tc_%sToMif_reqAvlCnt = r_%sToMif_reqAvlCnt;\n",
		mif.m_queueW+1,
		mod.m_modName.Lc().c_str(),
		mod.m_modName.Lc().c_str());

	if (mif.m_mifWr.m_bMultiWr || mif.m_mifRd.m_bMultiRd) {
		mifPostInstr.Append("\tif (c_t%d_%sToMif_reqRdy%s)\n",
			mod.m_execStg, mod.m_modName.Lc().c_str(), notReqBusy);

		if (is_wx) {
			if (mif.m_bMifRd && mif.m_bMifWr)
				mifPostInstr.Append("\t\tc_%sToMif_reqAvlCnt -= c_t%d_bReadMem ? (ht_uint4)1 : c_t%d_reqQwCnt;\n",
				mod.m_modName.Lc().c_str(), mod.m_execStg, mod.m_execStg);
			else if (mif.m_bMifRd)
				mifPostInstr.Append("\t\tc_%sToMif_reqAvlCnt -= 1;\n",
				mod.m_modName.Lc().c_str(), mod.m_execStg, mod.m_execStg);
			else
				mifPostInstr.Append("\t\tc_%sToMif_reqAvlCnt -= c_t%d_reqQwCnt;\n",
				mod.m_modName.Lc().c_str(), mod.m_execStg, mod.m_execStg);
		} else if (bNeed_reqQwSplit)
			mifPostInstr.Append("\t\tc_%sToMif_reqAvlCnt -= c_t%d_reqQwSplit ? c_t%d_reqQwCnt : (ht_uint4)1;\n",
			mod.m_modName.Lc().c_str(), mod.m_execStg, mod.m_execStg);
		else
			mifPostInstr.Append("\t\tc_%sToMif_reqAvlCnt -= c_t%d_reqQwCnt;\n",
			mod.m_modName.Lc().c_str(), mod.m_execStg);

		mifPostInstr.Append("\tif (i_mifTo%sP0_reqAvl)\n",
			mod.m_modName.Uc().c_str());
		mifPostInstr.Append("\t\tc_%sToMif_reqAvlCnt += 1;\n",
			mod.m_modName.Lc().c_str());

		mifPostInstr.Append("\tbool c_%sToMif_reqAvlCntBusy = c_%sToMif_reqAvlCnt < 8;\n",
			mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());
	} else {
		mifPostInstr.Append("\tbool c_%sToMif_reqAvlCntBusy = r_%sToMif_reqAvlCntBusy;\n",
			mod.m_modName.Lc().c_str(), mod.m_modName.Lc().c_str());

		mifPostInstr.Append("\tif ((c_t%d_%sToMif_reqRdy%s) != i_mifTo%sP0_reqAvl) {\n",
			mod.m_execStg, mod.m_modName.Lc().c_str(),
			notReqBusy,
			mod.m_modName.Uc().c_str());

		if (mif.m_queueW > 0) {
			mifPostInstr.Append("\t\tc_%sToMif_reqAvlCnt = r_%sToMif_reqAvlCnt + (i_mifTo%sP0_reqAvl.read() ? 1 : -1);\n",
				mod.m_modName.Lc().c_str(),
				mod.m_modName.Lc().c_str(),
				mod.m_modName.Uc().c_str());
			mifPostInstr.Append("\t\tc_%sToMif_reqAvlCntBusy = r_%sToMif_reqAvlCnt == 1 && !i_mifTo%sP0_reqAvl.read();\n",
				mod.m_modName.Lc().c_str(),
				mod.m_modName.Lc().c_str(),
				mod.m_modName.Uc().c_str());
		} else
			mifPostInstr.Append("\t\tc_%sToMif_reqAvlCnt = r_%sToMif_reqAvlCnt + 1ul;\n",
			mod.m_modName.Lc().c_str(),
			mod.m_modName.Lc().c_str());

		mifPostInstr.Append("\t}\n");
	}
	mifPostInstr.NewLine();

	if (mif.m_bMifRd) {

		if (mif.m_mifRd.m_rspGrpIdW == 0) {
			// using single register

			m_mifDecl.Append("\tsc_uint<%s_RD_RSP_CNT_W> r_rdGrpRspCnt;\n",
				mod.m_modName.Upper().c_str());

			if (mif.m_mifRd.m_bPause) {
				m_mifDecl.Append("\tbool r_rdGrpRsmWait;\n");
				m_mifDecl.Append("\tsc_uint<%s_INSTR_W> r_rdGrpRsmInstr;\n",
					mod.m_modName.Upper().c_str());
			}

			m_mifDecl.Append("\tbool r_m1_rdRspRdy;\n");
			if (mif.m_mifRd.m_bPoll) {
				m_mifDecl.Append("\tbool r_m2_rdRspRdy;\n");
				m_mifDecl.Append("\tbool r_m3_rdRspRdy;\n");
			}
			m_mifDecl.Append("\tCMemRdRspIntf r_m1_rdRsp;\n");
			m_mifDecl.NewLine();

			if (mif.m_mifRd.m_bPause) {
				m_mifDecl.Append("\tbool c_t%d_memReadPause;\n", mod.m_execStg);
				m_mifDecl.Append("\tbool r_t%d_rdRspPauseCntZero;\n", mod.m_execStg);
			}

			if (mif.m_mifRd.m_bPoll) {
				m_mifDecl.Append("\tbool r_t%d_rdRspPollCntZero;\n", mod.m_execStg);
			}
			m_mifDecl.Append("\tbool r_t%d_rdRspBusyCntMax;\n", mod.m_execStg);
			m_mifDecl.NewLine();

			mifPreInstr.Append("\tsc_uint<%s_RD_RSP_CNT_W> c_rdGrpRspCnt;\n",
				mod.m_modName.Upper().c_str());
			mifPreInstr.Append("\tc_rdGrpRspCnt = r_rdGrpRspCnt;\n");
			mifPreInstr.NewLine();

			if (mif.m_mifRd.m_bPause) {
				m_mifDecl.Append("\tbool c_rdGrpRsmWait;\n");
				mifPreInstr.Append("\tc_rdGrpRsmWait = r_rdGrpRsmWait;\n");
				mifPreInstr.NewLine();

				m_mifDecl.Append("\tsc_uint<%s_INSTR_W> c_rdGrpRsmInstr;\n",
					mod.m_modName.Upper().c_str());
				mifPreInstr.Append("\tc_rdGrpRsmInstr = r_rdGrpRsmInstr;\n");
				mifPreInstr.NewLine();

				if (mif.m_mifRd.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0) {
					m_mifDecl.Append("\tsc_uint<%s_HTID_W> c_rdGrpRsmHtId;\n",
						mod.m_modName.Upper().c_str());
					mifPreInstr.Append("\tc_rdGrpRsmHtId = r_rdGrpRsmHtId;\n");
					mifPreInstr.NewLine();
				}
			}

			mifReg.Append("\tr_rdGrpRspCnt = r_reset1x ? (sc_uint<%s_RD_RSP_CNT_W>)0 : c_rdGrpRspCnt;\n",
				mod.m_modName.Upper().c_str());
			mifReg.NewLine();

			if (mif.m_mifRd.m_bPause) {
				mifReg.Append("\tr_rdGrpRsmWait = !r_reset1x && c_rdGrpRsmWait;\n");
				mifReg.NewLine();

				mifReg.Append("\tr_rdGrpRsmInstr = c_rdGrpRsmInstr;\n");
				mifReg.NewLine();

				if (mif.m_mifRd.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0) {
					mifReg.Append("\tr_rdGrpRsmHtId = c_rdGrpRsmHtId;\n");
					mifReg.NewLine();
				}
			}

		} else if (mif.m_mifRd.m_rspGrpIdW <= 2) {
			// using array of registers

			m_mifDecl.Append("\tsc_uint<%s_RD_RSP_CNT_W> r_rdGrpRspCnt[%d];\n",
				mod.m_modName.Upper().c_str(), 1<<mif.m_mifRd.m_rspGrpIdW);

			if (mif.m_mifRd.m_bPause) {
				m_mifDecl.Append("\tbool r_rdGrpRsmWait[%d];\n",
					1<<mif.m_mifRd.m_rspGrpIdW);

				m_mifDecl.Append("\tsc_uint<%s_INSTR_W> r_rdGrpRsmInstr[%d];\n",
					mod.m_modName.Upper().c_str(),1<< mif.m_mifRd.m_rspGrpIdW);

				if (mif.m_mifRd.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifDecl.Append("\tsc_uint<%s_HTID_W> r_rdGrpRsmHtId[%d];\n",
					mod.m_modName.Upper().c_str(), 1<<mif.m_mifRd.m_rspGrpIdW);
			}

			if (mif.m_mifRd.m_bPoll) {
				if (mif.m_mifRd.m_rspGrpId.size() > 0)
					m_mifDecl.Append("\tbool r_rdGrpRspCntZero[%d];\n", 
						1<<mif.m_mifRd.m_rspGrpIdW);
			}

			m_mifDecl.Append("\tbool r_m1_rdRspRdy;\n");
			if (mif.m_mifRd.m_bPoll)
				m_mifDecl.Append("\tbool r_m2_rdRspRdy;\n");
			if (mif.m_mifRd.m_bPause && mif.m_mifRd.m_bPoll)
				m_mifDecl.Append("\tbool r_m3_rdRspRdy;\n");

			m_mifDecl.Append("\tsc_uint<%s_RD_GRP_ID_W> r_m1_rdRspGrpId;\n",
				mod.m_modName.Upper().c_str());
			if (mif.m_mifRd.m_bPoll)
				m_mifDecl.Append("\tsc_uint<%s_RD_GRP_ID_W> r_m2_rdRspGrpId;\n",
				mod.m_modName.Upper().c_str());
			if (mif.m_mifRd.m_bPause && mif.m_mifRd.m_bPoll)
				m_mifDecl.Append("\tsc_uint<%s_RD_GRP_ID_W> r_m3_rdRspGrpId;\n",
				mod.m_modName.Upper().c_str());
			m_mifDecl.Append("\tCMemRdRspIntf r_m1_rdRsp;\n");
			//if (mif.m_mifRd.m_bMultiRd && mif.m_mifRd.m_bPoll)
			//	m_mifDecl.Append("\tht_uint1 r_m2_rdRspQwCnt;\n");
			//if (mif.m_mifRd.m_bMultiRd && mif.m_mifRd.m_bPause && mif.m_mifRd.m_bPoll)
			//	m_mifDecl.Append("\tht_uint1 r_m3_rdRspQwCnt;\n");
			m_mifDecl.NewLine();

			if (mif.m_mifRd.m_bPause) {
				m_mifDecl.Append("\tbool c_t%d_memReadPause;\n", mod.m_execStg);
				m_mifDecl.Append("\tbool r_t%d_rdRspPauseCntZero;\n", mod.m_execStg);
			}

			if (mif.m_mifRd.m_bPoll) {
				if (mif.m_mifRd.m_rspGrpId.size() == 0)
					m_mifDecl.Append("\tbool r_t%d_rdRspPollCntZero;\n", mod.m_execStg);
			}
			m_mifDecl.Append("\tbool r_t%d_rdRspBusyCntMax;\n", mod.m_execStg);
			m_mifDecl.Append("\tbool r_m%d_bRdRspRdy[%d];\n",
				mif.m_mifRd.m_bPoll ? 3 : 1, 1<<mif.m_mifRd.m_rspGrpIdW);

			m_mifDecl.NewLine();

			mifPreInstr.Append("\tsc_uint<%s_RD_RSP_CNT_W> c_rdGrpRspCnt[%d];\n",
				mod.m_modName.Upper().c_str(), 1<<mif.m_mifRd.m_rspGrpIdW);
			for (int i = 0; i < 1<<mif.m_mifRd.m_rspGrpIdW; i += 1)
				mifPreInstr.Append("\tc_rdGrpRspCnt[%d] = r_rdGrpRspCnt[%d];\n", i, i);
			mifPreInstr.NewLine();

			if (mif.m_mifRd.m_bPause) {
				m_mifDecl.Append("\tbool c_rdGrpRsmWait[%d];\n",
					1<<mif.m_mifRd.m_rspGrpIdW);
				for (int i = 0; i < 1<<mif.m_mifRd.m_rspGrpIdW; i += 1)
					mifPreInstr.Append("\tc_rdGrpRsmWait[%d] = r_rdGrpRsmWait[%d];\n", i, i);
				mifPreInstr.NewLine();

				m_mifDecl.Append("\tsc_uint<%s_INSTR_W> c_rdGrpRsmInstr[%d];\n",
					mod.m_modName.Upper().c_str(), 1<<mif.m_mifRd.m_rspGrpIdW);
				for (int i = 0; i < 1<<mif.m_mifRd.m_rspGrpIdW; i += 1)
					mifPreInstr.Append("\tc_rdGrpRsmInstr[%d] = r_rdGrpRsmInstr[%d];\n", i, i);
				mifPreInstr.NewLine();

				if (mif.m_mifRd.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0) {
					m_mifDecl.Append("\tsc_uint<%s_HTID_W> c_rdGrpRsmHtId[%d];\n",
						mod.m_modName.Upper().c_str(), 1<<mif.m_mifRd.m_rspGrpIdW);
					for (int i = 0; i < 1<<mif.m_mifRd.m_rspGrpIdW; i += 1)
						mifPreInstr.Append("\tc_rdGrpRsmHtId[%d] = r_rdGrpRsmHtId[%d];\n", i, i);
					mifPreInstr.NewLine();
				}
			}

			if (mif.m_mifRd.m_bPoll && mif.m_mifRd.m_rspGrpId.size() > 0) {
				mifPreInstr.Append("\tbool c_rdGrpRspCntZero[%d];\n",
					1<<mif.m_mifRd.m_rspGrpIdW);
				for (int i = 0; i < 1<<mif.m_mifRd.m_rspGrpIdW; i += 1)
					mifPreInstr.Append("\tc_rdGrpRspCntZero[%d] = r_rdGrpRspCntZero[%d];\n", i, i);
				mifPreInstr.NewLine();
			}

			for (int i = 0; i < 1<<mif.m_mifRd.m_rspGrpIdW; i += 1) {
				mifReg.Append("\tr_m%d_bRdRspRdy[%d] = c_m%d_bRdRspRdy[%d];\n",
					mif.m_mifRd.m_bPoll ? 3 : 1, i,
					mif.m_mifRd.m_bPoll ? 2 : 0, i);
			}
			mifReg.NewLine();

			for (int i = 0; i < 1<<mif.m_mifRd.m_rspGrpIdW; i += 1)
				mifReg.Append("\tr_rdGrpRspCnt[%d] = r_reset1x ? (sc_uint<%s_RD_RSP_CNT_W>)0 : c_rdGrpRspCnt[%d];\n",
					i, mod.m_modName.Upper().c_str(), i);
			mifReg.NewLine();

			if (mif.m_mifRd.m_bPause) {
				for (int i = 0; i < 1<<mif.m_mifRd.m_rspGrpIdW; i += 1)
					mifReg.Append("\tr_rdGrpRsmWait[%d] = !r_reset1x && c_rdGrpRsmWait[%d];\n", i, i);
				mifReg.NewLine();

				for (int i = 0; i < 1<<mif.m_mifRd.m_rspGrpIdW; i += 1)
					mifReg.Append("\tr_rdGrpRsmInstr[%d] = c_rdGrpRsmInstr[%d];\n", i, i);
				mifReg.NewLine();

				if (mif.m_mifRd.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0) {
					for (int i = 0; i < 1<<mif.m_mifRd.m_rspGrpIdW; i += 1)
						mifReg.Append("\tr_rdGrpRsmHtId[%d] = c_rdGrpRsmHtId[%d];\n", i, i);
					mifReg.NewLine();
				}
			}

			if (mif.m_mifRd.m_bPoll && mif.m_mifRd.m_rspGrpId.size() > 0) {
				for (int i = 0; i < 1<<mif.m_mifRd.m_rspGrpIdW; i += 1)
					mifReg.Append("\tr_rdGrpRspCntZero[%d] = r_reset1x ? true : c_rdGrpRspCntZero[%d];\n",
						i, i);
				mifReg.NewLine();
			}

		} else {
			// using dist ram
			m_mifRamDecl.Append("\tht_dist_ram<sc_uint<%s_RD_RSP_CNT_W>, %s_RD_GRP_ID_W-%s> m_rdGrpRspCnt[%d];\n",
				mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str(), pRdGrpShfAmt, rdGrpBankCnt);

			if (mif.m_mifRd.m_bPoll && mif.m_mifRd.m_rspGrpId.size() > 0)
				m_mifRamDecl.Append("\tht_dist_ram<bool, %s_RD_GRP_ID_W-%s> m_rdGrpRspCntZero[%d];\n",
					mod.m_modName.Upper().c_str(), pRdGrpShfAmt, rdGrpBankCnt);

			if (mif.m_mifRd.m_bPause) {
				m_mifRamDecl.Append("\tht_dist_ram<bool, %s_RD_GRP_ID_W-%s> m_rdGrpRsmWait[%d];\n",
					mod.m_modName.Upper().c_str(), pRdGrpShfAmt, rdGrpBankCnt);
				m_mifRamDecl.Append("\tht_dist_ram<sc_uint<%s_INSTR_W>, %s_RD_GRP_ID_W-%s> m_rdGrpRsmInstr[%d];\n",
					mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str(), pRdGrpShfAmt, rdGrpBankCnt);

				if (mif.m_mifRd.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifRamDecl.Append("\tht_dist_ram<sc_uint<%s_HTID_W>, %s_RD_GRP_ID_W-%s> m_rdGrpRsmHtId[%d];\n",
						mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str(), pRdGrpShfAmt, rdGrpBankCnt);
			}

			m_mifDecl.Append("\tsc_uint<%s_RD_GRP_ID_W+1> r_rdRspGrpInitCnt;\n", mod.m_modName.Upper().c_str());
			m_mifDecl.NewLine();

			m_mifDecl.Append("\tbool r_m1_rdRspRdy;\n");
			if (mif.m_mifRd.m_bPoll)
				m_mifDecl.Append("\tbool r_m2_rdRspRdy;\n");
			if (mif.m_mifRd.m_bPause && mif.m_mifRd.m_bPoll)
				m_mifDecl.Append("\tbool r_m3_rdRspRdy;\n");
			m_mifDecl.Append("\tsc_uint<%s_RD_GRP_ID_W> r_m1_rdRspGrpId;\n",
				mod.m_modName.Upper().c_str());
			if (mif.m_mifRd.m_bPoll)
				m_mifDecl.Append("\tsc_uint<%s_RD_GRP_ID_W> r_m2_rdRspGrpId;\n",
				mod.m_modName.Upper().c_str());
			if (mif.m_mifRd.m_bPause && mif.m_mifRd.m_bPoll)
				m_mifDecl.Append("\tsc_uint<%s_RD_GRP_ID_W> r_m3_rdRspGrpId;\n",
				mod.m_modName.Upper().c_str());
			m_mifDecl.Append("\tCMemRdRspIntf r_m1_rdRsp;\n");
			//if (bNeed_rdRspQwIdx)
			//	m_mifDecl.Append("\tht_uint3 r_m1_rdRspQwIdx;\n");
			//if (mif.m_mifRd.m_bMultiRd && mif.m_mifRd.m_bPoll)
			//	m_mifDecl.Append("\tht_uint1 r_m2_rdRspQwCnt;\n");
			//if (mif.m_mifRd.m_bMultiRd && mif.m_mifRd.m_bPause && mif.m_mifRd.m_bPoll)
			//	m_mifDecl.Append("\tht_uint1 r_m3_rdRspQwCnt;\n");
			m_mifDecl.NewLine();

			m_mifDecl.Append("\tbool r_t%d_bReadRspBusy;\n", mod.m_execStg);
			if (mif.m_mifRd.m_bPause)
				m_mifDecl.Append("\tbool r_t%d_rdRspPauseCntZero;\n", mod.m_execStg);
			if (mif.m_mifRd.m_bPoll) {
				if (mif.m_mifRd.m_rspGrpId.size() == 0)
					m_mifDecl.Append("\tbool r_t%d_rdRspPollCntZero;\n", mod.m_execStg);
			}
			m_mifDecl.Append("\tbool r_t%d_rdRspBusyCntMax;\n", mod.m_execStg);
			m_mifDecl.Append("\tbool r_m%d_bRdRspRdy[%d];\n",
				mif.m_mifRd.m_bPoll ? 3 : 1, rdGrpBankCnt);

			m_mifDecl.Append("\tsc_uint<%s_RD_RSP_CNT_W> r_rdGrpRspCnt[%d];\n", mod.m_modName.Upper().c_str(), rdGrpBankCnt);
			m_mifDecl.Append("\tsc_uint<%s_RD_GRP_ID_W> r_rdGrpRspCntAddr[%d];\n", mod.m_modName.Upper().c_str(), rdGrpBankCnt);

			m_mifDecl.NewLine();

			for (int bankIdx = 0; bankIdx < rdGrpBankCnt; bankIdx += 1) {
				mifRamClock.Append("\tm_rdGrpRspCnt[%d].clock();\n", bankIdx);

				if (mif.m_mifRd.m_bPoll && mif.m_mifRd.m_rspGrpId.size() > 0)
					mifRamClock.Append("\tm_rdGrpRspCntZero[%d].clock();\n", bankIdx);

				if (mif.m_mifRd.m_bPause) {
					mifRamClock.Append("\tm_rdGrpRsmWait[%d].clock();\n", bankIdx);
					mifRamClock.Append("\tm_rdGrpRsmInstr[%d].clock();\n", bankIdx);

					if (mif.m_mifRd.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0)
						mifRamClock.Append("\tm_rdGrpRsmHtId[%d].clock();\n", bankIdx);
				}
			}

			mifRamClock.NewLine();
		}

		if (mif.m_mifRd.m_maxRsmDly > 0 && mif.m_mifRd.m_bPause) {
			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				m_mifDecl.Append("\tbool r_rdRspRsmValid[%d];\n", mif.m_mifRd.m_maxRsmDly);
				m_mifDecl.Append("\tCHtRsm r_rdRspRsm[%d];\n", mif.m_mifRd.m_maxRsmDly);
			} else {
				m_mifDecl.Append("\tbool r_rdRspRsmRdy[%d];\n", mif.m_mifRd.m_maxRsmDly);
				m_mifDecl.Append("\tht_uint%d r_rdRspRsmInstr[%d];\n", mod.m_instrW, mif.m_mifRd.m_maxRsmDly);
			}
		}
	}

	if (mif.m_bMifWr) {

		if (mif.m_mifWr.m_rspGrpIdW == 0) {

			m_mifDecl.Append("\tsc_uint<%s_WR_RSP_CNT_W> r_wrGrpRspCnt;\n",
				mod.m_modName.Upper().c_str());

			if (mif.m_mifWr.m_bPause) {
				m_mifDecl.Append("\tbool r_wrGrpRsmWait;\n");
				m_mifDecl.Append("\tsc_uint<%s_INSTR_W> r_wrGrpRsmInstr;\n",
					mod.m_modName.Upper().c_str());
			}

			if (mif.m_mifWr.m_bPause) {
				m_mifDecl.Append("\tbool r_m1_wrRspRdy;\n");
				m_mifDecl.NewLine();
				m_mifDecl.Append("\tbool c_t%d_memWritePause;\n", mod.m_execStg);
			}

			m_mifDecl.Append("\tbool r_t%d_wrGrpRspCntZero;\n", mod.m_execStg);
			m_mifDecl.Append("\tbool r_t%d_wrGrpRspCntMax;\n", mod.m_execStg);
			m_mifDecl.Append("\tbool r_m1_bWrRspRdy;\n");
			m_mifDecl.NewLine();

			mifPreInstr.Append("\tsc_uint<%s_WR_RSP_CNT_W> c_wrGrpRspCnt;\n",
				mod.m_modName.Upper().c_str());
			mifPreInstr.Append("\tc_wrGrpRspCnt = r_wrGrpRspCnt;\n");
			mifPreInstr.NewLine();

			if (mif.m_mifWr.m_bPause) {
				m_mifDecl.Append("\tbool c_wrGrpRsmWait;\n");
				mifPreInstr.Append("\tc_wrGrpRsmWait = r_wrGrpRsmWait;\n");
				mifPreInstr.NewLine();

				m_mifDecl.Append("\tsc_uint<%s_INSTR_W> c_wrGrpRsmInstr;\n",
					mod.m_modName.Upper().c_str());
				mifPreInstr.Append("\tc_wrGrpRsmInstr = r_wrGrpRsmInstr;\n");
				mifPreInstr.NewLine();

				if (mif.m_mifWr.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0) {
					m_mifDecl.Append("\tsc_uint<%s_HTID_W> c_wrGrpRsmHtId;\n",
						mod.m_modName.Upper().c_str());
					mifPreInstr.Append("\tc_wrGrpRsmHtId = r_wrGrpRsmHtId;\n");
					mifPreInstr.NewLine();
				}
			}

			mifReg.Append("\tr_m1_bWrRspRdy = c_m0_bWrRspRdy;\n");
			mifReg.NewLine();

			mifReg.Append("\tr_wrGrpRspCnt = r_reset1x ? (sc_uint<%s_WR_RSP_CNT_W>)0 : c_wrGrpRspCnt;\n",
				mod.m_modName.Upper().c_str());
			mifReg.NewLine();

			if (mif.m_mifWr.m_bPause) {
				mifReg.Append("\tr_wrGrpRsmWait = !r_reset1x && c_wrGrpRsmWait;\n");
				mifReg.NewLine();

				mifReg.Append("\tr_wrGrpRsmInstr = c_wrGrpRsmInstr;\n");
				mifReg.NewLine();

				if (mif.m_mifWr.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0) {
					mifReg.Append("\tr_wrGrpRsmHtId = c_wrGrpRsmHtId;\n");
					mifReg.NewLine();
				}
			}

		} else if (mif.m_mifWr.m_rspGrpIdW <= 2) {
			// using array of registers

			m_mifDecl.Append("\tsc_uint<%s_WR_RSP_CNT_W> r_wrGrpRspCnt[%d];\n",
				mod.m_modName.Upper().c_str(), 1<<mif.m_mifWr.m_rspGrpIdW);

			if (mif.m_mifWr.m_bPause) {
				m_mifDecl.Append("\tbool r_wrGrpRsmWait[%d];\n",
					1<<mif.m_mifWr.m_rspGrpIdW);
				m_mifDecl.Append("\tsc_uint<%s_INSTR_W> r_wrGrpRsmInstr[%d];\n",
					mod.m_modName.Upper().c_str(),1<< mif.m_mifWr.m_rspGrpIdW);
				if (mif.m_mifWr.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0)
					m_mifDecl.Append("\tsc_uint<%s_HTID_W> r_wrGrpRsmHtId[%d];\n",
					mod.m_modName.Upper().c_str(), 1<<mif.m_mifWr.m_rspGrpIdW);
			}

			if (mif.m_mifWr.m_bPause) {
				m_mifDecl.Append("\tbool r_m1_wrRspRdy;\n");
				m_mifDecl.Append("\tsc_uint<MIF_TID_W> r_m1_wrRspTid;\n");
				m_mifDecl.NewLine();

				m_mifDecl.Append("\tbool c_t%d_memWritePause;\n", mod.m_execStg);
			}

			m_mifDecl.Append("\tbool r_t%d_wrGrpRspCntZero;\n", mod.m_execStg);
			m_mifDecl.Append("\tbool r_t%d_wrGrpRspCntMax;\n", mod.m_execStg);
			m_mifDecl.Append("\tbool r_m1_bWrRspRdy[%d];\n", 1<<mif.m_mifWr.m_rspGrpIdW);
			m_mifDecl.NewLine();

			mifPreInstr.Append("\tsc_uint<%s_WR_RSP_CNT_W> c_wrGrpRspCnt[%d];\n",
				mod.m_modName.Upper().c_str(), 1<<mif.m_mifWr.m_rspGrpIdW);
			for (int i = 0; i < 1<<mif.m_mifWr.m_rspGrpIdW; i += 1)
				mifPreInstr.Append("\tc_wrGrpRspCnt[%d] = r_wrGrpRspCnt[%d];\n", i, i);
			mifPreInstr.NewLine();

			if (mif.m_mifWr.m_bPause) {
				m_mifDecl.Append("\tbool c_wrGrpRsmWait[%d];\n",
					1<<mif.m_mifWr.m_rspGrpIdW);
				for (int i = 0; i < 1<<mif.m_mifWr.m_rspGrpIdW; i += 1)
					mifPreInstr.Append("\tc_wrGrpRsmWait[%d] = r_wrGrpRsmWait[%d];\n", i, i);
				mifPreInstr.NewLine();

				m_mifDecl.Append("\tsc_uint<%s_INSTR_W> c_wrGrpRsmInstr[%d];\n",
					mod.m_modName.Upper().c_str(), 1<<mif.m_mifWr.m_rspGrpIdW);
				for (int i = 0; i < 1<<mif.m_mifWr.m_rspGrpIdW; i += 1)
					mifPreInstr.Append("\tc_wrGrpRsmInstr[%d] = r_wrGrpRsmInstr[%d];\n", i, i);
				mifPreInstr.NewLine();

				if (mif.m_mifWr.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0) {
					m_mifDecl.Append("\tsc_uint<%s_HTID_W> c_wrGrpRsmHtId[%d];\n",
						mod.m_modName.Upper().c_str(), 1<<mif.m_mifWr.m_rspGrpIdW);
					for (int i = 0; i < 1<<mif.m_mifWr.m_rspGrpIdW; i += 1)
						mifPreInstr.Append("\tc_wrGrpRsmHtId[%d] = r_wrGrpRsmHtId[%d];\n", i, i);
					mifPreInstr.NewLine();
				}
			}

			for (int i = 0; i < 1<<mif.m_mifWr.m_rspGrpIdW; i += 1)
				mifReg.Append("\tr_m1_bWrRspRdy[%d] = c_m0_bWrRspRdy[%d];\n", i, i);
			mifReg.NewLine();

			for (int i = 0; i < 1<<mif.m_mifWr.m_rspGrpIdW; i += 1)
				mifReg.Append("\tr_wrGrpRspCnt[%d] = r_reset1x ? (sc_uint<%s_WR_RSP_CNT_W>)0 : c_wrGrpRspCnt[%d];\n",
				i, mod.m_modName.Upper().c_str(), i);
			mifReg.NewLine();

			if (mif.m_mifWr.m_bPause) {
				for (int i = 0; i < 1<<mif.m_mifWr.m_rspGrpIdW; i += 1)
					mifReg.Append("\tr_wrGrpRsmWait[%d] = !r_reset1x && c_wrGrpRsmWait[%d];\n", i, i);
				mifReg.NewLine();

				for (int i = 0; i < 1<<mif.m_mifWr.m_rspGrpIdW; i += 1)
					mifReg.Append("\tr_wrGrpRsmInstr[%d] = c_wrGrpRsmInstr[%d];\n", i, i);
				mifReg.NewLine();

				if (mif.m_mifWr.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0) {
					for (int i = 0; i < 1<<mif.m_mifWr.m_rspGrpIdW; i += 1)
						mifReg.Append("\tr_wrGrpRsmHtId[%d] = c_wrGrpRsmHtId[%d];\n", i, i);
					mifReg.NewLine();
				}
			}
		} else {
			// using dist ram
			m_mifRamDecl.Append("\tht_dist_ram<sc_uint<%s_WR_RSP_CNT_W>, %s_WR_GRP_ID_W-%s> m_wrGrpRspCnt[%d];\n",
				mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str(), pWrGrpShfAmt, wrGrpBankCnt);
			if (mif.m_mifWr.m_bPause) {
				m_mifRamDecl.Append("\tht_dist_ram<bool, %s_WR_GRP_ID_W-%s> m_wrGrpRsmWait[%d];\n",
					mod.m_modName.Upper().c_str(), pWrGrpShfAmt, wrGrpBankCnt);
				m_mifRamDecl.Append("\tht_dist_ram<sc_uint<%s_INSTR_W>, %s_WR_GRP_ID_W-%s> m_wrGrpRsmInstr[%d];\n",
					mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str(), pWrGrpShfAmt, wrGrpBankCnt);
				if (mif.m_mifWr.m_rspGrpId.size() != 0)
					m_mifRamDecl.Append("\tht_dist_ram<sc_uint<%s_HTID_W>, %s_WR_GRP_ID_W-%s> m_wrGrpRsmHtId[%d];\n",
					mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str(), pWrGrpShfAmt, wrGrpBankCnt);
			}
			m_mifRamDecl.NewLine();

			m_mifDecl.Append("\tsc_uint<%s_WR_GRP_ID_W+1> r_wrRspGrpInitCnt;\n", mod.m_modName.Upper().c_str());
			m_mifDecl.NewLine();

			if (mif.m_mifWr.m_bPause) {
				m_mifDecl.Append("\tbool r_m1_wrRspRdy;\n");
				m_mifDecl.Append("\tsc_uint<MIF_TID_W> r_m1_wrRspTid;\n");
				m_mifDecl.NewLine();
			}

			m_mifDecl.Append("\tbool r_t%d_bWriteRspBusy;\n", mod.m_execStg);
			m_mifDecl.Append("\tbool r_t%d_wrGrpRspCntZero;\n", mod.m_execStg);
			m_mifDecl.Append("\tbool r_t%d_wrGrpRspCntMax;\n", mod.m_execStg);
			m_mifDecl.Append("\tbool r_m1_bWrRspRdy[%d];\n", wrGrpBankCnt);
			m_mifDecl.Append("\tsc_uint<%s_WR_RSP_CNT_W> r_wrGrpRspCnt[%d];\n", mod.m_modName.Upper().c_str(), wrGrpBankCnt);
			m_mifDecl.Append("\tsc_uint<%s_WR_GRP_ID_W> r_wrGrpRspCntAddr[%d];\n", mod.m_modName.Upper().c_str(), wrGrpBankCnt);

			m_mifDecl.NewLine();

			for (int bankIdx = 0; bankIdx < wrGrpBankCnt; bankIdx += 1) {
				mifRamClock.Append("\tm_wrGrpRspCnt[%d].clock();\n", bankIdx);
				if (mif.m_mifWr.m_bPause) {
					mifRamClock.Append("\tm_wrGrpRsmWait[%d].clock();\n", bankIdx);
					mifRamClock.Append("\tm_wrGrpRsmInstr[%d].clock();\n", bankIdx);

					if (mif.m_mifWr.m_rspGrpId.size() != 0)
						mifRamClock.Append("\tm_wrGrpRsmHtId[%d].clock();\n", bankIdx);
				}
			}
			mifRamClock.NewLine();
		}
	}

	if (mif.m_bMifRd) {
		mifPostInstr.Append("\tbool c_m0_rdRspRdy = i_mifTo%sP0_rdRspRdy.read();\n",
			mod.m_modName.Uc().c_str());
		mifPostInstr.Append("\tbool c_m1_rdRspRdy = r_m1_rdRspRdy;\n");
		if (mif.m_mifRd.m_bPoll)
			mifPostInstr.Append("\tbool c_m2_rdRspRdy = r_m2_rdRspRdy;\n");
		if (mif.m_mifRd.m_bPause && mif.m_mifRd.m_bPoll || mif.m_mifRd.m_bPoll && mif.m_mifRd.m_rspGrpIdW == 0)
			mifPostInstr.Append("\tbool c_m3_rdRspRdy = r_m3_rdRspRdy;\n");
		mifPostInstr.NewLine();

		if (mif.m_mifRd.m_rspGrpIdW > 0) {
			mifPostInstr.Append("\tsc_uint<%s_RD_GRP_ID_W> c_m0_rdRspGrpId = i_mifTo%sP0_rdRsp.read().m_tid & ((1 << %s_RD_GRP_ID_W)-1);\n",
				mod.m_modName.Upper().c_str(),
				mod.m_modName.Uc().c_str(),
				mod.m_modName.Upper().c_str());
			mifPostInstr.Append("\tsc_uint<%s_RD_GRP_ID_W> c_m1_rdRspGrpId = r_m1_rdRspGrpId;\n",
				mod.m_modName.Upper().c_str());
			if (mif.m_mifRd.m_bPoll)
				mifPostInstr.Append("\tsc_uint<%s_RD_GRP_ID_W> c_m2_rdRspGrpId = r_m2_rdRspGrpId;\n",
				mod.m_modName.Upper().c_str());
			if (mif.m_mifRd.m_bPause && mif.m_mifRd.m_bPoll)
				mifPostInstr.Append("\tsc_uint<%s_RD_GRP_ID_W> c_m3_rdRspGrpId = r_m3_rdRspGrpId;\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.NewLine();
		}
	}

	if (mif.m_bMifRd) {
		char rdRspGrpId[128];
		if (mif.m_mifRd.m_rspGrpId.size() == 0)
			sprintf(rdRspGrpId, "r_t%d_htId", mod.m_execStg-1);
		else if (mif.m_mifRd.m_bRspGrpIdPriv)
			sprintf(rdRspGrpId, "r_t%d_htPriv.m_%s",
			mod.m_execStg-1, mif.m_mifRd.m_rspGrpId.AsStr().c_str());
		else
			sprintf(rdRspGrpId, "c_%s", mif.m_mifRd.m_rspGrpId.AsStr().c_str());

		char const * pRdRspGrpW = mif.m_mifRd.m_rspGrpId.size() > 0 ? "_RD_GRP_ID_W" : "_HTID_W";

		if (mif.m_mifRd.m_rspCntW.AsInt() > 0) {
			if (mif.m_maxDstW > 0)
				mifPostInstr.Append("\tsc_uint<%s_MIF_DST_W> c_m1_rdRsp_dst = (r_m1_rdRsp.m_tid >> (1+%s%s)) & ((1ul << %s_MIF_DST_W)-1);\n",
				mod.m_modName.Upper().c_str(),
				mod.m_modName.Upper().c_str(), pRdRspGrpW, mod.m_modName.Upper().c_str());
		} else {
			if (mif.m_maxDstW > 0)
				mifPostInstr.Append("\tsc_uint<%s_MIF_DST_W> c_m1_rdRsp_dst = (i_mifTo%sP0_rdRsp.read().m_tid >> (1+%s%s)) & ((1ul << %s_MIF_DST_W)-1);\n",
				mod.m_modName.Upper().c_str(), mod.m_modName.Uc().c_str(),
				mod.m_modName.Upper().c_str(), pRdRspGrpW, mod.m_modName.Upper().c_str());
		}
		if (mif.m_mifRd.m_rdDstList.size() > 1)
			mifPostInstr.Append("\tsc_uint<%s_MIF_DST_IDX_W> c_m1_rdRsp_dstIdx = c_m1_rdRsp_dst & %s_MIF_DST_IDX_MSK;\n",
			mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());

		for (size_t i = 0; i < mif.m_mifRd.m_rdDstList.size(); i += 1) {
			CMifRdDst &mifRdDst = mif.m_mifRd.m_rdDstList[i];

			if (bNeed_rdRspQwIdx && mifRdDst.m_dstIdx.size() > 0 && (is_wx || mifRdDst.m_memSrc == "host")) {
				mifPostInstr.Append("\tht_uint3 c_m1_rdRspQwIdx = (r_m1_rdRsp.m_tid >> MIF_TID_QWCNT_SHF) & MIF_TID_QWCNT_MSK;\n");
				break;
			}
		}
		//if (mif.m_mifRd.m_bMultiRd) {
		//	if (mif.m_mifRd.m_bPoll)
		//		mifPostInstr.Append("\tht_uint1 c_m2_rdRspQwCnt = r_m2_rdRspQwCnt;\n");
		//	if (mif.m_mifRd.m_bPause && mif.m_mifRd.m_bPoll)
		//		mifPostInstr.Append("\tht_uint1 c_m3_rdRspQwCnt = r_m3_rdRspQwCnt;\n");
		//	mifPostInstr.NewLine();
		//}
	}

	if (mif.m_bMifRd && mif.m_mifRd.m_bPause) {

		if (mif.m_mifRd.m_maxRsmDly > 0) {
			if (mod.m_threads.m_htIdW.AsInt() > 0) {
				mifPostInstr.Append("\tbool c_rdRspRsmValid = false;\n");
				mifPostInstr.Append("\tCHtRsm c_rdRspRsm;\n");
			} else {
				mifPostInstr.Append("\tbool c_rdRspRsmRdy = false;\n");
				mifPostInstr.Append("\tht_uint%d c_rdRspRsmInstr = 0;\n", mod.m_instrW);
			}
			mifPostInstr.NewLine();
		}

		if (mif.m_mifRd.m_rspGrpIdW == 0) {

			mifPostInstr.Append("\tif (c_m%d_rdRspRdy) {\n",
				mif.m_mifRd.m_bPoll ? 3 : 1);
			mifPostInstr.NewLine();
			mifPostInstr.Append("\t\tbool c_m%d_rdGrpRsmWait = r_rdGrpRsmWait == true || c_t%d_memReadPause;\n",
				mif.m_mifRd.m_bPoll ? 3 : 1, mod.m_execStg);
			mifPostInstr.NewLine();
			mifPostInstr.Append("\t\tif (c_m%d_rdGrpRsmWait && r_rdGrpRspCnt == 1) {\n",
				mif.m_mifRd.m_bPoll ? 3 : 1);
			mifPostInstr.Append("\t\t\t// last read response arrived, resume thread\n");
			mifPostInstr.Append("\t\t\tc_rdGrpRsmWait = false;\n");

			if (mifRsmCnt > 1) {
				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					mifPostInstr.NewLine();
					mifPostInstr.Append("\t\t\tCHtRsm rdRspRsm;\n");
					mifPostInstr.Append("\t\t\trdRspRsm.m_htId = c_m%d_rdRspGrpId;\n",
						mif.m_mifRd.m_bPoll ? 3 : 1);
					mifPostInstr.Append("\t\t\trdRspRsm.m_htInstr = c_rdGrpRsmInstr;\n");
					mifPostInstr.NewLine();
					mifPostInstr.Append("\t\t\tm_mif_rdRsmQue.push(rdRspRsm);\n");
				} else {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = c_rdGrpRsmInstr;\n");
				}
			} else {
				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtId = c_m%d_rdRspGrpId;\n",
						mif.m_mifRd.m_bPoll ? 3 : 1);
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = c_rdGrpRsmInstr;\n");
				} else {
					if (mif.m_mifRd.m_maxRsmDly > 0) {
						mifPostInstr.Append("\t\t\tc_rdRspRsmRdy = true;\n");
						mifPostInstr.Append("\t\t\tc_rdRspRsmInstr = c_rdGrpRsmInstr;\n");
					} else {
						mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
						mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = c_rdGrpRsmInstr;\n");
					}
				}
			}

			mifPostInstr.Append("\t\t}\n");
			mifPostInstr.Append("\t}\n");
			mifPostInstr.Append("\n");

		} else if (mif.m_mifRd.m_rspGrpIdW <= 2) {

			mifPostInstr.Append("\tif (c_m%d_rdRspRdy) {\n", mif.m_mifRd.m_bPoll ? 3 : 1);
			mifPostInstr.Append("\t\tbool c_m%d_rdGrpRsmWait = r_rdGrpRsmWait[INT(c_m%d_rdRspGrpId)] == true\n",
				mif.m_mifRd.m_bPoll ? 3 : 1, mif.m_mifRd.m_bPoll ? 3 : 1);
			mifPostInstr.Append("\t\t\t|| (c_t%d_memReadPause && %s == c_m%d_rdRspGrpId);\n",
				mod.m_execStg, rdRspGrpName, mif.m_mifRd.m_bPoll ? 3 : 1);
			mifPostInstr.NewLine();
			mifPostInstr.Append("\t\tif (c_m%d_rdGrpRsmWait && r_rdGrpRspCnt[INT(c_m%d_rdRspGrpId)] == 1) {\n",
				mif.m_mifRd.m_bPoll ? 3 : 1, mif.m_mifRd.m_bPoll ? 3 : 1);
			mifPostInstr.Append("\t\t\t// last read response arrived, resume thread\n");
			mifPostInstr.Append("\t\t\tc_rdGrpRsmWait[INT(c_m%d_rdRspGrpId)] = false;\n",
				mif.m_mifRd.m_bPoll ? 3 : 1);
			mifPostInstr.NewLine();

			if (mifRsmCnt > 1) {

				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					if (mif.m_mifRd.m_maxRsmDly == 0) {
						mifPostInstr.Append("\t\t\tCHtRsm rdRspRsm;\n");
						if (mif.m_mifRd.m_rspGrpId.size() == 0)
							mifPostInstr.Append("\t\t\trdRspRsm.m_htId = c_m%d_rdRspGrpId;\n",
								mif.m_mifRd.m_bPoll ? 3 : 1);
						else
							mifPostInstr.Append("\t\t\trdRspRsm.m_htId = c_rdGrpRsmHtId[INT(c_m%d_rdRspGrpId)];\n",
								mif.m_mifRd.m_bPoll ? 3 : 1);
						mifPostInstr.Append("\t\t\trdRspRsm.m_htInstr = c_rdGrpRsmInstr[INT(c_m%d_rdRspGrpId)];\n",
							mif.m_mifRd.m_bPoll ? 3 : 1);
						mifPostInstr.NewLine();
						mifPostInstr.Append("\t\t\tm_mif_rdRsmQue.push(rdRspRsm);\n");
					} else {
						mifPostInstr.Append("\t\t\tc_rdRspRsmValid = true;\n");
						mifPostInstr.Append("\t\t\tc_rdRspRsm.m_htId = c_rdGrpRsmHtId[INT(c_m%d_rdRspGrpId)];\n",
							mif.m_mifRd.m_bPoll ? 3 : 1);
						mifPostInstr.Append("\t\t\tc_rdRspRsm.m_htInstr = c_rdGrpRsmInstr[INT(c_m%d_rdRspGrpId)];\n",
							mif.m_mifRd.m_bPoll ? 3 : 1);
					}
				} else {
					if (mif.m_mifRd.m_maxRsmDly == 0) {
						mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
						mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = c_rdGrpRsmInstr[INT(c_m%d_rdRspGrpId)];\n",
							mif.m_mifRd.m_bPoll ? 3 : 1);
					} else {
						mifPostInstr.Append("\t\t\tc_rdRspRsmRdy = true;\n");
						mifPostInstr.Append("\t\t\tc_rdRspRsmInstr = c_rdGrpRsmInstr[INT(c_m%d_rdRspGrpId)];\n",
							mif.m_mifRd.m_bPoll ? 3 : 1);
					}
				}

			} else {

				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					if (mif.m_mifRd.m_maxRsmDly == 0) {
						mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
						if (mif.m_mifRd.m_rspGrpId.size() == 0)
							mifPostInstr.Append("\t\t\tc_t0_rsmHtId = c_m%d_rdRspGrpId;\n",
								mif.m_mifRd.m_bPoll ? 3 : 1);
						else
							mifPostInstr.Append("\t\t\tc_t0_rsmHtId = c_rdGrpRsmHtId[INT(c_m%d_rdRspGrpId)];\n",
								mif.m_mifRd.m_bPoll ? 3 : 1);

						mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = c_rdGrpRsmInstr[INT(c_m%d_rdRspGrpId)];\n",
							mif.m_mifRd.m_bPoll ? 3 : 1);
					} else {
						mifPostInstr.Append("\t\t\tc_rdRspRsmValid = true;\n");
						mifPostInstr.Append("\t\t\tc_rdRspRsm.m_htId = c_rdGrpRsmHtId[INT(c_m%d_rdRspGrpId)];\n",
							mif.m_mifRd.m_bPoll ? 3 : 1);
						mifPostInstr.Append("\t\t\tc_rdRspRsm.m_htInstr = c_rdGrpRsmInstr[INT(c_m%d_rdRspGrpId)];\n",
							mif.m_mifRd.m_bPoll ? 3 : 1);
					}
				} else {
					if (mif.m_mifRd.m_maxRsmDly == 0) {
						mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
						mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = c_rdGrpRsmInstr[INT(c_m%d_rdRspGrpId)];\n",
							mif.m_mifRd.m_bPoll ? 3 : 1);
					} else {
						mifPostInstr.Append("\t\t\tc_rdRspRsmRdy = true;\n");
						mifPostInstr.Append("\t\t\tc_rdRspRsmInstr = c_rdGrpRsmInstr[INT(c_m%d_rdRspGrpId)];\n",
							mif.m_mifRd.m_bPoll ? 3 : 1);
					}
				}
			}

			mifPostInstr.Append("\t\t}\n");
			mifPostInstr.Append("\t}\n");

		} else {

			mifPostInstr.Append("\tm_rdGrpRsmWait[c_m%d_rdRspGrpId & %s].read_addr(c_m%d_rdRspGrpId >> %s);\n",
				mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask,
				mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpShfAmt);
			mifPostInstr.Append("\tm_rdGrpRsmInstr[c_m%d_rdRspGrpId & %s].read_addr(c_m%d_rdRspGrpId >> %s);\n",
				mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask,
				mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpShfAmt);
			if (mif.m_mifRd.m_rspGrpId.size() != 0 && mod.m_threads.m_htIdW.AsInt() > 0)
				mifPostInstr.Append("\tm_rdGrpRsmHtId[c_m%d_rdRspGrpId & %s].read_addr(c_m%d_rdRspGrpId >> %s);\n",
					mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask,
					mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpShfAmt);

			mifPostInstr.Append("\tif (c_m%d_rdRspRdy) {\n",
				mif.m_mifRd.m_bPoll ? 3 : 1);

			mifPostInstr.Append("\t\tm_rdGrpRsmWait[c_m%d_rdRspGrpId & %s].write_addr(c_m%d_rdRspGrpId >> %s);\n",
				mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask,
				mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpShfAmt);
			mifPostInstr.NewLine();
			mifPostInstr.Append("\t\tbool c_m%d_rdGrpRsmWait = m_rdGrpRsmWait[c_m%d_rdRspGrpId & %s].read_mem();\n",
				mif.m_mifRd.m_bPoll ? 3 : 1, mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask);
			mifPostInstr.NewLine();
			mifPostInstr.Append("\t\tif (c_m%d_rdGrpRsmWait == true && r_rdGrpRspCnt[c_m%d_rdRspGrpId & %s] == 1) {\n",
				mif.m_mifRd.m_bPoll ? 3 : 1, mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask);

			mifPostInstr.Append("\t\t\t// last read response arrived, resume thread\n");
			mifPostInstr.Append("\t\t\tm_rdGrpRsmWait[c_m%d_rdRspGrpId & %s].write_mem(false);\n",
				mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask);
			mifPostInstr.NewLine();

			if (mifRsmCnt > 1) {

				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					if (mif.m_mifRd.m_maxRsmDly == 0) {
						mifPostInstr.Append("\t\t\tCHtRsm rdRspRsm;\n");
						if (mif.m_mifRd.m_rspGrpId.size() == 0)
							mifPostInstr.Append("\t\t\trdRspRsm.m_htId = c_m%d_rdRspGrpId;\n",
								mif.m_mifRd.m_bPoll ? 3 : 1);
						else
							mifPostInstr.Append("\t\t\trdRspRsm.m_htId = m_rdGrpRsmHtId[c_m%d_rdRspGrpId & %s].read_mem();\n",
								mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask);
						mifPostInstr.Append("\t\t\trdRspRsm.m_htInstr = m_rdGrpRsmInstr[c_m%d_rdRspGrpId & %s].read_mem();\n",
							mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask);
						mifPostInstr.NewLine();
						mifPostInstr.Append("\t\t\tm_mif_rdRsmQue.push(rdRspRsm);\n");
					} else {
						mifPostInstr.Append("\t\t\tc_rdRspRsmValid = true;\n");
						if (mif.m_mifRd.m_rspGrpId.size() == 0)
							mifPostInstr.Append("\t\t\tc_rdRspRsm.m_htId = c_m%d_rdRspGrpId;\n",
								mif.m_mifRd.m_bPoll ? 3 : 1);
						else
							mifPostInstr.Append("\t\t\tc_rdRspRsm.m_htId = m_rdGrpRsmHtId[c_m%d_rdRspGrpId & %s].read_mem();\n",
								mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask);
						mifPostInstr.Append("\t\t\tc_rdRspRsm.m_htInstr = m_rdGrpRsmInstr[c_m%d_rdRspGrpId & %s].read_mem();\n",
							mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask);
					}
				} else {
					if (mif.m_mifRd.m_maxRsmDly == 0) {
						mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
						mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = m_rdGrpRsmInstr[c_m%d_rdRspGrpId & %s].read_mem();\n",
							mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask);
					} else {
						mifPostInstr.Append("\t\t\tc_rdRspRsmRdy = true;\n");
						mifPostInstr.Append("\t\t\tc_rdRspRsmInstr = m_rdGrpRsmInstr[c_m%d_rdRspGrpId & %s].read_mem();\n",
							mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask);
					}
				}

			} else {

				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					if (mif.m_mifRd.m_maxRsmDly == 0) {
						mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
						if (mif.m_mifRd.m_rspGrpId.size() == 0)
							mifPostInstr.Append("\t\t\tc_t0_rsmHtId = c_m%d_rdRspGrpId;\n",
							mif.m_mifRd.m_bPoll ? 3 : 1);
						else
							mifPostInstr.Append("\t\t\tc_t0_rsmHtId = m_rdGrpRsmHtId[c_m%d_rdRspGrpId & %s].read_mem();\n",
							mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask);
						mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = m_rdGrpRsmInstr[c_m%d_rdRspGrpId & %s].read_mem();\n",
							mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask);
					} else {
						mifPostInstr.Append("\t\t\tc_rdRspRsmValid = true;\n");
						if (mif.m_mifRd.m_rspGrpId.size() == 0)
							mifPostInstr.Append("\t\t\tc_rdRspRsm.m_htId = c_m%d_rdRspGrpId;\n",
								mif.m_mifRd.m_bPoll ? 3 : 1);
						else
							mifPostInstr.Append("\t\t\tc_rdRspRsm.m_htId = m_rdGrpRsmHtId[c_m%d_rdRspGrpId & %s].read_mem();\n",
								mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask);
						mifPostInstr.Append("\t\t\tc_rdRspRsm.m_htInstr = m_rdGrpRsmInstr[c_m%d_rdRspGrpId & %s].read_mem();\n",
							mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask);
					}
				} else {
					if (mif.m_mifRd.m_maxRsmDly == 0) {
						mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
						mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = m_rdGrpRsmInstr[c_m%d_rdRspGrpId & %s].read_mem();\n",
							mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask);
					} else {
						mifPostInstr.Append("\t\t\tc_rdRspRsmRdy = true;\n");
						mifPostInstr.Append("\t\t\tc_rdRspRsmInstr = m_rdGrpRsmInstr[c_m%d_rdRspGrpId & %s].read_mem();\n",
							mif.m_mifRd.m_bPoll ? 3 : 1, pRdGrpIdMask);
					}
				}
			}

			mifPostInstr.Append("\t\t}\n");
			mifPostInstr.Append("\t}\n");
		}

		if (mif.m_mifRd.m_maxRsmDly > 0) {
			if (mifRsmCnt > 1) {
				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					mifPostInstr.Append("\tif (r_rdRspRsmValid[%d])\n", mif.m_mifRd.m_maxRsmDly-1);
					mifPostInstr.Append("\t\tm_mif_rdRsmQue.push(r_rdRspRsm[%d]);\n", mif.m_mifRd.m_maxRsmDly-1);
				} else {
					mifPostInstr.Append("\tif (r_rdRspRsmRdy[%d]) {\n", mif.m_mifRd.m_maxRsmDly-1);
					mifPostInstr.Append("\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\tc_t0_rsmHtInstr = r_rdRspRsmInstr[%d];\n", mif.m_mifRd.m_maxRsmDly-1);
					mifPostInstr.Append("\t}\n");
				}
			} else {
				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					mifPostInstr.Append("\tc_t0_rsmHtRdy = r_rdRspRsmValid[%d];\n", mif.m_mifRd.m_maxRsmDly-1);
					mifPostInstr.Append("\tc_t0_rsmHtId = r_rdRspRsm[%d].m_htId;\n", mif.m_mifRd.m_maxRsmDly-1);
					mifPostInstr.Append("\tc_t0_rsmHtInstr = r_rdRspRsm[%d].m_htInstr;\n", mif.m_mifRd.m_maxRsmDly-1);
				} else {
					mifPostInstr.Append("\tc_t0_rsmHtRdy = r_rdRspRsmRdy[%d];\n", mif.m_mifRd.m_maxRsmDly-1);
					mifPostInstr.Append("\tc_t0_rsmHtInstr = r_rdRspRsmInstr[%d];\n", mif.m_mifRd.m_maxRsmDly-1);
				}
			}
			mifPostInstr.Append("\n");
		}
	}

	if (mif.m_bMifRd) {
		char rdRspGrpId[128];
		if (mif.m_mifRd.m_rspGrpId.size() == 0)
			sprintf(rdRspGrpId, "r_t%d_htId", mod.m_execStg-1);
		else if (mif.m_mifRd.m_bRspGrpIdPriv) {
			if (mod.m_threads.m_htIdW.AsInt() == 0)
				sprintf(rdRspGrpId, "r_htPriv.m_%s",
					mif.m_mifRd.m_rspGrpId.AsStr().c_str());
			else
				sprintf(rdRspGrpId, "c_t%d_htPriv.m_%s",
					mod.m_execStg-1, mif.m_mifRd.m_rspGrpId.AsStr().c_str());
		} else
			sprintf(rdRspGrpId, "c_%s", mif.m_mifRd.m_rspGrpId.AsStr().c_str());

		if (mif.m_mifRd.m_rspGrpIdW == 0) {
			mifPostInstr.Append("\t// update r_rdGrpRspCnt\n");

			if (mif.m_bMifRd && mif.m_bMifWr)
				mifPostInstr.Append("\tbool c_t%d_bRdReqRdy = c_t%d_%sToMif_reqRdy%s && c_t%d_bReadMem;\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), notReqBusy, mod.m_execStg);
			else
				mifPostInstr.Append("\tbool c_t%d_bRdReqRdy = c_t%d_%sToMif_reqRdy%s;\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), notReqBusy);

			mifPostInstr.Append("\tc_rdGrpRspCnt = r_rdGrpRspCnt;\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.NewLine();

			if (mif.m_mifRd.m_bMultiRd) {
				mifPostInstr.Append("\tif (c_m%d_rdRspRdy)\n", mif.m_mifRd.m_bPoll ? 3 : 1);
				mifPostInstr.Append("\t\tc_rdGrpRspCnt -= 1;\n");
				mifPostInstr.Append("\tif (c_t%d_bRdReqRdy)\n", mod.m_execStg);
				if (is_wx)
					mifPostInstr.Append("\t\tc_rdGrpRspCnt = c_rdGrpRspCnt + (c_t%d_reqQwCnt == 1 ? 1 : 8u);\n", mod.m_execStg);
				else
					mifPostInstr.Append("\t\tc_rdGrpRspCnt += c_t%d_reqQwCnt;\n", mod.m_execStg);
			} else {
				mifPostInstr.Append("\tif (c_m%d_rdRspRdy != c_t%d_bRdReqRdy)\n",
					mif.m_mifRd.m_bPoll ? 3 : 1, mod.m_execStg);
				mifPostInstr.Append("\t\tc_rdGrpRspCnt += c_m%d_rdRspRdy ? (sc_uint<%s_RD_RSP_CNT_W>)-1 : (sc_uint<%s_RD_RSP_CNT_W>)+1;\n",
					mif.m_mifRd.m_bPoll ? 3 : 1, mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());
			}
			mifPostInstr.NewLine();

			mifPostInstr.Append("\tsc_uint<%s_RD_RSP_CNT_W> c_t%d_rdRspBusyCnt = c_rdGrpRspCnt;\n",
				mod.m_modName.Upper().c_str(), mod.m_execStg-1);

		} else if (mif.m_mifRd.m_rspGrpIdW <= 2) {

			mifPostInstr.Append("\t// update r_rdGrpRspCnt\n");
			mifPostInstr.Append("\tbool c_m%d_bRdRspRdy[%d];\n",
				mif.m_mifRd.m_bPoll ? 2 : 0,
				1 << mif.m_mifRd.m_rspGrpIdW);
			mifPostInstr.Append("\tbool c_m%d_bRdRspRdy[%d];\n",
				mif.m_mifRd.m_bPoll ? 3 : 1,
				1 << mif.m_mifRd.m_rspGrpIdW);

			mifPostInstr.NewLine();

			mifPostInstr.Append("\tfor (unsigned i = 0; i < %du; i += 1) {\n", 1<<mif.m_mifRd.m_rspGrpIdW);
			mifPostInstr.Append("\t\tc_m%d_bRdRspRdy[i] = c_m%d_rdRspRdy && (c_m%d_rdRspGrpId & %s) == i;\n",
				mif.m_mifRd.m_bPoll ? 2 : 0,
				mif.m_mifRd.m_bPoll ? 2 : 0,
				mif.m_mifRd.m_bPoll ? 2 : 0,
				pRdGrpIdMask);
			mifPostInstr.Append("\t\tc_m%d_bRdRspRdy[i] = r_m%d_bRdRspRdy[i];\n",
				mif.m_mifRd.m_bPoll ? 3 : 1,
				mif.m_mifRd.m_bPoll ? 3 : 1);

			mifPostInstr.NewLine();

			if (mif.m_bMifRd && mif.m_bMifWr)
				mifPostInstr.Append("\t\tbool c_t%d_bRdReqRdy = c_t%d_%sToMif_reqRdy%s && c_t%d_bReadMem && (c_t%d_%sToMif_req.m_tid & %s) == i;\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), notReqBusy, mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), pRdGrpIdMask);
			else
				mifPostInstr.Append("\t\tbool c_t%d_bRdReqRdy = c_t%d_%sToMif_reqRdy%s && (c_t%d_%sToMif_req.m_tid & %s) == i;\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), notReqBusy, mod.m_execStg, mod.m_modName.Lc().c_str(), pRdGrpIdMask);

			mifPostInstr.Append("\t\tc_rdGrpRspCnt[i] = r_rdGrpRspCnt[i];\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.NewLine();

			if (mif.m_mifRd.m_bMultiRd) {
				mifPostInstr.Append("\t\tif (c_m%d_bRdRspRdy[i] || c_t%d_bRdReqRdy) {\n",
					mif.m_mifRd.m_bPoll ? 3 : 1, mod.m_execStg);
				mifPostInstr.Append("\t\t\tif (c_m%d_bRdRspRdy[i])\n", mif.m_mifRd.m_bPoll ? 3 : 1);
				mifPostInstr.Append("\t\t\t\tc_rdGrpRspCnt[i] -= 1;\n");
				mifPostInstr.Append("\t\t\tif (c_t%d_bRdReqRdy)\n", mod.m_execStg);
				if (is_wx)
					mifPostInstr.Append("\t\t\t\tc_rdGrpRspCnt[i] = c_rdGrpRspCnt[i] + (c_t%d_reqQwCnt == 1 ? 1 : 8u);\n", mod.m_execStg);
				else
					mifPostInstr.Append("\t\t\t\tc_rdGrpRspCnt[i] += c_t%d_reqQwCnt;\n", mod.m_execStg);
				mifPostInstr.Append("\t\t}\n");
			} else {
				mifPostInstr.Append("\t\tif (c_m%d_bRdRspRdy[i] != c_t%d_bRdReqRdy)\n",
					mif.m_mifRd.m_bPoll ? 3 : 1, mod.m_execStg);
				mifPostInstr.Append("\t\t\tc_rdGrpRspCnt[i] += c_m%d_bRdRspRdy[i] ? (sc_uint<%s_RD_RSP_CNT_W>)-1 : (sc_uint<%s_RD_RSP_CNT_W>)+1;\n",
					mif.m_mifRd.m_bPoll ? 3 : 1, mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());
			}

			if (mif.m_mifRd.m_bPoll && mif.m_mifRd.m_rspGrpId.size() > 0) {
				mifPostInstr.Append("\n");
				mifPostInstr.Append("\t\tc_rdGrpRspCntZero[i] = c_rdGrpRspCnt[i] == 0;\n");
			}
			mifPostInstr.Append("\t}\n");
			mifPostInstr.NewLine();

			mifPostInstr.Append("\tsc_uint<%s_RD_RSP_CNT_W> c_t%d_rdRspBusyCnt = c_rdGrpRspCnt[INT(%s)];\n",
				mod.m_modName.Upper().c_str(), mod.m_execStg-1, rdRspGrpId);
		} else {

			mifPostInstr.Append("\t// update m_rdGrpRspCnt rams\n");
			mifPostInstr.Append("\tsc_uint<%s_RD_GRP_ID_W> c_rdGrpRspCntAddr[%d];\n",
				mod.m_modName.Upper().c_str(),
				rdGrpBankCnt);
			mifPostInstr.Append("\tsc_uint<%s_RD_RSP_CNT_W> c_rdGrpRspCnt[%d];\n",
				mod.m_modName.Upper().c_str(),
				rdGrpBankCnt);
			mifPostInstr.Append("\tbool c_m%d_bRdRspRdy[%d];\n",
				mif.m_mifRd.m_bPoll ? 2 : 0,
				rdGrpBankCnt);
			mifPostInstr.Append("\tbool c_m%d_bRdRspRdy[%d];\n",
				mif.m_mifRd.m_bPoll ? 3 : 1,
				rdGrpBankCnt);

			mifPostInstr.NewLine();
			mifPostInstr.Append("\tfor (unsigned i = 0; i < %du; i += 1) {\n", rdGrpBankCnt);
			mifPostInstr.Append("\t\tc_m%d_bRdRspRdy[i] = c_m%d_rdRspRdy && (c_m%d_rdRspGrpId & %s) == i;\n",
				mif.m_mifRd.m_bPoll ? 2 : 0,
				mif.m_mifRd.m_bPoll ? 2 : 0,
				mif.m_mifRd.m_bPoll ? 2 : 0,
				pRdGrpIdMask);
			mifPostInstr.Append("\t\tc_m%d_bRdRspRdy[i] = r_m%d_bRdRspRdy[i];\n",
				mif.m_mifRd.m_bPoll ? 3 : 1,
				mif.m_mifRd.m_bPoll ? 3 : 1);

			mifPostInstr.Append("\t\tc_rdGrpRspCntAddr[i] = c_m%d_bRdRspRdy[i] ? c_m%d_rdRspGrpId : %s;\n",
				mif.m_mifRd.m_bPoll ? 2 : 0, mif.m_mifRd.m_bPoll ? 2 : 0, rdRspGrpId);

			mifPostInstr.Append("\t\tm_rdGrpRspCnt[i].read_addr(c_rdGrpRspCntAddr[i] >> %s);\n", pRdGrpShfAmt);
			mifPostInstr.Append("\t\tbool bBypass = c_rdGrpRspCntAddr[i] == r_rdGrpRspCntAddr[i] && (r_rdGrpRspCntAddr[i] & %s) == i;\n",
				pRdGrpIdMask);
			mifPostInstr.NewLine();

			if (mif.m_bMifRd && mif.m_bMifWr)
				mifPostInstr.Append("\t\tbool c_t%d_bRdReqRdy = c_t%d_%sToMif_reqRdy%s && c_t%d_bReadMem && (c_t%d_%sToMif_req.m_tid & %s) == i;\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), notReqBusy, mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), pRdGrpIdMask);
			else
				mifPostInstr.Append("\t\tbool c_t%d_bRdReqRdy = c_t%d_%sToMif_reqRdy%s && (c_t%d_%sToMif_req.m_tid & %s) == i;\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), notReqBusy, mod.m_execStg, mod.m_modName.Lc().c_str(), pRdGrpIdMask);

			mifPostInstr.Append("\t\tsc_uint<%s_RD_RSP_CNT_W> c_rdGrpRspCntOut = r_rdGrpRspCnt[i];\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.NewLine();

			if (mif.m_mifRd.m_bMultiRd) {
				mifPostInstr.Append("\t\tif ((c_m%d_bRdRspRdy[i] || c_t%d_bRdReqRdy) && r_rdRspGrpInitCnt[%s_RD_GRP_ID_W] == 1) {\n",
					mif.m_mifRd.m_bPoll ? 3 : 1, mod.m_execStg, mod.m_modName.Upper().c_str());

				mifPostInstr.Append("\t\t\tif (c_m%d_bRdRspRdy[i])\n", mif.m_mifRd.m_bPoll ? 3 : 1);
				mifPostInstr.Append("\t\t\t\tc_rdGrpRspCntOut -= 1;\n");
				mifPostInstr.Append("\t\t\tif (c_t%d_bRdReqRdy)\n", mod.m_execStg);
				if (is_wx)
					mifPostInstr.Append("\t\t\t\tc_rdGrpRspCntOut = c_rdGrpRspCntOut + (c_t%d_reqQwCnt == 1 ? 1 : 8u);\n", mod.m_execStg);
				else
					mifPostInstr.Append("\t\t\t\tc_rdGrpRspCntOut += c_t%d_reqQwCnt;\n", mod.m_execStg);
			} else {
				mifPostInstr.Append("\t\tif (c_m%d_bRdRspRdy[i] != c_t%d_bRdReqRdy && r_rdRspGrpInitCnt[%s_RD_GRP_ID_W] == 1) {\n",
					mif.m_mifRd.m_bPoll ? 3 : 1, mod.m_execStg, mod.m_modName.Upper().c_str());

				mifPostInstr.Append("\t\t\tc_rdGrpRspCntOut += c_m%d_bRdRspRdy[i] ? (sc_uint<%s_RD_RSP_CNT_W>)-1 : (sc_uint<%s_RD_RSP_CNT_W>)+1;\n",
					mif.m_mifRd.m_bPoll ? 3 : 1, mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());
			}

			mifPostInstr.Append("\t\t\tm_rdGrpRspCnt[i].write_addr(r_rdGrpRspCntAddr[i] >> %s);\n", pRdGrpShfAmt);
			mifPostInstr.Append("\t\t\tm_rdGrpRspCnt[i].write_mem(c_rdGrpRspCntOut);\n");

			if (mif.m_mifRd.m_bPoll && mif.m_mifRd.m_rspGrpId.size() > 0) {
				mifPostInstr.NewLine();
				mifPostInstr.Append("\t\t\tm_rdGrpRspCntZero[i].write_addr(r_rdGrpRspCntAddr[i] >> %s);\n", pRdGrpShfAmt);
				mifPostInstr.Append("\t\t\tm_rdGrpRspCntZero[i].write_mem(c_rdGrpRspCntOut == 0);\n");
			}
			mifPostInstr.Append("\t\t}\n");
			mifPostInstr.NewLine();
			mifPostInstr.Append("\t\tc_rdGrpRspCnt[i] = bBypass ? c_rdGrpRspCntOut : m_rdGrpRspCnt[i].read_mem();\n");
			mifPostInstr.Append("\t}\n");
			mifPostInstr.NewLine();

			mifPostInstr.Append("\tsc_uint<%s_RD_RSP_CNT_W> c_t%d_rdRspBusyCnt = c_rdGrpRspCnt[%s & %s];\n",
				mod.m_modName.Upper().c_str(), mod.m_execStg-1, rdRspGrpId, pRdGrpIdMask);
		}

		mifPostInstr.Append("\tbool c_t%d_rdRspBusyCntMax = c_t%d_rdRspBusyCnt >= ((1ul << %s_RD_RSP_CNT_W)-%d);\n",
			mod.m_execStg-1, mod.m_execStg-1, mod.m_modName.Upper().c_str(), mod.m_mif.m_mifRd.m_bMultiRd ? 8 : 1);
		if (mif.m_mifRd.m_bPause)
			mifPostInstr.Append("\tbool c_t%d_rdRspPauseCntZero = c_t%d_rdRspBusyCnt == 0;\n",
			mod.m_execStg-1, mod.m_execStg-1);
		if (mif.m_mifRd.m_bPoll) {
			if (mif.m_mifRd.m_rspGrpId.size() == 0)
				mifPostInstr.Append("\tbool c_t%d_rdRspPollCntZero = c_t%d_rdRspBusyCnt == 0;\n",
					mod.m_execStg-1, mod.m_execStg-1);
		}
		mifPostInstr.NewLine();

		if (mif.m_mifRd.m_rspGrpIdW > 2) {
			mifPostInstr.Append("\tbool c_t%d_bReadRspBusy = c_m%d_rdRspRdy && ((c_m%d_rdRspGrpId & %s) == (%s & %s));\n",
				mod.m_execStg-1,
				mif.m_mifRd.m_bPoll ? 2 : 0,
				mif.m_mifRd.m_bPoll ? 2 : 0,
				pRdGrpIdMask, rdRspGrpId, pRdGrpIdMask);
			mifPostInstr.NewLine();
		}
	}

	if (bNeed_reqBusy) {
		mifPostInstr.Append("\tif (r_t%d_reqQwIdx > 0) {\n", mod.m_execStg+1);
		mifPostInstr.Append("\t\tc_t%d_%sToMif_reqRdy = true;\n",
			mod.m_execStg+1, mod.m_modName.Lc().c_str());
		mifPostInstr.Append("\t\tc_t%d_%sToMif_req = r_t%d_%sToMif_req;\n",
			mod.m_execStg+1, mod.m_modName.Lc().c_str(), mod.m_execStg+2, mod.m_modName.Lc().c_str());
		mifPostInstr.Append("\t\tc_t%d_%sToMif_req.m_addr &= ~0x38ULL;\n",
			mod.m_execStg+1, mod.m_modName.Lc().c_str());
		mifPostInstr.Append("\t\tc_t%d_%sToMif_req.m_addr |= (r_t%d_%sToMif_req.m_addr + 8) & 0x38;\n",
			mod.m_execStg+1, mod.m_modName.Lc().c_str(), mod.m_execStg+2, mod.m_modName.Lc().c_str());
		if (bNeed_reqQwIdx)
			mifPostInstr.Append("\t\tc_t%d_reqQwCnt = r_t%d_reqQwCnt;\n", mod.m_execStg+1, mod.m_execStg+2);
		if (bNeed_reqQwSplit)
			mifPostInstr.Append("\t\tc_t%d_reqQwSplit = r_t%d_reqQwSplit;\n", mod.m_execStg+1, mod.m_execStg+2);
		if (mif.m_mifWr.m_bReqPause)
			mifPostInstr.Append("\t\tc_t%d_bWrpRsm = r_t%d_bWrpRsm;\n", mod.m_execStg+1, mod.m_execStg+2);
		mifPostInstr.NewLine();

		char const * pRdRspGrpW = mif.m_mifRd.m_rspGrpId.size() == 0 ? "_HTID_W" : "_RD_GRP_ID_W";

		if (!is_wx && mif.m_mifRd.m_bMultiRd) {
			mifPostInstr.Append("\t\tif (c_t%d_%sToMif_req.m_type == MEM_REQ_RD) {\n",
				mod.m_execStg+1, mod.m_modName.Lc().c_str());

			if (mif.m_mifRd.m_rdDstList.size() > 1) {
				mifPostInstr.Append("\t\t\tsc_uint<%s_MIF_DST_IDX_W> c_t%d_mifDstIdx = (c_t%d_%sToMif_req.m_tid >> (%s_MIF_DST_IDX_SHF + 1 + %s%s))\n",
					mod.m_modName.Upper().c_str(),
					mod.m_execStg+1, mod.m_execStg+1, mod.m_modName.Lc().c_str(),
					mod.m_modName.Upper().c_str(),
					mod.m_modName.Upper().c_str(), pRdRspGrpW);
				mifPostInstr.Append("\t\t\t\t& %s_MIF_DST_IDX_MSK;\n", mod.m_modName.Upper().c_str());
				mifPostInstr.Append("\n");
				mifPostInstr.Append("\t\t\tswitch (c_t%d_mifDstIdx) {\n", mod.m_execStg+1);
			}

			for (size_t i = 0; i < mif.m_mifRd.m_rdDstList.size(); i += 1) {
				CMifRdDst &mifRdDst = mif.m_mifRd.m_rdDstList[i];

				char *pTabs = "\t\t\t";
				if (mif.m_mifRd.m_rdDstList.size() > 1) {
					mifPostInstr.Append("\t\t\tcase %d:\n", (int)i);
					mifPostInstr.Append("\t\t\t\t{\n");
					pTabs = "\t\t\t\t\t";
				}

				if (mif.m_mifRd.m_bMultiRd && mifRdDst.m_dstIdx.size() > 0) {
					char const * pIdxName = 0;
					if (mifRdDst.m_dstIdx == "fldIdx1")
						pIdxName = "FLD_IDX1";
					else if (mifRdDst.m_dstIdx == "fldIdx2")
						pIdxName = "FLD_IDX2";
					else if (mifRdDst.m_dstIdx == "varIdx1")
						pIdxName = "VAR_IDX1";
					else if (mifRdDst.m_dstIdx == "varIdx2")
						pIdxName = "VAR_IDX2";
					else if (mifRdDst.m_dstIdx == "varAddr1")
						pIdxName = "VAR_ADDR1";
					else if (mifRdDst.m_dstIdx == "varAddr2")
						pIdxName = "VAR_ADDR2";
					else if (mifRdDst.m_dstIdx == "rspIdx")
						pIdxName = "RSP_IDX";
					else
						HtlAssert(0);

					mifPostInstr.Append("%ssc_uint<%s_MIF_DST_%s_%s_W> %s = (c_t%d_%sToMif_req.m_tid\n", pTabs,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(), pIdxName,
						mifRdDst.m_dstIdx.c_str(),
						mod.m_execStg+1, mod.m_modName.Lc().c_str());
					mifPostInstr.Append("%s\t>> (%s_MIF_DST_%s_%s_SHF + 1 + %s%s)) & %s_MIF_DST_%s_%s_MSK;\n", pTabs,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(), pIdxName,
						mod.m_modName.Upper().c_str(), pRdRspGrpW,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(), pIdxName);
					mifPostInstr.Append("%s%s += 1u;\n", pTabs, mifRdDst.m_dstIdx.c_str());
					mifPostInstr.Append("%sc_t%d_%sToMif_req.m_tid = (c_t%d_%sToMif_req.m_tid &\n", pTabs,
						mod.m_execStg+1, mod.m_modName.Lc().c_str(),
						mod.m_execStg+1, mod.m_modName.Lc().c_str());
					mifPostInstr.Append("%s\t~(%s_MIF_DST_%s_%s_MSK << (%s_MIF_DST_%s_%s_SHF + 1 + %s%s))) |\n", pTabs,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(), pIdxName,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(), pIdxName,
						mod.m_modName.Upper().c_str(), pRdRspGrpW);
					mifPostInstr.Append("%s\t(%s << (%s_MIF_DST_%s_%s_SHF + 1 + %s%s));\n", pTabs,
						mifRdDst.m_dstIdx.c_str(),
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(), pIdxName,
						mod.m_modName.Upper().c_str(), pRdRspGrpW);
				}

				if (mif.m_mifRd.m_rdDstList.size() > 1) {
					mifPostInstr.Append("\t\t\t\t}\n");
					mifPostInstr.Append("\t\t\t\tbreak;\n");
				}
			}

			if (mif.m_mifRd.m_rdDstList.size() > 1) {
				mifPostInstr.Append("\t\t\tdefault:\n");
				mifPostInstr.Append("\t\t\t\tassert(0);\n");
				mifPostInstr.Append("\t\t\t}\n");
			}

			mifPostInstr.Append("\t\t}\n");
		}

		mifPostInstr.Append("\t}\n");
		mifPostInstr.NewLine();
	}

	int mifReqStgCnt = 0;

	if (mif.m_bMifRd) {
		mifPostInstr.Append("\t// write read response to ram\n");

		if (!is_wx && mif.m_mifRd.m_bMultiRd)
			mifReqStgCnt = 1;

		if (mif.m_mifRd.m_rspCntW.AsInt() > 0)
			mifPostInstr.Append("\tif (r_m1_rdRspRdy) {\n");
		else
			mifPostInstr.Append("\tif (i_mifTo%sP0_rdRspRdy.read()) {\n", mod.m_modName.Uc().c_str());

		char *pTabs = "\t";
		if (mif.m_mifRd.m_rdDstList.size() > 1)
			pTabs = "\t\t";

		if (is_wx && mif.m_mifRd.m_bMultiRd) {
			mifPostInstr.Append("%sht_uint3 c_m1_rdRspQwFirst = (c_m1_rdRsp_dst >> %s_MIF_DST_QW_FIRST_SHF) & %s_MIF_DST_QW_FIRST_MSK;\n",
				pTabs, mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());
			mifPostInstr.Append("%sht_uint3 c_m1_rdRspQwLast = (c_m1_rdRsp_dst >> %s_MIF_DST_QW_LAST_SHF) & %s_MIF_DST_QW_LAST_MSK;\n",
				pTabs, mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());
			mifPostInstr.Append("%sbool c_m1_varWrEn = c_m1_rdRspQwFirst <= c_m1_rdRspQwIdx && c_m1_rdRspQwIdx <= c_m1_rdRspQwLast;\n", pTabs);
			mifPostInstr.Append("\n");
		}

		if (mif.m_mifRd.m_rdDstList.size() > 1) {
			mifPostInstr.Append("\t\tswitch (c_m1_rdRsp_dstIdx) {\n");
			pTabs = "\t\t\t";
		} else
			pTabs = "\t\t";

		for (size_t i = 0; i < mif.m_mifRd.m_rdDstList.size(); i += 1) {
			CMifRdDst &mifRdDst = mif.m_mifRd.m_rdDstList[i];

			if (mif.m_mifRd.m_rdDstList.size() > 1) {
				mifPostInstr.Append("\t\tcase %d:\n", (int)i);
				mifPostInstr.Append("\t\t\t{\n");
				pTabs = "\t\t\t\t";
			}

			// read rsp data to rdRspData variable
			if (mif.m_mifRd.m_rspCntW.AsInt() > 0) {

				mifPostInstr.Append("%sht_uint64 c_m1_rdRspData = (%s_t)r_m1_rdRsp.m_data;\n", pTabs, mifRdDst.m_rdType.c_str());

			} else {

				mifPostInstr.Append("%sht_uint64 c_m1_rdRspData = (%s_t)i_mifTo%sP0_rdRsp.read().m_data;\n", pTabs,
					mifRdDst.m_rdType.c_str(), mod.m_modName.Uc().c_str());
			}

			if (mifRdDst.m_infoW.size() > 0) {
				// function call

				if (mifRdDst.m_infoW.AsInt() > 0) {
					mifPostInstr.Append("%ssc_uint<%s_MIF_DST_%s_INFO_W> c_m1_rdRsp_info = (c_m1_rdRsp_dst\n", pTabs,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());
					mifPostInstr.Append("%s\t>> %s_MIF_DST_%s_INFO_SHF) & %s_MIF_DST_%s_INFO_MSK;\n", pTabs,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());
				}
				mifPostInstr.NewLine();

				if (mifRdDst.m_bMultiRd && !is_wx) {
					mifPostInstr.Append("%ssc_uint<%s_MIF_DST_%s_RSP_IDX_W> c_m1_rspIdx = (c_m1_rdRsp_dst\n", pTabs,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());
					mifPostInstr.Append("%s\t>> %s_MIF_DST_%s_RSP_IDX_SHF) & %s_MIF_DST_%s_RSP_IDX_MSK;\n", pTabs,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());

					//if (bNeed_rdRspQwIdx)
					//	mifPostInstr.Append("%sc_m1_rspIdx += c_m1_rdRspQwIdx;\n", pTabs);
					mifPostInstr.NewLine();
				}

				if (is_wx && mifRdDst.m_bMultiRd)
					mifPostInstr.Append("%sif (c_m1_varWrEn)\n\t", pTabs);

				mifPostInstr.Append("%sReadMemResp_%s(", pTabs, mifRdDst.m_name.c_str());
				m_mifFuncDecl.Append("\tvoid ReadMemResp_%s(", mifRdDst.m_name.c_str());

				if (mifRdDst.m_bMultiRd) {
					if (is_wx)
						mifPostInstr.Append("c_m1_rdRspQwIdx - c_m1_rdRspQwFirst, ");
					else
						mifPostInstr.Append("c_m1_rspIdx, ");
					m_mifFuncDecl.Append("ht_uint3 rspIdx, ");
				}

				if (mif.m_mifRd.m_rspGrpIdW > 0) {
					mifPostInstr.Append("c_m1_rdRspGrpId, ");
					m_mifFuncDecl.Append("sc_uint<%s_RD_GRP_ID_W> rdRspGrpId, ", mod.m_modName.Upper().c_str());
				}

				if (mifRdDst.m_infoW.AsInt() > 0) {
					mifPostInstr.Append("c_m1_rdRsp_info, ");
					m_mifFuncDecl.Append("sc_uint<%s_MIF_DST_%s_INFO_W> rdRsp_info, ",
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());
				}

				mifPostInstr.Append("c_m1_rdRspData);\n");
				m_mifFuncDecl.Append("ht_uint64 rdRspData);\n");

			} else {

				if (mifRdDst.m_varAddr0W > 0) {
					mifPostInstr.Append("%ssc_uint<%s_MIF_DST_%s_VAR_ADDR0_W> c_m1_varAddr0 = (c_m1_rdRsp_dst >> %s_MIF_DST_%s_VAR_ADDR0_SHF) & %s_MIF_DST_%s_VAR_ADDR0_MSK;\n", pTabs,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());
				}

				if (mifRdDst.m_varAddr1W > 0) {
					mifPostInstr.Append("%ssc_uint<%s_MIF_DST_%s_VAR_ADDR1_W> c_m1_varAddr1 = (c_m1_rdRsp_dst >> %s_MIF_DST_%s_VAR_ADDR1_SHF) & %s_MIF_DST_%s_VAR_ADDR1_MSK;\n", pTabs,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());
				}

				if (mifRdDst.m_varAddr2W > 0) {
					mifPostInstr.Append("%ssc_uint<%s_MIF_DST_%s_VAR_ADDR2_W> c_m1_varAddr2 = (c_m1_rdRsp_dst >> %s_MIF_DST_%s_VAR_ADDR2_SHF) & %s_MIF_DST_%s_VAR_ADDR2_MSK;\n", pTabs,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());
				}

				string varIdx;
				if (mifRdDst.m_varDimen1 > 0) {
					varIdx = "[INT(c_m1_varIdx1)]";
					mifPostInstr.Append("%ssc_uint<%s_MIF_DST_%s_VAR_IDX1_W> c_m1_varIdx1 = (c_m1_rdRsp_dst >> %s_MIF_DST_%s_VAR_IDX1_SHF) & %s_MIF_DST_%s_VAR_IDX1_MSK;\n", pTabs,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());
				}

				if (mifRdDst.m_varDimen2 > 0) {
					varIdx += "[INT(c_m1_varIdx2)]";
					mifPostInstr.Append("%ssc_uint<%s_MIF_DST_%s_VAR_IDX2_W> c_m1_varIdx2 = (c_m1_rdRsp_dst >> %s_MIF_DST_%s_VAR_IDX2_SHF) & %s_MIF_DST_%s_VAR_IDX2_MSK;\n", pTabs,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());
				}

				string fldIdx;
				if (mifRdDst.m_fldDimen1 > 0) {
					fldIdx = "[INT(c_m1_fldIdx1)]";
					mifPostInstr.Append("%ssc_uint<%s_MIF_DST_%s_FLD_IDX1_W> c_m1_fldIdx1 = (c_m1_rdRsp_dst\n", pTabs,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());
					mifPostInstr.Append("%s\t>> %s_MIF_DST_%s_FLD_IDX1_SHF) & %s_MIF_DST_%s_FLD_IDX1_MSK;\n", pTabs,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());
				}

				if (mifRdDst.m_fldDimen2 > 0) {
					fldIdx += "[INT(c_m1_fldIdx2)]";
					mifPostInstr.Append("%ssc_uint<%s_MIF_DST_%s_FLD_IDX2_W> c_m1_fldIdx2 = (c_m1_rdRsp_dst\n", pTabs,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());
					mifPostInstr.Append("%s\t>> %s_MIF_DST_%s_FLD_IDX2_SHF) & %s_MIF_DST_%s_FLD_IDX2_MSK;\n", pTabs,
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(),
						mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str());
				}

				// memory destination address and write enables
				string dotFieldName;
				if (mifRdDst.m_pNgvRam == 0)
					dotFieldName = mifRdDst.m_field.size() > 0 ? (string(".m_") + mifRdDst.m_field) : "";
				else
					dotFieldName = mifRdDst.m_field.size() > 0 ? (string(".") + mifRdDst.m_field) : "";

				// generate bit range for subrange writes
				char dataIdx[32] = "";
				if (mifRdDst.m_dataLsb.AsInt() > 0 || mifRdDst.m_fldW != 64)
					sprintf(dataIdx, "(%d,%d)", mifRdDst.m_fldW + mifRdDst.m_dataLsb.AsInt()-1, mifRdDst.m_dataLsb.AsInt());

				string dstIdx;
				// write rdRspData to destination
				if (mifRdDst.m_pSharedVar) {
					CField &sharedVar = *mifRdDst.m_pSharedVar;

					// scalar variable
					string fieldStr;
					if (mifRdDst.m_field.size() > 0)
						fieldStr = "." + mifRdDst.m_field;

					if (bNeed_rdRspQwIdx && mifRdDst.m_dstIdx.size() > 0 && (is_wx || mifRdDst.m_memSrc == "host")) {
						string idxVarName = mifRdDst.m_dstIdx[0] == 'v' ? "VAR_" : "FLD_";
						idxVarName += mifRdDst.m_dstIdx[3] == 'A' ? "ADDR" : "IDX";
						int backIdx = mifRdDst.m_dstIdx.size() - 1;
						idxVarName += mifRdDst.m_dstIdx[backIdx] == '1' ? "1" : "2";

						if (is_wx && mifRdDst.m_bMultiRd)
							mifPostInstr.Append("%sc_m1_%s += (sc_uint<%s_MIF_DST_%s_%s_W>)(c_m1_rdRspQwIdx - c_m1_rdRspQwFirst);\n", pTabs,
							mifRdDst.m_dstIdx.c_str(),
							mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(), idxVarName.c_str());
						else
							mifPostInstr.Append("%sc_m1_%s += (sc_uint<%s_MIF_DST_%s_%s_W>)c_m1_rdRspQwIdx;\n", pTabs,
							mifRdDst.m_dstIdx.c_str(),
							mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(), idxVarName.c_str());
					}

					if (sharedVar.m_addr1W.AsInt() > 0) {

						if (mifRdDst.m_varAddr1W > 0 && mifRdDst.m_varAddr2W > 0)
							mifPostInstr.Append("%sm_%s%s.write_addr( c_m1_varAddr1, c_m1_varAddr2 );\n",
							pTabs, mifRdDst.m_var.c_str(), varIdx.c_str());
						else if (mifRdDst.m_varAddr1W > 0)
							mifPostInstr.Append("%sm_%s%s.write_addr(c_m1_varAddr1);\n",
							pTabs, mifRdDst.m_var.c_str(), varIdx.c_str());
						else if (mifRdDst.m_varAddr2W > 0)
							mifPostInstr.Append("%sm_%s%s.write_addr(c_m1_varAddr2);\n",
							pTabs, mifRdDst.m_var.c_str(), varIdx.c_str());

						if (is_wx && mifRdDst.m_bMultiRd)
							mifPostInstr.Append("%sif (c_m1_varWrEn)\n\t", pTabs);

						if (fieldStr.size() == 0)
							mifPostInstr.Append("%sm_%s%s.write_mem(c_m1_rdRspData%s);\n", pTabs,
							mifRdDst.m_var.c_str(), varIdx.c_str(),
							dataIdx);
						else
							mifPostInstr.Append("%sm_%s%s.write_mem()%s%s = c_m1_rdRspData%s;\n", pTabs,
							mifRdDst.m_var.c_str(), varIdx.c_str(), fieldStr.c_str(), fldIdx.c_str(),
							dataIdx);

					} else if (sharedVar.m_queueW.AsInt() > 0) {

						if (is_wx && mifRdDst.m_bMultiRd)
							mifPostInstr.Append("%sif (c_m1_varWrEn)\n\t", pTabs);

						mifPostInstr.Append("%sm_%s%s.push( c_m1_rdRspData%s );\n", pTabs,
							mifRdDst.m_var.c_str(), varIdx.c_str(),
							dataIdx);

					} else {

						if (is_wx && mifRdDst.m_bMultiRd)
							mifPostInstr.Append("%sif (c_m1_varWrEn)\n\t", pTabs);

						string castStr;
						switch (mifRdDst.m_fldW) {
						case 8: castStr = "(uint8_t)"; break;
						case 16: castStr = "(uint16_t)"; break;
						case 32: castStr = "(uint32_t)"; break;
						default: break;
						}

						mifPostInstr.Append("%sc_%s%s%s%s = %sc_m1_rdRspData%s;\n", pTabs,
							mifRdDst.m_var.c_str(), varIdx.c_str(), fieldStr.c_str(), fldIdx.c_str(),
							castStr.c_str(),
							dataIdx);
					}
				} else if (mifRdDst.m_pNgvRam) {

					if (bNeed_rdRspQwIdx && mifRdDst.m_dstIdx.size() > 0 && (is_wx || mifRdDst.m_memSrc == "host")) {
						string idxVarName = mifRdDst.m_dstIdx[0] == 'v' ? "VAR_" : "FLD_";
						idxVarName += mifRdDst.m_dstIdx[3] == 'A' ? "ADDR" : "IDX";
						int backIdx = mifRdDst.m_dstIdx.size() - 1;
						idxVarName += mifRdDst.m_dstIdx[backIdx] == '1' ? "1" : "2";

						if (is_wx && mifRdDst.m_bMultiRd)
							mifPostInstr.Append("%sc_m1_%s += (sc_uint<%s_MIF_DST_%s_%s_W>)(c_m1_rdRspQwIdx - c_m1_rdRspQwFirst);\n", pTabs,
							mifRdDst.m_dstIdx.c_str(),
							mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(), idxVarName.c_str());
						else
							mifPostInstr.Append("%sc_m1_%s += (sc_uint<%s_MIF_DST_%s_%s_W>)c_m1_rdRspQwIdx;\n", pTabs,
							mifRdDst.m_dstIdx.c_str(),
							mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(), idxVarName.c_str());
					}
					if (is_wx && mifRdDst.m_bMultiRd)
						mifPostInstr.Append("%sc_m1_%sWrEn%s%s%s = c_m1_varWrEn;\n", pTabs,
						mifRdDst.m_var.c_str(), varIdx.c_str(), dotFieldName.c_str(), fldIdx.c_str());

					if (mifRdDst.m_varAddr1W > 0 && mifRdDst.m_varAddr2W > 0)
						mifPostInstr.Append("%sc_m1_%sMwData%s.write_addr(c_m1_varAddr1, c_m1_varAddr2);\n", pTabs,
						mifRdDst.m_var.c_str(), varIdx.c_str());
					else if (mifRdDst.m_varAddr1W > 0)
						mifPostInstr.Append("%sc_m1_%sMwData%s.write_addr(c_m1_varAddr1);\n", pTabs,
						mifRdDst.m_var.c_str(), varIdx.c_str());
					else if (mifRdDst.m_varAddr2W > 0)
						mifPostInstr.Append("%sc_m1_%sMwData%s.write_addr(c_m1_varAddr2);\n", pTabs,
						mifRdDst.m_var.c_str(), varIdx.c_str());

					string castStr;
					switch (mifRdDst.m_fldW) {
					case 8: castStr = "(uint8_t)"; break;
					case 16: castStr = "(uint16_t)"; break;
					case 32: castStr = "(uint32_t)"; break;
					default: break;
					}

					mifPostInstr.Append("%sc_m1_%sMwData%s%s%s = %sc_m1_rdRspData%s;\n", pTabs,
						mifRdDst.m_var.c_str(), varIdx.c_str(), dotFieldName.c_str(), fldIdx.c_str(),
						castStr.c_str(),
						dataIdx);

				} else {

					if (bNeed_rdRspQwIdx && mifRdDst.m_dstIdx.size() > 0 && (is_wx || mifRdDst.m_memSrc == "host"))
					{
						string idxVarName = mifRdDst.m_dstIdx[0] == 'v' ? "VAR_" : "FLD_";
						idxVarName += mifRdDst.m_dstIdx[3] == 'A' ? "ADDR" : "IDX";
						int backIdx = mifRdDst.m_dstIdx.size()-1;
						idxVarName += mifRdDst.m_dstIdx[backIdx] == '1' ? "1" : "2";

						if (is_wx && mifRdDst.m_bMultiRd)
							mifPostInstr.Append("%sc_m1_%s += (sc_uint<%s_MIF_DST_%s_%s_W>)(c_m1_rdRspQwIdx - c_m1_rdRspQwFirst);\n", pTabs,
							mifRdDst.m_dstIdx.c_str(),
							mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(), idxVarName.c_str());
						else
							mifPostInstr.Append("%sc_m1_%s += (sc_uint<%s_MIF_DST_%s_%s_W>)c_m1_rdRspQwIdx;\n", pTabs,
							mifRdDst.m_dstIdx.c_str(),
							mod.m_modName.Upper().c_str(), mifRdDst.m_name.Upper().c_str(), idxVarName.c_str());
					}
					if (is_wx && mifRdDst.m_bMultiRd)
						mifPostInstr.Append("%sc_m1_%sWrEn%s%s%s = c_m1_varWrEn;\n", pTabs,
						mifRdDst.m_var.c_str(), varIdx.c_str(), dotFieldName.c_str(), fldIdx.c_str());
					else
						mifPostInstr.Append("%sc_m1_%sWrEn%s%s%s = true;\n", pTabs,
						mifRdDst.m_var.c_str(), varIdx.c_str(), dotFieldName.c_str(), fldIdx.c_str());

					if (mifRdDst.m_varAddr0W > 0) {
						if (mifRdDst.m_varAddr1W > 0 && mifRdDst.m_varAddr2W > 0)
							mifPostInstr.Append("%sc_m1_%sWrAddr%s = (c_m1_varAddr0, c_m1_varAddr1, c_m1_varAddr2);\n", pTabs,
								mifRdDst.m_var.c_str(), varIdx.c_str());
						else if (mifRdDst.m_varAddr1W > 0)
							mifPostInstr.Append("%sc_m1_%sWrAddr%s = (c_m1_varAddr0, c_m1_varAddr1);\n", pTabs,
								mifRdDst.m_var.c_str(), varIdx.c_str());
						else if (mifRdDst.m_varAddr2W > 0)
							mifPostInstr.Append("%sc_m1_%sWrAddr%s = (c_m1_varAddr0, c_m1_varAddr2);\n", pTabs,
								mifRdDst.m_var.c_str(), varIdx.c_str());
						else
							mifPostInstr.Append("%sc_m1_%sWrAddr%s = c_m1_varAddr0;\n", pTabs,
								mifRdDst.m_var.c_str(), varIdx.c_str());
					} else {
						if (mifRdDst.m_varAddr1W > 0 && mifRdDst.m_varAddr2W > 0)
							mifPostInstr.Append("%sc_m1_%sWrAddr%s = (c_m1_varAddr1, c_m1_varAddr2);\n", pTabs,
							mifRdDst.m_var.c_str(), varIdx.c_str());
						else if (mifRdDst.m_varAddr1W > 0)
							mifPostInstr.Append("%sc_m1_%sWrAddr%s = c_m1_varAddr1;\n", pTabs,
							mifRdDst.m_var.c_str(), varIdx.c_str());
						else if (mifRdDst.m_varAddr2W > 0)
							mifPostInstr.Append("%sc_m1_%sWrAddr%s = c_m1_varAddr2;\n", pTabs,
							mifRdDst.m_var.c_str(), varIdx.c_str());
					}

					string castStr;
					switch (mifRdDst.m_fldW) {
					case 8: castStr = "(uint8_t)"; break;
					case 16: castStr = "(uint16_t)"; break;
					case 32: castStr = "(uint32_t)"; break;
					default: break;
					}

					mifPostInstr.Append("%sc_m1_%sWrData%s%s%s = %sc_m1_rdRspData%s;\n", pTabs,
						mifRdDst.m_var.c_str(), varIdx.c_str(), dotFieldName.c_str(), fldIdx.c_str(),
						castStr.c_str(),
						dataIdx);
				}
			}

			if (mif.m_mifRd.m_rdDstList.size() > 1) {
				mifPostInstr.Append("\t\t\t\tbreak;\n");
				mifPostInstr.Append("\t\t\t}\n");
			}
		}

		if (mif.m_mifRd.m_rdDstList.size() > 1) {
			mifPostInstr.Append("\t\tdefault:\n");
			mifPostInstr.Append("\t\t\tassert(0);\n");
			mifPostInstr.Append("\t\t}\n");
		}

		mifPostInstr.Append("\t}\n");
		mifPostInstr.NewLine();
	}

	if (bMifWr) {
		mifPostInstr.Append("\t// Handle write responses\n");
		mifPostInstr.Append("\tbool c_m0_wrRspRdy = i_mifTo%sP0_wrRspRdy.read();\n",
			mod.m_modName.Uc().c_str());
	}

	if (mif.m_bMifWr && mif.m_mifWr.m_bPause) {

		if (mif.m_mifWr.m_rspGrpIdW == 0) {

			mifPostInstr.Append("\tif (r_m1_wrRspRdy) {\n");
			mifPostInstr.NewLine();
			mifPostInstr.Append("\t\tbool c_m1_wrGrpRsmWait = r_wrGrpRsmWait == true || c_t%d_memWritePause;\n",
				mod.m_execStg);
			mifPostInstr.NewLine();
			mifPostInstr.Append("\t\tif (c_m1_wrGrpRsmWait && r_wrGrpRspCnt == 1) {\n");
			mifPostInstr.Append("\t\t\t// last write response arrived, resume thread\n");
			mifPostInstr.Append("\t\t\tc_wrGrpRsmWait = false;\n");

			if (mifRsmCnt > 1) {
				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					mifPostInstr.NewLine();
					mifPostInstr.Append("\t\t\tCHtRsm wrRspRsm;\n");
					mifPostInstr.Append("\t\t\twrRspRsm.m_htId = r_m1_wrRspGrpId;\n");
					mifPostInstr.Append("\t\t\twrRspRsm.m_htInstr = c_wrGrpRsmInstr;\n");
					mifPostInstr.NewLine();
					mifPostInstr.Append("\t\t\tm_mif_wrRsmQue.push(wrRspRsm);\n");
				} else {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = c_wrGrpRsmInstr;\n");
				}
			} else {
				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtId = r_m1_wrRspGrpId;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = c_wrGrpRsmInstr;\n");
				} else {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = c_wrGrpRsmInstr;\n");
				}
			}

			mifPostInstr.Append("\t\t}\n");
			mifPostInstr.Append("\t}\n");
			mifPostInstr.Append("\t\n");

		} else if (mif.m_mifWr.m_rspGrpIdW <= 2) {

			mifPostInstr.Append("\tsc_uint<%s_WR_GRP_ID_W> c_m1_wrRspGrpId = r_m1_wrRspTid & ((1 << %s_WR_GRP_ID_W)-1);\n",
				mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());

			mifPostInstr.Append("\tif (r_m1_wrRspRdy) {\n");
			mifPostInstr.Append("\t\tbool c_m1_wrGrpRsmWait = r_wrGrpRsmWait[INT(c_m1_wrRspGrpId)] == true\n");
			mifPostInstr.Append("\t\t\t|| (c_t%d_memWritePause && %s == c_m1_wrRspGrpId);\n",
				mod.m_execStg, wrRspGrpName);
			mifPostInstr.NewLine();
			mifPostInstr.Append("\t\tif (c_m1_wrGrpRsmWait && r_wrGrpRspCnt[INT(c_m1_wrRspGrpId)] == 1) {\n");
			mifPostInstr.Append("\t\t\t// last read response arrived, resume thread\n");
			mifPostInstr.Append("\t\t\tc_wrGrpRsmWait[INT(c_m1_wrRspGrpId)] = false;\n");
			mifPostInstr.NewLine();

			if (mifRsmCnt > 1) {

				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					mifPostInstr.Append("\t\t\tCHtRsm wrRspRsm;\n");
					if (mif.m_mifWr.m_rspGrpId.size() == 0)
						mifPostInstr.Append("\t\t\twrRspRsm.m_htId = c_m1_wrRspGrpId;\n");
					else
						mifPostInstr.Append("\t\t\twrRspRsm.m_htId = r_wrGrpRsmHtId[INT(c_m1_wrRspGrpId)];\n");
					mifPostInstr.Append("\t\t\twrRspRsm.m_htInstr = r_wrGrpRsmInstr[INT(c_m1_wrRspGrpId)];\n");
					mifPostInstr.NewLine();
					mifPostInstr.Append("\t\t\tm_mif_wrRsmQue.push(wrRspRsm);\n");
				} else {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = c_wrGrpRsmInstr[INT(c_m1_wrRspGrpId)];\n");
				}

			} else {

				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					if (mif.m_mifWr.m_rspGrpId.size() == 0)
						mifPostInstr.Append("\t\t\tc_t0_rsmHtId = c_m1_wrRspGrpId;\n");
					else
						mifPostInstr.Append("\t\t\tc_t0_rsmHtId = r_wrGrpRsmHtId[INT(c_m1_wrRspGrpId)];\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = c_wrGrpRsmInstr[INT(c_m1_wrRspGrpId)];\n");
				} else {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = c_wrGrpRsmInstr[INT(c_m1_wrRspGrpId)];\n");
				}
			}

			mifPostInstr.Append("\t\t}\n");
			mifPostInstr.Append("\t}\n");

		} else {

			mifPostInstr.Append("\tsc_uint<%s_WR_GRP_ID_W> c_m1_wrRspGrpId = r_m1_wrRspTid & ((1ul << %s_WR_GRP_ID_W)-1);\n",
				mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());
			mifPostInstr.Append("\tm_wrGrpRsmWait[c_m1_wrRspGrpId & %s].read_addr(c_m1_wrRspGrpId >> %s);\n", pWrGrpIdMask, pWrGrpShfAmt);
			mifPostInstr.Append("\tm_wrGrpRsmInstr[c_m1_wrRspGrpId & %s].read_addr(c_m1_wrRspGrpId >> %s);\n", pWrGrpIdMask, pWrGrpShfAmt);
			if (mif.m_mifWr.m_rspGrpId.size() != 0)
				mifPostInstr.Append("\tm_wrGrpRsmHtId[c_m1_wrRspGrpId & %s].read_addr(c_m1_wrRspGrpId >> %s);\n", pWrGrpIdMask, pWrGrpShfAmt);

			mifPostInstr.Append("\tif (r_m1_wrRspRdy) {\n");

			mifPostInstr.Append("\t\tm_wrGrpRsmWait[c_m1_wrRspGrpId & %s].write_addr(c_m1_wrRspGrpId >> %s);\n", pWrGrpIdMask, pWrGrpShfAmt);
			mifPostInstr.Append("\t\tbool c_m1_wrGrpRsmWait = m_wrGrpRsmWait[c_m1_wrRspGrpId & %s].read_mem();\n", pWrGrpIdMask);
			mifPostInstr.NewLine();
			mifPostInstr.Append("\t\tif (c_m1_wrGrpRsmWait && r_wrGrpRspCnt[c_m1_wrRspGrpId & %s] == 1) {\n", pWrGrpIdMask);

			mifPostInstr.Append("\t\t\t// last read response arrived, resume thread\n");
			mifPostInstr.Append("\t\t\tm_wrGrpRsmWait[c_m1_wrRspGrpId & %s].write_mem(false);\n", pWrGrpIdMask);
			mifPostInstr.NewLine();

			if (mifRsmCnt > 1) {

				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					mifPostInstr.Append("\t\t\tCHtRsm wrRspRsm;\n");
					if (mif.m_mifWr.m_rspGrpId.size() == 0)
						mifPostInstr.Append("\t\t\twrRspRsm.m_htId = c_m1_wrRspGrpId;\n");
					else
						mifPostInstr.Append("\t\t\twrRspRsm.m_htId = m_wrGrpRsmHtId[c_m1_wrRspGrpId & %s].read_mem();\n", pWrGrpIdMask);
					mifPostInstr.Append("\t\t\twrRspRsm.m_htInstr = m_wrGrpRsmInstr[c_m1_wrRspGrpId & %s].read_mem();\n", pWrGrpIdMask);
					mifPostInstr.NewLine();
					mifPostInstr.Append("\t\t\tm_mif_wrRsmQue.push(wrRspRsm);\n");
				} else {
					mifPostInstr.Append("\t\t\tc_bWrGrpRsmRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_wrMemRspHtInstr = m_wrGrpRsmInstr[c_m1_wrRspGrpId & %s].read_mem();\n");
				}

			} else {

				if (mod.m_threads.m_htIdW.AsInt() > 0) {
					mifPostInstr.Append("\t\t\tc_t0_rsmHtRdy = true;\n");
					if (mif.m_mifWr.m_rspGrpId.size() == 0)
						mifPostInstr.Append("\t\t\tc_t0_rsmHtId = c_m1_wrRspGrpId;\n");
					else
						mifPostInstr.Append("\t\t\tc_t0_rsmHtId = m_wrGrpRsmHtId[c_m1_wrRspGrpId & %s].read_mem();\n", pWrGrpIdMask);
					mifPostInstr.Append("\t\t\tc_t0_rsmHtInstr = m_wrGrpRsmInstr[c_m1_wrRspGrpId & %s].read_mem();\n", pWrGrpIdMask);
				} else {
					mifPostInstr.Append("\t\t\tc_bWrGrpRsmRdy = true;\n");
					mifPostInstr.Append("\t\t\tc_wrMemRspHtInstr = m_wrGrpRsmInstr[c_m1_wrRspGrpId & %s].read_mem();\n");
				}
			}

			mifPostInstr.Append("\t\t}\n");
			mifPostInstr.Append("\t}\n");
		}
	}

	int reqQwIdxStgCnt = 1;
	if (mif.m_bMifWr) {
		char wrRspGrpId[128];
		string wrRspGrpIdW;
		if (mif.m_mifWr.m_rspGrpId.size() == 0) {
			sprintf(wrRspGrpId, "r_t%d_htId", mod.m_execStg-1);
			wrRspGrpIdW = VA("%s_HTID_W", mod.m_modName.Upper().c_str());
		} else if (mif.m_mifWr.m_bRspGrpIdPriv) {
			sprintf(wrRspGrpId, "c_t%d_htPriv.m_%s",
				mod.m_execStg-1, mif.m_mifWr.m_rspGrpId.AsStr().c_str());
			wrRspGrpIdW = VA("%s_WR_GRP_ID_W", mod.m_modName.Upper().c_str());
		} else {
			sprintf(wrRspGrpId, "c_%s", mif.m_mifWr.m_rspGrpId.AsStr().c_str());
			wrRspGrpIdW = VA("%s_WR_GRP_ID_W", mod.m_modName.Upper().c_str());
		}

		if (mif.m_mifWr.m_rspGrpIdW == 0) {
			mifPostInstr.Append("\t// update r_wrGrpRspCnt\n");
			mifPostInstr.Append("\tbool c_m0_bWrRspRdy = i_mifTo%sP0_wrRspRdy.read();\n",
				mod.m_modName.Uc().c_str());

			mifPostInstr.NewLine();

			if (mif.m_bMifRd && mif.m_bMifWr)
				mifPostInstr.Append("\tbool c_t%d_bWrReqRdy = c_t%d_%sToMif_reqRdy%s && !c_t%d_bReadMem;\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), notReqBusy, mod.m_execStg, mod.m_modName.Lc().c_str());
			else
				mifPostInstr.Append("\tbool c_t%d_bWrReqRdy = c_t%d_%sToMif_reqRdy%s;\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), notReqBusy);

			mifPostInstr.Append("\tc_wrGrpRspCnt = r_wrGrpRspCnt;\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.NewLine();

			if (mif.m_mifWr.m_bMultiWr && is_hc1) {
				mifPostInstr.Append("\tif (c_t%d_bWrReqRdy)\n", mod.m_execStg);
				mifPostInstr.Append("\t\tc_wrGrpRspCnt += c_t%d_reqQwCnt;\n", mod.m_execStg);
				mifPostInstr.Append("\tif (r_m1_bWrRspRdy)\n");
				mifPostInstr.Append("\t\tc_wrGrpRspCnt -= 1;\n");

			} else if (mif.m_mifWr.m_bMultiWr && is_hc2) {
				mifPostInstr.Append("\tif (c_t%d_bWrReqRdy)\n", mod.m_execStg);
				mifPostInstr.Append("\t\tc_wrGrpRspCnt += (c_t%d_reqQwCnt == 8 && c_t%d_%sToMif_req.m_host) ? (ht_uint4)1 : c_t%d_reqQwCnt;\n",
					mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), mod.m_execStg);
				mifPostInstr.Append("\tif (r_m1_bWrRspRdy)\n");
				mifPostInstr.Append("\t\tc_wrGrpRspCnt -= 1;\n");

			} else {
				mifPostInstr.Append("\tif (r_m1_bWrRspRdy != c_t%d_bWrReqRdy)\n",
					mod.m_execStg);
				mifPostInstr.Append("\t\tc_wrGrpRspCnt += r_m1_bWrRspRdy ? (sc_uint<%s_WR_RSP_CNT_W>)-1 : (sc_uint<%s_WR_RSP_CNT_W>)+1;\n",
					mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());
			}
			mifPostInstr.NewLine();

			mifPostInstr.Append("\tsc_uint<%s_WR_RSP_CNT_W> c_t%d_wrGrpRspCnt = c_wrGrpRspCnt;\n",
				mod.m_modName.Upper().c_str(), mod.m_execStg-1);
		} else if (mif.m_mifWr.m_rspGrpIdW <= 2) {
			mifPostInstr.Append("\t// update r_wrGrpRspCnt\n");
			mifPostInstr.Append("\tbool c_m0_bWrRspRdy[%d];\n",
				1<<mif.m_mifWr.m_rspGrpIdW);
			mifPostInstr.NewLine();
			mifPostInstr.Append("\tfor (unsigned i = 0; i < %du; i += 1) {\n", 1<<mif.m_mifWr.m_rspGrpIdW);
			mifPostInstr.Append("\t\tc_m0_bWrRspRdy[i] = i_mifTo%sP0_wrRspRdy.read() && (i_mifTo%sP0_wrRspTid.read() & %s) == i;\n",
				mod.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), pWrGrpIdMask);

			mifPostInstr.NewLine();

			if (mif.m_bMifRd && mif.m_bMifWr)
				mifPostInstr.Append("\t\tbool c_t%d_bWrReqRdy = c_t%d_%sToMif_reqRdy%s && !c_t%d_bReadMem && (c_t%d_%sToMif_req.m_tid & %s) == i;\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), notReqBusy, mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), pWrGrpIdMask);
			else
				mifPostInstr.Append("\t\tbool c_t%d_bWrReqRdy = c_t%d_%sToMif_reqRdy%s && (c_t%d_%sToMif_req.m_tid & %s) == i;\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), notReqBusy, mod.m_execStg, mod.m_modName.Lc().c_str(), pWrGrpIdMask);

			mifPostInstr.Append("\t\tc_wrGrpRspCnt[i] = r_wrGrpRspCnt[i];\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.NewLine();

			if (mif.m_mifWr.m_bMultiWr && is_hc1) {
				mifPostInstr.Append("\t\tif (r_m1_bWrRspRdy[i] || c_t%d_bWrReqRdy) {\n", mod.m_execStg);
				mifPostInstr.Append("\t\t\tif (c_t%d_bWrReqRdy)\n", mod.m_execStg);
				mifPostInstr.Append("\t\t\t\tc_wrGrpRspCnt[i] += c_t%d_reqQwCnt;\n", mod.m_execStg);
				mifPostInstr.Append("\t\t\tif (r_m1_bWrRspRdy[i])\n");
				mifPostInstr.Append("\t\t\t\tc_wrGrpRspCnt[i] -= 1;\n");
				mifPostInstr.Append("\t\t}\n");

			} else if (mif.m_mifWr.m_bMultiWr && is_hc2) {
				mifPostInstr.Append("\t\tif (r_m1_bWrRspRdy[i] || c_t%d_bWrReqRdy) {\n", mod.m_execStg);
				mifPostInstr.Append("\t\t\tif (c_t%d_bWrReqRdy)\n", mod.m_execStg);
				mifPostInstr.Append("\t\t\t\tc_wrGrpRspCnt[i] += (c_t%d_reqQwCnt == 8 && c_t%d_%sToMif_req.m_host) ? (ht_uint4)1 : c_t%d_reqQwCnt;\n",
					mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), mod.m_execStg);
				//c_t%d_reqQwCnt;\n", mod.m_execStg);
				mifPostInstr.Append("\t\t\tif (r_m1_bWrRspRdy[i])\n");
				mifPostInstr.Append("\t\t\t\tc_wrGrpRspCnt[i] -= 1;\n");
				mifPostInstr.Append("\t\t}\n");

			} else {
				mifPostInstr.Append("\t\tif (r_m1_bWrRspRdy[i] != c_t%d_bWrReqRdy)\n",
					mod.m_execStg, mod.m_modName.Upper().c_str());
				mifPostInstr.Append("\t\t\tc_wrGrpRspCnt[i] += r_m1_bWrRspRdy[i] ? (sc_uint<%s_WR_RSP_CNT_W>)-1 : (sc_uint<%s_WR_RSP_CNT_W>)+1;\n",
					mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());
			}

			mifPostInstr.Append("\t}\n");
			mifPostInstr.NewLine();

			mifPostInstr.Append("\tsc_uint<%s_WR_RSP_CNT_W> c_t%d_wrGrpRspCnt = c_wrGrpRspCnt[INT(%s)];\n",
				mod.m_modName.Upper().c_str(), mod.m_execStg-1, wrRspGrpId);
		} else {
			mifPostInstr.Append("\t// update m_wrGrpRspCnt rams\n");
			mifPostInstr.Append("\tsc_uint<%s_WR_GRP_ID_W> c_wrGrpRspCntAddr[%d];\n",
				mod.m_modName.Upper().c_str(),
				wrGrpBankCnt);
			mifPostInstr.Append("\tsc_uint<%s_WR_RSP_CNT_W> c_wrGrpRspCnt[%d];\n",
				mod.m_modName.Upper().c_str(),
				wrGrpBankCnt);
			mifPostInstr.Append("\tsc_uint<%s_WR_GRP_ID_W> c_m0_wrRspGrpId = i_mifTo%sP0_wrRspTid.read() & ((1 << %s_WR_GRP_ID_W)-1);\n",
				mod.m_modName.Upper().c_str(),
				mod.m_modName.Uc().c_str(),
				mod.m_modName.Upper().c_str());
			mifPostInstr.Append("\tbool c_m0_bWrRspRdy[%d];\n",
				wrGrpBankCnt);
			mifPostInstr.NewLine();
			mifPostInstr.Append("\tfor (unsigned i = 0; i < %du; i += 1) {\n", wrGrpBankCnt);
			mifPostInstr.Append("\t\tc_m0_bWrRspRdy[i] = i_mifTo%sP0_wrRspRdy.read() && (i_mifTo%sP0_wrRspTid.read() & %s) == i;\n",
				mod.m_modName.Uc().c_str(), mod.m_modName.Uc().c_str(), pWrGrpIdMask);
			mifPostInstr.Append("\t\tc_wrGrpRspCntAddr[i] = c_m0_bWrRspRdy[i] ? c_m0_wrRspGrpId : %s;\n", wrRspGrpId);
			mifPostInstr.Append("\t\tm_wrGrpRspCnt[i].read_addr(c_wrGrpRspCntAddr[i] >> %s);\n", pWrGrpShfAmt);
			mifPostInstr.Append("\t\tbool bBypass = c_wrGrpRspCntAddr[i] == r_wrGrpRspCntAddr[i] && (r_wrGrpRspCntAddr[i] & %s) == i;\n",
				pWrGrpIdMask);
			mifPostInstr.NewLine();

			if (mif.m_bMifRd && mif.m_bMifWr)
				mifPostInstr.Append("\t\tbool c_t%d_bWrReqRdy = c_t%d_%sToMif_reqRdy%s && !c_t%d_bReadMem && (c_t%d_%sToMif_req.m_tid & %s) == i;\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), notReqBusy, mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), pWrGrpIdMask);
			else
				mifPostInstr.Append("\t\tbool c_t%d_bWrReqRdy = c_t%d_%sToMif_reqRdy%s && (c_t%d_%sToMif_req.m_tid & %s) == i;\n",
				mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), notReqBusy, mod.m_execStg, mod.m_modName.Lc().c_str(), pWrGrpIdMask);

			mifPostInstr.Append("\t\tsc_uint<%s_WR_RSP_CNT_W> c_wrGrpRspCntOut = r_wrGrpRspCnt[i];\n",
				mod.m_modName.Upper().c_str());
			mifPostInstr.NewLine();

			if (mif.m_mifWr.m_bMultiWr && is_hc1) {
				mifPostInstr.Append("\t\tif ((r_m1_bWrRspRdy[i] || c_t%d_bWrReqRdy) && r_wrRspGrpInitCnt[%s_WR_GRP_ID_W] == 1) {\n",
					mod.m_execStg, mod.m_modName.Upper().c_str());

				mifPostInstr.Append("\t\t\tif (c_t%d_bWrReqRdy)\n", mod.m_execStg);
				mifPostInstr.Append("\t\t\t\tc_wrGrpRspCntOut += c_t%d_reqQwCnt;\n", mod.m_execStg);
				mifPostInstr.Append("\t\t\tif (r_m1_bWrRspRdy[i])\n");
				mifPostInstr.Append("\t\t\t\tc_wrGrpRspCntOut -= 1;\n");

			} else if (mif.m_mifWr.m_bMultiWr && is_hc2) {
				mifPostInstr.Append("\t\tif ((r_m1_bWrRspRdy[i] || c_t%d_bWrReqRdy) && r_wrRspGrpInitCnt[%s_WR_GRP_ID_W] == 1) {\n",
					mod.m_execStg, mod.m_modName.Upper().c_str());

				mifPostInstr.Append("\t\t\tif (c_t%d_bWrReqRdy)\n", mod.m_execStg);
				mifPostInstr.Append("\t\t\t\tc_wrGrpRspCntOut += (c_t%d_reqQwCnt == 8 && c_t%d_%sToMif_req.m_host) ? (ht_uint4)1 : c_t%d_reqQwCnt;\n",
					mod.m_execStg, mod.m_execStg, mod.m_modName.Lc().c_str(), mod.m_execStg);
				//c_t%d_reqQwCnt;\n", mod.m_execStg);
				mifPostInstr.Append("\t\t\tif (r_m1_bWrRspRdy[i])\n");
				mifPostInstr.Append("\t\t\t\tc_wrGrpRspCntOut -= 1;\n");

			} else {
				mifPostInstr.Append("\t\tif (r_m1_bWrRspRdy[i] != c_t%d_bWrReqRdy && r_wrRspGrpInitCnt[%s_WR_GRP_ID_W] == 1) {\n",
					mod.m_execStg, mod.m_modName.Upper().c_str());
				mifPostInstr.Append("\t\t\tc_wrGrpRspCntOut += r_m1_bWrRspRdy[i] ? (sc_uint<%s_WR_RSP_CNT_W>)-1 : (sc_uint<%s_WR_RSP_CNT_W>)+1;\n",
					mod.m_modName.Upper().c_str(), mod.m_modName.Upper().c_str());
			}

			mifPostInstr.Append("\t\t\tm_wrGrpRspCnt[i].write_addr(r_wrGrpRspCntAddr[i] >> %s);\n", pWrGrpShfAmt);
			mifPostInstr.Append("\t\t\tm_wrGrpRspCnt[i].write_mem(c_wrGrpRspCntOut);\n");
			mifPostInstr.Append("\t\t}\n");
			mifPostInstr.NewLine();
			mifPostInstr.Append("\t\tc_wrGrpRspCnt[i] = bBypass ? c_wrGrpRspCntOut : m_wrGrpRspCnt[i].read_mem();\n");
			mifPostInstr.Append("\t}\n");
			mifPostInstr.NewLine();

			mifPostInstr.Append("\tsc_uint<%s_WR_RSP_CNT_W> c_t%d_wrGrpRspCnt = c_wrGrpRspCnt[%s & %s];\n",
				mod.m_modName.Upper().c_str(), mod.m_execStg-1, wrRspGrpId, pWrGrpIdMask);
		}

		if (mif.m_mifWr.m_bMultiWr)
			mifPostInstr.Append("\tbool c_t%d_wrGrpRspCntMax = c_t%d_wrGrpRspCnt >= ((1ul << %s_WR_RSP_CNT_W)-8);\n",
				mod.m_execStg-1, mod.m_execStg-1, mod.m_modName.Upper().c_str());
		else
			mifPostInstr.Append("\tbool c_t%d_wrGrpRspCntMax = c_t%d_wrGrpRspCnt == ((1ul << %s_WR_RSP_CNT_W)-1);\n",
				mod.m_execStg-1, mod.m_execStg-1, mod.m_modName.Upper().c_str());

		mifPostInstr.Append("\tbool c_t%d_wrGrpRspCntZero = c_t%d_wrGrpRspCnt == 0;\n",
			mod.m_execStg-1, mod.m_execStg-1);
		mifPostInstr.NewLine();

		if (mif.m_mifWr.m_rspGrpIdW > 2) {
			mifPostInstr.Append("\tbool c_t%d_bWriteRspBusy = i_mifTo%sP0_wrRspRdy.read() && ((c_m0_wrRspGrpId & %s) == (%s & %s));\n",
				mod.m_execStg-1, mod.m_modName.Uc().c_str(), pWrGrpIdMask, wrRspGrpId, pWrGrpIdMask);
			mifPostInstr.NewLine();
		}

		if (mif.m_mifWr.m_bMultiWr) {

			mifPostInstr.Append("\tbool c_t%d_bWriteReq = c_t%d_%sToMif_reqRdy && c_t%d_%sToMif_req.m_type == MEM_REQ_WR;\n",
				mod.m_execStg+1, mod.m_execStg+1, mod.m_modName.Lc().c_str(), mod.m_execStg+1, mod.m_modName.Lc().c_str());

			mifPostInstr.NewLine();

			mifPostInstr.Append("\t// Read data for memory write\n");

			for (size_t i = 0; i < mif.m_mifWr.m_wrSrcList.size(); i += 1) {
				CMifWrSrc &mifWrSrc = mif.m_mifWr.m_wrSrcList[i];

				if (mifWrSrc.m_pIntGbl && (mifWrSrc.m_varAddr0W > 0 || mifWrSrc.m_varAddr1W > 0))
					mifPostInstr.Append("\tht_uint%d c_%sMifRdAddr = 0;\n",
					mifWrSrc.m_pIntGbl->m_addr0W.AsInt() + mifWrSrc.m_pIntGbl->m_addr1W.AsInt() + mifWrSrc.m_pIntGbl->m_addr2W.AsInt(),
					mifWrSrc.m_pIntGbl->m_gblName.c_str());
			}

			mifPostInstr.Append("\tif (c_t%d_bWriteReq) {\n", mod.m_execStg+1);
			mifPostInstr.Append("\t\tsc_uint<%s_MIF_SRC_IDX_W> c_t%d_mifSrcIdx = (c_t%d_%sToMif_req.m_tid >> (%s_MIF_SRC_IDX_SHF + 1 + %s))\n",
				mod.m_modName.Upper().c_str(), mod.m_execStg+1, mod.m_execStg+1,
				mod.m_modName.Lc().c_str(), mod.m_modName.Upper().c_str(), wrRspGrpIdW.c_str());
			mifPostInstr.Append("\t\t\t& %s_MIF_SRC_IDX_MSK;\n", mod.m_modName.Upper().c_str());
			mifPostInstr.NewLine();

			mifPostInstr.Append("\t\tswitch (c_t%d_mifSrcIdx) {\n", mod.m_execStg+1);

			bool bSharedBram = false;
			bool bGlobalVarP3 = false;
			bool bGlobalVarP4 = false;
			for (size_t i = 0; i < mif.m_mifWr.m_wrSrcList.size(); i += 1) {
				CMifWrSrc &mifWrSrc = mif.m_mifWr.m_wrSrcList[i];

				mifPostInstr.Append("\t\tcase %d:\n", (int)i+1);
				mifPostInstr.Append("\t\t\t{\n");

				char buf[64];

				if (mifWrSrc.m_varAddr0W > 0) {
					mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_VAR_ADDR0_W> c_t%d_varAddr0 = (c_t%d_%sToMif_req.m_tid\n",
						mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
						mod.m_execStg+1,
						mod.m_execStg+1, mod.m_modName.Lc().c_str());
					mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_VAR_ADDR0_SHF + 1 + %s)) & %s_MIF_SRC_%s_VAR_ADDR0_MSK;\n",
						mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
						wrRspGrpIdW.c_str(),
						mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
				}

				if (mifWrSrc.m_varAddr1W > 0) {
					mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_VAR_ADDR1_W> c_t%d_varAddr1 = (c_t%d_%sToMif_req.m_tid\n",
						mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
						mod.m_execStg+1,
						mod.m_execStg+1, mod.m_modName.Lc().c_str());
					mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_VAR_ADDR1_SHF + 1 + %s)) & %s_MIF_SRC_%s_VAR_ADDR1_MSK;\n",
						mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
						wrRspGrpIdW.c_str(),
						mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
				}

				if (mifWrSrc.m_varAddr2W > 0) {
					mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_VAR_ADDR2_W> c_t%d_varAddr2 = (c_t%d_%sToMif_req.m_tid\n",
						mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
						mod.m_execStg+1,
						mod.m_execStg+1, mod.m_modName.Lc().c_str());
					mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_VAR_ADDR2_SHF + 1 + %s)) & %s_MIF_SRC_%s_VAR_ADDR2_MSK;\n",
						mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
						wrRspGrpIdW.c_str(),
						mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
				}

				string varIdx;
				string fldIdx;
				if (mifWrSrc.m_pIntGbl == 0 || (mifWrSrc.m_pIntGbl->m_addr0W.AsInt() == 0 && mifWrSrc.m_pIntGbl->m_addr1W.AsInt() == 0)) {
					if (mifWrSrc.m_varDimen1 > 0) {
						sprintf(buf, "[INT(c_t%d_varIdx1)]", mod.m_execStg+1);
						varIdx = buf;
						mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_VAR_IDX1_W> c_t%d_varIdx1 = (c_t%d_%sToMif_req.m_tid\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							mod.m_execStg+1,
							mod.m_execStg+1, mod.m_modName.Lc().c_str());
						mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_VAR_IDX1_SHF + 1 + %s)) & %s_MIF_SRC_%s_VAR_IDX1_MSK;\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							wrRspGrpIdW.c_str(),
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
					}

					if (mifWrSrc.m_varDimen2 > 0) {
						sprintf(buf, "[INT(c_t%d_varIdx2)]", mod.m_execStg+1);
						varIdx += buf;
						mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_VAR_IDX2_W> c_t%d_varIdx2 = (c_t%d_%sToMif_req.m_tid\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							mod.m_execStg+1,
							mod.m_execStg+1, mod.m_modName.Lc().c_str());
						mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_VAR_IDX2_SHF + 1 + %s)) & %s_MIF_SRC_%s_VAR_IDX2_MSK;\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							wrRspGrpIdW.c_str(),
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
					}

					if (mifWrSrc.m_fldDimen1 > 0) {
						sprintf(buf, "[INT(c_t%d_fldIdx1)]", mod.m_execStg+1);
						fldIdx = buf;
						mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_FLD_IDX1_W> c_t%d_fldIdx1 = (c_t%d_%sToMif_req.m_tid\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							mod.m_execStg+1,
							mod.m_execStg+1, mod.m_modName.Lc().c_str());
						mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_FLD_IDX1_SHF + 1 + %s)) & %s_MIF_SRC_%s_FLD_IDX1_MSK;\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							wrRspGrpIdW.c_str(),
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
					}

					if (mifWrSrc.m_fldDimen2 > 0) {
						sprintf(buf, "[INT(c_t%d_fldIdx2)]", mod.m_execStg+1);
						fldIdx += buf;
						mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_FLD_IDX2_W> c_t%d_fldIdx2 = (c_t%d_%sToMif_req.m_tid\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							mod.m_execStg+1,
							mod.m_execStg+1, mod.m_modName.Lc().c_str());
						mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_FLD_IDX2_SHF + 1 + %s)) & %s_MIF_SRC_%s_FLD_IDX2_MSK;\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							wrRspGrpIdW.c_str(),
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
					}
				}
				mifPostInstr.NewLine();

				if (mifWrSrc.m_srcIdx == "varAddr1")
					mifPostInstr.Append("\t\t\t\tc_t%d_varAddr1 += r_t%d_reqQwIdx;\n", mod.m_execStg+1, mod.m_execStg+1);
				else if (mifWrSrc.m_srcIdx == "varAddr2")
					mifPostInstr.Append("\t\t\t\tc_t%d_varAddr2 += r_t%d_reqQwIdx;\n", mod.m_execStg+1, mod.m_execStg+1);

				else if (mifWrSrc.m_pIntGbl == 0 || (mifWrSrc.m_pIntGbl->m_addr0W.AsInt() == 0 && mifWrSrc.m_pIntGbl->m_addr1W.AsInt() == 0)) {
					if (mifWrSrc.m_srcIdx == "varIdx1")
						mifPostInstr.Append("\t\t\t\tc_t%d_varIdx1 += r_t%d_reqQwIdx;\n", mod.m_execStg+1, mod.m_execStg+1);
					else if (mifWrSrc.m_srcIdx == "varIdx2")
						mifPostInstr.Append("\t\t\t\tc_t%d_varIdx2 += r_t%d_reqQwIdx;\n", mod.m_execStg+1, mod.m_execStg+1);
					else if (mifWrSrc.m_srcIdx == "fldIdx1")
						mifPostInstr.Append("\t\t\t\tc_t%d_fldIdx1 += r_t%d_reqQwIdx;\n", mod.m_execStg+1, mod.m_execStg+1);
					else if (mifWrSrc.m_srcIdx == "fldIdx2")
						mifPostInstr.Append("\t\t\t\tc_t%d_fldIdx2 += r_t%d_reqQwIdx;\n", mod.m_execStg+1, mod.m_execStg+1);
				}
				mifPostInstr.NewLine();

				if (mifWrSrc.m_pSharedVar) {
					HtlAssert(mifWrSrc.m_bMultiWr);
					mifReqStgCnt = max(mifReqStgCnt, 1);
					char dotField[128];
					if (mifWrSrc.m_field.size() > 0)
						sprintf(dotField, ".%s", mifWrSrc.m_field.c_str());
					else
						dotField[0] = '\0';

					if (mifWrSrc.m_pSharedVar->m_queueW.AsInt() > 0) {

						mifPostInstr.Append("\t\t\t\tc_t%d_%sToMif_req.m_data = m_%s%s.front()%s%s;\n",
							mod.m_execStg+1, mod.m_modName.Lc().c_str(), mifWrSrc.m_var.c_str(), varIdx.c_str(), dotField, fldIdx.c_str());
						mifPostInstr.Append("\t\t\t\tm_%s%s.pop();\n",
							mifWrSrc.m_var.c_str(), varIdx.c_str());

					} else if (mifWrSrc.m_varAddr1W == 0) {
						mifPostInstr.Append("\t\t\t\tc_t%d_%sToMif_req.m_data = r_%s%s%s%s;\n",
							mod.m_execStg+1, mod.m_modName.Lc().c_str(), mifWrSrc.m_var.c_str(), varIdx.c_str(), dotField, fldIdx.c_str());

					} else if (mifWrSrc.m_pSharedVar->m_ramType == eDistRam) {
						if (mifWrSrc.m_varAddr2W == 0)
							mifPostInstr.Append("\t\t\t\tm_%s%s.read_addr(c_t%d_varAddr1);\n",
							mifWrSrc.m_var.c_str(), varIdx.c_str(), mod.m_execStg+1);
						else
							mifPostInstr.Append("\t\t\t\tm_%s%s.read_addr(c_t%d_varAddr1, c_t%d_varAddr2);\n",
							mifWrSrc.m_var.c_str(), varIdx.c_str(), mod.m_execStg+1, mod.m_execStg+1);

						mifPostInstr.Append("\t\t\t\tc_t%d_%sToMif_req.m_data = m_%s%s.read_mem()%s%s;\n",
							mod.m_execStg+1, mod.m_modName.Lc().c_str(), mifWrSrc.m_var.c_str(), varIdx.c_str(), dotField, fldIdx.c_str());
					} else {
						// block ram
						bSharedBram = true;

						HtlAssert(mifWrSrc.m_bMultiWr);
						mifReqStgCnt = max(mifReqStgCnt, 2);

						if (mifWrSrc.m_srcIdx != "varAddr1" && mifWrSrc.m_srcIdx != "varAddr2") {
							if (mifWrSrc.m_varAddr1W > 0)
								reqQwIdxStgCnt = max(reqQwIdxStgCnt, 2);
							else
								reqQwIdxStgCnt = max(reqQwIdxStgCnt, 1);
						}

						if (mifWrSrc.m_varAddr2W == 0)
							mifPostInstr.Append("\t\t\t\tm_%s%s.read_addr(c_t%d_varAddr1);\n",
							mifWrSrc.m_var.c_str(), varIdx.c_str(), mod.m_execStg+1);
						else
							mifPostInstr.Append("\t\t\t\tm_%s%s.read_addr(c_t%d_varAddr1, c_t%d_varAddr2);\n",
							mifWrSrc.m_var.c_str(), varIdx.c_str(), mod.m_execStg+1, mod.m_execStg+1);
					}
				} else if (mifWrSrc.m_pIntGbl) {
					HtlAssert(mifWrSrc.m_bMultiWr);
					mifReqStgCnt = max(mifReqStgCnt, 1);

					if (mifWrSrc.m_pIntGbl->m_addr0W.AsInt() > 0 || mifWrSrc.m_pIntGbl->m_addr1W.AsInt() > 0) {

						// determine when ram read data is valid (in 2 or 3 clocks)
						int fieldIdx = mifWrSrc.m_pField->m_gblFieldIdx;
						CField &field = mifWrSrc.m_pIntGbl->m_allFieldList[fieldIdx];

						if (field.m_readerList[0].m_pRamPort->m_bMifRead && field.m_ramType == eDistRam
							&& (field.m_readerList.size() == 1 || field.m_readerList[1].m_pRamPort == 0 || mod.m_clkRate == eClk2x))
						{
							// internal gbl var, 1x ram access
							bGlobalVarP3 = true;
							mifWrSrc.m_bGblVarP3 = true;
							mifReqStgCnt = max(mifReqStgCnt, 3);
						} else {
							bGlobalVarP4 = true;
							mifWrSrc.m_bGblVarP4 = true;
							mifReqStgCnt = max(mifReqStgCnt, 4);
						}

						if (mifWrSrc.m_srcIdx != "varAddr1" && mifWrSrc.m_srcIdx != "varAddr2") {
							if (mifWrSrc.m_varAddr0W > 0 || mifWrSrc.m_varAddr1W > 0)
								reqQwIdxStgCnt = max(reqQwIdxStgCnt, mod.m_clkRate == eClk2x ? 3 : 4);
						}

						if (mifWrSrc.m_varAddr0W > 0) {
							if (mifWrSrc.m_varAddr2W > 0)
								mifPostInstr.Append("\t\t\t\tc_%sMifRdAddr = (c_t%d_varAddr0, c_t%d_varAddr1, c_t%d_varAddr2);\n",
									mifWrSrc.m_pIntGbl->m_gblName.c_str(),
									mod.m_execStg+1, mod.m_execStg+1, mod.m_execStg+1);
							else if (mifWrSrc.m_varAddr1W > 0)
								mifPostInstr.Append("\t\t\t\tc_%sMifRdAddr = (c_t%d_varAddr0, c_t%d_varAddr1);\n",
									mifWrSrc.m_pIntGbl->m_gblName.c_str(),
									mod.m_execStg+1, mod.m_execStg+1);
							else
								mifPostInstr.Append("\t\t\t\tc_%sMifRdAddr = c_t%d_varAddr0;\n",
									mifWrSrc.m_pIntGbl->m_gblName.c_str(),
									mod.m_execStg+1);
						} else {
							if (mifWrSrc.m_varAddr2W > 0)
								mifPostInstr.Append("\t\t\t\tc_%sMifRdAddr = (c_t%d_varAddr1, c_t%d_varAddr2);\n",
									mifWrSrc.m_pIntGbl->m_gblName.c_str(),
									mod.m_execStg+1, mod.m_execStg+1);
							else if (mifWrSrc.m_varAddr1W > 0)
								mifPostInstr.Append("\t\t\t\tc_%sMifRdAddr = c_t%d_varAddr1;\n",
									mifWrSrc.m_pIntGbl->m_gblName.c_str(),
									mod.m_execStg+1);
						}
					} else {

						char dotField[128];
						if (mifWrSrc.m_field.size() > 0)
							sprintf(dotField, ".m_%s", mifWrSrc.m_field.c_str());
						else
							dotField[0] = '\0';

						if (mifWrSrc.m_pIntGbl->m_gblPortList[0].m_bSrcRead) {
							// if read on port[0] then must need 2x read port
							mifPostInstr.Append("\t\t\t\tc_t%d_%sToMif_req.m_data = r_%sMifRdData_2x%s.read()%s%s;\n",
								mod.m_execStg+1, mod.m_modName.Lc().c_str(),
								mifWrSrc.m_var.c_str(), varIdx.c_str(), dotField, fldIdx.c_str());
						} else {
							mifPostInstr.Append("\t\t\t\tc_t%d_%sToMif_req.m_data = r_%sMifRdData%s%s%s;\n",
								mod.m_execStg+1, mod.m_modName.Lc().c_str(),
								mifWrSrc.m_var.c_str(), varIdx.c_str(), dotField, fldIdx.c_str());
						}
					}
				} else
					HtlAssert(0);

				mifPostInstr.Append("\t\t\t}\n");
				mifPostInstr.Append("\t\t\tbreak;\n");
			}

			mifPostInstr.Append("\t\tdefault:\n");
			mifPostInstr.Append("\t\t\tbreak;\n");
			mifPostInstr.Append("\t\t}\n");
			mifPostInstr.Append("\t}\n");
			mifPostInstr.NewLine();

			if (bSharedBram || bGlobalVarP3 || bGlobalVarP4) {
				mifReg.Append("\tr_t%d_bWriteReq = !c_reset1x && c_t%d_bWriteReq;\n",
					mod.m_execStg+2, mod.m_execStg+1);
				m_mifDecl.Append("\tbool r_t%d_bWriteReq;\n", mod.m_execStg+2);
				mifPostInstr.Append("\tbool c_t%d_bWriteReq = r_t%d_bWriteReq;\n",
					mod.m_execStg+2, mod.m_execStg+2);
			}

			if (bSharedBram) {
				mifPostInstr.Append("\tif (c_t%d_bWriteReq) {\n", mod.m_execStg+2);
				mifPostInstr.Append("\t\t// shared block ram access\n");
				mifPostInstr.Append("\t\tsc_uint<%s_MIF_SRC_IDX_W> c_t%d_mifSrcIdx = (c_t%d_%sToMif_req.m_tid >> (%s_MIF_SRC_IDX_SHF + 1 + %s))\n",
					mod.m_modName.Upper().c_str(), mod.m_execStg+2, mod.m_execStg+2,
					mod.m_modName.Lc().c_str(), mod.m_modName.Upper().c_str(), wrRspGrpIdW.c_str());
				mifPostInstr.Append("\t\t\t& %s_MIF_SRC_IDX_MSK;\n", mod.m_modName.Upper().c_str());
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tswitch (c_t%d_mifSrcIdx) {\n", mod.m_execStg+2);

				for (size_t i = 0; i < mif.m_mifWr.m_wrSrcList.size(); i += 1) {
					CMifWrSrc &mifWrSrc = mif.m_mifWr.m_wrSrcList[i];

					if (mifWrSrc.m_pSharedVar == 0 || mifWrSrc.m_pSharedVar->m_ramType != eBlockRam)
						continue;

					mifPostInstr.Append("\t\tcase %d:\n", (int)i+1);
					mifPostInstr.Append("\t\t\t{\n");

					char buf[64];

					string varIdx;
					if (mifWrSrc.m_varDimen1 > 0) {
						sprintf(buf, "[INT(c_t%d_varIdx1)]", mod.m_execStg+2);
						varIdx = buf;
						mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_VAR_IDX1_W> c_t%d_varIdx1 = (c_t%d_%sToMif_req.m_tid\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							mod.m_execStg+2,
							mod.m_execStg+2, mod.m_modName.Lc().c_str());
						mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_VAR_IDX1_SHF + 1 + %s)) & %s_MIF_SRC_%s_VAR_IDX1_MSK;\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							wrRspGrpIdW.c_str(),
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
					}

					if (mifWrSrc.m_varDimen2 > 0) {
						sprintf(buf, "[INT(c_t%d_varIdx2)]", mod.m_execStg+2);
						varIdx += buf;
						mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_VAR_IDX2_W> c_t%d_varIdx2 = (c_t%d_%sToMif_req.m_tid\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							mod.m_execStg+2,
							mod.m_execStg+2, mod.m_modName.Lc().c_str());
						mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_VAR_IDX2_SHF + 1 + %s)) & %s_MIF_SRC_%s_VAR_IDX2_MSK;\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							wrRspGrpIdW.c_str(),
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
					}

					string fldIdx;
					if (mifWrSrc.m_fldDimen1 > 0) {
						sprintf(buf, "[INT(c_t%d_fldIdx1)]", mod.m_execStg+2);
						fldIdx = buf;
						mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_FLD_IDX1_W> c_t%d_fldIdx1 = (c_t%d_%sToMif_req.m_tid\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							mod.m_execStg+2,
							mod.m_execStg+2, mod.m_modName.Lc().c_str());
						mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_FLD_IDX1_SHF + 1 + %s)) & %s_MIF_SRC_%s_FLD_IDX1_MSK;\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							wrRspGrpIdW.c_str(),
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
					}

					if (mifWrSrc.m_fldDimen2 > 0) {
						sprintf(buf, "[INT(c_t%d_fldIdx2)]", mod.m_execStg+2);
						fldIdx += buf;
						mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_FLD_IDX2_W> c_t%d_fldIdx2 = (c_t%d_%sToMif_req.m_tid\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							mod.m_execStg+2,
							mod.m_execStg+2, mod.m_modName.Lc().c_str());
						mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_FLD_IDX2_SHF + 1 + %s)) & %s_MIF_SRC_%s_FLD_IDX2_MSK;\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							wrRspGrpIdW.c_str(),
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
					}
					mifPostInstr.NewLine();

					if (mifWrSrc.m_srcIdx == "varIdx1")
						mifPostInstr.Append("\t\t\t\tc_t%d_varIdx1 += r_t%d_reqQwIdx;\n", mod.m_execStg+2, mod.m_execStg+2);
					else if (mifWrSrc.m_srcIdx == "varIdx2")
						mifPostInstr.Append("\t\t\t\tc_t%d_varIdx2 += r_t%d_reqQwIdx;\n", mod.m_execStg+2, mod.m_execStg+2);
					else if (mifWrSrc.m_srcIdx == "fldIdx1")
						mifPostInstr.Append("\t\t\t\tc_t%d_fldIdx1 += r_t%d_reqQwIdx;\n", mod.m_execStg+2, mod.m_execStg+2);
					else if (mifWrSrc.m_srcIdx == "fldIdx2")
						mifPostInstr.Append("\t\t\t\tc_t%d_fldIdx2 += r_t%d_reqQwIdx;\n", mod.m_execStg+2, mod.m_execStg+2);
					mifPostInstr.NewLine();

					if (mifWrSrc.m_pSharedVar) {
						char dotField[128];
						if (mifWrSrc.m_field.size() > 0)
							sprintf(dotField, ".m_%s", mifWrSrc.m_field.c_str());
						else
							dotField[0] = '\0';

						mifPostInstr.Append("\t\t\t\tc_t%d_%sToMif_req.m_data = m_%s%s.read_mem()%s%s;\n",
							mod.m_execStg+2, mod.m_modName.Lc().c_str(), mifWrSrc.m_var.c_str(), varIdx.c_str(), dotField, fldIdx.c_str());
					} else
						HtlAssert(0);

					mifPostInstr.Append("\t\t\t}\n");
					mifPostInstr.Append("\t\t\tbreak;\n");
				}

				mifPostInstr.Append("\t\tdefault:\n");
				mifPostInstr.Append("\t\t\tbreak;\n");
				mifPostInstr.Append("\t\t}\n");
				mifPostInstr.Append("\t}\n");
				mifPostInstr.NewLine();
			}

			if (bGlobalVarP3 || bGlobalVarP4) {
				mifReg.Append("\tr_t%d_bWriteReq = !c_reset1x && c_t%d_bWriteReq;\n",
					mod.m_execStg+3, mod.m_execStg+2);
				m_mifDecl.Append("\tbool r_t%d_bWriteReq;\n", mod.m_execStg+3);
				mifPostInstr.Append("\tbool c_t%d_bWriteReq = r_t%d_bWriteReq;\n",
					mod.m_execStg+3, mod.m_execStg+3);
			}

			if (bGlobalVarP3) {
				int mifWrStg = mod.m_execStg+3;
				mifPostInstr.Append("\tif (c_t%d_bWriteReq) {\n", mifWrStg);
				mifPostInstr.Append("\t\t// global variable access\n");
				mifPostInstr.Append("\t\tsc_uint<%s_MIF_SRC_IDX_W> c_t%d_mifSrcIdx = (c_t%d_%sToMif_req.m_tid >> (%s_MIF_SRC_IDX_SHF + 1 + %s))\n",
					mod.m_modName.Upper().c_str(), mifWrStg, mifWrStg,
					mod.m_modName.Lc().c_str(), mod.m_modName.Upper().c_str(), wrRspGrpIdW.c_str());
				mifPostInstr.Append("\t\t\t& %s_MIF_SRC_IDX_MSK;\n", mod.m_modName.Upper().c_str());
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tswitch (c_t%d_mifSrcIdx) {\n", mifWrStg);

				for (size_t i = 0; i < mif.m_mifWr.m_wrSrcList.size(); i += 1) {
					CMifWrSrc &mifWrSrc = mif.m_mifWr.m_wrSrcList[i];

					if (!mifWrSrc.m_bGblVarP3)
						continue;

					mifPostInstr.Append("\t\tcase %d:\n", (int)i+1);
					mifPostInstr.Append("\t\t\t{\n");

					char buf[64];

					string varIdx;
					if (mifWrSrc.m_varDimen1 > 0) {
						sprintf(buf, "[INT(c_t%d_varIdx1)]", mifWrStg);
						varIdx = buf;
						mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_VAR_IDX1_W> c_t%d_varIdx1 = (c_t%d_%sToMif_req.m_tid\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							mifWrStg,
							mifWrStg, mod.m_modName.Lc().c_str());
						mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_VAR_IDX1_SHF + 1 + %s)) & %s_MIF_SRC_%s_VAR_IDX1_MSK;\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							wrRspGrpIdW.c_str(),
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
					}

					if (mifWrSrc.m_varDimen2 > 0) {
						sprintf(buf, "[INT(c_t%d_varIdx2)]", mifWrStg);
						varIdx += buf;
						mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_VAR_IDX2_W> c_t%d_varIdx2 = (c_t%d_%sToMif_req.m_tid\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							mifWrStg,
							mifWrStg, mod.m_modName.Lc().c_str());
						mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_VAR_IDX2_SHF + 1 + %s)) & %s_MIF_SRC_%s_VAR_IDX2_MSK;\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							wrRspGrpIdW.c_str(),
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
					}

					string fldIdx;
					if (mifWrSrc.m_fldDimen1 > 0) {
						sprintf(buf, "[INT(c_t%d_fldIdx1)]", mifWrStg);
						fldIdx = buf;
						mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_FLD_IDX1_W> c_t%d_fldIdx1 = (c_t%d_%sToMif_req.m_tid\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							mifWrStg,
							mifWrStg, mod.m_modName.Lc().c_str());
						mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_FLD_IDX1_SHF + 1 + %s)) & %s_MIF_SRC_%s_FLD_IDX1_MSK;\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							wrRspGrpIdW.c_str(),
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
					}

					if (mifWrSrc.m_fldDimen2 > 0) {
						sprintf(buf, "[INT(c_t%d_fldIdx2)]", mifWrStg);
						fldIdx += buf;
						mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_FLD_IDX2_W> c_t%d_fldIdx2 = (c_t%d_%sToMif_req.m_tid\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							mifWrStg,
							mifWrStg, mod.m_modName.Lc().c_str());
						mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_FLD_IDX2_SHF + 1 + %s)) & %s_MIF_SRC_%s_FLD_IDX2_MSK;\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							wrRspGrpIdW.c_str(),
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
					}
					mifPostInstr.NewLine();

					if (mifWrSrc.m_srcIdx == "varIdx1")
						mifPostInstr.Append("\t\t\t\tc_t%d_varIdx1 += r_t%d_reqQwIdx;\n", mifWrStg, mifWrStg);
					else if (mifWrSrc.m_srcIdx == "varIdx2")
						mifPostInstr.Append("\t\t\t\tc_t%d_varIdx2 += r_t%d_reqQwIdx;\n", mifWrStg, mifWrStg);
					else if (mifWrSrc.m_srcIdx == "fldIdx1")
						mifPostInstr.Append("\t\t\t\tc_t%d_fldIdx1 += r_t%d_reqQwIdx;\n", mifWrStg, mifWrStg);
					else if (mifWrSrc.m_srcIdx == "fldIdx2")
						mifPostInstr.Append("\t\t\t\tc_t%d_fldIdx2 += r_t%d_reqQwIdx;\n", mifWrStg, mifWrStg);
					mifPostInstr.NewLine();

					char dotField[128];
					if (mifWrSrc.m_field.size() > 0)
						sprintf(dotField, ".m_%s", mifWrSrc.m_field.c_str());
					else
						dotField[0] = '\0';

					if (mifWrSrc.m_pIntGbl->m_gblPortList[0].m_bSrcRead) {
						// if read on port[0] then must need 2x read port
						mifPostInstr.Append("\t\t\t\tc_t%d_%sToMif_req.m_data = r_%sMifRdData_2x%s.read()%s%s;\n",
							mifWrStg, mod.m_modName.Lc().c_str(),
							mifWrSrc.m_var.c_str(), varIdx.c_str(), dotField, fldIdx.c_str());
					} else {
						mifPostInstr.Append("\t\t\t\tc_t%d_%sToMif_req.m_data = r_%sMifRdData%s%s%s;\n",
							mifWrStg, mod.m_modName.Lc().c_str(),
							mifWrSrc.m_var.c_str(), varIdx.c_str(), dotField, fldIdx.c_str());
					}

					mifPostInstr.Append("\t\t\t}\n");
					mifPostInstr.Append("\t\t\tbreak;\n");
				}

				mifPostInstr.Append("\t\tdefault:\n");
				mifPostInstr.Append("\t\t\tbreak;\n");
				mifPostInstr.Append("\t\t}\n");
				mifPostInstr.Append("\t}\n");
				mifPostInstr.NewLine();
			}

			if (bGlobalVarP4) {
				mifReg.Append("\tr_t%d_bWriteReq = !c_reset1x && c_t%d_bWriteReq;\n",
					mod.m_execStg+4, mod.m_execStg+3);
				m_mifDecl.Append("\tbool r_t%d_bWriteReq;\n", mod.m_execStg+4);
				mifPostInstr.Append("\tbool c_t%d_bWriteReq = r_t%d_bWriteReq;\n",
					mod.m_execStg+4, mod.m_execStg+4);
			}

			if (bGlobalVarP4) {
				int mifWrStg = mod.m_execStg+4;
				mifPostInstr.Append("\tif (c_t%d_bWriteReq) {\n", mifWrStg);
				mifPostInstr.Append("\t\t// global variable access\n");
				mifPostInstr.Append("\t\tsc_uint<%s_MIF_SRC_IDX_W> c_t%d_mifSrcIdx = (c_t%d_%sToMif_req.m_tid >> (%s_MIF_SRC_IDX_SHF + 1 + %s))\n",
					mod.m_modName.Upper().c_str(), mifWrStg, mifWrStg,
					mod.m_modName.Lc().c_str(), mod.m_modName.Upper().c_str(), wrRspGrpIdW.c_str());
				mifPostInstr.Append("\t\t\t& %s_MIF_SRC_IDX_MSK;\n", mod.m_modName.Upper().c_str());
				mifPostInstr.NewLine();

				mifPostInstr.Append("\t\tswitch (c_t%d_mifSrcIdx) {\n", mifWrStg);

				for (size_t i = 0; i < mif.m_mifWr.m_wrSrcList.size(); i += 1) {
					CMifWrSrc &mifWrSrc = mif.m_mifWr.m_wrSrcList[i];

					if (!mifWrSrc.m_bGblVarP4)
						continue;

					mifPostInstr.Append("\t\tcase %d:\n", (int)i+1);
					mifPostInstr.Append("\t\t\t{\n");

					char buf[64];

					string varIdx;
					if (mifWrSrc.m_varDimen1 > 0) {
						sprintf(buf, "[INT(c_t%d_varIdx1)]", mifWrStg);
						varIdx = buf;
						mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_VAR_IDX1_W> c_t%d_varIdx1 = (c_t%d_%sToMif_req.m_tid\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							mifWrStg,
							mifWrStg, mod.m_modName.Lc().c_str());
						mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_VAR_IDX1_SHF + 1 + %s)) & %s_MIF_SRC_%s_VAR_IDX1_MSK;\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(), 
							wrRspGrpIdW.c_str(),
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
					}

					if (mifWrSrc.m_varDimen2 > 0) {
						sprintf(buf, "[INT(c_t%d_varIdx2)]", mifWrStg);
						varIdx += buf;
						mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_VAR_IDX2_W> c_t%d_varIdx2 = (c_t%d_%sToMif_req.m_tid\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							mifWrStg,
							mifWrStg, mod.m_modName.Lc().c_str());
						mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_VAR_IDX2_SHF + 1 + %s)) & %s_MIF_SRC_%s_VAR_IDX2_MSK;\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(), 
							wrRspGrpIdW.c_str(),
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
					}

					string fldIdx;
					if (mifWrSrc.m_fldDimen1 > 0) {
						sprintf(buf, "[INT(c_t%d_fldIdx1)]", mifWrStg);
						fldIdx = buf;
						mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_FLD_IDX1_W> c_t%d_fldIdx1 = (c_t%d_%sToMif_req.m_tid\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							mifWrStg,
							mifWrStg, mod.m_modName.Lc().c_str());
						mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_FLD_IDX1_SHF + 1 + %s)) & %s_MIF_SRC_%s_FLD_IDX1_MSK;\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							wrRspGrpIdW.c_str(),
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
					}

					if (mifWrSrc.m_fldDimen2 > 0) {
						sprintf(buf, "[INT(c_t%d_fldIdx2)]", mifWrStg);
						fldIdx += buf;
						mifPostInstr.Append("\t\t\t\tsc_uint<%s_MIF_SRC_%s_FLD_IDX2_W> c_t%d_fldIdx2 = (c_t%d_%sToMif_req.m_tid\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							mifWrStg,
							mifWrStg, mod.m_modName.Lc().c_str());
						mifPostInstr.Append("\t\t\t\t\t>> (%s_MIF_SRC_%s_FLD_IDX2_SHF + 1 + %s)) & %s_MIF_SRC_%s_FLD_IDX2_MSK;\n",
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str(),
							wrRspGrpIdW.c_str(),
							mod.m_modName.Upper().c_str(), mifWrSrc.m_name.Upper().c_str());
					}
					mifPostInstr.NewLine();

					if (mifWrSrc.m_srcIdx == "varIdx1")
						mifPostInstr.Append("\t\t\t\tc_t%d_varIdx1 += r_t%d_reqQwIdx;\n", mifWrStg, mifWrStg);
					else if (mifWrSrc.m_srcIdx == "varIdx2")
						mifPostInstr.Append("\t\t\t\tc_t%d_varIdx2 += r_t%d_reqQwIdx;\n", mifWrStg, mifWrStg);
					else if (mifWrSrc.m_srcIdx == "fldIdx1")
						mifPostInstr.Append("\t\t\t\tc_t%d_fldIdx1 += r_t%d_reqQwIdx;\n", mifWrStg, mifWrStg);
					else if (mifWrSrc.m_srcIdx == "fldIdx2")
						mifPostInstr.Append("\t\t\t\tc_t%d_fldIdx2 += r_t%d_reqQwIdx;\n", mifWrStg, mifWrStg);
					mifPostInstr.NewLine();

					char dotField[128];
					if (mifWrSrc.m_field.size() > 0)
						sprintf(dotField, ".m_%s", mifWrSrc.m_field.c_str());
					else
						dotField[0] = '\0';

					if (mifWrSrc.m_pIntGbl->m_gblPortList[0].m_bSrcRead) {
						// if read on port[0] then must need 2x read port
						mifPostInstr.Append("\t\t\t\tc_t%d_%sToMif_req.m_data = r_%sMifRdData_2x%s.read()%s%s;\n",
							mifWrStg, mod.m_modName.Lc().c_str(),
							mifWrSrc.m_var.c_str(), varIdx.c_str(), dotField, fldIdx.c_str());
					} else {
						mifPostInstr.Append("\t\t\t\tc_t%d_%sToMif_req.m_data = r_%sMifRdData%s%s%s;\n",
							mifWrStg, mod.m_modName.Lc().c_str(),
							mifWrSrc.m_var.c_str(), varIdx.c_str(), dotField, fldIdx.c_str());
					}

					mifPostInstr.Append("\t\t\t}\n");
					mifPostInstr.Append("\t\t\tbreak;\n");
				}

				mifPostInstr.Append("\t\tdefault:\n");
				mifPostInstr.Append("\t\t\tbreak;\n");
				mifPostInstr.Append("\t\t}\n");
				mifPostInstr.Append("\t}\n");
				mifPostInstr.NewLine();
			}
		}
	}

	if (bNeed_reqQwIdx) {
		mifPostInstr.Append("\tsc_uint<MIF_TID_QWCNT_W> c_t%d_reqQwIdx = r_t%d_reqQwIdx;\n",
			mod.m_execStg, mod.m_execStg+1);
		mifPostInstr.Append("\tif (c_t%d_%sToMif_reqRdy && ", mod.m_execStg+1, mod.m_modName.Lc().c_str());
		if (is_wx)
			mifPostInstr.Append("c_t%d_%sToMif_req.m_type == MEM_REQ_WR && ", mod.m_execStg+1, mod.m_modName.Lc().c_str());
		if (bNeed_reqQwSplit)
			mifPostInstr.Append("c_t%d_reqQwSplit && ", mod.m_execStg+1);
		mifPostInstr.Append("r_t%d_reqQwIdx < c_t%d_reqQwCnt-1)\n", mod.m_execStg+1, mod.m_execStg+1);
		mifPostInstr.Append("\t\tc_t%d_reqQwIdx += 1;\n", mod.m_execStg);
		mifPostInstr.Append("\telse\n");
		mifPostInstr.Append("\t\tc_t%d_reqQwIdx = 0;\n", mod.m_execStg);
		mifPostInstr.NewLine();

		for (int i = 1; i < reqQwIdxStgCnt; i += 1)
			mifPostInstr.Append("\tsc_uint<MIF_TID_QWCNT_W> c_t%d_reqQwIdx = r_t%d_reqQwIdx;\n",
			mod.m_execStg+i, mod.m_execStg+i);
		mifPostInstr.NewLine();
	}

	if (mif.m_mifWr.m_bReqPause) {
		mifPostInstr.Append("\tbool c_bWrpRsm = c_t%d_bWrpRsm && c_t%d_%sToMif_reqRdy && ",
			mod.m_execStg+1, mod.m_execStg+1, mod.m_modName.Lc().c_str());
		mifPostInstr.Append("c_t%d_%sToMif_req.m_type == MEM_REQ_WR && ", mod.m_execStg+1, mod.m_modName.Lc().c_str());
		mifPostInstr.Append("r_t%d_reqQwIdx == c_t%d_reqQwCnt-1;\n", mod.m_execStg+1, mod.m_execStg+1);

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			mifPostInstr.Append("\tsc_uint<%s_HTID_W> c_wrpRsmCnt = r_wrpRsmCnt;\n", mod.m_modName.Upper().c_str());
			mifPostInstr.Append("\tc_wrpRsmCnt += c_bWrpRsm ? 1u : 0u;\n");
			mifPostInstr.NewLine();

			mifPostInstr.Append("\tif ((c_bWrpRsm || r_wrpRsmCnt > 0) && !r_t%d_htCmdRdy) {\n",
				mod.m_execStg+1);
			mifPostInstr.Append("\t\tm_htCmdQue.push(m_wrpRsmQue.front());\n");
			mifPostInstr.Append("\t\tm_wrpRsmQue.pop();\n");
			mifPostInstr.Append("\t\tc_wrpRsmCnt -= 1u;\n");
			mifPostInstr.Append("\t}\n");
			mifPostInstr.NewLine();
		} else {
			mifPostInstr.Append("\tif (c_bWrpRsm)\n");
			mifPostInstr.Append("\t\tc_htCmdValid = true;\n");
			mifPostInstr.NewLine();
		}

		if (mod.m_threads.m_htIdW.AsInt() > 0) {
			m_mifRamDecl.Append("\tht_dist_que<CHtCmd, %s_HTID_W> m_wrpRsmQue;\n",
				mod.m_modName.Upper().c_str());
			mifRamClock.Append("\tm_wrpRsmQue.clock(r_reset1x);\n");
		}
	}

	if (mifRsmCnt > 1 && mod.m_threads.m_htIdW.AsInt() > 0) {
		mifPostInstr.Append("\t// Resume memory response thread\n");

		if (mif.m_bMifRd && mif.m_mifRd.m_bPause) {
			if (mod.m_threads.m_htIdW.AsInt() == 0) {
				mifPostInstr.Append("\tif (r_bRdGrpRsmRdy) {\n");
				mifPostInstr.Append("\t\tc_t0_rsmHtRdy = true;\n");
				mifPostInstr.Append("\t\tc_t0_rsmHtInstr = r_rdMemRspHtInstr;\n");
				mifPostInstr.Append("\t\tc_bRdGrpRsmRdy = false;\n");
			} else {
				m_mifRamDecl.Append("\tht_dist_que<CHtRsm, %s_HTID_W> m_mif_rdRsmQue;\n",
					mod.m_modName.Upper().c_str());
				mifRamClock.Append("\tm_mif_rdRsmQue.clock(r_reset1x);\n");

				mifPostInstr.Append("\tif (!c_t0_rsmHtRdy && !m_mif_rdRsmQue.empty()) {\n");
				mifPostInstr.Append("\t\tc_t0_rsmHtRdy = true;\n");
				mifPostInstr.Append("\t\tc_t0_rsmHtId = m_mif_rdRsmQue.front().m_htId;\n");
				mifPostInstr.Append("\t\tc_t0_rsmHtInstr = m_mif_rdRsmQue.front().m_htInstr;\n");
				mifPostInstr.Append("\t\tm_mif_rdRsmQue.pop();\n");
			}
			mifPostInstr.Append("\t}\n");
		}

		if (mif.m_bMifWr && mif.m_mifWr.m_bPause) {
			if (mod.m_threads.m_htIdW.AsInt() == 0) {
				mifPostInstr.Append("\tif (r_bWrGrpRsmRdy) {\n");
				mifPostInstr.Append("\t\tc_t0_rsmHtRdy = true;\n");
				mifPostInstr.Append("\t\tc_t0_rsmHtInstr = r_wrMemRspHtInstr;\n");
				mifPostInstr.Append("\t\tc_bWrGrpRsmRdy = false;\n");
			} else {
				m_mifRamDecl.Append("\tht_dist_que<CHtRsm, %s_HTID_W> m_mif_wrRsmQue;\n",
					mod.m_modName.Upper().c_str());
				mifRamClock.Append("\tm_mif_wrRsmQue.clock(r_reset1x);\n");

				mifPostInstr.Append("\tif (!c_t0_rsmHtRdy && !m_mif_wrRsmQue.empty()) {\n");
				mifPostInstr.Append("\t\tc_t0_rsmHtRdy = true;\n");
				mifPostInstr.Append("\t\tc_t0_rsmHtId = m_mif_wrRsmQue.front().m_htId;\n");
				mifPostInstr.Append("\t\tc_t0_rsmHtInstr = m_mif_wrRsmQue.front().m_htInstr;\n");
				mifPostInstr.Append("\t\tm_mif_wrRsmQue.pop();\n");
			}
			mifPostInstr.Append("\t}\n");
		}
	}

	mifPostInstr.NewLine();
	mifPostInstr.NewLine();

	if (mif.m_mifRd.m_bMultiRd || mif.m_mifWr.m_bMultiWr) {
		if (bNeed_reqBusy) {
			m_mifDecl.Append("\tbool r_t%d_reqBusy;\n", mod.m_execStg);
			mifPostInstr.Append("\tbool c_t%d_reqBusy = c_t%d_%sToMif_reqRdy && c_t%d_reqQwIdx > 0;\n\n",
				mod.m_execStg-1, mod.m_execStg, mod.m_modName.Lc().c_str(), mod.m_execStg);
			mifReg.Append("\tr_t%d_reqBusy = c_t%d_reqBusy;\n",
				mod.m_execStg, mod.m_execStg-1);
		}
	}

	if (mif.m_bMifRd && mif.m_mifRd.m_rspGrpIdW > 2) {
		mifPostInstr.Append("\tsc_uint<%s_RD_GRP_ID_W+1> c_rdRspGrpInitCnt = r_rdRspGrpInitCnt;\n",
			mod.m_modName.Upper().c_str());
		mifPostInstr.Append("\tif (r_rdRspGrpInitCnt < (1 << %s_RD_GRP_ID_W)) {\n",
			mod.m_modName.Upper().c_str());
		mifPostInstr.Append("\t\tm_rdGrpRspCnt[r_rdRspGrpInitCnt & %s].write_addr((sc_uint<%s_RD_GRP_ID_W-%s>)(r_rdRspGrpInitCnt >> %s));\n",
			pRdGrpIdMask, mod.m_modName.Upper().c_str(), pRdGrpShfAmt, pRdGrpShfAmt);
		mifPostInstr.Append("\t\tm_rdGrpRspCnt[r_rdRspGrpInitCnt & %s].write_mem(0);\n", pRdGrpIdMask);
		mifPostInstr.NewLine();
		if (mif.m_mifRd.m_bPause) {
			mifPostInstr.Append("\t\tm_rdGrpRsmWait[r_rdRspGrpInitCnt & %s].write_addr((sc_uint<%s_RD_GRP_ID_W-%s>)(r_rdRspGrpInitCnt >> %s));\n",
				pRdGrpIdMask, mod.m_modName.Upper().c_str(), pRdGrpShfAmt, pRdGrpShfAmt);
			mifPostInstr.Append("\t\tm_rdGrpRsmWait[r_rdRspGrpInitCnt & %s].write_mem(false);\n", pRdGrpIdMask);
			mifPostInstr.NewLine();
		}
		if (mif.m_mifRd.m_bPoll && mif.m_mifRd.m_rspGrpId.size() > 0) {
			mifPostInstr.Append("\t\tm_rdGrpRspCntZero[r_rdRspGrpInitCnt & %s].write_addr((sc_uint<%s_RD_GRP_ID_W-%s>)(r_rdRspGrpInitCnt >> %s));\n",
				pRdGrpIdMask, mod.m_modName.Upper().c_str(), pRdGrpShfAmt, pRdGrpShfAmt);
			mifPostInstr.Append("\t\tm_rdGrpRspCntZero[r_rdRspGrpInitCnt & %s].write_mem(true);\n", pRdGrpIdMask);
			mifPostInstr.NewLine();
		}
		mifPostInstr.Append("\t\tc_rdRspGrpInitCnt = r_rdRspGrpInitCnt + 1u;\n");
		mifPostInstr.Append("\t}\n");
		mifPostInstr.Append("\tif (r_reset1x) c_rdRspGrpInitCnt = 0;\n");
	}

	if (mif.m_bMifWr && mif.m_mifWr.m_rspGrpIdW > 2) {
		mifPostInstr.Append("\tsc_uint<%s_WR_GRP_ID_W+1> c_wrRspGrpInitCnt = r_wrRspGrpInitCnt;\n",
			mod.m_modName.Upper().c_str());
		mifPostInstr.Append("\tif (r_wrRspGrpInitCnt < (1 << %s_WR_GRP_ID_W)) {\n",
			mod.m_modName.Upper().c_str());
		mifPostInstr.Append("\t\tm_wrGrpRspCnt[r_wrRspGrpInitCnt & %s].write_addr((sc_uint<%s_WR_GRP_ID_W-%s>)(r_wrRspGrpInitCnt >> %s));\n",
			pWrGrpIdMask, mod.m_modName.Upper().c_str(), pWrGrpShfAmt, pWrGrpShfAmt);
		mifPostInstr.Append("\t\tm_wrGrpRspCnt[r_wrRspGrpInitCnt & %s].write_mem(0);\n", pWrGrpIdMask);
		mifPostInstr.NewLine();
		if (mif.m_mifWr.m_bPause) {
			mifPostInstr.Append("\t\tm_wrGrpRsmWait[r_wrRspGrpInitCnt & %s].write_addr((sc_uint<%s_WR_GRP_ID_W-%s>)(r_wrRspGrpInitCnt >> %s));\n",
				pWrGrpIdMask, mod.m_modName.Upper().c_str(), pWrGrpShfAmt, pWrGrpShfAmt);
			mifPostInstr.Append("\t\tm_wrGrpRsmWait[r_wrRspGrpInitCnt & %s].write_mem(false);\n", pWrGrpIdMask);
			mifPostInstr.NewLine();
		}
		mifPostInstr.Append("\t\tc_wrRspGrpInitCnt = r_wrRspGrpInitCnt + 1u;\n");
		mifPostInstr.Append("\t}\n");
		mifPostInstr.Append("\tif (r_reset1x) c_wrRspGrpInitCnt = 0;\n");
	}

	if (bNeed_reqQwIdx)
	{
		m_mifDecl.Append("\tht_uint4 r_t%d_reqQwCnt;\n", mod.m_execStg+1);
		m_mifDecl.Append("\tht_uint4 r_t%d_reqQwCnt;\n", mod.m_execStg+2);

		mifReg.Append("\tr_t%d_reqQwCnt = r_reset1x ? (ht_uint4)0 : c_t%d_reqQwCnt;\n",
			mod.m_execStg+1, mod.m_execStg);
		mifReg.Append("\tr_t%d_reqQwCnt = r_reset1x ? (ht_uint4)0 : c_t%d_reqQwCnt;\n",
			mod.m_execStg+2, mod.m_execStg+1);
	}

	if (bNeed_reqQwSplit)
	{
		m_mifDecl.Append("\tbool r_t%d_reqQwSplit;\n", mod.m_execStg+1);
		m_mifDecl.Append("\tbool r_t%d_reqQwSplit;\n", mod.m_execStg+2);

		mifReg.Append("\tr_t%d_reqQwSplit = c_t%d_reqQwSplit;\n",
			mod.m_execStg+1, mod.m_execStg);
		mifReg.Append("\tr_t%d_reqQwSplit = c_t%d_reqQwSplit;\n",
			mod.m_execStg+2, mod.m_execStg+1);
	}

	if (bNeed_reqQwIdx)
	{
		for (int i = 0; i < reqQwIdxStgCnt; i += 1)
			m_mifDecl.Append("\tht_uint3 r_t%d_reqQwIdx;\n", mod.m_execStg+1+i);

		mifReg.Append("\tr_t%d_reqQwIdx = r_reset1x ? (ht_uint3)0 : c_t%d_reqQwIdx;\n",
			mod.m_execStg+1, mod.m_execStg);

		for (int i = 1; i < reqQwIdxStgCnt; i += 1)
			mifReg.Append("\tr_t%d_reqQwIdx = c_t%d_reqQwIdx;\n", mod.m_execStg+1+i, mod.m_execStg+i);
	}

	for (int i = 0; i < mifReqStgCnt; i += 1) {
		mifPreInstr.Append("\tbool c_t%d_%sToMif_reqRdy = r_t%d_%sToMif_reqRdy;\n",
			mod.m_execStg+1+i, mod.m_modName.Lc().c_str(),
			mod.m_execStg+1+i, mod.m_modName.Lc().c_str());
		mifPreInstr.Append("\tCMemRdWrReqIntf c_t%d_%sToMif_req = r_t%d_%sToMif_req;\n",
			mod.m_execStg+1+i, mod.m_modName.Lc().c_str(),
			mod.m_execStg+1+i, mod.m_modName.Lc().c_str());
		mifPreInstr.Append("\n");
	}

	for (int i = 0; i < mifReqStgCnt+1; i += 1) {
		m_mifDecl.Append("\tbool r_t%d_%sToMif_reqRdy;\n",
			mod.m_execStg+1+i, mod.m_modName.Lc().c_str());
		mifReg.Append("\tr_t%d_%sToMif_reqRdy = !r_reset1x && c_t%d_%sToMif_reqRdy;\n",
			mod.m_execStg+1+i, mod.m_modName.Lc().c_str(),
			mod.m_execStg+i, mod.m_modName.Lc().c_str());
		m_mifDecl.Append("\tCMemRdWrReqIntf r_t%d_%sToMif_req;\n",
			mod.m_execStg+1+i, mod.m_modName.Lc().c_str());
		mifReg.Append("\tr_t%d_%sToMif_req = c_t%d_%sToMif_req;\n",
			mod.m_execStg+1+i, mod.m_modName.Lc().c_str(),
			mod.m_execStg+i, mod.m_modName.Lc().c_str());
	}

	mifReg.Append("\tr_%sToMif_reqAvlCnt = r_reset1x ? (ht_uint%d)%ld : c_%sToMif_reqAvlCnt;\n",
		mod.m_modName.Lc().c_str(),
		mif.m_queueW+1, 1ul << mif.m_queueW,
		mod.m_modName.Lc().c_str());
	mifReg.Append("\tr_%sToMif_reqAvlCntBusy = !r_reset1x && c_%sToMif_reqAvlCntBusy;\n",
		mod.m_modName.Lc().c_str(),
		mod.m_modName.Lc().c_str());
	mifReg.NewLine();

	if (mif.m_mifWr.m_bReqPause)
	{
		if (mod.m_threads.m_htIdW.AsInt() > 0)
			m_mifDecl.Append("\tsc_uint<%s_HTID_W> r_wrpRsmCnt;\n", mod.m_modName.Upper().c_str());
		m_mifDecl.Append("\tbool r_t%d_bWrpRsm;\n", mod.m_execStg+1);
		m_mifDecl.Append("\tbool r_t%d_bWrpRsm;\n", mod.m_execStg+2);

		if (mod.m_threads.m_htIdW.AsInt() > 0)
			mifReg.Append("\tr_wrpRsmCnt = r_reset1x ? (sc_uint<%s_HTID_W>)0 : c_wrpRsmCnt;\n", mod.m_modName.Upper().c_str());
		mifReg.Append("\tr_t%d_bWrpRsm = !r_reset1x && c_t%d_bWrpRsm;\n",
			mod.m_execStg+1, mod.m_execStg);
		mifReg.Append("\tr_t%d_bWrpRsm = !r_reset1x && c_t%d_bWrpRsm;\n",
			mod.m_execStg+2, mod.m_execStg+1);
		mifReg.NewLine();
	}

	if (mif.m_bMifRd) {
		if (mif.m_mifRd.m_rspGrpIdW > 2) {
			mifReg.Append("\tr_rdRspGrpInitCnt = c_rdRspGrpInitCnt;\n");
			mifReg.Append("\tr_t%d_bReadRspBusy = c_t%d_bReadRspBusy;\n", mod.m_execStg, mod.m_execStg-1);
		}
		mifReg.Append("\tr_t%d_rdRspBusyCntMax = c_t%d_rdRspBusyCntMax;\n", mod.m_execStg, mod.m_execStg-1);
		if (mif.m_mifRd.m_bPause)
			mifReg.Append("\tr_t%d_rdRspPauseCntZero = c_t%d_rdRspPauseCntZero;\n", mod.m_execStg, mod.m_execStg-1);
		if (mif.m_mifRd.m_bPoll) {
			if (mif.m_mifRd.m_rspGrpId.size() == 0)
				mifReg.Append("\tr_t%d_rdRspPollCntZero = c_t%d_rdRspPollCntZero;\n", mod.m_execStg, mod.m_execStg-1);
		}
		if (mif.m_mifRd.m_rspCntW.AsInt() > 0) {
			m_mifCtorInit.Append("\t\tr_m1_rdRspRdy = false;\n");

			mifReg.Append("\tr_m1_rdRspRdy = !r_reset1x && c_m0_rdRspRdy;\n");
			if (mif.m_mifRd.m_bPoll)
				mifReg.Append("\tr_m2_rdRspRdy = c_m1_rdRspRdy;\n");
			if (mif.m_mifRd.m_bPause && mif.m_mifRd.m_bPoll || mif.m_mifRd.m_bPoll && mif.m_mifRd.m_rspGrpIdW == 0)
				mifReg.Append("\tr_m3_rdRspRdy = c_m2_rdRspRdy;\n");
			if (mif.m_mifRd.m_rspGrpIdW > 0)
				mifReg.Append("\tr_m1_rdRspGrpId = c_m0_rdRspGrpId;\n");
			if (mif.m_mifRd.m_rspGrpIdW > 0 && mif.m_mifRd.m_bPoll)
				mifReg.Append("\tr_m2_rdRspGrpId = c_m1_rdRspGrpId;\n");
			if (mif.m_mifRd.m_rspGrpIdW > 0 && (mif.m_mifRd.m_bPause && mif.m_mifRd.m_bPoll || mif.m_mifRd.m_bPoll && mif.m_mifRd.m_rspGrpIdW == 0))
				mifReg.Append("\tr_m3_rdRspGrpId = c_m2_rdRspGrpId;\n");
			mifReg.Append("\tr_m1_rdRsp = i_mifTo%sP0_rdRsp;\n",
				mod.m_modName.Uc().c_str());
			mifReg.NewLine();
		}

		if (mif.m_mifRd.m_maxRsmDly > 0 && mif.m_mifRd.m_bPause) {
			if (mod.m_threads.m_htIdW.AsInt() > 0) {

				for (int i = mif.m_mifRd.m_maxRsmDly-1; i > 0; i -= 1) {
					mifReg.Append("\tr_rdRspRsmValid[%d] = r_rdRspRsmValid[%d];\n", i, i-1);
					mifReg.Append("\tr_rdRspRsm[%d] = r_rdRspRsm[%d];\n", i, i-1);
				}
				mifReg.Append("\tr_rdRspRsmValid[0] = c_rdRspRsmValid;\n");
				mifReg.Append("\tr_rdRspRsm[0] = c_rdRspRsm;\n");

			} else {
				for (int i = mif.m_mifRd.m_maxRsmDly-1; i > 0; i -= 1) {
					mifReg.Append("\tr_rdRspRsmRdy[%d] = r_rdRspRsmRdy[%d];\n", i, i-1);
					mifReg.Append("\tr_rdRspRsmInstr[%d] = r_rdRspRsmInstr[%d];\n", i, i-1);
				}
				mifReg.Append("\tr_rdRspRsmRdy[0] = c_rdRspRsmRdy;\n");
				mifReg.Append("\tr_rdRspRsmInstr[0] = c_rdRspRsmInstr;\n");
			}
		}

		if (mif.m_mifRd.m_rspGrpIdW > 2) {
			for (int bankIdx = 0; bankIdx < rdGrpBankCnt; bankIdx += 1)
				mifReg.Append("\tr_rdGrpRspCnt[%d] = r_reset1x ? (sc_uint<%s_RD_RSP_CNT_W>)0 : c_rdGrpRspCnt[%d];\n", bankIdx, mod.m_modName.Upper().c_str(), bankIdx);

			if (mif.m_mifRd.m_bPoll && !mif.m_mifRd.m_bPause)
				mifReg.Append("\tr_m1_rdRspGrpId = c_m0_rdRspGrpId;\n");

			for (int bankIdx = 0; bankIdx < rdGrpBankCnt; bankIdx += 1) {
				mifReg.Append("\tr_m%d_bRdRspRdy[%d] = c_m%d_bRdRspRdy[%d];\n",
					mif.m_mifRd.m_bPoll ? 3 : 1, bankIdx,
					mif.m_mifRd.m_bPoll ? 2 : 0, bankIdx);
			}

			for (int bankIdx = 0; bankIdx < rdGrpBankCnt; bankIdx += 1)
				mifReg.Append("\tr_rdGrpRspCntAddr[%d] = c_rdGrpRspCntAddr[%d];\n", bankIdx, bankIdx);
		}
	}

	if (mif.m_bMifWr) {
		if (mif.m_mifWr.m_rspGrpIdW > 2) {
			mifReg.Append("\tr_wrRspGrpInitCnt = c_wrRspGrpInitCnt;\n");
			mifReg.Append("\tr_t%d_bWriteRspBusy = c_t%d_bWriteRspBusy;\n", mod.m_execStg, mod.m_execStg-1);
		}
		mifReg.Append("\tr_t%d_wrGrpRspCntMax = c_t%d_wrGrpRspCntMax;\n", mod.m_execStg, mod.m_execStg-1);
		mifReg.Append("\tr_t%d_wrGrpRspCntZero = c_t%d_wrGrpRspCntZero;\n", mod.m_execStg, mod.m_execStg-1);

		if (mif.m_mifWr.m_rspCntW.AsInt() > 0 && mif.m_mifWr.m_bPause) {
			m_mifCtorInit.Append("\t\tr_m1_wrRspRdy = false;\n");

			mifReg.Append("\tr_m1_wrRspRdy = !r_reset1x && c_m0_wrRspRdy;\n",
				mod.m_modName.Uc().c_str());
			if (mif.m_mifWr.m_rspGrpIdW > 0)
				mifReg.Append("\tr_m1_wrRspTid = i_mifTo%sP0_wrRspTid;\n",
				mod.m_modName.Uc().c_str());
			mifReg.NewLine();
		}

		if (mif.m_mifWr.m_rspGrpIdW > 2) {
			for (int bankIdx = 0; bankIdx < wrGrpBankCnt; bankIdx += 1)
				mifReg.Append("\tr_wrGrpRspCnt[%d] = r_reset1x ? (sc_uint<%s_WR_RSP_CNT_W>)0 : c_wrGrpRspCnt[%d];\n", bankIdx, mod.m_modName.Upper().c_str(), bankIdx);

			for (int bankIdx = 0; bankIdx < wrGrpBankCnt; bankIdx += 1)
				mifReg.Append("\tr_m1_bWrRspRdy[%d] = c_m0_bWrRspRdy[%d];\n", bankIdx, bankIdx);

			for (int bankIdx = 0; bankIdx < wrGrpBankCnt; bankIdx += 1)
				mifReg.Append("\tr_wrGrpRspCntAddr[%d] = c_wrGrpRspCntAddr[%d];\n", bankIdx, bankIdx);
		}
	}

	mifReg.NewLine();

	mifOut.Append("\to_%sP0ToMif_reqRdy = r_t%d_%sToMif_reqRdy;\n",
		mod.m_modName.Lc().c_str(), mod.m_execStg + 1 + mifReqStgCnt, mod.m_modName.Lc().c_str());
	mifOut.Append("\to_%sP0ToMif_req = r_t%d_%sToMif_req;\n",
		mod.m_modName.Lc().c_str(), mod.m_execStg + 1 + mifReqStgCnt, mod.m_modName.Lc().c_str());

	mifPostInstr.Append("\n\n");
	m_mifRamDecl.NewLine();
	mifRamClock.NewLine();
}
