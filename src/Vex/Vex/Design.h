/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include "Lex.h"

struct CPortList;

class CDesign : CLex
{
private:
	enum ESigType { sig_port, sig_input, sig_output, sig_wire };
private:
	struct CSynAttrib {
		CSynAttrib(string &attrib, string &inst, string &value) : m_attrib(attrib), m_inst(inst), m_value(value) {}

		string m_attrib;
		string m_inst;
		string m_value;
	};

	struct CPrimPort {
		char	*m_pName;
		bool	m_bInput;
		bool	m_bClock;
		short	m_bits;
	};
	//struct CPrim {
	//	char		*m_pName;
	//	int			m_useCnt;
	//	CPrimPort	*m_pPorts;

	//	CPrimPort *FindPrimPort(string &portName) {
	//		CPrimPort *pPort;
	//		for (pPort = m_pPorts; pPort->m_pName && pPort->m_pName != portName; pPort += 1);
	//		return pPort->m_pName ? pPort : 0;
	//	}
	//};

	//typedef map<string, CPrim *>		PrimMap;
	//typedef pair<string, CPrim *>		PrimMap_Pair;
	//typedef PrimMap::iterator			PrimMap_Iter;

	struct COperand {
		COperand() : m_value(0), m_width(-1) {}
		COperand(long long value, int width) : m_value(value), m_width(width) {}
		COperand(string str) {
			const char *pNum = str.c_str();
			m_value = 0;
			m_width = -1;
			while (isdigit(*pNum))
				m_value = m_value * 10 + *pNum++ - '0';

			if (m_value <= 1)
				m_width = 1;
			if (*pNum == '\0')
				return;

			if (pNum[0] == '\'') {
				if (tolower(pNum[1]) == 'h') {
					if (m_value > 64)
						return;
					pNum += 2;
					m_width = (int)m_value;
					m_value = 0;
					while (isdigit(*pNum) || tolower(*pNum) >= 'a' && tolower(*pNum) <= 'f')
						m_value = m_value * 16 + (isdigit(*pNum) ? (*pNum++ - '0') : (tolower(*pNum++) - 'a' + 10));
				} else if (tolower(pNum[1] == 'd')) {
					if (m_value > 64)
						return;
					pNum += 2;
					m_width = (int)m_value;
					m_value = 0;
					while (isdigit(*pNum))
						m_value = m_value * 10 + *pNum++ - '0';
				} else if (tolower(pNum[1] == 'b')) {
					if (m_value > 64)
						return;
					pNum += 2;
					m_width = (int)m_value;
					m_value = 0;
					while (*pNum == '0' || *pNum == '1')
						m_value = m_value * 2 + *pNum++ - '0';
				}
			}

			if (*pNum != '\0')
				m_width = -1;
		}

		COperand operator - () { return COperand( -m_value, m_width < 2 ? -1 : m_width); }
		COperand operator ~ () { return COperand( ~m_value, m_width); }
		COperand operator ! () { return COperand( !m_value, m_width == 1 ? 1 : -1); }

