#include "Ht.h"
using namespace Ht;

void CHtModelAuUnit::UnitThread()
{
	uint64_t *op1Addr = NULL, *op2Addr = NULL, *resAddr = NULL;
	uint64_t vecLen = 0;

	do {
		uint8_t msgType;
		uint64_t msgData;
		if (RecvHostMsg(msgType, msgData)) {
			switch (msgType) {
			case OP1_ADDR: op1Addr = (uint64_t *)msgData; break;
			case OP2_ADDR: op2Addr = (uint64_t *)msgData; break;
			case RES_ADDR: resAddr = (uint64_t *)msgData; break;
			case VEC_LEN:  vecLen = msgData; break;
			default: assert(0);
			}
		}

		uint32_t vecIdx, stride;
		if (RecvCall_htmain(vecIdx, stride)) {
			uint64_t sum = 0;
			while (vecIdx < vecLen) {
				uint64_t op1 = *(op1Addr + vecIdx);
				uint64_t op2 = *(op2Addr + vecIdx);
				uint64_t res = op1 + op2;
				sum += res;
				*(resAddr + vecIdx) = res;
				vecIdx += stride;
			}
			while (!SendReturn_htmain(sum));
		}
	} while (!RecvHostHalt());
	// The CHtHif class destructor issues a "Halt" message
	// to the units to terminate execution. The do {} while(),
	// mimics the behavior of the functionality provided by
	// the HT infrastructure.
}
