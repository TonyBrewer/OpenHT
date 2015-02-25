/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Thread management for memcached.
 */
#include "MemCached.h"

#ifdef ONLOAD
#include <onload/extensions.h>
extern char        **g_onload_stack;
#endif

/* Lock for cache operations (item_*, assoc_*) */
pthread_mutex_t cache_lock;

/* Connection lock around accepting new connections */
pthread_mutex_t conn_lock = PTHREAD_MUTEX_INITIALIZER;

#if !defined(HAVE_GCC_ATOMICS) && !defined(__sun)
pthread_mutex_t atomics_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* Lock for global stats */
static pthread_mutex_t stats_lock;

static pthread_mutex_t *item_locks;
/* size of the item lock hash table */
static uint32_t item_lock_count;
#define hashsize(n) ((unsigned long int)1<<(n))
#define hashmask(n) (hashsize(n)-1)
/* this lock is temporarily engaged during a hash table expansion */
static pthread_mutex_t item_global_lock;
/* thread-specific variable for deeply finding the item lock type */
pthread_key_t item_lock_type_key;

static LIBEVENT_DISPATCHER_THREAD dispatcher_thread;

/*
 * Each libevent instance has a wakeup pipe, which other threads
 * can use to signal that they've put a new connection on its queue.
 */

/*
 * Number of worker threads that have finished setting themselves up.
 */
int init_count = 0;
pthread_mutex_t init_lock;
pthread_cond_t init_cond;

void thread_libevent_process(int fd, short which, void *arg);

unsigned short refcount_incr(unsigned short *refcount) {
#ifdef HAVE_GCC_ATOMICS
    return __sync_add_and_fetch(refcount, 1);
#elif defined(__sun)
    return atomic_inc_ushort_nv(refcount);
#else
    unsigned short res;
    mutex_lock(&atomics_mutex);
    (*refcount)++;
    res = *refcount;
    mutex_unlock(&atomics_mutex);
    return res;
#endif
}

unsigned short refcount_decr(unsigned short *refcount) {
#ifdef HAVE_GCC_ATOMICS
    return __sync_sub_and_fetch(refcount, 1);
#elif defined(__sun)
    return atomic_dec_ushort_nv(refcount);
#else
    unsigned short res;
    mutex_lock(&atomics_mutex);
    (*refcount)--;
    res = *refcount;
    mutex_unlock(&atomics_mutex);
    return res;
#endif
}

/* Convenience functions for calling *only* when in ITEM_LOCK_GLOBAL mode */
void item_lock_global(void) {
    mutex_lock(&item_global_lock);
}

void item_unlock_global(void) {
    mutex_unlock(&item_global_lock);
}

void item_lock(uint32_t hv) {
    //uint8_t *lock_type = pthread_getspecific(item_lock_type_key);
    //if (likely(*lock_type == ITEM_LOCK_GRANULAR)) {
        mutex_lock(&item_locks[(hv & hashmask(hashpower)) % item_lock_count]);
    //} else {
    //    mutex_lock(&item_global_lock);
    //}
}

/* Special case. When ITEM_LOCK_GLOBAL mode is enabled, this should become a
 * no-op, as it's only called from within the item lock if necessary.
 * However, we can't mix a no-op and threads which are still synchronizing to
 * GLOBAL. So instead we just always try to lock. When in GLOBAL mode this
 * turns into an effective no-op. Threads re-synchronize after the power level
 * switch so it should stay safe.
 */
void *item_trylock(uint32_t hv) {
    pthread_mutex_t *lock = &item_locks[(hv & hashmask(hashpower)) % item_lock_count];
    if (pthread_mutex_trylock(lock) == 0) {
        return lock;
    }
    return NULL;
}

void item_trylock_unlock(void *lock) {
    mutex_unlock((pthread_mutex_t *) lock);
}

void item_unlock(uint32_t hv) {
    //uint8_t *lock_type = pthread_getspecific(item_lock_type_key);
    //if (likely(*lock_type == ITEM_LOCK_GRANULAR)) {
        mutex_unlock(&item_locks[(hv & hashmask(hashpower)) % item_lock_count]);
    //} else {
    //    mutex_unlock(&item_global_lock);
    //}
}

