/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *  memcached - memory caching daemon
 *
 *       http://www.danga.com/memcached/
 *
 *  Copyright 2003 Danga Interactive, Inc.  All rights reserved.
 *
 *  Use and distribution licensed under the BSD license.  See
 *  the LICENSE file for full text.
 *
 *  Authors:
 *      Anatoly Vorobey <mellon@pobox.com>
 *      Brad Fitzpatrick <brad@danga.com>
 */
#include "MemCached.h"
#include "Timer.h"
#include "Powersave.h"
#ifdef ONLOAD
#include <onload/extensions.h>
#define     ONLOAD_STACK_NAME_MAX   8
bool		g_use_onload;
char        **g_onload_stack = NULL;
#endif

CTimerBase *    g_pTimerBase;
CTimer *        g_pTimers[32];
CSample *       g_pSamples[4];
int             g_maxXmitLinkTimeCpuClks;
uint64_t        g_maxRecvBufFillClks;
CHtHif *		g_pHtHif;
double          g_clocks_per_sec = 1.0;


/* FreeBSD 4.x doesn't have IOV_MAX exposed. */
#ifndef IOV_MAX
#if defined(__FreeBSD__) || defined(__APPLE__)
# define IOV_MAX 1024
#endif
#endif

/*
 * forward declarations
 */
static int new_socket(struct addrinfo *ai);

/** exported globals **/
CStats g_stats;
time_t g_processStartTime;     /* when the process was started */
FILE *tfp = 0;

struct slab_rebalance slab_rebal;
volatile int slab_rebalance_signal;

/** file scope variables **/
static CConn * g_pListenConnHead = NULL;
struct event_base *g_pMainEventLoop;

/* This reduces the latency without adding lots of extra wiring to be able to
 * notify the listener thread of when to listen again.
 * Also, the clock timer could be broken out into its own thread and we
 * can block the listener via a condition.
 */
volatile bool g_bAllowNewConns = true;
static struct event g_maxConnsEvent;
static void MaxConnsHandler(const int fd, const short which, void *arg) {
	struct timeval t;
	t.tv_sec = 0;
	t.tv_usec = 10000;

	if (fd == -42 || g_bAllowNewConns == false) {
		/* reschedule in 10ms if we need to keep polling */
		evtimer_set(&g_maxConnsEvent, MaxConnsHandler, 0);
		event_base_set(g_pMainEventLoop, &g_maxConnsEvent);
		evtimer_add(&g_maxConnsEvent, &t);
	} else {
		evtimer_del(&g_maxConnsEvent);
		AcceptNewConns(true);
	}
}

#define REALTIME_MAXDELTA 60*60*24*30

/*
 * given time value that's either unix time or delta from current unix time, return
 * unix time. Use the fact that delta can't exceed one month (and real time value can't
 * be that low).
 */
rel_time_t realtime(const time_t exptime) {
    /* no. of seconds in 30 days - largest possible delta exptime */

    if (exptime == 0) return 0; /* 0 means never expire */

    if (exptime > REALTIME_MAXDELTA) {
        /* if item expiration is at/before the server started, give it an
           expiration time of 1 second after the server started.
           (because 0 means don't expire).  without this, we'd
           underflow and wrap around to some large value way in the
           future, effectively making items expiring in the past
           really expiring never */
        if (exptime <= g_processStartTime)
            return (rel_time_t)1;
        return (rel_time_t)(exptime - g_processStartTime);
    } else {
        return (rel_time_t)(exptime + current_time);
    }
}

void CStats::Init() {
    m_currItems = m_totalItems = m_currConns = m_totalConns = m_connStructs = 0;
    m_rejectedConns = 0;
    m_currBytes = m_listenDisabledNum = 0;
    m_hashBytes = m_hashPowerLevel = 0;
    m_slabsMoved = 0;
	m_bHashIsExpanding = false;
    m_bAcceptingConns = true; /* assuming we start in this state. */
    m_bSlabReassignRunning = false;

    /* make the time we started always be 2 seconds before we really
       did, so time(0) - time.started is never zero.  if so, things
       like 'g_settings.oldest_live' which act as booleans as well as
       values are now false in boolean context... */
    g_processStartTime = time(0) - 2;
    stats_prefix_init();
}

