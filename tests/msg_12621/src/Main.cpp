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

	pUnit->SendCall_htmain(CNT);

	// wait for return
	uint32_t rtn_elemCnt;
	while (!pUnit->RecvReturn_htmain(rtn_elemCnt))
		usleep(1);

	delete pHtHif;

	printf("RTN: elemCnt = %d\n", rtn_elemCnt);

	int err_cnt = 0;
	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}
