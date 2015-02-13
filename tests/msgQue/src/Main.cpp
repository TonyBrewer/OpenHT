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

	pUnit->SendCall_htmain();

	// wait for return
	bool error;
	while (!pUnit->RecvReturn_htmain(error))
		usleep(1);

	delete pHtHif;

	if (error)
		printf("FAILED: detected %d issues!\n", 1);
	else
		printf("PASSED\n");

	return error;
}
