#include <stdint.h>
#include <stdio.h>
#include <omp.h>

void vadd (uint64_t *a, uint64_t *b, uint64_t *c, uint64_t vecLen, uint64_t *result) {

#pragma omp target 
    {
        uint64_t act_sum = 0;
#pragma omp parallel for reduction(+:act_sum)
        for (uint64_t i = 0; i < vecLen; i++) {
            uint64_t sum = a[i] + b[i];
            c[i] = sum;
            act_sum += sum;
        }

        printf("After parallel act_sum is %lld\n", act_sum);

        *result = act_sum;
    }
}
