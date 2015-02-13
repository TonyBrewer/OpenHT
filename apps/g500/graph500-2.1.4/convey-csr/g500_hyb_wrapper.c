/* Modified January 2011 for Convey computers */
/*
  Routine to perform part of Graph500 make_bfs_tree that will run on
  coprocessor. This routine and its callees will become the pdk instruction
*/

#include <stdio.h>
#include <stdlib.h>

// for openmp definition
#include "../compat.h"
#include "../graph500.h"
#include "../timer.h"

#include "convey-csr.h"

#undef CNY_DEBUG

#define MIN(a,b) (((a) < (b)) ? (a) : (b) )

void init_bfs_tree (int64_t nv, int64_t *bfs_tree);

void scatter_bfs (int64_t *k2, int64_t *bfs_tree, int64_t *bfs_packed_cp);
void scatter_check (int64_t *k2, int64_t *bfs_tree, int64_t *bfs_packed);

void top_down (int64_t nv, int64_t *k1, int64_t *k2, 
  int64_t *oldk2, int64_t *bfs_tree, int64_t *vlist, 
  int64_t *xoff, XADJ_SIZE *xadj);

void flip_addr (int64_t **a, int64_t **b);

void bottom_up ( int64_t g500_ctl, int64_t nv,
  int64_t *bfs_tree, int64_t *bfs_packed_cp, 
  XADJ_SIZE *xadj, int64_t *xoff,
  int64_t **bfs_tree_bit, int64_t **bfs_tree_bit_new, 
  int64_t *k1, int64_t *k2, int64_t *oldk2);

int64_t zero = 0;
int64_t one   = 0x0000000000000001;
int64_t one64 = 0xffffffffffffffff;   // -1
int64_t shift6 = 6;
int64_t and63 = 0x000000000000003f;
int64_t and63_c = 0xffffffffffffff80;

#define BIT_LENGTH(n) ( (n) == ( (n) & and63_c) ? ((n) >> shift6 ) : ((n >> shift6 ) + one) )

