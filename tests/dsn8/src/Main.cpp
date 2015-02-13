#include "Ht.h"
using namespace Ht;

#define CNT 16
uint64_t arr[CNT];

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtSuUnit *pSuUnit = new CHtSuUnit(pHtHif);

	for (int i = 0; i < CNT; i++) arr[i] = i;

	uint64_t trail = 0xf;
	pSuUnit->SendCall_htmain(trail);

	// wait for return
	while (!pSuUnit->RecvReturn_htmain(trail)) {
		uint8_t msgType;
		uint64_t msgData;
		if (pSuUnit->RecvHostMsg(msgType, msgData))
			printf("Recieved Msg(%d, 0x%016llx)\n",
			       msgType, (long long)msgData);
		usleep(1000);
	}

	printf("RTN:\n");
	for (unsigned i = 0; i < 1; i++)
		printf("  [%d] = 0x%016llx\n", i, (long long)trail);

	int err_cnt = 0;
	if (trail != 0x00000f8193ac3d7eLL) {
		printf("expected one argument with value 0x00000f8193ac3d7e\n");
		err_cnt++;
	}

	delete pHtHif;

	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}
