/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#ifndef HASH_H
#define    HASH_H

#ifdef    __cplusplus
extern "C" {
#endif

uint32_t Hash(const void *key, size_t length, const uint32_t initval);

#ifdef    __cplusplus
}
#endif

#endif    /* HASH_H */

