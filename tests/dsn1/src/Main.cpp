#include "Ht.h"
using namespace Ht;

#define CNT 16
uint64_t arr[CNT];

int main(int argc, char **argv)
{
	for (int i = 0; i < CNT; i++)
		arr[i] = i;

	CHtHif *pHtHif;
	try {
		pHtHif = new CHtHif();
	}
	catch (CHtException &htException) {
		printf("new CHtHif threw an exception: '%s'\n", htException.GetMsg().c_str());
		exit(1);
	}

	CHtSuUnit *pUnit = new CHtSuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	pHtHif->SendAllHostMsg(SU_ARRAY_ADDR, (uint64_t)&arr);
	//pUnit->SendHostMsg(SU_ARRAY_ADDR, (uint64_t)&arr);

	pUnit->SendCall_htmain(CNT);

	// wait for return
	uint32_t rtn_elemCnt;
	while (!pUnit->RecvReturn_htmain(rtn_elemCnt))
		usleep(1);

	delete pUnit;
	delete pHtHif;

	printf("RTN: elemCnt = %d\n", rtn_elemCnt);

	// check results
	int err_cnt = 0;
	for (unsigned i = 0; i < CNT; i++) {
		if (arr[i] != i + 1) {
			printf("arr[%d] is %lld, should be %d\n",
			       i, (long long)arr[i], i + 1);
			err_cnt++;
		}
	}

	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}
