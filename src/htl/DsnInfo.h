/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <assert.h>
#include "AppArgs.h"
#include "Lex.h"
#include "HtCode.h"
#include "HtString.h"
#include "CxrIntf.h"
#include "HtdFile.h"
#include "HtiFile.h"

struct CThreads;
struct CModule;
struct CRam;
struct CRamPort;
struct CRamPortField;
struct CDsnInfo;
struct CStruct;
struct CField;
struct CRamIntf;
struct CCxrParam;
struct CQueIntf;
struct CCxrEntry;
struct CNgvInfo;

class VA {
public:
	VA(char const * pFmt, ... ) {
		va_list args;
		va_start( args, pFmt );
		char buf[4096];
		vsprintf(buf, pFmt, args);
		m_str = string(buf);
	}
	VA(string &str) { m_str = str; }

	size_t size() { return m_str.size(); }
	operator string() { return m_str; }
	char const * c_str() { return m_str.c_str(); }

	bool operator == (char const * pStr) { return m_str == pStr; }

private:
	string m_str;
};

extern vector<CHtString> const g_nullHtStringVec;

void ErrorMsg(EMsgType msgType, CLineInfo & lineInfo, const char *pFormat, va_list & args);

inline int FindLg2(uint64_t value, bool bLg2ZeroEqOne=true, bool bFixLg2Pwr2=false) {
	if (bFixLg2Pwr2)
		value -= 1;
	int lg2;
	for (lg2 = 0; value != 0 && lg2 < 64; lg2 += 1)
		value >>= 1;
	return (lg2==0 && bLg2ZeroEqOne) ? 1 : lg2;
}

inline bool IsPowerTwo(uint64_t x) { return (x & (x-1)) == 0; }

struct CDimenList {
	CDimenList() { m_elemCnt = 0; }

	void InitDimen() {
		assert(m_dimenIndex.size() == 0);
		m_elemCnt = 1;
		for (size_t i = 0; i < m_dimenList.size(); i += 1) {
			CHtString & dimenStr = m_dimenList[i];
			m_dimenDecl += VA("[%d]", dimenStr.AsInt());
			m_dimenIndex += VA("[idx%d]", i+1);
			m_elemCnt *= dimenStr.AsInt();
			m_dimenTabs += "\t";
		}
	}

	void DimenListInit(CLineInfo & lineInfo) {
		for (size_t i = 0; i < m_dimenList.size(); i += 1)
			m_dimenList[i].InitValue(lineInfo, false, 0);
	}

	int GetDimen(size_t idx) const { // zero based index
		return m_dimenList.size() <= idx ? 0 : m_dimenList[idx].AsInt();
	}

	vector<CHtString>	m_dimenList;
	string				m_dimenTabs;
	string				m_dimenIndex;
	string				m_dimenDecl;
	size_t				m_elemCnt;
};

#define ATOMIC_INC 1
#define ATOMIC_SET 2
#define ATOMIC_ADD 4

struct CField : CDimenList {
	CField(string type, string name, string bitWidth, string base, string dimen1, string dimen2, bool bSrcRead,
		bool bSrcWrite, bool bMifRead, bool bMifWrite, HtdFile::ERamType ramType)
	{
		Init();

		m_type = type;
		m_name = name;
		m_bitWidth = bitWidth;
		m_base = base;
		if (dimen1.size() > 0)
			m_dimenList.push_back(dimen1);
		if (dimen2.size() > 0)
			m_dimenList.push_back(dimen2);
		m_bSrcRead = bSrcRead;
		m_bSrcWrite = bSrcWrite;
		m_bMifRead = bMifRead;
		m_bMifWrite = bMifWrite;
		m_ramType = ramType;
	}

	CField(string type, string name, string bitWidth, string base, vector<CHtString> const &dimenList, bool bSrcRead,
		bool bSrcWrite, bool bMifRead, bool bMifWrite, HtdFile::ERamType ramType, int atomicMask)
	{
		Init();

		m_type = type;
		m_name = name;
		m_bitWidth = bitWidth;
		m_base = base;
		m_dimenList = dimenList;
		m_bSrcRead = bSrcRead;
		m_bSrcWrite = bSrcWrite;
		m_bMifRead = bMifRead;
		m_bMifWrite = bMifWrite;
		m_ramType = ramType;
		m_atomicMask = atomicMask;
	}

	CField(string typeName, string fieldName, bool bIfDefHtv=false) {
		Init();

		m_type = typeName;
		m_name = fieldName;
		m_bIfDefHtv = bIfDefHtv;
	}

	CField(string hostType, string typeName, string fieldName, bool bIsUsed) {
		Init();

		m_hostType = hostType;
		m_type = typeName;
		m_name = fieldName;
		m_bIsUsed = bIsUsed;
	}

	CField(char *pName) {
		Init();

		m_name = pName;
	}

	CField(string typeName, string fieldName, string dimen1, string dimen2) {
		Init();

		m_type = typeName;
		m_name = fieldName;
		if (dimen1.size() > 0)
			m_dimenList.push_back(dimen1);
		if (dimen2.size() > 0)
			m_dimenList.push_back(dimen2);
	}

	CField(string type, string name, string dimen1, string dimen2, string rdSelW, string wrSelW,
		string addr1W, string addr2W, string queueW, HtdFile::ERamType ramType, string reset)
	{
		Init();

		m_type =type;
		m_name = name;
		if (dimen1.size() > 0)
			m_dimenList.push_back(dimen1);
		if (dimen2.size() > 0)
			m_dimenList.push_back(dimen2);
		m_rdSelW = rdSelW;
		m_wrSelW = wrSelW;
		m_addr1W = addr1W;
		m_addr2W = addr2W;
		m_queueW = queueW;
		m_ramType = ramType;
		m_reset = reset;
	}

	CField(string type, string name, string dimen1, string dimen2, string addr1W, string addr2W, string addr1Name, string addr2Name) {
		Init();

		m_type = type;
		m_name = name;
		if (dimen1.size() > 0)
			m_dimenList.push_back(dimen1);
		if (dimen2.size() > 0)
			m_dimenList.push_back(dimen2);
		m_addr1W = addr1W;
		m_addr2W = addr2W;
		m_addr1Name = addr1Name;
		m_addr2Name = addr2Name;
		m_queueW = "";
		m_ramType = HtdFile::eDistRam;
	}

	CField(string &type, string &name, string &dimen1, string &dimen2, CHtString &rngLow, CHtString &rngHigh,
		bool bInit, bool bConn, bool bReset, bool bZero)
	{
		Init();

		m_type = type;
		m_name = name;
		if (dimen1.size() > 0)
			m_dimenList.push_back(dimen1);
		if (dimen2.size() > 0)
			m_dimenList.push_back(dimen2);
		m_rngLow = rngLow;
		m_rngHigh = rngHigh;
		m_bInit = bInit;
		m_bConn = bConn;
		m_bReset = bReset;
		m_bZero = bZero;
	}

	// used for AddFuntion parameters
	CField(string &dir, string &type, string &name) {
		Init();

		m_dir = dir;
		m_type = type;
		m_name = name;
	}

private:
	void Init() {
		m_lineInfo = CPreProcess::m_lineInfo;

		m_pStruct = 0;
		m_pPrivGbl = 0;
		m_atomicMask = 0;

		m_bIfDefHtv = false;
		m_bOne1xReadIntf = false;
		m_bTwo1xReadIntf = false;
		m_bOne2xReadIntf = false;
		m_bOne1xWriteIntf = false;
		m_bTwo1xWriteIntf = false;
		m_bOne2xWriteIntf = false;
		m_bIhmReadOnly = false;
	}

public:
	string		m_hostType;
	string		m_dir;
	string		m_type;
	string		m_base;
	string		m_name;
	CHtString	m_bitWidth;
	CHtString	m_rdSelW;
	CHtString	m_wrSelW;
	CHtString	m_addr1W;
	CHtString	m_addr2W;
	CHtString	m_queueW;
	string		m_addr1Name;
	string		m_addr2Name;
	string		m_reset;
	bool		m_bSrcRead;
	bool		m_bSrcWrite;
	bool		m_bMifRead;
	bool		m_bMifWrite;
	bool		m_bIsUsed;
	HtdFile::ERamType	m_ramType;
	int			m_atomicMask;

	CHtString	m_rngLow;	// instruction stage low
	CHtString	m_rngHigh;	// instruction stage high
	bool		m_bInit;	// initialize first stage register
	bool		m_bConn;	// multiple stages are directly connected
	bool		m_bReset;	// reset stage registers
	bool		m_bZero;	// zero input stage
	bool		m_bIfDefHtv;	// field needs #ifndef _HTV

	int			m_rdAddr1W;
	int			m_rdAddr2W;

	bool		m_bIhmReadOnly;

	CRam *		m_pPrivGbl;
	CStruct *	m_pStruct;

	vector<CRamPortField>	m_readerList;
	vector<CRamPortField>	m_writerList;

	CLineInfo	m_lineInfo;

	// flags indicating internal ram access types
	bool		m_bOne1xReadIntf;
	bool		m_bTwo1xReadIntf;
	bool		m_bOne2xReadIntf;
	bool		m_bOne1xWriteIntf;
	bool		m_bTwo1xWriteIntf;
	bool		m_bOne2xWriteIntf;

	// internal ram field index
	int			m_gblFieldIdx;
};

struct CStruct {
	CStruct() : m_bSrcRead(false), m_bSrcWrite(false), m_bMifRead(false), m_bMifWrite(false),
		m_bCStyle(false), m_bUnion(false), m_bShared(false), m_bInclude(false), m_bNeedIntf(false), m_atomicMask(0)
	{
		m_lineInfo = CPreProcess::m_lineInfo;
	}
    CStruct(string structName, bool bCStyle, bool bUnion, EScope scope, bool bInclude, string modName)
		: m_bCStyle(bCStyle), m_bUnion(bUnion), m_bShared(false), m_scope(scope), m_bInclude(bInclude),
		m_bNeedIntf(false), m_structName(structName), m_modName(modName), m_atomicMask(0)
	{
		m_lineInfo = CPreProcess::m_lineInfo;
	}
	CStruct & AddGlobalField(string type, string name, string dimen1="", string dimen2="", bool bRead=true,
		bool bWrite=true, HtdFile::ERamType ramType = HtdFile::eDistRam) {

		CField field(type, name, "", "", dimen1, dimen2, bRead, bWrite, false, false, ramType);

		m_fieldList.push_back(field);

		return *this;
	}

	CStruct & AddStructField2(string type, string name, bool bIfDefHTV=false) {

		CField field(type, name, bIfDefHTV);

		m_fieldList.push_back(field);

		return *this;
	}

	CStruct & AddStructField(string type, string name, string bitWidth="", string base="", vector<CHtString> const &dimenList=g_nullHtStringVec,
		bool bSrcRead=true, bool bSrcWrite=true, bool bMifRead=false, bool bMifWrite=false, HtdFile::ERamType ramType = HtdFile::eDistRam, int atomicMask=0) {


		m_fieldList.push_back(CField(type, name, bitWidth, base, dimenList, bSrcRead, bSrcWrite, bMifRead, bMifWrite, ramType, atomicMask));

		m_bMifRead |= bMifRead;
		m_bMifWrite |= bMifWrite;

		// return reference to new struct for anonymous struct/unions
		if (name.size() == 0 && (type == "union" || type == "struct")) {
			CField & field = m_fieldList.back();
			field.m_pStruct = new CStruct();
			field.m_pStruct->m_bCStyle = true;
			field.m_pStruct->m_bUnion = type == "union";
			return *field.m_pStruct;
		} else
			return *(CStruct *)0;
	}

