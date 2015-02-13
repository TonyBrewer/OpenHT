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
	AutoDeclType("ht_uint1", true);
	AutoDeclType("ht_uint2", true);
	AutoDeclType("ht_uint3", true);
	AutoDeclType("ht_uint4", true);
	AutoDeclType("ht_uint7", true);
	AutoDeclType("ht_uint56", true);

	InitNativeCTypes();

	ValidateUsedTypes();

	// Cxr sets module used flag
	InitAndValidateModCxr();

	// Cxr sets mod_INSTR_W
	ValidateDesignInfo();

	// Initialize addr1W from addr1 and addr2W from addr2
	InitAddrWFromAddrName();

	InitAndValidateModMif();
	InitAndValidateModNgv();
	InitAndValidateModIpl();
	InitAndValidateModRam();
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

	for (size_t structIdx = 0; structIdx < m_structList.size(); structIdx += 1) {
		CStruct & struc = m_structList[structIdx];

		InitFieldDimenValues(struc.m_fieldList);
	}

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		mod.m_stage.m_privWrStg.InitValue(mod.m_stage.m_lineInfo, false, 1);
		mod.m_stage.m_execStg.InitValue(mod.m_stage.m_lineInfo, false, 1);

		if (mod.m_stage.m_privWrStg.AsInt() > 1 || mod.m_stage.m_execStg.AsInt() > 1)
			mod.m_stage.m_bStageNums = true;

		for (size_t callIdx = 0; callIdx < mod.m_cxrCallList.size(); callIdx += 1) {
			CCxrCall & cxrCall = mod.m_cxrCallList[callIdx];

			cxrCall.m_queueW.InitValue(cxrCall.m_lineInfo);
		}

		if (mod.m_bHasThreads) {
			mod.m_threads.m_htIdW.InitValue(mod.m_threads.m_lineInfo);

			for (size_t prIdx = 0; prIdx < mod.m_threads.m_htPriv.m_fieldList.size(); prIdx += 1) {
				CField & priv = mod.m_threads.m_htPriv.m_fieldList[prIdx];

				priv.m_addr1W.InitValue(priv.m_lineInfo, false, 0);
				priv.m_addr2W.InitValue(priv.m_lineInfo, false, 0);
				priv.DimenListInit(priv.m_lineInfo);

				priv.InitDimen();
			}
		}

		for (size_t shIdx = 0; shIdx < mod.m_shared.m_fieldList.size(); shIdx += 1) {
			CField & shared = mod.m_shared.m_fieldList[shIdx];

			shared.m_queueW.InitValue(shared.m_lineInfo, false, 0);
			shared.DimenListInit(shared.m_lineInfo);
			shared.m_rdSelW.InitValue(shared.m_lineInfo, false, 0);
			shared.m_wrSelW.InitValue(shared.m_lineInfo, false, 0);
			shared.m_addr1W.InitValue(shared.m_lineInfo, false, 0);
			shared.m_addr2W.InitValue(shared.m_lineInfo, false, 0);

			shared.InitDimen();
		}

		for (size_t stgIdx = 0; stgIdx < mod.m_stage.m_fieldList.size(); stgIdx += 1) {
			CField &field = mod.m_stage.m_fieldList[stgIdx];

			field.m_rngLow.InitValue(field.m_lineInfo);
			field.m_rngHigh.InitValue(field.m_lineInfo);

			field.DimenListInit(field.m_lineInfo);
			field.InitDimen();
		}

		for (size_t gvIdx = 0; gvIdx < mod.m_globalVarList.size(); gvIdx += 1) {
			CRam & gv = *mod.m_globalVarList[gvIdx];

			gv.m_addr1W.InitValue(gv.m_lineInfo, false, 0);
			gv.m_addr2W.InitValue(gv.m_lineInfo, false, 0);
			gv.DimenListInit(gv.m_lineInfo);
			gv.m_rdStg.InitValue(gv.m_lineInfo, false, 1);
			gv.m_wrStg.InitValue(gv.m_lineInfo, false, 1);

			gv.InitDimen();

			for (size_t fldIdx = 0; fldIdx < gv.m_fieldList.size(); fldIdx += 1) {
				CField & field = gv.m_fieldList[fldIdx];

				field.DimenListInit(field.m_lineInfo);
				field.InitDimen();
			}
		}

		for (size_t gblIdx = 0; gblIdx < mod.m_intGblList.size(); gblIdx += 1) {
			CRam & intRam = *mod.m_intGblList[gblIdx];

			intRam.m_addr0W.InitValue(intRam.m_lineInfo, false, 0);
			intRam.m_addr1W.InitValue(intRam.m_lineInfo, false, 0);
			intRam.m_addr2W.InitValue(intRam.m_lineInfo, false, 0);
			intRam.DimenListInit(intRam.m_lineInfo);
			intRam.m_rdStg.InitValue(intRam.m_lineInfo, false, 1);
			intRam.m_wrStg.InitValue(intRam.m_lineInfo, false, 1);

			intRam.InitDimen();

			for (size_t fldIdx = 0; fldIdx < intRam.m_fieldList.size(); fldIdx += 1) {
				CField & field = intRam.m_fieldList[fldIdx];

				field.DimenListInit(field.m_lineInfo);
				field.InitDimen();
			}
		}

		for (size_t gblIdx = 0; gblIdx < mod.m_extRamList.size(); gblIdx += 1) {
			CRam & extRam = mod.m_extRamList[gblIdx];

			extRam.m_addr0W.InitValue(extRam.m_lineInfo, false, 0);
			extRam.m_addr1W.InitValue(extRam.m_lineInfo, false, 0);
			extRam.m_addr2W.InitValue(extRam.m_lineInfo, false, 0);
			extRam.DimenListInit(extRam.m_lineInfo);
			extRam.m_rdStg.InitValue(extRam.m_lineInfo, false, 1);
			extRam.m_wrStg.InitValue(extRam.m_lineInfo, false, 1);

			extRam.InitDimen();

			for (size_t fldIdx = 0; fldIdx < extRam.m_fieldList.size(); fldIdx += 1) {
				CField & field = extRam.m_fieldList[fldIdx];

				field.DimenListInit(field.m_lineInfo);
				field.InitDimen();
			}
		}

		// MIF values
		if (mod.m_mif.m_bMifWr) {
			CMifWr & mifWr = mod.m_mif.m_mifWr;
			mifWr.m_queueW.InitValue(mifWr.m_lineInfo);

			int defValue = max(6, 9 - mod.m_threads.m_htIdW.AsInt());
			mifWr.m_rspCntW.InitValue(mifWr.m_lineInfo, false, defValue);

			if (mifWr.m_rspCntW.AsInt() < 1 || mifWr.m_rspCntW.AsInt() > 9)
				ParseMsg(Error, mifWr.m_lineInfo, "rspCntW out of range, value %d, allowed range 1-9",
					mifWr.m_rspCntW.AsInt());
		}

		if (mod.m_mif.m_bMifRd) {
			CMifRd & mifRd = mod.m_mif.m_mifRd;
			mifRd.m_queueW.InitValue(mifRd.m_lineInfo);

			int defValue = max(6, 9 - mod.m_threads.m_htIdW.AsInt());
			mifRd.m_rspCntW.InitValue(mifRd.m_lineInfo, false, defValue);

			if (mifRd.m_rspCntW.AsInt() < 1 || mifRd.m_rspCntW.AsInt() > 9)
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
		for (size_t ramIdx = 0; ramIdx < mod.m_globalVarList.size(); ramIdx += 1) {
			CRam &gv = *mod.m_globalVarList[ramIdx];

			if (gv.m_addr1Name.size() > 0) {
				int addr1W;
				bool bHtId = true;
				bool bPrivate = true;
				bool bShared = true;
				bool bStage = true;

				if (FindVariableWidth(gv.m_lineInfo, mod, gv.m_addr1Name, bHtId, bPrivate, bShared, bStage, addr1W)) {
					if (gv.m_addr1W.size() == 0) {
						gv.m_addr1W = CHtString(VA("%d", addr1W));
						gv.m_addr1W.SetValue(addr1W);
					}
					else if (gv.m_addr1W.AsInt() != addr1W)
						ParseMsg(Error, gv.m_lineInfo, "addr1Name (%d) and addr1W (%d) have inconsistent widths",
							addr1W, gv.m_addr1W.AsInt());
				}
			}

			if (gv.m_addr2Name.size() > 0) {
				int addr2W;
				bool bHtId = true;
				bool bPrivate = true;
				bool bShared = true;
				bool bStage = true;

				if (FindVariableWidth(gv.m_lineInfo, mod, gv.m_addr2Name, bHtId, bPrivate, bShared, bStage, addr2W)) {
					if (gv.m_addr2W.size() == 0) {
						gv.m_addr2W = CHtString(VA("%d", addr2W));
						gv.m_addr2W.SetValue(addr2W);
					}
					else if (gv.m_addr2W.AsInt() != addr2W) {
						ParseMsg(Error, gv.m_lineInfo, "addr2Name (%d) and addr2W (%d) have inconsistent widths",
							addr2W, gv.m_addr2W.AsInt());
					}
				}
			}
		}

		// check internal global variables for specification of an addrName but not an addrW
		for (size_t ramIdx = 0; ramIdx < mod.m_intGblList.size(); ramIdx += 1) {
			CRam &intGbl = *mod.m_intGblList[ramIdx];

			if (intGbl.m_addr1W.size() == 0 && intGbl.m_addr1Name.size() > 0) {
				int addr1W;
				bool bHtId = true;
				bool bPrivate = true;
				bool bShared = true;
				bool bStage = true;

				if (FindVariableWidth(intGbl.m_lineInfo, mod, intGbl.m_addr1Name, bHtId, bPrivate, bShared, bStage, addr1W)) {
					intGbl.m_addr1W = CHtString(VA("%d", addr1W));
					intGbl.m_addr1W.SetValue(addr1W);
				}
			}

			if (intGbl.m_addr2W.size() == 0 && intGbl.m_addr2Name.size() > 0) {
				int addr2W;
				bool bHtId = true;
				bool bPrivate = true;
				bool bShared = true;
				bool bStage = true;

				if (FindVariableWidth(intGbl.m_lineInfo, mod, intGbl.m_addr2Name, bHtId, bPrivate, bShared, bStage, addr2W)) {
					intGbl.m_addr2W = CHtString(VA("%d", addr2W));
					intGbl.m_addr2W.SetValue(addr2W);
				}
			}
		}

		// check external global variables for specification of an addrName but not an addrW
		for (size_t ramIdx = 0; ramIdx < mod.m_extRamList.size(); ramIdx += 1) {
			CRam &extRam = mod.m_extRamList[ramIdx];

			if (extRam.m_addr1W.size() == 0 && extRam.m_addr1Name.size() > 0) {
				int addr1W;
				bool bHtId = true;
				bool bPrivate = true;
				bool bShared = false;
				bool bStage = true;

				if (FindVariableWidth(extRam.m_lineInfo, mod, extRam.m_addr1Name, bHtId, bPrivate, bShared, bStage, addr1W)) {
					extRam.m_addr1W = CHtString(VA("%d", addr1W));
					extRam.m_addr1W.SetValue(addr1W);
				}
			}

			if (extRam.m_addr2W.size() == 0 && extRam.m_addr2Name.size() > 0) {
				int addr2W;
				bool bHtId = true;
				bool bPrivate = true;
				bool bShared = false;
				bool bStage = true;

				if (FindVariableWidth(extRam.m_lineInfo, mod, extRam.m_addr2Name, bHtId, bPrivate, bShared, bStage, addr2W)) {
					extRam.m_addr2W = CHtString(VA("%d", addr2W));
					extRam.m_addr2W.SetValue(addr2W);
				}
			}
		}

		// check private variables for specification of an addrName but not an addrW
		for (size_t prIdx = 0; prIdx < mod.m_threads.m_htPriv.m_fieldList.size(); prIdx += 1) {
			CField & priv = mod.m_threads.m_htPriv.m_fieldList[prIdx];

			if (priv.m_addr1W.size() == 0 && priv.m_addr1Name.size() > 0) {
				int addr1W;
				bool bHtId = true;
				bool bPrivate = true;
				bool bShared = true;
				bool bStage = true;

				if (FindVariableWidth(priv.m_lineInfo, mod, priv.m_addr1Name, bHtId, bPrivate, bShared, bStage, addr1W)) {
					priv.m_addr1W = CHtString(VA("%d", addr1W));
					priv.m_addr1W.SetValue(addr1W);
				}
			}

			if (priv.m_addr2W.size() == 0 && priv.m_addr2Name.size() > 0) {
				int addr2W;
				bool bHtId = false;
				bool bPrivate = true;
				bool bShared = true;
				bool bStage = true;

				if (FindVariableWidth(priv.m_lineInfo, mod, priv.m_addr2Name, bHtId, bPrivate, bShared, bStage, addr2W)) {
					priv.m_addr2W = CHtString(VA("%d", addr2W));
					priv.m_addr2W.SetValue(addr2W);
				}
			}
		}
	}
}

