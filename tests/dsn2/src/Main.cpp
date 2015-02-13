#include "Ht.h"
using namespace Ht;

#define CNT 16

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtSuUnit *pSuUnit = new CHtSuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	// initialize memory array
	uint64_t memData[CNT];
	for (int i = 0; i < CNT; i++)
		memData[i] = i;

	uint64_t inData[CNT];
	for (int i = 0; i < CNT; i += 1)
		inData[i] = i << 16;

	uint64_t outData[CNT];

	pSuUnit->SendHostMsg(SU_ARRAY_ADDR, (uint64_t)&memData);

	pSuUnit->SendCall_htmain();

	pSuUnit->SendHostData(CNT, inData);

	pSuUnit->SendHostDataMarker();

	// Coproc echos sends back

	int recvCnt = 0;
	int returnCnt = 0;
	int markerCnt = 0;

	while (recvCnt < CNT || returnCnt < 1 || markerCnt < 1) {
		switch (pSuUnit->RecvPoll()) {
		case Ht::eRecvHostData:
			recvCnt += pSuUnit->RecvHostData(CNT - recvCnt, outData + recvCnt);
			break;
		case Ht::eRecvHostDataMarker:
			pSuUnit->RecvHostDataMarker();
			markerCnt += 1;
			break;
		case Ht::eRecvReturn:
			uint16_t loopCnt;
			pSuUnit->RecvReturn_htmain(loopCnt);
			returnCnt += 1;
			break;
		case Ht::eRecvIdle:
			usleep(1000);
			break;
		default:
			assert(0);
		}
	}

	delete pHtHif;

	int errCnt = 0;
	if (recvCnt != CNT || returnCnt != 1 || markerCnt != 1)
		errCnt += 1;

	for (unsigned i = 0; i < CNT; i += 1) {
		if ((outData[i] & 0xffff) != i || ((outData[i] >> 16) & 0xffff) != i) {
			printf("ERROR: miscompare, expected 0x%04x%04x, actual 0x%08llx\n",
			       i, i, (long long)outData[i]);
			errCnt += 1;
		}
	}

	if (errCnt)
		printf("FAILED: detected %d issues!\n", errCnt);
	else
		printf("PASSED\n");

	return errCnt;
}
