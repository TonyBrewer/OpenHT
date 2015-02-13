#include "Ht.h"
using namespace Ht;

void Ht::HtCoprocModel(Ht::CHtHifLibBase * pHtHifLibBase)
{
	CHtModelHif *pModel = new CHtModelHif(pHtHifLibBase);
	int nau = pModel->GetUnitCnt();


	CHtModelAuUnit ** pAuUnits = new CHtModelAuUnit *;

	for (int i = 0; i < nau; i += 1)
		pAuUnits[i] = new CHtModelAuUnit(pModel);

	uint8_t msgType;
	uint64_t msgData;
	uint32_t vecIdx, stride;
	uint64_t *op1Addr = NULL, *op2Addr = NULL, *resAddr = NULL;
	uint64_t vecLen = 0;
	uint64_t op1, op2, res, sum;

	printf("nau = %d\n", nau);

	int haltCnt = 0;
	while (haltCnt < nau) {
		for (int au = 0; au < nau; au += 1) {
			if (pAuUnits[au]->RecvHostMsg(msgType, msgData)) {
				switch (msgType) {
				case OP1_ADDR: op1Addr = (uint64_t *)msgData; break;
				case OP2_ADDR: op2Addr = (uint64_t *)msgData; break;
				case RES_ADDR: resAddr = (uint64_t *)msgData; break;
				case VEC_LEN:  vecLen = msgData; break;
				default: assert(0);
				}
			}
			if (pAuUnits[au]->RecvCall_htmain(vecIdx, stride)) {
				sum = 0;
				while (vecIdx < vecLen) {
					op1 = *(op1Addr + vecIdx);
					op2 = *(op2Addr + vecIdx);
					res = op1 + op2;
					sum += res;
					*(resAddr + vecIdx) = res;

					//printf("op1=%lld op2=%lld\n",
					//	(long long unsigned)op1,
					//	(long long unsigned)op2);

					vecIdx += stride;
				}

				while (!pAuUnits[au]->SendReturn_htmain(sum)) ;
			}
			haltCnt += pAuUnits[au]->RecvHostHalt();
		}
	}

	delete pModel;
}
