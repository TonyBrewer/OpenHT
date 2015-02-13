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
#include "HtfeArgs.h"
#include "HtfeDesign.h"

// Parse ht_dist_que declaration
CHtfeIdent * CHtfeDesign::ParseHtQueueDecl(CHtfeIdent *pHier)
{
	bool bBlockRam = GetString() == "sc_block_que" || GetString() == "ht_block_que";

	if (GetNextToken() != tk_less) {
		ParseMsg(PARSE_ERROR, "expected ht_dist_que<type, depth>");
		return 0;
	}
	GetNextToken();

	CHtfeTypeAttrib typeAttrib;
	CHtfeIdent *pBaseType = ParseTypeDecl(pHier, typeAttrib);

	if (typeAttrib.m_bIsStatic)
		ParseMsg(PARSE_ERROR, "static type specifier not supported");

	if (pBaseType == 0) {
		ParseMsg(PARSE_ERROR, "expected a type (ht_block_que<type, depth>)");
		SkipTo(tk_semicolon);
		return 0;
	}

	if (GetToken() != tk_comma) {
		ParseMsg(PARSE_ERROR, "expected ht_dist_que<type, depth>");
		return 0;
	}

	GetNextToken();
	CConstValue addrWidth1;
	if (!ParseConstExpr(addrWidth1) || GetToken() != tk_greater) {
		ParseMsg(PARSE_ERROR, "expected ht_dist_que<type, depth>");
		return 0;
	}

	if (addrWidth1.GetSint64() > 16) {
		ParseMsg(PARSE_ERROR, "expected ht_dist_que addrBits <= 16");
		addrWidth1 = 5;
	}

	char typeName[256];
	sprintf(typeName, "ht_dist_que<%s,%d>", pBaseType->GetName().c_str(), (int)addrWidth1.GetSint64());

	GetNextToken();

	// now insert unique ht_dist_que type
	CHtfeIdent *pType = pHier->InsertType(typeName);

	if (pType->GetId() == CHtfeIdent::id_new) {
		pType->SetId(CHtfeIdent::id_class);
		pType->SetType(pBaseType);
		pType->SetWidth(pBaseType->GetWidth());

		if (bBlockRam)
			pType->SetIsHtBlockQue();
		else
			pType->SetIsHtDistQue();

		pType->SetHtMemoryAddrWidth1((int)addrWidth1.GetSint64());
	}

	return pType;
}

// Parse ht_dist_que declaration
CHtfeIdent * CHtfeDesign::ParseHtDistRamDecl(CHtfeIdent *pHier)
{
	if (GetNextToken() != tk_less) {
		ParseMsg(PARSE_ERROR, "expected a < (ht_dist_ram<type, AW1, AW2=0>)");
		return 0;
	}
	GetNextToken();

	CHtfeTypeAttrib typeAttrib;
	CHtfeIdent *pBaseType = ParseTypeDecl(pHier, typeAttrib);

	if (typeAttrib.m_bIsStatic)
		ParseMsg(PARSE_ERROR, "static type specifier not supported");

	if (pBaseType == 0) {
		ParseMsg(PARSE_ERROR, "expected a type (ht_block_ram<type, AW1, AW2=0>)");
		SkipTo(tk_semicolon);
		return 0;
	}

	if (GetToken() != tk_comma) {
		ParseMsg(PARSE_ERROR, "expected a comma (ht_dist_ram<type, AW1, AW2=0>)");
		return 0;
	}

	GetNextToken();
	CConstValue addrWidth1;
	if (!ParseConstExpr(addrWidth1)) {
		ParseMsg(PARSE_ERROR, "expected a constant for AW1 (ht_dist_ram<type, AW1, AW2=0>)");
		return 0;
	}

	CConstValue addrWidth2(0);
	if (GetToken() == tk_comma) {
		GetNextToken();

		if (!ParseConstExpr(addrWidth2)) {
			ParseMsg(PARSE_ERROR, "expected a constant for AW2 (ht_dist_ram<type, AW1, AW2=0>)");
			return 0;
		}
	}

	if (GetToken() != tk_greater) {
		ParseMsg(PARSE_ERROR, "expected a > (ht_dist_ram<type, AW1, AW2=0>)");
		return 0;
	}
	GetNextToken();

	if (addrWidth1.GetSint64() + addrWidth2.GetSint64() > 10) {
		ParseMsg(PARSE_ERROR, "expected ht_dist_ram AW1 + AW2 <= 10");
		addrWidth1 = CConstValue(5);
		addrWidth2 = CConstValue(0);
	}

	char typeName[256];
	sprintf(typeName, "ht_dist_ram<%s,%d,%d>", pBaseType->GetName().c_str(), (int)addrWidth1.GetSint64(), (int)addrWidth2.GetSint64());

	// now insert unique sc_dist_ram type
	CHtfeIdent *pType = pHier->InsertType(typeName);

	if (pType->GetId() == CHtfeIdent::id_new) {
		pType->SetId(CHtfeIdent::id_class);
		pType->SetType(pBaseType);
		pType->SetWidth(pBaseType->GetWidth());
		pType->SetIsHtDistRam();
		pType->SetHtMemoryAddrWidth1((int)addrWidth1.GetSint64());
		pType->SetHtMemoryAddrWidth2((int)addrWidth2.GetSint64());
	}

	return pType;
}

// Parse ht_block_ram declaration
CHtfeIdent * CHtfeDesign::ParseHtBlockRamDecl(CHtfeIdent *pHier)
{
	if (GetNextToken() != tk_less) {
		ParseMsg(PARSE_ERROR, "expected a < (ht_block_ram<type, AW1, AW2=0, bDoReg=false>)");
		return 0;
	}
	GetNextToken();

	CHtfeTypeAttrib typeAttrib;
	CHtfeIdent *pBaseType = ParseTypeDecl(pHier, typeAttrib);

	if (typeAttrib.m_bIsStatic)
		ParseMsg(PARSE_ERROR, "static type specifier not supported");

	if (pBaseType == 0) {
		ParseMsg(PARSE_ERROR, "expected a type (ht_block_ram<type, AW1, AW2=0, bDoReg=false>)");
		SkipTo(tk_semicolon);
		return 0;
	}

	if (GetToken() != tk_comma) {
		ParseMsg(PARSE_ERROR, "expected a comma (ht_block_ram<type, AW1, AW2=0, bDoReg=false>)");
		return 0;
	}

	GetNextToken();
	CConstValue addrWidth1;
	if (!ParseConstExpr(addrWidth1)) {
		ParseMsg(PARSE_ERROR, "expected a constant for AW1 (ht_block_ram<type, AW1, AW2=0, bDoReg=false>)");
		return 0;
	}

	CConstValue addrWidth2(0);
	if (GetToken() == tk_comma) {
		GetNextToken();

		if (!ParseConstExpr(addrWidth2)) {
			ParseMsg(PARSE_ERROR, "expected a constant for AW2 (ht_block_ram<type, AW1, AW2=0, bDoReg=false>)");
			return 0;
		}
	}

	bool bDoReg = false;
	if (GetToken() == tk_comma) {
		if (GetNextToken() != tk_identifier || GetString() != "true" && GetString() != "false") {
			ParseMsg(PARSE_ERROR, "expected a constant for AW2 (ht_block_ram<type, AW1, AW2=0, bDoReg=false>)");
			return 0;
		}

		bDoReg = GetString() == "true";

		GetNextToken();
	}

	if (GetToken() != tk_greater) {
		ParseMsg(PARSE_ERROR, "expected a > (ht_block_ram<type, AW1, AW2=0, bDoReg=false>)");
		return 0;
	}
	GetNextToken();

	if (addrWidth1.GetSint64() + addrWidth2.GetSint64() > 20) {
		ParseMsg(PARSE_ERROR, "expected ht_block_ram AW1 + AW2 <= 20");
		addrWidth1 = CConstValue(5);
		addrWidth2 = CConstValue(0);
	}

	char typeName[256];
	sprintf(typeName, "ht_block_ram<%s,%d,%d,%s>", pBaseType->GetName().c_str(), (int)addrWidth1.GetSint64(), (int)addrWidth2.GetSint64(),
		bDoReg ? "true" : "false");

	// now insert unique ht_block_ram type
	CHtfeIdent *pType = pHier->InsertType(typeName);

	if (pType->GetId() == CHtfeIdent::id_new) {
		pType->SetId(CHtfeIdent::id_class);
		pType->SetType(pBaseType);
		pType->SetWidth(pBaseType->GetWidth());
		pType->SetIsHtBlockRam();
		pType->SetHtMemoryAddrWidth1((int)addrWidth1.GetSint64());
		pType->SetHtMemoryAddrWidth2((int)addrWidth2.GetSint64());

		if (bDoReg)
			pType->SetIsHtBlockRamDoReg();
	}

	return pType;
}

