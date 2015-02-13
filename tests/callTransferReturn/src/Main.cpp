#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	// Only supports AEUCNT = 1 in this design (otherwise alloc the others)

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	// Declare and send data to Coprocessor
	uint64_t testData = 0xa;
	pAuUnit->SendCall_htmain(testData);

	// Wait for return
	while (!pAuUnit->RecvReturn_htmain(testData)) {
		usleep(1000);
	}

	// Check data to make sure we get our expected 0xabcdef back
	int err_cnt = 0;
	if (testData != 0x0000000000abcdefLL) {
		printf("Expected return data: 0x0000000000abcdef\n");
		printf("Actual return data  : 0x%016llx\n\n", (unsigned long long)testData);
		err_cnt++;
	}

	delete pAuUnit;
	delete pHtHif;

	if (err_cnt) {
		printf("FAILED\n");
	} else {
		printf("PASSED\n");
	}

	return err_cnt;
}
