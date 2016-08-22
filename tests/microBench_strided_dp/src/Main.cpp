#include "Ht.h"
using namespace Ht;

// use host memory
//#define HOSTMEM

#ifdef HT_SYSC
#define M       128
#define N       1024//128*1024
#define M1      (M+1)
#define NREP	5//100
#else
#ifdef HT_VSIM
#define M       2//128
#define N       1024//128*1024
#define M1      (M+1)
#define NREP	5//100
#else
#define M       128
#define N       1024*1024
#define M1      (M+1)
#define NREP    100
#endif
#endif

void usage(char *);
uint64_t delta_usec(uint64_t *usr_stamp);

int main(int argc, char **argv)
{
	uint64_t i;
	double *x, *y, t;
	uint64_t stride;
	double ttr[M-1];

	printf("Strided Microbenchmark (double)\n");
	printf("  Initializing arrays\n");
	fflush(stdout);

	x = (double *)malloc((M1*N) * sizeof(double));
	y = (double *)malloc((M1*N) * sizeof(double));

	for (i = 0; i < (M1*N); i++) {
		x[i] = (double)i;
		y[i] = (double)i;
	}

	CHtHif *pHtHif = new CHtHif();
	int unitCnt = pHtHif->GetUnitCnt();
	printf("  Unit Count: %d\n", unitCnt);
	fflush(stdout);

	CHtAuUnit ** pAuUnits = new CHtAuUnit * [unitCnt];
	for (int unit = 0; unit < unitCnt; unit++)
		pAuUnits[unit] = new CHtAuUnit(pHtHif);

	// Coprocessor memory arrays
	double *cp_x = (double*)pHtHif->MemAlloc((M1*N) * sizeof(double));
	double *cp_y = (double*)pHtHif->MemAlloc((M1*N) * sizeof(double));
	if (!cp_x || !cp_y) {
		fprintf(stderr, "ht_cp_malloc() failed.\n");
		exit(-1);
	}
	pHtHif->MemCpy(cp_x, x, (M1*N) * sizeof(double));
	pHtHif->MemCpy(cp_y, y, (M1*N) * sizeof(double));


	// Send arguments to all units using messages
	pHtHif->SendAllHostMsg(OPX_ADDR, (uint64_t)cp_x);
	pHtHif->SendAllHostMsg(OPY_ADDR, (uint64_t)cp_y);

	// Send calls to units
	int errCnt = 0;
	printf(" STRIDE       TIME (SEC)           MFLOPS           MBYTES            RATIO\n");
	fflush(stdout);
	for (stride = 1; stride < M; stride++) {
		delta_usec(NULL);
		for (uint64_t i = 0; i < NREP; i++) {
			t = (i+1);
			pHtHif->SendAllHostMsg(VEC_LEN, (uint64_t)N*stride);
			for (int unit = 0; unit < unitCnt; unit++)
				pAuUnits[unit]->SendCall_htmain(unit*stride /*offset*/, unitCnt*stride /*stride*/, t);

			// Wait for returns
			for (int unit = 0; unit < unitCnt; unit++) {
				while (!pAuUnits[unit]->RecvReturn_htmain())
					usleep(1000);
			}
		}
		ttr[stride-1] = delta_usec(NULL)*1.0e-6;
		double tmr = 2.0*1.0e-6*(double)N*(double)NREP/ttr[stride-1];
		double tbr = 3.0*sizeof(long)*1.0e-6*(double)N*(double)NREP/ttr[stride-1];
		printf(" %6ld %16.7f %16.7f %16.7f %16.7f\n", stride, ttr[stride-1], tmr, tbr, ttr[stride-1]/ttr[0]);
		fflush(stdout);
	}

	printf("Completed...\n");
	printf("\n");
	printf("PASSED\n");
	printf("\n");

	// free memory
	free(x);
	free(y);
	pHtHif->MemFree(cp_x);
	pHtHif->MemFree(cp_y);

	for (int unit = 0; unit < unitCnt; unit++)
		delete pAuUnits[unit];
	delete [] pAuUnits;
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
