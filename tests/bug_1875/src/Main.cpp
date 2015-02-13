#include <string.h>
#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	pAuUnit->SendCall_bug();

	while (!pAuUnit->RecvReturn_bug())
		usleep(1000);

	delete pAuUnit;
	delete pHtHif;

	printf("PASSED\n");

	return 0;
}
