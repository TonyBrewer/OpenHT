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

class CHtfeIdent;

/////////////////////////////////////////////////////////////////
// Operand class for expression evaluation
//

typedef vector<CHtfeOperand *>		HtfeOperandList_t;

struct CScSubField {
	CScSubField() {}
	CScSubField(CHtfeIdent *pIdent, HtfeOperandList_t &indexList, int subFieldWidth, bool bBitIndex=false) 
		: m_pIdent(pIdent), m_indexList(indexList), m_subFieldWidth(subFieldWidth), m_bBitIndex(bBitIndex), m_pNext(0) {}

	CHtfeIdent *		m_pIdent;
	HtfeOperandList_t	m_indexList;
	int					m_subFieldWidth;
	bool				m_bBitIndex;
	CScSubField	*		m_pNext;
};

class CHtfeOperand {
protected:
    CHtfeOperand() { Init(); }

public:
    ~CHtfeOperand() {
        if (m_pOperand1) delete m_pOperand1;
        if (m_pOperand2) delete m_pOperand2;
        if (m_pOperand3) delete m_pOperand3;
        for (unsigned int i = 0; i < m_paramList.size(); i += 1)
            delete m_paramList[i];
    }
    void Init() {
        m_pMember = 0; m_minWidth = 0; m_sigWidth = 0; m_bIsConstValue = false; m_bIsConstWidth = false;
        m_pOperand1 = 0; m_pOperand2 = 0; m_pOperand3 = 0; m_bIsScX = false; m_bIsSigned = false; m_bIsPosSignedConst = false;
        m_bIsLeaf = false; m_bIsParenExpr = false; m_exprWidth = 0; m_pMemberType = 0; m_pCastType = 0;
		m_pSubField = 0; m_operatorToken = CHtfeLex::tk_eof; m_bIsErrOp = false; m_verWidth = 0; m_opWidth = 0; m_pLinkedOp = 0;
    }

	void InitAsOperator(CLineInfo const & lineInfo, CHtfeLex::EToken op, CHtfeOperand * pOp1 = 0, CHtfeOperand * pOp2 = 0, CHtfeOperand * pOp3 = 0)
	{
		SetLineInfo(lineInfo);
		SetOperator(op);
		SetOperand1(pOp1);
		SetOperand2(pOp2);
		SetOperand3(pOp3);
	}

	void InitAsFunction(CLineInfo const & lineInfo, CHtfeIdent * pMember)
	{
		SetLineInfo(lineInfo);
		SetIsLeaf();
		SetMember(pMember);
	}

	void InitAsIdentifier(CLineInfo const & lineInfo, CHtfeIdent * pMember)
	{
		SetLineInfo(lineInfo);
		SetIsLeaf();
		SetMember(pMember);
	}

	void InitAsConstant(CLineInfo const & lineInfo, CConstValue const & value) {
		SetLineInfo(lineInfo);
		SetIsLeaf();
		SetConstValue(value);
	}

	void InitAsString(CLineInfo const & lineInfo, string & name) {
		SetLineInfo(lineInfo);
		SetIsLeaf();
		m_string = name;
	}

    CHtfeIdent *GetVariable() { return (m_pMember && m_pMember->IsVariable()) ? m_pMember : 0; }
    CHtfeIdent *GetFunction() { return (m_pMember && m_pMember->IsFunction()) ? m_pMember : 0; }
    CHtfeIdent *GetMember() {
        if (!m_bIsLeaf && m_operatorToken == CHtfeLex::tk_typeCast)
            return m_pOperand2->GetMember();
        else
            return m_pMember;
    }
    const CHtfeIdent *GetMember() const {
        if (!m_bIsLeaf && m_operatorToken == CHtfeLex::tk_typeCast)
            return m_pOperand2->GetMember();
        else
            return m_pMember;
    }
   	void SetCastType(CHtfeIdent *pCastType) { m_pCastType = pCastType; }
	CHtfeIdent *GetCastType() { return m_pCastType; }
 	void SetMemberType(CHtfeIdent *pMemberType) { m_pMemberType = pMemberType; }
	CHtfeIdent *GetMemberType() {
		if (m_pMemberType)
			return m_pMemberType;
        else if (m_pMember)
            return m_pMember->IsType() ? m_pMember : m_pMember->GetType();
        else if (!m_bIsLeaf && m_operatorToken == CHtfeLex::tk_typeCast)
            return m_pOperand1->GetMemberType();
        else
            return 0;
    }
	void SetLinkedOp(CHtfeOperand * pLinkedOp) { m_pLinkedOp = pLinkedOp; }
    CHtfeOperand &SetReadRef() {
        if (m_pMember && m_pMember->IsVariable())
            m_pMember->SetReadRef(this);
        return *this;
    }
    CHtfeOperand &SetWriteRef() {
        if (m_pMember && m_pMember->IsVariable())
            m_pMember->SetWriteRef(this);
        return *this;
    }

