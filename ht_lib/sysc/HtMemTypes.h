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

#include <stdint.h>
#include <assert.h>
#include <systemc.h>

#ifdef _HTV
#define HT_CYCLE() 0
#define assert(a)
#define ht_assert(a)
#define printf(...)
#define fprintf(...)
#else
#define SC_X 0
#define ht_topology_top
#define ht_prim
#define ht_noload
#define ht_state
#define ht_local
#define ht_clk(...)
#define ht_attrib(name,inst,value)
#endif

ht_prim ht_clk("clk") inline void HtResetFlop (bool &r_reset, const bool &i_reset) { r_reset = i_reset; }

#ifndef _HTV

namespace Ht {
	extern sc_core::sc_trace_file * g_vcdp;
}

inline void ht_assert(bool bCond)
{
	if (bCond) return;
	if (Ht::g_vcdp) sc_close_vcd_trace_file(Ht::g_vcdp);
	assert(0);
}

template<typename T>
inline T ht_bad_data(T const &)
{
	return (T)0xDBADBADBADBADBADLL;
}

template <typename T, int addrBits> class ht_dist_que {
public:
	ht_dist_que() {
		ht_assert(addrBits < 10);
		m_maxEntriesUsed = 0;
		m_bPop = false;
		m_bPush = false;
		m_bPushWasReset = false;
		m_bPopWasReset = false;
	}
	~ht_dist_que() {
#ifdef QUE_DEBUG
		if (m_pName)
			fprintf(stderr, "%s - maxEntries = %d of %d\n", m_pName, m_maxEntriesUsed, (1 << addrBits));
#endif
	}
	void SetName(char *pName) { m_pName = pName; }
	uint32_t size() {
		return (uint32_t)((m_cmpWrIdx.read() >= m_cmpRdIdx.read())
			? m_cmpWrIdx.read() - m_cmpRdIdx.read()
			: (m_cmpWrIdx.read() + (1 << (addrBits+1)) - m_cmpRdIdx.read())); }
	bool empty() { return m_cmpWrIdx.read() == m_cmpRdIdx.read(); }
	bool full(uint32_t num=0) { return size() >= (1 << addrBits) - num; }
	const T &front() { return m_que[m_rdIdx & ((1 << addrBits)-1)]; }
	T &read_entry_debug(int idx) { return m_que[(m_rdIdx + idx) & ((1 << addrBits)-1)]; }

	void pop() {
		if (m_bPopWasReset && m_bPushWasReset) {
			ht_assert(!empty());
			ht_assert(!m_bPop);
			m_bPop = true;
		}
	}
	void push(T data) {
		if (m_bPopWasReset && m_bPushWasReset) {
			ht_assert(!full());
			ht_assert(!m_bPush);
			m_bPush = true;
			m_pushData = data;
		}
	}
	bool push_in_use() { return m_bPush; }
	void clock(bool bReset) {
		push_clock(bReset);
		pop_clock(bReset);
	}
	void push_clock(bool bReset) {
		uint32_t wrIdx = (uint32_t)m_cmpWrIdx.read();

		if (bReset) {
			wrIdx = 0;
			m_bPushWasReset = true;
		} else if (m_bPush) {
			m_que[m_wrIdx & ((1 << addrBits)-1)] = m_pushData;
			wrIdx += 1;
			if (m_maxEntriesUsed < size()) m_maxEntriesUsed = size();
		}

		m_wrIdx = wrIdx;
		m_cmpWrIdx = wrIdx;
		m_bPush = false;
	}
	void pop_clock(bool bReset) {
		uint32_t rdIdx = (uint32_t)m_cmpRdIdx.read();

		if (bReset) {
			rdIdx = 0;
			m_bPopWasReset = true;
		} else if (m_bPop) {
			ht_assert(!empty());
			rdIdx += 1;
		}

		m_rdIdx = rdIdx;
		m_cmpRdIdx = rdIdx;
		m_bPop = false;
	}
	uint32_t read_addr() { return m_rdIdx & ((1 << addrBits)-1); }
	uint32_t write_addr() { return m_wrIdx & ((1 << addrBits)-1); }
	uint32_t capacity() { return (1 << addrBits); }

	friend void sc_trace(sc_trace_file *tf, const ht_dist_que & v, const std::string & NAME ) {
	}

private:
	char * m_pName;
	uint32_t m_maxEntriesUsed;
	T m_que[1 << addrBits];
	T m_pushData;
	T m_postClockFrontData;
	uint32_t m_rdIdx;
	uint32_t m_wrIdx;
	sc_signal<sc_uint<addrBits+1> > m_cmpWrIdx; // compares use an sc_signal to allow different rd/wr clocks
	sc_signal<sc_uint<addrBits+1> > m_cmpRdIdx;
	bool m_bPop;
	bool m_bPush;
	bool m_bPopWasReset;
	bool m_bPushWasReset;
};

