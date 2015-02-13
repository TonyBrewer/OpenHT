#include <string.h>

#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	char buf[16];

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	pAuUnit->SendCall_htmain((uint64_t)&buf);

	while (!pAuUnit->RecvReturn_htmain())
		usleep(1000);

	delete pHtHif;

	int err = 0;
	if (strcmp(buf, "Hello")) {
		printf("FAILED: %s != Hello!\n", buf);
		err = 1;
	} else {
		printf("PASSED\n");
	}

	return err;
}
