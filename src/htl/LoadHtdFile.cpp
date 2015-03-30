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

///////////////////////////
// Error message routine

void ErrorMsg(EMsgType msgType, CLineInfo const & lineInfo, const char *pFormat, va_list &args)
{
	char buf[1024];
	vsprintf(buf, pFormat, args);

	char const *pMsgType = 0;
	switch (msgType) {
	case Info:    pMsgType = "        "; break;
	case Warning: pMsgType = "Warning - "; CPreProcess::m_warningCnt += 1; break;
	case Error:   pMsgType = "Error - "; CPreProcess::m_errorCnt += 1; break;
	case Fatal:   pMsgType = "Fatal - "; break;
	}

	printf("%s(%d) : %s%s\n", lineInfo.m_fileName.c_str(), lineInfo.m_lineNum, pMsgType, buf);
	fflush(stdout);

	if (msgType == Fatal)
		ErrorExit();
}

void AssertMsg(char *file, int line)
{
	int argc;
	char const ** argv;
	g_appArgs.GetArgs(argc, argv);

	printf("Fatal : internal inconsistency detected\n   File: %s\n   Line: %d\n   Command Line:", file, line);

	for (int i = 0; i < argc; i += 1)
		printf(" %s", argv[i]);

	printf("\n");

	ErrorExit();
}

///////////////////////////
// HtdFile interface routines

void CDsnInfo::AddIncludeList(vector<CIncludeFile> const & includeList)
{
	m_includeList = includeList;
}

void CDsnInfo::AddDefine(void * pHandle, string defineName, string defineValue,
	string defineScope, string modName)
{
	vector<string> paramList;
	m_defineTable.Insert(defineName, paramList, defineValue, false, false, defineScope, modName);
	return;
}

void CDsnInfo::AddTypeDef(string name, string type, string width, string scope, string modName)
{
	AddTypeDef(name, type, width, false, scope, modName);
}

void CDsnInfo::AddTypeDef(string name, string type, string width, bool bInitWidth, string scope, string modName)
{
	m_typedefList.push_back(CTypeDef(name, type, width, scope, modName));
	if (bInitWidth)
		m_typedefList.back().m_width.InitValue(m_typedefList.back().m_lineInfo, false, 0);
}

CRecord * CDsnInfo::AddStruct(string structName, bool bCStyle, bool bUnion, EScope scope, bool bInclude, string modName)
{
	m_recordList.push_back(new CRecord(structName, bCStyle, bUnion, scope, bInclude, modName));
	return m_recordList.back();
}

CModule * CDsnInfo::AddModule(string name, EClkRate clkRate)
{
	m_modList.push_back(new CModule(name, clkRate, false));

	CModule * pModule = m_modList.back();

	if (g_appArgs.GetIqModName() == name)
		pModule->AddQueIntf("hif", "hifInQue", Pop, "5");

	if (g_appArgs.GetOqModName() == name)
		pModule->AddQueIntf("hif", "hifOutQue", Push, "5");

	return pModule;
}

CMifRd * CDsnInfo::AddReadMem(CModule * pModule, string queueW, string rspGrpId, string rspGrpW,
	string rspCntW, bool maxBw, bool bPause, bool bPoll, bool bMultiRd)
{
	CMif & mif = pModule->m_mif;

	// check if mif read interface was already added
	if (mif.m_bMifRd) {
		ParseMsg(Error, "AddReadMem already specified for module");
		return &mif.m_mifRd;
	}

	mif.m_bMif = true;
	mif.m_bMifRd = true;

	mif.m_mifRd.m_queueW = queueW;
	mif.m_mifRd.m_rspGrpId = rspGrpId;
	mif.m_mifRd.m_rspGrpW = rspGrpW;
	mif.m_mifRd.m_rspCntW = rspCntW;
	mif.m_mifRd.m_bMaxBw = maxBw;
	mif.m_mifRd.m_bPause = bPause;
	mif.m_mifRd.m_bPoll = bPoll;
	mif.m_mifRd.m_bMultiQwRdReq = bMultiRd;

	mif.m_mifRd.m_lineInfo = CPreProcess::m_lineInfo;

	if (pModule->m_memPortList.size() < 1) {
		pModule->m_memPortList.resize(1);
		pModule->m_memPortList[0] = new CModMemPort;
	}

	CModMemPort & modMemPort = *pModule->m_memPortList[0];

	modMemPort.m_bIsMem = true;
	modMemPort.m_bRead = true;

	return &mif.m_mifRd;
}

