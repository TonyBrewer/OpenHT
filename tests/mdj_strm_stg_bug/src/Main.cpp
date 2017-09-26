#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	// Send calls to units
	pAuUnit->SendCall_htmain();

	// Wait for return
	bool passed;
	while (!pAuUnit->RecvReturn_htmain(passed))
		usleep(1000);

	if (passed)
		printf("PASSED\n");
	else
		printf("FAILED\n");

	delete pHtHif;

	return (passed) ? 0 : 1;
}