		COperand operator << (COperand &op) { return COperand( m_value << op.m_value, m_width + (1 << op.m_width) - 1); }
		COperand operator + (COperand &op) { return COperand( m_value + op.m_value, op.m_width == m_width ? m_width : -1); }
		COperand operator - (COperand &op) { return COperand( m_value - op.m_value, op.m_width == m_width ? m_width : -1); }
		COperand operator * (COperand &op) { return COperand( GetValue() * op.GetValue(), op.m_width == m_width ? m_width : -1); }
		COperand operator / (COperand &op) { return COperand( GetValue() / op.GetValue(), op.m_width == m_width ? m_width : -1); }
		COperand operator % (COperand &op) { return COperand( GetValue() % op.GetValue(), op.m_width == m_width ? m_width : -1); }
		COperand operator | (COperand &op) { return COperand( m_value | op.m_value, op.m_width == m_width ? m_width : -1); }
		COperand operator & (COperand &op) { return COperand( m_value & op.m_value, op.m_width == m_width ? m_width : -1); }
		COperand operator ^ (COperand &op) { return COperand( m_value ^ op.m_value, op.m_width == m_width ? m_width : -1); }
		COperand operator > (COperand &op) { return COperand( GetValue() > op.GetValue(), op.m_width == m_width ? 1 : -1); }
		COperand operator >= (COperand &op) { return COperand( GetValue() >= op.GetValue(), op.m_width == m_width ? 1 : -1); }
		COperand operator < (COperand &op) { return COperand( GetValue() < op.GetValue(), op.m_width == m_width ? 1 : -1); }
		COperand operator <= (COperand &op) { return COperand( GetValue() <= op.GetValue(), op.m_width == m_width ? 1 : -1); }
		COperand operator != (COperand &op) { return COperand( GetValue() != op.GetValue(), op.m_width == m_width ? 1 : -1); }
		COperand operator == (COperand &op) { return COperand( GetValue() == op.GetValue(), op.m_width == m_width ? 1 : -1); }
		COperand operator && (COperand &op) { return COperand( GetValue() & op.GetValue(), op.m_width == 1 && m_width == 1 ? 1 : -1); }
		COperand operator || (COperand &op) { return COperand( GetValue() | op.GetValue(), op.m_width == 1 && m_width == 1 ? 1 : -1); }
		static COperand SelectOperator(COperand &op1, COperand &op2, COperand &op3) {
			return COperand(op1.m_value ? op2.m_value : op3.m_value, (op1.m_width != 1 || op2.m_width != op3.m_width) ? -1 : op2.m_width);
		}
		COperand operator , (COperand &op) {
			if (m_width < 0 || op.m_width < 0)
				return COperand(0, -1);
			return COperand((m_value << op.m_width) | op.GetValue(), m_width + op.m_width);
		}

		string GetDebugStr() {
			char buf[64];
			if (m_width < 0)
				sprintf(buf, "`h%llx", GetValue());
			else
				sprintf(buf, "%d`h%llx", m_width, GetValue());
			return string(buf);
		}

		long long GetValue() {
			if (m_width < 0 || m_width == 0x40)
				return m_value;
			else
				return m_value & ((1 << m_width) - 1);
		}
		void SetValue(long long value) { m_value = value; }

		int GetWidth() { return m_width; }
		void SetWidth(int width) { m_width = width; }

	private:
		long long				m_value;
		int						m_width;
	};

	struct CDefparam {
		CDefparam(string &heirName, bool bIsString, string &paramValue) :
			m_heirName(heirName), m_bIsString(bIsString), m_paramValue(paramValue) {}

		string					m_heirName;
		bool					m_bIsString;
		string					m_paramValue;
		CLineInfo				m_lineInfo;
	};

	typedef map<string, CDefparam>			DefparamMap;
	typedef pair<string, CDefparam>			DefparamMap_ValuePair;
	typedef DefparamMap::iterator			DefparamMap_Iter;
	typedef pair<DefparamMap_Iter, bool>	DefparamMap_InsertPair;

	struct CInst;
	struct CSignalPort {
		CSignalPort() : m_pInst(0), m_portIdx(-1) {}
		bool IsInput() { return m_pInst->m_instPorts[m_portIdx].m_pModulePort->m_sigType == sig_input; }
		bool IsOutput() { return m_pInst->m_instPorts[m_portIdx].m_pModulePort->m_sigType == sig_output; }
		bool IsInstPrim() { return m_pInst->m_pModule->m_bPrim; }
		string &GetModuleName() { return m_pInst->m_pModule->m_name; }
		CInst * GetInst() { return m_pInst; }

		CInst			*m_pInst;
		int				m_portIdx;
		int				m_portBit;
	};
	typedef vector<CSignalPort>			SignalPortVector;

	struct CSignal;
	class CSignalPortIter {
	public:
		CSignalPortIter(CSignal *pSignal, int sigBit);
		//CSignalPortIter(vector<CInstPort> *pInstPorts) : m_pInstPorts(pInstPorts), m_idx(0) {}
		//CSignalPortIter(CInstPortIter &iter) { *this = iter; }

		bool end() { return m_portIdx == (int)m_pSignalPorts->size(); }
		void operator ++ (int) { m_portIdx += 1; }
		CSignalPort * operator -> () { return &(*m_pSignalPorts)[m_portIdx]; }

	private:
		SignalPortVector	*m_pSignalPorts;
		int					m_portIdx;
	};