CMifWr * CDsnInfo::AddWriteMem(CModule * pModule, string queueW, string rspGrpId, string rspGrpW, string rspCntW,
	bool maxBw, bool bPause, bool bPoll, bool bReqPause, bool bMultiWr)
{
	CMif & mif = pModule->m_mif;

	// check if mif write interface was already added for 'mifId'
	if (mif.m_bMifWr) {
		ParseMsg(Error, "AddWriteMem already specified for module");
		return &mif.m_mifWr;
	}

	mif.m_bMif = true;
	mif.m_bMifWr = true;

	mif.m_mifWr.m_queueW = queueW;
	mif.m_mifWr.m_rspGrpId = rspGrpId;
	mif.m_mifWr.m_rspGrpW = rspGrpW;
	mif.m_mifWr.m_rspCntW = rspCntW;
	mif.m_mifWr.m_bMaxBw = maxBw;
	mif.m_mifWr.m_bPause = bPause;
	mif.m_mifWr.m_bPoll = bPoll;
	mif.m_mifWr.m_bReqPause = bReqPause;
	mif.m_mifWr.m_bMultiQwWrReq = bMultiWr;

	mif.m_mifWr.m_lineInfo = CPreProcess::m_lineInfo;

	if (pModule->m_memPortList.size() < 1) {
		pModule->m_memPortList.resize(1);
		pModule->m_memPortList[0] = new CModMemPort;
	}

	CModMemPort & modMemPort = *pModule->m_memPortList[0];

	modMemPort.m_bIsMem = true;
	modMemPort.m_bWrite = true;

	return &mif.m_mifWr;
}

void CDsnInfo::AddCall(void * pHandle, string funcName, bool bCall, bool bFork, string queueW, string dest)
{
	CModule * pModule = (CModule *)pHandle;
	pModule->m_cxrCallList.push_back(CCxrCall(funcName, bCall, bFork, queueW, dest, false));
}

void CDsnInfo::AddXfer(void * pHandle, string funcName, string queueW)
{
	CModule * pModule = (CModule *)pHandle;
	string dest = "auto";
	bool bCall = false;
	bool bFork = false;
	bool bXfer = true;
	pModule->m_cxrCallList.push_back(CCxrCall(funcName, bCall, bFork, queueW, dest, bXfer));
}

CCxrEntry * CDsnInfo::AddEntry(CModule * pModule, string funcName, string entryInstr, string reserve, bool &bHost)
{
	if (bHost) {
		bool bCall = true;
		bool bFork = false;
		AddCall(m_modList[0], funcName, bCall, bFork, string("0"), string("auto"));
	}

	return &pModule->AddEntry(funcName, entryInstr, reserve);
}

CCxrReturn * CDsnInfo::AddReturn(void * pHandle, string func)
{
	CModule * pModule = (CModule *)pHandle;
	pModule->m_cxrReturnList.push_back(CCxrReturn(func));
	return &pModule->m_cxrReturnList.back();
}

CStage * CDsnInfo::AddStage(CModule * pModule, string privWrStg, string execStg)
{
	CStage & stage = pModule->m_stage;

	stage.m_privWrStg = privWrStg;
	stage.m_execStg = execStg;
	stage.m_lineInfo = CPreProcess::m_lineInfo;

	return &stage;
}

CRecord * CDsnInfo::AddShared(void * pHandle, bool bReset)
{
	CModule * pModule = (CModule *)pHandle;
	pModule->m_bResetShared = bReset;
	return &pModule->m_shared;
}

