/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT levstrcmp application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Main.h"
#include "Ht.h"
using namespace Ht;

#ifndef VERSION
#define VERSION "X"
#endif
#ifndef SVNREV
#define SVNREV "X"
#endif

#ifndef EXAMPLE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sched.h>
#include <limits.h>
#include <pthread.h>

#define EDITDIS_INIT
#include "editdis.h"

// #define DEBUG

// --------------

uint64_t rdtsc2(void)
{
  uint64_t tics;
  struct timeval st;
  gettimeofday(&st, NULL);
  tics = (uint64_t)((uint64_t)st.tv_sec * 1000000UL + (uint64_t)st.tv_usec);
  return tics;
}

// --------------

int init_structs(void)
{
  int i = 0;

  for(i=0;i<MAX_NAME_LEN;i++){
    nbin[i].frec = NULL;
    nbin[i].crec = NULL;
    nbin[i].lrec = NULL;
    nbin[i].num_buckets = 0;
    nbin[i].tot_entries = 0;
  }

  siz_lastnames = NUM_BLOCK_LNAMES;
  lastnames = (unsigned char *)malloc((siz_lastnames+1) * sizeof(unsigned char));
  if(lastnames == NULL){
    fprintf(stderr,"\nError, unable to alloc mem for lastnames data (%lu)\n\n",(siz_lastnames+1));
    return 1;
  }
  mem_curr += (siz_lastnames+1) * sizeof(unsigned char);
  mem_hiwater = MXMAX(mem_curr,mem_hiwater);

#ifdef SIMPLE_HASH
  if(!skip_bitvec){
    siz_bitvec = NUM_BLOCK_LNAMES;
    bitvec = (BITVEC *)malloc((siz_bitvec+1) * sizeof(BITVEC));
    if(bitvec == NULL){
      fprintf(stderr,"\nError, unable to alloc mem for bitvec filter (%ld)\n\n",(siz_bitvec+1));
      return 1;
    }
    mem_curr += (siz_bitvec+1) * sizeof(BITVEC);
    mem_hiwater = MXMAX(mem_curr,mem_hiwater);
   
    for(i=0;i<256;i++){
      ptable[i] = i;
    }
   
    srand(21U);

    int j = 0;
    int k = 256;
    while(k > 0){
      i = rand() % k--;
      j = ptable[k];
      ptable[k] = ptable[i];
      ptable[i] = j;
    }
  }
#endif

  siz_lastindxs = NUM_BLOCK_LNAMES;
  lastindxs = (uint64_t *)malloc((siz_lastindxs+1) * sizeof(uint64_t));
  if(lastindxs == NULL){
    fprintf(stderr,"\nError, unable to alloc mem for lastindxs data (%lu)\n\n",(siz_lastindxs+1));
    return 1;
  }
  mem_curr += (siz_lastindxs+1) * sizeof(uint64_t);
  mem_hiwater = MXMAX(mem_curr,mem_hiwater);

  siz_bucketlist = NUM_BLOCK_BUCKETLIST;
  bucketlist = (BUCKETLIST *)malloc((siz_bucketlist+1) * sizeof(BUCKETLIST));
  if(bucketlist == NULL){
    fprintf(stderr,"\nError, unable to alloc mem for bucketlist (%lu)\n\n",(siz_bucketlist+1));
    return 1;
  }
  mem_curr += (siz_bucketlist+1) * sizeof(BUCKETLIST);
  mem_hiwater = MXMAX(mem_curr,mem_hiwater);

  // --------------

  clocks_per_sec = 1000000.0;
  rdtsc0 = &rdtsc2;

  return 0;
}

// --------------
extern CHtHif *m_pHtHif;

