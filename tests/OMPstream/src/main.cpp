#include "hif.h"

extern int app_main();

int main(int argc, char **argv) {

    ht_init_coproc();

    app_main();

    ht_detach();

    return 0;
}
