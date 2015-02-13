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

class CSbxClks {
public:
	CSbxClks() { m_bInit = false; }

	int GetClkCnt() { return (int)m_sbxClks.size(); }
	void AddClkInKhz(const string &name, int freqInKhz) { m_sbxClks.push_back(CSbxClk(name, freqInKhz)); }
	void Initialize();
	double GetRefPeriodInNs() { if (!m_bInit) Initialize(); return 1000000.0 / m_refFreqInKhz; }
	int GetRefFreqInKhz() { if (!m_bInit) Initialize(); return m_refFreqInKhz; }
	int GetVcoMul() { if (!m_bInit) Initialize(); return m_vcoMul; }
	int GetVcoFreqInKhz() { if (!m_bInit) Initialize(); return m_refFreqInKhz * m_vcoMul; }
	string &GetClkName(int i) { return m_sbxClks[i].m_name; }
	int GetClkDiv(int i) { return GetVcoFreqInKhz() / m_sbxClks[i].m_freqInKhz; }
	int GetClkFreqInKhz(int i) { return m_sbxClks[i].m_freqInKhz; }
	string &GetSlowClkName() { return m_sbxClks[0].m_name; }
	bool HasReset() { return m_sbxReset.size() > 0; }
	void SetResetName(string reset) { m_sbxReset = reset; }
	string GetResetName() { return m_sbxReset; }

public:
	struct CSbxClk {
		CSbxClk(const string &name, int freqInKhz) : m_name(name), m_freqInKhz(freqInKhz) {}
		string	m_name;
		int		m_freqInKhz;
	};

private:
	vector<CSbxClk>		m_sbxClks;
	string				m_sbxReset;	// reset signal name
private:
	bool				m_bInit;
	int					m_refFreqInKhz;
	int					m_vcoMul;
};

/////////////////////////////////////////////////////////////////
// Sandbox generation option table
//

typedef map<string, string>			SbxOptMap;
typedef SbxOptMap::iterator			SbxOptMap_Iter;
typedef pair<SbxOptMap_Iter, bool>	SbxOptMap_InsertPair;
typedef pair<string, string>		SbxOptMap_ValuePair;

class CSbxOptIter;
class CSbxOpt
{
	friend class CSbxOptIter;
public:
	string Insert(string optName, string optLine) {
		SbxOptMap_InsertPair insertPair;
		insertPair = m_sbxOptMap.insert(SbxOptMap_ValuePair(optName, optName + " " + optLine));
		return insertPair.first->second;
	}
	string * Find(const string &optName) {
		SbxOptMap_Iter iter = m_sbxOptMap.find(optName);
		if (iter == m_sbxOptMap.end())
			return 0;
		return &iter->second;
	}
private:
	SbxOptMap		m_sbxOptMap;
};

class CSbxOptIter
{
public:
	CSbxOptIter(CSbxOpt &sbxOpt) : m_pSbxOpt(&sbxOpt), m_iter(sbxOpt.m_sbxOptMap.begin()) {}
	void Begin() { m_iter = m_pSbxOpt->m_sbxOptMap.begin(); }
	bool End() { return m_iter == m_pSbxOpt->m_sbxOptMap.end(); }
	void operator ++ (int) { m_iter++; }
	string* operator ->() { return &m_iter->second; }
	string* operator () () { return &m_iter->second; }
private:
	CSbxOpt *m_pSbxOpt;
	SbxOptMap_Iter m_iter;
};
