// HT Host Platform Driver Interface for Convey WX-690 

#include "Ht.h"

#include <wdm_user.h>
// FIXME - Temporarily disable CSR Access
//#include "<wdm_admin.h>
#include <assert.h>

namespace Ht {
	extern int g_htDebug;
}
using namespace Ht;

void ht_cp_info(void ** ppCoproc, uint64_t * pSig, bool *needFlush, volatile bool *busy, uint64_t *partNumber, uint64_t *appEngineCnt) {
	assert(WDM_INVALID == 0);

	*needFlush = false;

	*ppCoproc = (void *)wdm_reserve_sig(WDM_CPID_ANY, NULL, (char *)HT_PERS);
	if ((wdm_coproc_t)*ppCoproc == WDM_INVALID) {
		if (g_htDebug > 1) {
			fprintf(stderr, "HTLIB: wdm_reserve_sig(\"%s\") failed with %s\n",
				(char *)HT_PERS, strerror(errno));
			if (errno != EBADR) {
				fprintf(stderr, " Please verify that the personality is installed in\n");
				fprintf(stderr, " /opt/micron/personalities or CNY_PERSONALITY_PATH is set.\n");
			}
		}
		throw CHtException(eHtBadDispatch, string("unable to reserve personality signature"));
	}

	if (wdm_attach((wdm_coproc_t)*ppCoproc, (char *)HT_PERS)) {
		if (g_htDebug > 1) {
			fprintf(stderr, "HTLIB: wdm_attach(\"%s\") failed with %s\n",
				(char *)HT_PERS, strerror(errno));
			fprintf(stderr, " Please verify that the personality is installed in\n");
			fprintf(stderr, " /opt/micron/personalities or CNY_PERSONALITY_PATH is set.\n");
		}
		wdm_release((wdm_coproc_t)*ppCoproc);
		throw CHtException(eHtBadDispatch, string("unable to attach"));
	}

	uint64_t aegs[2];
	wdm_dispatch_t ds;
	memset((void *)&ds, 0, sizeof(ds));
	ds.ae[0].aeg_ptr_r = aegs;
	ds.ae[0].aeg_cnt_r = 2;
	ds.ae[0].aeg_base_r = 2;
	if (wdm_aeg_write_read((wdm_coproc_t)*ppCoproc, &ds)) {
		if (g_htDebug > 1)
			fprintf(stderr, "HTLIB: wdm_aeg_write_read() failed with %s\n",
			strerror(errno));
		wdm_detach((wdm_coproc_t)*ppCoproc);
		wdm_release((wdm_coproc_t)*ppCoproc);
		throw CHtException(eHtBadDispatch, string("unable to read/write aeg registers"));
	}

	*partNumber = aegs[0];
	*appEngineCnt = aegs[1];
}

void ht_cp_fw_attach(void *pCoproc, void **ppCoprocFw) {
	// FIXME - Temporarily disable CSR Access
	/*assert(WDM_INVALID == 0);

	wdm_attrs_t attr;
	if (wdm_attributes((wdm_coproc_t)pCoproc, &attr) != 0) {
		fprintf(stderr, "HTLIB: wdm_attributes failed with %s\n",
			strerror(errno));
		return;
	}

	*ppCoprocFw = (void *)wdm_attach_fw(attr.attr_cpid);
	if ((wdm_admin_t)*ppCoprocFw == WDM_ADM_INVALID) {
		if (g_htDebug > 1) {
			fprintf(stderr, "HTLIB: wdm_attach_fw failed with %s\n",
				strerror(errno));
		}
		wdm_detach_fw((wdm_admin_t)*ppCoprocFw);
		throw CHtException(eHtBadDispatch, string("unable to attach fw"));
	}*/
}

