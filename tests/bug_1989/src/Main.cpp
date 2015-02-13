#include "Ht.h"
using namespace Ht;

// use host memory
#define HOSTMEM

void usage(char *);
uint64_t delta_usec(uint64_t *usr_stamp);

int main(int argc, char **argv)
{
	uint32_t i, vecLen;
	uint64_t *a1, *a2, *a3;

	// check command line args
	if (argc == 1) {
		vecLen = 100;  // default vecLen
	} else if (argc == 2) {
		uint64_t arg = atoi(argv[1]);
		vecLen = (uint32_t)arg;
		assert(arg == vecLen);
		if (vecLen <= 0) {
			usage(argv[0]);
			return 0;
		}
	} else {
		usage(argv[0]);
		return 0;
	}

	printf("Running with vecLen = %u\n", vecLen);
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

	CHtHif *pHtHif = new CHtHif();
	int unitCnt = pHtHif->GetUnitCnt();
	printf("#AUs = %d\n", unitCnt);

	CHtAuUnit ** pAuUnits = new CHtAuUnit * [unitCnt];
	for (int unit = 0; unit < unitCnt; unit++)
		pAuUnits[unit] = new CHtAuUnit(pHtHif);

#ifdef HOSTMEM
	uint64_t *cp_a1 = a1;
	uint64_t *cp_a2 = a2;
	uint64_t *cp_a3 = a3;
#else
	// Coprocessor memory arrays
	uint64_t *cp_a1 = (uint64_t*)pHtHif->MemAlloc(vecLen * sizeof(uint64_t));
	uint64_t *cp_a2 = (uint64_t*)pHtHif->MemAlloc(vecLen * sizeof(uint64_t));
	uint64_t *cp_a3 = (uint64_t*)pHtHif->MemAlloc(vecLen * sizeof(uint64_t));
	if (!cp_a1 || !cp_a2 || !cp_a3) {
		fprintf(stderr, "CP MemAlloc() failed.\n");
		exit(-1);
	}
	pHtHif->MemCpy(cp_a1, a1, vecLen * sizeof(uint64_t));
	pHtHif->MemCpy(cp_a2, a2, vecLen * sizeof(uint64_t));
	pHtHif->MemSet(cp_a3, 0, vecLen * sizeof(uint64_t));
#endif

	// Send arguments to all units using messages
	pHtHif->SendAllHostMsg(OP1_ADDR, (uint64_t)cp_a1);
	pHtHif->SendAllHostMsg(OP2_ADDR, (uint64_t)cp_a2);
	pHtHif->SendAllHostMsg(RES_ADDR, (uint64_t)cp_a3);

	// Send calls to units
	(void) delta_usec(NULL);
	uint32_t unitOffset = 0;
	uint32_t unitVecLen = (vecLen + unitCnt - 1) / unitCnt;

	for (int unit = 0; unit < unitCnt; unit++) {
		pAuUnits[unit]->SendCall_htmain(unitOffset, unitVecLen);
		unitOffset += unitVecLen;
		unitVecLen = min(unitVecLen, vecLen - unitOffset);
	}

	// Wait for returns
	uint64_t actSum = 0;
	uint64_t unitSum;
	for (int unit = 0; unit < unitCnt; unit++) {
		while (!pAuUnits[unit]->RecvReturn_htmain(unitSum))
			usleep(1000);
		actSum += unitSum;
	}

	uint64_t delta = delta_usec(NULL);
	float rbw = (2.0 * sizeof(uint64_t) * vecLen) / delta;
	float wbw = (1.0 * sizeof(uint64_t) * vecLen) / delta;
	printf("RTN: sum = %llu; BW(r/w) = (%.1f/%.1f) MB/sec\n",
		(long long)actSum, rbw, wbw);

#ifndef HOSTMEM
	pHtHif->MemCpy(a3, cp_a3, vecLen * sizeof(uint64_t));
#endif

	// check results
	int errCnt = 0;
	uint64_t expSum = 0;
	for (i = 0; i < vecLen; i++) {
		if (a3[i] != a1[i] + a2[i]) {
			printf("a3[%llu] is %llu, should be %llu\n",
			       (long long)i, (long long)a3[i], (long long)(a1[i] + a2[i]));
			errCnt++;
		}
		expSum += a1[i] + a2[i];
	}
	if (actSum != expSum) {
		printf("actSum %llu != expSum %llu\n", (long long)actSum, (long long)expSum);
		errCnt++;
	}

	if (errCnt)
		printf("FAILED: detected %d issues!\n", errCnt);
	else
		printf("PASSED\n");

	// free memory
	free(a1);
	free(a2);
	free(a3);
#ifndef HOSTMEM
	pHtHif->MemFree(cp_a1);
	pHtHif->MemFree(cp_a2);
	pHtHif->MemFree(cp_a3);
#endif

	delete pHtHif;

	return errCnt;
}

// Print usage message and exit with error.
void
usage(char *p)
{
	printf("usage: %s [count (default 100)] \n", p);
	exit(1);
}