// Parse ht_block_ram declaration
CHtfeIdent * CHtfeDesign::ParseHtAsymBlockRamDecl(CHtfeIdent *pHier, bool bMultiRead)
{
	char const * pSyntax = bMultiRead ?
		"ht_mrd_block_ram<type, SW, AW1, AW2=0, bDoReg=false>" : "ht_mwr_block_ram<type, SW, AW1, AW2=0, bDoReg=false>";

	if (GetNextToken() != tk_less) {
		ParseMsg(PARSE_ERROR, "expected a < (%s)", pSyntax);
		return 0;
	}
	GetNextToken();

	CHtfeTypeAttrib typeAttrib;
	CHtfeIdent *pBaseType = ParseTypeDecl(pHier, typeAttrib);

	if (typeAttrib.m_bIsStatic)
		ParseMsg(PARSE_ERROR, "static type specifier not supported");

	if (pBaseType == 0) {
		ParseMsg(PARSE_ERROR, "expected a type (%s)", pSyntax);
		SkipTo(tk_semicolon);
		return 0;
	}

	if (GetToken() != tk_comma) {
		ParseMsg(PARSE_ERROR, "expected a comma (%s)", pSyntax);
		return 0;
	}
	GetNextToken();

	CConstValue selWidth;
	if (!ParseConstExpr(selWidth)) {
		ParseMsg(PARSE_ERROR, "expected a constant for SW (%s)", pSyntax);
		return 0;
	}

	if (GetToken() != tk_comma) {
		ParseMsg(PARSE_ERROR, "expected a comma (%s)", pSyntax);
		return 0;
	}
	GetNextToken();

	CConstValue addrWidth1;
	if (!ParseConstExpr(addrWidth1)) {
		ParseMsg(PARSE_ERROR, "expected a constant for AW1 (%s)", pSyntax);
		return 0;
	}

	CConstValue addrWidth2(0);
	if (GetToken() == tk_comma) {
		GetNextToken();

		if (!ParseConstExpr(addrWidth2)) {
			ParseMsg(PARSE_ERROR, "expected a constant for AW2 (%s)", pSyntax);
			return 0;
		}
	}

	bool bDoReg = false;
	if (GetToken() == tk_comma) {
		if (GetNextToken() != tk_identifier || GetString() != "true" && GetString() != "false") {
			ParseMsg(PARSE_ERROR, "expected a true or false for bDoReg (%s)", pSyntax);
			return 0;
		}

		bDoReg = GetString() == "true";

		GetNextToken();
	}

	if (GetToken() != tk_greater) {
		ParseMsg(PARSE_ERROR, "expected a > (ht_block_ram<type, AW1, AW2=0, bDoReg=false>)");
		return 0;
	}
	GetNextToken();

	if (addrWidth1.GetSint64() + addrWidth2.GetSint64() > 20) {
		ParseMsg(PARSE_ERROR, "expected ht_%s_block_ram AW1 + AW2 <= 20", bMultiRead ? "mrd" : "mwr");
		addrWidth1 = CConstValue(8);
		addrWidth2 = CConstValue(0);
	}

	char typeName[256];
	sprintf(typeName, "ht_%s_block_ram<%s,%d,%d,%s>",
		bMultiRead ? "mrd" : "mwr",
		pBaseType->GetName().c_str(),
		(int)addrWidth1.GetSint64(),
		(int)addrWidth2.GetSint64(),
		bDoReg ? "true" : "false");

	// now insert unique ht_asym_block_ram type
	CHtfeIdent *pType = pHier->InsertType(typeName);

	if (pType->GetId() == CHtfeIdent::id_new) {
		pType->SetId(CHtfeIdent::id_class);
		pType->SetType(pBaseType);
		pType->SetWidth(pBaseType->GetWidth());

		if (bMultiRead)
			pType->SetIsHtMrdBlockRam();
		else
			pType->SetIsHtMwrBlockRam();

		pType->SetHtMemoryAddrWidth1((int)addrWidth1.GetSint64());
		pType->SetHtMemoryAddrWidth2((int)addrWidth2.GetSint64());
		pType->SetHtMemorySelWidth((int)selWidth.GetSint64());

		if (bDoReg)
			pType->SetIsHtBlockRamDoReg();
	}

	return pType;
}

void CHtfeDesign::HtQueueVarDecl(CHtfeIdent *pHier, CHtfeIdent *pIdent)
{
	// An HtQueue was declared. Insert required registers for queue (r_rdIdx, r_wrIdx)
	string cQueName = pIdent->GetName();
	if (cQueName.substr(0,2) == "r_" || cQueName.substr(0,2) == "m_")
		cQueName.replace(0, 2, "c_");
	else
		cQueName.insert(0, "c_");

	string rQueName = pIdent->GetName();
	if (rQueName.substr(0,2) == "m_")
		rQueName.replace(0, 2, "r_");

	string dimStr;
	if (pIdent->GetDimenCnt() > 0) {
		// an array of ht_dist_que's
		char dimIdx[16];
		for (size_t i = 0; i < pIdent->GetDimenCnt(); i += 1) {
			sprintf(dimIdx, "%d", pIdent->GetDimenList()[i]);
			dimStr.append("[").append(dimIdx).append("]");
		}
	}

	int idxWidth = pIdent->GetType()->GetHtMemoryAddrWidth1()+pIdent->GetType()->GetHtMemoryAddrWidth2()+1;

	char buf[4096];
	if (pIdent->IsHtBlockQue())
		sprintf(buf, "{ sc_uint<%d> %s_WrAddr%s; sc_uint<%d> ht_noload %s_WrAddr2%s; sc_uint<%d> %s_RdAddr%s; %s ht_noload %s_RdData%s; sc_uint<%d> %s_Cnt%s; %s %s_WrData%s; bool %s_WrEn%s; sc_uint<%d> %s_WrAddr%s; sc_uint<%d> %s_RdAddr%s; }",
			idxWidth, rQueName.c_str(), dimStr.c_str(),
			idxWidth, rQueName.c_str(), dimStr.c_str(), 
			idxWidth, rQueName.c_str(), dimStr.c_str(),
			pIdent->GetType()->GetType()->GetName().c_str(), cQueName.c_str(), dimStr.c_str(),
			idxWidth, cQueName.c_str(), dimStr.c_str(),
			pIdent->GetType()->GetType()->GetName().c_str(), cQueName.c_str(), dimStr.c_str(),
			cQueName.c_str(), dimStr.c_str(),
			idxWidth, cQueName.c_str(), dimStr.c_str(),
			idxWidth, cQueName.c_str(), dimStr.c_str() );
	else
		// declare variables needed for ht_dist_que push, this is done once per array of ht_dist_que variables
		sprintf(buf, "{ sc_uint<%d> %s_WrAddr%s; sc_uint<%d> %s_RdAddr%s; %s ht_noload %s_RdData%s; sc_uint<%d> %s_Cnt%s; %s %s_WrData%s; bool %s_WrEn%s; sc_uint<%d> %s_WrAddr%s; sc_uint<%d> %s_RdAddr%s; }",
			idxWidth, rQueName.c_str(), dimStr.c_str(), 
			idxWidth, rQueName.c_str(), dimStr.c_str(),
			pIdent->GetType()->GetType()->GetName().c_str(), cQueName.c_str(), dimStr.c_str(),
			idxWidth, cQueName.c_str(), dimStr.c_str(),
			pIdent->GetType()->GetType()->GetName().c_str(), cQueName.c_str(), dimStr.c_str(),
			cQueName.c_str(), dimStr.c_str(),
			idxWidth, cQueName.c_str(), dimStr.c_str(),
			idxWidth, cQueName.c_str(), dimStr.c_str() );

	OpenLine(buf);
	GetNextToken();

	ParseCompoundStatement(pHier, false);

	CHtfeIdent *pRdIdx = pHier->FindIdent(rQueName + "_RdAddr");
	pRdIdx->SetIsIgnoreSignalCheck();

	CHtfeIdent *pWrIdx = pHier->FindIdent(rQueName + "_WrAddr");
	pWrIdx->SetIsIgnoreSignalCheck();

	if (pIdent->IsHtBlockQue()) {
		CHtfeIdent *pWrIdx = pHier->FindIdent(rQueName + "_WrAddr2");
		pWrIdx->SetIsIgnoreSignalCheck();
	}

	CHtfeIdent *pRam = pHier->FindIdent(cQueName + "_RdData");
	pRam->SetIsHtPrimOutput(0);
	pRam->SetIsReadOnly();
	pRam->SetHtMemoryAddrWidth1(pIdent->GetType()->GetHtMemoryAddrWidth1());

	CHtfeIdent *pCnt = pHier->FindIdent(cQueName + "_Cnt");

    if (g_pHtfeArgs->IsHtQueCntSynKeepEnabled())
        pCnt->AddHtAttrib("syn_keep", pCnt->GetName(), -1, -1, "1", GetLineInfo());

	pCnt->SetIsWriteOnly();
	pCnt->SetIsIgnoreSignalCheck();
}

