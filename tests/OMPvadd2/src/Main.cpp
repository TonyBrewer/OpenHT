//#include "Ht.h"
#include "hif.h"

//using namespace Ht;

void usage(char *);

extern "C" void vadd (uint64_t *a, uint64_t *b, uint64_t *c, 
                      uint64_t vecLen, uint64_t *result);

#define CNY_HTC_HOST 1
#define HIF_CODE 1
#include "../src_htc/rose_vadd.c"

int main(int argc, char **argv)
{
	uint64_t i;
	uint64_t vecLen;
	uint64_t *a1, *a2, *a3;

	// check command line args
	if (argc == 1) {
		vecLen = 100;  // default vecLen
	} else if (argc == 2) {
		vecLen = atoi(argv[1]);
		if (vecLen <= 0) {
			usage(argv[0]);
			return 0;
		}
	} else {
		usage(argv[0]);
		return 0;
	}

	printf("Running with vecLen = %llu\n", (long long)vecLen);
	printf("Initializing arrays\n");
	fflush(stdout);

	a1 = (uint64_t *)malloc(vecLen * sizeof(uint64_t));
	a2 = (uint64_t *)malloc(vecLen * sizeof(uint64_t));
	a3 = (uint64_t *)malloc(vecLen * sizeof(uint64_t));
	memset(a3, 0, vecLen * sizeof(uint64_t));

	for (i = 0; i < vecLen; i++) {
		a1[i] = i;
		a2[i] = 2 * i;
	}

        ht_init_coproc();

	// Coprocessor memory arrays
	uint64_t *cp_a1 = (uint64_t*)__htc_cp_alloc(vecLen * sizeof(uint64_t));
	uint64_t *cp_a2 = (uint64_t*)__htc_cp_alloc(vecLen * sizeof(uint64_t));
	uint64_t *cp_a3 = (uint64_t*)__htc_cp_calloc(vecLen, sizeof(uint64_t));
	if (!cp_a1 || !cp_a2 || !cp_a3) {
		fprintf(stderr, "ht_cp_malloc() failed.\n");
		exit(-1);
	}
	__htc_memcpy(cp_a1, a1, vecLen * sizeof(uint64_t));
	__htc_memcpy(cp_a2, a2, vecLen * sizeof(uint64_t));

        uint64_t act_sum;

        vadd(cp_a1, cp_a2, cp_a3, vecLen, &act_sum);

	printf("RTN: act_sum = %llu\n", (long long)act_sum);

	__htc_memcpy(a3, cp_a3, vecLen * sizeof(uint64_t));

	// check results
	int err_cnt = 0;
	uint64_t exp_sum = 0;
	for (i = 0; i < vecLen; i++) {
                if (a3[i] != a1[i] + a2[i]) {
			printf("a3[%llu] is %llu, should be %llu\n",
			       (long long)i, (long long)a3[i], (long long)(a1[i] + a2[i]));
			err_cnt++;
                }
		exp_sum += a1[i] + a2[i];
		//printf("i=%llu:  a1=%llu + a2=%llu => a3=%llu\n",
		//	(long long)i, (long long)a1[i], (long long)a2[i], (long long)a3[i]);
	}
	if (act_sum != exp_sum) {
		printf("act_sum %llu != exp_sum %llu\n", (long long)act_sum, (long long)exp_sum);
		err_cnt++;
	}

	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	// free memory
	free(a1);
	free(a2);
	free(a3);
	__htc_cp_free(cp_a1);
	__htc_cp_free(cp_a2);
	__htc_cp_free(cp_a3);

	ht_detach();

	return err_cnt;
}

// Print usage message and exit with error.
void
usage(char *p)
{
	printf("usage: %s [count (default 100)] \n", p);
	exit(1);
}
