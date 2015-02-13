#include "Ht.h"

void HtCoprocModel()
{
  CHtModelHif *pModel = new CHtModelHif;
  CHtModelAuUnit *pAuUnit = pModel->AllocAuUnit();

  bool done = false;
  bool markerRcvd = false;
  // Loop until all AUs are done echoing data
  while (!done) {
    if(pAuUnit->RecvCall_htmain()) {
      while (!markerRcvd) {
	uint64_t temp;
	if (pAuUnit->RecvHostData(1, &temp)) {
	  while(!pAuUnit->SendHostData(1, &temp));
	}

	if (pAuUnit->RecvHostDataMarker()) {
	  markerRcvd = true;
	  while(!pAuUnit->SendReturn_htmain());
	}
      }
    }

    done = pAuUnit->RecvHostHalt();
  }

  pModel->FreeAllAuUnits();
  delete pModel;

}