	struct CPortSignal {
		CPortSignal() : m_pSignal(0) {}
		CPortSignal(CSignal *pSignal, int bit) : m_pSignal(pSignal), m_sigBit(bit) {}
		CSignalPortIter GetSignalPortIter() { return CSignalPortIter(m_pSignal, m_sigBit); }
		int GetLoadCnt();
		CInst * GetFirstInputInst();

		CSignal			*m_pSignal;
		int				m_sigBit;
	};

	struct CInstPort {
		//CInstPort(CPrimPort *pPrimPort) : m_pPrimPort(pPrimPort), m_pModulePort(0), m_pVector(0) {}
		CInstPort(CSignal *pModulePort) : m_pModulePort(pModulePort), m_pVector(0) {}
		string GetModulePortName() { return m_pModulePort->m_name; }
		int GetModulePortWidth() { return m_pModulePort->GetWidth(); }
		CPortSignal * GetPortSignal(int portBitIdx=-1) {
			if (m_pModulePort->GetWidth() == 1) {
				assert(portBitIdx == -1);
				return m_pSingle;
			} else {
				assert(portBitIdx != -1);
				return &(*m_pVector)[portBitIdx];
			}
		}
			
		//CPrimPort					*m_pPrimPort;
		CSignal						*m_pModulePort;
		union {
			vector<CPortSignal>		*m_pVector;
			CPortSignal				*m_pSingle;
		};
	};


	class CInstPortIter {
	public:
		CInstPortIter(vector<CInstPort> *pInstPorts) : m_pInstPorts(pInstPorts), m_idx(0) {}
		  //		CInstPortIter(CInstPortIter &iter) { *this = iter; }

		bool end() { return m_idx == (int)m_pInstPorts->size(); }
		void operator ++ (int) { m_idx += 1; }
		CInstPort * operator -> () { return &(*m_pInstPorts)[m_idx]; }

	private:
		vector<CInstPort>	*m_pInstPorts;
		int					m_idx;
	};

	struct CModule;
	struct CInst {
		CInst() : m_bDefparamInit(false) {}
		CInstPortIter GetInstPortIter() { return CInstPortIter(&m_instPorts); }
		CPortSignal *GetPortSignal(char *pName);
		string &GetModuleName() { return m_pModule->m_name; }

		string				m_name;
		int					m_lvl;
		bool				m_bDefparamInit;
		string				m_initStr;
		CModule				*m_pModule;
		CLineInfo			m_lineInfo;
		vector<CInstPort>	m_instPorts;
		vector<CDefparam>	m_defparams;
		vector<CSynAttrib>	m_synAttrib;
	};

	typedef map<string, CInst *>		InstMap;
	typedef pair<string, CInst *>		InstMap_ValuePair;
	typedef InstMap::iterator			InstMap_Iter;
	typedef pair<InstMap_Iter, bool>	InstMap_InsertPair;

	struct CSignal {
		CSignal() : m_bIndexed(false), m_bUnconnected(false), m_pIndexedSignalPorts(0) {}
		int GetLoadCnt();

		int GetWidth() { return m_bIndexed ? (m_upperIdx - m_lowerIdx + 1) : 1; }

		string					m_name;
		ESigType				m_sigType;
		bool					m_bDeclared;
		bool					m_bIndexed;
		bool					m_bClock;
		bool					m_bPortUsed;	// flag used when parsing an instance's port list
		bool					m_bUnconnected;
		CLineInfo				m_lineInfo;
		int						m_upperIdx;
		int						m_lowerIdx;
		int						m_portIdx;	// position in port list
		int						m_lvl;

		union {
			vector<SignalPortVector>	*m_pIndexedSignalPorts;
			SignalPortVector			*m_pSignalPorts;
		};
	};

	typedef map<string, CSignal *>		SignalMap;
	typedef pair<string, CSignal *>		SignalMap_ValuePair;
	typedef SignalMap::iterator			SignalMap_Iter;
	typedef pair<SignalMap_Iter, bool>	SignalMap_InsertPair;

	struct CModule {
		CModule() : m_bPrim(false), m_bDummyLoad(false), m_useCnt(0) {}

		string					m_name;
		bool					m_bPrim;
		bool					m_bDummyLoad;
		int						m_useCnt;
		SignalMap				m_signalMap;
		InstMap					m_instMap;
		DefparamMap				m_defparamMap;
		vector<CSynAttrib>		m_synAttrib;
		string					m_ifdefName;
		vector<string>			m_ifdefLine;

