#include <stdint.h>
#include <stdio.h>
#include <omp.h>

void vadd (uint64_t *a, uint64_t *b, uint64_t *c, uint64_t vecLen, uint64_t *result) {
    int N;
    N = 1000;

#pragma omp target teams distribute parallel for num_teams(8)
    for (uint64_t i = 0; i < vecLen; i++) {
        uint64_t sum = a[i] + b[i];
        c[i] = sum;
    }

#pragma omp target 
    {
        uint64_t sum0;
#pragma omp parallel for reduction(+:sum0)
        for (uint64_t i = 0; i < vecLen; i++) {
            sum0 += c[i];
        }

        *result = sum0;
    }
}
