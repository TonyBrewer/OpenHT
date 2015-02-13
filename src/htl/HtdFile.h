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

#include "DsnInfo.h"

#include <stdio.h>

#include <string>
#include <vector>
#include <map>
using namespace std;

extern vector<CHtString> const g_nullHtStringVec;

enum EScope { eHost, eModule, eUnit };

struct CDsnInfo;

class HtdFile {
public:
	HtdFile() {
		m_parseWarn = true;
		m_pOpenMod = new Module("dsninfo"); 
	}
	virtual ~HtdFile() {}

	void setDsnInfo(CDsnInfo * pDsnInfo) { m_pDsnInfo = pDsnInfo; }
	void setLex(CLex * pLex) { m_pLex = pLex; }
	bool loadHtdFile(string fileName);

	static bool FindBuiltinType(string name, int & width, bool & bSigned);

public:
	enum EStructType { eStructAll, eStructRamRdData, eGenRamWrData, eGenRamWrEn };
	enum EGenStructMode { eGenStructRdData, eGenStructWrData, eGenStructWrEn, eGenStruct };
	enum EClkRate { eClk1x, eClk2x, eClkUnknown };
	enum ECmdType { eCmdCall, eCmdAsyncCall, eCmdRtn, eCmdAsyncRtn, eCmdXfer };
	enum ECmdDir { eCmdIn, eCmdOut };
	enum ERamType { eBlockRam, eDistRam, eAutoRam };
	enum EQueType { Push, Pop };
	enum EHostMsgDir { Inbound, Outbound, InAndOutbound };

public:

	struct CParamList {
		const char *m_pName;
		void *		m_pValue;
		bool		m_bRequired;
		EToken		m_token;
		int			m_deprecatedPair;
		bool		m_bPresent;
	};

	class HtdList {
	public:
		bool isInList(string name) {
			return index(name) >= 0;
		}
		void insert(string name) {
			m_list.push_back(name);
		}
		int index(string name) {
			for (size_t i = 0; i < m_list.size(); i += 1)
				if (m_list[i] == name)
					return (int)i;
			return -1;
		}
		void clear() {
			m_list.clear();
		};
	private:
		vector<string>	m_list;
	};

	struct Module {
		Module(string name) : m_name(name), m_handle(0) {}

		string	m_name;
		void *	m_handle;
		HtdList	m_callList;
		HtdList	m_entryList;
		HtdList	m_returnList;
		HtdList	m_globalList;
		HtdList	m_sharedList;
		HtdList	m_stageList;
		HtdList	m_instrList;
		HtdList	m_msgIntfList;
		HtdList	m_hostMsgList;
		HtdList	m_functionList;
		HtdList	m_traceList;
		HtdList	m_defineList;
		HtdList	m_typedefNameList;
		HtdList	m_structNameList;
		HtdList m_primStateList;
		HtdList m_barrierList;
		HtdList m_streamReadList;
		HtdList m_stencilBufferList;
	};

	class HtdModList {
	public:
		bool isInList(string name) {
			return index(name) >= 0;
		}
		Module * insert(string name) {
			m_list.push_back(Module(name));
			return &m_list.back();
		}
		int index(string name) {
			for (size_t i = 0; i < m_list.size(); i += 1)
				if (m_list[i].m_name == name)
					return (int)i;
			return -1;
		}
		Module * operator [] (int idx) { return &m_list[idx]; }
	private:
		vector<Module>	m_list;
	};

	struct HtdDsn {
		HtdList			m_defineNameList;
		HtdList			m_typedefNameList;
		HtdList			m_structNameList;
		HtdModList		m_modList;

		HtdList			m_functionParamList;
		HtdList			m_entryParamList;
		HtdList			m_returnParamList;
		HtdList			m_globalFieldList;
		HtdList			m_globalVarList;
		HtdList			m_privateFieldList;
		HtdList			m_structFieldList;
		//HtdList			m_stageVariableList;
		//HtdList			m_sharedVariableList;
		HtdList			m_mifRdDstList;
		HtdList			m_mifWrSrcList;
	};

private:
	string ParseType(int & builtinWidth, bool & bSignedBuiltin);
	void ParseTypedef();
	void ParseRecord();
	void ParseFieldList();
	string ParseArrayDimen();

	void (HtdFile::*m_pParseMethod)();

	void ParseDsnInfoMethods();
	void ParseModuleMethods();
	void ParseFunctionMethods();
	void ParseEntryMethods();
	void ParseReturnMethods();
	void ParseGlobalMethods();
	void ParseNewGlobalMethods();
	void ParsePrivateMethods();
	void ParseStructMethods();
	void ParseStageMethods();
	void ParseSharedMethods();
	void ParseMifRdMethods();
	void ParseMifWrMethods();
	void ParseIhmMethods();

	bool ParseParameters(CParamList *params, bool * bNewGlobal=0);
	bool ParseIntRange(vector<int> * pIntList);
	bool ParseRandomParameter(CParamList * pParam);

	void SkipTo(EToken skipTk) {
		EToken tk;
		do {
		    tk = m_pLex->GetNextTk();
		} while (tk != skipTk && tk != eTkEof);
	}

private:
	// routines to load a design
	virtual void AddIncludeList(vector<CIncludeFile> const & includeList) = 0;
	virtual void	AddDefine(void * pModule, string name, string value, string scope = "unit", string modName = "") = 0;
	virtual void	AddTypeDef(string name, string type, string width, string scope = string("unit"), string modName = string("")) = 0;
	virtual void	* AddModule(string name, EClkRate clkRate) = 0;
	virtual void	* AddStruct(string name, bool bCStyle, bool bUnion, EScope scope, bool bInclude=false, string modName = "") = 0;