		CSignal *FindPort(string &name) {
			SignalMap_Iter iter = m_signalMap.find(name);
			if (iter == m_signalMap.end())
				return 0;
			return iter->second;
		}
		CInst *FindInst(string &name) {
			InstMap_Iter iter = m_instMap.find(name);
			if (iter == m_instMap.end())
				return 0;
			return iter->second;
		}
	};

	typedef map<string, CModule>		ModuleMap;
	typedef pair<string, CModule>		ModuleMap_ValuePair;
	typedef ModuleMap::iterator			ModuleMap_Iter;
	typedef pair<ModuleMap_Iter, bool>	ModuleMap_InsertPair;

	struct CPortSignalInfo {
		CPortSignalInfo(CSignal *pSignal, int upperBit, int lowerBit) : m_bConst(false), m_pSignal(pSignal), m_upperBit(upperBit), m_lowerBit(lowerBit) {}
		CPortSignalInfo(uint64 value, int upperBit, int lowerBit) : m_bConst(true), m_value(value), m_upperBit(upperBit), m_lowerBit(lowerBit) {}

		bool	m_bConst;
		uint64	m_value;
		CSignal	*m_pSignal;
		int		m_upperBit;
		int		m_lowerBit;
	};

public:
	CDesign(const char *pDsn);
	~CDesign(void);

	void Parse();
	void PerformModuleChecks();
	void GetTotalInstCnt(FILE *fp, CModule *pModule);
	void GetTotalInstCnt(CModule *pModule);
	void SetInstLvl(FILE *fp, char *pInstNamePrefix);
	void WritePrpFile(const string &vexName);
	void GenConnRpt(FILE *fp);
	void GenFFChainRpt(FILE *fp);
	void GenVppFile(char *pName, bool bIsLedaEnabled);
	void GenFixture(const string &verilogName);
	void GenVexTestRunAll(const string &runAllFile);
	void GenFixtureCommon(const string &commonDir);
	void GenFixtureCommonRandomH(const string &randomHFile);
	void GenFixtureCommonRandomCpp(const string &randomCppFile);
	void GenFixtureCommonVcdDiff(const string &vcdDiffFile);
	void GenFixtureModule(const string &moduleDir, const string &primFileName, CModule *pModule);
	void GenFixtureModuleRunTest(const string &runTestFile, CModule *pModule);
	void GetModulePorts(CModule *pModule, CPortList &portList);
	void GenFixtureModuleSystemC(const string &systemcDir, const string &primFileName, CModule *pModule, CPortList &portList);
	void GenFixtureModuleSystemCScript(const string &systemcScript);
	void GenFixtureModuleSystemCMainCpp(const string &mainCppFile, const string &primFileName, CModule *pModule, CPortList &portList);
	void GenFixtureModuleVerilog(const string &verilogDir, const string &primFileName, CModule *pModule, CPortList &portList);
	void GenFixtureModuleVerilogScript(const string &vcsScriptFile, const string &primFileName);
	void GenFixtureModuleVerilogSimvCmds(const string &simvCmdsFile);
	void GenFixtureModuleVerilogPliTab(const string &pliTabDir);
	void GenFixtureModuleVerilogFixtureV(const string &fixtureVFile, CModule *pModule, CPortList &portList);
	void GenFixtureModuleVerilogMainCpp(const string &mainCppFile, CModule *pModule, CPortList &portList);
	void MkDir(const string &newDir);

private:
	void ModuleMapInsert(string name, CPrimPort *pPorts);
	CSignal *SignalMapInsert(string &name, int upperBit, int lowerBit);
	CInst *InstMapInsert(string &name);
	bool ModuleMapInsert(string &name);
	void DefparamMapInsert(string &heirName, bool bIsString, string &paramValue);

	void ParseModule();
	void ParseSignal(ESigType sigType);
	void ParseIfdef();
	void ParseDefparam();
	void ParseSynthesis();
	//void ParsePrim(CPrim *pPrim);
	void ParseModuleInst(CModule *pModule);
	bool ParsePortSignalList(CInst *pInst, int portIdx, unsigned int &portBit);
	bool ParsePortSignal(CSignal *&pSignal, int &upperBit, int &lowerBit);
	bool ParseConcatSignal(vector<CPortSignalInfo> &signalList);

