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

class CHtfeOperand;
class CHtfeParam;
class CScSensitiveTbl;
class CHtfeIdentTbl;
class CHtfeIdentTblIter;
class CHtIdentElem;
class CHtfeStatement;

enum EPortDir { port_zero, port_in, port_out, port_inout, port_signal };
enum ESeqState { ss_PreClk, ss_PostClk, ss_Continuous, ss_Function, ss_Unknown, ss_Error };

struct CHtDistRamWeWidth {
	CHtDistRamWeWidth() { m_highBit = -1; m_lowBit = -1; }
	CHtDistRamWeWidth(short highBit, short lowBit) : m_highBit(highBit), m_lowBit(lowBit) {}
	short		m_highBit;
	short		m_lowBit;
};

struct CHtfeTypeAttrib {
	CHtfeTypeAttrib() {
		m_bIsConst = m_bIsHtPrim = m_bIsHtNoLoad = m_bIsHtState = m_bIsHtLocal = m_bIsStatic = false;
	}

	bool m_bIsConst;
	bool m_bIsHtPrim;
	bool m_bIsHtNoLoad;
	bool m_bIsHtState;
	bool m_bIsHtLocal;
	bool m_bIsStatic;
	vector<string> m_htPrimClkList;
};

/*********************************************************
** CRefInfo
*********************************************************/

struct CRefInfo {
	CRefInfo() { m_bIsRead = m_bIsWritten = m_bIsAssignOutput = m_bIsHtPrimOutput = false; m_seqState = ss_PreClk; }

	bool		m_bIsRead:1;
	bool		m_bIsWritten:1;
	bool		m_bIsAssignOutput:1;
	bool		m_bIsHtPrimOutput:1;
	ESeqState	m_seqState:8;
	string		m_instanceName;
};

class CHtfeIdent
{
public:
    friend class CHtfeSignal;

    enum EIdentId { id_new, id_enum, id_struct, id_variable, id_function, id_const, id_class, id_nameSpace, id_enumType, id_instancePort, id_intType };
    enum EFuncId { fid_unknown=0, fid_and_reduce, fid_nand_reduce, fid_or_reduce, fid_nor_reduce, fid_xor_reduce, fid_xnor_reduce, fid_range };

protected:
    CHtfeIdent() : m_name("") { Init(); }

public:

	void Init() {
		m_id = id_new;
		m_pType = 0;
		m_pPrevHier = 0;
		m_pIdentTbl = 0;
		m_pFlatTbl = 0;
		m_pFlatIdent = 0;
		m_pHierIdent = 0;
		m_sizeof = -1;
		m_beginBlockId = 0;
		m_nextUniqueId = 0;		// unique ID for flat ident table
		m_unnamedId = 0;		// unique ID for unnamed structures within a heirarchy
		m_pSensitiveTbl = 0;
		m_sensitiveType = CHtfeLex::tk_eof;
		m_portDir = port_zero;
		m_pParenOperator = 0;
		m_pClockIdent = 0;
		m_pClkEnIdent = 0;
		m_pInstancePortTbl = 0;
		m_pFunctionCallList = 0;
		m_pTokenList = 0;
		m_pStatementHead = 0;
		m_ppStatementInsertTail = 0;
		m_ppStatementAppendTail = 0;
		m_pAssignmentStatement = 0;
		m_pMemInitStatementHead = 0;
		m_ppMemInitStatementTail = 0;
		m_pSignal = 0;
		m_pAssigningMethod = 0;
		m_pPortSignal = 0;
		m_pCallerList = 0;	// list of callers (can be other functions or methods)
		m_pReaderList = 0;	// list of functions or methods where variable is read
		m_pClockList = 0;	// used by fixture generation
		m_pHtAttribList = 0;
		m_constValue = 0;
		m_width = 0;
	
		// width of bit field types
        m_opWidth = 0;
		m_structPos = 0;	// starting position of member within structure
        m_declStatementLevel = 0;
        m_funcId = fid_unknown;
		m_pUserConvType = 0;
		m_pOverloadedNext = 0;
		m_pMemVar = 0;

		m_bIsReturnRef = false;
        m_bConstVarRead = false;
        m_bIsConstVar = false;
		m_bIsMethod = false;
		m_bIsBodyDefined = false;  // for function declarations
		m_bIsParamListDeclared = false;
		m_bIsType = false;
		m_bIsVariableWidth = false;
		m_bIsModule = false;
		m_bIsScFixture = false;
		m_bIsScFixtureTop = false;
		m_bIsScTopology = false;
		m_bIsScTopologyTop = false;
		m_bIsRegister = false;
		m_bIsRegClkEn = false;
		m_bIsBlockRam = false;
		m_bIsLocal = false;  // function local type
		m_bIsCtor = false;
		m_bIsParenOverloaded = false;
		m_bIsMethodDefined = false;
		m_bIsStatic = false;
		m_bIsTypeDef = false;
		m_bIsParam = false;
		m_bIsConst = false;
		m_bIsSigned = false;
        m_bPromoteToSigned = false;
		m_bIsClock = false;
		m_bIsReference = false;
		m_bIsInputFile = false;
		m_bIsSeqCheck = false;
		m_bIsPrimOutput = false;
		m_bIsNoLoad = false;
		m_bIsReadOnly = false;
		m_bIsWriteOnly = false;
		m_bIsRemoveIfWriteOnly = false;
		m_bIsScState = false;
		m_bIsScLocal = false;
		m_bIsScPrim = false;
		m_bIsScClockedPrim = false;
		m_bIsScPrimInput = false;
		//m_bIsHtPrimOutput = false;
		//m_bIsAssignOutput = false;
		m_bIsUserDefinedType = false;
		m_bIsUserDefinedFunc = false;
		m_bIsAutoDecl = false;
		m_bIsFlatIdent = false;
		m_bIsArrayIdent = false;
		m_bIsMultipleWriterErrorReported = false;
		m_bIsMultipleReaderErrorReported = false;
		m_bIsIgnoreSignalCheck = false;
		m_bGlobalReadRef = false;
		m_bIsFunctionCalled = false;
		m_bIsOverloadedOperator = false;
		m_bIsUserConversion = false;
		m_bNullConvOperator = false;
		m_bIsConstructor = false;
		m_bIsMemVarWrEn = false;
		m_bIsMemVarWrData = false;

		// id_types
		m_bIsStruct = false;

		// HtQueue state
		m_bIsHtBlockQue = false;
		m_bIsHtDistQue = false;
		m_bIsHtQueueRam = false;
		m_bIsHtQueuePushSeen = false;
		m_bIsHtQueuePopSeen = false;
		m_bIsHtQueueFrontSeen = false;
		m_bIsHtQueuePushClockSeen = false;
		m_bIsHtQueuePopClockSeen = false;

		// HtMemory state
		m_bIsHtDistRam = false;
		m_bIsHtBlockRam = false;
		m_bIsHtMrdBlockRam = false;
		m_bIsHtMwrBlockRam = false;
		m_bIsHtBlockRamDoReg = false;
		m_bIsHtMemoryReadAddrSeen = false;
		m_bIsHtMemoryReadMemSeen = false;
		m_bIsHtMemoryReadClockSeen = false;
		m_bIsHtMemoryWriteAddrSeen = false;
		m_bIsHtMemoryWriteMemSeen = false;
		m_bIsHtMemoryWriteClockSeen = false;
		m_scMemoryAddrWidth1 = 0;
		m_scMemoryAddrWidth2 = 0;
		m_scMemorySelWidth = 0;
		m_pHtDistRamWeWidth = 0;
	};

	~CHtfeIdent() {
        m_id = id_new;
    }
    CHtfeIdent *GetThis() { return this; }

	static void InitVerilogKeywordTbl();

	const string GetVerilogName() const {
		const char *pSrc = m_name.c_str();
		char dst[256], *pDst = dst;
		do {
			switch (*pSrc) {
			case '[': *pDst++ = '$'; break;
			case ']': break;
			default: *pDst++ = *pSrc; break;
			}
		} while (*pSrc++);
		string instName = dst;
		return instName;
	}

	void SetName(string const & name) { m_name = name; }

	const string GetName(CHtfeOperand *pOp = 0) const {
		return m_inlineName.size() > 0 ? m_inlineName : m_name;
	}
	const string GetSignalName(CHtfeOperand *pOp = 0) const {
		string name = m_inlineName.size() > 0 ? m_inlineName : m_name;
		if (name[name.size() - 1] == '>') name += " ";
		return name;
	}
	void SetInlineName(string const & inlineName) {
		m_inlineName = inlineName;
	}

	CHtfeIdent *	SetSizeof(int bytes) {
		m_sizeof = bytes;
		return this;
	}

	CHtfeIdent *	SetIsSigned(bool bIsSigned=true);
	bool IsSigned() const { return m_bIsSigned; }

    void SetFuncId(EFuncId funcId) { m_funcId = funcId; }
    EFuncId GetFuncId() { return m_funcId; }

	CHtfeIdent *	SetPromoteToSigned(bool bPromoteToSigned=true);

	bool IsPromoteToSigned() const { return m_bPromoteToSigned; }

