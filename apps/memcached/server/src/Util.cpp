/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "MemCached.h"

/* Avoid warnings on solaris, where isspace() is an index into an array, and gcc uses signed chars */
#define xisspace(c) isspace((unsigned char)c)

bool safe_strtoull(const char *str, uint64_t *out) {
   char *endptr;
   unsigned long long ull;

    assert(out != NULL);
    //errno = 0;
    *out = 0;
    ull = strtoull(str, &endptr, 10);
    if ((errno == ERANGE) || (str == endptr)) {
        return false;
    }

    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        if ((long long) ull < 0) {
            /* only check for negative signs in the uncommon case when
             * the unsigned number is so big that it's negative as a
             * signed number. */
            char c = *endptr;
            *endptr = 0;
            if (strchr(str, '-') != NULL) {
	        *endptr = c;
                return false;
            }
	    *endptr = c;
        }
        *out = ull;
        return true;
    }
    return false;
}

bool safe_strtoll(const char *str, int64_t *out) {
    char *endptr;
    long long ll;

    assert(out != NULL);
    //errno = 0;
    *out = 0;
    ll = strtoll(str, &endptr, 10);
    if ((errno == ERANGE) || (str == endptr)) {
        return false;
    }

    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        *out = ll;
        return true;
    }
    return false;
}

bool safe_strtoul(const char *str, uint32_t *out) {
    char *endptr = NULL;
    unsigned long l = 0;

    assert(out);
    assert(str);
    *out = 0;
    //errno = 0;

    l = strtoul(str, &endptr, 10);
    if ((errno == ERANGE) || (str == endptr)) {
        return false;
    }

    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        if ((long) l < 0) {
            /* only check for negative signs in the uncommon case when
             * the unsigned number is so big that it's negative as a
             * signed number. */
            char c = *endptr;
            *endptr = 0;
            if (strchr(str, '-') != NULL) {
	        *endptr = c;
                return false;
            }
	    *endptr = c;
        }
        *out = l;
        return true;
    }

    return false;
}

bool safe_strtol(const char *str, int32_t *out) {
	char *endptr;
    long l;

    assert(out != NULL);
    //errno = 0;
    *out = 0;
    l = strtol(str, &endptr, 10);
    if ((errno == ERANGE) || (str == endptr)) {
        return false;
    }

    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        *out = l;
        return true;
    }
    return false;
}

void vperror(const char *fmt, ...) {
    //int old_errno = errno;
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    if (vsnprintf(buf, sizeof(buf), fmt, ap) == -1) {
        buf[sizeof(buf) - 1] = '\0';
    }
    va_end(ap);

    //errno = old_errno;

    perror(buf);
}

#ifndef HAVE_HTONLL
static uint64_t mc_swap64(uint64_t in) {
#ifdef ENDIAN_LITTLE
    /* Little endian, flip the bytes around until someone makes a faster/better
    * way to do this. */
    int64_t rv = 0;
    int i = 0;
     for(i = 0; i<8; i++) {
        rv = (rv << 8) | (in & 0xff);
        in >>= 8;
     }
    return rv;
#else
    /* big-endian machines don't need byte swapping */
    return in;
#endif
}

uint64_t ntohll(uint64_t val) {
   return mc_swap64(val);
}

uint64_t htonll(uint64_t val) {
   return mc_swap64(val);
}
#endif

