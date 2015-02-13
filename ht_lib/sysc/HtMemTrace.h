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

#ifdef _WIN32
#define fseek(a,b,c) _fseeki64(a,b,c)
#define ftell(a) _ftelli64(a)
#endif

#define MAX_FILE_LINE_SIZE 256
#define FILE_LINE_HASH_SIZE 256
#define FILE_LINE_MASK 0xff

#define MEM_TRACE_MAGIC 0x13579BDF
#define MEM_TRACE_VERSION 2

namespace Ht {

	class CMemTrace {
	public:
		struct CMemTraceHdr {
			uint32_t m_magic;
			uint32_t m_version;
			uint64_t m_memReqCount;
			int64_t m_fileLineOffset;
			uint64_t m_fileLineCount;
		};

		struct CMemTraceFmt {
			uint64_t m_ae:2;
			uint64_t m_unit:4;
			uint64_t m_src:8;
			uint64_t m_host:1;
			uint64_t m_mc:3;
			uint64_t m_bank:7;
			uint64_t m_op:1;
			uint64_t m_startSize:2;
			uint64_t m_startSign:1;
			uint64_t m_compSize:2;
			uint64_t m_addrSize:2;
			uint64_t m_addrSign:1;
		};

		struct CFileLine {
			char const * m_file;
			int m_line;
			CFileLine * m_pNext;
		};

		enum OpenMode { Read, Write };

	public:

		struct CAddrRange {
			CAddrRange(uint64_t b, uint64_t s) : m_base(b), m_size(s) {}
			uint64_t m_base;
			uint64_t m_size;
		};

		struct AddrRangeCmp {
			bool operator() (CAddrRange const & a, CAddrRange const & b) const {
				return a.m_base + a.m_size <= b.m_base;
			}
		};
		typedef map<CAddrRange, bool, AddrRangeCmp> AddrRangeMap_t;

		class CAddrRangeMap {
		public:
			void Insert(uint64_t base, uint64_t size, bool bCoproc) {
				uint64_t b = base;
				uint64_t e = base + size;

				AddrRangeMap_t::iterator i;
				for ( i=m_addrRangeMap.begin(); i != m_addrRangeMap.end(); ) {

					if (e < (*i).first.m_base)
						break;

					if ((*i).first.m_base + (*i).first.m_size < b) {
						i++;
						continue;
					}

					// overlap
					b = min(b, (*i).first.m_base);
					e = max(e, (*i).first.m_base + (*i).first.m_size);

					AddrRangeMap_t::iterator ii = i; ii++;

					m_addrRangeMap.erase(i);
					i = ii;
				}

				CAddrRange ar(b, e-b);
				m_addrRangeMap.insert(std::pair<CAddrRange, bool>(ar, bCoproc));
			}
			bool Find(uint64_t addr, bool &bCoproc) {
				CAddrRange ar(addr, 0);
				AddrRangeMap_t::iterator iter = m_addrRangeMap.lower_bound(ar);
				bool bInMap = iter != m_addrRangeMap.end() &&
					iter->first.m_base <= addr && addr < (iter->first.m_base + iter->first.m_size);
				if (bInMap)
					bCoproc = iter->second;
				return bInMap;
			}
		private:
			AddrRangeMap_t m_addrRangeMap;
		};

	public:
		CMemTrace() {
			m_lock = 0;
			m_bSystemcAddressValidation = false;
		}

		~CMemTrace() {
			if (m_fp)
				Close();
		}

		void Open(OpenMode mode, const char *pName) {
			m_mode = mode;
			m_fp = fopen(pName, mode == Write ? "wb" : "rb");
			if (m_fp == 0)
				printf("Could not open memory trace file: %s\n", pName), exit(1);

			m_prevStartTime = 0;
			m_prevAddr = 0;
			m_nxtAvlFileLine = 0;
			memset(m_pFileLineHash, 0, sizeof(CFileLine *) * FILE_LINE_HASH_SIZE);

			if (mode == Write) {
				m_memTraceHdr.m_magic = MEM_TRACE_MAGIC;
				m_memTraceHdr.m_version = MEM_TRACE_VERSION;
				m_memTraceHdr.m_memReqCount = 0;

				fwrite(&m_memTraceHdr, sizeof(CMemTraceHdr), 1, m_fp);
			} else {
				fread(&m_memTraceHdr, sizeof(CMemTraceHdr), 1, m_fp);
				if (m_memTraceHdr.m_magic != MEM_TRACE_MAGIC)
					printf("Unknown file type\n"), exit(1);
				if (m_memTraceHdr.m_version != MEM_TRACE_VERSION)
					printf("Unsupported memory trace file version\n");

				fseek(m_fp, m_memTraceHdr.m_fileLineOffset, SEEK_SET);

				uint32_t len;

				for (unsigned i = 0; i < m_memTraceHdr.m_fileLineCount; i += 1) {
					fread(&m_fileLineSpace[i].m_line, sizeof(uint32_t), 1, m_fp);
					fread(&len, sizeof(uint32_t), 1, m_fp);
					m_fileLineSpace[i].m_file = new char [len];
					fread((void *)m_fileLineSpace[i].m_file, len, 1, m_fp);
				}

				fseek(m_fp, sizeof(CMemTraceHdr), SEEK_SET);
				m_memReqIdx = 0;
			}
		}

