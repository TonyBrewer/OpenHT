#include "bmp.h"
#include "hif.h"

extern int app_main(int argc, char **argv);

int main(int argc, char **argv) {

    ht_init_coproc();

    app_main(argc, argv);

    ht_detach();

    return 0;
}
