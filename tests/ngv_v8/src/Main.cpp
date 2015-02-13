#include "Ht.h"
using namespace Ht;

#include <ArgStruct.h>

#define CNT 16
uint64_t arr[CNT];

#define CALL_RTN_CNT 3

int main(int argc, char **argv)
{
	for (int i = 0; i < CNT; i++)
		arr[i] = i;

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	ArgStruct argStruct;
	argStruct.a = 1;
	argStruct.b = 2;
	argStruct.c = 3;
	argStruct.d = 4;
	argStruct.e = true;

	int callCnt = 0;
	int rtnCnt = 0;
	while (callCnt < CALL_RTN_CNT || rtnCnt < CALL_RTN_CNT) {
		if (callCnt < CALL_RTN_CNT && pUnit->SendCall_htmain(argStruct, true))
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
