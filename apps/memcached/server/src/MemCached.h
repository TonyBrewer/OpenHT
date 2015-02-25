/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/** \file
 * The main memcached header holding commonly used data
 * structures and function prototypes.
 */

#pragma once
#ifdef _WIN32
#pragma warning ( disable : 4200 )
#endif

/* some POSIX systems need the following definition
 * to get mlockall flags out of sys/mman.h.  */
#ifndef _P1003_1B_VISIBLE
#define _P1003_1B_VISIBLE
#endif

/* need this to get IOV_MAX on some platforms. */
#ifndef __need_IOV_MAX
#define __need_IOV_MAX
#endif

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/resource.h>
#include <sys/un.h>
#include <sys/uio.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sysexits.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <stddef.h>

#include <event.h>
#include <pthread.h>

#include "McdProtocol.h"

#include "Ht.h"
using namespace Ht;

#include "Timer.h"

#ifdef _WIN32
#define close_socket(fd) closesocket(fd)
#else
#define close_socket(fd) close(fd)
#endif

extern CHtHif * g_pHtHif;
extern CSample * g_pSamples[4];
extern CTimer * g_pTimers[32];
extern int      g_maxXmitLinkTimeCpuClks;
extern uint64_t g_maxRecvBufFillClks;

extern volatile bool g_bAllowNewConns;
extern struct event_base *g_pMainEventLoop;

#define MC_VERSION "Convey 1.4.15"
#define MC_PACKAGE "memcached"
#define ENDIAN_LITTLE 1

#ifndef _WIN32
#define HAVE_GCC_ATOMICS 1
#define HAVE_GETPAGESIZES 1
#define HAVE_MLOCKALL 1
#define HAVE_SIGIGNORE 1
#endif

/** Size of an incr buf. */
#define INCR_MAX_STORAGE_LEN 24

#define DATA_BUFFER_SIZE 2048
#define UDP_READ_BUFFER_SIZE 65536
#define UDP_MAX_PAYLOAD_SIZE 1400
#define UDP_HEADER_SIZE 8
#define MAX_SENDBUF_SIZE (256 * 1024 * 1024)
/* I'm told the max length of a 64-bit num converted to string is 20 bytes.
 * Plus a few for spaces, \r\n, \0 */
#define SUFFIX_SIZE 24

/** High water marks for buffer shrinking */
#define READ_BUFFER_HIGHWAT 8192
#define ITEM_LIST_HIGHWAT 400
#define IOV_LIST_HIGHWAT 600
#define MSG_LIST_HIGHWAT 100

/* Initial power multiplier for the hash table */
#define HASHPOWER_DEFAULT 16

/* Slab sizing definitions. */
#define POWER_SMALLEST 1
#define POWER_LARGEST  200
#define CHUNK_ALIGN_BYTES 8
#define MAX_NUMBER_OF_SLAB_CLASSES (POWER_LARGEST + 1)

/** How long an object can reasonably be assumed to be locked before
    harvesting it on a low memory condition. */
#define TAIL_REPAIR_TIME (3 * 3600)

/* warning: don't use these macros with a function, as it evals its arg twice */
#define ITEM_get_cas(i) (((i)->it_flags & ITEM_CAS) ? \
        (i)->data->cas : (uint64_t)0)

#define ITEM_set_cas(i,v) { \
    if ((i)->it_flags & ITEM_CAS) { \
        (i)->data->cas = v; \
    } \
}

#define ITEM_key(item) (((char*)&((item)->data)) \
         + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))

#define ITEM_suffix(item) ((char*) &((item)->data) + (item)->nkey + 1 \
         + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))

#define ITEM_data(item) ((char*) &((item)->data) + (item)->nkey + 1 \
         + (item)->nsuffix \
         + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))

#define ITEM_ntotal(item) (sizeof(CItem) + (item)->nkey + 1 \
         + (item)->nsuffix + (item)->nbytes \
         + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))

#define STAT_KEY_LEN 128
#define STAT_VAL_LEN 128

/** Append an indexed stat with a stat name (with format), value format
    and value */
#define APPEND_NUM_FMT_STAT(name_fmt, num, name, fmt, val)          \
    klen = snprintf(key_str, STAT_KEY_LEN, name_fmt, num, name);    \
    vlen = snprintf(val_str, STAT_VAL_LEN, fmt, val);               \
    AddStats(key_str, klen, val_str, vlen);

/** Common APPEND_NUM_FMT_STAT format. */
#define APPEND_NUM_STAT(num, name, fmt, val) \
    APPEND_NUM_FMT_STAT("%d:%s", num, name, fmt, val)


#define IS_UDP(x) (x == udp_transport)

enum EProtocol {
    ascii_prot = 3, /* arbitrary value. */
    binary_prot,
    negotiating_prot /* Discovering the protocol */
};

enum ETransport {
    local_transport, /* Unix sockets*/
    tcp_transport,
    udp_transport
};

#include "PersIntf.h"

#if defined(HT_MODEL) || defined(CHECK_USING_MODEL)
#include "../src_model/Unit.h"
#endif

#include "Thread.h"
#include "Conn.h"

enum item_lock_types {
    ITEM_LOCK_GRANULAR = 0,
    ITEM_LOCK_GLOBAL
};

/** Time relative to server start. Smaller than time_t on 64-bit systems. */
typedef unsigned int rel_time_t;

#include "Settings.h"

/**
 * Global stats.
 */
