#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	uint64_t a, b, c;

	a = 100;
	b = 350;

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
	if (c != a+b) 
		printf("FAILED:  c = %llu, expected %llu\n", (long long)c, (long long)(a + b));
	else
		printf("PASSED:  c = %llu\n", (long long)c);

	return error;
}