void free_structs(void)
{
  int i = 0;
  struct BUCKET *tmp_crec = NULL;
  struct BUCKET *tmp_nrec = NULL;

  for(i=0;i<MAX_NAME_LEN;i++){
    tmp_crec = nbin[i].frec;
    if(tmp_crec != NULL){
      tmp_nrec = tmp_crec->next;
      if(tmp_crec->cpname != NULL){
        if(!no_coproc){
          m_pHtHif->MemFree(tmp_crec->cpname);
        }else{
          free(tmp_crec->cpname);
        }
        tmp_crec->cpname = NULL;
      }
      free(tmp_crec->index);
      free(tmp_crec->indx2);
      free(tmp_crec);
      tmp_crec = tmp_nrec;
    }
    nbin[i].frec = NULL;
    nbin[i].crec = NULL;
    nbin[i].lrec = NULL;
  }

  if(lastnames != NULL){
    free(lastnames);
    lastnames = NULL;
  }
  siz_lastnames = 0;

#ifdef SIMPLE_HASH
  if(bitvec != NULL){
    free(bitvec);
    bitvec = NULL;
  }
  siz_bitvec = 0;
#endif

  if(lastindxs != NULL){
    free(lastindxs);
    lastindxs = NULL;
  }
  siz_lastindxs = 0;

  if(pBloomList != NULL){
    free(pBloomList);
    pBloomList = NULL;
  }

  if(bucketlist != NULL){
    free(bucketlist);
    bucketlist = NULL;
  }
  siz_bucketlist = 0;

  return;
}

// --------------

// translate used ascii into 6 bits

static char __attribute((used)) xlate(int ch) {
  static int table[256];
  static int next = -1;

  if (next < 0) {
    next = 0;
    for (int i=0; i<256; i++)
      table[i] = -1;
  }

  if (table[ch] < 0) {
    if (next >= 64) {
      fprintf(stderr, "\nError, xlate table exhausted, unable to translate 0x%x\n\n", ch);
      exit(1);
    }
    table[ch] = next++;
  }

  return table[ch];
}

// --------------

#define HASH_7 (((u_int8_t)1<<7)-1)

uint8_t HASH(char *name, int len)
{
  int i;
  uint8_t index;

  for (index=len, i=0; i<MXMIN(4,len); ++i){
    index = ptable[index ^ name[i]];
  }

  index ^= (index >> 7);

  return (index & HASH_7);
}

// --------------

int get_next_name(FILE *fp, char *lastname)
{
  int i = 0;
  int j = 0;
  int len = 0;
  int beg = 0;
  char line[2048] = { "" };

  if(rec_length <= 0){
    if(fgets(line,2047,fp) != NULL){
      len = (int)strlen(line);
      if(len >= 2*MAX_NAME_LEN){
        len = 2*MAX_NAME_LEN-1;
      }
    }else{
      return -1;
    }
  }else{
    i = fread(line,rec_length,1,fp);
    if(i == 1){
      len = rec_length;
    }else{
      return -1;
    }
  }

  for(i=len-1;i>=0;i--){
    if( (line[i] == '\r') || (line[i] == '\n') || (line[i] == ' ') || (line[i] == '\0') ){
      line[i] = '\0';
    }else{
      len = i+1;
      break;
    }
  }

  beg = 0;
  if(skip_leading_ws){
    for(i=0;i<len;i++){
      if(line[i] == ' '){
        beg++;
      }else{
        break;
      }
    }
  }

  j = 0;
  for(i=beg;i<len;i++){
    if(case_sensitive){
      lastname[j] = line[i];
    }else{
      lastname[j] = toupper(line[i]);
    }
    j++;
  }
  len = j;

  lastname[len] = '\0';

  return len;
}

// --------------

