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

#include "HtString.h"

struct CThreads;
struct CField;
struct CModule;

// Call/return module interface
enum ECxrType { CxrCall, CxrReturn, CxrTransfer };
enum ECxrDir { CxrIn, CxrOut };

struct CCxrIntf {
	CCxrIntf(CHtString &funcName, CModule &srcMod, int srcInstIdx, CThreads *pSrcGroup, CModule &dstMod,
			int dstInstIdx, CThreads *pDstGroup, ECxrType cxrType, ECxrDir cxrDir, int queueW, vector<CField> * pFieldList)
		: m_funcName(funcName), m_pSrcMod(&srcMod), m_srcInstIdx(srcInstIdx), m_pSrcGroup(pSrcGroup),
		m_pDstMod(&dstMod), m_dstInstIdx(dstInstIdx), m_pDstGroup(pDstGroup),
		m_cxrType(cxrType), m_cxrDir(cxrDir), m_queueW(queueW), m_pFieldList(pFieldList), m_bCxrIntfFields(true),
		m_bRtnJoin(false), m_bMultiInstRtn(false), m_reserveCnt(0)
	{
		m_instCnt = 1;
		m_instIdx = 0;
		m_srcReplCnt = 1;
		m_srcReplId = 0;
		m_dstReplCnt = 1;
		m_dstReplId = 0;
	}

	const char * GetCxrTypeName() {
		return m_cxrType == CxrCall ? "Call" : (m_cxrType == CxrTransfer ? "Xfer" : "Rtn");
	}
	int GetQueDepthW() { return m_queueW; }

	int GetPortReplCnt() { return m_cxrDir == CxrIn ? m_srcReplCnt : m_dstReplCnt; }
	int GetPortReplId() { return m_cxrDir == CxrIn ? m_srcReplId : m_dstReplId; }

	const char * GetDstToSrcLc() {
		if (m_dstToSrcLc.size() == 0)
			m_dstToSrcLc = m_dstInstName.Lc() + "To" + m_srcInstName.Uc();
		return m_dstToSrcLc.c_str();
	}

	const char * GetSignalSrcToDstLc();
	const char * GetSignalDstToSrcLc();

	const char * GetSignalNameSrcToDstLc();
	const char * GetSignalNameDstToSrcLc();
	const char * GetSignalIndexSrcToDst();
	const char * GetSignalIndexDstToSrc();

	const char * GetPortNameSrcToDstLc();
	const char * GetPortNameDstToSrcLc();
	const char * GetPortReplIndex();
	const char * GetPortReplDecl();

	const char * GetSrcToDstLc() {
		if (m_srcToDstLc.size() == 0)
			m_srcToDstLc = m_srcInstName.Lc() + "To" + m_dstInstName.Uc();
		return m_srcToDstLc.c_str();
	}

	const char * GetSrcToDstUc();

	const char * GetIntfName() {
		if (m_intfName.size() == 0)
			m_intfName = m_funcName.Uc() + GetCxrTypeName();
		return m_intfName.c_str();
	}

	bool IsCall() { return m_cxrType == CxrCall; }
	bool IsXfer() { return m_cxrType == CxrTransfer; }
	bool IsCallOrXfer() { return m_cxrType == CxrCall || m_cxrType == CxrTransfer; }

	CHtString			m_funcName;
	CModule *			m_pSrcMod;
	int					m_srcInstIdx;
	CThreads *			m_pSrcGroup;
	CModule *			m_pDstMod;
	int					m_dstInstIdx;
	CThreads *			m_pDstGroup;
	ECxrType			m_cxrType;
	ECxrDir				m_cxrDir;
	int					m_queueW;
	vector<CField> *	m_pFieldList;
	vector<CField>		m_fullFieldList;

	string				m_entryInstr;
	int					m_rtnSelId;
	int					m_callIdx;
	int					m_rtnIdx;
	int					m_rtnReplId;
	int					m_callIntfIdx;	// matching outbound call interface for current return interface
	int					m_instCnt;
	int					m_instIdx;

	int					m_srcReplCnt;
	int					m_srcReplId;
	int					m_dstReplCnt;
	int					m_dstReplId;

	CHtString			m_srcInstName;
	CHtString			m_dstInstName;
	bool				m_bCxrIntfFields;

	bool				m_bRtnJoin;

	bool				m_bMultiInstRtn;
	int					m_reserveCnt;

private:
	string				m_signalSrcToDstLc;
	string				m_signalDstToSrcLc;
	string				m_signalNameSrcToDstLc;
	string				m_signalNameDstToSrcLc;
	string				m_signalIndexSrcToDst;
	string				m_signalIndexDstToSrc;
	string				m_srcToDstLc;
	string				m_srcToDstUc;
	string				m_dstToSrcLc;
	string				m_portNameSrcToDstLc;
	string				m_portNameDstToSrcLc;
	string				m_portReplDecl;
	string				m_portReplIndex;
	string				m_intfName;
};
