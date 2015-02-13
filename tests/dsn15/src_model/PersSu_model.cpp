#include "Ht.h"
#ifdef HT_MODEL

void HtCoprocCallback(CHtModelSuUnit *, CHtModelSuUnit::ERecvType);

void HtCoprocModel()
{
	CHtModelHif *pModel = new CHtModelHif;
	CHtModelSuUnit *pUnit = pModel->AllocSuUnit();

	pUnit->SetCallback(&HtCoprocCallback);

	uint64_t data[5];
	int popCnt = 5;
	int idx = 0;

	for (;; ) {
		if (pUnit->RecvHostDataMarker()) {
			while (!pUnit->SendHostDataMarker()) ;

			while (!pUnit->SendReturn_echo(1)) ;
		} else {
			int recvCnt = pUnit->RecvHostData(5, data);
			int sendCnt = 0;
			while (sendCnt < recvCnt)
				sendCnt += pUnit->SendHostData(recvCnt - sendCnt, data + sendCnt);
		}

		if (pUnit->RecvPoll(true, true, false) == CHtModelSuUnit::eRecvHalt)
			break;
	}

	pModel->FreeSuUnit(pUnit);
	delete pModel;
}

void HtCoprocCallback(CHtModelSuUnit *pSuUnit, CHtModelSuUnit::ERecvType recvType)
{
	switch (recvType) {
	case CHtModelSuUnit::eRecvHostMsg:
	{
		uint8_t msgType;
		uint64_t msgData;

		pSuUnit->RecvHostMsg(msgType, msgData);
	}
	break;
	case CHtModelSuUnit::eRecvCall:
	{
		uint16_t cnt;

		pSuUnit->RecvCall_echo(cnt);
	}
	break;
	default:
		assert(0);
	}
}

#endif