void CHtfeDesign::HtMemoryVarDecl(CHtfeIdent *pHier, CHtfeIdent *pIdent)
{
	// An HtDistRam or HtBlockRam was declared. Insert required variables
	//   for memory (c_rdIdx, c_wrIdx, c_rdData, c_wrData, c_wrEn)
	// Note that the wrEn is declared the width of the ram for LutRam and a single bit for blockRam
	string cQueName = pIdent->GetName();
	if (cQueName.substr(0,2) == "r_" || cQueName.substr(0,2) == "m_")
		cQueName.replace(0, 2, "c_");
	else
		cQueName.insert(0, "c_");

	string dimStr;
	if (pIdent->GetDimenCnt() > 0) {
		// an array of memory's
		char dimIdx[16];
		for (size_t i = 0; i < pIdent->GetDimenCnt(); i += 1) {
			//_itoa(pIdent->GetDimenList()[i], dimIdx, 10);
			sprintf(dimIdx, "%d", pIdent->GetDimenList()[i]);
			dimStr.append("[").append(dimIdx).append("]");
		}
	}

	int addrWidth1 = pIdent->GetType()->GetHtMemoryAddrWidth1();
	int addrWidth2 = pIdent->GetType()->GetHtMemoryAddrWidth2();
	int idxWidth = addrWidth1 + addrWidth2;

	int dataWidth = pIdent->GetType()->GetType()->GetWidth();

	char buf[1024];
	if (pIdent->IsHtMrdBlockRam()) {
		int selWidth = pIdent->GetType()->GetHtMemorySelWidth();
		sprintf(buf, "{ sc_uint<%d> %s_RdAddr%s; sc_uint<%d> %s_WrAddr%s; ht_noload sc_uint<%d> %s_WrSel%s;"
			" struct %s__MRD__ { %s m_mrd[%d]; }; %s__MRD__ ht_noload %s_RdData%s; %s %s_WrData%s; bool %s_WrEn%s; }",
			idxWidth, cQueName.c_str(), dimStr.c_str(),
			idxWidth, cQueName.c_str(), dimStr.c_str(),
			selWidth, cQueName.c_str(), dimStr.c_str(),
			cQueName.c_str(), pIdent->GetType()->GetType()->GetName().c_str(), 1<<selWidth,
			cQueName.c_str(), cQueName.c_str(), dimStr.c_str(),
			pIdent->GetType()->GetType()->GetName().c_str(), cQueName.c_str(), dimStr.c_str(),
			cQueName.c_str(), dimStr.c_str() );
	} else if (pIdent->IsHtMwrBlockRam()) {
		int selWidth = pIdent->GetType()->GetHtMemorySelWidth();
		sprintf(buf, "{ sc_uint<%d> %s_RdAddr%s; sc_uint<%d> %s_WrAddr%s; sc_uint<%d> ht_noload %s_RdSel%s; %s ht_noload %s_RdData%s;"
			" struct %s__MWR__ { %s m_mwr[%d]; }; %s__MWR__ %s_WrData%s; bool %s_WrEn%s; }",
			idxWidth, cQueName.c_str(), dimStr.c_str(),
			idxWidth, cQueName.c_str(), dimStr.c_str(),
			selWidth, cQueName.c_str(), dimStr.c_str(),
			pIdent->GetType()->GetType()->GetName().c_str(), cQueName.c_str(), dimStr.c_str(),
			cQueName.c_str(), pIdent->GetType()->GetType()->GetName().c_str(), 1<<selWidth,
			cQueName.c_str(), cQueName.c_str(), dimStr.c_str(),
			cQueName.c_str(), dimStr.c_str() );
	} else {
		sprintf(buf, "{ sc_uint<%d> %s_RdAddr%s; sc_uint<%d> %s_WrAddr%s; %s ht_noload %s_RdData%s; %s %s_WrData%s; bool %s_WrEn%s; }",
			idxWidth, cQueName.c_str(), dimStr.c_str(),
			idxWidth, cQueName.c_str(), dimStr.c_str(),
			pIdent->GetType()->GetType()->GetName().c_str(), cQueName.c_str(), dimStr.c_str(),
			pIdent->GetType()->GetType()->GetName().c_str(), cQueName.c_str(), dimStr.c_str(),
			cQueName.c_str(), dimStr.c_str() );
	}

	OpenLine(buf);
	GetNextToken();

	if (pHier->GetId() != CHtfeIdent::id_class)
		ParseMsg(PARSE_FATAL, "");

	ParseCompoundStatement(pHier, false);

	CHtfeIdent *pRam = pHier->FindIdent(cQueName + "_RdData");
	//pRam->SetIsHtQueueRam();
	pRam->SetIsHtPrimOutput(0);
	pRam->SetIsReadOnly();

	if (pIdent->IsHtMwrBlockRam()) {
		CHtfeIdent *pRdSel = pHier->FindIdent(cQueName + "_RdSel");
		pRdSel->SetIsWriteOnly();
	}

	CHtfeIdent *pRdIdx = pHier->FindIdent(cQueName + "_RdAddr");
	pRdIdx->SetIsWriteOnly();

	if (pIdent->IsHtMrdBlockRam()) {
		CHtfeIdent *pWrSel = pHier->FindIdent(cQueName + "_WrSel");
		pWrSel->SetIsWriteOnly();
	}

	CHtfeIdent *pWrIdx = pHier->FindIdent(cQueName + "_WrAddr");
	pWrIdx->SetIsWriteOnly();

	CHtfeIdent *pWrData = pHier->FindIdent(cQueName + "_WrData");
	pWrData->SetIsWriteOnly();

	CHtfeIdent *pWrEn = pHier->FindIdent(cQueName + "_WrEn");
	pWrEn->SetIsWriteOnly();
	if (pIdent->IsHtDistRam()) {
		vector<CHtDistRamWeWidth> *pWeWidth = new vector<CHtDistRamWeWidth>;
		pWeWidth->push_back(CHtDistRamWeWidth(dataWidth-1, 0));
		pIdent->SetHtDistRamWeWidth(pWeWidth);
		pWrEn->SetHtDistRamWeWidth(pWeWidth);
	}

	pIdent->SetIsReadOnly();
	pIdent->SetIsWriteOnly();
}

