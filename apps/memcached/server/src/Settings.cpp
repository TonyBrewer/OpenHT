#include "MemCached.h"

CSettings g_settings;

void CSettings::Init(int argc, char **argv)
{
    g_settings.use_cas = true;
    g_settings.access = 0700;
    g_settings.port = 11211;
    g_settings.udpport = 11211;
    /* By default this string should be NULL for getaddrinfo() */
    g_settings.inter = NULL;
    g_settings.maxbytes = 64 * 1024 * 1024; /* default is 64MB */
    g_settings.m_maxConns = 1024;         /* to limit connections-related memory to about 5MB */
    g_settings.m_verbose = 0;
	g_settings.m_cnyVerbose = 0;
    g_settings.oldest_live = 0;
    g_settings.evict_to_free = 1;       /* push old items out of cache when memory runs out */
    g_settings.socketpath = NULL;       /* by default, not using a unix socket */
    g_settings.factor = 1.25;
    g_settings.chunk_size = 48;         /* space for a modest key and value */
    g_settings.num_threads = 4;         /* N workers */
    g_settings.num_threads_per_udp = 0;
    g_settings.prefix_delimiter = ':';
    g_settings.detail_enabled = 0;
    g_settings.reqs_per_event = 20;
    g_settings.backlog = 1024;
    g_settings.binding_protocol = negotiating_prot;
    g_settings.item_size_max = 1024 * 1024; /* The famous 1MB upper limit. */
    g_settings.maxconns_fast = false;
    g_settings.hashpower_init = 0;
    g_settings.slab_reassign = false;
    g_settings.slab_automove = 0;
    g_settings.m_maxXmitLinkTimeUsec = 1;
    g_settings.powersave = false;	/* reduce CPU usage to save power when possible */
#ifdef ONLOAD
    g_settings.onload_stacks = 16;
#endif

    enum {
        MAXCONNS_FAST = 0,
        HASHPOWER_INIT,
        SLAB_REASSIGN,
        SLAB_AUTOMOVE,
#ifdef ONLOAD
	ONLOAD_STACKS,
#endif
        POWERSAVE
    };
    char * const subopts_tokens[] = {
        (char *)"maxconns_fast",
        (char *)"hashpower",
        (char *)"slab_reassign",
        (char *)"slab_automove",
#ifdef ONLOAD
	(char *)"onload_stacks",
#endif
        (char *)"powersave",
        NULL
    };

    char *subopts;
    char *subopts_value;
    int c;

    tcp_specified = false;
    udp_specified = false;
    lock_memory = false;
    do_daemonize = false;
    preallocate = false;
    maxcore = 0;
    username = NULL;
    pid_file = NULL;
    size_max = 0;
    unit = '\0';
    protocol_specified = false;

    /* process arguments */
    while (-1 != (c = getopt(argc, argv,
          "a:"  /* access mask for unix socket */
          "p:"  /* TCP port number to listen on */
          "s:"  /* unix socket path to listen on */
          "U:"  /* UDP port number to listen on */
          "m:"  /* max memory to use for items in megabytes */
          "M"   /* return error on memory exhausted */
          "c:"  /* max simultaneous connections */
          "k"   /* lock down all paged memory */
          "hi"  /* help, licence info */
          "r"   /* maximize core file limit */
          "v"   /* verbose */
          "V"   /* cny verbose */
          "d"   /* daemon mode */
          "l:"  /* interface to listen on */
          "u:"  /* user identity to run as */
          "P:"  /* save PID in file */
          "f:"  /* factor? */
          "n:"  /* minimum space allocated for key+value+flags */
          "t:"  /* threads */
          "D:"  /* prefix delimiter? */
          "L"   /* Large memory pages */
          "R:"  /* max requests per event */
          "C"   /* Disable use of CAS */
          "b:"  /* backlog queue limit */
          "B:"  /* Binding protocol */
          "I:"  /* Max item size */
          "S"   /* Sasl ON */
          "o:"  /* Extended generic options */
        ))) {
        switch (c) {
        case 'a':
            /* access for unix domain socket, as octal mask (like chmod)*/
            g_settings.access= strtol(optarg,NULL,8);
            break;
        case 'U':
            g_settings.udpport = atoi(optarg);
            udp_specified = true;
            break;
        case 'p':
            g_settings.port = atoi(optarg);
            tcp_specified = true;
            break;
        case 's':
            g_settings.socketpath = optarg;
            break;
        case 'm':
            g_settings.maxbytes = ((size_t)atoi(optarg)) * 1024 * 1024;
            break;
        case 'M':
            g_settings.evict_to_free = 0;
            break;
        case 'c':
            g_settings.m_maxConns = atoi(optarg);
            break;
        case 'h':
            Usage();
            exit(EXIT_SUCCESS);
        case 'i':
            UsageLicense();
            exit(EXIT_SUCCESS);
        case 'k':
            lock_memory = true;
            break;
        case 'v':
            g_settings.m_verbose++;
            break;
        case 'V':
            g_settings.m_cnyVerbose++;
            break;
        case 'l':
            if (g_settings.inter != NULL) {
                size_t len = strlen(g_settings.inter) + strlen(optarg) + 2;
                char *p = (char *)malloc(len);
                if (p == NULL) {
                    fprintf(stderr, "Failed to allocate memory\n");
                    exit(EX_USAGE);
                }
                snprintf(p, len, "%s,%s", g_settings.inter, optarg);
                free(g_settings.inter);
                g_settings.inter = p;
            } else {
                g_settings.inter= strdup(optarg);
            }
            break;
        case 'd':
            do_daemonize = true;
            break;
        case 'r':
            maxcore = 1;
            break;
        case 'R':
            g_settings.reqs_per_event = atoi(optarg);
            if (g_settings.reqs_per_event == 0) {
                fprintf(stderr, "Number of requests per event must be greater than 0\n");
                exit(EX_USAGE);
            }
            break;
        case 'u':
            username = optarg;
            break;
        case 'P':
            pid_file = optarg;
            break;
        case 'f':
            g_settings.factor = atof(optarg);
            if (g_settings.factor <= 1.0) {
                fprintf(stderr, "Factor must be greater than 1\n");
                exit(EX_USAGE);
            }
            break;
        case 'n':
            g_settings.chunk_size = atoi(optarg);
            if (g_settings.chunk_size == 0) {
                fprintf(stderr, "Chunk size must be greater than 0\n");
                exit(EX_USAGE);
            }
            break;
        case 't':
            g_settings.num_threads = atoi(optarg);
            if (g_settings.num_threads <= 0) {
                fprintf(stderr, "Number of threads must be greater than 0\n");
                exit(EX_USAGE);
            }
            /* There're other problems when you get above 64 threads.
             * In the future we should portably detect # of cores for the
             * default.
             */
            if (g_settings.num_threads > 64) {
                fprintf(stderr, "WARNING: Setting a high number of worker"
                                "threads is not recommended.\n"
                                " Set this value to the number of cores in"
                                " your machine or less.\n");
            }
            break;
        case 'D':
            if (! optarg || ! optarg[0]) {
                fprintf(stderr, "No delimiter specified\n");
                exit(EX_USAGE);
            }
            g_settings.prefix_delimiter = optarg[0];
            g_settings.detail_enabled = 1;
            break;
        case 'L' :
            if (EnableLargePages() == 0) {
                preallocate = true;
            } else {
                fprintf(stderr, "Cannot enable large pages on this system\n"
                    "(There is no Linux support as of this version)\n");
                exit(EX_USAGE);
            }
            break;
        case 'C' :
            g_settings.use_cas = false;
            break;
        case 'b' :
            g_settings.backlog = atoi(optarg);
            break;
        case 'B':
            protocol_specified = true;
            if (strcmp(optarg, "auto") == 0) {
                g_settings.binding_protocol = negotiating_prot;
            } else if (strcmp(optarg, "binary") == 0) {
                g_settings.binding_protocol = binary_prot;
            } else if (strcmp(optarg, "ascii") == 0) {
                g_settings.binding_protocol = ascii_prot;
            } else {
                fprintf(stderr, "Invalid value for binding protocol: %s\n"
                        " -- should be one of auto, binary, or ascii\n", optarg);
                exit(EX_USAGE);
            }
            break;
        case 'I':
            unit = optarg[strlen(optarg)-1];
            if (unit == 'k' || unit == 'm' ||
                unit == 'K' || unit == 'M') {
                optarg[strlen(optarg)-1] = '\0';
                size_max = atoi(optarg);
                if (unit == 'k' || unit == 'K')
                    size_max *= 1024;
                if (unit == 'm' || unit == 'M')
                    size_max *= 1024 * 1024;
                g_settings.item_size_max = size_max;
            } else {
                g_settings.item_size_max = atoi(optarg);
            }
            if (g_settings.item_size_max < 1024) {
                fprintf(stderr, "Item max size cannot be less than 1024 bytes.\n");
                exit(EX_USAGE);
            }
            if (g_settings.item_size_max > 1024 * 1024 * 128) {
                fprintf(stderr, "Cannot set item size limit higher than 128 mb.\n");
                exit(EX_USAGE);
            }
            if (g_settings.item_size_max > 1024 * 1024) {
                fprintf(stderr, "WARNING: Setting item max size above 1MB is not"
                    " recommended!\n"
                    " Raising this limit increases the minimum memory requirements\n"
                    " and will decrease your memory efficiency.\n"
                );
            }
            break;
        case 'S': /* set Sasl authentication to true. Default is false */
            fprintf(stderr, "This server is not built with SASL support.\n");
            exit(EX_USAGE);
            break;
	case 'o': /* It's sub-opts time! */
            subopts = optarg;

            while (*subopts != '\0') {

            switch (getsubopt(&subopts, subopts_tokens, &subopts_value)) {
            case MAXCONNS_FAST:
                g_settings.maxconns_fast = true;
                break;
            case HASHPOWER_INIT:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing numeric argument for hashpower\n");
                    exit(EX_USAGE);
                }
                g_settings.hashpower_init = atoi(subopts_value);
                if (g_settings.hashpower_init < 12) {
                    fprintf(stderr, "Initial hashtable multiplier of %d is too low\n",
                        g_settings.hashpower_init);
                    exit(EX_USAGE);
                } else if (g_settings.hashpower_init > 64) {
                    fprintf(stderr, "Initial hashtable multiplier of %d is too high\n"
                        "Choose a value based on \"STAT hash_power_level\" from a running instance\n",
                        g_settings.hashpower_init);
                    exit(EX_USAGE);
                }
                break;
            case SLAB_REASSIGN:
                g_settings.slab_reassign = true;
                break;
            case SLAB_AUTOMOVE:
                if (subopts_value == NULL) {
                    g_settings.slab_automove = 1;
                    break;
                }
                g_settings.slab_automove = atoi(subopts_value);
                if (g_settings.slab_automove < 0 || g_settings.slab_automove > 2) {
                    fprintf(stderr, "slab_automove must be between 0 and 2\n");
                    exit(EX_USAGE);
                }
                break;

#ifdef ONLOAD
	    case ONLOAD_STACKS:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing numeric argument for onload_stacks\n");
                    exit(EX_USAGE);
                }
		g_settings.onload_stacks = atoi(subopts_value);
		if (g_settings.onload_stacks < 0) {
		    fprintf(stderr, "Onload stack count of %d is too low\n",
			    g_settings.onload_stacks);
		    exit(EX_USAGE);
		}
		break;
#endif
	    case POWERSAVE:
	        g_settings.powersave = true;
		if (subopts_value != NULL) {
		    g_settings.powersave_preset = atoi(subopts_value);
		    if (g_settings.powersave_preset < 0 || g_settings.powersave_preset > 3) {
			fprintf(stderr, "Option to powersave (%d) out of range (0-3 allowed)\n",
				g_settings.powersave_preset);
			exit(EX_USAGE);
		    }
		    if (g_settings.powersave_preset == 0) g_settings.powersave = false;
		} else {
		    g_settings.powersave_preset = 2;
		}
		break;

            default:
                printf("Illegal suboption \"%s\"\n", subopts_value);
                exit(EX_USAGE);
            }

            }
            break;
        default:
            fprintf(stderr, "Illegal argument \"%c\"\n", c);
            exit(EX_USAGE);
        }
    }


    /*
     * Use one workerthread to serve each UDP port if the user specified
     * multiple ports
     */
    if (g_settings.inter != NULL && strchr(g_settings.inter, ',')) {
        g_settings.num_threads_per_udp = 1;
    } else {
        g_settings.num_threads_per_udp = g_settings.num_threads;
    }

    if (tcp_specified && !udp_specified) {
        g_settings.udpport = g_settings.port;
    } else if (udp_specified && !tcp_specified) {
        g_settings.port = g_settings.udpport;
    }
}

