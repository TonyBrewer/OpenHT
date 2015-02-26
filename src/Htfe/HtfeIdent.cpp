/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Htfe.h"
#include "HtfeDesign.h"

CHtfeIdent * CHtfeIdent::SetPromoteToSigned(bool bPromoteToSigned) {
	m_bPromoteToSigned = bPromoteToSigned;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetPromoteToSigned(bPromoteToSigned);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetPromoteToSigned(bPromoteToSigned);
	return this;
}

CHtfeIdent * CHtfeIdent::SetIsSigned(bool bIsSigned) {
	m_bIsSigned = bIsSigned;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsSigned(bIsSigned);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetIsSigned(bIsSigned);
	return this;
}

CHtfeIdent * CHtfeIdent::SetType(CHtfeIdent *pType) {
	m_pType = pType;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetType(pType);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetType(pType);
	return this;
}

void CHtfeIdent::SetLineInfo(const CLineInfo &lineInfo) {
	m_lineInfo = lineInfo;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetLineInfo(lineInfo);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetLineInfo(lineInfo);
}

CHtfeIdent * CHtfeIdent::SetId(EIdentId id) {
	m_id = id;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetId(id);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetId(id);
	return this;
}

void CHtfeIdent::InsertSensitive(CHtfeIdent * pIdent, int elemIdx) {
    if (m_pSensitiveTbl == 0)
        m_pSensitiveTbl = new CScSensitiveTbl;
    m_pSensitiveTbl->Insert(pIdent, elemIdx);
}

CScSensitiveTbl & CHtfeIdent::GetSensitiveTbl() {
    if (m_pSensitiveTbl == 0)
        m_pSensitiveTbl = new CScSensitiveTbl;
    return *m_pSensitiveTbl;
}

void CHtfeIdent::NewFlatTable() {
	m_pFlatTbl = new CHtfeIdentTbl;
}

CHtfeIdent * CHtfeIdent::InsertIdent(const string &name, bool bInsertFlatTbl, bool bAllowOverloading)
{
	if (bInsertFlatTbl) {
		string flatVarName = name;
		if (m_name.size() == 0 && name != "true" && name != "false")
			flatVarName = name + "$g";
		return InsertIdent(name, flatVarName);
	}

    if (m_pIdentTbl == 0) m_pIdentTbl = new CHtfeIdentTbl;
	CHtfeIdent *pHierIdent = m_pIdentTbl->Insert(name, bAllowOverloading);
	pHierIdent->SetPrevHier(this);
	pHierIdent->SetIsLocal(IsLocal());
    return pHierIdent;
}

CHtfeIdent * CHtfeIdent::InsertIdent(const string &heirVarName, const string &flatVarName)
{
    if (m_pIdentTbl == 0) m_pIdentTbl = new CHtfeIdentTbl;
	CHtfeIdent *pHierIdent = m_pIdentTbl->Insert(heirVarName);
	if (pHierIdent->GetId() != id_new)
		return pHierIdent;

	pHierIdent->SetPrevHier(this);
	pHierIdent->SetIsLocal(IsLocal());

	CHtfeIdent * pFlatIdent = InsertFlatIdent(pHierIdent, flatVarName);

	pHierIdent->SetFlatIdent(pFlatIdent);

	return pHierIdent;
}

CHtfeIdent * CHtfeIdent::InsertFlatIdent(CHtfeIdent * pHierIdent, const string &flatVarName) { 
	// New variable, insert into flat identifier table
	if (m_pFlatTbl == 0) {
		if (IsFlatIdent())
			m_pFlatTbl = new CHtfeIdentTbl;
		else
			m_pFlatTbl = &m_pFlatIdent->GetFlatIdentTbl();
	}

	CHtfeIdent *pFlatIdent;
	int nextUniqueId;
	if (m_verilogKeywordTbl.Find(flatVarName, nextUniqueId)) {
		// name conflict, make name unique
		char buf[256];
		sprintf(buf, "%s$$%d", flatVarName.c_str(), nextUniqueId);
		pFlatIdent = m_pFlatTbl->Insert(buf);
	} else {
		pFlatIdent = m_pFlatTbl->Insert(flatVarName);
		if (pFlatIdent->GetId() != id_new) {
			// name conflict, make name unique
			char buf[256];
			sprintf(buf, "%s$$%d", flatVarName.c_str(), pFlatIdent->GetNextUniqueId());
			pFlatIdent = m_pFlatTbl->Insert(buf);
		}
	}
	Assert(pFlatIdent->GetId() == id_new);

	pFlatIdent->SetIsLocal(IsLocal());
 	pFlatIdent->SetIsFlatIdent();

	pFlatIdent->SetHierIdent(pHierIdent);
	pFlatIdent->SetType(pHierIdent->GetType());

	return pFlatIdent;
}

