#include "Ht.h"
using namespace Ht;

void usage(char *);

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

	CHtHif *pHtHif = new CHtHif();
	int unitCnt = pHtHif->GetUnitCnt();
	printf("#AUs = %d\n", unitCnt);

	CHtAuUnit ** pAuUnits = new CHtAuUnit * [unitCnt];
	for (int unit = 0; unit < unitCnt; unit++)
		pAuUnits[unit] = new CHtAuUnit(pHtHif);

	// Coprocessor memory arrays
	uint64_t *cp_a1 = (uint64_t*)pHtHif->MemAlloc(vecLen * sizeof(uint64_t));
	uint64_t *cp_a2 = (uint64_t*)pHtHif->MemAlloc(vecLen * sizeof(uint64_t));
	uint64_t *cp_a3 = (uint64_t*)pHtHif->MemAlloc(vecLen * sizeof(uint64_t));
	if (!cp_a1 || !cp_a2 || !cp_a3) {
		fprintf(stderr, "ht_cp_malloc() failed.\n");
		exit(-1);
	}
	pHtHif->MemCpy(cp_a1, a1, vecLen * sizeof(uint64_t));
	pHtHif->MemCpy(cp_a2, a2, vecLen * sizeof(uint64_t));
	pHtHif->MemSet(cp_a3, 0, vecLen * sizeof(uint64_t));

	// avoid bank aliasing for performance
	if (unitCnt > 16 && !(unitCnt & 1)) unitCnt -= 1;
	printf("stride = %d\n", unitCnt);

	fflush(stdout);

	// Send arguments to all units using messages
	pHtHif->SendAllHostMsg(OP1_ADDR, (uint64_t)cp_a1);
	pHtHif->SendAllHostMsg(OP2_ADDR, (uint64_t)cp_a2);
	pHtHif->SendAllHostMsg(RES_ADDR, (uint64_t)cp_a3);
	pHtHif->SendAllHostMsg(VEC_LEN, (uint64_t)vecLen);

	// Send calls to units
	for (int unit = 0; unit < unitCnt; unit++)
		pAuUnits[unit]->SendCall_htmain(unit /*offset*/, unitCnt /*stride*/);

	// Artificially wait until some time has passed (and reset deasserts)
	usleep(100000);

	int errCnt = 0;

	uint64_t wrData = 0x5a5a5a5a5a;
	pHtHif->UserIOCsrWr(0, wrData);
	uint64_t rdData = 0;
	pHtHif->UserIOCsrRd(0, rdData);

	if (rdData != wrData) {
		printf("ERROR: CSR Read data did not match Write data!\n");
		errCnt++;
	}

	// Wait for returns
	for (int unit = 0; unit < unitCnt; unit++) {
		while (!pAuUnits[unit]->RecvReturn_htmain())
			usleep(1000);
	}


	pHtHif->MemCpy(a3, cp_a3, vecLen * sizeof(uint64_t));

	// check results
	uint64_t actSum = 0;
	uint64_t expSum = 0;
	for (i = 0; i < vecLen; i++) {
		expSum += a1[i] + a2[i];
		actSum += a3[i];
	}

	if (actSum != expSum) {
		printf("ERROR: actSum = %ld, expSum = %ld!\n", actSum, expSum);
		errCnt++;
	}

	if (errCnt)
		printf("FAILED (%d issues)\n", errCnt);
	else
		printf("PASSED\n");

	// free memory
	free(a1);
	free(a2);
	free(a3);
	pHtHif->MemFree(cp_a1);
	pHtHif->MemFree(cp_a2);
	pHtHif->MemFree(cp_a3);

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