void CSettings::Usage() {
    printf(MC_PACKAGE " " MC_VERSION "\n");
    printf("-p <num>      TCP port number to listen on (default: 11211)\n"
           "-U <num>      UDP port number to listen on (default: 11211, 0 is off)\n"
           "-s <file>     UNIX socket path to listen on (disables network support)\n"
           "-a <mask>     access mask for UNIX socket, in octal (default: 0700)\n"
           "-l <addr>     interface to listen on (default: INADDR_ANY, all addresses)\n"
           "              <addr> may be specified as host:port. If you don't specify\n"
           "              a port number, the value you specified with -p or -U is\n"
           "              used. You may specify multiple addresses separated by comma\n"
           "              or by using -l multiple times\n"

           "-d            run as a daemon\n"
           "-r            maximize core file limit\n"
           "-u <username> assume identity of <username> (only when run as root)\n"
           "-m <num>      max memory to use for items in megabytes (default: 64 MB)\n"
           "-M            return error on memory exhausted (rather than removing items)\n"
           "-c <num>      max simultaneous connections (default: 1024)\n"
           "-k            lock down all paged memory.  Note that there is a\n"
           "              limit on how much memory you may lock.  Trying to\n"
           "              allocate more than that would fail, so be sure you\n"
           "              set the limit correctly for the user you started\n"
           "              the daemon with (not for -u <username> user;\n"
           "              under sh this is done with 'ulimit -S -l NUM_KB').\n"
           "-v            verbose (print errors/warnings while in event loop)\n"
           "-vv           very verbose (also print client commands/reponses)\n"
           "-vvv          extremely verbose (also print internal state transitions)\n"
           "-h            print this help and exit\n"
           "-i            print memcached and libevent license\n"
           "-P <file>     save PID in <file>, only used with -d option\n"
           "-f <factor>   chunk size growth factor (default: 1.25)\n"
           "-n <bytes>    minimum space allocated for key+value+flags (default: 48)\n");
    printf("-L            Try to use large memory pages (if available). Increasing\n"
           "              the memory page size could reduce the number of TLB misses\n"
           "              and improve the performance. In order to get large pages\n"
           "              from the OS, memcached will allocate the total item-cache\n"
           "              in one large chunk.\n");
    printf("-D <char>     Use <char> as the delimiter between key prefixes and IDs.\n"
           "              This is used for per-prefix stats reporting. The default is\n"
           "              \":\" (colon). If this option is specified, stats collection\n"
           "              is turned on automatically; if not, then it may be turned on\n"
           "              by sending the \"stats detail on\" command to the server.\n");
    printf("-t <num>      number of threads to use (default: 4)\n");
    printf("-R            Maximum number of requests per event, limits the number of\n"
           "              requests process for a given connection to prevent \n"
           "              starvation (default: 20)\n");
    printf("-C            Disable use of CAS\n");
    printf("-b            Set the backlog queue limit (default: 1024)\n");
    printf("-B            Binding protocol - one of ascii, binary, or auto (default)\n");
    printf("-I            Override the size of each slab page. Adjusts max item size\n"
           "              (default: 1mb, min: 1k, max: 128m)\n");
#ifdef ENABLE_SASL
    printf("-S            Turn on Sasl authentication\n");
#endif
    printf("-o            Comma separated list of extended or experimental options\n"
           "              - (EXPERIMENTAL) maxconns_fast: immediately close new\n"
           "                connections if over maxconns limit\n"
           "              - hashpower: An integer multiplier for how large the hash\n"
           "                table should be. Can be grown at runtime if not big enough.\n"
           "                Set this based on \"STAT hash_power_level\" before a \n"
           "                restart.\n"
           "              - (EXPERIMENTAL) powersave[=n]: Dynamically scale host CPU usage\n"
	   "                with workload to reduce power consumption. Higher values for\n"
	   "                n (default=2) should result in more power savings, at the cost\n"
	   "                of increased latency for smaller workloads.\n"
#ifdef ONLOAD
           "              - onload_stacks: Speficy number of Onload stacks to allocate\n"
	   "                (Default: 16).\n"
#endif
           );
    return;
}

