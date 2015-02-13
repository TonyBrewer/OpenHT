#include <string.h>

#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	Char_t buf[16];

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	Host_t callHs;
	callHs.a = 0x45454545;
	callHs.b = 0x3423;
	callHs.c[0] = 0x12;
	callHs.c[1] = 0x67;
	callHs.c[2] = -34;
	callHs.d = 0x92929292;

	pAuUnit->SendCall_htmain(buf, callHs);

	Host_t rtnHs;
	Char_t * pRtnBuf;
	void * pVoid;
	while (!pAuUnit->RecvReturn_htmain(rtnHs, pRtnBuf, pVoid))
		usleep(1000);

	delete pHtHif;

	int err = 0;
	if (strcmp(buf, "Hello")) {
		printf("FAILED: %s != Hello!\n", buf);
		err = 1;
	} else if ((((uint64_t)buf ^ (uint64_t)pRtnBuf) & 0xffffffffffffLL) != 0) {
		printf("FAILED: buf != pRtnBuf\n");
		err = 1;
	} else if (callHs.a != rtnHs.a || callHs.b != rtnHs.b || memcmp(callHs.c, rtnHs.c, 5) != 0 || callHs.d != rtnHs.d) {
		printf("FAILED: callHs != rtnHs\n");
		err = 1;
	} else {
		printf("PASSED\n");
	}

	return err;
}