void CHtfeIdent::DeleteIdent(const string &name) {
	m_pIdentTbl->Delete(name);
}

CHtfeIdent * CHtfeIdent::FindIdent(const string &name) const {
	return m_pIdentTbl ? m_pIdentTbl->Find(name) : 0;
}

CHtfeIdent * CHtfeIdent::FindIdent(const char *pName) const {
	const string name(pName);
	return m_pIdentTbl ? m_pIdentTbl->Find(name) : 0;
}

CHtfeIdentTbl & CHtfeIdent::GetIdentTbl() {
    if (m_pIdentTbl == 0)
        m_pIdentTbl = new CHtfeIdentTbl;
    return *m_pIdentTbl;
}

CHtfeIdentTbl & CHtfeIdent::GetFlatIdentTbl() {
	if (m_pFlatTbl == 0) 
		m_pFlatTbl = new CHtfeIdentTbl;
	return *m_pFlatTbl;
}

void CHtfeIdent::ValidateIndexList(vector<CHtfeOperand *> *pIndexList) {
	if (pIndexList == 0) {
		Assert(m_dimenList.size() == 0);
		return;
	}
 	Assert(pIndexList->size() == m_dimenList.size());
	for (size_t idxNum = 0; idxNum < m_dimenList.size(); idxNum += 1) {
		Assert((*pIndexList)[idxNum]->IsConstValue());
		Assert((*pIndexList)[idxNum]->GetConstValue().GetSint64() < m_dimenList[idxNum]);
	}
}

void CHtfeIdent::ValidateIndexList(CHtfeOperand *pOperand) {
	if (pOperand->GetDimenCnt() == 0) {
		Assert(GetDimenCnt() == 0);
		return;
	}
 	Assert(pOperand->GetDimenCnt() == GetDimenCnt());
	for (size_t idxNum = 0; idxNum < GetDimenCnt(); idxNum += 1)
		Assert(pOperand->GetDimenIdx(idxNum) < (int64_t)GetDimenSize(idxNum));
}

CHtfeIdent * CHtfeIdent::SetWidth(int width) {
	m_width = width;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetWidth(width);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetWidth(width);
	return this;
}

int CHtfeIdent::GetWidth(CLineInfo lineInfo) const {
	CLineInfo info = lineInfo.m_lineNum == 0 ? m_lineInfo : lineInfo;
	if (m_width > 0)
		return m_width;
	else if (m_pType)
        return m_pType->GetWidth(info);
	else {
		ErrorMsg(PARSE_ERROR, info, "zero width type");
		ErrorMsg(PARSE_FATAL, info, "Previous errors prohibit further parsing");
		return 0;
	}
}

CHtfeIdent * CHtfeIdent::SetOpWidth(int width) {
	m_opWidth = width;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetOpWidth(width);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetOpWidth(width);
	return this;
}

void CHtfeIdent::SetIsRegister(bool bIsRegister) {
	m_bIsRegister = bIsRegister;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsRegister(bIsRegister);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetIsRegister(bIsRegister);
}

void CHtfeIdent::SetHtDistRamWeWidth(vector<CHtDistRamWeWidth> *pWeWidth) {
	m_pHtDistRamWeWidth = pWeWidth;
	if (IsHierIdent())
		m_pFlatIdent->SetHtDistRamWeWidth(pWeWidth);

	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetHtDistRamWeWidth(pWeWidth);
	}
}

void CHtfeIdent::SetIsReadOnly(bool bIsReadOnly) {
	m_bIsReadOnly = bIsReadOnly;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsReadOnly(bIsReadOnly);
	}
}

