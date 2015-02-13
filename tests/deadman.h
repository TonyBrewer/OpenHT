#pragma once

class Deadman {
public:
	Deadman(unsigned int sec = 300) {
		m_sec = sec;
		pthread_t thread;
		pthread_create(&thread, 0, threadStart, this);
	}

private:
	static void *threadStart(void *pThis) {
		((Deadman*)pThis)->threadCode();
		return 0;
	}

	void threadCode() {
		(void)sleep(m_sec);
		fprintf(stderr, "HTLIB: DEADMAN @ %u sec\n", m_sec);
		exit(-1);
	}

	unsigned int m_sec;
};
