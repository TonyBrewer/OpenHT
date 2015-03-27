//typedef int omp_lock_t;
//typedef int bool;
#if 0

int __htc_lock(int P_op,omp_lock_t P_L)
#endif
#ifdef HTC_KEEP_MODULE___HTC_LOCK
#include "Ht.h"
#include "Pers__htc_lock.h"
void CPers__htc_lock::Pers__htc_lock()
{
  if (PR_htValid) {
    switch(PR_htInst){
      case LOCK__START:
{
        int T_result;

        if (SendReturnBusy___htc_lock()) {
          HtRetry();
          break; 
        }
        T_result = ((int )0);
// request lock
        if (P_op == 0) {
          if ((S_lk & 1 << P_L) == 0) {
            S_lk = ((int )(S_lk | 1 << P_L));
            T_result = ((int )1);
          }
// request unlock
        }
        else {
          S_lk = ((int )(S_lk & ~(1 << P_L)));
        }
        SendReturn___htc_lock(T_result);
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
#endif /* HTC_KEEP_MODULE___HTC_LOCK */
#if 0

int rhomp_test_lock(omp_lock_t P_L)
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
        if (SendCallBusy___htc_lock()) {
          HtRetry();
          break; 
        }
        SendCall___htc_lock(RHOMP_TEST_LOCK__CALL_RTN__14,0,P_L);
        break; 
      }
      case RHOMP_TEST_LOCK__CALL_RTN__14:
{
        if (SendReturnBusy_rhomp_test_lock()) {
          HtRetry();
          break; 
        }
        SendReturn_rhomp_test_lock(P___retval_lock);
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

void rhomp_set_lock(omp_lock_t P_L)
#endif
#ifdef HTC_KEEP_MODULE_RHOMP_SET_LOCK
#include "Ht.h"
#include "PersRhomp_set_lock.h"
void CPersRhomp_set_lock::PersRhomp_set_lock()
{
  if (PR_htValid) {
    switch(PR_htInst){
      case RHOMP_SET_LOCK__START:
{
        HtContinue(RHOMP_SET_LOCK__LOOP_TOP__4);
        break; 
      }
      case RHOMP_SET_LOCK__LOOP_TOP__4:
{
        if (SendCallBusy___htc_lock()) {
          HtRetry();
          break; 
        }
        SendCall___htc_lock(RHOMP_SET_LOCK__CALL_RTN__15,0,P_L);
        break; 
      }
      case RHOMP_SET_LOCK__CALL_RTN__15:
{
        if (P_retval_lock == 1) {
          HtContinue(RHOMP_SET_LOCK__LOOP_EXIT__4);
          break; 
        }
        // printf("waiting\n");
        HtContinue(RHOMP_SET_LOCK__LOOP_TOP__4);
        break; 
      }
      case RHOMP_SET_LOCK__LOOP_EXIT__4:
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

void rhomp_unset_lock(omp_lock_t P_L)
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
        if (SendCallBusy___htc_lock()) {
          HtRetry();
          break; 
        }
        SendCall___htc_lock(RHOMP_UNSET_LOCK__CALL_RTN__16,1,P_L);
        break; 
      }
      case RHOMP_UNSET_LOCK__CALL_RTN__16:
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

omp_lock_t rhomp_init_lock()
#endif
#ifdef HTC_KEEP_MODULE_RHOMP_INIT_LOCK
#include "Ht.h"
#include "PersRhomp_init_lock.h"
void CPersRhomp_init_lock::PersRhomp_init_lock()
{
  if (PR_htValid) {
    switch(PR_htInst){
    case RHOMP_INIT_LOCK:
{
        if (SendReturnBusy_rhomp_init_lock()) {
          HtRetry();
          break; 
        }

        P_result = -1;
        // 0 and 1 reserved for critical and atomic, so start counter
        // at 2;
        if (S_lock_counter < 2) {
            S_lock_counter = 1;
        }
        if (S_lock_counter < 64) {
            P_result = ((omp_lock_t )S_lock_counter);
            S_lock_counter = S_lock_counter + 1;
        } else if (!S_Q.empty()) {
            P_result = S_Q.front();
            S_Q.pop();
        }
        SendReturn_rhomp_init_lock(P_result);
        break; 
      }
      case RHOMP_DESTROY_LOCK:
{
        if (SendReturnBusy_rhomp_destroy_lock()) {
          HtRetry();
          break; 
        }
        S_Q.push(P_lk);
        SendReturn_rhomp_destroy_lock();
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

void rhomp_begin_named_critical(omp_lock_t P_L)
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
          HtContinue(RHOMP_BEGIN_NAMED_CRITICAL__IF_ELSE__34);
          break; 
        }
        SendCall_rhomp_set_lock(RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__17,P_L);
        break; 
      }
      case RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__17:
{
        if (SendReturnBusy_rhomp_begin_named_critical()) {
          HtRetry();
          break; 
        }
        SendReturn_rhomp_begin_named_critical();
        HtContinue(RHOMP_BEGIN_NAMED_CRITICAL__IF_JOIN__34);
        break; 
      }
      case RHOMP_BEGIN_NAMED_CRITICAL__IF_ELSE__34:
{
        if (SendCallBusy_rhomp_set_lock()) {
          HtRetry();
          break; 
        }
        SendCall_rhomp_set_lock(RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__18,3);
        break; 
      }
      case RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__18:
{
        if (SendCallBusy_rhomp_init_lock()) {
          HtRetry();
          break; 
        }
        if (!(P_L == 0)) {
          HtContinue(RHOMP_BEGIN_NAMED_CRITICAL__IF_JOIN__33);
          break; 
        }
        SendCall_rhomp_init_lock(RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__19);
        break; 
      }
      case RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__19:
{
        P___htc_t_fn8 = ((omp_lock_t )P_retval_rhomp_init_lock);
        P_temp = ((omp_lock_t )P___htc_t_fn8);
        P_GW = ((omp_lock_t)P_temp);
        HtContinue(RHOMP_BEGIN_NAMED_CRITICAL__IF_JOIN__33);
        break; 
      }
      case RHOMP_BEGIN_NAMED_CRITICAL__IF_JOIN__33:
{
        if (SendCallBusy_rhomp_unset_lock()) {
          HtRetry();
          break; 
        }
        SendCall_rhomp_unset_lock(RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__20,3);
        break; 
      }
      case RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__20:
{
        if (SendCallBusy_rhomp_set_lock()) {
          HtRetry();
          break; 
        }
        SendCall_rhomp_set_lock(RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__21,P_GW);
        break; 
      }
      case RHOMP_BEGIN_NAMED_CRITICAL__CALL_RTN__21:
{
        if (SendReturnBusy_rhomp_begin_named_critical()) {
          HtRetry();
          break; 
        }
        SendReturn_rhomp_begin_named_critical();
        break; 
      }
      case RHOMP_BEGIN_NAMED_CRITICAL__IF_JOIN__34:
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