void stats_reset(void) {
    STATS_LOCK();
    g_stats.m_totalItems = 0;
	g_stats.m_totalConns = 0;
    g_stats.m_rejectedConns = 0;
    g_stats.m_listenDisabledNum = 0;
    stats_prefix_clear();
    STATS_UNLOCK();
    threadlocal_stats_reset();
    item_stats_reset();
}

const char *ProtocolText(EProtocol prot) {
	char const *rv = "unknown";
	switch(prot) {
	case ascii_prot:
		rv = "ascii";
		break;
	case binary_prot:
		rv = "binary";
		break;
	case negotiating_prot:
		rv = "auto-negotiate";
		break;
	}
	return rv;
}

/*
 * Sets whether we are listening for new connections or not.
 */
void DoAcceptNewConns(const bool bDoAccept) {
    CConn *pConn;

    for (pConn = g_pListenConnHead; pConn; pConn = pConn->m_pNext) {
        if (bDoAccept) {
            pConn->UpdateEvent(EV_READ | EV_PERSIST);
            if (listen(pConn->m_sfd, g_settings.backlog) != 0) {
                perror("listen");
            }
        }
        else {
            pConn->UpdateEvent(0);
            if (listen(pConn->m_sfd, 0) != 0) {
                perror("listen");
            }
        }
    }

    if (bDoAccept) {
        g_stats.m_bAcceptingConns = true;
    } else {
        g_stats.m_bAcceptingConns = false;
        __sync_fetch_and_add(&g_stats.m_listenDisabledNum, 1);
        g_bAllowNewConns = false;
        MaxConnsHandler(-42, 0, 0);
    }
}

