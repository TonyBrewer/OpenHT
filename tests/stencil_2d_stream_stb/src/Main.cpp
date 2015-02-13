#include "Ht.h"
using namespace Ht;

#include "Stencil.h"

void usage(char *);

int main(int argc, char **argv)
{
	uint32_t rows, cols;
	StType_t *a1, *a3, *e3;

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

	a1 = (StType_t *)malloc((rows + Y_SIZE-1) * (cols + X_SIZE-1) * sizeof(StType_t));
	a3 = (StType_t *)malloc((rows + Y_SIZE-1) * (cols + X_SIZE-1) * sizeof(StType_t));
	memset(a1, 0, (rows + Y_SIZE-1) * (cols + X_SIZE-1) * sizeof(StType_t));
	memset(a3, 0, (rows + Y_SIZE-1) * (cols + X_SIZE-1) * sizeof(StType_t));

	for (uint32_t r = 0; r < rows; r++)
		for (uint32_t c = 0; c < cols; c++) {
			a1[(r+Y_ORIGIN)*(cols + X_SIZE-1)+(c+X_ORIGIN)] = ((r+Y_ORIGIN) << 8) | (c+X_ORIGIN);
	}

	CHtHif *pHtHif = new CHtHif();
	int unitCnt = pHtHif->GetUnitCnt();
	printf("#AUs = %d\n", unitCnt);

	CHtAuUnit ** pAuUnits = new CHtAuUnit * [unitCnt];
	for (int unit = 0; unit < unitCnt; unit++)
		pAuUnits[unit] = new CHtAuUnit(pHtHif);

	// Coprocessor memory arrays
	StType_t *cp_a1 = (StType_t*)pHtHif->MemAlloc((rows+Y_SIZE-1) * (cols+X_SIZE-1) * sizeof(StType_t));
	StType_t *cp_a3 = (StType_t*)pHtHif->MemAlloc((rows+Y_SIZE-1) * (cols+X_SIZE-1) * sizeof(StType_t));
	if (!cp_a1 || !cp_a3) {
		fprintf(stderr, "ht_cp_malloc() failed.\n");
		exit(-1);
	}
	pHtHif->MemCpy(cp_a1, a1, (rows+Y_SIZE-1) * (cols+X_SIZE-1) * sizeof(StType_t));
	pHtHif->MemSet(cp_a3, 0, (rows+Y_SIZE-1) * (cols+X_SIZE-1) * sizeof(StType_t));

	fflush(stdout);

	CCoef coef;
	StType_t v = 1;
	for (uint32_t x = 0; x < X_SIZE; x += 1)
		for (uint32_t y = 0; y < Y_SIZE; y += 1)
			coef.m_coef[y][x] = v++;

	// Send calls to units
	uint32_t rowsPerUnit = rows / unitCnt;
	for (int unit = 0; unit < unitCnt; unit++) {
		StType_t * pSrcAddr = cp_a1 + unit * (cols+X_SIZE-1) * rowsPerUnit;
		StType_t * pDstAddr = cp_a3 + unit * (cols+X_SIZE-1) * rowsPerUnit;

		// last unit gets residual
		uint32_t unitRows;
		if (unit == unitCnt-1)
			unitRows = rows - unit * rowsPerUnit;
		else
			unitRows = rowsPerUnit;

		pAuUnits[unit]->SendCall_htmain((uint64_t)pSrcAddr, (uint64_t)pDstAddr, unitRows, cols, coef);
	}

	// generate expected results while waiting for returns
	e3 = (StType_t *)malloc((rows+Y_SIZE-1) * (cols+X_SIZE-1) * sizeof(StType_t));
	memset(e3, 0, (rows + Y_SIZE-1) * (cols + X_SIZE-1) * sizeof(StType_t));

	for (uint32_t row = 0; row < rows; row++) {
		for (uint32_t col = 0; col < cols; col++) {
			uint32_t rslt = 0;
			for (uint32_t x = 0; x < X_SIZE; x += 1)
				for (uint32_t y = 0; y < Y_SIZE; y += 1)
					rslt += a1[(row+y)*(cols+X_SIZE-1) + col+x] * coef.m_coef[y][x];

			e3[(row+Y_ORIGIN)*(cols+X_SIZE-1) + col+X_ORIGIN] = rslt;
		}
	}

	// Wait for returns
	for (int unit = 0; unit < unitCnt; unit++) {
		while (!pAuUnits[unit]->RecvReturn_htmain())
			usleep(1000);
	}

	pHtHif->MemCpy(a3, cp_a3, (rows+Y_SIZE-1) * (cols+X_SIZE-1) * sizeof(StType_t));

	// check results
	int err_cnt = 0;
	for (uint32_t col = 0; col < (cols+X_SIZE-1); col++) {
		for (uint32_t row = 0; row < (rows+Y_SIZE-1); row++) {
			if (a3[row*(cols+X_SIZE-1) + col] != e3[row*(cols+X_SIZE-1) + col]) {
				printf("a3[row=%u, col=%u] is %u, should be %u\n",
					   row, col, a3[row*(cols+X_SIZE-1) + col], e3[row*(cols+X_SIZE-1) + col]);
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
