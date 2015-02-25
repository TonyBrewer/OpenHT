/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Hash table
 *
 * The hash function used here is by Bob Jenkins, 1996:
 *    <http://burtleburtle.net/bob/hash/doobs.html>
 *       "By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.
 *       You may use this code any way you wish, private, educational,
 *       or commercial.  It's free."
 *
 * The rest of the file is licensed under the BSD license.  See LICENSE.
 */

#include "MemCached.h"

static pthread_cond_t maintenance_cond = PTHREAD_COND_INITIALIZER;


typedef  unsigned       char ub1;   /* unsigned 1-byte quantities */

/* how many powers of 2's worth of buckets we use */
unsigned int hashpower = HASHPOWER_DEFAULT;

#define hashsize(n) (1ULL<<(n))
#define hashmask(n) (hashsize(n)-1)

/* Main hash table. This is where we look except during expansion. */
static CItem** primary_hashtable = 0;

/*
 * Previous hash table. During expansion, we look here for keys that haven't
 * been moved over to the primary yet.
 */
static CItem** old_hashtable = 0;

/* Number of items in the hash table. */
static unsigned int hash_items = 0;

/* Flag: Are we in the middle of expanding now? */
static bool expanding = false;
static bool started_expanding = false;

/*
 * During expansion we migrate values with bucket granularity; this is how
 * far we've gotten so far. Ranges from 0 .. hashsize(hashpower - 1) - 1.
 */
static unsigned int expand_bucket = 0;

void assoc_init(const int hashtable_init) {
    if (hashtable_init) {
        hashpower = hashtable_init;
    }
    primary_hashtable = (CItem **)calloc(hashsize(hashpower), sizeof(void *));
    if (! primary_hashtable) {
        fprintf(stderr, "Failed to init hashtable.\n");
        exit(EXIT_FAILURE);
    }
    STATS_LOCK();
    g_stats.m_hashPowerLevel = hashpower;
    g_stats.m_hashBytes = hashsize(hashpower) * sizeof(void *);
    STATS_UNLOCK();
}

CItem *assoc_find(const char *key, const size_t nkey, const uint32_t hv) {
    CItem *it;
    unsigned int oldbucket;
    CItem *ret = NULL;
    int depth = 0;

    if (expanding &&
        (oldbucket = (hv & hashmask(hashpower - 1))) >= expand_bucket)
    {
        it = old_hashtable[oldbucket];
    } else {
        it = primary_hashtable[hv & hashmask(hashpower)];
    }

    while (it) {
        if ((nkey == it->nkey) && (memcmp(key, ITEM_key(it), nkey) == 0)) {
            ret = it;
            break;
        }
        it = it->h_next;
        ++depth;
    }
    return ret;
}

/* returns the address of the item pointer before the key.  if *item == 0,
   the item wasn't found */

static CItem** _hashitem_before (const char *key, const size_t nkey, const uint32_t hv) {
    CItem **pos;
    unsigned int oldbucket;

    if (expanding &&
        (oldbucket = (hv & hashmask(hashpower - 1))) >= expand_bucket)
    {
        pos = &old_hashtable[oldbucket];
    } else {
        pos = &primary_hashtable[hv & hashmask(hashpower)];
    }

    while (*pos && ((nkey != (*pos)->nkey) || memcmp(key, ITEM_key(*pos), nkey))) {
        pos = &(*pos)->h_next;
    }
    return pos;
}

/* grows the hashtable to the next power of 2. */
static void assoc_expand(void) {
    old_hashtable = primary_hashtable;

    primary_hashtable = (CItem **)calloc(hashsize(hashpower + 1), sizeof(void *));
    if (primary_hashtable) {
        if (g_settings.m_verbose > 1)
            fprintf(stderr, "Hash table expansion starting\n");
        hashpower++;
        expanding = true;
        expand_bucket = 0;
        g_stats.m_hashPowerLevel = hashpower;
        __sync_fetch_and_add(&g_stats.m_hashBytes, hashsize(hashpower) * sizeof(void *));
        g_stats.m_bHashIsExpanding = true;
    } else {
        primary_hashtable = old_hashtable;
        /* Bad news, but we can keep running. */
    }
}

static void assoc_start_expand(void) {
    if (started_expanding)
        return;
    started_expanding = true;
    pthread_cond_signal(&maintenance_cond);
}