static int new_socket(struct addrinfo *ai) {
    int sfd;
    int flags;

    if ((sfd = (int)socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1) {
        return -1;
    }

    if ((flags = fcntl(sfd, F_GETFL, 0)) < 0 ||
        fcntl(sfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("setting O_NONBLOCK");
        close(sfd);
        return -1;
    }
    return sfd;
}


/*
 * Sets a socket's send buffer size to the maximum allowed by the system.
 */
static void maximize_sndbuf(const int m_sfd) {
    socklen_t intsize = sizeof(int);
    int last_good = 0;
    int min, max, avg;
    int old_size;

    /* Start with the default size. */
    if (getsockopt(m_sfd, SOL_SOCKET, SO_SNDBUF, (char *)&old_size, &intsize) != 0) {
        if (g_settings.m_verbose > 0)
            perror("getsockopt(SO_SNDBUF)");
        return;
    }

    /* Binary-search for the real maximum. */
    min = old_size;
    max = MAX_SENDBUF_SIZE;

    while (min <= max) {
        avg = ((unsigned int)(min + max)) / 2;
        if (setsockopt(m_sfd, SOL_SOCKET, SO_SNDBUF, (char *)&avg, intsize) == 0) {
            last_good = avg;
            min = avg + 1;
        } else {
            max = avg - 1;
        }
    }

    if (g_settings.m_verbose > 1)
        fprintf(stderr, "<%d send buffer was %d, now %d\n", m_sfd, old_size, last_good);
}

/**
 * Create a socket and bind it to a specific port number
 * @param interface the interface to bind to
 * @param port the port number to bind to
 * @param transport the transport protocol (TCP / UDP)
 * @param portnumber_file A filepointer to write the port numbers to
 *        when they are successfully added to the list of ports we
 *        listen on.
 */
static int server_socket(const char *interface_,
                         int port,
                         ETransport transport,
                         FILE *portnumber_file) {
    int m_sfd;
    struct linger ling = {0, 0};
    struct addrinfo *ai;
    struct addrinfo *next;
    char port_buf[NI_MAXSERV];
    int error;
    int success = 0;
    int flags =1;
	struct addrinfo hints = { 0 };
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;

	if (IS_UDP(transport)) {
        hints.ai_protocol = IPPROTO_UDP;
        hints.ai_socktype = SOCK_DGRAM;
        //hints.ai_family = AF_INET;
	} else {
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_socktype = SOCK_STREAM;
        //hints.ai_family = AF_UNSPEC;
	}

    if (port == -1) {
        port = 0;
    }
    snprintf(port_buf, sizeof(port_buf), "%d", port);
    error= getaddrinfo(interface_, port_buf, &hints, &ai);
    if (error != 0) {
        if (error != EAI_SYSTEM)
          fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(error));
        else
          perror("getaddrinfo()");
        return 1;
    }

    for (next= ai; next; next= next->ai_next) {
        CConn *listen_conn_add;
        if ((m_sfd = new_socket(next)) == -1) {
            /* getaddrinfo can return "junk" addresses,
             * we make sure at least one works before erroring.
             */
            if (errno == EMFILE) {
                /* ...unless we're out of fds */
                perror("server_socket");
                exit(EX_OSERR);
            }
            continue;
        }

#ifdef IPV6_V6ONLY
        if (next->ai_family == AF_INET6) {
            error = setsockopt(m_sfd, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &flags, sizeof(flags));
            if (error != 0) {
                perror("setsockopt");
                close(m_sfd);
                continue;
            }
        }
#endif

        setsockopt(m_sfd, SOL_SOCKET, SO_REUSEADDR, (char *)&flags, sizeof(flags));
        if (IS_UDP(transport)) {
            maximize_sndbuf(m_sfd);
        } else {
            error = setsockopt(m_sfd, SOL_SOCKET, SO_KEEPALIVE, (char *)&flags, sizeof(flags));
            if (error != 0)
                perror("setsockopt");

            error = setsockopt(m_sfd, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling));
            if (error != 0)
                perror("setsockopt");

            error = setsockopt(m_sfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flags, sizeof(flags));
            if (error != 0)
                perror("setsockopt");
        }

        if (bind(m_sfd, next->ai_addr, (int)next->ai_addrlen) == -1) {
            if (errno != EADDRINUSE) {
                perror("bind()");
                close(m_sfd);
                freeaddrinfo(ai);
                return 1;
            }
            close(m_sfd);
            continue;
        } else {
            success++;
            if (!IS_UDP(transport) && listen(m_sfd, g_settings.backlog) == -1) {
                perror("listen()");
                close(m_sfd);
                freeaddrinfo(ai);
                return 1;
            }
            if (portnumber_file != NULL &&
                (next->ai_addr->sa_family == AF_INET ||
                 next->ai_addr->sa_family == AF_INET6)) {
                union {
                    struct sockaddr_in in;
                    struct sockaddr_in6 in6;
                } my_sockaddr;
                socklen_t len = sizeof(my_sockaddr);
                if (getsockname(m_sfd, (struct sockaddr*)&my_sockaddr, &len)==0) {
                    if (next->ai_addr->sa_family == AF_INET) {
                        fprintf(portnumber_file, "%s INET: %u\n",
                                IS_UDP(transport) ? "UDP" : "TCP",
                                ntohs(my_sockaddr.in.sin_port));
                    } else {
                        fprintf(portnumber_file, "%s INET6: %u\n",
                                IS_UDP(transport) ? "UDP" : "TCP",
                                ntohs(my_sockaddr.in6.sin6_port));
                    }
                }
            }
        }

        if (IS_UDP(transport)) {
            int c;

            for (c = 0; c < g_settings.num_threads_per_udp; c++) {
                /* this is guaranteed to hit all threads because we round-robin */
                dispatch_conn_new(m_sfd, false, EV_READ | EV_PERSIST,
                                  UDP_READ_BUFFER_SIZE, transport);
            }
        } else {
            if (!(listen_conn_add = NewConn(m_sfd, true,
                                             EV_READ | EV_PERSIST, 1,
                                             transport, g_pMainEventLoop, 0))) {
                fprintf(stderr, "failed to create listening connection\n");
                exit(EXIT_FAILURE);
            }
            listen_conn_add->m_pNext = g_pListenConnHead;
            g_pListenConnHead = listen_conn_add;
        }
    }

    freeaddrinfo(ai);

    /* Return zero iff we detected no errors in starting up connections */
    return success == 0;
}