static void wait_for_thread_registration(int nthreads) {
    while (init_count < nthreads) {
        pthread_cond_wait(&init_cond, &init_lock);
    }
}

void switch_item_lock_type(enum item_lock_types type) {
	CDispCmd dispCmd = { 0 };
    int i;

    switch (type) {
        case ITEM_LOCK_GRANULAR:
            dispCmd.m_cmd = 'l';
            break;
        case ITEM_LOCK_GLOBAL:
            dispCmd.m_cmd = 'g';
            break;
        default:
            fprintf(stderr, "Unknown lock type: %d\n", type);
            assert(0);
            break;
    }

    pthread_mutex_lock(&init_lock);
    init_count = 0;
    for (i = 0; i < g_settings.num_threads; i++)
		g_pWorkers[i]->m_dispCmdQue.Push(dispCmd);

    wait_for_thread_registration(g_settings.num_threads);
    pthread_mutex_unlock(&init_lock);
}

/*
 * Sets whether or not we accept new connections.
 */
void AcceptNewConns(const bool do_accept) {
    pthread_mutex_lock(&conn_lock);
    DoAcceptNewConns(do_accept);
    pthread_mutex_unlock(&conn_lock);
}
/****************************** LIBEVENT THREADS *****************************/

/* Which thread we assigned a connection to most recently. */
static int last_thread = -1;

#ifdef ONLOAD
static int next_stack = 0;
#endif

/*
 * Dispatches a new connection to another thread. This is only ever called
 * from the main thread, either during initialization (for UDP) or because
 * of an incoming connection.
 */
void dispatch_conn_new(int sfd, bool bListening, int eventFlags,
                       int readBufferSize, ETransport transport) {

    int tid = (last_thread + 1) % g_settings.num_threads;
    CWorker *pWorker = g_pWorkers[tid];
    last_thread = tid;

#ifdef ONLOAD
    // As we round-robin among the worker threads, also round-robin among
    // Onload stacks. Note that onload_move_fd() works for TCP sockets only.
    if (g_onload_stack && transport == tcp_transport) {
        onload_set_stackname(ONLOAD_THIS_THREAD, ONLOAD_SCOPE_PROCESS, g_onload_stack[next_stack]);
        onload_move_fd(sfd);
        onload_set_stackname(ONLOAD_THIS_THREAD, ONLOAD_SCOPE_PROCESS, "");
        next_stack = (next_stack + 1) % g_settings.onload_stacks;
    }
#endif

	CDispCmd dispCmd;
	dispCmd.m_cmd = 'c';
	dispCmd.m_sfd = sfd;
    dispCmd.m_bListening = bListening;
    dispCmd.m_eventFlags = eventFlags;
    dispCmd.m_readBufferSize = readBufferSize;
    dispCmd.m_transport = transport;

	pWorker->m_dispCmdQue.Push(dispCmd);
}

/*
 * Returns true if this is the thread that listens for new TCP connections.
 */
int is_listen_thread() {
#ifdef _WIN32
    return pthread_self().p == dispatcher_thread.thread_id.p;
#else
    return pthread_self() == dispatcher_thread.thread_id;
#endif
}

/********************************* ITEM ACCESS *******************************/

/*
 * Allocates a new item.
 */
CItem * ItemAlloc(char *key, size_t nkey, uint32_t hv, int flags, rel_time_t exptime, int nbytes) {
    CItem *it;
    /* do_item_alloc handles its own locks */
    it = do_item_alloc(key, nkey, hv, flags, exptime, nbytes);
    return it;
}

/*
 * Returns an item if it hasn't been marked as expired,
 * lazy-expiring as needed.
 */
CItem *ItemGet(const char *key, const size_t nkey, const uint32_t hv) {
    item_lock(hv);
    CItem *it = do_item_get(key, nkey, hv);
    item_unlock(hv);
    return it;
}

CItem *ItemTouch(const char *key, const size_t nkey, uint32_t hv, uint32_t exptime) {
    item_lock(hv);
    CItem *it = do_item_touch(key, nkey, exptime, hv);
    item_unlock(hv);
    return it;
}

