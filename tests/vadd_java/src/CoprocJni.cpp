#include "Ht.h"
using namespace Ht;

#include "CoprocJni.h"
#include "com_vadd_coprocJni_CoprocJni.h"

static CoprocJni coprocJni;

JNIEXPORT jint JNICALL Java_com_vadd_coprocJni_CoprocJni_initCoproc
(JNIEnv * pEnv, 
 jclass cl, 
 jlong vecLen)
{
  coprocJni.vecLen = vecLen;
  coprocJni.pHtHif = new CHtHif();
  coprocJni.unitCnt = coprocJni.pHtHif->GetUnitCnt();
  printf("#AUs = %d\n", coprocJni.unitCnt);

  for (int unit = 0; unit < coprocJni.unitCnt; unit++)
    coprocJni.pUnit[unit] = new CHtAuUnit(coprocJni.pHtHif);

  // Coprocessor memory arrays
  coprocJni.cp_a1 = (uint64_t*)coprocJni.pHtHif->MemAlloc(vecLen * sizeof(uint64_t));
  coprocJni.cp_a2 = (uint64_t*)coprocJni.pHtHif->MemAlloc(vecLen * sizeof(uint64_t));
  coprocJni.cp_a3 = (uint64_t*)coprocJni.pHtHif->MemAlloc(vecLen * sizeof(uint64_t));
  if (!coprocJni.cp_a1 || !coprocJni.cp_a2 || !coprocJni.cp_a3) {
    fprintf(stderr, "ht_cp_malloc() failed.\n");
    return(-1);
  }
  return(0);
}

JNIEXPORT void JNICALL Java_com_vadd_coprocJni_CoprocJni_exitCoproc
        (JNIEnv * pEnv, jclass cl)
{
  coprocJni.pHtHif->MemFree(coprocJni.cp_a1);
  coprocJni.pHtHif->MemFree(coprocJni.cp_a2);
  coprocJni.pHtHif->MemFree(coprocJni.cp_a3);
  delete coprocJni.pHtHif;
}

JNIEXPORT jlong JNICALL Java_com_vadd_coprocJni_CoprocJni_vadd
(JNIEnv * pEnv, jclass cl,
 jlong vecLen,
 jlongArray a1, jlongArray a2, jlongArray a3
 )
{
  assert(vecLen <= (jlong)coprocJni.vecLen);
  jlong * tempBuf;
  // convert java array to c++ array and then copy to coproc memory
  // accessing coproc memory directly from a core is very slow so this should be
  // faster for larger arrays
  tempBuf = pEnv->GetLongArrayElements(a1, NULL);
  coprocJni.pHtHif->MemCpy(coprocJni.cp_a1, (char*)tempBuf, vecLen * sizeof(uint64_t));
  pEnv->ReleaseLongArrayElements(a1, tempBuf, JNI_ABORT);

  tempBuf = pEnv->GetLongArrayElements(a2, NULL);
  coprocJni.pHtHif->MemCpy(coprocJni.cp_a2,(char*) tempBuf, vecLen * sizeof(uint64_t));
  pEnv->ReleaseLongArrayElements(a2, tempBuf, JNI_ABORT);

  coprocJni.pHtHif->MemSet(coprocJni.cp_a3, 0, vecLen * sizeof(uint64_t));

  // avoid bank aliasing for performance
  int32_t unitCnt = coprocJni.unitCnt;
  if (unitCnt > 16 && !(unitCnt & 1)) unitCnt -= 1;
  printf("stride = %d\n", unitCnt);
  fflush(stdout);
  // Send arguments to all units using messages
  coprocJni.pHtHif->SendAllHostMsg(OP1_ADDR, (uint64_t)coprocJni.cp_a1);
  coprocJni.pHtHif->SendAllHostMsg(OP2_ADDR, (uint64_t)coprocJni.cp_a2);
  coprocJni.pHtHif->SendAllHostMsg(RES_ADDR, (uint64_t)coprocJni.cp_a3);
  coprocJni.pHtHif->SendAllHostMsg(VEC_LEN, (uint64_t)vecLen);

  // Send calls to units
  for (int unit = 0; unit < unitCnt; unit++)
    coprocJni.pUnit[unit]->SendCall_htmain(unit /*offset*/, unitCnt /*stride*/);
  // Wait for returns
  uint64_t act_sum = 0;
  for (int unit = 0; unit < unitCnt; unit++) {
    uint64_t au_sum;
    while (!coprocJni.pUnit[unit]->RecvReturn_htmain(au_sum))
      usleep(1000);
    printf("unit=%-2d: au_sum %llu \n", unit, (long long)au_sum);
    fflush(stdout);
    act_sum += au_sum;
  }
  // copy results from coproc and then convert to java array
  tempBuf = new jlong[vecLen];
  coprocJni.pHtHif->MemCpy((char*)tempBuf, coprocJni.cp_a3, vecLen * sizeof(uint64_t));
  pEnv->SetLongArrayRegion(a3, 0, vecLen, tempBuf);
  delete[] tempBuf;
  return(act_sum);
}