static int server_sockets(int port, ETransport transport,
                          FILE *portnumber_file) {
    if (g_settings.inter == NULL) {
        return server_socket(g_settings.inter, port, transport, portnumber_file);
    } else {
        // tokenize them and bind to each one of them..
        char *b;
        int ret = 0;
        char *list = strdup(g_settings.inter);
		char *p;

        if (list == NULL) {
            fprintf(stderr, "Failed to allocate memory for parsing server interface string\n");
            return 1;
        }

        for (p = strtok_r(list, ";,", &b);
             p != NULL;
             p = strtok_r(NULL, ";,", &b)) {

            int the_port = port;
            char *s = strchr(p, ':');
            if (s != NULL) {
                *s = '\0';
                ++s;
                if (!safe_strtol(s, &the_port)) {
                    fprintf(stderr, "Invalid port number: \"%s\"", s);
                    return 1;
                }
            }
            if (strcmp(p, "*") == 0) {
                p = NULL;
            }
            ret |= server_socket(p, the_port, transport, portnumber_file);
        }
        free(list);
        return ret;
    }
}

static int new_socket_unix(void) {
    int m_sfd;
    int flags;

    if ((m_sfd = (int)socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket()");
        return -1;
    }

    if ((flags = fcntl(m_sfd, F_GETFL, 0)) < 0 ||
        fcntl(m_sfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("setting O_NONBLOCK");
        close(m_sfd);
        return -1;
    }
    return m_sfd;
}

static int server_socket_unix(const char *path, int access_mask) {
    int m_sfd;
    struct linger ling = {0, 0};
    struct sockaddr_un addr;
    struct stat tstat;
    int flags =1;
    int old_umask;

    if (!path) {
        return 1;
    }

    if ((m_sfd = new_socket_unix()) == -1) {
        return 1;
    }

    /*
     * Clean up a previous socket file if we left it around
     */
    if (lstat(path, &tstat) == 0) {
        if (S_ISSOCK(tstat.st_mode))
            unlink(path);
    }

    setsockopt(m_sfd, SOL_SOCKET, SO_REUSEADDR, (char *)&flags, sizeof(flags));
    setsockopt(m_sfd, SOL_SOCKET, SO_KEEPALIVE, (char *)&flags, sizeof(flags));
    setsockopt(m_sfd, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling));

    /*
     * the memset call clears nonstandard fields in some impementations
     * that otherwise mess things up.
     */
    memset(&addr, 0, sizeof(addr));

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    assert(strcmp(addr.sun_path, path) == 0);
    old_umask = umask( ~(access_mask&0777));
    if (bind(m_sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind()");
        close(m_sfd);
        umask(old_umask);
        return 1;
    }
    umask(old_umask);
    if (listen(m_sfd, g_settings.backlog) == -1) {
        perror("listen()");
        close(m_sfd);
        return 1;
    }
    if (!(g_pListenConnHead = NewConn(m_sfd, true,
                                 EV_READ | EV_PERSIST, 1,
                                 local_transport, g_pMainEventLoop, 0))) {
        fprintf(stderr, "failed to create listening connection\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

/*
 * We keep the current time of day in a global variable that's updated by a
 * timer event. This saves us a bunch of time() system calls (we really only
 * need to get the time once a second, whereas there can be tens of thousands
 * of requests a second) and allows us to use server-start-relative timestamps
 * rather than absolute UNIX timestamps, a space savings on systems where
 * sizeof(time_t) > sizeof(unsigned int).
 */
volatile rel_time_t current_time;
static struct event clockevent;

/* libevent uses a monotonic clock when available for event scheduling. Aside
 * from jitter, simply ticking our internal timer here is accurate enough.
 * Note that users who are setting explicit dates for expiration times *must*
 * ensure their clocks are correct before starting memcached. */
static void clock_handler(const int fd, const short which, void *arg) {
    static bool initialized = false;
#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
    static bool monotonic = false;
    static time_t monotonic_start;
#endif
    struct timeval t;
	t.tv_sec = 1;
	t.tv_usec = 0;

    if (initialized) {
        /* only delete the event if it's actually there. */
        evtimer_del(&clockevent);
    } else {
        initialized = true;
        /* g_processStartTime is initialized to time() - 2. We initialize to 1 so
         * flush_all won't underflow during tests. */
#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
        struct timespec ts;
        if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
            monotonic = true;
            monotonic_start = ts.tv_sec - 2;
        }
#endif
    }

    evtimer_set(&clockevent, clock_handler, 0);
    event_base_set(g_pMainEventLoop, &clockevent);
    evtimer_add(&clockevent, &t);

#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
    if (monotonic) {
        struct timespec ts;
        if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
            return;
        current_time = (rel_time_t) (ts.tv_sec - monotonic_start);
        return;
    }
#endif
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        current_time = (rel_time_t) (time(0) - g_processStartTime);
    }
}

static void save_pid(const char *pid_file) {
    FILE *fp;
    if (access(pid_file, F_OK) == 0) {
        if ((fp = fopen(pid_file, "r")) != NULL) {
            char buffer[1024];
            if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                unsigned int pid;
                if (safe_strtoul(buffer, &pid) && kill((pid_t)pid, 0) == 0) {
                    fprintf(stderr, "WARNING: The pid file contained the following (running) pid: %u\n", pid);
                }
            }
            fclose(fp);
        }
    }

    if ((fp = fopen(pid_file, "w")) == NULL) {
        vperror("Could not open the pid file %s for writing", pid_file);
        return;
    }

    fprintf(fp,"%ld\n", (long)getpid());
    if (fclose(fp) == -1) {
        vperror("Could not close the pid file %s", pid_file);
    }
}

static void remove_pidfile(const char *pid_file) {
  if (pid_file == NULL)
      return;

  if (unlink(pid_file) != 0) {
      vperror("Could not remove the pid file %s", pid_file);
  }

}

static void sig_handler(const int sig) {
    printf("SIGINT handled.\n");
    exit(EXIT_SUCCESS);
}

#ifndef HAVE_SIGIGNORE
int sigignore(int sig) {
    struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;

    if (sigemptyset(&sa.sa_mask) == -1 || sigaction(sig, &sa, 0) == -1) {
        return -1;
    }
    return 0;
}
#endif

/**
 * Do basic sanity check of the runtime environment
 * @return true if no errors found, false if we can't use this env
 */
static bool sanitycheck(void) {
    /* One of our biggest problems is old and bogus libevents */
    const char *ever = event_get_version();
    if (ever != NULL) {
        if (strncmp(ever, "1.", 2) == 0) {
            /* Require at least 1.3 (that's still a couple of years old) */
            if ((ever[2] == '1' || ever[2] == '2') && !isdigit(ever[3])) {
                fprintf(stderr, "You are using libevent %s.\nPlease upgrade to"
                        " a more recent version (1.3 or newer)\n",
                        event_get_version());
                return false;
            }
        }
    }

    return true;
}

#ifdef _WIN32
void windows_socket_init() {
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,0), &wsaData) != 0) {
        fprintf(stderr, "Socket Initialization Error. Program  aborted\n");
        return;
    }
}
#endif

