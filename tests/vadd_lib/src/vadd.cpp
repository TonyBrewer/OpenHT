#include "Ht.h"
using namespace Ht;

uint64_t vadd(uint64_t *a1, uint64_t *a2, uint64_t *a3, uint64_t vecLen)
{
	CHtHif *pHtHif = new CHtHif();

	int unitCnt = pHtHif->GetUnitCnt();
	CHtAuUnit ** pAuUnits = new CHtAuUnit * [unitCnt];

	for (int unitId = 0; unitId < unitCnt; unitId += 1)
		pAuUnits[unitId] = new CHtAuUnit(pHtHif);

	int unit = 0;

	pHtHif->SendAllHostMsg(OP1_ADDR, (uint64_t)a1);
	pHtHif->SendAllHostMsg(OP2_ADDR, (uint64_t)a2);
	pHtHif->SendAllHostMsg(RES_ADDR, (uint64_t)a3);
	pHtHif->SendAllHostMsg(VEC_LEN, (uint64_t)vecLen);

	for (unit = 0; unit < unitCnt; unit++) {
		pAuUnits[unit]->SendCall_htmain(unit /*offset*/, unitCnt /*stride*/);
	}

	uint64_t sum = 0;
	uint64_t au_sum;
	for (int unit = 0; unit < unitCnt; unit++) {
		while (!pAuUnits[unit]->RecvReturn_htmain(au_sum))
			usleep(1000);
		printf("unit=%-2d: au_sum %llu \n", unit, (long long)au_sum);
		fflush(stdout);
		sum += au_sum;
	}

	printf("RTN: sum = %llu\n", (long long) sum);

	delete pHtHif;

	return sum;
}

