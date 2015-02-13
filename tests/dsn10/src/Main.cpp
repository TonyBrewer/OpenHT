#include "Ht.h"
using namespace Ht;

#define CNT 16
uint64_t arr[CNT];

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtSuUnit *pSuUnit = new CHtSuUnit(pHtHif);

	for (int i = 0; i < CNT; i++) arr[i] = i;

	int err_cnt = 0;
	for (int testCnt = 0; testCnt < 50; testCnt += 1) {

		uint64_t callTrail = 0xf;
		pSuUnit->SendCall_htmain(callTrail);

		// wait for return
		uint64_t rtnTrail;
		while (!pSuUnit->RecvReturn_htmain(rtnTrail)) {
			uint8_t msgType;
			uint64_t msgData;
			if (pSuUnit->RecvHostMsg(msgType, msgData))
				printf("Recieved Msg(%d, 0x%016llx)\n",
					   msgType, (long long)msgData);
			usleep(1000);
		}

		if (rtnTrail != 0x00000f8193ac3d7eLL) {
			printf("Loop failed - expected one argument with value 0x00000f8193ac3d7e\n");
			err_cnt++;
		}
	}

	delete pSuUnit;
	delete pHtHif;

	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}
