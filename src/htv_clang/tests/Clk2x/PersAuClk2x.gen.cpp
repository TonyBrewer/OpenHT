// List of fileNum / pathName used for temporary variables
//  1  C:/Users/TBrewer/Documents/llvm-project/my_tests/ht_lib/HtIntTypes.h
//  2  C:/Users/TBrewer/Documents/llvm-project/my_tests/ht_lib/HtMemTypes.h
//  3  C:/Users/TBrewer/Documents/llvm-project/my_tests/Clk2x/PersAuCommon.h
//  4  C:/Users/TBrewer/Documents/llvm-project/my_tests/ht_lib/HtSyscTypes.h
//  5  C:/Users/TBrewer/Documents/llvm-project/my_tests/Clk2x\HostIntf.h
//  6  C:/Users/TBrewer/Documents/llvm-project/my_tests/ht_lib\Ht.h
//  7  C:/Users/TBrewer/Documents/llvm-project/my_tests/Clk2x/PersAuClk2x.h
//  8  C:/Users/TBrewer/Documents/llvm-project/my_tests/Clk2x/PersAuClk2x_src.cpp
//  9  C:\Users\TBrewer\Documents\llvm-project\my_tests\Clk2x\PersAuClk2x.cpp

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
    uint64_t m_value : W;
  };

template<int W>
  class sc_int {
  public:
    int64_t m_value : W;
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
    int m_pushIdx : D;
    int m_popIdx : D;
    T m_que[1 << D];
  };

template<typename T>
  class sc_in {
  public:
    T m_value;
  };

template<typename T>
  class sc_out {
  public:
    T m_value;
  };

template<typename T>
  class sc_signal {
  public:
    T m_value;
  };

class ClockList {
public:
};

class ScModuleBase {
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

typedef sc_uint<1> CtlInst_t;
typedef sc_uint<1> Clk2xInst_t;
typedef sc_uint<1> Clk2xRtnInst_t;
typedef sc_uint<1> Clk1xInst_t;
typedef sc_uint<1> Clk1xRtnInst_t;
typedef sc_uint<4> Clk1xHtId_t;
struct CHtAssertIntf {
  bool m_bAssert;
  bool m_bCollision;
  uint8_t m_module;
  uint16_t m_lineNum;
  uint32_t m_info;
};

struct CHostCtrlMsgIntf {
  union  {
    struct  {
      uint64_t m_bValid : 1;
      uint64_t m_msgType : 7;
      uint64_t m_msgData : 56;
    };

    uint64_t m_data64;
  };

};

struct CHostDataQueIntf {
  ht_uint64 m_data;
  uint64_t m_ctl : 3;
};

struct CClk2xToCtl_Clk2xRtnIntf {
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

class CPersAuClk2x : public ScModuleBase {
public:
  sc_in<bool> i_clock1x;
  sc_in<bool> i_clock2x;
  sc_in<bool> i_reset;
  sc_out<CHtAssertIntf> o_clk2xToHta_assert;
  sc_in<CHostCtrlMsgIntf> i_hifToAu_ihm;
  sc_out<CHostCtrlMsgIntf> o_clk2xToHti_ohm;
  sc_in<bool> i_htiToClk2x_ohmAvl;
  sc_in<bool> i_ctlToClk2x_Clk2xCallRdy;
  sc_in<CCtlToClk2x_Clk2xCallIntf> i_ctlToClk2x_Clk2xCall;
  sc_out<bool> o_clk2xToCtl_Clk2xCallAvl;
  sc_out<bool> o_clk2xToClk1x_Clk1xCallRdy;
  sc_out<CClk2xToClk1x_Clk1xCallIntf> o_clk2xToClk1x_Clk1xCall;
  sc_in<bool> i_clk1xToClk2x_Clk1xCallAvl;
  sc_out<bool> o_clk2xToCtl_Clk2xRtnRdy;
  sc_out<CClk2xToCtl_Clk2xRtnIntf> o_clk2xToCtl_Clk2xRtn;
  sc_in<bool> i_ctlToClk2x_Clk2xRtnAvl;
  sc_in<bool> i_clk1xToClk2x_Clk1xRtnRdy;
  sc_in<CClk1xToClk2x_Clk1xRtnIntf> i_clk1xToClk2x_Clk1xRtn;
  sc_out<bool> o_clk2xToClk1x_Clk1xRtnAvl;
  sc_signal<CHostCtrlMsgIntf> r_hifToAu_ihm_1x;
  CHostCtrlMsgIntf r_clk2xToHti_ohm_1x;
  sc_signal<CHostCtrlMsgIntf> r_clk2xToHti_ohm_2x;
  sc_signal<bool> r_htiToClk2x_ohmAvl_1x;
  bool r_clk2xToHti_ohmAvlCnt;
  CHostCtrlMsgIntf c_clk2xToHti_ohm;
  sc_uint<1> c_clk2xToHti_ohmAvlCnt;
  ht_dist_que<CCtlToClk2x_Clk2xCallIntf, 5> m_ctlToClk2x_Clk2xCallQue;
  ht_dist_que<CClk1xToClk2x_Clk1xRtnIntf, 5> m_clk1xToClk2x_Clk1xRtnQue;
  bool c_clk2xToClk1x_Clk1xCallRdy;
  CClk2xToClk1x_Clk1xCallIntf c_clk2xToClk1x_Clk1xCall;
  bool c_clk2xToCtl_Clk2xRtnRdy;
  CClk2xToCtl_Clk2xRtnIntf c_clk2xToCtl_Clk2xRtn;
  bool r_clk2xToCtl_Clk2xCallAvl;
  sc_signal<bool> r_clk2xToClk1x_Clk1xCallRdy;
  bool r_clk2xToClk1x_Clk1xCallRdy_1x;
  sc_uint<6> r_clk2xToClk1x_Clk1xCallAvlCnt;
  bool r_clk2xToClk1x_Clk1xCallAvlCntZero;
  bool r_clk2xToCtl_Clk2xRtnRdy;
  sc_uint<6> r_clk2xToCtl_Clk2xRtnAvlCnt;
  bool r_clk2xToCtl_Clk2xRtnAvlCntZero;
  bool r_clk2xToClk1x_Clk1xRtnAvl_1x;
  sc_signal<sc_uint<6>> r_clk2xToClk1x_Clk1xRtnAvlCnt_2x;
  sc_signal<CClk2xToClk1x_Clk1xCallIntf> r_clk2xToClk1x_Clk1xCall;
  CClk2xToClk1x_Clk1xCallIntf r_clk2xToClk1x_Clk1xCall_1x;
  CClk2xToCtl_Clk2xRtnIntf r_clk2xToCtl_Clk2xRtn;
  struct CHtCmd {
    uint32_t m_htInstr : 1;
  };

  struct CHtPriv {
    Clk2xRtnInst_t m_rtnInst;
  };

