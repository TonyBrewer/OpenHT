//
// HtBcm Personality Library API
//

#ifdef __cplusplus
extern "C" {
#endif

struct __attribute__ ((aligned(64))) CHtBcmTask {
	uint8_t m_midState[32];
	uint8_t m_data[12];
	uint32_t m_initNonce;
	uint32_t m_lastNonce;
	uint32_t m_target[3];        // make a task one 64B access
};

struct CHtBcmRslt {
	uint8_t		m_midState[32];
	uint8_t		m_data[12];
	uint32_t	m_nonce;        // make a task one 64B access
	uint8_t		m_filler[16];   // make a task one 64B access
};

typedef void (*ReportNonceCb_t)(void *pThread, struct CHtBcmRslt *);
typedef void (*FreeBcmTaskCb_t)(struct CHtBcmTask *);

int HtBcmPrepare(ReportNonceCb_t pReportNonceCb, FreeBcmTaskCb_t pFreeBcmTaskCb);

void HtBcmAddNewTask(struct CHtBcmTask *pTask);

long long HtBcmScanHash(void *pThread);

void HtBcmShutdown();

void HtBcmFlushWork();

void HtBcmPauseWork(bool pause);

#ifdef __cplusplus
}
#endif