/* Note: this isn't an assoc_update.  The key must not already exist to call this */
int assoc_insert(CItem *it) {
    unsigned int oldbucket;

//    assert(assoc_find(ITEM_key(it), it->nkey) == 0);  /* shouldn't have duplicately named things defined */

    if (expanding &&
        (oldbucket = (it->hv & hashmask(hashpower - 1))) >= expand_bucket)
    {
        it->h_next = old_hashtable[oldbucket];
        old_hashtable[oldbucket] = it;
    } else {
        it->h_next = primary_hashtable[it->hv & hashmask(hashpower)];
        primary_hashtable[it->hv & hashmask(hashpower)] = it;
    }

    hash_items++;
    if (! expanding && hash_items > (hashsize(hashpower) * 3) / 2) {
        assoc_start_expand();
    }

    return 1;
}

void assoc_delete(const char *key, const size_t nkey, const uint32_t hv) {
    CItem **before = _hashitem_before(key, nkey, hv);

    if (*before) {
        CItem *nxt;
        hash_items--;
        /* The DTrace probe cannot be triggered as the last instruction
         * due to possible tail-optimization by the compiler
         */
        nxt = (*before)->h_next;
        (*before)->h_next = 0;   /* probably pointless, but whatever. */
        *before = nxt;
        return;
    }
    /* Note:  we never actually get here.  the callers don't delete things
       they can't find. */
    assert(*before != 0);
}


static volatile int do_run_maintenance_thread = 1;

#define DEFAULT_HASH_BULK_MOVE 1
int hash_bulk_move = DEFAULT_HASH_BULK_MOVE;

static void *assoc_maintenance_thread(void *arg) {

    while (do_run_maintenance_thread) {
        int ii = 0;

        /* Lock the cache, and bulk move multiple buckets to the new
         * hash table. */
        item_lock_global();
        mutex_lock(&cache_lock);

        for (ii = 0; ii < hash_bulk_move && expanding; ++ii) {
            CItem *it, *next;
            int bucket;

            for (it = old_hashtable[expand_bucket]; NULL != it; it = next) {
                next = it->h_next;

                bucket = it->hv/*hash(ITEM_key(it), it->nkey, 0)*/ & hashmask(hashpower);
                it->h_next = primary_hashtable[bucket];
                primary_hashtable[bucket] = it;
            }

            old_hashtable[expand_bucket] = NULL;

            expand_bucket++;
            if (expand_bucket == hashsize(hashpower - 1)) {
                expanding = false;
                free(old_hashtable);
                __sync_fetch_and_sub(&g_stats.m_hashBytes, hashsize(hashpower - 1) * sizeof(void *));
                g_stats.m_bHashIsExpanding = false;
                if (g_settings.m_verbose > 1)
                    fprintf(stderr, "Hash table expansion done\n");
            }
        }

        mutex_unlock(&cache_lock);
        item_unlock_global();

        if (!expanding) {
            /* finished expanding. tell all threads to use fine-grained locks */
            switch_item_lock_type(ITEM_LOCK_GRANULAR);
            slabs_rebalancer_resume();
            /* We are done expanding.. just wait for next invocation */
            mutex_lock(&cache_lock);
            started_expanding = false;
            pthread_cond_wait(&maintenance_cond, &cache_lock);
            /* Before doing anything, tell threads to use a global lock */
            mutex_unlock(&cache_lock);
            slabs_rebalancer_pause();
            switch_item_lock_type(ITEM_LOCK_GLOBAL);
            mutex_lock(&cache_lock);
            assoc_expand();
            mutex_unlock(&cache_lock);
        }
    }
    return NULL;
}

static pthread_t maintenance_tid;

int start_assoc_maintenance_thread() {
    int ret;
    char *env = getenv("MEMCACHED_HASH_BULK_MOVE");
    if (env != NULL) {
        hash_bulk_move = atoi(env);
        if (hash_bulk_move == 0) {
            hash_bulk_move = DEFAULT_HASH_BULK_MOVE;
        }
    }
    if ((ret = pthread_create(&maintenance_tid, NULL,
                              assoc_maintenance_thread, NULL)) != 0) {
        fprintf(stderr, "Can't create thread: %s\n", strerror(ret));
        return -1;
    }
    return 0;
}

void stop_assoc_maintenance_thread() {
    mutex_lock(&cache_lock);
    do_run_maintenance_thread = 0;
    pthread_cond_signal(&maintenance_cond);
    mutex_unlock(&cache_lock);

    /* Wait for the maintenance thread to stop */
    pthread_join(maintenance_tid, NULL);
}


