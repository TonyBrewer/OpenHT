/* -*- mode: C; mode: folding; fill-column: 70; -*- */
/* Copyright 2010,  Georgia Institute of Technology, USA. */
/* See COPYING for license. */
#include "../compat.h"
#include "../timer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <xmmintrin.h>
#include <omp.h>
#include <semaphore.h>

#include "convey-csr.h"

#define MIN(a,b) ( (isnan(a)) ? (a) : ( (isnan(b)) ? (b) : ((a) < (b)) ? (a) : (b) ) )

static int64_t int64_fetch_add (int64_t* p, int64_t incr);
static int64_t int64_fetch_and (int64_t* p, int64_t incr);

void loop_indices(int64_t n, int64_t *istart_for_thread, int64_t *iend_for_thread, int64_t tid, int64_t number_threads);

void init_bfs_tree_wrapper ( int64_t nv, int64_t *bfs_tree);

void init_m1 (int64_t nv, int64_t *bfs_tree, __m128i one128i, int64_t tid, int64_t number_threads);

void top_down_openmp (int64_t nv, int64_t level, int64_t *k1, int64_t *k2, int64_t *oldk2, int64_t *bfs_packed_host, int64_t *vlist, int64_t *xoff, XADJ_SIZE *xadj, int64_t *bfs_bit_map_host, int64_t number_threads, int64_t tid);

void top_down_host (int64_t nv, int64_t *level, int64_t *k1, int64_t *k2, int64_t *bfs_packed_host, int64_t *vlist, int64_t *xoff, XADJ_SIZE *xadj, int64_t *bfs_bit_map_host, int64_t number_threads, int64_t tid, int64_t bit_map_size, __m128i one128i, int64_t srcvtx, int64_t bottom_constant);

double bottom_threshold();

double host_total_time = 0.0;

extern double walltime();

extern void barrier_mk_init(void);
extern void barrier_mk(int count);

static sem_t sem1;
static sem_t sem2;
static sem_t sem3;
static int my_count = 0;

int64_t sixty_three =  63;
int64_t six =  6;


