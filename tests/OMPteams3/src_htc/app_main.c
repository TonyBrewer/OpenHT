#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmp_interface.h"
#include <omp.h>

extern int __htc_get_unit_count();
extern int global_radius;

int app_main(int argc, char **argv) {

    uint32_t bufsize = 1000;


    // Allocate target temp buffer.
    extern void *stencil_cp_alloc(size_t);
    uint8_t *unew = (uint8_t *)stencil_cp_alloc(bufsize * sizeof(uint8_t));

    printf("unit count is %d\n",  __htc_get_unit_count());

    int i;

#pragma omp target
#pragma omp teams distribute num_teams(4) dist_schedule(static, 12)
    for (i = 0; i < bufsize; i++) {
        printf("team %d thread %d i is %d\n", (int)omp_get_team_num(), 
                       (int)omp_get_thread_num(), i);
        unew[i] = omp_get_team_num() * (omp_get_thread_num()+1);
    }

    int sum = 0;

    for (i = 0; i < bufsize; i++) {
        //  printf("i = %d  val = %d\n", i, unew[i]);
        sum += unew[i];
    } 

    printf("sum is %d  %s\n", sum, (sum == 1488) ? "PASSED" : "FAILED");

    return 0;
}
