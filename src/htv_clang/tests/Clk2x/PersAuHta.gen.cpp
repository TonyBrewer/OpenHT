typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef long long int64_t;
typedef int int32_t;
typedef short int16_t;
typedef char int8_t;
template<int W>
  class sc_uint {
  public:
    sc_uint<W>();
    sc_uint<W>(uint64_t value);
    sc_uint<W>(uint32_t value);
    sc_uint<W>(int64_t value);
    sc_uint<W>(int32_t value);
    uint64_t operator= (uint64_t value);
    uint64_t operator|= (uint64_t value);
    uint64_t operator&= (uint64_t value);
    uint64_t operator^= (uint64_t value);
    operator uint64_t ();
  private:
    uint64_t m_value = W;
  };
template<int W>
  class sc_int {
  public:
    int64_t operator= (int64_t value);
    int64_t operator|= (int64_t value);
    int64_t operator&= (int64_t value);
    int64_t operator^= (int64_t value);
    operator int64_t ();
  private:
    int64_t m_value = W;
  };
typedef sc_uint<1> ht_uint1;
typedef sc_uint<2> ht_uint2;
typedef sc_uint<3> ht_uint3;
typedef sc_uint<4> ht_uint4;
typedef sc_uint<5> ht_uint5;
typedef sc_uint<6> ht_uint6;
typedef sc_uint<7> ht_uint7;
typedef sc_uint<8> ht_uint8;
typedef sc_uint<9> ht_uint9;
typedef sc_uint<10> ht_uint10;
typedef sc_uint<11> ht_uint11;
typedef sc_uint<12> ht_uint12;
typedef sc_uint<13> ht_uint13;
typedef sc_uint<14> ht_uint14;
typedef sc_uint<15> ht_uint15;
typedef sc_uint<16> ht_uint16;
typedef sc_uint<17> ht_uint17;
typedef sc_uint<18> ht_uint18;
typedef sc_uint<19> ht_uint19;
typedef sc_uint<20> ht_uint20;
typedef sc_uint<21> ht_uint21;
typedef sc_uint<22> ht_uint22;
typedef sc_uint<23> ht_uint23;
typedef sc_uint<24> ht_uint24;
typedef sc_uint<25> ht_uint25;
typedef sc_uint<26> ht_uint26;
typedef sc_uint<27> ht_uint27;
typedef sc_uint<28> ht_uint28;
typedef sc_uint<29> ht_uint29;
typedef sc_uint<30> ht_uint30;
typedef sc_uint<31> ht_uint31;
typedef sc_uint<32> ht_uint32;
typedef sc_uint<33> ht_uint33;
typedef sc_uint<34> ht_uint34;
typedef sc_uint<35> ht_uint35;
typedef sc_uint<36> ht_uint36;
typedef sc_uint<37> ht_uint37;
typedef sc_uint<38> ht_uint38;
typedef sc_uint<39> ht_uint39;
typedef sc_uint<40> ht_uint40;
typedef sc_uint<41> ht_uint41;
typedef sc_uint<42> ht_uint42;
typedef sc_uint<43> ht_uint43;
typedef sc_uint<44> ht_uint44;
typedef sc_uint<45> ht_uint45;
typedef sc_uint<46> ht_uint46;
typedef sc_uint<47> ht_uint47;
typedef sc_uint<48> ht_uint48;
typedef sc_uint<49> ht_uint49;
typedef sc_uint<50> ht_uint50;
typedef sc_uint<51> ht_uint51;
typedef sc_uint<52> ht_uint52;
typedef sc_uint<53> ht_uint53;
typedef sc_uint<54> ht_uint54;
typedef sc_uint<55> ht_uint55;
typedef sc_uint<56> ht_uint56;
typedef sc_uint<57> ht_uint57;
typedef sc_uint<58> ht_uint58;
typedef sc_uint<59> ht_uint59;
typedef sc_uint<60> ht_uint60;
typedef sc_uint<61> ht_uint61;
typedef sc_uint<62> ht_uint62;
typedef sc_uint<63> ht_uint63;
typedef sc_uint<64> ht_uint64;
typedef sc_int<1> ht_int1;
typedef sc_int<2> ht_int2;
typedef sc_int<3> ht_int3;
typedef sc_int<4> ht_int4;
typedef sc_int<5> ht_int5;
typedef sc_int<6> ht_int6;
typedef sc_int<7> ht_int7;
typedef sc_int<8> ht_int8;
typedef sc_int<9> ht_int9;
typedef sc_int<10> ht_int10;
typedef sc_int<11> ht_int11;
typedef sc_int<12> ht_int12;
typedef sc_int<13> ht_int13;
typedef sc_int<14> ht_int14;
typedef sc_int<15> ht_int15;
typedef sc_int<16> ht_int16;
typedef sc_int<17> ht_int17;
typedef sc_int<18> ht_int18;
typedef sc_int<19> ht_int19;
typedef sc_int<20> ht_int20;
typedef sc_int<21> ht_int21;
typedef sc_int<22> ht_int22;
typedef sc_int<23> ht_int23;
typedef sc_int<24> ht_int24;
typedef sc_int<25> ht_int25;
typedef sc_int<26> ht_int26;
typedef sc_int<27> ht_int27;
typedef sc_int<28> ht_int28;
typedef sc_int<29> ht_int29;
typedef sc_int<30> ht_int30;
typedef sc_int<31> ht_int31;
typedef sc_int<32> ht_int32;
typedef sc_int<33> ht_int33;
typedef sc_int<34> ht_int34;
typedef sc_int<35> ht_int35;
typedef sc_int<36> ht_int36;
typedef sc_int<37> ht_int37;
typedef sc_int<38> ht_int38;
typedef sc_int<39> ht_int39;
typedef sc_int<40> ht_int40;
typedef sc_int<41> ht_int41;
typedef sc_int<42> ht_int42;
typedef sc_int<43> ht_int43;
typedef sc_int<44> ht_int44;
typedef sc_int<45> ht_int45;
typedef sc_int<46> ht_int46;
typedef sc_int<47> ht_int47;
typedef sc_int<48> ht_int48;
typedef sc_int<49> ht_int49;
typedef sc_int<50> ht_int50;
typedef sc_int<51> ht_int51;
typedef sc_int<52> ht_int52;
typedef sc_int<53> ht_int53;
typedef sc_int<54> ht_int54;
typedef sc_int<55> ht_int55;
typedef sc_int<56> ht_int56;
typedef sc_int<57> ht_int57;
typedef sc_int<58> ht_int58;
typedef sc_int<59> ht_int59;
typedef sc_int<60> ht_int60;
typedef sc_int<61> ht_int61;
typedef sc_int<62> ht_int62;
typedef sc_int<63> ht_int63;
typedef sc_int<64> ht_int64;
template<typename T, int D>
  class ht_dist_que {
  public:
    void push (T);
    T front ();
    void pop ();
    bool empty ();
    void clock (bool);
    void push_clock (bool);
    void pop_clock (bool);
  private:
  };