/*
 * Links an item into the LRU and hashtable.
 */
int item_link(CItem *item) {
	uint32_t hv = item->hv;
    item_lock(hv);
    int ret = do_item_link(item);
    item_unlock(hv);
    return ret;
}

/*
 * Decrements the reference count on an item and adds it to the freelist if
 * needed.
 */
void item_remove(CItem *item) {
    uint32_t hv = item->hv;
    item_lock(hv);
    do_item_remove(item);
    item_unlock(hv);
}

/*
 * Replaces one item with another in the hashtable.
 * Unprotected by a mutex lock since the core server does not require
 * it to be thread-safe.
 */
int item_replace(CItem *old_it, CItem *new_it) {
    return do_item_replace(old_it, new_it);
}

/*
 * Unlinks an item from the LRU and hashtable.
 */
void item_unlink(CItem *item) {
    uint32_t hv = item->hv;
    item_lock(hv);
    do_item_unlink(item);
    item_unlock(hv);
}

/*
 * Moves an item to the back of the LRU queue.
 */
void item_update(CItem *item) {
    uint32_t hv = item->hv;
    item_lock(hv);
    do_item_update(item);
    item_unlock(hv);
}

/*
 * Does arithmetic on a numeric item value.
 */
EDeltaResultType add_delta(CConn *c, const char *key, const size_t nkey,
                                 const uint32_t hv, const bool incr,
                                 const uint64_t delta, char *buf,
                                 uint64_t *cas) {
    EDeltaResultType ret;

	item_lock(hv);
    ret = c->DoAddDelta(key, nkey, hv, incr, delta, buf, cas);
    item_unlock(hv);
    return ret;
}

/*
 * Stores an item in the cache (high level, obeys set/add/replace semantics)
 */
EStoreItemType store_item(CItem *item, int comm, CConn* c) {
    EStoreItemType ret;
    uint32_t hv = item->hv;
    item_lock(hv);
    ret = c->DoStoreItem(item, comm);
    item_unlock(hv);
    return ret;
}

/*
 * Flushes expired items after a flush_all call
 */
void item_flush_expired() {
    mutex_lock(&cache_lock);
    do_item_flush_expired();
    mutex_unlock(&cache_lock);
}

/*
 * Dumps part of the cache
 */
char *item_cachedump(const unsigned int slabs_clsid, const unsigned int limit, unsigned int *bytes) {
    char *ret;

    mutex_lock(&cache_lock);
    ret = do_item_cachedump(slabs_clsid, limit, bytes);
    mutex_unlock(&cache_lock);
    return ret;
}

/*
 * Dumps statistics about slab classes
 */
void  item_stats(CConn *c) {
    mutex_lock(&cache_lock);
    c->DoItemStats();
    mutex_unlock(&cache_lock);
}

void  item_stats_totals(CConn *c) {
    mutex_lock(&cache_lock);
    do_item_stats_totals(c);
    mutex_unlock(&cache_lock);
}

/*
 * Dumps a list of objects of each size in 32-byte increments
 */
void  item_stats_sizes(CConn *c) {
    mutex_lock(&cache_lock);
    do_item_stats_sizes(c);
    mutex_unlock(&cache_lock);
}

/******************************* GLOBAL STATS ******************************/

void STATS_LOCK() {
    pthread_mutex_lock(&stats_lock);
}

void STATS_UNLOCK() {
    pthread_mutex_unlock(&stats_lock);
}

