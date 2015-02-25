/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// Client.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifdef _WIN32
#include <Winsock2.h>

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "Thread.h"

bool	threadsStart = false;
int	cmdPctTotal = 0;

void	processArgs(int argc, char* argv[], ArgValues &argV);
bool	parseCmdPctFile(string &cmdPctFile);

uint64_t        getTime(uint64_t *currUsec = 0, int *currSec = 0);

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

    struct	ArgValues argValues;

    processArgs(argc, argv, argValues);

    pthread_t		*pThreadId;
    CThread		**threads;
    bool		bBinary;
    bool		bUdp;

    pThreadId = (pthread_t *)malloc(argValues.threadCnt * (sizeof(pthread_t)));
    threads = (CThread **)malloc(argValues.threadCnt * (sizeof(CThread *)));

    for (int i = 0; i < argValues.threadCnt; i++) {
	if(argValues.binding == 0) {
	    bBinary = (i&2)==2;
	} else if(argValues.binding == 1) {
	    bBinary = true;
	} else {
	    bBinary = false;
	}
	if(argValues.tcpPort == 0) {
	    if(argValues.udpPort == 0) {
		printf("Tcp and Udp disabled\n");
		exit(-1);
	    }
	    bUdp = true;	    // tcp=0 udp!=0
	} else {
	    if(argValues.udpPort == 0) {
		bUdp = false;	    // tcp!=0 udp=0
	    } else {
		bUdp = (i&1)==1;    // tcp!=0 udp!=0
	    }
	}
	threads[i] = new CThread(i, bBinary, bUdp, argValues);
    }

    for (int i = 0; i < argValues.threadCnt; i++) {
	pthread_create(&pThreadId[i], NULL, CThread::Startup, threads[i]);
    }
    //
    //	start all threads together
    //
    threadsStart = true;
    for (int i = 0; i < argValues.threadCnt; i++) {
	pthread_join(pThreadId[i], 0);
    }
    if(argValues.test == 2) {
	for(int i=0; i<argValues.threadCnt; i++) {
	    threads[i]->printPerfData();
	}
    }
    for (int i = 0; i < argValues.threadCnt; i++) {
	if(argValues.verbose) {
	    printf("thread %d   %s   %s  %s",
		i, (threads[i]->GetBinEnabled() == true) ? "bin  ":"ascii",
		(threads[i]->GetUdpEnabled() == true) ? "udp" : "tcp",
		(threads[i]->GetIsTiming() == true) ? "timed" : "    ");
	    if(threads[i]->GetUdpEnabled() == true) {
		printf(" %d packets dropped\n", threads[i]->GetUdpDropped());
	    } else {
		printf("\n");
	    }
	}
    }
    printf("Test %s\n", (threadFails == 0) ? "passed" : "failed");
    free(pThreadId);
    free(threads);
    return 0;
}

#ifdef _WIN32
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL


int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static int tzflag;

    if (NULL != tv) {
        GetSystemTimeAsFileTime(&ft);

        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;

        /*converting file time to unix epoch*/
        tmpres -= DELTA_EPOCH_IN_MICROSECS;
        tmpres /= 10; /*convert into microseconds*/
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }
    return 0;
}
#endif

struct ArgTbl {
    string		argName;	// random, trace etc
    char		expected;	// value to read next n=none N=numeric d=none_default
    int			defaultVal;
    uint64_t		offset;
    string		helpStr;
};


string randHelp =	"\t\tGenerate -rt random tests for each thread (default)\n";
string dirHelp =	"\t\tGenerate directed tests for each thread\n";
string perfHelp =	"\t\tGenerate performance tests for each thread\n";
string statsHelp =	"\t\tRun the STATS command and print the results\n";
string hostHelp =	"<host>\tName of host running memcached. May be specified as host:port\n"
				"\t\tor host. If port not specified, port in -p or -U is used\n"
				"\t\tDefault ia 127.0.0.1\n";