void CDsnInfo::InitFieldDimenValues(vector<CField> &fieldList)
{
	for (size_t fldIdx = 0; fldIdx < fieldList.size(); fldIdx += 1) {
		CField & field = fieldList[fldIdx];

		if (field.m_type == "union" || field.m_type == "struct")
			InitFieldDimenValues(field.m_pStruct->m_fieldList);
		else {
			field.DimenListInit(field.m_lineInfo);
			field.InitDimen();
		}
	}
}

bool BramTargetListCmp( const CBramTarget & elem1, const CBramTarget & elem2 )
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

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		// add group private to target list
		if (mod.m_bHasThreads && mod.m_threads.m_htPriv.m_fieldList.size() > 0) {

			CBramTarget target;
			target.m_name = "";
			target.m_pRamType = &mod.m_threads.m_ramType;
			target.m_depth = 1 << mod.m_threads.m_htIdW.AsInt();
			target.m_width = FindFieldListWidth("", mod.m_threads.m_lineInfo, mod.m_threads.m_htPriv.m_fieldList);
			target.m_copies = (int)mod.m_modInstList.size();
			target.m_brams = FindBramCnt(target.m_depth, target.m_width);
			target.m_slicePerBramRatio = FindSlicePerBramRatio(target.m_depth, target.m_width) / target.m_brams;
			target.m_varType = "Private";
			target.m_modName = mod.m_modName.AsStr();

			m_bramTargetList.push_back(target);
		}

		for (size_t gblIdx = 0; gblIdx < mod.m_intGblList.size(); gblIdx += 1) {
			CRam & intRam = *mod.m_intGblList[gblIdx];

			for (size_t fldIdx = 0; fldIdx < intRam.m_fieldList.size(); fldIdx += 1) {
				CField & field = intRam.m_allFieldList[fldIdx];

				CBramTarget target;
				target.m_name = string(intRam.m_gblName.c_str()) + "." + field.m_name;
				target.m_pRamType = &field.m_ramType;
				target.m_depth = 1 << (intRam.m_addr0W.AsInt() + intRam.m_addr1W.AsInt() + intRam.m_addr2W.AsInt());
				target.m_width = FindTypeWidth(field);
				target.m_width *= field.m_elemCnt;
				target.m_copies = (int)mod.m_modInstList.size();
				target.m_copies *= intRam.m_elemCnt;
				target.m_copies *= (int)(field.m_readerList.size() + 1) / 2;
				target.m_brams = FindBramCnt(target.m_depth, target.m_width);
				target.m_slicePerBramRatio = FindSlicePerBramRatio(target.m_depth, target.m_width) / target.m_brams;
				target.m_varType = "Global";
				target.m_modName = mod.m_modName.AsStr();

				m_bramTargetList.push_back(target);
			}
		}

		for (size_t shIdx = 0; shIdx < mod.m_shared.m_fieldList.size(); shIdx += 1) {
			CField &shared = mod.m_shared.m_fieldList[shIdx];

			if (shared.m_addr1W.AsInt() == 0 && shared.m_queueW.AsInt() == 0)
				continue;

			CBramTarget target;
			target.m_name = shared.m_name;
			target.m_pRamType = &shared.m_ramType;
			if (shared.m_addr1W.AsInt() > 0)
				target.m_depth = 1 << (shared.m_addr1W.AsInt() + shared.m_addr2W.AsInt());
			else
				target.m_depth = 1 << shared.m_queueW.AsInt();
			target.m_width = FindTypeWidth(shared);
			target.m_width *= shared.m_elemCnt;
			target.m_copies = (int)mod.m_modInstList.size();
			target.m_copies *= shared.m_elemCnt;
			target.m_brams = FindBramCnt(target.m_depth, target.m_width);
			target.m_slicePerBramRatio = FindSlicePerBramRatio(target.m_depth, target.m_width) / target.m_brams;
			target.m_varType = "Shared";
			target.m_modName = mod.m_modName.AsStr();

			m_bramTargetList.push_back(target);
		}
	}

	// sort vector based on slicePerBramRatio
	sort( m_bramTargetList.begin(), m_bramTargetList.end(), BramTargetListCmp );

	// Now decide which rams to implement with block rams
	int brams18KbAvailCnt = g_appArgs.GetMax18KbBramPerUnit();

	// first check if depth is too great for distributed rams
	for (size_t targetIdx = 0; targetIdx < m_bramTargetList.size(); targetIdx += 1) {
		CBramTarget & target = m_bramTargetList[targetIdx];

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

		if (target.m_varType != "Shared" && target.m_slicePerBramRatio > g_appArgs.GetMinLutToBramRatio() && target.m_brams * target.m_copies <= brams18KbAvailCnt) {
			*target.m_pRamType = eBlockRam;
			brams18KbAvailCnt -= target.m_brams * target.m_copies;
		}
	}
}

