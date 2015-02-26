#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	uint64_t *arr;
	ht_posix_memalign((void **)&arr, 64, sizeof(uint64_t) * 8);

	uint64_t act_sum = 0;
	uint64_t exp_sum = 0;
	uint64_t i;
	for (i = 0; i < 8; i++) {
		arr[i] = i;
		exp_sum += i;
	}

	// Alloc Coprocessor
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit * pAuUnit = new CHtAuUnit(pHtHif);

	pAuUnit->SendHostMsg(ADDR, (uint64_t)arr);

	// Call Unit
	pAuUnit->SendCall_htmain();

	while (!pAuUnit->RecvReturn_htmain(act_sum)) {
		usleep(1000);
	}

	// check results
	int err = 0;
	if (act_sum != exp_sum) {
		printf("act_sum %llu != exp_sum %llu\n", (long long)act_sum, (long long)exp_sum);
		printf("FAILED\n");
		err++;
	} else {
		printf("PASSED\n");
	}

	delete pAuUnit;
	delete pHtHif;

	return err;
}
