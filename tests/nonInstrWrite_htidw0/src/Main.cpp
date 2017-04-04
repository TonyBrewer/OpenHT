#include "Ht.h"
using namespace Ht;

void usage(char *);

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	pAuUnit->SendCall_htmain();

	while (!pAuUnit->RecvReturn_htmain())
		usleep(1000);

	// This test really just tests compilation. If we got here, we passed.
	printf("PASSED\n");

	delete pHtHif;

	return 0;
}
