#include <Ht.h>
using namespace Ht;
#include "HtBcmLib.h"
#include "HtBcm.h"

static CHtBcm *g_pHtBcm;

//
// Library interface routines
//

int HtBcmPrepare(ReportNonceCb_t pReportNonceCb, FreeBcmTaskCb_t pFreeBcmTaskCb)
{
	g_pHtBcm = new CHtBcm(pReportNonceCb, pFreeBcmTaskCb);

	return g_pHtBcm->GetUnitCnt();
}

void HtBcmAddNewTask(CHtBcmTask *pTask)
{
	g_pHtBcm->AddNewTask(pTask);
}

long long HtBcmScanHash(void *pThread)
{
	return g_pHtBcm->ScanHash(pThread);
}

void HtBcmShutdown()
{
	delete g_pHtBcm;
}

void HtBcmFlushWork()
{
	g_pHtBcm->FlushWork();
}

void HtBcmPauseWork(bool pause)
{
	g_pHtBcm->PauseWork(pause);
}
