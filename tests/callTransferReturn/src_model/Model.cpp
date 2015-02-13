#include <string.h>
#include "Ht.h"

void HtCoprocModel()
{
	CHtModelHif *pModel = new CHtModelHif;
	CHtModelAuUnit *pAuUnit = pModel->AllocAuUnit();

	bool halted = false;

	do {
		uint64_t data = 0;
		if (pAuUnit->RecvCall_htmain(data)) {
			data = (data << 4) | 0xb;
			data = (data << 4) | 0xc;
			data = (data << 4) | 0xd;
			data = (data << 4) | 0xe;
			data = (data << 4) | 0xf;
			pAuUnit->SendReturn_htmain(data);
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
