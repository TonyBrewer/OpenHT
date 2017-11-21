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

#include <stdio.h>

#include <string>
#include <vector>
#include <map>
using namespace std;

extern vector<CHtString> const g_nullHtStringVec;

enum EScope { eHost, eModule, eUnit };

struct CModule;
struct CRecord;
struct CDsnInfo;
struct CCxrReturn;
struct CCxrCall;
struct CMifRd;
struct CMifWr;
struct CRam;
struct CFunction;
struct CCxrEntry;
struct CStage;

class HtdFile {
public:
	HtdFile()
	{
		m_parseWarn = true;
		m_pOpenMod = new Module("dsninfo");
	}
	virtual ~HtdFile() {}

	void setDsnInfo(CDsnInfo * pDsnInfo) { m_pDsnInfo = pDsnInfo; }
	void setLex(CLex * pLex) { m_pLex = pLex; }
	bool loadHtdFile(string fileName);

public:
	enum EStructType { eStructAll, eStructRamRdData, eGenRamWrData, eGenRamWrEn };
	enum EGenStructMode { eGenStructRdData, eGenStructWrData, eGenStructWrEn, eGenStruct };
	enum EClkRate { eClk1x, eClk2x, eClkUnknown };
	enum ECmdType { eCmdCall, eCmdAsyncCall, eCmdRtn, eCmdAsyncRtn, eCmdXfer };
	enum ECmdDir { eCmdIn, eCmdOut };
	enum ERamType { eBlockRam, eDistRam, eAutoRam, eUnknownRam, eRegRam };
	enum EQueType { Push, Pop };
	enum EHostMsgDir { Inbound, Outbound, InAndOutbound };

public:

	enum EParamType { ePrmInteger, ePrmInt, ePrmIdent, ePrmIntRange, ePrmBoolean, ePrmType, ePrmIntList, ePrmString, ePrmParamStr, ePrmIdentRef, ePrmParamLst, ePrmUnknown };

	struct CParamList {
		const char *m_pName;
		void *		m_pValue;
		bool		m_bRequired;
		EParamType	m_paramType;
		bool		m_bDeprecated;
		bool		m_bPresent;
		int			m_dupIdx;
	};

	class HtdList {
	public:
		bool isInList(string name)
		{
			return index(name) >= 0;
		}
		void insert(string name)
		{
			m_list.push_back(name);
		}
		int index(string name)
		{
			for (size_t i = 0; i < m_list.size(); i += 1)
				if (m_list[i] == name)
					return (int)i;
			return -1;
		}
		void clear()
		{
			m_list.clear();
		};
	private:
		vector<string>	m_list;
	};

	struct Module {
		Module(string name) : m_name(name), m_pModule(0) {}

		string	m_name;
		CModule * m_pModule;
		HtdList	m_callList;
		HtdList	m_entryList;
		HtdList	m_returnList;
		HtdList	m_globalList;
		HtdList	m_sharedList;
		HtdList	m_stageList;
		HtdList	m_instrList;
		HtdList	m_msgIntfList;
		HtdList	m_uioIntfList;
		HtdList	m_uioSimIntfList;
		HtdList	m_uioCsrIntfList;
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
		bool isInList(string name)
		{
			return index(name) >= 0;
		}
		Module * insert(string name)
		{
			m_list.push_back(Module(name));
			return &m_list.back();
		}
		int index(string name)
		{
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
		HtdList			m_ngvList;
		HtdList			m_privateFieldList;
		HtdList			m_structFieldList;
		//HtdList			m_stageVariableList;
		//HtdList			m_sharedVariableList;
		HtdList			m_mifRdDstList;
		HtdList			m_mifWrSrcList;
		HtdList			m_callInstParamList;
		HtdList			m_modInstParamList;
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
	void ParsePrivateMethods();
	//void ParseStructMethods();
	void ParseStageMethods();
	void ParseSharedMethods();
	void ParseMifRdMethods();
	void ParseMifWrMethods();
	void ParseIhmMethods();

	bool ParseParameters(CParamList *params);
	bool ParseIntRange(vector<int> * pIntList);
	bool ParseRandomParameter(CParamList * pParam);

	void SkipTo(EToken skipTk)
	{
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

	virtual bool IsInTypeList(string &name) = 0;
	virtual bool IsInStructList(string &name) = 0;

	virtual void AddThreads(void * pModule, string htIdW, string resetInstr, bool bPause) = 0;
	virtual void AddInstr(void * pModule, string name) = 0;
	virtual void AddTrace(void * pModule, string name) = 0;
	virtual void AddScPrim(void * pModule, string scPrimDecl, string scPrimCall) = 0;
	virtual void AddPrimstate(void * pModule, string type, string name, string include) = 0;
	virtual void * AddHostMsg(void * pModule, EHostMsgDir msgDir, string msgName) = 0;
	virtual void AddHostData(void * pModule, EHostMsgDir msgDir, bool bMaxBw) = 0;
	virtual void AddBarrier(void * pModule, string name, string barIdW) = 0;

	virtual void * AddMsgDst(void * pOpenIhm, string var, string dataLsb, string addr1Lsb, string addr2Lsb,
		string idx1Lsb, string idx2Lsb, string field, string fldIdx1Lsb, string fldIdx2Lsb, bool bReadOnly) = 0;

	// routines to save a design
	virtual bool GetModule(string &name) = 0;

	CMifRd * m_pOpenMifRd;
	CMifWr * m_pOpenMifWr;
	CCxrEntry * m_pOpenEntry;
	CRecord * m_pOpenRecord;
	CStage * m_pOpenStage;
	vector<CRam *> * m_pOpenGlobal;
	CCxrCall * m_pOpenCall;
	CCxrReturn * m_pOpenReturn;
	void * m_pOpenIhm;
	CFunction * m_pOpenFunction;

private:
	CDsnInfo * m_pDsnInfo;
	CLex * m_pLex;
	Module * m_pOpenMod;
	HtdDsn * m_pDsn;
	bool m_parseWarn;
	CModule * m_pParseModule;

public:
	void * GetCurrModHandle() { return m_pParseModule; }
};