	bool GetSizeof(uint64_t &bytes) {
		bytes = m_sizeof;
		return m_sizeof > 0;
	}
 
	CHtfeIdent *SetType(CHtfeIdent *pType);
    CHtfeIdent *GetType() { return m_pType; }
    CHtfeIdent *GetBaseType() { return m_pType ? m_pType->GetType() : 0; }

    const CLineInfo &GetLineInfo() { return m_lineInfo; }
    void SetLineInfo(const CLineInfo &lineInfo);
    EIdentId GetId() const { return m_id; }
    char *GetIdStr() const {
        switch (m_id) {
            case id_enum: return "enum";
            case id_variable: return "var";
            case id_function: return "func";
            case id_const: return "const";
            case id_class: return "class";
            default: return "???";
        }
    }
    CHtfeIdent *SetId(EIdentId id);
    bool IsPort() {
        return m_id == id_variable && m_portDir != port_zero && m_portDir != port_signal;
    }
    bool IsInPort() { return m_id == id_variable && m_portDir == port_in; }
    bool IsOutPort() { return m_id == id_variable && m_portDir == port_out; }
    bool IsSignal() { 
        return m_id == id_variable && m_portDir == port_signal;
    }

    size_t GetDimenCnt() const { return m_dimenList.size(); }
	void SetDimenList(vector<int> &dimenList) {
		m_dimenList = dimenList;
		if (m_pFlatIdent)
			m_pFlatIdent->SetDimenList(dimenList);
	}
	vector<int> &GetDimenList() { return m_dimenList; }
    void SetDimen(size_t idx, int dimen) {
        m_dimenList[idx] = dimen;
        if (m_pFlatIdent)
            m_pFlatIdent->SetDimen(idx, dimen);
    }
    int GetDimen(size_t idx) { return m_dimenList[idx]; }

	void SetLinearIdx(int linearIdx) { m_linearIdx = linearIdx; }
	int GetLinearIdx() { return m_linearIdx; }

    void AppendParam(CHtfeParam const &param) { m_paramList.push_back(param); }
    int GetParamCnt() { return m_paramList.size(); }
    CHtfeIdent *GetParamType(int paramId) { return m_paramList[paramId].GetType(); }
    string &GetParamName(int paramId) { return m_paramList[paramId].GetName(); }
    CHtfeIdent *GetParamIdent(int paramId) { return m_paramList[paramId].GetIdent(); }
    void SetParamIdent(int paramId, CHtfeIdent *pIdent) { m_paramList[paramId].SetIdent(pIdent); }
    bool GetParamIsConst(int paramId) { return m_paramList[paramId].GetIsConst(); }
	bool GetParamIsRef(int paramId) { return m_paramList[paramId].GetIsReference(); }
    bool GetParamIsScState(int paramId) { return m_paramList[paramId].GetIsScState(); }
    int GetParamWidth(int paramId) { return m_paramList[paramId].GetWidth(); }
    int GetParamOutputCnt() {
        int outputCnt = 0;
        for (int paramId = 0; paramId < GetParamCnt(); paramId += 1) {
            if (!GetParamIsConst(paramId))
                outputCnt += 1;
        }
        return outputCnt;
    }
    void SetParamName(int paramId, string &name) { m_paramList[paramId].SetName(name); }
    void SetParamName(int paramId, const char *pName) {
		const string name(pName);
		m_paramList[paramId].SetName(name);
	}

	void SetIsParamNoload(int paramId, bool bNoload=true) { m_paramList[paramId].SetIsNoload(bNoload); }
	bool IsParamNoload(int paramId) { return m_paramList[paramId].IsNoload(); }

	void SetParamDefaultValue(int paramId, CConstValue & value) {
		m_paramList[paramId].SetDefaultValue(value);
	}
	bool ParamHasDefaultValue(int paramId) { return m_paramList[paramId].HasDefaultValue(); }
	CConstValue & GetParamDefaultValue(int paramId) { return m_paramList[paramId].GetDefaultValue(); }

    void InsertSensitive(CHtfeIdent * pIdent, int elemIdx);
    CScSensitiveTbl &GetSensitiveTbl();
    void SetSensitiveType(CHtfeLex::EToken token) { m_sensitiveType = token; }
    CHtfeLex::EToken GetSensitiveType() { return m_sensitiveType; }

    void InsertArrayElements(int structBitPos=-1);

	void NewFlatTable();
    CHtfeIdent *InsertIdent(const string &name, bool bInsertFlatTbl=true, bool bAllowOverloading=false);
	CHtfeIdent *InsertIdent(const string &heirVarName, const string &flatVarName);
	CHtfeIdent * InsertFlatIdent(CHtfeIdent * pHierIdent, const string &flatVarName);
	void DeleteIdent(const string &name);
    CHtfeIdent *FindIdent(const string &name) const;
    CHtfeIdent *FindIdent(const char *pName) const;
    CHtfeIdentTbl &GetIdentTbl();
    const CHtfeIdentTbl &GetIdentTbl() const {
        return *m_pIdentTbl;
    }
    CHtfeIdent *InsertType(string &name) { return InsertIdent(name, false)->SetIsType(); }
    CHtfeIdent *InsertType(const char *pName) { const string name(pName); return InsertIdent(name, false)->SetIsType(); }
    CHtfeIdent *FindType(string &name) { CHtfeIdent *pType = FindIdent(name); return pType && pType->IsType() ? pType : 0; }

  	void			SetFlatIdentTbl(CHtfeIdentTbl & flatTbl) { m_pFlatTbl = &flatTbl; }
	CHtfeIdentTbl &	GetFlatIdentTbl();

	CHtfeIdent *		FindHierFunction() {
		CHtfeIdent *pIdent = this;
		while (pIdent && !pIdent->IsFunction())
			pIdent = pIdent->GetPrevHier();
		Assert(pIdent);
		return pIdent;
	}

	bool			IsHierModule() {
		CHtfeIdent *pIdent = this;
		while (pIdent && !pIdent->IsModule())
			pIdent = pIdent->GetPrevHier();
		return pIdent != 0;
	}

	CHtfeIdent *		FindHierModule() {
		CHtfeIdent *pIdent = this;
		while (pIdent && !pIdent->IsModule())
			pIdent = pIdent->GetPrevHier();
		//Assert(pIdent);
		return pIdent;
	}

	CHtfeIdent *		FindHierMethod() {
		CHtfeIdent *pIdent = this;
		while (pIdent && !pIdent->IsMethod())
			pIdent = pIdent->GetPrevHier();
		return pIdent;
	}

	CHtfeIdent *		FindHierMethodOrFunction() {
		CHtfeIdent *pIdent = this;
		while (pIdent && !pIdent->IsMethod() && !pIdent->IsFunction())
			pIdent = pIdent->GetPrevHier();
		return pIdent;
	}

	bool			IsHierMethodOrFunction() {
		CHtfeIdent *pIdent = this;
		while (pIdent && !pIdent->IsMethod() && !pIdent->IsFunction())
			pIdent = pIdent->GetPrevHier();
		return pIdent != 0;
	}

	CHtfeIdent * FindHierIdent(const string &name) const {
		const CHtfeIdent *pHier = this;
		CHtfeIdent *pIdent = 0;
		while (pHier && pIdent == 0) {
			pIdent = pHier->FindIdent(name);
			pHier = pHier->GetPrevHier();
		}
		return pIdent;
	}

	void AppendFunctionCall(CHtfeIdent *pIdent) {
        if (m_pFunctionCallList == 0)
            m_pFunctionCallList = new vector<CHtfeIdent *>;
        m_pFunctionCallList->push_back(pIdent);
    }
    int GetFunctionCallCount() { return m_pFunctionCallList == 0 ? 0 : m_pFunctionCallList->size(); }
    CHtfeIdent *GetFunctionCallIdent(int i) { return (*m_pFunctionCallList)[i]; }

	void ValidateIndexList(vector<CHtfeOperand *> *pIndexList);
	void ValidateIndexList(CHtfeOperand *pOperand);
	size_t GetDimenSize(int idx) const { return m_dimenList[idx]; }

	bool DimenIter(vector<int> &refList);
	bool DimenIter(vector<int> &refList, vector<bool> &constList);

	void AllocRefInfo() { Assert(m_refInfo.size() == 0); m_refInfo.resize(GetDimenElemCnt()); }

	void SetIsAssignOutput(CHtfeOperand *pOp);
	void SetIsHtPrimOutput(CHtfeOperand *pOp);

	void SetReadRef(CHtfeOperand *pOp);
	void SetReadRef(int elemIdx) {
		if (GetPrevHier() && GetPrevHier()->IsArrayIdent()) { GetPrevHier()->SetReadRef(elemIdx); return; }
		if (!IsFlatIdent()) { m_pFlatIdent->SetReadRef(elemIdx); return; }
		if ((int)m_refInfo.size() < GetDimenElemCnt())
			m_refInfo.resize(GetDimenElemCnt());
		Assert(elemIdx < (int)m_refInfo.size());
		m_refInfo[elemIdx].m_bIsRead = true;
	}