CHtfeStatement * CHtfeDesign::ParseHtQueueStatement(CHtfeIdent *pHier)
{
	// parse a reference to a HtQueue variable and return a statement list
	//   used for HtQueue routines that return void (Push, Pop, Clock)

	const CHtfeIdent *pHier2 = pHier;
	CHtfeIdent *pIdent = ParseVariableRef(pHier, pHier2, false, true); // check for existing identifier

	vector<CHtfeOperand *> indexList;
	ParseVariableIndexing(pHier, pIdent, indexList);

	if (GetToken() != tk_period || GetNextToken() != tk_identifier || GetNextToken() != tk_lparen) {
		ParseMsg(PARSE_ERROR, "expected ht_dist_que function");
		SkipTo(tk_semicolon);
		GetNextToken();
		return 0;
	}

	string queName = pIdent->GetName();
	string refDimStr;
	string declDimStr;

	char buf[1024];
	for (size_t i = 0; i < pIdent->GetDimenCnt(); i += 1) {
		sprintf(buf, "[%d]", pIdent->GetDimenList()[i]);
		declDimStr.append(buf);

		refDimStr.append("[0]");
	}

	string cQueName = queName;
	if (cQueName.substr(0,2) == "r_" || cQueName.substr(0,2) == "m_")
		cQueName.replace(0, 2, "c_");
	else
		cQueName.insert(0, "c_");

	string rQueName = queName;
	if (rQueName.substr(0,2) == "m_")
		rQueName.replace(0, 2, "r_");

	CHtfeStatement *pStatement = 0;
	CHtfeIdent *pFunction = pHier->FindHierFunction();

	if (GetString() == "push") {

		pIdent->SetIsHtQueuePushSeen();
		pIdent->AddWriter(pFunction);

		if (pIdent->GetWriterListCnt() > 1 && !pIdent->IsMultipleWriterErrorReported()) {
			pIdent->SetIsMultipleWriterErrorReported();
			ParseMsg(PARSE_ERROR, "push for ht_dist_que %s in methods %s and %s", pIdent->GetName().c_str(),
				pIdent->GetWriterList()[0]->GetName().c_str(), pIdent->GetWriterList()[1]->GetName().c_str());
		}

		GetNextToken();
		CHtfeOperand *pOperand = ParseExpression(pHier);

		sprintf(buf, "{{ %s_WrData%s = 0; %s_WrEn%s = true; %s_WrAddr%s = %s_WrAddr%s + 1; }}",
			cQueName.c_str(), refDimStr.c_str(),
			cQueName.c_str(), refDimStr.c_str(),
			cQueName.c_str(), refDimStr.c_str(), rQueName.c_str(), refDimStr.c_str() );

		OpenLine(buf);
		GetNextToken();

		pStatement = ParseCompoundStatement(pHier);

		if (pStatement) {
			delete pStatement->GetExpr()->GetOperand2();
			pStatement->GetExpr()->SetOperand2( pOperand );
		}

		// set actual indexing expressions
		for (CHtfeStatement *pSt = pStatement; pSt; pSt = pSt->GetNext()) {
			pSt->GetExpr()->GetOperand1()->SetIndexList(indexList);

			if (pSt->GetExpr()->GetOperand2()->GetOperator() == tk_plus)
				pSt->GetExpr()->GetOperand2()->GetOperand1()->SetIndexList(indexList);
		}

	} else if (GetString() == "pop") {

		pIdent->SetIsHtQueuePopSeen();
		pIdent->AddReader(pFunction);

		if (pIdent->GetReaderListCnt() > 1 && !pIdent->IsMultipleWriterErrorReported()) {
			pIdent->SetIsMultipleWriterErrorReported();
			ParseMsg(PARSE_ERROR, "pop for ht_dist_que %s in methods %s and %s", pIdent->GetName().c_str(),
				pIdent->GetReaderList()[0]->GetName().c_str(), pIdent->GetReaderList()[1]->GetName().c_str());
		}

		sprintf(buf, "%s_RdAddr%s = %s_RdAddr%s + 1;",
			cQueName.c_str(), refDimStr.c_str(), rQueName.c_str(), refDimStr.c_str() );

		OpenLine(buf);
		GetNextToken();
		pStatement = ParseStatement(pHier);

		CHtfeStatement * pSt = pStatement;
		if (!pSt->GetExpr()->IsLeaf()) {
			pSt->GetExpr()->GetOperand1()->SetIndexList(indexList);
			pSt->GetExpr()->GetOperand2()->GetOperand1()->SetIndexList(indexList);
		}
		Assert(pSt->GetNext() == 0);

		GetNextToken();
		
	} else if (GetString() == "clock") {
		if (GetNextToken() != tk_identifier) {
			ParseMsg(PARSE_ERROR, "expected a reset signal for ht_dist_que Clock");
			SkipTo(tk_semicolon);
			GetNextToken();
			return 0;
		}

		string resetStr = GetString();
		if (GetNextToken() == tk_period) {
			if (GetNextToken() != tk_identifier || GetString() != "read" || GetNextToken() != tk_lparen || GetNextToken() != tk_rparen)
				ParseMsg(PARSE_ERROR, "unknown member specified for reset");
			else
				resetStr += ".read()";
			GetNextToken();
		}

		if (GetToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "parsing reset signal, expected ')'");
			return 0;
		}

		pStatement = 0;

		if (!pIdent->IsHtQueuePushClockSeen()) {
			InitHtQueuePushPreClockStatements(pHier, pIdent, cQueName, rQueName);
			//if (pIdent->IsHtQueuePushSeen())
				InitHtQueuePushClockStatements(pHier, pIdent, pStatement, resetStr, cQueName, rQueName, refDimStr);
			pIdent->SetIsHtQueuePushClockSeen();
		}

		if (!pIdent->IsHtQueuePopClockSeen()) {
			InitHtQueuePopPreClockStatements(pHier, pIdent, cQueName, rQueName);
			InitHtQueuePopClockStatements(pHier, pIdent, pStatement, resetStr, cQueName, rQueName, refDimStr);
			pIdent->SetIsHtQueuePopClockSeen();
		}

		pIdent->AddWriter(pHier);
		pIdent->AddReader(pHier);
		
	} else if (GetString() == "push_clock") {

		if (GetNextToken() != tk_identifier) {
			ParseMsg(PARSE_ERROR, "expected a reset signal for ht_dist_que push_clock");
			SkipTo(tk_semicolon);
			GetNextToken();
			return 0;
		}

		string resetStr = GetString();
		if (GetNextToken() == tk_period) {
			if (GetNextToken() != tk_identifier || GetString() != "read" || GetNextToken() != tk_lparen || GetNextToken() != tk_rparen)
				ParseMsg(PARSE_ERROR, "unknown member specified for reset");
			else
				resetStr += ".read()";
			GetNextToken();
		}

		if (!pIdent->IsHtQueuePushClockSeen()) {
			InitHtQueuePushPreClockStatements(pHier, pIdent, cQueName, rQueName);
			InitHtQueuePushClockStatements(pHier, pIdent, pStatement, resetStr, cQueName, rQueName, refDimStr);
			pIdent->SetIsHtQueuePushClockSeen();
		}

		pIdent->AddWriter(pHier);

	} else if (GetString() == "pop_clock") {

		if (GetNextToken() != tk_identifier) {
			ParseMsg(PARSE_ERROR, "parsing 'pop_clock( reset )', expected a reset signal");
			SkipTo(tk_semicolon);
			GetNextToken();
			return 0;
		}

		string resetStr = GetString();
		if (GetNextToken() == tk_period) {
			if (GetNextToken() != tk_identifier || GetString() != "read" || GetNextToken() != tk_lparen || GetNextToken() != tk_rparen)
				ParseMsg(PARSE_ERROR, "unknown member specified for reset");
			else
				resetStr += ".read()";
			GetNextToken();
		}

		if (!pIdent->IsHtQueuePopClockSeen()) {
			InitHtQueuePopPreClockStatements(pHier, pIdent, cQueName, rQueName);
			InitHtQueuePopClockStatements(pHier, pIdent, pStatement, resetStr, cQueName, rQueName, refDimStr);
			pIdent->SetIsHtQueuePopClockSeen();
		}

		pIdent->AddReader(pHier);

	} else {
		ParseMsg(PARSE_ERROR, "unknown ht_dist_que function");
		SkipTo(tk_semicolon);
		GetNextToken();
		return 0;
	}

	if (GetToken() != tk_rparen || GetNextToken() != tk_semicolon) {
		ParseMsg(PARSE_ERROR, "expected ht_dist_que function");
		SkipTo(tk_semicolon);
		GetNextToken();
		return 0;
	}

	GetNextToken();

	return pStatement;
}

void CHtfeDesign::InitHtQueuePushPreClockStatements(CHtfeIdent * pHier, CHtfeIdent * pIdent, string &cQueName, string &rQueName)
{
	char buf[1024];

	CHtfeIdent *pModule = pHier->FindHierModule();
	CHtfeIdent *pFunction = pHier->FindHierFunction();

	vector<int> refList(pIdent->GetDimenCnt(), 0);

	do {
		string initDimStr;

		for (size_t i = 0; i < refList.size(); i += 1) {
			sprintf(buf, "[%d]", refList[i]);
			initDimStr += buf;
		}

		// initialize the push variables
		sprintf(buf, "{ %s_WrEn%s = false; %s_WrData%s = 0; %s_WrAddr%s = %s_WrAddr%s; }",
			cQueName.c_str(), initDimStr.c_str(),
			cQueName.c_str(), initDimStr.c_str(),
			cQueName.c_str(), initDimStr.c_str(), rQueName.c_str(), initDimStr.c_str() );

		OpenLine(buf);
		GetNextToken();

		// insert statements at beginning of block
		CHtfeStatement * pStatement = ParseCompoundStatement(pHier, false);

		// mark WrEn as write only
		pModule->FindIdent(cQueName + "_WrEn")->SetIsWriteOnly();

		// mark WrData as write only
		pModule->FindIdent(cQueName + "_WrData")->SetIsWriteOnly();

		if (pFunction->IsMethod())
			pFunction->InsertStatementList(pStatement);
		else
			pFunction->InsertMemInitStatements(pStatement);

	} while (!pIdent->DimenIter(refList));
}

void CHtfeDesign::InitHtQueuePopPreClockStatements(CHtfeIdent * pHier, CHtfeIdent * pIdent, string &cQueName, string &rQueName)
{
	char buf[1024];

	//CHtfeIdent *pModule = pHier->FindHierModule();
	CHtfeIdent *pFunction = pHier->FindHierFunction();

	vector<int> refList(pIdent->GetDimenCnt(), 0);

	do {
		string initDimStr;

		for (size_t i = 0; i < refList.size(); i += 1) {
			sprintf(buf, "[%d]", refList[i]);
			initDimStr += buf;
		}

		sprintf(buf, "%s_RdAddr%s = %s_RdAddr%s;",
			cQueName.c_str(), initDimStr.c_str(), rQueName.c_str(), initDimStr.c_str() );

		OpenLine(buf);
		GetNextToken();

		CHtfeStatement *pStatement = ParseStatement(pHier);

		// insert statement at beginning of block
		if (pFunction->IsMethod())
			pFunction->InsertStatementList(pStatement);
		else
			pFunction->InsertMemInitStatements(pStatement);

	} while (!pIdent->DimenIter(refList));
}