CHtfeIdent * CHtfeIdent::SetIsWriteOnly(bool bIsWriteOnly) {
	m_bIsWriteOnly = bIsWriteOnly;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsWriteOnly(bIsWriteOnly);
	}
	return this;
}

void CHtfeIdent::SetIsRemoveIfWriteOnly(bool bIsRemoveIfWriteOnly) {
	m_bIsRemoveIfWriteOnly = bIsRemoveIfWriteOnly;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsRemoveIfWriteOnly(bIsRemoveIfWriteOnly);
	}
}

void CHtfeIdent::SetPortDir(EPortDir portDir) {
	m_portDir = portDir;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetPortDir(portDir);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetPortDir(portDir);
}

void CHtfeIdent::SetIsParam(bool bIsParam) {
	m_bIsParam = bIsParam;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsParam(bIsParam);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetIsParam(bIsParam);
}

int CHtfeIdent::GetDimenElemIdx(CHtfeOperand * pOperand) {
	if (pOperand == 0)
		return -1;

	int elemIdx = 0;
	int idxMul = 1;
	vector<CHtfeOperand *> &indexList = pOperand->GetIndexList();
	for (size_t i = 0; i < m_dimenList.size(); i += 1) {
		if (!indexList[i]->IsConstValue())
			return -1;
		elemIdx += idxMul * indexList[i]->GetConstValue().GetSint32();
		idxMul *= m_dimenList[i];
	}
	return elemIdx;
}

int CHtfeIdent::GetDimenElemIdx(vector<int> const & refList)
{
	Assert(m_dimenList.size() == refList.size());
	int elemIdx = 0;
	int idxMul = 1;
	for (size_t i = 0; i < m_dimenList.size(); i += 1) {
		elemIdx += idxMul * refList[i];
		idxMul *= m_dimenList[i];
	}
	return elemIdx;
}

int CHtfeIdent::CalcElemIdx() {
	vector<int> &dimenList = (GetPrevHier() && GetPrevHier()->IsArrayIdent()) ? GetPrevHier()->GetDimenList() : GetDimenList();
	int elemIdx = 0;
	int idxMul = 1;
	for (size_t dimIdx = 0; dimIdx < dimenList.size(); dimIdx += 1) {
		elemIdx += idxMul * m_indexList[dimIdx];
		idxMul *= dimenList[dimIdx];
	}
	return elemIdx;
}

void CHtfeIdent::SetIsConst(bool bIsConst) {
	m_bIsConst = bIsConst;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsConst(bIsConst);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetIsConst(bIsConst);
}

void CHtfeIdent::SetIsReference(bool bIsReference) {
	m_bIsReference = bIsReference;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsReference(bIsReference);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetIsReference(bIsReference);
}

void CHtfeIdent::SetIsNoLoad(bool bIsHtNoLoad) {
	m_bIsNoLoad = bIsHtNoLoad;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsNoLoad(bIsHtNoLoad);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetIsNoLoad(bIsHtNoLoad);
}

void CHtfeIdent::SetIsScPrim(bool bIsHtPrim) {
	m_bIsScPrim = bIsHtPrim;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsScPrim(bIsHtPrim);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetIsScPrim(bIsHtPrim);
}

void CHtfeIdent::SetIsScLocal(bool bIsHtLocal) {
	m_bIsScLocal = bIsHtLocal;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsScLocal(bIsHtLocal);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetIsScLocal(bIsHtLocal);
}

//void CHtfeIdent::SetIsHtPrimOutput(bool bIsScPrimOutput) {
//	m_bIsScPrimOutput |= bIsScPrimOutput;
//	if (m_dimenList.size() > 0) {
//		CHtfeIdentTblIter iter(GetIdentTbl());
//		for (iter.Begin(); !iter.End(); iter++)
//			iter->SetIsHtPrimOutput(bIsScPrimOutput);
//	}
//	if (m_pFlatIdent) m_pFlatIdent->SetIsHtPrimOutput(bIsScPrimOutput);
//}

void CHtfeIdent::SetHtPrimClkList(vector<string> const & htPrimClkList) {
	m_htPrimClkList = htPrimClkList;
	m_bIsScClockedPrim = htPrimClkList.size() > 0;

	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetHtPrimClkList(htPrimClkList);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetHtPrimClkList(htPrimClkList);
}

