/*****************************************************************************/
// Generated with htl
//
// Hybrid Thread File - ../ht/sysc/PersAuCommon.h
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2013 : Created by Convey Computer Corp. This file is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
#pragma once

//////////////////////////////////
// Common #define's

#define HT_INVALID 0
#define HT_CONT 1
#define HT_CALL 2
#define HT_RTN 3
#define HT_JOIN 4
#define HT_RETRY 5
#define HT_TERM 6
#define HT_PAUSE 7
#define HT_JOIN_AND_CONT 8

#define HIF_HTID_W          		0
#define HIF_INST_W          		0
#define CTL_INST_W          		1
#define CLK2X_INST_W        		1
#define CLK2X_RTN_INST_W    		1
#define CLK1X_INST_W        		1
#define CLK1X_RTN_INST_W    		1
#define CTL_HTID_W          		0
#define CLK2X_HTID_W        		0
#define CLK1X_HTID_W        		4

//////////////////////////////////
// Common typedef's

typedef sc_uint<CTL_INST_W>		CtlInst_t;
typedef sc_uint<CLK2X_INST_W>		Clk2xInst_t;
typedef sc_uint<CLK2X_RTN_INST_W>		Clk2xRtnInst_t;
typedef sc_uint<CLK1X_INST_W>		Clk1xInst_t;
typedef sc_uint<CLK1X_RTN_INST_W>		Clk1xRtnInst_t;
typedef sc_uint<CLK1X_HTID_W>		Clk1xHtId_t;

//////////////////////////////////
// Common structs and unions


#ifndef _HTV
#include "HtMon.h"
#endif

#ifdef _HTV
#define INT(a) a
#else
#define INT(a) int(a)
#define HT_CYC_1X ((long long)sc_time_stamp().value()/10000)
#endif

//////////////////////////////////
// Ht Assert interfaces

#ifdef HT_ASSERT
#	ifdef _HTV
#		define HtAssert(expression, info) \
		if (!c_htAssert.m_bAssert && !(expression)) {\
			c_htAssert.m_bAssert = true; \
			c_htAssert.m_lineNum = __LINE__; \
			c_htAssert.m_info = info;\
		}
#	else
#		define HtAssert(expression, info) \
		if (!(expression)) {\
			fprintf(stderr, "HtAssert: file %s, line %d, info 0x%x\n", __FILE__, __LINE__, (int)info);\
			assert(expression);\
			uint32_t *pNull = 0; *pNull = 0;\
		}
#	endif
#else
#	ifdef _HTV
#		define HtAssert(expression, info)
#	else
#		define HtAssert(expression, info) assert(expression)
#	endif
#endif

struct CHtAssertIntf {
	bool m_bAssert;
	bool m_bCollision;
	uint8_t m_module;
	uint16_t m_lineNum;
	uint32_t m_info;
//};

//struct CHtAssertIntf : CHtAssert {
	//CHtAssertIntf() {}
	bool operator == (const CHtAssertIntf & rhs) const {
		return
			m_bAssert == rhs.m_bAssert &&
			m_bCollision == rhs.m_bCollision &&
			m_module == rhs.m_module &&
			m_lineNum == rhs.m_lineNum &&
			m_info == rhs.m_info
			;
	}
	CHtAssertIntf & operator = (const CHtAssertIntf & rhs) {
		m_bAssert = rhs.m_bAssert;
		m_bCollision = rhs.m_bCollision;
		m_module = rhs.m_module;
		m_lineNum = rhs.m_lineNum;
		m_info = rhs.m_info;
		return *this;
	}
	CHtAssertIntf & operator = (int zero) {
		//assert(zero == 0);
		m_bAssert = 0;
		m_bCollision = 0;
		m_module = 0;
		m_lineNum = 0;
		m_info = 0;
		return *this;
	}
# ifdef HT_SYSC
	friend void sc_trace(sc_trace_file *tf, const CHtAssert & v, const std::string & NAME ) {
		sc_trace(tf, v.m_bAssert, NAME + ".m_bAssert");
		sc_trace(tf, v.m_bCollision, NAME + ".m_bCollision");
		sc_trace(tf, v.m_module, NAME + ".m_module");
		sc_trace(tf, v.m_lineNum, NAME + ".m_lineNum");
		sc_trace(tf, v.m_info, NAME + ".m_info");
	}
	friend ostream& operator << ( ostream& os,  CHtAssert const & v ) {
		os << "(" << "???" << ")";
		return os;
	}
# endif
};

//////////////////////////////////
// Host Control interfaces