void CHtfeDesign::InitHtQueuePushClockStatements(CHtfeIdent * pHier, CHtfeIdent * pIdent, CHtfeStatement * &pStatement, 
	string &resetStr, string &cQueName, string &rQueName, string &refDimStr)
{
	vector<int> refList(pIdent->GetDimenCnt(), 0);

	char buf[1024];
	do {
		string initDimStr;

		for (size_t i = 0; i < refList.size(); i += 1) {
			sprintf(buf, "[%d]", refList[i]);
			initDimStr += buf;
		}

		string statementStr = "{ ";
		sprintf(buf, "%s_Cnt%s = %s_WrAddr%s - %s_RdAddr%s; ",
			cQueName.c_str(), initDimStr.c_str(),
			rQueName.c_str(), initDimStr.c_str(),
			rQueName.c_str(), initDimStr.c_str() );
		statementStr += buf;

		if (pIdent->IsHtBlockQue()) {
			sprintf(buf, "%s_WrAddr2%s = %s ? 0 : %s_WrAddr%s; ",
				rQueName.c_str(), initDimStr.c_str(),
				resetStr.c_str(),
				rQueName.c_str(), initDimStr.c_str() );
			statementStr += buf;
		}
		sprintf(buf, "%s_WrAddr%s = %s ? 0 : %s_WrAddr%s; ",
			rQueName.c_str(), initDimStr.c_str(),
			resetStr.c_str(),
			cQueName.c_str(), initDimStr.c_str() );
		statementStr += buf;
		statementStr += " }";

		OpenLine(statementStr);
		GetNextToken();
		CHtfeStatement * pStatement2 = ParseCompoundStatement(pHier);

		// append new statements to end of existing list
		CHtfeStatement **ppSt = &pStatement;
		while (*ppSt != 0)
			ppSt = (*ppSt)->GetPNext();

		*ppSt = pStatement2;

	} while (!pIdent->DimenIter(refList));
}

void CHtfeDesign::InitHtQueuePopClockStatements(CHtfeIdent * pHier, CHtfeIdent * pIdent, CHtfeStatement * &pStatement, 
	string &resetStr, string &cQueName, string &rQueName, string &refDimStr)
{
	vector<int> refList(pIdent->GetDimenCnt(), 0);

	char buf[1024];
	do {
		string initDimStr;

		for (size_t i = 0; i < refList.size(); i += 1) {
			sprintf(buf, "[%d]", refList[i]);
			initDimStr += buf;
		}

		string statementStr = "{ ";
		sprintf(buf, "%s_RdAddr%s = %s ? 0 : %s_RdAddr%s; ",
			rQueName.c_str(), initDimStr.c_str(),
			resetStr.c_str(),
			cQueName.c_str(), initDimStr.c_str() );
		statementStr += buf;
		statementStr += " }";

		OpenLine(statementStr);
		GetNextToken();
		CHtfeStatement * pStatement2 = ParseCompoundStatement(pHier);

		// append new statements to end of existing list
		CHtfeStatement **ppSt = &pStatement;
		while (*ppSt != 0)
			ppSt = (*ppSt)->GetPNext();

		*ppSt = pStatement2;

	} while (!pIdent->DimenIter(refList));
}

CHtfeOperand * CHtfeDesign::ParseHtQueueExpr(CHtfeIdent *pHier, CHtfeIdent *pIdent, vector<CHtfeOperand *> &indexList)
{
	// parse a reference to a HtQueue variable and return an expression
	//   used for HtQueue routines that return a value (Front, Empty, Full)

	if (GetToken() != tk_period || GetNextToken() != tk_identifier || GetNextToken() != tk_lparen) {
		ParseMsg(PARSE_ERROR, "expected ht_dist_que function");
		SkipTo(tk_semicolon);
		GetNextToken();
		return 0;
	}

	string queName = pIdent->GetName();
	string refDimStr;
	for (size_t i = 0; i < pIdent->GetDimenCnt(); i += 1)
		refDimStr.append("[0]");

	string cQueName = queName;
	if (cQueName.substr(0,2) == "r_" || cQueName.substr(0,2) == "m_")
		cQueName.replace(0, 2, "c_");
	else
		cQueName.insert(0, "c_");

	string rQueName = queName;
	if (rQueName.substr(0,2) == "m_")
		rQueName.replace(0, 2, "r_");

	int addrWidth = pIdent->GetType()->GetHtMemoryAddrWidth1()+pIdent->GetType()->GetHtMemoryAddrWidth2();
	int capacity = 1 << addrWidth;

	char buf[512];
	CHtfeOperand *pOperand;
	if (GetString() == "size") {
		sprintf(buf, "%s_Cnt%s )",
			cQueName.c_str(), refDimStr.c_str() );

		OpenLine(buf);
		GetNextToken();

		pOperand = ParseExpression(pHier, false, true, false);
		pOperand->SetIndexList(indexList);
		
		GetNextToken();
		
		if (GetNextToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "expected ht_dist_que function");
			SkipTo(tk_semicolon);
			GetNextToken();
			return 0;
		}
	} else 	if (GetString() == "read_addr") {

		sprintf(buf, "%s_RdAddr%s(%d,0) )", rQueName.c_str(), refDimStr.c_str(),
			addrWidth-1 );

		OpenLine(buf);
		GetNextToken();

		pOperand = ParseExpression(pHier, false, true, false);
		pOperand->SetIndexList(indexList);
		
		GetNextToken();
		
		if (GetNextToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "parsing 'read_addr()', expected a )");
			SkipTo(tk_semicolon);
			GetNextToken();
			return 0;
		}
	} else 	if (GetString() == "write_addr") {

		sprintf(buf, "%s_WrAddr%s(%d,0) )", rQueName.c_str(), refDimStr.c_str(),
			addrWidth-1 );

		OpenLine(buf);
		GetNextToken();

		pOperand = ParseExpression(pHier, false, true, false);
		pOperand->SetIndexList(indexList);
		
		GetNextToken();
		
		if (GetNextToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "parsing 'write_addr()', expected a )");
			SkipTo(tk_semicolon);
			GetNextToken();
			return 0;
		}
	} else if (GetString() == "capacity") {
		sprintf(buf, "0x%x )", capacity );

		OpenLine(buf);
		GetNextToken();

		pOperand = ParseExpression(pHier, false, true, false);
		
		GetNextToken();
		
		if (GetNextToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "parsing 'capacity()', expected a )");
			SkipTo(tk_semicolon);
			GetNextToken();
			return 0;
		}
	} else if (GetString() == "empty") {
		if (pIdent->IsHtBlockQue())
			sprintf(buf, "(%s_WrAddr2%s == %s_RdAddr%s) )", rQueName.c_str(), refDimStr.c_str(), rQueName.c_str(), refDimStr.c_str());
		else
			sprintf(buf, "(%s_WrAddr%s == %s_RdAddr%s) )", rQueName.c_str(), refDimStr.c_str(), rQueName.c_str(), refDimStr.c_str());

		OpenLine(buf);
		GetNextToken();

		pOperand = ParseExpression(pHier, false, true, false);
		pOperand->GetOperand1()->SetIndexList(indexList);
		pOperand->GetOperand2()->SetIndexList(indexList);
	
		GetNextToken();
		
		if (GetNextToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "parsing 'empty()', expected a )");
			SkipTo(tk_semicolon);
			GetNextToken();
			return 0;
		}
	} else if (GetString() == "front") {
		if (GetNextToken() != tk_rparen)
			ParseMsg(PARSE_ERROR, "expected a )");

		string bufStr = cQueName + "_RdData" + refDimStr;

		while (GetNextToken() == tk_period) {
			bufStr += ".";
			if (GetNextToken() != tk_identifier)
				ParseMsg(PARSE_ERROR, "expected an identifier");
			bufStr += GetString();
		}
		bufStr += ")";

		OpenLine(bufStr);
		GetNextToken();

		pOperand = ParseExpression(pHier, false, true, false);
		pOperand->SetIndexList(indexList);

		pIdent->SetIsHtQueueFrontSeen();
		
	} else if (GetString() == "full") {
		int num = 0;
		if (GetNextToken() == tk_num_int || GetToken() == tk_num_hex) {
			if (!GetSint32ConstValue(num) || num > capacity-1) {
				ParseMsg(PARSE_ERROR, "full remaining value exceeds ht_dist_que capacity");
				num = 0;
			}
			GetNextToken();

		} else if (GetToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "expected integer constant parameter for full");
			SkipTo(tk_semicolon);
			GetNextToken();
			return 0;
		}

		sprintf(buf, "( %s_Cnt%s >= 0x%x - 0x%x) )",
			cQueName.c_str(), refDimStr.c_str(),
			capacity, num);

		OpenLine(buf);
		GetNextToken();

		pOperand = ParseExpression(pHier, false, true, false);
		pOperand->GetOperand1()->SetIndexList(indexList);
		
		if (GetNextToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "parsing 'full()', expected )");
			SkipTo(tk_semicolon);
			GetNextToken();
			return 0;
		}
	} else {
		ParseMsg(PARSE_ERROR, "expected ht_dist_que function, '%s'", GetString().c_str());
		SkipTo(tk_semicolon);
		GetNextToken();
		return 0;
	}

	GetNextToken();

	return pOperand;
}

