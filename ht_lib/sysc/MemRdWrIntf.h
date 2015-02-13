/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// MemRdWrIntf.h
//
//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////
// Memory interfaces

#define MIF_TID_W 32
#define MIF_TID_QWCNT_W 3
#define MIF_TID_QWCNT_SHF (MIF_TID_W - MIF_TID_QWCNT_W)
#define MIF_TID_QWCNT_MSK ((1u<<MIF_TID_QWCNT_W)-1)

#define MEM_TYPE_W 2
#define MEM_ADDR_W 48
#define MEM_DATA_W 64

#define MEM_REQ_RD 0
#define MEM_REQ_WR 1
#define MEM_REQ_SET_BIT_63 3

#define MEM_REQ_U8 0
#define MEM_REQ_U16 1
#define MEM_REQ_U32 2
#define MEM_REQ_U64 3

struct CMemRdWrReqIntf {
	// WARNING - these fields are used to interface with the PDK
	ht_uint64 m_data;
	sc_uint<MEM_ADDR_W> m_addr;
	sc_uint<MIF_TID_W> m_tid;
	sc_uint<MEM_TYPE_W> m_type;
	ht_uint1 m_host;
	ht_uint2 m_size;

#ifndef _HTV
	uint32_t m_line;
	char const * m_file;
	uint64_t m_time;
	bool m_orderChk;

	CMemRdWrReqIntf() {}
	bool operator == (const CMemRdWrReqIntf & rhs) const {
		return m_host == rhs.m_host &&
			m_tid == rhs.m_tid &&
			m_type == rhs.m_type &&
			m_size == rhs.m_size &&
			m_addr == rhs.m_addr &&
			m_data == rhs.m_data;
	}
	CMemRdWrReqIntf & operator = (const CMemRdWrReqIntf & rhs) {
		m_host = rhs.m_host;
		m_tid = rhs.m_tid;
		m_type = rhs.m_type;
		m_addr = rhs.m_addr;
		m_data = rhs.m_data;
		m_size = rhs.m_size;

		m_line = rhs.m_line;
		m_file = rhs.m_file;
		m_time = rhs.m_time;
		m_orderChk = rhs.m_orderChk;

		return *this;
	}
	CMemRdWrReqIntf & operator = (int zero) {
		assert(zero == 0);
		m_host = 0;
		m_tid = 0;
		m_type = 0;
		m_addr = 0;
		m_data = 0;
		m_size = 0;
		return *this;
	}
	friend void sc_trace(sc_trace_file *tf, const CMemRdWrReqIntf & v, const std::string & NAME ) {
		sc_trace(tf, v.m_host, NAME + ".m_host");
		sc_trace(tf, v.m_tid, NAME + ".m_tid");
		sc_trace(tf, v.m_type, NAME + ".m_type");
		sc_trace(tf, v.m_addr, NAME + ".m_addr");
		sc_trace(tf, v.m_data, NAME + ".m_data");
		sc_trace(tf, v.m_size, NAME + ".m_size");
	}
	friend ostream& operator << ( ostream& os, CMemRdWrReqIntf const & v ) {
		os << "(" << "???" << ")";
		return os;
	}
#endif
};

struct CMemRdRspIntf {
	// WARNING - these fields are used to interface with the PDK
	ht_uint64 m_data;
	ht_uint32 m_tid;

#ifndef _HTV
	CMemRdRspIntf() {}
	bool operator == (const CMemRdRspIntf & rhs) const {
		return m_tid == rhs.m_tid &&
			m_data == rhs.m_data;
	}
	CMemRdRspIntf & operator = (const CMemRdRspIntf & rhs) {
		m_tid = rhs.m_tid;
		m_data = rhs.m_data;
		return *this;
	}
	friend void sc_trace(sc_trace_file *tf, const CMemRdRspIntf & v, const std::string & NAME ) {
		sc_trace(tf, v.m_tid, NAME + ".m_tid");
		sc_trace(tf, v.m_data, NAME + ".m_data");
	}
	friend ostream& operator << ( ostream& os, CMemRdRspIntf const & v ) {
		os << "(" << "???" << ")";
		return os;
	}
#endif
};
