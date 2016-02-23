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

# ifdef CNYOS_API
#  include <convey/usr/cny_comp.h>
#  include <convey/sys/cnysys_arch.h>
# else
#  include <wdm_user.h>
# endif

extern "C" long PersDisp();
extern "C" void PersInfo(unsigned long *aeg2, unsigned long *aeg3);
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
#		ifndef CNYOS_API
			/////////////////////////////////////////////////////////////////////////////
			// Convey Driver API
			/////////////////////////////////////////////////////////////////////////////

			void CHtHifBase::HtCpInfo(bool *needFlush, volatile bool *busy, uint64_t *aeg2, uint64_t *aeg3) {

				*needFlush = false;

				m_coproc = wdm_reserve_sig(WDM_CPID_ANY, NULL, (char *)HT_PERS);
				if (m_coproc == WDM_INVALID) {
					if (g_htDebug > 1) {
						fprintf(stderr, "HTLIB: wdm_reserve_sig(\"%s\") failed with %s\n",
							(char *)HT_PERS, strerror(errno));
						if (errno != EBADR) {
							fprintf(stderr, " Please verify that the personality is installed in\n");
							fprintf(stderr, " /opt/convey/personalities or CNY_PERSONALITY_PATH is set.\n");
						}
					}
					throw CHtException(eHtBadDispatch, string("unable to reserve personality signature"));
				}

				if (wdm_attach(m_coproc, (char *)HT_PERS)) {
					if (g_htDebug > 1) {
						fprintf(stderr, "HTLIB: wdm_attach(\"%s\") failed with %s\n",
							(char *)HT_PERS, strerror(errno));
						fprintf(stderr, " Please verify that the personality is installed in\n");
						fprintf(stderr, " /opt/convey/personalities or CNY_PERSONALITY_PATH is set.\n");
					}
					wdm_release(m_coproc);
					throw CHtException(eHtBadDispatch, string("unable to attach"));
				}

				uint64_t aegs[2];
				wdm_dispatch_t ds;
				memset((void *)&ds, 0, sizeof(ds));
				ds.ae[0].aeg_ptr_r = aegs;
				ds.ae[0].aeg_cnt_r = 2;
				ds.ae[0].aeg_base_r = 2;
				if (wdm_aeg_write_read(m_coproc, &ds)) {
					if (g_htDebug > 1)
						fprintf(stderr, "HTLIB: wdm_aeg_write_read() failed with %s\n",
							strerror(errno));
					wdm_detach(m_coproc);
					wdm_release(m_coproc);
					throw CHtException(eHtBadDispatch, string("unable to read/write aeg registers"));
				}

				*aeg2 = aegs[0];
				*aeg3 = aegs[1];
			}

			void CHtHifBase::HtCpDispatch(uint64_t *pBase) {

				wdm_dispatch_t ds;
				memset((void *)&ds, 0, sizeof(ds));
				assert(WDM_AE_CNT <= HT_HIF_AE_CNT_MAX);
				for (int i=0; i<WDM_AE_CNT; i++) {
					ds.ae[i].aeg_ptr_s = &pBase[i];
					ds.ae[i].aeg_cnt_s = 1;
				}
				if (wdm_dispatch(m_coproc, &ds)) {
					if (g_htDebug > 1)
						fprintf(stderr, "HTLIB: wdm_dispatch() failed with %s\n", strerror(errno));
					wdm_detach(m_coproc);
					wdm_release(m_coproc);
					throw CHtException(eHtBadDispatch, string("unable to dispatch"));
				}
			}

			void CHtHifBase::HtCpDispatchWait(uint64_t *pBase) {
				int ret = 0;
				while (!(ret = wdm_dispatch_status(m_coproc)))
					usleep(10000);

				if (ret < 0) {
					fprintf(stderr, "HTLIB: wdm_dispatch_status() failed with %s\n",
						strerror(errno));
				}
			}

			void CHtHifBase::HtCpRelease() {
				wdm_detach(m_coproc);
				wdm_release(m_coproc);
			}
