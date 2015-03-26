/*
1. A local copy of reduction variable  is made and initialized depending on the op(e.g. 0 for +).
2. Compiler finds standard reduction expressions containing op and uses them to update the local copy. 
3. Local copies are reduced into a single value and combined with the original global value.

*/
#include <stdio.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "hif.h"

int main()
{
  int i;
  int total=100;
  long sumx;
  long *suma = &sumx;

  pers_attach();

#pragma omp target
  {
      long sum=0;
      long sum2 = 0;

#pragma omp parallel for reduction(+:sum)
      for (i=0; i<= total; i++){
          sum = sum + i;
          //          printf("i is %d and sum is %ld\n", i, sum);
      }
      printf("after parallel region, sum is %ld\n", sum);

      long sum0;
#pragma omp parallel private(sum0)
      {
          sum0=0; 

#pragma omp for private(i)
          for (i=0; i<= total; i++)
              sum0=sum0+i;
          
#pragma omp critical
          sum2 = sum2 + sum0; 
      }
      printf("sum of 1 to %d = %ld and %ld\n",total, sum, sum2);

      *suma = (sum + sum2) / 2;
  }

  printf("sumx is %ld %s\n", sumx, sumx == 5050 ? "PASSED" : "FAILED");

  return (int)sumx;
}
