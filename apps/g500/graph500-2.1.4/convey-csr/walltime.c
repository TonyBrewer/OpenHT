// walltime.c - return walltime
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

double walltime ()
{
  double mic, time;
  double mega = 0.000001;
  struct timeval tp;
//  struct timezone tzp;
  (void) gettimeofday(&tp,NULL);
  time = (double) (tp.tv_sec);
  mic  = (double) (tp.tv_usec);
  time = (time + mic * mega);
  return(time);
}
