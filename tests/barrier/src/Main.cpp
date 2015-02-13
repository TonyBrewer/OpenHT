#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	pUnit->SendCall_htmain();

	// wait for return
	uint32_t err_cnt;
	while (!pUnit->RecvReturn_htmain(err_cnt))
		usleep(1);

	delete pUnit;
	delete pHtHif;

	if (err_cnt > 0)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}