int main (int argc, char **argv) {
    struct passwd *pw;
    struct rlimit rlim;
    int retval = EXIT_SUCCESS;

    /* listening sockets */
    static int *l_socket = NULL;

    /* udp socket */
    static int *u_socket = NULL;

	#ifdef _WIN32
	// initialize windows sockets
	windows_socket_init();
	#endif

    if (!sanitycheck()) {
        return EX_OSERR;
    }

    /* handle SIGINT */
    signal(SIGINT, sig_handler);

    /* init settings */
	g_settings.Init(argc, argv);

    /* set stderr non-buffering (for running under, say, daemontools) */
    setbuf(stderr, NULL);

    if (g_settings.maxcore != 0) {
        struct rlimit rlim_new;
        /*
         * First try raising to infinity; if that fails, try bringing
         * the soft limit to the hard.
         */
        if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
            rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
            if (setrlimit(RLIMIT_CORE, &rlim_new)!= 0) {
                /* failed. try raising just to the old max */
                rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
                (void)setrlimit(RLIMIT_CORE, &rlim_new);
            }
        }
        /*
         * getrlimit again to see what we ended up with. Only fail if
         * the soft limit ends up 0, because then no core files will be
         * created at all.
         */

        if ((getrlimit(RLIMIT_CORE, &rlim) != 0) || rlim.rlim_cur == 0) {
            fprintf(stderr, "failed to ensure corefile creation\n");
            exit(EX_OSERR);
        }
    }

    /*
     * If needed, increase rlimits to allow as many connections
     * as needed.
     */

    if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) {
        fprintf(stderr, "failed to getrlimit number of files\n");
        exit(EX_OSERR);
    } else {
        rlim.rlim_cur = g_settings.m_maxConns;
        rlim.rlim_max = g_settings.m_maxConns;
        if (setrlimit(RLIMIT_NOFILE, &rlim) != 0) {
            fprintf(stderr, "failed to set rlimit for open files. Try starting as root or requesting smaller maxconns value.\n");
            exit(EX_OSERR);
        }
    }

    /* lose root privileges if we have them */
    if (getuid() == 0 || geteuid() == 0) {
        if (g_settings.username == 0 || *g_settings.username == '\0') {
            fprintf(stderr, "can't run as root without the -u switch\n");
            exit(EX_USAGE);
        }
        if ((pw = getpwnam(g_settings.username)) == 0) {
            fprintf(stderr, "can't find the user %s to switch to\n", g_settings.username);
            exit(EX_NOUSER);
        }
        if (setgid(pw->pw_gid) < 0 || setuid(pw->pw_uid) < 0) {
            fprintf(stderr, "failed to assume identity of user %s\n", g_settings.username);
            exit(EX_OSERR);
        }
    }

    /* daemonize if requested */
    /* if we want to ensure our ability to dump core, don't chdir to / */
    if (g_settings.do_daemonize) {
        if (sigignore(SIGHUP) == -1) {
            perror("Failed to ignore SIGHUP");
        }
        if (daemonize(g_settings.maxcore, g_settings.m_verbose) == -1) {
            fprintf(stderr, "failed to daemon() in order to daemonize\n");
            exit(EXIT_FAILURE);
        }
    }

    /* lock paged memory if needed */
    if (g_settings.lock_memory) {
#ifdef HAVE_MLOCKALL
        int res = mlockall(MCL_CURRENT | MCL_FUTURE);
        if (res != 0) {
            fprintf(stderr, "warning: -k invalid, mlockall() failed: %s\n",
                    strerror(errno));
        }
#else
        fprintf(stderr, "warning: -k invalid, mlockall() not supported on this platform.  proceeding without.\n");
#endif
    }

    if (g_settings.powersave) {
        uint64_t sc, ec;
        time_t time_start, time_now, time_passed;
        struct timeval st;

        // Calculate clock rate
        gettimeofday(&st, NULL);
        time_now = time_start = st.tv_sec * 1000000L + (time_t)st.tv_usec;
        time_passed = 0L;

        sc = CTimer::Rdtsc();

        while (time_passed < 200000L) {
            gettimeofday(&st, NULL);
            time_now = st.tv_sec * 1000000L + (time_t)st.tv_usec;
            time_passed = time_now - time_start;
        }
    
        ec = CTimer::Rdtsc();

        g_clocks_per_sec = ((double)(ec - sc) / (double)time_passed) * 1000000.0;
        if (g_clocks_per_sec < 1.0) {
            g_clocks_per_sec = 1.0;
        }

        //fprintf(stderr,"clocks_per_sec = %lf\n", g_clocks_per_sec);
        PowersaveInit();
    }
    
