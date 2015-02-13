#include "Ht.h"
using namespace Ht;

#ifdef HT_MODEL

class CHtModelHifEx : public CHtModelHif {
public:
	CHtModelHifEx(Ht::CHtHifLibBase * pHtHifLibBase) : CHtModelHif(pHtHifLibBase) {}
};

class CHtModelAuUnitEx : public CHtModelAuUnit {
public:
	CHtModelAuUnitEx(CHtModelHif * pHtModelHif) : CHtModelAuUnit(pHtModelHif) {}
	void UnitThread();
	void RecvCallback( Ht::ERecvType );
};

void Ht::HtCoprocModel(Ht::CHtHifLibBase * pHtHifLibBase)
{
	CHtModelHifEx *pModelHif = new CHtModelHifEx(pHtHifLibBase);

	pModelHif->StartUnitThreads<CHtModelAuUnitEx>();

	pModelHif->WaitForUnitThreads();

	delete pModelHif;
}

void CHtModelAuUnitEx::UnitThread()
{
	SetUsingCallback(true);

	uint64_t data[5];
	int popCnt = 5;
	int idx = 0;

	while (idx < TEST_CNT) {
		// read all available data
		int cnt = min(popCnt, TEST_CNT - idx);

		// receive data from host
		int list[500];
		int listIdx = 0;
		int rcvdCnt = 0;
		while (rcvdCnt < cnt) {
			rcvdCnt += list[listIdx] = RecvHostData(cnt - rcvdCnt, data + rcvdCnt);
			if (listIdx < 499) listIdx += 1;
		}

		for (int j = 0; j < cnt; j += 1)
			assert(data[j] == (idx + j) * 1);

		// echo data back
		int sentCnt = 0;
		while (sentCnt < cnt) {
			sentCnt += SendHostData(cnt - sentCnt, data + sentCnt);
			if (sentCnt < cnt)
				// wait until output queue is not full
				usleep(1);
		}

		idx += cnt;
		if (--popCnt == 0)
			popCnt = 5;
	}

	FlushHostData();
}

void CHtModelAuUnitEx::RecvCallback(Ht::ERecvType recvType)
{
	switch (recvType) {
	case Ht::eRecvHostMsg:
	{
		uint8_t msgType;
		uint64_t msgData;

		RecvHostMsg(msgType, msgData);

		SendHostMsg(msgType, msgData);
	}
	break;
	case Ht::eRecvCall:
	{
		uint64_t in1, in2, in3, in4;

		RecvCall_htmain(in1, in2, in3, in4);

		SendReturn_htmain(in1, in2, in3, in4);
	}
	break;
	default:
		assert(0);
	}
}

#endif