string tcppHelp =	"<num>\tTcp port to connect, 0 is off";
string udppHelp =	"<num>\tUdp port to connect, 11211 is normal, 0 is off";
string bindHelp =	"<bind>\tBinding protocol - one of ascii, binary, or auto (default)\n";
string helpHelp =	"\t\tPrint this help and exit\n";
string rtHelp =		"<num>\tNumber of random tests to generate per thread";
string tHelp =		"<num>\tNumber of threads to generate";
string vHelp =		"\t\tVerbose, print thread information\n";
string vvHelp =		"\t\tVery Verbose, print tracing for every command\n";
string vvvHelp =	"\t\tVery Very Verbose, print sizes of keys and data\n";
string pscHelp =	"<num>\tNumber of stores in the performance test";
string pgcHelp =	"<num>\tNumber of gets in the performance test";
string pklHelp =	"<num>\tKey length in the performance test";
string pdlHelp =	"<num>\tData length in the performance test";
string pstpHelp =	"<num>\tStop after the stores have been sent";
string seedHelp =	"<seed>\tRandom number generator seed, 0=pick random seed";
string stopHelp =	"<num>\tStop on error flag";
string getCntHelp =	"<num>\tNumber of getq or ascii get k,k,...to issue before waiting\n"
                        "\t\tfor reply, max is 10";
string errHelp =	"<num>\tNumber of -t threads generating protocol errors";
string statHelp =	"<num>\tStats test to run 0=all 1=stat 2=items 3=settings 4=sizes\n"
			"\t\t5=slabs 6=detail_on 7=detail_off 8-detail_dump 9=cachedump\n"
			"\t\t10=reset";
string nofHelp =	"<num>\tRun multiple copies of the test, the first must use -nf 0\n"
			"\t\tthe second -nf 1 etc. Wait for the seed to be printed before\n"
			"\t\tstarting second,third etc";
string fillHelp =	"<num>\tPercent of keys to make valid before starting 0-100";
string cmdPctHelp =	"<file>\tFile containing the percentages of each command to run\n";
string timeHelp =	"<sec>\tSeconds to run the rand test, overrides -rt";
string datalmaxHelp =	"<num>\tMax data length, min value is 1";
string datalminHelp =	"<num>\tMin data length, must be <= max (-Lmax value)";
string traceKeyHelp =	"<keyIdx>\tIndex of key to trace";
string timeThreadsHelp ="<num>\tNumber of -t threads testing timing of the commands";
string timeWindowHelp =	"<num>\tNumber of seconds either side of an expected timeout to\n"
			"\t\tignore a key being timed out";
string progcHelp =	"<num>\tPrint progress line every num tests";
string progtHelp =	"<sec>\tPrint progress line every sec seconds";
string progpHelp =	"\t\tReplace the progress line in -Pc or -Pt with a .\n";

