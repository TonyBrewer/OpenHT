#include "Ht.h"
using namespace Ht;

static int debug = 0;

void CHtModelAuUnit::UnitThread()
{
	uint64_t *op1Base = NULL, *op2Base = NULL, *resBase = NULL;
	uint64_t mcRow = 0, mcCol = 0, comRowCol = 0;

	do {
		uint8_t msgType;
		uint64_t msgData;
		if (RecvHostMsg(msgType, msgData)) {
			switch (msgType) {
			case MA_BASE: op1Base = (uint64_t *)msgData; break;
			case MB_BASE: op2Base = (uint64_t *)msgData; break;
			case MC_BASE: resBase = (uint64_t *)msgData; break;
			case MC_ROW: mcRow = msgData; break;
			case MC_COL: mcCol = msgData; break;
			case COMMON: comRowCol = msgData; break;
			default: assert(0);
			}
		}

		uint32_t rowIdx, stride;
		if (RecvCall_htmain(rowIdx, stride)) {
			// Loop over entire dataset
			while (rowIdx < mcRow) {
				uint64_t *op1Addr = op1Base + rowIdx;           // Row in Matrix A
				uint64_t *op2Addr = op2Base;                    // Col in Matrix B
				uint64_t *resAddr = resBase + (rowIdx * mcCol); // Row in Matrix C

				if (debug)
					printf("Working Row: %d | mcRow = %lld\n", rowIdx, (unsigned long long)mcRow);

				// Loop over each element in the row
				for (unsigned i = 0; i < mcCol; i++) {
					if (debug)
						printf("Column: %d\n", i);

					// Loop over each calculation per row entry
					uint64_t sum = 0;
					for (unsigned j = 0; j < comRowCol; j++) {
						if (debug)
							printf("Addr: %lld : %lld   ",
							       (unsigned long long)op1Addr, (unsigned long long)op2Addr);

						uint64_t op1 = *op1Addr;
						uint64_t op2 = *op2Addr;
						sum += op1 * op2;

						if (debug)
							printf("Data: %lld : %lld\n",
							       (unsigned long long)op1, (unsigned long long)op2);

						op1Addr += mcRow;
						op2Addr += mcCol;
					}

					// Store Matrix C element
					*resAddr = sum;

					// Increment to next element in same row in Matrix A
					// Increment to next column in Matrix B
					// Increment to next element in same row in Matrix C
					op1Addr = op1Base + rowIdx;
					op2Addr = op2Base + (i + 1);
					resAddr++;
				}

				// Go to next available row
				rowIdx += stride;
			}

			while (!SendReturn_htmain());
		}
	} while (!RecvHostHalt());
	// The CHtHif class destructor issues a "Halt" message
	// to the units to terminate execution. The do {} while(),
	// mimics the behavior of the functionality provided by
	// the HT infrastructure.
}