    bool IsVariable() { return m_pMember && m_pMember->IsVariable(); }

    bool IsFunction() { return m_pMember && m_pMember->IsFunction(); }
    bool IsConstValue() const {
        return m_bIsConstValue ||
            m_pMember && (m_pMember->IsIdEnum() || m_pMember->IsIdConst());
    }
    bool IsUnknownWidth() { return GetMinWidth() <= 0; }
	void SetIsSigned(bool bIsSigned) { m_bIsSigned = bIsSigned; }
	bool IsSigned() {
		//if (IsLeaf())
		//	return IsConstValue() ? GetConstValue().IsSigned() : GetMemberType()->IsSigned();
		//else
			return m_bIsSigned;
	}
    const CConstValue GetConstValue() const {
        if (m_bIsConstValue)
            return m_constValue;
        else if (m_pMember && (m_pMember->IsIdEnum() || m_pMember->IsIdConst()))
            return m_pMember->GetConstValue();	
        else {
            Assert(0);
            return m_constValue;
        }
    }
    CConstValue &GetConstValueRef() {
        if (m_bIsConstValue)
            return m_constValue;
        else if (m_pMember && (m_pMember->IsIdEnum() || m_pMember->IsIdConst()))
            return m_pMember->GetConstValue();	
        else {
            Assert(0);
            return m_constValue;
        }
    }
    void SetConstWidth(bool isConstWidth) {
        m_bIsConstWidth = isConstWidth;
		//printf("SetConstWidth(%d)\n", isConstWidth);
    }
    bool IsConstWidth() { return m_bIsConstWidth || IsConstValue(); }
	void SetIsPosSignedConst(bool bIsPosSignedConst) {
		m_bIsPosSignedConst = bIsPosSignedConst;
	}
	bool IsPosSignedConst() { return m_bIsPosSignedConst || IsConstValue() && GetConstValue().IsPos(); }

	void SetIsConstValue(bool bIsConstValue) { m_bIsConstValue = bIsConstValue; }
    void SetConstValue(const CConstValue &value) {
        m_bIsConstValue = true;
        m_constValue = value;
    }
    void SetMinWidth(int minWidth) {
        m_minWidth = minWidth;
    }
    int GetMinWidth() const {
        if (m_minWidth > 0) 
            return m_minWidth;
        if (m_pMember) 
            return m_pMember->GetMinWidth();
        if (IsConstValue())
            return GetConstValue().GetMinWidth();
        return 0;
    }
    void SetSigWidth(int sigWidth) {
        m_sigWidth = sigWidth;
    }
    int GetSigWidth() const {
		if (m_sigWidth > 0) return m_sigWidth;
		return IsConstValue() ? GetConstValue().GetSigWidth() : GetMinWidth();
    }
	int GetWidth() {
		if (IsConstValue())
			return GetConstValue().GetMinWidth();
		if (m_pMember)
			return m_pMember->GetWidth();
		Assert(0);
		return 0;
	}
	void SetOpWidth(int opWidth) {
		m_opWidth = opWidth;
	}
	int GetOpWidth() {
		return m_opWidth;
	}
	void SetVerWidth(int verWidth) {
		m_verWidth = verWidth;
	}
	int GetVerWidth() {
		return m_verWidth;
	}
    void SetIsLeaf(bool bIsLeaf=true) { m_bIsLeaf = bIsLeaf; }
    bool IsLeaf() const { return m_bIsLeaf; }

	void SetIsErrOp() { m_bIsErrOp = true; }
	bool IsErrOp() const { return m_bIsErrOp; }

    void SetMember(CHtfeIdent *pMember) {
        if (!m_bIsLeaf && m_operatorToken == CHtfeLex::tk_typeCast)
            m_pOperand2->SetMember(pMember);
        else {
            m_pMember = pMember; //m_arrayDimension = pMember->GetDimenCnt();
        }
    }

    void SetBitIdxOperand(CHtfeOperand *pBitIdxOp) { m_paramList.push_back(pBitIdxOp); }

    void SetParamList(vector<CHtfeOperand *> &paramList) {
        if (!m_bIsLeaf && m_operatorToken == CHtfeLex::tk_typeCast)
            m_pOperand2->SetParamList(paramList);
        else
            m_paramList = paramList;
    }
    vector<CHtfeOperand *> &GetParamList() { return m_paramList; }
	int GetParamListCnt() { return (int)m_paramList.size(); }

	size_t GetDimenCnt() { return m_indexList.size(); }
	int64_t GetDimenIdx(int idx) { Assert(m_indexList[idx]->IsConstValue()); return m_indexList[idx]->GetConstValue().GetSint64(); }
	CHtfeOperand * GetDimenIndex(int dimenIdx) { return m_indexList[dimenIdx]; }
	void SetIndexList(vector<CHtfeOperand *> &indexList) { m_indexList = indexList; }
    vector<CHtfeOperand *> &GetIndexList() { return m_indexList; }

