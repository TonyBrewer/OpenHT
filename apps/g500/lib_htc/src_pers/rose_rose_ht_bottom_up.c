#include <stdint.h>
#include <stdio.h>
#include <omp.h>
#define ONE 0x0000000000000001
#define ZERO 0
#define COPROC_XADJ_SIZE uint32_t
#define DEBUG 0
/* 
extern int __htc_get_unit_count(); */
#ifdef CNY_HTC_COPROC
// These functions will be compiled for the coprocessor
// ----------------------------------------------------------------------
#if 0

uint8_t bufp(htc_tid_t P___htc_parent_htid,htc_tid_t P___htc_my_omp_thread_num,htc_tid_t P___htc_my_omp_team_size,uint32_t P_vertex,uint64_t P_xoff[],uint32_t P_xadj[],uint64_t P_bfs_tree[],uint64_t P_bmapOldAddr[],uint8_t P_xadj_index_shift)
#endif
#ifdef HTC_KEEP_MODULE_BUFP
#include "Ht.h"
#include "PersBufp.h"
void CPersBufp::PersBufp()
{
  if (PR_htValid) {
    switch(PR_htInst){
      case BUFP__START:
{
        if (ReadMemBusy()) {
          HtRetry();
          break; 
        }
        P_mif_rdVal_index = PR_htId;
        P_updated = ((uint8_t )((uint8_t )1));
/* xoff0 =((uint64_t )xoff[((unsigned int )2) * vertex]); */
        ReadMem_f0(((MemAddr_t )(((MemAddr_t )P_xoff) + (0 + ((unsigned int )2) * P_vertex << 3U))),PR_htId);
        ReadMemPause(BUFP__READMEM__32);
        break; 
      }
      case BUFP__READMEM__32:
{
        if (ReadMemBusy()) {
          HtRetry();
          break; 
        }
        P_xoff0 = ((uint64_t )(GR_mifRdVal_bufp_fld0()));
/* xoff1 =((uint64_t )xoff[((unsigned int )2) * vertex +((unsigned int )1)]); */
        ReadMem_f0(((MemAddr_t )(((MemAddr_t )P_xoff) + (0 + (((unsigned int )2) * P_vertex + ((unsigned int )1)) << 3U))),PR_htId);
        ReadMemPause(BUFP__READMEM__33);
        break; 
      }
      case BUFP__READMEM__33:
{
        P_xoff1 = ((uint64_t )(GR_mifRdVal_bufp_fld0()));
/*    if ( (xoff1 - vso) <= 0 ) {
        // clear done ones
        // BMAP_UPD: bit_working = bit_working & ( (one << bitCnt) ^ one64); 
        updated = 1;
        return updated;
    }
    */
;
        P_xoff0 = ((uint64_t )P_xoff0);
        HtContinue(BUFP__LOOP_TOP__10);
        break; 
      }
      case BUFP__LOOP_TOP__10:
{
        if (ReadMemBusy()) {
          HtRetry();
          break; 
        }
        if (!(P_xoff0 < P_xoff1)) {
          HtContinue(BUFP__LOOP_EXIT__10);
          break; 
        }
// XADJ_LD
/* neighbor =((uint32_t )xadj[xoff0 <<((int )xadj_index_shift)]); */
        ReadMem_f1(((MemAddr_t )(((MemAddr_t )P_xadj) + (0 + (P_xoff0 << ((int )P_xadj_index_shift)) << 2U))),PR_htId);
        ReadMemPause(BUFP__READMEM__34);
        break; 
      }
      case BUFP__READMEM__34:
{
        uint32_t P_bmapIdx;
        if (ReadMemBusy()) {
          HtRetry();
          break; 
        }
        P_neighbor = ((uint32_t )(GR_mifRdVal_bufp_fld1()));
        P_bmapIdx = ((uint32_t )(P_neighbor >> 6));
/* oldAddr =((uint64_t )bmapOldAddr[bmapIdx]); */
        ReadMem_f0(((MemAddr_t )(((MemAddr_t )P_bmapOldAddr) + (0 + P_bmapIdx << 3U))),PR_htId);
        ReadMemPause(BUFP__READMEM__35);
        break; 
      }
      case BUFP__READMEM__35:
{
        uint8_t P_bmapBitIdx;
        uint64_t P_oldAddr;
        if (WriteMemBusy()) {
          HtRetry();
          break; 
        }
        P_oldAddr = ((uint64_t )(GR_mifRdVal_bufp_fld0()));
// Careful ordering of bmapBitIdx after reading bmapOldAddr allows
// bmapBitIdx to be a temp and not allocated as part of private
// state.
        P_bmapBitIdx = ((uint8_t )((uint8_t )(P_neighbor & ((unsigned int )0x3f))));
// is neighbor in frontier?
// check old bit map instead of level
        P_updated = ((uint8_t )0);
        if ((P_oldAddr >> P_bmapBitIdx & 0x0000000000000001) == 0) {
// moved before next stmt to save a state
          P_updated = ((uint8_t )1);
          WriteMem_uint64(((MemAddr_t )(((MemAddr_t )P_bfs_tree) + (0 + P_vertex << 3U))),((uint64_t )P_neighbor));
          WriteMemPause(BUFP__LOOP_EXIT__10);
          break; 
        }
        P_xoff0 = P_xoff0 + 1;
        HtContinue(BUFP__LOOP_TOP__10);
        break; 
      }
      case BUFP__LOOP_EXIT__10:
{
        if (SendReturnBusy_bufp()) {
          HtRetry();
          break; 
        }
        SendReturn_bufp(P_updated);
        break; 
      }
      default:
{
        HtAssert(0,0);
        break; 
      }
    }
  }
}
#endif /* HTC_KEEP_MODULE_BUFP */
#endif /* CNY_HTC_COPROC */
#define BFS_PACKED_X(k) (bfsPackedAddr[2*k])
#define VLIST_X(k) (bfsPackedAddr[2*k+1])
enum CommandType {INIT,SCATTER,BFS} ;
#ifdef CNY_HTC_COPROC
/* 
static void OUT__1__6827__(htc_tid_t __htc_parent_htid,htc_tid_t __htc_my_omp_thread_num,htc_tid_t __htc_my_omp_team_size); */
#if 0

