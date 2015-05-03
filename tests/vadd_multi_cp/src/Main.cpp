#include "Ht.h"
using namespace Ht;

void usage(char *);

#define CP_CNT 2

int main(int argc, char **argv)
{
	int i;
	int totalVecLen;
	uint64_t *a1, *a2, *a3;

	// check command line args
	if (argc == 1) {
		totalVecLen = 100;  // default vecLen
	} else if (argc == 2) {
		totalVecLen = atoi(argv[1]);
		if (totalVecLen <= 0) {
			usage(argv[0]);
			return 0;
		}
	} else {
		usage(argv[0]);
		return 0;
	}

	printf("Running with vecLen = %llu\n", (long long)totalVecLen);
	printf("Initializing arrays\n");
	fflush(stdout);

	a1 = (uint64_t *)malloc(totalVecLen * sizeof(uint64_t));
	a2 = (uint64_t *)malloc(totalVecLen * sizeof(uint64_t));
	a3 = (uint64_t *)malloc(totalVecLen * sizeof(uint64_t));
	memset(a3, 0, totalVecLen * sizeof(uint64_t));

	for (i = 0; i < totalVecLen; i++) {
		a1[i] = i;
		a2[i] = 2 * i;
	}

	CHtHif *pHtHif[CP_CNT];
	int unitCnt[CP_CNT];
	int cpVecLen[CP_CNT];
	int totalUnitCnt = 0;

	for (int cpIdx = 0; cpIdx < CP_CNT; cpIdx += 1) {
		pHtHif[cpIdx] = new CHtHif();
		totalUnitCnt += unitCnt[cpIdx] = pHtHif[cpIdx]->GetUnitCnt();
		printf("CP%d #AUs = %d\n", cpIdx, unitCnt[cpIdx]);
	}

	int unitVecInc = (totalVecLen + CP_CNT - 1) / CP_CNT;
	int remVecLen = totalVecLen;

	uint64_t *cp_a1[CP_CNT];
	uint64_t *cp_a2[CP_CNT];
	uint64_t *cp_a3[CP_CNT];

	CHtAuUnit ** pAuUnits = new CHtAuUnit *[totalUnitCnt];
	int unit = 0;
	int vecStart = 0;
	for (int cpIdx = 0; cpIdx < CP_CNT; cpIdx += 1) {
		for (int cpUnit = 0; cpUnit < unitCnt[cpIdx]; cpUnit++, unit++)
			pAuUnits[unit] = new CHtAuUnit(pHtHif[cpIdx]);

		cpVecLen[cpIdx] = remVecLen < unitVecInc ? remVecLen : unitVecInc;
		remVecLen -= cpVecLen[cpIdx];

		// Coprocessor memory arrays
		cp_a1[cpIdx] = (uint64_t*)pHtHif[cpIdx]->MemAlloc(cpVecLen[cpIdx] * sizeof(uint64_t));
		cp_a2[cpIdx] = (uint64_t*)pHtHif[cpIdx]->MemAlloc(cpVecLen[cpIdx] * sizeof(uint64_t));
		cp_a3[cpIdx] = (uint64_t*)pHtHif[cpIdx]->MemAlloc(cpVecLen[cpIdx] * sizeof(uint64_t));

		if (!cp_a1 || !cp_a2 || !cp_a3) {
			fprintf(stderr, "ht_cp_malloc() failed.\n");
			exit(-1);
		}
		pHtHif[cpIdx]->MemCpy(cp_a1[cpIdx], a1 + vecStart, cpVecLen[cpIdx] * sizeof(uint64_t));
		pHtHif[cpIdx]->MemCpy(cp_a2[cpIdx], a2 + vecStart, cpVecLen[cpIdx] * sizeof(uint64_t));
		pHtHif[cpIdx]->MemSet(cp_a3[cpIdx], 0, cpVecLen[cpIdx] * sizeof(uint64_t));

		vecStart += unitVecInc;

		// avoid bank aliasing for performance
		if (unitCnt[cpIdx] > 16 && !(unitCnt[cpIdx] & 1)) 
			unitCnt[cpIdx] -= 1;
		printf("CP%d stride = %d\n", cpIdx, unitCnt[cpIdx]);
	}

	fflush(stdout);

	// Send arguments to all units using messages
	for (int cpIdx = 0; cpIdx < CP_CNT; cpIdx += 1) {
		pHtHif[cpIdx]->SendAllHostMsg(OP1_ADDR, (uint64_t)cp_a1[cpIdx]);
		pHtHif[cpIdx]->SendAllHostMsg(OP2_ADDR, (uint64_t)cp_a2[cpIdx]);
		pHtHif[cpIdx]->SendAllHostMsg(RES_ADDR, (uint64_t)cp_a3[cpIdx]);
		pHtHif[cpIdx]->SendAllHostMsg(VEC_LEN, (uint64_t)cpVecLen[cpIdx]);
	}

	// Send calls to units
	for (int unit = 0; unit < totalUnitCnt; unit++)
		pAuUnits[unit]->SendCall_htmain(unit /*offset*/, unitCnt[0] /*stride*/);

	// Wait for returns
	uint64_t actSum = 0;
	uint64_t unitSum;
	for (int unit = 0; unit < totalUnitCnt; unit++) {
		while (!pAuUnits[unit]->RecvReturn_htmain(unitSum))
			usleep(1000);
		actSum += unitSum;
	}

	printf("RTN: sum = %llu\n", (long long)actSum);

	vecStart = 0;
	for (int cpIdx = 0; cpIdx < CP_CNT; cpIdx += 1) {
		pHtHif[cpIdx]->MemCpy(a3 + vecStart, cp_a3[cpIdx], cpVecLen[cpIdx] * sizeof(uint64_t));
		vecStart += unitVecInc;
	}

	// check results
	int errCnt = 0;
	uint64_t expSum = 0;
	for (i = 0; i < totalVecLen; i++) {
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
	for (int cpIdx = 0; cpIdx < CP_CNT; cpIdx += 1) {
		pHtHif[cpIdx]->MemFree(cp_a1[cpIdx]);
		pHtHif[cpIdx]->MemFree(cp_a2[cpIdx]);
		pHtHif[cpIdx]->MemFree(cp_a3[cpIdx]);

		delete pHtHif[cpIdx];
	}

	return errCnt;
}

// Print usage message and exit with error.
void
usage(char *p)
{
	printf("usage: %s [count (default 100)] \n", p);
	exit(1);
}
