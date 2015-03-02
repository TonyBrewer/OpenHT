/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT levstrcmp application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Ht.h"
using namespace Ht;

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

static const char __attribute__((used)) svnid[] = "@(#)$Id: procnam.cpp 6200 2014-05-09 19:35:16Z mruff $";
 
#define MAX_AU                 64
#define MAX_ARGS               10
#define NUM_OUTSTANDING_CALLS  3
#define PRINT_INTERVAL_SEC     30

struct BUCKETSUBMAP{
  uint32_t bucketidL;
  uint32_t lenL;
  uint32_t numL;
  unsigned char (*bucketaddrL)[COPROC_FILTER_WIDTH];
  uint32_t bucketidR;
  uint32_t lenR;
  uint32_t numR;
  unsigned char (*bucketaddrR)[COPROC_FILTER_WIDTH];
  struct BUCKETSUBMAP *next;
};
typedef struct{
  struct BUCKETSUBMAP *frec;
  struct BUCKETSUBMAP *crec;
  struct BUCKETSUBMAP *lrec;
}BUCKETMAP;
static BUCKETMAP bucketmap[MAX_CPUS][MAX_AU];

static uint64_t num_cp_compares[MAX_CPUS];
static uint64_t num_edis_called[MAX_CPUS];
static uint64_t num_edis_printed[MAX_CPUS];
static uint64_t num_hits_sent[MAX_CPUS];
static uint64_t num_hits_recvd[MAX_CPUS];

union CStrPair {
  struct {
    uint64_t m_conIdL:16;
    uint64_t m_conIdxL:16;
    uint64_t m_conIdR:16;
    uint64_t m_conIdxR:16;
  };
  uint64_t m_data64;
};

CHtHif *m_pHtHif = NULL;
CHtAuUnit **ppAuUnit = NULL;

static int global_auCnt = 0;

static uint64_t prev_compare_cnt = 0;
static uint64_t running_compare_cnt = 0;
static uint64_t running_print_cnt = 0;

static int print_status_cnt = 50000;
static int print_status_timer = 0;
static uint64_t pn_prev_time = 0;
static uint64_t pn_start_time = 0;
static int print_status_estcomp = 0;
static uint64_t print_time = 0;

static uint64_t max_compares = 0;

// #define DEBUG
// #define DEBUG2
// #define PAR_DEBUG
// #define PAR_DEBUG2
// #define PAR_DEBUG4
// #define DEBUG_RESULTS

// #define SKIP_IO
// #define DONT_SKIP_DUPLICATES
// #define SKIP_BAILOUT

// to be consistent for all methods ...
#define INCLUDE_SELF

static pthread_mutex_t memst_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t outfp_lock = PTHREAD_MUTEX_INITIALIZER;

#if defined(PAR_DEBUG) || defined(PAR_DEBUG2)
static pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;
static long gettid(void) { return (long)syscall( __NR_gettid ); }
#endif

// turn off inline below to get timing with call overhead ...

#define inline_type inline __attribute__((__always_inline__))
// #define inline_type inline __attribute__((__noinline__))

// --------------

