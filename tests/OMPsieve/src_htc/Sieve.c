#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "hif.h"

#define NSRCH 100

int a_declaration;

main() {
  char *prime;
  int i;

  pers_attach();

  prime = (char *) malloc(NSRCH*sizeof(prime));
  memset(prime,1,NSRCH*sizeof(prime));
  
#pragma omp target
{
  for(i=2;i<NSRCH;i++) {
      // printf("i is %d and prime[i] is %d\n", i, prime[i]);
    if(prime[i]) { 
      int idx=i*i;
      while(idx < NSRCH) {
          // printf("setting prime for idx %d to 0\n", idx);
        prime[idx] = 0;
        idx += i;
      }
    }
  }
} //end pragma omp target

  
  int n=0;
  for(i=1;i<NSRCH;i++) {
    if(prime[i]) {
      n++;
      printf("%d is prime\n",i);
    }
  }

  printf("%s\n", (n==26) ? "PASSED" : "FAILED");

  pers_detach();
}