	CStruct & AddField(string type, string name, string dimen1, string dimen2) {

		CField field(type, name, dimen1, dimen2);

		m_fieldList.push_back(field);

		return *this;
	}

	CStruct & AddSharedField(string type, string name, string dimen1, string dimen2, string rdSelW, string wrSelW, 
		string addr1W, string addr2W, string queueW, HtdFile::ERamType ramType, string reset) {

		CField field(type, name, dimen1, dimen2, rdSelW, wrSelW, addr1W, addr2W, queueW, ramType, reset);

		m_fieldList.push_back(field);

		return *this;
	}

	CStruct & AddPrivateField(string type, string name, string dimen1, string dimen2,
		string addr1W, string addr2W, string addr1, string addr2) 
	{

		CField field(type, name, dimen1, dimen2, addr1W, addr2W, addr1, addr2);

		m_fieldList.push_back(field);

		return *this;
	}

	bool AddStageField(string &type, string &name, string &dimen1, string &dimen2,
		string *pRange, bool bInit, bool bConn, bool bReset, bool bZero) {

		CHtString rngLow = pRange[0];
		CHtString rngHigh = pRange[1];

		rngLow.InitValue(CPreProcess::m_lineInfo);
		rngHigh.InitValue(CPreProcess::m_lineInfo);

		if (rngLow.AsInt() < 1)
			CPreProcess::ParseMsg(Error, "minimum range value is 1");

		if (rngLow.AsInt() > rngHigh.AsInt())
			CPreProcess::ParseMsg(Error, "illegal range low value greater than high value");

		for (size_t fldIdx = 0; fldIdx < m_fieldList.size(); fldIdx += 1) {
			CField &field = m_fieldList[fldIdx];

			if (field.m_name == name && 
				(field.m_rngLow.AsInt() <= rngLow.AsInt() && rngLow.AsInt() <= field.m_rngHigh.AsInt() ||
				field.m_rngLow.AsInt() <= rngHigh.AsInt() && rngHigh.AsInt() <= field.m_rngHigh.AsInt())) {
				CPreProcess::ParseMsg(Error, "field '%s' was previously declared with overlapping ranges", name.c_str());

				return false;
			}
		}

		m_fieldList.push_back(CField(type, name, dimen1, dimen2, rngLow, rngHigh, bInit, bConn, bReset, bZero));

		return true;//rngHigh.AsInt() > 0 || !bNoReg;
	}

public:
	bool				m_bSrcRead;
	bool				m_bSrcWrite;
	bool				m_bMifRead;
	bool				m_bMifWrite;
	bool				m_bCStyle;
	bool				m_bUnion;
	bool				m_bShared;
	EScope				m_scope;
	bool				m_bInclude;
	bool				m_bNeedIntf;

	string				m_structName;
	string				m_modName;
	int					m_atomicMask;
	vector<CField>		m_fieldList;
	CLineInfo			m_lineInfo;
};

struct CRam : CStruct, CDimenList {
	CRam() {}

	CRam(string moduleName, string ramName, string addrName, string addrBits, HtdFile::ERamType ramType=HtdFile::eDistRam) {

		m_lineInfo = CPreProcess::m_lineInfo;
		m_modName = moduleName;
		m_gblName = ramName;
		m_addr1Name = addrName;
		m_addr1W = addrBits;
		m_pIntGbl = 0;
		m_bReplAccess = false;
		m_ramType = ramType;
		m_bGlobal = false;
		m_bPrivGbl = false;
		m_bShAddr = false;
	}

	CRam(string ramName, string addrName, string addrBits, bool bReplAccess, HtdFile::ERamType ramType=HtdFile::eDistRam) {

		m_lineInfo = CPreProcess::m_lineInfo;
		m_gblName = ramName;
		m_addr1Name = addrName;
		m_addr1W = addrBits;
		m_bReplAccess = bReplAccess;
		m_ramType = ramType;
		m_bGlobal = false;
		m_bPrivGbl = false;
		m_bShAddr = false;
	}

	CRam(string ramName, string addr1, string addr2, string addr1W, string addr2W, string dimen1, string dimen2, string &rdStg, string &wrStg) {

		m_lineInfo = CPreProcess::m_lineInfo;
		m_gblName = ramName;
		m_addr1Name = addr1;
		m_addr2Name = addr2;
		m_addr1W = addr1W;
		m_addr2W = addr2W;
		if (dimen1.size() > 0)
			m_dimenList.push_back(dimen1);
		if (dimen2.size() > 0)
			m_dimenList.push_back(dimen2);
		m_bReplAccess = false;
		m_ramType = HtdFile::eDistRam;
		m_bGlobal = false;
		m_rdStg = rdStg;
		m_wrStg = wrStg;
		m_bPrivGbl = false;
		m_bShAddr = false;
	}

	CRam(string &type, string &name, string &dimen1, string &dimen2, string &addr1, string &addr2,
		string &addr1W, string &addr2W, string &rdStg, string &wrStg, bool bMaxIw, bool bMaxMw,
		HtdFile::ERamType ramType) {

		m_lineInfo = CPreProcess::m_lineInfo;
		m_type = type;
		m_gblName = name;
		m_addr1Name = addr1;
		m_addr2Name = addr2;
		m_addr1W = addr1W;
		m_addr2W = addr2W;
		if (dimen1.size() > 0)
			m_dimenList.push_back(dimen1);
		if (dimen2.size() > 0)
			m_dimenList.push_back(dimen2);
		m_bReplAccess = false;
		m_ramType = HtdFile::eDistRam;
		m_bGlobal = false;
		m_rdStg = rdStg;
		m_wrStg = wrStg;
		m_bMaxIw = bMaxIw;
		m_bMaxMw = bMaxMw;
		m_ramType = ramType;
		m_bPrivGbl = false;
		m_bShAddr = false;
	}

public:
	CLineInfo			m_lineInfo;
	CHtString			m_modName;
	CHtString			m_gblName;
	HtdFile::ERamType	m_ramType;
	CRam *				m_pIntGbl;
	bool				m_bReplAccess;
	CModule *			m_pIntRamMod;
	CModule *			m_pExtRamMod;

	string				m_type;
	CHtString			m_addr0W;
	CHtString			m_addr1W;
	CHtString			m_addr2W;
	string				m_addr1Name;
	string				m_addr2Name;
	bool				m_bExtern;
	bool				m_bGlobal;
	CHtString			m_rdStg;
	CHtString			m_wrStg;
	bool				m_bMaxIw;
	bool				m_bMaxMw;
	bool				m_bPrivGbl;
	bool				m_bShAddr;

	CNgvInfo *			m_pNgvInfo;

	vector<CField>		m_allFieldList;	// list of fields from external and internal rams
	vector<CRam *>		m_extRamList;
	vector<CRamPort>	m_gblPortList;	// list of ram interfaces (internal and external)
};

struct CNgvModInfo {
	CNgvModInfo(CModule * pMod, CRam * pNgv) : m_pMod(pMod), m_pNgv(pNgv) {}
	CModule * m_pMod;
	CRam * m_pNgv;
};

struct CNgvInfo {
	CNgvInfo() : m_atomicMask(0) {}
	vector<CNgvModInfo> m_modInfoList;
	int m_atomicMask;
	string m_ngvWrType;
	HtdFile::ERamType m_ramType;
	bool m_bNgvWrDataClk2x;
	bool m_bNgvWrCompClk2x;
	bool m_bNgvAtomicFast;
	bool m_bNgvAtomicSlow;
	bool m_bNgvMaxSel;
	int m_ngvFieldCnt;
};

struct CQueIntf : CStruct {
	CQueIntf(string &modName, string &queName, HtdFile::EQueType queType, string queueW, HtdFile::ERamType ramType, bool bMaxBw) 
		: m_queType(queType), m_queueW(queueW), m_ramType(ramType), m_bMaxBw(bMaxBw) {

		m_queName = queName;
		m_modName = modName;
		m_lineInfo = CPreProcess::m_lineInfo;
	}

	CHtString m_modName;
	CHtString m_queName;
	HtdFile::EQueType m_queType;
	string m_queueW;
	HtdFile::ERamType m_ramType;
	bool m_bMaxBw;
	CLineInfo m_lineInfo;
};

class CStructElemIter {
public:
	struct CStack {
		CStack(CStruct * pStruct, size_t fieldIdx) : m_pStruct(pStruct), m_fieldIdx(fieldIdx) {}
		CStruct * m_pStruct;
		size_t m_fieldIdx;
		string m_heirName;
		vector<int> m_refList;
	};
public:
	// methods are defined in GenUtilities.cpp
	CStructElemIter(CDsnInfo * pDsnInfo, string typeName);

	bool end();
	CField & operator()();
	CField * operator->();
	void operator ++ (int);
	bool IsStructOrUnion();
	string GetHeirFieldName(bool bAddPreDot=true);

private:
	CDsnInfo * m_pDsnInfo;
	vector<CStack> m_stack;
};

struct CModIdx {
	CModIdx() : m_bIsUsed(false) {}
	CModIdx(CModule & mod, size_t idx, string modPath)
		: m_pMod(&mod), m_idx(idx), m_bIsUsed(true), m_modPath(modPath) {}

	CModule	*	m_pMod;
	size_t		m_idx;
	bool		m_bIsUsed;
	int			m_replCnt;
	string		m_modPath;
};

// Call/Transfer/Return interface info
struct CCxrCall {
	// Constructor used for AddCall
	CCxrCall(string &funcName, bool bCall, bool bFork, string &queueW, string &dest, bool bXfer)
		: m_funcName(funcName), m_bCall(bCall), m_bFork(bFork), m_queueW(queueW), m_dest(dest), m_bXfer(bXfer) {
			
		m_lineInfo = CPreProcess::m_lineInfo;
		m_bCallFork = bFork;
		m_bXferFork = false;
	}

public:
	CHtString		m_funcName;		// for Xfer and Call - name of target function
	bool			m_bCall;		// bCall and bFork can both be set
	bool			m_bFork;
	CHtString		m_queueW;
	string			m_dest;
	bool			m_bXfer;		// if bXfer then bCall and bFork are false

	CThreads *		m_pGroup;
	CCxrEntry *		m_pPairedEntry;
	CModIdx			m_pairedEntry;
	CModIdx			m_pairedFunc;
	vector<CModIdx>	m_pairedReturnList;
	int				m_forkCntW;

	bool			m_bCallFork;
	bool			m_bXferFork;

	CLineInfo		m_lineInfo;
};

struct CCxrCallList {
	void Insert(CModIdx &modIdx) {
		for (size_t i = 0; i < m_modIdxList.size(); i += 1)
			if (m_modIdxList[i].m_pMod == modIdx.m_pMod && m_modIdxList[i].m_idx == modIdx.m_idx)
				return;
		m_modIdxList.push_back(modIdx);
	}
	CCxrCall &GetCxrCall(size_t listIdx);
	size_t size() { return m_modIdxList.size(); }
	CModIdx & operator [] (size_t i) { return m_modIdxList[i]; }

	vector<CModIdx>		m_modIdxList;
};

struct CCxrEntry {
	CCxrEntry(string &funcName, string &entryInstr, string &reserve) 
		: m_funcName(funcName), m_entryInstr(entryInstr), m_reserveCnt(reserve) {
			
		m_lineInfo = CPreProcess::m_lineInfo;
		m_bCallFork = false;
		m_bXferFork = false;
	}

