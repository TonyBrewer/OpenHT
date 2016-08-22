#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	uint64_t a[8] __attribute__ ((aligned (64)));
	uint64_t b[8] __attribute__ ((aligned (64)));
	uint64_t c[8] __attribute__ ((aligned (64)));

	for (int i=0; i<8; i++) {
		a[i] = 100+i;
		b[i] = 350+i;
		c[i] = 0;
	}

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	// Send call 
	while (!pUnit->SendCall_htmain((uint64_t)&a, (uint64_t)&b, (uint64_t)&c));

	// Wait for return
	while (!pUnit->RecvReturn_htmain())
		usleep(1000);

	delete pUnit;
	delete pHtHif;

	// check results
	int error = 0;
	for (int i=0; i<8; i++) {
		if (c[0] != a[0]+b[0]) {
			printf("FAILED:  c[%d] = %llu, expected %llu\n", i, (long long)c[0], (long long)(a[0] + b[0]));
			error++;
		}
	}
	
	if (error == 0)
		printf("PASSED\n");

	return error;
}
