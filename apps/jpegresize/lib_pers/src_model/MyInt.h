/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

// My variable width integer classes

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

////////////////////////////////////////////////////////////////////////////////////////
// my_int

#ifdef MY_UINT
class my_uint_temp {
public:
	my_uint_temp(uint64_t value, int width) : m_value(value), m_width(width) {}

	operator uint64_t () { return m_value; }

	int width() { return m_width; }
	uint64_t mask() { return ~0ULL >> (64 - width()); }
	uint64_t value() { return m_value & mask(); }

	my_uint_temp operator ~ () {
		return my_uint_temp(~m_value & mask(), width());
	}

private:
	uint64_t m_value;
	int m_width;
};

class my_uint_ref {
public:
	my_uint_ref(uint64_t &value, int hi, int lo) : m_value(value), m_hi(hi), m_lo(lo) {}

	operator uint64_t () { return m_value >> m_lo; }

	uint64_t operator = (uint64_t value) {
		uint64_t posMask = mask() << m_lo;
		m_value &= ~posMask;
		m_value |= posMask & (value << m_lo);
		return value;
	}

	my_uint_temp operator ~ () {
		return my_uint_temp(~(m_value >> m_lo) & mask(), width());
	}

	int width() { return m_hi - m_lo + 1; }
	uint64_t mask() { return ~0ULL >> (64 - width()); }
	uint64_t value() { return (m_value >> m_lo) & mask(); }

private:
	uint64_t &m_value;
	int m_hi;
	int m_lo;
};

template<int W>
class my_uint {
public:
	my_uint() {}
	my_uint(uint64_t value) { m_value = value; }
	my_uint(uint32_t value) { m_value = value; }
	my_uint(int64_t value) { m_value = value; }
	my_uint(int32_t value) { m_value = value; }

	uint64_t operator = (uint64_t value) { m_value = value; return *this; }
	uint64_t operator |= (uint64_t value) { m_value |= value; return *this; }
	uint64_t operator &= (uint64_t value) { m_value &= value; return *this; }
	uint64_t operator ^= (uint64_t value) { m_value ^= value; return *this; }

	bool operator == (my_uint const & rhs) const { return m_value == rhs.m_value; }
	bool operator == (uint64_t rhs) const { return m_value == rhs; }
	bool operator == (int rhs) const { return m_value == rhs; }

	bool operator >= (my_uint const & rhs) const { return m_value >= rhs.m_value; }
	bool operator >= (uint64_t rhs) const { return m_value >= rhs; }

	my_uint<W> operator ~ () {
		return ~m_value;
	}
	my_uint_ref operator [] (int bit) {
		assert(bit < W);
		assert(0 <= bit);
		return my_uint_ref(m_value64, bit, bit);
	}

	my_uint_ref operator () (int hi, int lo) {
		assert(hi < W);
		assert(lo <= hi);
		return my_uint_ref(m_value64, hi, lo);
	}

	operator uint64_t() { return m_value; }

	int width() { return W; }
	uint64_t mask() { return ~0ULL >> (64 - width()); }
	uint64_t value() { return m_value; }

private:
	union {
		uint64_t	m_value:W;
		uint64_t	m_value64;
	};
};

// Concatenate operator
template<int LW, int RW>
my_uint<LW+RW> operator , (my_uint<LW> lhs, my_uint<RW> rhs) {
		return my_uint<LW+RW>( (lhs.value() << rhs.width()) | rhs.value() );
}

template<int RW>
my_uint_temp operator , (my_uint_ref lhs, my_uint<RW> rhs) {
		return my_uint_temp( (lhs.value() << rhs.width()) | rhs.value(), lhs.width() + rhs.width() );
}

template<int RW>
my_uint_temp operator , (my_uint_temp lhs, my_uint<RW> rhs) {
		return my_uint_temp( (lhs.value() << rhs.width()) | rhs.value(), lhs.width() + rhs.width() );
}

