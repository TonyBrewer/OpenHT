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
struct CDsnInfo;
struct CType;
struct CHtInt;
struct CRecord;
struct CField;
struct CCxrParam;
struct CQueIntf;
struct CCxrEntry;
struct CNgvInfo;

extern CHtInt g_uint64;
extern CHtInt g_uint32;
extern CHtInt g_uint16;
extern CHtInt g_uint8;
extern CHtInt g_int64;
extern CHtInt g_int32;
extern CHtInt g_int16;
extern CHtInt g_int8;
extern CHtInt g_bool;

class VA {
public:
	VA(char const * pFmt, ...)
	{
		va_list args;
		va_start(args, pFmt);
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

inline int FindLg2(uint64_t value, bool bLg2ZeroEqOne = true, bool bFixLg2Pwr2 = false)
{
	if (bFixLg2Pwr2)
		value -= 1;
	int lg2;
	for (lg2 = 0; value != 0 && lg2 < 64; lg2 += 1)
		value >>= 1;
	return (lg2 == 0 && bLg2ZeroEqOne) ? 1 : lg2;
}

inline bool IsPowerTwo(uint64_t x) { return (x & (x - 1)) == 0; }

// Types

enum EType { eRecord, eTypedef, eClang, eHtInt };

struct CType {
	CType(EType eType, string const & name, int clangBitWidth, int minAlign, int packedBitWidth = -1) :
		m_eType(eType), m_typeName(name), m_clangBitWidth(clangBitWidth), 
		m_clangMinAlign(minAlign), m_packedBitWidth(packedBitWidth)
	{
	}

public:
	virtual CRecord * AsRecord() { return 0; }
	virtual CHtInt * AsInt() { return 0; }

	bool IsRecord() { return m_eType == eRecord; }
	bool IsInt() { return m_eType == eClang || m_eType == eHtInt; }
	bool IsEmbeddedUnion();

	int GetClangBitWidth();
	int GetPackedBitWidth();

public:
	EType m_eType;
	string m_typeName;
	int m_clangBitWidth;
	int m_clangMinAlign;

private:
	int m_packedBitWidth;
};

enum ESign { eSigned, eUnsigned };

struct CHtInt : CType {
	CHtInt(ESign eSign, string const & name, int bitWidth, int minAlign, int fldWidth=-1) :
		CType(eClang, name, bitWidth, minAlign, fldWidth > 0 ? fldWidth : bitWidth), m_eSign(eSign), m_fldWidth(fldWidth)
	{
	}

	CHtInt * AsInt() { return this; }

public:
	ESign m_eSign;
	int m_fldWidth;
};

struct CDimenList {
	CDimenList() { m_elemCnt = 0; }

	void InitDimen(CLineInfo & lineInfo)
	{
		if (m_elemCnt > 0) return;

		DimenListInit(lineInfo);
		assert(m_dimenIndex.size() == 0);
		m_elemCnt = 1;
		for (size_t i = 0; i < m_dimenList.size(); i += 1) {
			CHtString & dimenStr = m_dimenList[i];
			m_dimenDecl += VA("[%d]", dimenStr.AsInt());
			m_dimenIndex += VA("[idx%d]", i + 1);
			m_elemCnt *= dimenStr.AsInt();
			m_dimenTabs += "\t";
		}
	}

private:
	void DimenListInit(CLineInfo & lineInfo)
	{
		for (size_t i = 0; i < m_dimenList.size(); i += 1)
			m_dimenList[i].InitValue(lineInfo, false, 0);
	}

public:
	int GetDimen(size_t idx) const
	{ // zero based index
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

	CField(CType * pType, string name, string bitWidth, string base, vector<CHtString> const &dimenList, bool bSrcRead,
		bool bSrcWrite, bool bMifRead, bool bMifWrite, HtdFile::ERamType ramType, int atomicMask,
		bool bSpanningFieldForced = false)
	{
		Init();

		m_pType = pType;
		m_name = name;
		m_fieldWidth = bitWidth;
		m_base = base;
		m_dimenList = dimenList;
		m_bSrcRead = bSrcRead;
		m_bSrcWrite = bSrcWrite;
		m_bMifRead = bMifRead;
		m_bMifWrite = bMifWrite;
		m_ramType = ramType;
		m_atomicMask = atomicMask;
		m_bSpanningFieldForced = bSpanningFieldForced;
	}

	CField(CType * pType, string fieldName, bool bIfDefHtv = false)
	{
		Init();

		m_pType = pType;
		m_name = fieldName;
		m_bIfDefHtv = bIfDefHtv;
	}

	CField(string hostType, CType * pType, string fieldName, bool bIsUsed)
	{
		Init();

		m_hostType = hostType;
		m_pType = pType;
		m_name = fieldName;
		m_bIsUsed = bIsUsed;
	}

	CField(char *pName)
	{
		Init();

		m_name = pName;
	}

	CField(CType * pType, string fieldName, string dimen1, string dimen2)
	{
		Init();

		m_pType = pType;
		m_name = fieldName;
		if (dimen1.size() > 0)
			m_dimenList.push_back(dimen1);
		if (dimen2.size() > 0)
			m_dimenList.push_back(dimen2);
	}

	CField(CType * pType, string name, string dimen1, string dimen2, string rdSelW, string wrSelW,
		string addr1W, string addr2W, string queueW, HtdFile::ERamType ramType, string reset)
	{
		Init();

		m_pType = pType;
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

	CField(CType * pType, string name, string dimen1, string dimen2, string addr1W, string addr2W, string addr1Name, string addr2Name)
	{
		Init();

		m_pType = pType;
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

	CField(CType * pType, string &name, string &dimen1, string &dimen2, CHtString &rngLow, CHtString &rngHigh,
		bool bInit, bool bConn, bool bReset, bool bZero)
	{
		Init();

		m_pType = pType;
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
	CField(string &dir, CType * pType, string &name)
	{
		Init();

		m_dir = dir;
		m_pType = pType;
		m_name = name;
	}

private:
	void Init()
	{
		m_lineInfo = CPreProcess::m_lineInfo;

		m_atomicMask = 0;

		m_bIfDefHtv = false;
		m_bIhmReadOnly = false;

		m_addr1IsHtId = false;
		m_addr1IsPrivate = false;
		m_addr1IsStage = false;
		m_addr2IsHtId = false;
		m_addr2IsPrivate = false;
		m_addr2IsStage = false;
	}

public:
	string		m_hostType;
	string		m_dir;
	string		m_base;
	string		m_name;
	CHtString	m_fieldWidth;
	CHtString	m_rdSelW;
	CHtString	m_wrSelW;
	CHtString	m_addr1W;
	CHtString	m_addr2W;
	CHtString	m_queueW;
	string		m_addr1Name;
	bool		m_addr1IsHtId;
	bool		m_addr1IsPrivate;
	bool		m_addr1IsStage;
	string		m_addr2Name;
	bool		m_addr2IsHtId;
	bool		m_addr2IsPrivate;
	bool		m_addr2IsStage;
	string		m_reset;
	bool		m_bSrcRead;
	bool		m_bSrcWrite;
	bool		m_bMifRead;
	bool		m_bMifWrite;
	bool		m_bIsUsed;
	HtdFile::ERamType	m_ramType;
	int			m_atomicMask;
	int			m_cLangFieldPos;

	// spanning field flags for global variable writes
	bool		m_bSpanningFieldForced;

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

	CLineInfo	m_lineInfo;

	CType *		m_pType;
	int			m_clangBitPos;	// position of this field within structure (including alignment gaps)
};

struct CRecord : CType {
	virtual ~CRecord() {}
	CRecord() : CType(eRecord, string(), -1, 0, -1), m_bReadForInstrRead(false), m_bWriteForInstrWrite(false),
		m_bReadForMifWrite(false), m_bWriteForMifRead(false),
		m_bCStyle(false), m_bUnion(false), m_bShared(false), m_bInclude(false), m_bNeedIntf(false), m_atomicMask(0)
	{
		m_lineInfo = CPreProcess::m_lineInfo;
		m_bConstructors = true;
	}
	CRecord(string const & typeName, bool bUnion) : CType(eRecord, typeName, -1, 0, -1), m_bCStyle(true), m_bUnion(bUnion) {
		m_bConstructors = true;
	}
	CRecord(string recordName, bool bCStyle, bool bUnion, EScope scope, bool bInclude, string modName)
		: CType(eRecord, recordName, -1, 0, -1), m_bCStyle(bCStyle), m_bUnion(bUnion), m_bShared(false), m_scope(scope), m_bInclude(bInclude),
		m_bNeedIntf(false), m_modName(modName), m_atomicMask(0)
	{
		m_lineInfo = CPreProcess::m_lineInfo;
		m_bConstructors = true;
	}
	CRecord * AsRecord()
	{
		HtlAssert(m_eType == eRecord);
		return this;
	}

	CRecord & AddStructField2(CType * pType, string name, bool bIfDefHTV = false)
	{

		m_fieldList.push_back(new CField(pType, name, bIfDefHTV));

		return *this;
	}

	CField * AddStructField(CType * pType, string const & name, string bitWidth = "", string base = "", vector<CHtString> const &dimenList = g_nullHtStringVec,
		bool bSrcRead = true, bool bSrcWrite = true, bool bMifRead = false, bool bMifWrite = false, HtdFile::ERamType ramType = HtdFile::eDistRam, int atomicMask = 0,
		bool bSpanningFieldForced = false)
	{
		m_fieldList.push_back(new CField(pType, name, bitWidth, base, dimenList, bSrcRead, bSrcWrite, bMifRead, bMifWrite, ramType, atomicMask, bSpanningFieldForced));
		m_fieldList.back()->InitDimen(CPreProcess::m_lineInfo);
		m_fieldList.back()->m_fieldWidth.InitValue(CPreProcess::m_lineInfo, false);

		m_bReadForMifWrite |= bMifRead;
		m_bWriteForMifRead |= bMifWrite;

		if (pType->IsInt() && bitWidth.size() > 0) {
			CHtInt * pIntType = pType->AsInt();
			m_fieldList.back()->m_pType = new CHtInt(pIntType->m_eSign, pIntType->m_typeName, pIntType->m_clangBitWidth,
				pIntType->m_clangMinAlign, m_fieldList.back()->m_fieldWidth.AsInt());
		}

		return m_fieldList.back();
	}

	CRecord & AddField(CType * pType, string name, string dimen1, string dimen2)
	{

		m_fieldList.push_back(new CField(pType, name, dimen1, dimen2));

		return *this;
	}

	void AddSharedField(CType * pType, string name, string dimen1, string dimen2, string rdSelW, string wrSelW,
		string addr1W, string addr2W, string queueW, HtdFile::ERamType ramType, string reset)
	{

		m_fieldList.push_back(new CField(pType, name, dimen1, dimen2, rdSelW, wrSelW, addr1W, addr2W, queueW, ramType, reset));
	}

	void AddPrivateField(CType * pType, string name, string dimen1, string dimen2,
		string addr1W, string addr2W, string addr1, string addr2)
	{
		m_fieldList.push_back(new CField(pType, name, dimen1, dimen2, addr1W, addr2W, addr1, addr2));
	}

	bool AddStageField(CType * pType, string &name, string &dimen1, string &dimen2,
		string *pRange, bool bInit, bool bConn, bool bReset, bool bZero)
	{

		CHtString rngLow = pRange[0];
		CHtString rngHigh = pRange[1];

		rngLow.InitValue(CPreProcess::m_lineInfo);
		rngHigh.InitValue(CPreProcess::m_lineInfo);

		if (rngLow.AsInt() < 1)
			CPreProcess::ParseMsg(Error, "minimum range value is 1");

		if (rngLow.AsInt() > rngHigh.AsInt())
			CPreProcess::ParseMsg(Error, "illegal range low value greater than high value");

		for (size_t fldIdx = 0; fldIdx < m_fieldList.size(); fldIdx += 1) {
			CField &field = *m_fieldList[fldIdx];

			if (field.m_name == name &&
				(field.m_rngLow.AsInt() <= rngLow.AsInt() && rngLow.AsInt() <= field.m_rngHigh.AsInt() ||
				field.m_rngLow.AsInt() <= rngHigh.AsInt() && rngHigh.AsInt() <= field.m_rngHigh.AsInt())) {
				CPreProcess::ParseMsg(Error, "field '%s' was previously declared with overlapping ranges", name.c_str());

				return false;
			}
		}

		m_fieldList.push_back(new CField(pType, name, dimen1, dimen2, rngLow, rngHigh, bInit, bConn, bReset, bZero));

		return true;//rngHigh.AsInt() > 0 || !bNoReg;
	}

public:
	bool				m_bReadForInstrRead;
	bool				m_bWriteForInstrWrite;
	bool				m_bReadForMifWrite;
	bool				m_bWriteForMifRead;
	bool				m_bCStyle;
	bool				m_bUnion;
	bool				m_bShared;
	EScope				m_scope;
	bool				m_bInclude;
	bool				m_bNeedIntf;
	bool				m_bConstructors;
	bool				m_bSpanningWrite;

	string				m_modName;
	int					m_atomicMask;
	vector<CField *>	m_fieldList;
	CLineInfo			m_lineInfo;
};

enum ERam { eShared, eNgv };

struct CRam : CRecord, CDimenList {
	CRam() {}

	CRam(string moduleName, string ramName, string addrName, string addrBits, HtdFile::ERamType ramType = HtdFile::eDistRam)
	{
		Init();
		m_modName = moduleName;
		m_gblName = ramName;
		m_addr1Name = addrName;
		m_addr1W = addrBits;
		m_pIntGbl = 0;
		m_ramType = ramType;
	}

	CRam(string ramName, string addrName, string addrBits, bool bReplAccess, HtdFile::ERamType ramType = HtdFile::eDistRam)
	{
		Init();
		m_gblName = ramName;
		m_addr1Name = addrName;
		m_addr1W = addrBits;
		m_bReplAccess = bReplAccess;
		m_ramType = ramType;
	}

	CRam(string ramName, string addr1, string addr2, string addr1W, string addr2W, string dimen1, string dimen2, string &rdStg, string &wrStg)
	{
		Init();
		m_gblName = ramName;
		m_addr1Name = addr1;
		m_addr2Name = addr2;
		m_addr1W = addr1W;
		m_addr2W = addr2W;
		if (dimen1.size() > 0)
			m_dimenList.push_back(dimen1);
		if (dimen2.size() > 0)
			m_dimenList.push_back(dimen2);
		m_ramType = HtdFile::eDistRam;
		m_rdStg = rdStg;
		m_wrStg = wrStg;
	}

	CRam(CType * pType, string &name, string &dimen1, string &dimen2, string &addr1, string &addr2,
		string &addr1W, string &addr2W, string &rdStg, string &wrStg, bool bMaxIw, bool bMaxMw,
		HtdFile::ERamType ramType, bool bRead, bool bWrite, bool bSpanningWrite)
	{
		Init();
		m_pType = pType;
		m_type = pType->m_typeName;
		m_gblName = name;
		m_addr1Name = addr1;
		m_addr2Name = addr2;
		m_addr1W = addr1W;
		m_addr2W = addr2W;
		if (dimen1.size() > 0)
			m_dimenList.push_back(dimen1);
		if (dimen2.size() > 0)
			m_dimenList.push_back(dimen2);
		m_ramType = HtdFile::eDistRam;
		m_rdStg = rdStg;
		m_wrStg = wrStg;
		m_bMaxIw = bMaxIw;
		m_bMaxMw = bMaxMw;
		m_ramType = ramType;
		m_bReadForInstrRead = bRead;
		m_bWriteForInstrWrite = bWrite;
		m_bSpanningWrite = bSpanningWrite;
	}

	CRam(string &type, string &name, vector<CHtString> & dimenList, string &addr1, string &addr2,
		string &addr1W, string &addr2W, string &rdStg, string &wrStg, bool bMaxIw, bool bMaxMw,
		HtdFile::ERamType ramType, bool bRead, bool bWrite)
	{
		Init();
		m_type = type;
		m_gblName = name;
		m_addr1Name = addr1;
		m_addr2Name = addr2;
		m_addr1W = addr1W;
		m_addr2W = addr2W;
		m_dimenList = dimenList;
		m_ramType = HtdFile::eDistRam;
		m_rdStg = rdStg;
		m_wrStg = wrStg;
		m_bMaxIw = bMaxIw;
		m_bMaxMw = bMaxMw;
		m_ramType = ramType;
		m_bReadForInstrRead = bRead;
		m_bWriteForInstrWrite = bWrite;
	}

	void Init() {
		m_lineInfo = CPreProcess::m_lineInfo;
		m_bReplAccess = false;
		m_bGlobal = false;
		m_bPrivGbl = false;
		m_addr1IsHtId = false;
		m_addr1IsPrivate = false;
		m_addr1IsStage = false;
		m_addr2IsHtId = false;
		m_addr2IsPrivate = false;
		m_addr2IsStage = false;
		m_bSpanningWrite = false;
	}

public:
	CLineInfo			m_lineInfo;
	CHtString			m_modName;
	CHtString			m_gblName;
	HtdFile::ERamType	m_ramType;
	CRam *				m_pIntGbl;
	bool				m_bReplAccess;

	string				m_type;
	CHtString			m_addr0W;
	CHtString			m_addr1W;
	CHtString			m_addr2W;
	string				m_addr1Name;
	bool				m_addr1IsHtId;
	bool				m_addr1IsPrivate;
	bool				m_addr1IsStage;
	string				m_addr2Name;
	bool				m_addr2IsHtId;
	bool				m_addr2IsPrivate;
	bool				m_addr2IsStage;
	bool				m_bGlobal;
	CHtString			m_rdStg;
	CHtString			m_wrStg;
	bool				m_bMaxIw;
	bool				m_bMaxMw;
	bool				m_bPrivGbl;
	int					m_addrW;	// sum of addr0W + addr1W + addr2W
	string				m_privName;

	ERam				m_eRam;
	CType *				m_pType;
	CNgvInfo *			m_pNgvInfo;
};

struct CNgvModInfo {
	CNgvModInfo(CModule * pMod, CRam * pNgv) : m_pMod(pMod), m_pNgv(pNgv) {}
	CModule * m_pMod;
	CRam * m_pNgv;
};

struct CSpanningField {
	CSpanningField() {
		m_bIgnore = false;
		m_bForced = false;
		m_bSpanning = false;
		m_bDupRange = false;
		m_bMrField = false;
		m_pDupField = 0;
	}

public:
	CLineInfo m_lineInfo;
	string m_ramName;
	CField * m_pField;
	CType * m_pType;
	string m_heirName;
	int m_pos;
	int m_width;
	bool m_bForced;
	bool m_bIgnore;
	bool m_bSpanning;
	bool m_bDupRange;
	bool m_bMrField;
	CSpanningField * m_pDupField;
};

struct CNgvInfo {
	CNgvInfo() : m_atomicMask(0), m_ngvReplCnt(0) {}
	vector<CNgvModInfo> m_modInfoList;
	vector<CSpanningField> m_spanningFieldList;
	int m_atomicMask;
	string m_ngvWrType;
	HtdFile::ERamType m_ramType;
	bool m_bNeedQue;
	bool m_bNgvWrDataClk2x;
	bool m_bNgvWrCompClk2x;
	bool m_bNgvAtomicFast;
	bool m_bNgvAtomicSlow;
	bool m_bUserSpanningWrite;
	bool m_bAutoSpanningWrite;
	bool m_bNgvMaxSel;
	int m_ngvFieldCnt;
	int m_ngvReplCnt;
	int m_wrCompStg;
	int m_wrDataStg;
	vector<pair<int, int> > m_wrPortList;
	bool m_bAllWrPortClk1x;
	CModule * m_pRdMod;
	int m_rdModCnt;
	int m_rdPortCnt;

	bool m_bOgv;
};

struct CQueIntf : CRecord {
	CQueIntf(string &modName, string &queName, HtdFile::EQueType queType, string queueW, HtdFile::ERamType ramType, bool bMaxBw)
		: m_queType(queType), m_queueW(queueW), m_ramType(ramType), m_bMaxBw(bMaxBw)
	{

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
		CStack(CRecord * pStruct, size_t fieldIdx, int bitPos) : m_pRecord(pStruct), m_fieldIdx(fieldIdx), m_heirBitPos(bitPos), m_elemBitPos(0) {}
		CRecord * m_pRecord;
		int m_fieldIdx;
		string m_heirName;
		int m_heirBitPos;
		int m_elemBitPos;	// element dimen offset
		vector<int> m_refList;
	};
public:
	// methods are defined in GenUtilities.cpp
	CStructElemIter(CDsnInfo * pDsnInfo, CType * pType, bool bFollowNamedStruct = true, bool bDimenIter = true);

	bool end();
	CField & operator()();
	CField * operator->();
	void operator ++ (int);
	bool IsStructOrUnion();
	string GetHeirFieldName(bool bAddPreDot = true);
	int GetHeirFieldPos();
	int GetWidth();
	CType * GetType();
	int GetMinAlign();
	bool IsSigned();
	int GetAtomicMask();

private:
	CDsnInfo * m_pDsnInfo;
	bool m_bDimenIter;
	bool m_bFollowNamedStruct;
	CType * m_pType;
	vector<CStack> m_stack;
};

struct CModIdx {
	CModIdx() : m_bIsUsed(false) {}
	CModIdx(CInstance * pModInst, CCxrCall * pCxrCall, size_t callIdx, string modPath)
		: m_pModInst(pModInst), m_pCxrCall(pCxrCall), m_callIdx(callIdx), m_bIsUsed(true), m_modPath(modPath)
	{
	}
	CModIdx(CInstance * pModInst, CCxrEntry * pCxrEntry, size_t entryIdx, string modPath)
		: m_pModInst(pModInst), m_pCxrEntry(pCxrEntry), m_entryIdx(entryIdx), m_bIsUsed(true), m_modPath(modPath)
	{
	}
	CModIdx(CInstance * pModInst, CCxrReturn * pCxrReturn, size_t returnIdx, string modPath)
		: m_pModInst(pModInst), m_pCxrReturn(pCxrReturn), m_returnIdx(returnIdx), m_bIsUsed(true), m_modPath(modPath)
	{
	}

	CCxrCall * GetCxrCall();
	CCxrEntry * GetCxrEntry();
	CCxrReturn * GetCxrReturn();

	CInstance *	m_pModInst;
	union {
		size_t		m_callIdx;
		size_t		m_entryIdx;
		size_t		m_returnIdx;
	};
private:
	union {
		CCxrCall * m_pCxrCall;
		CCxrEntry * m_pCxrEntry;
		CCxrReturn * m_pCxrReturn;
	};
public:
	bool		m_bIsUsed;
	int			m_replCnt;
	string		m_modPath;
};

struct CCallInstParam {
	CCallInstParam(string const &name, string const &value) : m_name(name), m_value(value), m_lineInfo(CPreProcess::m_lineInfo) {}

	string m_name;
	string m_value;
	CLineInfo m_lineInfo;
};

// Call/Transfer/Return interface info
struct CCxrCall {
	// Constructor used for AddCall
	CCxrCall(string const &modEntry, string const &callName, string const &instName, bool bCall, bool bFork, string const &queueW, string const &dest, bool bXfer)
		: m_modEntry(modEntry), m_callName(callName), m_instName(instName), m_bCall(bCall), m_bFork(bFork), m_queueW(queueW), m_dest(dest), m_bXfer(bXfer)
	{

		m_lineInfo = CPreProcess::m_lineInfo;
		m_bCallFork = bFork;
		m_bXferFork = false;
	}

	void AddInstParam(string const &name, string const &value) {
		m_instParamList.push_back(CCallInstParam(name, value));
	}

public:
	CHtString		m_modEntry;		// for Xfer and Call - name of target function
	CHtString		m_callName;
	string			m_instName;
	bool			m_bCall;		// bCall and bFork can both be set
	bool			m_bFork;
	CHtString		m_queueW;
	string			m_dest;
	bool			m_bXfer;		// if bXfer then bCall and bFork are false

	CThreads *		m_pGroup;
	CCxrEntry *		m_pPairedEntry;
	CModIdx			m_pairedEntry;
	CModIdx			m_pairedFunc;
	int				m_forkCntW;

	bool			m_bCallFork;
	bool			m_bXferFork;

	vector<CCallInstParam> m_instParamList;

	CLineInfo		m_lineInfo;
};

struct CCxrEntry {
	CCxrEntry(string &modEntry, string &entryInstr, string &reserve)
		: m_modEntry(modEntry), m_entryInstr(entryInstr), m_reserveCnt(reserve)
	{

		m_lineInfo = CPreProcess::m_lineInfo;
		m_bCallFork = false;
		m_bXferFork = false;
		m_bIsUsed = false;
	}

	CCxrEntry & AddParam(string hostType, CType * pType, string paramName, bool bIsUsed)
	{
		m_paramList.push_back(new CField(hostType, pType, paramName, bIsUsed));
		return *this;
	}

	CHtString	m_modEntry;
	string		m_entryInstr;
	CHtString	m_reserveCnt;

	CModule *		m_pMod;
	CThreads *		m_pGroup;

	vector<CField *>	m_paramList;

	bool m_bCallFork;
	bool m_bXferFork;
	bool m_bIsUsed;

	CLineInfo	m_lineInfo;
};

struct CCxrReturn {
	CCxrReturn(string const &modEntry) : m_modEntry(modEntry)
	{

		m_lineInfo = CPreProcess::m_lineInfo;
		m_bRtnJoin = false;
	}

	CCxrReturn & AddParam(string &hostType, CType * pType, string &paramName, bool bIsUsed)
	{
		m_paramList.push_back(new CField(hostType, pType, paramName, bIsUsed));
		return *this;
	}

	CHtString		m_modEntry;

	CThreads *		m_pGroup;
	vector<CField *>	m_paramList;

	bool			m_bRtnJoin;

	CLineInfo		m_lineInfo;
};

struct CFunction {
	CFunction(CType * pType, string &name) : m_name(name), m_pType(pType)
	{
		m_lineInfo = CPreProcess::m_lineInfo;
	}

	CFunction & AddParam(string &dir, CType * pType, string &name)
	{
		m_paramList.push_back(new CField(dir, pType, name));

		return *this;
	}

	string		m_name;
	CType *		m_pType;

	vector<CField *>	m_paramList;
	CLineInfo		m_lineInfo;
};

struct CThreads {
	CThreads()
	{
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
	CType *		m_pRtnSelType;
	CType *		m_pRtnInstrType;
	CType *		m_pRtnHtIdType;
	CHtString	m_htIdW;
	CType *		m_pHtIdType;
	bool		m_bPause;
	CLineInfo	m_lineInfo;

	CRam		m_smCmd;	// list of fields in sm command (tid/state)
	CRecord		m_htPriv;	// list of field in tid info ram
	bool		m_bCallFork;
	bool		m_bXferFork;
	bool		m_bRtnJoin;
};

struct CHifMsg {
	CHifMsg(string msgName, string regName, string dataBase, string dataWidth, string idxBase, string idxWidth) :
		m_msgName(msgName), m_stateName(regName), m_dataLsb(dataBase), m_dataW(dataWidth),
		m_idxBase(idxBase), m_idxWidth(idxWidth)
	{

		if (m_stateName.substr(0, 2) == "r_" || m_stateName.substr(0, 2) == "m_")
			m_stateName = m_stateName.substr(2);

		m_bRamState = false;
	}

	CHifMsg(string msgName, string memName, string fieldName, string dataBase, string dataWidth, string idxBase, string idxWidth) :
		m_msgName(msgName), m_stateName(memName), m_fieldName(fieldName), m_dataLsb(dataBase), m_dataW(dataWidth),
		m_idxBase(idxBase), m_idxWidth(idxWidth)
	{

		if (m_stateName.substr(0, 2) == "r_" || m_stateName.substr(0, 2) == "m_")
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
	CFunc(string funcName, string funcRtnType)
	{
		m_funcName = funcName;
		m_rtnType = funcRtnType;
	}

public:
	string	m_funcName;
	string	m_rtnType;
};

struct CScPrim {
	CScPrim(string scPrimDecl, string scPrimFunc)
	{
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

struct CRefDimen {
	CRefDimen(int value, int size) : m_value(value), m_size(size), m_isIdx(false) {}
	int m_value;
	int m_size;
	bool m_isIdx;
};

struct CRefAddr {
	CRefAddr(int value, int sizeW, bool isHtId) : m_value(value), m_sizeW(sizeW), m_isIdx(false), m_isHtId(isHtId) {}
	int m_value;
	int m_sizeW;
	bool m_isIdx;
	bool m_isHtId;
};

struct CVarAddr {
	CVarAddr(int sizeW, bool isHtId) : m_sizeW(sizeW), m_isHtId(isHtId) {}
	int m_sizeW;
	bool m_isHtId;
};

struct CFieldRef {
	string m_fieldName;
	vector<CRefDimen> m_refDimenList;
	vector<CRefAddr> m_refAddrList;
};

struct CMifRdDst {
	CMifRdDst(string const & name, string const & var, 
		string const & memSrc, string const & atomic, CType * pRdType) :
		m_name(name), m_var(var), m_memSrc(memSrc), m_atomic(atomic), m_pRdType(pRdType)
	{
		m_lineInfo = CPreProcess::m_lineInfo;
		m_pSharedVar = 0;
		m_pPrivVar = 0;
		m_pGblVar = 0;

		m_fldW = 0;
		m_dstW = 0;
		m_varAddr0W = 0;
		m_varAddr1W = 0;
		m_varAddr2W = 0;
		m_varAddr1IsHtId = false;
		m_varAddr2IsHtId = false;
		m_varDimen1 = 0;
		m_varDimen2 = 0;
		m_fldDimen1 = 0;
		m_fldDimen2 = 0;
	}

	CMifRdDst(string name, string infoW, string stgCnt, string const & elemCntW, string memSrc, CType * pRdType) :
		m_name(name), m_infoW(infoW), m_rsmDly(stgCnt), m_elemCntW(elemCntW), m_memSrc(memSrc), m_pRdType(pRdType)
	{
		m_lineInfo = CPreProcess::m_lineInfo;
		m_pSharedVar = 0;
		m_pPrivVar = 0;
		m_pGblVar = 0;
		m_atomic = "read";

		m_fldW = 0;
		m_dstW = 0;
		m_varAddr0W = 0;
		m_varAddr1W = 0;
		m_varAddr2W = 0;
		m_varAddr1IsHtId = false;
		m_varAddr2IsHtId = false;
		m_varDimen1 = 0;
		m_varDimen2 = 0;
		m_fldDimen1 = 0;
		m_fldDimen2 = 0;
	}

	CHtString	m_name;
	string		m_var;
	CHtString	m_infoW;
	CHtString	m_rsmDly;
	CHtString	m_elemCntW;
	bool        m_bMultiQwRdReq;
	bool		m_bMultiQwHostRdReq;
	bool		m_bMultiQwCoprocRdReq;
	bool		m_bMultiQwHostRdMif;
	bool		m_bMultiQwCoprocRdMif;
	bool		m_bMultiElemRd;
	string      m_memSrc;
	string      m_atomic;

	CLineInfo	m_lineInfo;

	CType *		m_pDstType;
	CType *		m_pRdType;
	CField *	m_pSharedVar;
	CField *	m_pPrivVar;
	CRam *		m_pGblVar;

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
	bool m_varAddr1IsHtId;
	bool m_varAddr2IsHtId;
	int m_maxElemCnt;

	vector<CFieldRef> m_fieldRefList;
};

struct CMifRd {
	CMifRd()
	{
		m_bHostRdReq = false;
		m_bCoprocRdReq = false;
		m_bMultiQwRdReq = false;
		m_bMultiQwHostRdReq = false;
		m_bMultiQwCoprocRdReq = false;
		m_bMultiQwHostRdMif = false;
		m_bMultiQwCoprocRdMif = false;
		m_bMultiQwRdRsp = false;
		m_bRdRspCallBack = false;
		m_bNeedRdRspInfoRam = false;
		m_bPause = false;
		m_bPoll = false;
		m_bMaxBw = false;
		m_bRspGrpIdPriv = false;
	}

public:
	CHtString	m_queueW;
	CHtString	m_rspGrpId;
	CHtString	m_rspGrpW;
	CHtString	m_rspCntW;
	bool		m_bMaxBw;
	bool        m_bPause;
	bool        m_bPoll;

	CLineInfo	m_lineInfo;

	int			m_rspGrpIdW;
	bool		m_bRspGrpIdPriv;
	bool		m_bHostRdReq;
	bool		m_bCoprocRdReq;
	bool        m_bMultiQwRdReq;
	bool		m_bMultiQwHostRdReq;
	bool		m_bMultiQwCoprocRdReq;
	bool		m_bMultiQwHostRdMif;
	bool		m_bMultiQwCoprocRdMif;
	bool        m_bMultiQwRdRsp;
	int			m_maxRsmDly;
	int			m_maxRdRspInfoW;
	bool		m_bNeedRdRspInfoRam;
	bool		m_bRdRspCallBack;
	int			m_rdRspStg;

	vector<int>			m_rdRspIdxWList;
	vector<CMifRdDst>	m_rdDstList;
};

struct CMifWrSrc {
	CMifWrSrc(string const & name, CType * pType, string const & var, string const & memDst,
		CType * pWrType) :
		m_name(name), m_pType(pType), m_var(var), m_memDst(memDst), m_pWrType(pWrType)
	{
		m_lineInfo = CPreProcess::m_lineInfo;
		m_pSharedVar = 0;
		m_pPrivVar = 0;
		m_pGblVar = 0;

		m_fldW = 0;
		m_srcW = 0;
		m_varAddr0W = 0;
		m_varAddr1W = 0;
		m_varAddr2W = 0;
		m_varAddr1IsHtId = false;
		m_varAddr2IsHtId = false;
		m_varAddr1IsIdx = false;
		m_varAddr2IsIdx = false;
		m_varDimen1 = 0;
		m_varDimen2 = 0;
		m_fldDimen1 = 0;
		m_fldDimen2 = 0;
	}

	CHtString	m_name;
	CType *		m_pType;
	string		m_var;
	string      m_memDst;

	CLineInfo	m_lineInfo;

	CType *		m_pSrcType;
	CType *		m_pWrType;
	CField *	m_pSharedVar;
	CField *	m_pPrivVar;
	CRam *		m_pGblVar;
	CField *	m_pField;
	bool        m_bMultiQwWrReq;
	bool        m_bMultiQwHostWrReq;
	bool        m_bMultiQwCoprocWrReq;
	bool        m_bMultiQwHostWrMif;
	bool        m_bMultiQwCoprocWrMif;
	bool		m_bMultiElemWr;
	bool		m_bConstDimenList;

	int m_memSize;	// size in bits of write data

	int m_fldW;
	int m_srcW;		// width of info in write request tid
	int m_varAddr0W;
	int m_varAddr1W;
	int m_varAddr2W;
	int m_varDimen1;
	int m_varDimen2;
	int m_fldDimen1;
	int m_fldDimen2;
	bool m_varAddr1IsHtId;
	bool m_varAddr2IsHtId;
	bool m_varAddr1IsIdx;
	bool m_varAddr2IsIdx;
	int m_maxElemCnt;
	string m_wrDataTypeName;

	vector<CFieldRef> m_fieldRefList;
};

struct CMifWr {
public:
	CMifWr()
	{
		m_bHostWrReq = false;
		m_bCoprocWrReq = false;
		m_bMultiQwWrReq = false;
		m_bMultiQwHostWrReq = false;
		m_bMultiQwCoprocWrReq = false;
		m_bMultiQwHostWrMif = false;
		m_bMultiQwCoprocWrMif = false;
		m_bRspGrpIdPriv = false;
		m_bDistRamAccessReq = false;
		m_bBlockRamAccessReq = false;
		m_bMaxBw = false;
		m_bPause = false;
		m_bPoll = false;
		m_bReqPause = false;

		m_rspGrpIdW = 0;
	}

public:
	CHtString	m_queueW;
	CHtString	m_rspGrpId;
	CHtString	m_rspGrpW;
	CHtString	m_rspCntW;
	bool		m_bMaxBw;
	bool        m_bPause;
	bool        m_bPoll;
	bool		m_bReqPause;

	CLineInfo	m_lineInfo;

	int			m_rspGrpIdW;
	bool		m_bRspGrpIdPriv;
	bool		m_bHostWrReq;
	bool		m_bCoprocWrReq;
	bool        m_bMultiQwWrReq;
	bool        m_bMultiQwHostWrReq;
	bool        m_bMultiQwCoprocWrReq;
	bool        m_bMultiQwHostWrMif;
	bool        m_bMultiQwCoprocWrMif;
	bool		m_bDistRamAccessReq;
	bool		m_bBlockRamAccessReq;

	vector<CMifWrSrc>		m_wrSrcList;
};

class CMif {
public:
	CMif()
	{
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

	int		m_mifReqStgCnt;
	bool	m_bSingleReqMode;
	int		m_maxDstW;
	int		m_maxSrcW;
	int		m_maxElemCnt;

	CMifRd	m_mifRd;
	CMifWr	m_mifWr;

	vector<int> m_memReqIdxWList;
};

class COutHostMsg {
public:
	COutHostMsg()
	{
		m_bOutHostMsg = false;
	}

	bool		m_bOutHostMsg;
	CLineInfo	m_lineInfo;
};

struct CMsgDst {
	CMsgDst(string &var, string &dataLsb, string &addr1Lsb, string &addr2Lsb, string &idx1Lsb,
		string &idx2Lsb, string &field, string &fldIdx1Lsb, string &fldIdx2Lsb, bool bReadOnly)
		: m_var(var), m_dataLsb(dataLsb), m_addr1Lsb(addr1Lsb), m_addr2Lsb(addr2Lsb), m_idx1Lsb(idx1Lsb),
		m_idx2Lsb(idx2Lsb), m_field(field), m_fldIdx1Lsb(fldIdx1Lsb), m_fldIdx2Lsb(fldIdx2Lsb), m_bReadOnly(bReadOnly)
	{

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
	CMsgIntf(string name, string dir, CType * pType, string dimen, string replCnt, string queueW, string reserve, bool autoConn)
		: m_name(name), m_dir(dir), m_pType(pType), m_pTypeIntf(pType), m_dimen(dimen),
		m_fanCnt(replCnt), m_queueW(queueW), m_reserve(reserve), m_bAutoConn(autoConn)
	{
		m_lineInfo = CPreProcess::m_lineInfo;
		m_pIntfList = 0;
		m_bAeConn = false;
		m_bInboundQueue = false;
	}

	string	m_name;
	string	m_dir;
	CType * m_pType;
	CType * m_pTypeIntf;
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
	CBarrier(string name, string barIdW) : m_name(name), m_barIdW(barIdW)
	{
		m_lineInfo = CPreProcess::m_lineInfo;
	}

	CHtString	m_name;
	CHtString	m_barIdW;

	CLineInfo	m_lineInfo;
};

struct CStream {
	CStream(bool bRead, string &name, CType * pType, string &strmBw, string &elemCntW, string &strmCnt,
		string &memSrc, vector<int> &memPort, string &access, string &reserve, bool paired, bool bClose, CType * pTag, string &rspGrpW)
		: m_bRead(bRead), m_name(name), m_pType(pType), m_strmBw(strmBw), m_elemCntW(elemCntW), m_strmCnt(strmCnt),
		m_memSrc(memSrc), m_memPort(memPort), m_access(access), m_reserve(reserve),
		m_paired(paired), m_bClose(bClose), m_pTag(pTag), m_rspGrpW(rspGrpW)
	{
		m_lineInfo = CPreProcess::m_lineInfo;
	}

	bool		m_bRead;
	CHtString	m_name;
	CType *		m_pType;
	CHtString	m_strmBw;
	CHtString	m_elemCntW;
	CHtString	m_strmCnt;
	CHtString	m_memSrc;
	vector<int>	m_memPort;
	CHtString	m_access;
	CHtString	m_reserve;
	bool		m_paired;
	bool		m_bClose;
	CType *		m_pTag;
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
	CStencilBuffer(string &name, CType * pType, vector<int> &gridSize,
		vector<int> &stencilSize, string &pipeLen)
		: m_name(name), m_pType(pType), m_gridSize(gridSize), m_stencilSize(stencilSize), m_pipeLen(pipeLen)
	{
		m_lineInfo = CPreProcess::m_lineInfo;
	}

	string m_name;
	CType * m_pType;
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
	CModMemPort() : m_bIsMem(false), m_bIsStrm(false), m_bRead(false), m_bWrite(false), 
		m_bMultiQwRdReq(false), m_bMultiQwWrReq(false), m_queueW(0)
	{
		m_rdStrmCnt = 0;
		m_wrStrmCnt = 0;
		m_portIdx = 0;
	}

	bool m_bIsMem;
	bool m_bIsStrm;
	bool m_bRead;
	bool m_bWrite;
	bool m_bMultiQwRdReq;
	bool m_bMultiQwWrReq;
	int m_queueW;

	int m_rdStrmCnt;
	int m_rdStrmCntW;
	int m_wrStrmCnt;
	int m_wrStrmCntW;

	vector<CMemPortStrm> m_strmList;
	int m_portIdx;
};

struct CInstanceParams {
	CInstanceParams(string const &modInstName, vector<int> &memPortList, int freq) : m_modInstName(modInstName), m_memPortList(memPortList)
	{
		m_lineInfo = CPreProcess::m_lineInfo;
	}

	CInstanceParams()
	{
		m_replCnt = -1;
	}

	string m_modInstName;
	vector<int> m_memPortList;
	int m_replCnt;

	CLineInfo m_lineInfo;
};

struct CCxrInstCall {
	CCxrInstCall(CCxrCall * pCxrCall) : m_pCxrCall(pCxrCall) {}

	CCxrCall * m_pCxrCall;
	vector<CModIdx> m_returnList;
};

struct CCxrInstReturn {
	CCxrInstReturn(CCxrReturn * pCxrReturn) : m_pCxrReturn(pCxrReturn) {}

	CCxrReturn * m_pCxrReturn;
	vector<CModIdx> m_callerList;
};

struct CCxrInstEntry {
	CCxrInstEntry(CCxrEntry * pCxrEntry) : m_pCxrEntry(pCxrEntry) {}

	CCxrEntry * m_pCxrEntry;
	vector<CModIdx> m_callerList;
};

// Instance info for a module
struct CInstance {
	//CInstance() {}
	CInstance(CModule *pMod, string const & instName, string const & modPath, CInstanceParams &modInstParams,
		int cxrSrcCnt, int replCnt = 1, int replId = 0);

	CModule *			m_pMod;
	CInstanceParams		m_instParams;
	CHtString			m_instName;
	CHtString			m_fileName;
	vector<string>		m_modPaths;		// module instance path name if specified in instance file
	vector<CCallInstParam>	m_callInstParamList;
	vector<CCxrInstReturn>	m_cxrInstReturnList;
	vector<CCxrInstCall> m_cxrInstCallList;
	int					m_cxrSrcCnt;
	vector<CCxrInstEntry> m_cxrInstEntryList;
	vector<CCxrIntf *>	m_cxrIntfList;	// List of interfaces for instance
	CHtString			m_replInstName;		// module replicated instance name
	int					m_instId;
	int					m_replCnt;
	int					m_replId;
};

struct CStage : CRecord {
	CStage() {
		m_bStageNums = false;
	}

	CHtString	m_privWrStg;
	CHtString	m_execStg;
	CLineInfo	m_lineInfo;
	bool		m_bStageNums;
};

struct CModuleInstParam {
	CModuleInstParam(string const &name, string const &default) : m_name(name), m_default(default), m_lineInfo(CPreProcess::m_lineInfo) {}

	string m_name;
	string m_default;
	CLineInfo m_lineInfo;
};

struct CInstSet {
	CInstSet() {
		m_totalInstCnt = 0;
	}
	int GetTotalCnt() { return m_totalInstCnt; }
	int GetInstCnt() { return (int)m_instSet.size(); }
	int GetReplCnt(int instIdx) { return (int)m_instSet[instIdx].size(); }

	void AddInst(CInstance * pModInst) {
		if (pModInst->m_replId == 0) {
			vector<CInstance *> tmp;
			m_instSet.push_back(tmp);
		}
		m_instSet.back().push_back(pModInst);
		m_totalInstCnt += 1;

		pModInst->m_instId = (int)m_instSet.size() - 1;
		pModInst->m_replId = (int)m_instSet.back().size() - 1;
	}

	CInstance * GetInst(int instCnt, int replCnt=0) {
		return m_instSet[instCnt][replCnt];
	}
private:
	int m_totalInstCnt;	// includes replicated inst count
	vector<vector<CInstance *> > m_instSet;
};

struct CModule {
	CModule(string modName, HtdFile::EClkRate clkRate, bool bIsUsed)
	{
		m_modName = modName;
		m_clkRate = clkRate;

		m_replInstName = "inst";

		m_bHasThreads = false;
		m_bRtnJoin = false;
		m_bCallFork = false;
		m_bXferFork = false;
		m_bHostIntf = false;
		m_bContAssign = false;
		m_bIsUsed = bIsUsed;
		m_bActiveCall = false;
		m_bInHostMsg = false;
		m_resetInstrCnt = 0;
		m_maxRtnReplCnt = 0;
		m_phaseCnt = 0;
		m_cxrEntryReserveCnt = 0;
		m_bResetShared = true;
		m_rsmSrcCnt = 0;
		m_bGvIwComp = false;
		m_gvIwCompStg = 0;
		m_pInstrType = 0;
	}

	void AddHostIntf()
	{
		m_bHostIntf = true;
	}

	void AddHtFunc(string funcName, string funcRtnType)
	{
		m_htFuncList.push_back(CFunc(funcName, funcRtnType));
	}

	CCxrEntry & AddEntry(string &funcName, string &entryInstr, string &reserve)
	{
		m_cxrEntryList.push_back(new CCxrEntry(funcName, entryInstr, reserve));
		m_cxrEntryList.back()->m_pGroup = &m_threads;

		return *m_cxrEntryList.back();
	}

	vector<CRam *> * AddGlobal()
	{
		return & m_ngvList;
	}

	CQueIntf & AddQueIntf(string modName, string queName, HtdFile::EQueType queType, string queueW, HtdFile::ERamType ramType = HtdFile::eDistRam, bool bMaxBw = false)
	{
		CQueIntf queIntf(modName, queName, queType, queueW, ramType, bMaxBw);
		m_queIntfList.push_back(queIntf);

		return m_queIntfList.back();
	}

	CHostMsg * AddHostMsg(HtdFile::EHostMsgDir msgDir, string msgName)
	{
		for (size_t msgIdx = 0; msgIdx < m_hostMsgList.size(); msgIdx += 1)
			if (m_hostMsgList[msgIdx].m_msgName == msgName) {
			if (m_hostMsgList[msgIdx].m_msgDir != msgDir)
				m_hostMsgList[msgIdx].m_msgDir = HtdFile::InAndOutbound;
			return &m_hostMsgList.back();
			}

		m_hostMsgList.push_back(CHostMsg(msgDir, msgName));
		return &m_hostMsgList.back();
	}

	void AddThreads(string htIdW, string resetInstr, bool bPause = false)
	{
		if (m_bHasThreads) {
			CPreProcess::ParseMsg(Error, "module %s already has a thread group", m_modName.Lc().c_str());
			return;
		}

		m_bHasThreads = true;
		m_threads.m_htIdW = htIdW;
		m_threads.m_resetInstr = resetInstr;
		m_threads.m_bPause = bPause;
		m_threads.m_lineInfo = CPreProcess::m_lineInfo;
		m_threads.m_ramType = HtdFile::eAutoRam;

		if (resetInstr.size() > 0)
			m_resetInstrCnt += 1;
	}

	CRecord * AddPrivate()
	{
		return &m_threads.m_htPriv;
	}

	void AddInstParam(string const &name, string const &default) {
		m_instParamList.push_back(CModuleInstParam(name, default));
	}

public:
	CHtString		m_modName;

	string			m_smTidName;
	string			m_smStateName;

	string			m_replInstName;

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
	int				m_maxRtnReplCnt; // maximum replication count for destination of return links
	size_t			m_rsmSrcCnt;

	CMif			m_mif;
	int				m_resetInstrCnt;	// number of groups with reset instructions
	int				m_cxrEntryReserveCnt;	// sum of AddEntry reserve values

	bool			m_bHasThreads;
	CThreads			m_threads;

	bool				m_gblDistRam;
	bool				m_gblBlockRam;

	bool				m_bGvIwComp;
	int					m_gvIwCompStg;

	CInstSet			m_instSet;
	vector<CCxrCall *>	m_cxrCallList;
	vector<CCxrEntry *>	m_cxrEntryList;
	vector<CCxrReturn *>	m_cxrReturnList;
	vector<CRam *>		m_ngvList;
	vector<CQueIntf>	m_queIntfList;
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
	vector<CModuleInstParam> m_instParamList;

	CStage				m_stage;
	CRecord				m_global;
	CRecord				m_shared;
	bool				m_bResetShared;
	string				m_smInitState;
	CType *				m_pInstrType;
	bool				m_bInHostMsg;
	int					m_phaseCnt;

	COutHostMsg			m_ohm;
};

struct CDefine {
	CDefine(string const &defineName, vector<string> const & paramList, string const &defineValue,
		bool bPreDefined, bool bHasParamList = false, string scope = string("unit"), string modName = string(""))
		: m_name(defineName), m_paramList(paramList), m_value(defineValue), m_scope(scope),
		m_modName(modName), m_bPreDefined(bPreDefined), m_bHasParamList(bHasParamList)
	{
	}

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
		bool bPreDefined = false, bool bHasParamList = false, string defineScope = string("unit"), string modName = string(""));
	bool FindStringValue(CLineInfo const &lineInfo, const char * &pValueStr, int &rtnValue, bool & bIsSigned);
	bool FindStringValue(CLineInfo const &lineInfo, const string &name, int &rtnValue, bool & bIsSigned)
	{
		char const * pValueStr = name.c_str();
		return FindStringValue(lineInfo, pValueStr, rtnValue, bIsSigned);
	}
	bool FindStringIsSigned(const string &name)
	{
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
		m_scope(scope), m_modName(modName)
	{
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

// info to instanciate a mif instance
struct CMifInst {
	CMifInst(int mifId, bool bMifRead, bool bMifWrite) : m_mifId(mifId), m_bMifRead(bMifRead), m_bMifWrite(bMifWrite) {}

	int m_mifId;
	bool m_bMifRead;
	bool m_bMifWrite;
};
struct CRdAddr {
	CRdAddr(string &addr1Name, string &addr2Name, HtdFile::ERamType ramType, int rdStg, string addrW)
		: m_addr1Name(addr1Name), m_addr2Name(addr2Name), m_ramType(ramType), m_rdStg(rdStg), m_addrW(addrW)
	{
	}

	bool operator== (CRdAddr const & rhs)
	{
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
		: m_srcModName(srcModName), m_srcParamsList(srcParamsList), m_dstModName(dstModName), m_dstParamsList(dstParamsList)
	{
	}

	CHtString	m_srcModName;
	vector<CInstanceParams>	m_srcParamsList;
	CHtString	m_dstModName;
	vector<CInstanceParams>	m_dstParamsList;
};

//struct CParamList {
//	const char *m_pName;
//	void *		m_pValue;
//	bool		m_bRequired;
//	EParamType	m_paramType;
//	int			m_deprecatedPair;
//	bool		m_bPresent;
//};

struct CBramTarget {
	string		m_name;
	int			m_depth;
	int			m_width;
	int			m_copies;
	int			m_slices;
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

	CDsnInfo()
	{
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

		CModule * pHifMod = AddModule(new CModule("hif", eClk1x, true));

		char sHostHtIdW[32];
		sprintf(sHostHtIdW, "%d", g_appArgs.GetHostHtIdW());

		AddDefine(pHifMod, "HIF_HTID_W", sHostHtIdW);

		pHifMod->AddThreads("HIF_HTID_W", "");
		pHifMod->AddHostIntf();

		bool bMifMultiRdSupport = g_appArgs.GetCoprocInfo().GetMaxHostQwReadCnt() > 1;

		AddReadMem(pHifMod, "5", "", "", "5", false, true, false, bMifMultiRdSupport);
		AddWriteMem(pHifMod, "5", "", "", "5", false, true, false, false, true);

		if (g_appArgs.GetEntryName().size() > 0) {
			bool bCall = true;
			bool bFork = false;
			AddCall(pHifMod, g_appArgs.GetEntryName(), g_appArgs.GetEntryName(), "", bCall, bFork, string("0"), string("auto"));
		}

		if (g_appArgs.GetIqModName().size() > 0)
			pHifMod->AddQueIntf(g_appArgs.GetIqModName(), "hifInQue", Push, "5");

		if (g_appArgs.GetOqModName().size() > 0)
			pHifMod->AddQueIntf(g_appArgs.GetOqModName(), "hifOutQue", Pop, "5");

		AddDefine(pHifMod, "HIF_INSTR_W", "0");

		CHtString::SetDefineTable(&m_defineTable);
		CHtIntParam::SetDefineTable(&m_defineTable);

		string name = "MEM_ADDR_W";
		string value = "48";
		vector<string> paramList;
		// jacobi depends on this, do not generate in code, it is in MemRdWrIntf.h
		m_defineTable.Insert(name, paramList, value, true, false, string("auto"));

		m_htModId = 0;
		m_maxReplCnt = 0;
	}

	virtual ~CDsnInfo() {}

	// Routines called by HtdFile::loadHtdFile
	void AddIncludeList(vector<CIncludeFile> const & includeList);
	void AddDefine(void * pModule, string name, string value, string scope = "unit", string modName = "");
	void AddTypeDef(string name, string type, string scope, string modName, string width);

	CModule * AddModule(string name, EClkRate clkRate);
	CRecord * AddStruct(string name, bool bCStyle, bool bUnion, EScope scope, bool bInclude = false, string modName = "");

	void AddModInstParam(CModule *pModule, string const &name, string const &default);
	CMifRd * AddReadMem(CModule * pModule, string queueW, string rspGrpId, string rspGrpW, string rspCntW, bool maxBw, bool bPause, bool bPoll, bool bMultiRd = false);
	CMifWr * AddWriteMem(CModule * pModule, string queueW, string rspGrpId, string rspGrpW, string rspCntW, bool maxBw, bool bPause, bool bPoll, bool bReqPause, bool bMultiWr = false);
	CCxrCall * AddCall(void * pModule, string const &modEntry, string const &xferName, string const &modInst, bool bCall, bool bFork, string const &queueW, string const &dest);
	CCxrCall * AddXfer(void * pModule, string const &modEntry, string const &xferName, string const &modInst, string const &queueW);
	CCxrEntry * AddEntry(CModule * pModule, string funcName, string entryInstr, string reserve, bool &bHost);
	CCxrReturn * AddReturn(void * pModule, string func);
	CStage * AddStage(CModule * pModule, string privWrStg, string execStg);
	CRecord * AddShared(void * pModule, bool bReset);
	CFunction * AddFunction(CModule * pModule, CType * pType, string name)
	{
		pModule->m_funcList.push_back(CFunction(pType, name));
		return &pModule->m_funcList.back();
	}
	void AddThreads(void * pModule, string htIdW, string resetInstr, bool bPause = false);
	void AddInstr(void * pModule, string name);
	void AddMsgIntf(void * pModule, string name, string dir, CType * pType, string dimen,
		string replCnt, string queueW, string reserve, bool autoConn);
	void AddTrace(void * pModule, string name);
	void AddScPrim(void * pModule, string scPrimDecl, string scPrimCall);
	void AddPrimstate(void * pModule, string type, string name, string include);
	void * AddHostMsg(void * pModule, HtdFile::EHostMsgDir msgDir, string msgName);
	void AddHostData(void * pModule, HtdFile::EHostMsgDir msgDir, bool bMaxBw);
	void AddGlobalVar(vector<CRam *> * pGlobalList, CType * pType, string name, string dimen1, string dimen2,
		string addr1W, string addr2W, string addr1, string addr2, string rdStg, string wrStg,
		bool bMaxIw, bool bMaxMw, ERamType ramType, bool bRead, bool bWrite, bool bSpanningWrite);
	void AddBarrier(void * pModule, string name, string barIdW);
	void AddStream(void * pStruct, bool bRead, string &name, CType * pType, string &strmBw, string &elemCntW, string &strmCnt,
		string &memSrc, vector<int> &memPort, string &access, string &reserve, bool paired, bool bClose, CType * pTag, string &rspGrpW);
	void AddStencilBuffer(void * pStruct, string &name, CType * pType, vector<int> &gridSize, vector<int> &stencilSize, string &pipeLen);

	void AddStageField(CStage * pStage, CType * pType, string name, string dimen1, string dimen2, string *pRange,
		bool bInit, bool bConn, bool bReset, bool bZero);

	void AddEntryParam(CCxrEntry * pEntry, string hostType, CType * pType, string paramName, bool bIsUsed) { pEntry->AddParam(hostType, pType, paramName, bIsUsed); }
	void AddReturnParam(CCxrReturn * pReturn, string hostType, CType * pType, string paramName, bool bIsUsed) { pReturn->AddParam(hostType, pType, paramName, bIsUsed); }
	void AddFunctionParam(CFunction * pFunction, string dir, CType * pType, string name) { pFunction->AddParam(dir, pType, name); }
	void AddCallInstParam(CCxrCall * pCall, string const &name, string const &value) { pCall->AddInstParam(name, value); }

	void * AddMsgDst(void * pOpenIhm, string var, string dataLsb, string addr1Lsb, string addr2Lsb,
		string idx1Lsb, string idx2Lsb, string field, string fldIdx1Lsb, string fldIdx2Lsb, bool bReadOnly);

	CMifWr * AddSrc(CMifWr * pMifWr, string const & name, CType * pType,
		string const & var, string const & memDst, CType * pWrType);
	void AddDst(CMifRd * pOpenMifRd, string const & name, string const & var,
		string const & memSrc, string const & atomic, CType * pRdType);
	void AddDst(CMifRd * pOpenMifRd, string name, string infoW, string stgCnt,
		string const & elemCntW, string memSrc, CType * pRdType);

	// Routines called by HtdFile::saveHtdFile
	bool GetModule(string &name) { return false; }
	bool GetDefines(string &name, string &value, string &scope) { return false; }
	bool GetTypedefs(string &name, string &type, string &width, string &scope) { return false; }
	bool GetStructs(string &name, bool &bUnion, bool &bHost, string &scope) { return false; }
	bool GetStructField(string &type, string &name, string &dimen1, string &dimen2) { return false; }
	bool GetGlobalRam(string &name, string &dimen1, string &dimen2, string &addr1W,
		string &addr2W, string &addr1, string &addr2, string &rdStg, string &wrStg, string &extern_)
	{
		return false;
	}
	bool GetGlobalField(string &type, string &name, string &dimen1, string &dimen2,
		string &read, string &write, string &ramType)
	{
		return false;
	}
	bool GetSharedVars(string &type, string &name, string &addr1W, string &addr2W, string &queueW,
		string &dimen1, string &dimen2, string &blockRam)
	{
		return false;
	}
	bool GetThreadGroups(string &name, string &groupW, string &resetInstr) { return false; }
	bool NeedClk2x();

	void InitOptNgv();
	void InitBramUsage();
	void InitMifRamType();
	void ReportRamFieldUsage();
	void DrawModuleCmdRelationships();

	void InitializeAndValidate();
	void InitAddrWFromAddrName();
	void InitAndValidateHif();
	void InitAndValidateModMif();
	void InitAndValidateModMif2();
	void InitAndValidateModCxr();
	void InitAndValidateModIpl();
	//void InitAndValidateModRam();
	void InitAndValidateModIhm();
	void InitAndValidateModOhm();
	void InitAndValidateModIhd();
	void InitAndValidateModMsg();
	void InitAndValidateModBar();
	void InitAndValidateModStrm();
	void InitAndValidateModStBuf();
	void InitAndValidateModNgv();
	void InitPrivateAsGlobal();

	void LoadDefineList();
	void AddTypeDef(string name, string type, string width, bool bInitWidth, string scope = string("unit"), string modName = string(""));
	bool FindFieldInStruct(CLineInfo const &lineInfo, string const &type, string const &fldName,
		bool bIndexCheck, bool &bCStyle, CField const * &pBaseField, CField const * &pLastField);
	bool IsInFieldList(CLineInfo const &lineInfo, string const &addrName, vector<CField *> const &fieldList,
		bool bCStyle, bool bIndexCheck, CField const * &pBaseField, CField const * &pLastField, string * pFullName);
	bool FindCIntType(string typeName, int &width, bool &bSigned);
	bool FindStructName(string const &structType, string &structName);
	int FindHostTypeWidth(CField const * pField);
	int FindHostTypeWidth(string const &varName, string const &typeName, CHtString const &bitWidth, CLineInfo const &lineInfo);
	void ValidateHostType(CType * pType, CLineInfo const & lineInfo);
	bool FindVariableWidth(CLineInfo const &lineInfo, CModule &mod, string name, bool &bHtId, bool &bPrivate, bool &bShared, bool &bStage, int &addr1W);
	bool FindFieldRefWidth(CLineInfo const &lineInfo, string const &fieldRef, vector<CField *> const &fieldList, int &varW);
	float FindSlicePerBramRatio(int depth, int width);
	int FindSliceCnt(int depth, int width);
	int FindBramCnt(int depth, int width);
	void InitNativeCTypes();
	bool IsNativeCType(string &type, bool bAllowHostPtr = false);
	void ValidateDesignInfo();
	void InitAndValidateTypes();
	void InitAndValidateRecord(CRecord * pRecord);
	string ParseTemplateParam(CLineInfo const & lineInfo, char const * &pStr, bool &error);
	CType * FindType(string const & typeName, int fieldWidth = -1, CLineInfo const & lineInfo = CLineInfo());
	CType * FindClangType(string typeName);
	CType * FindHtIntType(string typeName, CLineInfo const & lineInfo);
	CType * FindHtIntType(ESign sign, int width);
	CType * FindScBigIntType(ESign sign, int width);
	int FindClangWidthAndMinAlign(CField * pField, int & minAlign);
	void InitFieldDimenValues(vector<CField *> &fieldList);
	void GenPersFiles();
	void ZeroStruct(CHtCode &code, string fieldBase, vector<CField *> &fieldList);
	CField * FindField(CLineInfo & lineInfo, CRecord * pStruct, string & fieldName);
	void ParseFieldRefList(CLineInfo & lineInfo, string & type, string & field, vector<CFieldRef> & fieldRefList);
	void ParseVarRefSpec(CLineInfo & lineInfo, string const & name, vector<CHtString> const & dimenList, vector<CVarAddr> const & varAddrList,
		string const & type, string & varRefSpec, vector<CFieldRef> & varRefList, CType * &pType);

	string GenIndexStr(bool bGen, char const * pFormat, int index);
	void InitCxrIntfInfo();
	bool IsCallDest(CInstance * pSrcModInst, CInstance * pDstModInst);
	void CheckRequiredEntryNames(vector<CModIdx> &callStk);
	bool CheckTransferReturn(vector<CModIdx> &callStk);
	void CreateDirectoryStructure();
	void GenerateCommonIncludeFile();
	void GenGlobalVarWriteTypes(CHtFile & htFile, CType * pType, int &atomicMask, vector<string> &gvTypeList, CRam * pGv = 0);
	void GenerateUnitTopFile();
	void GenerateAeTopFile();
	void GenerateHtaFiles();
	void GenerateHtiFiles();
	void GenerateHifFiles();
	void GenerateMifFiles(int mifId);
	void GenerateNgvFiles();
	void GenerateModuleFiles(CModule &modInfo);
	void GenModInstInc(CModule &mod);
	void GenStruct(FILE *incFp, string intfName, CRecord &ram, EGenStructMode mode = eGenStruct, bool bEmptyContructor = false);
	void GenPersBanner(CHtFile &htFile, const char *unitName, const char *dsnName, bool is_h, const char *incName=0);
	void GenerateBanner(CHtFile &htFile, const char *fileName, bool is_h);
	void GenIntfStruct(FILE *incFp, string intfName, vector<CField *> &fieldList, bool bCStyle, bool bInclude, bool bData64, bool bUnion);
	void GenRamIntfStruct(FILE *incFp, string intfName, CRam &ram, EStructType type);
	void GenUserStructs(FILE *incFp, CRecord * pUserRecord, char const * pTabs = "");
	void GenUserStructs(CHtCode &htFile, CRecord * pUserRecord, char const * pTabs = "");
	void GenStructIsEqual(CHtCode &htFile, char const * pTabs, string prefixName, string &typeName, vector<CField *> &fieldList, bool bCStyle, const char *&pStr, EStructType structType = eStructAll, bool bHeader = true, int idxInit = 0);
	void GenStructAssign(CHtCode &htFile, char const * pTabs, string prefixName, string &typeName, vector<CField *> &fieldList, bool bCStyle, const char *&pStr, EStructType structType = eStructAll, bool bHeader = true, int idxInit = 0);
	void GenStructAssignToZero(CHtCode &htFile, char const * pTabs, string prefixName, string &typeName, vector<CField *> &fieldList, bool bCStyle, const char *&pStr, EStructType structType = eStructAll, bool bHeader = true, int idxInit = 0);
	void GenStructScTrace(CHtCode &htFile, char const * pTabs, string prefixName1, string prefixName2, string &typeName, vector<CField *> &fieldList, bool bCStyle, const char *&pStr, EStructType structType = eStructAll, bool bHeader = true, int idxInit = 0);
	void GenStructStream(CHtCode &htFile, char const * pTabs, string prefixName, string &typeName, vector<CField *> &fieldList, bool bCStyle, const char *&pStr, EStructType structType = eStructAll, bool bHeader = true, int idxInit = 0);
	void GenModDecl(EVcdType vcdType, CHtCode &htFile, string &modName, VA type, VA var, vector<CHtString> const & dimenList = g_nullHtStringVec);
	void GenVcdDecl(CHtCode &sosCode, EVcdType vcdType, CHtCode &declCode, string &modName, VA type, VA var, vector<CHtString> const & dimenList = g_nullHtStringVec);
	void GenModVar(EVcdType vcdType, string &vcdModName, bool &bFirstModVar, string decl, string dimen, string name, string val, vector<CHtString> const & dimenList = g_nullHtStringVec);
	void GenModTrace(EVcdType vcdType, string &modName, VA name, VA val);
	void GenVcdTrace(CHtCode &sosCode, EVcdType vcdType, string &modName, VA name, VA val);
	string GenFieldType(CField * pField, bool bConst);
	void GenStructInit(FILE *fp, string &tabs, string prefixName, CField * pField, int idxCnt, bool bZero);

	enum EFieldListMode { FieldList, Union, Struct };
	void GenUserStructFieldList(FILE *incFp, bool bIsHtPriv, vector<CField *> &fieldList, bool bCStyle, EFieldListMode mode, string tabs, bool bUnion = false);
	void GenUserStructFieldList(CHtCode &htFile, bool bIsHtPriv, vector<CField *> &fieldList, bool bCStyle, EFieldListMode mode, string tabs, bool bUnion = false);
	void GenUserStructBadData(CHtCode &htFile, bool bHeader, string intfName, vector<CField *> &fieldList, bool bCStyle, string tabs);
	void GenSmCmd(FILE *incFp, CModule &modInfo);
	void GenCmdInfo(FILE *incFp, CModule &modInfo);
	void GenSmInfo(FILE *incFp, CRecord &smInfo);
	void GenRamWrEn(FILE *incFp, string intfName, CRecord &ram);

	void GenHtMon();

	void MarkNeededIncludeStructs();
	void MarkNeededIncludeStructs(CRecord * pRecord);
	bool FindStringValue(CLineInfo &lineInfo, string ramIdxW, int &ramIdxWidth) { bool bIsSigned; return m_defineTable.FindStringValue(lineInfo, ramIdxW, ramIdxWidth, bIsSigned); }

	bool IsInstanceLinkPresent(CHtString &, int, CHtString &, int) { return false; }

	void GenMsvsProjectFiles();

	void AddDesign(string unitName, string hostIntf)
	{
		m_unitName = unitName;
		m_hostIntf = hostIntf;
		m_bDumpIntf = false;
		m_bRndRetry = false;
		m_bRndInit = false;
		m_bBlockRamDoReg = false;//true;
	}

	void AddPerfMon(bool bPerfMon)
	{
		m_bPerfMon = bPerfMon;
	}

	void AddRndRetry(bool bRndRetry)
	{
		m_bRndRetry = bRndRetry;
	}

	void AddRndInit(bool bRndInit)
	{
		m_bRndInit = bRndInit;
	}

	CModule * AddModule(CModule * smInfo)
	{
		m_modList.push_back(smInfo);
		return m_modList.back();
	}

	CTypeDef * FindTypeDef(string typeName);
	CRecord * FindRecord(string recordName);

	bool IsTypeNameValid(string typeName, CModule * pMod = 0)
	{
		if (pMod) {
			string instrType = VA("%sInstr_t", pMod->m_modName.Uc().c_str());
			if (typeName == instrType)
				return true;
		}
		return IsInTypeList(typeName) || IsInStructList(typeName) || IsBaseType(typeName);
	}

	bool IsInTypeList(string &typeName)
	{
		for (size_t typeIdx = 0; typeIdx < m_typedefList.size(); typeIdx += 1)
			if (m_typedefList[typeIdx].m_name == typeName)
				return true;
		return false;
	}

	bool IsInStructList(string &typeName)
	{
		for (size_t typeIdx = 0; typeIdx < m_recordList.size(); typeIdx += 1)
			if (m_recordList[typeIdx]->m_typeName == typeName)
				return true;
		return false;
	}

	bool IsBaseType(string &type)
	{
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

	int			m_maxReplCnt;

	vector<CInstanceInfo>	m_instanceInfoList;

	vector<CHtInt *>	m_htIntList;
	vector<CHtInt *>	m_scBigIntList;
	vector<CModule *>	m_modList;
	vector<CInstance *>	m_dsnInstList;
	CDefineTable		m_defineTable;
	vector<CTypeDef>	m_typedefList;
	vector<CType>		m_typeList;
	vector<CRecord *>	m_recordList;
	vector<CMifInst>	m_mifInstList;
	vector<CNgvInfo *>	m_ngvList;

	vector<CBasicType>	m_nativeCTypeList;
	vector<CIncludeFile>	m_includeList;

	void GenPrimStateStatements(CModule &mod);
	void GenModTDSUStatements(CModule &mod); // typedef/define/struct/union
	void GenModMsgStatements(CModule &mod);
	void GenModBarStatements(CInstance * pModInst);
	void GenModIplStatements(CInstance * pModInst);
	void GenModIhmStatements(CModule &mod);
	void GenModOhmStatements(CInstance * pModInst);
	void GenModCxrStatements(CInstance * pModInst);
	void GenModIhdStatements(CInstance * pModInst);
	void GenModOhdStatements(CInstance * pModInst);
	void GenModMifStatements(CInstance * pModInst);
	void GenRamPreRegStatements(HtdFile::EClkRate eClk2x);
	void GenModStrmStatements(CInstance * pModInst);
	void GenModStBufStatements(CModule * pMod);
	void GenModNgvStatements(CInstance * pModInst);
	void GenModOptNgvStatements(CModule * mod, CRam * pGv);

	void FindSpanningWriteFields(CNgvInfo * pNgvInfo);
	void FindMemoryReadSpanningFields(CNgvInfo * pNgvInfo);

	void GenAeNextMsgIntf(HtiFile::CMsgIntfConn * pMicAeNext);
	void GenAePrevMsgIntf(HtiFile::CMsgIntfConn * pMicAePrev);

	void SetMsgIntfConnUsedFlags(bool bInBound, CMsgIntfConn * pConn, CModule &mod, CMsgIntf * pMsgIntf);

	void WritePersCppFile(CInstance * pModInst, bool bNeedClk2x);
	void WritePersIncFile(CInstance * pModInst, bool bNeedClk2x);
	//void GenStruct(string intfName, CRecord &ram, EGenStructMode mode, bool bEmptyContructor);
	void GenRamIntfStruct(CHtCode &code, char const * pTabs, string intfName, CRam &ram, EStructType type);
	void GenRamWrEn(CHtCode &code, char const * pTabs, string intfName, CRecord &ram);
	bool DimenIter(vector<CHtString> const &dimenList, vector<int> &refList);
	string IndexStr(vector<int> &refList, int startPos = -1, int endPos = -1, bool bParamStr = false);

	string GenRamAddr(CInstance & modInst, CRam &ram, CHtCode *pCode, string accessSelW, string accessSelName, const char *pInStg, const char *pOutStg, bool bWrite, bool bNoSelectAssign = false);
	//void GenRamAddr(CInstance &modInst, CHtCode &ramPreSm, string &addrType, CRam &ram, string ramAddrName, const char *pInStg, const char *pOutStg, bool bNoSelectAssign=true);
	int CountPhaseResetFanout(CInstance * pModInst);
	void GenDimenInfo(CDimenList & ramList, CDimenList & fieldList, vector<int> & dimenList, string & dimenDecl, string & dimenIndex, string * pVarIndex = 0, string * pFldIndex = 0);
	string GenRamIndexLoops(CHtCode &ramCode, vector<int> & dimenList, bool bOpenParen = false);
	string GenRamIndexLoops(CHtCode &ramCode, const char *pTabs, CDimenList &dimenList, bool bOpenParen = false, int idxInit = 0, string * pScTraceIndex = 0);
	void GenRamIndexLoops(FILE *fp, CDimenList &dimenList, const char *pTabs);
	void GenIndexLoops(CHtCode &htCode, string &tabs, vector<CHtString> & dimenList, bool bOpenParen = false);

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
	CHtCode m_iplReset1x;
	CHtCode	m_iplT0Stg2x;
	CHtCode	m_iplT1Stg2x;
	CHtCode	m_iplT2Stg2x;
	CHtCode	m_iplTsStg2x;
	CHtCode	m_iplPostInstr2x;
	CHtCode	m_iplReg2x;
	CHtCode	m_iplPostReg2x;
	CHtCode	m_iplOut2x;
	CHtCode m_iplReset2x;
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
	CHtCode m_mifBadDecl;
	CHtCode	m_mifPreInstr1x;
	CHtCode	m_mifPostInstr1x;
	CHtCode	m_mifRamClock1x;
	CHtCode	m_mifReg1x;
	CHtCode	m_mifPostReg1x;
	CHtCode	m_mifReset1x;
	CHtCode	m_mifOut1x;
	CHtCode	m_mifPreInstr2x;
	CHtCode	m_mifPostInstr2x;
	CHtCode	m_mifRamClock2x;
	CHtCode	m_mifReg2x;
	CHtCode	m_mifPostReg2x;
	CHtCode	m_mifReset2x;
	CHtCode	m_mifOut2x;
	CHtCode m_mifAvlCntChk;

	CHtCode m_vcdSos;	// vcd start_of_simulation

	// Sections of code for typedef, struct, union declarations
	CHtCode m_tdsuDefines;
};

extern CDsnInfo * g_pDsnInfo;

// Code genertion loop / unroll routines
struct CLoopInfo {
	CLoopInfo(CHtCode &htCode, string &tabs, vector<CHtString> & dimenList, int stmtCnt = 1, int initIdx = 0);
	string IndexStr(int startPos = -1, int endPos = -1, bool bParamStr = false);
	bool Iter(bool bNewLine = true);
	void LoopEnd(bool bNewLine);

	CHtCode & m_htCode;
	string & m_tabs;
	vector<CHtString> & m_dimenList;
	int m_stmtCnt;
	vector<int> m_refList;
	vector<bool> m_unrollList;
	int m_preDimCnt;
};

inline CInstance::CInstance(CModule *pMod, string const & instName, string const & modPath, CInstanceParams &modInstParams, 
	int cxrSrcCnt, int replCnt, int replId)
	: m_pMod(pMod), m_instParams(modInstParams), m_cxrSrcCnt(cxrSrcCnt), m_replCnt(replCnt), m_replId(replId)
{
	m_modPaths.push_back(modPath);
	m_instName = instName.size() == 0 ? pMod->m_modName : CHtString(instName);

	for (size_t i = 0; i < pMod->m_cxrReturnList.size(); i += 1)
		m_cxrInstReturnList.push_back(CCxrInstReturn(pMod->m_cxrReturnList[i]));

	for (size_t i = 0; i < pMod->m_cxrEntryList.size(); i += 1)
		m_cxrInstEntryList.push_back(CCxrInstEntry(pMod->m_cxrEntryList[i]));

	for (size_t i = 0; i < pMod->m_cxrCallList.size(); i += 1)
		m_cxrInstCallList.push_back(CCxrInstCall(pMod->m_cxrCallList[i]));
}

inline CCxrCall * CModIdx::GetCxrCall() {
	HtlAssert(m_pCxrCall == m_pModInst->m_pMod->m_cxrCallList[m_callIdx]);
	return m_pCxrCall;
}
inline CCxrEntry * CModIdx::GetCxrEntry() {
	HtlAssert(m_pCxrEntry == m_pModInst->m_pMod->m_cxrEntryList[m_callIdx]);
	return m_pCxrEntry;
}
inline CCxrReturn * CModIdx::GetCxrReturn() {
	HtlAssert(m_pCxrReturn == m_pModInst->m_pMod->m_cxrReturnList[m_callIdx]);
	return m_pCxrReturn;
}
