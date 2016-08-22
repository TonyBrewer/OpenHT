#include "Ht.h"
using namespace Ht;

// use host memory
//#define HOSTMEM

#ifdef HT_VSIM
#define NTIMES 1
#else
#define NTIMES 10
#endif

void usage(char *);
uint64_t delta_usec(uint64_t *usr_stamp);

int main(int argc, char **argv)
{
	uint64_t i;
	uint64_t vecLen;
	uint64_t *a1, *a2, *a3;
	uint64_t *a1Chk, *a2Chk, *a3Chk;
	uint64_t scalar;

	// Check/Get Command-Line Args
	if (argc == 1) {
		vecLen = 100;  // default vecLen
		scalar = 3;   // default scalar
	} else if (argc == 2) {
		vecLen = atoi(argv[1]);
		scalar = 3;   // default scalar
		if (vecLen <= 0) {
			usage(argv[0]);
			return 0;
		}
	} else if (argc == 3) {
		vecLen = atoi(argv[1]);
		scalar = atoi(argv[2]);
		if (vecLen <= 0) {
			usage(argv[0]);
			return 0;
		}
	} else {
		usage(argv[0]);
		return 0;
	}

	uint64_t times[NTIMES][4];
	double avgtime[4], mintime[4], maxtime[4];
	uint64_t bytes[4] = {
		(2 * sizeof(uint64_t) * vecLen),
		(2 * sizeof(uint64_t) * vecLen),
		(3 * sizeof(uint64_t) * vecLen),
		(3 * sizeof(uint64_t) * vecLen)
	};

	printf("Stream Microbenchmark (Standard)\n");
	printf("  Running with vecLen = %llu, scalar = %llu\n", (long long)vecLen, (long long)scalar);
	printf("  Initializing arrays\n");
	fflush(stdout);

	// Initialize Arrays on Host
	a1 = (uint64_t *)malloc(vecLen * sizeof(uint64_t));
	a2 = (uint64_t *)malloc(vecLen * sizeof(uint64_t));
	a3 = (uint64_t *)malloc(vecLen * sizeof(uint64_t));
	a1Chk = (uint64_t *)malloc(vecLen * sizeof(uint64_t));
	a2Chk = (uint64_t *)malloc(vecLen * sizeof(uint64_t));
	a3Chk = (uint64_t *)malloc(vecLen * sizeof(uint64_t));

	for (i = 0; i < vecLen; i++) {
		a1[i] = i;
		a2[i] = 2 * i;
		a3[i] = 3 * i;
	}

	memcpy(a1Chk, a1, vecLen * sizeof(uint64_t));
	memcpy(a2Chk, a2, vecLen * sizeof(uint64_t));
	memcpy(a3Chk, a3, vecLen * sizeof(uint64_t));


	// Get Coprocessor / Unit handles
	CHtHif *pHtHif = new CHtHif();
	int unitCnt = pHtHif->GetUnitCnt();
	printf("  Unit Count = %d\n", unitCnt);

	CHtAuUnit ** pAuUnits = new CHtAuUnit * [unitCnt];
	for (int unit = 0; unit < unitCnt; unit++)
		pAuUnits[unit] = new CHtAuUnit(pHtHif);

	// Initialize Coprocessor Arrays and copy from Host
#ifdef HOSTMEM
	uint64_t *cp_a1 = a1;
	uint64_t *cp_a2 = a2;
	uint64_t *cp_a3 = a3;
#else
	uint64_t *cp_a1 = (uint64_t*)pHtHif->MemAlloc(vecLen * sizeof(uint64_t));
	uint64_t *cp_a2 = (uint64_t*)pHtHif->MemAlloc(vecLen * sizeof(uint64_t));
	uint64_t *cp_a3 = (uint64_t*)pHtHif->MemAlloc(vecLen * sizeof(uint64_t));
	if (!cp_a1 || !cp_a2 || !cp_a3) {
		fprintf(stderr, "ht_cp_malloc() failed.\n");
		exit(-1);
	}
	pHtHif->MemCpy(cp_a1, a1, vecLen * sizeof(uint64_t));
	pHtHif->MemCpy(cp_a2, a2, vecLen * sizeof(uint64_t));
	pHtHif->MemCpy(cp_a3, a3, vecLen * sizeof(uint64_t));
#endif

	// avoid bank aliasing for performance
	if (unitCnt > 16 && !(unitCnt & 1)) unitCnt -= 1;

	fflush(stdout);

	// Send some arguments to all units using messages
	pHtHif->SendAllHostMsg(OP1_ADDR, (uint64_t)cp_a1);
	pHtHif->SendAllHostMsg(OP2_ADDR, (uint64_t)cp_a2);
	pHtHif->SendAllHostMsg(OP3_ADDR, (uint64_t)cp_a3);
	pHtHif->SendAllHostMsg(VEC_LEN, (uint64_t)vecLen);
	pHtHif->SendAllHostMsg(SCALAR, (uint64_t)scalar);

	// Send calls to units
	for (int loop = 0; loop < NTIMES; loop++) {
		printf("  Loop %d:", loop);
		for (int operation = 0; operation < 4; operation++) {
			printf("  Op %d", operation);
			delta_usec(NULL);
			for (int unit = 0; unit < unitCnt; unit++)
				pAuUnits[unit]->SendCall_htmain(operation, unit /*offset*/, unitCnt /*stride*/);

			// Wait for returns
			for (int unit = 0; unit < unitCnt; unit++)
				while (!pAuUnits[unit]->RecvReturn_htmain())
					usleep(1000);
			times[loop][operation] = delta_usec(NULL);
		}
		printf("\n");
	}
	printf("Completed...\n");


	// Copy Arrays back to Host
#ifndef HOSTMEM
	pHtHif->MemCpy(a1, cp_a1, vecLen * sizeof(uint64_t));
	pHtHif->MemCpy(a2, cp_a2, vecLen * sizeof(uint64_t));
	pHtHif->MemCpy(a3, cp_a3, vecLen * sizeof(uint64_t));
#endif

	// Print Stats
	for (int loop = 0; loop < NTIMES; loop++) {
		for (int operation = 0; operation < 4; operation++) {
			avgtime[operation] += times[loop][operation];
			if (loop == 0) {
				mintime[operation] = times[loop][operation];
				maxtime[operation] = times[loop][operation];
			} else {
				mintime[operation] = (mintime[operation] < times[loop][operation]) ? mintime[operation] : times[loop][operation];
				maxtime[operation] = (maxtime[operation] > times[loop][operation]) ? maxtime[operation] : times[loop][operation];
			}
		}
	}
	for (int operation = 0; operation < 4; operation++)
		avgtime[operation] /= NTIMES;

	printf("\n");
	printf("\n");
	printf("Function  Rate (MB/s)     Avg time     Min time     Max time\n");
	for (int operation = 0; operation < 4; operation++) {
		printf("%8d%13.4f  %11.4f  %11.4f  %11.4f\n",
		       operation,
		       bytes[operation]/mintime[operation],
		       avgtime[operation],
		       mintime[operation],
		       maxtime[operation]);
	}
	printf("\n");

	// check results
	printf("\n");
	printf("\n");
	int errCnt = 0;
	for (int loop = 0; loop < NTIMES; loop++) {
		for (uint i = 0; i < vecLen; i++) {
			a3Chk[i] = a1Chk[i];
			a2Chk[i] = a3Chk[i]*scalar;
			a3Chk[i] = a1Chk[i] + a2Chk[i];
			a1Chk[i] = a2Chk[i] + scalar * a3Chk[i];
		}
	}
	for (uint i = 0; i < vecLen; i++) {
		if (a1Chk[i] != a1[i]) {
			fprintf(stderr, "ERROR - A1[%d] = %ld (expected %ld)\n", i, a1[i], a1Chk[i]);
			errCnt++;
		}
		if (a2Chk[i] != a2[i]) {
			fprintf(stderr, "ERROR - A2[%d] = %ld (expected %ld)\n", i, a2[i], a2Chk[i]);
			errCnt++;
		}
		if (a3Chk[i] != a3[i]) {
			fprintf(stderr, "ERROR - A3[%d] = %ld (expected %ld)\n", i, a3[i], a3Chk[i]);
			errCnt++;
		}
	}
	printf("\n");
	if (errCnt) {
		fprintf(stderr, "FAILED (errCnt = %d)\n", errCnt);
	} else {
		printf("PASSED\n");
	} 
	printf("\n");

	// free memory
	free(a1);
	free(a2);
	free(a3);
#ifndef HOSTMEM
	pHtHif->MemFree(cp_a1);
	pHtHif->MemFree(cp_a2);
	pHtHif->MemFree(cp_a3);
#endif

	for (int unit = 0; unit < unitCnt; unit++)
		delete pAuUnits[unit];
	delete pHtHif;

	return errCnt;
}

// Print usage message and exit with error.
void
usage(char *p)
{
	printf("usage: %s [count (default 100)] [scalar (default 3)]\n", p);
	exit(1);
}
