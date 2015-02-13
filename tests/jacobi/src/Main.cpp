#include "Ht.h"
using namespace Ht;

// Jacobi Relaxation

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef long long int FIXEDPNT;
typedef FIXEDPNT DTYPE;

#define FRAC_W 23
#define ONE (1 << FRAC_W)

#define M 128   // 100
#define N 128   // 100

#define i_dim 3
#define j_dim 3

const FIXEDPNT EPSILONF = (FIXEDPNT)(ONE * 0.001);
const FIXEDPNT W0F = (FIXEDPNT)(ONE * (0.7));
const FIXEDPNT W1F = (FIXEDPNT)(ONE * (0.2 * .25));
const FIXEDPNT W2F = (FIXEDPNT)(ONE * (0.1 * .25));
const FIXEDPNT HALFF = (FIXEDPNT)(ONE * 0.5);

#define EPSILON 0.00001
#define W0 (0.7)
#define W1 (0.2 * .25)
#define W2 (0.1 * .25)

DTYPE A[M][N];
DTYPE B[M][N];

void init()
{
	int i, j;

	srand(1);
	double val;
	DTYPE valW;

	for (i = 0; i < M; i++) {
		for (j = 0; j < N; j++) {
			val = (((double)rand() / RAND_MAX) * 2.0) - 1.0;
			valW = (DTYPE)(val * ONE);
			A[j][i] = valW;
			//            fprintf(stderr,"val is %g and A val is %g\n",
			//                    val,
			//                    (double)(A[j][i])/(double)(ONE));
			if ((i == 0) || (j == 0) || (i == (M - 1)) || (j == (N - 1)))
				B[j][i] = valW;
			else
				B[j][i] = (DTYPE)(123456789.0 * ONE);
		}
	}
}

void display()
{
	int i, j;

	for (i = 0; i < 10; i++)
		for (j = 0; j < 10; j++)
			fprintf(stderr, "A[%d,%d] = %g\n", j, i, (double)(A[j][i]) / (double)(ONE));

	for (i = M - 10; i < M; i++)
		for (j = N - 10; j < N; j++)
			fprintf(stderr, "A[%d,%d] = %g\n", j, i, (double)(A[j][i]) / (double)(ONE));
}

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtIuUnit *pIuUnit = new CHtIuUnit(pHtHif);


#if 0
	fprintf(stderr, "epsilon=%llx w0=%llx w1=%llx w2=%llx half=%llx one=%llx\n",
		(unsigned long long)(EPSILONF),
		(unsigned long long)(W0F),
		(unsigned long long)(W1F),
		(unsigned long long)(W2F),
		(unsigned long long)(HALFF),
		(unsigned long long)(ONE));
	fprintf(stderr, "epsilon=%f w0=%f w1=%f w2=%f half=%f one=%f\n",
		(double)EPSILONF / (double)ONE,
		(double)W0F / (double)ONE,
		(double)W1F / (double)ONE,
		(double)W2F / (double)ONE,
		(double)HALFF / (double)ONE,
		(double)ONE / (double)ONE);
#endif

	init();

	//    iterations = jacob(A, B, M, N, W0F, W1F, W2F, EPSILONF);
	pIuUnit->SendCall_jacob((uint64_t)A, (uint64_t)B, M, N, W0F, W1F, W2F, EPSILONF);

	// wait for return
	int32_t iterations;
	while (!pIuUnit->RecvReturn_jacob(iterations))
		usleep(1000);

	fprintf(stderr, "change is %g\n", (double)(A[0][0]) / (double)ONE);
	fprintf(stderr, "Complete in %d iterations\n", iterations);

#if RESULT
	display();
#endif

	delete pHtHif;

	if (iterations == 107)
		printf("PASSED\n");
	else
		printf("FAILED\n");

	return 0;
}
