#include <string.h>

#include "Ht.h"
using namespace Ht;

void CHtModelAuUnit::UnitThread()
{
	do {
		uint64_t pBuf = 0;
		if (RecvCall_htmain(pBuf)) {
			(void)strcpy((char *)pBuf, "Hello");
			SendReturn_htmain();
		}
	} while (!RecvHostHalt());
	// The CHtHif class destructor issues a "Halt" message
	// to the units to terminate execution. The do {} while(),
	// mimics the behavior of the functionality provided by
	// the HT infrastructure.
}