	void SetWriteRef(CHtfeOperand *pOp);
	void SetWriteRef(int elemIdx) {
		if (GetPrevHier() && GetPrevHier()->IsArrayIdent()) { GetPrevHier()->SetWriteRef(elemIdx); return; }
		if (!IsFlatIdent()) { m_pFlatIdent->SetWriteRef(elemIdx); return; }
		if ((int)m_refInfo.size() < GetDimenElemCnt())
			m_refInfo.resize(GetDimenElemCnt());
		Assert(elemIdx < (int)m_refInfo.size());
		m_refInfo[elemIdx].m_bIsWritten = true;
	}

	bool IsAssignOutput() {
		if (!IsFlatIdent())
			return m_pFlatIdent->IsAssignOutput();
		else {
			for (size_t idx = 0; idx < m_refInfo.size(); idx += 1)
				if (m_refInfo[idx].m_bIsAssignOutput)
					return true;
			return false;
		}
	}
	bool IsHtPrimOutput() {
		if (!IsFlatIdent())
			return m_pFlatIdent->IsHtPrimOutput();
		else {
			for (size_t idx = 0; idx < m_refInfo.size(); idx += 1)
				if (m_refInfo[idx].m_bIsHtPrimOutput)
					return true;
			return false;
		}
	}
	bool IsReadRef() {
		if (!IsFlatIdent())
			return m_pFlatIdent->IsReadRef();
		else {
			for (size_t idx = 0; idx < m_refInfo.size(); idx += 1)
				if (m_refInfo[idx].m_bIsRead)
					return true;
			return false;
		}
	}
	bool IsWriteRef() {
		if (!IsFlatIdent())
			return m_pFlatIdent->IsWriteRef();
		else {
			for (size_t idx = 0; idx < m_refInfo.size(); idx += 1)
				if (m_refInfo[idx].m_bIsWritten)
					return true;
			return false;
		}
	}

	bool IsAssignOutput(int elemIdx) {
		if (elemIdx < 0)
			return IsAssignOutput();
		if (!IsFlatIdent())
			return m_pFlatIdent->IsAssignOutput(elemIdx);
		else
			return (m_refInfo.size() == 0) ? false : m_refInfo[elemIdx].m_bIsAssignOutput;
	}
	bool IsHtPrimOutput(int elemIdx) {
		if (elemIdx < 0)
			return IsHtPrimOutput();
		if (!IsFlatIdent())
			return m_pFlatIdent->IsHtPrimOutput(elemIdx);
		else
			return (m_refInfo.size() == 0) ? false : m_refInfo[elemIdx].m_bIsHtPrimOutput;
	}
	bool IsReadRef(int elemIdx) {
		if (elemIdx < 0)
			return IsReadRef();
		if (!IsFlatIdent())
			return m_pFlatIdent->IsReadRef(elemIdx);
		else
			return (m_refInfo.size() == 0) ? false : m_refInfo[elemIdx].m_bIsRead;
	}
	bool IsWriteRef(int elemIdx) {
		if (elemIdx < 0)
			return IsWriteRef();
		if (!IsFlatIdent())
			return m_pFlatIdent->IsWriteRef(elemIdx);
		else
			return (m_refInfo.size() == 0) ? false : m_refInfo[elemIdx].m_bIsWritten;
	}

	void ClearRefInfo() {
		for (size_t elemIdx = 0; elemIdx < m_refInfo.size(); elemIdx += 1)
			m_refInfo[elemIdx].m_bIsRead = m_refInfo[elemIdx].m_bIsWritten = false;
	}

    CHtfeIdent *SetWidth(int width);
    int GetWidth(CLineInfo lineInfo=CLineInfo()) const;

    int GetMinWidth() const {
        if (m_id == id_const || m_id == id_enumType) return m_constValue.GetMinWidth();
        return GetWidth();
    }

    CHtfeIdent *SetOpWidth(int width);
    int GetOpWidth() const {
		if (m_opWidth > 0)
			return m_opWidth;
		else if (m_pType)
            return m_pType->GetOpWidth();
		else {
			ErrorMsg(PARSE_ERROR, m_lineInfo, "zero width for type %s", GetName().c_str());
			ErrorMsg(PARSE_FATAL, m_lineInfo, "Previous errors prohibit further parsing");
			return 0;
		}
    }

    bool IsIdEnumType() const { return m_id == id_enumType; }
    bool IsIdEnum() const { return m_id == id_enum; }
    bool IsIdConst() const { return m_id == id_const; }
    CConstValue & GetConstValue() { return m_constValue; }
    void SetConstValue(const CConstValue & constValue) {
		m_constValue = constValue;
		if (m_pFlatIdent) m_pFlatIdent->SetConstValue(constValue);
	}

	void SetIsReturnRef(bool bIsReturnRef) { m_bIsReturnRef = bIsReturnRef; }
	bool IsReturnRef() { return m_bIsReturnRef; }

    void SetConstVarRead(bool bVarRead) {
        m_bConstVarRead = bVarRead;
        if (m_pHierIdent)
            m_pHierIdent->SetConstVarRead(bVarRead);
    }
    bool IsConstVarRead() { return m_bConstVarRead; }

    void SetIsConstVar(bool bIsConstVar) { m_bIsConstVar = bIsConstVar; }
    bool IsConstVar() { return m_bIsConstVar; }

    void SetIsBodyDefined(bool bIsBodyDefined=true) { m_bIsBodyDefined = bIsBodyDefined; }
    bool IsBodyDefined() { return m_bIsBodyDefined; }

    void SetIsParamListDeclared(bool bDeclared=true) { m_bIsParamListDeclared = bDeclared; }
    bool IsParamListDeclared() { return m_bIsParamListDeclared; }

    void SetIsMethod(bool bIsMethod=true) { m_bIsMethod = bIsMethod; }
    bool IsMethod() const { return m_bIsMethod; }

	void SetIsGlobalReadRef(bool bGlobalReadRef=true) { m_bGlobalReadRef = bGlobalReadRef; }
	bool IsGlobalReadRef() { return m_bGlobalReadRef; }

	void SetIsFunctionCalled(bool bCalled) { m_bIsFunctionCalled = bCalled; }
	bool IsFunctionCalled() { return m_bIsFunctionCalled; }

	bool IsVariable() const { Assert(this);  return m_id == id_variable; }
	bool IsFunction() const { Assert(this);  return m_id == id_function; }
    bool IsType() const { return m_bIsType; }

    CHtfeIdent *GetPrevHier() const { 
		if (m_pHierIdent)
			return m_pHierIdent->GetPrevHier();
		else
			return m_pPrevHier;
	}
    void SetPrevHier(CHtfeIdent *pHier) {
		m_pPrevHier = pHier;
		if (m_pFlatTbl == 0)
			SetFlatIdentTbl(m_pPrevHier->GetFlatIdentTbl()); 
	}

	bool IsStructMember() const {
		Assert(this);
		return m_pPrevHier && m_pPrevHier->IsStruct();
	}

    CHtfeIdent *SetIsType(bool bIsType=true) { m_bIsType = bIsType; return this; }

    void SetIsVariableWidth(bool bIsVariableWidth=true) { m_bIsVariableWidth = bIsVariableWidth; }
    bool IsVariableWidth() { return m_bIsVariableWidth; }

    void SetIsLocal(bool bIsLocal = true) { m_bIsLocal = bIsLocal; }
    bool IsLocal() const { return m_bIsLocal; }

    void SetIsCtor(bool bIsCtor = true) { m_bIsCtor = bIsCtor; }
    bool IsCtor() const { return m_bIsCtor; }

    void SetIsMethodDefined(bool bIsMethodDefined=true) { m_bIsMethodDefined = bIsMethodDefined; }
    bool IsMethodDefined() const { return m_bIsMethodDefined; }

    bool IsParenOverloaded() const { return m_bIsParenOverloaded; }
    void SetIsParenOverloaded(bool bIsParenOverloaded=true) { m_bIsParenOverloaded = bIsParenOverloaded; }

    void SetIsModule(bool bIsModule=true) { m_bIsModule = bIsModule; }
    bool IsModule() const { return m_bIsModule; }

    void SetIsScFixture(bool bIsScFixture=true) { m_bIsScFixture = bIsScFixture; }
    bool IsScFixture() const { return m_bIsScFixture; }

    void SetIsScFixtureTop(bool bIsScFixtureTop=true) { m_bIsScFixtureTop = bIsScFixtureTop; }
    bool IsScFixtureTop() const { return m_bIsScFixtureTop; }

    void SetIsScTopologyTop(bool bIsScTopologyTop=true) { m_bIsScTopologyTop = bIsScTopologyTop; }
    bool IsScTopologyTop() const { return m_bIsScTopologyTop; }

    void SetIsScTopology(bool bIsScTopology=true) { m_bIsScTopology = bIsScTopology; }
    bool IsScTopology() const { return m_bIsScTopology; }

    void SetIsRegister(bool bIsRegister=true);
    bool IsRegister() const { return m_bIsRegister; }

    void SetIsRegClkEn(bool bIsRegClkEn=true) { m_bIsRegClkEn = bIsRegClkEn; }
    bool IsRegClkEn() const { return m_bIsRegClkEn; }