	CCxrEntry & AddParam(string hostType, string typeName, string paramName, bool bIsUsed) {
		CField param(hostType, typeName, paramName, bIsUsed);
		m_paramList.push_back(param);
		return *this;
	}

	CHtString	m_funcName;
	string		m_entryInstr;
	CHtString	m_reserveCnt;

	CModule *		m_pMod;
	CThreads *		m_pGroup;
	CCxrCallList	m_pairedCallList;

	vector<CField>	m_paramList;

	bool m_bCallFork;
	bool m_bXferFork;

	CLineInfo	m_lineInfo;
};

struct CCxrReturn {
	CCxrReturn(string &func) : m_funcName(func) {
			
		m_lineInfo = CPreProcess::m_lineInfo;
		m_bRtnJoin = false;
	}

	CCxrReturn & AddParam(string &hostType, string &typeName, string &paramName, bool bIsUsed) {
		CField param(hostType, typeName, paramName, bIsUsed);
		m_paramList.push_back(param);
		return *this;
	}

	CHtString		m_funcName;

	CCxrCallList	m_pairedCallList;

	CThreads *		m_pGroup;
	vector<CField>	m_paramList;

	bool m_bRtnJoin;

	CLineInfo		m_lineInfo;
};

struct CFunction {
	CFunction(string &type, string &name) : m_name(name), m_type(type) {
		m_lineInfo = CPreProcess::m_lineInfo;
	}

	CFunction & AddParam(string &dir, string &type, string &name) {
		CField param(dir, type, name);
		m_paramList.push_back(param);

		return *this;
	}

	string		m_name;
	string		m_type;

	vector<CField>	m_paramList;
	CLineInfo		m_lineInfo;
};

struct CThreads {
	CThreads() {
		m_rtnSelW = 0;
		m_rtnInstrW = 0;
		m_rtnHtIdW = 0;
		m_ramType = HtdFile::eDistRam;

		m_lineInfo = CPreProcess::m_lineInfo;
		m_bCallFork = false;
		m_bXferFork = false;
		m_bRtnJoin = false;
	}

	string		m_htIdIntfName;
	HtdFile::ERamType	m_ramType;
	string		m_resetInstr;
	int			m_rtnSelW;
	int			m_rtnInstrW;
	int			m_rtnHtIdW;
	string		m_rtnSelType;
	string		m_rtnInstrType;
	string		m_rtnHtIdType;
	CHtString	m_htIdW;
	string		m_htIdType;
	bool		m_bPause;
	CLineInfo	m_lineInfo;

	CRam		m_smCmd;	// list of fields in sm command (tid/state)
	CStruct		m_htPriv;	// list of field in tid info ram
	bool		m_bCallFork;
	bool		m_bXferFork;
	bool		m_bRtnJoin;
};

struct CHifMsg {
	CHifMsg(string msgName, string regName, string dataBase, string dataWidth, string idxBase, string idxWidth) :
		m_msgName(msgName), m_stateName(regName), m_dataLsb(dataBase), m_dataW(dataWidth),
		m_idxBase(idxBase), m_idxWidth(idxWidth) {

		if (m_stateName.substr(0,2) == "r_" || m_stateName.substr(0,2) == "m_")
			m_stateName = m_stateName.substr(2);

		m_bRamState = false;
	}

	CHifMsg(string msgName, string memName, string fieldName, string dataBase, string dataWidth, string idxBase, string idxWidth) :
		m_msgName(msgName), m_stateName(memName), m_fieldName(fieldName), m_dataLsb(dataBase), m_dataW(dataWidth),
		m_idxBase(idxBase), m_idxWidth(idxWidth) {

		if (m_stateName.substr(0,2) == "r_" || m_stateName.substr(0,2) == "m_")
			m_stateName = m_stateName.substr(2);

		m_bRamState = true;
	}

public:
	string	m_msgName;
	string	m_stateName;
	string	m_fieldName;
	string	m_dataLsb;
	string	m_dataW;
	string	m_idxBase;
	string	m_idxWidth;
	bool	m_bRamState;
};

struct CFunc {
	CFunc(string funcName, string funcRtnType) {
		m_funcName = funcName;
		m_rtnType = funcRtnType;
	}

public:
	string	m_funcName;
	string	m_rtnType;
};

struct CScPrim {
	CScPrim(string scPrimDecl, string scPrimFunc) {
		m_scPrimDecl = scPrimDecl;
		m_scPrimFunc = scPrimFunc;
	}

public:
	string	m_scPrimDecl;
	string	m_scPrimFunc;
};

struct CPrimState {
	CPrimState(string type, string name, string include) : m_type(type), m_name(name), m_include(include) {}

	string	m_type;
	string	m_name;
	string	m_include;
};

struct CMifRdPrefetch {
	CMifRdPrefetch(string pidW, string plenW, string mlenW) : m_pidW(pidW), m_plenW(plenW), m_mlenW(mlenW) {}

	string	m_pidW;
	string	m_plenW;
	string	m_mlenW;
};

struct CMifRdDst {
	CMifRdDst(string name, string var, string field, string dataLsb, bool bMultiRd, string dstIdx, string memSrc, string atomic, string rdType) :
		m_name(name), m_var(var), m_field(field), m_dataLsb(dataLsb), m_bMultiRd(bMultiRd), m_dstIdx(dstIdx), m_memSrc(memSrc), m_atomic(atomic), m_rdType(rdType) {
			m_lineInfo = CPreProcess::m_lineInfo;
			m_pSharedVar = 0;
			m_pIntGbl = 0;
			m_pExtRam = 0;
			m_pNgvRam = 0;

			m_fldW = 0;
			m_dstW = 0;
			m_varAddr0W = 0;
			m_varAddr1W = 0;
			m_varAddr2W = 0;
			m_varDimen1 = 0;
			m_varDimen2 = 0;
			m_fldDimen1 = 0;
			m_fldDimen2 = 0;
		}

	CMifRdDst(string name, string infoW, string stgCnt, bool bMultiRd, string memSrc, string rdType) :
		m_name(name), m_infoW(infoW), m_rsmDly(stgCnt), m_bMultiRd(bMultiRd), m_memSrc(memSrc), m_rdType(rdType) {
			m_lineInfo = CPreProcess::m_lineInfo;
			m_pSharedVar = 0;
			m_pIntGbl = 0;
			m_pExtRam = 0;
			m_pNgvRam = 0;
            m_atomic = "read";
			m_dstIdx = "rspIdx";

			m_fldW = 0;
			m_dstW = 0;
			m_varAddr0W = 0;
			m_varAddr1W = 0;
			m_varAddr2W = 0;
			m_varDimen1 = 0;
			m_varDimen2 = 0;
			m_fldDimen1 = 0;
			m_fldDimen2 = 0;
		}

	CHtString	m_name;
	string		m_var;
	string		m_field;
	CHtString	m_dataLsb;
	CHtString	m_infoW;
	CHtString	m_rsmDly;
    bool        m_bMultiRd;
    string      m_dstIdx;
    string      m_memSrc;
    string      m_atomic;
	CHtString	m_rdType;

	CLineInfo	m_lineInfo;

	CField *	m_pSharedVar;
	CRam *		m_pIntGbl;
	CRam *		m_pExtRam;
	CRam *		m_pNgvRam;

	int m_memSize;	// size in bits of read data

	int m_fldW;
	int m_dstW;		// width of info in read request tid
	int m_varAddr0W;
	int m_varAddr1W;
	int m_varAddr2W;
	int m_varDimen1;
	int m_varDimen2;
	int m_fldDimen1;
	int m_fldDimen2;
};

class CMifRd {
public:
    CMifRd() {
        m_bMultiRd = false;
        m_bPause = false;
        m_bPoll = false;
        m_bMaxBw = false;
        m_bRspGrpIdPriv = false;
    }

public:
	CHtString	m_queueW;
	CHtString	m_rspGrpId;
	CHtString	m_rspCntW;
	bool		m_bMaxBw;
    bool        m_bPause;
    bool        m_bPoll;

	CLineInfo	m_lineInfo;

	int			m_rspGrpIdW;
	bool		m_bRspGrpIdPriv;
    bool        m_bMultiRd;
	int			m_maxRsmDly;

	vector<CMifRdDst>		m_rdDstList;
};

struct CMifWrSrc {
	CMifWrSrc(string name, string var, string field, bool bMultiWr, string srcIdx, string memDst) :
		m_name(name), m_var(var), m_field(field), m_bMultiWr(bMultiWr), m_srcIdx(srcIdx), m_memDst(memDst) {
			m_lineInfo = CPreProcess::m_lineInfo;
			m_pSharedVar = 0;
			m_pIntGbl = 0;
			m_pExtRam = 0;
			m_pNgvRam = 0;

			m_fldW = 0;
			m_srcW = 0;
			m_varAddr0W = 0;
			m_varAddr1W = 0;
			m_varAddr2W = 0;
			m_varDimen1 = 0;
			m_varDimen2 = 0;
			m_fldDimen1 = 0;
			m_fldDimen2 = 0;

			m_bGblVarP3 = false;
			m_bGblVarP4 = false;
		}

	CHtString	m_name;
	string		m_var;
	string		m_field;
    bool        m_bMultiWr;
    string      m_srcIdx;
    string      m_memDst;

	CLineInfo	m_lineInfo;

	CField *	m_pSharedVar;
	CRam *		m_pIntGbl;
	CRam *		m_pExtRam;
	CRam *		m_pNgvRam;
	CField *	m_pField;

	int m_fldW;
	int m_srcW;		// width of info in write request tid
	int m_varAddr0W;
	int m_varAddr1W;
	int m_varAddr2W;
	int m_varDimen1;
	int m_varDimen2;
	int m_fldDimen1;
	int m_fldDimen2;

	bool m_bGblVarP3;
	bool m_bGblVarP4;
};

struct CMifWr {
public:
    CMifWr() {
		m_bMultiWr = false;
        m_bRspGrpIdPriv = false;
        m_bMaxBw = false;
        m_bPause = false;
        m_bPoll = false;
		m_bReqPause = false;

		m_rspGrpIdW = 0;
    }

public:
	CHtString	m_queueW;
	CHtString	m_rspGrpId;
	CHtString	m_rspCntW;
	bool		m_bMaxBw;
    bool        m_bPause;
    bool        m_bPoll;
	bool		m_bReqPause;

	CLineInfo	m_lineInfo;

	int			m_rspGrpIdW;
	bool		m_bRspGrpIdPriv;
    bool        m_bMultiWr;

	vector<CMifWrSrc>		m_wrSrcList;
};

class CMif {
public:
	CMif() {
		m_queueW = 0;

		m_bMif = false;
		m_bMifRd = false;
		m_bMifWr = false;
	}

public:
	int		m_queueW;

	bool	m_bMif;
	bool	m_bMifRd;
	bool	m_bMifWr;

	bool	m_bSingleReqMode;
	int		m_maxDstW;
	int		m_maxSrcW;

	CMifRd	m_mifRd;
	CMifWr	m_mifWr;
};

class COutHostMsg {
public:
	COutHostMsg() {
		m_bOutHostMsg = false;
	}

	bool		m_bOutHostMsg;
	CLineInfo	m_lineInfo;
};

