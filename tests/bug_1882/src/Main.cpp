#include "Ht.h"
using namespace Ht;

// Main Program
int main(int argc, char **argv) {

	printf("\nStarting simple stream\n\n");

	// Init coprocessor - Alloc 1 AU
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	// Make initial call
	pAuUnit->SendCall_htmain();

	// Send a single 64b piece of data, followed by a marker
	uint64_t data = 0xffffULL;
	while (!pAuUnit->SendHostData(1, &data)) {
		usleep(1000);
	}

	pAuUnit->SendHostDataMarker();
	pAuUnit->FlushHostData();

	// Wait to receive data
	uint64_t rcvd = 0;
	while (!pAuUnit->RecvHostData(1, &rcvd)) {
		usleep(1000);
	}
	
	// Return from coprocessor
	while (!pAuUnit->RecvReturn_htmain()) {
		usleep(1000);
	}

	delete pAuUnit;
	delete pHtHif;

	printf("PASSED\n");
}
