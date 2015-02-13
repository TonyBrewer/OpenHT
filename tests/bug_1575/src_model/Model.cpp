#include "Ht.h"

void HtCoprocModel() {
  CHtModelHif *pModel = new CHtModelHif;
  CHtModelAuUnit *pAuUnit = pModel->AllocAuUnit();
  
  bool halted = false;

  uint32_t sum = 0, len = 0, i = 0;

  do {
    if (pAuUnit->RecvCall_htmain(len)) {
      for (i = 0; i < len; i++) {
	sum += i;
      }
      
      pAuUnit->SendReturn_htmain(sum);
    }
    
    // The CHtHif class destructor issues a "Halt" message
    // to the units to terminate execution. However the
    // model does not contain the infrastructure logic to
    // handle this message so the model must handle the "Halt".
    halted = pAuUnit->RecvHostHalt();
  } while (!halted);
  
  pModel->FreeAuUnit(pAuUnit);
  delete pModel;
}
