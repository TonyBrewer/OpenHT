#pragma once


template<typename T>
class sc_in {
public:
	bool pos();
	operator T() { return m_value; }
	T read() { return m_value; }
private:
	T m_value;
};

template<typename T>
class sc_out {
public:
	sc_out<T> & operator = ( const T & value ) { m_value = value; return *this; }
private:
	T m_value;
};

template<typename T>
class sc_signal {
public:
	sc_signal<T> & operator = ( const T & value ) { m_value = value; return *this; }
	operator T() { return m_value; }
	T read() { return m_value; }
private:
	T m_value;
};

class ClockList {
public:
	void operator << (bool);
};

class ScModuleBase {
public:
	ClockList sensitive;
};

#define SC_MODULE(A) \
	_Pragma("htv SC_MODULE") \
	class A : public ScModuleBase

#define SC_CTOR(A) \
	_Pragma("htv SC_CTOR") \
	A()

#define SC_METHOD(A)  \
	_Pragma("htv SC_METHOD") \
	A()

// ht_tps - timing path start
#define ht_tps class _ht_tps_;

// ht_tpe - timing path end
#define ht_tpe class _ht_tpe_;