#ifdef ONLOAD
    g_use_onload = onload_is_present();
    //fprintf(stderr, "Onload %s detected.\n", g_use_onload ? "environment" : "not");

    // Pre-generate stack names. Onload will instantiate each stack on first use.
    if (g_use_onload && g_settings.onload_stacks > 0) {
        int i;
        
        g_onload_stack = (char **)calloc(g_settings.onload_stacks, sizeof(char *));
        if (! g_onload_stack) {
            fprintf(stderr, "error: unable to allocate memory\n");
            exit(EXIT_FAILURE);
        }

        for (i=0; i<g_settings.onload_stacks; i++) {
            g_onload_stack[i] = (char *)calloc(ONLOAD_STACK_NAME_MAX, sizeof(char));
            if (! g_onload_stack[i]) {
            fprintf(stderr, "error: unable to allocate memory\n");
                exit(EXIT_FAILURE);
            }
            sprintf(g_onload_stack[i], "mcd%02d", i);
            //fprintf(stderr, "Onload stack: %s\n", g_onload_stack[i]);
        }
    }
#endif

    /* initialize main thread libevent instance */
    g_pMainEventLoop = event_init();

    /* initialize other stuff */
    g_stats.Init();

    assoc_init(g_settings.hashpower_init);
    slabs_init(g_settings.maxbytes, g_settings.factor, g_settings.preallocate);

    /*
     * ignore SIGPIPE signals; we can use errno == EPIPE if we
     * need that information
     */
    if (sigignore(SIGPIPE) == -1) {
        perror("failed to ignore SIGPIPE; sigaction");
        exit(EX_OSERR);
    }

