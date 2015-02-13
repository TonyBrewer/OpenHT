#include <stdint.h>
#include <stdio.h>
#include <omp.h>
#define ONE 0x0000000000000001
#define ZERO 0
#define COPROC_XADJ_SIZE uint32_t
#define DEBUG 0
extern int __htc_get_unit_count();
#ifdef CNY_HTC_COPROC
// These functions will be compiled for the coprocessor
// ----------------------------------------------------------------------

uint8_t bufp(uint32_t vertex,uint64_t xoff[],uint32_t xadj[],uint64_t bfs_tree[],uint64_t bmapOldAddr[],uint8_t xadj_index_shift)
{
  uint8_t updated = 1;
  uint64_t xoff0 = xoff[2 * vertex];
  uint64_t xoff1 = xoff[2 * vertex + 1];
/*    if ( (xoff1 - vso) <= 0 ) {
        // clear done ones
        // BMAP_UPD: bit_working = bit_working & ( (one << bitCnt) ^ one64); 
        updated = 1;
        return updated;
    }
    */
//    updated = (xoff1 - vso) <= 0;
// each pipe does for loop
  for (xoff0 = xoff0; xoff0 < xoff1; ++xoff0) {
// XADJ_LD
    uint32_t neighbor = xadj[xoff0 << xadj_index_shift];
    uint32_t bmapIdx = neighbor >> 6;
    uint64_t oldAddr = bmapOldAddr[bmapIdx];
// Careful ordering of bmapBitIdx after reading bmapOldAddr allows
// bmapBitIdx to be a temp and not allocated as part of private
// state.
    uint8_t bmapBitIdx = (neighbor & 0x3f);
// is neighbor in frontier?
// check old bit map instead of level
    updated = 0;
    if ((oldAddr >> bmapBitIdx & 0x0000000000000001) == 0) {
// moved before next stmt to save a state
      updated = 1;
// have each pipe write this
      bfs_tree[vertex] = neighbor;
      break; 
    }
  }
  return updated;
}
#endif /* CNY_HTC_COPROC */
#define BFS_PACKED_X(k) (bfsPackedAddr[2*k])
#define VLIST_X(k) (bfsPackedAddr[2*k+1])
enum CommandType {INIT=0,SCATTER=1,BFS=2} ;
#ifdef CNY_HTC_COPROC

