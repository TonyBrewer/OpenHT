/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
/* See items.c */
uint64_t get_cas_id(void);

/*@null@*/
CItem *do_item_alloc(char *key, const size_t nkey, const uint32_t hv, const int flags, const rel_time_t exptime, const int nbytes);
void item_free(CItem *it);
bool item_size_ok(const size_t nkey, const int flags, const int nbytes);

int  do_item_link(CItem *it);     /** may fail if transgresses limits */
void do_item_unlink(CItem *it);
void do_item_unlink_nolock(CItem *it);
void do_item_remove(CItem *it);
void do_item_update(CItem *it);   /** update LRU time to current and reposition */
int  do_item_replace(CItem *it, CItem *new_it);

/*@null@*/
char *do_item_cachedump(const unsigned int slabs_clsid, const unsigned int limit, unsigned int *bytes);
void do_item_stats_totals(CConn *c);
/*@null@*/
void do_item_stats_sizes(CConn *c);
void do_item_flush_expired(void);

CItem *do_item_get(const char *key, const size_t nkey, const uint32_t hv);
CItem *do_item_touch(const char *key, const size_t nkey, uint32_t exptime, const uint32_t hv);
void item_stats_reset(void);
extern pthread_mutex_t cache_lock;
void item_stats_evictions(uint64_t *evicted);
