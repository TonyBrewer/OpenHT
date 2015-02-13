#include "Ht.h"

// State Struct to keep track of information per AU
typedef struct modelAUState {
  bool	   called;
  int32_t lastIdx;
} modelAUState_t;


uint64_t doFirFilter(int64_t *inAddrBase,
		     int64_t *outAddrBase,
		     int64_t *cofAddrBase,
		     uint32_t numTaps,
		     uint64_t rcvIdx);


void HtCoprocModel() {

  // Init Coprocessor Model
  CHtModelHif *pModel = new CHtModelHif;
  CHtModelAuUnit *const *pAuUnits = pModel->AllocAllAuUnits();
  uint32_t unitCnt = pModel->GetAuUnitCnt();
  
  // Loop until all AUs are done echoing data
  uint32_t haltCnt = 0;

  // Host Messages
  int64_t *inAddrBase = NULL;
  int64_t *outAddrBase = NULL;
  int64_t *cofAddrBase = NULL;
  uint32_t numTaps = 0;

  // Init State for each AU (so we can keep track of work)
  modelAUState_t *modelAUState = (modelAUState_t *)calloc(unitCnt, sizeof(modelAUState_t));
  uint32_t au;
  for (au = 0; au < unitCnt; au++) {
    modelAUState[au].called = false;
    modelAUState[au].lastIdx = 0;
  }

  while (haltCnt < unitCnt) {
    for (au = 0; au < unitCnt; au++) {

      // Receive Host Messages
      uint8_t msgType;
      uint64_t msgData;
      if (pAuUnits[au]->RecvHostMsg(msgType, msgData)) {
	switch (msgType) {
	case INPUT_BASE:  inAddrBase  = (int64_t *)msgData; break;
	case OUTPUT_BASE: outAddrBase = (int64_t *)msgData; break;
	case COEFF_BASE:  cofAddrBase = (int64_t *)msgData; break;
	case NUM_TAPS:    numTaps     = (uint32_t)msgData; break;
	default: assert(0);
	}
      }

      uint32_t rcvAu;
      int32_t lastIdx;
      uint64_t rcvIdx = 0;
      uint64_t sndIdx = 0;
      uint32_t errs = 0;

      // Call (Wait until we receive the initial call from the Host!)
      if (pAuUnits[au]->RecvCall_htmain(rcvAu, lastIdx)) {
	modelAUState[au].lastIdx = lastIdx;
	modelAUState[au].called = true;
	printf("Model: AU %2d - Called - Last Index = %d\n", rcvAu, lastIdx);
      }

      // Processing
      if (modelAUState[au].called == true) {
	// Try to read from host
	if (pAuUnits[au]->RecvHostData(1, &rcvIdx)) {

	  // Perform FIR Filter Operation...
	  sndIdx = doFirFilter(inAddrBase, outAddrBase, cofAddrBase, numTaps, rcvIdx);
	  
	  // Echo for now
	  while (!pAuUnits[au]->SendHostData(1, &sndIdx));
	}


	if (((int32_t)rcvIdx == modelAUState[au].lastIdx) || (modelAUState[au].lastIdx == -1)) {
	  // Return to Host
	  while (!pAuUnits[au]->SendReturn_htmain(errs));
	}
      }

      haltCnt += pAuUnits[au]->RecvHostHalt();

    }
  }

  // Free Coproccessor Model
  pModel->FreeAllAuUnits();
  delete pModel;

}

uint64_t doFirFilter(int64_t *inAddrBase,
		     int64_t *outAddrBase,
		     int64_t *cofAddrBase,
		     uint32_t numTaps,
		     uint64_t rcvIdx) {

  // FIR Filter Length
  uint64_t length = ((rcvIdx+1) < numTaps) ? (rcvIdx+1) : numTaps;
  uint32_t i;
  int64_t filterOut = 0;

  // FIR Filter Processing
  for (i = 0; i < length; i++) {
    filterOut += cofAddrBase[i] * inAddrBase[rcvIdx-i];
  }

  outAddrBase[rcvIdx] = filterOut;

  return rcvIdx;
}
