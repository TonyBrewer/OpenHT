#include "Ht.h"
using namespace Ht;

#define LOOP_CNT 256

int main(int argc, char **argv)
{
	printf("%s\n", argv[0]);

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	int callCnt = 16;
	int rtnCnt = 0;

	uint16_t rtn_errCnt;
	int errCnt = 0;

	for (int i = 0; i < callCnt; i += 1) {
		while (!pUnit->SendCall_htmain(LOOP_CNT));

		if (pUnit->RecvReturn_htmain(rtn_errCnt)) {
			errCnt += rtn_errCnt;
			rtnCnt += 1;
		}
	}

	// wait for return
	while (rtnCnt < callCnt) {
		while (!pUnit->RecvReturn_htmain(rtn_errCnt))
			usleep(1);

		errCnt += rtn_errCnt;
		rtnCnt += 1;
	}

	delete pHtHif;

	if (errCnt > 0)
		printf("FAILED: detected %d issues!\n", errCnt);
	else
		printf("PASSED\n");

	return errCnt;
}