void
g500_hyb_wrapper ( int64_t g500_ctl, int64_t nv,
               int64_t *bfs_tree,
	       XADJ_SIZE *xadj, int64_t *xoff,
               int64_t *bfs_packed_cp,
               int64_t *bfs_bit_map_cp, int64_t *bfs_bit_map_new_cp,
               int64_t level, int64_t *k1, int64_t *k2 )
{
  int use_coproc = 1;

// Do bottom up algorithm on coprocessor

#if defined (TIMER_ON)
  static double coproc_scatter_time;
  static double postproc_time;
  static double bottom_time;
  extern double walltime();
  double t0;
  int64_t size_bytes = sizeof(int64_t) * (*k2);
  double one_gb_pts = 1024.*1024.*1024.;
#endif // TIMER_ON

  int64_t oldk2;

  int64_t bit_map_size;
  bit_map_size = BIT_LENGTH(nv);

//////////////////////////////////////////////////////////////
// Call 1 - CP instruction caep03 
// build bfs_tree on coproc with scatter

  if ( use_coproc != 0 )
    {

#if defined (SCATTER_AND_BOTTOM_UP)
// do nothing

#else

#if defined (TIMER_ON)
      t0 = walltime();
#endif // TIMER_ON

      pers_scatter_bfs (k2, bfs_tree, bfs_packed_cp);

      //scatter_check (k2, bfs_tree, bfs_packed_cp);

#if defined (TIMER_ON)
      coproc_scatter_time = walltime() - t0;
      printf ("coproc scatter size %ld rate (GB/s) %f time %f\n",*k2,3*(size_bytes/(one_gb_pts*coproc_scatter_time)),coproc_scatter_time);
#endif // TIMER_ON

#endif // SCATTER_AND_BOTTOM_UP
    }
  else
    {
      scatter_bfs (k2, bfs_tree, bfs_packed_cp);
    }

//////////////////////////////////////////////////////////////
// Call 2 - CP instruction caep00
//  do loop around bottom up algorithm
      while (*k1 != *k2) {

#if defined (CNY_DEBUG)
        fprintf(stderr, "DBG:  level = %d\n",level);
        for (int b=0; b<bit_map_size; b++) {
          fprintf(stderr, "DBG:  bfs_bit_map_cp[%d]=0x%016llx\n",b,bfs_bit_map_cp[b]);
        }
#endif // CNY_DEBUG

#if defined (TIMER_ON)
        t0 = walltime();
#endif // TIMER_ON

#if defined (CNY_DEBUG)
  fprintf(stderr, "Calling bottom up:  *k1=%ld *k2=%ld\n", *k1, *k2);
#endif // CNY_DEBUG
  int check=0;

  int64_t *ck_bfs_tree;
  int64_t *ck_bit_map, *ck_bit_map_new;
  int64_t *ck_k1, *ck_k2;
  int64_t ck_oldk2;
  int64_t ck_k1_v, ck_k2_v;
  ck_k1 = &ck_k1_v;
  ck_k2 = &ck_k2_v;
  if (check) {
    ck_bfs_tree = pers_cp_malloc (nv * sizeof (*ck_bfs_tree));
    ck_bit_map = pers_cp_malloc (nv/64 * sizeof (*ck_bit_map));
    ck_bit_map_new = pers_cp_malloc (nv/64 * sizeof (*ck_bit_map_new));
    *ck_k1 = *k1;
    *ck_k2 = *k2;
    for (int i=0; i<nv; i++) {
      ck_bfs_tree[i] = bfs_tree[i];
    }
    for (int i=0; i<nv/64+1; i++) {
      ck_bit_map[i] = bfs_bit_map_cp[i];
      ck_bit_map_new[i] = bfs_bit_map_new_cp[i];
    }
    bottom_up ( g500_ctl, nv,
          ck_bfs_tree, bfs_packed_cp, 
          xadj, xoff,
          &ck_bit_map, &ck_bit_map_new, 
          ck_k1, ck_k2, &ck_oldk2);
  }


#if 0
    for (int i=0; i<nv; i++) {
        fprintf(stderr,"bfs_tree[%d] is %ld\n", i, bfs_tree[i]);
    }
#endif
  if ( use_coproc != 0 ) {

        pers_bottom_up ( g500_ctl, nv,

          bfs_tree, bfs_packed_cp, 
          xadj, xoff,
          &bfs_bit_map_cp, &bfs_bit_map_new_cp, 
          k1, k2, &oldk2);
#if defined (CNY_DEBUG)
        fprintf(stderr, "AFTER coproc call to ht_bottom_up\n");
#endif
  } else { // not on coprocessor
        bottom_up ( g500_ctl, nv,
          bfs_tree, bfs_packed_cp, 
          xadj, xoff,
          &bfs_bit_map_cp, &bfs_bit_map_new_cp, 
          k1, k2, &oldk2);
  }

  if (check) {
    if (*ck_k1 != *k1) {
      fprintf(stderr, "ERROR:  ck_k1 (%lld) != k1 (%lld)\n", *ck_k1, *k1);
    }
    if (*ck_k2 != *k2) {
      fprintf(stderr, "ERROR:  ck_k2 (%lld) != k2 (%lld)\n", *ck_k2, *k2);
    }
    if (ck_oldk2 != oldk2) {
      fprintf(stderr, "ERROR:  ck_oldk2 (%lld) != oldk2 (%lld)\n", ck_oldk2, oldk2);
    }
    for (int i=0; i<nv; i++) {
      if (ck_bfs_tree[i] != bfs_tree[i])
        fprintf(stderr, "ERROR:  ck_bfs_tree[%d] (0x%llx) != bfs_tree[%d] (0x%llx)\n", i, ck_bfs_tree[i], i, bfs_tree[i]);
    }
    for (int i=0; i<nv/64+1; i++) {
      if (ck_bit_map[i] != bfs_bit_map_cp[i]) {
        fprintf(stderr, "ERROR:  ck_bit_map[i] (0x%llx) != bfs_bit_map_cp[i] (0x%llx)\n", i, ck_bit_map[i], i, bfs_bit_map_cp[i]);
      }
      if (ck_bit_map_new[i] != bfs_bit_map_new_cp[i]) {
        fprintf(stderr, "ERROR:  ck_bit_map_new[i] (0x%llx) != bfs_bit_map_new_cp[i] (0x%llx)\n", i, ck_bit_map_new[i], i, bfs_bit_map_new_cp[i]);
      }
    }
  }

#if defined (CNY_DEBUG)
  fprintf(stderr, "After bottom up:  *k1=%ld *k2=%ld\n", *k1, *k2);
#endif // CNY_DEBUG

#if defined (CNY_DEBUG)
for (int i=0;i<nv;i++)
  fprintf(stderr, "xbfs_tree[%d]=%llx (0x%llx)\n", i, bfs_tree[i], &bfs_tree[i]);

  for (int ixx=0;ixx<nv/64+1;ixx++){
    fprintf (stderr, "ixx %d bit maps %lx %lx \n",ixx,bfs_bit_map_cp[ixx],bfs_bit_map_new_cp[ixx]);
  }
#endif // CNY_DEBUG

#if defined (TIMER_ON)
        bottom_time = walltime() - t0;
        printf ("bottom_up: *k2 %ld level %ld time %f\n", *k2, level, bottom_time);
        level++;
#endif // TIMER_ON
      } // end while - while is sometimes in inlined code

//////////////////////////////////////////////////////////////

  return;
}