void CSettings::UsageLicense() {
    printf(MC_PACKAGE " " MC_VERSION "\n\n");
    printf(
    "Copyright (c) 2003, Danga Interactive, Inc. <http://www.danga.com/>\n"
    "All rights reserved.\n"
    "\n"
    "Redistribution and use in source and binary forms, with or without\n"
    "modification, are permitted provided that the following conditions are\n"
    "met:\n"
    "\n"
    "    * Redistributions of source code must retain the above copyright\n"
    "notice, this list of conditions and the following disclaimer.\n"
    "\n"
    "    * Redistributions in binary form must reproduce the above\n"
    "copyright notice, this list of conditions and the following disclaimer\n"
    "in the documentation and/or other materials provided with the\n"
    "distribution.\n"
    "\n"
    "    * Neither the name of the Danga Interactive nor the names of its\n"
    "contributors may be used to endorse or promote products derived from\n"
    "this software without specific prior written permission.\n"
    "\n"
    "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n"
    "\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n"
    "LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n"
    "A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\n"
    "OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n"
    "SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT\n"
    "LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\n"
    "DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n"
    "THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n"
    "(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n"
    "OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
    "\n"
    "\n"
    "This product includes software developed by Niels Provos.\n"
    "\n"
    "[ libevent ]\n"
    "\n"
    "Copyright 2000-2003 Niels Provos <provos@citi.umich.edu>\n"
    "All rights reserved.\n"
    "\n"
    "Redistribution and use in source and binary forms, with or without\n"
    "modification, are permitted provided that the following conditions\n"
    "are met:\n"
    "1. Redistributions of source code must retain the above copyright\n"
    "   notice, this list of conditions and the following disclaimer.\n"
    "2. Redistributions in binary form must reproduce the above copyright\n"
    "   notice, this list of conditions and the following disclaimer in the\n"
    "   documentation and/or other materials provided with the distribution.\n"
    "3. All advertising materials mentioning features or use of this software\n"
    "   must display the following acknowledgement:\n"
    "      This product includes software developed by Niels Provos.\n"
    "4. The name of the author may not be used to endorse or promote products\n"
    "   derived from this software without specific prior written permission.\n"
    "\n"
    "THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR\n"
    "IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES\n"
    "OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.\n"
    "IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,\n"
    "INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT\n"
    "NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\n"
    "DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n"
    "THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n"
    "(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF\n"
    "THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
    );

    return;
}

