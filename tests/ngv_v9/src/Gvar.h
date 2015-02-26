#pragma once

#include <stdint.h>

union CSubFld {
	struct {
		char m_sa : 1;
		char m_sb : 3;
		char m_sc : 2;
	};
	char m_s;
};

struct CGVar {
	char m_a;
	short m_b[2];
	CSubFld m_c[3];
	uint32_t m_d;
	uint64_t m_e[12];
};