int read_names(FILE *fp)
{
  int i = 0;
  int j = 0;
  int len = 0;
  int len2 = 0;
  uint64_t indx = 0;
  char lastname[2*MAX_NAME_LEN+1] = { "" };
  char cplastname[MAX_NAME_LEN+1] = { "" };
  int64_t min_len = LONG_MAX - 1;
  int64_t max_len = 0;
  int64_t cnt_len = 0;
  uint32_t bucketid = 0;
  uint64_t idx_lastnames = 0;

  indx = 0;
  while((len = get_next_name(fp,lastname)) >= 0){

#if 0
    printf("name = %2d <%s>\n",len,lastname);
#endif

    if(len < min_len){
      min_len = len;
    }

    if(len > max_len){
      max_len = len;
    }

    if(max_len >= MAX_NAME_LEN){
      fprintf(stderr,"\nError, name length (>=%lu) exceeds max (%d) (%s...)\n\n",max_len,MAX_NAME_LEN-1,lastname);
      return 1;
    }

    cnt_len += len;

    if((idx_lastnames+(len+1)) >= siz_lastnames){
      mem_curr -= (siz_lastnames+1) * sizeof(unsigned char);
      siz_lastnames += (NUM_BLOCK_LNAMES + (len+1));
      lastnames = (unsigned char *)realloc(lastnames, ((siz_lastnames+1) * sizeof(unsigned char)));
      if(lastnames == NULL){
        fprintf(stderr,"\nError, unable to expand mem for lastnames data (%lu)\n\n",(siz_lastnames+1));
        return 1;
      }
      mem_curr += (siz_lastnames+1) * sizeof(unsigned char);
      mem_hiwater = MXMAX(mem_curr,mem_hiwater);
    }

    memcpy(&lastnames[idx_lastnames], lastname, (len+1));

#ifdef SIMPLE_HASH
    if(!skip_bitvec){
      if((indx+1) >= siz_bitvec){
        mem_curr -= (siz_bitvec+1) * sizeof(BITVEC);
        siz_bitvec += NUM_BLOCK_LNAMES;
        bitvec = (BITVEC *)realloc(bitvec, ((siz_bitvec+1) * sizeof(BITVEC)));
        if(bitvec == NULL){
          fprintf(stderr,"\nError, unable to expand mem for bitvec filter (%lu)\n\n",(siz_bitvec+1));
          return 1;
        }
        mem_curr += (siz_bitvec+1) * sizeof(BITVEC);
        mem_hiwater = MXMAX(mem_curr,mem_hiwater);
      }
    }
#endif

    if((indx+1) >= siz_lastindxs){
      mem_curr -= (siz_lastindxs+1) * sizeof(uint64_t);
      siz_lastindxs += NUM_BLOCK_LNAMES;
      lastindxs = (uint64_t *)realloc(lastindxs, ((siz_lastindxs+1) * sizeof(uint64_t)));
      if(lastindxs == NULL){
        fprintf(stderr,"\nError, unable to expand mem for lastindxs data (%lu)\n\n",(siz_lastindxs+1));
        return 1;
      }
      mem_curr += (siz_lastindxs+1) * sizeof(uint64_t);
      mem_hiwater = MXMAX(mem_curr,mem_hiwater);
    }

    lastindxs[indx] = idx_lastnames;

    memcpy(cplastname, lastname, (len+1));
    len2 = MXMIN(len,COPROC_FILTER_SIZE);
    for(i=len2;i<COPROC_FILTER_WIDTH;i++){
      cplastname[i] = ' ';
    }
    cplastname[COPROC_FILTER_WIDTH] = '\0';

    if(!no_coproc){
      for(i=0;i<COPROC_FILTER_WIDTH;i++){
        cplastname[i] = xlate(cplastname[i]);
      }
    }

    if(nbin[len].crec == NULL){

      nbin[len].crec = (struct BUCKET *)malloc(sizeof(struct BUCKET));
      if(nbin[len].crec == NULL){
        fprintf(stderr,"\nError, unable to alloc mem for initial bucket info\n\n");
        return 1;
      }
      mem_curr += sizeof(struct BUCKET);
      mem_hiwater = MXMAX(mem_curr,mem_hiwater);

      nbin[len].crec->next = NULL;
      nbin[len].crec->len = len;
      nbin[len].crec->num_entries = 0;
      nbin[len].frec = nbin[len].crec;
      nbin[len].lrec = nbin[len].crec;

      for(j=0;j<MAX_NAME_LEN;j++){
        nbin[len].crec->bmask[j] = 0;
      }

      nbin[len].num_buckets++;

      nbin[len].crec->index = (uint64_t *)malloc((NUM_ENTRIES_PER_BUCKET+1) * sizeof(uint64_t));
      if(nbin[len].crec->index == NULL){
        fprintf(stderr,"\nError, unable to alloc mem for bucket index data (%d)\n\n",(NUM_ENTRIES_PER_BUCKET+1));
        return 1;
      }
      mem_curr += (NUM_ENTRIES_PER_BUCKET+1) * sizeof(uint64_t);
      mem_hiwater = MXMAX(mem_curr,mem_hiwater);

      nbin[len].crec->indx2 = (uint64_t *)malloc((NUM_ENTRIES_PER_BUCKET+1) * sizeof(uint64_t));
      if(nbin[len].crec->indx2 == NULL){
        fprintf(stderr,"\nError, unable to alloc mem for bucket indx2 data (%d)\n\n",(NUM_ENTRIES_PER_BUCKET+1));
        return 1;
      }
      mem_curr += (NUM_ENTRIES_PER_BUCKET+1) * sizeof(uint64_t);
      mem_hiwater = MXMAX(mem_curr,mem_hiwater);

      nbin[len].crec->cpname = (unsigned char (*)[COPROC_FILTER_WIDTH])malloc((NUM_ENTRIES_PER_BUCKET+1) * COPROC_FILTER_WIDTH);
      if(nbin[len].crec->cpname == NULL){
        fprintf(stderr,"\nError, unable to alloc mem for bucket string data (%d)\n\n",((NUM_ENTRIES_PER_BUCKET+1) * COPROC_FILTER_WIDTH));
        return 1;
      }
      mem_curr += (NUM_ENTRIES_PER_BUCKET+1) * COPROC_FILTER_WIDTH;
      mem_hiwater = MXMAX(mem_curr,mem_hiwater);

      nbin[len].crec->bucketid = bucketid;

      if((bucketid+1) >= siz_bucketlist){
        mem_curr -= (siz_bucketlist+1) * sizeof(BUCKETLIST);
        siz_bucketlist += NUM_BLOCK_BUCKETLIST;
        bucketlist = (BUCKETLIST *)realloc(bucketlist, ((siz_bucketlist+1) * sizeof(BUCKETLIST)));
        if(bucketlist == NULL){
          fprintf(stderr,"\nError, unable to expand mem for bucketlist (%lu)\n\n",(siz_bucketlist+1));
          return 1;
        }
        mem_curr += (siz_bucketlist+1) * sizeof(BUCKETLIST);
        mem_hiwater = MXMAX(mem_curr,mem_hiwater);
      }

      bucketlist[bucketid].crec = nbin[len].crec;
      bucketid++;

    }else if(nbin[len].crec->num_entries > (NUM_ENTRIES_PER_BUCKET-1)){

      nbin[len].crec = (struct BUCKET *)malloc(sizeof(struct BUCKET));
      if(nbin[len].crec == NULL){
        fprintf(stderr,"\nError, unable to alloc mem for addl bucket info\n\n");
        return 1;
      }
      mem_curr += sizeof(struct BUCKET);
      mem_hiwater = MXMAX(mem_curr,mem_hiwater);

      nbin[len].crec->next = NULL;
      nbin[len].crec->len = len;
      nbin[len].crec->num_entries = 0;
      nbin[len].lrec->next = nbin[len].crec;
      nbin[len].lrec = nbin[len].crec;

      for(j=0;j<MAX_NAME_LEN;j++){
        nbin[len].crec->bmask[j] = 0;
      }

      nbin[len].num_buckets++;

      nbin[len].crec->index = (uint64_t *)malloc((NUM_ENTRIES_PER_BUCKET+1) * sizeof(uint64_t));
      if(nbin[len].crec->index == NULL){
        fprintf(stderr,"\nError, unable to alloc mem for bucket index data (%d)\n\n",(NUM_ENTRIES_PER_BUCKET+1));
        return 1;
      }
      mem_curr += (NUM_ENTRIES_PER_BUCKET+1) * sizeof(uint64_t);
      mem_hiwater = MXMAX(mem_curr,mem_hiwater);

      nbin[len].crec->indx2 = (uint64_t *)malloc((NUM_ENTRIES_PER_BUCKET+1) * sizeof(uint64_t));
      if(nbin[len].crec->indx2 == NULL){
        fprintf(stderr,"\nError, unable to alloc mem for bucket indx2 data (%d)\n\n",(NUM_ENTRIES_PER_BUCKET+1));
        return 1;
      }
      mem_curr += (NUM_ENTRIES_PER_BUCKET+1) * sizeof(uint64_t);
      mem_hiwater = MXMAX(mem_curr,mem_hiwater);

      nbin[len].crec->cpname = (unsigned char (*)[COPROC_FILTER_WIDTH])malloc((NUM_ENTRIES_PER_BUCKET+1) * COPROC_FILTER_WIDTH);
      if(nbin[len].crec->cpname == NULL){
        fprintf(stderr,"\nError, unable to alloc mem for bucket string data (%d)\n\n",((NUM_ENTRIES_PER_BUCKET+1) * COPROC_FILTER_WIDTH));
        return 1;
      }
      mem_curr += (NUM_ENTRIES_PER_BUCKET+1) * COPROC_FILTER_WIDTH;
      mem_hiwater = MXMAX(mem_curr,mem_hiwater);

      nbin[len].crec->bucketid = bucketid;

      if((bucketid+1) >= siz_bucketlist){
        mem_curr -= (siz_bucketlist+1) * sizeof(BUCKETLIST);
        siz_bucketlist += NUM_BLOCK_BUCKETLIST;
        bucketlist = (BUCKETLIST *)realloc(bucketlist, ((siz_bucketlist+1) * sizeof(BUCKETLIST)));
        if(bucketlist == NULL){
          fprintf(stderr,"\nError, unable to expand mem for bucketlist (%lu)\n\n",(siz_bucketlist+1));
          return 1;
        }
        mem_curr += (siz_bucketlist+1) * sizeof(BUCKETLIST);
        mem_hiwater = MXMAX(mem_curr,mem_hiwater);
      }

      bucketlist[bucketid].crec = nbin[len].crec;
      bucketid++;

    }

    memcpy(nbin[len].crec->cpname[nbin[len].crec->num_entries], cplastname, COPROC_FILTER_WIDTH);
    nbin[len].crec->index[nbin[len].crec->num_entries] = idx_lastnames;
    nbin[len].crec->indx2[nbin[len].crec->num_entries] = indx;
    nbin[len].crec->num_entries++;
    nbin[len].tot_entries++;

#ifdef SIMPLE_HASH
    if(!skip_bitvec){
      bitvec[indx].index = idx_lastnames;
      bitvec[indx].bvec[0] = 0;
      bitvec[indx].bvec[1] = 0;
      bitvec[indx].bvec[2] = 0;
      bitvec[indx].bvec[3] = 0;
      bitvec[indx].bhash = HASH(lastname, len);
# if 0
      printf("%-*s %d\n",output_rec_length,lastname,bitvec[indx].bhash);
# endif
    }
#endif

    idx_lastnames += (len+1);

    indx++;

  }

  tot_num_names = indx;

  max_name_len = (int)max_len;

#ifndef SIMPLE_HASH
  if(!skip_bitvec){
    CreateNameHashIdxList();
  }
#endif

  printf("edit dis    = %d\n",edit_distance);
  printf("num cpus    = %d\n",num_cpus);
  printf("num names   = %lu\n",tot_num_names);
  if(tot_num_names > 0){
    printf("min len     = %lu\n",min_len);
    printf("max len     = %lu\n",max_len);
    printf("mean len    = %lu\n", cnt_len / tot_num_names);
  }else{
    printf("min len     = %lu\n", 0UL);
    printf("max len     = %lu\n", 0UL);
    printf("mean len    = %lu\n", 0UL);
  }
  printf("cont size   = %d\n",NUM_ENTRIES_PER_BUCKET);
  if(print_detail > 1){
    printf("max nam len = %d\n",MAX_NAME_LEN);
    printf("use cp      = %d\n",!no_coproc);
    if(no_coproc){
      printf("use filter  = %d\n",!no_filter);
    }
  }
  fflush(stdout);

#ifdef DEBUG
  int di = 0;
  uint64_t dj = 0;
  struct BUCKET *tmp_crec = NULL;

  for(di=0;di<MAX_NAME_LEN;di++){
    if(nbin[di].crec != NULL){
      if(nbin[di].crec->num_entries > 0){
        printf("nbin[%3d].num_buckets = %lu  tot_entries = %lu\n",di,nbin[di].num_buckets,nbin[di].tot_entries);
        tmp_crec = nbin[di].frec;
        while(tmp_crec != NULL){
          for(dj=0;dj<tmp_crec->num_entries;dj++){
            printf("nbin[%3d].index[%lu] = %lu (%s)\n",di,dj,tmp_crec->index[dj],(char *)&lastnames[tmp_crec->index[dj]]);
          }
          tmp_crec = tmp_crec->next;
        }
      }
    }
  }
#endif

  return 0;
}