void __HTC_HOST_ENTRY_OUT__1__6291__(uint8_t function,uint64_t *bfsAddr,uint64_t *bfsPackedAddr,uint64_t *bmapOldAddr,uint64_t *bmapNewAddr,uint64_t *xoff,uint32_t *xadj,uint32_t lb,uint32_t ub,uint64_t *update_count)
{
  uint32_t nt = ub - lb + 1;
  uint32_t my_update_count = 0;
  if (nt > 512) {
    nt = 512;
  }
#pragma omp parallel  num_threads(nt)
{
    switch(function){
      case INIT:
{
{
{
            
#pragma omp for  nowait schedule(static , 1)
            for (uint32_t k = lb; k < ub; k++) {
              bfsAddr[k] = 0xffffffffffffffffULL;
            }
          }
        }
        break; 
      }
      case SCATTER:
{
{
{
            
#pragma omp for  nowait schedule(static , 1)
            for (uint32_t k = lb; k < ub; k++) {
              bfsAddr[bfsPackedAddr[2 * k + 1]] = bfsPackedAddr[2 * k];
            }
          }
        }
        break; 
      }
      case BFS:
{
{
          
#pragma omp for  nowait schedule(static , 1)
          for (uint32_t index = lb; index < ub; index++) {
            uint64_t mask;
            uint8_t bitCnt = 0;
            uint8_t bmapUpdCnt = 0;
            mask = bmapOldAddr[index];
            if (mask != 0) {
              for (bitCnt = bitCnt; bitCnt < 64; bitCnt++) {
                if ((mask >> bitCnt & 0x0000000000000001) == 0x0000000000000001) {
// call bufp
                  uint32_t vertex = ((uint32_t )(index * 64)) + bitCnt;
/* xadj_index_shift */
                  if ((bufp(vertex,xoff,xadj,bfsAddr,bmapOldAddr,((uint8_t )(((uint64_t )bfsPackedAddr) & 0x0000000000000001)))) & 0x0000000000000001) {
                    mask = (mask & ~(1ULL << bitCnt));
                    bmapUpdCnt++;
                  }
                }
                else {
                  uint16_t tmask = (uint16_t )(mask >> bitCnt + 1);
                  uint16_t skip = 0;
                  if ((tmask & 0xff) == 0) {
                    skip = 8;
                    tmask >>= 8;
                  }
                  if ((tmask & 0x0f) == 0) {
                    skip += 4;
                    tmask >>= 4;
                  }
                  if ((tmask & 0x03) == 0) {
                    skip += 2;
                    tmask >>= 2;
                  }
                  skip += 1 - (tmask & 0x0000000000000001);
                  bitCnt += skip;
                }
//
//                            if ((bitCnt > 63) || (mask <= (1ULL < bitCnt))) {
//                                  bitCnt = 63; // no more higher
//                            }
// break state with comment
              }
            }
            if (bmapUpdCnt) {
              my_update_count += bmapUpdCnt;
            }
            bmapNewAddr[index] = mask;
          }
           *update_count = ((uint64_t )my_update_count);
        }
        break; 
      }
    }
/* end of parallel */
  }
  if (function == BFS) {
     *update_count = ((uint64_t )my_update_count);
  }
/* pragma omp target */
}
#endif /* CNY_HTC_COPROC */
#ifdef CNY_HTC_HOST

void bottom_up_ctl(uint8_t function,
/* bfs_tree */
uint64_t *bfsAddr,
/* bfs_packed */
uint64_t *bfsPackedAddr,
/* bfs_tree_bit */
uint64_t *bmapOldAddr,
/* bfs_tree_bit_new */
uint64_t *bmapNewAddr,uint64_t *xoff,uint32_t *xadj,
/* CTL parameters */
uint32_t bmapIdx,uint32_t lb,uint32_t ub,uint64_t *update_count)
{
  int __htc_unit_index = bmapIdx;
  __htc_units[bmapIdx] ->  SendCall___HTC_HOST_ENTRY_OUT__1__6291__ (0,0,1,function,bfsAddr,bfsPackedAddr,bmapOldAddr,bmapNewAddr,xoff,xadj,lb,ub,update_count);
if (__htc_unit_index == __htc_get_unit_count()-1) {
   for (; __htc_unit_index >= 0; __htc_unit_index--) {
  while(!__htc_units[__htc_unit_index] ->  RecvReturn___HTC_HOST_ENTRY_OUT__1__6291__ ()){
    usleep(0);
  }
    }
}
}
#endif /* CNY_HTC_HOST */
#ifdef CNY_HTC_HOST
extern "C"

void pers_init_bfs_tree(int64_t nv,uint64_t *bfs_tree)
{
  int unitCnt = __htc_get_unit_count();
#if DEBUG
#endif
  uint64_t S_bfsSize = (uint64_t )nv;
  uint64_t chunk = ((int )(S_bfsSize / unitCnt + 1));
  int unit;
//#pragma omp parallel num_threads(unitCnt)
//#pragma omp for nowait schedule(static, 1)
  for (unit = 0; unit < unitCnt; unit++) {
    uint64_t lb = (uint64_t )(unit * chunk);
    uint64_t ub = lb + chunk;
    if (ub > S_bfsSize) {
      ub = S_bfsSize;
    }
#if DEBUG
#endif
    bottom_up_ctl(INIT,bfs_tree,0,0,0,0,0,unit,lb,ub,0);
/* bfsAddr */
/* bfs_packed */
/* bfs_tree_bit */
/* bfs_tree_bit_new */
/* xoff */
/* xadj */
/* bmapIdx */
  }
#if DEBUG
#endif
}
#endif /* CNY_HTC_HOST */
#ifdef CNY_HTC_HOST
extern "C"

