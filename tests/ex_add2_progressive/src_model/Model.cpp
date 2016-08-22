#include "Ht.h"
using namespace Ht;

void CHtModelAuUnit::UnitThread()
{
	do {
		uint64_t op1, op2, op3;
		if (RecvCall_htmain(op1, op2, op3)) {
			uint64_t *op1Mem = (uint64_t *)op1;
			uint64_t *op2Mem = (uint64_t *)op2;
			uint64_t *op3Mem = (uint64_t *)op3;
			uint64_t res = *op1Mem + *op2Mem;
			*op3Mem = res;
			while (!SendReturn_htmain());
		}
	} while (!RecvHostHalt());
	// The CHtHif class destructor issues a "Halt" message
	// to the units to terminate execution. The do {} while(),
	// mimics the behavior of the functionality provided by
	// the HT infrastructure.
}
