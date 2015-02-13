#include "Ht.h"
using namespace Ht;

#define CNT 16
uint64_t arr[CNT];

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtQuUnit *pQuUnit = new CHtQuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	for (int i = 0; i < CNT; i++)
		arr[i] = i;

	pQuUnit->SendHostMsg(SU_ARRAY_ADDR, (uint64_t)&arr);

	pQuUnit->SendCall_htmain(CNT);

	// wait for return
	uint32_t rtn_elemCnt;
	while (!pQuUnit->RecvReturn_htmain(rtn_elemCnt))
		usleep(1000);

	printf("RTN: elemCnt = %d\n", rtn_elemCnt);

	// check results
	int err_cnt = 0;
	for (unsigned i = 0; i < CNT; i++) {
		if (arr[i] != i + 1)
			printf("arr[%d] is %lld, should be %d\n",
			       i, (long long)arr[i], i + 1);
	}

	delete pQuUnit;
	delete pHtHif;

	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}
