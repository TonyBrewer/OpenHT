/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT levstrcmp application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>
#include <omp.h>

#include "levstrcmp.h"

static const char __attribute__((used)) svnid[] = "@(#)$Id: procname.cpp 5720 2013-05-31 18:35:11Z mkelly $";

// #define PRINT_INDEXES
// #define DEBUG_EDITDIS
// #define SKIP_BAILOUT

static uint64_t edis_called = 0;

typedef struct{
  uint64_t l;
  uint64_t r;
}HITS;
HITS *hits[MAX_CPUS];
uint64_t my_hits[MAX_CPUS];

// turn off inline below to get timing with call overhead ...

#define inline_type inline __attribute__((__always_inline__))
// #define inline_type inline __attribute__((__noinline__))

// --------------

static inline_type bool Damerau_Levenshtein( unsigned char *strF, int lenF, unsigned char *strS, int lenS, int edis )
{
  int w = 1;
  int s = 1;
  int a = 1;
  int d = 1;
  unsigned char lrow0[MAX_NAME_LEN+1];
  unsigned char lrow1[MAX_NAME_LEN+1];
  unsigned char lrow2[MAX_NAME_LEN+1];
  unsigned char *row0;
  unsigned char *row1;
  unsigned char *row2;
  int i, j;
#ifndef SKIP_BAILOUT
  int mind;
#endif

  if(!trimmed_strings){
    while ( lenF && strF[lenF-1]==' ' ) lenF--;
    while ( lenS && strS[lenS-1]==' ' ) lenS--; // Trim incoming strings
  }

  if(edis < abs(lenF - lenS))
    return false;

#if 0
  if( (lenF > MAX_NAME_LEN) || (lenS > MAX_NAME_LEN) ){
    fprintf(stderr,"\nError, string length(s) > max (%d,%d max: %d)\n\n",lenF,lenS,MAX_NAME_LEN);
    exit(1);
  }
#endif

  row0 = &lrow0[0];
  row1 = &lrow1[0];
  row2 = &lrow2[0];

  for (j = 0; j <= lenS; j++) {
    row1[j] = j * a;
  }

  for (i = 0; i < lenF; i++) {

    unsigned char *dummy;
    row2[0] = (i + 1) * d;

#ifndef SKIP_BAILOUT
    mind = INT_MAX;
#endif

    for (j = 0; j < lenS; j++) {

      /* substitution */
      row2[j + 1] = row1[j] + s * (strF[i] != strS[j]);

      /* swap */
      if (i > 0 && j > 0 && strF[i - 1] == strS[j] && strF[i] == strS[j - 1] && 
          row2[j + 1] > row0[j - 1] + w) {
        row2[j + 1] = row0[j - 1] + w;
      }

      /* deletion */
      if (row2[j + 1] > row1[j + 1] + d) {
        row2[j + 1] = row1[j + 1] + d;
      }

      /* insertion */
      if (row2[j + 1] > row2[j] + a) {
        row2[j + 1] = row2[j] + a;
      }

#ifndef SKIP_BAILOUT
      mind = MXMIN(mind, row2[j+1]);
#endif

    }

#ifndef SKIP_BAILOUT
    if(mind > edis){
# if 0
      printf("bailing out ... %d %d %d %s %s\n",mind,i,j,strF,strS);
# endif
      return false;
    }
#endif

    dummy = row0;
    row0 = row1;
    row1 = row2;
    row2 = dummy;

  }

  if(row1[lenS] <= edis)
    return true;

  return false;
}

// --------------

static inline_type bool Levenshtein( unsigned char *strF, int lenF, unsigned char *strS, int lenS, int edis )
{
  int i, j;
#ifndef SKIP_BAILOUT
  int mind;
#endif
  unsigned char lvmat[MAX_NAME_LEN+1][MAX_NAME_LEN+1];

  if(!trimmed_strings){
    while ( lenF && strF[lenF-1]==' ' ) lenF--;
    while ( lenS && strS[lenS-1]==' ' ) lenS--; // Trim incoming strings
  }

  if(edis < abs(lenF - lenS))
    return false;

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
    if(mind > edis){
# if 0
      printf("bailing out ... %d %d %d %s %s\n",mind,i,j,strF,strS);
# endif
      return false;
    }
#endif
  }

  if( lvmat[lenF][lenS] <= edis )
    return true;

  return false;
}