template <typename T, int addrBits> class ht_block_que {
public:
	ht_block_que() {
		ht_assert(addrBits < 18);
		m_maxEntriesUsed = 0;
		m_bPop = false;
		m_bPush = false;
		m_bPushWasReset = false;
		m_bPopWasReset = false;
	}
	~ht_block_que() {
#ifdef QUE_DEBUG
		if (m_pName)
			fprintf(stderr, "%s - maxEntries = %d of %d\n", m_pName, m_maxEntriesUsed, (1 << addrBits));
#endif
	}
	void SetName(char *pName) { m_pName = pName; }
	uint32_t size() {
		return (uint32_t)((m_cmpWrIdx.read() >= m_cmpRdIdx.read())
			? m_cmpWrIdx.read() - m_cmpRdIdx.read()
			: (m_cmpWrIdx.read() + (1 << (addrBits+1)) - m_cmpRdIdx.read()));
	}
	bool empty() { return m_cmpWrIdx2.read() == m_cmpRdIdx.read(); }
	bool full(uint32_t num=0) { return size() >= (1 << addrBits) - num; }
	const T &front() { return m_que[m_rdIdx & ((1 << addrBits)-1)]; }
	T &EntryIgnore(int idx) { return m_que[idx]; }

	void pop() {
		if (m_bPopWasReset && m_bPushWasReset) {
			ht_assert(!empty());
			ht_assert(!m_bPop);
			m_bPop = true;
		}
	}
	void push(T data) {
		if (m_bPopWasReset && m_bPushWasReset) {
			ht_assert(!full());
			ht_assert(!m_bPush);
			m_bPush = true;
			m_pushData = data;
		}
	}
	bool push_in_use() { return m_bPush; }
	void clock(bool bReset) {
		push_clock(bReset);
		pop_clock(bReset);
	}
	void push_clock(bool bReset) {
		uint32_t wrIdx = (uint32_t)m_cmpWrIdx.read();

		if (bReset) {
			wrIdx = 0;
			m_cmpWrIdx2 = 0;
			m_bPushWasReset = true;
		} else if (m_bPush) {
			m_que[m_wrIdx & ((1 << addrBits)-1)] = m_pushData;
			wrIdx += 1;
			if (m_maxEntriesUsed < size()) m_maxEntriesUsed = size();
		}

		m_cmpWrIdx2 = m_cmpWrIdx;
		m_wrIdx = wrIdx;
		m_cmpWrIdx = wrIdx;
		m_bPush = false;
	}
	void pop_clock(bool bReset) {
		uint32_t rdIdx = (uint32_t)m_cmpRdIdx.read();

		if (bReset) {
			rdIdx = 0;
			m_bPopWasReset = true;
		} else if (m_bPop) {
			ht_assert(!empty());
			rdIdx += 1;
		}

		m_rdIdx = rdIdx;
		m_cmpRdIdx = rdIdx;
		m_bPop = false;
	}
	uint32_t read_addr() { return m_rdIdx & ((1 << addrBits)-1); }
	uint32_t write_addr() { return m_wrIdx & ((1 << addrBits)-1); }
	uint32_t capacity() { return (1 << addrBits); }

	friend void sc_trace(sc_trace_file *tf, const ht_block_que & v, const std::string & NAME ) {
	}

private:
	char * m_pName;
	uint32_t m_maxEntriesUsed;
	T m_que[1 << addrBits];
	T m_pushData;
	T m_postClockFrontData;
	uint32_t m_rdIdx;
	uint32_t m_wrIdx;
	sc_signal<sc_uint<addrBits+1> > m_cmpWrIdx; // compares use an sc_signal to allow different rd/wr clocks
	sc_signal<sc_uint<addrBits+1> > m_cmpWrIdx2;
	sc_signal<sc_uint<addrBits+1> > m_cmpRdIdx;
	bool m_bPop;
	bool m_bPush;
	bool m_bPopWasReset;
	bool m_bPushWasReset;
};

