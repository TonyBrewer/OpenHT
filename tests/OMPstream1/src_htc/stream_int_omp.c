/*-----------------------------------------------------------------------*/
/* Program: Stream                                                       */
/* Revision: $Id: stream_omp.c,v 5.4 2009/02/19 13:57:12 mccalpin Exp mccalpin $ */
/* Original code developed by John D. McCalpin                           */
/* Programmers: John D. McCalpin                                         */
/*              Joe R. Zagar                                             */
/*                                                                       */
/* This program measures memory transfer rates in MB/s for simple        */
/* computational kernels coded in C.                                     */
/*-----------------------------------------------------------------------*/
/* Copyright 1991-2003: John D. McCalpin                                 */
/*-----------------------------------------------------------------------*/
/* License:                                                              */
/*  1. You are free to use this program and/or to redistribute           */
/*     this program.                                                     */
/*  2. You are free to modify this program for your own use,             */
/*     including commercial use, subject to the publication              */
/*     restrictions in item 3.                                           */
/*  3. You are free to publish results obtained from running this        */
/*     program, or from works that you derive from this program,         */
/*     with the following limitations:                                   */
/*     3a. In order to be referred to as "STREAM benchmark results",     */
/*         published results must be in conformance to the STREAM        */
/*         Run Rules, (briefly reviewed below) published at              */
/*         http://www.cs.virginia.edu/stream/ref.html                    */
/*         and incorporated herein by reference.                         */
/*         As the copyright holder, John McCalpin retains the            */
/*         right to determine conformity with the Run Rules.             */
/*     3b. Results based on modified source code or on runs not in       */
/*         accordance with the STREAM Run Rules must be clearly          */
/*         labelled whenever they are published.  Examples of            */
/*         proper labelling include:                                     */
/*         "tuned STREAM benchmark results"                              */
/*         "based on a variant of the STREAM benchmark code"             */
/*         Other comparable, clear and reasonable labelling is           */
/*         acceptable.                                                   */
/*     3c. Submission of results to the STREAM benchmark web site        */
/*         is encouraged, but not required.                              */
/*  4. Use of this program or creation of derived works based on this    */
/*     program constitutes acceptance of these licensing restrictions.   */
/*  5. Absolutely no warranty is expressed or implied.                   */
/*-----------------------------------------------------------------------*/
# include <stdio.h>
# include <math.h>
# include <float.h>
# include <limits.h>
# include <sys/time.h>
# include <stdlib.h>
# include <omp.h>
extern int __htc_get_unit_count();


/* INSTRUCTIONS:
 *
 *	1) Stream requires a good bit of memory to run.  Adjust the
 *          value of 'N' (below) to give a 'timing calibration' of 
 *          at least 20 clock-ticks.  This will provide rate estimates
 *          that should be good to about 5% precision.
 */

# define NN	20000
# define NTIMES	10
# define OFFSET	0

/*
 *	3) Compile the code with full optimization.  Many compilers
 *	   generate unreasonably bad code before the optimizer tightens
 *	   things up.  If the results are unreasonably good, on the
 *	   other hand, the optimizer might be too smart for me!
 *
 *         Try compiling with:
 *               cc -O stream_omp.c -o stream_omp
 *
 *         This is known to work on Cray, SGI, IBM, and Sun machines.
 *
 *
 *	4) Mail the results to mccalpin@cs.virginia.edu
 *	   Be sure to include:
 *		a) computer hardware model number and software revision
 *		b) the compiler flags
 *		c) all of the output from the test case.
 * Thanks!
 *
 */

# define HLINE "-------------------------------------------------------------\n"

# ifndef MYMIN
# define MYMIN(x,y) ((x)<(y)?(x):(y))
# endif
# ifndef MYMAX
# define MYMAX(x,y) ((x)>(y)?(x):(y))
# endif

static long long int	a[NN+OFFSET],
    b[NN+OFFSET],
    c[NN+OFFSET];

static double	avgtime[4] = {0}, maxtime[4] = {0},
		mintime[4] = {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX};

static char	*label[4] = {"Total:      ", "Scale:     ",
    "Add:       ", "Triad:     "};

static double	bytes[4] = {
    2 * sizeof(long long int) * NN,
    2 * sizeof(long long int) * NN,
    3 * sizeof(long long int) * NN,
    3 * sizeof(long long int) * NN
    };