void top_hybrid (int64_t nv, int64_t srcvtx, 
  double bottom_constant,
  int64_t *k1, int64_t *k2,
  int64_t *level,
  int64_t *bfs_tree, int64_t *vlist,
  int64_t *bfs_packed_host, int64_t *vlist_host, 
  int64_t *xoff, XADJ_SIZE *xadj,
  int64_t *bfs_bit_map_host,
  int64_t *bfs_bit_map_cp)
{

// nested parallelism is not done
#undef ENABLE_NESTING
//#define ENABLE_NESTING
#if defined (ENABLE_NESTING)
// ensure nested parallelism is enabled
//  int nest = omp_get_nested();
//  printf ("nesting value is %d \n",nest);
  int nest = 1;
  omp_set_nested(nest);

// ensure dynamic parallelism is enabled
//  int dyn = omp_get_dynamic();
//  printf ("dynamic value is %d \n",dyn);
  int dyn = 1;
  omp_set_dynamic(dyn);
#endif // ENABLE_NESTING

int64_t shift6 = 6;
int64_t and63_c = 0xffffffffffffff80;
int64_t one = 0x0000000000000001;

#define BIT_LENGTH(n) ( (n) == ( (n) & and63_c) ? ((n) >> shift6 ) : ((n >> shift6 ) + one) )


  int64_t bit_map_size;

  int64_t frontier, unvisited;

  int     one32 = 0xffffffff;
  __m128i one128i;


  int64_t number_threads, number_threads_minus_1;

  double t0;
#if defined (TIMER_ON)
  double one_gb_pts = 1024.*1024.*1024.;
#endif // TIMER_ON

  barrier_mk_init();

// all host processing

// determine size of bit_map
  bit_map_size = BIT_LENGTH(nv);

  one128i = _mm_set1_epi32 (one32);

  double real_host_total_time = walltime();

  OMP("omp parallel shared(t0, level, bottom_constant, frontier, unvisited, k1, k2, number_threads_minus_1, number_threads, host_total_time)")
  {
    int64_t tid, number_threads_for_host;
    OMP("omp single") 
    {
// set the number of threads to be 1 less than the maximum
      number_threads = omp_get_num_threads();
      number_threads_minus_1 = number_threads - 1;
#if defined (TIMER_ON)
      printf ("number_threads %d \n",number_threads);
#endif // TIMER_ON
    }
    tid = omp_get_thread_num();

///////////////////////////////////////////////////////////////////
// code for HC-2ex
      number_threads_for_host = number_threads_minus_1;
      if (tid != number_threads_minus_1)
      {
// threads 0-(n-2) here
          top_down_host (nv, level, k1, k2, bfs_packed_host, vlist_host, xoff, xadj, bfs_bit_map_host, number_threads_for_host, tid, bit_map_size, one128i, srcvtx, bottom_constant);

// copy data to coprocessor and began CP initialization of data
        OMP("omp single nowait") 
          {

//////////////////////////////////////////////////////////////
// copy packed data to coprocessor

#if defined (TIMER_ON)
      double copy_bfs_vlist_bit_time;
      t0 = walltime();
#endif // TIMER_ON

      int64_t size_bytes = sizeof(int64_t) * (bit_map_size+2*(*k2));
      pers_cp_memcpy(bfs_bit_map_cp, bfs_bit_map_host, size_bytes );

#if defined (TIMER_ON)
      copy_bfs_vlist_bit_time = walltime() - t0;
      printf ("copy bit map and twice k2 %ld rate (GB/s) %f time %f \n",bit_map_size+2*(*k2),(size_bytes/(one_gb_pts*copy_bfs_vlist_bit_time)),copy_bfs_vlist_bit_time);
      host_total_time += copy_bfs_vlist_bit_time;
      printf ("host (init+top_down+copy) time %f \n",host_total_time);
#endif // TIMER_ON

          }
      } // end 0-(n-2) region
    else
      {
#if defined (TIMER_ON)
        double init_cp_time  = walltime();
#endif // TIMER_ON

//////////////////////////////////////////////////////////////
// do CP processing
        init_bfs_tree_wrapper(nv, bfs_tree);
//////////////////////////////////////////////////////////////
#if defined (TIMER_ON)
        init_cp_time = walltime() - init_cp_time;
        printf ("init CP size %ld rate (GB/s) %f time %f\n",nv,(sizeof(int64_t)*nv/(one_gb_pts*init_cp_time)),init_cp_time);
#endif // TIMER_ON

      } // end (n-1) region

  } // end parallel

  real_host_total_time = walltime() - real_host_total_time;

  host_total_time = real_host_total_time;
#if defined (TIMER_ON)
  printf("The REAL time it takes for host + init is %g\n", real_host_total_time);
  printf ("adjusted host (init+top_down+copy) time %f \n",host_total_time);
#endif // TIMER_ON

  return;
}

//////////////////////////////////////