	void SetHtDistRamWeWidth(vector<CHtDistRamWeWidth> *pWeWidth);
	vector<CHtDistRamWeWidth> *GetHtDistRamWeWidth() { return m_pHtDistRamWeWidth; }

	void SetHtDistRamWeSubRange(int highBit, int lowBit) {
		if (highBit-lowBit+1 == GetWidth())
			return;

		vector<CHtDistRamWeWidth>::iterator iter;

		// split current write widths so current subrange starts and ends on a boundry
		for (iter = (*m_pHtDistRamWeWidth).begin(); iter < (*m_pHtDistRamWeWidth).end(); iter++) {
			if (lowBit == iter->m_lowBit)
				break;
			if (lowBit > iter->m_lowBit && lowBit <= iter->m_highBit) {
				int origLowBit = iter->m_lowBit;
				iter->m_lowBit = lowBit;
				(*m_pHtDistRamWeWidth).insert(iter, CHtDistRamWeWidth(lowBit-1, origLowBit));
				break;
			}
		}

		for (iter = (*m_pHtDistRamWeWidth).begin(); iter < (*m_pHtDistRamWeWidth).end(); iter++) {
			if (highBit == iter->m_highBit)
				break;
			if (highBit >= iter->m_lowBit && highBit < iter->m_highBit) {
				int origLowBit = iter->m_lowBit;
				iter->m_lowBit = highBit+1;
				(*m_pHtDistRamWeWidth).insert(iter, CHtDistRamWeWidth(highBit, origLowBit));
				break;
			}
		}
	}
    bool IsHtDistRamWe() { return m_pHtDistRamWeWidth != 0; }

	void SetIsReadOnly(bool bIsReadOnly=true);
    bool IsReadOnly() { return m_bIsConst || m_bIsReadOnly; }

	CHtfeIdent * SetIsWriteOnly(bool bIsWriteOnly = true);
	bool IsWriteOnly() { return m_bIsParam && !m_bIsConst || m_bIsWriteOnly; }

	void SetIsMemVarWrData(bool bIsMemVarWrData = true) { m_bIsMemVarWrData = bIsMemVarWrData; }
	bool IsMemVarWrData() {
		if (m_pHierIdent) { return m_pHierIdent->IsMemVarWrData(); }
		return m_bIsMemVarWrData;
	}

	void SetIsMemVarWrEn(bool bIsMemVarWrEn = true) { m_bIsMemVarWrEn = bIsMemVarWrEn; }
	bool IsMemVarWrEn() {
		if (m_pHierIdent) { return m_pHierIdent->IsMemVarWrEn(); }
		return m_bIsMemVarWrEn;
	}

	void SetMemVar(CHtfeIdent * pMemVar) { m_pMemVar = pMemVar; }
	CHtfeIdent * GetMemVar() { 
		if (m_pHierIdent) { return m_pHierIdent->GetMemVar(); }
		return m_pMemVar;
	}

	void SetIsRemoveIfWriteOnly(bool bIsRemoveIfWriteOnly = true);
	bool IsRemoveIfWriteOnly() { return m_bIsRemoveIfWriteOnly; }

    CHtfeIdent *GetParenOperator() { return m_pParenOperator; }
    void SetParenOperator(CHtfeIdent *pParenOperator) { m_pParenOperator = pParenOperator; }

    CHtfeIdent *SetIsStatic(bool bIsStatic=true) { m_bIsStatic = bIsStatic; return this; }
    bool IsStatic() { return m_bIsStatic; }

    void SetPortDir(EPortDir portDir);
    EPortDir GetPortDir() const { return m_portDir; }

    void SetIsTypeDef(bool bIsTypeDef=true) { m_bIsTypeDef = bIsTypeDef; }
    bool IsTypeDef() { return m_bIsTypeDef; }

    void SetIsParam(bool bIsParam=true);
    bool IsParam() { return m_bIsParam; }

	int GetDimenElemCnt() {
		int elemCnt = 1;
		for (size_t i = 0; i < m_dimenList.size(); i += 1)
			elemCnt *= m_dimenList[i];
		return elemCnt;
	}
	string GetDimenElemName(size_t elemIdx) {
		string name;
		char dimStr[16];
		for (int dimIdx = (int)m_dimenList.size()-1; dimIdx >= 0; dimIdx -= 1) {
			sprintf(dimStr, "[%d]", (int)(elemIdx % m_dimenList[dimIdx]));
			elemIdx /= m_dimenList[dimIdx];
			name = dimStr + name;
		}
		name = GetName() + name;
		return name;
	}
	string GetVerilogName(size_t elemIdx, bool bWithName=true) {
		string name;
		char dimStr[16];
		for (int dimIdx = (int)m_dimenList.size()-1; dimIdx >= 0; dimIdx -= 1) {
			sprintf(dimStr, "$%d", (int)(elemIdx % m_dimenList[dimIdx]));
			elemIdx /= m_dimenList[dimIdx];
			name = dimStr + name;
		}
		if (bWithName)
			name = GetName() + name;
		return name;
	}
	int GetDimenElemIdx(CHtfeOperand * pOperand);
	int GetDimenElemIdx(vector<int> const & refList);

	void SetIndexList(vector<int> &indexList) { m_indexList = indexList; }
	vector<int> *GetIndexList() { return &m_indexList; }
	//void SetElemIdx(int elemIdx) { m_elemIdx = elemIdx; }
	int GetElemIdx() { return CalcElemIdx(); }

	int CalcElemIdx();

    void InitSeqState(ESeqState seqState) {
		if (m_pFlatIdent) { m_pFlatIdent->InitSeqState(seqState); return; }
		if (m_refInfo.size() == 0) m_refInfo.resize(GetDimenElemCnt());

		int elemCnt = GetDimenElemCnt();
		for (int i = 0; i < elemCnt; i += 1)
			m_refInfo[i].m_seqState = seqState;
	}
    void SetSeqState(ESeqState seqState, CHtfeOperand *pOperand);
    ESeqState GetSeqState(CHtfeOperand *pOperand);

    void SetIsClock(bool bIsClock=true) {
		m_bIsClock = bIsClock;
		if (m_pFlatIdent) m_pFlatIdent->SetIsClock(bIsClock);
	}
    bool IsClock() { return m_bIsClock; }

    void SetIsConst(bool bIsConst=true);
    bool IsConst() { return m_bIsConst; }

    void SetIsSeqCheck(bool bIsSeqCheck=true) { m_bIsSeqCheck = bIsSeqCheck; }
    bool IsSeqCheck() { return m_bIsSeqCheck; }

    void SetClockIdent(CHtfeIdent *pIdent) {
		m_pClockIdent = pIdent;
		if (m_pFlatIdent) m_pFlatIdent->SetClockIdent(pIdent);
	}
    CHtfeIdent *GetClockIdent() const { return m_pClockIdent; }

    void SetClkEnIdent(CHtfeIdent *pIdent) { m_pClkEnIdent = pIdent; }
    CHtfeIdent *GetClkEnIdent() const { return m_pClkEnIdent; }

    CHtfeLex::CTokenList *GetTokenList() { return m_pTokenList ? m_pTokenList : (m_pTokenList = new CHtfeLex::CTokenList); }

    void SetIsReference(bool bIsReference=true);
    bool IsReference() { return m_bIsReference; }

    void SetIsNoLoad(bool bIsHtNoLoad=true);
    bool IsNoLoad() { return m_bIsNoLoad; }
 
    void SetIsScState(bool bIsHtState=true) { m_bIsScState = bIsHtState; }
    bool IsScState() {
		return m_bIsScState || (m_pFlatIdent && m_pFlatIdent->IsScState());
	}

    void SetIsScPrim(bool bIsHtPrim=true);
    bool IsScPrim() { return m_bIsScPrim; }

    void SetIsScLocal(bool bIsHtLocal=true);
    bool IsScLocal() { return m_bIsScLocal; }

    void SetIsScPrimInput(bool bIsScPrimInput=true) { m_bIsScPrimInput |= bIsScPrimInput; }
    bool IsScPrimInput() { return m_bIsScPrimInput; }

    void SetIsPrimOutput(bool bIsPrimOutput=true) { m_bIsPrimOutput |= bIsPrimOutput; }
    bool IsPrimOutput() { return m_bIsPrimOutput; }

    void SetInstanceName(string &instanceName) { m_instanceName = instanceName; }
    string &GetInstanceName() { return m_instanceName; }

    void SetInstanceName(int elemIdx, string &instanceName) { m_refInfo[elemIdx].m_instanceName = instanceName; }
    string &GetInstanceName(int elemIdx) { return m_refInfo[elemIdx].m_instanceName; }

    void SetHtPrimClkList(vector<string> const & htPrimClkList);
    vector<string> & GetHtPrimClkList() { return m_htPrimClkList; }

    bool IsScClockedPrim() { return m_bIsScClockedPrim; }

    void SetIsInputFile(bool bIsInputFile=true) { m_bIsInputFile = bIsInputFile; }
    bool IsInputFile() { return m_bIsInputFile; }

