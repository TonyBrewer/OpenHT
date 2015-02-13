#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	uint64_t *arr;
	uint32_t length = 5;
	ht_posix_memalign((void **)&arr, 64, sizeof(uint64_t) * 8 * length);
	memset(arr, 0, sizeof(uint64_t) * 8 * length);

	uint64_t act_sum = 0;
	uint64_t exp_sum = 0;
	uint64_t i;
	for (i = 0; i < length; i++) {
		exp_sum += (0+1+2+3+4+5+6+7);
	}

	// Alloc Coprocessor
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit * pAuUnit = new CHtAuUnit(pHtHif);

	pAuUnit->SendHostMsg(ADDR, (uint64_t)arr);

	// Call Unit
	pAuUnit->SendCall_htmain(length);

	while (!pAuUnit->RecvReturn_htmain()) {
		usleep(1000);
	}

	for (i = 0; i < length; i++) {
		act_sum += arr[(i<<3) + 0];
		act_sum += arr[(i<<3) + 1];
		act_sum += arr[(i<<3) + 2];
		act_sum += arr[(i<<3) + 3];
		act_sum += arr[(i<<3) + 4];
		act_sum += arr[(i<<3) + 5];
		act_sum += arr[(i<<3) + 6];
		act_sum += arr[(i<<3) + 7];
	}

	// check results
	int err = 0;
	if (act_sum != exp_sum) {
		printf("act_sum %llu != exp_sum %llu\n", (long long)act_sum, (long long)exp_sum);
		printf("FAILED\n");
		err++;
	} else {
	  printf("PASSED - sum %llu\n", (long long)act_sum);
	}

	ht_free_memalign(arr);
	delete pAuUnit;
	delete pHtHif;

	return err;
}