void __HTC_HOST_ENTRY_OUT__1__6291__(htc_tid_t P___htc_parent_htid,htc_tid_t P___htc_my_omp_thread_num,htc_tid_t P___htc_my_omp_team_size,uint8_t P_function,uint64_t *P_bfsAddr,uint64_t *P_bfsPackedAddr,uint64_t *P_bmapOldAddr,uint64_t *P_bmapNewAddr,uint64_t *P_xoff,uint32_t *P_xadj,uint32_t P_lb,uint32_t P_ub,uint64_t *P_update_count)
#endif
#ifdef HTC_KEEP_MODULE___HTC_HOST_ENTRY_OUT__1__6291__
#include "Ht.h"
#include "Pers__HTC_HOST_ENTRY_OUT__1__6291__.h"
void CPers__HTC_HOST_ENTRY_OUT__1__6291__::Pers__HTC_HOST_ENTRY_OUT__1__6291__()
{
  if (PR_htValid) {
    switch(PR_htInst){
      case __HTC_HOST_ENTRY_OUT__1__6291____START:
{
        P___htc_uplevel_index_0 = ((__HTC_HOST_ENTRY_OUT__1__6291___uplevel_index_t )PR_htId);
        GW___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___update_count_update_count(PR_htId,P_update_count);
        GW___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___ub_ub(PR_htId,P_ub);
        GW___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___lb_lb(PR_htId,P_lb);
        GW___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___xadj_xadj(PR_htId,P_xadj);
        GW___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___xoff_xoff(PR_htId,P_xoff);
        GW___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___bmapNewAddr_bmapNewAddr(PR_htId,P_bmapNewAddr);
        GW___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___bmapOldAddr_bmapOldAddr(PR_htId,P_bmapOldAddr);
        GW___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___bfsPackedAddr_bfsPackedAddr(PR_htId,P_bfsPackedAddr);
        GW___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___bfsAddr_bfsAddr(PR_htId,P_bfsAddr);
        GW___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___function_function(PR_htId,P_function);
        P_nt = ((uint32_t )(P_ub - P_lb + ((unsigned int )1)));
        GW___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___my_update_count_my_update_count(P___htc_uplevel_index_0,((uint32_t )((uint32_t )0)));
        HtContinue(__HTC_HOST_ENTRY_OUT__1__6291____GW__36);
        break; 
      }
      case __HTC_HOST_ENTRY_OUT__1__6291____GW__36:
{
        if (P_nt > 512) {
          P_nt = ((uint32_t )512);
        }
/* XOMP_parallel_start(OUT__1__6827__,0,1,nt,"/nethome/rmeyer/systemc/tests/g500/lib_htc/src_htc/rose_ht_bottom_up.c",62); */
;
        P___htc_t_nt5 = ((int16_t )P_nt);
        P___htc_t_piv4 = ((int16_t )0);
        HtContinue(__HTC_HOST_ENTRY_OUT__1__6291____FORK_LOOP_TOP__6);
        break; 
      }
      case __HTC_HOST_ENTRY_OUT__1__6291____FORK_LOOP_TOP__6:
{
        if (SendCallBusy_OUT__1__6827__()) {
          HtRetry();
          break; 
        }
        if (!(P___htc_t_piv4 < P___htc_t_nt5)) {
          RecvReturnPause_OUT__1__6827__(__HTC_HOST_ENTRY_OUT__1__6291____FORK_LOOP_EXIT__6);
          break; 
        }
        SendCallFork_OUT__1__6827__(__HTC_HOST_ENTRY_OUT__1__6291____FORK_LOOP_JOIN_CYCLE__6,((htc_tid_t )PR_htId),((htc_tid_t )P___htc_t_piv4),((htc_tid_t )P___htc_t_nt5));
        P___htc_t_piv4 = P___htc_t_piv4 + 1;
        HtContinue(__HTC_HOST_ENTRY_OUT__1__6291____FORK_LOOP_TOP__6);
        break; 
      }
      case __HTC_HOST_ENTRY_OUT__1__6291____FORK_LOOP_JOIN_CYCLE__6:
{
        RecvReturnJoin_OUT__1__6827__();
        break; 
      }
      case __HTC_HOST_ENTRY_OUT__1__6291____FORK_LOOP_EXIT__6:
{
        MemAddr_t P___htc_t_arrptr23;
        if (WriteMemBusy()) {
          HtRetry();
          break; 
        }
/* XOMP_parallel_end("/nethome/rmeyer/systemc/tests/g500/lib_htc/src_htc/rose_ht_bottom_up.c",148); */
;
        if (!(P_function == BFS)) {
          HtContinue(__HTC_HOST_ENTRY_OUT__1__6291____WRITEMEM__37__IF_JOIN__44);
          break; 
        }
        P___htc_t_arrptr23 = ((MemAddr_t )P_update_count);
        WriteMem_uint64(((MemAddr_t )((MemAddr_t )P___htc_t_arrptr23)),((uint64_t )((uint64_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___my_update_count_my_update_count()))));
        WriteMemPause(__HTC_HOST_ENTRY_OUT__1__6291____WRITEMEM__37__IF_JOIN__44);
        break; 
      }
      case __HTC_HOST_ENTRY_OUT__1__6291____WRITEMEM__37__IF_JOIN__44:
{
        if (SendReturnBusy___HTC_HOST_ENTRY_OUT__1__6291__()) {
          HtRetry();
          break; 
        }
        SendReturn___HTC_HOST_ENTRY_OUT__1__6291__();
        break; 
      }
      default:
{
        HtAssert(0,0);
        break; 
      }
    }
  }
/* pragma omp target */
}
#endif /* HTC_KEEP_MODULE___HTC_HOST_ENTRY_OUT__1__6291__ */
#endif /* CNY_HTC_COPROC */
#ifdef CNY_HTC_HOST
/* bfs_tree */
/* bfs_packed */
/* bfs_tree_bit */
/* bfs_tree_bit_new */
/* CTL parameters */
#endif /* CNY_HTC_HOST */
#ifdef CNY_HTC_HOST
/* extern "C" */
#if DEBUG
#endif
//#pragma omp parallel num_threads(unitCnt)
//#pragma omp for nowait schedule(static, 1)
#if DEBUG
#endif
/* bfsAddr */
/* bfs_packed */
/* bfs_tree_bit */
/* bfs_tree_bit_new */
/* xoff */
/* xadj */
/* bmapIdx */
#if DEBUG
#endif
#endif /* CNY_HTC_HOST */
#ifdef CNY_HTC_HOST
/* extern "C" */
#if DEBUG
#endif
// BFS_SIZE used for K2 on scatter instruction
#if DEBUG
#endif
//#pragma omp parallel num_threads(unitCnt)
//#pragma omp for nowait schedule(static , 1) private(unit)
#if DEBUG
#endif
/* bfsAddr */
/* bfs_packed */
/* bfs_tree_bit */
/* bfs_tree_bit_new */
/* xoff */
/* xadj */
/* bmapIdx */
/* omp parallel */
#if DEBUG
#endif
#endif /* CNY_HTC_HOST */
#ifdef CNY_HTC_HOST
/* extern "C" */
#if DEBUG
#endif
/* bfsSize / 64 */
#if DEBUG
#endif
//#pragma omp parallel num_threads(unitCnt) reduction(+:updCnt)
//#pragma omp for nowait schedule(static , 1) private(unit)
/* bfsAddr */
/* xadj_index_shift passed in bfs_packed slot */
/* bfs_tree_bit */
/* bfs_tree_bit_new */
/* xoff */
/* xadj */
/* bmapIdx */
/* lb */
/* ub */
#if DEBUG
#endif
// flip addresses for next iteration
/* while (*k1 != *k2) */
#endif /* CNY_HTC_HOST */
#if 0

