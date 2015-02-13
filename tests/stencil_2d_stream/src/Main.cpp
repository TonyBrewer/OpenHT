#include "Ht.h"
using namespace Ht;

#include "Stencil.h"

void usage(char *);

int main(int argc, char **argv)
{
	uint32_t rows, cols;
	uint32_t *a1, *a3, *e3;

	int const radius = STENCIL_RADIUS;

	// check command line args
	if (argc == 1) {
		rows = 100;  // default rows
		cols = 100;  // default cols
	} else if (argc == 3) {
		rows = atoi(argv[1]);
		if (rows <= 0 || rows > 1023) {
			usage(argv[1]);
			return 0;
		}
		cols = atoi(argv[2]);
		if (cols <= 0 || cols > 1023) {
			usage(argv[2]);
			return 0;
		}
	} else {
		usage(argv[0]);
		return 0;
	}

	printf("Running with rows = %u, cols = %u\n", rows, cols);
	printf("Initializing arrays\n");
	fflush(stdout);

	a1 = (uint32_t *)malloc((rows + radius * 2) * (cols + radius * 2) * sizeof(uint32_t));
	a3 = (uint32_t *)malloc((rows + radius * 2) * (cols + radius * 2) * sizeof(uint32_t));
	memset(a1, 0, (rows + radius * 2) * (cols + radius * 2) * sizeof(uint32_t));
	memset(a3, 0, (rows + radius * 2) * (cols + radius * 2) * sizeof(uint32_t));

	for (uint32_t r = 0; r < rows; r++)
		for (uint32_t c = 0; c < cols; c++) {
			a1[(r+radius)*(cols + radius * 2)+(c+radius)] = ((r+radius) << 8) | (c+radius);
	}

	CHtHif *pHtHif = new CHtHif();
	int unitCnt = pHtHif->GetUnitCnt();
	printf("#AUs = %d\n", unitCnt);

	CHtAuUnit ** pAuUnits = new CHtAuUnit * [unitCnt];
	for (int unit = 0; unit < unitCnt; unit++)
		pAuUnits[unit] = new CHtAuUnit(pHtHif);

	// Coprocessor memory arrays
	uint32_t *cp_a1 = (uint32_t*)pHtHif->MemAlloc((rows+radius*2) * (cols+radius*2) * sizeof(uint32_t));
	uint32_t *cp_a3 = (uint32_t*)pHtHif->MemAlloc((rows+radius*2) * (cols+radius*2) * sizeof(uint32_t));
	if (!cp_a1 || !cp_a3) {
		fprintf(stderr, "ht_cp_malloc() failed.\n");
		exit(-1);
	}
	pHtHif->MemCpy(cp_a1, a1, (rows+radius*2) * (cols+radius*2) * sizeof(uint32_t));
	pHtHif->MemSet(cp_a3, 0, (rows+radius*2) * (cols+radius*2) * sizeof(uint32_t));

	// avoid bank aliasing for performance
	if (unitCnt > 16 && !(unitCnt & 1)) unitCnt -= 1;
	printf("stride = %d\n", unitCnt);

	fflush(stdout);

	// Send calls to units
	uint32_t rowsPerUnit = (cols + unitCnt - 1) / unitCnt;
	for (int unit = 0; unit < unitCnt; unit++) {
		uint32_t * pSrcAddr = cp_a1 + unit * (cols+radius*2) * rowsPerUnit;
		uint32_t * pDstAddr = cp_a3 + unit * (cols+radius*2) * rowsPerUnit;

		uint32_t unitRows = (unit+1)*rowsPerUnit > rows ? (rows - unit*rowsPerUnit) : rowsPerUnit;

		pAuUnits[unit]->SendCall_htmain((uint64_t)pSrcAddr, (uint64_t)pDstAddr, unitRows, cols);
	}

	// generate expected results while waiting for returns
	e3 = (uint32_t *)malloc((rows+radius*2) * (cols+radius*2) * sizeof(uint32_t));
	memset(e3, 0, (rows + radius * 2) * (cols + radius * 2) * sizeof(uint32_t));

	uint32_t coef[5] = { STENCIL_COEF2, STENCIL_COEF1, STENCIL_COEF0/2, STENCIL_COEF1, STENCIL_COEF2 };

	for (uint32_t row = radius; row < rows+radius; row++) {
		for (uint32_t col = radius; col < cols+radius; col++) {
			uint32_t rslt = 0;
			for (uint32_t c = col - STENCIL_RADIUS; c <= col + STENCIL_RADIUS; c++)
				rslt += a1[row*(cols+radius*2) + c] * coef[c - col + STENCIL_RADIUS];

			for (uint32_t r = row - STENCIL_RADIUS; r <= row + STENCIL_RADIUS; r++)
				rslt += a1[r*(cols+radius*2) + col] * coef[r - row + STENCIL_RADIUS];

			e3[row*(cols+radius*2) + col] = rslt >> 8;
		}
	}

	// Wait for returns
	for (int unit = 0; unit < unitCnt; unit++) {
		while (!pAuUnits[unit]->RecvReturn_htmain())
			usleep(1000);
	}

	pHtHif->MemCpy(a3, cp_a3, (rows+radius*2) * (cols+radius*2) * sizeof(uint32_t));

	// check results
	int err_cnt = 0;
	for (uint32_t col = 0; col < (cols+radius*2); col++) {
		for (uint32_t row = 0; row < (rows+radius*2); row++) {
			if (a3[row*(cols+radius*2) + col] != e3[row*(cols+radius*2) + col]) {
				printf("a3[row=%u, col=%u] is %u, should be %u\n",
					   row, col, a3[row*(cols+radius*2) + col], e3[row*(cols+radius*2) + col]);
				err_cnt++;
			}
		}
	}

	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	// free memory
	free(a1);
	free(a3);
	pHtHif->MemFree(cp_a1);
	pHtHif->MemFree(cp_a3);

	delete pHtHif;

	return err_cnt;
}

// Print usage message and exit with error.
void
usage(char *p)
{
	printf("usage: %s [count (default 100)] \n", p);
	exit(1);
}