struct CStats {
	void Init();

public:
    pthread_mutex_t mutex;
    unsigned int  m_currItems;
    unsigned int  m_totalItems;
    uint64_t      m_currBytes;
    unsigned int  m_currConns;
    unsigned int  m_totalConns;
    uint64_t      m_rejectedConns;
    unsigned int  m_reservedFds;
    unsigned int  m_connStructs;
    bool          m_bAcceptingConns;  /* whether we are currently accepting */
    uint64_t      m_listenDisabledNum;
    unsigned int  m_hashPowerLevel; /* Better hope it's not over 9000 */
    uint64_t      m_hashBytes;       /* size used for hash tables */
    bool          m_bHashIsExpanding; /* If the hash table is being expanded */
    bool          m_bSlabReassignRunning; /* slab reassign in progress */
    uint64_t      m_slabsMoved;       /* times slabs were moved around */
};

extern CStats g_stats;
extern time_t g_processStartTime;

#define ITEM_LINKED 1
#define ITEM_CAS 2

/* temp */
#define ITEM_SLABBED 4

#define ITEM_FETCHED 8

/**
 * Structure for storing items within memcached.
 */
struct CItem {
    CItem *			next;
    CItem *			prev;
    CItem *			h_next;    /* hash chain next */
    rel_time_t      time;       /* least recent access */
    rel_time_t      exptime;    /* expire time */
    int             nbytes;     /* size of data */
    unsigned short  refcount;
    uint8_t         nsuffix;    /* length of flags-and-length string */
    uint8_t         it_flags;   /* ITEM_* above */
    uint8_t         slabs_clsid;/* which slab class we're in */
    uint8_t         nkey;       /* key length, w/terminating null and padding */
	uint32_t		hv;			/* hash value */
    /* this odd type prevents type-punning issues when we do
     * the little shuffle to save space when not using CAS. */
    union {
        uint64_t cas;
        char end;
    } data[];
    /* if it_flags & ITEM_CAS we have 8 bytes CAS */
    /* then null-terminated key */
    /* then " flags length\r\n" (no terminating null) */
    /* then data with terminating \r\n (no terminating null; it's binary!) */
};

typedef struct {
    pthread_t thread_id;        /* unique ID of this thread */
    struct event_base *base;    /* libevent handle this thread uses */
} LIBEVENT_DISPATCHER_THREAD;

/* current time of day (updated periodically) */
extern volatile rel_time_t current_time;

/* TODO: Move to slabs.h? */
extern volatile int slab_rebalance_signal;

struct slab_rebalance {
    void *slab_start;
    void *slab_end;
    void *slab_pos;
    int s_clsid;
    int d_clsid;
    int busy_items;
    uint8_t done;
};

extern struct slab_rebalance slab_rebal;

/*
 * Functions
 */
void DoAcceptNewConns(const bool bDoAccept);
CConn *NewConn(const int sfd, bool bListening, const int event_flags,
	const int read_buffer_size, ETransport transport, event_base * pBase, CWorker * pWorker);
extern int daemonize(int nochdir, int noclose);

static inline int mutex_lock(pthread_mutex_t *mutex)
{
    while (pthread_mutex_trylock(mutex));
    return 0;
}

#define mutex_unlock(x) pthread_mutex_unlock(x)

#include "Stats.h"
#include "Slabs.h"
#include "Assoc.h"
#include "Items.h"
#include "Hash.h"
#include "Util.h"

/*
 * Functions such as the libevent-related calls that need to do cross-thread
 * communication in multithreaded mode (rather than actually doing the work
 * in the current thread) are called via "dispatch_" frontends, which are
 * also #define-d to directly call the underlying code in singlethreaded mode.
 */

void thread_init(int nthreads, struct event_base *main_base);
void dispatch_conn_new(int sfd, bool bListening, int event_flags, int read_buffer_size, ETransport transport);

/* Lock wrappers for cache functions that are called from main loop. */
EDeltaResultType add_delta(CConn *c, const char *key, const size_t nkey, 
                                 const uint32_t hv, const bool incr,
                                 const uint64_t delta, char *buf,
                                 uint64_t *cas);
void AcceptNewConns(const bool do_accept);
int   is_listen_thread(void);
CItem *ItemAlloc(char *key, size_t nkey, uint32_t hv, int flags, rel_time_t exptime, int nbytes);
char *item_cachedump(const unsigned int slabs_clsid, const unsigned int limit, unsigned int *bytes);
void  item_flush_expired(void);
CItem *ItemGet(const char *key, const size_t nkey, const uint32_t hv);
CItem *ItemTouch(const char *key, const size_t nkey, const uint32_t hv, uint32_t exptime);
int   item_link(CItem *it);
void  item_remove(CItem *it);
int   item_replace(CItem *it, CItem *new_it);
void  item_stats(CConn *c);
void  item_stats_totals(CConn *c);
void  item_stats_sizes(CConn *c);
void  item_unlink(CItem *it);
void  item_update(CItem *it);

void item_lock_global(void);
void item_unlock_global(void);
void item_lock(uint32_t hv);
void *item_trylock(uint32_t hv);
void item_trylock_unlock(void *arg);
void item_unlock(uint32_t hv);
void switch_item_lock_type(enum item_lock_types type);
unsigned short refcount_incr(unsigned short *refcount);
unsigned short refcount_decr(unsigned short *refcount);
void STATS_LOCK(void);
void STATS_UNLOCK(void);
void threadlocal_stats_reset(void);
void stats_reset(void);

rel_time_t realtime(const time_t exptime);


EStoreItemType store_item(CItem *item, int comm, CConn *c);
const char *ProtocolText(EProtocol prot);

#if HAVE_DROP_PRIVILEGES
extern void drop_privileges(void);
#else
#define drop_privileges()
#endif

/* If supported, give compiler hints for branch prediction. */
#if !defined(__GNUC__) || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)
#define __builtin_expect(x, expected_value) (x)
#endif

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
