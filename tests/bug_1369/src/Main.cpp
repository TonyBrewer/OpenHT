#include "Ht.h"
#include <errno.h>

void usage(char *);

int main(int argc, char **argv)
{
	uint64_t *array1;
	uint64_t arrayLen = 4096;
	uint64_t numReqs = 5000;

	CAuHif *m_pAuHif = new CAuHif();

	int unitCnt = m_pAuHif->unitCnt();

	fprintf(stderr, "#AUs = %d\n", unitCnt);

	//array1 = (uint64_t*)(cny_cp_malloc)(arrayLen * 8);
	array1 = (uint64_t *)(malloc)(arrayLen * 8);

	int cmd;

#define CMD_LD 0
#define CMD_ST 1
	for (int unit = 0; unit < unitCnt; unit++) {
		if (unit % 2 == 0)
			cmd = CMD_LD;
		else
			cmd = CMD_ST;
		fprintf(stderr, "starting unit %d: cmd=%s array1=%p\n", unit, cmd ? "ST" : "LD", array1);
		m_pAuHif->SendHostMsg(unit, RQ_ADDR, (uint64_t)array1);
		m_pAuHif->SendHostMsg(unit, ARRAY_LEN, (uint64_t)arrayLen);

		m_pAuHif->AsyncCall_htmain(unit, cmd, numReqs);
	}

	uint64_t rqCnt = 0;
	uint64_t hostStCnt = 0;
	for (int unit = 0; unit < unitCnt; unit++) {
		fprintf(stderr, "waiting for unit %d to complete\n", unit);
		while (!m_pAuHif->Return_htmain(unit, rqCnt))
			HtSleep(100);
		printf("unit=%d:  rqCnt=%llu hostStCnt=%llu\n",
		       unit, (long long)rqCnt, (long long)hostStCnt);
	}

	delete m_pAuHif;

	printf("PASSED\n");
	return 0;
}
