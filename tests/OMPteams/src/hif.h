#ifndef HIF_H
#define HIF_H

#include <stdint.h>
#include <omp.h>
#include "Ht.h"

using namespace Ht;

extern CHtHif *pHtHif;
extern CHtAuUnit ** __htc_units;

void *stencil_cp_calloc(size_t elems, size_t size);
void *stencil_cp_alloc(size_t size);
void stencil_cp_free(void *ptr);

extern "C" int __htc_get_unit_count();
extern "C" void ht_attach ();
void ht_init_coproc ();
extern "C" void ht_detach ();

#endif /*  HIF_H */
