// HT Host Platform Driver Interface for Convey HC-2ex

#include "Ht.h"

#include <convey/usr/cny_comp.h>
#include <convey/sys/cnysys_arch.h>

namespace Ht {
	extern int g_htDebug;
}
using namespace Ht;

extern "C" long PersDisp();
extern "C" void PersInfo(unsigned long *partNumber, uint64_t *appEngineCnt);

void ht_cp_info(void ** pCoproc, uint64_t * pSig, bool *needFlush, volatile bool *busy, uint64_t *partNumber, uint64_t *appEngineCnt) {

	if (cny_cp_sys_get_sysiconnect() == CNY_SYSTYPE_ICONNECT_FSB) {
		*needFlush = true;
		if (g_htDebug > 2)
			fprintf(stderr, "HTLIB: Enabling OBLK Flushing\n");
	}

	int srtn = 0;
	cny_image_t sig;
	cny_image_t sig2 = 0L;
	cny_get_signature((char *)HT_PERS, &sig, &sig2, &srtn);
	*(cny_image_t *)pSig = sig;

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
		sigs_for_attach[0] = *pSig;
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
		copcall_stat_fmt(*(cny_image_t*)pSig, (void(*)()) & PersInfo, &status,
			"AA", partNumber, appEngineCnt);

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

void ht_cp_dispatch(void * pCoproc, uint64_t *pBase) {}

void ht_cp_dispatch_wait(void * pCoproc, uint64_t sig, uint64_t *pBase) {
	l_copcall_fmt((cny_image_t)sig, PersDisp, "AAAAA", HT_AE_CNT, pBase[0], pBase[1], pBase[2], pBase[3]);
}

void ht_cp_release(void * pCoproc) {}

void * ht_cp_mem_alloc(void * pCoproc, size_t size) {
	return cny_cp_malloc(size);
}

void ht_cp_mem_free(void * pCoproc, void * pMem) {
	cny_cp_free(pMem);
}

int ht_cp_memalign_alloc(void * pCoproc, void ** ppMem, size_t align, size_t size) {
	return cny_cp_posix_memalign(ppMem, align, size);
}

void * ht_cp_memset(void * pCoproc, void * pDst, int ch, size_t size) {
	return cny_cp_memset(pDst, ch, size);
}

void * ht_cp_memcpy(void * pCoproc, void * pDst, void * pSrc, size_t size) {
	return cny_cp_memcpy(pDst, pSrc, size);
}

size_t ht_cp_mem_size(void * pCoproc) {
	return cny_cp_mem_size();
}