void
top_down_openmp (int64_t nv, int64_t level, int64_t *k1, int64_t *k2, int64_t *oldk2, int64_t *bfs_packed_host, int64_t *vlist, int64_t *xoff, XADJ_SIZE *xadj, int64_t *bfs_bit_map_host, int64_t number_threads, int64_t tid)
{

#define BFS_PACKED_HOST_X(k) (bfs_packed_host[2*k])
#define VLIST_X(k) (bfs_packed_host[2*k+1])

  int64_t one = 0x0000000000000001;
  int64_t zero  = 0x0000000000000000;
  int64_t k;

//#define THREAD_BUF_LEN_HOST 3 for debug
#define THREAD_BUF_LEN_HOST 30000
//#define THREAD_BUF_LEN_HOST 20000
//#define THREAD_BUF_LEN_HOST 10000
  int64_t nbuf[2*THREAD_BUF_LEN_HOST];
  int64_t kbuf = 0;
  int64_t vk;
  int64_t word_index, bit_index, bit_integer_value;
  int64_t v_level, bfs_j_bit_value, marked;

  int64_t istart_for_thread, iend_for_thread;
  int64_t n = *oldk2 - *k1;
  loop_indices (n, &istart_for_thread, &iend_for_thread, tid, number_threads);
  istart_for_thread += *k1;
  iend_for_thread += *k1;

// need barrier here on n-1 threads
  barrier_mk(number_threads);

//  printf ("n %d istart_for_thread %d iend_for_thread %d \n",n,istart_for_thread,iend_for_thread);
//  OMP("omp for")

  for (k = istart_for_thread; k < iend_for_thread; ++k) {
//  for (k = *k1; k < *oldk2; ++k) 
//    const int64_t v = vlist[k];
    const int64_t v = VLIST_X(k);
    const int64_t vso = xoff[2*v];
    const int64_t veo = xoff[2*v+1];
//           printf ("loop iter k %d \n",k);
    int64_t vo;
    v_level = v;
    for (vo = vso; vo < veo; ++vo) {
      const int64_t j = xadj[vo];
// check bit value
      word_index = j >> six;
      bit_index = j & sixty_three;
      bit_integer_value = ~(one << bit_index);

      bfs_j_bit_value = (bfs_bit_map_host[word_index] >> bit_index) & one;
      if ( bfs_j_bit_value != one ) continue;

// do a fetch and AND
//  if the fetched value still is unmarked then continue with the processing
//  get value and hammer the appropriate bit - then test fetch
        marked = int64_fetch_and (&bfs_bit_map_host[word_index], bit_integer_value);
        if ( (marked & (~bit_integer_value) ) == zero ) continue;
// update
#define BETTER_PARALLEL
//#undef BETTER_PARALLEL
#if defined (BETTER_PARALLEL)
        if (kbuf < THREAD_BUF_LEN_HOST) 
          {
// add to queue
/*
            nbuf_bfs_packed[kbuf] = bfs_tree[j];
            nbuf[kbuf++] = j;
*/
            nbuf[2*kbuf] = v_level;
            nbuf[2*kbuf+1] = j;
            kbuf++;

          } else {
// empty queue
            int64_t voff = int64_fetch_add (k2, THREAD_BUF_LEN_HOST);
            for (vk = 0; vk < THREAD_BUF_LEN_HOST; ++vk) {
              int64_t voff_vk = voff + vk;
              BFS_PACKED_HOST_X(voff_vk) = nbuf[2*vk];
              VLIST_X(voff_vk) = nbuf[2*vk+1];
            }
// put newest elements in queue
            nbuf[0] = v_level;
            nbuf[1] = j;
            kbuf = 1;
        }
#else

// working
// use a thread specific temporary array to reduce number of atomic instructions

        int64_t k2_temp;
        k2_temp = int64_fetch_add (k2,one);
// interleave these two
/*
        bfs_packed_host[k2_temp] = v_level;
        vlist[k2_temp] = j;
*/
        BFS_PACKED_HOST_X(k2_temp) = v_level;
        VLIST_X(k2_temp) = j;
#endif

    }
  } // end k loop

#if defined (BETTER_PARALLEL)
// drain final elements from queue
  if (kbuf) {
    int64_t voff = int64_fetch_add (k2, kbuf);
    for (vk = 0; vk < kbuf; ++vk) {
      int64_t voff_vk = voff + vk;
      BFS_PACKED_HOST_X(voff_vk) = nbuf[2*vk];
      VLIST_X(voff_vk) = nbuf[2*vk+1];
    }
  }
#endif

  return;
}
//////////////////////////////////////////////////////////////////////
void top_down_host (int64_t nv, int64_t *level, int64_t *k1, int64_t *k2, int64_t *bfs_packed_host, int64_t *vlist_host, int64_t *xoff, XADJ_SIZE *xadj, int64_t *bfs_bit_map_host, int64_t number_threads, int64_t tid, int64_t bit_map_size, __m128i one128i, int64_t srcvtx, int64_t bottom_constant)
{
#define BFS_PACKED_HOST_X(k) (bfs_packed_host[2*k])
#define VLIST_X(k) (bfs_packed_host[2*k+1])

  int64_t k;
  int64_t one = 0x0000000000000001;

  int64_t frontier, unvisited;

#if defined (TIMER_ON)
  double t0;
  double one_gb_pts = 1024.*1024.*1024.;
#endif // TIMER_ON

#if defined (TIMER_ON)
  if (tid == 0)	// use tid 0 to ensure same thread is timing
    {
      t0 = walltime();
    }
#endif // TIMER_ON

  init_m1 (bit_map_size, bfs_bit_map_host, one128i, tid, number_threads);

#if defined (TIMER_ON)
  if (tid == 0)	// use tid 0 to ensure same thread is timing
    {
      double init_time = walltime() - t0;
      host_total_time = init_time;
      printf ("init host bit map size %ld rate (GB/s) %f time %f\n",bit_map_size,(sizeof(int64_t)*bit_map_size/(one_gb_pts*init_time)),init_time);
    }
#endif // TIMER_ON

  barrier_mk(number_threads);

// there are 2 sections of code that can be done in parallel
// but running a single thread is fine
  OMP("omp single nowait")
    {
////////////////////////////////////////////////////////////////
// fix last points

// finally clear out any stray points at end when not a multiple of 64
// so want to create 0      ... 0         1       ...
//                  off-end ... off-end  included included
// there are
      int64_t k;
      int64_t word_index, bit_index, bit_integer_value, bit_scalar;
      int64_t off_end_points = 64*bit_map_size - nv;
//    printf ("off_end_points %d \n",off_end_points);
      for (k=(64-off_end_points);k < 64; k++) {
        word_index = bit_map_size-1;
        bit_index = k;
        bit_scalar = bfs_bit_map_host[word_index];
        bfs_bit_map_host[word_index] = bit_scalar & (~(one << bit_index));
      }

////////////////////////////////////////////////////////////////
// Level 1 processing for bfs, vlist, bit_map
      *level = 1;    // note that level counting should commence with 1 not 0

#define BFS_PACKED_X(k) (bfs_packed_host[2*k])
#define VLIST_X(k) (bfs_packed_host[2*k+1])

// interleave now
      BFS_PACKED_X(0) = srcvtx; // level 1
      VLIST_X(0) = srcvtx;

// Level 1 bit_map update (does not need level number though)
// mark first point
      int64_t j = srcvtx; // vlist_host[0];
      word_index = j >> six;
      bit_index = j & sixty_three;
      bit_integer_value = one << bit_index;
      bfs_bit_map_host[word_index] &= (~bit_integer_value);
//
// Level 2 setup
      *level = 2;
      *k1 = 0; *k2 = 1;

////////////////////////////////////////////////////////////////
    } // end single
    frontier = 1; unvisited = nv - 1;	// local to each thread
    barrier_mk(number_threads);
////////////////////////////////////////////////////////////////

// do top down steps
    while ((*k1 != *k2) && ( bottom_constant*(frontier*frontier) < unvisited ) ) {
      int64_t oldk2 = *k2;
//    printf ("start of while: tid %d *k1 %ld *k2 %ld oldk2 %ld level %ld \n",tid,*k1,*k2,oldk2,*level);
#if defined (TIMER_ON)
      if (tid == 0)	// use tid 0 to ensure same thread is timing
        {
          t0 = walltime();
        }
#endif // TIMER_ON

      top_down_openmp (nv, *level, k1, k2, &oldk2, bfs_packed_host, vlist_host, xoff, xadj, bfs_bit_map_host, number_threads, tid);

// need barrier
      barrier_mk(number_threads);

      OMP("omp single nowait") 
        {
          *k1 = oldk2;
          *level = *level + 1;
        }

      barrier_mk(number_threads);

      frontier = *k2 - *k1;
      unvisited = nv - *k2;

#if defined (TIMER_ON)
      if (tid == 0)	// use tid 0 to ensure same thread is timing
        {
          double top_time = walltime() - t0;
          host_total_time += top_time;
          double unv_front2_ratio = (double)unvisited/(double)(frontier*frontier);
          printf ("top_down: *k1 %ld *k2 %ld level %ld tb_ratio %f time %f\n",*k1,*k2,*level-1, unv_front2_ratio, top_time);
          printf ("next iter will use frontier %ld unvisited %ld ratio %f\n",frontier,unvisited,unv_front2_ratio);
        }
#endif // TIMER_ON

    }

  return;
}
//////////////////////////////////////////////////////////////////////