extern double mysecond();
extern void checkSTREAMresults();
#ifdef TUNED
extern void tuned_STREAM_Copy();
extern void tuned_STREAM_Scale(long long int scalar);
extern void tuned_STREAM_Add();
extern void tuned_STREAM_Triad(long long int scalar);
#endif
int
app_main()
    {
    int			quantum, checktick();
    int			BytesPerWord;
    register int	j, k;
    long long int		scalar;
    double t, times[4][NTIMES];

    /* --- SETUP --- determine precision and check timing --- */

    printf(HLINE);
    BytesPerWord = sizeof(long long int);
    printf("This system uses %d bytes per LONG LONG INT PRECISION word.\n",
	BytesPerWord);

    printf(HLINE);
    printf("Array size = %d, Offset = %d\n" , NN, OFFSET);
    printf("Total memory required = %.1f MB.\n",
	(3.0 * BytesPerWord) * ( (double) NN / 1048576.0));
    printf("Each test is run %d times, but only\n", NTIMES);
    printf("the *best* time for each is used.\n");

#ifdef _OPENMP
    printf(HLINE);
#pragma omp parallel private(k)
    {
    k = omp_get_num_threads();
    printf ("Number of Threads requested = %i\n",k);
    }
#endif

    /* Get initial value for system clock. */
#pragma omp parallel for
    for (j=0; j<NN; j++) {
	a[j] = 1;
	b[j] = 2;
	c[j] = 0;
	}

    printf(HLINE);

    if  ( (quantum = checktick()) >= 1) 
	printf("Your clock granularity/precision appears to be "
	    "%d microseconds.\n", quantum);
    else
	printf("Your clock granularity appears to be "
	    "less than one microsecond.\n");

    t = mysecond();
#pragma omp parallel for
    for (j = 0; j < NN; j++)
	a[j] = 2 * a[j];
    t = 1.0E6 * (mysecond() - t);

    printf("Each test below will take on the order"
	" of %d microseconds.\n", (int) t  );
    printf("   (= %d clock ticks)\n", (int) (t/quantum) );
    printf("Increase the size of the arrays if this shows that\n");
    printf("you are not getting at least 20 clock ticks per test.\n");

    printf(HLINE);

    printf("WARNING -- The above is only a rough guideline.\n");
    printf("For best results, please be sure you know the\n");
    printf("precision of your system timer.\n");
    printf(HLINE);
    
    /*	--- MAIN LOOP --- repeat test cases NTIMES times --- */

    scalar = 3;
    int unitCount = __htc_get_unit_count();

    // Setup lb and ub for chunks across units
    long long int *bounds = (long long int*)malloc(sizeof(long long int)*unitCount+1);
    for (k = 0; k < unitCount; k++) {
        bounds[k] = k * (NN/unitCount);
    }
    bounds[unitCount] = NN;

#define THREADSPERUNIT 32

    for (k=0; k<NTIMES; k++)
	{
	times[0][k] = mysecond();

#pragma omp target teams num_teams(unitCount)
        {
            int lb = bounds[omp_get_team_num()];
            int ub = bounds[omp_get_team_num()+1];

#pragma omp parallel  num_threads(THREADSPERUNIT)
            {
#pragma omp for schedule(static,1) nowait
                for (j=lb; j<ub; j++)
                    c[j] = a[j];
                
#pragma omp for schedule(static,1) nowait
                for (j=lb; j<ub; j++)
                    b[j] = scalar*c[j];
                
                
#pragma omp for schedule(static,1) nowait
                for (j=lb; j<ub; j++)
                    c[j] = a[j]+b[j];
                
#pragma omp for schedule(static,1) nowait
                for (j=lb; j<ub; j++)
                    a[j] = b[j]+scalar*c[j];

            } // end of parallel
        } // end of target region

	times[0][k] = mysecond() - times[0][k];

        printf("finished iteration %d\n", k);
	}


    /*	--- SUMMARY --- */

    for (k=1; k<NTIMES; k++) /* note -- skip first iteration */
	{
	for (j=0; j<4; j++)
	    {
	    avgtime[j] = avgtime[j] + times[j][k];
	    mintime[j] = MYMIN(mintime[j], times[j][k]);
	    maxtime[j] = MYMAX(maxtime[j], times[j][k]);
	    }
	}
    
    printf("Function      Rate (MB/s)   Avg time     Min time     Max time\n");
    for (j=0; j<1; j++) {
	avgtime[j] = avgtime[j]/(double)(NTIMES-1);

	printf("%s%11.4f  %11.4f  %11.4f  %11.4f\n", label[j],
	       1.0E-06 * bytes[j]/mintime[j]*4,
	       avgtime[j],
	       mintime[j],
	       maxtime[j]);
    }
    printf(HLINE);

    /* --- Check Results --- */
    checkSTREAMresults();
    printf(HLINE);

    return 0;
}

