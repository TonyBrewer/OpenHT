#include "Ht.h"
using namespace Ht;


#ifdef HT_VSIM
#define NRUNS 5
#else
#define NRUNS 50
#endif


void usage(char *);
double atime();

int main(int argc, char **argv)
{
  int numRuns = 5;
  int i;
  double total = 0.0;
  double t1, t2;

  if (argc == 1) {
    numRuns = NRUNS;  // default numRuns
  } else if (argc == 2) {
    numRuns = atoi(argv[1]);
    if (numRuns <= 0) {
      usage(argv[0]);
      return 0;
    }
  } else {
    usage(argv[0]);
    return 0;
  }

  printf("Dispatch Microbenchmark\n");
  printf("  Running with numRuns = %d\n", numRuns);
  fflush(stdout);

  CHtHif *pHtHif = new CHtHif();
  CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);
  for(i = 0; i < numRuns; i++)
  {
    t1 = atime();
    pAuUnit->SendCall_htmain();

    while (!pAuUnit->RecvReturn_htmain())
      usleep(1000);
    t2 = atime();
    total += (t2-t1);
    //printf("call %d time: %f\n", i+1, t2-t1);
  }
  printf("Completed...\n");
  printf("\n");
  printf("\n");
  printf("Total Time: %f\n", total);
  printf("Average Call Time: %f\n", total/numRuns);
  printf("\n");
  printf("PASSED\n");
  printf("\n");

  delete pAuUnit;
  delete pHtHif;
  return 0;
}

double atime()
{
  struct timeval tp;
  struct timezone tzp;
  gettimeofday(&tp, &tzp);
  return ((double)tp.tv_sec + (double)tp.tv_usec * 1.e-6);
}



// Print usage message and exit with error.
void
usage(char *p)
{
	printf("usage: %s [numRuns (default 50)]\n", p);
	exit(1);
}