	virtual bool IsInTypeList(string &name) = 0;
	virtual bool IsInStructList(string &name) = 0;

	virtual void * AddReadMem(void * pModule, string queueW, string rspGrpId, string rspCntW, bool maxBw, bool bPause, bool bPoll, bool bMultiRd=false) = 0;
	virtual void * AddWriteMem(void * pModule, string queueW, string rspGrpId, string rspCntW,bool maxBw, bool bPause, bool bPoll, bool bReqPause, bool bMultiWr=false) = 0;
	virtual void AddCall(void * pModule, string funcName, bool bCall, bool bFork, string queueW, string dest) = 0;
	virtual void AddXfer(void * pModule, string funcName, string queueW) = 0;
	virtual void * AddEntry(void * pModule, string funcName, string entryInstr, string reserve, bool &bHost) = 0;
	virtual void * AddReturn(void * pModule, string func) = 0;
	virtual void * AddStage(void * pModule, string privWrStg, string execStg ) = 0;
	virtual void * AddShared(void * pModule, bool bReset) = 0;
	virtual void * AddPrivate(void * pModule ) = 0;
	virtual void * AddFunction(void * pModule, string type, string name) = 0;
	virtual void AddThreads(void * pModule, string htIdW, string resetInstr, bool bPause) = 0;
	virtual void AddInstr(void * pModule, string name) = 0;
	virtual void AddTrace(void * pModule, string name) = 0;
	virtual void AddScPrim(void * pModule, string scPrimDecl, string scPrimCall) = 0;
	virtual void AddPrimstate(void * pModule, string type, string name, string include) = 0;
	virtual void * AddHostMsg(void * pModule, EHostMsgDir msgDir, string msgName) = 0;
	virtual void AddHostData(void * pModule, EHostMsgDir msgDir, bool bMaxBw) = 0;
	virtual void * AddGlobalRam(void * pModule, string ramName, string dimen1, string dimen2, string addr1W, string addr2W, string addr1, string addr2, string rdStg, string wrStg, bool bExtern) = 0;
	virtual void AddBarrier(void * pModule, string name, string barIdW) = 0;

	virtual void AddGlobalField(void * pStruct, string type, string name, string dimen1="", string dimen2="",
					bool bRead=true, bool bWrite=true, ERamType ramType = eDistRam) = 0;
	virtual void *AddStructField(void * pStruct, string type, string name, string bitWidth="", string base="", vector<CHtString> const &dimenList=g_nullHtStringVec,
					bool bSrcRead=true, bool bSrcWrite=true, bool bMifRead=false, bool bMifWrite=false, ERamType ramType = eDistRam, int atomicMask=0) = 0;
	virtual void * AddPrivateField(void * pStruct, string type, string name, string dimen1, string dimen2, string addr1W, string addr2W, string addr1, string addr2) = 0;
	virtual void * AddSharedField(void * pStruct, string type, string name, string dimen1, string dimen2, string rdSelW, string wrSelW,
							string addr1W, string addr2W, string queueW, ERamType ramType, string reset) = 0;
	virtual void * AddStageField(void * pStruct, string type, string name, string dimen1, string dimen2, string *pRange,
		bool bInit, bool bConn, bool bReset, bool bZero) = 0;

	virtual void AddEntryParam(void * pEntry, string hostType, string typeName, string paramName, bool bIsUsed) = 0;
	virtual void AddReturnParam(void * pReturn, string hostType, string typeName, string paramName, bool bIsUsed) = 0;
	virtual void AddFunctionParam(void * pFunction, string dir, string type, string name) = 0;

	virtual void * AddMsgDst(void * pOpenIhm, string var, string dataLsb, string addr1Lsb, string addr2Lsb,
		string idx1Lsb, string idx2Lsb, string field, string fldIdx1Lsb, string fldIdx2Lsb, bool bReadOnly) = 0;

	virtual void * AddSrc(void * pOpenMifWr, string name, string var, string field, bool bMultiWr, string srcIdx, string memDst) = 0;
	virtual void * AddDst(void * pOpenMifRd, string name, string var, string field, string dataLsb,
		bool bMultiRd, string dstIdx, string memSrc, string atomic, string rdType) = 0;
	virtual void * AddDst(void * pOpenMifRd, string name, string infoW, string stgCnt,
		bool bMultiRd, string memSrc, string rdType) = 0;

	// routines to save a design
	virtual bool GetModule(string &name) = 0;

	void * m_pOpenMifRd;
	void * m_pOpenMifWr;
	void * m_pOpenEntry;
	void * m_pOpenStruct;
	void * m_pOpenReturn;
	void * m_pOpenIhm;
	void * m_pOpenFunction;

private:
	CDsnInfo * m_pDsnInfo;
	CLex * m_pLex;
	Module * m_pOpenMod;
	HtdDsn * m_pDsn;
	bool m_parseWarn;
	void * m_currModuleHandle;

public:
	void * GetCurrModHandle() { return(m_currModuleHandle); }
};
