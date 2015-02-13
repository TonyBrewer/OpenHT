#include <string.h>

#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	uint64_t data[16] = {
		0xdeadbeef00001234ULL,
		0xdeadbeef00002345ULL,
		0xdeadbeef00003456ULL,
		0xdeadbeef00004567ULL
	};

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	pAuUnit->SendCall_htmain((uint64_t)&data);

	uint16_t err;
	while (!pAuUnit->RecvReturn_htmain(err))
		usleep(1000);

	delete pHtHif;

	if (err > 0)
		printf("FAILED\n");
	else
		printf("PASSED\n");

	return err;
}
