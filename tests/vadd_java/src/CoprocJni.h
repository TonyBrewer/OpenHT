#pragma once
#include <jni.h>

#define CNY_HT_AEUCNT 16

struct CoprocJni {
  CHtHif * pHtHif;
  volatile int32_t unitCnt;
  CHtAuUnit * pUnit[CNY_HT_AEUCNT];
  uint64_t vecLen;
  uint64_t *cp_a1;
  uint64_t *cp_a2;
  uint64_t *cp_a3;
  CoprocJni() {
    unitCnt = 1;
  }
};
