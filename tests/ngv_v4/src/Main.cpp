#include "Ht.h"
using namespace Ht;

#define CNT 16
uint64_t arr[CNT];

int main(int argc, char **argv)
{
	for (int i = 0; i < CNT; i++)
		arr[i] = i;

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	int callCnt = 0;
	int rtnCnt = 0;
	while (callCnt < 5 || rtnCnt < 5) {
		if (callCnt < 5 && pUnit->SendCall_htmain())
			callCnt += 1;

		if (rtnCnt < 5 && pUnit->RecvReturn_htmain())
			rtnCnt += 1;

		usleep(1);
	}

	delete pUnit;
	delete pHtHif;

	int err_cnt = 0;
	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}