ArgValues	*avd = 0;
ArgTbl		argTblEntry[] = {
	//	name		expect	default		offset in struct
	{	"rand",		'd',	0,		(uint64_t)&avd->test,		randHelp  },
	{	"dir",		'n',	1,		(uint64_t)&avd->test,		dirHelp },
	{	"perf",		'n',	2,		(uint64_t)&avd->test,		perfHelp },
	{	"stats",	'n',	3,		(uint64_t)&avd->test,		statsHelp },
	{	"-s",		'N',	0,		(uint64_t)&avd->seed,		seedHelp },
	{	"-p",		'N',	11211,		(uint64_t)&avd->tcpPort,	tcppHelp },
	{	"-U",		'N',	0,	    	(uint64_t)&avd->udpPort,	udppHelp },
	{	"-H",		'H',	0,		0,				hostHelp },
	{	"-B",		'B',	0,		(uint64_t)&avd->binding,	bindHelp },	
	{	"-h",		'h',	0,		0,				helpHelp },
	{	"-rt",		'N',	200,		(uint64_t)&avd->cmdCnt,		rtHelp },
	{	"-v",		'V',	1,		(uint64_t)&avd->verbose,	vHelp },
	{	"-vv",		'V',	2,		(uint64_t)&avd->verbose,	vvHelp },
	{	"-vvv",		'V',	3,		(uint64_t)&avd->verbose,	vvvHelp },
	{	"-t",		'N',	1,		(uint64_t)&avd->threadCnt,	tHelp },
	{	"-e",		'N',	0,		(uint64_t)&avd->threadCntE,	errHelp },
	{	"-psc",		'N',	200,		(uint64_t)&avd->perfStoreCnt,	pscHelp },
	{	"-pgc",		'N',	200,		(uint64_t)&avd->perfGetCnt,	pgcHelp },
	{	"-pkl",		'N',	16,		(uint64_t)&avd->perfKeyLength,	pklHelp },
	{	"-pdl",		'N',	440,		(uint64_t)&avd->perfDataLength,	pdlHelp },
	{	"-pstop",	'N',	0,		(uint64_t)&avd->perfStop,	pstpHelp },
	{	"-soe",		'N',	1,		(uint64_t)&avd->stop,		stopHelp },
	{	"-gqcnt",	'N',	5,		(uint64_t)&avd->getMax,		getCntHelp },
	{	"-stat",	'N',	0,		(uint64_t)&avd->statTest,	statHelp },
	{	"-nf",		'N',	0,		(uint64_t)&avd->noFlush,	nofHelp },
	{	"-f",		'N',	0,		(uint64_t)&avd->fillPct,	fillHelp },
	{	"-c",		'C',	0,		0,				cmdPctHelp },
	{	"-T",		'N',	0,		(uint64_t)&avd->perfTime,	timeHelp },
	{	"-Lmax",	'N',	2000,		(uint64_t)&avd->dataMax,	datalmaxHelp },
	{	"-Lmin",	'N',	20,		(uint64_t)&avd->dataMin,	datalminHelp },
	{	"-K",		'N',	-1,		(uint64_t)&avd->traceKey,	traceKeyHelp },
	{	"-TT",		'N',	0,		(uint64_t)&avd->threadCntT,	timeThreadsHelp },
	{	"-TW",		'N',	2,		(uint64_t)&avd->keyTimeWindow,	timeWindowHelp },
	{	"-Pc",		'N',	0,		(uint64_t)&avd->progC,		progcHelp },
	{	"-Pt",		'N',	0,		(uint64_t)&avd->progT,		progtHelp },
	{	"-Pp",		'V',	1,		(uint64_t)&avd->progP,		progpHelp },
	{	"",		'q',	0,		0 }
    };

void
usage(string &progName, bool exitNow = true)
{
    int		i;

    printf("Usage: %s\n", progName.c_str());
    for(i=0; argTblEntry[i].expected != 'q'; i++) {
	switch(argTblEntry[i].expected) {
	case 'n':
	case 'd':
	case 'h':
	case 'V':
	case 'B':
	case 'H':
	case 'C':
	    printf("%s %s",
		argTblEntry[i].argName.c_str(),
		argTblEntry[i].helpStr.c_str());
	    break;
	case 'N':
	    printf("%s %s (default %d)\n",
		argTblEntry[i].argName.c_str(),
		argTblEntry[i].helpStr.c_str(),
		argTblEntry[i].defaultVal);
	    break;
	}
    }
    printf("\n");
    if(exitNow == true) {
	exit(0);
    }
}


