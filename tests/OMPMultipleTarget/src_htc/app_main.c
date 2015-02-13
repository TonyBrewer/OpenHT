#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmp_interface.h"
#include <omp.h>

extern int __htc_get_unit_count();
extern int global_radius;

void foo(uint8_t *a) {
#pragma omp target teams num_teams(8)
    {
        a[omp_get_team_num()] = omp_get_team_num();
    }

    printf("end of function foo\n");
}

int app_main(int argc, char **argv) {

    uint32_t bufsize = 1000;
    uint32_t bufsize2 = 1000;


    // Allocate target temp buffer.
    extern void *stencil_cp_alloc(size_t);
    uint8_t *unew = (uint8_t *)stencil_cp_alloc(bufsize * sizeof(uint8_t));

    printf("unit count is %d\n",  __htc_get_unit_count());

    int i;
    int k;

#pragma omp target
#pragma omp teams distribute parallel for num_threads(17) firstprivate(k) schedule(static,33) num_teams(5)
    for (i = 0; i < bufsize; i++) {
        k = (int)omp_get_team_num() + bufsize - bufsize2;
        //        printf("first target team %d thread %d i is %d\n", k,
        //                       (int)omp_get_thread_num(), i);
        unew[i] = (omp_get_team_num()+1) * omp_get_thread_num() + k - k;
    }

#pragma omp target
#pragma omp teams distribute parallel for num_threads(7) firstprivate(k) schedule(static,33) num_teams(8)
    for (i = 0; i < bufsize; i++) {
        k = (int)omp_get_team_num();
        //        printf("second target team %d thread %d i is %d\n", k,
        //                       (int)omp_get_thread_num(), i);
        unew[i] += (omp_get_team_num()+1) * omp_get_thread_num();
    }

    int sum = 0;

    for (i = 0; i < bufsize; i++) {
        sum += unew[i];
    } 

    printf("sum is %d  %s\n", sum, (sum == 13977) ? "PASSED" : "FAILED");

    foo(unew);

    for (i = 0; i < 8; i++) {
        sum += unew[i];
    } 

    printf("sum is %d  %s\n", sum, (sum == (13977+28)) ? "PASSED" : "FAILED");

    return 0;
}
