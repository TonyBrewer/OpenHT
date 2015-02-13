/* Copyright 2010,  Georgia Institute of Technology, USA. */
/* See COPYING for license. */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include <assert.h>
#include <alloca.h>
#include <omp.h>
#include <pthread.h>

#include "convey-csr.h"
#include "../timer.h"
#include "../compat.h"

extern void g500_hyb_wrapper ( int64_t g500_ctl, int64_t nv,
               int64_t *bfs_tree,
	       XADJ_SIZE *xadj, int64_t *xoff,
               int64_t *bfs_packed_cp,
               int64_t *bfs_bit_map_cp, int64_t *bfs_bit_map_new_cp,
               int64_t level, int64_t *k1, int64_t *k2);

static int64_t int64_fetch_add (int64_t* p, int64_t incr);
static int64_t int64_casval(int64_t* p, int64_t oldval, int64_t newval);
static int int64_cas(int64_t* p, int64_t oldval, int64_t newval);

double bottom_threshold();

void top_hybrid (int64_t nv, int64_t srcvtx, double bottom_constant,
  int64_t *k1, int64_t *k2, 
  int64_t *level, 
  int64_t *bfs_tree, int64_t *vlist,
  int64_t *bfs_packed_host, int64_t *vlist_host, 
  int64_t *xoff, XADJ_SIZE *xadj, 
  int64_t *bfs_bit_map_host, int64_t *bfs_bit_map_cp);

void host_processing_openmp(int64_t nv, int64_t srcvtx,
  int64_t *k1, int64_t *k2, int64_t *level,
  int64_t *bfs_packed_host, int64_t *vlist_host,
  int64_t *xoff, XADJ_SIZE *xadj, int64_t *bfs_bit_map_host);


extern double walltime();

#include "../graph500.h"
#include "../xalloc.h"
#include "../generator/graph_generator.h"

#define MINVECT_SIZE 2

static int64_t maxvtx, nv, sz;
int64_t * restrict xoff; /* Length 2*nv+2 */
int64_t * restrict cny_xoff; /* Length 2*nv+2 */


XADJ_SIZE * restrict xadjstore; /* Length MINVECT_SIZE + (xoff[nv] == nedge) */
XADJ_SIZE * restrict xadj;
XADJ_SIZE * restrict cny_xadjstore; /* Length MINVECT_SIZE + (xoff[nv] == nedge) */
XADJ_SIZE * restrict cny_xadj;

static void
find_nv (const struct packed_edge * restrict IJ, const int64_t nedge)
{
  maxvtx = -1;
  OMP("omp parallel") {
    int64_t k, gmaxvtx, tmaxvtx = -1;

    OMP("omp for")
      for (k = 0; k < nedge; ++k) {
	if (get_v0_from_edge(&IJ[k]) > tmaxvtx)
	  tmaxvtx = get_v0_from_edge(&IJ[k]);
	if (get_v1_from_edge(&IJ[k]) > tmaxvtx)
	  tmaxvtx = get_v1_from_edge(&IJ[k]);
      }
    gmaxvtx = maxvtx;
    while (tmaxvtx > gmaxvtx)
      gmaxvtx = int64_casval (&maxvtx, gmaxvtx, tmaxvtx);
  }
  nv = 1+maxvtx;
}

static int
alloc_graph (int64_t nedge)
{
  sz = (2*nv+2) * sizeof (*xoff);
  xoff = xmalloc_large_ext (sz);
  if (!xoff) return -1;

  cny_xoff = pers_cp_malloc (sz);
  if (!cny_xoff) return -1;

  return 0;
}

static void
free_graph (void)
{

  xfree_large (xadjstore);
  xfree_large (xoff);

  pers_cp_free (cny_xadjstore);
  pers_cp_free (cny_xoff);

}

#define XOFF(k) (xoff[2*(k)])
#define XENDOFF(k) (xoff[1+2*(k)])

