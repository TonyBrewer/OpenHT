/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#define MAX_VERBOSITY_LEVEL 2

/* When adding a setting, be sure to update process_stat_settings */
/**
 * Globally accessible settings as derived from the commandline.
 */
struct CSettings {

	void Init(int argc, char **argv);
	void Usage();
	void UsageLicense();
	int EnableLargePages();

public:
	// flags used in main
    bool tcp_specified;
    bool udp_specified;
    bool lock_memory;
    bool do_daemonize;
    bool preallocate;
    int maxcore;
    char *username;
    char *pid_file;
    int size_max;
    char unit;
    bool protocol_specified;

public:
    int         m_maxXmitLinkTimeUsec;  // maximum time packet is linked for transmission
    size_t maxbytes;
    int			m_maxConns;
    int port;
    int udpport;
    char *inter;
    int		m_verbose;
	int		m_cnyVerbose;
    rel_time_t oldest_live; /* ignore existing items older than this */
    int evict_to_free;
    char *socketpath;   /* path to unix socket if using local socket */
    int access;  /* access mask (a la chmod) for unix domain socket */
    double factor;          /* chunk size growth factor */
    int chunk_size;
    int num_threads;        /* number of worker (without dispatcher) libevent threads to run */
    int num_threads_per_udp; /* number of worker threads serving each udp socket */
    char prefix_delimiter;  /* character that marks a key prefix (for stats) */
    int detail_enabled;     /* nonzero if we're collecting detailed stats */
    int reqs_per_event;     /* Maximum number of io to process on each
                               io-event. */
    bool use_cas;
    EProtocol binding_protocol;
    int backlog;
    int item_size_max;        /* Maximum item size, and upper end for slabs */
    bool maxconns_fast;     /* Whether or not to early close connections */
    bool slab_reassign;     /* Whether or not slab reassignment is allowed */
    int slab_automove;     /* Whether or not to automatically move slabs */
    int hashpower_init;     /* Starting hash power level */

#ifdef ONLOAD
    int onload_stacks; 		/* Number of Onload stacks to allocate. */
#endif

    bool powersave;
    int powersave_preset;
};
extern CSettings g_settings;
