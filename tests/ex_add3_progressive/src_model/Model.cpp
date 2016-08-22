#include "Ht.h"
using namespace Ht;

void CHtModelAuUnit::UnitThread()
{
	do {
		uint64_t op1, op2, op3;
		if (RecvCall_htmain(op1, op2, op3)) {
			uint64_t op1Mem[8], op2Mem[8], op3Mem[8];
			for (int i = 0; i < 8; i++) {
				op1Mem[i] = *((uint64_t *)op1 + i);
				op2Mem[i] = *((uint64_t *)op2 + i);
			}
			for (int i = 0; i < 8; i++) {
				op3Mem[i] = op1Mem[i] + op2Mem[i];
				fprintf(stderr, "%lld = %lld + %lld\n", (long long)op3Mem[i], (long long)op1Mem[i], (long long)op2Mem[i]);
			}
			for (int i = 0; i < 8; i++) {
				*((uint64_t *)op3 + i) = op3Mem[i];
			}
			while (!SendReturn_htmain());
		}
	} while (!RecvHostHalt());
	// The CHtHif class destructor issues a "Halt" message
	// to the units to terminate execution. The do {} while(),
	// mimics the behavior of the functionality provided by
	// the HT infrastructure.
}