void threadlocal_stats_reset(void) {
    int ii, sid;
    for (ii = 0; ii < g_settings.num_threads; ++ii) {
        //pthread_mutex_lock(&g_pWorkers[ii]->m_stats.mutex);

        g_pWorkers[ii]->m_stats.m_getCmds = 0;
        g_pWorkers[ii]->m_stats.m_getMisses = 0;
        g_pWorkers[ii]->m_stats.m_touchCmds = 0;
        g_pWorkers[ii]->m_stats.m_touchMisses = 0;
        g_pWorkers[ii]->m_stats.m_deleteMisses = 0;
        g_pWorkers[ii]->m_stats.m_incrMisses = 0;
        g_pWorkers[ii]->m_stats.m_decrMisses = 0;
        g_pWorkers[ii]->m_stats.m_casMisses = 0;
        g_pWorkers[ii]->m_stats.m_bytesRead = 0;
        g_pWorkers[ii]->m_stats.m_bytesWritten = 0;
        g_pWorkers[ii]->m_stats.m_flushCmds = 0;
        g_pWorkers[ii]->m_stats.m_connYields = 0;
        g_pWorkers[ii]->m_stats.m_authCmds = 0;
        g_pWorkers[ii]->m_stats.m_authErrors = 0;

        for(sid = 0; sid < MAX_NUMBER_OF_SLAB_CLASSES; sid++) {
            g_pWorkers[ii]->m_stats.m_slabStats[sid].m_setCmds = 0;
            g_pWorkers[ii]->m_stats.m_slabStats[sid].m_getHits = 0;
            g_pWorkers[ii]->m_stats.m_slabStats[sid].m_touchHits = 0;
            g_pWorkers[ii]->m_stats.m_slabStats[sid].m_deleteHits = 0;
            g_pWorkers[ii]->m_stats.m_slabStats[sid].m_incrHits = 0;
            g_pWorkers[ii]->m_stats.m_slabStats[sid].m_decrHits = 0;
            g_pWorkers[ii]->m_stats.m_slabStats[sid].m_casHits = 0;
            g_pWorkers[ii]->m_stats.m_slabStats[sid].m_casBadVal = 0;
        }

        //pthread_mutex_unlock(&g_pWorkers[ii]->m_stats.mutex);
    }
}

void threadlocal_stats_aggregate(CThreadStats *stats) {
    int ii, sid;

    /* The struct has a mutex, but we can safely set the whole thing
     * to zero since it is unused when aggregating. */
    memset(stats, 0, sizeof(*stats));

    for (ii = 0; ii < g_settings.num_threads; ++ii) {
        //pthread_mutex_lock(&g_pWorkers[ii]->m_stats.mutex);

        stats->m_getCmds += g_pWorkers[ii]->m_stats.m_getCmds;
        stats->m_getMisses += g_pWorkers[ii]->m_stats.m_getMisses;
        stats->m_touchCmds += g_pWorkers[ii]->m_stats.m_touchCmds;
        stats->m_touchMisses += g_pWorkers[ii]->m_stats.m_touchMisses;
        stats->m_deleteMisses += g_pWorkers[ii]->m_stats.m_deleteMisses;
        stats->m_decrMisses += g_pWorkers[ii]->m_stats.m_decrMisses;
        stats->m_incrMisses += g_pWorkers[ii]->m_stats.m_incrMisses;
        stats->m_casMisses += g_pWorkers[ii]->m_stats.m_casMisses;
        stats->m_bytesRead += g_pWorkers[ii]->m_stats.m_bytesRead;
        stats->m_bytesWritten += g_pWorkers[ii]->m_stats.m_bytesWritten;
        stats->m_flushCmds += g_pWorkers[ii]->m_stats.m_flushCmds;
        stats->m_connYields += g_pWorkers[ii]->m_stats.m_connYields;
        stats->m_authCmds += g_pWorkers[ii]->m_stats.m_authCmds;
        stats->m_authErrors += g_pWorkers[ii]->m_stats.m_authErrors;

        for (sid = 0; sid < MAX_NUMBER_OF_SLAB_CLASSES; sid++) {
            stats->m_slabStats[sid].m_setCmds +=
                g_pWorkers[ii]->m_stats.m_slabStats[sid].m_setCmds;
            stats->m_slabStats[sid].m_getHits +=
                g_pWorkers[ii]->m_stats.m_slabStats[sid].m_getHits;
            stats->m_slabStats[sid].m_touchHits +=
                g_pWorkers[ii]->m_stats.m_slabStats[sid].m_touchHits;
            stats->m_slabStats[sid].m_deleteHits +=
                g_pWorkers[ii]->m_stats.m_slabStats[sid].m_deleteHits;
            stats->m_slabStats[sid].m_decrHits +=
                g_pWorkers[ii]->m_stats.m_slabStats[sid].m_decrHits;
            stats->m_slabStats[sid].m_incrHits +=
                g_pWorkers[ii]->m_stats.m_slabStats[sid].m_incrHits;
            stats->m_slabStats[sid].m_casHits +=
                g_pWorkers[ii]->m_stats.m_slabStats[sid].m_casHits;
            stats->m_slabStats[sid].m_casBadVal +=
                g_pWorkers[ii]->m_stats.m_slabStats[sid].m_casBadVal;
        }

        //pthread_mutex_unlock(&g_pWorkers[ii]->m_stats.mutex);
    }
}

