#include <Ht.h>
using namespace Ht;
#include <stdio.h>
#include "HtBcmLib.h"
#include "HtBcm.h"

static bool zeroInputFlag = false;

//
// HtBcm host routines
//

CHtBcm::CHtBcm(ReportNonceCb_t pReportNonceCb, FreeBcmTaskCb_t pFreeBcmTaskCb)
{
	// set callback routines
	m_pReportNonceCb = pReportNonceCb;
	m_pFreeBcmTaskCb = pFreeBcmTaskCb;

	// initialize host interface
	m_pHtHif = new CHtHif();
	m_unitCnt = m_pHtHif->GetUnitCnt();
	m_pHtUnits = new CHtAuUnit * [m_unitCnt];

	for (int unit = 0; unit < m_unitCnt; unit++)
		m_pHtUnits[unit] = new CHtAuUnit(m_pHtHif);

	// allocate result buffers and send addresses to units
	posix_memalign((void **)&m_pBcmRslt, 64, sizeof(CHtBcmRslt) * m_unitCnt * BCM_RESULT_BUF_PER_UNIT);

	for (int unitId = 0; unitId < m_unitCnt; unitId += 1)
		for (int bufId = 0; bufId < BCM_RESULT_BUF_PER_UNIT; bufId += 1)
			m_pHtUnits[unitId]->SendHostMsg(BCM_RSLT_BUF_AVL, (uint64_t)(m_pBcmRslt + (unitId * BCM_RESULT_BUF_PER_UNIT) + bufId));

	// allocate space for task count per unit
	m_pUnitTaskCnt = new int [m_unitCnt];
	memset(m_pUnitTaskCnt, 0, sizeof(int) * m_unitCnt);
}

CHtBcm::~CHtBcm()
{
	for (int unit = 0; unit < m_unitCnt; unit++)
		m_pHtUnits[unit]->SendHostMsg(BCM_FORCE_RETURN, true);

	free(m_pBcmRslt);
	delete[] m_pUnitTaskCnt;
	for (int unit = 0; unit < m_unitCnt; unit++)
		delete m_pHtUnits[unit];
	delete[] m_pHtUnits;
	delete m_pHtHif;
}

void CHtBcm::AddNewTask(CHtBcmTask *pTask)
{
	m_newTaskQue.push(pTask);
}

void CHtBcm::FlushWork()
{
	// Clear queue
	m_newTaskQue = queue<CHtBcmTask *>();
	// Send Return MSG to Coprocessor to abort work
	for (int unit = 0; unit < m_unitCnt; unit++)
		m_pHtUnits[unit]->SendHostMsg(BCM_FORCE_RETURN, true);
}

// a.k.a. "cool mode"
void CHtBcm::PauseWork(bool pause)
{
	zeroInputFlag = pause;
}

long long CHtBcm::ScanHash(void *pThread)
{
	bool bAvailWork = !m_newTaskQue.empty();

	// loop through all units twice, making sure each has at least one task before giving a second
	for (int unitTasks = 0; unitTasks < 2 && bAvailWork; unitTasks += 1) {
		for (int unitId = 0; unitId < m_unitCnt && bAvailWork; unitId += 1) {
			if (m_pUnitTaskCnt[unitId] == unitTasks) {
				// give new task to unit
				CHtBcmTask *pBcmTask = m_newTaskQue.front();
				m_newTaskQue.pop();

				m_pHtUnits[unitId]->SendCall_bcm((void *)pBcmTask);

				// Send zeroInputFlag
				m_pHtUnits[unitId]->SendHostMsg(BCM_ZERO_INPUT, zeroInputFlag);

				m_pUnitTaskCnt[unitId] += 1;

				bAvailWork = !m_newTaskQue.empty();
			}
		}
	}

	long long numHashes = 0;

	// check if any unit has a result, or unit has completed a task
	for (int unitId = 0; unitId < m_unitCnt; unitId += 1) {
		ERecvType eRecvType;
		do {
			eRecvType = m_pHtUnits[unitId]->RecvPoll();

			switch (eRecvType) {
			case eRecvIdle:
				break;
			case eRecvHostMsg:
			{
				uint8_t msgType;
				uint64_t msgData;
				m_pHtUnits[unitId]->RecvHostMsg(msgType, msgData);
				if (msgType == BCM_RSLT_BUF_RDY) {
					CHtBcmRslt *pBcmRslt = (CHtBcmRslt *)msgData;

					(*m_pReportNonceCb)(pThread, pBcmRslt);

					m_pHtUnits[unitId]->SendHostMsg(BCM_RSLT_BUF_AVL, msgData);
				} else if (msgType == BCM_HASHES_COMPLETED) {
					numHashes += (long long)msgData;
				}
			}
			break;
			case eRecvReturn:
			{
				void *pBcmTask = 0;
				m_pHtUnits[unitId]->RecvReturn_bcm(pBcmTask);

				if (pBcmTask != 0) {
					// task completed
					(*m_pFreeBcmTaskCb)((CHtBcmTask *)pBcmTask);

					m_pUnitTaskCnt[unitId] -= 1;
				}
			}
			break;
			default:
				assert(0);
			}
		} while (eRecvType != eRecvIdle);
	}

	return numHashes;
}
