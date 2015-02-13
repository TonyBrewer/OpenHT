#include "Ht.h"
using namespace Ht;

#include "pers.h"

#define TH_CNT 32
#define TH_ELEM_CNT 128
#define ELEM_CNT (TH_CNT * TH_ELEM_CNT)

uint64_t arr[ELEM_CNT];

int main(int argc, char **argv)
{
	assert(TH_ELEM_CNT < (1 << MAX_ELEM_CNT_W));

	CHtHif *pHtHif = new CHtHif();
	CHtSuUnit *pSuUnit = new CHtSuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	for (int i = 0; i < ELEM_CNT; i++)
		arr[i] = i;

	pSuUnit->SendHostMsg(SU_ARRAY_ADDR, (uint64_t)&arr);

	uint32_t rtn_elemCnt;
	uint32_t totalElemCnt = 0;
	uint32_t rtn_threadId;

	int callIdx = 0;
	int rtnIdx = 0;
	while (callIdx < TH_CNT || rtnIdx < TH_CNT) {
		if (callIdx < TH_CNT && pSuUnit->SendCall_htmain(callIdx % 13, callIdx * TH_ELEM_CNT, TH_ELEM_CNT, callIdx)) {
			callIdx += 1;
		} else if (pSuUnit->RecvReturn_htmain(rtn_elemCnt, rtn_threadId)) {
			rtnIdx += 1;
			totalElemCnt += rtn_elemCnt;
			printf("Thread %d finished\n", rtn_threadId);
		} else {
			usleep(1);
		}
	}

	assert(totalElemCnt == ELEM_CNT);

	// check results
	int err_cnt = 0;
	for (unsigned i = 0; i < ELEM_CNT; i++) {
		if (arr[i] != i + 1) {
			err_cnt += 1;
			printf("arr[%d] is %lld, should be %d\n",
			       i, (long long)arr[i], i + 1);
		}
	}

	delete pHtHif;

	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}