    void SetOperator(CHtfeLex::EToken token) { m_operatorToken = token; }
    CHtfeLex::EToken GetOperator() const { return m_operatorToken; }

    void SetOperand1(CHtfeOperand *pOperand1) { m_pOperand1 = pOperand1; }
    void SetOperand2(CHtfeOperand *pOperand2) { m_pOperand2 = pOperand2; }
    void SetOperand3(CHtfeOperand *pOperand3) { m_pOperand3 = pOperand3; }

    CHtfeOperand *GetOperand1() { return m_pOperand1; }
    CHtfeOperand *GetOperand2() { return m_pOperand2; }
    CHtfeOperand *GetOperand3() { return m_pOperand3; }

    const CHtfeOperand *GetOperand1() const { return m_pOperand1; }
    const CHtfeOperand *GetOperand2() const { return m_pOperand2; }
    const CHtfeOperand *GetOperand3() const { return m_pOperand3; }

	CHtfeOperand *GetOperand(int idx) {
		switch (idx) { 
			case 0: return m_pOperand1;
			case 1: return m_pOperand2;
			case 2: return m_pOperand3;
			default: Assert(0);
		}
		return 0;
	}

	void SetOperand(int idx, CHtfeOperand *pOperand) {
		switch (idx) { 
			case 0: m_pOperand1 = pOperand; break;
			case 1: m_pOperand2 = pOperand; break;
			case 2: m_pOperand3 = pOperand; break;
			default: Assert(0);
		}
	}

    void SetIsParenExpr(bool bIsParenExpr=true) {
        if (!m_bIsLeaf && m_operatorToken == CHtfeLex::tk_typeCast)
            m_pOperand2->SetIsParenExpr(bIsParenExpr);
        else
            m_bIsParenExpr = bIsParenExpr;
    }
    bool IsParenExpr() { return m_bIsParenExpr; }

    string &GetString() { return m_string; }

    void SetLineInfo(const CLineInfo &lineInfo) { m_lineInfo = lineInfo; }
    const CLineInfo &GetLineInfo() { return m_lineInfo; }

	void SetIsScX() { m_bIsScX = true; }
	bool IsScX() { return m_bIsScX; }

	CScSubField ** GetSubFieldP() { return & m_pSubField; }
	CScSubField * GetSubField() { return m_pSubField; }

    bool IsLinkedOp() { return m_pLinkedOp != 0; }
	CHtfeOperand * GetLinkedOp() { return m_pLinkedOp; }

	int GetSubFieldWidth();

	bool IsSubFieldValid() { return m_pSubField != 0; }

	void GetDistRamWeWidth(int &highBit, int &lowBit) {
		highBit = m_distRamWeWidth.m_highBit;
		lowBit = m_distRamWeWidth.m_lowBit;
	}
	void SetDistRamWeWidth(short highBit, short lowBit) {
		m_distRamWeWidth = CHtDistRamWeWidth(highBit,  lowBit);
	}

	void operator delete (void * pVoid) {
		CHtfeOperand *pOp = (CHtfeOperand *)pVoid;
		if (pOp->IsErrOp())
			return;

		::delete(pOp);
		//free(pOp);
	}

private:
    CHtfeIdent *              m_pMember;
	CHtfeIdent *              m_pMemberType;	// valid if pMember is a field or signal read()/write()
	CHtfeIdent *              m_pCastType;	// valid if type cast is present

	CHtfeOperand *            m_pLinkedOp;	// linked operand for common subexpression generation

    // m_minWidth - minimum width of operand, bottom up
    //   Set by variable width, subrange width, lower expression tree width
    //   Zero means unknown
    int                     m_minWidth;

    // m_sigWidth - significant width of operand, bottom up
    int                     m_sigWidth;

    // m_exprWidth - width that expression requires, top down
    //   Set by an assignment, function parameters, cast operator,
    //   if otherwise not forces, then set to widest operand in expression
    int						m_exprWidth;

	// m_opWidth - width of operation (C/C++ 32, 64, 256)
	int						m_opWidth;

	// width of verilog expression, generated as temp variables are determined
	int						m_verWidth;

    bool					m_bIsConstValue;
    bool					m_bIsConstWidth;
	bool					m_bIsPosSignedConst;
	bool					m_bIsErrOp;
    bool					m_bIsLeaf;
    bool					m_bIsParenExpr;
    CConstValue				m_constValue;
    string					m_string;
    vector<CHtfeOperand *>	m_paramList;
    vector<CHtfeOperand *>	m_indexList;
    CHtfeLex::EToken			m_operatorToken;
    CHtfeOperand *			m_pOperand1;
    CHtfeOperand *			m_pOperand2;
    CHtfeOperand *			m_pOperand3;
    CLineInfo				m_lineInfo;
	//int						m_fieldVarIdx;
	CScSubField *			m_pSubField;
	CHtDistRamWeWidth		m_distRamWeWidth;

	struct {
		bool	m_bIsSigned:1;
		bool	m_bIsScX:1;
	};
};
