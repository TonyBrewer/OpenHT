#include "Ht.h"
using namespace Ht;

#define MAX_UNITS 64

int main(int argc, char **argv)
{
	uint64_t data[MAX_UNITS];
	
	for (int unit = 0; unit < MAX_UNITS; unit++)
		data[unit] = (uint64_t)unit;

	CHtHif *pHtHif = new CHtHif();
	int unitCnt = pHtHif->GetUnitCnt();
	CHtAuUnit ** pAuUnits = new CHtAuUnit * [unitCnt];

	for (int unitId = 0; unitId < unitCnt; unitId += 1)
		pAuUnits[unitId] = new CHtAuUnit(pHtHif);

	// Send calls to units
	for (int unit = 0; unit < unitCnt; unit++)
		pAuUnits[unit]->SendCall_htmain(&data[unit]);

	// Wait for returns
	for (int unit = 0; unit < unitCnt; unit++) {
		while (!pAuUnits[unit]->RecvReturn_htmain())
			usleep(1000);
	}

	delete pHtHif;

	// check results
	int err_cnt = 0;
	for (int unit = 0; unit < unitCnt; unit++) {
		if (data[unit] != ~(uint64_t)unit) {
			printf("data[%d] is %llu, should be %llu\n",
			       unit, (long long)data[unit], ~(long long)unit);
			err_cnt++;
		}
	}

	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}