struct CMsgDst {
	CMsgDst(string &var, string &dataLsb, string &addr1Lsb, string &addr2Lsb, string &idx1Lsb,
		string &idx2Lsb, string &field, string &fldIdx1Lsb, string &fldIdx2Lsb, bool bReadOnly)
		: m_var(var), m_dataLsb(dataLsb), m_addr1Lsb(addr1Lsb), m_addr2Lsb(addr2Lsb), m_idx1Lsb(idx1Lsb),
			m_idx2Lsb(idx2Lsb), m_field(field), m_fldIdx1Lsb(fldIdx1Lsb), m_fldIdx2Lsb(fldIdx2Lsb), m_bReadOnly(bReadOnly) {
	
		m_lineInfo = CPreProcess::m_lineInfo;

		m_pShared = 0;
		m_pField = 0;
	}

	string m_var;
	CHtString m_dataLsb;
	CHtString m_addr1Lsb;
	CHtString m_addr2Lsb;
	CHtString m_idx1Lsb;
	CHtString m_idx2Lsb;
	string m_field;
	CHtString m_fldIdx1Lsb;
	CHtString m_fldIdx2Lsb;
	bool m_bReadOnly;

	CField * m_pShared;
	CField const * m_pField;

	CLineInfo m_lineInfo;
};

struct CMsgIntf {
	CMsgIntf(string name, string dir, string type, string dimen, string replCnt, string queueW, string reserve, bool autoConn) 
		: m_name(name), m_dir(dir), m_type(type), m_typeIntf(type), m_dimen(dimen),
		m_fanCnt(replCnt), m_queueW(queueW), m_reserve(reserve), m_bAutoConn(autoConn)
	{
		m_lineInfo = CPreProcess::m_lineInfo;
		m_pIntfList = 0;
		m_bAeConn = false;
		m_bInboundQueue = false;
	}

	string	m_name;
	string	m_dir;
	string	m_type;
	string	m_typeIntf;
	CHtString m_dimen;
	CHtString m_fanCnt;	// fanin/fanout
	CHtString m_queueW;
	CHtString m_reserve;
	bool m_bAutoConn;

	int			m_srcFanout;
	int			m_srcReplCnt;
	HtdFile::EClkRate	m_srcClkRate;
	bool		m_bInboundQueue;
	vector<vector<HtiFile::CMsgIntfConn *> > m_msgIntfInstList;
	bool m_bAeConn;

	vector<CMsgIntf *> * m_pIntfList;

	CHtString	m_outModName;	// module name with output interface

	CLineInfo m_lineInfo;
};

struct CHostMsg {
	CHostMsg(HtdFile::EHostMsgDir msgDir, string msgName) : m_msgDir(msgDir), m_msgName(msgName) {}

	HtdFile::EHostMsgDir		m_msgDir;
	string			m_msgName;
	vector<CMsgDst>	m_msgDstList;
};

struct CBarrier {
	CBarrier(string name, string barIdW) : m_name(name), m_barIdW(barIdW) {
		m_lineInfo = CPreProcess::m_lineInfo;
	}

	CHtString	m_name;
	CHtString	m_barIdW;

	CLineInfo	m_lineInfo;
};

struct CStream {
	CStream(bool bRead, string &name, string &type, string &strmBw, string &elemCntW, string &strmCnt,
		string &memSrc, vector<int> &memPort, string &access, string &reserve, bool paired, bool bClose, string &tag, string &rspGrpW)
		: m_bRead(bRead), m_name(name), m_type(type), m_strmBw(strmBw), m_elemCntW(elemCntW), m_strmCnt(strmCnt),
		m_memSrc(memSrc), m_memPort(memPort), m_access(access), m_reserve(reserve),
		m_paired(paired), m_bClose(bClose), m_tag(tag), m_rspGrpW(rspGrpW)
	{
		m_lineInfo = CPreProcess::m_lineInfo;
	}

	bool		m_bRead;
	CHtString	m_name;
	CHtString	m_type;
	CHtString	m_strmBw;
	CHtString	m_elemCntW;
	CHtString	m_strmCnt;
	CHtString	m_memSrc;
	vector<int>	m_memPort;
	CHtString	m_access;
	CHtString	m_reserve;
	bool		m_paired;
	bool		m_bClose;
	string		m_tag;
	CHtString	m_rspGrpW;

	int			m_elemBitW;	// calulated from m_type
	int			m_elemByteW;
	int			m_qwElemCnt;
	int			m_strmCntW;
	int			m_bufFull;
	int			m_bufPtrW;
	vector<int>	m_arbRr;

	CLineInfo	m_lineInfo;
};

struct CStencilBuffer {
	CStencilBuffer(string &name,string & type, vector<int> &gridSize, 
		vector<int> &stencilSize, string &pipeLen)
		: m_name(name), m_type(type), m_gridSize(gridSize), m_stencilSize(stencilSize), m_pipeLen(pipeLen)
	{
		m_lineInfo = CPreProcess::m_lineInfo;
	}

	string m_name;
	string m_type;
	vector<int> m_gridSize;
	vector<int> m_stencilSize;
	CHtString m_pipeLen;

	CLineInfo m_lineInfo;
};

struct CMemPortStrm {
	CMemPortStrm(CStream * pStrm, int strmIdx, bool bRead) : m_pStrm(pStrm), m_strmIdx(strmIdx), m_bRead(bRead) {}

	CStream * m_pStrm;
	int m_strmIdx;
	bool m_bRead;
};

struct CModMemPort {
	CModMemPort() : m_bIsMem(false), m_bIsStrm(false), m_bRead(false), m_bWrite(false), m_bMultiRd(false), m_bMultiWr(false), m_queueW(0)
	{
		m_rdStrmCnt = 0;
		m_wrStrmCnt = 0;
		m_portIdx = 0;
	}

	bool m_bIsMem;
	bool m_bIsStrm;
	bool m_bRead;
	bool m_bWrite;
	bool m_bMultiRd;
	bool m_bMultiWr;
	int m_queueW;

	int m_rdStrmCnt;	
	int m_rdStrmCntW;
	int m_wrStrmCnt;	
	int m_wrStrmCntW;

	vector<CMemPortStrm> m_strmList;
	int m_portIdx;
};

struct CInstanceParams {
	CInstanceParams(int instId, vector<int> &memPortList, int freq) : m_instId(instId), m_memPortList(memPortList)
	{ m_lineInfo = CPreProcess::m_lineInfo; }

	CInstanceParams() {
		m_instId = -1;
		m_replCnt = -1;
	}

	int m_instId;
	vector<int> m_memPortList;
	int m_replCnt;

	CLineInfo m_lineInfo;
};

// Instance info for a module
struct CModInst {
	CModInst(){}
	CModInst(CModule *pMod, string & modPath, CInstanceParams &modInstParams, int cxrSrcCnt, int replCnt=1, int replId=0)
		: m_pMod(pMod), m_instParams(modInstParams), m_cxrSrcCnt(cxrSrcCnt), m_replCnt(replCnt), m_replId(replId) {
		m_modPaths.push_back(modPath);
	}

	CModule *			m_pMod;
	CInstanceParams		m_instParams;
	CHtString			m_instName;		// Instance name
	vector<string>		m_modPaths;		// module instance path name if specified in instance file
	int					m_cxrSrcCnt;
	vector<CCxrIntf>	m_cxrIntfList;	// List of interfaces for instance
	int					m_replCnt;
	int					m_replId;
};

struct CStage : CStruct {
	CHtString	m_privWrStg;
	CHtString	m_execStg;
	CLineInfo	m_lineInfo;
	bool		m_bStageNums;
};

struct CModule {
	CModule(string modName, HtdFile::EClkRate clkRate, bool bIsUsed) {
		m_modName = modName;
		m_clkRate = clkRate;

		m_instName = "inst";

		m_bHasThreads = false;
		m_bRtnJoin = false;
		m_bCallFork = false;
		m_bXferFork = false;
		m_bHostIntf = false;
		m_bContAssign = false;
		m_bIsUsed = bIsUsed;
		m_bActiveCall = false;
		m_bInHostMsg = false;
		m_stage.m_bStageNums = false;
		m_resetInstrCnt = 0;
		m_maxRtnReplCnt = 0;
		m_phaseCnt = 0;
		m_cxrEntryReserveCnt = 0;
		m_bResetShared = true;
		m_rsmSrcCnt = 0;
	}

	void AddHostIntf() {
		m_bHostIntf = true;
	}

	void AddHtFunc(string funcName, string funcRtnType) {
		m_htFuncList.push_back(CFunc(funcName, funcRtnType));
	}

	CCxrEntry & AddEntry(string &funcName, string &entryInstr, string &reserve) {
		m_cxrEntryList.push_back(new CCxrEntry(funcName, entryInstr, reserve));
		m_cxrEntryList.back()->m_pGroup = & m_threads;

		return *m_cxrEntryList.back();
	}

	vector<CRam *> & AddGlobal() {
		return m_globalVarList;
	}

	CRam & AddExtRam(string ramName, string addr1, string addr2, string addr1W, string addr2W, string dimen1, string dimen2, string &rdStg, string &wrStg) {
		CRam ram(ramName, addr1, addr2, addr1W, addr2W, dimen1, dimen2, rdStg, wrStg);
		m_extRamList.push_back(ram);
		return m_extRamList.back();
	}

	CRam & AddIntRam(string ramName, string addrName, string addrBits, bool bRepAccess=false, HtdFile::ERamType ramType=HtdFile::eDistRam) {
		CRam * pRam = new CRam(ramName, addrName, addrBits, bRepAccess, ramType);
		m_intGblList.push_back(pRam);
		return *m_intGblList.back();
	}

	CRam & AddIntRam(string ramName, string addr1, string addr2, string addr1W, string addr2W, string dimen1, string dimen2, string &rdStg, string &wrStg) {
		CRam * pRam = new CRam(ramName, addr1, addr2, addr1W, addr2W, dimen1, dimen2, rdStg, wrStg);
		m_intGblList.push_back(pRam);
		return *m_intGblList.back();
	}

	CQueIntf & AddQueIntf(string modName, string queName, HtdFile::EQueType queType, string queueW, HtdFile::ERamType ramType=HtdFile::eDistRam, bool bMaxBw=false) {
		CQueIntf queIntf(modName, queName, queType, queueW, ramType, bMaxBw);
		m_queIntfList.push_back(queIntf);

		return m_queIntfList.back();
	}

	CHostMsg * AddHostMsg(HtdFile::EHostMsgDir msgDir, string msgName) {
		for (size_t msgIdx = 0; msgIdx < m_hostMsgList.size(); msgIdx += 1)
			if (m_hostMsgList[msgIdx].m_msgName == msgName) {
				if (m_hostMsgList[msgIdx].m_msgDir != msgDir)
					m_hostMsgList[msgIdx].m_msgDir = HtdFile::InAndOutbound;
				return &m_hostMsgList.back();
			}

		m_hostMsgList.push_back(CHostMsg(msgDir, msgName));
		return &m_hostMsgList.back();
	}

	void AddThreads(string htIdW, string resetInstr, bool bPause=false) {
		if (m_bHasThreads) {
			CPreProcess::ParseMsg(Error, "module %s already has a thread group", m_modName.Lc().c_str());
				return;
		}

		m_bHasThreads = true;
		m_threads.m_htIdW = htIdW;
		m_threads.m_resetInstr = resetInstr;
		m_threads.m_bPause = bPause;
		m_threads.m_lineInfo = CPreProcess::m_lineInfo;

		if (resetInstr.size() > 0)
			m_resetInstrCnt += 1;
	}