void
init_bfs_tree (int64_t nv, int64_t *bfs_tree)
{
  int64_t k1;
  for (k1 = 0; k1 < nv; ++k1)
    bfs_tree[k1] = one64;

  return;
}

void
top_down (int64_t nv, int64_t *k1, int64_t *k2, int64_t *oldk2, int64_t *bfs_tree, int64_t *vlist, int64_t *xoff, XADJ_SIZE *xadj)
{
  int64_t k;
  for (k = *k1; k < *oldk2; ++k) {
    const int64_t v = vlist[k];
    const int64_t vso = xoff[2*v];
    const int64_t veo = xoff[2*v+1];
    int64_t vo;
    for (vo = vso; vo < veo; ++vo) {
      const int64_t j = xadj[vo];
      if (bfs_tree[j] == one64) {
        bfs_tree[j] = v;
        vlist[(*k2)++] = j;
      }
    }
  }
  *k1 = *oldk2;
  return;
}

void
bottom_up ( int64_t g500_ctl, int64_t nv,
   int64_t *bfs_tree, int64_t *bfs_packed_cp, 
   XADJ_SIZE *xadj, int64_t *xoff,
   int64_t **bfs_tree_bit, int64_t **bfs_tree_bit_new,
   int64_t *k1, int64_t *k2, int64_t *oldk2)
{
  int64_t i, i_out;
  int64_t bit_scalar, bit_working, bit_extract;
  int64_t i_out_mod64, i_out_div64;
  int64_t *temp;

// split nv over the number of function pipes in chunks of 64
//
  *oldk2 = *k2;

  fprintf(stderr, "bottom_up:  k1=%lld k2=%lld oldk2=%lld\n", (long long)*k1, (long long)*k2, (long long)*oldk2);


  int64_t i_out_div64_neighbor, bit_scalar_neighbor;
  int64_t i_out_mod64_neighbor, bit_extract_neighbor;

  for (i_out = 0; i_out < nv; i_out+=64) {
    i_out_div64 = i_out / 64;

    bit_scalar = (*bfs_tree_bit)[i_out_div64];
    bit_working = bit_scalar;

      for (i = i_out; i < MIN(i_out+64,nv); i++) {

//  do 64 at a time per pipe,
//    to reduce bfs_tree loads by factor of 64
//    since all final steps are bottom_up can delete vlist store, but still need k2 incr
//
        i_out_mod64 = i - i_out;
        bit_extract = ((bit_scalar >> i_out_mod64) & one);
        if (bit_extract == one) {

          const int64_t vso = xoff[2*i];
          const int64_t veo = xoff[2*i+1];
          int64_t vo;

// if a vertex has no connection, mark it as updated in bfs_tree_bit
          if ( (veo - vso) <= 0 )
            {
                bit_working = bit_working & ( (one << i_out_mod64) ^ one64); // clear done ones
                (*k2)++;
                goto nexti;
            }

// each pipe does for loop
          for (vo = vso; vo < veo; ++vo) {
            const int64_t neighbor = xadj[vo];

// is neighbor in frontier?
// check old bit map instead of level
            i_out_div64_neighbor = neighbor / 64;
            i_out_mod64_neighbor = neighbor % 64;
            bit_scalar_neighbor = (*bfs_tree_bit)[i_out_div64_neighbor];
//fprintf (stderr, "div64_neighbor=%d mod64_neighbor=%d bit_scalar_neighbor=%d\n", i_out_div64_neighbor, i_out_mod64_neighbor, bit_scalar_neighbor);
            bit_extract_neighbor = ((bit_scalar_neighbor >> i_out_mod64_neighbor) & one);
//fprintf(stderr, "bottom_up:  i=%d vo=%d neighbor=%lld bmap[%d] bit=%d bit_working=%llx\n", i, vo, neighbor, i_out_div64_neighbor, i_out_mod64_neighbor, bit_working);
            if (bit_extract_neighbor == zero)
              {
                bfs_tree[i] = neighbor; // have each pipe write this
                bit_working = bit_working & ( (one << i_out_mod64) ^ one64); // clear done ones

//              vlist[(*k2)++] = i;     // pass i to AE0 or pass k2 between AE?
                (*k2)++;
#if defined (CNY_DEBUG)
  fprintf(stderr, "bottom_up:  Updated bmapIdx[%d] bit %d, neighbor is bmap[%d] bit %d, *k2=%lld\n", i_out_div64, i_out_mod64, i_out_div64_neighbor, i_out_mod64_neighbor, *k2);
  fprintf(stderr, "bottom_up:  bfs_tree[%d]=%lld \n", i, bfs_tree[i]);
#endif // CNY_DEBUG
                goto nexti;
              }
          }
        }
       else
// already updated
        {
          bit_working = bit_working & ( (one << i_out_mod64) ^ one64); // clear done ones
        }
nexti:
        continue;
      }
    (*bfs_tree_bit_new)[i_out_div64] = bit_working;
  }

  *k1 = *oldk2;

  fprintf(stderr, "bottom_up return:  k1=%lld k2=%lld oldk2=%lld\n", (long long)*k1, (long long)*k2, (long long)*oldk2);

////////////////////////////////////////////////////////////////
// flip addresses - see separate function
////////////////////////////////////////////////////////////////
  temp = *bfs_tree_bit;
  *bfs_tree_bit = *bfs_tree_bit_new;
  *bfs_tree_bit_new = temp;

  return;
}

void
scatter_bfs (int64_t *k2, int64_t *bfs_tree, int64_t *bfs_packed)
{
  int64_t k;

#define BFS_PACKED_X(k) (bfs_packed[2*k])
#define VLIST_X(k) (bfs_packed[2*k+1])

  for (k=0; k<(*k2); k++) {
//    bfs_tree[vlist[k]] = bfs_packed[k];
    bfs_tree[VLIST_X(k)] = BFS_PACKED_X(k);
  }
  return;
}

void
scatter_check (int64_t *k2, int64_t *bfs_tree, int64_t *bfs_packed)
{
  int64_t k;

#define BFS_PACKED_X(k) (bfs_packed[2*k])
#define VLIST_X(k) (bfs_packed[2*k+1])

  for (k=0; k<(*k2); k++) {
    if (bfs_tree[VLIST_X(k)] != BFS_PACKED_X(k))
      fprintf(stderr, "ERROR:  bfs_tree[%d] (0x%llx) = 0x%llx , expected 0x%llx\n", 
                               VLIST_X(k), &bfs_tree[VLIST_X(k)], bfs_tree[VLIST_X(k)], BFS_PACKED_X(k));
  }
  return;
}
