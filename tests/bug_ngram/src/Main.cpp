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

	return 0;
}
