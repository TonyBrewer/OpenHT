#include "bmp.h"
#include "hif.h"

#define CNY_HTC_HOST 1
#define HIF_CODE 1

#include "../src_htc/rose_app_main.c"

int main(int argc, char **argv) {

    ht_init_coproc();

    app_main(argc, argv);

    ht_detach();

    return 0;
}
