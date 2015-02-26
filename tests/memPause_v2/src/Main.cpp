#include "Ht.h"
using namespace Ht;

#define CNT 16
uint64_t arr[CNT];

#define TH 8
#define TESTS   8

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtSuUnit *pSuUnit = new CHtSuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	for (int i = 0; i < CNT; i++)
		arr[i] = i;

	uint32_t err = 0;
	for (int tst = 0; tst < TESTS; tst += 1) {
		for (int th = 0; th < TH; th += 1)
			while(!pSuUnit->SendCall_htmain((uint64_t)&arr, tst));

		// wait for return
		uint32_t thErr;
		for (int th = 0; th < TH; th += 1) {
			while (!pSuUnit->RecvReturn_htmain(thErr))
				usleep(1000);

			err += thErr;
		}
	}
	delete pHtHif;

	if (err)
		printf("\nFAILED: detected %d issues!\n", err);
	else
		printf("\nPASSED\n");

	return err;
}
