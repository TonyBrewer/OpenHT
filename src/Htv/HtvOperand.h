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

class CHtvOperand : public CHtfeOperand
{
public:
	CHtvOperand() : CHtfeOperand() {
		m_bIsSubExprRequired = false; m_bIsSubExprGenerated = false; m_bIsExprTempVarWidth = false;
		m_bIsArrayTempGenerated = false; m_bIsFieldTempGenerated = false;
		m_bIsFuncGenerated = false; m_pTempOp = 0;
	}

	CHtvOperand * GetLinkedOp() { return (CHtvOperand *)CHtfeOperand::GetLinkedOp(); }

	CHtvOperand * GetOperand1() { return (CHtvOperand *)CHtfeOperand::GetOperand1(); }
	CHtvOperand * GetOperand2() { return (CHtvOperand *)CHtfeOperand::GetOperand2(); }
	CHtvOperand * GetOperand3() { return (CHtvOperand *)CHtfeOperand::GetOperand3(); }

	CHtvIdent * GetMember() { return (CHtvIdent *)CHtfeOperand::GetMember(); }
	CHtvIdent *GetCastType() { return (CHtvIdent *)CHtfeOperand::GetCastType(); }
	CHtvIdent *GetMemberType() { return (CHtvIdent *)CHtfeOperand::GetMemberType(); }

	size_t GetParamCnt() { return GetParamList().size(); }
	CHtvOperand * GetParam(int i) { return (CHtvOperand *)(GetParamList()[i]); }

	size_t GetIndexCnt() { return GetIndexList().size(); }
	CHtvOperand * GetIndex(int i) { return (CHtvOperand *)(GetIndexList()[i]); }

	void SetIsSubExprRequired(bool bFuncFound) {
		m_bIsSubExprRequired = bFuncFound;
	}
	bool IsSubExprRequired() { 
		return m_bIsSubExprRequired;
	}

    bool IsExprTempVarSigned() {
        return m_bExprTempVarSigned;
    }
	bool HasExprTempVar() { return m_exprTempVar.size() > 0; }
	void SetExprTempVar(string exprTempVar, bool bSigned) {
		m_exprTempVar = exprTempVar;
        m_bExprTempVarSigned = bSigned;
	}
	void SetIsExprTempVarSigned(bool bSigned) {
        m_bExprTempVarSigned = bSigned;
	}
	string GetExprTempVar() {
		return m_exprTempVar;
	}
	bool HasArrayTempVar() { return m_arrayTempVar.size() > 0; }
	void SetArrayTempVar(string arrayTempVar) {
		bool bLinkedOp = IsLinkedOp() && arrayTempVar.size() > 0 && !HasArrayTempVar();

		m_arrayTempVar = arrayTempVar;

		if (bLinkedOp)
			GetLinkedOp()->SetArrayTempVar(arrayTempVar);
	}
	string GetArrayTempVar() {
		return m_arrayTempVar;
	}
	bool IsArrayTempGenerated() { return m_bIsArrayTempGenerated; }
	void SetArrayTempGenerated(bool bGenerated) {
		if (m_bIsArrayTempGenerated == bGenerated)
			return;
		m_bIsArrayTempGenerated = bGenerated;
		if (IsLinkedOp())
			GetLinkedOp()->SetArrayTempGenerated(bGenerated);
	}

    bool IsFuncGenerated() {
        return m_bIsFuncGenerated;
    }
    void SetIsFuncGenerated(bool bGenerated) {
        m_bIsFuncGenerated = bGenerated;
    }

	// A function that returns a value has a funcTempVar that is assigned the value
	// A function that returns a reference does not have a funcTempVar
	bool HasFuncTempVar() { return m_funcTempVar.size() > 0; }
	void SetFuncTempVar(string funcTempVar, bool bSigned) {
		m_funcTempVar = funcTempVar;
        m_bFuncTempVarSigned = bSigned;
	}
	string GetFuncTempVar() {
		return m_funcTempVar;
	}

	bool HasFieldTempVar() { return m_fieldTempVar.size() > 0; }
	void SetFieldTempVar(string fieldTempVar) {
		bool bLinkedOp = IsLinkedOp() && fieldTempVar.size() > 0 && !HasFieldTempVar();

		m_fieldTempVar = fieldTempVar;

		if (bLinkedOp)
			GetLinkedOp()->SetFieldTempVar(fieldTempVar);
	}
	string GetFieldTempVar() {
		return m_fieldTempVar;
	}

	bool IsFieldTempGenerated() { return m_bIsFieldTempGenerated; }
	void SetFieldTempGenerated(bool bGenerated) {
		if (m_bIsFieldTempGenerated == bGenerated)
			return;
		m_bIsFieldTempGenerated = bGenerated;
		if (IsLinkedOp())
			GetLinkedOp()->SetFieldTempGenerated(bGenerated);
	}

	void SetIsSubExprGenerated(bool bGenerated) {
		m_bIsSubExprGenerated = bGenerated;
	}
	bool IsSubExprGenerated() {
		return m_bIsSubExprGenerated;
	}

	void SetExprTempVarWidth(int exprTempVarWidth) {
		m_bIsExprTempVarWidth = true;
        m_exprTempVarWidth = exprTempVarWidth;
	}
	bool IsExprTempVarWidth() {
		return m_bIsExprTempVarWidth;
	}
    int GetExprTempVarWidth() {
        return m_exprTempVarWidth;
    }
	void SetTempOp(CHtvOperand * pTempOp)
	{
		m_pTempOp = pTempOp;
	}
	CHtvOperand * GetTempOp()
	{
		return m_pTempOp;
	}
	void SetTempOpSubGened(bool bGened = true)
	{
		m_bTempOpSubGened = bGened;
	}
	bool IsTempOpSubGened() { return m_bTempOpSubGened; }

private:
	bool					m_bIsSubExprRequired;
	bool					m_bIsSubExprGenerated;
	bool					m_bIsExprTempVarWidth;	// specifies that temp var needs subfield indexing
	string					m_exprTempVar;
    int                     m_exprTempVarWidth;
    bool                    m_bExprTempVarSigned;
	bool					m_bIsArrayTempGenerated;
	bool					m_bIsFieldTempGenerated;
	string					m_arrayTempVar;
	string					m_fieldTempVar;
	string					m_funcTempVar;
    bool                    m_bIsFuncGenerated;
    bool                    m_bFuncTempVarSigned;
	CHtvOperand *			m_pTempOp;
	bool					m_bTempOpSubGened;
};