void
processArgs(int argc, char *argv[], ArgValues &argV)
{
    int		i;
    uint64_t	ap;
    int		val;
    string	progName;
    const char	*c;
    //
    //	set defaults
    //
    progName = *argv;
    ap = (uint64_t)&argV;
    for(i=0; argTblEntry[i].expected != 'q'; i++) {
	switch(argTblEntry[i].expected) {
	    case 'N':
	    case 'd':				// default
	    case 'B':				// binding
		*((int *)(ap + argTblEntry[i].offset)) = argTblEntry[i].defaultVal;
	    case 'n':				// not default
		break;
	    case 'V':				// verbose default is 0
		*((int *)(ap + argTblEntry[i].offset)) = 0;
		break;
	    case 'H':
		argV.hostName = "";
		break;
	    case 'C':
		argV.cmdPctFile = "";
		break;
	}
    }
    argc--;								// skip program name
    argv++;
    while(argc) {
	for(i=0; argTblEntry[i].expected != 'q'; i++) {
	    if(argTblEntry[i].argName == *argv) {
		switch(argTblEntry[i].expected) {
		case 'n':				// random, directed, performance
		case 'd':				// default
		case 'V':				// verbose flags
		    *((int *)(ap + argTblEntry[i].offset)) = argTblEntry[i].defaultVal;
		    break;
		case 'N':
		    argc--;
		    if(argc == 0 || *argv[1] == '-') {
			usage(progName, false);
			printf("Expecting value for %s\n", *argv);
			exit(-1);
		    }
		    argv++;
		    val = atoi(*argv);
		    *((int *)(ap + argTblEntry[i].offset)) = val;
		    break;
		case 'B':
		    argc--;
		    if(argc == 0 || *argv[1] == '-') {
			usage(progName, false);
			printf("Expecting value for %s\n", *argv);
			exit(-1);
		    }
		    argv++;
		    if(strcmp(*argv, "auto") == 0) {
			*((int *)(ap + argTblEntry[i].offset)) = 0;		// random
		    } else if(strcmp(*argv, "binary") == 0) {
			*((int *)(ap + argTblEntry[i].offset)) = 1;		// binary
		    } else if(strcmp(*argv, "ascii") == 0) {
			*((int *)(ap + argTblEntry[i].offset)) = 2;		// ascii
		    } else {
			usage(progName, false);
			printf("Expecting auto | binary | ascii  got %s\n", *argv);
			exit(-1);
		    }
		    break;
		case 'h':
		    usage(progName);
		    break;
		case 'H':
		    argc--;
		    if(argc == 0 || *argv[1] == '-') {
			usage(progName, false);
			printf("Expecting hostname for %s\n", *argv);
			exit(-1);
		    }
		    argv++;
		    argV.hostName = *argv;
		    break;
		case 'C':
		    argc--;
		    if(argc == 0 || *argv[1] == '-') {
			usage(progName, false);
			printf("Expecting filename for %s\n", *argv);
			exit(-1);
		    }
		    argv++;
		    argV.cmdPctFile = *argv;
		    break;
		default:
		    usage(progName);
		    break;
		}
		argc--;
		argv++;
		break;
	    }
	}
	if(argTblEntry[i].expected == 'q') {
	    usage(progName, false);
	    printf("Unknown argument <%s>\n", *argv);
	    exit(-1);
	}
    }
    //
    //	fixup the hostname:port if needed
    //
    if(argV.hostName.length() > 0) {
	size_t		pos, pos1;
	pos = argV.hostName.find_first_of(':');
	if(pos == 0) {
	    usage(progName, false);
	    printf("Bad host name <%s>\n", argV.hostName.c_str());
	    exit(-1);
	}
	if(pos != string::npos) {
	    string portS = argV.hostName.substr(pos+1);
	    pos1 = portS.find_last_not_of("0123456789");
	    if(pos1 != string::npos || portS.length() == 0) {
		usage(progName, false);
		printf("Bad port number <%s>\n", portS.c_str());
		exit(-1);
	    }
	    int port = atoi(portS.c_str());
	    argV.tcpPort = port;
	    if(argV.udpPort != 0) {
		argV.udpPort = port;
	    }
	    argV.hostName.resize(pos);
	}
    }
    //
    // Set the master seed if needed
    //
    if(argV.seed == 0) {
        argV.seed = (int)getTime();
    }
    //
    // Get the local host information
    //
    if(argV.hostName.length() > 0) {
	hostent * localHost = gethostbyname(argV.hostName.c_str());
	if(localHost == 0) {
	    printf("Unable to resolve host name %s\n", argV.hostName.c_str());
	    exit(-1);
	}
	char * localIP = inet_ntoa (*(struct in_addr *)*localHost->h_addr_list);
	argV.hostIp = localIP;
    } else {
	argV.hostIp = "127.0.0.1";				// default to local
    }

    if(argV.verbose > 0) {
	printf("hostname %s  Ip %s   tcpPort %d   udpPort %d\n",
	    (argV.hostName.length() > 0) ? argV.hostName.c_str() : "127.0.0.1",
	    argV.hostIp.c_str(),
	    argV.tcpPort, argV.udpPort);
    }
    if(argV.getMax > 10) {
        printf("-gqcnt forced to be <= 10\n");
        argV.getMax = 10;
    }
    //
    //	check fillPct 0-100
    //
    if(argV.fillPct < 0 || argV.fillPct > 100) {
	usage(progName, false);
	printf("Invalid value for -f (%d)\n", argV.fillPct);
	exit(-1);
    }
    if(argV.dataMax < 1) {
	usage(progName, false);
        printf("Invalid value for -Lmax (%d), min is 1\n", argV.dataMax);
        exit(-1);
    }
    if(argV.dataMin > argV.dataMax) {
	usage(progName, false);
        printf("Invalid value for -Lmin (%d), must be <= -Lmax (%d)\n",
		argV.dataMin, argV.dataMax);
        exit(-1);
    }
    //
    //	parse the Cmd file if present
    //
    if(argV.cmdPctFile.length() != 0) {
	if(parseCmdPctFile(argV.cmdPctFile) == false) {
	    if(errno != 0) {
		c = strerror(errno);
	    } else {
		c = "";
	    }
	    usage(progName, false);
	    printf("Unable to parse cmdPct file <%s> : %s\n",
		argV.cmdPctFile.c_str(), c);
	    exit(-1);
	}
    } else {
	int pct = 100 / eCmdCnt;
	cmdPctTotal = 0;
	for(i=0; i<eCmdCnt; i++) {
	    cmdPctTotal += pct;
	    cmdPct[i].percent = pct;
	    cmdPct[i].minRnd = i * pct;
	    cmdPct[i].maxRnd = ((i+1) * pct) - 1;
	}
    }
}

