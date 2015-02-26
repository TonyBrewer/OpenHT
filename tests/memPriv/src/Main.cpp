#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	uint64_t data[4];

	pUnit->SendCall_main(data);

	bool bErr;
	while (!pUnit->RecvReturn_main(bErr))
		usleep(1);

	delete pHtHif;

	if (bErr)
		printf("FAILED\n");
	else
		printf("PASSED\n");

	return bErr ? 1 : 0;
}
