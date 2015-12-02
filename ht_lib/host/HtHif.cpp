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

		void CHtHifBase::HtCpInfo(bool *needFlush, volatile bool *busy, uint64_t *aeg2, uint64_t *aeg3) {
			ht_cp_info(&m_pCoproc, &m_sig, needFlush, busy, aeg2, aeg3);
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

#	else
		void CHtHifBase::HtCpInfo(bool *needFlush, volatile bool *busy, uint64_t *partNumber, uint64_t *appEngineCnt) {}
		void CHtHifBase::HtCpDispatch(uint64_t *pBase) {}
		void CHtHifBase::HtCpDispatchWait(uint64_t *pBase) {}
		void CHtHifBase::HtCpRelease() {}
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

	void * CHtHifBase::MemAlloc(size_t size) {
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
			return ht_cp_mem_size();
#		else
			return 2ll*1024*1024*1024;
#		endif
	}

}
