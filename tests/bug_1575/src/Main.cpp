#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv) {

	uint32_t i = 0;
	uint64_t *a1;
	uint32_t listLen = 10;
	uint32_t sum = 0;

	printf("Running private variable state test.\n");
	printf("Checking if private variables updated after a SendCallFork_x async thread returns retain their values.\n\n");
	fflush(stdout);

	a1 = (uint64_t *)malloc(listLen * 8);

	for (i = 0; i < listLen; i++) {
		a1[i] = i;
	}

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	// Coprocessor memory arrays
	uint64_t *cp_a1 = (uint64_t*)pHtHif->MemAllocAlign(4 * 1024, listLen * sizeof(uint64_t));
	pHtHif->MemCpy(cp_a1, a1, listLen * sizeof(uint64_t));

	pAuUnit->SendCall_htmain(listLen);

	while (!pAuUnit->RecvReturn_htmain(sum)) {
		usleep(1000);
	}

	// free memory
	free(a1);
	pHtHif->MemFreeAlign(cp_a1);

	delete pAuUnit;
	delete pHtHif;

	uint32_t sumCheck = 0;
	for (i = 0; i < listLen; i++) {
		sumCheck += i;
	}

	if (sum == sumCheck) {
		printf("PASSED\n");
	} else {
		printf("FAILED - Expected %ld, Received %ld\n", (unsigned long int) sumCheck, (unsigned long int) sum);
	}

	fflush(stdout);

	return 0;
}