void
init_m1 (int64_t n, int64_t *x, __m128i one128i, int64_t tid, int64_t number_threads)
{
/* Initialize n points in memory by -1 */

/* if this needs to be modified for avx see
/usr/lib/gcc/x86_64-redhat-linux/4.4.4/include/avxintrin.h
or later

#include <avxintrin.h>
_mm256_stream_si256 (__m256i *__A, __m256i __B)


_mm256_set1_epi64x (long long __A)
{
  return __extension__ (__m256i)(__v4di){ __A, __A, __A, __A };
}
*/
  int64_t one64 = 0xffffffffffffffff;

  int64_t thirtyone = 31;
  int64_t three = 3;
  int64_t unroll = 4;
  int64_t k1;

//  if (n <= 0) return;

// number thread threads to use calculation done in calling routine

/* want cache bypass for the stores

   step to the nearest 32-byte aligned data
     so do 1-3 point if necessary
   do multiples of 4
   finish (if necessary) with 1-3 more points

   note that the compiler also unrolls by 4
*/

// starting point
  int64_t offset_start, offset_end, istart;
  int64_t istart_for_thread, iend_for_thread;
  int64_t iend_minus_three;

  loop_indices (n, &istart_for_thread, &iend_for_thread, tid, number_threads);

//#define DEBUG_INITM1
#undef DEBUG_INITM1
#if defined (DEBUG_INITM1)
  printf ("n %d istart_for_thread %d iend_for_thread %d \n",n,istart_for_thread,iend_for_thread);
#endif // DEBUG_INITM1

// each thread will get a section of the code to do,
//  but then must do an appropriate alignment

// hand partition the loop
// on the HC-2ex use the first n-1 threads and not the last thread

/*
  offset_start = (thirtyone & (long)&(x[0])) >> three;
  istart = (unroll - offset_start) % unroll;
*/

  offset_start = (thirtyone & (long)&(x[istart_for_thread])) >> three;
  istart = istart_for_thread + (unroll - offset_start) % unroll;
  iend_minus_three = iend_for_thread - 3;
  offset_end = (iend_for_thread-istart) & three;

//#define GENERIC_COPY
#undef GENERIC_COPY
#if defined (GENERIC_COPY)
  for (k1 = istart_for_thread; k1 < iend_for_thread; k1++) {
    x[k1] = one64;
  }
#else // GENERIC_COPY

//  if (n <= 256)
// must be at least 4 points per thread for unroll logic below to work
  if ((iend_for_thread - istart_for_thread) <= 4)
    {
// do generic code
      for (k1 = istart_for_thread; k1 < iend_for_thread; k1++) {
        x[k1] = one64;
      }
    }
   else
  {
#if defined (DEBUG_INITM1)
  printf ("tid %d offset_start %d istart_for_thread %d offset_end %d iend_for_thread %d \n",tid,offset_start,istart_for_thread,offset_end,iend_for_thread);
#endif // DEBUG_INITM1
//  OMP("omp single nowait")
  {

// do 1-3 points if not 16-byte aligned
    switch ( offset_start ) {
      case 1:
        x[0+istart_for_thread] = one64;
        x[1+istart_for_thread] = one64;
        x[2+istart_for_thread] = one64;
#if defined (DEBUG_INITM1)
        printf ("tid %d 3 start pts \n %d\n %d\n %d\n",tid,0+istart_for_thread, 1+istart_for_thread, 2+istart_for_thread  );
#endif // DEBUG_INITM1
        break;
      case 2:
        x[0+istart_for_thread] = one64;
        x[1+istart_for_thread] = one64;
#if defined (DEBUG_INITM1)
        printf ("tid %d 2 start pts \n %d\n %d\n",tid,0+istart_for_thread, 1+istart_for_thread );
#endif // DEBUG_INITM1
        break;
      case 3:
        x[0+istart_for_thread] = one64;
#if defined (DEBUG_INITM1)
        printf ("tid %d 1 start pts \n %d\n",tid,0+istart_for_thread );
#endif // DEBUG_INITM1
        break;
      default:
        break;
    }

// do final 1-3 points if necessary
    switch ( offset_end ) {
      case 1:
        x[iend_for_thread-1] = one64;
#if defined (DEBUG_INITM1)
        printf ("tid %d 1 end pts \n %d\n",tid,iend_for_thread-1 );
#endif // DEBUG_INITM1
        break;
      case 2:
        x[iend_for_thread-2] = one64;
        x[iend_for_thread-1] = one64;
#if defined (DEBUG_INITM1)
        printf ("tid %d 2 end pts \n %d\n %d\n",tid,iend_for_thread-2, iend_for_thread-1 );
#endif // DEBUG_INITM1
        break;
      case 3:
        x[iend_for_thread-3] = one64;
        x[iend_for_thread-2] = one64;
        x[iend_for_thread-1] = one64;
#if defined (DEBUG_INITM1)
        printf ("tid %d 3 end pts \n %d\n %d\n %d\n",tid, iend_for_thread-3, iend_for_thread-2, iend_for_thread-1 );
#endif // DEBUG_INITM1
        break;
      default:
        break;
    }

  } // end single

#if defined (DEBUG_INITM1)
//  printf ("tid %d n %d istart %d iend_minus_three %d \n",tid,n,istart,iend_minus_three);
#endif // DEBUG_INITM1
//  OMP("omp for")
    for (k1 = istart; k1 < iend_minus_three; k1+=unroll) {
#if defined (DEBUG_INITM1)
        printf ("tid %d in loop points \n %d\n %d\n %d\n %d\n",tid,k1,k1+1,k1+2,k1+3);
#endif // DEBUG_INITM1
/*
      x[4*k1] = one64;
      x[4*k1+1] = one64;
      x[4*k1+2] = one64;
      x[4*k1+3] = one64;
*/
// Have not seen a large performance difference between the two stores
// for the small sizes I've looked at so far
// Also I may want the data to reside in cache after the store

// do not dirty the cache
/*
      _mm_stream_si128((__m128i*)&x[k1], one128i);
      _mm_stream_si128((__m128i*)&x[k1+2], one128i);
*/

// works better for scale 24
// dirty the cache
      _mm_store_si128((__m128i*)&x[k1], one128i);
      _mm_store_si128((__m128i*)&x[k1+2], one128i);
    }
  }
#endif // GENERIC_COPY

  return;
}