void CDsnInfo::AddThreads(void * pHandle, string htIdW, string resetInstr, bool bPause)
{
	CModule * pModule = (CModule *)pHandle;
	pModule->AddThreads(htIdW, resetInstr, bPause);
}

void CDsnInfo::AddInstr(void * pHandle, string name)
{
	CModule * pModule = (CModule *)pHandle;
	pModule->m_instrList.push_back(name);
}

void CDsnInfo::AddMsgIntf(void * pHandle, string name, string dir, CType * pType, string dimen, string replCnt, string queueW, string reserve, bool autoConn)
{
	CModule * pModule = (CModule *)pHandle;
	pModule->m_msgIntfList.push_back(new CMsgIntf(name, dir, pType, dimen, replCnt, queueW, reserve, autoConn));
}

void CDsnInfo::AddTrace(void * pHandle, string name)
{
	CModule * pModule = (CModule *)pHandle;
	pModule->m_traceList.push_back(name);
}

void CDsnInfo::AddScPrim(void * pHandle, string scPrimDecl, string scPrimCall)
{
	CModule * pModule = (CModule *)pHandle;
	pModule->m_scPrimList.push_back(CScPrim(scPrimDecl, scPrimCall));
}

void CDsnInfo::AddPrimstate(void * pHandle, string type, string name, string include)
{
	CModule * pModule = (CModule *)pHandle;
	pModule->m_primStateList.push_back(CPrimState(type, name, include));
}

void CDsnInfo::AddBarrier(void * pHandle, string name, string barIdW)
{
	CModule * pModule = (CModule *)pHandle;
	pModule->m_barrierList.push_back(new CBarrier(name, barIdW));
}

void CDsnInfo::AddStream(void * pHandle, bool bRead, string &name, CType * pType, string &strmBw, string &elemCntW,
	string &strmCnt, string &memSrc, vector<int> &memPort, string &access, string &reserve, bool paired, bool bClose, CType * pTag, string &rspGrpW)
{
	CModule * pMod = (CModule *)pHandle;
	pMod->m_streamList.push_back(new CStream(bRead, name, pType, strmBw, elemCntW, strmCnt, memSrc, memPort, access, reserve, paired, bClose, pTag, rspGrpW));

	for (size_t i = 0; i < memPort.size(); i += 1) {
		while (pMod->m_memPortList.size() < (size_t)memPort[i] + 1) {
			pMod->m_memPortList.push_back(new CModMemPort);
			pMod->m_memPortList.back()->m_portIdx = (int)pMod->m_memPortList.size() - 1;
		}

		CModMemPort & modMemPort = *pMod->m_memPortList[memPort[i]];

		modMemPort.m_bIsStrm = true;
		modMemPort.m_bRead |= bRead;
		modMemPort.m_bWrite |= !bRead;
		modMemPort.m_bMultiQwRdReq |= bRead;
		modMemPort.m_bMultiQwWrReq |= !bRead;
		modMemPort.m_queueW = 5;
	}
}

void CDsnInfo::AddStencilBuffer(void * pHandle, string &name, CType * pType, vector<int> &gridSize,
	vector<int> &stencilSize, string &pipeLen)
{
	CModule * pMod = (CModule *)pHandle;
	pMod->m_stencilBufferList.push_back(new CStencilBuffer(name, pType, gridSize, stencilSize, pipeLen));

}

void * CDsnInfo::AddHostMsg(void * pHandle, HtdFile::EHostMsgDir msgDir, string msgName)
{
	CModule * pModule = (CModule *)pHandle;

	switch (msgDir) {
	case HtdFile::Inbound:
		pModule->m_bInHostMsg = true;
		break;
	case HtdFile::Outbound:
		pModule->m_ohm.m_bOutHostMsg = true;
		pModule->m_ohm.m_lineInfo = m_lineInfo;
		break;
	default:
		break;
	}

	return pModule->AddHostMsg(msgDir, msgName);
}

