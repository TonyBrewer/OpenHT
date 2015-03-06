/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include "Ht.h"
using namespace Ht;

#include <string>
using namespace std;

#include "AppArgs.h"

AppArgs * g_pAppArgs;

AppArgs::AppArgs(int argc, char **argv)
{
	string jobsFile;

	m_threads = 1;
	m_duration = 10;
	m_bReadOnce = false;
	m_bNoWrite = false;
	m_bLoopTest = false;
	m_bAspectRatio = false;
	m_bBenchmark = false;
	m_bCheck = false;
	m_bHostOnly = false;
	m_bHostMem = false;
	m_bDebugInfo = false;
	m_bDaemon = false;
	m_bRemote = false;
	m_bAddRestarts = false;
	m_persMode = 3;
	m_quality = 80;

	m_server="0.0.0.0";
	m_port=18811;

	AppJob appJob;

	int argPos;
	int fileCnt = 0;
	for (argPos=1; argPos < argc; argPos++) {
		if (argv[argPos][0] == '-') {
			if ((strcmp(argv[argPos], "-h") == 0) ||
				(strcmp(argv[argPos], "-help") == 0)) {
					Help();
					exit(0);
			} else if (strcmp(argv[argPos], "-threads") == 0) {
				argPos += 1;
				if (argPos >=  argc) {
					printf("Expected value for -threads\n");
					Help();
				}
				char * pEnd;
				m_threads = strtol(argv[argPos], &pEnd, 10);
				if (!isdigit(*argv[argPos]) || *pEnd != '\0') {
					printf("Expected thread count value for -threads\n");
					Help();
				}
			} else if (strcmp(argv[argPos], "-persMode") == 0) {
				argPos += 1;
				if (argPos >=  argc) {
					printf("Expected mode string for -persMode\n");
					Help();
				}
				if (strcmp(argv[argPos], "dh_only") == 0)
					m_persMode = 1;
				else if (strcmp(argv[argPos], "ve_only") == 0)
					m_persMode = 2;
				else if (strcmp(argv[argPos], "both") == 0)
					m_persMode = 3;
				else if (strcmp(argv[argPos], "none") == 0)
					m_persMode = 0;
				else {
					printf("Expected mode for -persMode (none, dh_only, ve_only, both)\n");
					Help();
				}				
			} else if (strcmp(argv[argPos], "-size") == 0) {
				argPos += 1;
				if ((argPos+1) >=  argc) {
					printf("Expected width and height values for -size\n");
					Help();
				}
				char * pEnd;
				appJob.m_width = strtol(argv[argPos], &pEnd, 10);
				if (!isdigit(*argv[argPos]) || *pEnd != '\0') {
					printf("Expected width value for -size\n");
					Help();
				}
				argPos += 1;
				appJob.m_height = strtol(argv[argPos], &pEnd, 10);
				if (!isdigit(*argv[argPos]) || *pEnd != '\0') {
					printf("Expected height value for -size\n");
					Help();
				}
			} else if (strcmp(argv[argPos], "-scale") == 0 || strcmp(argv[argPos], "-resize") == 0) {
				argPos += 1;
				if (argPos >=  argc) {
					printf("Expected scale value for -scale\n");
					Help();
				}
				char * pEnd;
				appJob.m_scale = strtol(argv[argPos], &pEnd, 10);
				if (!isdigit(*argv[argPos]) || *pEnd != '%') {
					printf("Expected scale value for -scale\n");
					Help();
				}
			} else if (strcmp(argv[argPos], "-jobs") == 0) {
				argPos += 1;
				if (argPos >=  argc) {
					printf("Expected job file name for -jobs\n");
					Help();
				}
				jobsFile = argv[argPos];
			} else if (strcmp(argv[argPos], "-benchmark") == 0) {
				m_bBenchmark = true;
				argPos += 1;
				if (argPos >=  argc) {
					printf("Expected time value for -benchmark\n");
					Help();
				}
				char * pEnd;
				m_duration = strtol(argv[argPos], &pEnd, 10);
				if (!isdigit(*argv[argPos]) || *pEnd != '\0' || m_duration <= 0) {
					printf("Expected time value for -benchmark\n");
					Help();
				}
			} else if (strcmp(argv[argPos], "-client_read") == 0) {
				m_bClientRead = true;
			} else if (strcmp(argv[argPos], "-client_write") == 0) {
				m_bClientWrite = true;				
			} else if (strcmp(argv[argPos], "-read_once") == 0) {
				m_bReadOnce = true;
			} else if (strcmp(argv[argPos], "-no_write") == 0) {
				m_bNoWrite = true;
			} else if (strcmp(argv[argPos], "-loop_test_ratio") == 0) {
				m_bLoopTest = true;
				m_bCheck = true;
				m_bAspectRatio = true;
#if defined(HT_VSIM) || defined(HT_APP)
				m_bHostMem = true;
#endif
			} else if (strcmp(argv[argPos], "-loop_test") == 0) {
				m_bLoopTest = true;
				m_bCheck = true;
#if defined(HT_VSIM) || defined(HT_APP)
				m_bHostMem = true;
#endif
			} else if (strcmp(argv[argPos], "-check") == 0) {
				m_bCheck = true;
				g_IsCheck=true;
#if defined(HT_VSIM) || defined(HT_APP)
				m_bHostMem = true;
#endif
			} else if (strcmp(argv[argPos], "-host_only") == 0) {
				m_bHostOnly = true;
			} else if (strcmp(argv[argPos], "-host_mem") == 0) {
				m_bHostMem = true;
			} else if (strcmp(argv[argPos], "-daemon") == 0) {
				m_bDaemon = true;
			} else if (strcmp(argv[argPos], "-remote") == 0) {
				m_bRemote = true;  
			} else if (strcmp(argv[argPos], "-aspect") == 0) {
				m_bKeepAspect = true;  				
			} else if (strcmp(argv[argPos], "-data_mover") == 0) {
				// now a noop
			} else if (strcmp(argv[argPos], "-add_restarts") == 0) {
				m_bAddRestarts = true;
			} else if (strcmp(argv[argPos], "-server") == 0) {
				argPos += 1;
				if (argPos >=  argc) {
					printf("Expected server name for -server\n");
					Help();
				}
				m_server=argv[argPos];
			} else if (strcmp(argv[argPos], "-port") == 0) {
				argPos += 1;
				if (argPos >=  argc) {
					printf("Expected port value for -port\n");
					Help();
				}
				char * pEnd;
				m_port = strtol(argv[argPos], &pEnd, 10);
				if (!isdigit(*argv[argPos]) || *pEnd != '\0') {
					printf("Expected port value for -port\n");
					Help();
				}
			} else if (strcmp(argv[argPos], "-quality") == 0) {
				argPos += 1;
				if (argPos >=  argc) {
					printf("Expected value for -quality\n");
					Help();
				}
				char * pEnd;
				m_quality = strtol(argv[argPos], &pEnd, 10);
				if (!isdigit(*argv[argPos]) || *pEnd != '\0') {
					printf("Expected value for -quality\n");
					Help();
				}				
			} else if (strcmp(argv[argPos], "-debug_info") == 0) {
				m_bDebugInfo = true;
#ifdef MAGICK_FALLBACK
			} else if (strcmp(argv[argPos], "-magick") == 0) {
				m_bMagickMode = true;
#endif
			} else {
				printf("Unknown command line switch: %s\n", argv[argPos]);
				Help();
			}
		} else {
			switch (fileCnt++) {
			case 0:
				appJob.m_inputFile = argv[argPos];
				break;
			case 1:
				appJob.m_outputFile = argv[argPos];
				break;
			}
		}
	}
#ifdef MAGICK_FALLBACK
	if (m_bHostOnly && !m_bMagickMode) {
#else
	if (m_bHostOnly) {
#endif
		m_bCheck = false;
		m_bNoWrite = true;
	}

	//In daemon/remote mode, the mode will determine whether we pass around input/output buffers or filenames
	//It is pretty much ignored in local mode
	if(m_bBenchmark) {
	  appJob.m_mode= (m_bReadOnce ? MODE_INBUFF : MODE_INFILE) | (m_bNoWrite ? 0 : MODE_OUTFILE);
	} else {
	  if(m_bRemote) {
	    appJob.m_mode|=m_bClientRead ? MODE_INBUFF : MODE_INFILE;
	    appJob.m_mode|=m_bClientWrite ? MODE_OUTBUFF : MODE_OUTFILE;
	  } else {
	    appJob.m_mode|=MODE_INFILE | MODE_OUTFILE;
	  }
	}

	//If a scale parameter has been specified, or we have been specifically instructed
	//to maintain the aspect ratio, then we need to set our mode to do so.
	if(appJob.m_scale || m_bKeepAspect) {
	  appJob.m_mode |= MODE_KEEPASPECT;
	}
	//if we are running as a client, then we don't need the coprocessor so we force "host_only" to 
	//keep from allocating coproc resources
	if (m_bRemote) {
		m_bHostOnly=true;
	}
	//if we are running as a daemon then we don't need to check most of the parameters as they will get 
	//passed in from the client
	if (!m_bDaemon) {
		if (jobsFile.size() > 0) {
			// check that command line jobs info was not specified
			if (appJob.m_inputFile.size() > 0 || appJob.m_outputFile.size() > 0 || appJob.m_width > 0 || appJob.m_height > 0 || appJob.m_scale > 0) {
				printf("  Error: command line job info (inputfile, outputfile, -scale, -size) not expected with -jobs\n");
				Help();
			}

			// read in jobs file
			ParseJobsFile( jobsFile );

		} else {
			if (appJob.m_inputFile.size() == 0) {
				printf("  Error: inputfile not specified\n");
				Help();
			}

			if (appJob.m_outputFile.size() == 0 && !m_bNoWrite) {
				printf("  Error: either outputfile or -no_write must be specified\n");
				Help();
			}

			if (appJob.m_scale == 0 && appJob.m_width == 0 && !m_bLoopTest) {
				printf("  Error: either -scale or -size must be specified\n");
				Help();
			}

			m_appJobs.push_back(appJob);
		}

		if (m_bReadOnce && m_bBenchmark)
			ReadAppJobPics();

		if (m_bRemote && m_server=="0.0.0.0") {
			printf("  A valid IP address must be specified for the remote server\n");
			Help();
		}

		if (m_bLoopTest && m_appJobs.size() != 1)
			printf("  Error: loop test requires a single input file to be specified\n");
	}
}

void AppArgs::ParseJobsFile( string jobsFile )
{
	FILE *fp = fopen(jobsFile.c_str(), "r");
	if (fp == 0) {
		printf("Could not open jobs file '%s'\n", jobsFile.c_str());
		exit(1);
	}

	int lineNum = 0;
	char buf[1024];
	while (fgets(buf, 1024, fp)) {
		lineNum += 1;
		int argc = 0;
		char *argv[32];

		char * pBuf = buf;
		for (;;) {
			while (*pBuf == ' ' || *pBuf == '\t')
				pBuf += 1;

			if (*pBuf == '\0' || *pBuf == '\n')
				break;

			argv[argc++] = pBuf;

			while (*pBuf != ' ' && *pBuf != '\t' && *pBuf != '\n' && *pBuf != '\0')
				pBuf += 1;

			if (*pBuf == '\0')
				break;

			*pBuf++ = '\0';
		}

		if (argc == 0 || argv[0][0] == '#')
			continue;

		AppJob appJob;
		int argPos;
		int fileCnt = 0;
		for (argPos=0; argPos < argc; argPos++) {
			if (argv[argPos][0] == '-') {
				if (strcmp(argv[argPos], "-size") == 0) {
					argPos += 1;
					char * pEnd;
					appJob.m_width = strtol(argv[argPos], &pEnd, 10);
					if (!isdigit(*argv[argPos]) || *pEnd != '\0') {
						printf("Expected width value for -size\n");
						Help();
					}
					argPos += 1;
					appJob.m_height = strtol(argv[argPos], &pEnd, 10);
					if (!isdigit(*argv[argPos]) || *pEnd != '\0') {
						printf("Expected height value for -size\n");
						Help();
					}
				} else if (strcmp(argv[argPos], "-scale") == 0 || strcmp(argv[argPos], "-resize") == 0) {
					argPos += 1;
					char * pEnd;
					appJob.m_scale = strtol(argv[argPos], &pEnd, 10);
					if (!isdigit(*argv[argPos]) || *pEnd != '%') {
						printf("Expected scale value for -scale\n");
						Help();
					}
				} else {
					printf("Unknown command line switch: %s\n", argv[argPos]);
					Help();
				}
			} else {
				switch (fileCnt++) {
				case 0:
					appJob.m_inputFile = argv[argPos];
					break;
				case 1:
					appJob.m_outputFile = argv[argPos];
					break;
				}
			}
		}

		if (appJob.m_inputFile.size() == 0) {
			printf("  Error: job file line %d, inputfile not specified\n", lineNum);
			Help();
		}

		if (appJob.m_outputFile.size() == 0 && !m_bNoWrite) {
			printf("  Error: job file line %d, either outputfile or -no_write must be specified\n", lineNum);
			Help();
		}

		if (appJob.m_scale == 0 && appJob.m_width == 0) {
			printf("  Error: job file line %d, either -scale or -size must be specified\n", lineNum);
			Help();
		}
		appJob.m_mode= (m_bReadOnce ? MODE_INBUFF : MODE_INFILE) | (m_bNoWrite ? 0 : MODE_OUTFILE);
		m_appJobs.push_back(appJob);
	}

	fclose(fp);
}

void AppArgs::ReadAppJobPics()
{
	for (size_t i = 0; i < m_appJobs.size(); i += 1) {
		AppJob & appJob = m_appJobs[i];

		FILE * inFp;
		if (!(inFp = fopen(appJob.m_inputFile.c_str(), "rb"))) {
			printf("Could not open input file '%s'\n", appJob.m_inputFile.c_str());
			exit(1);
		}

		// read input file to memory buffer
		struct stat buf;
		fstat( fileno(inFp), &buf );

		appJob.m_inputBufferSize = (int)buf.st_size;
		// allocate an increment of 2 M
		int allocSize = (int)buf.st_size/(2*1024*1024)*(2*1024*1024)+(2*1024*1024);

		bool bFail = ht_posix_memalign( (void **)&appJob.m_inputBuffer, 2*1024*1024, allocSize);
		assert(!bFail);
		//appJob.m_inputBuffer = (uint8_t*)malloc(buf.st_size);

		if (fread( appJob.m_inputBuffer, 1, appJob.m_inputBufferSize, inFp ) != (size_t)appJob.m_inputBufferSize) {
			printf("Could not read input file '%s'\n", appJob.m_inputFile.c_str());
			exit(1);
		}

		fclose(inFp);
	}
}

void AppArgs::Help()
{
	printf("Command line syntax:\n");
	printf("   -help                    prints command line syntax\n");
	printf("   -threads <num>           sets number of threads creating work for coprocessor\n");
	printf("   -scale <num>%%            sets scaling factor for image\n");
	printf("   -size <width> <height>   sets width and height for image\n");
	printf("   -jobs <fileName>         sets name of jobs file\n");
	printf("   -benchmark <time>        sets duration of benchmark in seconds\n");
	//printf("   -read_once               sets flag to read input file only once\n");
	printf("   -no_write                sets flag to not write output image\n");
	printf("   -loop_test               executes a verification test looping through image sizes\n");
	printf("   -loop_test_ratio         executes a verification test looping through image sizes w/ fixed aspect ratio\n");
	printf("   -check                   performs verification on generated images\n");
	printf("   -host_only               executes only host code - used for performance verification\n");
	printf("   -daemon                  execute in daemon mode and accept resize requests from clients\n");
	printf("   -remote                  send resize request to a remote daemon for execution\n");
	printf("   -server                  IP to bind to in daemon mode, or address of remote server in client mode\n");
	printf("                            Default value is 0.0.0.0.  Must be specified for with -remote\n");
	printf("   -port                    TCP port to use for the daemon.  Default is 18810.\n");

	printf("\n");
	printf("Example command lines:\n");
	printf("   cnyJpegResize inputFile -scale 50%% outputFile\n");
	printf("   cnyJpegResize inputFile -size 324w 485h outputFile\n");
	printf("   cnyJpegResize -jobs jobsFile.txt -threads 8 -time 10 -no_read -no_write -check_output\n");

	printf("\n");
	printf("Note: all jobs in job file are performed once if -benchmark is not specified or\n");
	printf("  repeatedly if -benchmark is specified\n");

	printf("\n");
	printf("Example jobs file:\n");
	printf("  sample/1036.jpeg -scale 50%% /output/out1036.jpeg\n");
	printf("  sample/1036.jpeg -scale 40%% /output/out1036.jpeg\n");

	exit(1);
}
