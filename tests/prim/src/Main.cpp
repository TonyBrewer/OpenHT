#include <string.h>

#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	pAuUnit->SendCall_htmain(123, 456);

	uint16_t r1, r2;
	while (!pAuUnit->RecvReturn_htmain(r1, r2))
		usleep(1000);

	delete pHtHif;

	int err = 0;
	if (r1 != 579 || r2 != 579) {
		printf("FAILED %d  %d\n", r1, r2);
		err = 1;
	} else {
		printf("PASSED\n");
	}

	return err;
}
