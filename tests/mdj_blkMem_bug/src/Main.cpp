#include "Ht.h"
using namespace Ht;

void usage(char *);

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	uint64_t *var;
	var = (uint64_t*)malloc(8*(1<<11)*sizeof(uint64_t));

	pHtHif->SendAllHostMsg(ADDR, (uint64_t)var);

	pAuUnit->SendCall_htmain();

	while (!pAuUnit->RecvReturn_htmain())
		usleep(1000);

	int errCnt = 0;
	for (uint64_t i = 0; i < 250*8; i++) {
		if (var[i] != i+1) {
			printf("ERROR! %ld -> Act=%ld, Exp=%ld\n", i, var[i], i+1);
			errCnt++;
		}
	}

	if (errCnt) {
		printf("FAILED - %d errors\n", errCnt);
	} else {
		printf("PASSED\n");
	}
	fflush(stdout);

	free(var);

	delete pHtHif;

	return 0;
}
