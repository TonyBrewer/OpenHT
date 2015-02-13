//#include "bmp.h"
#include "hif.h"

#define CNY_HTC_HOST 1
#define HIF_CODE 1

#include "../src_htc/rose_stream_int_omp.c"

int main(int argc, char **argv) {

    ht_init_coproc();

    app_main();

    ht_detach();

    return 0;
}
