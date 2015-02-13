#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	for (int i = 0; i < 8; i += 1) {
		while (!pAuUnit->SendCall_htmain());
	}

	// wait for return
	int errCnt = 0;
	for (int i = 0; i < 8; i += 1) {
		bool bError;
		while (!pAuUnit->RecvReturn_htmain(bError))
			usleep(1000);

		if (bError)
			errCnt += 1;
	}

	delete pHtHif;

	if (errCnt > 0)
		printf("FAILED: detected %d issues!\n", errCnt);
	else
		printf("PASSED\n");

	return 0;
}
