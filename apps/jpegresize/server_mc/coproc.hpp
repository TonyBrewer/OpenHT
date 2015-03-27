#pragma once
#include "JpegResizeHif.h"
#include "job.hpp"
#include "JobInfo.h"

class coproc {
public:
  coproc(int threads);
  ~coproc();
  bool resize(jobDefinition * job);
  inline void jobStats(double &seconds, long long &jobCnt, double &jobActiveAvg, double &jobTimeAvg) {
    if(resizeHif)
      resizeHif->jobStats(seconds, jobCnt, jobActiveAvg, jobTimeAvg);
  }
  inline void jobStats(uint64_t &timeStamp, long long &jobCount, long long &jobTime) {
    if(resizeHif) 
      resizeHif->jobStats(timeStamp, jobCount, jobTime);
  }
  inline JobInfo * outputDequeue(void) {return resizeHif->outputDequeue();};
    
private:
  JpegResizeHif * resizeHif;
  bool magickFallback(jobDefinition *job);
};
