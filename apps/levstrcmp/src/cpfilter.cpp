/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT levstrcmp application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Main.h"

#ifndef EXAMPLE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <limits.h>
#include <pthread.h>

#include "editdis.h"

static const char __attribute__((used)) svnid[] = "@(#)$Id: cpfilter.cpp 5497 2013-04-26 15:03:10Z mkelly $";

// #define DEBUG 

// --------------

bool Levenshtein_CP(unsigned char *strF, int origlenF, unsigned char *strS, int origlenS, int d, int filter_width)
{
  int lenF = 0;
  int lenS = 0;
  int i, j;
#ifndef SKIP_BAILOUT
  int mind;
#endif
  unsigned char lvmat[MAX_NAME_LEN+1][MAX_NAME_LEN+1];

  // NOTE edit distance d may have already been increased
  // to account for when we have truncated strings ...

  if(d < abs(origlenF - origlenS))
    return false;

  lenF = MXMIN(origlenF, filter_width);
  lenS = MXMIN(origlenS, filter_width);

#if 0
  if( (lenF > MAX_NAME_LEN) || (lenS > MAX_NAME_LEN) ){
    fprintf(stderr,"\nError, string length(s) > max (%d,%d max: %d)\n\n",lenF,lenS,MAX_NAME_LEN);
    exit(1);
  }
#endif

  for (i = 0; i <= lenF; i++)
    lvmat[i][0] = i;

  for (j = 0; j <= lenS; j++)
    lvmat[0][j] = j;

  for (j = 1; j <= lenS; j++)
  {
#ifndef SKIP_BAILOUT
    mind = INT_MAX;
#endif
    for (i = 1; i <= lenF; i++)
    {
      if (strF[i - 1] == strS[j - 1])
      {
        lvmat[i][j] = lvmat[i - 1][j - 1];
      }
      else
      {
        lvmat[i][j] = MXMIN(MXMIN(
          (lvmat[i - 1][j] + 1),      // deletion
          (lvmat[i][j - 1] + 1)),     // insertion
          (lvmat[i - 1][j - 1] + 1)); // substitution
      }
#ifndef SKIP_BAILOUT
      mind = MXMIN(mind, lvmat[i][j]);
#endif
    }
#ifndef SKIP_BAILOUT
    if(mind > d){
# if 0
      printf("bailing out ... %d %s %s\n",mind,strF,strS);
# endif
      return false;
    }
#endif
  }

#if 0
  printf("%d <%s> %d <%s> %d\n",lenF,strF,lenS,strS,lvmat[lenF][lenS]);
#endif

  if( lvmat[lenF][lenS] <= d )
    return true;

  return false;
}

// --------------

int coproc_filter(int lenL, int lenR, int adjD, int countL, int countR, int startL,
                  uint32_t bucketidL, uint32_t bucketidR,
                  unsigned char (*nameL)[COPROC_FILTER_WIDTH],
                  unsigned char (*nameR)[COPROC_FILTER_WIDTH], PAIRS *pairs_array)
{
  int i = 0;
  int j = 0;
  int npairs = 0;

  for(i=0;i<countL;i++){
    for(j=0;j<countR;j++){

      // NOTE: real cp hw does not return self-self hits, but this does so
      //       counts of cp hits and host compares may be different
      //       see INCLUDE_SELF sections in parent code ...

#ifdef DEBUG
      printf("adjD = %d comparing index %d %u %u <%s> with %d %u %u <%s>\n",adjD,
              startL+i,bucketidL,lenL,nameL[startL+i],j,bucketidR,lenR,nameR[j]);
#endif

      if( Levenshtein_CP( nameL[startL+i], lenL, nameR[j], lenR, adjD, COPROC_FILTER_SIZE ) ){
#if 0
        printf("Levenshtein_CP returns TRUE\n");
#endif
        pairs_array[npairs].indexL = startL+i;
        pairs_array[npairs].bucketidL = bucketidL;
        pairs_array[npairs].indexR = j;
        pairs_array[npairs].bucketidR = bucketidR;
        npairs++;
#if 0
      }else{
        printf("Levenshtein_CP returns FALSE\n");
#endif
      }

    }
  }

  return npairs;
}

#endif // EXAMPLE