void CDsnInfo::AddHostData(void * pHandle, HtdFile::EHostMsgDir msgDir, bool bMaxBw)
{
	CModule * pModule = (CModule *)pHandle;

	switch (msgDir) {
	case HtdFile::Inbound:
		m_modList[0]->AddQueIntf(pModule->m_modName.AsStr(), "ihd", Push, "5", HtdFile::eDistRam, bMaxBw);
		pModule->AddQueIntf("hif", "ihd", Pop, "5", HtdFile::eDistRam, bMaxBw);
		break;
	case HtdFile::Outbound:
		m_modList[0]->AddQueIntf(pModule->m_modName.AsStr(), "ohd", Pop, "5");
		pModule->AddQueIntf("hif", "ohd", Push, "5");
		break;
	default:
		break;
	}
}

void CDsnInfo::AddGlobalVar(vector<CRam *> * pGlobalList, CType * pType, string name, string dimen1, string dimen2,
	string addr1W, string addr2W, string addr1, string addr2, string rdStg, string wrStg,
	bool bMaxIw, bool bMaxMw, ERamType ramType, bool bRead, bool bWrite)
{
	(*pGlobalList).push_back(new CRam(pType, name, dimen1, dimen2, addr1, addr2,
		addr1W, addr2W, rdStg, wrStg, bMaxIw, bMaxMw, ramType, bRead, bWrite));
}

void CDsnInfo::AddStageField(CStage * pStage, CType * pType, string name, string dimen1, string dimen2,
	string *pRange, bool bInit, bool bConn, bool bReset, bool bZero)
{
	bool bStageNums = pStage->AddStageField(pType, name, dimen1, dimen2, pRange, bInit, bConn, bReset, bZero);
	pStage->m_bStageNums |= bStageNums;
}

void CDsnInfo::AddStructField(CRecord * pRecord, CType * pType, string const & name, string bitWidth, string base, vector<CHtString> const &dimenList,
	bool bSrcRead, bool bSrcWrite, bool bMifRead, bool bMifWrite, HtdFile::ERamType ramType, int atomicMask)
{

}


void * CDsnInfo::AddMsgDst(void * pHandle, string var, string dataLsb, string addr1Lsb, string addr2Lsb,
	string idx1Lsb, string idx2Lsb, string field, string fldIdx1Lsb, string fldIdx2Lsb, bool bReadOnly)
{
	CHostMsg * pHostMsg = (CHostMsg *)pHandle;
	pHostMsg->m_msgDstList.push_back(CMsgDst(var, dataLsb, addr1Lsb, addr2Lsb,
		idx1Lsb, idx2Lsb, field, fldIdx1Lsb, fldIdx2Lsb, bReadOnly));
	return pHostMsg;
}

CMifWr * CDsnInfo::AddSrc(CMifWr * pMifWr, string const & name, CType * pType,
	string const & var, string const & memDst, CType * pWrType, string const & elemCntW)
{
	pMifWr->m_wrSrcList.push_back(CMifWrSrc(name, pType, var, memDst, pWrType, elemCntW));
	return pMifWr;
}

void CDsnInfo::AddDst(CMifRd * pMifRd, string const & name, string const & var,
	string const & field, string const & dataLsb, bool bMultiRd, string const & dstIdx,
	string const & memSrc, string const & atomic, CType * pRdType, string const & elemCntW)
{
	pMifRd->m_rdDstList.push_back(CMifRdDst(name, var, field, dataLsb, bMultiRd, dstIdx, memSrc, atomic, pRdType, elemCntW));
}

void CDsnInfo::AddDst(CMifRd * pMifRd, string name, string infoW, string stgCnt,
	bool bMultiRd, string memSrc, CType * pRdType)
{
	pMifRd->m_rdDstList.push_back(CMifRdDst(name, infoW, stgCnt, bMultiRd, memSrc, pRdType));
}