CHtfeIdent * CHtfeIdent::InsertInstancePort(string &name) {
    if (m_pInstancePortTbl == 0) 
		m_pInstancePortTbl = new CHtfeIdentTbl;
    return m_pInstancePortTbl->Insert(name);
}

void CHtfeIdent::InsertMemInitStatements(CHtfeStatement *pStatement) {
	if (pStatement == 0)
		return;

	Assert(IsFunction());

	// insert statements at end of existing list
	CHtfeStatement **ppTail = &pStatement;
	while (*ppTail)
		ppTail = (*ppTail)->GetPNext();

	if (m_pMemInitStatementHead == 0)
		m_ppMemInitStatementTail = &m_pMemInitStatementHead;

	*m_ppMemInitStatementTail = pStatement;
	m_ppMemInitStatementTail = ppTail;
}

void CHtfeIdent::InsertStatementList(CHtfeStatement *pStatement) {
	if (pStatement == 0)
		return;

	Assert(IsFunction());
	// insert statements after previously inserted statements, but before appended statements

	CHtfeStatement **ppTail = &pStatement;
	while (*ppTail)
		ppTail = (*ppTail)->GetPNext();

	if (m_pStatementHead == 0) {
		m_ppStatementAppendTail = &m_pStatementHead;
		m_ppStatementInsertTail = &m_pStatementHead;
	}

	if (m_ppStatementAppendTail == m_ppStatementInsertTail) {
		// Only insert so far
		*m_ppStatementAppendTail = pStatement;
		*m_ppStatementInsertTail = pStatement;

		m_ppStatementAppendTail = ppTail;
		m_ppStatementInsertTail = ppTail;
	} else {
		*ppTail = *m_ppStatementInsertTail;
		*m_ppStatementInsertTail = pStatement;
		m_ppStatementInsertTail = ppTail;
	}
	Assert(*m_ppStatementAppendTail == 0);
}

void CHtfeIdent::AppendStatementList(CHtfeStatement *pStatement) {
	// append statements to end of list
	CHtfeStatement **ppTail = &pStatement;
	while (*ppTail)
		ppTail = (*ppTail)->GetPNext();

	if (m_pStatementHead == 0) {
		m_ppStatementAppendTail = &m_pStatementHead;
		m_ppStatementInsertTail = &m_pStatementHead;
	}

	*m_ppStatementAppendTail = pStatement;
	m_ppStatementAppendTail = ppTail;
}

void CHtfeIdent::SetIsHtDistQue(bool bIsHtDistQue) {
	m_bIsHtDistQue = bIsHtDistQue;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsHtDistQue(bIsHtDistQue);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetIsHtDistQue(bIsHtDistQue);
}

void CHtfeIdent::SetIsHtBlockQue(bool bIsHtBlockQue) {
	m_bIsHtBlockQue = bIsHtBlockQue;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsHtBlockQue(bIsHtBlockQue);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetIsHtBlockQue(bIsHtBlockQue);
}

void CHtfeIdent::SetIsIgnoreSignalCheck(bool bIsIgnoreSignalCheck) {
	m_bIsIgnoreSignalCheck = bIsIgnoreSignalCheck;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsIgnoreSignalCheck(bIsIgnoreSignalCheck);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetIsIgnoreSignalCheck(bIsIgnoreSignalCheck);
}

void CHtfeIdent::SetIsHtDistRam(bool bIsHtDistRam) {
	m_bIsHtDistRam = bIsHtDistRam;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsHtDistRam(bIsHtDistRam);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetIsHtDistRam(bIsHtDistRam);
}

void CHtfeIdent::SetIsHtBlockRam(bool bIsHtBlockRam) {
	m_bIsHtBlockRam = bIsHtBlockRam;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsHtBlockRam(bIsHtBlockRam);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetIsHtBlockRam(bIsHtBlockRam);
}

void CHtfeIdent::SetIsHtMrdBlockRam(bool bIsHtMrdBlockRam) {
	m_bIsHtMrdBlockRam = bIsHtMrdBlockRam;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsHtMrdBlockRam(bIsHtMrdBlockRam);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetIsHtMrdBlockRam(bIsHtMrdBlockRam);
}

