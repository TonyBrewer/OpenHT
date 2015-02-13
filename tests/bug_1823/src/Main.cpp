#include <string.h>
#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	pAuUnit->SendCall_add(1, 2, 3);

	uint32_t rsltC, rsltP, rsltS;
	while (!pAuUnit->RecvReturn_add(rsltC, rsltP, rsltS))
		usleep(1000);

	delete pAuUnit;
	delete pHtHif;

	int err = 0;
	if (rsltC != 1+2+3 || rsltP != 1+2+3 || rsltS != 1+2+3) {
		printf("FAILED: rsltC %d, rsltP %d, rsltS %d\n", rsltC, rsltP, rsltS);
		err = 1;
	} else {
		printf("PASSED\n");
	}

	return err;
}
