#pragma once

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef int int32_t;

template<int W>
class sc_uint {
public:
	sc_uint();
	sc_uint(uint64_t value);
	sc_uint(uint32_t value);
	sc_uint(int64_t value);
	sc_uint(int32_t value);

	uint64_t operator = (uint64_t value);
	uint64_t operator |= (uint64_t value);
	uint64_t operator &= (uint64_t value);
	uint64_t operator ^= (uint64_t value);
	operator uint64_t();
private:
	uint64_t m_value:W;
};