void CHtfeIdent::SetIsHtMwrBlockRam(bool bIsHtMwrBlockRam) {
	m_bIsHtMwrBlockRam = bIsHtMwrBlockRam;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsHtMwrBlockRam(bIsHtMwrBlockRam);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetIsHtMwrBlockRam(bIsHtMwrBlockRam);
}

void CHtfeIdent::SetIsHtBlockRamDoReg(bool bIsHtBlockRam) {
	m_bIsHtBlockRam = bIsHtBlockRam;
	m_bIsHtBlockRamDoReg = true;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetIsHtBlockRamDoReg(bIsHtBlockRam);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetIsHtBlockRam(bIsHtBlockRam);
}

void CHtfeIdent::SetHtMemorySelWidth(int selWidth) {
	m_scMemorySelWidth = selWidth;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetHtMemorySelWidth(selWidth);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetHtMemorySelWidth(selWidth);
}

void CHtfeIdent::SetHtMemoryAddrWidth1(int addrWidth1) {
	m_scMemoryAddrWidth1 = addrWidth1;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetHtMemoryAddrWidth1(addrWidth1);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetHtMemoryAddrWidth1(addrWidth1);
}

void CHtfeIdent::SetHtMemoryAddrWidth2(int addrWidth2) {
	m_scMemoryAddrWidth2 = addrWidth2;
	if (m_dimenList.size() > 0) {
		CHtfeIdentTblIter iter(GetIdentTbl());
		for (iter.Begin(); !iter.End(); iter++)
			iter->SetHtMemoryAddrWidth2(addrWidth2);
	}
	if (m_pFlatIdent) m_pFlatIdent->SetHtMemoryAddrWidth2(addrWidth2);
}

void CHtfeIdent::AddGlobalRef(CHtIdentElem &identElem) {
    for (unsigned int i = 0; i < m_globalRefSet.size(); i += 1)
        if (m_globalRefSet[i].Contains(identElem)) {
			if (identElem.m_elemIdx == -1)
				m_globalRefSet[i].m_elemIdx = -1;
            return;
		}

    m_globalRefSet.push_back(identElem);
}

void CHtfeIdent::InsertArrayElements(int structBitPos)
{
	// Enter array members into table
	vector<int>	iterList;
	for (size_t i = 0; i < m_dimenList.size(); i += 1)
		iterList.push_back(0);

	size_t i;
	do {
		string memberName = GetName();

		char idxStr[16];
		for (i = 0; i < m_dimenList.size(); i += 1) {
			sprintf(idxStr, "%d", iterList[i]);
			memberName.append("[").append(idxStr).append("]");
		}

		CHtfeIdent *pMemberElem = InsertIdent(memberName, false);

		// set flags appropriate for a member
		if (structBitPos >= 0) {
			pMemberElem->SetStructPos(structBitPos);
			structBitPos += GetWidth();
		}
		pMemberElem->SetType(GetType());


		for (i = 0; i < m_dimenList.size(); i += 1) {
			iterList[i] += 1;
			if (iterList[i] == m_dimenList[i])
				iterList[i] = 0;
			else
				break;
		}
	} while (i < m_dimenList.size());
}

void CHtfeIdent::SetSeqState(ESeqState seqState, CHtfeOperand *pOp)
{
	if (GetPrevHier() && GetPrevHier()->IsArrayIdent()) { GetPrevHier()->SetSeqState(seqState, pOp); return; }
	if (!IsFlatIdent()) { m_pFlatIdent->SetSeqState(seqState, pOp); return; }

	if (m_dimenList.size() > 0)
		Assert(pOp && pOp->GetDimenCnt() == m_dimenList.size());
	if (m_refInfo.size() == 0)
		m_refInfo.resize(GetDimenElemCnt());

	if (m_dimenList.size() == 0 || pOp == 0)
		m_refInfo[0].m_seqState = seqState;
	else if (pOp == 0) {
		Assert(0);
		for (size_t idx = 0; idx < m_refInfo.size(); idx += 1)
			m_refInfo[idx].m_seqState = seqState;
	} else {
		vector<bool> constList(m_dimenList.size());
		vector<int> refList(m_dimenList.size());

		for (size_t dimIdx = 0; dimIdx < m_dimenList.size(); dimIdx += 1) {
			constList[dimIdx] = pOp->GetIndexList()[dimIdx]->IsConstValue();
			refList[dimIdx] = constList[dimIdx] ? pOp->GetIndexList()[dimIdx]->GetConstValue().GetSint32() : 0;
		}

		do {
			int idx = GetDimenElemIdx(refList);
			//for (size_t dimIdx = 0; dimIdx < m_dimenList.size(); dimIdx += 1) {
			//	idx *= m_dimenList[dimIdx];
			//	idx += refList[dimIdx];
			//}

			Assert(idx < (int)m_refInfo.size());
			m_refInfo[idx].m_seqState = seqState;
		} while(!DimenIter(refList, constList));
	}
}

