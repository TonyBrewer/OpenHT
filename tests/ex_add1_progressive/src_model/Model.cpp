#include "Ht.h"
using namespace Ht;

void CHtModelAuUnit::UnitThread()
{
	do {
		uint64_t op1, op2;
		if (RecvCall_htmain(op1, op2)) {
			uint64_t res = 0;
			res = op1 + op2;
			while (!SendReturn_htmain(res));
		}
	} while (!RecvHostHalt());
	// The CHtHif class destructor issues a "Halt" message
	// to the units to terminate execution. The do {} while(),
	// mimics the behavior of the functionality provided by
	// the HT infrastructure.
}