// --------------

int editdis(int argc, char *argv[])
{
  int i = 0;
  int srtn = 0;
  int has_oname = 0;
  FILE *infp = NULL;
  char fname[256] = { "" };
  char oname[256] = { "" };
  char hostname[256] = { "" };
  uint64_t sc, sc1, ec;
  double total_spent = 0.0;
  time_t time_now = 0;

  if(argc < 2){
    fprintf(stderr,"\nError, need args: <input-file> [-edis ##] [-recl ##] [-xrec ##] [-lws] [-pairs] [-o <output-file>] [-t #threads] [-Case-sensitive] [-no-coproc] [-no-filter]\n\n");
    return 1;
  }

  if(strncasecmp(argv[1],"-v",2) == 0){
    printf("\nCNYLEVED - Convey Levenshtein Edit Distance program (%s-%s)\n\n",VERSION,SVNREV);
    return 0;
  }else if( (strncasecmp(argv[1],"-h",2) == 0) ||
            (strncasecmp(argv[1],"-u",2) == 0) ||
            (strncasecmp(argv[1],"-?",2) == 0)  ){
    fprintf(stderr,"\nprogram usage: <input-file> [-edis ##] [-recl ##] [-xrec ##] [-lws] [-pairs] [-o <output-file>] [-t #threads] [-Case-sensitive] [-no-coproc] [-no-filter]\n\n");
    return 0;
  }

  strcpy(fname,argv[1]);

  infp = fopen(fname,"r");
  if(infp == NULL){
    fprintf(stderr,"\nError, cannot open input file <%s>\n\n",fname);
    return 1;
  }

  num_cpus = -1;
  no_coproc = 0;
  no_filter = 0;
  use_ascii = 0;
  mem_curr = 0;
  has_oname = 0;
  rec_length = 0;
  skip_bitvec = 0;
  max_name_len = 0;
  mem_hiwater = 0;
  cp_mem_curr = 0;
  cp_mem_hiwater = 0;
  edit_distance = 2;
  print_detail = 1;
  skip_leading_ws = 1;
  output_rec_length = 0;
  case_sensitive = 0;

  lastnames = NULL;
  lastindxs = NULL;
  bucketlist = NULL;

#ifdef _SC_NPROCESSORS_ONLN
  max_cpus = sysconf(_SC_NPROCESSORS_ONLN);
#else
  max_cpus = MAX_CPUS;
#endif

  i = 2;
  while(i < argc){
    if( (strncasecmp(argv[i],"-t",2) == 0) ||
        (strncasecmp(argv[i],"-ncpu",5) == 0) ||
        (strncasecmp(argv[i],"-numcpu",7) == 0) ||
        (strncasecmp(argv[i],"-num_cpu",8) == 0) ){
      if((i+1) < argc){
        if(strcasecmp(argv[i+1],"all") == 0){
          num_cpus = max_cpus;
        }else{
          num_cpus = atoi(argv[i+1]);
        }
      }
      i++;
    }else if(strncasecmp(argv[i],"-o",2) == 0){
      if((i+1) < argc){
        has_oname = 1;
        strcpy(oname,argv[i+1]);
      }
      i++;
    }else if(strncasecmp(argv[i],"-r",2) == 0){
      if((i+1) < argc){
        rec_length = atoi(argv[i+1]);
      }
      i++;
    }else if(strncasecmp(argv[i],"-x",2) == 0){
      if((i+1) < argc){
        output_rec_length = atoi(argv[i+1]);
      }
      i++;
    }else if(strncasecmp(argv[i],"-l",2) == 0){
      skip_leading_ws = 0;
    }else if( (strncasecmp(argv[i],"-noc",4) == 0) ||
              (strncasecmp(argv[i],"-no-c",5) == 0) ||
              (strncasecmp(argv[i],"-x86",4) == 0) ){
      no_coproc = 1;
    }else if( (strncasecmp(argv[i],"-nof",4) == 0) ||
              (strncasecmp(argv[i],"-no-f",5) == 0) ){
      no_filter = 1;
    }else if(strncmp(argv[i],"-C",2) == 0){
      case_sensitive = 1;
    }else if(strncasecmp(argv[i],"-a",2) == 0){
      use_ascii = 1;
    }else if(strncasecmp(argv[i],"-p",2) == 0){
      skip_bitvec = 1;
    }else if(strncasecmp(argv[i],"-info",5) == 0){
      print_detail = 2;
    }else if(strncasecmp(argv[i],"-e",2) == 0){
      if((i+1) < argc){
        edit_distance = atoi(argv[i+1]);
      }
      i++;
    }
    i++;
  }

  if(num_cpus > MAX_CPUS)
    num_cpus = MAX_CPUS;

  if(num_cpus == 0)
    num_cpus = 1;

  if(num_cpus < 0)
    num_cpus = max_cpus;

  if(output_rec_length < 0){
    skip_bitvec = 1;
    output_rec_length = MXMAX(DEFAULT_XREC_LEN,abs(output_rec_length));
  }

  if(rec_length > 0){
    if(output_rec_length == 0){
      output_rec_length = rec_length;
    }else if(output_rec_length < rec_length){
      output_rec_length = rec_length;
    }
  }

  if(rec_length >= MAX_NAME_LEN){
    fprintf(stderr,"\nError, record length (%d) exceeds maximum (%d)\n\n",rec_length,MAX_NAME_LEN-1);
    fclose(infp);
    return 1;
  }

  if(output_rec_length >= MAX_NAME_LEN){
    fprintf(stderr,"\nError, output record length (%d) exceeds maximum (%d)\n\n",output_rec_length,MAX_NAME_LEN-1);
    fclose(infp);
    return 1;
  }

  if( (edit_distance < 0) || (edit_distance > 4) ){
    fprintf(stderr,"\nError, edit distance specified (%d) is not in valid range (0 <= edis <= 4)\n\n", edit_distance);
    fclose(infp);
    return 1;
  }

  if(!has_oname){
    strcpy(oname,fname);
    srtn = (int)strlen(oname);
    for(i=srtn-1;i>=0;i--){
      if(oname[i] == '.'){
        oname[i] = '\0';
        break;
      }
    }
    if(!skip_bitvec){
#ifdef SIMPLE_HASH
      strcat(oname,".shf");
#else
      strcat(oname,".chf");
#endif
    }else{
      strcat(oname,".hit");
    }
  }

  if(strcmp(oname,"-") == 0){

    outfp = stdout;
    use_ascii = 1;

  }else{

    outfp = fopen(oname,"w");
    if(outfp == NULL){
      fprintf(stderr,"\nError, cannot open reults file <%s>\n\n",oname);
      fclose(infp);
      return 1;
    }

  }

  if(no_filter){
    no_coproc = 1;
  }

  printf("\nCNYLEVED - Convey Levenshtein Edit Distance program (%s-%s)\n\n",VERSION,SVNREV);

  time_now = time(NULL);
  printf("start time  = %s",ctime(&time_now));

  printf("input file  = %s\n",fname);
  if(!skip_bitvec){
#ifdef SIMPLE_HASH
    printf("output file = %s (simple hash)\n",oname);
#else
    printf("output file = %s (convey hash)\n",oname);
#endif
  }else{
    printf("output file = %s\n",oname);
  }

  gethostname(hostname, 255);
  printf("hostname    = %s\n",hostname);
  // char dirbuf[PATH_MAX];
  // printf("curr dir    = %s\n",getcwd(dirbuf, PATH_MAX));
  printf("curr dir    = %s\n", get_current_dir_name());

  printf("skip lws    = %d\n",skip_leading_ws);
  printf("case sens   = %d\n",case_sensitive);

  if(rec_length > 0)
    printf("input recl  = %d\n",rec_length);

  // --------------

  srtn = init_structs();
  if(srtn != 0){
    fclose(infp);
    fclose(outfp);
    return 1;
  }

  sc = (*rdtsc0)();

  srtn = read_names(infp);
  if(srtn != 0){
    fclose(infp);
    fclose(outfp);
    return 1;
  }

  fclose(infp);

  if(output_rec_length == 0){
    output_rec_length = max_name_len;
  }

  if(output_rec_length == 0){
    output_rec_length = DEFAULT_XREC_LEN;
  }

  if(output_rec_length <= 0){
    fprintf(stderr,"\nError, invalid output record length (%d) specified\n\n", output_rec_length);
    return 1;
  }

  if(output_rec_length >= MAX_NAME_LEN){
    fprintf(stderr,"\nError, output record length (%d) exceeds maximum (%d)\n\n",output_rec_length,MAX_NAME_LEN-1);
    return 1;
  }

  printf("output recl = %d\n",output_rec_length);

  ec = (*rdtsc0)();
  total_spent = (double)((double)ec - (double)sc) / clocks_per_sec;

  printf("load file time = %.5lf (sec)\n",total_spent);
  printf("memory usage   = %lu (mb)\n",(mem_hiwater + 1048575) / 1048576);
  fflush(stdout);

  sc1 = (*rdtsc0)();

  process_names();

  ec = (*rdtsc0)();
  total_spent = (double)((double)ec - (double)sc1) / clocks_per_sec;

  fflush(NULL);

  printf("process names time = %.5lf (sec)\n",total_spent);
  printf("memory usage       = %lu (mb)\n",(mem_hiwater + 1048575) / 1048576);
  if(!no_coproc){
    printf("cp memory usage    = %lu (mb)\n",(cp_mem_hiwater + 1048575) / 1048576);
  }

  free_structs();

  fclose(outfp);

  ec = (*rdtsc0)();

  time_now = time(NULL);
  printf("end time           = %s",ctime(&time_now));

  total_spent = (double)((double)ec - (double)sc) / clocks_per_sec;
  printf("total program time = %.5lf (sec)\n",total_spent);

  return 0;
}

#endif // EXAMPLE