ESeqState CHtfeIdent::GetSeqState(CHtfeOperand *pOp)
{
	if (GetPrevHier() && GetPrevHier()->IsArrayIdent())
		return GetPrevHier()->GetSeqState(pOp);

	if (!IsFlatIdent())
		return m_pFlatIdent->GetSeqState(pOp);

	if (m_dimenList.size() > 0)
		Assert(pOp && pOp->GetDimenCnt() == m_dimenList.size());

	Assert((int)m_refInfo.size() == GetDimenElemCnt());

	if (m_dimenList.size() == 0)
		return m_refInfo[0].m_seqState;
	else if (pOp == 0) {
		Assert(0);
		return ss_Unknown;
	} else {
		vector<bool> constList(m_dimenList.size());
		vector<int> refList(m_dimenList.size());

		for (size_t dimIdx = 0; dimIdx < m_dimenList.size(); dimIdx += 1) {
			constList[dimIdx] = pOp->GetIndexList()[dimIdx]->IsConstValue();
			refList[dimIdx] = constList[dimIdx] ? pOp->GetIndexList()[dimIdx]->GetConstValue().GetSint32() : 0;
		}

		bool bFirst = true;
		ESeqState seqState;
		do {
			int idx = GetDimenElemIdx(refList);
			//for (size_t dimIdx = 0; dimIdx < m_dimenList.size(); dimIdx += 1) {
			//	idx *= m_dimenList[dimIdx];
			//	idx += refList[dimIdx];
			//}

			Assert(idx < (int)m_refInfo.size());
			if (bFirst)
				seqState = m_refInfo[idx].m_seqState;

			else if (m_refInfo[idx].m_seqState != seqState) {
				ErrorMsg(PARSE_ERROR, pOp->GetLineInfo(), "Inconsistent sequence state amoung elements of array '%s'\n", GetName().c_str());
				break;
			}
		} while(!DimenIter(refList, constList));

		return seqState;
	}
}

void CHtfeIdent::SetIsAssignOutput(CHtfeOperand *pOp)
{
	if (GetPrevHier() && GetPrevHier()->IsArrayIdent()) { GetPrevHier()->SetIsAssignOutput(pOp); return; }
	if (!IsFlatIdent()) { m_pFlatIdent->SetIsAssignOutput(pOp); return; }

	if (m_dimenList.size() > 0)
		Assert(pOp && pOp->GetDimenCnt() == m_dimenList.size());
	if (m_refInfo.size() == 0)
		m_refInfo.resize(GetDimenElemCnt());

	if (m_dimenList.size() == 0)
		m_refInfo[0].m_bIsAssignOutput = true;
	else if (pOp == 0) {
		Assert(0);
		for (size_t idx = 0; idx < m_refInfo.size(); idx += 1)
			m_refInfo[idx].m_bIsAssignOutput = true;
	} else {
		vector<bool> constList(m_dimenList.size());
		vector<int> refList(m_dimenList.size());

		for (size_t dimIdx = 0; dimIdx < m_dimenList.size(); dimIdx += 1) {
			constList[dimIdx] = pOp->GetIndexList()[dimIdx]->IsConstValue();
			refList[dimIdx] = constList[dimIdx] ? pOp->GetIndexList()[dimIdx]->GetConstValue().GetSint32() : 0;
		}

		do {
			int idx = GetDimenElemIdx(refList);
			//for (size_t dimIdx = 0; dimIdx < m_dimenList.size(); dimIdx += 1) {
			//	idx *= m_dimenList[dimIdx];
			//	idx += refList[dimIdx];
			//}

			if (idx < (int)m_refInfo.size()) // can be out of range
				m_refInfo[idx].m_bIsAssignOutput = true;
		} while(!DimenIter(refList, constList));
	}
}