int ht_csr_read(void * pCoprocFw, uint64_t offset, uint64_t * data) {
	// FIXME - Temporarily disable CSR Access
	throw CHtException(eHtBadDispatch, string("csr access not supported on this platform"));
	return -1;
	/*if ((wdm_admin_t)pCoprocFw == WDM_ADM_INVALID) {
		if (g_htDebug > 1) {
			fprintf(stderr, "HTLIB: fw was not attached\n");
		}
		throw CHtException(eHtBadDispatch, string("unable to access csr"));
		return -1;
	}
	return wdm_fpga_read_csr((wdm_admin_t)pCoprocFw, WDM_AEMC0, offset, data);*/
}

int ht_csr_write(void * pCoprocFw, uint64_t offset, uint64_t data) {
	// FIXME - Temporarily disable CSR Access
	throw CHtException(eHtBadDispatch, string("csr access not supported on this platform"));
	return -1;
	/*if ((wdm_admin_t)pCoprocFw == WDM_ADM_INVALID) {
		if (g_htDebug > 1) {
			fprintf(stderr, "HTLIB: fw was not attached\n");
		}
		throw CHtException(eHtBadDispatch, string("unable to access csr"));
		return -1;
	}
	return wdm_fpga_write_csr((wdm_admin_t)pCoprocFw, WDM_AEMC0, offset, data);*/
}

void ht_cp_dispatch(void * pCoproc, uint64_t *pBase) {

	wdm_dispatch_t ds;
	memset((void *)&ds, 0, sizeof(ds));
	assert(WDM_AE_CNT <= HT_HIF_AE_CNT_MAX);
	for (int i = 0; i<WDM_AE_CNT; i++) {
		ds.ae[i].aeg_ptr_s = &pBase[i];
		ds.ae[i].aeg_cnt_s = 1;
	}
	if (wdm_dispatch((wdm_coproc_t)pCoproc, &ds)) {
		if (g_htDebug > 1)
			fprintf(stderr, "HTLIB: wdm_dispatch() failed with %s\n", strerror(errno));
		wdm_detach((wdm_coproc_t)pCoproc);
		wdm_release((wdm_coproc_t)pCoproc);
		throw CHtException(eHtBadDispatch, string("unable to dispatch"));
	}
}

void ht_cp_dispatch_wait(void * pCoproc, uint64_t sig, uint64_t *pBase) {
	int ret = 0;
	while (!(ret = wdm_dispatch_status((wdm_coproc_t)pCoproc)))
		usleep(10000);

	if (g_htDebug > 2)
		fprintf(stderr, "HTLIB: wdm_dispatch_status() returned %d\n", ret);

	if (ret < 0) {
		fprintf(stderr, "HTLIB: wdm_dispatch_status() failed with %s\n",
			strerror(errno));
	}
}

void ht_cp_release(void * pCoproc) {
	wdm_detach((wdm_coproc_t)pCoproc);
	wdm_release((wdm_coproc_t)pCoproc);
}

void ht_cp_fw_release(void * pCoprocFw) {
	// FIXME - Temporarily disable CSR Access
	//wdm_detach_fw((wdm_admin_t)pCoprocFw);
}

void * ht_cp_mem_alloc(void * pCoproc, size_t size) {
	return wdm_malloc((wdm_coproc_t)pCoproc, size);
}

void ht_cp_mem_free(void * pCoproc, void * pMem) {
	wdm_free((wdm_coproc_t)pCoproc, pMem);
}

int ht_cp_memalign_alloc(void * pCoproc, void ** ppMem, size_t align, size_t size) {
	return wdm_posix_memalign((wdm_coproc_t)pCoproc, ppMem, align, size);
}

void * ht_cp_memset(void * pCoproc, void * pDst, int ch, size_t size) {
	return wdm_memset((wdm_coproc_t)pCoproc, pDst, ch, size);
}

void * ht_cp_memcpy(void * pCoproc, void * pDst, void * pSrc, size_t size) {
	return wdm_memcpy((wdm_coproc_t)pCoproc, pDst, pSrc, size);
}

size_t ht_cp_mem_size(void * pCoproc) {
	wdm_attrs_t attr;
	if (!wdm_attributes((wdm_coproc_t)pCoproc, &attr))
		return attr.attr_memsize;
	return 0;
}