/*
 * On systems that supports multiple page sizes we may reduce the
 * number of TLB-misses by using the biggest available page size
 */
int CSettings::EnableLargePages() {
#if defined(HAVE_GETPAGESIZES) && defined(HAVE_MEMCNTL)
    int ret = -1;
    size_t sizes[32];
    int avail = getpagesizes(sizes, 32);
    if (avail != -1) {
        size_t max = sizes[0];
        struct memcntl_mha arg = {0};
        int ii;

        for (ii = 1; ii < avail; ++ii) {
            if (max < sizes[ii]) {
                max = sizes[ii];
            }
        }

        arg.mha_flags   = 0;
        arg.mha_pagesize = max;
        arg.mha_cmd = MHA_MAPSIZE_BSSBRK;

        if (memcntl(0, 0, MC_HAT_ADVISE, (caddr_t)&arg, 0, 0) == -1) {
            fprintf(stderr, "Failed to set large pages: %s\n",
                    strerror(errno));
            fprintf(stderr, "Will use default page size\n");
        } else {
            ret = 0;
        }
    } else {
        fprintf(stderr, "Failed to get supported pagesizes: %s\n",
                strerror(errno));
        fprintf(stderr, "Will use default page size\n");
    }

    return ret;
#else
    return -1;
#endif
}
