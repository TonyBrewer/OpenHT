#include "Ht.h"
using namespace Ht;

#include <cmath>

void usage(char *);
int num_length(uint64_t);

int default_size = 3;


int main(int argc, char **argv) {

	int debug = 0;
	int mat_int_len, temp;

#ifdef HT_MODEL
	default_size = 15;
#endif

#ifdef HT_SYSC
	default_size = 15;
#endif

#ifdef HT_VSIM
	default_size = 3;
#endif

	uint64_t i, j, k;
	uint64_t aRow, aCol, bRow, bCol;
	uint64_t *a1, *a2, *a3;

	// check command line args
	if (argc <= 2) {
		// Defaults
		aRow = default_size;
		aCol = default_size;
		bRow = default_size;
		bCol = default_size;
		if (argc==2) {
			if (atoi(argv[1]) == 1 || atoi(argv[1]) == 0) {
				debug = atoi(argv[1]);
			} else {
				usage(argv[0]);
			}
			debug = atoi(argv[1])==1 ? 1 : 0;
		}

	} else if (argc == 5 || argc == 6) {
		// Grab Command Line Values
		aRow = atoi(argv[1]);
		aCol = atoi(argv[2]);
		bRow = atoi(argv[3]);
		bCol = atoi(argv[4]);
		if (argc==6)
			debug = atoi(argv[5])==1 ? 1 : 0;

		if (aRow <= 0 || aCol <= 0 || bRow <= 0 || bCol <= 0) {
			usage(argv[0]);
			return 0;
		}
	} else {
		usage(argv[0]);
		return 0;
	}

	if (aCol != bRow) {
		printf("ERROR: Number of columns in Matrix A (%lld) does not equal the number of rows in Matrix B (%lld)\n\n",
			(long long)aCol, (long long)bRow);
		exit(1);
	}

	printf("Running with: Matrix A: (%lldx%lld) and Matrix B (%lldx%lld)\n",
		(long long)aRow, (long long)aCol, (long long)bRow, (long long)bCol);
	printf("Initializing arrays\n");
	fflush(stdout);

	a1 = (uint64_t *)malloc(aRow * aCol * 8);
	a2 = (uint64_t *)malloc(bRow * bCol * 8);
	a3 = (uint64_t *)malloc(aRow * bCol * 8);
	memset(a3, 0, aRow * bCol * 8);

	/* 
	* Matrix A and B are stored differently in memory (to make reads more efficient?)
	* The following are 3x3 examples: (A counts from 8 -> 0, B counts from 0 -> 8)
	*
	* Actual Numbers:         Memory Locations (by Index)
	* 
	* A: 8  7  6              A: 0  3  6
	*    5  4  5                 1  4  7
	*    2  1  0                 2  5  8
	*
	* B: 0  1  2              B: 0  1  2
	*    3  4  5                 3  4  5
	*    6  7  8                 6  7  8
	*
	* I did this to try to make A Rows available on strides, as well as B Columns on strides...
	*
	*/

	// Fill Matrix A
	k = 0;
	for (i = 0; i < aCol; i++) {
		for (j = aRow; j > 0; j--) {
			a1[k] = (aRow*aCol)-aCol*(aRow-j)-i-1;
			k++;
		}
	}

	// Fill Matrix B
	for (i = 0; i < bRow*bCol; i++) {
		a2[i] = i;
	}

	//Print Matrices
	mat_int_len = 1;
	temp = 0;
	for(i = 0; i < aRow*bCol; i++) {
		temp = num_length(a1[i]);
		mat_int_len = (temp > mat_int_len) ? temp : mat_int_len;
	}
	printf("Matrix A:\n");
	for (i = 0; i < aRow; i++) {
		for (j = 0; j < aCol; j++) {
			printf("%*lld ", mat_int_len, (long long)a1[i+aRow*j]);
		}
		printf("\n");
	}
	printf("\n\n");

	mat_int_len = 1;
	temp = 0;
	for(i = 0; i < aRow*bCol; i++) {
		temp = num_length(a1[i]);
		mat_int_len = (temp > mat_int_len) ? temp : mat_int_len;
	}
	printf("Matrix B:\n");
	for(i = 0; i < bRow*bCol; i++) {
		if (i > 0) {
			if (i%bCol == 0) {
				printf("\n");
			}
		}
		printf("%*lld ", mat_int_len, (long long)a2[i]);
	}
	printf("\n\n");


	// Debug - Print Matrix Values at memory locations
	if (debug) {
		printf("A - MEM\n");
		for (i = 0; i < aRow*aCol; i++) {
			printf("%lld - %lld\n", (long long)i, (long long)a1[i]);
		}

		printf("B - MEM\n");
		for (i = 0; i < bRow*bCol; i++) {
			printf("%lld - %lld\n", (long long)i, (long long)a2[i]);
		}
	}

	CHtHif *pHtHif = new CHtHif();

	// Coprocessor memory arrays
	uint64_t *cp_a1 = (uint64_t *)pHtHif->MemAllocAlign(4 * 1024, aRow * aCol * sizeof(uint64_t));
	uint64_t *cp_a2 = (uint64_t *)pHtHif->MemAllocAlign(4 * 1024, bRow * bCol * sizeof(uint64_t));
	uint64_t *cp_a3 = (uint64_t *)pHtHif->MemAllocAlign(4 * 1024, aRow * bCol * sizeof(uint64_t));

	pHtHif->MemCpy(cp_a1, a1, aRow * aCol * sizeof(uint64_t));
	pHtHif->MemCpy(cp_a2, a2, bRow * bCol * sizeof(uint64_t));
	pHtHif->MemSet(cp_a3, 0, aRow * bCol * sizeof(uint64_t));

	int unitCnt = pHtHif->GetUnitCnt();
	CHtAuUnit ** pAuUnits = new CHtAuUnit * [unitCnt];

	for (int unitId = 0; unitId < unitCnt; unitId += 1)
		pAuUnits[unitId] = new CHtAuUnit(pHtHif);

	printf("#AUs = %d\n", unitCnt);

	// avoid bank aliasing for performance
	if (unitCnt > 16 && !(unitCnt & 1)) unitCnt -= 1;
	printf("stride = %d\n", unitCnt);

	fflush(stdout);

	pHtHif->SendAllHostMsg(MA_BASE, (uint64_t)cp_a1);
	pHtHif->SendAllHostMsg(MB_BASE, (uint64_t)cp_a2);
	pHtHif->SendAllHostMsg(MC_BASE, (uint64_t)cp_a3);
	pHtHif->SendAllHostMsg(MC_ROW, (uint32_t)aRow);
	pHtHif->SendAllHostMsg(MC_COL, (uint32_t)bCol);
	pHtHif->SendAllHostMsg(COMMON, (uint32_t)aCol);

	for (int unit = 0; unit < unitCnt; unit++)
		pAuUnits[unit]->SendCall_htmain(unit /*rowOffset*/, unitCnt /*stride*/);

	for (int unit = 0; unit < unitCnt; unit++) {
		while (!pAuUnits[unit]->RecvReturn_htmain())
			usleep(1000);
		fflush(stdout);
	}

	pHtHif->MemCpy(a3, cp_a3, aRow * bCol * sizeof(uint64_t));

	// Print Resulting Matrix
	mat_int_len = 1;
	temp = 0;
	for(i = 0; i < aRow*bCol; i++) {
		temp = num_length(a3[i]);
		mat_int_len = (temp > mat_int_len) ? temp : mat_int_len;
	}

	printf("\nMatrix C:\n");
	for(i = 0; i < aRow*bCol; i++) {
		if (i > 0) {
			if (i%bCol == 0) {
				printf("\n");
			}
		}
		printf("%*lld ", mat_int_len, (long long)a3[i]);
	}
	printf("\n\n");

	if (debug) {
		printf("C - MEM\n");
		for (i = 0; i < aRow*bCol; i++) {
			printf("%lld - %lld\n", (long long)i, (long long)a3[i]);
		}
	}

	// Do error checking
	int err_cnt = 0;
	uint64_t rowNum = 0, colNum = 0, calcNum = 0, eleNum = 0;
	uint64_t *val;

	val = (uint64_t *)malloc(aRow * bCol * 8);
	memset(val, 0, aRow * bCol * 8);

	// Calculate the resulting matrix to check against coprocessor results
	for (rowNum = 0; rowNum < aRow; rowNum++) {

		for (colNum = 0; colNum < bCol; colNum++) {

			for (calcNum = 0; calcNum < aCol; calcNum++) {

				val[eleNum] += a1[rowNum+(calcNum*aRow)] * a2[colNum+(calcNum*bCol)];

			}

			eleNum++;

		}

	}

	// Check results
	for (eleNum = 0; eleNum < aRow*bCol; eleNum++) {

		if (val[eleNum] != a3[eleNum]) {
			err_cnt++;
			printf("Found element mismatch at matrix position %lld - found value %lld, expected value %lld.\n",
				(unsigned long long)eleNum, (unsigned long long)a3[eleNum], (unsigned long long)val[eleNum]);
		}

	}

	if (err_cnt == 0) {
		// Test Passed
		printf("PASSED\n\n");
	} else {
		// Test Failed
		printf("FAILED - error count %d\n\n", err_cnt);
	}

	// free memory
	free(a1);
	free(a2);
	free(a3);
	pHtHif->MemFreeAlign(cp_a1);
	pHtHif->MemFreeAlign(cp_a2);
	pHtHif->MemFreeAlign(cp_a3);
	free(val);

	delete pHtHif;

	return err_cnt;
}

// Print usage message and exit with error.
void usage(char *p) {
	printf("usage: %s [debug(0/1):0]\n", p);
	printf("\tDefault Matrix A size %d.\n", default_size);
	printf("\tDefault Matrix B size %d.\n\n", default_size);
	printf("usage: %s [aRow] [aCol] [bRow] [bCol] [debug(0/1):0]\n", p);
	exit(1);
}

int num_length(uint64_t num) {
	int ret = 0;
	if (num == 0) {
		ret =  1;
	} else {
		ret = (int)floor(log10((double)num)) + 1;
	}

	return ret;
}