#ifdef TRACE
	if (!(tfp = fopen("TraceFile.txt", "w"))) {
		fprintf(stderr, "Could not open TraceFile.txt for writing\n");
		exit(-1);
	}
#endif

    g_pTimerBase = new CTimerBase(5);
    g_maxXmitLinkTimeCpuClks = (int)(g_settings.m_maxXmitLinkTimeUsec * g_pTimerBase->GetCpuClksPerUsec());
    g_maxRecvBufFillClks = (uint64_t)(g_pTimerBase->GetCpuClksPerUsec() * 5.0);

    g_pTimers[6] = g_pTimerBase->NewTimer("RecvPkt()", g_settings.num_threads);
    g_pSamples[1] = g_pTimerBase->NewSample("  Recv Full Cnt", g_settings.num_threads);
    g_pTimers[4] = g_pTimerBase->NewTimer("  Coproc Resp Time", g_settings.num_threads);
    g_pTimers[5] = g_pTimerBase->NewTimer("ProcessRespBuf()", g_settings.num_threads);
    g_pTimers[7] = g_pTimerBase->NewTimer("  GetHifRspCmd(qwCnt)", g_settings.num_threads);
    g_pTimers[8] = g_pTimerBase->NewTimer("  ProcessCmdBinGet()", g_settings.num_threads);
    g_pTimers[9] = g_pTimerBase->NewTimer("    ItemGet()", g_settings.num_threads);
    g_pTimers[10] = g_pTimerBase->NewTimer("    ItemGet() hit", g_settings.num_threads);
    g_pTimers[11] = g_pTimerBase->NewTimer("Transmit()", g_settings.num_threads);
    g_pTimers[12] = g_pTimerBase->NewTimer("  sendmsg()", g_settings.num_threads);
    g_pSamples[0] = g_pTimerBase->NewSample("  Xmit Accum Cnt", g_settings.num_threads);
    g_pSamples[2] = g_pTimerBase->NewSample("  Free Conn Cnt", g_settings.num_threads);

    // dispatch coprocessor
	CHtHifParams hifParams;
	hifParams.m_appUnitMemSize = MCD_RECV_BUF_SIZE;
	hifParams.m_iBlkTimerUSec = 0;
	hifParams.m_ctlTimerUSec = 1;
	hifParams.m_oBlkTimerUSec = 4;