template <typename T, int AW1, int AW2=0>
class ht_dist_ram {
public:
	ht_dist_ram() {
		m_bReset = true;
		m_bWrAddr = false;
		m_bRdAddr = false;
		m_bWrData = false;
	}
	const T & read_mem_debug(uint64_t rdIdx1, uint64_t rdIdx2=0) {
		return m_mem[(rdIdx1 + (rdIdx2 << AW1)) & ((1<<AW1) * (1<<AW2) - 1)];
	}
	void read_addr_ignore(uint64_t rdIdx1, uint64_t rdIdx2=0) {
		m_rdAddr = (rdIdx1 + (rdIdx2 << AW1)) & ((1<<AW1) * (1<<AW2) - 1);
	}
	const T & read_mem_ignore() { return m_mem[m_rdAddr]; }
	void read_addr(uint64_t rdIdx1, uint64_t rdIdx2 = 0) {
		uint64_t rdAddr = (rdIdx1 + (rdIdx2 << AW1)) & ((1 << AW1) * (1 << AW2) - 1);
		ht_assert(m_bReset || !m_bRdAddr || m_rdAddr == rdAddr);
		m_bRdAddr = true;
		m_rdAddr = rdAddr;
	}
	void read_addr(uint64_t rdIdx1, uint64_t rdIdx2 = 0) const {
		uint64_t rdAddr = (rdIdx1 + (rdIdx2 << AW1)) & ((1 << AW1) * (1 << AW2) - 1);
		ht_assert(m_bReset || !m_bRdAddr || m_rdAddr == rdAddr);
		const_cast<ht_dist_ram*>(this)->m_bRdAddr = true;
		const_cast<ht_dist_ram*>(this)->m_rdAddr = rdAddr;
	}
	bool read_in_use() { return m_bRdAddr; }
	void write_addr(uint64_t wrIdx1, uint64_t wrIdx2=0) {
		uint64_t wrAddr = (wrIdx1 + (wrIdx2 << AW1)) & ((1 << AW1) * (1 << AW2) - 1);
		ht_assert(m_bReset || !m_bWrAddr || m_wrAddr == wrAddr);
		m_wrAddr = wrAddr;
		if (!m_bWrAddr)
			m_wrData = m_mem[m_bReset ? 0 : m_wrAddr];
		m_bWrAddr = true;
	}
	uint64_t write_addr_debug() { return m_wrAddr; }
	bool write_in_use() { return m_bWrAddr; }
	T & write_mem_debug(uint64_t wrIdx1, uint64_t wrIdx2=0) {
		uint64_t wrAddr = (wrIdx1 + (wrIdx2 << AW1)) & ((1<<AW1) * (1<<AW2) - 1);
		return m_mem[wrAddr];
	}
	const T & read_mem() const {
		if (m_bReset) return m_mem[0];
		ht_assert(m_bRdAddr);
		return m_mem[m_rdAddr];
	}
	T & write_mem() {
		m_bWrData = true;
		return m_wrData; 
	}
	void write_mem(const T &wrData) {
		m_bWrData = true;
		m_wrData = wrData;
	}
	void read_clock(bool bReset = false) {
		m_bReset = bReset; 
		m_bRdAddr = false; 
	}
	void write_clock(bool bReset = false) {
		m_bReset = bReset;
		if (!m_bReset && m_bWrData) {
			ht_assert(m_bWrAddr);
			m_mem[m_wrAddr] = m_wrData;
		}
		m_bWrData = false;
		m_bWrAddr = false;
	}
	void clock(bool bReset=false) {
		read_clock(bReset);
		write_clock(bReset);
	}