static int64_t
prefix_sum (int64_t *buf)
{
  int nt, tid;
  int64_t slice_begin, slice_end, t1, t2, k;

  nt = omp_get_num_threads ();
  tid = omp_get_thread_num ();

  t1 = nv / nt;
  t2 = nv % nt;
  slice_begin = t1 * tid + (tid < t2? tid : t2);
  slice_end = t1 * (tid+1) + ((tid+1) < t2? (tid+1) : t2);

  buf[tid] = 0;
  for (k = slice_begin; k < slice_end; ++k)
    buf[tid] += XOFF(k);
  OMP("omp barrier");
  OMP("omp single")
    for (k = 1; k < nt; ++k)
      buf[k] += buf[k-1];
  if (tid)
    t1 = buf[tid-1];
  else
    t1 = 0;
  for (k = slice_begin; k < slice_end; ++k) {
    int64_t tmp = XOFF(k);
    XOFF(k) = t1;
    t1 += tmp;
  }
  OMP("omp flush (xoff)");
  OMP("omp barrier");
  return buf[nt-1];
}

static int
setup_deg_off (const struct packed_edge * restrict IJ, int64_t nedge)
{
  int err = 0;
  int64_t *buf = NULL;
  xadj = NULL;
  OMP("omp parallel") {
    int64_t k, accum;
    OMP("omp for")
      for (k = 0; k < 2*nv+2; ++k)
	xoff[k] = 0;
    OMP("omp for")
      for (k = 0; k < nedge; ++k) {
        int64_t i = get_v0_from_edge(&IJ[k]);
        int64_t j = get_v1_from_edge(&IJ[k]);
	if (i != j) { /* Skip self-edges. */
	  if (i >= 0)
	    OMP("omp atomic")
	      ++XOFF(i);
	  if (j >= 0)
	    OMP("omp atomic")
	      ++XOFF(j);
	}
      }
    OMP("omp single") {
      buf = alloca (omp_get_num_threads () * sizeof (*buf));
      if (!buf) {
	perror ("alloca for prefix-sum hosed");
	abort ();
      }
    }
    OMP("omp for")
      for (k = 0; k < nv; ++k)
	if (XOFF(k) < MINVECT_SIZE) XOFF(k) = MINVECT_SIZE;

    accum = prefix_sum (buf);

    OMP("omp for")
      for (k = 0; k < nv; ++k)
	XENDOFF(k) = XOFF(k);

    OMP("omp single") {
      XOFF(nv) = accum;
      if (!(xadjstore = xmalloc_large_ext ((XOFF(nv) + MINVECT_SIZE) * sizeof (*xadjstore))))
	err = -1;
      if (!err) {
	xadj = &xadjstore[MINVECT_SIZE]; /* Cheat and permit xadj[-1] to work. */
  	for (k = 0; k < XOFF(nv) + MINVECT_SIZE; ++k)
	  xadjstore[k] = -1;
      }

      if (!(cny_xadjstore = pers_cp_malloc ((XOFF(nv) + MINVECT_SIZE) * sizeof (*cny_xadjstore))))
        err = -1;
      if (!err) {
        cny_xadj = &cny_xadjstore[MINVECT_SIZE]; /* Cheat and permit xadj[-1] to work. */
      } else {
        xfree_large (xadjstore);
      }

    }
  }

  return !xadj;
}

static void
scatter_edge (const int64_t i, const int64_t j)
{
  int64_t where;
  where = int64_fetch_add (&XENDOFF(i), 1);
  xadj[where] = j;
}

static int
i64cmp (const void *a, const void *b)
{
#if defined (XADJ_32)
  const int ia = *(const int*)a;
  const int ib = *(const int*)b;
#else
  const int64_t ia = *(const int64_t*)a;
  const int64_t ib = *(const int64_t*)b;
#endif
  if (ia < ib) return -1;
  if (ia > ib) return 1;
  return 0;
}