void pers_scatter_bfs(uint64_t *k2,uint64_t *bfs_tree,uint64_t *bfs_packed)
{
  int unitCnt = __htc_get_unit_count();
#if DEBUG
#endif
// BFS_SIZE used for K2 on scatter instruction
  uint64_t S_bfsSize =  *k2;
#if DEBUG
#endif
  uint64_t chunk = ((int )(S_bfsSize / unitCnt + 1));
  int unit;
//#pragma omp parallel num_threads(unitCnt)
//#pragma omp for nowait schedule(static , 1) private(unit)
  for (unit = 0; unit < unitCnt; unit++) {
    uint64_t lb = (uint64_t )(unit * chunk);
    uint64_t ub = lb + chunk;
    if (ub > S_bfsSize) {
      ub = S_bfsSize;
    }
#if DEBUG
#endif
    bottom_up_ctl(SCATTER,bfs_tree,bfs_packed,0,0,0,0,unit,lb,ub,0);
/* bfsAddr */
/* bfs_packed */
/* bfs_tree_bit */
/* bfs_tree_bit_new */
/* xoff */
/* xadj */
/* bmapIdx */
/* omp parallel */
  }
#if DEBUG
#endif
}
#endif /* CNY_HTC_HOST */
#ifdef CNY_HTC_HOST
extern "C"

void pers_bottom_up(int64_t g500_ctl,int64_t nv,uint64_t *bfs_tree,uint64_t *bfs_packed_cp,uint32_t *xadj,uint64_t *xoff,uint64_t **bfs_tree_bit,uint64_t **bfs_tree_bit_new,uint64_t *k1,uint64_t *k2,uint64_t *oldk2)
{
#if DEBUG
#endif
  int unitCnt = __htc_get_unit_count();
  uint64_t bfsSize = (uint64_t )nv;
/* bfsSize / 64 */
  uint64_t ub1 = bfsSize + 63 >> 6;
  uint64_t chunk = ((int )(ub1 / unitCnt + 1));
  while( *k1 !=  *k2){
     *oldk2 =  *k2;
#if DEBUG
#endif
    uint64_t updCnt = 0;
    int unit;
    uint64_t update_count[64];
//#pragma omp parallel num_threads(unitCnt) reduction(+:updCnt)
//#pragma omp for nowait schedule(static , 1) private(unit)
    for (unit = 0; unit < unitCnt; unit++) {
      uint64_t lb = (uint64_t )(unit * chunk);
      uint64_t ub = lb + chunk;
      if (ub > ub1) {
        ub = ub1;
      }
      update_count[unit] = 0;
      bottom_up_ctl(BFS,bfs_tree,((uint64_t *)((g500_ctl & 0xFFFFFF) == 64)), *bfs_tree_bit, *bfs_tree_bit_new,xoff,xadj,unit,lb,ub,&update_count[unit]);
/* bfsAddr */
/* xadj_index_shift passed in bfs_packed slot */
/* bfs_tree_bit */
/* bfs_tree_bit_new */
/* xoff */
/* xadj */
/* bmapIdx */
/* lb */
/* ub */
    }
    for (unit = 0; unit < unitCnt; unit++) {
      updCnt += update_count[unit];
    }
#if DEBUG
#endif
     *k2 += updCnt;
     *k1 =  *oldk2;
// flip addresses for next iteration
    uint64_t *temp;
    temp =  *bfs_tree_bit;
     *bfs_tree_bit =  *bfs_tree_bit_new;
     *bfs_tree_bit_new = temp;
/* while (*k1 != *k2) */
  }
}
#endif /* CNY_HTC_HOST */
