#include "Ht.h"
using namespace Ht;

#define CNT 16
uint64_t arr[CNT];

int main(int argc, char **argv)
{
	for (int i = 0; i < CNT; i++)
		arr[i] = i;

	CHtHif *pHtHif = new CHtHif();

	int auCnt = pHtHif->GetUnitCnt();
	printf("#AUs = %d\n", auCnt);
	fflush(stdout);

	CHtAuUnit **pUnits = new CHtAuUnit * [auCnt];

	for (int i = 0; i < auCnt; i += 1)
		pUnits[i] = new CHtAuUnit(pHtHif);

	for (int i = 0; i < auCnt; i += 1)
		while (!pUnits[i]->SendCall_htmain())
			usleep(1);

	// wait for return
	for (int i = 0; i < auCnt; i += 1)
		while (!pUnits[i]->RecvReturn_htmain())
			usleep(1);

	delete pHtHif;

	printf("PASSED\n");

	return 0;
}
