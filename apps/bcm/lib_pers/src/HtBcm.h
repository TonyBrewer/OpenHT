//
// HtBcm interface routines
//

#define BCM_RESULT_BUF_PER_UNIT 4

class CHtBcm {
public:
	CHtBcm(ReportNonceCb_t pReportNonceCb, FreeBcmTaskCb_t pFreeBcmTaskCb);
	~CHtBcm();

	int GetUnitCnt() { return m_pHtHif->GetUnitCnt(); }
	void AddNewTask(CHtBcmTask *pTask);
	long long ScanHash(void *pThead);
	void FlushWork();
	void PauseWork(bool pause);

private:
	CHtHif *m_pHtHif;
	int m_unitCnt;
	CHtAuUnit **m_pHtUnits;
	CHtBcmRslt *m_pBcmRslt;
	int *m_pUnitTaskCnt;
	queue<CHtBcmTask *> m_newTaskQue;

	ReportNonceCb_t m_pReportNonceCb;
	void (*m_pFreeBcmTaskCb)(CHtBcmTask *);
};
