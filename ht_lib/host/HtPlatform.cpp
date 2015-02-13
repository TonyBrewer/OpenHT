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

//
// Memory wrappers
//
int ht_posix_memalign(void **memptr, size_t alignment, size_t size) {
	int ret;
#if !defined(_WIN32)
	ret = posix_memalign(memptr, alignment, size);
#else
	*memptr = _aligned_malloc(size, alignment);
	ret = *memptr == NULL ? -1 : 0;
#endif
	return ret;
}

void ht_free_memalign(void *ptr) {
#if defined(_WIN32)
	_aligned_free(ptr);
#else
	free(ptr);
#endif
}

#ifdef HT_SYSC
bool __CnyRndInit = true;
uint32_t __CnyMTRand()
{
	static bool m_init;
	static MTRand_int32 m_rng;

	if (!m_init) {
		unsigned long seed = 0xdeadbeef;
#ifndef _WIN32
		char *ep = getenv("CNY_HTSEED");
		if (ep) {
			seed = atoi(ep);
			fprintf(stderr, "CNY_HTSEED: 0x%lx\n", seed);
		}
#endif
		m_rng.seed(seed);
		m_init = true;
	}
	return m_rng();
}
#endif
