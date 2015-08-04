#include <string.h>

#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	printf("Test: memRdRspFunc\n");

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	uint64_t data[256+32];
	uint64_t rslt = 0;
	uint64_t rslt64Dly = 0;

	for (int i = 0; i < 256; i += 1) {
		data[i] = 123 + i;
		rslt += data[i] * i;

		if (i < 32)
			rslt64Dly += data[i] * i;
	}

	uint32_t * data32 = (uint32_t *)&data[256];
	uint32_t rslt32 = 0;

	for (int i = 0; i < 64; i += 1) {
		data32[i] = 127 + i;
		rslt32 += data32[i] * i;
	}

	pAuUnit->SendCall_htmain(data);

	while (!pAuUnit->RecvReturn_htmain())
		usleep(1000);

	delete pHtHif;

	printf("PASSED\n");

	return 0;
}