	bool IsInputPreLevelZero(char *pInstNamePrefix, CSignal *pSignal, int sigBit);
	void SetInstLvl(CSignal *pSignal, int sigBit, int lvl);
	void SetInstLvl(CInst *pInst, int lvl);

	uint64 GenDefparamInit(CDefparam *pDefparam, char lutSizeCh);
	bool ParseInitExpr(CDefparam *pDefparam, int i, uint32 &usedMask, COperand &op);
	bool EvalInitExpr(CDefparam *pDefparam, EToken tk, vector<COperand> &operandStack, vector<EToken> &operatorStack);

	void CheckPortDeclarations();
	void ClearPortUsedFlags(CModule *pModule);
	void CheckPortUsedFlags(CModule *pModule);

	bool IsDInputSignalDriverFDPrim(CInst *pInst);

	string GetVppName(string &str);

private:
	static CPrimPort X_IPAD_PrimPorts[];
	static CPrimPort X_FF_PrimPorts[];
	static CPrimPort X_OPAD_PrimPorts[];
	static CPrimPort X_OBUF_PrimPorts[];
	static CPrimPort X_BUF_PrimPorts[];
	static CPrimPort X_INV_PrimPorts[];
	static CPrimPort X_IBUFDS_PrimPorts[];
	static CPrimPort X_PLL_ADV_PrimPorts[];
	static CPrimPort X_LUT5_PrimPorts[];
	static CPrimPort X_LUT6_PrimPorts[];
	static CPrimPort LUT6_PrimPorts[];
	static CPrimPort LUT5_PrimPorts[];
	static CPrimPort LUT4_PrimPorts[];
	static CPrimPort LUT3_PrimPorts[];
	static CPrimPort LUT2_PrimPorts[];
	static CPrimPort LUT1_PrimPorts[];
	static CPrimPort LUT6_L_PrimPorts[];
	static CPrimPort LUT5_L_PrimPorts[];
	static CPrimPort LUT4_L_PrimPorts[];
	static CPrimPort LUT3_L_PrimPorts[];
	static CPrimPort LUT2_L_PrimPorts[];
	static CPrimPort LUT1_L_PrimPorts[];
	static CPrimPort FD_PrimPorts[];
	static CPrimPort FDE_PrimPorts[];
	static CPrimPort FDR_PrimPorts[];
	static CPrimPort FDRE_PrimPorts[];
	static CPrimPort FDS_PrimPorts[];
	static CPrimPort FDSE_PrimPorts[];
	static CPrimPort FDRS_PrimPorts[];
	static CPrimPort FDRSE_PrimPorts[];
	static CPrimPort X_ZERO_PrimPorts[];
	static CPrimPort GND_PrimPorts[];
	static CPrimPort X_ONE_PrimPorts[];
	static CPrimPort VCC_PrimPorts[];
	static CPrimPort X_CARRY4_PrimPorts[];
	static CPrimPort CARRY4_PrimPorts[];
	static CPrimPort X_MUX2_PrimPorts[];
	static CPrimPort X_CKBUF_PrimPorts[];
	static CPrimPort SRL16_PrimPorts[];
	static CPrimPort SRL16E_PrimPorts[];
	static CPrimPort MUXF7_PrimPorts[];
	static CPrimPort MUXF8_PrimPorts[];
	static CPrimPort DUMMY_LOAD_PrimPorts[];
	static CPrimPort RAM32X1D_PrimPorts[];
	static CPrimPort DSP48E_PrimPorts[];
	static CPrimPort DSP48E1_PrimPorts[];
	static CPrimPort RAMB36E1_PrimPorts[];

private:
	//CPrim *FindPrim(string primName) {
	//	PrimMap_Iter iter = m_primMap.find(primName);
	//	if (iter == m_primMap.end())
	//		return 0;
	//	return iter->second;
	//}
	CModule *FindModule(string &name) {
		ModuleMap_Iter iter = m_moduleMap.find(name);
		if (iter == m_moduleMap.end())
			return 0;
		return &iter->second;
	}
	CSignal *SignalMapFind(string &name) {
		SignalMap_Iter iter = m_pModule->m_signalMap.find(name);
		if (iter == m_pModule->m_signalMap.end())
			return 0;
		return iter->second;
	}

private:

	ModuleMap		m_moduleMap;
	CModule			*m_pModule;

	CSignal			m_zeroSignal;
	CSignal			m_oneSignal;
};