  CHtCmd r_htCmd;
  CHtPriv r_htPriv;
  bool r_resetComplete;
  sc_uint<2> r_t2_htSel;
  CCtlToClk2x_Clk2xCallIntf r_t2_ctlToClk2x_Clk2xCall;
  CClk1xToClk2x_Clk1xRtnIntf r_t2_clk1xToClk2x_Clk1xRtn;
  bool r_t2_htValid;
  CHtCmd r_t2_htCmd;
  bool r_t3_htValid;
  sc_uint<2> r_t3_htSel;
  CHtPriv r_t3_htPriv;
  sc_uint<1> r_t3_htInstr;
  CHtPriv c_t3_htPriv;
  sc_uint<4> c_t3_htCtrl;
  sc_uint<1> c_t3_htNextInstr;
  ht_uint9 c_msg;
  ht_uint9 r_msg;
  CHtAssertIntf c_htAssert;
  bool r_htBusy;
  bool r_htCmdValid;
  sc_signal<CHtAssertIntf> r_clk2xToHta_assert;
  CHtAssertIntf r_clk2xToHta_assert_1x;
  bool r_phase;
  bool r_reset1x;
  sc_signal<bool> c_reset1x;
  void PersAuClk2x_1x ();
  void PersAuClk2x_2x ();
};

void CPersAuClk2x::PersAuClk2x_1x () {
  bool rv$9$117;
  {
    rv$9$117 = i_clk1xToClk2x_Clk1xRtnRdy.m_value;
  }
  if (rv$9$117)
    {
      CClk1xToClk2x_Clk1xRtnIntf rv$9$118;
      CClk1xToClk2x_Clk1xRtnIntf rv$9$118b;
      {
        CClk1xToClk2x_Clk1xRtnIntf rv$4$8;
        CClk1xToClk2x_Clk1xRtnIntf const & pv$4$8 = i_clk1xToClk2x_Clk1xRtn.m_value;
        {
          Clk2xInst_t rv$3$327;
          sc_uint<1> const & pv$3$327 = pv$4$8.m_rtnInst;
          {
            rv$3$327.m_value = pv$3$327.m_value;
          }
          rv$4$8.m_rtnInst = rv$3$327;
        }
        rv$9$118b = rv$4$8;
      }
      CClk1xToClk2x_Clk1xRtnIntf pv$9$118 = rv$9$118b;
      {
        Clk2xInst_t rv$3$327b;
        sc_uint<1> pv$3$327b = pv$9$118.m_rtnInst;
        {
          rv$3$327b.m_value = pv$3$327b.m_value;
        }
        rv$9$118.m_rtnInst = rv$3$327b;
      }
      CClk1xToClk2x_Clk1xRtnIntf data$9$118 = rv$9$118;
      {
        CClk1xToClk2x_Clk1xRtnIntf const & pv$2$6 = data$9$118;
        {
          sc_uint<1> const & pv$3$327c = pv$2$6.m_rtnInst;
          {
            m_clk1xToClk2x_Clk1xRtnQue.m_que[++m_clk1xToClk2x_Clk1xRtnQue.m_pushIdx].m_rtnInst.m_value = pv$3$327c.m_value;
          }
        }
      }
    }
  CHtAssertIntf rv$9$123;
  {
    CHtAssertIntf rv$4$26;
    CHtAssertIntf const & pv$4$26 = r_clk2xToHta_assert.m_value;
    {
      rv$4$26.m_info = pv$4$26.m_info;
      rv$4$26.m_lineNum = pv$4$26.m_lineNum;
      rv$4$26.m_module = pv$4$26.m_module;
      rv$4$26.m_bCollision = pv$4$26.m_bCollision;
      rv$4$26.m_bAssert = pv$4$26.m_bAssert;
    }
    rv$9$123 = rv$4$26;
  }
  CHtAssertIntf const & rhs$9$123 = rv$9$123;
  {
    r_clk2xToHta_assert_1x.m_bAssert = rhs$9$123.m_bAssert;
    r_clk2xToHta_assert_1x.m_bCollision = rhs$9$123.m_bCollision;
    r_clk2xToHta_assert_1x.m_module = rhs$9$123.m_module;
    r_clk2xToHta_assert_1x.m_lineNum = rhs$9$123.m_lineNum;
    r_clk2xToHta_assert_1x.m_info = rhs$9$123.m_info;
  }
  CHostCtrlMsgIntf rv$9$125;
  {
    CHostCtrlMsgIntf rv$4$8b;
    CHostCtrlMsgIntf const & pv$4$8b = i_hifToAu_ihm.m_value;
    {
      CHostCtrlMsgIntf rv$3$145;
      CHostCtrlMsgIntf const & pv$3$145 = pv$4$8b;
      {
      }
      rv$4$8b = rv$3$145;
    }
    rv$9$125 = rv$4$8b;
  }
  CHostCtrlMsgIntf const & value$9$125 = rv$9$125;
  {
    CHostCtrlMsgIntf const & pv$4$25 = value$9$125;
    {
      CHostCtrlMsgIntf const & pv$3$145b = pv$4$25;
      {
        CHostCtrlMsgIntf const & pv$3$149 = pv$3$145b;
        {
          r_hifToAu_ihm_1x.m_value.m_bValid = pv$3$149.m_bValid;
          r_hifToAu_ihm_1x.m_value.m_msgType = pv$3$149.m_msgType;
          r_hifToAu_ihm_1x.m_value.m_msgData = pv$3$149.m_msgData;
        }
        r_hifToAu_ihm_1x.m_value.m_data64 = pv$3$145b.m_data64;
      }
    }
  }
  bool rv$9$127;
  {
    rv$9$127 = i_htiToClk2x_ohmAvl.m_value;
  }
  bool const & value$9$127 = rv$9$127;
  {
    r_htiToClk2x_ohmAvl_1x.m_value = value$9$127;
  }
  CHostCtrlMsgIntf rv$9$128;
  {
    CHostCtrlMsgIntf rv$4$26b;
    CHostCtrlMsgIntf const & pv$4$26b = r_clk2xToHti_ohm_2x.m_value;
    {
      CHostCtrlMsgIntf rv$3$145b;
      CHostCtrlMsgIntf const & pv$3$145c = pv$4$26b;
      {
      }
      rv$4$26b = rv$3$145b;
    }
    rv$9$128 = rv$4$26b;
  }
  CHostCtrlMsgIntf pv$9$128 = rv$9$128;
  {
    CHostCtrlMsgIntf pv$3$145d = pv$9$128;
    {
      CHostCtrlMsgIntf pv$3$149b = pv$3$145d;
      {
        r_clk2xToHti_ohm_1x.m_bValid = pv$3$149b.m_bValid;
        r_clk2xToHti_ohm_1x.m_msgType = pv$3$149b.m_msgType;
        r_clk2xToHti_ohm_1x.m_msgData = pv$3$149b.m_msgData;
      }
      r_clk2xToHti_ohm_1x.m_data64 = pv$3$145d.m_data64;
    }
  }
  bool rv$9$129;
  {
    rv$9$129 = c_reset1x.m_value;
  }
  bool reset$9$129 = rv$9$129;
  {
    if (reset$9$129)
      m_clk1xToClk2x_Clk1xRtnQue.m_pushIdx = 0;
  }
  bool rv$9$131;
  {
    rv$9$131 = r_clk2xToClk1x_Clk1xCallRdy.m_value;
  }
  r_clk2xToClk1x_Clk1xCallRdy_1x = rv$9$131;
  CClk2xToClk1x_Clk1xCallIntf rv$9$132;
  {
    CClk2xToClk1x_Clk1xCallIntf rv$4$26c;
    CClk2xToClk1x_Clk1xCallIntf const & pv$4$26c = r_clk2xToClk1x_Clk1xCall.m_value;
    {
      Clk2xInst_t rv$3$366;
      sc_uint<1> const & pv$3$366 = pv$4$26c.m_rtnInst;
      {
        rv$3$366.m_value = pv$3$366.m_value;
      }
      rv$4$26c.m_rtnInst = rv$3$366;
    }
    rv$9$132 = rv$4$26c;
  }
  CClk2xToClk1x_Clk1xCallIntf pv$9$132 = rv$9$132;
  {
    sc_uint<1> pv$3$366b = pv$9$132.m_rtnInst;
    {
      r_clk2xToClk1x_Clk1xCall_1x.m_rtnInst.m_value = pv$3$366b.m_value;
    }
  }
  uint64_t rv$9$133;
  sc_uint<6> rv$9$133b;
  {
    sc_uint<6> rv$4$27;
    sc_uint<6> const & pv$4$27 = r_clk2xToClk1x_Clk1xRtnAvlCnt_2x.m_value;
    {
      rv$4$27.m_value = pv$4$27.m_value;
    }
    rv$9$133b = rv$4$27;
  }
  {
    rv$9$133 = rv$9$133b.m_value;
  }
  r_clk2xToClk1x_Clk1xRtnAvl_1x = rv$9$133 > 0;
  ;
  bool & r_reset$9$136 = r_reset1x;
  bool rv$9$136;
  {
    rv$9$136 = i_reset.m_value;
  }
  bool const & i_reset$9$136 = rv$9$136;
  {
    r_reset$9$136 = i_reset$9$136;
  }
  bool const & value$9$137 = r_reset1x;
  {
    c_reset1x.m_value = value$9$137;
  }
  CHtAssertIntf const & value$9$142 = r_clk2xToHta_assert_1x;
  {
    CHtAssertIntf const & rhs$4$17 = value$9$142;
    {
      o_clk2xToHta_assert.m_value.m_bAssert = rhs$4$17.m_bAssert;
      o_clk2xToHta_assert.m_value.m_bCollision = rhs$4$17.m_bCollision;
      o_clk2xToHta_assert.m_value.m_module = rhs$4$17.m_module;
      o_clk2xToHta_assert.m_value.m_lineNum = rhs$4$17.m_lineNum;
      o_clk2xToHta_assert.m_value.m_info = rhs$4$17.m_info;
    }
  }
  CHostCtrlMsgIntf const & value$9$143 = r_clk2xToHti_ohm_1x;
  {
    CHostCtrlMsgIntf const & pv$4$17 = value$9$143;
    {
      CHostCtrlMsgIntf const & pv$3$145e = pv$4$17;
      {
        CHostCtrlMsgIntf const & pv$3$149c = pv$3$145e;
        {
          o_clk2xToHti_ohm.m_value.m_bValid = pv$3$149c.m_bValid;
          o_clk2xToHti_ohm.m_value.m_msgType = pv$3$149c.m_msgType;
          o_clk2xToHti_ohm.m_value.m_msgData = pv$3$149c.m_msgData;
        }
        o_clk2xToHti_ohm.m_value.m_data64 = pv$3$145e.m_data64;
      }
    }
  }
  bool const & value$9$145 = r_clk2xToClk1x_Clk1xCallRdy_1x;
  {
    o_clk2xToClk1x_Clk1xCallRdy.m_value = value$9$145;
  }
  CClk2xToClk1x_Clk1xCallIntf const & value$9$146 = r_clk2xToClk1x_Clk1xCall_1x;
  {
    CClk2xToClk1x_Clk1xCallIntf const & pv$4$17b = value$9$146;
    {
      sc_uint<1> const & pv$3$366c = pv$4$17b.m_rtnInst;
      {
        o_clk2xToClk1x_Clk1xCall.m_value.m_rtnInst.m_value = pv$3$366c.m_value;
      }
    }
  }
  bool const & value$9$147 = r_clk2xToClk1x_Clk1xRtnAvl_1x;
  {
    o_clk2xToClk1x_Clk1xRtnAvl.m_value = value$9$147;
  }
}
void CPersAuClk2x::PersAuClk2x_2x () {
  bool rv$9$153;
  {
    rv$9$153 = i_ctlToClk2x_Clk2xCallRdy.m_value;
  }
  if (rv$9$153)
    {
      CCtlToClk2x_Clk2xCallIntf rv$9$154;
      CCtlToClk2x_Clk2xCallIntf rv$9$154b;
      {
        CCtlToClk2x_Clk2xCallIntf rv$4$8c;
        CCtlToClk2x_Clk2xCallIntf const & pv$4$8c = i_ctlToClk2x_Clk2xCall.m_value;
        {
          CtlInst_t rv$3$288;
          sc_uint<1> const & pv$3$288 = pv$4$8c.m_rtnInst;
          {
            rv$3$288.m_value = pv$3$288.m_value;
          }
          rv$4$8c.m_rtnInst = rv$3$288;
        }
        rv$9$154b = rv$4$8c;
      }
      CCtlToClk2x_Clk2xCallIntf pv$9$154 = rv$9$154b;
      {
        CtlInst_t rv$3$288b;
        sc_uint<1> pv$3$288b = pv$9$154.m_rtnInst;
        {
          rv$3$288b.m_value = pv$3$288b.m_value;
        }
        rv$9$154.m_rtnInst = rv$3$288b;
      }
      CCtlToClk2x_Clk2xCallIntf data$9$154 = rv$9$154;
      {
        CCtlToClk2x_Clk2xCallIntf const & pv$2$6b = data$9$154;
        {
          sc_uint<1> const & pv$3$288c = pv$2$6b.m_rtnInst;
          {
            m_ctlToClk2x_Clk2xCallQue.m_que[++m_ctlToClk2x_Clk2xCallQue.m_pushIdx].m_rtnInst.m_value = pv$3$288c.m_value;
          }
        }
      }
    }
  bool c_clk2xToCtl_Clk2xCallAvl = false;
  bool c_clk2xToClk1x_Clk1xRtnAvl = false;
  sc_uint<2> rv$9$159;
  sc_uint<2> rv$9$159b;
  int32_t value$9$159 = 0;
  {
    rv$9$159b.m_value = value$9$159;
  }
  sc_uint<2> pv$9$159 = rv$9$159b;
  {
    rv$9$159.m_value = pv$9$159.m_value;
  }
  sc_uint<2> c_t1_htSel = rv$9$159;
  bool c_t1_htValid = false;
  bool c_htBusy = r_htBusy;
  bool c_htCmdValid = r_htCmdValid;
  bool rv$9$164;
  {
    rv$9$164 = c_reset1x.m_value;
  }
  bool c_resetComplete = !rv$9$164;
  bool rv$9$166;
  {
    rv$9$166 = m_ctlToClk2x_Clk2xCallQue.m_pushIdx == m_ctlToClk2x_Clk2xCallQue.m_popIdx;
  }
  if (!rv$9$166 && !r_htBusy && r_resetComplete)
    {
      uint64_t value$7$22 = 0;
      {
        c_t1_htSel.m_value = value$7$22;
      }
      c_t1_htValid = true;
      {
        ++m_ctlToClk2x_Clk2xCallQue.m_popIdx;
      }
      c_clk2xToCtl_Clk2xCallAvl = true;
      c_htBusy = true;
    }
  else
    {
      bool rv$9$172;
      {
        rv$9$172 = m_clk1xToClk2x_Clk1xRtnQue.m_pushIdx == m_clk1xToClk2x_Clk1xRtnQue.m_popIdx;
      }
      if (!rv$9$172 && r_resetComplete)
        {
          uint64_t value$7$23 = 1;
          {
            c_t1_htSel.m_value = value$7$23;
          }
          c_t1_htValid = true;
          {
            ++m_clk1xToClk2x_Clk1xRtnQue.m_popIdx;
          }
          c_clk2xToClk1x_Clk1xRtnAvl = true;
        }
      else
        if (r_htCmdValid && r_resetComplete)
          {
            uint64_t value$7$24 = 2;
            {
              c_t1_htSel.m_value = value$7$24;
            }
            c_t1_htValid = true;
            c_htCmdValid = false;
          }
    }
  CCtlToClk2x_Clk2xCallIntf rv$9$182;
  CCtlToClk2x_Clk2xCallIntf rv$9$182b;
  {
    CCtlToClk2x_Clk2xCallIntf rv$2$7;
    CCtlToClk2x_Clk2xCallIntf const & pv$2$7 = m_ctlToClk2x_Clk2xCallQue.m_que[m_ctlToClk2x_Clk2xCallQue.m_popIdx];
    {
      CtlInst_t rv$3$288c;
      sc_uint<1> const & pv$3$288d = pv$2$7.m_rtnInst;
      {
        rv$3$288c.m_value = pv$3$288d.m_value;
      }
      rv$2$7.m_rtnInst = rv$3$288c;
    }
    rv$9$182b = rv$2$7;
  }
  CCtlToClk2x_Clk2xCallIntf pv$9$182 = rv$9$182b;
  {
    CtlInst_t rv$3$288d;
    sc_uint<1> pv$3$288e = pv$9$182.m_rtnInst;
    {
      rv$3$288d.m_value = pv$3$288e.m_value;
    }
    rv$9$182.m_rtnInst = rv$3$288d;
  }
  CCtlToClk2x_Clk2xCallIntf c_t1_ctlToClk2x_Clk2xCall = rv$9$182;
  CClk1xToClk2x_Clk1xRtnIntf rv$9$183;
  CClk1xToClk2x_Clk1xRtnIntf rv$9$183b;
  {
    CClk1xToClk2x_Clk1xRtnIntf rv$2$7b;
    CClk1xToClk2x_Clk1xRtnIntf const & pv$2$7b = m_clk1xToClk2x_Clk1xRtnQue.m_que[m_clk1xToClk2x_Clk1xRtnQue.m_popIdx];
    {
      Clk2xInst_t rv$3$327c;
      sc_uint<1> const & pv$3$327d = pv$2$7b.m_rtnInst;
      {
        rv$3$327c.m_value = pv$3$327d.m_value;
      }
      rv$2$7b.m_rtnInst = rv$3$327c;
    }
    rv$9$183b = rv$2$7b;
  }
  CClk1xToClk2x_Clk1xRtnIntf pv$9$183 = rv$9$183b;
  {
    Clk2xInst_t rv$3$327d;
    sc_uint<1> pv$3$327e = pv$9$183.m_rtnInst;
    {
      rv$3$327d.m_value = pv$3$327e.m_value;
    }
    rv$9$183.m_rtnInst = rv$3$327d;
  }
  CClk1xToClk2x_Clk1xRtnIntf c_t1_clk1xToClk2x_Clk1xRtn = rv$9$183;
  CHtCmd rv$9$184;
  CHtCmd const & pv$9$184 = r_htCmd;
  {
    rv$9$184.m_htInstr = pv$9$184.m_htInstr;
  }
  CHtCmd c_t1_htCmd = rv$9$184;
  bool c_t2_htValid = r_t2_htValid;
  sc_uint<2> rv$9$190;
  sc_uint<2> const & pv$9$190 = r_t2_htSel;
  {
    rv$9$190.m_value = pv$9$190.m_value;
  }
  sc_uint<2> c_t2_htSel = rv$9$190;
  CHtPriv rv$9$191;
  CHtPriv const & pv$9$191 = r_htPriv;
  {
    Clk2xRtnInst_t rv$7$139;
    sc_uint<1> const & pv$7$139 = pv$9$191.m_rtnInst;
    {
      rv$7$139.m_value = pv$7$139.m_value;
    }
    rv$9$191.m_rtnInst = rv$7$139;
  }
  CHtPriv c_t2_htPriv = rv$9$191;
  sc_uint<1> rv$9$192;
  {
  }
  sc_uint<1> c_t2_htInstr = rv$9$192;
  CCtlToClk2x_Clk2xCallIntf rv$9$194;
  CCtlToClk2x_Clk2xCallIntf const & pv$9$194 = r_t2_ctlToClk2x_Clk2xCall;
  {
    CtlInst_t rv$3$288e;
    sc_uint<1> const & pv$3$288f = pv$9$194.m_rtnInst;
    {
      rv$3$288e.m_value = pv$3$288f.m_value;
    }
    rv$9$194.m_rtnInst = rv$3$288e;
  }
  CCtlToClk2x_Clk2xCallIntf c_t2_ctlToClk2x_Clk2xCall = rv$9$194;
  CClk1xToClk2x_Clk1xRtnIntf rv$9$195;
  CClk1xToClk2x_Clk1xRtnIntf const & pv$9$195 = r_t2_clk1xToClk2x_Clk1xRtn;
  {
    Clk2xInst_t rv$3$327e;
    sc_uint<1> const & pv$3$327f = pv$9$195.m_rtnInst;
    {
      rv$3$327e.m_value = pv$3$327f.m_value;
    }
    rv$9$195.m_rtnInst = rv$3$327e;
  }
  CClk1xToClk2x_Clk1xRtnIntf c_t2_clk1xToClk2x_Clk1xRtn = rv$9$195;
  CHtCmd rv$9$196;
  CHtCmd const & pv$9$196 = r_t2_htCmd;
  {
    rv$9$196.m_htInstr = pv$9$196.m_htInstr;
  }
  CHtCmd c_t2_htCmd = rv$9$196;
  uint64_t rv$9$198;
  {
    rv$9$198 = c_t2_htSel.m_value;
  }
  switch (rv$9$198)
  {
    case 0:
	{
      {
        uint64_t value$7$29 = 0;
        {
          c_t2_htInstr.m_value = value$7$29;
        }
      }
    int zero$9$201 = 0;
    {
      uint64_t value$7$143 = 0;
      {
        c_t2_htPriv.m_rtnInst.m_value = value$7$143;
      }
    }
    Clk2xRtnInst_t rv$9$202;
    sc_uint<1> const & pv$9$202b = c_t2_ctlToClk2x_Clk2xCall.m_rtnInst;
    {
      rv$9$202.m_value = pv$9$202b.m_value;
    }
    sc_uint<1> pv$9$202 = rv$9$202;
    {
      c_t2_htPriv.m_rtnInst.m_value = pv$9$202.m_value;
    }
	}
    break;
    case 1:
      {
        sc_uint<1> const & pv$9$205 = c_t2_clk1xToClk2x_Clk1xRtn.m_rtnInst;
        {
          c_t2_htInstr.m_value = pv$9$205.m_value;
        }
      }
    break;
    case 2:
      {
        uint64_t value$9$208 = c_t2_htCmd.m_htInstr;
        {
          c_t2_htInstr.m_value = value$9$208;
        }
      }
    break;
    default:
      ;
  }
  CHtPriv const & pv$9$215 = r_t3_htPriv;
  {
    sc_uint<1> const & pv$7$139b = pv$9$215.m_rtnInst;
    {
      c_t3_htPriv.m_rtnInst.m_value = pv$7$139b.m_value;
    }
  }
  uint64_t value$3$17 = 0;
  {
    c_t3_htCtrl.m_value = value$3$17;
  }
  sc_uint<1> const & pv$9$218 = r_t3_htInstr;
  {
    c_t3_htNextInstr.m_value = pv$9$218.m_value;
  }
  sc_uint<9> const & pv$9$220 = r_msg;
  {
    c_msg.m_value = pv$9$220.m_value;
  }
  c_clk2xToClk1x_Clk1xCallRdy = false;
  CClk2xToClk1x_Clk1xCallIntf rv$9$229;
  {
    CClk2xToClk1x_Clk1xCallIntf rv$4$26d;
    CClk2xToClk1x_Clk1xCallIntf const & pv$4$26d = r_clk2xToClk1x_Clk1xCall.m_value;
    {
      Clk2xInst_t rv$3$366b;
      sc_uint<1> const & pv$3$366d = pv$4$26d.m_rtnInst;
      {
        rv$3$366b.m_value = pv$3$366d.m_value;
      }
      rv$4$26d.m_rtnInst = rv$3$366b;
    }
    rv$9$229 = rv$4$26d;
  }
  CClk2xToClk1x_Clk1xCallIntf pv$9$229 = rv$9$229;
  {
    sc_uint<1> pv$3$366e = pv$9$229.m_rtnInst;
    {
      c_clk2xToClk1x_Clk1xCall.m_rtnInst.m_value = pv$3$366e.m_value;
    }
  }
  c_clk2xToCtl_Clk2xRtnRdy = false;
  int zero$9$232 = 0;
  {
    uint64_t value$3$273 = 0;
    {
      c_clk2xToCtl_Clk2xRtn.m_rtnInst.m_value = value$3$273;
    }
  }
  c_clk2xToHti_ohm.m_data64 = 0;
  int zero$9$236 = 0;
  {
    c_htAssert.m_bAssert = 0;
    c_htAssert.m_bCollision = 0;
    c_htAssert.m_module = 0;
    c_htAssert.m_lineNum = 0;
    c_htAssert.m_info = 0;
  }
  {
    if (r_t3_htValid)
      {
        uint64_t rv$7$38;
        {
          rv$7$38 = r_t3_htInstr.m_value;
        }
        switch (rv$7$38)
        {
          case 0:
            {
              bool rv$8$10;
              {
                ;
                bool rv$9$80;
                {
                  rv$9$80 = r_clk2xToClk1x_Clk1xCallRdy.m_value;
                }
                rv$8$10 = r_clk2xToClk1x_Clk1xCallAvlCntZero || ( rv$9$80 & r_phase );
              }
              bool rv$8$10b;
              {
                rv$8$10b = r_clk2xToHti_ohmAvlCnt == 0;
              }
              if (rv$8$10 || rv$8$10b)
                {
                  {
                    ;
                    uint64_t value$3$22 = 5;
                    {
                      c_t3_htCtrl.m_value = value$3$22;
                    }
                  }
                  break;
                }
              ;
              sc_uint<7> rv$5$104;
              sc_uint<7> rv$5$104b;
              int32_t value$5$104 = 16;
              {
                rv$5$104b.m_value = value$5$104;
              }
              sc_uint<7> pv$5$104 = rv$5$104b;
              {
                rv$5$104.m_value = pv$5$104.m_value;
              }
              sc_uint<7> msgType$5$104 = rv$5$104;
              sc_uint<56> rv$8$17;
              ht_uint56 rv$8$17b;
              uint64_t rv$7$44;
              {
                rv$7$44 = c_msg.m_value;
              }
              uint64_t value$7$44 = rv$7$44;
              {
                rv$8$17b.m_value = value$7$44;
              }
              sc_uint<56> pv$8$17 = rv$8$17b;
              {
                rv$8$17.m_value = pv$8$17.m_value;
              }
              sc_uint<56> msgData$8$17 = rv$8$17;
              {
                ;
                c_clk2xToHti_ohm.m_bValid = true;
                uint64_t rv$9$111;
                {
                  rv$9$111 = msgType$5$104.m_value;
                }
                c_clk2xToHti_ohm.m_msgType = rv$9$111;
                uint64_t rv$9$112;
                {
                  rv$9$112 = msgData$8$17.m_value;
                }
                c_clk2xToHti_ohm.m_msgData = rv$9$112;
              }
              sc_uint<1> rv$7$30;
              sc_uint<1> rv$7$30b;
              uint32_t value$7$30 = 1;
              {
                rv$7$30b.m_value = value$7$30;
              }
              sc_uint<1> pv$7$30 = rv$7$30b;
              {
                rv$7$30.m_value = pv$7$30.m_value;
              }
              sc_uint<1> rtnInst$7$30 = rv$7$30;
              {
                ;
                c_clk2xToClk1x_Clk1xCallRdy = true;
                sc_uint<1> const & pv$9$93 = rtnInst$7$30;
                {
                  c_clk2xToClk1x_Clk1xCall.m_rtnInst.m_value = pv$9$93.m_value;
                }
                uint64_t value$3$19 = 2;
                {
                  c_t3_htCtrl.m_value = value$3$19;
                }
              }
            }
          break;
          case 1:
            {
              bool rv$8$24;
              {
                ;
                rv$8$24 = r_clk2xToCtl_Clk2xRtnAvlCntZero;
              }
              if (rv$8$24)
                {
                  {
                    ;
                    uint64_t value$3$22b = 5;
                    {
                      c_t3_htCtrl.m_value = value$3$22b;
                    }
                  }
                  break;
                }
              {
                ;
                c_clk2xToCtl_Clk2xRtnRdy = true;
                CtlInst_t rv$9$70;
                sc_uint<1> const & pv$9$70b = r_t3_htPriv.m_rtnInst;
                {
                  rv$9$70.m_value = pv$9$70b.m_value;
                }
                sc_uint<1> pv$9$70 = rv$9$70;
                {
                  c_clk2xToCtl_Clk2xRtn.m_rtnInst.m_value = pv$9$70.m_value;
                }
                uint64_t value$3$20 = 3;
                {
                  c_t3_htCtrl.m_value = value$3$20;
                }
              }
            }
          break;
          default:
            ;
        }
      }
  }
  ;
  ;
  ;
  ;
  CHtPriv rv$9$265;
  CHtPriv const & pv$9$265 = r_htPriv;
  {
    Clk2xRtnInst_t rv$7$139b;
    sc_uint<1> const & pv$7$139c = pv$9$265.m_rtnInst;
    {
      rv$7$139b.m_value = pv$7$139c.m_value;
    }
    rv$9$265.m_rtnInst = rv$7$139b;
  }
  CHtPriv c_htPriv = rv$9$265;
  CHtCmd rv$9$266;
  CHtCmd const & pv$9$266 = r_htCmd;
  {
    rv$9$266.m_htInstr = pv$9$266.m_htInstr;
  }
  CHtCmd c_htCmd = rv$9$266;
  uint64_t rv$9$268;
  {
    rv$9$268 = r_t3_htSel.m_value;
  }
  switch (rv$9$268)
  {
    case 0:
      case 1:
        case 2:
          if (r_t3_htValid)
            {
              CHtPriv const & pv$9$273 = c_t3_htPriv;
              {
                sc_uint<1> const & pv$7$139d = pv$9$273.m_rtnInst;
                {
                  c_htPriv.m_rtnInst.m_value = pv$7$139d.m_value;
                }
              }
            }
    uint64_t rv$9$274;
    {
      rv$9$274 = c_t3_htCtrl.m_value;
    }
    if (r_t3_htValid && rv$9$274 != 4)
      {
        bool rv$9$275;
        int rhs$3$18 = 1;
        {
          rv$9$275 = c_t3_htCtrl.m_value == rhs$3$18;
        }
        bool rv$9$275b;
        int rhs$3$25 = 8;
        {
          rv$9$275b = c_t3_htCtrl.m_value == rhs$3$25;
        }
        bool rv$9$275c;
        int rhs$3$22 = 5;
        {
          rv$9$275c = c_t3_htCtrl.m_value == rhs$3$22;
        }
        c_htCmdValid = rv$9$275 || rv$9$275b || rv$9$275c;
        uint64_t rv$9$276;
        {
          rv$9$276 = c_t3_htNextInstr.m_value;
        }
        c_htCmd.m_htInstr = rv$9$276;
      }
    break;
    default:
      ;
  }
  if (r_clk2xToCtl_Clk2xRtnRdy)
    c_htBusy = false;
  sc_uint<6> rv$9$286;
  sc_uint<6> rv$9$286b;
  uint64_t rv$9$286c;
  {
    rv$9$286c = r_clk2xToClk1x_Clk1xCallAvlCnt.m_value;
  }
  bool rv$9$287;
  {
    rv$9$287 = i_clk1xToClk2x_Clk1xCallAvl.m_value;
  }
  uint64_t value$9$286 = rv$9$286c + ( r_phase & rv$9$287 ) - c_clk2xToClk1x_Clk1xCallRdy;
  {
    rv$9$286b.m_value = value$9$286;
  }
  sc_uint<6> pv$9$286 = rv$9$286b;
  {
    rv$9$286.m_value = pv$9$286.m_value;
  }
  sc_uint<6> c_clk2xToClk1x_Clk1xCallAvlCnt = rv$9$286;
  bool rv$9$288;
  int rhs$9$288 = 0;
  {
    rv$9$288 = c_clk2xToClk1x_Clk1xCallAvlCnt.m_value == rhs$9$288;
  }
  bool c_clk2xToClk1x_Clk1xCallAvlCntZero = rv$9$288;
  bool rv$9$289;
  {
    rv$9$289 = r_clk2xToClk1x_Clk1xCallRdy.m_value;
  }
  bool c_clk2xToClk1x_Clk1xCallHold = rv$9$289 & r_phase;
  c_clk2xToClk1x_Clk1xCallRdy |= c_clk2xToClk1x_Clk1xCallHold;
  sc_uint<6> rv$9$291;
  sc_uint<6> rv$9$291b;
  uint64_t rv$9$291c;
  {
    rv$9$291c = r_clk2xToCtl_Clk2xRtnAvlCnt.m_value;
  }
  bool rv$9$292;
  {
    rv$9$292 = i_ctlToClk2x_Clk2xRtnAvl.m_value;
  }
  uint64_t value$9$291 = rv$9$291c + rv$9$292 - c_clk2xToCtl_Clk2xRtnRdy;
  {
    rv$9$291b.m_value = value$9$291;
  }
  sc_uint<6> pv$9$291 = rv$9$291b;
  {
    rv$9$291.m_value = pv$9$291.m_value;
  }
  sc_uint<6> c_clk2xToCtl_Clk2xRtnAvlCnt = rv$9$291;
  bool rv$9$293;
  int rhs$9$293 = 0;
  {
    rv$9$293 = c_clk2xToCtl_Clk2xRtnAvlCnt.m_value == rhs$9$293;
  }
  bool c_clk2xToCtl_Clk2xRtnAvlCntZero = rv$9$293;
  CHostCtrlMsgIntf rv$9$295;
  {
    CHostCtrlMsgIntf rv$4$27b;
    CHostCtrlMsgIntf const & pv$4$27b = r_hifToAu_ihm_1x.m_value;
    {
      CHostCtrlMsgIntf rv$3$145c;
      CHostCtrlMsgIntf const & pv$3$145f = pv$4$27b;
      {
      }
      rv$4$27b = rv$3$145c;
    }
    rv$9$295 = rv$4$27b;
  }
  if (rv$9$295.m_bValid && r_phase)
    {
      CHostCtrlMsgIntf rv$9$296;
      {
        CHostCtrlMsgIntf rv$4$27c;
        CHostCtrlMsgIntf const & pv$4$27c = r_hifToAu_ihm_1x.m_value;
        {
          CHostCtrlMsgIntf rv$3$145d;
          CHostCtrlMsgIntf const & pv$3$145g = pv$4$27c;
          {
          }
          rv$4$27c = rv$3$145d;
        }
        rv$9$296 = rv$4$27c;
      }
      switch (rv$9$296.m_msgType)
      {
        case 16:
          {
            ht_uint9 rv$9$298;
            CHostCtrlMsgIntf rv$9$298b;
            {
              CHostCtrlMsgIntf rv$4$27d;
              CHostCtrlMsgIntf const & pv$4$27d = r_hifToAu_ihm_1x.m_value;
              {
                CHostCtrlMsgIntf rv$3$145e;
                CHostCtrlMsgIntf const & pv$3$145h = pv$4$27d;
                {
                }
                rv$4$27d = rv$3$145e;
              }
              rv$9$298b = rv$4$27d;
            }
            uint64_t value$9$298 = ( rv$9$298b.m_msgData >> 0 );
            {
              rv$9$298.m_value = value$9$298;
            }
            sc_uint<9> pv$9$298 = rv$9$298;
            {
              c_msg.m_value = pv$9$298.m_value;
            }
          }
        break;
        default:
          break;
      }
    }
  uint64_t value$9$305 = r_clk2xToHti_ohmAvlCnt;
  {
    c_clk2xToHti_ohmAvlCnt.m_value = value$9$305;
  }
  bool rv$9$306;
  {
    rv$9$306 = r_htiToClk2x_ohmAvl_1x.m_value;
  }
  if (( rv$9$306 & r_phase ) != c_clk2xToHti_ohm.m_bValid)
    {
      uint64_t value$9$307 = 1;
      {
        c_clk2xToHti_ohmAvlCnt.m_value ^= value$9$307;
      }
    }
  CHostCtrlMsgIntf rv$9$309;
  {
    CHostCtrlMsgIntf rv$4$27e;
    CHostCtrlMsgIntf const & pv$4$27e = r_clk2xToHti_ohm_2x.m_value;
    {
      CHostCtrlMsgIntf rv$3$145f;
      CHostCtrlMsgIntf const & pv$3$145i = pv$4$27e;
      {
      }
      rv$4$27e = rv$3$145f;
    }
    rv$9$309 = rv$4$27e;
  }
  bool rv$9$309b;
  {
    rv$9$309b = c_reset1x.m_value;
  }
  if (rv$9$309.m_bValid && r_phase && !rv$9$309b)
    {
      CHostCtrlMsgIntf rv$9$310;
      {
        CHostCtrlMsgIntf rv$4$26e;
        CHostCtrlMsgIntf const & pv$4$26e = r_clk2xToHti_ohm_2x.m_value;
        {
          CHostCtrlMsgIntf rv$3$145g;
          CHostCtrlMsgIntf const & pv$3$145j = pv$4$26e;
          {
          }
          rv$4$26e = rv$3$145g;
        }
        rv$9$310 = rv$4$26e;
      }
      CHostCtrlMsgIntf pv$9$310 = rv$9$310;
      {
        CHostCtrlMsgIntf pv$3$145k = pv$9$310;
        {
          CHostCtrlMsgIntf pv$3$149d = pv$3$145k;
          {
            c_clk2xToHti_ohm.m_bValid = pv$3$149d.m_bValid;
            c_clk2xToHti_ohm.m_msgType = pv$3$149d.m_msgType;
            c_clk2xToHti_ohm.m_msgData = pv$3$149d.m_msgData;
          }
          c_clk2xToHti_ohm.m_data64 = pv$3$145k.m_data64;
        }
      }
    }
  sc_uint<6> rv$9$312;
  sc_uint<6> rv$9$312b;
  uint64_t rv$9$312c;
  sc_uint<6> rv$9$312d;
  {
    sc_uint<6> rv$4$27f;
    sc_uint<6> const & pv$4$27f = r_clk2xToClk1x_Clk1xRtnAvlCnt_2x.m_value;
    {
      rv$4$27f.m_value = pv$4$27f.m_value;
    }
    rv$9$312d = rv$4$27f;
  }
  {
    rv$9$312c = rv$9$312d.m_value;
  }
  uint64_t rv$9$313;
  sc_uint<6> rv$9$313b;
  {
    sc_uint<6> rv$4$27g;
    sc_uint<6> const & pv$4$27g = r_clk2xToClk1x_Clk1xRtnAvlCnt_2x.m_value;
    {
      rv$4$27g.m_value = pv$4$27g.m_value;
    }
    rv$9$313b = rv$4$27g;
  }
  {
    rv$9$313 = rv$9$313b.m_value;
  }
  uint64_t value$9$312 = rv$9$312c + c_clk2xToClk1x_Clk1xRtnAvl - ( rv$9$313 > 0 && !r_phase );
  {
    rv$9$312b.m_value = value$9$312;
  }
  sc_uint<6> pv$9$312 = rv$9$312b;
  {
    rv$9$312.m_value = pv$9$312.m_value;
  }
  sc_uint<6> c_clk2xToClk1x_Clk1xRtnAvlCnt_2x = rv$9$312;
  bool rv$9$317;
  {
    rv$9$317 = c_reset1x.m_value;
  }
  r_resetComplete = !rv$9$317 && c_resetComplete;
  sc_uint<2> const & pv$9$319 = c_t1_htSel;
  {
    r_t2_htSel.m_value = pv$9$319.m_value;
  }
  CCtlToClk2x_Clk2xCallIntf const & pv$9$320 = c_t1_ctlToClk2x_Clk2xCall;
  {
    sc_uint<1> const & pv$3$288g = pv$9$320.m_rtnInst;
    {
      r_t2_ctlToClk2x_Clk2xCall.m_rtnInst.m_value = pv$3$288g.m_value;
    }
  }
  CClk1xToClk2x_Clk1xRtnIntf const & pv$9$321 = c_t1_clk1xToClk2x_Clk1xRtn;
  {
    sc_uint<1> const & pv$3$327g = pv$9$321.m_rtnInst;
    {
      r_t2_clk1xToClk2x_Clk1xRtn.m_rtnInst.m_value = pv$3$327g.m_value;
    }
  }
  bool rv$9$322;
  {
    rv$9$322 = c_reset1x.m_value;
  }
  r_t2_htValid = !rv$9$322 && c_t1_htValid;
  CHtCmd const & pv$9$323 = c_t1_htCmd;
  {
    r_t2_htCmd.m_htInstr = pv$9$323.m_htInstr;
  }
  bool rv$9$325;
  {
    rv$9$325 = c_reset1x.m_value;
  }
  r_t3_htValid = !rv$9$325 && c_t2_htValid;
  sc_uint<2> const & pv$9$326 = c_t2_htSel;
  {
    r_t3_htSel.m_value = pv$9$326.m_value;
  }
  CHtPriv const & pv$9$327 = c_t2_htPriv;
  {
    sc_uint<1> const & pv$7$139e = pv$9$327.m_rtnInst;
    {
      r_t3_htPriv.m_rtnInst.m_value = pv$7$139e.m_value;
    }
  }
  sc_uint<1> const & pv$9$328 = c_t2_htInstr;
  {
    r_t3_htInstr.m_value = pv$9$328.m_value;
  }
  sc_uint<9> const & pv$9$332 = c_msg;
  {
    r_msg.m_value = pv$9$332.m_value;
  }
  CHtCmd const & pv$9$334 = c_htCmd;
  {
    r_htCmd.m_htInstr = pv$9$334.m_htInstr;
  }
  CHtAssertIntf const & value$9$335 = c_htAssert;
  {
    CHtAssertIntf const & rhs$4$25 = value$9$335;
    {
      r_clk2xToHta_assert.m_value.m_bAssert = rhs$4$25.m_bAssert;
      r_clk2xToHta_assert.m_value.m_bCollision = rhs$4$25.m_bCollision;
      r_clk2xToHta_assert.m_value.m_module = rhs$4$25.m_module;
      r_clk2xToHta_assert.m_value.m_lineNum = rhs$4$25.m_lineNum;
      r_clk2xToHta_assert.m_value.m_info = rhs$4$25.m_info;
    }
  }
  bool rv$9$336;
  {
    rv$9$336 = c_reset1x.m_value;
  }
  r_htCmdValid = !rv$9$336 && c_htCmdValid;
  CHtPriv const & pv$9$337 = c_htPriv;
  {
    sc_uint<1> const & pv$7$139f = pv$9$337.m_rtnInst;
    {
      r_htPriv.m_rtnInst.m_value = pv$7$139f.m_value;
    }
  }
  bool rv$9$338;
  {
    rv$9$338 = c_reset1x.m_value;
  }
  r_htBusy = !rv$9$338 && c_htBusy;
  bool rv$9$341;
  {
    rv$9$341 = c_reset1x.m_value;
  }
  r_clk2xToCtl_Clk2xCallAvl = !rv$9$341 && c_clk2xToCtl_Clk2xCallAvl;
  CClk2xToClk1x_Clk1xCallIntf const & value$9$343 = c_clk2xToClk1x_Clk1xCall;
  {
    CClk2xToClk1x_Clk1xCallIntf const & pv$4$25b = value$9$343;
    {
      sc_uint<1> const & pv$3$366f = pv$4$25b.m_rtnInst;
      {
        r_clk2xToClk1x_Clk1xCall.m_value.m_rtnInst.m_value = pv$3$366f.m_value;
      }
    }
  }
  bool rv$9$344;
  {
    rv$9$344 = c_reset1x.m_value;
  }
  bool const & value$9$344 = !rv$9$344 && c_clk2xToClk1x_Clk1xCallRdy;
  {
    r_clk2xToClk1x_Clk1xCallRdy.m_value = value$9$344;
  }
  bool rv$9$345;
  {
    rv$9$345 = c_reset1x.m_value;
  }
  sc_uint<6> rv$9$345b;
  sc_uint<6> rv$9$345c;
  int32_t value$9$345 = 32;
  {
    rv$9$345c.m_value = value$9$345;
  }
  sc_uint<6> pv$9$345b = rv$9$345c;
  {
    rv$9$345b.m_value = pv$9$345b.m_value;
  }
  sc_uint<6> rv$9$345d;
  sc_uint<6> const & pv$9$345c = c_clk2xToClk1x_Clk1xCallAvlCnt;
  {
    rv$9$345d.m_value = pv$9$345c.m_value;
  }
  sc_uint<6> pv$9$345 = rv$9$345 ? rv$9$345b : rv$9$345d;
  {
    r_clk2xToClk1x_Clk1xCallAvlCnt.m_value = pv$9$345.m_value;
  }
  bool rv$9$346;
  {
    rv$9$346 = c_reset1x.m_value;
  }
  r_clk2xToClk1x_Clk1xCallAvlCntZero = !rv$9$346 && c_clk2xToClk1x_Clk1xCallAvlCntZero;
  CClk2xToCtl_Clk2xRtnIntf const & rhs$9$348 = c_clk2xToCtl_Clk2xRtn;
  {
    sc_uint<1> const & pv$3$268 = rhs$9$348.m_rtnInst;
    {
      r_clk2xToCtl_Clk2xRtn.m_rtnInst.m_value = pv$3$268.m_value;
    }
  }
  bool rv$9$349;
  {
    rv$9$349 = c_reset1x.m_value;
  }
  r_clk2xToCtl_Clk2xRtnRdy = !rv$9$349 && c_clk2xToCtl_Clk2xRtnRdy;
  bool rv$9$350;
  {
    rv$9$350 = c_reset1x.m_value;
  }
  sc_uint<6> rv$9$350b;
  sc_uint<6> rv$9$350c;
  int32_t value$9$350 = 32;
  {
    rv$9$350c.m_value = value$9$350;
  }
  sc_uint<6> pv$9$350b = rv$9$350c;
  {
    rv$9$350b.m_value = pv$9$350b.m_value;
  }
  sc_uint<6> rv$9$350d;
  sc_uint<6> const & pv$9$350c = c_clk2xToCtl_Clk2xRtnAvlCnt;
  {
    rv$9$350d.m_value = pv$9$350c.m_value;
  }
  sc_uint<6> pv$9$350 = rv$9$350 ? rv$9$350b : rv$9$350d;
  {
    r_clk2xToCtl_Clk2xRtnAvlCnt.m_value = pv$9$350.m_value;
  }
  bool rv$9$351;
  {
    rv$9$351 = c_reset1x.m_value;
  }
  r_clk2xToCtl_Clk2xRtnAvlCntZero = !rv$9$351 && c_clk2xToCtl_Clk2xRtnAvlCntZero;
  bool rv$9$353;
  {
    rv$9$353 = c_reset1x.m_value;
  }
  sc_uint<6> rv$9$353b;
  sc_uint<6> rv$9$353c;
  int32_t value$9$353b = 0;
  {
    rv$9$353c.m_value = value$9$353b;
  }
  sc_uint<6> pv$9$353 = rv$9$353c;
  {
    rv$9$353b.m_value = pv$9$353.m_value;
  }
  sc_uint<6> rv$9$353d;
  sc_uint<6> const & pv$9$353b = c_clk2xToClk1x_Clk1xRtnAvlCnt_2x;
  {
    rv$9$353d.m_value = pv$9$353b.m_value;
  }
  sc_uint<6> const & value$9$353 = rv$9$353 ? rv$9$353b : rv$9$353d;
  {
    sc_uint<6> const & pv$4$25c = value$9$353;
    {
      r_clk2xToClk1x_Clk1xRtnAvlCnt_2x.m_value.m_value = pv$4$25c.m_value;
    }
  }
  CHostCtrlMsgIntf const & value$9$355 = c_clk2xToHti_ohm;
  {
    CHostCtrlMsgIntf const & pv$4$25d = value$9$355;
    {
      CHostCtrlMsgIntf const & pv$3$145l = pv$4$25d;
      {
        CHostCtrlMsgIntf const & pv$3$149e = pv$3$145l;
        {
          r_clk2xToHti_ohm_2x.m_value.m_bValid = pv$3$149e.m_bValid;
          r_clk2xToHti_ohm_2x.m_value.m_msgType = pv$3$149e.m_msgType;
          r_clk2xToHti_ohm_2x.m_value.m_msgData = pv$3$149e.m_msgData;
        }
        r_clk2xToHti_ohm_2x.m_value.m_data64 = pv$3$145l.m_data64;
      }
    }
  }
  uint64_t rv$9$356;
  bool rv$9$356b;
  {
    rv$9$356b = c_reset1x.m_value;
  }
  sc_uint<1> rv$9$356c;
  sc_uint<1> rv$9$356d;
  int32_t value$9$356 = 1;
  {
    rv$9$356d.m_value = value$9$356;
  }
  sc_uint<1> pv$9$356 = rv$9$356d;
  {
    rv$9$356c.m_value = pv$9$356.m_value;
  }
  sc_uint<1> rv$9$356e;
  sc_uint<1> const & pv$9$356b = c_clk2xToHti_ohmAvlCnt;
  {
    rv$9$356e.m_value = pv$9$356b.m_value;
  }
  {
    rv$9$356 = (rv$9$356b ? rv$9$356c : rv$9$356e).m_value;
  }
  r_clk2xToHti_ohmAvlCnt = rv$9$356;
  bool rv$9$358;
  {
    rv$9$358 = c_reset1x.m_value;
  }
  bool reset$9$358 = rv$9$358;
  {
    bool reset$2$10 = reset$9$358;
    {
      if (reset$2$10)
        m_ctlToClk2x_Clk2xCallQue.m_pushIdx = 0;
    }
    bool reset$2$10b = reset$9$358;
    {
      if (reset$2$10b)
        m_ctlToClk2x_Clk2xCallQue.m_popIdx = 0;
    }
  }
  bool rv$9$359;
  {
    rv$9$359 = c_reset1x.m_value;
  }
  bool reset$9$359 = rv$9$359;
  {
    if (reset$9$359)
      m_clk1xToClk2x_Clk1xRtnQue.m_popIdx = 0;
  }
  ;
  bool rv$9$362;
  {
    rv$9$362 = c_reset1x.m_value;
  }
  r_phase = rv$9$362 || !r_phase;
  bool const & value$9$367 = r_clk2xToCtl_Clk2xCallAvl;
  {
    o_clk2xToCtl_Clk2xCallAvl.m_value = value$9$367;
  }
  bool const & value$9$368 = r_clk2xToCtl_Clk2xRtnRdy;
  {
    o_clk2xToCtl_Clk2xRtnRdy.m_value = value$9$368;
  }
  CClk2xToCtl_Clk2xRtnIntf const & value$9$369 = r_clk2xToCtl_Clk2xRtn;
  {
    CClk2xToCtl_Clk2xRtnIntf const & rhs$4$17b = value$9$369;
    {
      sc_uint<1> const & pv$3$268b = rhs$4$17b.m_rtnInst;
      {
        o_clk2xToCtl_Clk2xRtn.m_value.m_rtnInst.m_value = pv$3$268b.m_value;
      }
    }
  }
}
