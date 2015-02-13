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

/////////////////////////////////////////////////////////////////
// Statement class
//
enum EStType { st_null, st_return, st_assign, st_if, st_switch, st_case, st_default, st_break, st_compound };

class CHtfeStatement {
protected:
    CHtfeStatement() {}

public:
	void Init(EStType stType, CLineInfo const &lineInfo) {
		m_lineInfo = lineInfo;
		m_stType = stType;
		m_pNext = 0;
		m_pExpr = 0;
        m_pCompound1 = 0;
		m_pCompound2 = 0;
		m_bIsScPrim = false;
		m_bInitStatement = false;
	}

    const CLineInfo &GetLineInfo() { return m_lineInfo; }
	void SetLineInfo(const CLineInfo &lineInfo) { m_lineInfo = lineInfo; }

    EStType GetStType() const { return m_stType; }
	void SetStType(EStType stType) { m_stType = stType; }

    CHtfeStatement **GetPNext() { return &m_pNext; }
    CHtfeStatement *GetNext() { return m_pNext; }
    const CHtfeStatement *GetNext() const { return m_pNext; }
	void SetNext(CHtfeStatement *pNext) { m_pNext = pNext; }

    CHtfeStatement **GetPCompound1() { return &m_pCompound1; }
    CHtfeStatement **GetPCompound2() { return &m_pCompound2; }

    void SetExpr(CHtfeOperand *pExpr) { m_pExpr = pExpr; }
    CHtfeOperand *GetExpr() { return m_pExpr; }
    const CHtfeOperand *GetExpr() const { return m_pExpr; }

    void SetCompound1(CHtfeStatement *pCompound1) { m_pCompound1 = pCompound1; }
    void SetCompound2(CHtfeStatement *pCompound2) { m_pCompound2 = pCompound2; }
    CHtfeStatement *GetCompound1() { return m_pCompound1; }
    const CHtfeStatement *GetCompound1() const { return m_pCompound1; }
    CHtfeStatement *GetCompound2() { return m_pCompound2; }

    void SetIsScPrim(bool bIsHtPrim=true) { m_bIsScPrim = bIsHtPrim; }
    bool IsScPrim() { return m_bIsScPrim; }

    bool HasScLocal() { return false; }

 	bool IsInitStatement() { return m_bInitStatement; }
	void SetIsInitStatement(bool bIsInitStatement=true) { m_bInitStatement = bIsInitStatement; }

private:
    EStType         m_stType;
	CLineInfo		m_lineInfo;
    CHtfeStatement    *m_pNext;
    CHtfeOperand      *m_pExpr;
    CHtfeStatement    *m_pCompound1;
    CHtfeStatement    *m_pCompound2;
    bool		    m_bIsScPrim;
	bool			m_bInitStatement;
};
