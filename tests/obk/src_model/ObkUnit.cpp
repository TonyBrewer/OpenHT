#include "Ht.h"
using namespace Ht;

#ifdef HT_MODEL

void Ht::HtCoprocModel(Ht::CHtHifLibBase * pHtHifLibBase)
{
	CHtModelHif *pModel = new CHtModelHif(pHtHifLibBase);
	int nau = pModel->GetUnitCnt();

	CHtModelAuUnit **pAuUnits = new CHtModelAuUnit *;

	for (int au = 0; au < nau; au += 1)
		pAuUnits[au] = new CHtModelAuUnit(pModel);

	int haltCnt = 0;
	do {
		for (int au = 0; au < nau; au += 1) {
			uint8_t arg_au;
			uint32_t sendCnt;

			if (pAuUnits[au]->RecvCall_htmain(arg_au, sendCnt)) {
				uint64_t data[256];
				for (uint64_t i = sendCnt; i > 0; ) {
					int j;
					for (j = 0; j < 256 && j < i; j += 1)
						data[j] = ((uint64_t)arg_au << 56) | (i - j);

					int sentCnt = 0;
					while (sentCnt < j) {
						int cnt = pAuUnits[au]->SendHostData(j - sentCnt, data + sentCnt);
						sentCnt += cnt;
					}

					i -= j;
				}

				while (!pAuUnits[au]->SendReturn_htmain())
					usleep(100);
			}

			if (pAuUnits[au]->RecvPoll() == eRecvHalt)
				haltCnt += 1;
		}
	} while (haltCnt < nau);

	delete pModel;
}

#endif
