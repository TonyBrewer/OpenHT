#ifndef EXTERN_C
#if defined(__cplusplus)
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif
#endif

#ifdef CNY_HTC_HOST

#include "Ht.h"

using namespace Ht;

EXTERN_C CHtAuUnit ** __htc_units;

#endif // CNY_HTC_HOST

#ifndef HT_VERILOG
EXTERN_C int __htc_get_unit_count();
EXTERN_C void pers_attach();
EXTERN_C void pers_detach();
EXTERN_C void *pers_cp_malloc(size_t size);
EXTERN_C void pers_cp_memcpy(void *dst, void *src, size_t len);
#endif

