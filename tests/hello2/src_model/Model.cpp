#include <string.h>

#define HT_USING_MODEL_UNIT_THREAD
#include "Ht.h"
using namespace Ht;

void Ht::HtCoprocModel(Ht::CHtHifLibBase * pHtHifLibBase)
{
	CHtModelHif *pModel = new CHtModelHif(pHtHifLibBase);

	// StartAllUnits returns when all units have finished
	pModel->StartUnitThreads<CHtModelAuUnit>();

	pModel->WaitForUnitThreads();

	delete pModel;
}

void CHtModelAuUnit::UnitThread()
{
	bool halted = false;

	do {
		uint64_t pBuf = 0;
		if (RecvCall_htmain(pBuf)) {
			(void)strcpy((char *)pBuf, "Hello");
			SendReturn_htmain();
		}

		// The CHtHif class destructor issues a "Halt" message
		// to the units to terminate execution. However the
		// model does not contain the infrastructure logic to
		// handle this message so the model must handle the "Halt".
		halted = RecvHostHalt();
	} while (!halted);
}