static void OUT__1__6827__(htc_tid_t P___htc_parent_htid,htc_tid_t P___htc_my_omp_thread_num,htc_tid_t P___htc_my_omp_team_size)
#endif
#ifdef HTC_KEEP_MODULE_OUT__1__6827__
#include "Ht.h"
#include "PersOUT__1__6827__.h"
void CPersOUT__1__6827__::PersOUT__1__6827__()
{
  if (PR_htValid) {
    switch(PR_htInst){
      case OUT__1__6827____START:
{
        P_mif_rdVal_index = PR_htId;
        P___htc_uplevel_index_0 = ((OUT__1__6827___uplevel_index_t )PR_htId);
        P___htc_uplevel_index_1 = ((__HTC_HOST_ENTRY_OUT__1__6291___uplevel_index_t )P___htc_parent_htid);
        HtContinue(OUT__1__6827____GR__38);
        break; 
      }
      case OUT__1__6827____GR__38:
{
        switch((GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___function_function())){
          case INIT:
{
            HtContinue(OUT__1__6827____SWITCH_CASE__49);
            break; 
          }
          case SCATTER:
{
            HtContinue(OUT__1__6827____SWITCH_CASE__50);
            break; 
          }
          case BFS:
{
            HtContinue(OUT__1__6827____SWITCH_CASE__51);
            break; 
          }
          default:
{
            HtContinue(OUT__1__6827____SWITCH_JOIN__48);
            break; 
          }
        }
        break; 
      }
      case OUT__1__6827____SWITCH_CASE__49:
{
        uint32_t P___htc_t_p_lower_;
/* XOMP_loop_static_init(lb,ub - 1,1,1); */
;;
        P___htc_t_p_lower_ = ((uint32_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___lb_lb() + 1 * P___htc_my_omp_thread_num));
        P_p_upper_ = ((uint32_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___ub_ub() - 1));
        P___htc_next_chunk7 = ((uint32_t )(1 * P___htc_my_omp_team_size));;
        P_p_index_ = ((uint32_t )P___htc_t_p_lower_);
        HtContinue(OUT__1__6827____LOOP_TOP__11);
        break; 
      }
      case OUT__1__6827____LOOP_TOP__11:
{
        MemAddr_t P___htc_t_arrptr24;
        if (WriteMemBusy()) {
          HtRetry();
          break; 
        }
        if (!(P_p_index_ <= P_p_upper_)) {
          HtContinue(OUT__1__6827____LOOP_EXIT__11);
          break; 
        }
        P___htc_t_arrptr24 = ((MemAddr_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___bfsAddr_bfsAddr()));
        WriteMem_uint64(((MemAddr_t )(((MemAddr_t )P___htc_t_arrptr24) + (0 + P_p_index_ << 3U))),((uint64_t )0xffffffffffffffffULL));
        P_p_index_ = ((uint32_t )(P_p_index_ + P___htc_next_chunk7));
        WriteMemPause(OUT__1__6827____LOOP_TOP__11);
        break; 
      }
      case OUT__1__6827____LOOP_EXIT__11:
{
/* XOMP_loop_end_nowait(); */
;
        HtContinue(OUT__1__6827____SWITCH_JOIN__48);
        break; 
      }
      case OUT__1__6827____SWITCH_CASE__50:
{
        uint32_t P___htc_t_p_lower___16;
/* XOMP_loop_static_init(lb,ub - 1,1,1); */
;;
        P___htc_t_p_lower___16 = ((uint32_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___lb_lb() + 1 * P___htc_my_omp_thread_num));
        P_p_upper___17 = ((uint32_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___ub_ub() - 1));
        P___htc_next_chunk8 = ((uint32_t )(1 * P___htc_my_omp_team_size));;
        P_p_index___15 = ((uint32_t )P___htc_t_p_lower___16);
        HtContinue(OUT__1__6827____LOOP_TOP__12);
        break; 
      }
      case OUT__1__6827____LOOP_TOP__12:
{
        MemAddr_t P___htc_t_arrptr25;
        if (ReadMemBusy()) {
          HtRetry();
          break; 
        }
        if (!(P_p_index___15 <= P_p_upper___17)) {
          HtContinue(OUT__1__6827____LOOP_EXIT__12);
          break; 
        }
        P___htc_t_arrptr25 = ((MemAddr_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___bfsPackedAddr_bfsPackedAddr()));
/* __htc_t_arrptr26 =((uint64_t )__htc_t_arrptr25[((unsigned int )2) * p_index___15 +((unsigned int )1)]); */
        ReadMem_f0(((MemAddr_t )(((MemAddr_t )P___htc_t_arrptr25) + (0 + (((unsigned int )2) * P_p_index___15 + ((unsigned int )1)) << 3U))),PR_htId);
        ReadMemPause(OUT__1__6827____READMEM__41);
        break; 
      }
      case OUT__1__6827____READMEM__41:
{
        MemAddr_t P___htc_t_arrptr28;
        if (ReadMemBusy()) {
          HtRetry();
          break; 
        }
        P___htc_t_arrptr26 = ((uint64_t )(GR_mifRdVal_OUT__1__6827___fld0()));
        P___htc_t_arrptr27 = ((MemAddr_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___bfsAddr_bfsAddr()));
        P___htc_t_arrptr28 = ((MemAddr_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___bfsPackedAddr_bfsPackedAddr()));
/* __htc_t_arrptr27[__htc_t_arrptr26] =((uint64_t )__htc_t_arrptr28[2 * p_index___15]); */
        ReadMem_f0(((MemAddr_t )(((MemAddr_t )P___htc_t_arrptr28) + (0 + 2 * P_p_index___15 << 3U))),PR_htId);
        ReadMemPause(OUT__1__6827____READMEM__39);
        break; 
      }
      case OUT__1__6827____READMEM__39:
{
        if (WriteMemBusy()) {
          HtRetry();
          break; 
        }
        WriteMem_uint64(((MemAddr_t )(((MemAddr_t )P___htc_t_arrptr27) + (0 + P___htc_t_arrptr26 << 3U))),((uint64_t )(GR_mifRdVal_OUT__1__6827___fld0())));
        P_p_index___15 = ((uint32_t )(P_p_index___15 + P___htc_next_chunk8));
        WriteMemPause(OUT__1__6827____LOOP_TOP__12);
        break; 
      }
      case OUT__1__6827____LOOP_EXIT__12:
{
/* XOMP_loop_end_nowait(); */
;
        HtContinue(OUT__1__6827____SWITCH_JOIN__48);
        break; 
      }
      case OUT__1__6827____SWITCH_CASE__51:
{
        uint32_t P___htc_t_p_lower___19;
/* XOMP_loop_static_init(lb,ub - 1,1,1); */
;;
        P___htc_t_p_lower___19 = ((uint32_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___lb_lb() + 1 * P___htc_my_omp_thread_num));
        P_p_upper___20 = ((uint32_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___ub_ub() - 1));
        P___htc_next_chunk9 = ((uint32_t )(1 * P___htc_my_omp_team_size));;
        P_p_index___18 = ((uint32_t )P___htc_t_p_lower___19);
        HtContinue(OUT__1__6827____LOOP_TOP__14);
        break; 
      }
      case OUT__1__6827____LOOP_TOP__14:
{
        MemAddr_t P___htc_t_arrptr29;
        if (ReadMemBusy()) {
          HtRetry();
          break; 
        }
        if (!(P_p_index___18 <= P_p_upper___20)) {
          HtContinue(OUT__1__6827____LOOP_EXIT__14);
          break; 
        }
        P_bitCnt = ((uint8_t )((uint8_t )0));
        P_bmapUpdCnt = ((uint8_t )((uint8_t )0));
        P___htc_t_arrptr29 = ((MemAddr_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___bmapOldAddr_bmapOldAddr()));
/* mask =((uint64_t )__htc_t_arrptr29[p_index___18]); */
        ReadMem_f0(((MemAddr_t )(((MemAddr_t )P___htc_t_arrptr29) + (0 + P_p_index___18 << 3U))),PR_htId);
        ReadMemPause(OUT__1__6827____READMEM__40);
        break; 
      }
      case OUT__1__6827____READMEM__40:
{
        P_mask = ((uint64_t )(GR_mifRdVal_OUT__1__6827___fld0()));
        if (!(P_mask != 0)) {
          HtContinue(OUT__1__6827____IF_JOIN__46);
          break; 
        };
        P_bitCnt = ((uint8_t )P_bitCnt);
        HtContinue(OUT__1__6827____LOOP_TOP__13);
        break; 
      }
      case OUT__1__6827____LOOP_TOP__13:
{
        uint32_t P_vertex;
        if (SendCallBusy_bufp()) {
          HtRetry();
          break; 
        }
        if (!(P_bitCnt < 64)) {
          HtContinue(OUT__1__6827____LOOP_EXIT__13);
          break; 
        }
        if (!((P_mask >> P_bitCnt & 0x0000000000000001) == 0x0000000000000001)) {
          HtContinue(OUT__1__6827____IF_ELSE__45);
          break; 
        }
// call bufp
        P_vertex = ((uint32_t )(((uint32_t )(P_p_index___18 * ((unsigned int )64))) + ((unsigned int )P_bitCnt)));
/* xadj_index_shift */
        SendCall_bufp(OUT__1__6827____CALL_RTN__22,P___htc_parent_htid,P___htc_my_omp_thread_num,P___htc_my_omp_team_size,P_vertex,GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___xoff_xoff(),GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___xadj_xadj(),GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___bfsAddr_bfsAddr(),GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___bmapOldAddr_bmapOldAddr(),((uint8_t )(((uint64_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___bfsPackedAddr_bfsPackedAddr())) & ((unsigned long )0x0000000000000001))));
        break; 
      }
      case OUT__1__6827____CALL_RTN__22:
{
        uint8_t P___htc_t_fn21;
        P___htc_t_fn21 = ((uint8_t )P_retval_bufp);
        if (P___htc_t_fn21 & 0x0000000000000001) {
          P_mask = ((uint64_t )(P_mask & ~(1ULL << P_bitCnt)));
          P_bmapUpdCnt = P_bmapUpdCnt + 1;
        }
        HtContinue(OUT__1__6827____IF_JOIN__45);
        break; 
      }
      case OUT__1__6827____IF_ELSE__45:
{
        uint16_t P_skip;
        uint16_t P_tmask;
        P_tmask = ((uint16_t )((uint16_t )(P_mask >> ((int )P_bitCnt) + 1)));
        P_skip = ((uint16_t )((uint16_t )0));
        if ((P_tmask & 0xff) == 0) {
          P_skip = ((uint16_t )8);
          P_tmask = ((uint16_t )(P_tmask >> 8));
        }
        if ((P_tmask & 0x0f) == 0) {
          P_skip = ((uint16_t )(P_skip + 4));
          P_tmask = ((uint16_t )(P_tmask >> 4));
        }
        if ((P_tmask & 0x03) == 0) {
          P_skip = ((uint16_t )(P_skip + 2));
          P_tmask = ((uint16_t )(P_tmask >> 2));
        }
        P_skip = ((uint16_t )(P_skip + (1 - (P_tmask & 0x0000000000000001))));
        P_bitCnt = ((uint8_t )(P_bitCnt + P_skip));
        HtContinue(OUT__1__6827____IF_JOIN__45);
        break; 
      }
      case OUT__1__6827____IF_JOIN__45:
{
        P_bitCnt = P_bitCnt + 1;
        HtContinue(OUT__1__6827____LOOP_TOP__13);
        break; 
      }
      case OUT__1__6827____LOOP_EXIT__13:
{;
        HtContinue(OUT__1__6827____IF_JOIN__46);
        break; 
      }
      case OUT__1__6827____IF_JOIN__46:
{
        if (!P_bmapUpdCnt) {
          HtContinue(OUT__1__6827____GW__42__IF_JOIN__47);
          break; 
        }
        GW___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___my_update_count_my_update_count(P___htc_uplevel_index_1,((uint32_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___my_update_count_my_update_count() + P_bmapUpdCnt)));
        HtContinue(OUT__1__6827____GW__42__IF_JOIN__47);
        break; 
      }
      case OUT__1__6827____GW__42__IF_JOIN__47:
{
        MemAddr_t P___htc_t_arrptr30;
        if (WriteMemBusy()) {
          HtRetry();
          break; 
        }
        P___htc_t_arrptr30 = ((MemAddr_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___bmapNewAddr_bmapNewAddr()));
        WriteMem_uint64(((MemAddr_t )(((MemAddr_t )P___htc_t_arrptr30) + (0 + P_p_index___18 << 3U))),((uint64_t )P_mask));
        P_p_index___18 = ((uint32_t )(P_p_index___18 + P___htc_next_chunk9));
        WriteMemPause(OUT__1__6827____LOOP_TOP__14);
        break; 
      }
      case OUT__1__6827____LOOP_EXIT__14:
{
        MemAddr_t P___htc_t_arrptr31;
        if (WriteMemBusy()) {
          HtRetry();
          break; 
        }
/* XOMP_loop_end_nowait(); */
;
        P___htc_t_arrptr31 = ((MemAddr_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___update_count_update_count()));
        WriteMem_uint64(((MemAddr_t )((MemAddr_t )P___htc_t_arrptr31)),((uint64_t )((uint64_t )(GR___htc_UplevelGV___HTC_HOST_ENTRY_OUT__1__6291___my_update_count_my_update_count()))));
        WriteMemPause(OUT__1__6827____WRITEMEM__43);
        break; 
      }
      case OUT__1__6827____WRITEMEM__43:
{
        HtContinue(OUT__1__6827____SWITCH_JOIN__48);
        break; 
      }
      case OUT__1__6827____SWITCH_JOIN__48:
{
        if (SendReturnBusy_OUT__1__6827__()) {
          HtRetry();
          break; 
        }
        SendReturn_OUT__1__6827__();
        break; 
      }
      default:
{
        HtAssert(0,0);
        break; 
      }
    }
  }
/* end of parallel */
}
#endif /* HTC_KEEP_MODULE_OUT__1__6827__ */