    void SetIsAutoDecl(bool bIsAutoDecl=true) { m_bIsAutoDecl = bIsAutoDecl; }
    bool IsAutoDecl() { return m_bIsAutoDecl; }

    CHtfeIdent *InsertInstancePort(string &name);
    void SetPortSignal(CHtfeIdent *pPortSignal) { m_pPortSignal = pPortSignal; }
    CHtfeIdent *GetPortSignal() { return m_pPortSignal; }

    void SetSignalIdent(CHtfeIdent *pSignal) { m_pSignal = pSignal; }
    CHtfeIdent *GetSignalIdent() { return m_pSignal; }

    void SetAssigningMethod(CHtfeIdent *pAssigningMethod) { m_pAssigningMethod = pAssigningMethod; }
    CHtfeIdent *GetAssigningMethod() { return m_pAssigningMethod; }

	CHtfeStatement * GetMemInitStatements() { return m_pMemInitStatementHead; }
	void InsertMemInitStatements(CHtfeStatement *pStatement);

	void InsertStatementList(CHtfeStatement *pStatement);
	void AppendStatementList(CHtfeStatement *pStatement);
    CHtfeStatement **GetPStatementList() { return &m_pStatementHead; }
    CHtfeStatement *GetStatementList() { return m_pStatementHead; }
    const CHtfeStatement *GetStatementList() const { return m_pStatementHead; }

    void SetAssignmentStatement(CHtfeStatement *pStatement) { m_pAssignmentStatement = pStatement; }
    CHtfeStatement *GetAssignmentStatement() { return m_pAssignmentStatement; }

    void AddHtAttrib(string name, string inst, int msb, int lsb, string value, CLineInfo lineInfo)
    {
        if (m_pFlatIdent) {
            m_pFlatIdent->AddHtAttrib(name, inst, msb, lsb, value, lineInfo);
            return;
        }
        if (m_pHtAttribList == 0)
            m_pHtAttribList = new vector<CHtAttrib>;

        m_pHtAttribList->push_back(CHtAttrib(name, inst, msb, lsb, value, lineInfo));
    }
    int GetScAttribCnt() { return m_pHtAttribList == 0 ? 0 : m_pHtAttribList->size(); }
    CHtAttrib &GetScAttrib(int i) { Assert(m_pHtAttribList); return (*m_pHtAttribList)[i]; }

	void AddNullConstructor(CHtfeIdent * pParamType);
	void AddNullUserConversion(CHtfeIdent * pConvType);
	void AddBuiltinMember(string const & name, CHtfeIdent::EFuncId funcId, CHtfeIdent * pRtnType);
	void AddBuiltinMember(string const & name, CHtfeIdent::EFuncId funcId, CHtfeIdent * pRtnType, CHtfeIdent * pParam1Type, CHtfeIdent * pParam2Type);
	void AddNullConvOverloadedOperator(CHtfeLex::EToken eToken, CHtfeIdent * pRtnType, CHtfeIdent * pParamType);
	void AddNullConvOverloadedOperator(CHtfeLex::EToken eToken, CHtfeIdent * pRtnType, CHtfeIdent * pParam1Type, CHtfeIdent * pParam2Type);

	void SetNullConvOperator(bool bNullConvOperator = true) { m_bNullConvOperator = bNullConvOperator; }
	bool IsNullConvOperator() { return m_bNullConvOperator; }

	void SetIsConstructor(bool bIsConstructor = true) { m_bIsConstructor = bIsConstructor; }
	bool IsConstructor() { return m_bIsConstructor; }

	void SetIsOverloadedOperator(bool bIsOverloadedOperator = true) { m_bIsOverloadedOperator = bIsOverloadedOperator; }
	bool IsOverloadedOperator() { return m_bIsOverloadedOperator; }

	void SetOverloadedOperator(CHtfeLex::EToken overloadedOperator) { m_overloadedOperator = overloadedOperator; }
	CHtfeLex::EToken GetOverloadedOperator() { return m_overloadedOperator; }

	CHtfeIdent * SetUserConvType(CHtfeIdent * pUserConvType) { m_pUserConvType = pUserConvType; return this; }
	CHtfeIdent * GetUserConvType() { return m_pUserConvType; }
	CHtfeIdent * GetConvType()
	{
		CHtfeIdent * pType = this;
		while (pType->GetUserConvType())
			pType = pType->GetUserConvType();
		return pType;
	}

	void SetOverloadedNext(CHtfeIdent * pOverloadedNext) { m_pOverloadedNext = pOverloadedNext; }
	CHtfeIdent * GetOverloadedNext() { return m_pOverloadedNext; }

	void SetIsUserConversion(bool bIsUserConversion = true) { m_bIsUserConversion = bIsUserConversion; }
	bool IsUserConversion() { return m_bIsUserConversion; }

	void SetIsStruct(bool bIsStruct = true) { m_bIsStruct = bIsStruct; }
	bool IsStruct() const { return m_bIsStruct; }

	void SetIsUserDefinedType() { m_bIsUserDefinedType = true; }
	bool IsUserDefinedType() { return m_bIsUserDefinedType; }

	void SetIsUserDefinedFunc() { m_bIsUserDefinedFunc = true; }
	bool IsUserDefinedFunc() { return m_bIsUserDefinedFunc; }

	int GetNextUnnamedId() { return m_unnamedId++; }

	void SetStructPos(int structPos) { m_structPos = structPos; }
	int GetStructPos() { return m_structPos; }

	void SetIsHtDistQue(bool bIsHtDistQue=true);
	bool IsHtDistQue() { return m_bIsHtDistQue; }

	void SetIsHtBlockQue(bool bIsHtBlockQue=true);
	bool IsHtBlockQue() { return m_bIsHtBlockQue; }

	bool IsHtQueue() { return m_bIsHtDistQue || m_bIsHtBlockQue; }

	void SetIsHtQueuePushSeen() {
		m_bIsHtQueuePushSeen = true;
		if (m_pFlatIdent) m_pFlatIdent->SetIsHtQueuePushSeen();
	}
	bool IsHtQueuePushSeen() { return m_bIsHtQueuePushSeen; }

	void SetIsHtQueuePushClockSeen() {
		m_bIsHtQueuePushClockSeen = true;
		if (m_pFlatIdent) m_pFlatIdent->SetIsHtQueuePushClockSeen();
	}
	bool IsHtQueuePushClockSeen() { return m_bIsHtQueuePushClockSeen; }

	void SetIsHtQueuePopSeen() {
		m_bIsHtQueuePopSeen = true;
		if (m_pFlatIdent) m_pFlatIdent->SetIsHtQueuePopSeen();
	}
	bool IsHtQueuePopSeen() {
		return m_bIsHtQueuePopSeen;
	}

	void SetIsHtQueueFrontSeen() {
		m_bIsHtQueueFrontSeen = true;
		if (m_pFlatIdent) m_pFlatIdent->SetIsHtQueueFrontSeen();
	}
	bool IsHtQueueFrontSeen() {
		return m_bIsHtQueueFrontSeen;
	}

	void SetIsHtQueuePopClockSeen() {
		m_bIsHtQueuePopClockSeen = true;
		if (m_pFlatIdent) m_pFlatIdent->SetIsHtQueuePopClockSeen();
	}
	bool IsHtQueuePopClockSeen() { return m_bIsHtQueuePopClockSeen; }

	void SetIsHtMemoryReadAddrSeen() {
		m_bIsHtMemoryReadAddrSeen = true;
		if (m_pFlatIdent) m_pFlatIdent->SetIsHtMemoryReadAddrSeen();
	}
	bool IsHtMemoryReadAddrSeen() {
		return m_bIsHtMemoryReadAddrSeen;
	}

	void SetIsHtMemoryReadMemSeen() {
		m_bIsHtMemoryReadMemSeen = true;
		if (m_pFlatIdent) m_pFlatIdent->SetIsHtMemoryReadMemSeen();
	}
	bool IsHtMemoryReadMemSeen() {
		return m_bIsHtMemoryReadMemSeen;
	}

	void SetIsHtMemoryReadClockSeen() {
		m_bIsHtMemoryReadClockSeen = true;
		if (m_pFlatIdent) m_pFlatIdent->SetIsHtMemoryReadClockSeen();
	}
	bool IsHtMemoryReadClockSeen() {
		return m_bIsHtMemoryReadClockSeen;
	}

	void SetIsHtMemoryWriteAddrSeen() {
		m_bIsHtMemoryWriteAddrSeen = true;
		if (m_pFlatIdent) m_pFlatIdent->SetIsHtMemoryWriteAddrSeen();
	}
	bool IsHtMemoryWriteAddrSeen() {
		return m_bIsHtMemoryWriteAddrSeen;
	}

	void SetIsHtMemoryWriteMemSeen() {
		m_bIsHtMemoryWriteMemSeen = true;
		if (m_pFlatIdent) m_pFlatIdent->SetIsHtMemoryWriteMemSeen();
	}
	bool IsHtMemoryWriteMemSeen() {
		return m_bIsHtMemoryWriteMemSeen;
	}

