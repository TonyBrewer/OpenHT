/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "cmdArgs.hpp"

void printHelp(void);
void printVersion(void);

struct arguments appArguments;

bool parseArgs(int argc, char *argv[]) {
  int longIndex;
  cmdLineArg opt;
  char *endptr;
  //Enable display of invalid argument messages
  opterr=1;
  //Since we may be processing batch mode options, we need to make sure to reset the getopt_long
  //state to the start
  optind=1;
  //Loop while there are still arguments to process.  The various case blocks should be pretty
  //self explanitory.  In cases where integer values as expected, the string to integer conversion
  //is performed and checked to ensure a value numeric value was given.
  while((opt=(cmdLineArg)getopt_long(argc, argv, shortOpts, longOpts, &longIndex ))!=-1) {
    switch(opt) {
      case argVersion:
	printVersion();
	return false;
	break;
      case argUnknown:
      case argHelp:
	printHelp();
	return false;
	break;
      case argInterface:
	appArguments.iface=optarg;
	break;
      case argPort:
	appArguments.port=strtol(optarg, &endptr,0);
	if(*endptr) {
	  printf("Invalid port: %s\n", optarg);
	  printHelp();
	  return false;
	} else if (appArguments.port==0) {
	  printf("--port requires a numeric argument\n");
	  printHelp();
	  return false;	  
	}
	break;
      case argNoServerRead:
	appArguments.noRead=true;
	break;
      case argNoServerWrite:
	appArguments.noWrite=true;
	break;
      case argDefaultQuality:
	appArguments.quality=strtol(optarg, &endptr,0);
	if(*endptr) {
	  printf("Invalid quality value: %s\n", optarg);
	  printHelp();
	  return false;	  
	} else if (appArguments.quality==0) {
	  printf("--default_quality requires a numeric argument\n");
	  printHelp();
	  return false;	  
	}	
	break;
      case argThreads:
	appArguments.threads=strtol(optarg, &endptr,0);
	if(*endptr) {
	  printf("Invalid thread count: %s\n", optarg);
	  printHelp();
	  return false;	  
	} else if (appArguments.threads==0) {
	  printf("--threads requires a numeric argument\n");
	  printHelp();
	  return false;	  
	} else if (appArguments.threads>80) {
	  printf("Maximum number of threads allowed is 80\n");
	  return false;
	}
	break;
      case argNoMagick:
	appArguments.noMagick=true;
	break;
      case argSyslog:
	appArguments.syslog=true;
	break;
      case argStats:
	appArguments.stats=strtol(optarg, &endptr,0);
	if(*endptr) {
	  printf("Invalid stats interval: %s\n", optarg);
	  printHelp();
	  return false;	  
	} else if (appArguments.stats==0) {
	  printf("--stats requires a numeric argument\n");
	  printHelp();
	  return false;	  
	}	
	break;
      default:
	printf("unknown option\n");
	break;
    }
  }
  //Argument sanity checks go here.
  return true;
}

void printHelp(void) {
  printf("jrd [options...]\n");
  printf("     --version            print version info\n");
  printf("     --help               print this help\n");
  printf("     --interface <ip>     ip address of interface to listen on (default all)\n");
  printf("     --port <num>         TCP port number (default 18811)\n");
  printf("     --no_server_read     disable direct read of input files\n");
  printf("     --no_server_write    disable direct write of output files\n");
  printf("     --threads <n>        number of threads\n");
  printf("     --no_magick_fallback do not use ImageMagick libraries to process images \n");
  printf("                          that cannot be processed by the coprocessor (return\n");
  printf("                          error instead)\n");
//klc not currently implemented
//  printf("     --syslog             log errors to syslog\n");
//klc not currently implemented
//  printf("     --default_quality    quality of output image (default 85)\n");
  printf("\n");
}

void printVersion(void) {
  printf("version 1.2.0\n");
}

void printArgs(void) {
  printf("interface: %s\n", appArguments.iface.c_str());
  printf("    port: %d\n", appArguments.port);
  printf(" quality: %d\n", appArguments.quality);
  printf(" threads: %d\n", appArguments.threads);
  printf("  noRead: %d\n", appArguments.noRead);
  printf(" noWrite: %d\n", appArguments.noWrite);
  printf("noMagick: %d\n", appArguments.noMagick);
  printf("  syslog: %d\n", appArguments.syslog);
  return;
}