static void
pack_vtx_edges (const int64_t i)
{
  int64_t kcur, k;
  if (XOFF(i)+1 >= XENDOFF(i)) return;
  qsort (&xadj[XOFF(i)], XENDOFF(i)-XOFF(i), sizeof(*xadj), i64cmp);
  kcur = XOFF(i);
  for (k = XOFF(i)+1; k < XENDOFF(i); ++k)
    if (xadj[k] != xadj[kcur])
      xadj[++kcur] = xadj[k];
  ++kcur;
  for (k = kcur; k < XENDOFF(i); ++k)
    xadj[k] = -1;
  XENDOFF(i) = kcur;
}

static void
pack_edges (void)
{
  int64_t v;

  OMP("omp for")
    for (v = 0; v < nv; ++v)
      pack_vtx_edges (v);
}

static void
gather_edges (const struct packed_edge * restrict IJ, int64_t nedge)
{
  OMP("omp parallel") {
    int64_t k;

    OMP("omp for")
      for (k = 0; k < nedge; ++k) {
        int64_t i = get_v0_from_edge(&IJ[k]);
        int64_t j = get_v1_from_edge(&IJ[k]);
	if (i >= 0 && j >= 0 && i != j) {
	  scatter_edge (i, j);
	  scatter_edge (j, i);
	}
      }

    pack_edges ();
  }
}

int 
create_graph_from_edgelist (struct packed_edge *IJ, int64_t nedge)
{
  find_nv (IJ, nedge);

  if (alloc_graph (nedge)) return -1;
  if (setup_deg_off (IJ, nedge)) {
    xfree_large (xoff);
    pers_cp_free (cny_xoff);
    return -1;
  }

  gather_edges (IJ, nedge);

// copy migrate data to CP
  static int64_t sz, sz_xadjstore;

  sz = (2*nv+2);
  sz_xadjstore = XOFF(nv) + MINVECT_SIZE;

// copy data to CP
  pers_cp_memcpy(cny_xoff, xoff, (sz * sizeof (*xoff)));
  pers_cp_memcpy(cny_xadjstore, xadjstore, (sz_xadjstore * sizeof (*xadjstore)));

  return 0;
}


int
make_bfs_tree (int64_t *bfs_tree_out_host, int64_t *bfs_tree_out, 
               int64_t *max_vtx_out, int64_t srcvtx)
{

  int64_t k1, k2;
  int64_t level = 0;

  int64_t bit_map_size;
  int64_t g500_ctl;
  double bottom_constant;

  double t0;
  double copy_bfs_vlist_bit_time;
  double one_gb_pts = 1024.*1024.*1024.;

  int err = 0;

// must calculate bit_map_size before following packed mapping
/*
  bit_map_size = nv / 64;
  if ( nv != 64*bit_map_size)
    bit_map_size++;
*/
int64_t one   = 0x0000000000000001;
int64_t shift6 = 6;
int64_t and63_c = 0xffffffffffffff80;

#define BIT_LENGTH(n) ( (n) == ( (n) & and63_c) ? ((n) >> shift6 ) : ((n >> shift6 ) + one) )
  bit_map_size = BIT_LENGTH(nv);

  int64_t * restrict bfs_bit_map_host = bfs_tree_out_host;
  int64_t * restrict bfs_packed_host = &bfs_tree_out_host[bit_map_size];
  int64_t * restrict vlist_host = &bfs_tree_out_host[bit_map_size+1];

  int64_t * restrict bfs_tree = bfs_tree_out;
  int64_t * restrict bfs_bit_map_cp = &bfs_tree_out[nv];
  int64_t * restrict bfs_packed_cp = &bfs_tree_out[nv+bit_map_size];
  int64_t * restrict vlist = &bfs_tree_out[nv+bit_map_size+1];
  int64_t * restrict bfs_bit_map_new_cp = &bfs_tree_out[2*nv+bit_map_size+1];

  *max_vtx_out = maxvtx;

// touch the first and last points of bfs_bit_map_host
  bfs_bit_map_host[0] = -1;
  bfs_bit_map_host[bit_map_size-1] = -1;

// touch the first and last points of bfs_tree_cp
  bfs_tree[0] = -1;
  bfs_tree[nv-1] = -1;

/////////////////////////////////////////////////////////////////
// use the system type to set the threshold
  bottom_constant = bottom_threshold();
#if defined (TIMER_ON)
  printf ("bottom constant for top/down threshold %f \n",bottom_constant);
#endif // TIMER_ON

  top_hybrid (nv, srcvtx, bottom_constant, &k1, &k2, &level,
    bfs_tree, vlist,
    bfs_packed_host, vlist_host, xoff, xadj, 
    bfs_bit_map_host, bfs_bit_map_cp);

//////////////////////////////////////////////////////////////

// G500 control register: 
//   to distinguish between 32 and 64-bit edges
//
//   ctl[6-0]  = 1000000 for 64-bit edges
//   ctl[6-0]  = 0100000 for 32-bit edges

// 9-7-2011 removing single_ae logic since no longer needed

#if defined (XADJ_32)
// 4 ae, 32 bit xadj (edges)
  g500_ctl = 32;
  static int once;
  if (!once++) printf ("Using int32_t XADJ\n");
#else
// 4 ae, 64 bit xadj (edges)
  g500_ctl = 64;
#endif

// bits 12:8 are no longer used
  int64_t max_num_reqs_per_thread=1;
// OR in the MAX_NUM_REQS/thread to bits 12:8 of g500_ctl
  g500_ctl = (g500_ctl | (max_num_reqs_per_thread <<8));

// Number of threads per pipe to use.  HW default = max = 512 
  int64_t num_threads_per_pipe=512;
// OR in the num_threads_per_pipe to bits 24:16 of g500_ctl
  g500_ctl = (g500_ctl | (num_threads_per_pipe <<16));

  g500_hyb_wrapper ( g500_ctl, nv, bfs_tree, cny_xadj, cny_xoff, 
    bfs_packed_cp, bfs_bit_map_cp, bfs_bit_map_new_cp, level, &k1, &k2 );

  return err;
}