	void SetIsHtMemoryWriteClockSeen() {
		m_bIsHtMemoryWriteClockSeen = true;
		if (m_pFlatIdent) m_pFlatIdent->SetIsHtMemoryWriteClockSeen();
	}
	bool IsHtMemoryWriteClockSeen() {
		return m_bIsHtMemoryWriteClockSeen;
	}

	void SetIsIgnoreSignalCheck(bool bIsIgnoreSignalCheck=true);
	bool IsIgnoreSignalCheck() { return m_bIsIgnoreSignalCheck; }

	void SetIsHtDistRam(bool bIsHtDistRam=true);
	bool IsHtDistRam() { return m_bIsHtDistRam; }

	void SetIsHtBlockRam(bool bIsHtBlockRam=true);
	void SetIsHtMrdBlockRam(bool bIsHtMrdBlockRam=true);
	void SetIsHtMwrBlockRam(bool bIsHtMwrBlockRam=true);
	void SetIsHtBlockRamDoReg(bool bIsHtBlockRam=true);
	bool IsHtBlockRam() { return m_bIsHtBlockRam; }
	bool IsHtMrdBlockRam() { return m_bIsHtMrdBlockRam; }
	bool IsHtMwrBlockRam() { return m_bIsHtMwrBlockRam; }
	bool IsHtBlockRamDoReg() { return m_bIsHtBlockRamDoReg; }
	bool IsHtMemory() { return m_bIsHtDistRam || m_bIsHtBlockRam || m_bIsHtMrdBlockRam || m_bIsHtMwrBlockRam; }

	void SetHtMemoryAddrWidth1(int addrWidth1);
	int GetHtMemoryAddrWidth1() { return m_scMemoryAddrWidth1; }

	void SetHtMemoryAddrWidth2(int addrWidth2);
	int GetHtMemoryAddrWidth2() { return m_scMemoryAddrWidth2; }

	void SetHtMemorySelWidth(int selWidth);
	int GetHtMemorySelWidth() { return m_scMemorySelWidth; }

	void SetIsArrayIdent(bool bIsArrayIdent=true) { m_bIsArrayIdent = bIsArrayIdent; }
	bool IsArrayIdent() { return m_bIsArrayIdent; }

	void SetIsFlatIdent(bool bIsFlatIdent=true) { m_bIsFlatIdent = bIsFlatIdent; }
	bool IsFlatIdent() { return m_bIsFlatIdent; }
	bool IsHierIdent() { return !m_bIsFlatIdent; }

	void		SetFlatIdent(CHtfeIdent *pFlatIdent) { m_pFlatIdent = pFlatIdent; }
	CHtfeIdent *	GetFlatIdent() { return m_pFlatIdent; }

	void		SetHierIdent(CHtfeIdent *pHierIdent) { m_pHierIdent = pHierIdent; }
	CHtfeIdent *	GetHierIdent() { return m_pHierIdent; }

	int				GetNextUniqueId() { return m_nextUniqueId++; }

	void		AddCaller(CHtfeIdent *pCaller) {
		Assert(!IsFlatIdent());
		Assert(IsFunction());
		pCaller = pCaller->FindHierFunction();
		Assert(pCaller && pCaller->IsFunction());

		if (m_pCallerList == 0)
			m_pCallerList = new vector<CHtfeIdent *>;
		else
			for (size_t i = 0; i < m_pCallerList->size(); i += 1)
				if ((*m_pCallerList)[i] == pCaller)
					return;
		m_pCallerList->push_back(pCaller);
	}
	const vector<CHtfeIdent *> &	GetCallerList() { return *m_pCallerList; }
	size_t GetCallerListCnt() { return m_pCallerList ? m_pCallerList->size() : 0; }

	void		AddWriter(CHtfeIdent *pWriter) {
		if (IsFlatIdent()) {
			m_pHierIdent->AddWriter(pWriter);
			return;
		}
		CHtfeIdent *pFunction = pWriter->FindHierFunction();
		Assert(pFunction);

		if (m_pWriterList == 0)
			m_pWriterList = new vector<CHtfeIdent *>;
		else
			for (size_t i = 0; i < m_pWriterList->size(); i += 1)
				if ((*m_pWriterList)[i] == pFunction)
					return;
		m_pWriterList->push_back(pFunction);
	}
	const vector<CHtfeIdent *> &	GetWriterList() { return *m_pWriterList; }
	size_t GetWriterListCnt() { return m_pWriterList ? m_pWriterList->size() : 0; }

	void		AddReader(CHtfeIdent *pReader) {
		if (pReader->IsModule())
			return;	// parsing a sensitivity variable
		if (IsFlatIdent()) {
			m_pHierIdent->AddReader(pReader);
			return;
		}

		CHtfeIdent *pFunction = pReader->FindHierFunction();
		Assert(pFunction);

		if (m_pReaderList == 0)
			m_pReaderList = new vector<CHtfeIdent *>;
		else
			for (size_t i = 0; i < m_pReaderList->size(); i += 1)
				if ((*m_pReaderList)[i] == pFunction)
					return;
		m_pReaderList->push_back(pFunction);
	}
	const vector<CHtfeIdent *> &	GetReaderList() { return *m_pReaderList; }
	size_t GetReaderListCnt() { return m_pReaderList ? m_pReaderList->size() : 0; }

	void		AddClock(CHtfeIdent *pClock) {
		if (m_pClockList == 0)
			m_pClockList = new vector<CHtfeIdent *>;
		m_pClockList->push_back(pClock);
	}
	const vector<CHtfeIdent *> &	GetClockList() { return *m_pClockList; }
	size_t GetClockListCnt() { return m_pClockList ? m_pClockList->size() : 0; }

	bool IsMultipleWriterErrorReported() { return m_bIsMultipleWriterErrorReported; }
	void SetIsMultipleWriterErrorReported() { m_bIsMultipleWriterErrorReported = true; }

	bool IsMultipleReaderErrorReported() { return m_bIsMultipleReaderErrorReported; }
	void SetIsMultipleReaderErrorReported() { m_bIsMultipleReaderErrorReported = true; }

	//int GetFieldVarIdxCnt() { return m_fieldVarIdxCnt; }
	//int IncFieldVarIdxCnt() { m_fieldVarIdxCnt += 1; return m_fieldVarIdxCnt-1; }

    void AddGlobalRef(CHtIdentElem &identElem);
    vector<CHtIdentElem> &GetGlobalRefSet() { return m_globalRefSet; }
	void MergeGlobalRefSet(vector<CHtIdentElem> &refSet);

	int GetNextBeginBlockId() { return ++m_beginBlockId; }

    void SetDeclStatementLevel(int level) {
        m_declStatementLevel = level;
        if (m_pFlatIdent)
            m_pFlatIdent->SetDeclStatementLevel(level);
    }
    int GetDeclStatementLevel() { return m_declStatementLevel; }

	bool IsMemberFunc() {
		return IsFunction() && GetPrevHier() && GetPrevHier()->IsStruct();
	}
	bool IsInlinedCall() {
		return GetGlobalRefSet().size() > 0 || IsMemberFunc();
	}

	CHtfeIdent * GetNextOverloadedFunc() { return 0; }

private:
    string				m_name;
	string				m_inlineName;		// name used for inlined function calls
    string				m_instanceName;
    vector<string>		m_htPrimClkList;
	vector<int>			m_dimenList;
    vector<CHtfeParam>	m_paramList;
	vector<CRefInfo>	m_refInfo;
	vector<int>			m_indexList;

	uint8_t			m_firstByte;
    EIdentId		m_id;
	CHtfeIdent *	m_pType;			// variable type
    CLineInfo		m_lineInfo;
    CHtfeIdent *	m_pPrevHier;
    CHtfeIdentTbl *	m_pIdentTbl;
	CHtfeIdentTbl *	m_pFlatTbl;
	CHtfeIdent *	m_pFlatIdent;
	CHtfeIdent *	m_pHierIdent;
	int				m_sizeof;
	int				m_beginBlockId;
	int				m_nextUniqueId;		// unique ID for flat ident table
	int				m_unnamedId;		// unique ID for unnamed structures within a heirarchy
    CScSensitiveTbl * m_pSensitiveTbl;
    CHtfeLex::EToken	m_sensitiveType;
    EPortDir		m_portDir;
	int				m_linearIdx;
    CHtfeIdent *		m_pParenOperator;
    CHtfeIdent *		m_pClockIdent;
    CHtfeIdent *		m_pClkEnIdent;
    CHtfeIdentTbl *	m_pInstancePortTbl;
    vector<CHtfeIdent *> *m_pFunctionCallList;
    vector<CHtIdentElem>  m_globalRefSet;	// list of global variables read by a function and called functions
    CHtfeLex::CTokenList *	m_pTokenList;
    CHtfeStatement *	m_pStatementHead;
	CHtfeStatement **	m_ppStatementInsertTail;
    CHtfeStatement **	m_ppStatementAppendTail;
    CHtfeStatement *	m_pAssignmentStatement;
	CHtfeStatement *	m_pMemInitStatementHead;
	CHtfeStatement ** m_ppMemInitStatementTail;
    CHtfeIdent *		m_pSignal;
    CHtfeIdent *		m_pAssigningMethod;
    CHtfeIdent *		m_pPortSignal;
	union {
		vector<CHtfeIdent *> *	m_pCallerList;	// list of callers (can be other functions or methods)
		vector<CHtfeIdent *> *	m_pWriterList;	// list of functions or methods where variable is writen
	};
	vector<CHtfeIdent *> *		m_pReaderList;	// list of functions or methods where variable is read
	vector<CHtfeIdent *> *		m_pClockList;	// list of functions or methods where variable is read
    vector<CHtAttrib> *			m_pHtAttribList;
    CConstValue		m_constValue;
    int				m_width;        // width of bit field types
    int				m_rdWidth;      // second width for asym block ram read type
	int				m_opWidth;		// width of integer operation 32, 64 or 256 (sc_biguint)
	int				m_structPos;	// starting position of member within structure
    int             m_declStatementLevel;
    EFuncId         m_funcId;
	CHtfeLex::EToken m_overloadedOperator;
	CHtfeIdent *	m_pUserConvType;
	CHtfeIdent *	m_pOverloadedNext;

