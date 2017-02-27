#include "Ht.h"
using namespace Ht;

void usage(char *);

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	uint64_t var = 0;

	pHtHif->SendAllHostMsg(ADDR, (uint64_t)&var);

	pAuUnit->SendCall_htmain();

	while (!pAuUnit->RecvReturn_htmain())
		usleep(1000);

	if (var == 55) {
	  printf("PASSED\n");
	} else {
	  printf("FAILED (%ld)\n", var);
	}

	delete pHtHif;

	return 0;
}
