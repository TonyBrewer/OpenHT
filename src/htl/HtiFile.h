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

#include "HtString.h"

using namespace std;

struct CDsnInfo;
struct CInstanceParams;
struct CModule;
struct CMsgIntf;
struct CUioIntf;
struct CType;

class HtiFile {
public:
	struct CUioIntfConn;
	struct CUioIntfInfo;
	struct CMsgIntfConn;
	struct CMsgIntfInfo;
	struct CMsgIntfParams;
public:
	HtiFile() {}
	virtual ~HtiFile() {}

	void SetLex(CLex * pLex) { m_pLex = pLex; }

	void LoadHtiFile(string fileName);

	int GetUnitTypeCnt() { return 1; }
	string GetUnitName(int unitTypeIdx)
	{
		return g_appArgs.GetUnitName();
	}

	void GetModInstParams(string modPath, CInstanceParams & modInstParams);
	void CheckModInstParamsUsed();

	size_t GetMsgIntfConnListSize() { return m_msgIntfConn.size(); }
	CMsgIntfConn * GetMsgIntfConn(size_t i) { return m_msgIntfConn[i]; }
	size_t GetUioIntfConnListSize() { return m_uioIntfConn.size(); }
	CUioIntfConn * GetUioIntfConn(size_t i) { return m_uioIntfConn[i]; }
	size_t GetUioSimIntfConnListSize() { return m_uioSimIntfConn.size(); }
	CUioIntfConn * GetUioSimIntfConn(size_t i) { return m_uioSimIntfConn[i]; }

	bool IsMsgPathMatch(CLineInfo & lineInfo, CMsgIntfInfo & info, CModule &mod, CMsgIntf * pMsgIntf, int & instIdx);
	bool IsUioPathMatch(CLineInfo & lineInfo, CUioIntfInfo & info, CModule &mod, CUioIntf * pUioIntf, int & instIdx);


public:

	enum EParamType { ePrmInteger, ePrmIdent, ePrmIntRange, ePrmBoolean, ePrmTypeStr, ePrmIntList, ePrmString, ePrmParamStr, ePrmIdentRef, ePrmIntSet, ePrmParamLst, ePrmUnknown };

	struct CParamList {
		const char *m_pName;
		void *		m_pValue;
		bool		m_bRequired;
		EParamType	m_paramType;
		int			m_deprecatedPair;
		bool		m_bPresent;
	};

	class HtiList {
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

	struct CModInstParams {
		CModInstParams(string unitName, string modPath, vector<int> &memPortList, string const &modInstName, int replCnt, vector<pair<string, string> > & paramPairList)
			: m_unitName(unitName), m_modPath(modPath), m_memPortList(memPortList), m_modInstName(modInstName),
			m_replCnt(replCnt), m_paramPairList(paramPairList), m_lineInfo(CPreProcess::m_lineInfo)
		{
			m_wasUsed = false;
		}

		string m_unitName;
		string m_modPath;
		vector<int> m_memPortList;
		string m_modInstName;
		int m_replCnt;
		vector<pair<string, string> > m_paramPairList;
		bool m_wasUsed;
		CLineInfo m_lineInfo;
	};

	struct CMsgIntfPath {
		CMsgIntfPath(string &unit, string &path, bool bInBound);

		bool IsMatch(CInstance * pInst, string &msgName, bool bInBound, bool &bError);

		string m_unitName;
		string m_instPath;
		bool m_bInBound;

		int m_unitIdx;
		int m_instIdx;
		int m_replIdx;

		string m_msgIntfName;
		vector<int> m_msgIntfIdx;

		CLineInfo m_lineInfo;
	};

	struct CMsgIntfParams : CMsgIntfPath {
		CMsgIntfParams(string &unit, string &path, bool bInBound, string &fanIO) :
			CMsgIntfPath(unit, path, bInBound), m_fanIO(fanIO) 
		{
			m_bIsUsed = false;
		}

		CHtString m_fanIO;
		bool m_bIsUsed;
	};

	struct CMsgIntfInfo : CMsgIntfPath {
		CMsgIntfInfo(string &unit, string &path, bool bInBound) :
			CMsgIntfPath(unit, path, bInBound)
		{
			m_pMsgIntf = 0;
		}

		CModule * m_pMod;
		CMsgIntf * m_pMsgIntf;
	};

	struct CMsgIntfConn {
		CMsgIntfConn(string &outUnit, string &outPath, string &inUnit, string &inPath, bool aeNext, bool aePrev) :
			m_outUnit(outUnit), m_outPath(outPath), m_inUnit(inUnit), m_inPath(inPath),
			m_aeNext(aeNext), m_aePrev(aePrev), m_outMsgIntf(outUnit, outPath, false),
			m_inMsgIntf(inUnit, inPath, true), m_lineInfo(CPreProcess::m_lineInfo)
		{
		}

		string m_outUnit;
		string m_outPath;
		string m_inUnit;
		string m_inPath;
		bool m_aeNext;
		bool m_aePrev;

		CMsgIntfInfo m_outMsgIntf;
		CMsgIntfInfo m_inMsgIntf;

		CLineInfo m_lineInfo;

		CType * m_pType;
	};

	struct CUioIntfInfo : CMsgIntfPath {
		CUioIntfInfo(string &unit, string &path, bool bInBound) :
			CMsgIntfPath(unit, path, bInBound)
		{
			m_pMod = NULL;
			m_pUioIntf = NULL;
		}

		CModule * m_pMod;
		CUioIntf * m_pUioIntf;
	};

	struct CUioIntfConn {
		CUioIntfConn(string &uioPort, string &unit, string &path, bool bInbound) :
			m_uioPort(uioPort), m_path(path), m_bInbound(bInbound),
			m_uioIntf(unit, path, bInbound), m_lineInfo(CPreProcess::m_lineInfo)
		{
			m_syscSimConn = NULL;
		}

		string m_uioPort;
		string m_path;
		bool m_bInbound;

		CUioIntfInfo m_uioIntf;

		CLineInfo m_lineInfo;

		CUioIntfConn * m_syscSimConn;

		CType * m_pType;
	};

private:
	void ParseHtiMethods();
	vector<int> ExpandIntSet(string intSet) { return vector<int>(); }

	// routines to load an hti file
	void AddUnitInst(int ae, int au, string name) {}
	void AddUnitParams(string unit, string entry, string memPortCnt) {}
	void AddModInstParams(string unit, string modPath, vector<int> &memPort, string modInstName, string replCnt, vector<pair<string, string> > & paramPairList);
	void AddMsgIntfConn(string &outUnit, string &outPath, string &inUnit, string &inPath, bool aeNext, bool aePrev);
	void AddMsgIntfParams(string &unit, string &path, bool bInBound, string &fanCnt);
	void AddUioIntfConn(string &uioPort, string &path, bool bInbound);
	void AddUioSimIntfConn(string &uioPort, string &path, bool bInbound);

	void SkipTo(EToken skipTk);

private:
	bool ParseParameters(CParamList *params);
	bool ParseIntRange(vector<int> * pIntList);

private:
	CLex * m_pLex;

	vector<CModInstParams> m_modInstParamsList;
	vector<CMsgIntfConn *> m_msgIntfConn;
	vector<CUioIntfConn *> m_uioIntfConn;
	vector<CUioIntfConn *> m_uioSimIntfConn;

public:
	vector<CMsgIntfParams> m_msgIntfParamsList;
};
