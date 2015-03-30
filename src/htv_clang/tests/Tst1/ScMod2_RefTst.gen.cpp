// List of fileNum / pathName used for temporary variables
//  1  C:/Users/TBrewer/Documents/llvm-project/my_tests/ht_lib\HtSyscTypes.h
//  2  C:\Users\TBrewer\Documents\llvm-project\my_tests\Tst1\ScMod2_RefTst.cpp

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

class RefTst : public ScModuleBase {
public:
  void RefTst1x ();
  struct A {
    int m_a : 3;
    int m_b : 4;
  };

  A m_A;
  struct B {
    int m_c : 5;
    int m_d : 6;
    A m_e;
  };

  B m_B;
  bool bo$2$39[2][3];
  int bi$2$41;
  A rv$2$43;
  A f$2$43;
  A rv$2$44;
  A g$2$44;
  B rv$2$45;
  A rv$2$13;
  B h$2$45;
  A rv$2$46;
  A i$2$46;
  A rv$2$47;
  A j$2$47;
  A rv$2$52;
  A k$2$52;
};

void RefTst::RefTst1x () {
  bi$2$41 = bo$2$39[1][2];
  {
    rv$2$43.m_a = m_A.m_b;
    rv$2$43.m_b = m_A.m_a;
  }
  f$2$43 = rv$2$43;
  {
    rv$2$44.m_a = m_B.m_e.m_b;
    rv$2$44.m_b = m_B.m_e.m_a;
  }
  g$2$44 = rv$2$44;
  {
    {
      rv$2$13.m_a = m_B.m_e.m_b;
      rv$2$13.m_b = m_B.m_e.m_a;
    }
    rv$2$45.m_e = rv$2$13;
    rv$2$45.m_d = m_B.m_d;
    rv$2$45.m_c = m_B.m_c;
  }
  h$2$45 = rv$2$45;
  {
    rv$2$46.m_a = m_B.m_e.m_b;
    rv$2$46.m_b = m_B.m_e.m_a;
  }
  i$2$46 = rv$2$46;
  {
    rv$2$47.m_a = true ? m_B.m_e.m_b : m_B.m_e.m_b;
    rv$2$47.m_b = true ? m_B.m_e.m_a : m_B.m_e.m_a;
  }
  j$2$47 = rv$2$47;
  {
    j$2$47.m_a = true ? m_B.m_e.m_a : m_B.m_e.m_a;
    j$2$47.m_b = true ? m_B.m_e.m_b : m_B.m_e.m_b;
  }
  if (true)
    m_B.m_e.m_b = 3;
  else
    m_B.m_e.m_b = 3;
  {
  }
  {
    rv$2$52.m_a = false ? true ? m_B.m_e.m_b : m_B.m_e.m_b : j$2$47.m_b;
    rv$2$52.m_b = false ? true ? m_B.m_e.m_a : m_B.m_e.m_a : j$2$47.m_a;
  }
  k$2$52 = rv$2$52;
  {
  }
  if (false)
    if (true)
      m_B.m_e.m_a = 2;
    else
      m_B.m_e.m_a = 2;
  else
    j$2$47.m_a = 2;
}
