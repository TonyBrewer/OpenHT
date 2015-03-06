/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <queue>
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <string.h>
#include "job.hpp"
#include "cmdArgs.hpp"

queue<job>  buildBatchQueue(job &globalJobArgs) {
  queue<job> jobQueue;
  vector<char *> argv;
  job jobArgs;
  string argLine;
  char *c;
  ifstream batchFile(globalJobArgs.inputFile);
  while(!batchFile.eof()) {
    //reset the argv vector for each line of the input file.  We don't 
    //want leftovers from previous lines.
    argv.clear();
    //argv[0] is the program name for normal argument processing.  Since we use
    //the same processing routine as for program invocation, we need actual
    //arguments to start at argv[1].  We don't care what is in argv[0]
    argv.push_back(NULL);
    if(!(getline(batchFile, argLine))) {
      continue;
    }
    //pre-populate the job arguments with the global options.  These can then
    //be overridden by the arguments from this line.
    jobArgs=globalJobArgs;
    //We don't want to keep the global input file as that is the job file.  Argument
    //processing puts the first file in inputFile only if it is empty so we need
    //to make sure it is.
    jobArgs.inputFile.clear();
    //The batch flag only makes sense for global arguments so we turn it off.  Leaving
    //it on would mean that the outputFile would never get set, among other things.
    jobArgs.batch=0;
    c=strtok((char *)argLine.c_str(), " ");
    while(c) {
      argv.push_back(c);
      c=strtok(NULL, " ");
    }
    parseArgs(jobArgs, argv.size(), argv.data());
#ifdef DEBUG
    printf("Adding job: %d\n", (int)(jobQueue.size()+1));
    printArgs(jobArgs);
#endif
    jobQueue.push(jobArgs);
  }
  return jobQueue;
}