float CDsnInfo::FindSlicePerBramRatio(int depth, int width)
{
	float ratio;
	if (depth <= 64)
		// 3 bits per slice
		ratio = width/3.0f;
	else
		// 1 bit per slice
		ratio = (float)((depth+127)/128 * width);

	return ratio;
}

int CDsnInfo::FindBramCnt(int depth, int width)
{
	int brams;
	if (width <= 1)
		brams = (depth + 16*1024-1)/(16*1024);
	else if (width <= 2)
		brams = (depth + 8*1024-1)/(8*1024);
	else if (width <= 4)
		brams = (depth + 4*1024-1)/(4*1024);
	else if (width <= 9)
		brams = (depth + 2*1024-1)/(2*1024);
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
	for (size_t i = 0; i < m_structList.size(); i += 1) {
		CStruct & struct_ = m_structList[i];

		if (struct_.m_structName != structType) continue;

		// found it, now determine with of struct/union
		structName = structType;
		return true;
	}

	return false;
}

bool CDsnInfo::FindIntType(string typeName, int &width, bool &bSigned)
{
	// first check if typeName was defined as a typedef
	for (size_t i = 0; i < m_typedefList.size(); i += 1) {
		CTypeDef & typeDef = m_typedefList[i];

		if (typeDef.m_name == typeName)
			return FindIntType(typeDef.m_type, width, bSigned);
	}

	if (FindCIntType(typeName, width, bSigned))
		return true;

	if (FindHtIntType(typeName, width, bSigned))
		return true;

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

bool CDsnInfo::FindHtIntType(string typeName, int & width, bool & bSigned)
{
	// first check if typeName was defined as a typedef
	for (size_t i = 0; i < m_typedefList.size(); i += 1) {
		CTypeDef & typeDef = m_typedefList[i];

		if (typeDef.m_name == typeName)
			return FindHtIntType(typeDef.m_type, width, bSigned);
	}

	if (sscanf(typeName.c_str(), "sc_int<%d>", &width) == 1) {
		bSigned = true;
		return true;
	} else if (sscanf(typeName.c_str(), "ht_int%d", &width) == 1) {
		bSigned = true;
		return true;
	} else if (sscanf(typeName.c_str(), "ht_uint%d", &width) == 1) {
		bSigned = false;
		return true;
	} else if (sscanf(typeName.c_str(), "ht_uint%d", &width) == 1) {
		bSigned = false;
		return true;
	} else
		return false;
}

int CDsnInfo::FindTypeWidth(CField const & field, int * pMinAlign, bool bHostType)
{
	return FindTypeWidth(field.m_name, field.m_type, field.m_bitWidth, field.m_lineInfo, pMinAlign, bHostType);
}

int CDsnInfo::FindHostTypeWidth(CField const & field, int * pMinAlign)
{
	return FindTypeWidth(field.m_name, field.m_hostType, field.m_bitWidth, field.m_lineInfo, pMinAlign, true);
}

int CDsnInfo::FindTypeWidth(string const &varName, string const &typeName, CHtString const &bitWidth, CLineInfo const &lineInfo, int * pMinAlign, bool bHostType)
{
	// first remove trailing astricks
	bool bPointer = false;
	int i;
	for (i = (int)typeName.size()-1; i >= 0; i -= 1) {
		if (typeName[i] == '*')
			bPointer = true;
		else if (typeName[i] != ' ')
			break;
	}

	if (!bHostType && bPointer)
		ParseMsg(Error, lineInfo, "use of pointer types is not supported");

	string baseName = typeName.substr(0, i+1);

	int minAlign;

	// first check if typeName was defined as a typedef
	for (size_t i = 0; i < m_typedefList.size(); i += 1) {
		CTypeDef & typeDef = m_typedefList[i];

		if (typeDef.m_name == baseName) {
			HtlAssert(bitWidth.size() == 0);
			int width = FindTypeWidth(varName, typeDef.m_type, typeDef.m_width, lineInfo, &minAlign, bHostType);
			if (pMinAlign) *pMinAlign = minAlign;
			return width == 0 ? 0 : (bPointer ? 64 : width);
		}
	}

	// not found as typedef, check user structs
	for (size_t i = 0; i < m_structList.size(); i += 1) {
		CStruct & struct_ = m_structList[i];

		if (struct_.m_structName != baseName) continue;

		// found it, now determine with of struct/union

		HtlAssert(bitWidth.size() == 0);
		int width = FindStructWidth(struct_, &minAlign, bHostType);
		if (pMinAlign) *pMinAlign = minAlign;
		return width == 0 ? 0 : (bPointer ? 64 : width);
	}

	// not found as typedef, must be a basic type
	for (size_t i = 0; i < m_nativeCTypeList.size(); i += 1) {
		CBasicType & basicType = m_nativeCTypeList[i];

		if (baseName == basicType.m_name) {
			if (bitWidth.size() > 0) {
				if (bitWidth.AsInt() > basicType.m_width)
					ParseMsg(Error, lineInfo, "%s bit field width (%d) too large for base type %s", varName.c_str(), bitWidth.AsInt(), typeName.c_str());
				if (pMinAlign) *pMinAlign = basicType.m_width;
				return bitWidth.AsInt();
			}
			if (pMinAlign) *pMinAlign = basicType.m_width;
			return bPointer ? 64 : basicType.m_width;
		}
	}

	if (bHostType) {
		if (baseName != "void" || !bPointer) {
			ParseMsg(Error, lineInfo, "type (%s) not supported for host call/return parameter", typeName.c_str());
			return 0;
		} else
			return 64;
	}

	int scTypeLen = 0;
	if (typeName.substr(0, scTypeLen=6) == "sc_int" ||
		typeName.substr(0, scTypeLen=7) == "sc_uint" ||
		typeName.substr(0, scTypeLen=9) == "sc_bigint" ||
		typeName.substr(0, scTypeLen=10) == "sc_biguint")
	{
		// evaluate width
		char buf[256];
		char * pBuf = buf;
		char const * pStr = typeName.c_str()+scTypeLen+1;
		while (*pStr != '>' && *pStr != '\0')
			*pBuf++ = *pStr++;
		*pBuf = '\0';

		int width;
		bool bIsSigned;
		if (!m_defineTable.FindStringValue(lineInfo, string(buf), width, bIsSigned)) {
			ParseMsg(Error, lineInfo, "unable to determine width, '%s'", typeName.c_str());
			return 0;
		}

		HtlAssert(bitWidth.size() == 0);
		return width;
	}

	ParseMsg(Error, lineInfo, "undefined type, '%s'", typeName.c_str());

	return 0;
}

bool CDsnInfo::FindFieldInStruct(CLineInfo const &lineInfo, string const &type,
	string const &fldName, bool bIndexCheck, bool &bCStyle, CField const * &pBaseField, CField const * &pLastField)
{
	size_t stIdx;
	for (stIdx = 0; stIdx < m_structList.size(); stIdx += 1) {
		CStruct & strut = m_structList[stIdx];

		if (strut.m_structName == type)
			break;
	}
	if (stIdx == m_structList.size()) {
		ParseMsg(Error, lineInfo, "did not find type in design struct/unions, '%s'", type.c_str());
		return false;
	}
	CStruct & strut = m_structList[stIdx];

	bCStyle = strut.m_bCStyle;

	return IsInFieldList(lineInfo, fldName, strut.m_fieldList, strut.m_bCStyle, bIndexCheck, pBaseField, pLastField, 0);
}

bool CDsnInfo::IsInFieldList(CLineInfo const &lineInfo, string const &fldName, vector<CField> const &fieldList,
	bool bCStyle, bool bIndexCheck, CField const * &pBaseField, CField const * &pLastField, string * pFullName)
{
	string identRef = fldName.substr(0, fldName.find_first_of('.'));
	string postName;
	if (identRef.size() < fldName.size())
		postName = fldName.substr(identRef.size()+1);

	char const *pStr = identRef.c_str();
	int spaceCnt = 0;
	while (isspace(*pStr)) pStr += 1, spaceCnt += 1;

	int identCnt = 0;
	while (isalpha(*pStr) || isdigit(*pStr) || *pStr == '_') pStr += 1, identCnt += 1;

	// ident name without leading white space or indexing
	string name = identRef.substr(spaceCnt, identCnt);

	for (size_t fldIdx = 0; fldIdx < fieldList.size(); fldIdx += 1) {
		CField const & field = fieldList[fldIdx];

		if (field.m_type == "union" || field.m_type == "struct") {	// CStyle anonamous struct/union
			size_t entryLen = pFullName ? pFullName->size() : 0;

			if (IsInFieldList(lineInfo, fldName, field.m_pStruct->m_fieldList, field.m_pStruct->m_bCStyle, true, pBaseField, pLastField, pFullName))
			{
				pBaseField = &field;
				return true;
			}

			if (pFullName)
				pFullName->erase(entryLen);

		} else if (field.m_name == "") {	// AddStruct/AddUnion anonamous struct/union
			bool bCStyle;
			if (FindFieldInStruct(lineInfo, field.m_type, fldName, true, bCStyle, pBaseField, pLastField)) {
				pBaseField = &field;
				return true;
			}

		} else {
			if (field.m_name != name)
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
				if (bIndexCheck && field.m_dimenList.size() < indexCnt)
					ParseMsg(Error, lineInfo, "unexpected variable/field index, %s", identRef.c_str());

				bool bSigned;
				int indexValue;
				m_defineTable.FindStringValue(lineInfo, pStr, indexValue, bSigned);

				if (*(pStr-1) != ']') {
					ParseMsg(Error, lineInfo, "variable/field index format error, %s", identRef.c_str());
					return false;
				}

				if (bIndexCheck && indexCnt <= field.m_dimenList.size() && indexValue >= field.m_dimenList[indexCnt-1].AsInt())
					ParseMsg(Error, lineInfo, "variable/field index value out of range, %s", identRef.c_str());

				while (isspace(*pStr)) pStr += 1;
			}

			if (bIndexCheck && indexCnt != field.m_dimenList.size())
				ParseMsg(Error, lineInfo, "variable/field has missing index, %s", identRef.c_str());

			if (postName.size() == 0) {
				pBaseField = &field;
				pLastField = &field;
				return true;
			}

			size_t stIdx;
			for (stIdx = 0; stIdx < m_structList.size(); stIdx += 1) {
				CStruct & strut = m_structList[stIdx];

				if (strut.m_structName == field.m_type)
					break;
			}

			if (stIdx == m_structList.size())
				return false;

			if (IsInFieldList(lineInfo, postName, m_structList[stIdx].m_fieldList, m_structList[stIdx].m_bCStyle, true,
				pBaseField, pLastField, pFullName))
			{
				pBaseField = &field;
				return true;
			}
		}
	}

	return false;
}

