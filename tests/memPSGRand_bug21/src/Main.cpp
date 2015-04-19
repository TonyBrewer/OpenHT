#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	uint64_t data[32*4]; // Max 32 Cycles Per Test

	pUnit->SendCall_main(data);

	while (!pUnit->RecvReturn_main())
		usleep(1);

	delete pUnit;
	delete pHtHif;

	printf("PASSED\n");

	return 0;
}