template<int LW>
my_uint_temp operator , (my_uint<LW> lhs, my_uint_ref rhs) {
		return my_uint_temp( (lhs.value() << rhs.width()) | rhs.value(), lhs.width() + rhs.width() );
}

my_uint_temp operator , (my_uint_ref lhs, my_uint_ref rhs) {
		return my_uint_temp( (lhs.value() << rhs.width()) | rhs.value(), lhs.width() + rhs.width() );
}

my_uint_temp operator , (my_uint_temp lhs, my_uint_ref rhs) {
		return my_uint_temp( (lhs.value() << rhs.width()) | rhs.value(), lhs.width() + rhs.width() );
}

template<int LW>
my_uint_temp operator , (my_uint<LW> lhs, my_uint_temp rhs) {
		return my_uint_temp( (lhs.value() << rhs.width()) | rhs.value(), lhs.width() + rhs.width() );
}

my_uint_temp operator , (my_uint_ref lhs, my_uint_temp rhs) {
		return my_uint_temp( (lhs.value() << rhs.width()) | rhs.value(), lhs.width() + rhs.width() );
}

my_uint_temp operator , (my_uint_temp lhs, my_uint_temp rhs) {
		return my_uint_temp( (lhs.value() << rhs.width()) | rhs.value(), lhs.width() + rhs.width() );
}
#endif

////////////////////////////////////////////////////////////////////////////////////////
// my_uint

template<int W>
class my_uint {
public:
	my_uint() {}
	my_uint(uint64_t value) { assert(value < (1ULL << width())); m_value = value; }
	my_uint(uint32_t value) { assert(value < (1ULL << width())); m_value = value; }
	my_uint(int64_t value) { assert(value >= 0 && value < (1LL << width())); m_value = value; }
	my_uint(int32_t value) { assert(value >= 0 && value < (1LL << width())); m_value = value; }

	uint64_t operator = (uint64_t value) { assert(value < (1ULL << width())); m_value = value; return m_value; }
	uint64_t operator = (uint32_t value) { assert(value < (1ULL << width())); m_value = value; return m_value; }
	uint64_t operator = (int64_t value) { assert(value >= 0 && value < (1LL << width())); m_value = value; return m_value; }
	uint64_t operator = (int32_t value) { assert(value >= 0 && value < (1LL << width())); m_value = value; return m_value; }

	uint64_t operator |= (uint64_t value) { m_value |= value; return m_value; }
	uint64_t operator &= (uint64_t value) { m_value &= value; return m_value; }
	uint64_t operator ^= (uint64_t value) { m_value ^= value; return m_value; }
	uint64_t operator >>= (uint64_t value) { m_value >>= value; return m_value; }
	uint64_t operator <<= (uint64_t value) { m_value <<= value; return m_value; }

	uint64_t operator += (uint64_t value) { assert(value < (1ULL << width())); m_value += value; return m_value; }
	uint64_t operator -= (uint64_t value) { assert(value < (1ULL << width())); m_value -= value; return m_value; }

	bool operator == (my_uint const & rhs) const { return m_value == rhs.m_value; }
	bool operator == (uint64_t rhs) const { return m_value == rhs; }
	bool operator == (uint32_t rhs) const { return m_value == rhs; }
	bool operator == (int32_t rhs) const { return m_value == rhs; }

	my_uint<W> operator ~ () { return ~m_value; }

	operator uint64_t() { return m_value; }

	int width() { return W; }
	uint64_t mask() { return ~0ULL >> (64 - width()); }
	uint64_t value() { return m_value; }

private:
	union {
		uint64_t	m_value:W;
		uint64_t	m_value64;
	};
};

