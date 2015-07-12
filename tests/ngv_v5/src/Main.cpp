#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	printf("ngv_v5\n");
	fflush(stdout);
		
	uint64_t data[4];
	pUnit->SendCall_htmain(data);

	while (!pUnit->RecvReturn_htmain())
		usleep(1);

	delete pUnit;
	delete pHtHif;

	printf("PASSED\n");

	return 0;
}
