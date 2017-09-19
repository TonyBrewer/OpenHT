#include "Ht.h"
using namespace Ht;
#include <errno.h>

void usage(char *);

int main(int argc, char **argv)
{
	uint64_t *array1;
	uint64_t arrayLen = 4096;
	uint64_t numReqs = 5000;

	CHtHif *pHtHif = new CHtHif();
	int unitCnt = pHtHif->GetUnitCnt();
	CHtAuUnit ** pAuUnits = new CHtAuUnit * [unitCnt];
	for (int unit = 0; unit < unitCnt; unit++)
		pAuUnits[unit] = new CHtAuUnit(pHtHif);

	fprintf(stderr, "#AUs = %d\n", unitCnt);

	//array1 = (uint64_t*)(cny_cp_malloc)(arrayLen * 8);
	array1 = (uint64_t *)(malloc)(arrayLen * 8);

	int cmd;

#define CMD_LD 0
#define CMD_ST 1

	pHtHif->SendAllHostMsg(RQ_ADDR, (uint64_t)array1);
	pHtHif->SendAllHostMsg(ARRAY_LEN, (uint64_t)arrayLen);

	for (int unit = 0; unit < unitCnt; unit++) {
		if (unit % 2 == 0)
			cmd = CMD_LD;
		else
			cmd = CMD_ST;
		fprintf(stderr, "starting unit %d: cmd=%s array1=%p\n", unit, cmd ? "ST" : "LD", array1);

		pAuUnits[unit]->SendCall_htmain(cmd, numReqs);
	}

	uint64_t rqCnt = 0;
	uint64_t hostStCnt = 0;
	for (int unit = 0; unit < unitCnt; unit++) {
		fprintf(stderr, "waiting for unit %d to complete\n", unit);
		while (!pAuUnits[unit]->RecvReturn_htmain(rqCnt))
			usleep(1000);
		printf("unit=%d:  rqCnt=%llu hostStCnt=%llu\n",
		       unit, (long long)rqCnt, (long long)hostStCnt);
	}

	delete pHtHif;

	printf("PASSED\n");
	return 0;
}
