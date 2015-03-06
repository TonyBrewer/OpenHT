#pragma once

#include <string>
#include <stdint.h>
#include <queue>


#define MODE_SERVER_READ 1 << 0
#define MODE_SERVER_WRITE 1 << 1
#define MODE_KEEP_ASPECT 1 << 2
#define MODE_MAGICK 1 << 3
#define MODE_NO_UPSCALE 1 << 4

#define DEFAULT_PORT 18811
#define DEFAULT_QUALITY 85

using namespace std;

struct job {
  int quality;
  int scale;
  int sizeX;
  int sizeY;
  uint32_t jobMode;
  string inputFile;
  string outputFile;
  string server;
  int port;
  int threads;
  bool batch;
  bool stats;
  
  job() {
    quality=DEFAULT_QUALITY;
    scale=0;
    sizeX=0;
    sizeY=0;
    jobMode=0;
    inputFile="";
    outputFile="";
    server="";
    port=DEFAULT_PORT;
    threads=1;
    batch=0;
    stats=0;
  };
};

queue<job> buildBatchQueue(job &globalJobArgs);