#		else
			/////////////////////////////////////////////////////////////////////////////
			// ConveyOS API
			/////////////////////////////////////////////////////////////////////////////
			static cny_image_t m_sig;

			void CHtHifBase::HtCpInfo(bool *needFlush, volatile bool *busy, uint64_t *aeg2, uint64_t *aeg3) {

				if (cny_cp_sys_get_sysiconnect() == CNY_SYSTYPE_ICONNECT_FSB) {
					*needFlush = true;
					if (g_htDebug > 2)
						fprintf(stderr, "HTLIB: Enabling OBLK Flushing\n");
				}

				int srtn = 0;
				cny_image_t sig2 = 0L;
				cny_get_signature((char *)HT_PERS, &m_sig, &sig2, &srtn);

				if (g_htDebug > 2)
					fprintf(stderr, "HTLIB: cny_get_signature(\"%s\", ...) returned %d\n", (char *)HT_PERS, srtn);

				if (srtn != 0) {
					if (g_htDebug > 1) {
						fprintf(stderr, "HT_LIB: Unable to get signature \"%s\" using the ConveyOS API\n",
							(char *)HT_PERS);
						fprintf(stderr, " Please verify that the personality is installed in\n");
						fprintf(stderr, " /opt/convey/personalities or CNY_PERSONALITY_PATH is set.\n");
					}
					throw CHtException(eHtBadDispatch, string("unable to get personality signature"));
				}

				if (!cny$coprocessor_ok) {
					long sigs_for_attach[2] = { 0L, 0L };
					sigs_for_attach[0] = (long)m_sig;
					cny_user_attach_coprocessor(sigs_for_attach);
				}

				if (cny$coprocessor_ok) {
					// make sure we have binary interleave
					if (cny_cp_interleave() == CNY_MI_3131) {
						if (g_htDebug > 1)
							fprintf(stderr, "interleave set to 3131, this personality requires binary\n");
						throw CHtException(eHtBadDispatch, string("inappropriate coprocessor memory interleave"));
					}

					cny$wait_strategy = CNY_POLL_WAIT;
					cny$wait_strategy_time = 100;

					int status = 0;
					copcall_stat_fmt(m_sig, (void (*)()) & PersInfo, &status,
						"AA", aeg2, aeg3);

					if (status) {
						if (g_htDebug > 1)
							fprintf(stderr, "unable to check personality status");
						throw CHtException(eHtBadDispatch, string("unable to check personality status"));
					}
				} else {
					if (g_htDebug > 1)
						printf("Coprocessor is not OK, terminating...\n");
					*busy = false;
					throw CHtException(eHtBadDispatch, string("bad coprocessor status"));
				}
			}

			void CHtHifBase::HtCpDispatch(uint64_t *pBase) {}
			void CHtHifBase::HtCpDispatchWait(uint64_t *pBase) {
				l_copcall_fmt(m_sig, PersDisp, "AAAAA",
				HT_AE_CNT, pBase[0], pBase[1], pBase[2], pBase[3]);
			}
			void CHtHifBase::HtCpRelease() {}
#		endif
#	else
		void CHtHifBase::HtCpInfo(bool *needFlush, volatile bool *busy, uint64_t *aeg2, uint64_t *aeg3) {}
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
	void * CHtHifBase::HostMemAllocHuge(void * pHostBaseAddr) {
		void * pMem;

#		if !defined(_WIN32) && !defined(CNYOS_API)
			int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE | MAP_HUGETLB;
			if (pHostBaseAddr != 0) flags |= MAP_FIXED;
			pMem = mmap(pHostBaseAddr, HUGE_PAGE_SIZE, PROT_READ | PROT_WRITE, flags, -1, 0);
			if (pMem == 0)
				fprintf(stderr, "HTLIB: Host huge page alloc failed (0x%llx, 0x%llx)\n",
				(long long)HUGE_PAGE_SIZE, (long long)pHostBaseAddr);
#		else
			pMem = HostMemAllocAlign(HUGE_PAGE_SIZE, HUGE_PAGE_SIZE);
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
	}
	void * CHtHifBase::MemAlloc(size_t size) {
#		if !defined(HT_SYSC) && !defined(HT_MODEL) && !defined(_WIN32)
#			ifndef CNYOS_API
				return wdm_malloc(m_coproc, size);
#			else
				return cny_cp_malloc(size);
#			endif
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
#			ifndef CNYOS_API
				wdm_free(m_coproc, pMem);
#			else
				cny_cp_free(pMem);
#			endif
#		else
			free(pMem);
#		endif
	}
	void * CHtHifBase::MemAllocAlign(size_t align, size_t size) {
		void * pMem;

#		if !defined(HT_SYSC) && !defined(HT_MODEL) && !defined(_WIN32)
#			ifndef CNYOS_API
				int ret = wdm_posix_memalign(m_coproc, &pMem, align, size);
#			else
				int ret = cny_cp_posix_memalign(&pMem, align, size);
#			endif
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
#			ifndef CNYOS_API
				return wdm_memset(m_coproc, pDst, ch, size);
#			else
				return cny_cp_memset(pDst, ch, size);
#			endif
#		else
			return memset(pDst, ch, size);
#		endif
	}
	void * CHtHifBase::MemCpy(void * pDst, void * pSrc, size_t size) {
#		if !defined(HT_SYSC) && !defined(HT_MODEL) && !defined(_WIN32)
#			ifndef CNYOS_API
				return wdm_memcpy(m_coproc, pDst, pSrc, size);
#			else
				return cny_cp_memcpy(pDst, pSrc, size);
#			endif
#		else
			return memcpy(pDst, pSrc, size);
#		endif
	}
	size_t CHtHifBase::GetMemSize(void) {
#		if !defined(HT_SYSC) && !defined(HT_MODEL) && !defined(HT_VSIM) && !defined(_WIN32)
#			ifndef CNYOS_API
				wdm_attrs_t attr;
				if (!wdm_attributes(m_coproc, &attr))
					return attr.attr_memsize;
				return 0;
#			else
				return cny_cp_mem_size();
#			endif
#		else
			return 2ll*1024*1024*1024;
#		endif
	}

}