	friend void sc_trace(sc_trace_file *tf, const ht_dist_ram & v, const std::string & NAME ) {
	}

private:
	bool m_bReset;
	T m_mem[(1<<AW1)*(1<<AW2)];
	bool m_bRdAddr;
	uint64_t m_rdAddr;
	bool m_bWrAddr;
	bool m_bWrData;
	T m_wrData;
	uint64_t m_wrAddr;
};

template <typename T, int AW1, int AW2=0, bool bDoReg=false>
class ht_block_ram {
public:
	ht_block_ram() {
		m_bReset = true;
		m_bWrAddr = false;
		m_bWrData = false;
		r_bWrData = false;
		r_wrAddr = 0;
		m_bRdAddr = false;
		r_bRdAddr = true;
		r_rdAddr = 0;
	}
	void RdAddrIgnore(uint64_t rdIdx1, uint64_t rdIdx2) {
		m_bRdAddr = true;
		m_rdAddr = (rdIdx1 + (rdIdx2 << AW1)) & ((1<<AW1) * (1<<AW2) - 1);
	}
	void read_addr(uint64_t rdIdx1, uint64_t rdIdx2 = 0) {
		ht_assert(m_bReset || !m_bRdAddr);
		m_bRdAddr = true;
		m_rdAddr = (rdIdx1 + (rdIdx2 << AW1)) & ((1 << AW1) * (1 << AW2) - 1);
	}
	void read_addr(uint64_t rdIdx1, uint64_t rdIdx2 = 0) const {
		ht_assert(m_bReset || !m_bRdAddr);
		const_cast<ht_block_ram*>(this)->m_bRdAddr = true;
		const_cast<ht_block_ram*>(this)->m_rdAddr = (rdIdx1 + (rdIdx2 << AW1)) & ((1 << AW1) * (1 << AW2) - 1);
	}
	uint64_t read_addr() { return m_rdAddr; }
	bool read_in_use() { return m_bRdAddr; }
	void write_addr(uint64_t wrIdx1, uint64_t wrIdx2=0) {
		uint64_t wrAddr = (wrIdx1 + (wrIdx2 << AW1)) & ((1 << AW1) * (1 << AW2) - 1);
		ht_assert(m_bReset || !m_bWrAddr || m_wrAddr == wrAddr);
		m_wrAddr = wrAddr;
		if (!m_bWrAddr)
			m_wrData = m_mem[m_bReset ? 0 : m_wrAddr];
		m_bWrAddr = true;
	}
	bool write_in_use() { return m_bWrAddr; }
	const T & read_mem_debug(uint64_t rdIdx1, uint64_t rdIdx2=0) {
		return m_mem[(rdIdx1 + (rdIdx2 << AW1)) & ((1<<AW1) * (1<<AW2) - 1)];
	}
	T read_mem() const {
		if (m_bReset) return m_mem[0];
		ht_assert(bDoReg ? r_bRdAddr2 : r_bRdAddr);
		return bDoReg ? r_doReg : ((r_bRdAddr && r_bWrData && r_rdAddr == r_wrAddr) ? ht_bad_data(r_doReg) : m_mem[r_rdAddr]);
	}
	T & write_mem() { ht_assert(m_bWrAddr); return m_wrData; }
	void write_mem(const T &wrData) {
		ht_assert(m_bReset || m_bWrAddr);
		m_bWrData = true;
		m_wrData = wrData;
	}
	T & write_mem_debug(uint64_t wrIdx1, uint64_t wrIdx2=0) {
		uint64_t wrAddr = (wrIdx1 + (wrIdx2 << AW1)) & ((1<<AW1) * (1<<AW2) - 1);
		return m_mem[wrAddr];
	}
	void read_clock(bool bReset = false) {
		m_bReset = bReset;
		if (!m_bReset && r_bRdAddr)
			r_doReg = (r_bRdAddr && r_bWrData && r_rdAddr == r_wrAddr) ? ht_bad_data(r_doReg) : m_mem[r_rdAddr];

		r_bRdAddr2 = r_bRdAddr;
		r_bRdAddr = m_bRdAddr;
		m_bRdAddr = false;

		r_rdAddr = m_rdAddr;
	}
	void write_clock(bool bReset = false) {
		m_bReset = bReset;
		if (!m_bReset && m_bWrData)
			m_mem[m_wrAddr] = m_wrData;

		r_bWrData = m_bWrData;
		r_wrAddr = m_wrAddr;
		m_bWrAddr = false;
		m_bWrData = false;
	}
	void clock(bool bReset = false) {
		read_clock(bReset);
		write_clock(bReset);
	}