	CStruct & AddPrivate() {
		return m_threads.m_htPriv;
	}

public:
	CHtString		m_modName;

	string			m_smTidName;
	string			m_smStateName;

	string			m_instName;

	int				m_instrW;

	HtdFile::EClkRate	m_clkRate;

	bool			m_bNeed2xClk;
	bool			m_bNeed2xReset;

	bool			m_bHostIntf;
	bool			m_bMultiThread;
	bool			m_bHtId;			// at least one group has non-zero width htId
	int				m_tsStg;
	int				m_wrStg;
	int				m_execStg;
	bool			m_bContAssign;
	bool			m_bIsUsed;
	bool			m_bActiveCall;
	bool			m_bRtnJoin;
	bool			m_bCallFork;
	bool			m_bXferFork;
	bool			m_bHtIdInit;
	int				m_instIdCnt;
	int				m_maxRtnReplCnt; // maximum replication count for destination of return links
	size_t			m_rsmSrcCnt;

	CMif			m_mif;
	int				m_resetInstrCnt;	// number of groups with reset instructions
	int				m_cxrEntryReserveCnt;	// sum of AddEntry reserve values

	bool			m_bHasThreads;
	CThreads			m_threads;

	bool				m_gblDistRam;
	bool				m_gblBlockRam;

	vector<CModInst>	m_modInstList;		// module instance list
	vector<CCxrCall>	m_cxrCallList;
	vector<CCxrEntry *>	m_cxrEntryList;
	vector<CCxrReturn>	m_cxrReturnList;
	vector<CRam *>		m_globalVarList;
	vector<CRam>		m_extRamList;
	vector<CRamPort>	m_extInstRamList;	// list of outbound ram interfaces
	vector<CRam *>		m_intGblList;
	vector<CQueIntf>	m_queIntfList;
	vector<CRamPort>	m_intInstRamList;
	vector<string>		m_instrList;		// instruction list
	vector<CFunc>		m_htFuncList;
	vector<CScPrim>		m_scPrimList;
	vector<CPrimState>	m_primStateList;
	vector<string>		m_traceList;
	vector<CHostMsg>	m_hostMsgList;
	vector<CMsgIntf *>	m_msgIntfList;
	vector<CBarrier *>	m_barrierList;
	vector<CFunction>	m_funcList;
	vector<CStream *>	m_streamList;
	vector<CModMemPort *> m_memPortList;
	vector<CStencilBuffer *> m_stencilBufferList;

	CStage				m_stage;
	CStruct				m_global;
	CStruct				m_shared;
	bool				m_bResetShared;
	string				m_smInitState;
	string				m_instrType;
	bool				m_bInHostMsg;
	int					m_phaseCnt;

	COutHostMsg			m_ohm;
};

struct CDefine {
	CDefine(string const &defineName, vector<string> const & paramList, string const &defineValue,
		bool bPreDefined, bool bHasParamList=false, string scope = string("unit"), string modName = string(""))
		: m_name(defineName), m_paramList(paramList), m_value(defineValue), m_scope(scope),
		m_modName(modName),  m_bPreDefined(bPreDefined), m_bHasParamList(bHasParamList) {}

public:
	string	m_name;
	vector<string> m_paramList;
	string	m_value;
	string	m_scope;
	string	m_modName;
	bool	m_bPreDefined;
	bool	m_bHasParamList;
};

class CDefineTable {
public:
	void Insert(string const &defineName, vector<string> const & paramList, string const &defineValue,
			bool bPreDefined=false, bool bHasParamList=false, string defineScope = string("unit"), string modName = string(""));
	bool FindStringValue(CLineInfo const &lineInfo, const char * &pValueStr, int &rtnValue, bool & bIsSigned);
	bool FindStringValue(CLineInfo const &lineInfo, const string &name, int &rtnValue, bool & bIsSigned) {
		char const * pValueStr = name.c_str();
		return FindStringValue(lineInfo, pValueStr, rtnValue, bIsSigned);
	}
	bool FindStringIsSigned(const string &name) {
		CLineInfo lineInfo;
		int rtnValue;
		bool bIsSigned = true;

		if (name.size() == 0)
			return true;

		if (!FindStringValue(lineInfo, name, rtnValue, bIsSigned)) {
			CPreProcess::ParseMsg(Error, lineInfo, "unable to determine value for '%s'", name.c_str());
		}

		return bIsSigned;
	}

	size_t size() { return m_defineList.size(); }
	CDefine & operator [] (size_t i) { return m_defineList[i]; }

private:

	bool FindStringInTable(string &name, string &value);
	bool ParseExpression(CLineInfo const &lineInfo, const char * &pPos, int &rtnValue, bool &bIsSigned);
	void EvaluateExpression(EToken tk, vector<int> &operandStack, vector<int> &operatorStack);
	int GetTokenPrec(EToken tk);
	EToken GetNextToken(const char *&pPos);
	EToken GetToken() { return m_tk; }
	int GetTokenValue();
	void SkipSpace(const char *&pPos);
	void SkipTypeSuffix(const char * &pPos);
	string &GetTokenString() { return m_tkString; }
	bool GetIsSigned() { return m_bIsSigned; }

private:
	vector<CDefine>		m_defineList;

	EToken				m_tk;
	string				m_tkString;
	bool				m_bIsSigned;
};

struct CTypeDef {
	CTypeDef(string &name, string &type, string &width,
			string scope = string("unit"),
			string modName = string(""))
		: m_name(name), m_type(type), m_width(width),
			m_scope(scope), m_modName(modName) {
			    m_lineInfo = CPreProcess::m_lineInfo;
		}

public:
	string m_name;
	string m_type;
	CHtString m_width;
	string	m_scope;
	string	m_modName;

	CLineInfo m_lineInfo;
};

enum EType { Scalar, Array, Queue };

struct CType {
	CType(EType type, string &typeName, string &baseType, string &width1, string width2="")
		: m_type(type), m_typeName(typeName), m_baseType(baseType), m_width1(width1), m_width2(width2) {}

public:
	EType	m_type;
	string m_typeName;
	string m_baseType;
	string m_width1;
	string m_width2;
};

// info to instanciate a mif instance
struct CMifInst {
	CMifInst(int mifId, bool bMifRead, bool bMifWrite) : m_mifId(mifId), m_bMifRead(bMifRead), m_bMifWrite(bMifWrite) {}

	int m_mifId;
	bool m_bMifRead;
	bool m_bMifWrite;
};
struct CRdAddr {
	CRdAddr(string &addr1Name, string &addr2Name, HtdFile::ERamType ramType, int rdStg, string addrW)
		: m_addr1Name(addr1Name), m_addr2Name(addr2Name), m_ramType(ramType), m_rdStg(rdStg), m_addrW(addrW) {}

	bool operator== (CRdAddr const & rhs) {
		return m_addr1Name == rhs.m_addr1Name && m_addr2Name == rhs.m_addr2Name && m_ramType == rhs.m_ramType
			&& m_rdStg == rhs.m_rdStg && m_addrW == rhs.m_addrW;
	}

	string m_addr1Name;
	string m_addr2Name;
	HtdFile::ERamType m_ramType;
	int m_rdStg;
	string m_addrW;
	string m_rdAddrName;
};

struct CInstanceInfo {
	CInstanceInfo(string &srcModName, vector<CInstanceParams> &srcParamsList, string &dstModName, vector<CInstanceParams> &dstParamsList)
		: m_srcModName(srcModName), m_srcParamsList(srcParamsList), m_dstModName(dstModName), m_dstParamsList(dstParamsList) {}

	CHtString	m_srcModName;
	vector<CInstanceParams>	m_srcParamsList;
	CHtString	m_dstModName;
	vector<CInstanceParams>	m_dstParamsList;
};

struct CParamList {
	const char *m_pName;
	void *		m_pValue;
	bool		m_bRequired;
	EToken		m_token;
	int			m_deprecatedPair;
	bool		m_bPresent;
};

struct CBramTarget {
	string		m_name;
	int			m_depth;
	int			m_width;
	int			m_copies;
	int			m_brams;
	float		m_slicePerBramRatio;
	HtdFile::ERamType *	m_pRamType;
	string		m_modName;
	string		m_varType;
};

struct CBasicType {
	CBasicType(string name, int width, bool bIsSigned) : m_name(name), m_width(width), m_bIsSigned(bIsSigned) {}

	string		m_name;
	int			m_width;
	bool		m_bIsSigned;
};

enum EVcdType { eVcdUser, eVcdAll, eVcdNone };

struct CDsnInfo : HtiFile, HtdFile, CLex {
	friend class HtiFile;

	CDsnInfo() {
		HtdFile::setDsnInfo(this);
		HtiFile::setLex(this);
		HtdFile::setLex(this);

        // fill in command line defined names
        for (int i = 0; i < g_appArgs.GetPreDefinedNameCnt(); i += 1)
            InsertPreProcessorName(g_appArgs.GetPreDefinedName(i), g_appArgs.GetPreDefinedValue(i));

		AddDesign(g_appArgs.GetUnitName(), "hif");
		AddPerfMon(g_appArgs.IsPerfMonEnabled());
		AddRndRetry(g_appArgs.IsRndRetryEnabled());
		AddRndInit(g_appArgs.IsRndInitEnabled());

		CModule & hifMod = AddModule(new CModule("hif", eClk1x, true));

		char sHostHtIdW[32];
		sprintf(sHostHtIdW, "%d", g_appArgs.GetHostHtIdW());

		AddDefine(&hifMod, "HIF_HTID_W", sHostHtIdW);

		hifMod.AddThreads("HIF_HTID_W", "");
		hifMod.AddHostIntf();

		bool bMifMultiRdSupport = g_appArgs.GetCoproc() != hc2 && g_appArgs.GetCoproc() != hc2ex &&
					  g_appArgs.GetCoproc() != wx690 && g_appArgs.GetCoproc() != wx2k;
		AddReadMem(&hifMod, "5", "", "5", false, true, false, bMifMultiRdSupport);
		AddWriteMem(&hifMod, "5", "", "5", false, true, false, false, true);

		if (g_appArgs.GetEntryName().size() > 0) {
			bool bCall = true;
			bool bFork = false;
			AddCall(&hifMod, g_appArgs.GetEntryName(), bCall, bFork, string("0"), string("auto"));
		}

		if (g_appArgs.GetIqModName().size() > 0)
			hifMod.AddQueIntf(g_appArgs.GetIqModName(), "hifInQue", Push, "5");

		if (g_appArgs.GetOqModName().size() > 0)
			hifMod.AddQueIntf(g_appArgs.GetOqModName(), "hifOutQue", Pop, "5");

		AddDefine(&hifMod, "HIF_INSTR_W", "0");

		CHtString::SetDefineTable( &m_defineTable );

		string name = "MEM_ADDR_W";
		string value = "48";
		vector<string> paramList;
		// jacobi depends on this, do not generate in code, it is in MemRdWrIntf.h
		m_defineTable.Insert(name, paramList, value, true, false, string("auto"));

		m_htModId = 0;
		m_maxInstId = 0;
		m_maxReplCnt = 0;
	}

	virtual ~CDsnInfo() {}

	// Routines called by HtdFile::loadHtdFile
	void AddIncludeList(vector<CIncludeFile> const & includeList);
	void AddDefine(void * pModule, string name, string value, string scope = "unit", string modName = "");
	void AddTypeDef(string name, string type, string scope, string modName, string width);

