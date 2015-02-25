/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// Client.cpp : Defines the entry point for the console application.
//
#include "Client.h"

volatile uint32_t g_remainingThreads;
volatile uint32_t g_storeThreads;
volatile uint32_t g_flushAllThreads;
volatile uint32_t g_errorReport = 0;


int g_valueLen = 100;
int g_storeCnt = 0x7fffffff;
int g_overlapReqNum = 16;
int g_keyLen = 36;
int g_useHostName = 0;
string g_hostname = "localhost";
int g_portnum = 11211;
double g_hitRate = 1.0;
double g_missInterval = 100000000000000.0;
int g_multiGetReqCnt = 1;
bool g_bFlushAll = true;
int g_keyStart = 0;
int g_samplingPeriod = 10;
int g_sampleCount=0;
bool g_bBinary = true;

CRangeInfo g_numThreads = { 1 };
CRangeInfo g_burstDelay = { 0 };


#include "Timer.h"

CTimerBase * g_pTimerBase;
CTimer * g_pTimers[32];

void CRangeInfo::SetValues(const char *pCmd, const char *pArg)
{
    char *pEnd;
    m_inc = 1;
    m_cur = m_min = m_max = strtol(pArg, &pEnd, 10);

    if (pEnd == pArg || (*pEnd != '\0' && *pEnd != ':')) {
        fprintf(stderr, "Expected %s to have an integer values (%s)\n", pCmd, pArg);
        exit(1);
    }

    if (*pEnd == ':') {
        const char *pMax = pEnd+1;
        m_max = strtol(pMax, &pEnd, 10);

        if (pEnd == pArg || (*pEnd != '\0' && *pEnd != ':' && *pEnd != '*')) {
            fprintf(stderr, "Expected %s to have an integer values (%s)\n", pCmd, pArg);
            exit(1);
        }

        if (m_min >= m_max) {
            fprintf(stderr, "Expected %s to have min less than max (min:max:inc)\n", pCmd);
            exit(1);
        }
    }

    if (*pEnd == ':') {
        pEnd += 1;

        if (*pEnd == '*') {
            m_bMul = true;
            pEnd += 1;
        }

        const char *pInc = pEnd;
        m_inc = strtol(pInc, &pEnd, 10);

        if (pEnd == pArg || *pEnd != '\0') {
            fprintf(stderr, "Expected %s to have an integer values (%s)\n", pCmd, pArg);
            exit(1);
        }

        if (m_inc > m_max - m_min) {
            fprintf(stderr, "Expected %s to have inc <= (max-min) (min:max:inc)\n", pCmd);
            exit(1);
        }

        if (m_bMul && m_min == 0) {
            fprintf(stderr, "Expected %s to have min value > 0 with inc multiplier\n", pCmd);
            exit(1);
        }
    }
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
	//----------------------
	// Initialize Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");
#endif

    /* process arguments */
	int c;
	int len;
	char hostName[32]="";
    while (-1 != (c = getopt(argc, argv,
		"t:"	// thread count
		"d:"	// value size
		"k:"	// key size
		"o:"	// outstanding requests per trhead
		"H:"	// host name : port number
        "r:"    // hit rate
        "s:"    // stores per thread
        "m:"    // multi-get count
        "f"     // flush all disable
        "u:"    // tid start num
        "p:"    // sampling period
	"c:"	// sample count
        "b:"    // burst mode
        "h"
        "A"     // Ascii get mode
        "B"     // Binary get mode
			     "n:"     //use hostname
		)))
	{
		switch (c) {
		case 't':
            g_numThreads.SetValues("-t", optarg);
            break;
        case 'A':
            g_bBinary = false;
            break;
        case 'B':
            g_bBinary = true;
            break;
        case 'f':
            g_bFlushAll = false;
            break;
        case 'b':
            g_burstDelay.SetValues("-b", optarg);
            break;
        case 'u':
            g_keyStart = atoi(optarg);
            if (g_keyStart < 0) {
                fprintf(stderr, "Unique key must be greater than 0\n");
                exit(-1);
            }
            break;
		case 'm':
			g_multiGetReqCnt = atoi(optarg);
			if (g_multiGetReqCnt < 1) {
				fprintf(stderr, "Number of get requests in multi-get must be greater then 1\n");
				exit(-1);
			}
			break;
		case 'd':
			g_valueLen = atoi(optarg);
			if (g_valueLen < 26) {
				fprintf(stderr, "Minimum data size is 26\n");
				exit(-1);
			}
			break;
		case 's':
			g_storeCnt = atoi(optarg);
			if (g_storeCnt < 1) {
				fprintf(stderr, "Minimum stores per thread is 1\n");
				exit(-1);
			}
			break;
		case 'o':
			g_overlapReqNum = atoi(optarg);
			if (g_overlapReqNum <= 0 || g_overlapReqNum > 256) {
				fprintf(stderr, "Number of outstanding get operations must be between 1 and 256\n");
				exit(1);
			}
			break;
		case 'p':
			g_samplingPeriod = atoi(optarg);
			if (g_samplingPeriod <= 0 || g_samplingPeriod > 1000) {
				fprintf(stderr, "Sampling period must be between 1 and 1000 sec\n");
				exit(1);
			}
			break;
		case 'c':
			g_sampleCount=atoi(optarg);
			break;
        case 'r':
            g_hitRate = atof(optarg);
            if (g_hitRate < 0.00 || g_hitRate > 0.95) {
                fprintf(stderr, "Hit rate must be in the range of 0.00 to 0.95\n");
                exit(1);
            }
            g_missInterval = 1.0 / (1.0 - g_hitRate);
            break;
		case 'H':
			{
				g_hostname = optarg;
				int pos = g_hostname.find_first_of(":");
				if (pos == 0) {
					fprintf(stderr, "Expected -H to have format hostname:port\n");
					exit(1);
				}
				const char *pPort = g_hostname.c_str() + pos + 1;
				char * pEnd;
				g_portnum = strtol(pPort, &pEnd, 10);
				if (*pEnd != '\0') {
					fprintf(stderr, "Expected -H to have format hostname:port\n");
					exit(1);
				}
				g_hostname.erase(pos);
			}
			break;
		case 'n':
		  g_useHostName=atoi(optarg);
		  break;
		default:
			printf("Illegal argument \"%c\"\n\n", c);
        case 'h':
            printf("McPerfClient command line help:\n");
            printf("  -t <num> or <min:max>       Thread count (default is 1)\n");
            printf("  -d <num>        Value size (minimum of 26, default is 500)\n");
            printf("  -o <num>        Simultaneous request count (default is 16)\n");
            printf("  -H <name:port>  System/port for TCP connection\n");
	    printf("  -n              Include first 8 chars of hostname in the key\n");
            printf("  -r <float>      Hit rate, (default 1.0)\n");
            printf("  -m <num>        Multi-get request count\n");
            printf("  -s <num>        Maximum stores per thread\n");
            printf("  -p <num>        Sampling period\n");
	    printf("  -c <num>        Collect at most <num> samples, then exit\n");
            exit(0);
		}
	}

    if (g_multiGetReqCnt > g_overlapReqNum)
        fprintf(stderr, "Multi-get request count (-m) must be less than overlap request count (-o)\n"), exit(1);

    FILE * rptFp = fopen("McPerfClient.txt", "w");
    if (rptFp == 0) {
        printf("Could not open MCPerfClient.txt\n");
        exit(1);
    }

    if(g_useHostName) {
      gethostname(hostName, g_useHostName);
      len=strnlen(hostName, g_useHostName);
    }

    for (int i = 0; i < argc; i += 1)
        fprintf(rptFp, "%s ", argv[i]);
    fprintf(rptFp, "\n , Gets/sec, min, avg, max\n");

	CThread *pThreads = new CThread [ g_numThreads.m_max ];

	g_storeThreads = g_numThreads.m_max;
	g_remainingThreads = g_numThreads.m_max;
	g_flushAllThreads = g_numThreads.m_max;

	for (int i = 0; i < g_numThreads.m_max; i += 1) {
		pThreads[i].SetTid(i);
		//pThreads[i].SetHostName(hostName, len);
		pThreads[i].CreateThread();

		pThreads[i].m_getRespMinTime = ~0ull;
		pThreads[i].m_getRespMaxTime = 0;
		pThreads[i].m_getRespTime = 0;
	}

	while (g_storeThreads > 0)
		sleep(1);

	printf("Store phase complete\n");

    bool bStop = false;
    int sampleCount=0;
	struct timeval st;
	for (;!bStop;) {
		uint64_t startGetReqCnt = 0;
		uint64_t startGetRespCnt = 0;
		for (int i = 0; i < g_numThreads.m_max; i += 1) {
			startGetRespCnt += pThreads[i].m_getRespCnt;
			startGetReqCnt += pThreads[i].m_getReqCnt;
        }

		gettimeofday(&st, NULL);
		uint64_t startUsec = st.tv_sec * 1000000ULL + (uint64_t)st.tv_usec;

		sleep(g_samplingPeriod);
		sampleCount++;
		bStop=bStop || (g_sampleCount && (sampleCount>=g_sampleCount));
		uint64_t endGetReqCnt = 0;
		uint64_t endGetRespCnt = 0;
		uint64_t minRespTime = ~0ull;
		uint64_t maxRespTime = 0;
		uint64_t avgRespTime = 0;

		for (int i = 0; i < g_numThreads.m_max; i += 1) {
			endGetReqCnt += pThreads[i].m_getReqCnt;
			endGetRespCnt += pThreads[i].m_getRespCnt;
			if (pThreads[i].m_getRespMinTime < minRespTime)
				minRespTime = pThreads[i].m_getRespMinTime;
			if (pThreads[i].m_getRespMaxTime > maxRespTime)
				maxRespTime = pThreads[i].m_getRespMaxTime;
			avgRespTime += pThreads[i].m_getRespTime;

			pThreads[i].m_getRespMinTime = ~0ull;
			pThreads[i].m_getRespMaxTime = 0;
			pThreads[i].m_getRespTime = 0;
		}
		avgRespTime /= (endGetRespCnt - startGetRespCnt + 1);

		gettimeofday(&st, NULL);
		uint64_t endUsec = st.tv_sec * 1000000ULL + (uint64_t)st.tv_usec;

        if (g_burstDelay.m_min != g_burstDelay.m_max) {
            printf("b%d ", g_burstDelay.m_cur);
            fprintf(rptFp, "b%d, ", g_burstDelay.m_cur);
            g_burstDelay.Inc();
            bStop = bStop || (g_burstDelay.m_cur > g_burstDelay.m_max);

        } else if (g_numThreads.m_min != g_numThreads.m_max) {
            printf("t%d ", g_numThreads.m_cur);
            fprintf(rptFp, "t%d, ", g_numThreads.m_cur);
            g_numThreads.m_cur += g_numThreads.m_inc;
            bStop = bStop || (g_numThreads.m_cur > g_numThreads.m_max);
        }

		printf("%7.2f G/s, min=%lld, avg=%lld, max=%lld\n",
			(endGetReqCnt - startGetReqCnt) * 1000000.0 / (endUsec - startUsec + 1),
			(long long)minRespTime, (long long)avgRespTime, (long long)maxRespTime);

        fprintf(rptFp, "%7.2f, %lld, %lld, %lld\n",
            (endGetReqCnt - startGetReqCnt) * 1000000.0 / (endUsec - startUsec + 1),
			(long long)minRespTime, (long long)avgRespTime, (long long)maxRespTime);
	}

    fclose(rptFp);

	return 0;
}