		void Close() {
			if (m_mode == Write) {

				m_memTraceHdr.m_fileLineCount = m_nxtAvlFileLine;
				m_memTraceHdr.m_fileLineOffset = ftell(m_fp);

				for (int i = 0; i < m_nxtAvlFileLine; i += 1) {
					fwrite(&m_fileLineSpace[i].m_line, sizeof(uint32_t), 1, m_fp);
					uint32_t len = (uint32_t)strnlen(m_fileLineSpace[i].m_file, 256) + 1;
					fwrite(&len, sizeof(uint32_t), 1, m_fp);
					fwrite(m_fileLineSpace[i].m_file, len, 1, m_fp);
				}

				fseek(m_fp, 0, SEEK_SET);
				fwrite(&m_memTraceHdr, sizeof(CMemTraceHdr), 1, m_fp);
			}

			fclose(m_fp);
			m_fp = 0;
		}

		void WriteReq(int ae, int unit, int op, int host, char const * file, int line,
			uint64_t startTime, uint64_t compTime, uint64_t addr)
		{
			CMemTraceFmt fmt;
			fmt.m_op = op;
			fmt.m_ae = ae;
			fmt.m_unit = unit;
			fmt.m_src = LookupSrc(file, line);
			fmt.m_host = host;
			fmt.m_mc = (addr >> 6) & 7;
			fmt.m_bank = ((addr >> 6) & 0x78) | ((addr >> 3) & 0x7);

			int64_t startDelta = startTime - m_prevStartTime;
			startDelta /= 1000;
			m_prevStartTime += startDelta * 1000;
			if (startDelta < 0) {
				startDelta = -startDelta;
				fmt.m_startSign = 1;
			} else
				fmt.m_startSign = 0;
			fmt.m_startSize = CprsInt(startDelta);

			//uint64_t bankAvlTime = compTime * 66 / 100;
			uint64_t compDelta = compTime - startTime;
			compDelta /= 1000;
			fmt.m_compSize = CprsInt(compDelta);

			int64_t addrDelta = (addr >> 13) - m_prevAddr;
			m_prevAddr += addrDelta;
			if (addrDelta < 0) {
				addrDelta = -addrDelta;
				fmt.m_addrSign = 1;
			} else
				fmt.m_addrSign = 0;
			fmt.m_addrSize = CprsInt(addrDelta);

			fwrite(&fmt, 5, 1, m_fp);
			fwrite(&startDelta, 1 << fmt.m_startSize, 1, m_fp);
			fwrite(&compDelta, 1 << fmt.m_compSize, 1, m_fp);
			fwrite(&addrDelta, 1 << fmt.m_addrSize, 1, m_fp);

			m_memTraceHdr.m_memReqCount += 1;
		}

		int LookupSrc(char const *file, int line) {
			// lookup file/line in a hash table
			CFileLine * pFileLine = m_pFileLineHash[line & FILE_LINE_MASK];
			while (pFileLine) {
				if (pFileLine->m_file == file && pFileLine->m_line == line)
					return (int)(pFileLine - m_fileLineSpace);
				pFileLine = pFileLine->m_pNext;
			}
			// not found, add new entry
			if (m_nxtAvlFileLine == MAX_FILE_LINE_SIZE)
				return MAX_FILE_LINE_SIZE-1;

			CFileLine * pNewEntry = &m_fileLineSpace[m_nxtAvlFileLine++];
			pNewEntry->m_pNext = m_pFileLineHash[line & FILE_LINE_MASK];
			m_pFileLineHash[line & FILE_LINE_MASK] = pNewEntry;
			pNewEntry->m_file = file;
			pNewEntry->m_line = line;

			return (int)(pNewEntry - m_fileLineSpace);
		}

