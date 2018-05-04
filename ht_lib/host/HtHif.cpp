/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Ht.h"

#if !defined(HT_MODEL) && !defined(HT_SYSC)
#include "HtPlatformIntf.h"
#endif

#if defined(HT_SYSC)
#include "SyscTop.h"

extern sc_trace_file *Ht::g_vcdp;
extern const char *Ht::g_vcdn;
#endif

namespace Ht {

	extern int g_htDebug;

#	if defined (HT_SYSC)
	extern int32_t g_coprocAeRunningCnt;
	extern bool g_bCsrFuncSet;
	extern void *g_pCsrMod;
	extern bool (*g_pCsrCmd)(void *self, int cmd, uint64_t addr, uint64_t data);
	extern bool (*g_pCsrRdRsp)(void *self, uint64_t &data);
#	endif

	#if !defined(HT_MODEL)
	void HtCoprocModel(Ht::CHtHifLibBase *) {}
	#endif

	CSyscTop * CHtHifBase::NewSyscTop() {
#		if defined(HT_SYSC)
			return new CSyscTop("SyscTop");
#		else
			return 0;
#		endif
	}

	int CHtHifBase::GetAeCnt() { return HT_AE_CNT; }
	int CHtHifBase::GetUnitCnt() {
#		if defined(HT_SYSC) || defined(HT_MODEL)
			return HT_UNIT_CNT * HT_AE_CNT;
#		else
			return m_pHtHifLibBase->GetUnitCnt();
#		endif
	}

	Ht::EAppMode CHtHifBase::GetAppMode() {
#		if defined(HT_SYSC)
			return eAppSysc;
#		elif defined(HT_VSIM)
			return eAppVsim;
#		elif defined(HT_MODEL)
			return eAppModel;
#		else
			return eAppRun;
#		endif
	}

	void CHtHifBase::HtCoprocSysc() {
#		if defined(HT_SYSC)
			if (Ht::g_vcdn) {
				Ht::g_vcdp = sc_core::sc_create_vcd_trace_file(Ht::g_vcdn);

				if (Ht::g_vcdp)
					fprintf(stderr, "HTLIB: Opened VCD trace file '%s.vcd'\n", Ht::g_vcdn);
				else {
					fprintf(stderr, "HTLIB: Warning: Unable to open '%s.vcd'\n", Ht::g_vcdn);
					exit(-1);
				}
			}

			sc_core::sc_report_handler::set_verbosity_level(sc_core::SC_LOW);

			while (g_coprocAeRunningCnt != 0)
					sc_core::sc_start(10000, sc_core::SC_NS);

			if (Ht::g_vcdp)
				sc_core::sc_close_vcd_trace_file(Ht::g_vcdp);

			sc_core::sc_stop();
#		endif
	}

#	if !defined(HT_MODEL) && !defined(HT_SYSC)

		void CHtHifBase::HtCpInfo(bool *needFlush, volatile bool *busy, uint64_t *aeg2, uint64_t *aeg3, const char *pHtPers) {
			ht_cp_info(&m_pCoproc, &m_sig, pHtPers, needFlush, busy, aeg2, aeg3);
			ht_cp_fw_attach(m_pCoproc, &m_pCoprocFw);
		}

		void CHtHifBase::HtCpDispatch(uint64_t *pBase) {
			ht_cp_dispatch(m_pCoproc, pBase);
		}

		void CHtHifBase::HtCpDispatchWait(uint64_t *pBase) {
			ht_cp_dispatch_wait(m_pCoproc, m_sig, pBase);
		}

		void CHtHifBase::HtCpRelease() {
			ht_cp_release(m_pCoproc);
		}

		void CHtHifBase::HtCpFwRelease() {
			ht_cp_fw_release(m_pCoprocFw);
		}

#	else
		void CHtHifBase::HtCpInfo(bool *needFlush, volatile bool *busy, uint64_t *partNumber, uint64_t *appEngineCnt, const char *pHtPers) {}
		void CHtHifBase::HtCpDispatch(uint64_t *pBase) {}
		void CHtHifBase::HtCpDispatchWait(uint64_t *pBase) {}
		void CHtHifBase::HtCpRelease() {}
		void CHtHifBase::HtCpFwRelease() {}
#	endif

