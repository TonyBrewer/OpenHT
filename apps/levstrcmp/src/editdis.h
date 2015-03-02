/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT levstrcmp application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#ifndef _EDITDIS_H
# define _EDITDIS_H

#define MAX_CPUS                32
#define MAX_NAME_LEN            256
#define NUM_BLOCK_LNAMES        1000000
#define NUM_BLOCK_BUCKETLIST    100000
#define NUM_BLOCK_HITS          1000000
#define NUM_ENTRIES_PER_BUCKET  32768
#define COPROC_FILTER_WIDTH     16
#define COPROC_FILTER_SIZE      10
#define NUM_HASH_RECS           200000

#define DEFAULT_XREC_LEN        20

// #define SIMPLE_HASH

#ifdef EDITDIS_INIT
# define EXTERN  
#else
# define EXTERN extern
#endif

EXTERN int skip_bitvec;
EXTERN int rec_length;
EXTERN int output_rec_length;
EXTERN int skip_leading_ws;
EXTERN int max_name_len;
EXTERN unsigned char *lastnames;
EXTERN uint64_t siz_lastnames;
EXTERN uint64_t *lastindxs;
EXTERN uint64_t siz_lastindxs;
EXTERN uint64_t siz_bitvec;
EXTERN uint64_t siz_bucketlist;

EXTERN uint8_t ptable[256];

typedef struct{
  unsigned char hashIdx;
  unsigned char m_mask[16];
}CBloom;
EXTERN CBloom *pBloomList;

typedef struct{
  uint64_t index;   //  8
  uint32_t bvec[4]; // +16
  uint8_t  bhash;   // +1
  uint8_t  pad[7];  // +7 = 32
}BITVEC;
EXTERN BITVEC *bitvec;

struct BUCKET{
  unsigned char (*cpname)[COPROC_FILTER_WIDTH];
  uint64_t *index;
  uint64_t *indx2;
  uint32_t num_entries;
  uint32_t bucketid;
  uint64_t len;
  unsigned char bmask[MAX_NAME_LEN];
  struct BUCKET *next;
};
typedef struct{
  struct BUCKET *crec;
}BUCKETLIST;
EXTERN BUCKETLIST *bucketlist;

typedef struct{
  struct BUCKET *frec;
  struct BUCKET *crec;
  struct BUCKET *lrec;
  uint64_t num_buckets;
  uint64_t tot_entries;
}NBIN;
EXTERN NBIN nbin[MAX_NAME_LEN];

typedef struct{
  uint32_t indexL;
  uint32_t bucketidL;
  uint32_t indexR;
  uint32_t bucketidR;
}PAIRS;

typedef struct{
  int d;
  int hit;
  int lenL;
  int lenR;
  uint64_t indexL;
  uint64_t indexR;
  uint64_t indx2L;
  uint64_t indx2R;
}HITS;

typedef struct{
  long start;
  long end;
  HITS *hits;
  pthread_t tid;
  int nowork;
  int usecp;
  uint64_t *num_hits;
}THR_INFO;

EXTERN FILE *outfp;
EXTERN int max_cpus;
EXTERN int num_cpus;
EXTERN int no_coproc;
EXTERN int no_filter;
EXTERN int use_ascii;
EXTERN int edit_distance;
EXTERN int print_detail;
EXTERN int case_sensitive;
EXTERN uint64_t tot_num_names;
EXTERN double clocks_per_sec;
EXTERN uint64_t mem_curr;
EXTERN uint64_t mem_hiwater;
EXTERN uint64_t cp_mem_curr;
EXTERN uint64_t cp_mem_hiwater;

EXTERN uint64_t (*rdtsc0)(void);

extern void process_names(void);
extern int  coproc_filter(int lenL, int lenR, int adjD, int countL, int countR, int startL, 
                          uint32_t bucketidL, uint32_t bucketidR,
                          unsigned char (*nameL)[COPROC_FILTER_WIDTH],
                          unsigned char (*nameR)[COPROC_FILTER_WIDTH], PAIRS *pairs);
extern void CreateNameHashIdxList(void);
extern void SetBloomList(uint64_t idx1, uint64_t idx2);

#define MXMIN(a,b) ((a)<=(b) ? (a) : (b))
#define MXMAX(a,b) ((a)>=(b) ? (a) : (b))

#ifndef mckstr
#define mckstr(s) #s
#define mckxstr(s) ((char *)mckstr(s))
#endif

#endif // _EDITDIS_H
