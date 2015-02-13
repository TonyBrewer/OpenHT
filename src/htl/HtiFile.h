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

struct CDsnInfo;
struct CInstanceParams;
struct CModule;
struct CMsgIntf;

class HtiFile {
public:
	struct CMsgIntfConn;
	struct CMsgIntfInfo;
public:
	HtiFile() {}
	virtual ~HtiFile() {}

	void setLex(CLex * pLex) { m_pLex = pLex; }

	void loadHtiFile(string fileName);

	int getUnitTypeCnt() { return 1; }
	string getUnitName(int unitTypeIdx) {
		return g_appArgs.GetUnitName();
	}

	void getModInstParams(string modPath, CInstanceParams & modInstParams);
	void checkModInstParamsUsed();
	size_t getMsgIntfConnListSize() { return m_msgIntfConn.size(); }
	CMsgIntfConn * getMsgIntfConn(size_t i) { return m_msgIntfConn[i]; }

	bool isMsgPathMatch(CLineInfo & lineInfo, CMsgIntfInfo & info, CModule &mod, CMsgIntf * pMsgIntf);

public:

	struct CParamList {
		const char *m_pName;
		void *		m_pValue;
		bool		m_bRequired;
		EToken		m_token;
		int			m_deprecatedPair;
		bool		m_bPresent;
	};

	class HtiList {
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

	struct CModInstParams {
		CModInstParams(string unitName, string modPath, vector<int> &memPortList, int instId, int replCnt) 
			: m_unitName(unitName), m_modPath(modPath), m_memPortList(memPortList), m_instId(instId),
			m_replCnt(replCnt), m_lineInfo(CPreProcess::m_lineInfo)
		{ m_wasUsed = false; }

		string m_unitName;
		string m_modPath;
		vector<int> m_memPortList;
		int m_instId;
		int m_replCnt;
		bool m_wasUsed;
		CLineInfo m_lineInfo;
	};

	struct CMsgIntfInfo {
		CMsgIntfInfo(bool bInBound, string &msgUnit, string &msgPath);

		bool m_bInBound;
		string m_unitName;
		int m_unitIdx;
		string m_modPath;
		int m_replIdx;
		string m_msgIntfName;
		vector<int> m_msgIntfIdx;

		CModule * m_pMod;
		CMsgIntf * m_pMsgIntf;
	};

	struct CMsgIntfConn {
		CMsgIntfConn( string &outUnit, string &outPath, string &inUnit, string &inPath, bool aeNext, bool aePrev ) :
			m_outUnit(outUnit), m_outPath(outPath), m_inUnit(inUnit), m_inPath(inPath),
			m_aeNext(aeNext), m_aePrev(aePrev), m_outMsgIntf(false, outUnit, outPath),
			m_inMsgIntf(true, inUnit, inPath), m_lineInfo(CPreProcess::m_lineInfo) {}

		string m_outUnit;
		string m_outPath;
		string m_inUnit;
		string m_inPath;
		bool m_aeNext;
		bool m_aePrev;

		CMsgIntfInfo m_outMsgIntf;
		CMsgIntfInfo m_inMsgIntf;

		CLineInfo m_lineInfo;

		string m_type;
	};

private:
	void ParseHtiMethods();
	vector<int> ExpandIntSet(string intSet) { return vector<int>(); }

	// routines to load an hti file
	void AddUnitInst( int ae, int au, string name ){}
	void AddUnitParams( string unit, string entry, string memPortCnt ){}
	void AddModInstParams( string unit, string modPath, vector<int> &memPort, string modInst, string replCnt );
	void AddMsgIntfConn( string &outUnit, string &outPath, string &inUnit, string &inPath, bool aeNext, bool aePrev );

	void SkipTo(EToken skipTk);

private:
	bool ParseParameters(CParamList *params);
	bool ParseIntRange(vector<int> * pIntList);

private:
	CLex * m_pLex;

	vector<CModInstParams> m_modInstParamsList;
	vector<CMsgIntfConn *> m_msgIntfConn;
};
