#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

extern uint64_t vadd(uint64_t *a1, uint64_t *a2, uint64_t *a3, uint64_t vecLen);
void usage (char *);

int main(int argc, char **argv)
{
    uint64_t i;
    uint64_t vecLen;
    uint64_t *a1, *a2, *a3;

    // check command line args
    if (argc == 1)
	vecLen = 100;         // default vecLen
    else if (argc == 2) {
	vecLen = atoi(argv[1]);
	if (vecLen > 0) {
	    printf("Running UserApp.exe with vecLen = %llu\n", (long long) vecLen);
	} else {
	    usage (argv[0]);
	    return 0;
	}
    } else {
	usage (argv[0]);
	return 0;
    }

    a1 = (uint64_t *) malloc(vecLen*sizeof(uint64_t));
    a2 = (uint64_t *) malloc(vecLen*sizeof(uint64_t));
    a3 = (uint64_t *) malloc(vecLen*sizeof(uint64_t));

    for (i = 0; i < vecLen; i++) {
	a1[i] = i;
	a2[i] = 2*i;
    }

    uint64_t act_sum;
    act_sum = vadd(a1, a2, a3, vecLen);
    printf("RTN: act_sum = %llu\n", (long long) act_sum);

    // check results
    int err_cnt = 0;
    uint64_t exp_sum=0;
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
    if (act_sum != exp_sum){
	printf("act_sum %llu != exp_sum %llu\n", (long long)act_sum, (long long)exp_sum);
	err_cnt++;
    }

    if (err_cnt)
	printf("FAILED: detected %d issues!\n", err_cnt);
    else
	printf("PASSED\n");

    return err_cnt;
}

// Print usage message and exit with error.
void
usage (char* p)
{
    printf("usage: %s [count (default 100)] \n", p);
    exit (1);
}