// --------------

static inline_type bool Levenshtein2Bvec( unsigned char hashIdxL, unsigned char *bitvecL, 
                                          unsigned char hashIdxR, unsigned char *bitvecR, 
                                          unsigned char *nameL, int lenL, unsigned char *nameR, int lenR )
{
#ifdef SIMPLE_HASH
  uint8_t bhashR = lnameR->hrec.bhash;
  if( ((lnameL->hrec.bvec[bhashR >> 5] >> (bhashR & 31)) & 1) != 0) {
    uint8_t bhashL = lnameL->hrec.bhash;
    if( ((lnameR->hrec.bvec[bhashL >> 5] >> (bhashL & 31)) & 1) != 0) {
#else
  // unsigned char hashIdxR = lnameR->m_hashIdx;
  // if( (lnameL->bloom.m_mask[hashIdxR >> 3] & (1ull << (hashIdxR & 0x7))) != 0) {
  //   unsigned char hashIdxL = lnameL->m_hashIdx;
  //   if( (lnameR->bloom.m_mask[hashIdxL >> 3] & (1ull << (hashIdxL & 0x7))) != 0) {
  if( (bitvecL[hashIdxR >> 3] & (1ull << (hashIdxR & 0x7))) != 0) {
    if( (bitvecR[hashIdxL >> 3] & (1ull << (hashIdxL & 0x7))) != 0) {
#endif
      edis_called++;
      return(Levenshtein(nameL, lenL, nameR, lenR, edistance));
    }
  }

  return false;
}

// --------------

void shuffle(uint64_t *array, uint64_t nelems)
{
#if 1
  // Fisher-Yates Shuffle ...
  uint64_t i, j, m;

  for(i=0;i<nelems;i++){
    array[i] = i;
  }

  m = nelems;
  while(m > 0){
    i = rand() % m--;
    j = array[m];
    array[m] = array[i];
    array[i] = j;
  }

# if 0
  for(i=0;i<nelems;i++){
    printf("array[%5lu] = %5lu\n",i,array[i]);
  }
# endif
#else
  uint64_t i, j;

  for(i=0;i<nelems;i++){
    array[i] = nelems+1;
  }

  i = 0;
  j = 0;
  while(i < nelems){
    j = rand() % nelems;
    if(array[j] == (nelems+1)){
      array[j] = i;
      i++;
    }
  }

# if 0
  for(i=0;i<nelems;i++){
    printf("array[%5lu] = %5lu\n",i,array[i]);
  }
# endif
#endif

  return;
}

// --------------

void process_names(int use_bvec)
{
  int k = 0;
  uint64_t i = 0;
  uint64_t j = 0;
  uint64_t ix = 0;
  uint64_t px = 0;
  double edis_pct = 0.0;
  double total_compares = 0.0;
  double pn_time = 0.0;
  uint64_t sc, ec;
  uint64_t *lhs_array = NULL;
  uint64_t *rhs_array = NULL;
  uint64_t num_hits = 0;
  int formatlen = 0;

  int num_threads = omp_get_max_threads();

  if(num_threads != num_cpus){
    fprintf(stderr,"\nError, num threads (%d) does not equal requested value (%d)\n\n",num_threads,num_cpus);
    exit(1);
  }

  if( (num_lhs <= 0) || (num_lhs >= (long)tot_num_names) ){
    num_lhs = (long)tot_num_names;
  }

  lhs_array = (uint64_t *)malloc(tot_num_names * sizeof(uint64_t));
  if(lhs_array == NULL){
    fprintf(stderr,"\nError, unable to alloc mem for lhs rand list (%lu)\n\n",tot_num_names);
    exit(1);
  }
  mem_curr += (tot_num_names * sizeof(uint64_t));
  mem_hiwater = MXMAX(mem_curr,mem_hiwater);
  if(auto_seed){
    int pid = getpid();
    srand((unsigned int)pid);
    printf("lhs auto seed      = %d\n",pid);
  }else{
    srand(101U);
  }

  shuffle(lhs_array,tot_num_names);

  rhs_array = (uint64_t *)malloc(tot_num_names * sizeof(uint64_t));
  if(rhs_array == NULL){
    fprintf(stderr,"\nError, unable to alloc mem for rhs rand list (%lu)\n\n",tot_num_names);
    exit(1);
  }
  mem_curr += (tot_num_names * sizeof(uint64_t));
  mem_hiwater = MXMAX(mem_curr,mem_hiwater);
  if(auto_seed){
    int pid = time(NULL) % (5 * getpid());
    srand((unsigned int)pid);
    printf("rhs auto seed      = %d\n",pid);
  }else{
    srand(2034U);
  }

  shuffle(rhs_array,tot_num_names);

  printf("num lhs names      = %lu\n",num_lhs);
  fflush(stdout);

  formatlen = 2;
  if( (num_lhs >= 100) && (num_lhs < 1000) ){
    formatlen = 3;
  }else if( (num_lhs >= 1000) && (num_lhs < 10000) ){
    formatlen = 4;
  }else if( (num_lhs >= 10000) && (num_lhs < 100000) ){
    formatlen = 5;
  }else if( (num_lhs >= 100000) && (num_lhs < 1000000) ){
    formatlen = 6;
  }else if( (num_lhs >= 1000000) && (num_lhs < 10000000) ){
    formatlen = 7;
  }else if( (num_lhs >= 10000000) && (num_lhs < 100000000) ){
    formatlen = 8;
  }else if( (num_lhs >= 100000000) && (num_lhs < 1000000000) ){
    formatlen = 9;
  }else{
    formatlen = 10;
  }

  for(k=0;k<num_cpus;k++){
    hits[k] = (HITS *)malloc(tot_num_names * sizeof(HITS));
    if(hits[k] == NULL){
      fprintf(stderr,"\nError, unable to alloc mem for hits array (%lu)\n\n",tot_num_names);
      exit(1);
    }
  }

  sc = (*rdtsc0)();

  if(use_bvec > 0){

    // NOTE: We do not bail out if (lenL - LenR) check fails edistance
    //       This means many calls return quickly
    //       The coproc application only compares strings that pass the length check

    for(i=0;i<(uint64_t)num_lhs;i++){
      if( (i > 0) && ((i % 1000) == 0) ){
        printf("completed %*lu outer loops\n",formatlen,i);
        fflush(stdout);
      }
      for(k=0;k<num_cpus;k++){
        my_hits[k] = 0;
      }
      ix = lhs_array[i];
      // parallel loop
#pragma omp parallel for shared(lnamerec, ix, num_hits, hits, my_hits, doio)
      for(j=0;j<tot_num_names;j++){
        bool b;
        int thrid = omp_get_thread_num();
        uint64_t jx = rhs_array[j];
        b = Levenshtein2Bvec( lnamerec[ix].m_hashIdx, lnamerec[ix].bloom.m_mask, 
                              lnamerec[jx].m_hashIdx, lnamerec[jx].bloom.m_mask, 
                              lnamerec[ix].lname, lnamerec[ix].len, lnamerec[jx].lname, lnamerec[jx].len );
        if(b){
#pragma omp atomic
          num_hits++;
#if 0
          printf("hit: %lu <%s> to %lu <%s>\n",ix,lnamerec[ix].lname,j,lnamerec[jx].lname);
#endif
          if(doio){
            hits[thrid][my_hits[thrid]].l = ix;
            hits[thrid][my_hits[thrid]].r = jx;
            my_hits[thrid]++;
          }
        }
#if 0
        __asm__ __volatile__("clflush (%0)" :: "r" (&lnamerec[ix].m_hashIdx));
        __asm__ __volatile__("clflush (%0)" :: "r" (&lnamerec[jx].m_hashIdx));
#endif
      }
      total_compares += tot_num_names;
      for(k=0;k<num_cpus;k++){
        if(my_hits[k] > 0){
          for(px=0;px<my_hits[k];px++){
#ifdef PRINT_INDEXES
            fprintf(outfp,"%lu,%lu\n",hits[k][px].l+1,hits[k][px].r+1);
#else
            fprintf(outfp,"%s %s\n",lnamerec[hits[k][px].l].lname, lnamerec[hits[k][px].r].lname);
#endif
          }
        }
      }
    }

  }else{ // use_bvec > 0

    // NOTE: We do not bail out if (lenL - LenR) check fails edistance
    //       This means many calls return quickly
    //       The coproc application only compares strings that pass the length check

    for(i=0;i<(uint64_t)num_lhs;i++){
      if( (i > 0) && ((i % 1000) == 0) ){
        printf("completed %*lu outer loops\n",formatlen,i);
        fflush(stdout);
      }
      for(k=0;k<num_cpus;k++){
        my_hits[k] = 0;
      }
      ix = lhs_array[i];
      // parallel loop
#pragma omp parallel for shared(lnamerec, ix, num_hits, hits, my_hits, doio)
      for(j=0;j<tot_num_names;j++){
        bool b;
        int thrid = omp_get_thread_num();
        uint64_t jx = rhs_array[j];
        b = Levenshtein(lnamerec[ix].lname, lnamerec[ix].len, lnamerec[jx].lname, lnamerec[jx].len, edistance);
        // b = Damerau_Levenshtein(lnamerec[ix].lname, lnamerec[ix].len, lnamerec[jx].lname, lnamerec[jx].len, edistance);
        if(b){
#pragma omp atomic
          num_hits++;
#if 0
          printf("hit: %lu <%s> to %lu <%s>\n",ix,lnamerec[ix].lname,j,lnamerec[jx].lname);
#endif
          if(doio){
            hits[thrid][my_hits[thrid]].l = ix;
            hits[thrid][my_hits[thrid]].r = jx;
            my_hits[thrid]++;
          }
        }
#if 0
        __asm__ __volatile__("clflush (%0)" :: "r" (lnamerec[ix].lname));
        __asm__ __volatile__("clflush (%0)" :: "r" (lnamerec[jx].lname));
#endif
      }
      total_compares += tot_num_names;
      for(k=0;k<num_cpus;k++){
        if(my_hits[k] > 0){
          for(px=0;px<my_hits[k];px++){
#ifdef PRINT_INDEXES
            fprintf(outfp,"%lu,%lu\n",hits[k][px].l+1,hits[k][px].r+1);
#else
            fprintf(outfp,"%s %s\n",lnamerec[hits[k][px].l].lname, lnamerec[hits[k][px].r].lname);
#endif
          }
        }
      }
    }

  } // use_bvec > 0

  ec = (*rdtsc0)();
  pn_time = (double)((double)ec - (double)sc) / clocks_per_sec;

  if(lhs_array != NULL){
    free(lhs_array);
    lhs_array = NULL;
  }

  if(rhs_array != NULL){
    free(rhs_array);
    rhs_array = NULL;
  }

  for(k=0;k<num_cpus;k++){
    if(hits[k] != NULL){
      free(hits[k]);
      hits[k] = NULL;
    }
  }

  printf("total edit calls   = %lu\n",(uint64_t)total_compares);
  printf("total edit hits    = %lu\n",num_hits);

  if(use_bvec > 0){
    edis_pct = ((double)edis_called * 100.0) / total_compares;
    printf("num act edit calls = %lu %.3lf%%\n",edis_called,edis_pct);
  }

  printf("process names time = %.5lf (sec)\n",pn_time);
  printf("compare avg time   = %.5lf (ns)\n",(pn_time * 1000000000.0) / total_compares);

  return;
}