static inline_type bool Levenshtein( unsigned char *strF, int lenF, unsigned char *strS, int lenS, int edis )
{
  int i, j;
#ifndef SKIP_BAILOUT
  int mind;
#endif
  unsigned char lvmat[MAX_NAME_LEN+1][MAX_NAME_LEN+1];

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

void process_hits(int idx, HITS *hits, int num_hits)
{
  int i = 0;
  int print_status_rollover = 0;
  uint64_t loc_edis_printed = 0;
  uint64_t pn_curr_time = 0;
  double time_sofar = 0.0;
  double rccm = 0.0;
  double pct_complete = 0.0;
  char pct_str[256] = { "" };
  double rate_sofar = 0.0;
  double est_secs = 0.0;
  uint64_t inst_compare_cnt = 0;
  double time_delta = 0.0;
  double rate_inst = 0.0;

  if(hits == NULL){
    return;
  }

  if(num_hits <= 0){
    return;
  }

  num_hits_recvd[idx] += num_hits;

  for(i=0;i<num_hits;i++){
#if 0
    if(hits[i].indx2L != hits[i].indx2R)
      printf("hit: %s %s\n",&lastnames[hits[i].indexL], &lastnames[hits[i].indexR]);
#endif
    if(hits[i].hit == 0){
      num_edis_called[idx]++;
      if( Levenshtein( &lastnames[hits[i].indexL], hits[i].lenL, &lastnames[hits[i].indexR], hits[i].lenR, hits[i].d ) ){
        hits[i].hit = 1;
      }
    }
  }

  // TODO - revisit locking for bitvec method ...

  pthread_mutex_lock(&outfp_lock);

  loc_edis_printed = 0;

  for(i=0;i<num_hits;i++){

    if(hits[i].hit){

      loc_edis_printed++;

#ifdef DEBUG_RESULTS
      printf("strings <%lu : %s> <%lu : %s> within distance %d\n", hits[i].indx2L+1, &lastnames[hits[i].indexL],
                                                                   hits[i].indx2R+1, &lastnames[hits[i].indexR],
                                                                   hits[i].d );
      fflush(stdout);
#endif

      if(!skip_bitvec){

#ifdef SIMPLE_HASH
        uint8_t bhash = bitvec[hits[i].indx2R].bhash;
        bitvec[hits[i].indx2L].bvec[bhash >> 5] |= (uint32_t)(1U << (bhash & 31));
#else
        SetBloomList(hits[i].indx2L, hits[i].indx2R);
#endif

#ifndef SKIP_IO
      }else{

        fprintf(outfp,"%-*s %-*s\n",max_name_len, &lastnames[hits[i].indexL],
                max_name_len, &lastnames[hits[i].indexR]);
#endif
      }

#ifndef DONT_SKIP_DUPLICATES
      if(hits[i].lenL != hits[i].lenR){

        loc_edis_printed++;

# ifdef DEBUG_RESULTS
        printf("reverse <%lu : %s> <%lu : %s> within distance %d\n", hits[i].indx2R+1, &lastnames[hits[i].indexR],
                                                                     hits[i].indx2L+1, &lastnames[hits[i].indexL],
                                                                     hits[i].d );
        fflush(stdout);
# endif

        if(!skip_bitvec){

# ifdef SIMPLE_HASH
          uint8_t bhash = bitvec[hits[i].indx2L].bhash;
          bitvec[hits[i].indx2R].bvec[bhash >> 5] |= (uint32_t)(1U << (bhash & 31));
# else
          SetBloomList(hits[i].indx2R, hits[i].indx2L);
# endif

#ifndef SKIP_IO
        }else{

          fprintf(outfp,"%-*s %-*s\n",max_name_len, &lastnames[hits[i].indexR],
                  max_name_len, &lastnames[hits[i].indexL]);
#endif
        }

      }
#endif // DONT_SKIP_DUPLICATES

    }

  }

  num_edis_printed[idx] += loc_edis_printed;
  __sync_fetch_and_add(&running_print_cnt, loc_edis_printed);

  // if -o - then skip status update code below ...

  if(outfp == stdout){
    pthread_mutex_unlock(&outfp_lock);
    return;
  }

  print_status_rollover = 1;
  if(no_filter){
    print_status_rollover = 20 * num_cpus;
  }

  if(print_status_cnt > print_status_rollover){

    pn_curr_time = (*rdtsc0)();
    time_delta = (double)((double)pn_curr_time - (double)print_time) / clocks_per_sec;

    if(time_delta >= (double)PRINT_INTERVAL_SEC){

      print_time = pn_curr_time;

      pct_complete = ((double)running_compare_cnt / (double)max_compares) * 100.0;
      if(pct_complete < 0.00010){
        sprintf(pct_str,"(%.1le%%)",pct_complete);
      }else if(pct_complete < 10.0){
        sprintf(pct_str,"(%.5lf%%)",pct_complete);
      }else{
        sprintf(pct_str,"(%.4lf%%)",pct_complete);
      }

      if(print_detail > 1){
        rccm = (double)running_compare_cnt / 1000000.0;
        if(rccm < 1000.0){
          printf("compares completed ... %6.2lf (M) %s\n",(rccm/1.0),pct_str);
        }else if(rccm < 1000000.0){
          printf("compares completed ... %6.2lf (G) %s\n",(rccm/1000.0),pct_str);
        }else if(rccm < 1000000000.0){
          printf("compares completed ... %6.2lf (T) %s\n",(rccm/1000000.0),pct_str);
        }else if(rccm < 1000000000000.0){
          printf("compares completed ... %6.2lf (P) %s\n",(rccm/1000000000.0),pct_str);
        }else{
          printf("compares completed ... %6.2lg     %s\n",(rccm/1.0),pct_str);
        }
      }else{
        printf("compares completed ... %s\n",pct_str);
      }

      print_status_timer++;
      if(print_status_timer >= 5){

        pn_curr_time = (*rdtsc0)();
        time_sofar = (double)((double)pn_curr_time - (double)pn_start_time) / clocks_per_sec;
        rate_sofar = ((double)running_compare_cnt / 1000000.0) / time_sofar;

        if(print_detail > 1){
          if(pn_prev_time == 0){
            printf("approx compare rate .. %9.2lf (M/sec) (inst %9.2lf)\n",rate_sofar,rate_sofar);
          }else if(running_compare_cnt <= prev_compare_cnt){
            printf("approx compare rate .. %9.2lf (M/sec) (inst n/a)\n",rate_sofar);
          }else{
            inst_compare_cnt = running_compare_cnt - prev_compare_cnt;
            time_delta = (double)((double)pn_curr_time - (double)pn_prev_time) / clocks_per_sec;
            rate_inst = ((double)inst_compare_cnt / 1000000.0) / time_delta;
            printf("approx compare rate .. %9.2lf (M/sec) (inst %9.2lf)\n",rate_sofar,rate_inst);
          }
        }

        printf("num hits found     ... %.3lf (M)\n",(double)running_print_cnt / 1000000.0);

        print_status_estcomp++;
        if(print_status_estcomp >= 3){
          if(time_sofar < 60.0){
            printf("time spent so far  ... %.4lf (sec)\n",time_sofar);
          }else if(time_sofar < 3600.0){
            printf("time spent so far  ... %.4lf (min)\n",time_sofar / 60.0);
          }else{
            printf("time spent so far  ... %.4lf (hrs)\n",time_sofar / 3600.0);
          }
          est_secs = ((double)max_compares / 1000000.0) / rate_sofar;
          est_secs -= time_sofar;
          if(est_secs < 0.0){
            est_secs = 100.0;
          }
          if(est_secs < 60.0){
            printf("est. time remaining .. %.4lf (sec)\n",est_secs);
          }else if(est_secs < 3600.0){
            printf("est. time remaining .. %.4lf (min)\n",est_secs / 60.0);
          }else{
            printf("est. time remaining .. %.4lf (hrs)\n",est_secs / 3600.0);
          }

          print_status_estcomp = 0;
        }

        pn_prev_time = pn_curr_time;
        prev_compare_cnt = running_compare_cnt;
        print_status_timer = 0;
      }

      fflush(stdout);

    }

    print_status_cnt = 0;

  }else{

    print_status_cnt++;

  }

  pthread_mutex_unlock(&outfp_lock);

  return;
}

// --------------

void thr_process_names(long lidx)
{
  int idx = (int)lidx;
  int d = 0;
  int d2 = 0;
  uint32_t i = 0;
  uint32_t j = 0;
  int k = 0;
  int cnt = 0;
  int lenL = 0;
  int lenR = 0;
  int auId = 0;
  int64_t numL = 0;
  uint64_t startL = 0;
  uint64_t totnumL = 0;
  struct BUCKETSUBMAP *tmp_crec = NULL;
  BUCKETMAP thrmap[MAX_AU];
  struct BUCKET *tmp_crecL = NULL;
  struct BUCKET *tmp_crecR = NULL;
  int num_hits = 0;
  HITS *hits = NULL;
  int num_pairs = 0;
  PAIRS *pairs = NULL;
  uint32_t indexL = 0;
  uint32_t indexR = 0;
  uint32_t bucketidL = 0;
  uint32_t bucketidR = 0;
  CHtAuUnit::CCallArgs_htmain call_argv;
  uint64_t rtn_argv;
  uint64_t blkdata;
  CStrPair cpair;
  int called[MAX_AU];
  int finished_sending[MAX_AU];
  int not_done = 0;
  int num_data = 0;
  int num_sent = 0;
  uint64_t num_compare = 0;
  uint64_t thr_hits_sent[MAX_AU];
  uint64_t thrtot_hits_sent = 0;
  int num_chunks = 0;
  int calls_issued = 0;

  hits = (HITS *)malloc((NUM_BLOCK_HITS+1) * sizeof(HITS));
  if(hits == NULL){
    fprintf(stderr,"\nError, unable to alloc mem for hits data (%d)\n\n",(NUM_BLOCK_HITS+1));
    exit(1);
  }

  pthread_mutex_lock(&memst_lock);
  mem_curr += (NUM_BLOCK_HITS+1) * sizeof(HITS);
  mem_hiwater = MXMAX(mem_curr,mem_hiwater);
  pthread_mutex_unlock(&memst_lock);

  if(!no_coproc){

    if(m_pHtHif == NULL){
      fprintf(stderr,"\nError, Hif is NULL\n\n");
      exit(1);
    }

    num_data = 0;
    for(auId=0;auId<global_auCnt;auId++){
      called[auId] = 0;
      thr_hits_sent[auId] = 0;
      finished_sending[auId] = 0;
      thrmap[auId].frec = bucketmap[idx][auId].frec;
      thrmap[auId].crec = bucketmap[idx][auId].frec;
      if(thrmap[auId].frec != NULL){
        num_data++;
      }
    }

#ifdef PAR_DEBUG
    pthread_mutex_lock(&print_lock);
    printf("mck info: %ld %d num_data=%d\n", gettid(), idx, num_data);
    fflush(stdout);
    pthread_mutex_unlock(&print_lock);
#endif

    if(num_data <= 0){

      free(hits);
      pthread_mutex_lock(&memst_lock);
      mem_curr -= (NUM_BLOCK_HITS+1) * sizeof(HITS);
      pthread_mutex_unlock(&memst_lock);

#ifdef PAR_DEBUG
      pthread_mutex_lock(&print_lock);
      printf("mck rtn:  %ld %d thread ending - no data\n", gettid(), idx);
      fflush(stdout);
      pthread_mutex_unlock(&print_lock);
#endif
      return;

    }

    num_sent = 0;
    not_done = 1;
    while(not_done){

      for(auId=0;auId<global_auCnt;auId++){

        if(thrmap[auId].frec != NULL){

#ifdef PAR_DEBUG4
          pthread_mutex_lock(&print_lock);
          printf("mck status: %ld %d auId=%d called=%d finish_sending=%d num_data=%d num_sent=%d\n",
                  gettid(), idx, auId, called[auId], finished_sending[auId],
                  num_sent, num_data);
          fflush(stdout);
          pthread_mutex_unlock(&print_lock);
#endif

          if(ppAuUnit[auId]->PeekHostData(blkdata)){
       
#ifdef PAR_DEBUG2
            pthread_mutex_lock(&print_lock);
            printf("mck fifo: %ld %d auId=%d\n", gettid(), idx, auId);
            fflush(stdout);
            pthread_mutex_unlock(&print_lock);
#endif

            // ----------------------

            cnt = 0;
            while( (cnt < 10000) && (ppAuUnit[auId]->RecvHostData(1, &blkdata) == 1)){
       
              cpair.m_data64 = blkdata;
       
              indexL = cpair.m_conIdxL;
              indexR = cpair.m_conIdxR;
              bucketidL = cpair.m_conIdL;
              bucketidR = cpair.m_conIdR;

#ifdef PAR_DEBUG2
              pthread_mutex_lock(&print_lock);
              printf("mck fif2: %ld %d auId=%u indexL=%d indexR=%d bucketidL=%d bucketidR=%d\n",
                      gettid(), idx, auId, indexL, indexR, bucketidL, bucketidR);
              fflush(stdout);
              pthread_mutex_unlock(&print_lock);
#endif

              if( !( (indexL == indexR) && 
                     (bucketidL == bucketidR) ) ){ // NOTE: personality already does this ...
       
                if(num_hits >= NUM_BLOCK_HITS){
                  process_hits(idx, hits, num_hits);
                  num_hits = 0;
                }
       
                tmp_crecL = bucketlist[bucketidL].crec;
                tmp_crecR = bucketlist[bucketidR].crec;

                d = edit_distance;
 
                hits[num_hits].d = d;
                hits[num_hits].hit = 0;
         
                // if cp filter is exact then no need to recompute these ...
         
                if( (tmp_crecL->len <= COPROC_FILTER_SIZE) && (tmp_crecR->len <= COPROC_FILTER_SIZE) ){
                  hits[num_hits].hit = 1;
                }
       
                hits[num_hits].lenL = tmp_crecL->len;
                hits[num_hits].lenR = tmp_crecR->len;
                hits[num_hits].indexL = tmp_crecL->index[indexL];
                hits[num_hits].indexR = tmp_crecR->index[indexR];
                hits[num_hits].indx2L = tmp_crecL->indx2[indexL];
                hits[num_hits].indx2R = tmp_crecR->indx2[indexR];
                num_hits++;
      
              } // NOTE: personality already does this ...

              if( !finished_sending[auId] )
                cnt++;

            }

            // ----------------------

          }else if(ppAuUnit[auId]->RecvReturn_htmain(rtn_argv)){
       
            called[auId]--;

            if( (called[auId] == 0) && (finished_sending[auId]) ){

              thr_hits_sent[auId] = rtn_argv;

              num_sent++;
              if(num_sent >= num_data){
                not_done = 0;
                for(j=0;j<(uint32_t)global_auCnt;j++){
                  thrtot_hits_sent += thr_hits_sent[j];
                }
                num_hits_sent[idx] += thrtot_hits_sent;
              }

            }

#ifdef PAR_DEBUG
            pthread_mutex_lock(&print_lock);
            if(finished_sending[auId]){
              printf("mck rtn:  %ld %d auId=%d called=%d finished_sending=%d num_sent=%d num_data=%d hits_sent=%lu\n", 
                      gettid(), idx, auId, called[auId], finished_sending[auId], num_sent, num_data, thr_hits_sent[auId]);
            }else if(not_done){
              printf("mck rtn:  %ld %d auId=%d called=%d finished_sending=%d num_sent=%d num_data=%d\n", 
                      gettid(), idx, auId, called[auId], finished_sending[auId], num_sent, num_data);
            }
            if(not_done == 0){
              printf("mck end:  %ld %d num_sent=%d num_data=%d hits_sent=%lu\n", 
                      gettid(), idx, num_sent, num_data, thrtot_hits_sent);
            }
            fflush(stdout);
            pthread_mutex_unlock(&print_lock);
#endif

          }else if( (called[auId] < NUM_OUTSTANDING_CALLS) && 
              (!finished_sending[auId]) ){

            calls_issued = 0;
            while( (called[auId] < NUM_OUTSTANDING_CALLS) && (calls_issued < 2) &&
                   (!finished_sending[auId]) ){

              lenL = thrmap[auId].crec->lenL;
              lenR = thrmap[auId].crec->lenR;

              d = edit_distance;
              d2 = d;

#if 0
              // This is handled internal to the coproc personality ...

              // BEDENBAUGH <-> BREIDENBAUGH issue
              // where truncation changes the result
              // bump d enough to cover this case ...
 
              if( (lenL > COPROC_FILTER_SIZE) || (lenR > COPROC_FILTER_SIZE) ){
                int cl = MXMAX(0,(lenL - COPROC_FILTER_SIZE));
                int cr = MXMAX(0,(lenR - COPROC_FILTER_SIZE));
                d2 += MXMIN(d, abs(cl - cr));
              }
#endif

              call_argv.m_strLenL  = lenL;
              call_argv.m_strLenR  = lenR;
              call_argv.m_mchAdjD  = d2;
              call_argv.m_conIdL   = thrmap[auId].crec->bucketidL;
              call_argv.m_conIdR   = thrmap[auId].crec->bucketidR;
              call_argv.m_conSizeL = thrmap[auId].crec->numL;
              call_argv.m_conSizeR = thrmap[auId].crec->numR;
              call_argv.m_conBaseL = (void *)&thrmap[auId].crec->bucketaddrL[0];
              call_argv.m_conBaseR = (void *)&thrmap[auId].crec->bucketaddrR[0];

              num_compare = thrmap[auId].crec->numL * thrmap[auId].crec->numR;
              num_cp_compares[idx] += num_compare;
              __sync_fetch_and_add(&running_compare_cnt, num_compare);
              num_compare = 0;

              called[auId]++;
     
              calls_issued++;
  
#ifdef PAR_DEBUG
              pthread_mutex_lock(&print_lock);
              printf("mck call: %ld %d auId=%d called=%d bucketidL=%d bucketidR=%d numL=%d numR=%d\n",
                      gettid(), idx, auId, called[auId], thrmap[auId].crec->bucketidL, 
                      thrmap[auId].crec->bucketidR, thrmap[auId].crec->numL, 
                      thrmap[auId].crec->numR);
              fflush(stdout);
              pthread_mutex_unlock(&print_lock);
#endif

              ppAuUnit[auId]->SendCall_htmain(call_argv);
       
              thrmap[auId].crec = thrmap[auId].crec->next;

              if(thrmap[auId].crec == NULL){

                call_argv.m_strLenL  = 0;
                call_argv.m_strLenR  = 0;
                call_argv.m_mchAdjD  = 0;
                call_argv.m_conIdL   = 0;
                call_argv.m_conIdR   = 0;
                call_argv.m_conSizeL = 0;
                call_argv.m_conSizeR = 0;
                call_argv.m_conBaseL = 0;
                call_argv.m_conBaseR = 0;

                called[auId]++;

#ifdef PAR_DEBUG
                pthread_mutex_lock(&print_lock);
                printf("mck call: %ld %d auId=%d called=%d last call\n", gettid(), idx, auId, called[auId]);
                fflush(stdout);
                pthread_mutex_unlock(&print_lock);
#endif

                ppAuUnit[auId]->SendCall_htmain(call_argv);

                finished_sending[auId] = 1;

                break;

              }
       
            }

          }else{
       
            usleep(1000);
       
          }

        } // if frec != NULL ...

      } // for auId ...

    }

  }else{ // no_coproc

    num_chunks = NUM_ENTRIES_PER_BUCKET/128;
    if(num_chunks < 1){
      num_chunks = 1;
    }

    if(!no_filter){
      pairs = (PAIRS *)malloc((num_chunks+1) * NUM_ENTRIES_PER_BUCKET * sizeof(PAIRS));
      if(pairs == NULL){
        fprintf(stderr,"\nError, unable to alloc mem for pairs data (%lu)\n\n",
                        (num_chunks+1) * NUM_ENTRIES_PER_BUCKET * sizeof(PAIRS));
        exit(1);
      }
      pthread_mutex_lock(&memst_lock);
      mem_curr += (num_chunks+1) * NUM_ENTRIES_PER_BUCKET * sizeof(PAIRS);
      mem_hiwater = MXMAX(mem_curr,mem_hiwater);
      pthread_mutex_unlock(&memst_lock);
    }

    for(auId=0;auId<global_auCnt;auId++){

      tmp_crec = bucketmap[idx][auId].frec;
      while(tmp_crec != NULL){

        lenL = tmp_crec->lenL;
        lenR = tmp_crec->lenR;

#ifdef PAR_DEBUG
        pthread_mutex_lock(&print_lock);
        printf("mck call: %ld %d auId=%u lenL=%u lenR=%u bidL=%u bidR=%u cntL=%u cntR=%u\n",
                gettid(),idx,auId,
                lenL,lenR,tmp_crec->bucketidL,tmp_crec->bucketidR,
                tmp_crec->numL,tmp_crec->numR);
        fflush(stdout);
        pthread_mutex_unlock(&print_lock);
#endif

        d = edit_distance;
        d2 = d;

        if(!no_filter){

          // BEDENBAUGH <-> BREIDENBAUGH issue
          // where truncation changes the result
          // bump d enough to cover this case ...
 
          if( (lenL > COPROC_FILTER_SIZE) || (lenR > COPROC_FILTER_SIZE) ){
            int cl = MXMAX(0,(lenL - COPROC_FILTER_SIZE));
            int cr = MXMAX(0,(lenR - COPROC_FILTER_SIZE));
            d2 += MXMIN(d, abs(cl - cr));
          }

          startL = 0;
          totnumL = 0;
          numL = (int64_t)num_chunks;

          while(totnumL < tmp_crec->numL){

            if((totnumL + numL) > tmp_crec->numL){
              numL = tmp_crec->numL - totnumL;
            }
            totnumL += numL;

            if(numL <= 0){
              break;
            }

            num_pairs = coproc_filter(lenL, lenR, d2, numL, tmp_crec->numR, startL, 
                                      tmp_crec->bucketidL, tmp_crec->bucketidR,
                                      tmp_crec->bucketaddrL, tmp_crec->bucketaddrR, pairs);

            startL += numL;

            num_compare = numL * tmp_crec->numR;
            num_cp_compares[idx] += num_compare;
            __sync_fetch_and_add(&running_compare_cnt, num_compare);
            num_compare = 0;

            k = 0;
            while(k < num_pairs){
 
#ifndef INCLUDE_SELF
              if( !( (pairs[k].indexL == pairs[k].indexR) &&
                     (pairs[k].bucketidL == pairs[k].bucketidR) ) ){
#endif

                if(num_hits >= NUM_BLOCK_HITS){
                  process_hits(idx, hits, num_hits);
                  num_hits = 0;
                }
   
                tmp_crecL = bucketlist[pairs[k].bucketidL].crec;
                tmp_crecR = bucketlist[pairs[k].bucketidR].crec;
   
                hits[num_hits].d = d;
                hits[num_hits].hit = 0;
   
                // if cp filter is exact then no need to recompute these ...
   
                if( (lenL <= COPROC_FILTER_SIZE) && (lenR <= COPROC_FILTER_SIZE) ){
                  hits[num_hits].hit = 1;
                }
   
#ifdef PAR_DEBUG2
                pthread_mutex_lock(&print_lock);
                printf("mck fif2: %ld %d adding indexs %lu %lu\n",gettid(),idx,
                        tmp_crecL->index[pairs[k].indexL], tmp_crecR->index[pairs[k].indexR]);
                fflush(stdout);
                pthread_mutex_unlock(&print_lock);
#endif

                hits[num_hits].lenL = tmp_crecL->len; // or lenL
                hits[num_hits].lenR = tmp_crecR->len; // or lenR
                hits[num_hits].indexL = tmp_crecL->index[pairs[k].indexL];
                hits[num_hits].indexR = tmp_crecR->index[pairs[k].indexR];
                hits[num_hits].indx2L = tmp_crecL->indx2[pairs[k].indexL];
                hits[num_hits].indx2R = tmp_crecR->indx2[pairs[k].indexR];
                num_hits++;

#ifndef INCLUDE_SELF
              }
#endif

              k++;
   
            }

          }

        }else{ // no_filter

          tmp_crecL = bucketlist[tmp_crec->bucketidL].crec;
          tmp_crecR = bucketlist[tmp_crec->bucketidR].crec;

          num_compare = 0;
          for(i=0;i<tmp_crecL->num_entries;i++){
            for(j=0;j<tmp_crecR->num_entries;j++){

#ifndef INCLUDE_SELF
              if( !( (i == j) && 
                     (tmp_crec->bucketidL == tmp_crec->bucketidR) ) ){
              // or just if(tmp_crecL->indx2[i] != tmp_crecR->indx2[j])
#endif

                if(num_hits >= NUM_BLOCK_HITS){
                  __sync_fetch_and_add(&running_compare_cnt, num_compare);
                  num_compare = 0;
                  process_hits(idx, hits, num_hits);
                  num_hits = 0;
                }

                num_edis_called[idx]++;
                if( Levenshtein( &lastnames[tmp_crecL->index[i]], lenL, &lastnames[tmp_crecR->index[j]], lenR, d ) ){

                  hits[num_hits].d = d;
                  hits[num_hits].hit = 1;

#ifdef PAR_DEBUG2
                  pthread_mutex_lock(&print_lock);
                  printf("mck fif2: %ld %d adding indexs %lu %lu\n",gettid(),idx,
                          tmp_crecL->index[i], tmp_crecR->index[j]);
                  fflush(stdout);
                  pthread_mutex_unlock(&print_lock);
#endif

                  hits[num_hits].lenL = tmp_crecL->len; // or lenL
                  hits[num_hits].lenR = tmp_crecR->len; // or lenR
                  hits[num_hits].indexL = tmp_crecL->index[i];
                  hits[num_hits].indexR = tmp_crecR->index[j];
                  hits[num_hits].indx2L = tmp_crecL->indx2[i];
                  hits[num_hits].indx2R = tmp_crecR->indx2[j];
                  num_hits++;

                }

                num_compare++;

#ifndef INCLUDE_SELF
              }
#endif

            }
            __sync_fetch_and_add(&running_compare_cnt, num_compare);
            num_compare = 0;
          }

          __sync_fetch_and_add(&running_compare_cnt, num_compare);
          num_compare = 0;

        } // no_filter

        tmp_crec = tmp_crec->next;

      }

    } // for auId ...

    if(!no_filter){
      free(pairs);
      pthread_mutex_lock(&memst_lock);
      mem_curr -= (num_chunks+1) * NUM_ENTRIES_PER_BUCKET * sizeof(PAIRS);
      pthread_mutex_unlock(&memst_lock);
    }

  } // no_coproc

  if(num_hits > 0){
    __sync_fetch_and_add(&running_compare_cnt, num_compare);
    num_compare = 0;
    process_hits(idx, hits, num_hits);
    num_hits = 0;
  }

  free(hits);
  pthread_mutex_lock(&memst_lock);
  mem_curr -= (NUM_BLOCK_HITS+1) * sizeof(HITS);
  pthread_mutex_unlock(&memst_lock);

#ifdef PAR_DEBUG
  pthread_mutex_lock(&print_lock);
  printf("mck rtn:  %ld %d thread ending\n", gettid(), idx);
  fflush(stdout);
  pthread_mutex_unlock(&print_lock);
#endif

  return;
}

// --------------

void process_names(void)
{
  int i = 0;
  int srtn = 0;
  int lenL = 0;
  int lenR = 0;
  int auId = 0;
  int thrId = 0;
  int done = 0;
  pthread_t tid;
  pthread_t thrinfo[MAX_CPUS];
  uint64_t tot_cp_compares = 0;
  uint64_t tot_edis_called = 0;
  uint64_t tot_edis_printed = 0;
  uint64_t tot_hits_sent = 0;
  uint64_t tot_hits_recvd = 0;
  struct BUCKET *tmp_crecL = NULL;
  int tmap[MAX_AU];
  int num_act_cpus = 0;
  int max_act_cpus = 0;
  uint64_t fpga_partno = 0;
  unsigned char lastname_xrec[MAX_NAME_LEN] = { "" };
#if !defined(_SYSTEM_C) && !defined(HT_MODEL) && !defined(HT_SYSC)
  unsigned char (*coproc_cpname)[COPROC_FILTER_WIDTH];
#endif
  uint64_t pn_end_time = 0;
  double pn_time = 0.0;
  int randarray[MAX_NAME_LEN];
  CHtHifParams params;

  pn_start_time = (*rdtsc0)();

  for(thrId=0;thrId<MAX_CPUS;thrId++){
    for(auId=0;auId<MAX_AU;auId++){
      bucketmap[thrId][auId].frec = NULL;
      bucketmap[thrId][auId].crec = NULL;
      bucketmap[thrId][auId].lrec = NULL;
    }
    num_cp_compares[thrId] = 0;
    num_edis_called[thrId] = 0;
    num_edis_printed[thrId] = 0;
    num_hits_sent[thrId] = 0;
    num_hits_recvd[thrId] = 0;
  }

  if(!no_coproc){

    fflush(NULL);

    // NOTE: this disables timers
    // which means a Return must be used to flush buffers (which we do) ...
    params.m_iBlkTimerUSec = 0;
    params.m_oBlkTimerUSec = 0;
    m_pHtHif = new CHtHif(&params);

    fflush(NULL);

    global_auCnt = m_pHtHif->GetUnitCnt();

    ppAuUnit = new CHtAuUnit * [global_auCnt];
    for (int unit = 0; unit < global_auCnt; unit++)
      ppAuUnit[unit] = new CHtAuUnit(m_pHtHif);

    printf("FPGA AU count = %d\n",global_auCnt);

    fpga_partno = m_pHtHif->GetPartNumber();

    printf("FPGA part no. = %03lx-%06lx-%03lx\n",fpga_partno>>9*4,(fpga_partno>>3*4)&0xffffff,fpga_partno&0xfff);

    fflush(stdout);

#if !defined(_SYSTEM_C) && !defined(HT_MODEL) && !defined(HT_SYSC)
    for(lenL=0;lenL<MAX_NAME_LEN;lenL++){

      if(nbin[lenL].num_buckets <= 0)
        continue;

      tmp_crecL = nbin[lenL].frec;
      while(tmp_crecL != NULL){

        coproc_cpname = (unsigned char (*)[COPROC_FILTER_WIDTH])m_pHtHif->MemAlloc((NUM_ENTRIES_PER_BUCKET+1)*COPROC_FILTER_WIDTH);
        if(coproc_cpname == NULL){
          fprintf(stderr,"\nError, unable to cp alloc mem for bucket string data (%d)\n\n",
                          ((NUM_ENTRIES_PER_BUCKET+1) * COPROC_FILTER_WIDTH));
          return;
        }
        cp_mem_curr += (NUM_ENTRIES_PER_BUCKET+1) * COPROC_FILTER_WIDTH;
        cp_mem_hiwater = MXMAX(cp_mem_curr,cp_mem_hiwater);

        m_pHtHif->MemCpy(coproc_cpname, tmp_crecL->cpname, (NUM_ENTRIES_PER_BUCKET+1) * COPROC_FILTER_WIDTH);
        free(tmp_crecL->cpname);
        mem_curr -= (NUM_ENTRIES_PER_BUCKET+1) * COPROC_FILTER_WIDTH;
        tmp_crecL->cpname = coproc_cpname;

        tmp_crecL = tmp_crecL->next;
      }

    }
#endif

  }else{

    global_auCnt = MAX_AU;

  }

  max_act_cpus = global_auCnt;
  num_act_cpus = num_cpus;
  if(num_act_cpus > max_act_cpus)
    num_act_cpus = max_act_cpus;

  // TODO - a better balanced or dynamic pool of containers across threads ...

  thrId = 0;
  for(auId=0;auId<global_auCnt;auId++){
    tmap[auId] = thrId;
    thrId++;
    if(thrId >= num_act_cpus){
      thrId = 0;
    }
  }

  for(i=0;i<MAX_NAME_LEN;i++){
    randarray[i] = i;
  }

  srand(21U);
  // srand(11U);

  int j = 0;
  int k = MAX_NAME_LEN;
  while(k > 0){
    i = rand() % k--;
    j = randarray[k];
    randarray[k] = randarray[i];
    randarray[i] = j;
  }

  auId = 0;
  thrId = 0;
  for(lenL=0;lenL<MAX_NAME_LEN;lenL++){

    if (nbin[lenL].num_buckets <= 0){
      continue;
    }

    tmp_crecL = nbin[lenL].frec;
    while(tmp_crecL != NULL){

      for(lenR=0;lenR<MAX_NAME_LEN;lenR++){
        nbin[lenR].crec = nbin[lenR].frec;
      }

      done = 0;
      while(done < MAX_NAME_LEN){

        i = 0;
        while(i < MAX_NAME_LEN){

          lenR = randarray[i];
          // lenR = i;

          if(nbin[lenR].crec != NULL){

            if(!tmp_crecL->bmask[lenR]){

             if ( lenL <= lenR + edit_distance * 1 && lenR <= lenL + edit_distance * 1 ){

               max_compares += (tmp_crecL->num_entries * nbin[lenR].crec->num_entries);
      
               thrId = tmap[auId];
      
               bucketmap[thrId][auId].crec = (struct BUCKETSUBMAP *)malloc(sizeof(struct BUCKETSUBMAP));
               if(bucketmap[thrId][auId].crec == NULL){
                 fprintf(stderr,"\nError, unable to alloc mem for bucket au map list\n\n");
                 return;
               }
               mem_curr += sizeof(struct BUCKETSUBMAP);
               mem_hiwater = MXMAX(mem_curr,mem_hiwater);
      
               bucketmap[thrId][auId].crec->next = NULL;
      
               if(bucketmap[thrId][auId].frec == NULL){
                 bucketmap[thrId][auId].frec = bucketmap[thrId][auId].crec;
                 bucketmap[thrId][auId].lrec = bucketmap[thrId][auId].crec;
               }else{
                 bucketmap[thrId][auId].lrec->next = bucketmap[thrId][auId].crec;
                 bucketmap[thrId][auId].lrec = bucketmap[thrId][auId].crec;
               }
      
#ifdef DEBUG2
               printf("data map: thrId=%d auId=%d bLId=%d bRId=%d lenL=%d lenR=%d\n", 
                       thrId, auId, tmp_crecL->bucketid, nbin[lenR].crec->bucketid, lenL, lenR);
               fflush(stdout);
#endif
      
               bucketmap[thrId][auId].crec->bucketidL = tmp_crecL->bucketid;
               bucketmap[thrId][auId].crec->lenL = lenL;
               bucketmap[thrId][auId].crec->numL = tmp_crecL->num_entries;
               bucketmap[thrId][auId].crec->bucketaddrL = tmp_crecL->cpname;
      
               bucketmap[thrId][auId].crec->bucketidR = nbin[lenR].crec->bucketid;
               bucketmap[thrId][auId].crec->lenR = lenR;
               bucketmap[thrId][auId].crec->numR = nbin[lenR].crec->num_entries;
               bucketmap[thrId][auId].crec->bucketaddrR = nbin[lenR].crec->cpname;
      
               auId++;
               if(auId >= global_auCnt){
                 auId = 0;
               }
      
#ifndef DONT_SKIP_DUPLICATES
               if(lenL != lenR){
                 nbin[lenR].crec->bmask[lenL] = 1;
               }
#endif

              } // if lenL <= lenR + edit_distance * 1 ...

            } // if tmp_crecL->bmask[lenR] ...

            nbin[lenR].crec = nbin[lenR].crec->next;

          } // if nbin[lenR].crec != NULL ...

          i++;

        } // while i < MAX_NAME_LEN ...

        done = 0;
        for(i=0;i<MAX_NAME_LEN;i++){
          if(nbin[i].crec == NULL){
            done++;
          }
        }

      } // while done < MAX_NAME_LEN ...

      tmp_crecL = tmp_crecL->next;

    } // while tmp_crecL ...

  } // for lenL ...

  // --------------

  for(i=0;i<num_cpus;i++){
    srtn = pthread_create(&tid,NULL,(void *(*)(void *))thr_process_names,(void *)(long)i);
    if(srtn != 0){
      fprintf(stderr,"\nError, pthread_create error %d\n\n",srtn);
      return;
    }
    thrinfo[i] = tid;
  }

  for(i=0;i<num_cpus;i++){
    pthread_join(thrinfo[i],NULL);
  }

  for(i=0;i<num_cpus;i++){
    tot_cp_compares += num_cp_compares[i];
    tot_edis_called += num_edis_called[i];
    tot_edis_printed += num_edis_printed[i];
    tot_hits_sent += num_hits_sent[i];
    tot_hits_recvd += num_hits_recvd[i];
  }

  // NOTE: this can often over count the host compares ...

  if(no_filter){
    tot_edis_called += tot_num_names;
  }

  if(!skip_bitvec){

#ifdef INCLUDE_SELF
    if(!no_coproc){

      // add self to every name ...

      for(uint64_t li=0;li<tot_num_names;li++){
# ifdef SIMPLE_HASH
        uint8_t bhash = bitvec[li].bhash;
        bitvec[li].bvec[bhash >> 5] |= (uint32_t)(1U << (bhash & 31));
# else
        SetBloomList(li, li);
# endif
        tot_edis_called++;
        tot_edis_printed++;
      }

    }
#endif

#if 1
    if(tot_num_names > 0){

      // write out header info ...
   
      if(!use_ascii){
        srtn = 0;
        fwrite(&srtn, sizeof(int), 1, outfp);
        fwrite(&output_rec_length, sizeof(int), 1, outfp);
        fwrite(&edit_distance, sizeof(int), 1, outfp);
        fwrite(&skip_leading_ws, sizeof(int), 1, outfp);
        // set 1 for Levenshtein algorithm ...
        srtn = (1 & 0xff) | (case_sensitive << 16);
        fwrite(&srtn, sizeof(int), 1, outfp);
      }else{
        // set 1 for Levenshtein algorithm ...
        srtn = (1 & 0xff) | (case_sensitive << 16);
        fprintf(outfp, "%d %d %d %d %d\n", 0, output_rec_length, edit_distance, skip_leading_ws, srtn);
      }

    }
#endif

    // write out bloom file ...
 
#ifdef SIMPLE_HASH
    if(!use_ascii){
  
      for(uint64_t li=0;li<tot_num_names;li++){
        strcpy((char *)lastname_xrec, (char *)&lastnames[bitvec[li].index]);
        lenL = (int)strlen((char *)lastname_xrec);
        for(i=lenL;i<output_rec_length;i++){
          lastname_xrec[i] = ' ';
        }
        lastname_xrec[output_rec_length] = '\0';
        fwrite(lastname_xrec, sizeof(unsigned char), output_rec_length, outfp);
        fwrite(&bitvec[li].bhash, sizeof(uint8_t), 1, outfp);
        fwrite(bitvec[li].bvec, sizeof(uint32_t), 4, outfp);
      }

    }else{

      for(uint64_t li=0;li<tot_num_names;li++){
        strcpy((char *)lastname_xrec, (char *)&lastnames[bitvec[li].index]);
        lenL = (int)strlen((char *)lastname_xrec);
        for(i=lenL;i<output_rec_length;i++){
          lastname_xrec[i] = ' ';
        }
        lastname_xrec[output_rec_length] = '\0';
        fprintf(outfp,"%-*s",output_rec_length,lastname_xrec);
        fprintf(outfp," %3u",bitvec[li].bhash);
        for(int j=0;j<4;j++){
          fprintf(outfp," 0x%-16x",bitvec[li].bvec[j]);
        }
        fprintf(outfp,"\n");
      }

    }
#else
    if(!use_ascii){

      for(uint64_t li=0;li<tot_num_names;li++){
        strcpy((char *)lastname_xrec, (char *)&lastnames[lastindxs[li]]);
        lenL = (int)strlen((char *)lastname_xrec);
        for(i=lenL;i<output_rec_length;i++){
          lastname_xrec[i] = ' ';
        }
        lastname_xrec[output_rec_length] = '\0';
        fwrite(lastname_xrec, sizeof(unsigned char), output_rec_length, outfp);
        fwrite(&pBloomList[li], sizeof(CBloom), 1, outfp);
      }
   
    }else{
   
      for(uint64_t li=0;li<tot_num_names;li++){
        strcpy((char *)lastname_xrec, (char *)&lastnames[lastindxs[li]]);
        lenL = (int)strlen((char *)lastname_xrec);
        for(i=lenL;i<output_rec_length;i++){
          lastname_xrec[i] = ' ';
        }
        lastname_xrec[output_rec_length] = '\0';
        fprintf(outfp,"%-*s",output_rec_length,lastname_xrec);
        fprintf(outfp," %3u",pBloomList[li].hashIdx);
        for(int j=0;j<16;j++){
          fprintf(outfp," 0x%-2x",pBloomList[li].m_mask[j]);
        }
        fprintf(outfp,"\n");
      }
   
    }
#endif

  }else{ // skip_bitvec

#ifdef INCLUDE_SELF
    if(!no_coproc){

      // add self to every name ...

      for(uint64_t li=0;li<tot_num_names;li++){
# ifndef SKIP_IO
        fprintf(outfp,"%-*s %-*s\n",max_name_len, &lastnames[lastindxs[li]],
                max_name_len, &lastnames[lastindxs[li]]);
# endif // SKIP_IO
        tot_edis_called++;
        tot_edis_printed++;
      }

    }
#endif

  } // skip_bitvec

  pn_end_time = (*rdtsc0)();
  pn_time = (double)((double)pn_end_time - (double)pn_start_time) / clocks_per_sec;

  fflush(stdout);

  if(print_detail > 1){
    // printf("max number of compares  = %lu\n",max_compares);
    // printf("N^2 number of compares  = %lu\n",tot_num_names * tot_num_names);
    printf("number of cp compares   = %lu\n",tot_cp_compares);
    if(!no_coproc)
      printf("number of cp hits sent  = %lu\n",tot_hits_sent);
    if(!no_filter)
      printf("number of cp hits recvd = %lu\n",tot_hits_recvd);
    printf("number of host compares = %lu\n",tot_edis_called);
    if((tot_cp_compares + tot_edis_called) > 0){
      printf("overall compare perf    = %.4lf (ns/comp)\n",
             (pn_time  * 1000000000.0) / (double)(tot_cp_compares + tot_edis_called) );
    }
  }

  printf("total hits found   = %lu\n",tot_edis_printed);

  fflush(NULL);

  if(!no_coproc){
    delete m_pHtHif;
    fflush(NULL);
  }

  return;
}

#endif // EXAMPLE