	friend void sc_trace(sc_trace_file *tf, const ht_block_ram & v, const std::string & NAME ) {
	}

private:
	bool m_bReset;
	T m_mem[(1<<AW1)*(1<<AW2)];
	T r_doReg;
	bool m_bRdAddr;
	uint64_t m_rdAddr;
	sc_signal<bool> r_bRdAddr;
	bool r_bRdAddr2;
	sc_signal<uint64_t> r_rdAddr;
	bool m_bWrData;
	sc_signal<bool> r_bWrData;
	bool m_bWrAddr;
	T m_wrData;
	uint64_t m_wrAddr;
	sc_signal<uint64_t> r_wrAddr;
};

////////////////////////////////
// T - type
// SW - select width
// AW1 - address 1 width
// AW2 - address 2 width
// bDoReg - data out register enable

// specialization for WC=1
template <typename T, int SW, int AW1, int AW2=0, bool bDoReg=false>
class ht_mrd_block_ram {
public:
	ht_mrd_block_ram() {
		m_bReset = true;
		m_bWrAddr = false;
		m_bWrData = false;
		r_bWrData = false;
		r_wrAddr = 0;
		m_bRdAddr = false;
		r_bRdAddr = true;
		r_rdAddr = 0;
	}
	void RdAddrIgnore(uint64_t rdIdx1, uint64_t rdIdx2) {
		m_bRdAddr = true;
		m_rdAddr = (rdIdx1 + (rdIdx2 << AW1)) & ((1<<AW1) * (1<<AW2) - 1);
	}
	void read_addr(uint64_t rdIdx1, uint64_t rdIdx2=0) {
		ht_assert(m_bReset || !m_bRdAddr);
		m_bRdAddr = true;
		m_rdAddr = (rdIdx1 + (rdIdx2 << AW1)) & ((1<<AW1) * (1<<AW2) - 1);
	}
	uint64_t read_addr() { return m_rdAddr; }
	void write_addr(uint64_t wrSel, uint64_t wrIdx1, uint64_t wrIdx2=0) {
		uint64_t wrAddr = (wrIdx1 + (wrIdx2 << AW1)) & ((1 << AW1) * (1 << AW2) - 1);
		ht_assert(wrSel < (1 << SW));
		ht_assert(m_bReset || !m_bWrAddr || m_wrAddr == wrAddr);
		m_wrAddr = wrAddr;
		m_wrSel = wrSel;
		if (!m_bWrAddr)
			m_wrData = m_bReset ? m_mem[0][0] : m_mem[m_wrAddr][m_wrSel];
		m_bWrAddr = true;
	}
	const T & read_mem_debug(uint64_t rdSel, uint64_t rdIdx1, uint64_t rdIdx2=0) {
		ht_assert(rdSel < (1<<SW));
		return m_mem[(rdIdx1 + (rdIdx2 << AW1)) & ((1<<AW1) * (1<<AW2) - 1)][rdSel];
	}
	T read_mem(uint64_t rdSel) {
		if (m_bReset) return m_mem[0][0];
		ht_assert(rdSel < (1 << SW));
		ht_assert(bDoReg ? r_bRdAddr2 : r_bRdAddr);
		return bDoReg ? r_doReg[rdSel] : ((r_bRdAddr && r_bWrData && r_rdAddr == r_wrAddr) ? ht_bad_data(r_doReg[0]) : m_mem[r_rdAddr][rdSel]);
	}
	T & write_mem() { ht_assert(m_bWrAddr); return m_wrData; }
	void write_mem(const T &wrData) {
		ht_assert(m_bReset || m_bWrAddr);
		m_bWrData = true;
		m_wrData = wrData;
	}
	T & write_mem_debug(uint64_t wrSel, uint64_t wrIdx1, uint64_t wrIdx2=0) {
		ht_assert(wrSel < (1<<SW));
		uint64_t wrAddr = (wrIdx1 + (wrIdx2 << AW1)) & ((1<<AW1) * (1<<AW2) - 1);
		return m_mem[wrAddr][wrSel];
	}
	void read_clock(bool bReset = false) {
		m_bReset = bReset;
		if (!m_bReset && r_bRdAddr) {
			for (uint64_t rdSel = 0; rdSel < (1<<SW); rdSel += 1)
				r_doReg[rdSel] = (r_bRdAddr && r_bWrData && r_rdAddr == r_wrAddr) ? ht_bad_data(r_doReg[0]) : m_mem[r_rdAddr][rdSel];
		}

		r_bRdAddr2 = r_bRdAddr;
		r_bRdAddr = m_bRdAddr;
		m_bRdAddr = false;

		r_rdAddr = m_rdAddr;
	}
	void write_clock(bool bReset = false) {
		m_bReset = bReset;
		if (!m_bReset && m_bWrData)
			m_mem[m_wrAddr][m_wrSel] = m_wrData;

		r_bWrData = m_bWrData;
		r_wrAddr = m_wrAddr;
		m_bWrAddr = false;
		m_bWrData = false;
	}
	void clock(bool bReset = false) {
		read_clock(bReset);
		write_clock(bReset);
	}