int CDsnInfo::FindStructWidth(CStruct & struct_, int * pMinAlign, bool bHostType)
{
	if (struct_.m_bUnion) {
		// find maximum field width
		int width = 0;
		int minAlign = 1;
		*pMinAlign = 1;
		for (size_t fldIdx = 0; fldIdx < struct_.m_fieldList.size(); fldIdx += 1) {
			CField & field = struct_.m_fieldList[fldIdx];

			int fldWidth;
			if (field.m_type == "struct" || field.m_type == "union")
				fldWidth = FindStructWidth(*field.m_pStruct, &minAlign, bHostType);
			else
				fldWidth = FindTypeWidth(field, &minAlign, bHostType) * field.m_elemCnt;

			if (*pMinAlign) *pMinAlign = max(*pMinAlign, minAlign);
			width = fldWidth > width ? fldWidth : width;
		}
		return width;

	} else {
		// struct find sum of field widths
		return FindFieldListWidth(struct_.m_structName, struct_.m_lineInfo, struct_.m_fieldList, pMinAlign, bHostType);
	}
}

int CDsnInfo::FindFieldListWidth(string structName, CLineInfo &lineInfo, vector<CField> &fieldList, int * pMinAlign, bool bHostType)
{
	int minAlign = 1;
	int width = 0;

	for (size_t fldIdx = 0; fldIdx < fieldList.size(); fldIdx += 1) {
		CField & field = fieldList[fldIdx];

		int fieldWidth;
		if (field.m_type == "struct" || field.m_type == "union")
			fieldWidth = FindStructWidth(*field.m_pStruct, &minAlign, bHostType);
		else
			fieldWidth = FindTypeWidth(field, &minAlign, bHostType) * field.m_elemCnt;

		if (bHostType) {
			int rem = width % minAlign;
			if (rem > 0 && bHostType) {
				static vector<string> structList;
				size_t i;
				for (i = 0; i < structList.size(); i += 1)
					if (structList[i] == structName)
						break;
				if (i == structList.size()) {
					structList.push_back(structName);
					ParseMsg(Error, lineInfo, "unsupported use of struct %s with unused bytes due to field alignment requirements", structName.c_str());
					ParseMsg(Info, lineInfo, "replace unused bytes with named fields as a work around");
				}
			}
			if (rem > 0)
				width = width - rem + minAlign;
		}

		width += fieldWidth;
	}

	return width;
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
	m_nativeCTypeList.push_back(CBasicType("long", sizeof(long)*8, true));
	m_nativeCTypeList.push_back(CBasicType("signed long", sizeof(long)*8, true));
	m_nativeCTypeList.push_back(CBasicType("unsigned long", sizeof(long)*8, false));
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

void CDsnInfo::ValidateFieldListTypes(CStruct & record)
{
	for (size_t fldIdx = 0; fldIdx < record.m_fieldList.size(); fldIdx += 1) {
		CField & field = record.m_fieldList[fldIdx];

		if (field.m_type == "union" || field.m_type == "struct")
			ValidateFieldListTypes( *field.m_pStruct );

		else if (!IsTypeNameValid( field.m_type ))
			ParseMsg(Error, field.m_lineInfo, "undeclared type, '%s'", field.m_type.c_str());

		if (field.m_bitWidth.size() > 0) {
			field.m_bitWidth.InitValue(field.m_lineInfo, false);

			int builtinWidth;
			bool bBuiltinSigned;
			HtlAssert(HtdFile::FindBuiltinType(field.m_type, builtinWidth, bBuiltinSigned));

			if (field.m_bitWidth.AsInt() > builtinWidth)
				ParseMsg(Error, field.m_lineInfo, "bit field width too large for field type");

			//field.m_type = VA("%s%d", bBuiltinSigned ? "ht_int" : "ht_uint", field.m_bitWidth.AsInt());
		}
	}

}

void CDsnInfo::ValidateUsedTypes()
{
	// Validate types in user structs/unions
	for (size_t idx = 0; idx < m_structList.size(); idx += 1) {
		ValidateFieldListTypes( m_structList[idx] );
	}

	// Validate types in modules
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		// Validate htShared variables
		for (size_t fldIdx = 0; fldIdx < mod.m_shared.m_fieldList.size(); fldIdx += 1) {
			CField & field = mod.m_shared.m_fieldList[fldIdx];

			if (!IsTypeNameValid( field.m_type, &mod ))
				ParseMsg(Error, field.m_lineInfo, "undeclared type, '%s'", field.m_type.c_str());
		}
	}
}