void CHtfeIdent::SetIsHtPrimOutput(CHtfeOperand *pOp)
{
	if (GetPrevHier() && GetPrevHier()->IsArrayIdent()) { GetPrevHier()->SetIsHtPrimOutput(pOp); return; }
	if (!IsFlatIdent()) { m_pFlatIdent->SetIsHtPrimOutput(pOp); return; }

	if (pOp && m_dimenList.size() > 0)
		Assert(pOp && pOp->GetDimenCnt() == m_dimenList.size());
	if (m_refInfo.size() == 0)
		m_refInfo.resize(GetDimenElemCnt());

	if (pOp == 0) {
		for (size_t idx = 0; idx < m_refInfo.size(); idx += 1)
			m_refInfo[idx].m_bIsHtPrimOutput = true;
	} else if (m_dimenList.size() == 0) {
		m_refInfo[0].m_bIsHtPrimOutput = true;
	} else {
		vector<bool> constList(m_dimenList.size());
		vector<int> refList(m_dimenList.size());

		for (size_t dimIdx = 0; dimIdx < m_dimenList.size(); dimIdx += 1) {
			constList[dimIdx] = pOp->GetIndexList()[dimIdx]->IsConstValue();
			refList[dimIdx] = constList[dimIdx] ? pOp->GetIndexList()[dimIdx]->GetConstValue().GetSint32() : 0;
		}

		do {
			int idx = GetDimenElemIdx(refList);
			//for (size_t dimIdx = 0; dimIdx < m_dimenList.size(); dimIdx += 1) {
			//	idx *= m_dimenList[dimIdx];
			//	idx += refList[dimIdx];
			//}

			if (idx < (int)m_refInfo.size()) // can be out of range
				m_refInfo[idx].m_bIsHtPrimOutput = true;
		} while(!DimenIter(refList, constList));
	}
}

void CHtfeIdent::SetReadRef(CHtfeOperand *pOp)
{
	if (GetPrevHier() && GetPrevHier()->IsArrayIdent()) { GetPrevHier()->SetReadRef(pOp); return; }
	if (!IsFlatIdent()) { m_pFlatIdent->SetReadRef(pOp); return; }

	if (m_dimenList.size() > 0)
		Assert(pOp && pOp->GetDimenCnt() == m_dimenList.size());
	if (m_refInfo.size() == 0)
		m_refInfo.resize(GetDimenElemCnt());

	if (m_dimenList.size() == 0)
		m_refInfo[0].m_bIsRead = true;
	else if (pOp == 0) {
		for (size_t idx = 0; idx < m_refInfo.size(); idx += 1)
			m_refInfo[idx].m_bIsRead = true;
	} else {
		vector<bool> constList(m_dimenList.size());
		vector<int> refList(m_dimenList.size());

		for (size_t dimIdx = 0; dimIdx < m_dimenList.size(); dimIdx += 1) {
			constList[dimIdx] = pOp->GetIndexList()[dimIdx]->IsConstValue();
			refList[dimIdx] = constList[dimIdx] ? pOp->GetIndexList()[dimIdx]->GetConstValue().GetSint32() : 0;
		}

		do {
			int idx = GetDimenElemIdx(refList);
			//for (size_t dimIdx = 0; dimIdx < m_dimenList.size(); dimIdx += 1) {
			//	idx *= m_dimenList[dimIdx];
			//	idx += refList[dimIdx];
			//}

			if (idx < (int)m_refInfo.size()) // can be out of range
				m_refInfo[idx].m_bIsRead = true;
		} while(!DimenIter(refList, constList));
	}
}

