#pragma once

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef long long int64_t;
typedef int int32_t;
typedef short int16_t;
typedef char int8_t;

template<int W>
class ht_uint {
public:
	ht_uint() {}
	ht_uint(uint64_t value) { m_value = value; }
	ht_uint(uint32_t value) { m_value = value; }
	ht_uint(int64_t value) { m_value = value; }
	ht_uint(int32_t value) { m_value = value; }

	uint64_t operator = (uint64_t value) { m_value = value; return *this; }
	uint64_t operator |= (uint64_t value) { m_value |= value; return *this; }
	uint64_t operator &= (uint64_t value) { m_value &= value; return *this; }
	uint64_t operator ^= (uint64_t value) { m_value ^= value; return *this; }

	bool operator == (ht_uint const & rhs) const { return m_value == rhs.m_value; }
	bool operator == (uint64_t rhs) const { return m_value == rhs; }
	bool operator == (int rhs) const { return m_value == rhs; }

	template<int W2>
	uint64_t operator , (ht_uint<W2> rhs) {
		return (m_value << W2) | rhs;
	}

	operator uint64_t() { return m_value; }
#ifndef HT_NO_PRIVATE
private:
#endif
	uint64_t	m_value:W;
}
#ifndef WIN32
__attribute__((packed))
#endif
	;


template<int W>
uint64_t operator , (uint64_t lhs, ht_uint<W> rhs) {
	return (lhs << W) | rhs;
}

template<int W>
class ht_int {
public:
	ht_int() {}
	ht_int(uint64_t value) { m_value = value; }
	ht_int(uint32_t value) { m_value = value; }
	ht_int(int64_t value) { m_value = value; }
	ht_int(int32_t value) { m_value = value; }

	int64_t operator = (int64_t value) { m_value = value; return *this; }
	int64_t operator |= (int64_t value) { m_value |= value; return *this; }
	int64_t operator &= (int64_t value) { m_value &= value; return *this; }
	int64_t operator ^= (int64_t value) { m_value ^= value; return *this; }

	ht_int operator * (ht_int & rhs) { ht_int rslt; Mul(rslt, *this, rhs); return rslt; }
	static void Mul(ht_int<W> & rslt, ht_int<W> const & lhs, ht_int<W> const & rhs);

	ht_int operator + (ht_int & rhs) { return m_value + rhs.m_value; }

	static ht_int fma(ht_int & a, ht_int  b, ht_int  c) {
		return a.m_value * b.m_value + c.m_value;
	}

	bool operator == (ht_int const & rhs) const { return m_value == rhs.m_value; }

	operator int64_t() { return m_value; }
#ifndef HT_NO_PRIVATE
private:
#endif
	int64_t	m_value:W;
}
#ifndef WIN32
__attribute__((packed))
#endif
	;

#pragma htv prim
#pragma htv tpp "lhs trs=0 tsu=0", "rhs trs=0 tsu=0", "rslt trs=2 tco=0"
template<int W> void ht_int<W>::Mul(ht_int<W> & rslt, ht_int<W> const & lhs, ht_int<W> const & rhs) {
	rslt = ht_int<W>((int64_t)(lhs.m_value * rhs.m_value));
}

