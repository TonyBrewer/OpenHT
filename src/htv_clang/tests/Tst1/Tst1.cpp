#include "Tst1.h"

struct A {
	A() {}
	int m:5;
} a;

struct B {
	A a2;
} c;

A b[3][4][5];

void func1(int const * const * & b)
{
	if (true)
		a.m = **b;
	else
		c.a2.m = 2;
}

void func2()
{
	struct A {
		int m:4;
	};
	A a;
	a.m = 5;
	int x=4;

	sc_uint<5> s = x;
	a.m = s;

	switch (x) {
	case 4:
		x = 5;
		break;
	default:
		if (x == 3)
			break;
		x = 2;
		break;
	}
}

int func3(int d, char const * e, float)
{
	A * f = b[3][d];
	return f[*e].m + func3( b[1][2][3].m, "a", 1.234f );
}