void CHtfeIdent::SetWriteRef(CHtfeOperand *pOp)
{
	if (GetPrevHier() && GetPrevHier()->IsArrayIdent()) { GetPrevHier()->SetWriteRef(pOp); return; }
	if (!IsFlatIdent()) { m_pFlatIdent->SetWriteRef(pOp); return; }

	if (m_dimenList.size() > 0)
		Assert(pOp && pOp->GetDimenCnt() == m_dimenList.size());
	if (m_refInfo.size() == 0)
		m_refInfo.resize(GetDimenElemCnt());

	if (m_dimenList.size() == 0)
		m_refInfo[0].m_bIsWritten = true;
	else if (pOp == 0) {
		for (size_t idx = 0; idx < m_refInfo.size(); idx += 1)
			m_refInfo[idx].m_bIsWritten = true;
	} else {
		vector<bool> constList(m_dimenList.size());
		vector<int> refList(m_dimenList.size());

		for (size_t dimIdx = 0; dimIdx < m_dimenList.size(); dimIdx += 1) {
			constList[dimIdx] = pOp->GetIndexList()[dimIdx]->IsConstValue();
			refList[dimIdx] = constList[dimIdx] ? pOp->GetIndexList()[dimIdx]->GetConstValue().GetSint32() : 0;
		}

		do {
			int idx = GetDimenElemIdx(refList);
			//for (size_t dimIdx = 0; dimIdx < m_dimenList.size(); dimIdx += 1) {
			//	idx *= m_dimenList[dimIdx];
			//	idx += refList[dimIdx];
			//}

			if (idx < (int)m_refInfo.size())    // can be out of range
				m_refInfo[idx].m_bIsWritten = true;
		} while(!DimenIter(refList, constList));
	}
}

void CHtfeIdent::MergeGlobalRefSet(vector<CHtIdentElem> &refSet)
{
    // merge the passed in reference set into the identifier's ref set, eliminate common references
    for (size_t ieIdx1 = 0; ieIdx1 < refSet.size(); ieIdx1 += 1) {
        CHtIdentElem & elem1 = refSet[ieIdx1];

        size_t ieIdx2;
        for (ieIdx2 = 0; ieIdx2 < m_globalRefSet.size(); ieIdx2 += 1) {
            CHtIdentElem & elem2 = m_globalRefSet[ieIdx2];

            if (elem1 == elem2)
                break;
        }

        if (ieIdx2 == m_globalRefSet.size())
            m_globalRefSet.push_back(elem1);
    }
}

bool CHtfeIdent::DimenIter(vector<int> &refList)
{
	// refList must be same size as dimenList and all elements zero
	Assert(m_dimenList.size() == refList.size());

	bool bDone;

	if (m_dimenList.size() > 0) {
		bDone = false;
		refList[0] += 1;
		for (size_t i = 0; i < m_dimenList.size(); i += 1) {
			if (refList[i] < m_dimenList[i])
				break;
			else {
				refList[i] = 0;
				if (i+1 < m_dimenList.size())
					refList[i+1] += 1;
				else
					bDone = true;
			}
		}
	} else
		bDone = true;

	return bDone;
}

bool CHtfeIdent::DimenIter(vector<int> &refList, vector<bool> &constList)
{
	// refList must be same size as dimenList and all elements zero
	//Assert(m_dimenList.size() == refList.size());

	bool bDone;

	if (refList.size() > 0) {
		bDone = false;
		int i;
		for (i = m_dimenList.size()-1; i >= 0; i -= 1) {
			if (!constList[i])
				break;
		}

		if (i < 0)
			return true;

		refList[i] += 1;
		while (i >= 0) {
			if (refList[i] < m_dimenList[i])
				break;
			else {
				refList[i] = 0;
				for (i -= 1; i >= 0; i -= 1)
					if (!constList[i])
						break;

				if (i >= 0)
					refList[i] += 1;
				else
					bDone = true;
			}
		}
	} else
		bDone = true;

	return bDone;
}

CHtfeIdent * CHtfeIdentTbl::Insert(const string &str, bool bAllowOverloading)
{
    IdentTblMap_InsertPair insertPair;
    insertPair = m_identTblMap.insert(IdentTblMap_ValuePair(str, (CHtfeIdent *)0));
	if (insertPair.first->second == 0) {
		insertPair.first->second = CHtfeDesign::NewIdent();
		insertPair.first->second->SetName(str);
	} else if (bAllowOverloading) {
		CHtfeIdent * pIdent = CHtfeDesign::NewIdent();
		pIdent->SetName(str);
		pIdent->SetOverloadedNext(insertPair.first->second);
		insertPair.first->second = pIdent;
	}
    return insertPair.first->second;
}
