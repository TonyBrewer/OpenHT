#include "Ht.h"
using namespace Ht;

#define CNT 16
uint64_t arr[CNT];
#define THREADS 8

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtSuUnit *pSuUnit = new CHtSuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	for (int i = 0; i < CNT; i++)
		arr[i] = i;


	for (int i = 0; i < THREADS; i += 1)
		while (!pSuUnit->SendCall_htmain((uint64_t)&arr)) ;

	// wait for return
	uint32_t err;
	uint32_t errCnt = 0;

	for (int i = 0; i < THREADS; i += 1) {
		while (!pSuUnit->RecvReturn_htmain(err))
			usleep(1000);
		errCnt += err;
	}

	delete pHtHif;

	if (err)
		printf("FAILED: detected %d issues!\n", err);
	else
		printf("PASSED\n");

	return err;
}
