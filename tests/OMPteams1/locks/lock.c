#include <omp.h>
#include <stdio.h>

static int lk;
static int lock_counter;
typedef int rhomp_lock_t;
//typedef int bool;

int lock(int op, rhomp_lock_t L) 
{
    int result = 0;

    if (op == 0) {   // request lock
        if ((lk & (1<<L)) == 0) {
            lk |= (1<<L);
            result = 1;
        }
    } else {         // request unlock
        lk &= ~(1<<L);
    }
    return result;

}

int rhomp_test_lock(rhomp_lock_t L) {
    return lock(0, L);
}


void rhomp_set_lock(rhomp_lock_t L) {
    while (lock(0, L) != 1) {}
    return;
}

void rhomp_unset_lock(rhomp_lock_t L) {
    lock(1, L);
}

rhomp_lock_t rhomp_init_lock() {
    rhomp_lock_t result = lock_counter;
    lock_counter++;
    return result;
}


void rhomp_begin_named_critical(rhomp_lock_t L) {
    rhomp_lock_t GW;
    if (L) {
        rhomp_set_lock(L);
        return;
    } else {
        rhomp_set_lock(3);
        if (L == 0) {
            rhomp_lock_t temp = rhomp_init_lock();
            GW = temp;
        }
        rhomp_unset_lock(3);
        rhomp_set_lock(GW);
        return;
    }
}


#if 0
int foo() {

    int counter = 0;
    int i;

    int junk = rhomp_init_lock();
    junk = rhomp_init_lock();

    for (i = 0; i < 150; i++) {
        junk = rhomp_init_lock();
        printf("initialized lock and value is %d\n", junk);
    }

    if (0) {
        junk = rhomp_test_lock(0);
    }

#pragma omp parallel num_threads(10)
    {
        rhomp_set_lock(1);
        printf("thread %d got a lock\n", omp_get_thread_num());
        counter++;
        rhomp_unset_lock(1);
        printf("thread %d unset the lock\n", omp_get_thread_num());
    }

    return counter;
}

int hello() {
    printf("result is %d\n", foo());
    return 0;
}
#endif

