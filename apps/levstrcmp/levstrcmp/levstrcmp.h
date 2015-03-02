/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT levstrcmp application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#ifndef _LEVSTRCMP_H
# define _LEVSTRCMP_H

#ifndef VERSION
#define VERSION "X"
#endif
#ifndef SVNREV
#define SVNREV "X"
#endif

#define MAX_CPUS           32
#define NUM_BLOCK_LNAMES   1000000
#define NUM_BLOCK_HITS     1000000

#ifdef LEVSTRCMP_INIT
# define EXTERN  
#else
# define EXTERN extern
#endif

// #define SIMPLE_HASH

// --------------

#ifdef SIMPLE_HASH

#define MAX_NAME_LEN           256

typedef struct{
  uint32_t bvec[4]; //  16
  uint8_t  bhash;   // +1
  uint8_t  pad[15]; // +15 = 32
}HASHREC;

typedef struct{
  HASHREC hrec;
  int len;
  unsigned char lname[MAX_NAME_LEN];
}LNAMEREC;
EXTERN LNAMEREC *lnamerec;

#else

#define MAX_NAME_LEN           256

typedef struct{
  unsigned char m_mask[16];
}CBloom;

typedef struct{
  unsigned char m_hashIdx;
  CBloom bloom;
  int len;
  unsigned char lname[MAX_NAME_LEN];
}LNAMEREC;
EXTERN LNAMEREC *lnamerec;

#endif

EXTERN int doio;
EXTERN int num_cpus;
EXTERN int auto_seed;
EXTERN int pad_length;
EXTERN int rec_length;
EXTERN int trimmed_strings;
EXTERN int edistance;
EXTERN int compare_algorithms;
EXTERN int skip_leading_ws;
EXTERN long num_lhs;
EXTERN uint64_t siz_lnamerec;
EXTERN FILE *outfp;
EXTERN uint64_t tot_num_names;
EXTERN double clocks_per_sec;
EXTERN uint64_t mem_curr;
EXTERN uint64_t mem_hiwater;

EXTERN uint64_t (*rdtsc0)(void);

extern void process_names(int use_bvec);

#define MXMIN(a,b) ((a)<=(b) ? (a) : (b))
#define MXMAX(a,b) ((a)>=(b) ? (a) : (b))

#endif // _LEVSTRCMP_H
