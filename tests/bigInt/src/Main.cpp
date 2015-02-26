#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	printf("BigInt\n");

	CHtHif *pHtHif;
	try {
		pHtHif = new CHtHif();
	}
	catch (CHtException &htException) {
		printf("new CHtHif threw an exception: '%s'\n", htException.GetMsg().c_str());
		exit(1);
	}

	CHtSuUnit *pUnit = new CHtSuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	pUnit->SendCall_htmain();

	// wait for return
	bool err;
	while (!pUnit->RecvReturn_htmain(err))
		usleep(1);

	delete pUnit;
	delete pHtHif;

	if (err)
		printf("FAILED\n");
	else
		printf("PASSED\n");

	return err ? 1 : 0;
}