#ifdef MCD_NUMA
	hifParams.m_numaSetCnt = 2;
#endif

    g_pHtHif = new CHtHif(&hifParams);

	int unitCnt = g_pHtHif->GetUnitCnt();

	if (g_settings.num_threads > unitCnt) {
		fprintf(stderr, "ERROR - the specified thread count is greater than the number of personality units\n");
		fprintf(stderr, "  Use -t command line option to specify the thread count, maximum value of %d\n",
            unitCnt);
		exit(1);
	}

    /* start up worker threads if MT mode */
    thread_init(g_settings.num_threads, g_pMainEventLoop);

    if (start_assoc_maintenance_thread() == -1) {
        exit(EXIT_FAILURE);
    }

    if (g_settings.slab_reassign &&
        start_slab_maintenance_thread() == -1) {
        exit(EXIT_FAILURE);
    }

    /* initialise clock event */
    clock_handler(0, 0, 0);

    /* create unix mode sockets after dropping privileges */
    if (g_settings.socketpath != NULL) {
        //errno = 0;
        if (server_socket_unix(g_settings.socketpath,g_settings.access)) {
            vperror("failed to listen on UNIX socket: %s", g_settings.socketpath);
            exit(EX_OSERR);
        }
    }

    /* create the listening socket, bind it, and init */
    if (g_settings.socketpath == NULL) {
        const char *portnumber_filename = getenv("MEMCACHED_PORT_FILENAME");
        char temp_portnumber_filename[PATH_MAX];
        FILE *portnumber_file = NULL;

        if (portnumber_filename != NULL) {
            snprintf(temp_portnumber_filename,
                     sizeof(temp_portnumber_filename),
                     "%s.lck", portnumber_filename);

            portnumber_file = fopen(temp_portnumber_filename, "a");
            if (portnumber_file == NULL) {
                fprintf(stderr, "Failed to open \"%s\": %s\n",
                        temp_portnumber_filename, strerror(errno));
            }
        }

        //errno = 0;
        if (g_settings.port && server_sockets(g_settings.port, tcp_transport,
                                           portnumber_file)) {
            vperror("failed to listen on TCP port %d", g_settings.port);
            exit(EX_OSERR);
        }

        /*
         * initialization order: first create the listening sockets
         * (may need root on low ports), then drop root if needed,
         * then daemonise if needed, then init libevent (in some cases
         * descriptors created by libevent wouldn't survive forking).
         */

        /* create the UDP listening socket and bind it */
        //errno = 0;
        if (g_settings.udpport && server_sockets(g_settings.udpport, udp_transport,
                                              portnumber_file)) {
            vperror("failed to listen on UDP port %d", g_settings.udpport);
            exit(EX_OSERR);
        }

        if (portnumber_file) {
            fclose(portnumber_file);
            rename(temp_portnumber_filename, portnumber_filename);
        }
    }

    /* Give the sockets a moment to open. I know this is dumb, but the error
     * is only an advisory.
     */
    usleep(1000);
    if (g_stats.m_currConns + g_stats.m_reservedFds >= (uint32_t)g_settings.m_maxConns - 1) {
        fprintf(stderr, "Maxconns setting is too low, use -c to increase.\n");
        exit(EXIT_FAILURE);
    }

    if (g_settings.pid_file != NULL) {
        save_pid(g_settings.pid_file);
    }

    /* Drop privileges no longer needed */
    drop_privileges();

    /* enter the event loop */
    if (event_base_loop(g_pMainEventLoop, 0) != 0) {
        retval = EXIT_FAILURE;
    }

    stop_assoc_maintenance_thread();

    /* remove the PID file if we're a daemon */
    if (g_settings.do_daemonize)
        remove_pidfile(g_settings.pid_file);
    /* Clean up strdup() call for bind() address */
    if (g_settings.inter)
      free(g_settings.inter);
    if (l_socket)
      free(l_socket);
    if (u_socket)
      free(u_socket);

    return retval;
}
