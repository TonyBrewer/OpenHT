#include "Ht.h"
using namespace Ht;

#define TEST_CNT 16
#define ARR_SIZE        64

class CHtSuUnitEx : public CHtSuUnit {
public:
	CHtSuUnitEx(CHtHif * pHtHif) : CHtSuUnit(pHtHif) {}
	void RecvCallback( Ht::ERecvType );
};

uint64_t memArr[ARR_SIZE];
uint64_t sendArr[ARR_SIZE];
uint64_t recvArr[ARR_SIZE];

int recvDataCnt = 0;
int markerCnt = 0;
uint16_t rtnCnt = 0;
CQueue<int, 64> dataCntQue;

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtSuUnitEx *pSuUnit = new CHtSuUnitEx(pHtHif);

	pSuUnit->SetUsingCallback(true);

	int err_cnt = 0;

	uint16_t callCnt = 0;
	int sendDataCnt = 0;

	for (int i = 0; i < ARR_SIZE; i++) {
		memArr[i] = 0x0300 | i;
		sendArr[i] = 0x04000000 | (i << 16);
	}

	pSuUnit->SendHostMsg(SU_ARRAY_ADDR, (uint64_t)&memArr);

	while (callCnt < TEST_CNT || markerCnt < TEST_CNT || rtnCnt < TEST_CNT) {
		if (callCnt < TEST_CNT && sendDataCnt == 0 && pSuUnit->SendCall_echo(callCnt)) {
			sendDataCnt = callCnt++ + 1;
			dataCntQue.Push(sendDataCnt);
			continue;
		}

		if (sendDataCnt > 1) {
			sendDataCnt -= pSuUnit->SendHostData(sendDataCnt - 1, sendArr);
			continue;
		}

		if (sendDataCnt == 1 && pSuUnit->SendHostDataMarker()) {
			sendDataCnt = 0;
			continue;
		}

		if (pSuUnit->RecvPoll() == Ht::eRecvIdle)
			usleep(1000);
	}

	delete pHtHif;

	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}

void CHtSuUnitEx::RecvCallback(Ht::ERecvType recvType)
{
	switch (recvType) {
	case Ht::eRecvHostData:
	{
		if (recvDataCnt == 0) {
			assert(!dataCntQue.Empty());
			recvDataCnt = dataCntQue.Front();
			dataCntQue.Pop();
			printf("eRecvHostData - recvDataCnt = %d\n", recvDataCnt);
		}

		assert(recvDataCnt > 1);
		recvDataCnt -= RecvHostData(recvDataCnt - 1, recvArr);
	}
	break;
	case Ht::eRecvHostDataMarker:
	{
		if (recvDataCnt == 0) {
			assert(!dataCntQue.Empty());
			recvDataCnt = dataCntQue.Front();
			dataCntQue.Pop();
			printf("eRecvDataMarker - recvDataCnt = %d\n", recvDataCnt);
		}

		assert(recvDataCnt == 1);
		bool bMarker = RecvHostDataMarker();
		assert(bMarker);
		recvDataCnt = 0;
		markerCnt += 1;
	}
	break;
	case Ht::eRecvReturn:
	{
		uint16_t recvDataCnt;
		RecvReturn_echo(recvDataCnt);
		rtnCnt += 1;
	}
	break;
	default:
		assert(0);
	}
}