	friend void sc_trace(sc_trace_file *tf, const ht_mrd_block_ram & v, const std::string & NAME ) {
	}

private:
	bool m_bReset;
	T m_mem[(1 << AW1)*(1 << AW2)][1 << SW];
	T r_doReg[1<<SW];
	bool m_bRdAddr;
	uint64_t m_rdAddr;
	bool r_bRdAddr;
	bool r_bRdAddr2;
	uint64_t r_rdAddr;
	bool m_bWrData;
	bool r_bWrData;
	bool m_bWrAddr;
	T m_wrData;
	uint64_t m_wrAddr;
	uint64_t m_wrSel;
	uint64_t r_wrAddr;
};

template <typename T, int SW, int AW1, int AW2=0, bool bDoReg=false>
class ht_mwr_block_ram {
public:
	ht_mwr_block_ram() {
		m_bReset = true;
		m_bWrAddr = false;
		m_bWrData = false;
		r_bWrData = false;
		r_wrAddr = 0;
		m_bRdAddr = false;
		r_bRdAddr = true;
		r_rdSel = 0;
		r_rdAddr = 0;
	}
	void RdAddrIgnore(uint64_t rdSel, uint64_t rdIdx1, uint64_t rdIdx2=0) {
		ht_assert(rdSel < (1<<SW));
		m_bRdAddr = true;
		m_rdSel = rdSel;
		m_rdAddr = (rdIdx1 + (rdIdx2 << AW1)) & ((1<<AW1) * (1<<AW2) - 1);
	}
	void read_addr(uint64_t rdSel, uint64_t rdIdx1, uint64_t rdIdx2=0) {
		ht_assert(m_bReset || rdSel < (1 << SW));
		ht_assert(m_bReset || !m_bRdAddr);
		m_bRdAddr = true;
		m_rdSel = rdSel;
		m_rdAddr = (rdIdx1 + (rdIdx2 << AW1)) & ((1<<AW1) * (1<<AW2) - 1);
	}
	uint64_t read_addr() { return m_rdAddr; }
	void write_addr(uint64_t wrIdx1, uint64_t wrIdx2=0) {
		uint64_t wrAddr = (wrIdx1 + (wrIdx2 << AW1)) & ((1 << AW1) * (1 << AW2) - 1);
		ht_assert(m_bReset || !m_bWrAddr || m_wrAddr == wrAddr);
		m_wrAddr = wrAddr;
		for (uint64_t wrSel = 0; wrSel < (1<<SW); wrSel += 1)
			m_wrData[wrSel] = 0;
		m_bWrAddr = true;
	}
	const T & read_mem_debug(uint64_t rdSel, uint64_t rdIdx1, uint64_t rdIdx2=0) {
		ht_assert(rdSel < (1<<SW));
		return m_mem[(rdIdx1 + (rdIdx2 << AW1)) & ((1<<AW1) * (1<<AW2) - 1)][rdSel];
	}
	T read_mem() {
		if (m_bReset) return m_mem[0][0];
		ht_assert(bDoReg ? r_bRdAddr2 : r_bRdAddr);
		return bDoReg ? r_doReg : ((r_bRdAddr && r_bWrData && r_rdAddr == r_wrAddr) ? ht_bad_data(r_doReg) : m_mem[r_rdAddr][r_rdSel]);
	}
	T & write_mem() { ht_assert(m_bWrAddr); return m_wrData; }
	void write_mem(uint64_t wrSel, const T &wrData) {
		ht_assert(m_bReset || wrSel < (1 << SW));
		ht_assert(m_bReset || m_bWrAddr);
		m_bWrData = true;
		m_wrData[m_bReset ? 0 : wrSel] = wrData;
	}
	T & write_mem_debug(uint64_t wrSel, uint64_t wrIdx1, uint64_t wrIdx2=0) {
		ht_assert(wrSel < (1<<SW));
		uint64_t wrAddr = (wrIdx1 + (wrIdx2 << AW1)) & ((1<<AW1) * (1<<AW2) - 1);
		return m_mem[wrAddr][wrSel];
	}
	void read_clock(bool bReset = false) {
		m_bReset = bReset;
		if (!m_bReset && r_bRdAddr)
			r_doReg = (r_bRdAddr && r_bWrData && r_rdAddr == r_wrAddr) ? ht_bad_data(r_doReg) : m_mem[r_rdAddr][r_rdSel];

		r_bRdAddr2 = r_bRdAddr;
		r_bRdAddr = m_bRdAddr;
		m_bRdAddr = false;

		r_rdSel = m_rdSel;
		r_rdAddr = m_rdAddr;
	}
	void write_clock(bool bReset = false) {
		m_bReset = bReset;
		if (!m_bReset && m_bWrData) {
			for (int wrSel = 0; wrSel < (1<<SW); wrSel += 1)
				m_mem[m_wrAddr][wrSel] = m_wrData[wrSel];
		}

		r_bWrData = m_bWrData;
		r_wrAddr = m_wrAddr;
		m_bWrAddr = false;
		m_bWrData = false;
	}
	void clock(bool bReset = false) {
		read_clock(bReset);
		write_clock(bReset);
	}

	friend void sc_trace(sc_trace_file *tf, const ht_mwr_block_ram & v, const std::string & NAME ) {
	}

private:
	bool m_bReset;
	T m_mem[(1 << AW1)*(1 << AW2)][1 << SW];
	T r_doReg;
	bool m_bRdAddr;
	uint64_t m_rdSel;
	uint64_t m_rdAddr;
	bool r_bRdAddr;
	bool r_bRdAddr2;
	uint64_t r_rdSel;
	uint64_t r_rdAddr;
	bool m_bWrData;
	bool r_bWrData;
	bool m_bWrAddr;
	T m_wrData[1<<SW];
	uint64_t m_wrSel;
	uint64_t m_wrAddr;
	uint64_t r_wrAddr;
};

#endif
