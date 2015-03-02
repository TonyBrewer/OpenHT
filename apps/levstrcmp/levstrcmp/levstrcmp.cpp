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

#define LEVSTRCMP_INIT
#include "levstrcmp.h"

static int case_sensitive = 0;

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
  siz_lnamerec = NUM_BLOCK_LNAMES;
  lnamerec = (LNAMEREC *)malloc((siz_lnamerec+1) * sizeof(LNAMEREC));
  if(lnamerec == NULL){
    fprintf(stderr,"\nError, unable to alloc mem for lnamerec data (%lu)\n\n",(siz_lnamerec+1));
    return 1;
  }
  mem_curr += (siz_lnamerec+1) * sizeof(unsigned char);
  mem_hiwater = MXMAX(mem_curr,mem_hiwater);

  // --------------

  clocks_per_sec = 1000000.0;
  rdtsc0 = &rdtsc2;

  return 0;
}

// --------------

void free_structs(void)
{
  if(lnamerec != NULL){
    free(lnamerec);
    lnamerec = NULL;
  }
  siz_lnamerec = 0;

  return;
}

// --------------

#ifdef SIMPLE_HASH
int get_next_name(FILE *fp, char *lastname, int use_bfilter, uint8_t *hashIdx, uint64_t *bvec)
#else
int get_next_name(FILE *fp, char *lastname, int use_bfilter, unsigned char *hashIdx, unsigned char *m_mask)
#endif
{
  int i = 0;
  int j = 0;
  int len = 0;
  int beg = 0;
  char line[2048] = { "" };

  if(rec_length == 0){
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
      if(use_bfilter >= 0){
#ifdef SIMPLE_HASH
        i = fread(hashIdx, sizeof(uint8_t), 1, fp);
        if(i != 1){
          return -1;
        }
        i = fread(bvec, sizeof(uint64_t), 4, fp);
        if(i != 4){
          return -1;
        }
#else
        i = fread(hashIdx, sizeof(unsigned char), 1, fp);
        if(i != 1){
          return -1;
        }
        i = fread(m_mask, sizeof(unsigned char), 16, fp);
        if(i != 16){
          return -1;
        }
#endif
      }
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

int read_names(FILE *fp, int use_bfilter)
{
  int i = 0;
  int j = 0;
  int len = 0;
  uint64_t indx = 0;
  char lastname[3*MAX_NAME_LEN+1] = { "" };
  int64_t min_len = LONG_MAX - 1;
  int64_t max_len = 0;
  int64_t cnt_len = 0;
#ifdef SIMPLE_HASH
  uint8_t hashIdx;
  uint64_t bvec[4];
#else
  unsigned char hashIdx;
  unsigned char m_mask[16];
#endif

  indx = 0;
#ifdef SIMPLE_HASH
  while((len = get_next_name(fp,lastname,use_bfilter,&hashIdx,bvec)) >= 0){
#else
  while((len = get_next_name(fp,lastname,use_bfilter,&hashIdx,m_mask)) >= 0){
#endif

#if 0
    printf("name = %2d <%s>\n",len,lastname);
#endif

    if(len < min_len){
      min_len = len;
    }

    if(len > max_len){
      max_len = len;
    }

    if(len > pad_length){
      fprintf(stderr,"\nError, name length (%d) exceeds max (%d) (%s)\n\n",len,pad_length,lastname);
      return 1;
    }

    cnt_len += len;

    if((indx+1) >= siz_lnamerec){
      mem_curr -= (siz_lnamerec+1) * sizeof(LNAMEREC);
      siz_lnamerec += NUM_BLOCK_LNAMES;
      lnamerec = (LNAMEREC *)realloc(lnamerec, ((siz_lnamerec+1) * sizeof(LNAMEREC)));
      if(lnamerec == NULL){
        fprintf(stderr,"\nError, unable to expand mem for lnamerec data (%lu)\n\n",(siz_lnamerec+1));
        return 1;
      }
      mem_curr += (siz_lnamerec+1) * sizeof(LNAMEREC);
      mem_hiwater = MXMAX(mem_curr,mem_hiwater);
    }

    if(!trimmed_strings){

      // pad name with trailing spaces to pad_length
      // to match how records are really stored and 
      // trimmed down again inside Levenshtein() ...
   
      if(len < pad_length){
        for(i=len;i<pad_length;i++){
          lastname[i] = ' ';
        }
        len = pad_length;
      }
      lastname[len] = '\0';

    }

#if 0
    printf("name = %2d <%s>\n",len,lastname);
#endif

    lnamerec[indx].len = len;

    memcpy(lnamerec[indx].lname, lastname, (len+1));

    if(use_bfilter > 0){

#ifdef SIMPLE_HASH
      lnamerec[indx].hrec.bhash = hashIdx;

      for(j=0;j<4;j++){
        lnamerec[indx].hrec.bvec[j] = bvec[j];
      }
#else
      lnamerec[indx].m_hashIdx = hashIdx;

      for(j=0;j<16;j++){
        lnamerec[indx].bloom.m_mask[j] = m_mask[j];
      }
#endif

#if 0
      printf("hash = %3u\n",lnamerec[indx].m_hashIdx);
      printf("bitvec = ");
      for(int j=0;j<16;j++){
        printf(" 0x%-2x",lnamerec[indx].bloom.m_mask[j]);
      }
      printf("\n");
#endif

    }

    indx++;

  }

  tot_num_names = indx;

  printf("num names   = %lu\n",tot_num_names);
  printf("min len     = %lu\n",min_len);
  printf("max len     = %lu\n",max_len);
  printf("mean len    = %lu\n",cnt_len / tot_num_names);
  fflush(stdout);

  return 0;
}

// --------------

int main(int argc, char *argv[])
{
  int i = 0;
  int srtn = 0;
  int has_oname = 0;
  int use_bfilter = -1;
  int algorithm = 0;
  FILE *infp = NULL;
  char fname[256] = { "" };
  char oname[256] = { "" };
  char hostname[256] = { "" };
  uint64_t sc, ec;
  double total_spent = 0.0;
  time_t time_now = 0;

  doio = 1;
  num_cpus = 1;
  num_lhs = -1;
  pad_length = 0;
  trimmed_strings = 0;
  edistance = 2;
  auto_seed = 0;
  rec_length = -1;
  skip_leading_ws = 1;
  outfp = NULL;
  case_sensitive = 0;

  if(argc < 2){
    fprintf(stderr,"\nError, need args: <input-file> [-recl ##] [-pad ##] [-bfilter 0|1] [-lws] [-seedauto] [-nlhs #lhs] [-edis ##] [-cut] [-Case-sensitive] [-xio | -o <output-file>] [-t threads]\n\n");
    return 1;
  }

  if(strncasecmp(argv[1],"-v",2) == 0){
    printf("\nLEVSTRCMP - Levenshtein String Compare program (%s-%s)\n\n",VERSION,SVNREV);
    return 0;
  }else if( (strncasecmp(argv[1],"-h",2) == 0) ||
            (strncasecmp(argv[1],"-u",2) == 0) ||
            (strncasecmp(argv[1],"-?",2) == 0)  ){
    fprintf(stderr,"\nprogram usage: <input-file> [-recl ##] [-pad ##] [-bfilter 0|1] [-lws] [-seedauto] [-nlhs #lhs] [-edis ##] [-cut] [-Case-sensitive] [-xio | -o <output-file>] [-t threads]\n\n");
    return 0;
  }

  strcpy(fname,argv[1]);

  infp = fopen(fname,"r");
  if(infp == NULL){
    fprintf(stderr,"\nError, cannot open input file <%s>\n\n",fname);
    return 1;
  }

  mem_curr = 0;
  has_oname = 0;
  mem_hiwater = 0;
  use_bfilter = -1;

  i = 2;
  while(i < argc){
    if(strncasecmp(argv[i],"-b",2) == 0){
      if((i+1) < argc){
        use_bfilter = atoi(argv[i+1]);
      }
      i++;
    }else if(strncasecmp(argv[i],"-r",2) == 0){
      if((i+1) < argc){
        rec_length = atoi(argv[i+1]);
      }
      i++;
    }
    i++;
  }

  if( (rec_length != 0) && (use_bfilter < 0) ){
    use_bfilter = 0;
  }

  if(use_bfilter >= 0){

    // read header info ...

    srtn = fread(&i, sizeof(int), 1, infp);
    if(srtn == 1){
      if(i == 0){
        srtn = fread(&i, sizeof(int), 1, infp);
        if(srtn == 1){
          if(rec_length < 0)
            rec_length = i;
        }
        srtn = fread(&i, sizeof(int), 1, infp);
        if(srtn == 1){
          edistance = i;
        }
        srtn = fread(&i, sizeof(int), 1, infp);
        if(srtn == 1){
          skip_leading_ws = i;
        }
        srtn = fread(&i, sizeof(int), 1, infp);
        if(srtn == 1){
          // lower half is alg, upper half is case_sensitive ...
          algorithm = i & 0xff;
          // Levenshtein is 1 ...
          if(algorithm != 1){
            fprintf(stderr,"\nError, compare alg in header (%d) is not valid\n\n", algorithm);
            return 1;
          }
          case_sensitive = i >> 16;
        }
      }
    }

  }

  i = 2;
  while(i < argc){
    if(strncasecmp(argv[i],"-o",2) == 0){
      if((i+1) < argc){
        strcpy(oname,argv[i+1]);
        has_oname = 1;
      }
      i++;
    }else if(strncasecmp(argv[i],"-p",2) == 0){
      if((i+1) < argc){
        pad_length = atoi(argv[i+1]);
      }
      i++;
    }else if(strncasecmp(argv[i],"-n",2) == 0){
      if((i+1) < argc){
        num_lhs = atol(argv[i+1]);
      }
      i++;
    }else if(strncasecmp(argv[i],"-t",2) == 0){
      if((i+1) < argc){
        num_cpus = atoi(argv[i+1]);
      }
      i++;
    }else if(strncasecmp(argv[i],"-e",2) == 0){
      if((i+1) < argc){
        edistance = atoi(argv[i+1]);
      }
      i++;
    }else if(strncmp(argv[i],"-c",2) == 0){
      trimmed_strings = 1;
    }else if(strncmp(argv[i],"-C",2) == 0){
      case_sensitive = 1;
    }else if(strncasecmp(argv[i],"-l",2) == 0){
      skip_leading_ws = 0;
    }else if(strncasecmp(argv[i],"-s",2) == 0){
      auto_seed = 1;
    }else if(strncasecmp(argv[i],"-x",2) == 0){
      doio = 0;
    }
    i++;
  }

  if(rec_length < 0){
    rec_length = 0;
  }

  if(rec_length > 0){
    if(pad_length < rec_length){
      pad_length = rec_length;
    }
  }

  if(pad_length == 0){
    pad_length = MAX_NAME_LEN-1;
  }

  if(pad_length > (MAX_NAME_LEN-1)){
    fprintf(stderr,"\nError, pad length (%d) exceeds max (%d)\n\n",pad_length,(MAX_NAME_LEN-1));
    return 1;
  }

  if(num_cpus == 0){
    num_cpus = 1;
  }

  if(num_cpus < 1){
#ifdef _SC_NPROCESSORS_ONLN
    num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
#else
    num_cpus = omp_get_num_threads(num_cpus);
#endif
  }

  if(num_cpus > MAX_CPUS){
    num_cpus = MAX_CPUS;
  }

  omp_set_num_threads(num_cpus);

  int num_threads = omp_get_max_threads();

  if(num_threads != num_cpus){
    fprintf(stderr,"\nError, num threads (%d) does not equal requested value (%d)\n\n",num_threads,num_cpus);
    exit(1);
  }

  if(doio){

    if(!has_oname){
      strcpy(oname,fname);
      srtn = (int)strlen(oname);
      for(i=srtn-1;i>=0;i--){
        if(oname[i] == '.'){
          oname[i] = '\0';
          break;
        }
      }
      strcat(oname,".hit");
    }
   
    if(strcmp(oname,"-") == 0){
   
      outfp = stdout;
      doio = 2;

    }else{
   
      outfp = fopen(oname,"w");
      if(outfp == NULL){
        fprintf(stderr,"\nError, cannot open reults file <%s>\n\n",oname);
        fclose(infp);
        return 1;
      }
   
    }

  }

  printf("\nLEVSTRCMP - Levenshtein String Compare program (%s-%s)\n\n",VERSION,SVNREV);

  time_now = time(NULL);
  printf("start time  = %s",ctime(&time_now));

  printf("input file  = %s",fname);
  if(rec_length > 0){
#ifdef SIMPLE_HASH
    printf(" (simple hash)\n");
#else
    printf(" (convey hash)\n");
#endif
    printf("rec length  = %d\n",rec_length);
    if(pad_length != rec_length){
      printf("pad length  = %d\n",pad_length);
    }
  }else{
    printf("\n");
    printf("pad length  = %d\n",pad_length);
  }
  printf("trim strs   = %d\n",trimmed_strings);
  if(doio){
    printf("output file = %s\n",oname);
  }else{
    printf("output file = %s\n","<no output>");
  }
  printf("edit dis    = %d\n",edistance);
  if(use_bfilter > 0){
    printf("algorithm   = Bloom\n");
  }else{
    printf("algorithm   = Levenshtein\n");
  }
  gethostname(hostname, 255);
  printf("hostname    = %s\n",hostname);
  printf("curr dir    = %s\n",get_current_dir_name());

  printf("skip lws    = %d\n",skip_leading_ws);
  printf("case sens   = %d\n",case_sensitive);

  printf("num cpus    = %d\n",num_cpus);

  // --------------

  srtn = init_structs();
  if(srtn != 0){
    fclose(infp);
    if(doio == 1)
      fclose(outfp);
    return 1;
  }

  sc = (*rdtsc0)();

  srtn = read_names(infp, use_bfilter);
  if(srtn != 0){
    fclose(infp);
    if(doio == 1)
      fclose(outfp);
    return 1;
  }

  fclose(infp);

  ec = (*rdtsc0)();
  total_spent = (double)((double)ec - (double)sc) / clocks_per_sec;

  printf("load file time = %.5lf (sec)\n",total_spent);
  printf("memory usage   = %lu (mb)\n",(mem_hiwater + 1048575) / 1048576);
  fflush(stdout);

  if(use_bfilter > 0){
    if(num_cpus > 1){
      printf("\nWarning, num_cpus (%d) > 1 which can skew bit-vector performance comparisons\n\n", num_cpus);
      fflush(stdout);
    }
  }

  process_names(use_bfilter);

  fflush(NULL);

  printf("memory usage       = %lu (mb)\n",(mem_hiwater + 1048575) / 1048576);

  free_structs();

  if(doio == 1)
    fclose(outfp);

  ec = (*rdtsc0)();

  time_now = time(NULL);
  printf("end time           = %s",ctime(&time_now));

  total_spent = (double)((double)ec - (double)sc) / clocks_per_sec;
  printf("total program time = %.5lf (sec)\n",total_spent);

  return 0;
}