	void * AddModule(string name, EClkRate clkRate);
	void * AddStruct(string name, bool bCStyle, bool bUnion, EScope scope, bool bInclude=false, string modName = "");

	void * AddReadMem(void * pModule, string queueW, string rspGrpIdW, string rspCntW, bool maxBw, bool bPause, bool bPoll, bool bMultiRd=false);
	void * AddWriteMem(void * pModule, string queueW, string rspGrpId, string rspCntW, bool maxBw, bool bPause, bool bPoll, bool bReqPause, bool bMultiWr=false);
	void AddCall(void * pModule, string funcName, bool bCall, bool bFork, string queueW, string dest);
	void AddXfer(void * pModule, string funcName, string queueW);
	void * AddEntry(void * pModule, string funcName, string entryInstr, string reserve, bool &bHost);
	void * AddReturn(void * pModule, string func);
	void * AddStage(void * pModule, string privWrStg, string execStg );
	void * AddShared(void * pModule, bool bReset);
	void * AddPrivate(void * pModule );
	void * AddFunction(void * pModule, string type, string name);
	void AddThreads(void * pModule, string htIdW, string resetInstr, bool bPause=false);
	void AddInstr(void * pModule, string name);
	void AddMsgIntf(void * pModule, string name, string dir, string type, string dimen, 
		string replCnt, string queueW, string reserve, bool autoConn);
	void AddTrace(void * pModule, string name);
	void AddScPrim(void * pModule, string scPrimDecl, string scPrimCall);
	void AddPrimstate(void * pModule, string type, string name, string include);
	void * AddHostMsg(void * pModule, HtdFile::EHostMsgDir msgDir, string msgName);
	void AddHostData(void * pModule, HtdFile::EHostMsgDir msgDir, bool bMaxBw);
	void * AddGlobal(void * pHandle);
	void AddGlobalVar(void * pHandle, string type, string name, string dimen1, string dimen2,
		string addr1W, string addr2W, string addr1, string addr2, string rdStg, string wrStg, bool bMaxIw, bool bMaxMw, ERamType ramType);
	void * AddGlobalRam(void * pModule, string ramName, string dimen1, string dimen2, string addr1W, string addr2W, 
		string addr1, string addr2, string rdStg, string wrStg, bool bExtern);
	void AddBarrier(void * pModule, string name, string barIdW);
	void AddStream(void * pStruct, bool bRead, string &name, string &type, string &strmBw, string &elemCntW, string &strmCnt,
		string &memSrc, vector<int> &memPort, string &access, string &reserve, bool paired, bool bClose, string &tag, string &rspGrpW);
	void AddStencilBuffer(void * pStruct, string &name, string & type, vector<int> &gridSize, vector<int> &stencilSize, string &pipeLen);

	void AddGlobalField(void * pStruct, string type, string name, string dimen1="", string dimen2="", bool bRead=true, bool bWrite=true, ERamType ramType = eDistRam);
	void * AddStructField(void * pStruct, string type, string name, string bitWidth="", string base="", vector<CHtString> const &dimenList=g_nullHtStringVec,
		bool bSrcRead=true, bool bSrcWrite=true, bool bMifRead=false, bool bMifWrite=false, ERamType ramType = eDistRam, int atomicMask=0);
	void * AddPrivateField(void * pStruct, string type, string name, string dimen1, string dimen2, string addr1W, string addr2W, string addr1, string addr2);
	void * AddSharedField(void * pStruct, string type, string name, string dimen1, string dimen2, string rdSelW, string wrSelW, string addr1W, string addr2W, string queueW, ERamType ramType, string reset);
	void * AddStageField(void * pStruct, string type, string name, string dimen1, string dimen2, string *pRange,
		bool bInit, bool bConn, bool bReset, bool bZero);

	void AddEntryParam(void * pEntry, string hostType, string typeName, string paramName, bool bIsUsed);
	void AddReturnParam(void * pReturn, string hostType, string typeName, string paramName, bool bIsUsed);
	void AddFunctionParam(void * pFunction, string dir, string type, string name);

	void * AddMsgDst(void * pOpenIhm, string var, string dataLsb, string addr1Lsb, string addr2Lsb,
		string idx1Lsb, string idx2Lsb, string field, string fldIdx1Lsb, string fldIdx2Lsb, bool bReadOnly);

	void * AddSrc(void * pOpenMifWr, string name, string var, string field, bool bMultiWr, string srcIdx, string memDst);
	void * AddDst(void * pOpenMifRd, string name, string var, string field, string dataLsb,
		bool bMultiRd, string dstIdx, string memSrc, string atomic, string rdType);
	void * AddDst(void * pOpenMifRd, string name, string infoW, string stgCnt,
		bool bMultiRd, string memSrc, string rdType);

 	// Routines called by HtdFile::saveHtdFile
	bool GetModule(string &name) { return false; }
	bool GetDefines(string &name, string &value, string &scope) { return false; }
	bool GetTypedefs(string &name, string &type, string &width, string &scope) { return false; }
	bool GetStructs(string &name, bool &bUnion, bool &bHost, string &scope) { return false; }
	bool GetStructField(string &type, string &name, string &dimen1, string &dimen2) { return false; }
	bool GetGlobalRam(string &name, string &dimen1, string &dimen2, string &addr1W,
		string &addr2W, string &addr1, string &addr2, string &rdStg, string &wrStg, string &extern_) { return false; }
	bool GetGlobalField(string &type, string &name, string &dimen1, string &dimen2,
		string &read, string &write, string &ramType) { return false; }
	bool GetSharedVars(string &type, string &name, string &addr1W, string &addr2W, string &queueW,
					string &dimen1, string &dimen2, string &blockRam)  { return false; }
	bool GetThreadGroups(string &name, string &groupW, string &resetInstr) { return false; }
	bool NeedClk2x();

	void InitBramUsage();
	void ReportRamFieldUsage();
	void DrawModuleCmdRelationships();

	void InitializeAndValidate();
	void InitAddrWFromAddrName();
	void InitAndValidateHif();
	void InitAndValidateModMif();
	void InitAndValidateModCxr();
	void InitAndValidateModIpl();
	void InitAndValidateModRam();
	void InitAndValidateModIhm();
	void InitAndValidateModOhm();
	void InitAndValidateModIhd();
	void InitAndValidateModMsg();
	void InitAndValidateModBar();
	void InitAndValidateModStrm();
	void InitAndValidateModStBuf();
	void InitAndValidateModNgv();

	void LoadDefineList();
	void AddTypeDef(string name, string type, string width, bool bInitWidth, string scope = string("unit"), string modName = string(""));
	bool FindFieldInStruct(CLineInfo const &lineInfo, string const &type, string const &fldName,
		bool bIndexCheck, bool &bCStyle, CField const * &pBaseField, CField const * &pLastField);
	bool IsInFieldList(CLineInfo const &lineInfo, string const &addrName, vector<CField> const &fieldList,
		bool bCStyle, bool bIndexCheck, CField const * &pBaseField, CField const * &pLastField, string * pFullName);
	int FindTypeWidth(CField const & field, int * pMinAlign=0, bool bHostType=false);
	bool FindIntType(string typeName, int &width, bool &bSigned);
	bool FindCIntType(string typeName, int &width, bool &bSigned);
	bool FindHtIntType(string typeName, int &width, bool &bSigned);
	int FindHostTypeWidth(CField const & field, int * pMinAlign=0);
	bool FindStructName(string const &structType, string &structName);
	int FindTypeWidth(string const &varName, string const &typeName, CHtString const &bitWidth, CLineInfo const &lineInfo, int * pMinAlign=0, bool bHostType=false);
	int FindStructWidth(CStruct & struct_, int * pMinAlign=0, bool bHostType=false);
	int FindFieldListWidth(string structName, CLineInfo &lineInfo, vector<CField> &fieldList, int * pMinAlign=0, bool bHostType=false);
	bool FindVariableWidth(CLineInfo const &lineInfo, CModule &mod, string name, bool bHtId, bool bPrivate, bool bShared, bool bStage, int &addr1W);
	float FindSlicePerBramRatio(int depth, int width);
	int FindBramCnt(int depth, int width);
	void InitNativeCTypes();
	bool IsNativeCType(string &type, bool bAllowHostPtr=false);
	void ValidateUsedTypes();
	void ValidateFieldListTypes(CStruct & record);
	void ValidateDesignInfo();
	void InitFieldDimenValues(vector<CField> &fieldList);
	void GenPersFiles();
	void ZeroStruct(CHtCode &code, string fieldBase, vector<CField> &fieldList);

	string GenIndexStr(bool bGen, char const * pFormat, int index);
	void InitCxrIntfInfo();
	bool AreModInstancesLinked(CModInst & srcModInst, CModInst & dstModInst);
	void CheckRequiredEntryNames(vector<CModIdx> &callStk);
	bool CheckTransferReturn(vector<CModIdx> &callStk);
	void CreateDirectoryStructure();
	void GenerateCommonIncludeFile();
	void GenGlobalVarWriteTypes(CHtFile & htFile, string &typeName, int &atomicMask, vector<string> &gvTypeList, CRam * pGv=0);
	void GenerateUnitTopFile();
	void GenerateAeTopFile();
	void GenerateHtaFiles();
	void GenerateHtiFiles();
	void GenerateHifFiles();
	void GenerateMifFiles(int mifId);
	void GenerateNgvFiles();
	void GenerateModuleFiles(CModule &modInfo);
	void GenStruct(FILE *incFp, string intfName, CStruct &ram, EGenStructMode mode=eGenStruct, bool bEmptyContructor=false);
	void GenPersBanner(CHtFile &htFile, const char *unitName, const char *dsnName, bool is_h);
	void GenerateBanner(CHtFile &htFile, const char *fileName, bool is_h);
	void GenIntfStruct(FILE *incFp, string intfName, vector<CField> &fieldList, bool bCStyle, bool bInclude, bool bData64, bool bUnion);
	void GenRamIntfStruct(FILE *incFp, string intfName, CRam &ram, EStructType type);
	void GenUserStructs(FILE *incFp, CStruct &userStruct, char const * pTabs="");
	void GenUserStructs(CHtCode &htFile, CStruct &userStruct, char const * pTabs="");
	void GenStructIsEqual(CHtCode &htFile, char const * pTabs, string prefixName, string &typeName, vector<CField> &fieldList, bool bCStyle, const char *&pStr, EStructType structType=eStructAll, bool bHeader=true);
	void GenStructAssign(CHtCode &htFile, char const * pTabs, string prefixName, string &typeName, vector<CField> &fieldList, bool bCStyle, const char *&pStr, EStructType structType=eStructAll, bool bHeader=true);
	void GenStructAssignToZero(CHtCode &htFile, char const * pTabs, string prefixName, string &typeName, vector<CField> &fieldList, bool bCStyle, const char *&pStr, EStructType structType=eStructAll, bool bHeader=true);
	void GenStructScTrace(CHtCode &htFile, char const * pTabs, string prefixName1, string prefixName2, string &typeName, vector<CField> &fieldList, bool bCStyle, const char *&pStr, EStructType structType=eStructAll, bool bHeader=true);
	void GenStructStream(CHtCode &htFile, char const * pTabs, string prefixName, string &typeName, vector<CField> &fieldList, bool bCStyle, const char *&pStr, EStructType structType=eStructAll, bool bHeader=true);
	void GenModDecl(EVcdType vcdType, CHtCode &htFile, string &modName, VA type, VA var, vector<CHtString> const & dimenList = g_nullHtStringVec);
	void GenVcdDecl(CHtCode &sosCode, EVcdType vcdType, CHtCode &declCode, string &modName, VA type, VA var, vector<CHtString> const & dimenList = g_nullHtStringVec);
	void GenModVar(EVcdType vcdType, string &vcdModName, bool &bFirstModVar, string decl, string dimen, string name, string val, vector<CHtString> const & dimenList = g_nullHtStringVec);
	void GenModTrace(EVcdType vcdType, string &modName, VA name, VA val);
	void GenVcdTrace(CHtCode &sosCode, EVcdType vcdType, string &modName, VA name, VA val);
	string GenFieldType(CField &field, bool bConst);
	void GenStructInit(FILE *fp, string &tabs, string prefixName, CField &field, int idxCnt, bool bZero);