ECmd
getCmdRnd(uint32_t r)
{
    int		rp;
    int		i;

    rp = r % cmdPctTotal;
    for(i=0; i<eCmdCnt; i++) {
	if(rp >= cmdPct[i].minRnd && rp <= cmdPct[i].maxRnd) {
	    break;
	}
    }
    return((ECmd)i);
}

bool
parseCmdPctFile(string &cmdPctFile)
{
    FILE	*f;
    char	buf[256];
    string	line;
    int		i;
    size_t	pos;
    string	lcmd;
    string	cmdStrT[eCmdCnt];
    int		pct;

    for(i=0; i<eCmdCnt; i++) {
	line = cmdStr[i];
	pos = line.find_first_of(" ");
	if(pos != string::npos) {
	    line.resize(pos);
	}
	cmdStrT[i] = line;
    }
    errno = 0;
    f = fopen(cmdPctFile.c_str(), "r");
    if(f == NULL) {
	return(false);
    }
    cmdPctTotal = 0;
    while(1) {
	if(fgets(buf, sizeof(buf), f) == NULL) {
	    if(feof(f)) {
		break;				// eof
	    }
	    return(false);			// some error
	}
	if(buf[strlen(buf)-1] == '\n') {
	    buf[strlen(buf)-1] = 0;
	}
	line = buf;
	if(line.length() == 0 || line[0] == '#') {
	    continue;
	}
	pos = line.find_first_not_of(" \t");	// remove leading whitespace
	if(pos != 0) {
	    line = line.substr(pos);		// remove it
	}
	lcmd = line;
	pos = lcmd.find_first_of(" \t");
	if(pos != string::npos) {
	    lcmd.resize(pos);
	}
	for(i=0; i<eCmdCnt; i++) {
	    if(lcmd.compare(cmdStrT[i]) == 0) {
		break;
	    }
	}
	if(i == eCmdCnt) {
	    printf("Invalid line in cmdPct file <%s>\n", line.c_str());
	    return(false);
	}
	if(cmdPct[i].percent != -1) {
	    printf("Duplicate line in cmdPct file <%s>\n", line.c_str());
	    return(false);
	}
	pos = line.find_first_of(" \t");		// whitespace after name
	pos = line.find_first_not_of(" \t", pos);	// first of #
	if(pos == string::npos || isdigit(line[pos]) == false) {
	    printf("Unable to find number in cmdPct file <%s>\n", line.c_str());
	    return(false);
	}
	pct = atoi(line.substr(pos).c_str());
        cmdPctTotal += pct;
        cmdPct[i].percent = pct;
    }
    pct = 0;
    for(i=0; i<eCmdCnt; i++) {
	cmdPct[i].minRnd = pct;
	cmdPct[i].maxRnd = cmdPct[i].minRnd + cmdPct[i].percent - 1;
	pct = cmdPct[i].maxRnd + 1;
    }
    return(true);
}