	void * CHtHifBase::HostMemAlloc(size_t size, bool bEnableSystemcAddressValidation) {
		void *ptr = malloc(size);
#		if defined(HT_SYSC)
			g_memTrace.SetAddressRange(ptr, size, false, bEnableSystemcAddressValidation);
#		endif
		return ptr;
	}
	void CHtHifBase::HostMemFree(void * pMem) {
		free(pMem);
	}
	void * CHtHifBase::HostMemAllocAlign(size_t align, size_t size, bool bEnableSystemcAddressValidation)
	{
		void * pMem;
		int ret = ht_posix_memalign(&pMem, align, size);
#		if defined(HT_SYSC)
			g_memTrace.SetAddressRange(pMem, size, false, bEnableSystemcAddressValidation);
#		endif
		if (ret) {
			pMem = NULL;
			fprintf(stderr, "HTLIB: Host posix_memalign(0x%llx, 0x%llx) returned %d\n",
				(long long)align, (long long)size, ret);
		}
		return pMem;
	}
	void CHtHifBase::HostMemFreeAlign(void * pMem) {
#		if defined(_WIN32)
			_aligned_free(pMem);
#		else
			free(pMem);
#		endif
	}
	void * CHtHifBase::HostMemAllocHuge(void * pHostBaseAddr, bool bEnableSystemcAddressValidation) {
		void * pMem;

#		if defined(__linux) && !defined(CNYOS_API)
			int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE | MAP_HUGETLB;
			if (pHostBaseAddr != 0) flags |= MAP_FIXED;
			pMem = mmap(pHostBaseAddr, HUGE_PAGE_SIZE, PROT_READ | PROT_WRITE, flags, -1, 0);
			if (pMem == MAP_FAILED)
				fprintf(stderr, "HTLIB: Host huge page alloc failed (0x%llx, 0x%llx): %s\n",
					(long long)HUGE_PAGE_SIZE, (long long)pHostBaseAddr, strerror(errno));
#		else
			pMem = HostMemAllocAlign(HUGE_PAGE_SIZE, HUGE_PAGE_SIZE, bEnableSystemcAddressValidation);
#		endif
		return pMem;
	}
	void CHtHifBase::HostMemFreeHuge(void * pMem) {
#		if !defined(_WIN32) && !defined(CNYOS_API)
			munlock(pMem, HUGE_PAGE_SIZE);
			if (munmap(pMem, HUGE_PAGE_SIZE) != 0)
				fprintf(stderr, "HTLIB: Host huge free failed (0x%llx)\n", (long long)pMem);
#		else
			HostMemFreeAlign(pMem);
#		endif
	}	void * CHtHifBase::MemAlloc(size_t size) {
#		if !defined(HT_SYSC) && !defined(HT_MODEL) && !defined(_WIN32)
			return ht_cp_mem_alloc(m_pCoproc, size);
#		else
			void *ptr = malloc(size);
#			ifdef HT_SYSC
				g_memTrace.SetAddressRange(ptr, size, true, false);
#			endif
			return ptr;
#		endif
	}
	void CHtHifBase::MemFree(void * pMem) {
#		if !defined(HT_SYSC) && !defined(HT_MODEL) && !defined(_WIN32)
			ht_cp_mem_free(m_pCoproc, pMem);
#		else
			free(pMem);
#		endif
	}
	void * CHtHifBase::MemAllocAlign(size_t align, size_t size) {
		void * pMem;

#		if !defined(HT_SYSC) && !defined(HT_MODEL) && !defined(_WIN32)
			int ret = ht_cp_memalign_alloc(m_pCoproc, &pMem, align, size);
#		else
			int ret = ht_posix_memalign(&pMem, align, size);
#			ifdef HT_SYSC
				g_memTrace.SetAddressRange(pMem, size, true, false);
#			endif
#		endif
		if (ret) {
			pMem = NULL;
			fprintf(stderr, "HTLIB: CP posix_memalign(0x%llx, 0x%llx) returned %d\n",
				(long long)align, (long long)size, ret);
		}
		return pMem;
	}
	void CHtHifBase::MemFreeAlign(void * pMem) {
#		if defined(_WIN32)
			_aligned_free(pMem);
#		else
			MemFree(pMem);
#		endif
	}
	void * CHtHifBase::MemSet(void * pDst, int ch, size_t size) {
#		if !defined(HT_SYSC) && !defined(HT_MODEL) && !defined(_WIN32)
			return ht_cp_memset(m_pCoproc, pDst, ch, size);
#		else
			return memset(pDst, ch, size);
#		endif
	}
	void * CHtHifBase::MemCpy(void * pDst, void * pSrc, size_t size) {
#		if !defined(HT_SYSC) && !defined(HT_MODEL) && !defined(_WIN32)
			return ht_cp_memcpy(m_pCoproc, pDst, pSrc, size);
#		else
			return memcpy(pDst, pSrc, size);
#		endif
	}
	size_t CHtHifBase::GetMemSize(void) {
#		if !defined(HT_SYSC) && !defined(HT_MODEL) && !defined(HT_VSIM) && !defined(_WIN32)
			return ht_cp_mem_size(m_pCoproc);
#		else
			return 2ll*1024*1024*1024;
#		endif
	}
	uint64_t CHtHifBase::UserIOCsrRd(uint64_t addr) {
#		if defined(HT_SYSC) || defined(HT_MODEL)
			if (g_bCsrFuncSet) {
				LockCsr();
				uint64_t rdData = 0;
				if (!g_pCsrCmd(g_pCsrMod, HT_CSR_RD, addr, 0)) {
					fprintf(stderr, "HTLIB: CSR Rd Failed: FIFO was full (this should never occur)\n");
					return -1;
				}
				while (!g_pCsrRdRsp(g_pCsrMod, rdData))
					usleep(10000);
				UnlockCsr();
				usleep(10000);
				return rdData;
			} else {
				fprintf(stderr, "HTLIB: CSR Rd Failed: No Module with AddUserIOCsrIntf available\n");
				return -1;
			}
#		else
			uint64_t rdData = 0;
			if (ht_csr_read(m_pCoprocFw, addr, &rdData)) {
				fprintf(stderr, "HTLIB: CSR Rd Failed\n");
				return -1;
			}
			return rdData;
#		endif
	}
	void CHtHifBase::UserIOCsrWr(uint64_t addr, uint64_t data) {
#		if defined(HT_SYSC) || defined(HT_MODEL)
			if (g_bCsrFuncSet) {
				LockCsr();
				if (!g_pCsrCmd(g_pCsrMod, HT_CSR_WR, addr, data)) {
					fprintf(stderr, "HTLIB: CSR Wr Failed: FIFO was full (this should never occur)\n");
				}
				usleep(10000);
				UnlockCsr();
			} else {
				fprintf(stderr, "HTLIB: CSR Wr Failed: No Module with AddUserIOCsrIntf available\n");
			}
#		else
			if (ht_csr_write(m_pCoprocFw, addr, data)) {
				fprintf(stderr, "HTLIB: CSR Wr Failed\n");
			}
			return;
#		endif
	}

}