////////////////////////////////////////////////////////////////
void loop_indices(int64_t n, int64_t *istart_for_thread, int64_t *iend_for_thread, int64_t tid, int64_t number_threads)
{
/*
   return loop start and end points given the original loop size,
   the thread id and the number of threads to use
*/
  int64_t n_work;
//  n_work = (n / number_threads) + 1;
  n_work = (n / number_threads);
// if not exactly divisible, bump the size by 1
  if ((number_threads * n_work) != n)
    n_work++;
  
  *istart_for_thread = MIN( tid * n_work, n);
  *iend_for_thread = MIN( (tid+1) * n_work, n);

  return;
}

//////////////////////////////////////////////////////////////////////

double
bottom_threshold()
{
// bottom_constant allows fine tuning of the number of top down/bottom up steps
//  start with a value of 1 since this would be correct if the
//  relative performance of top down and bottom up are the same
//   as bottom_constant increases more bottom up steps are done
//
//  bottom_constant = 1: example on s16 this gives level4:8, level3:64 steps
//  bottom_constant = 2: example on s16 this gives level4:4, level3:63 steps
//
// bottom constant should be a function of top down vs. bottom up bandwidth
//
/*
 Calculations for formula to determine threshold
 host
 HC-1/HC-1ex	bw  4 for unit stride, divide by 4.5 for gathers
 HC-2/HC-2ex	bw 24 for unit stride, divide by 4.5 for gathers

 CP
 ISC rate	bw 30 for gather
 July 2012	bw 50 for gather

*/
  double CP_bw = 50.0;
  double ratio_host_unit_to_gather = 4.5;
  double host_unit_bw = 24.0;

  double host_gather_bw = host_unit_bw / ratio_host_unit_to_gather;
  double bottom_constant = CP_bw / host_gather_bw;
#if defined (TIMER_ON)
  printf ("bottom_constant is %f\n", bottom_constant);
#endif // TIMER_ON

   return bottom_constant;
}

////////////////////////////////////////////////////////////////


void barrier_mk_init(void)
{
  sem_init(&sem1, 0, 1);
  sem_init(&sem2, 0, 0);
  sem_init(&sem3, 0, 1);

  return;
}

void barrier_mk(int count)
{
//  usleep(1000000*tid); // only for testing

  sem_wait(&sem1);

  my_count++;

  if(my_count == count){
    sem_wait(&sem3);
    sem_post(&sem2);
  }

  sem_post(&sem1);

  sem_wait(&sem2);
  sem_post(&sem2);

  sem_wait(&sem1);

  my_count--;

  if(my_count == 0){
    sem_wait(&sem2);
    sem_post(&sem3);
  }

  sem_post(&sem1);

  sem_wait(&sem3);
  sem_post(&sem3);

  return;
}
////////////////////////////////////////////////////////////////

int64_t
int64_fetch_add (int64_t* p, int64_t incr)
{
  return __sync_fetch_and_add (p, incr);
}

int64_t
int64_fetch_and (int64_t* p, int64_t incr)
{
  return __sync_fetch_and_and (p, incr);
}
