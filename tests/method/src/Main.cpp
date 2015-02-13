#include <string.h>

#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	pAuUnit->SendCall_htmain();

	int32_t err;
	while (!pAuUnit->RecvReturn_htmain(err))
		usleep(1000);

	delete pHtHif;

	if (err)
		printf("FAILED (err=0x%x)\n", err);
	else
		printf("PASSED\n");

	return err;
}