CHtfeStatement * CHtfeDesign::ParseHtMemoryStatement(CHtfeIdent *pHier)
{
	// parse a reference to a HtMemory variable and return a statement list
	//   used for HtMemory routines that return void (read_addr, write_addr, write_mem, clock)

	const CHtfeIdent *pHier2 = pHier;
	CHtfeIdent *pIdent = ParseVariableRef(pHier, pHier2, false, true); // check for existing identifier

	vector<CHtfeOperand *> indexList;
	ParseVariableIndexing(pHier, pIdent, indexList);

	if (GetToken() != tk_period || GetNextToken() != tk_identifier || GetNextToken() != tk_lparen) {
		ParseMsg(PARSE_ERROR, "expected ht_dist_ram/ht_block_ram function");
		SkipTo(tk_semicolon);
		GetNextToken();
		return 0;
	}

	string queName = pIdent->GetName();
	string refDimStr;
	string declDimStr;

	char buf[1024];
	for (size_t i = 0; i < pIdent->GetDimenCnt(); i += 1) {
		sprintf(buf, "[%d]", pIdent->GetDimenList()[i]);
		declDimStr.append(buf);

		refDimStr.append("[0]");
	}

	string cQueName = queName;
	if (cQueName.substr(0,2) == "r_" || cQueName.substr(0,2) == "m_")
		cQueName.replace(0, 2, "c_");
	else
		cQueName.insert(0, "c_");

	string rQueName = queName;
	if (rQueName.substr(0,2) == "m_")
		rQueName.replace(0, 2, "r_");

	//int addrWidth = pIdent->GetType()->GetHtMemoryAddrWidth1()+pIdent->GetType()->GetHtMemoryAddrWidth2();

	CHtfeStatement *pStatement;

	if (GetString() == "read_addr") {
		// need one or two addresses

		pIdent->SetIsHtMemoryReadAddrSeen();

		GetNextToken();

		CHtfeOperand *pRdSelOp = 0;
		if (pIdent->GetType()->IsHtMwrBlockRam()) {
			pRdSelOp = ParseExpression(pHier, true);

			if (GetToken() != tk_comma) {
				ParseMsg(PARSE_ERROR, "expected write_addr(rdSel, rdAddr1 - missing ,");
				SkipTo(tk_semicolon);
				return 0;
			}
			GetNextToken();
		}

		CHtfeOperand *pIdxOp1 = ParseExpression(pHier, true);

		CHtfeOperand *pIdxOp2 = 0;
		if (pIdent->GetType()->GetHtMemoryAddrWidth2() > 0) {
			if (GetToken() != tk_comma) {
				ParseMsg(PARSE_ERROR, "expected read_addr(idx1, idx2) - missing ,");
				SkipTo(tk_semicolon);
				return 0;
			}
			GetNextToken();
			pIdxOp2 = ParseExpression(pHier);
		}

		if (GetToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "expected read_addr(idx1, idx2) - missing )");
			SkipTo(tk_semicolon);
			return 0;
		}

		if (pIdent->GetType()->GetHtMemoryAddrWidth2() == 0) {
			if (pIdent->GetType()->IsHtMwrBlockRam())
				sprintf(buf, "{{ %s_RdSel%s = 0; %s_RdAddr%s = 0; }}",
					cQueName.c_str(), refDimStr.c_str(),
					cQueName.c_str(), refDimStr.c_str() );
			else
				sprintf(buf, "{{ %s_RdAddr%s = 0; }}",
					cQueName.c_str(), refDimStr.c_str() );
		} else {
			int highBitIdx1 = pIdent->GetType()->GetHtMemoryAddrWidth1() - 1;
			int lowBitIdx2 = highBitIdx1 + 1;
			int highBitIdx2 = lowBitIdx2 + pIdent->GetType()->GetHtMemoryAddrWidth2() - 1;

			if (pIdent->GetType()->IsHtMwrBlockRam())
				sprintf(buf, "{{ %s_RdSel%s = 0; %s_RdAddr%s(%d,0) = 0; %s_RdAddr%s(%d,%d) = 0; }}",
					cQueName.c_str(), refDimStr.c_str(),
					cQueName.c_str(), refDimStr.c_str(), highBitIdx1,
					cQueName.c_str(), refDimStr.c_str(), highBitIdx2, lowBitIdx2 );
			else
				sprintf(buf, "{{ %s_RdAddr%s(%d,0) = 0; %s_RdAddr%s(%d,%d) = 0; }}",
					cQueName.c_str(), refDimStr.c_str(), highBitIdx1,
					cQueName.c_str(), refDimStr.c_str(), highBitIdx2, lowBitIdx2 );
		}

		OpenLine(buf);
		GetNextToken();

		pStatement = ParseCompoundStatement(pHier);
		CHtfeStatement *pSt = pStatement;

		if (pIdent->GetType()->IsHtMwrBlockRam()) {
			delete pSt->GetExpr()->GetOperand2();
			pSt->GetExpr()->SetOperand2( pRdSelOp );
			pSt = pSt->GetNext();
		}

		delete pSt->GetExpr()->GetOperand2();
		pSt->GetExpr()->SetOperand2( pIdxOp1 );
		pSt = pSt->GetNext();

		if (pIdent->GetType()->GetHtMemoryAddrWidth2() > 0) {
			delete pSt->GetExpr()->GetOperand2();
			pSt->GetExpr()->SetOperand2( pIdxOp2 );
			pSt = pSt->GetNext();
		}
		Assert(pSt == 0);

		// set actual indexing expressions
		for (pSt = pStatement; pSt; pSt = pSt->GetNext())
			pSt->GetExpr()->GetOperand1()->SetIndexList(indexList);

	} else if (GetString() == "write_addr") {
		// need one or two addresses

		pIdent->SetIsHtMemoryWriteAddrSeen();

		GetNextToken();

		CHtfeOperand *pWrSelOp = 0;
		if (pIdent->GetType()->IsHtMrdBlockRam()) {
			pWrSelOp = ParseExpression(pHier, true);

			if (GetToken() != tk_comma) {
				ParseMsg(PARSE_ERROR, "expected write_addr(wrSel, wrAddr1 - missing ,");
				SkipTo(tk_semicolon);
				return 0;
			}
			GetNextToken();
		}

		CHtfeOperand *pIdxOp1 = ParseExpression(pHier, true);

		CHtfeOperand *pIdxOp2 = 0;
		if (pIdent->GetType()->GetHtMemoryAddrWidth2() > 0) {
			if (GetToken() != tk_comma) {
				ParseMsg(PARSE_ERROR, "expected write_addr(idx1, idx2) - missing ,");
				SkipTo(tk_semicolon);
				return 0;
			}
			GetNextToken();
			pIdxOp2 = ParseExpression(pHier, true);
		}

		if (GetToken() != tk_rparen) {
			ParseMsg(PARSE_ERROR, "expected write_addr(idx1, idx2) - missing )");
			SkipTo(tk_semicolon);
			return 0;
		}

		if (pIdent->GetType()->GetHtMemoryAddrWidth2() == 0) {
			if (pIdent->GetType()->IsHtMrdBlockRam())
				sprintf(buf, "{{ %s_WrSel%s = 0; %s_WrAddr%s = 0; }}",
					cQueName.c_str(), refDimStr.c_str(),
					cQueName.c_str(), refDimStr.c_str() );
			else
				sprintf(buf, "{{ %s_WrAddr%s = 0; }}",
					cQueName.c_str(), refDimStr.c_str() );

		} else {
			int highBitIdx1 = pIdent->GetType()->GetHtMemoryAddrWidth1() - 1;
			int lowBitIdx2 = highBitIdx1 + 1;
			int highBitIdx2 = lowBitIdx2 + pIdent->GetType()->GetHtMemoryAddrWidth2() - 1;

			if (pIdent->GetType()->IsHtMrdBlockRam())
				sprintf(buf, "{{ %s_WrSel%s = 0; %s_WrAddr%s(%d,0) = 0; %s_WrAddr%s(%d,%d) = 0; }}",
					cQueName.c_str(), refDimStr.c_str(),
					cQueName.c_str(), refDimStr.c_str(), highBitIdx1,
					cQueName.c_str(), refDimStr.c_str(), highBitIdx2, lowBitIdx2 );
			else
				sprintf(buf, "{{ %s_WrAddr%s(%d,0) = 0; %s_WrAddr%s(%d,%d) = 0; }}",
					cQueName.c_str(), refDimStr.c_str(), highBitIdx1,
					cQueName.c_str(), refDimStr.c_str(), highBitIdx2, lowBitIdx2 );
		}

		OpenLine(buf);
		GetNextToken();

		pStatement = ParseCompoundStatement(pHier);
		CHtfeStatement *pSt = pStatement;

		if (pIdent->GetType()->IsHtMrdBlockRam()) {
			delete pSt->GetExpr()->GetOperand2();
			pSt->GetExpr()->SetOperand2( pWrSelOp );
			pSt = pSt->GetNext();
		}

		delete pSt->GetExpr()->GetOperand2();
		pSt->GetExpr()->SetOperand2( pIdxOp1 );
		pSt = pSt->GetNext();

		if (pIdent->GetType()->GetHtMemoryAddrWidth2() > 0) {
			delete pSt->GetExpr()->GetOperand2();
			pSt->GetExpr()->SetOperand2( pIdxOp2 );
			pSt = pSt->GetNext();
		}
		Assert(pSt == 0);

		// set actual indexing expressions
		for (pSt = pStatement; pSt; pSt = pSt->GetNext())
			pSt->GetExpr()->GetOperand1()->SetIndexList(indexList);

	} else if (GetString() == "write_mem") {

		pIdent->SetIsHtMemoryWriteMemSeen();

		string wrDataStr = cQueName + "_WrData" + refDimStr;
		string wrEnStr = cQueName + "_WrEn" + refDimStr;
		
		CHtfeOperand *pOperand;
		CHtfeOperand *pWrSelOp = 0;
		if (GetNextToken() != tk_rparen) {
			// full width write

			if (pIdent->GetType()->IsHtMwrBlockRam()) {
				pWrSelOp = ParseExpression(pHier, true);

				if (GetToken() != tk_comma) {
					ParseMsg(PARSE_ERROR, "expected write_mem(wrSel, wrData) - missing ,");
					SkipTo(tk_semicolon);
					return 0;
				}
				GetNextToken();
			}

			pOperand = ParseExpression(pHier, true);

			if (pOperand == 0 || GetToken() != tk_rparen) {
				ParseMsg(PARSE_ERROR, "expected 'write_mem() = expression' or 'write_mem( expression )'");
				SkipTo(tk_semicolon);
				return 0;
			}
			GetNextToken();

			if (GetToken() != tk_semicolon) {
				ParseMsg(PARSE_ERROR, "'write_mem( expression );', expected ; ");
				SkipTo(tk_semicolon);
			}

		} else {
			if (pIdent->IsHtBlockRam() || pIdent->IsHtMrdBlockRam() || pIdent->IsHtMwrBlockRam()) {
				ParseMsg(PARSE_ERROR, "ht_block_ram memories must use 'write_mem( expression );'");
				SkipTo(tk_semicolon);
				return 0;
			}

			GetNextToken();
			while (GetToken() == tk_period) {
				wrDataStr += ".";

				if (GetNextToken() != tk_identifier)
					ParseMsg(PARSE_ERROR, "expected a member identifier");

				wrDataStr += GetString();

				GetNextToken();
				while (GetToken() == tk_lbrack) {
					GetNextToken();

					CConstValue idxValue;
					if (!ParseConstExpr(idxValue)) {
						ParseMsg(PARSE_ERROR, "Parsing '[ integer ]', expected contant index");
						SkipTo(tk_rbrack);
					}
					if (GetToken() != tk_rbrack) {
						ParseMsg(PARSE_ERROR, "Parsing '[ integer ]', expected ]");
						SkipTo(tk_rbrack);
					}
					GetNextToken();

					char buf[32];
					sprintf(buf, "[%d]", (int)idxValue.GetSint64());
					wrDataStr += buf;
				}			
			}

			while (GetToken() == tk_lparen) {
				GetNextToken();

				CConstValue idx1Value;
				if (!ParseConstExpr(idx1Value)) {
					ParseMsg(PARSE_ERROR, "Parsing '( upperIdx, lowerIdx )', expected contant integer indecies");
					SkipTo(tk_rparen);
					break;
				}

				if (GetToken() != tk_comma)
					ParseMsg(PARSE_ERROR, "Expected a comma");
				GetNextToken();

				CConstValue idx2Value;
				if (!ParseConstExpr(idx2Value)) {
					ParseMsg(PARSE_ERROR, "Parsing '( upperIdx, lowerIdx )', expected contant integer indecies");
					SkipTo(tk_rparen);
					break;
				}

				if (GetToken() != tk_rparen) {
					ParseMsg(PARSE_ERROR, "Parsing '( upperIdx, lowerIdx )', expected contant integer indecies");
					SkipTo(tk_rbrack);
					break;
				}
				GetNextToken();

				char buf[32];
				sprintf(buf, "(%d,%d)", (int)idx1Value.GetSint64(), (int)idx2Value.GetSint64());
				wrDataStr += buf;
				break;
			}

			if (GetToken() != tk_equal) {
				ParseMsg(PARSE_ERROR, "'write_mem() = expression;', expected = ");
				SkipTo(tk_semicolon);
				return 0;
			}
			GetNextToken();

			pOperand = ParseExpression(pHier);

			if (pOperand == 0)
				ParseMsg(PARSE_FATAL, "Previous errors prohibit further parsing");

			if (GetToken() != tk_semicolon) {
				ParseMsg(PARSE_ERROR, "'write_mem() = expression;', expected ; ");
				SkipTo(tk_semicolon);
			}
		}

		if (pIdent->GetType()->IsHtMwrBlockRam())
			sprintf(buf, "{{ %s.m_mwr[0xdeadbeef] = 0; }}", wrDataStr.c_str() );
		else
			sprintf(buf, "{{ %s = 0; }}", wrDataStr.c_str() );

		OpenLine(buf);
		GetNextToken();

		pStatement = ParseCompoundStatement(pHier);
		if (!pStatement)
			return 0;

		if (pIdent->GetType()->IsHtMwrBlockRam()) {
			CScSubField * pSubField = pStatement->GetExpr()->GetOperand1()->GetSubField();
			while (pSubField && (!pSubField->m_indexList[0]->IsConstValue() || pSubField->m_indexList[0]->GetConstValue().GetUint32() != 0xdeadbeef))
				pSubField = pSubField->m_pNext;
			Assert(pSubField);

			delete pSubField->m_indexList[0];
			pSubField->m_indexList[0] = pWrSelOp;
		}

		delete pStatement->GetExpr()->GetOperand2();
		pStatement->GetExpr()->SetOperand2( pOperand );

		// Set Write Enable mask
		int lowBit, width;
		bool bIsConst = GetSubFieldRange(pStatement->GetExpr()->GetOperand1(), lowBit, width, false);

		if (!bIsConst)
			ParseMsg(PARSE_FATAL, pStatement->GetExpr()->GetOperand1()->GetLineInfo(), "non-constant bit range in sc_memory");

		int highBit = lowBit + width - 1;

		int dataWidth = pIdent->GetType()->GetType()->GetWidth();

		CHtfeIdent *pWrEn = 0;
		string wrEnName = cQueName + "_WrEn";
		const CHtfeIdent *pHier2;
		for (pHier2 = pHier; pHier2; pHier2 = pHier2->GetPrevHier()) {
			pWrEn = pHier2->FindIdent(wrEnName);
			if (pWrEn)
				break;
		}
		Assert(pWrEn);

		while (lowBit <= highBit) {

			int width;
			if (pIdent->IsHtBlockRam() || pIdent->IsHtMrdBlockRam() || pIdent->IsHtMwrBlockRam() || dataWidth == 1) {
				width = highBit - lowBit + 1;
				sprintf(buf, "{{ %s = 1; }}", wrEnStr.c_str());
			} else {
				if (highBit - lowBit + 1 > 64)
					width = 64;
				else
					width = highBit - lowBit + 1;

				sprintf(buf, "{{ %s = true; }}", wrEnStr.c_str());

				pWrEn->SetHtDistRamWeSubRange(lowBit + width - 1, lowBit);
			}

			OpenLine(buf);
			GetNextToken();

			CHtfeStatement *pStatement2 = ParseCompoundStatement(pHier);

			if (!pIdent->IsHtBlockRam() && !pIdent->IsHtMrdBlockRam() && !pIdent->IsHtMwrBlockRam() && dataWidth > 1) {
				CHtfeOperand *pWrEnOp = pStatement2->GetExpr()->GetOperand1();
				pWrEnOp->SetDistRamWeWidth(highBit, lowBit);
			}

			pStatement2->SetNext(pStatement);
			pStatement = pStatement2;

			lowBit += width;
		}		

		// set actual indexing expressions
		for (CHtfeStatement *pSt = pStatement; pSt; pSt = pSt->GetNext())
			pSt->GetExpr()->GetOperand1()->SetIndexList(indexList);

		GetNextToken();

		return pStatement;

	} else if (GetString() == "clock") {

		HtMemoryReadVarInit(pHier, pIdent, cQueName, refDimStr, declDimStr, indexList);
		HtMemoryWriteVarInit(pHier, pIdent, cQueName, refDimStr, declDimStr, indexList);

		// just drop the statement (clock is required for systemC)
		pStatement = 0;
		GetNextToken();

		pIdent->AddWriter(pHier);
		pIdent->AddReader(pHier);

	} else if (GetString() == "read_clock") {

		HtMemoryReadVarInit(pHier, pIdent, cQueName, refDimStr, declDimStr, indexList);

		// just drop the statement (clock is required for systemC)
		pStatement = 0;
		GetNextToken();

		pIdent->AddReader(pHier);

	} else if (GetString() == "write_clock") {

		HtMemoryWriteVarInit(pHier, pIdent, cQueName, refDimStr, declDimStr, indexList);

		// just drop the statement (clock is required for systemC)
		pStatement = 0;
		GetNextToken();

		pIdent->AddWriter(pHier);

	} else {
		ParseMsg(PARSE_ERROR, "unknown sc_memory function");
		SkipTo(tk_semicolon);
		GetNextToken();
		return 0;
	}

	if (GetToken() != tk_rparen || GetNextToken() != tk_semicolon) {
		ParseMsg(PARSE_ERROR, "expected )");
		SkipTo(tk_semicolon);
		GetNextToken();
		return 0;
	}
	GetNextToken();

	return pStatement;
}