#ifdef _HTV
struct CHostCtrlMsgIntf {
#else
struct CHostCtrlMsg {
#endif
	union {
		struct {
			uint64_t m_bValid:1;
			uint64_t m_msgType:7;
			uint64_t m_msgData:56;
		};
		uint64_t		m_data64;
	};
};

#ifdef _HTV
#define CHostCtrlMsg CHostCtrlMsgIntf
#else
struct CHostCtrlMsgIntf : CHostCtrlMsg {
	CHostCtrlMsgIntf() {}
	bool operator == (const CHostCtrlMsg & rhs) const {
		return
			m_bValid == rhs.m_bValid &&
			m_msgType == rhs.m_msgType &&
			m_msgData == rhs.m_msgData
			;
	}
	CHostCtrlMsg & operator = (const CHostCtrlMsg & rhs) {
		m_bValid = rhs.m_bValid;
		m_msgType = rhs.m_msgType;
		m_msgData = rhs.m_msgData;
		return *this;
	}
	CHostCtrlMsg & operator = (int zero) {
		assert(zero == 0);
		m_bValid = 0;
		m_msgType = 0;
		m_msgData = 0;
		return *this;
	}
# ifdef HT_SYSC
	friend void sc_trace(sc_trace_file *tf, const CHostCtrlMsg & v, const std::string & NAME ) {
		sc_trace(tf, v.m_bValid, NAME + ".m_bValid");
		sc_trace(tf, v.m_msgType, NAME + ".m_msgType");
		sc_trace(tf, v.m_msgData, NAME + ".m_msgData");
	}
	friend ostream& operator << ( ostream& os,  CHostCtrlMsg const & v ) {
		os << "(" << "???" << ")";
		return os;
	}
};
# endif
#endif

//////////////////////////////////
// Host Queue interfaces

#define HT_DQ_NULL	1
#define HT_DQ_DATA	2
#define HT_DQ_FLSH	3
#define HT_DQ_CALL	6
#define HT_DQ_HALT	7
#ifdef _HTV
struct CHostDataQueIntf {
#else
struct CHostDataQue {
#endif
	ht_uint64 m_data;
	uint64_t m_ctl:3;
};

#ifdef _HTV
#define CHostDataQue CHostDataQueIntf
#else
struct CHostDataQueIntf : CHostDataQue {
	CHostDataQueIntf() {}
	bool operator == (const CHostDataQue & rhs) const {
		return
			m_data == rhs.m_data &&
			m_ctl == rhs.m_ctl
			;
	}
	CHostDataQue & operator = (const CHostDataQue & rhs) {
		m_data = rhs.m_data;
		m_ctl = rhs.m_ctl;
		return *this;
	}
	CHostDataQue & operator = (int zero) {
		assert(zero == 0);
		m_data = 0;
		m_ctl = 0;
		return *this;
	}
# ifdef HT_SYSC
	friend void sc_trace(sc_trace_file *tf, const CHostDataQue & v, const std::string & NAME ) {
		sc_trace(tf, v.m_data, NAME + ".m_data");
		sc_trace(tf, v.m_ctl, NAME + ".m_ctl");
	}
	friend ostream& operator << ( ostream& os,  CHostDataQue const & v ) {
		os << "(" << "???" << ")";
		return os;
	}
};
# endif
#endif

//////////////////////////////////
// Module Queue interfaces

//////////////////////////////////
// Command interfaces

struct CClk2xToCtl_Clk2xRtnIntf {
	CtlInst_t m_rtnInst;
//};

//struct CClk2xToCtl_Clk2xRtnIntf : CClk2xToCtl_Clk2xRtn {
	CClk2xToCtl_Clk2xRtnIntf() {}
	bool operator == (const CClk2xToCtl_Clk2xRtnIntf & rhs) const {
		return
			m_rtnInst == rhs.m_rtnInst
			;
	}
	CClk2xToCtl_Clk2xRtnIntf & operator = (const CClk2xToCtl_Clk2xRtnIntf & rhs) {
		m_rtnInst = rhs.m_rtnInst;
		return *this;
	}
	CClk2xToCtl_Clk2xRtnIntf & operator = (int zero) {
		//assert(zero == 0);
		m_rtnInst = 0;
		return *this;
	}
# ifdef HT_SYSC
	friend void sc_trace(sc_trace_file *tf, const CClk2xToCtl_Clk2xRtn & v, const std::string & NAME ) {
		sc_trace(tf, v.m_rtnInst, NAME + ".m_rtnInst");
	}
	friend ostream& operator << ( ostream& os,  CClk2xToCtl_Clk2xRtn const & v ) {
		os << "(" << "???" << ")";
		return os;
	}
# endif
};

#ifdef _HTV
struct CCtlToClk2x_Clk2xCallIntf {
#else
struct CCtlToClk2x_Clk2xCall {
#endif
	CtlInst_t m_rtnInst;
};

#ifdef _HTV
#define CCtlToClk2x_Clk2xCall CCtlToClk2x_Clk2xCallIntf
#else
struct CCtlToClk2x_Clk2xCallIntf : CCtlToClk2x_Clk2xCall {
	CCtlToClk2x_Clk2xCallIntf() {}
	bool operator == (const CCtlToClk2x_Clk2xCall & rhs) const {
		return
			m_rtnInst == rhs.m_rtnInst
			;
	}
	CCtlToClk2x_Clk2xCall & operator = (const CCtlToClk2x_Clk2xCall & rhs) {
		m_rtnInst = rhs.m_rtnInst;
		return *this;
	}
	CCtlToClk2x_Clk2xCall & operator = (int zero) {
		assert(zero == 0);
		m_rtnInst = 0;
		return *this;
	}
# ifdef HT_SYSC
	friend void sc_trace(sc_trace_file *tf, const CCtlToClk2x_Clk2xCall & v, const std::string & NAME ) {
		sc_trace(tf, v.m_rtnInst, NAME + ".m_rtnInst");
	}
	friend ostream& operator << ( ostream& os,  CCtlToClk2x_Clk2xCall const & v ) {
		os << "(" << "???" << ")";
		return os;
	}
};
# endif
#endif

#ifdef _HTV
struct CClk1xToClk2x_Clk1xRtnIntf {
#else
struct CClk1xToClk2x_Clk1xRtn {
#endif
	Clk2xInst_t m_rtnInst;
};

#ifdef _HTV
#define CClk1xToClk2x_Clk1xRtn CClk1xToClk2x_Clk1xRtnIntf
#else
struct CClk1xToClk2x_Clk1xRtnIntf : CClk1xToClk2x_Clk1xRtn {
	CClk1xToClk2x_Clk1xRtnIntf() {}
	bool operator == (const CClk1xToClk2x_Clk1xRtn & rhs) const {
		return
			m_rtnInst == rhs.m_rtnInst
			;
	}
	CClk1xToClk2x_Clk1xRtn & operator = (const CClk1xToClk2x_Clk1xRtn & rhs) {
		m_rtnInst = rhs.m_rtnInst;
		return *this;
	}
	CClk1xToClk2x_Clk1xRtn & operator = (int zero) {
		assert(zero == 0);
		m_rtnInst = 0;
		return *this;
	}
# ifdef HT_SYSC
	friend void sc_trace(sc_trace_file *tf, const CClk1xToClk2x_Clk1xRtn & v, const std::string & NAME ) {
		sc_trace(tf, v.m_rtnInst, NAME + ".m_rtnInst");
	}
	friend ostream& operator << ( ostream& os,  CClk1xToClk2x_Clk1xRtn const & v ) {
		os << "(" << "???" << ")";
		return os;
	}
};
# endif
#endif

#ifdef _HTV
struct CClk2xToClk1x_Clk1xCallIntf {
#else
struct CClk2xToClk1x_Clk1xCall {
#endif
	Clk2xInst_t m_rtnInst;
};

#ifdef _HTV
#define CClk2xToClk1x_Clk1xCall CClk2xToClk1x_Clk1xCallIntf
#else
struct CClk2xToClk1x_Clk1xCallIntf : CClk2xToClk1x_Clk1xCall {
	CClk2xToClk1x_Clk1xCallIntf() {}
	bool operator == (const CClk2xToClk1x_Clk1xCall & rhs) const {
		return
			m_rtnInst == rhs.m_rtnInst
			;
	}
	CClk2xToClk1x_Clk1xCall & operator = (const CClk2xToClk1x_Clk1xCall & rhs) {
		m_rtnInst = rhs.m_rtnInst;
		return *this;
	}
	CClk2xToClk1x_Clk1xCall & operator = (int zero) {
		assert(zero == 0);
		m_rtnInst = 0;
		return *this;
	}
# ifdef HT_SYSC
	friend void sc_trace(sc_trace_file *tf, const CClk2xToClk1x_Clk1xCall & v, const std::string & NAME ) {
		sc_trace(tf, v.m_rtnInst, NAME + ".m_rtnInst");
	}
	friend ostream& operator << ( ostream& os,  CClk2xToClk1x_Clk1xCall const & v ) {
		os << "(" << "???" << ")";
		return os;
	}
};
# endif
#endif

//////////////////////////////////
// Ram interfaces

