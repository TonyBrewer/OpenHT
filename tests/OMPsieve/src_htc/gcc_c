/* kcollins - RandomAccess core_single_cpu kernel from HPCC */

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include "hif.h"

#define NSRCH 100

int a_declaration;

main() {
  char *prime;
  int i;

  //  pers_attach();

  prime = (char *) malloc(NSRCH);
  memset(prime,1,NSRCH);
  
#pragma omp target
{
  for(i=2;i<NSRCH;i++) {
    if(prime[i]) { 
      int idx=i*i;
      while(idx < NSRCH) {
        prime[idx] = 0;
        printf("i is %d idx is %d clearing\n", (int)i, (int)idx);
        idx += i;
      }
    }
  }
} //end pragma omp target

  
  for(i=1;i<NSRCH;i++) {
    if(prime[i]) printf("%d is prime\n",i);
  }

  //  pers_detach();
}
