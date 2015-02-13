typedef int rhomp_lock_t;
//typedef int bool;
extern int ___htc_dummy_decl_for_preproc_info;
#if 0

int lock(htc_tid_t P___htc_parent_htid,htc_tid_t P___htc_my_omp_thread_num,htc_tid_t P___htc_my_omp_team_size,int P_op,rhomp_lock_t P_L)
#endif
#ifdef HTC_KEEP_MODULE_LOCK
#include "Ht.h"
#include "PersLock.h"
void CPersLock::PersLock()
{
  if (PR_htValid) {
    switch(PR_htInst){
      case LOCK__START:
{
        int P_result;
        if (SendReturnBusy_lock()) {
          HtRetry();
          break; 
        }
        P_result = ((int )0);
// request lock
        if (P_op == 0) {
          if ((S_lk & 1 << P_L) == 0) {
            S_lk = ((int )(S_lk | 1 << P_L));
            P_result = ((int )1);
          }
// request unlock
        }
        else {
          S_lk = ((int )(S_lk & ~(1 << P_L)));
        }
        SendReturn_lock(P_result);
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
#endif /* HTC_KEEP_MODULE_LOCK */
#if 0

int rhomp_test_lock(htc_tid_t P___htc_parent_htid,htc_tid_t P___htc_my_omp_thread_num,htc_tid_t P___htc_my_omp_team_size,rhomp_lock_t P_L)
#endif
#ifdef HTC_KEEP_MODULE_RHOMP_TEST_LOCK
#include "Ht.h"
#include "PersRhomp_test_lock.h"
void CPersRhomp_test_lock::PersRhomp_test_lock()
{
  if (PR_htValid) {
    switch(PR_htInst){
      case RHOMP_TEST_LOCK__START:
{
        if (SendCallBusy_lock()) {
          HtRetry();
          break; 
        }
        SendCall_lock(RHOMP_TEST_LOCK__CALL_RTN__5,P___htc_parent_htid,P___htc_my_omp_thread_num,P___htc_my_omp_team_size,0,P_L);
        break; 
      }
      case RHOMP_TEST_LOCK__CALL_RTN__5:
{
        int P___htc_t_fn2;
        if (SendReturnBusy_rhomp_test_lock()) {
          HtRetry();
          break; 
        }
        P___htc_t_fn2 = ((int )P_retval_lock);
        SendReturn_rhomp_test_lock(P___htc_t_fn2);
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
#endif /* HTC_KEEP_MODULE_RHOMP_TEST_LOCK */
#if 0

void rhomp_set_lock(htc_tid_t P___htc_parent_htid,htc_tid_t P___htc_my_omp_thread_num,htc_tid_t P___htc_my_omp_team_size,rhomp_lock_t P_L)
#endif
#ifdef HTC_KEEP_MODULE_RHOMP_SET_LOCK
#include "Ht.h"
#include "PersRhomp_set_lock.h"
void CPersRhomp_set_lock::PersRhomp_set_lock()
{
  if (PR_htValid) {
    switch(PR_htInst){
      case RHOMP_SET_LOCK__START:
{;
        HtContinue(RHOMP_SET_LOCK__LOOP_TOP__1);
        break; 
      }
      case RHOMP_SET_LOCK__LOOP_TOP__1:
{
        if (SendCallBusy_lock()) {
          HtRetry();
          break; 
        }
        SendCall_lock(RHOMP_SET_LOCK__CALL_RTN__6,P___htc_parent_htid,P___htc_my_omp_thread_num,P___htc_my_omp_team_size,0,P_L);
        break; 
      }
      case RHOMP_SET_LOCK__CALL_RTN__6:
{
        int P___htc_t_fn3;
        P___htc_t_fn3 = ((int )P_retval_lock);
        if (!(P___htc_t_fn3 != 1)) {
          HtContinue(RHOMP_SET_LOCK__LOOP_EXIT__1);
          break; 
        }
        HtContinue(RHOMP_SET_LOCK__LOOP_TOP__1);
        break; 
      }
      case RHOMP_SET_LOCK__LOOP_EXIT__1:
{
        if (SendReturnBusy_rhomp_set_lock()) {
          HtRetry();
          break; 
        }
        SendReturn_rhomp_set_lock();
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
#endif /* HTC_KEEP_MODULE_RHOMP_SET_LOCK */
#if 0

void rhomp_unset_lock(htc_tid_t P___htc_parent_htid,htc_tid_t P___htc_my_omp_thread_num,htc_tid_t P___htc_my_omp_team_size,rhomp_lock_t P_L)
#endif
#ifdef HTC_KEEP_MODULE_RHOMP_UNSET_LOCK
#include "Ht.h"
#include "PersRhomp_unset_lock.h"
void CPersRhomp_unset_lock::PersRhomp_unset_lock()
{
  if (PR_htValid) {
    switch(PR_htInst){
      case RHOMP_UNSET_LOCK__START:
{
        if (SendCallBusy_lock()) {
          HtRetry();
          break; 
        }
        SendCall_lock(RHOMP_UNSET_LOCK__CALL_RTN__7,P___htc_parent_htid,P___htc_my_omp_thread_num,P___htc_my_omp_team_size,1,P_L);
        break; 
      }
      case RHOMP_UNSET_LOCK__CALL_RTN__7:
{
        if (SendReturnBusy_rhomp_unset_lock()) {
          HtRetry();
          break; 
        }
        SendReturn_rhomp_unset_lock();
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
#endif /* HTC_KEEP_MODULE_RHOMP_UNSET_LOCK */
#if 0

rhomp_lock_t rhomp_init_lock(P___htc_parent_htid,P___htc_my_omp_thread_num,P___htc_my_omp_team_size)
htc_tid_t P___htc_parent_htid;
htc_tid_t P___htc_my_omp_thread_num;
htc_tid_t P___htc_my_omp_team_size;
#endif
#ifdef HTC_KEEP_MODULE_RHOMP_INIT_LOCK
#include "Ht.h"
#include "PersRhomp_init_lock.h"
void CPersRhomp_init_lock::PersRhomp_init_lock()
{
  if (PR_htValid) {
    switch(PR_htInst){
      case RHOMP_INIT_LOCK__START:
{
        rhomp_lock_t P_result;
        if (SendReturnBusy_rhomp_init_lock()) {
          HtRetry();
          break; 
        }
        P_result = ((rhomp_lock_t )S_lock_counter);
        S_lock_counter = S_lock_counter + 1;
        SendReturn_rhomp_init_lock(P_result);
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
#endif /* HTC_KEEP_MODULE_RHOMP_INIT_LOCK */
#if 0

void rhomp_begin_named_critical(htc_tid_t P___htc_parent_htid,htc_tid_t P___htc_my_omp_thread_num,htc_tid_t P___htc_my_omp_team_size,rhomp_lock_t P_L)
#endif
#ifdef HTC_KEEP_MODULE_RHOMP_BEGIN_NAMED_CRITICAL
#include "Ht.h"
#include "PersRhomp_begin_named_critical.h"
void CPersRhomp_begin_named_critical::PersRhomp_begin_named_critical()
{
  if (PR_htValid) {
    switch(PR_htInst){
      case RHOMP_BEGIN_NAMED_CRITICAL__START:
{
        if (SendCallBusy_rhomp_set_lock()) {
          HtRetry();
          break; 
        }
        if (!P_L) {
          HtContinue(RHOMP_BEGIN_NAMED_CRITICAL__IF_ELSE__14);
          break; 
        }
        SendCall_rhomp_set_lock(RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__8,P_L);
        break; 
      }
      case RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__8:
{
        if (SendReturnBusy_rhomp_begin_named_critical()) {
          HtRetry();
          break; 
        }
        SendReturn_rhomp_begin_named_critical();
        break; 
        HtContinue(RHOMP_BEGIN_NAMED_CRITICAL__IF_JOIN__14);
        break; 
      }
      case RHOMP_BEGIN_NAMED_CRITICAL__IF_ELSE__14:
{
        if (SendCallBusy_rhomp_set_lock()) {
          HtRetry();
          break; 
        }
        SendCall_rhomp_set_lock(RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__9,3);
        break; 
      }
      case RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__9:
{
        if (SendCallBusy_rhomp_init_lock()) {
          HtRetry();
          break; 
        }
        if (!(P_L == 0)) {
          HtContinue(RHOMP_BEGIN_NAMED_CRITICAL__IF_JOIN__13);
          break; 
        }
        SendCall_rhomp_init_lock(RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__10);
        break; 
      }
      case RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__10:
{
        rhomp_lock_t P_temp;
        rhomp_lock_t P___htc_t_fn4;
        P___htc_t_fn4 = ((rhomp_lock_t )P_retval_rhomp_init_lock);
        P_temp = ((rhomp_lock_t )P___htc_t_fn4);
        P_GW = ((rhomp_lock_t )P_temp);
        HtContinue(RHOMP_BEGIN_NAMED_CRITICAL__IF_JOIN__13);
        break; 
      }
      case RHOMP_BEGIN_NAMED_CRITICAL__IF_JOIN__13:
{
        if (SendCallBusy_rhomp_unset_lock()) {
          HtRetry();
          break; 
        }
        SendCall_rhomp_unset_lock(RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__11,3);
        break; 
      }
      case RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__11:
{
        if (SendCallBusy_rhomp_set_lock()) {
          HtRetry();
          break; 
        }
        SendCall_rhomp_set_lock(RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__12,P_GW);
        break; 
      }
      case RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__12:
{
        if (SendReturnBusy_rhomp_begin_named_critical()) {
          HtRetry();
          break; 
        }
        SendReturn_rhomp_begin_named_critical();
        break; 
      }
      case RHOMP_BEGIN_NAMED_CRITICAL__IF_JOIN__14:
{
        if (SendReturnBusy_rhomp_begin_named_critical()) {
          HtRetry();
          break; 
        }
        SendReturn_rhomp_begin_named_critical();
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
#endif /* HTC_KEEP_MODULE_RHOMP_BEGIN_NAMED_CRITICAL */
#if 0
#endif
