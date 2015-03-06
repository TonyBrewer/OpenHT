#pragma once
#include "job.hpp"

struct serverStats {
  uint64_t timeStamp;
  long long jobCount;
  long long jobTime;
};

serverStats getServerStats(job globalJobArgs);
void printStats(serverStats start, serverStats finish, long long clientJobCount);
inline void printStats(serverStats start, serverStats finish) { printStats(start, finish, 0); }