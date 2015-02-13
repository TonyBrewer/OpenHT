#include "Ht.h"
using namespace Ht;

#ifndef SEED
# define SEED -1
#endif

#ifndef MAX_VAL
# define MAX_VAL 200
#endif

#ifdef HT_VSIM
# define DATA_LEN 50
# define TAP_CNT 3
#else
#ifdef HT_SYSC
# define DATA_LEN 500
# define TAP_CNT 50
#else
# define DATA_LEN 500
# define TAP_CNT 50
#endif
#endif

// State Struct to keep track of information per AU
typedef struct auState {
	bool	   done;      // Flag - AU completed all tasks
	bool	   doneCall;  // Flag - AU completed call, do send/recv

	int32_t lastIdx;    // Last Index that will be sent to this AU
} auState_t;


// State Struct to keep track of sent/rcvd information (mostly for debug)
typedef struct infoState {
	bool	   sent;
	bool	   rcvd;
} infoState_t;


int main(int argc, char **argv) {

	printf("FIR Filter Example Application\n");

	uint32_t numAUActive = 0;
	uint32_t errCnt = 0;

	// Arrays for Inputs/Coefficients/Outputs
	int64_t *inputArr;
	int64_t *outputArr;
	int64_t *coeffArr;

	// Length of Input/Output Data
	// Only used to know when to stop sending data to AUs and how much space to allocate
	uint32_t dataLen = DATA_LEN;

	// Number of Taps for FIR Filter
	uint32_t numTaps = TAP_CNT;


	// Random Seed for Input Data/Coefficient Generation
	struct timeval st;
	gettimeofday(&st, NULL);
	unsigned int seed = (SEED != -1) ? SEED : (int)st.tv_usec;
	printf("Using Random Seed: 0x%x\n", seed);
	srand(seed);

	// Array Alloc
	inputArr  = (int64_t*)calloc(dataLen, sizeof(int64_t));
	outputArr = (int64_t*)calloc(dataLen, sizeof(int64_t));

	coeffArr  = (int64_t*)calloc(numTaps, sizeof(int64_t));

	// Random data for input/coeff (+/- numbers)
	uint32_t i;
	for (i = 0; i < dataLen; i++) {
		inputArr[i] = (rand() % (MAX_VAL - 1)) + 1 - MAX_VAL/2;
	}
	for (i = 0; i < numTaps; i++) {
		coeffArr[i] = (rand() % (MAX_VAL - 1)) + 1 - MAX_VAL/2;
	}

	CHtHif *pHtHif = new CHtHif();

	// Coprocessor memory arrays
	int64_t *cpInputArr = (int64_t*)pHtHif->MemAllocAlign(4 * 1024, dataLen * sizeof(uint64_t));
	int64_t *cpOutputArr = (int64_t*)pHtHif->MemAllocAlign(4 * 1024, dataLen * sizeof(uint64_t));
	int64_t *cpCoeffArr = (int64_t*)pHtHif->MemAllocAlign(4 * 1024, numTaps * sizeof(uint64_t));

	pHtHif->MemCpy(cpInputArr,  inputArr, dataLen * sizeof(uint64_t));
	pHtHif->MemSet(cpOutputArr,        0, dataLen * sizeof(uint64_t));
	pHtHif->MemCpy(cpCoeffArr,  coeffArr, numTaps * sizeof(uint64_t));

	// Setup Coprocessor
	uint32_t unitCnt = pHtHif->GetUnitCnt();
	CHtAuUnit ** pAuUnits = new CHtAuUnit * [unitCnt];
	printf("AU Count: %d\n", unitCnt);

	for (uint32_t unitId = 0; unitId < unitCnt; unitId += 1)
		pAuUnits[unitId] = new CHtAuUnit(pHtHif);

	// Init State for each AU (so we can keep track of work)
	auState_t *auState = (auState_t *)calloc(unitCnt, sizeof(auState_t));
	uint32_t au;
	for (au = 0; au < unitCnt; au++) {
		numAUActive++;
		auState[au].done = false;
		auState[au].doneCall = false;
		auState[au].lastIdx = dataLen - (dataLen % unitCnt) + au;
		if (auState[au].lastIdx > (int32_t)dataLen-1) {
			auState[au].lastIdx -= unitCnt;
		}
		if (auState[au].lastIdx < 0) {
			// If lastIdx < 0 (happens when unitCnt > dataLen), make -1 to skip this AU)
			// This is handled on coprocessor, looks like normal call from the host
			auState[au].lastIdx = -1;
		}
	}

	// Init State for all data information (so we can keep track of work)
	infoState_t *infoState = (infoState_t *)calloc(dataLen, sizeof(infoState_t));
	for (i = 0; i < dataLen; i++) {
		infoState[i].sent = false;
		infoState[i].rcvd = false;
	}

	// Send Array Pointers to Alloc'd AUs
	pHtHif->SendAllHostMsg(INPUT_BASE, (uint64_t)cpInputArr);
	pHtHif->SendAllHostMsg(OUTPUT_BASE,(uint64_t)cpOutputArr);
	pHtHif->SendAllHostMsg(COEFF_BASE, (uint64_t)cpCoeffArr);
	pHtHif->SendAllHostMsg(NUM_TAPS,   (uint32_t)numTaps);


	// Loop until all AUs have completed tasks
	// Use Streaming Interface to Send/Receive data
	uint64_t curIdx = 0;
	while (numAUActive > 0) {
		for (au = 0; au < unitCnt; au++) {

			uint64_t rcvIdx;

			// AU complete, skip to next AU
			if (auState[au].done == true) {
				continue;
			}

			// Call
			if (auState[au].doneCall == false) {

				// Call Coprocessor to become ready to receive data
				if (pAuUnits[au]->SendCall_htmain(au, auState[au].lastIdx)) {
					printf("Host:  AU %2d - Call\n", au);
					auState[au].doneCall = true;
				}
			}

			// Send
			if ((auState[au].doneCall == true) && (auState[au].lastIdx >= (int32_t)curIdx) && (curIdx < dataLen)) {

				// Wait until we're sure the data was sent
				while (!pAuUnits[au]->SendHostData(1, &curIdx)) {
					usleep(1000);
				}
				//printf("Host:  AU %2d - Sent Work - Index %d\n", au, (uint32_t)curIdx);
				infoState[curIdx].sent = true;

				// Flush on last sent index to this AU
				if (auState[au].lastIdx == (int32_t)curIdx) {
					pAuUnits[au]->FlushHostData();
				}

				// Increment the index for the next AU
				curIdx++;
			}

			// Receive
			while (pAuUnits[au]->RecvHostData(1, &rcvIdx)) {
				//printf("Host:  AU %2d - Received Response - Index %d\n", au, (uint32_t)rcvIdx);
				infoState[rcvIdx].rcvd = true;
			}

			// Return
			uint32_t errs;
			if (pAuUnits[au]->RecvReturn_htmain(errs)) {
				printf("Host:  AU %2d - Return\n", au);

				errCnt += errs;

				numAUActive--;
				auState[au].done = true;
			}
		}
	}


	// Copy Return Data and Free Coprocessor Resources
	pHtHif->MemCpy(outputArr, cpOutputArr, dataLen * sizeof(uint64_t));

	pHtHif->MemFreeAlign(cpInputArr);
	pHtHif->MemFreeAlign(cpOutputArr);
	pHtHif->MemFreeAlign(cpCoeffArr);

	// Disable Coprocessor
	for (uint32_t unitId = 0; unitId < unitCnt; unitId += 1)
		delete pAuUnits[unitId];

	delete pHtHif;

	// Error Checking
	for (i = 0; i < dataLen; i++) {
		if (infoState[i].sent==true && infoState[i].rcvd==true) {
			// Passed
		} else if (infoState[i].sent != true) {
			printf("ERROR: Index %d was not sent\n", i);
			errCnt++;
		} else if (infoState[i].rcvd != true) {
			printf("ERROR: Index %d was not received\n", i);
			errCnt++;
		}
	}

	int64_t *filterCheck;
	filterCheck = (int64_t*)calloc(dataLen, sizeof(int64_t));

	for (i = 0; i < dataLen; i++) {
		uint32_t length = ((i+1) < numTaps) ? (i+1) : numTaps;
		uint32_t j;

		// FIR Filter Processing
		for (j = 0; j < length; j++) {
			filterCheck[i] += coeffArr[j] * inputArr[i-j];
		}

		if (filterCheck[i] != outputArr[i]) {
			printf("ERROR! Index %d - %lld != %lld\n",
				i,
				(long long int)filterCheck[i],
				(long long int)outputArr[i]);
			errCnt++;
		}
	}

	if (errCnt == 0) {
		// Test Passed
		printf("PASSED\n\n");
	} else {
		// Test Failed
		printf("FAILED - Saw %d Errors\n\n", errCnt);
	}

	// Free Host Resources
	free(inputArr);
	free(outputArr);
	free(coeffArr);
	free(auState);
	free(infoState);
	free(filterCheck);

	return 0;
}