////////////////////////////////////////////////////////////////

void
destroy_graph (void)
{
  free_graph ();
}

#if defined(_OPENMP)
#if defined(__GNUC__)||defined(__INTEL_COMPILER)
// debug - remove later
//#if ((defined(__GNUC__)||defined(__INTEL_COMPILER)) && (!defined(__CONVEY)))
int64_t
int64_fetch_add (int64_t* p, int64_t incr)
{
  return __sync_fetch_and_add (p, incr);
}
int64_t
int64_casval(int64_t* p, int64_t oldval, int64_t newval)
{
  return __sync_val_compare_and_swap (p, oldval, newval);
}
int
int64_cas(int64_t* p, int64_t oldval, int64_t newval)
{
  return __sync_bool_compare_and_swap (p, oldval, newval);
}
#else
/* XXX: These are not correct, but suffice for the above uses. */
int64_t
int64_fetch_add (int64_t* p, int64_t incr)
{
  int64_t t;
  OMP("omp critical") {
    t = *p;
    *p += incr;
  }
  OMP("omp flush (p)");
  return t;
}
int64_t
int64_casval(int64_t* p, int64_t oldval, int64_t newval)
{
  int64_t v;
  OMP("omp critical (CAS)") {
    v = *p;
    if (v == oldval)
      *p = newval;
  }
  OMP("omp flush (p)");
  return v;
}
int
int64_cas(int64_t* p, int64_t oldval, int64_t newval)
{
  int out = 0;
  OMP("omp critical (CAS)") {
    int64_t v = *p;
    if (v == oldval) {
      *p = newval;
      out = 1;
    }
  }
  OMP("omp flush (p)");
  return out;
}
#endif
#else
int64_t
int64_fetch_add (int64_t* p, int64_t incr)
{
  int64_t t = *p;
  *p += incr;
  return t;
}
int64_t
int64_casval(int64_t* p, int64_t oldval, int64_t newval)
{
  int64_t v = *p;
  if (v == oldval)
    *p = newval;
  return v;
}
int
int64_cas(int64_t* p, int64_t oldval, int64_t newval)
{
  int64_t v = *p;
  int out = 0;
  if (v == oldval) {
    *p = newval;
    out = 1;
  }
  return out;
}
#endif