	struct {
        bool m_bConstVarRead:1;
        bool m_bIsConstVar:1;
        bool m_bIsMethod:1;
        bool m_bIsBodyDefined:1;  // for function declarations
        bool m_bIsParamListDeclared:1;
        bool m_bIsType:1;
		bool m_bIsReturnRef:1;
        bool m_bIsVariableWidth:1;
        bool m_bIsModule:1;
        bool m_bIsScFixture:1;
        bool m_bIsScFixtureTop:1;
		bool m_bIsScTopology:1;
        bool m_bIsScTopologyTop:1;
        bool m_bIsRegister:1;
        bool m_bIsRegClkEn:1;
        bool m_bIsBlockRam:1;
        bool m_bIsLocal:1;  // function local type
        bool m_bIsCtor:1;
        bool m_bIsParenOverloaded:1;
        bool m_bIsMethodDefined:1;
        bool m_bIsStatic:1;
        bool m_bIsTypeDef:1;
        bool m_bIsParam:1;
        bool m_bIsConst:1;
		bool m_bIsSigned:1;
        bool m_bPromoteToSigned:1;
        bool m_bIsClock:1;
        bool m_bIsReference:1;
        bool m_bIsInputFile:1;
        bool m_bIsSeqCheck:1;
        bool m_bIsPrimOutput:1;
        bool m_bIsNoLoad:1;
		bool m_bIsReadOnly:1;
		bool m_bIsWriteOnly:1;
		bool m_bIsRemoveIfWriteOnly:1;
        bool m_bIsScState:1;
        bool m_bIsScLocal:1;
        bool m_bIsScPrim:1;
        bool m_bIsScClockedPrim:1;
        bool m_bIsScPrimInput:1;
		bool m_bIsUserDefinedType:1;
		bool m_bIsUserDefinedFunc:1;
		bool m_bIsAutoDecl:1;
		bool m_bIsFlatIdent:1;
		bool m_bIsArrayIdent:1;
		bool m_bIsMultipleWriterErrorReported:1;
		bool m_bIsMultipleReaderErrorReported:1;
		bool m_bIsIgnoreSignalCheck:1;
		bool m_bGlobalReadRef:1;
		bool m_bIsFunctionCalled:1;	// flag for recursive function call check
		bool m_bIsOverloadedOperator:1;
		bool m_bIsUserConversion:1;
		bool m_bNullConvOperator:1;
		bool m_bIsConstructor:1;
		bool m_bIsMemVarWrEn : 1;
		bool m_bIsMemVarWrData : 1;

		// id_types
		bool m_bIsStruct:1;
    };

	struct {	// HtQueue state
		bool m_bIsHtBlockQue:1;
		bool m_bIsHtDistQue:1;
		bool m_bIsHtQueueRam:1;
		bool m_bIsHtQueuePushSeen:1;
		bool m_bIsHtQueuePopSeen:1;
		bool m_bIsHtQueueFrontSeen:1;
		bool m_bIsHtQueuePushClockSeen:1;
		bool m_bIsHtQueuePopClockSeen:1;
	};

	struct {	// HtMemory state
		bool		m_bIsHtDistRam:1;
		bool		m_bIsHtBlockRam:1;
		bool		m_bIsHtMrdBlockRam:1;
		bool		m_bIsHtMwrBlockRam:1;
		bool		m_bIsHtBlockRamDoReg:1;
		bool		m_bIsHtMemoryReadAddrSeen:1;
		bool		m_bIsHtMemoryReadMemSeen:1;
		bool		m_bIsHtMemoryReadClockSeen:1;
		bool		m_bIsHtMemoryWriteAddrSeen:1;
		bool		m_bIsHtMemoryWriteMemSeen:1;
		bool		m_bIsHtMemoryWriteClockSeen:1;
		uint8_t		m_scMemorySelWidth;
		uint8_t		m_scMemoryAddrWidth1;
		uint8_t		m_scMemoryAddrWidth2;

		vector<CHtDistRamWeWidth> * m_pHtDistRamWeWidth;
		CHtfeIdent * m_pMemVar;
	};
	uint8_t			m_lastByte;

	static CKeywordMap	m_verilogKeywordTbl;
};

class CHtIdentElem {
public:
	CHtIdentElem() { m_pIdent = 0; }
	CHtIdentElem(CHtfeIdent * pIdent, int elemIdx) : m_pIdent(pIdent), m_elemIdx(elemIdx) {}
	bool operator < (const CHtIdentElem &b) const {
		if (m_pIdent->GetName() < b.m_pIdent->GetName())
			return true;
		if (m_pIdent->GetName() > b.m_pIdent->GetName())
			return false;
		if (m_elemIdx < b.m_elemIdx)
			return true;
		if (m_elemIdx > b.m_elemIdx)
			return false;
		return false;
	}
	bool operator > (const CHtIdentElem &b) const {
		if (m_pIdent->GetName() > b.m_pIdent->GetName())
			return true;
		if (m_pIdent->GetName() < b.m_pIdent->GetName())
			return false;
		if (m_elemIdx > b.m_elemIdx)
			return true;
		if (m_elemIdx < b.m_elemIdx)
			return false;
		return false;
	}
	bool operator == (const CHtIdentElem &b) const {
		return m_pIdent == b.m_pIdent && m_elemIdx == b.m_elemIdx;
	}
	bool Contains(const CHtIdentElem &b) const {
		return m_pIdent == b.m_pIdent && (m_elemIdx == -1 || b.m_elemIdx == -1 || m_elemIdx == b.m_elemIdx);
	}
public:
	CHtfeIdent *	m_pIdent;
	int			m_elemIdx;
};

typedef map<string, CHtfeIdent *>			IdentTblMap;
typedef pair<string, CHtfeIdent *>		IdentTblMap_ValuePair;
typedef IdentTblMap::iterator			IdentTblMap_Iter;
typedef IdentTblMap::const_iterator		IdentTblMap_ConstIter;
typedef pair<IdentTblMap_Iter, bool>	IdentTblMap_InsertPair;

/*********************************************************
** Identifier table
*********************************************************/

class CHtfeIdentTbl  
{
    friend class CHtfeIdentTblIter;
	friend class CScArrayTblIter;
	friend class CScElemTblIter;
public:
	CHtfeIdent * Insert(const string &str, bool bAllowOverloading=false);
    CHtfeIdent * Find(const string &str) {
        IdentTblMap_Iter iter = m_identTblMap.find(str);
        if (iter == m_identTblMap.end())
            return 0;
        return iter->second;
    }
	void Delete(const string &str) {
		m_identTblMap.erase(str);
	}
    int GetCount() { return m_identTblMap.size(); }

    IdentTblMap & GetIdentTblMap() { return m_identTblMap; }
    const IdentTblMap & GetIdentTblMap() const { return m_identTblMap; }
private:
    IdentTblMap		m_identTblMap;
};

class CHtfeIdentTblIter
{
public:
	CHtfeIdentTblIter() {}
    CHtfeIdentTblIter(CHtfeIdentTbl &identTbl) : m_pIdentTbl(&identTbl), m_iter(identTbl.m_identTblMap.begin()) {}
    void Begin() { m_iter = m_pIdentTbl->m_identTblMap.begin(); }
    bool End() { return m_iter == m_pIdentTbl->m_identTblMap.end(); }
    void operator ++ (int) { m_iter++; }
    CHtfeIdent* operator ->() { return m_iter->second; }
    CHtfeIdent* operator () () { return m_iter->second; }
private:
    CHtfeIdentTbl *m_pIdentTbl;
    IdentTblMap_Iter m_iter;
};

