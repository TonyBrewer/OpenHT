template<typename A, int I, long long D>
class C {
public:
	C() { d = D; }
	A a;
	int b:I;
	double d;
};

int d;

C<bool, 4, 25> c1;

double e;

int f1(int const & r) {
	c1.a = false;
	c1.b = 3;

	C<char, 5, 123> c2;
	c2.a = 'a';
	c2.b = 0xb;

	return c1.b + c2.b + r;
}

int f1(short r2) {
	return r2 + r3;
}

void f2() {
	short a = 3;
	f1(a);
	int b = 4;
	f1(b);
}