typedef ht_uint<1>		ht_uint1;
typedef ht_uint<2>		ht_uint2;
typedef ht_uint<3>		ht_uint3;
typedef ht_uint<4>		ht_uint4;
typedef ht_uint<5>		ht_uint5;
typedef ht_uint<6>		ht_uint6;
typedef ht_uint<7>		ht_uint7;
typedef ht_uint<8>		ht_uint8;
typedef ht_uint<9>		ht_uint9;
typedef ht_uint<10>		ht_uint10;
typedef ht_uint<11>		ht_uint11;
typedef ht_uint<12>		ht_uint12;
typedef ht_uint<13>		ht_uint13;
typedef ht_uint<14>		ht_uint14;
typedef ht_uint<15>		ht_uint15;
typedef ht_uint<16>		ht_uint16;
typedef ht_uint<17>		ht_uint17;
typedef ht_uint<18>		ht_uint18;
typedef ht_uint<19>		ht_uint19;
typedef ht_uint<20>		ht_uint20;
typedef ht_uint<21>		ht_uint21;
typedef ht_uint<22>		ht_uint22;
typedef ht_uint<23>		ht_uint23;
typedef ht_uint<24>		ht_uint24;
typedef ht_uint<25>		ht_uint25;
typedef ht_uint<26>		ht_uint26;
typedef ht_uint<27>		ht_uint27;
typedef ht_uint<28>		ht_uint28;
typedef ht_uint<29>		ht_uint29;
typedef ht_uint<30>		ht_uint30;
typedef ht_uint<31>		ht_uint31;
typedef ht_uint<32>		ht_uint32;
typedef ht_uint<33>		ht_uint33;
typedef ht_uint<34>		ht_uint34;
typedef ht_uint<35>		ht_uint35;
typedef ht_uint<36>		ht_uint36;
typedef ht_uint<37>		ht_uint37;
typedef ht_uint<38>		ht_uint38;
typedef ht_uint<39>		ht_uint39;
typedef ht_uint<40>		ht_uint40;
typedef ht_uint<41>		ht_uint41;
typedef ht_uint<42>		ht_uint42;
typedef ht_uint<43>		ht_uint43;
typedef ht_uint<44>		ht_uint44;
typedef ht_uint<45>		ht_uint45;
typedef ht_uint<46>		ht_uint46;
typedef ht_uint<47>		ht_uint47;
typedef ht_uint<48>		ht_uint48;
typedef ht_uint<49>		ht_uint49;
typedef ht_uint<50>		ht_uint50;
typedef ht_uint<51>		ht_uint51;
typedef ht_uint<52>		ht_uint52;
typedef ht_uint<53>		ht_uint53;
typedef ht_uint<54>		ht_uint54;
typedef ht_uint<55>		ht_uint55;
typedef ht_uint<56>		ht_uint56;
typedef ht_uint<57>		ht_uint57;
typedef ht_uint<58>		ht_uint58;
typedef ht_uint<59>		ht_uint59;
typedef ht_uint<60>		ht_uint60;
typedef ht_uint<61>		ht_uint61;
typedef ht_uint<62>		ht_uint62;
typedef ht_uint<63>		ht_uint63;
typedef ht_uint<64>		ht_uint64;

typedef ht_int<1>		ht_int1;
typedef ht_int<2>		ht_int2;
typedef ht_int<3>		ht_int3;
typedef ht_int<4>		ht_int4;
typedef ht_int<5>		ht_int5;
typedef ht_int<6>		ht_int6;
typedef ht_int<7>		ht_int7;
typedef ht_int<8>		ht_int8;
typedef ht_int<9>		ht_int9;
typedef ht_int<10>		ht_int10;
typedef ht_int<11>		ht_int11;
typedef ht_int<12>		ht_int12;
typedef ht_int<13>		ht_int13;
typedef ht_int<14>		ht_int14;
typedef ht_int<15>		ht_int15;
typedef ht_int<16>		ht_int16;
typedef ht_int<17>		ht_int17;
typedef ht_int<18>		ht_int18;
typedef ht_int<19>		ht_int19;
typedef ht_int<20>		ht_int20;
typedef ht_int<21>		ht_int21;
typedef ht_int<22>		ht_int22;
typedef ht_int<23>		ht_int23;
typedef ht_int<24>		ht_int24;
typedef ht_int<25>		ht_int25;
typedef ht_int<26>		ht_int26;
typedef ht_int<27>		ht_int27;
typedef ht_int<28>		ht_int28;
typedef ht_int<29>		ht_int29;
typedef ht_int<30>		ht_int30;
typedef ht_int<31>		ht_int31;
typedef ht_int<32>		ht_int32;
typedef ht_int<33>		ht_int33;
typedef ht_int<34>		ht_int34;
typedef ht_int<35>		ht_int35;
typedef ht_int<36>		ht_int36;
typedef ht_int<37>		ht_int37;
typedef ht_int<38>		ht_int38;
typedef ht_int<39>		ht_int39;
typedef ht_int<40>		ht_int40;
typedef ht_int<41>		ht_int41;
typedef ht_int<42>		ht_int42;
typedef ht_int<43>		ht_int43;
typedef ht_int<44>		ht_int44;
typedef ht_int<45>		ht_int45;
typedef ht_int<46>		ht_int46;
typedef ht_int<47>		ht_int47;
typedef ht_int<48>		ht_int48;
typedef ht_int<49>		ht_int49;
typedef ht_int<50>		ht_int50;
typedef ht_int<51>		ht_int51;
typedef ht_int<52>		ht_int52;
typedef ht_int<53>		ht_int53;
typedef ht_int<54>		ht_int54;
typedef ht_int<55>		ht_int55;
typedef ht_int<56>		ht_int56;
typedef ht_int<57>		ht_int57;
typedef ht_int<58>		ht_int58;
typedef ht_int<59>		ht_int59;
typedef ht_int<60>		ht_int60;
typedef ht_int<61>		ht_int61;
typedef ht_int<62>		ht_int62;
typedef ht_int<63>		ht_int63;
typedef ht_int<64>		ht_int64;
