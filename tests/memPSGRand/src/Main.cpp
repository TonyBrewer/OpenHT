#include "Ht.h"
using namespace Ht;

#define LOOPCNT 8

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	uint64_t data[(32*8)+LOOPCNT]; // Max 32 Cycles Per Test

	for (uint i = 0; i < LOOPCNT; i++) {

		printf("Running Loop %d of %d...", i+1, LOOPCNT);

		while (!pUnit->SendCall_main(&data[i]))
			usleep(1000);

		while (!pUnit->RecvReturn_main())
			usleep(1000);

		printf("OK\n");

	}

	delete pUnit;
	delete pHtHif;

	printf("PASSED\n");

	return 0;
}
