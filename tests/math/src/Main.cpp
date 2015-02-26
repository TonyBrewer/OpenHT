#include <string.h>

#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	pAuUnit->SendCall_htmain();

	uint64_t errMask;
	while (!pAuUnit->RecvReturn_htmain(errMask))
		usleep(1000);

	delete pHtHif;

	if (errMask != 0)
		printf("FAILED (errMask=0x%llx)\n", (unsigned long long)errMask);
	else
		printf("PASSED\n");

	return errMask != 0 ? 1 : 0;
}