typedef my_uint<1>		my_uint1;
typedef my_uint<2>		my_uint2;
typedef my_uint<3>		my_uint3;
typedef my_uint<4>		my_uint4;
typedef my_uint<5>		my_uint5;
typedef my_uint<6>		my_uint6;
typedef my_uint<7>		my_uint7;
typedef my_uint<8>		my_uint8;
typedef my_uint<9>		my_uint9;
typedef my_uint<10>		my_uint10;
typedef my_uint<11>		my_uint11;
typedef my_uint<12>		my_uint12;
typedef my_uint<13>		my_uint13;
typedef my_uint<14>		my_uint14;
typedef my_uint<16>		my_uint16;
typedef my_uint<17>		my_uint17;

typedef my_uint<20>		my_uint20;

typedef my_uint<26>		my_uint26;
typedef my_uint<32>		my_uint32;

////////////////////////////////////////////////////////////////////////////////////////
// my_int

template<int W>
class my_int {
public:
	my_int() {}
	my_int(uint64_t value) { assert(value < (1ULL << (width()-1))); m_value = value; }
	my_int(uint32_t value) { assert(value < (1ULL << (width()-1))); m_value = value; }
	my_int(int64_t value) { assert(value >= -(1LL << (width()-1)) && value < (1LL << (width()-1))); m_value = value; }
	my_int(int32_t value) { assert(value >= -(1LL << (width()-1)) && value < (1LL << (width()-1))); m_value = value; }

	int64_t operator = (uint64_t value) { assert(value < (1ULL << (width()-1))); m_value = value; return m_value; }
	int64_t operator = (uint32_t value) { assert(value < (1ULL << (width()-1))); m_value = value; return m_value; }
	int64_t operator = (int64_t value) { assert(value >= -(1LL << (width()-1)) && value < (1LL << (width()-1))); m_value = value; return m_value; }
	int64_t operator = (int32_t value) { assert(value >= -(1LL << (width()-1)) && value < (1LL << (width()-1))); m_value = value; return m_value; }

	int64_t operator |= (int64_t value) { m_value |= value; return m_value; }
	int64_t operator &= (int64_t value) { m_value &= value; return m_value; }
	int64_t operator ^= (int64_t value) { m_value ^= value; return m_value; }

	int64_t operator += (int64_t value) { m_value += value; return m_value; }
	int64_t operator -= (int64_t value) { m_value -= value; return m_value; }
	int64_t operator >>= (int64_t value) { m_value >>= value; return m_value; }

	bool operator == (my_int const & rhs) const { return m_value == rhs.m_value; }
	bool operator == (int64_t rhs) const { return m_value == rhs; }
	bool operator == (int rhs) const { return m_value == rhs; }

	my_int<W> operator ~ () { return ~m_value; }

	operator int64_t() { return m_value; }

	int width() { return W; }
	int64_t mask() { return ~0ULL >> (64 - width()); }
	int64_t value() { return m_value; }

private:
	union {
		int64_t	m_value:W;
		int64_t	m_value64;
	};
};

typedef my_int<1>		my_int1;
typedef my_int<2>		my_int2;
typedef my_int<3>		my_int3;
typedef my_int<4>		my_int4;
typedef my_int<5>		my_int5;
typedef my_int<6>		my_int6;
typedef my_int<7>		my_int7;
typedef my_int<8>		my_int8;
typedef my_int<9>		my_int9;
typedef my_int<10>		my_int10;
typedef my_int<11>		my_int11;
typedef my_int<12>		my_int12;
typedef my_int<13>		my_int13;
typedef my_int<14>		my_int14;
typedef my_int<15>		my_int15;
typedef my_int<16>		my_int16;
typedef my_int<17>		my_int17;
typedef my_int<18>		my_int18;
typedef my_int<19>		my_int19;
typedef my_int<20>		my_int20;
typedef my_int<21>		my_int21;
typedef my_int<22>		my_int22;
typedef my_int<23>		my_int23;
typedef my_int<24>		my_int24;
typedef my_int<25>		my_int25;
typedef my_int<26>		my_int26;
typedef my_int<27>		my_int27;
typedef my_int<28>		my_int28;

typedef my_int<30>		my_int30;
typedef my_int<31>		my_int31;