void CDsnInfo::ReportRamFieldUsage()
{
	if (! m_bramTargetList.size()) return;

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
	fprintf(fp, "%s      <dd>Min Slice to BRAM ratio<sup>1</sup> (use -lb to override): %d</dd>\n",
		ws, g_appArgs.GetMinLutToBramRatio());
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
	fprintf(fp, "%s        <th># LUTs</th>\n", ws);
	fprintf(fp, "%s        <th>Total</th>\n", ws);
	fprintf(fp, "%s        <th># 18Kb</th>\n", ws);
	fprintf(fp, "%s        <th>Total</th>\n", ws);
	fprintf(fp, "%s      </tr>\n", ws);

	for (size_t targetIdx = 0; targetIdx < m_bramTargetList.size(); targetIdx += 1) {
		CBramTarget & target = m_bramTargetList[targetIdx];
		int num_luts = target.m_width * (int)((target.m_depth+31)/32);

		fprintf(fp, "%s      <tr>\n", ws);
		fprintf(fp, "%s        <td>%-17s</td><td>%-17s</td><td>%-16s</td><td>%2d</td><td>%2d</td><td>%2d</td><td>%2d</td><td>%2d</td><td>%2d</td><td>%2d</td><td>%6.2f</td><td>%s</td>\n",
			ws, target.m_modName.c_str(), target.m_varType.c_str(),
			target.m_name.c_str(), target.m_depth, target.m_width,
			target.m_copies, num_luts, num_luts * target.m_copies,
			target.m_brams, target.m_brams * target.m_copies,
			(double)target.m_slicePerBramRatio,
			target.m_depth == 1 ? "Reg" : (*target.m_pRamType == eDistRam ? "Dist" : "Block"));
		fprintf(fp, "%s      </tr>\n", ws);
	}

	fprintf(fp, "%s    </dd></dl></table>\n", ws);

	bool bHasGRams = false;
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule * pMod = m_modList[modIdx];

		if (!pMod->m_bIsUsed || !pMod->m_intGblList.size()) continue;
		bHasGRams = true;
	}

	if (bHasGRams) {
		fprintf(fp, "%s    <br/>\n", ws);
		fprintf(fp, "%s    <dt><b>Global RAMs</b></dt>\n", ws);
	}

	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed || !mod.m_intGblList.size()) continue;

		fprintf(fp, "%s    <br/>\n", ws);
		fprintf(fp, "%s      <dd>Module: %s</dd>\n", ws, mod.m_modName.Uc().c_str());

		for (size_t intRamIdx = 0; intRamIdx < mod.m_intGblList.size(); intRamIdx += 1) {
			CRam &intRam = *mod.m_intGblList[intRamIdx];

			fprintf(fp, "%s        <dd><dl><dd>Internal Ram: %s</dd>\n", ws, intRam.m_gblName.Uc().c_str());

			for (size_t fldIdx = 0; fldIdx < intRam.m_allFieldList.size(); fldIdx += 1) {
				CField &field = intRam.m_allFieldList[fldIdx];

				fprintf(fp, "%s          <dd><dl><dd>Field %d: %s, %s</dd>\n", ws,
					(int)fldIdx, field.m_name.c_str(), field.m_ramType == eDistRam ? "DistRam" : "BlockRam");

				fprintf(fp, "%s            <dd><dl><dd>Readers:", ws);
				char *pFirst = "";
				for (size_t rdIdx = 0; rdIdx < field.m_readerList.size(); rdIdx += 1, pFirst = ",") {
					char const *pModStr;
					if (field.m_readerList[rdIdx].m_pRamPort == 0)
						pModStr = "---";
					else if (field.m_readerList[rdIdx].m_pRamPort->m_bIntIntf)
						pModStr =  mod.m_modName.Uc().c_str();
					else
						pModStr = field.m_readerList[rdIdx].m_pRamPort->m_pRam->m_pExtRamMod->m_modName.Uc().c_str();

					fprintf(fp, "%s %s", pFirst, pModStr);
				}
				fprintf(fp, "</dd>\n");
				fprintf(fp, "%s            </dl></dd>\n", ws);

				fprintf(fp, "%s            <dd><dl><dd>Writers:", ws);
				pFirst = "";
				for (size_t wrIdx = 0; wrIdx < field.m_writerList.size(); wrIdx += 1, pFirst = ",") {
					char const *pModStr = field.m_writerList[wrIdx].m_pRamPort->m_bIntIntf ? mod.m_modName.Uc().c_str()
						: field.m_writerList[wrIdx].m_pRamPort->m_pRam->m_pExtRamMod->m_modName.Uc().c_str();

					fprintf(fp, "%s %s", pFirst, pModStr);
				}
				fprintf(fp, "</dd>\n");
				fprintf(fp, "%s            </dl></dd>\n", ws);
				fprintf(fp, "%s          </dl></dd>\n", ws);
				fprintf(fp, "%s        </dl></dd>\n", ws);
			}
		}
	}

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

		int modInstCnt = (int)mod.m_modInstList.size();
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
			CModInst &modInst = mod.m_modInstList[rep];
			map<string, bool> cxr_seen;
			for (size_t intfIdx = 0; intfIdx < modInst.m_cxrIntfList.size(); intfIdx += 1) {
				CCxrIntf &cxrIntf = modInst.m_cxrIntfList[intfIdx];

				string key = cxrIntf.GetIntfName();
				if (cxr_seen.find(key) != cxr_seen.end()) continue;
				cxr_seen.insert(pair<string, bool>(key, true));

				int destUnitCnt = (int)cxrIntf.m_pDstMod->m_modInstList.size();
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

					string destName = cxrIntf.m_pDstMod->m_modName.Uc() + destRepStr;

					string label = " [label=\"";
					label += cxrIntf.m_funcName.c_str();
					label += "\"";
					label += cxrIntf.IsCall() ? " color=red" :
						 cxrIntf.IsXfer() ? " color=deeppink" : " color=green";
					label += "]";

					if (cxrIntf.m_cxrDir == CxrOut) {
						fs << "n" << modName << " -> n" << destName;
						fs << label << ";\n";
					}
				}
			}

			// msg
			for (size_t outIdx = 0; outIdx < mod.m_msgIntfList.size(); outIdx += 1) {
				CMsgIntf * pOutMsgIntf = mod.m_msgIntfList[outIdx];
				if (pOutMsgIntf->m_dir != "out") continue;
				for (size_t modIdx2 = 0; modIdx2 < m_modList.size(); modIdx2 += 1) {
					CModule &mod2 = *m_modList[modIdx2];
					if (!mod2.m_bIsUsed) continue;
					for (size_t msgIdx = 0; msgIdx < mod2.m_msgIntfList.size(); msgIdx += 1) {
						CMsgIntf * pMsgIntf = mod2.m_msgIntfList[msgIdx];
						if (pMsgIntf->m_dir != "in") continue;
						if (pMsgIntf->m_name != pOutMsgIntf->m_name) continue;

						int destUnitCnt = (int)mod2.m_modInstList.size();
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