void CHtfeDesign::HtMemoryWriteVarInit(CHtfeIdent *pHier, CHtfeIdent *pIdent, string & cQueName, string & refDimStr, string & declDimStr,
	vector<CHtfeOperand *> indexList)
{
	CHtfeIdent *pFunction = pHier->FindHierFunction();

	if (!pIdent->IsHtMemoryWriteClockSeen()) {

		vector<int> refList(pIdent->GetDimenCnt(), 0);

		char buf[1024];
		do {
			string initDimStr;

			for (size_t i = 0; i < refList.size(); i += 1) {
				sprintf(buf, "[%d]", refList[i]);
				initDimStr += buf;
			}

			// initialize write variables
			char buf[1024];
			if (pIdent->GetType()->IsHtMrdBlockRam())
				sprintf(buf, "{ %s_WrAddr%s = 0; %s_WrData%s = 0; %s_WrEn%s = 0; %s_WrSel%s = false; }",
					cQueName.c_str(), initDimStr.c_str(),
					cQueName.c_str(), initDimStr.c_str(),
					cQueName.c_str(), initDimStr.c_str(),
					cQueName.c_str(), initDimStr.c_str() );
			else
				sprintf(buf, "{ %s_WrAddr%s = 0; %s_WrData%s = 0; %s_WrEn%s = 0; }",
					cQueName.c_str(), initDimStr.c_str(),
					cQueName.c_str(), initDimStr.c_str(),
					cQueName.c_str(), initDimStr.c_str() );

			OpenLine(buf);
			GetNextToken();

			// insert statements at beginning of block
			CHtfeStatement *pStatement = ParseCompoundStatement(pHier, false);

			// memory initialization statements are inserted at the head of a method's statement list
			//   for non-method functions, they are inserted into a list and at synthesis the list
			//    is inserted to the calling method's statement list.
			if (pFunction->IsMethod())
				pFunction->InsertStatementList(pStatement);
			else
				pFunction->InsertMemInitStatements(pStatement);

			if (g_pHtfeArgs->IsLvxEnabled()) {
				pStatement->GetExpr()->GetOperand2()->SetIsScX();		// WrIdx = X
				pStatement->GetNext()->GetExpr()->GetOperand2()->SetIsScX();	// WrData = X
			}

		} while (!pIdent->DimenIter(refList));

		pIdent->SetIsHtMemoryWriteClockSeen();
	}

	pIdent->AddWriter(pFunction);
}

