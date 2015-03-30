struct AA {
	char m_c[3][2];
	short m_s;
	int m_i;
} __attribute__((packed));

struct BB {
	int m_fi1:3;
	int m_fi2:28;
	short m_fs3:1;
	short m_fs4:8;
	char m_fc5:3;
	char m_fc6:4;
	char m_fc7:2;
} __attribute__((packed));

struct CC {
	int m_ii;
	union {
		AA aa1;
		AA aa2;
		BB bb1;
		BB bb2;
		int cc;
		int dd[2];
	};
	char ee;
} __attribute__((packed));

#include "HtSyscTypes.h"
#include "HtIntTypes.h"

SC_MODULE(IfSwitch) {

	// comment
	sc_in<bool> i_clock1x;

#pragma htv tps "1x"
	ht_int15 a, b, c;
#pragma htv tpe "1x"
	ht_int15 r;

	bool &rtnBool(bool &a) { return a; }
	void IfSwitch1x();
	void IfSwitch_src();

	SC_CTOR(IfSwitch) {
		SC_METHOD(IfSwitch1x);
		sensitive << i_clock1x.pos();
	}
};

void IfSwitch::IfSwitch1x()
{
	IfSwitch_src();
}

void IfSwitch::IfSwitch_src()
{
	bool y = true;
	bool z = rtnBool(y);

	if (a == b) {
		if (b == c)
			r = a * b + c;
		else
			r = 5;
	} else
		r = c;
}
