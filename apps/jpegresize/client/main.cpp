/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <queue>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "cmdArgs.hpp"
#include "resize.hpp"
#include "serverStats.hpp"

int main(int argc, char *argv[]) {
  job globalJobArgs;
  serverStats start;
  serverStats finish;
  long jobCount=1;
  if(!parseArgs(globalJobArgs, argc, argv)) {
    exit(1);
  }
  //printArgs(globalJobArgs);
  if(globalJobArgs.stats) {
    start=getServerStats(globalJobArgs);
  }
  if(globalJobArgs.batch) {
    jobCount=batchResize(globalJobArgs);
  } else {
    resize(globalJobArgs);
  }
  if(globalJobArgs.stats) {
    finish=getServerStats(globalJobArgs);
    printf("\ncumulative server stats:\n");
    printStats(start, finish);
    printf("\nthis client stats:\n");
    printStats(start, finish, jobCount);
  }
}
