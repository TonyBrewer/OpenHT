#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtSuUnit *pSuUnit = new CHtSuUnit(pHtHif);

	pSuUnit->SendCall_cxr1();

	// wait for return
	while (!pSuUnit->RecvReturn_cxr1()) usleep(1000);

	delete pHtHif;

	printf("PASSED\n");

	return 0;
}
