#include <string.h>

#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	printf("%s\n", argv[0]);

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	uint8_t mem[6] = { 8, 9, 10, 11, 12, 13 };

	while (!pAuUnit->SendCall_htmain(mem));

	while (!pAuUnit->RecvReturn_htmain())
		usleep(1000);

	delete pHtHif;

	printf("PASSED\n");

	return 0;
}
