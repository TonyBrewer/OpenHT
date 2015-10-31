#include "Ht.h"
using namespace Ht;

#define CNT 16
uint64_t arr[CNT];

int main(int argc, char **argv)
{
	for (int i = 0; i < CNT; i++)
		arr[i] = i;

	CHtHif *pHtHif = new CHtHif();

	int unitCnt = pHtHif->GetUnitCnt();
	printf("#AUs = %d\n", unitCnt);
	fflush(stdout);

	CHtAuUnit **pUnits = new CHtAuUnit* [unitCnt];
	for (int i = 0; i < unitCnt; i += 1)
		pUnits[i] = new CHtAuUnit(pHtHif);

	int recvCnt = 0;
	int sendCnt = 0;
	do {
		if (sendCnt < unitCnt && pUnits[sendCnt]->SendCall_htmain(CNT))
			sendCnt += 1;

		// wait for return
		uint32_t rtn_elemCnt;
		if (pUnits[recvCnt]->RecvReturn_htmain(rtn_elemCnt))
			recvCnt += 1;

	} while (recvCnt < unitCnt);

	delete pHtHif;

	int err_cnt = 0;
	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}