class CHtfeOperatorIter {
public:
	CHtfeOperatorIter() {}
	CHtfeOperatorIter(CHtfeIdent * pOpType, bool bHier = false) : m_pOpType(pOpType), m_bHier(bHier), m_bFirst(true) {}
	void Begin() { m_bFirst = true; m_pMethod = 0; }
	bool End() { return !m_bFirst && m_iter == m_pOpType->GetIdentTbl().GetIdentTblMap().end(); }
	void operator ++ (int) {
		do {
			if (m_bFirst) {
				m_bFirst = false;
				m_iter = m_pOpType->GetIdentTbl().GetIdentTblMap().begin();
				SetMethod();
			} else {
				if (m_pMethod->GetOverloadedNext())
					m_pMethod = m_pMethod->GetOverloadedNext();
				else {
					m_iter++;
					SetMethod();
				}
			}
			if (m_bHier && m_pOpType->GetPrevHier() && End()) {
				m_pOpType = m_pOpType->GetPrevHier();
				m_iter = m_pOpType->GetIdentTbl().GetIdentTblMap().begin();
				SetMethod();
			}
		} while (!End() && !IsOverloadedOperator() && !IsUserConversion() && !IsConstructor());
	}

	bool IsUserConversion() { return m_bFirst ? false : GetMethod()->IsUserConversion(); }
	bool IsOverloadedOperator() { return m_bFirst ? false : GetMethod()->IsOverloadedOperator(); }
	bool IsConstructor() { return m_bFirst ? false : GetMethod()->IsConstructor() && GetMethod()->GetParamCnt() == 1; }
	CHtfeLex::EToken GetOverloadedOperator() { return GetMethod()->GetOverloadedOperator(); }
	CHtfeIdent * GetMethod() { return m_pMethod; }
	CHtfeIdent * GetType() {
		CHtfeIdent * pType = 0;
		if (m_bFirst)
			pType = m_pOpType;
		else if (GetMethod()->IsOverloadedOperator()) {
			pType = GetMethod()->GetParamType(0);
			return pType;
		} else if (GetMethod()->IsUserConversion()) {
			pType = GetMethod()->GetType();
			return pType;
		} else if (GetMethod()->IsConstructor())
			pType = GetMethod()->GetParamType(0);
		else {
			Assert(0);
			pType = 0;
		}
		// Conv ht_uint, ht_int, sc_in, sc_out to an int type
		return pType;
		return pType->GetConvType();
	}
	int GetConvCnt() { return m_bFirst ? 0 : 1; }
private:
	void SetMethod() { m_pMethod = End() ? 0 : m_iter->second; }
private:
	CHtfeIdent * m_pOpType;
	bool m_bHier;
	bool m_bFirst;
	IdentTblMap_Iter m_iter;
	CHtfeIdent * m_pMethod;
};

class CScElemTblIter
{
public:
	CScElemTblIter(CHtfeIdentTbl &identTbl) : m_pIdentTbl(&identTbl) { Begin(); }
	void Begin() {
		if (m_pIdentTbl->GetCount() == 0) {
			m_iter = m_pIdentTbl->m_identTblMap.end();
			return;
		}
		m_iter = m_pIdentTbl->m_identTblMap.begin();
		m_indexElem.m_pIdent = m_iter->second;
		m_indexElem.m_elemIdx = 0;
	}
	bool End() { return m_iter == m_pIdentTbl->m_identTblMap.end(); }
	void operator ++ (int) {
		m_indexElem.m_elemIdx += 1;
		if (m_indexElem.m_elemIdx == m_indexElem.m_pIdent->GetDimenElemCnt()) {
			m_iter++;
			if (m_iter != m_pIdentTbl->m_identTblMap.end())
				m_indexElem.m_pIdent = m_iter->second;
			m_indexElem.m_elemIdx = 0;
		}
	}
	CHtIdentElem* operator ->() { return &m_indexElem; }
    CHtIdentElem operator () () { return m_indexElem; }
private:
	CHtIdentElem		m_indexElem;
    CHtfeIdentTbl *		m_pIdentTbl;
    IdentTblMap_Iter	m_iter;
};

class CScArrayTblIter
{
public:
    CScArrayTblIter(CHtfeIdentTbl &identTbl) : m_pIdentTbl(&identTbl) { Begin(); }
    void Begin() {
		m_bArrayTbl = false;
		m_pArrayTbl = 0;
		if (m_pIdentTbl->GetCount() == 0) {
			m_iter = m_pIdentTbl->m_identTblMap.end();
			return;
		}
		m_iter = m_pIdentTbl->m_identTblMap.begin();

		if (m_iter->second->GetDimenCnt() > 0) {
			m_bArrayTbl = m_iter->second->GetIdentTbl().GetCount() > 0;
			if (m_bArrayTbl)
				m_arrayIter = m_iter->second->GetIdentTbl().GetIdentTblMap().begin();
		}
	}
    bool End() { return !m_bArrayTbl && m_iter == m_pIdentTbl->m_identTblMap.end(); }
    void operator ++ (int) {
		if (m_bArrayTbl) {
			m_arrayIter++;
			if (m_arrayIter == m_pArrayTbl->GetIdentTblMap().end()) {
				m_bArrayTbl = false;
				m_iter++;
			} else
				return;
		} else
			m_iter++;

		if (m_iter == m_pIdentTbl->m_identTblMap.end())
			return;

		if (m_iter->second->GetDimenCnt() > 0) {
			if (m_iter->second->IsFlatIdent()) {
				m_bArrayTbl = m_iter->second->GetFlatIdentTbl().GetCount() > 0;
				m_pArrayTbl = &m_iter->second->GetFlatIdentTbl();
			} else {
				m_bArrayTbl = m_iter->second->GetIdentTbl().GetCount() > 0;
				m_pArrayTbl = &m_iter->second->GetIdentTbl();
			}
			if (m_bArrayTbl)
				m_arrayIter = m_pArrayTbl->GetIdentTblMap().begin();
		}
	}
	CHtfeIdent* operator ->() { return m_bArrayTbl ? m_arrayIter->second : m_iter->second; }
    CHtfeIdent* operator () () { return m_bArrayTbl ? m_arrayIter->second : m_iter->second; }
private:
	bool				m_bArrayTbl;
    CHtfeIdentTbl *		m_pIdentTbl;
	IdentTblMap_Iter	m_arrayIter;
    IdentTblMap_Iter	m_iter;
	CHtfeIdentTbl *		m_pArrayTbl;
};


class CScArrayIdentIter
{
public:
    CScArrayIdentIter(CHtfeIdent *pIdent, CHtfeIdentTbl &identTbl) : m_pIdent(pIdent), m_pIdentTbl(&identTbl) { Begin(); }
    void Begin() {
		if (m_pIdentTbl && m_pIdentTbl->GetCount() > 0) {
			m_iter = m_pIdentTbl->GetIdentTblMap().begin();
			m_bArrayTbl = true;
			m_bFirst = false;
		} else {
			m_bArrayTbl = false;
			m_bFirst = true;
		}
	}
	bool End() { return m_bArrayTbl ? (m_iter == m_pIdentTbl->GetIdentTblMap().end()) : !m_bFirst; }
    void operator ++ (int) {
		if (m_bArrayTbl)
			m_iter++;
		else
			m_bFirst = false;
	}
	CHtfeIdent* operator ->() { return m_bArrayTbl ? m_iter->second : m_pIdent; }
    CHtfeIdent* operator () () { return m_bArrayTbl ? m_iter->second : m_pIdent; }
private:
    IdentTblMap_Iter	m_iter;
	CHtfeIdent *			m_pIdent;
	bool				m_bFirst;
	bool				m_bArrayTbl;
    CHtfeIdentTbl *		m_pIdentTbl;
};

typedef map<CHtIdentElem, void *>			SensitiveTblMap;
typedef pair<CHtIdentElem, void *>			SensitiveTblMap_ValuePair;
typedef SensitiveTblMap::iterator			SensitiveTblMap_Iter;
typedef SensitiveTblMap::const_iterator		SensitiveTblMap_ConstIter;
typedef pair<SensitiveTblMap_Iter, bool>	SensitiveTblMap_InsertPair;

class CScSensitiveTbl
{
    friend class CScSensitiveTblIter;
public:
	void Insert(CHtfeIdent * pIdent, int elemIdx) {
        m_sensitiveTblMap.insert(SensitiveTblMap_ValuePair(CHtIdentElem(pIdent, elemIdx), (void *)0));
	}
	int GetCount() { return m_sensitiveTblMap.size(); }

    SensitiveTblMap & GetSensitiveTblMap() { return m_sensitiveTblMap; }
    const SensitiveTblMap & GetSensitiveTblMap() const { return m_sensitiveTblMap; }
public:
    SensitiveTblMap		m_sensitiveTblMap;
};

class CScSensitiveTblIter
{
public:
	CScSensitiveTblIter() {}
    CScSensitiveTblIter(CScSensitiveTbl &sensitiveTbl) : m_pSensitiveTbl(&sensitiveTbl), m_iter(sensitiveTbl.m_sensitiveTblMap.begin()) {}
    void Begin() { m_iter = m_pSensitiveTbl->m_sensitiveTblMap.begin(); }
    bool End() { return m_iter == m_pSensitiveTbl->m_sensitiveTblMap.end(); }
    void operator ++ (int) { m_iter++; }
    const CHtIdentElem* operator ->() { return &m_iter->first; }
    const CHtIdentElem & operator () () { return m_iter->first; }
private:
    CScSensitiveTbl *m_pSensitiveTbl;
    SensitiveTblMap_Iter m_iter;
};