void slab_stats_aggregate(CThreadStats *stats, CSlabStats *out) {
    int sid;

    out->m_setCmds = 0;
    out->m_getHits = 0;
    out->m_touchHits = 0;
    out->m_deleteHits = 0;
    out->m_incrHits = 0;
    out->m_decrHits = 0;
    out->m_casHits = 0;
    out->m_casBadVal = 0;

    for (sid = 0; sid < MAX_NUMBER_OF_SLAB_CLASSES; sid++) {
        out->m_setCmds += stats->m_slabStats[sid].m_setCmds;
        out->m_getHits += stats->m_slabStats[sid].m_getHits;
        out->m_touchHits += stats->m_slabStats[sid].m_touchHits;
        out->m_deleteHits += stats->m_slabStats[sid].m_deleteHits;
        out->m_decrHits += stats->m_slabStats[sid].m_decrHits;
        out->m_incrHits += stats->m_slabStats[sid].m_incrHits;
        out->m_casHits += stats->m_slabStats[sid].m_casHits;
        out->m_casBadVal += stats->m_slabStats[sid].m_casBadVal;
    }
}

/*
 * Initializes the thread subsystem, creating various worker threads.
 *
 * nthreads  Number of worker event handler threads to spawn
 * main_base Event base for main thread
 */
void thread_init(int nthreads, struct event_base *main_base) {
    int         i;
    int         power;

    pthread_mutex_init(&cache_lock, NULL);
    pthread_mutex_init(&stats_lock, NULL);

    pthread_mutex_init(&init_lock, NULL);
    pthread_cond_init(&init_cond, NULL);

    //pthread_mutex_init(&cqi_freelist_lock, NULL);
    //cqi_freelist = NULL;

    /* Want a wide lock table, but don't waste memory */
    if (nthreads < 3) {
        power = 10;
    } else if (nthreads < 4) {
        power = 11;
    } else if (nthreads < 5) {
        power = 12;
    } else {
        /* 8192 buckets, and central locks don't scale much past 5 threads */
        power = 13;
    }

    item_lock_count = hashsize(power);

    item_locks = (pthread_mutex_t *)calloc(item_lock_count, sizeof(pthread_mutex_t));
    if (! item_locks) {
        perror("Can't allocate item locks");
        exit(1);
    }
    for (i = 0; i < (int)item_lock_count; i++) {
        pthread_mutex_init(&item_locks[i], NULL);
    }
    pthread_key_create(&item_lock_type_key, NULL);
    pthread_mutex_init(&item_global_lock, NULL);

    dispatcher_thread.base = main_base;
    dispatcher_thread.thread_id = pthread_self();

	g_pWorkers = (CWorker **)calloc(nthreads, sizeof(CWorker *));

	if (! g_pWorkers) {
        perror("Can't allocate thread descriptors");
        exit(1);
    }

    /* Reserve three fds for the libevent base */
    g_stats.m_reservedFds += nthreads * 3;

	CWorker::InitCpuSets();

    /* Create threads after we've done all the libevent setup. */
    for (i = 0; i < nthreads; i++)
        CWorker::CreateThread(i);

    /* Wait for all the threads to set themselves up before returning. */
    pthread_mutex_lock(&init_lock);
    wait_for_thread_registration(nthreads);
    pthread_mutex_unlock(&init_lock);

#ifdef MCD_NUMA
	g_pHtHif->StartCoproc();
#endif
}

