#include "Ht.h"
using namespace Ht;

class CHtAuUnitEx : public CHtAuUnit {
public:
	CHtAuUnitEx(CHtHif * pHtHif, int numaSet=-1) : CHtAuUnit(pHtHif, numaSet) {}
	void RecvCallback( Ht::ERecvType );
	void UnitThread();
private:
	bool volatile m_bHostDone;
	int m_maxRecvCnt;
	int m_totalRcvd;
	int m_rcvdCnt;
	uint64_t m_data[5];
};

class CHtHifEx : public CHtHif {
public:
	CHtHifEx(CHtHifParams * pParams) : CHtHif(pParams) {}
	//CHtAuUnitEx * NewAuUnit(int numaSet=-1) { return new CHtAuUnitEx(this, numaSet); }
};

int main(int argc, char **argv)
{
	for (int i = 0; i < 100; i += 1) {
		CHtHifParams params;
		//params.m_iBlkTimerUSec = 0;
		//params.m_oBlkTimerUSec = 0;

		CHtHifEx *pHtHif = new CHtHifEx(&params);

		pHtHif->StartUnitThreads<CHtAuUnitEx>();

		pHtHif->WaitForUnitThreads();

		delete pHtHif;

		printf("loop %d\n", i);
	}

	// Either hang or pass
	printf("PASSED\n");
}

void CHtAuUnitEx::UnitThread()
{
	m_bHostDone = false;
	m_maxRecvCnt = 1;
	m_totalRcvd = 0;
	m_rcvdCnt = 0;

	SetUsingCallback(true);

	int pushCnt = 1;
	bool bCallFlag = true;

	uint64_t data[5];
	for (int i = 0; i < TEST_CNT; ) {
		// push some data
		int cnt = min(pushCnt, TEST_CNT - i);
		for (int j = 0; j < cnt; j += 1)
			data[j] = (i + j) * 1;

		i += cnt;

		int sentCnt = 0;
		while (sentCnt < cnt)
			sentCnt += SendHostData(cnt - sentCnt, data + sentCnt);

		// now either call or send msg
		if (bCallFlag) {
			uint64_t in1 = i * 7;
			uint64_t in2 = i * 11;
			uint64_t in3 = i * 3;
			uint64_t in4 = i * 5;
			while (!SendCall_htmain(in1, in2, in3, in4));
		} else {
			SendHostMsg(MODEL_MSG, i * 31);
		}

		bCallFlag = !bCallFlag;

		if (++pushCnt > 5)
			pushCnt = 1;

		RecvPoll();
	}

	// finished sending to coproc, wait for receive to finish
	while (!m_bHostDone)
		RecvPoll();
}

void CHtAuUnitEx::RecvCallback(Ht::ERecvType recvType)
{
	switch (recvType) {
	case Ht::eRecvHostMsg:
		{
			uint8_t msgType;
			uint64_t msgData;

			RecvHostMsg(msgType, msgData);
		}
		break;
	case Ht::eRecvReturn:
		{
			uint64_t out1, out2, out3, out4;

			RecvReturn_htmain(out1, out2, out3, out4);
		}
		break;
	case Ht::eRecvHostData:
		{
			// must be re-entrant
			int cnt = min(TEST_CNT - m_totalRcvd, m_maxRecvCnt);

			int tmpCnt = RecvHostData(m_maxRecvCnt - m_rcvdCnt, m_data + m_rcvdCnt);

			for (int i = 0; i < tmpCnt; i += 1)
				assert(m_data[m_rcvdCnt + i] == m_totalRcvd + i);

			m_rcvdCnt += tmpCnt;
			m_totalRcvd += tmpCnt;

			if (m_rcvdCnt == m_maxRecvCnt) {
				if (++m_maxRecvCnt > 5)
					m_maxRecvCnt = 1;

				m_rcvdCnt = 0;
			}

			if (m_totalRcvd == TEST_CNT)
				m_bHostDone = true;
		}
		break;
	default:
		assert(0);
	}
}
