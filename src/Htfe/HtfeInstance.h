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

class CHtfeIdent;
class CHtfeSignal;
class CHtfeInstance;

/*********************************************************
** Instance Port table
*********************************************************/

class CHtfeInstancePort
{
public:
    CHtfeInstancePort(const string &name) : m_name(name) {}
public:
    string &GetName() { return m_name; }
    void SetSignal(CHtfeSignal *pSignal) { m_pSignal = pSignal; }
    CHtfeSignal *GetSignal() { return m_pSignal; }
    void SetModulePort(CHtfeIdent *pModulePort) { m_pModulePort = pModulePort; }
    CHtfeIdent *GetModulePort() { return m_pModulePort; }
    void SetInstance(CHtfeInstance *pInstance) { m_pInstance = pInstance; }
    CHtfeInstance *GetInstance() { return m_pInstance; }
private:
    string			m_name;
    CHtfeSignal     *m_pSignal;
    CHtfeIdent      *m_pModulePort;
    CHtfeInstance   *m_pInstance;
};

typedef map<string, CHtfeInstancePort>		InstancePortTblMap;
typedef pair<string, CHtfeInstancePort>		InstancePortTblMap_ValuePair;
typedef InstancePortTblMap::iterator		InstancePortTblMap_Iter;
typedef pair<InstancePortTblMap_Iter, bool>	InstancePortTblMap_InsertPair;

class CHtfeInstancePortTblIter;
class CHtfeInstancePortTbl  
{
    friend class CHtfeInstancePortTblIter;
public:
    CHtfeInstancePort * Insert(const string &name) {
        InstancePortTblMap_InsertPair insertPair;
        insertPair = m_signalTblMap.insert(InstancePortTblMap_ValuePair(name, CHtfeInstancePort(name)));
        return &insertPair.first->second;
    }
    CHtfeInstancePort * Find(const string &name) {
        InstancePortTblMap_Iter iter = m_signalTblMap.find(name);
        if (iter == m_signalTblMap.end())
            return 0;
        return &iter->second;
    }
    int GetCount() { return m_signalTblMap.size(); }
private:
    InstancePortTblMap		m_signalTblMap;
};

class CHtfeInstancePortTblIter
{
public:
    CHtfeInstancePortTblIter(CHtfeInstancePortTbl &signalTbl) : m_pInstancePortTbl(&signalTbl), m_iter(signalTbl.m_signalTblMap.begin()) {}
    void Begin() { m_iter = m_pInstancePortTbl->m_signalTblMap.begin(); }
    bool End() { return m_iter == m_pInstancePortTbl->m_signalTblMap.end(); }
    void operator ++ (int) { m_iter++; }
    CHtfeInstancePort* operator ->() { return &m_iter->second; }
    CHtfeInstancePort* operator () () { return &m_iter->second; }
private:
    CHtfeInstancePortTbl *m_pInstancePortTbl;
    InstancePortTblMap_Iter m_iter;
};

/*********************************************************
** Signal table
*********************************************************/

class CHtfeInstancePort;
typedef vector<CHtfeInstancePort *> CHtfeInstancePortList;

class CHtfeSignal
{
public:
    CHtfeSignal(const string &name) : m_name(name) {
        m_pType = 0; m_bIsClock = false; m_pTokenList = 0;
        m_inRefCnt = m_outRefCnt = m_inoutRefCnt = 0;
    }

    string &GetName() { return m_name; }
    void SetType(CHtfeIdent *pType) { m_pType = pType; }
    CHtfeIdent *GetType() { return m_pType; }
    void SetLineInfo(const CLineInfo &lineInfo) { m_lineInfo = lineInfo; }
    const CLineInfo &GetLineInfo() { return m_lineInfo; }
    void SetIsClock(bool bIsClock)
    { m_pTokenList = new CHtfeLex::CTokenList; m_bIsClock = bIsClock; }
    bool IsClock() { return m_bIsClock; }
    int GetRefCnt() { return m_inRefCnt + m_outRefCnt + m_inoutRefCnt; }
    int GetInRefCnt() { return m_inRefCnt; }
    int GetOutRefCnt() { return m_outRefCnt; }
    int GetInoutRefCnt() { return m_inoutRefCnt; }
    CHtfeLex::CTokenList const &GetClockTokenList() { return *m_pTokenList; }
	void AddReference(CHtfeInstancePort *pInstancePort);
	const CHtfeInstancePortList &GetRefList() { return m_refList; } 
    void AppendToken(CHtfeLex::EToken token, string &s) {
        CHtfeLex::CToken l(token, CHtfeLex::GetTokenString(token).length() > 0 ? 
            CHtfeLex::GetTokenString(token) : s);
        m_pTokenList->push_back(l);
    }
private:
    string		m_name;
    CHtfeIdent *  m_pType;
    short       m_inRefCnt;
    short       m_outRefCnt;
    short       m_inoutRefCnt;
    CLineInfo   m_lineInfo;
    bool        m_bIsClock;
    CHtfeLex::CTokenList  *m_pTokenList;
    CHtfeInstancePortList m_refList;
};

