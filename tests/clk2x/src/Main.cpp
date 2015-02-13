#include <string.h>

#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	pAuUnit->SendHostMsg(IHM_MSG2X, 0x123);

	pAuUnit->SendCall_htmain();

	uint8_t msgType;
	uint64_t msgData;
	while (!pAuUnit->RecvHostMsg(msgType, msgData))
		usleep(1000);

	assert(msgType == OHM_MSG2X && msgData == 0x123);

	while (!pAuUnit->RecvReturn_htmain())
		usleep(1000);

	delete pHtHif;

	int err = 0;
	if (false) {
		printf("FAILED\n");
		err = 1;
	} else {
		printf("PASSED\n");
	}

	return err;
}
