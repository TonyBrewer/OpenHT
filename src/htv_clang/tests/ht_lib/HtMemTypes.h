#pragma once

template<typename T, int D>
class ht_dist_que {
public:
	void push(T data) { m_que[m_pushIdx++] = data; }
	T front() { return m_que[m_popIdx]; }
	void pop() { m_popIdx++; }
	bool empty() { return m_pushIdx == m_popIdx; }
	void clock(bool reset) { push_clock(reset); pop_clock(reset); }
	void push_clock(bool reset) { if (reset) m_pushIdx = 0; }
	void pop_clock(bool reset) { if (reset) m_popIdx = 0; }
private:
	int m_pushIdx:D;
	int m_popIdx:D;
	T m_que[1<<D];
};
