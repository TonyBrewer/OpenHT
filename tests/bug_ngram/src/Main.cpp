#include <string.h>

#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	pAuUnit->SendCall_htmain();

	while (!pAuUnit->RecvReturn_htmain())
		usleep(1000);

	delete pAuUnit;
	delete pHtHif;

	// This bug causes sysc/verilog to be unbuildable.  If the test builds/runs, the bug will have been tested
	printf("PASSED\n");

	return 0;
}
