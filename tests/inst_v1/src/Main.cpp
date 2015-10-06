#include <string.h>

#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	printf("%s\n", argv[0]);

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	pAuUnit->SendCall_htmain();

	while (!pAuUnit->RecvReturn_htmain())
		usleep(1000);

	delete pHtHif;

	printf("PASSED\n");

	return 0;
}