	enum EFieldListMode { FieldList, Union, Struct };
	void GenUserStructFieldList(FILE *incFp, bool bIsHtPriv, vector<CField> &fieldList, bool bCStyle, EFieldListMode mode, string tabs, bool bUnion=false);
	void GenUserStructFieldList(CHtCode &htFile, bool bIsHtPriv, vector<CField> &fieldList, bool bCStyle, EFieldListMode mode, string tabs, bool bUnion=false);
	void GenUserStructBadData(CHtCode &htFile, bool bHeader, string intfName, vector<CField> &fieldList, bool bCStyle, string tabs);
	void GenSmCmd(FILE *incFp, CModule &modInfo);
	void GenCmdInfo(FILE *incFp, CModule &modInfo);
	void GenSmInfo(FILE *incFp, CStruct &smInfo);
	void GenRamWrEn(FILE *incFp, string intfName, CStruct &ram);

	void AutoDeclType(string type, bool bInitWidth=false, string scope = string("auto"));

	void GenHtMon();

	void MarkNeededIncludeStructs();
	void MarkNeededIncludeStructs(CStruct & struct_);
	bool FindStringValue(CLineInfo &lineInfo, string ramIdxW, int &ramIdxWidth) { bool bIsSigned; return m_defineTable.FindStringValue(lineInfo, ramIdxW, ramIdxWidth, bIsSigned); }

	bool IsInstanceLinkPresent(CHtString &, int, CHtString &, int) { return false; }

	void GenMsvsProjectFiles();

	void AddDesign(string unitName, string hostIntf) {
		m_unitName = unitName;
		m_hostIntf = hostIntf;
		m_bDumpIntf = false;
		m_bRndRetry = false;
		m_bRndInit = false;
		m_bBlockRamDoReg = false;//true;
	}

	void AddPerfMon(bool bPerfMon) {
		m_bPerfMon = bPerfMon;
	}

	void AddRndRetry(bool bRndRetry) {
		m_bRndRetry = bRndRetry;
	}

	void AddRndInit(bool bRndInit) {
		m_bRndInit = bRndInit;
	}

	CDsnInfo & AddScalarType(string name, string type, string width) {
		if (IsInTypeList(name))
			ParseMsg(Error, "type with name '%s' was previously declared", name.c_str());
		else if (!IsBaseType(type))
			ParseMsg(Error, "type '%s' is not a base type", type.c_str());
		else
			m_typeList.push_back(CType(Scalar, name, type, width));

		return *this;
	}

	CDsnInfo & AddArrayType(string name, string type, string dimen1W, string dimen2W) {
		if (IsInTypeList(name))
			ParseMsg(Error, "type with name '%s' was previously declared", name.c_str());
		else if (!IsBaseType(type))
			ParseMsg(Error, "type '%s' is not a base type", type.c_str());
		else
			m_typeList.push_back(CType(Array, name, type, dimen1W, dimen2W));

		return *this;
	}

	CDsnInfo & AddQueueType(string name, string type, string sizeW) {
		if (IsInTypeList(name))
			ParseMsg(Error, "type with name '%s' was previously declared", name.c_str());
		else if (!IsBaseType(type))
			ParseMsg(Error, "type '%s' is not a base type", type.c_str());
		else
			m_typeList.push_back(CType(Queue, name, type, sizeW));

		return *this;
	}

	CModule & AddModule(CModule * smInfo) {
		m_modList.push_back(smInfo);
		return *m_modList.back();
	}

	CTypeDef * FindTypeDef(string typeName);
	CStruct * FindStruct(string structName);
	string FindFieldType(CField &field);

	bool IsTypeNameValid(string typeName, CModule * pMod=0) {
		if (pMod) {
			string instrType = VA("%sInstr_t", pMod->m_modName.Uc().c_str());
			if (typeName == instrType)
				return true;
		}
		return IsInTypeList(typeName) || IsInStructList(typeName) || IsBaseType(typeName);
	}

	bool IsInTypeList(string &typeName) {
		for (size_t typeIdx = 0; typeIdx < m_typedefList.size(); typeIdx += 1)
			if (m_typedefList[typeIdx].m_name == typeName)
				return true;
		return false;
	}

	bool IsInStructList(string &typeName) {
		for (size_t typeIdx = 0; typeIdx < m_structList.size(); typeIdx += 1)
			if (m_structList[typeIdx].m_structName == typeName)
				return true;
		return false;
	}

	bool IsBaseType(string &type) {
		int width;
		char suffix[256];
		if (sscanf(type.c_str(), "int%d_t%s", &width, suffix) == 1 && width > 0 && width <= 64)
			return true;
		if (sscanf(type.c_str(), "uint%d_t%s", &width, suffix) == 1 && width > 0 && width <= 64)
			return true;
		if (sscanf(type.c_str(), "ht_int%d%s", &width, suffix) == 1 && width > 0 && width <= 64)
			return true;
		if (sscanf(type.c_str(), "ht_uint%d%s", &width, suffix) == 1 && width > 0 && width <= 64)
			return true;
		if (sscanf(type.c_str(), "sc_int<%d>", &width) == 1 && width > 0 && width <= 64)
			return true;
		if (sscanf(type.c_str(), "sc_uint<%d>", &width) == 1 && width > 0 && width <= 64)
			return true;
		if (type == "bool" || type == "short" || type == "char" || type == "int" || type == "unsigned int"
			|| type == "unsigned short" || type == "unsigned char" || type == "long" || type == "unsigned long"
			|| type == "long long" || type == "unsigned long long")
			return true;
		return false;
	}

public:
	int			m_htModId;
	CHtString	m_unitName;

	string		m_hostIntf;

	bool		m_bDumpIntf;
	bool		m_bPerfMon;
	bool		m_bRndRetry;
	bool		m_bRndInit;
	bool		m_bBlockRamDoReg;
	vector<CBramTarget> m_bramTargetList;

	vector<int>	m_instIdList;
	int			m_maxInstId;
	int			m_maxReplCnt;

	vector<CInstanceInfo>	m_instanceInfoList;

	vector<CModule *>	m_modList;
	vector<CModInst>	m_dsnInstList;
	CDefineTable		m_defineTable;
	vector<CTypeDef>	m_typedefList;
	vector<CType>		m_typeList;
	vector<CStruct>		m_structList;
	vector<CMifInst>	m_mifInstList;
	vector<CNgvInfo *>	m_ngvList;

	vector<CBasicType>	m_nativeCTypeList;
	vector<CIncludeFile>	m_includeList;

	void GenPrimStateStatements(CModule &mod);
	void GenModTDSUStatements(CModule &mod); // typedef/define/struct/union
	void GenModMsgStatements(CModule &mod);
	void GenModBarStatements(CModule &mod);
	void GenModIplStatements(CModule &mod, int modInstIdx);
	void GenModIhmStatements(CModule &mod);
	void GenModOhmStatements(CModule &mod);
	void GenModCxrStatements(CModule &mod, int modInstIdx);
	void GenModRamStatements(CModInst &modInst);
	void GenModIhdStatements(CModule &mod);
	void GenModOhdStatements(CModule &mod);
	void GenModMifStatements(CModule &mod);
	void GenRamPreRegStatements(HtdFile::EClkRate eClk2x);
	void GenModStrmStatements(CModule &mod);
	void GenModStBufStatements(CModule * pMod);
	void GenModNgvStatements(CModule &mod);

	void GenAeNextMsgIntf(HtiFile::CMsgIntfConn * pMicAeNext);
	void GenAePrevMsgIntf(HtiFile::CMsgIntfConn * pMicAePrev);

	void SetMsgIntfConnUsedFlags(bool bInBound, CMsgIntfConn * pConn, CModule &mod, CMsgIntf * pMsgIntf);

	void WritePersCppFile(CModule &mod, int modInstIdx, bool bNeedClk2x);
	void WritePersIncFile(CModule &mod, int modInstIdx, bool bNeedClk2x);
	//void GenStruct(string intfName, CStruct &ram, EGenStructMode mode, bool bEmptyContructor);
	void GenRamIntfStruct(CHtCode &code, char const * pTabs, string intfName, CRam &ram, EStructType type);
	void GenRamWrEn(CHtCode &code, char const * pTabs, string intfName, CStruct &ram);
	bool DimenIter(vector<CHtString> const &dimenList, vector<int> &refList);
	string IndexStr(vector<int> &refList, int startPos=-1, int endPos=-1, bool bParamStr=false);

	string GenRamAddr(CModInst & modInst, CRam &ram, CHtCode *pCode, string accessSelW, string accessSelName, const char *pInStg, const char *pOutStg, bool bWrite, bool bNoSelectAssign=false);
	//void GenRamAddr(CModInst &modInst, CHtCode &ramPreSm, string &addrType, CRam &ram, string ramAddrName, const char *pInStg, const char *pOutStg, bool bNoSelectAssign=true);
	int CountPhaseResetFanout(CModInst &modInst);
	void GenDimenInfo(CDimenList & ramList, CDimenList & fieldList, vector<int> & dimenList, string & dimenDecl, string & dimenIndex, string * pVarIndex=0, string * pFldIndex=0);
	string GenRamIndexLoops(CHtCode &ramCode, vector<int> & dimenList, bool bOpenParen=false);
	string GenRamIndexLoops(CHtCode &ramCode, const char *pTabs, CDimenList &dimenList, bool bOpenParen=false);
	void GenRamIndexLoops(FILE *fp, CDimenList &dimenList, const char *pTabs);
	void GenIndexLoops(CHtCode &htCode, string &tabs, vector<CHtString> & dimenList, bool bOpenParen=false);

	void IplNewLine(CHtCode * pIplTxStg);
	void IplComment(CHtCode * pIplTxStg, CHtCode & iplReg, const char *);

	void ListIgnoredModules();

private:
	CLex m_lex;
	HtiFile m_htiFile;
	vector<string> m_moduleFileList;

	// Sections of code for prim state
	CHtCode m_psInclude;
	CHtCode m_psDecl;

	// Sections of code for stencil buffers (stBuf)
	CHtCode	m_stBufFuncDecl;
	CHtCode	m_stBufFuncDef;
	CHtCode	m_stBufRegDecl;
	CHtCode m_stBufPreInstr1x;
	CHtCode m_stBufPostInstr1x;
	CHtCode m_stBufReg1x;
	CHtCode m_stBufPreInstr2x;
	CHtCode m_stBufPostInstr2x;
	CHtCode m_stBufReg2x;

