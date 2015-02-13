#include <stdint.h>
#include <omp.h>
#include "Ht.h"

using namespace Ht;

CHtHif *pHtHif;
CHtAuUnit ** __htc_units = NULL;

void *stencil_cp_calloc(size_t elems, size_t size)
{
	void *ptr = pHtHif->MemAllocAlign( 4*1024, elems*size);
        pHtHif->MemSet(ptr, (char)0, elems*size);
	return ptr;
}

void *stencil_cp_alloc(size_t size)
{
	void *ptr = pHtHif->MemAllocAlign( 4*1024, size);
	return ptr;
}

void stencil_cp_free(void *ptr)
{
	pHtHif->MemFreeAlign(ptr);
	return;
}

extern "C" int __htc_get_unit_count() {
    fprintf(stderr,"__htc_get_unit_count returning %d\n", pHtHif->GetUnitCnt());
    return pHtHif->GetUnitCnt();
}


extern "C" void ht_attach ()
{
	// when running in systemC, we only want to construct the class once
	if (!pHtHif) {
		pHtHif = new CHtHif();
#ifdef DEBUG
        	fprintf(stderr, "Hif constructed \n");
	} else {
	    	fprintf(stderr, "Hif exists already \n");
#endif /* DEBUG */
	}
}

void ht_init_coproc ()
{
	if (!pHtHif) {
		ht_attach();
        }

        int unitCnt = pHtHif->GetUnitCnt();

        printf("initializing units array for %d units\n", unitCnt);

        // Initialize the __htc_units array
        if (!__htc_units) {
            __htc_units = (CHtAuUnit **)malloc(unitCnt * sizeof(CHtAuUnit *));
            for (int unit = 0; unit < unitCnt; unit++)
                __htc_units[unit] = new CHtAuUnit(pHtHif);
        }
}

extern "C" void ht_detach ()
{
// don't delete the class when running in systemC
#ifndef HT_SYSC
#ifdef DEBUG
		fprintf(stderr, "deleting pHtHif\n");
#endif /* DEBUG */
	delete pHtHif;
	pHtHif = NULL;
#endif // HT_SYSC

}
