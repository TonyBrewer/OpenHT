#include <pthread.h>

#include "Ht.h"
using namespace Ht;

#undef DEBUG
#define DEBUG

static CHtHif *pHtHif;
static CHtAuUnit **pAuUnits;

///////////////////////////////////////////////////////////////////////////////
// HTC symbols
///////////////////////////////////////////////////////////////////////////////

CHtAuUnit ** __htc_units = NULL;

extern "C" void pers_attach();
extern "C" int __htc_get_unit_count()
{
	if (!pHtHif) pers_attach();

        return 8;

	return pHtHif->GetUnitCnt();
}

///////////////////////////////////////////////////////////////////////////////
// lib API
///////////////////////////////////////////////////////////////////////////////

extern "C" void pers_attach()
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&mutex);

	// did a racing thread alread do this?
	if (pHtHif)
		return;

	pHtHif = new CHtHif();

	int unitCnt = pHtHif->GetUnitCnt();
	pAuUnits = (CHtAuUnit **)malloc(unitCnt * sizeof(CHtAuUnit *));
	for (int unit = 0; unit < unitCnt; unit++)
		pAuUnits[unit] = new CHtAuUnit(pHtHif);

	__htc_units = pAuUnits;
	
#ifdef DEBUG
	fprintf(stderr, "Hif constructed #AUs = %d\n", unitCnt);
#endif

	pthread_mutex_unlock(&mutex);
}

//extern "C" void *pers_cp_malloc(size_t size)
extern "C" uint64_t pers_cp_malloc(size_t size)
{
	if (!pHtHif) pers_attach();

        //	void *ptr = pHtHif->MemAllocAlign(4 * 1024, size);
	uint64_t ptr  = (uint64_t)pHtHif->MemAllocAlign(4 * 1024, size);

	return ptr;
}

extern "C" void pers_cp_free(void *ptr)
{
	if (!pHtHif) pers_attach();

	pHtHif->MemFreeAlign(ptr);
	return;
}

extern "C" void pers_cp_memcpy(void *dst, void *src, size_t len)
{
	if (!pHtHif) pers_attach();

	pHtHif->MemCpy(dst, src, len);
	return;
}

extern "C" void pers_detach()
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&mutex);

	// did a racing thread alread do this?
	if (!pHtHif)
		return;

#ifdef DEBUG
	fprintf(stderr, "deleting pHtHif\n");
#endif

	delete pHtHif;
	pHtHif = NULL;

	pthread_mutex_unlock(&mutex);
}