		bool ReadReq(int &ae, int &unit, int &mc, int &bank, int &op, int &src, int &host,
			uint64_t & startTime, uint64_t &compTime, uint64_t &addr) {

				CMemTraceFmt fmt;

				if (m_memReqIdx++ == m_memTraceHdr.m_memReqCount || fread(&fmt, 5, 1, m_fp) != 1)
					return false;

				int64_t startDelta = 0;
				fread(&startDelta, 1 << fmt.m_startSize, 1, m_fp);

				uint64_t compDelta = 0;
				fread(&compDelta, 1 << fmt.m_compSize, 1, m_fp);

				int64_t addrDelta = 0;
				fread(&addrDelta, 1 << fmt.m_addrSize, 1, m_fp);

				if (fmt.m_startSign)
					startDelta = -startDelta;

				if (fmt.m_addrSign)
					addrDelta = -addrDelta;

				startTime = m_prevStartTime += startDelta;
				compTime = m_prevStartTime + compDelta;
				addr = (m_prevAddr += addrDelta) << 13;

				addr |= ((fmt.m_bank << 6) & 0x1fc0) | (fmt.m_mc << 6) | ((fmt.m_bank << 3) & 0x38);

				ae = fmt.m_ae;
				unit = fmt.m_unit;
				mc = fmt.m_mc;
				bank = fmt.m_bank;
				op = fmt.m_op;
				src = fmt.m_src;
				host = fmt.m_host;

				return true;
		}

		bool LookupFileLine(int src, char const * &file, int &line) {
			if ((unsigned)src >= m_memTraceHdr.m_fileLineCount)
				return false;

			file = m_fileLineSpace[src].m_file;
			line = m_fileLineSpace[src].m_line;

			return true;
		}

		int CprsInt(uint64_t in) {
			if (in < (1 << 8))
				return 0;
			if (in < (1 << 16))
				return 1;
			if (in < (1LL << 32))
				return 2;

			return 3;
		}

		uint64_t GetMemReqCount() {
			return(m_memTraceHdr.m_memReqCount);
		}

		void SetAddressRange(void * pBase, uint64_t size, bool bCoproc, bool bSystemcAddressValidation) {
			// memory allocated for coprocessor use is generally done with a few large blocks
			// The address range lookup structure is organized as a binary tree. Each node
			// represents a valid address range.

			// align range to 64B boundaries
			int low = (uint64_t)pBase & 0x3f;
			pBase = (uint8_t *)pBase - low;
			size += low;

			size = (size + 0x3f) & ~0x3fULL;

			m_bSystemcAddressValidation |= bSystemcAddressValidation;

			Lock();
			m_addrRangeMap.Insert((uint64_t) pBase, size, bCoproc);
			Unlock();
		}

		bool IsValidAddress(uint64_t addr, bool &bCoproc) {
			return m_addrRangeMap.Find(addr, bCoproc);
		}

		bool IsAddressValidationEnabled() { return m_bSystemcAddressValidation; }

	private:
		void Lock() {
			while (__sync_fetch_and_or(&m_lock, 1) == 1)
				usleep(1);
		}

		void Unlock() {
			m_lock = 0;
		}

	private:
		OpenMode m_mode;
		FILE * m_fp;
		CMemTraceHdr m_memTraceHdr;
		uint64_t m_memReqIdx;

		uint64_t m_prevStartTime;
		uint64_t m_prevAddr;

		int m_nxtAvlFileLine;
		CFileLine m_fileLineSpace[MAX_FILE_LINE_SIZE];
		CFileLine * m_pFileLineHash[FILE_LINE_HASH_SIZE];

		bool m_bSystemcAddressValidation;
		CAddrRangeMap m_addrRangeMap;

		volatile int64_t m_lock;
	};

} // namespace Ht

#ifdef MEM_TRACE_TEST
// Example code for reading a memTrace file

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "MemTrace.h"

int main(int argc, char **argv) {
	if (argc < 2)
		printf("Command line format error, expected MemTrace.txt file\n"), exit(1);

	CMemTrace memTrace;

	memTrace.Open(CMemTrace::Read, argv[1]);

	char *file;
	int ae, unit, mc, bank, op, src, line, host;
	uint64_t startTime, compTime, addr;

	while (memTrace.ReadReq(ae, unit, mc, bank, op, src, host, startTime, compTime, addr)) {

		if (!memTrace.LookupFileLine(src, file, line))
			assert(0);

		printf("ae=%d, uint=%d, mc=%d, bank=%d, op=%d, host=%d, startTime=%lld, compTime=%lld, addr=%016llx\n",
			ae, unit, mc, bank, op, host, startTime, compTime, addr);
		printf(" file=%s, line=%d\n", file, line);
	}

	memTrace.Close();

	return 0;
}

#endif
