#include "Ht.h"
using namespace Ht;

#define LOOP_CNT 100

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	pUnit->SendCall_htmain(LOOP_CNT);

	// wait for return
	uint16_t rtn_errorCnt;
	while (!pUnit->RecvReturn_htmain(rtn_errorCnt))
		usleep(1);

	delete pHtHif;

	printf("RTN: errorCnt = %d\n", rtn_errorCnt);

	if (rtn_errorCnt > 0)
		printf("FAILED: detected %d issues!\n", rtn_errorCnt);
	else
		printf("PASSED\n");

	return rtn_errorCnt;
}