template<typename T>
  class sc_in {
  public:
    bool pos ();
    operator T ();
    T read ();
  private:
    T m_value;
  };
template<typename T>
  class sc_out {
  public:
    sc_out<T> & operator= (T const & value);
  private:
    T m_value;
  };
template<typename T>
  class sc_signal {
  public:
    sc_signal<T> & operator= (T const & value);
    operator T ();
    T read ();
  private:
    T m_value;
  };
class ClockList {
public:
  void operator<< (bool);
};
class ModuleBase {
public:
  ClockList sensitive;
};
struct CHtHifParams {
  uint32_t m_ctlTimerUSec;
  uint32_t m_iBlkTimerUSec;
  uint32_t m_oBlkTimerUSec;
  uint64_t m_appPersMemSize;
  uint64_t m_appUnitMemSize;
  int32_t m_numaSetCnt;
  uint8_t m_numaSetUnitCnt[4];
};
void ResetFDSE (bool & r_reset, bool const & i_reset) {
  r_reset = i_reset;
}
typedef sc_uint<1> CtlInst_t;
typedef sc_uint<1> Clk2xInst_t;
typedef sc_uint<1> Clk2xRtnInst_t;
typedef sc_uint<1> Clk1xInst_t;
typedef sc_uint<1> Clk1xRtnInst_t;
typedef sc_uint<4> Clk1xHtId_t;
struct CHtAssertIntf {
  CHtAssertIntf & operator= (CHtAssertIntf const & rhs);
  CHtAssertIntf & operator= (int zero);
  bool m_bAssert;
  bool m_bCollision;
  ht_uint8 m_module;
  ht_uint16 m_lineNum;
  ht_uint32 m_info;
};
struct CHostCtrlMsgIntf {
  union  {
    struct  {
      uint64_t m_bValid = 1;
      uint64_t m_msgType = 7;
      uint64_t m_msgData = 56;
    };
    uint64_t m_data64;
  };
};
struct CHostDataQueIntf {
  ht_uint64 m_data;
  uint64_t m_ctl = 3;
};
struct CClk2xToCtl_Clk2xRtnIntf {
  CClk2xToCtl_Clk2xRtnIntf & operator= (CClk2xToCtl_Clk2xRtnIntf const & rhs);
  CClk2xToCtl_Clk2xRtnIntf & operator= (int zero);
  CtlInst_t m_rtnInst;
};
struct CCtlToClk2x_Clk2xCallIntf {
  CtlInst_t m_rtnInst;
};
struct CClk1xToClk2x_Clk1xRtnIntf {
  Clk2xInst_t m_rtnInst;
};
struct CClk2xToClk1x_Clk1xCallIntf {
  Clk2xInst_t m_rtnInst;
};
class CPersAuHta : public ModuleBase {
  sc_in<bool> i_clock1x;
  sc_in<CHtAssertIntf> i_ctlToHta_assert;
  sc_in<CHtAssertIntf> i_clk2xToHta_assert;
  sc_in<CHtAssertIntf> i_clk1xToHta_assert;
  sc_out<CHtAssertIntf> o_htaToHif_assert;
  CHtAssertIntf r_ctlToHta_assert;
  CHtAssertIntf r_clk2xToHta_assert;
  CHtAssertIntf r_clk1xToHta_assert;
  CHtAssertIntf r_htaToHif_assert;
  void PersAuHta1x ();
  CPersAuHta()   {
    this->PersAuHta1x();
    this->sensitive << this->i_clock1x.pos();
  }
};
void CPersAuHta::PersAuHta1x () {
  CHtAssertIntf c_htaToHif_assert = CHtAssertIntf();
  c_htaToHif_assert.m_module = 0;
  if (this->r_ctlToHta_assert.m_bAssert)
    c_htaToHif_assert.m_module |= 0;
  if (this->r_clk2xToHta_assert.m_bAssert)
    c_htaToHif_assert.m_module |= 1;
  if (this->r_clk1xToHta_assert.m_bAssert)
    c_htaToHif_assert.m_module |= 2;
  c_htaToHif_assert.m_info = 0;
  c_htaToHif_assert.m_info |= this->r_ctlToHta_assert.m_info.operator uint64_t();
  c_htaToHif_assert.m_info |= this->r_clk2xToHta_assert.m_info.operator uint64_t();
  c_htaToHif_assert.m_info |= this->r_clk1xToHta_assert.m_info.operator uint64_t();
  c_htaToHif_assert.m_lineNum = 0;
  c_htaToHif_assert.m_lineNum |= this->r_ctlToHta_assert.m_lineNum.operator uint64_t();
  c_htaToHif_assert.m_lineNum |= this->r_clk2xToHta_assert.m_lineNum.operator uint64_t();
  c_htaToHif_assert.m_lineNum |= this->r_clk1xToHta_assert.m_lineNum.operator uint64_t();
  bool c_bAssert_1_0 = this->r_ctlToHta_assert.m_bAssert || this->r_clk2xToHta_assert.m_bAssert;
  bool c_bCollision_1_0 = this->r_ctlToHta_assert.m_bAssert && this->r_clk2xToHta_assert.m_bAssert;
  bool c_bAssert_1_1 = this->r_clk1xToHta_assert.m_bAssert;
  bool c_bCollision_1_1 = this->r_clk1xToHta_assert.m_bAssert;
  bool c_bAssert_2_0 = c_bAssert_1_0 || c_bAssert_1_1;
  bool c_bCollision_2_0 = c_bCollision_1_0 || c_bCollision_1_1 || c_bAssert_1_0 && c_bAssert_1_1;
  c_htaToHif_assert.m_bAssert = c_bAssert_2_0;
  c_htaToHif_assert.m_bCollision = c_bCollision_2_0;
  this->r_ctlToHta_assert = this->i_ctlToHta_assert.operator CHtAssertIntf();
  this->r_clk2xToHta_assert = this->i_clk2xToHta_assert.operator CHtAssertIntf();
  this->r_clk1xToHta_assert = this->i_clk1xToHta_assert.operator CHtAssertIntf();
  this->r_htaToHif_assert = c_htaToHif_assert;
  this->o_htaToHif_assert = this->r_htaToHif_assert;
}
