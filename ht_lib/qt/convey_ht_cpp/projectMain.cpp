/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */


#include <string.h>

#include "Ht.h"

int main(int argc, char **argv)
{
    char buf[16];

    C%UNIT_NAME%Hif *m_p%UNIT_NAME%Hif = new C%UNIT_NAME%Hif();

    m_p%UNIT_NAME%Hif->AsyncCall_htmain(0, (uint64_t)&buf);

    while (!m_p%UNIT_NAME%Hif->Return_htmain(0)) {
        HtSleep(1000);
    }

    delete m_p%UNIT_NAME%Hif;

    int err = 0;
    if (strcmp(buf, "Hello")) {
        printf("FAILED: %s != Hello!\n", buf);
        err = 1;
    } else {
        printf("PASSED\n");
    }

    return err;
}

