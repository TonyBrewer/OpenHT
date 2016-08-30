#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	uint64_t a = 0x12345678;
	uint64_t res1 = 0;
	uint64_t res2 = 0;
	uint64_t res3 = 0;
	uint64_t res4 = 0;

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	pHtHif->SendAllHostMsg(VAR_ADDR, (uint64_t)&a);

	// Send call 
	while (!pUnit->SendCall_htmain());

	// Wait for return
	while (!pUnit->RecvReturn_htmain(res1, res2, res3, res4))
		usleep(1000);

	delete pUnit;
	delete pHtHif;

	// check results
	int error = 0;
	if (a != res1) {
		printf("FAILED \"P_\" (0x%lx != 0x%lx)\n", a, res1);
		printf("Assigning res1 happened incorrectly at the top of PersCtl!\n");
		error++;
	}

	if (a != res2) {
		printf("FAILED \"PR_\" (0x%lx != 0x%lx)\n", a, res2);
		printf("Assigning res2 happened incorrectly at the top of PersCtl!\n");
		error++;
	}

	if (a != res3) {
		printf("FAILED \"P_\" (0x%lx != 0x%lx)\n", a, res3);
		printf("Assigning res3 happened incorrectly in the ASSIGN instruction of PersCtl!\n");
		error++;
	}

	if (a != res4) {
		printf("FAILED \"PR_\" (0x%lx != 0x%lx)\n", a, res4);
		printf("Assigning res4 happened incorrectly in the ASSIGN instruction of PersCtl!\n");
		error++;
	}

	if (error == 0) {
		printf("PASSED\n");
	}

	return error;
}