	// Sections of code for stream (strm)
	CHtCode m_strmCtorInit;
	CHtCode	m_strmFuncDecl;
	CHtCode	m_strmFuncDef;
	CHtCode m_strmIoDecl;
	CHtCode	m_strmRegDecl;
	CHtCode m_strmPreInstr1x;
	CHtCode m_strmPostInstr1x;
	CHtCode m_strmReg1x;
	CHtCode m_strmOut1x;
	CHtCode m_strmPreInstr2x;
	CHtCode m_strmPostInstr2x;
	CHtCode m_strmReg2x;
	CHtCode m_strmOut2x;
	CHtCode m_strmBadDecl;
	CHtCode m_strmAvlCntChk;

	// Sections of code for barrier (bar)
	CHtCode	m_barFuncDecl;
	CHtCode	m_barFuncDef;
	CHtCode m_barIoDecl;
	CHtCode	m_barRegDecl;
	CHtCode m_barPreInstr1x;
	CHtCode m_barPostInstr1x;
	CHtCode m_barReg1x;
	CHtCode m_barOut1x;
	CHtCode m_barPreInstr2x;
	CHtCode m_barPostInstr2x;
	CHtCode m_barReg2x;
	CHtCode m_barOut2x;

	// Sections of code for message interface (MSG)
	CHtCode	m_msgFuncDecl;
	CHtCode	m_msgFuncDef;
	CHtCode m_msgIoDecl;
	CHtCode	m_msgRegDecl;
	CHtCode m_msgPreInstr1x;
	CHtCode m_msgPostInstr1x;
	CHtCode m_msgReg1x;
	CHtCode m_msgPostReg1x;
	CHtCode m_msgOut1x;
	CHtCode m_msgPreInstr2x;
	CHtCode m_msgPostInstr2x;
	CHtCode m_msgReg2x;
	CHtCode m_msgPostReg2x;
	CHtCode m_msgOut2x;

	// Sections of code for instruction pipeline (IPL)
	CHtCode m_iplCtorInit;
	CHtCode	m_iplDefines;
	CHtCode m_iplRefDecl;
	CHtCode m_iplRefInit;
	CHtCode	m_iplFuncDecl;
	CHtCode	m_iplMacros;
	CHtCode	m_iplRegDecl;
	CHtCode m_iplBadDecl;
	CHtCode	m_iplT0Stg1x;
	CHtCode	m_iplT1Stg1x;
	CHtCode	m_iplT2Stg1x;
	CHtCode	m_iplTsStg1x;
	CHtCode	m_iplPostInstr1x;
	CHtCode	m_iplReg1x;
	CHtCode	m_iplPostReg1x;
	CHtCode	m_iplOut1x;
	CHtCode m_iplClrShared1x;
	CHtCode	m_iplT0Stg2x;
	CHtCode	m_iplT1Stg2x;
	CHtCode	m_iplT2Stg2x;
	CHtCode	m_iplTsStg2x;
	CHtCode	m_iplPostInstr2x;
	CHtCode	m_iplReg2x;
	CHtCode	m_iplPostReg2x;
	CHtCode	m_iplOut2x;
	CHtCode m_iplClrShared2x;
	CHtCode m_iplEosChecks;

	CHtCode	m_htSelDef;

	// Sections of code for inbound host msg interface
	CHtCode	m_ihmIoDecl;
	CHtCode	m_ihmRegDecl;
	CHtCode	m_ihmReg1x;
	CHtCode	m_ihmPostInstr1x;
	CHtCode	m_ihmPostInstr2x;

	// Sections of code for outbound host msg interface
	CHtCode	m_ohmMacros;
	CHtCode	m_ohmIoDecl;
	CHtCode	m_ohmRegDecl;
	CHtCode	m_ohmFuncDecl;
	CHtCode	m_ohmPreInstr1x;
	CHtCode	m_ohmPostInstr1x;
	CHtCode	m_ohmReg1x;
	CHtCode	m_ohmPostReg1x;
	CHtCode	m_ohmOut1x;
	CHtCode	m_ohmPreInstr2x;
	CHtCode	m_ohmPostInstr2x;
	CHtCode	m_ohmReg2x;
	CHtCode	m_ohmPostReg2x;
	CHtCode	m_ohmOut2x;
	CHtCode m_ohmAvlCntChk;

	// Sections of code for cxr interface
	CHtCode	m_cxrDefines;
	CHtCode	m_cxrIoDecl;
	CHtCode	m_cxrRegDecl;
	CHtCode	m_cxrFuncDecl;
	CHtCode	m_cxrMacros;
	CHtCode	m_cxrPreInstr1x;
	CHtCode	m_cxrT0Stage1x;
	CHtCode	m_cxrTsStage1x;
	CHtCode	m_cxrPostInstr1x;
	CHtCode	m_cxrReg1x;
	CHtCode	m_cxrPostReg1x;
	CHtCode	m_cxrOut1x;
	CHtCode	m_cxrPreInstr2x;
	CHtCode	m_cxrT0Stage2x;
	CHtCode	m_cxrTsStage2x;
	CHtCode	m_cxrPostInstr2x;
	CHtCode	m_cxrReg2x;
	CHtCode	m_cxrPostReg2x;
	CHtCode	m_cxrOut2x;
	CHtCode m_cxrAvlCntChk;

	// Sections of code for rams
	CHtCode	m_gblDefines;
	CHtCode	m_gblMacros;
	CHtCode	m_gblIoDecl;

	CHtCode m_gblRefDecl;
	CHtCode	m_gblRegDecl;
	CHtCode	m_gblPreInstr1x;
	CHtCode	m_gblPreInstr2x;
	CHtCode	m_gblPostInstr1x;
	CHtCode	m_gblPostInstr2x;
	CHtCode	m_gblPreCont;
	CHtCode	m_gblReg1x;
	CHtCode	m_gblReg2x;
	CHtCode m_gblPostReg1x;
	CHtCode m_gblPostReg2x;
	CHtCode	m_gblOut1x;
	CHtCode	m_gblOut2x;		// output list for 2x clock
	CHtCode	m_gblOutCont;	// output list for continuous assignment
	CHtCode	m_gblSenCont;	// sensitivity list for continuous assignment

	// Sections of code for inbound host data
	CHtCode m_ihdCtorInit;
	CHtCode	m_ihdIoDecl;
	CHtCode	m_ihdRamDecl;
	CHtCode	m_ihdRegDecl;
	CHtCode	m_ihdFuncDecl;
	CHtCode	m_ihdMacros;
	CHtCode	m_ihdRamClock1x;
	CHtCode	m_ihdPreInstr1x;
	CHtCode	m_ihdPostInstr1x;
	CHtCode	m_ihdReg1x;
	CHtCode	m_ihdPostReg1x;
	CHtCode	m_ihdOut1x;
	CHtCode	m_ihdRamClock2x;
	CHtCode	m_ihdPreInstr2x;
	CHtCode	m_ihdPostInstr2x;
	CHtCode	m_ihdReg2x;
	CHtCode	m_ihdPostReg2x;
	CHtCode	m_ihdOut2x;

	// Sections of code for outbound host data
	CHtCode	m_ohdIoDecl;
	CHtCode	m_ohdRegDecl;
	CHtCode	m_ohdFuncDecl;
	CHtCode m_ohdAvlCntChk;
	CHtCode	m_ohdMacros;
	CHtCode	m_ohdPreInstr1x;
	CHtCode	m_ohdPostInstr1x;
	CHtCode	m_ohdReg1x;
	CHtCode	m_ohdPostReg1x;
	CHtCode	m_ohdOut1x;
	CHtCode	m_ohdPreInstr2x;
	CHtCode	m_ohdPostInstr2x;
	CHtCode	m_ohdReg2x;
	CHtCode	m_ohdPostReg2x;
	CHtCode	m_ohdOut2x;

	// Sections of code for MIF
	CHtCode	m_mifCtorInit;
	CHtCode	m_mifRamDecl;
	CHtCode m_mifIoDecl;
	CHtCode	m_mifDecl;
	CHtCode	m_mifFuncDecl;
	CHtCode	m_mifDefines;
	CHtCode	m_mifMacros;
	CHtCode	m_mifPreInstr1x;
	CHtCode	m_mifPostInstr1x;
	CHtCode	m_mifRamClock1x;
	CHtCode	m_mifReg1x;
	CHtCode	m_mifPostReg1x;
	CHtCode	m_mifOut1x;
	CHtCode	m_mifPreInstr2x;
	CHtCode	m_mifPostInstr2x;
	CHtCode	m_mifRamClock2x;
	CHtCode	m_mifReg2x;
	CHtCode	m_mifPostReg2x;
	CHtCode	m_mifOut2x;
	CHtCode m_mifAvlCntChk;

	CHtCode m_vcdSos;	// vcd start_of_simulation

	// Sections of code for typedef, struct, union declarations
	CHtCode m_tdsuDefines;
};

// External ram interface info
struct CRamPort {
	CRamPort(CModule &modInfo, string instNum, CRam &ram, int selWidth, int selIdx) {
		m_instName = modInfo.m_modName.AsStr() + instNum;

		m_pModInfo = &modInfo;
		m_pRam = &ram;

		m_selWidth = selWidth;
		m_selIdx = selIdx;

		m_bAllBlockRam = false;

		m_bIntIntf = false;			// set if internal read/write
		m_bSrcRead = false;			// set if src read access
		m_bMifRead = false;			// set if mif read access
		m_bOne1xReadIntf = false;
		m_bFirst1xReadIntf = false;
		m_bSecond1xReadIntf = false;
		m_bOne2xReadIntf = false;
		m_bSrcWrite = false;			// set if src write access
		m_bMifWrite = false;			// set if mif write access
		m_bOne1xWriteIntf = false;
		m_bFirst1xWriteIntf = false;
		m_bSecond1xWriteIntf = false;
		m_bOne2xWriteIntf = false;
	}

public:
	CHtString	m_instName;
	CModule *	m_pModInfo;
	CRam *		m_pRam;
	int			m_selWidth;
	int			m_selIdx;

	bool		m_bIntIntf;
	bool		m_bSrcRead;
	bool		m_bSrcWrite;
	bool		m_bMifRead;
	bool		m_bMifWrite;

	bool		m_bAllBlockRam;
	bool		m_bOne1xReadIntf;
	bool		m_bFirst1xReadIntf;
	bool		m_bSecond1xReadIntf;
	bool		m_bOne2xReadIntf;
	bool		m_bOne1xWriteIntf;
	bool		m_bFirst1xWriteIntf;
	bool		m_bSecond1xWriteIntf;
	bool		m_bOne2xWriteIntf;
};

struct CRamPortField {
	CRamPortField(CRamPort * pRamPort) {
		m_pRamPort = pRamPort;

		m_bOne1xReadIntf = false;
		m_bFirst1xReadIntf = false;
		m_bSecond1xReadIntf = false;
		m_bOne2xReadIntf = false;
		m_bOne1xWriteIntf = false;
		m_bFirst1xWriteIntf = false;
		m_bSecond1xWriteIntf = false;
		m_bOne2xWriteIntf = false;
		m_bShAddr = false;
	}

	CRamPort *	m_pRamPort;

	bool		m_bOne1xReadIntf;
	bool		m_bFirst1xReadIntf;
	bool		m_bSecond1xReadIntf;
	bool		m_bOne2xReadIntf;
	bool		m_bOne1xWriteIntf;
	bool		m_bFirst1xWriteIntf;
	bool		m_bSecond1xWriteIntf;
	bool		m_bOne2xWriteIntf;
	bool		m_bShAddr;
};