# define	M	20

int
checktick()
    {
    int		i, minDelta, Delta;
    double	t1, t2, timesfound[M];

/*  Collect a sequence of M unique time values from the system. */

    for (i = 0; i < M; i++) {
	t1 = mysecond();
	while( ((t2=mysecond()) - t1) < 1.0E-6 )
	    ;
	timesfound[i] = t1 = t2;
	}

/*
 * Determine the minimum difference between these M values.
 * This result will be our estimate (in microseconds) for the
 * clock granularity.
 */

    minDelta = 1000000;
    for (i = 1; i < M; i++) {
	Delta = (int)( 1.0E6 * (timesfound[i]-timesfound[i-1]));
	minDelta = MYMIN(minDelta, MYMAX(Delta,0));
	}

   return(minDelta);
    }



/* A gettimeofday routine to give access to the wall
   clock timer on most UNIX-like systems.  */

#include <sys/time.h>

double mysecond()
{
        struct timeval tp;
        struct timezone tzp;
        int i;

        i = gettimeofday(&tp,&tzp);
        return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

void checkSTREAMresults ()
{
	long long int aj,bj,cj,scalar;
	long long int asum,bsum,csum;
	long long int epsilon;
	int	j,k;

    /* reproduce initialization */
	aj = 1;
	bj = 2;
	cj = 0;
    /* a[] is modified during timing check */
	aj = 2 * aj;
    /* now execute timing loop */
	scalar = 3;
	for (k=0; k<NTIMES; k++)
        {
            cj = aj;
            bj = scalar*cj;
            cj = aj+bj;
            aj = bj+scalar*cj;
        }
	aj = aj * (long long int) (NN);
	bj = bj * (long long int) (NN);
	cj = cj * (long long int) (NN);

	asum = 0;
	bsum = 0;
	csum = 0;
	for (j=0; j<NN; j++) {
		asum += a[j];
		bsum += b[j];
		csum += c[j];
	}
#ifdef VERBOSE
	printf ("Results Comparison: \n");
	printf ("        Expected  : %lld %lld %lld \n",aj,bj,cj);
	printf ("        Observed  : %lld %lld %lld \n",asum,bsum,csum);
#endif

#define abs(a) ((a) >= 0 ? (a) : -(a))
	epsilon = 0;

	if (abs(aj-asum)/asum > epsilon) {
		printf ("Failed Validation on array a[]\n");
		printf ("        Expected  : %lld \n",aj);
		printf ("        Observed  : %lld \n",asum);
	}
	else if (abs(bj-bsum)/bsum > epsilon) {
		printf ("Failed Validation on array b[]\n");
		printf ("        Expected  : %lld \n",bj);
		printf ("        Observed  : %lld \n",bsum);
	}
	else if (abs(cj-csum)/csum > epsilon) {
		printf ("Failed Validation on array c[]\n");
		printf ("        Expected  : %lld \n",cj);
		printf ("        Observed  : %lld \n",csum);
	}
	else {
		printf ("Solution Validates\n");
	}
}

#if 0
void tuned_STREAM_Copy()
{
	int j;
#pragma omp parallel for schedule(static, 1)
        for (j=0; j<NN; j++)
            c[j] = a[j];
}

void tuned_STREAM_Scale(long long int scalar)
{
	int j;
#pragma omp parallel for schedule(static, 1)
	for (j=0; j<NN; j++)
	    b[j] = scalar*c[j];
}

void tuned_STREAM_Add()
{
	int j;
#pragma omp parallel for schedule(static, 1)
	for (j=0; j<NN; j++)
	    c[j] = a[j]+b[j];
}

void tuned_STREAM_Triad(long long int scalar)
{
	int j;
#pragma omp parallel for schedule(static, 1)
	for (j=0; j<NN; j++)
	    a[j] = b[j]+scalar*c[j];
}
#endif

