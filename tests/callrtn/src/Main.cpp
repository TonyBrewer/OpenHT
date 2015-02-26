#include <string.h>

#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	pAuUnit->SendHostMsg(MSGVAR, 0);

	pAuUnit->SendHostMsg(MSGVAR, 1);

	pAuUnit->SendCall_htmain(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);

	bool err = false;
	int32_t rtn[10];
	while (!pAuUnit->RecvReturn_htmain(err,
					   rtn[0], rtn[1], rtn[2], rtn[3], rtn[4],
					   rtn[5], rtn[6], rtn[7], rtn[8], rtn[9]))
		usleep(1000);

	delete pHtHif;

	for (int i = 0; i < 10; i++)
		err |= rtn[i] != i;

	if (err)
		printf("FAILED\n");
	else
		printf("PASSED\n");

	return err;
}