void CHtfeDesign::HtMemoryReadVarInit(CHtfeIdent *pHier, CHtfeIdent *pIdent, string & cQueName, string & refDimStr, string & declDimStr,
	vector<CHtfeOperand *> indexList)
{
	CHtfeIdent *pFunction = pHier->FindHierFunction();

	if (!pIdent->IsHtMemoryReadClockSeen()) {

		vector<int> refList(pIdent->GetDimenCnt(), 0);

		char buf[1024];
		do {
			string initDimStr;

			for (size_t i = 0; i < refList.size(); i += 1) {
				sprintf(buf, "[%d]", refList[i]);
				initDimStr += buf;
			}

			// initialize write variables
			char buf[1024];
			if (pIdent->GetType()->IsHtMwrBlockRam())
				sprintf(buf, "{{ %s_RdSel%s = 0; %s_RdAddr%s = 0; }}",
					cQueName.c_str(), initDimStr.c_str(),
					cQueName.c_str(), initDimStr.c_str() );
			else
				sprintf(buf, "{{ %s_RdAddr%s = 0; }}",
					cQueName.c_str(), initDimStr.c_str() );

			OpenLine(buf);
			GetNextToken();

			// insert statements at beginning of block
			CHtfeStatement *pStatement = ParseCompoundStatement(pHier, false);

			// memory initialization statements are inserted at the head of a method's statement list
			//   for non-method functions, they are inserted into a list and at synthesis the list
			//    is inserted to the calling method's statement list.
			if (pFunction->IsMethod())
				pFunction->InsertStatementList(pStatement);
			else
				pFunction->InsertMemInitStatements(pStatement);

			if (g_pHtfeArgs->IsLvxEnabled())
				pStatement->GetExpr()->GetOperand2()->SetIsScX();				// RdIdx = X

		} while (!pIdent->DimenIter(refList));

		pIdent->SetIsHtMemoryReadClockSeen();
	}

	pIdent->AddReader(pFunction);
}

CHtfeOperand * CHtfeDesign::ParseHtMemoryExpr(CHtfeIdent *pHier, CHtfeIdent *pIdent, vector<CHtfeOperand *> &indexList)
{
	// parse a reference to a HtMemory variable and return an expression
	//   used for HtMemory routines that return a value (read_mem, read_in_use)

	if (GetToken() != tk_period || GetNextToken() != tk_identifier || GetNextToken() != tk_lparen) {
		ParseMsg(PARSE_ERROR, "expected ht_memory function");
		SkipTo(tk_semicolon);
		GetNextToken();
		return 0;
	}

	string refDimStr;
	for (size_t i = 0; i < pIdent->GetDimenCnt(); i += 1)
		refDimStr.append("[0]");

	string cQueName = pIdent->GetName();
	if (cQueName.substr(0,2) == "r_" || cQueName.substr(0,2) == "m_")
		cQueName.replace(0, 2, "c_");
	else
		cQueName.insert(0, "c_");

	CHtfeOperand *pOperand;
	CHtfeOperand * pRdSelOp = 0;

	if (GetString() == "read_mem") {

		pIdent->SetIsHtMemoryReadMemSeen();

		GetNextToken();

		if (pIdent->GetType()->IsHtMrdBlockRam())
			pRdSelOp = ParseExpression(pHier, true);

		if (GetToken() != tk_rparen)
			ParseMsg(PARSE_ERROR, "expected a )");

		string rdDataStr = cQueName + "_RdData" + refDimStr;

		if (pIdent->GetType()->IsHtMrdBlockRam())
			rdDataStr += ".m_mrd[0xdeadbeef]";

		GetNextToken();
		while (GetToken() == tk_period) {
			rdDataStr += ".";
			if (GetNextToken() != tk_identifier)
				ParseMsg(PARSE_ERROR, "expected an identifier");
			rdDataStr += GetString();

			GetNextToken();
			while (GetToken() == tk_lbrack) {
				//if (pIdent->GetDimenCnt() == dimIdx)
				//	break;

				GetNextToken();

				CConstValue idxValue;
				if (!ParseConstExpr(idxValue)) {
					ParseMsg(PARSE_ERROR, "Parsing '[ integer ]', expected contant index");
					SkipTo(tk_rbrack);
				}
				if (GetToken() != tk_rbrack) {
					ParseMsg(PARSE_ERROR, "Parsing '[ integer ]', expected ]");
					SkipTo(tk_rbrack);
				}
				GetNextToken();

				char buf[32];
				sprintf(buf, "[%d]", (int)idxValue.GetSint64());
				rdDataStr += buf;
			}			
		}
		rdDataStr += ")";

		OpenLine(rdDataStr);
		GetNextToken();

		pOperand = ParseExpression(pHier, false, true, false);
		pOperand->SetIndexList(indexList);

		if (pIdent->GetType()->IsHtMrdBlockRam()) {
			CScSubField * pSubField = pOperand->GetSubField();
			while (pSubField && (!pSubField->m_indexList[0]->IsConstValue() || pSubField->m_indexList[0]->GetConstValue().GetUint32() != 0xdeadbeef))
				pSubField = pSubField->m_pNext;
			Assert(pSubField);

			delete pSubField->m_indexList[0];
			pSubField->m_indexList[0] = pRdSelOp;
		}

	} else {
		ParseMsg(PARSE_ERROR, "unknown ht_memory member");
		SkipTo(tk_semicolon);
		GetNextToken();
		return 0;
	}

	GetNextToken();

	return pOperand;
}
