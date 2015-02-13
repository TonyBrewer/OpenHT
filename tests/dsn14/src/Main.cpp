#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtSuUnit *pSuUnit = new CHtSuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	pSuUnit->SendCall_htmain();

	// wait for return
	while (!pSuUnit->RecvReturn_htmain())
		usleep(1000);

	delete pHtHif;

	printf("PASSED\n");

	return 0;
}