typedef map<string, CHtfeSignal>			SignalTblMap;
typedef pair<string, CHtfeSignal>			SignalTblMap_ValuePair;
typedef SignalTblMap::iterator			SignalTblMap_Iter;
typedef pair<SignalTblMap_Iter, bool>	SignalTblMap_InsertPair;

class CHtfeSignalTblIter;
class CHtfeSignalTbl  
{
    friend class CHtfeSignalTblIter;
public:
    CHtfeSignal * Insert(string &str) {
        SignalTblMap_InsertPair insertPair;
        insertPair = m_signalTblMap.insert(SignalTblMap_ValuePair(str, CHtfeSignal(str)));
        return &insertPair.first->second;
    }
    CHtfeSignal * Find(const string &str) {
        SignalTblMap_Iter iter = m_signalTblMap.find(str);
        if (iter == m_signalTblMap.end())
            return 0;
        return &iter->second;
    }
private:
    SignalTblMap		m_signalTblMap;
};

class CHtfeSignalTblIter
{
public:
    CHtfeSignalTblIter(CHtfeSignalTbl &signalTbl) : m_pSignalTbl(&signalTbl), m_iter(signalTbl.m_signalTblMap.begin()) {}
    void Begin() { m_iter = m_pSignalTbl->m_signalTblMap.begin(); }
    bool End() { return m_iter == m_pSignalTbl->m_signalTblMap.end(); }
    void operator ++ (int) { m_iter++; }
    CHtfeSignal* operator ->() { return &m_iter->second; }
    CHtfeSignal* operator () () { return &m_iter->second; }
private:
    CHtfeSignalTbl * m_pSignalTbl;
    SignalTblMap_Iter m_iter;
};

/*********************************************************
** Instance table
*********************************************************/

class CHtfeInstance
{
public:
    CHtfeInstance(const string &name) : m_name(name), m_bInUse(false) {}
public:
    string &GetName() { return m_name; }
    int GetPortCnt() { return m_portTbl.GetCount(); }
    CHtfeInstancePort *InsertPort(const string &name) { return m_portTbl.Insert(name); }
    bool IsInUse() { return m_bInUse; }
    void SetInUse(bool bInUse) { m_bInUse = bInUse; }
    void SetLineInfo(const CLineInfo &lineInfo) { m_lineInfo = lineInfo; }
    const CLineInfo &GetLineInfo() { return m_lineInfo; }
    void SetModule(CHtfeIdent *pModule) { m_pModule = pModule; }
    CHtfeIdent *GetModule() { return m_pModule; }
    CHtfeInstancePortTbl &GetInstancePortTbl() { return m_portTbl; }
private:
    string		m_name;
    bool		m_bInUse;
    CLineInfo	m_lineInfo;
    CHtfeIdent		*m_pModule;
    CHtfeInstancePortTbl m_portTbl;
};

typedef map<string, CHtfeInstance>		InstanceTblMap;
typedef pair<string, CHtfeInstance>		InstanceTblMap_ValuePair;
typedef InstanceTblMap::iterator		InstanceTblMap_Iter;
typedef pair<InstanceTblMap_Iter, bool>	InstanceTblMap_InsertPair;

class CHtfeInstanceTblIter;
class CHtfeInstanceTbl  
{
    friend class CHtfeInstanceTblIter;
public:
    CHtfeInstance * Insert(const string &name) {
        InstanceTblMap_InsertPair insertPair;
        insertPair = m_signalTblMap.insert(InstanceTblMap_ValuePair(name, CHtfeInstance(name)));
        return &insertPair.first->second;
    }
    CHtfeInstance * Find(const string &name) {
        InstanceTblMap_Iter iter = m_signalTblMap.find(name);
        if (iter == m_signalTblMap.end())
            return 0;
        return &iter->second;
    }
    int GetCount() { return m_signalTblMap.size(); }
private:
    InstanceTblMap		m_signalTblMap;
};

class CHtfeInstanceTblIter
{
public:
    CHtfeInstanceTblIter(CHtfeInstanceTbl &signalTbl) : m_pInstanceTbl(&signalTbl), m_iter(signalTbl.m_signalTblMap.begin()) {}
    void Begin() { m_iter = m_pInstanceTbl->m_signalTblMap.begin(); }
    bool End() { return m_iter == m_pInstanceTbl->m_signalTblMap.end(); }
    void operator ++ (int) { m_iter++; }
    CHtfeInstance* operator ->() { return &m_iter->second; }
    CHtfeInstance* operator () () { return &m_iter->second; }
private:
    CHtfeInstanceTbl *m_pInstanceTbl;
    InstanceTblMap_Iter m_iter;
};
