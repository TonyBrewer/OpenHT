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
// Parameter Info for function parameter list
//

class CHtfeParam {
public:
    CHtfeParam(CHtfeIdent *pType, bool bIsConst, bool bIsReference, bool bIsHtState, string &name) {
		m_pType = pType; m_name = name; m_bIsConst = bIsConst; m_bIsScState = bIsHtState; m_bIsReference = bIsReference;
		m_pIdent = 0; m_bHasDefaultValue = false;
	}
    CHtfeParam(CHtfeIdent *pType, bool bIsConst, string &name) {
		m_pType = pType; m_name = name; m_bIsConst = bIsConst; m_bIsScState = false; m_bIsReference = false;
		m_pIdent = 0; m_bHasDefaultValue = false; 
	}
    CHtfeParam(CHtfeIdent *pType, bool bIsConst)
    { 
		m_pType = pType; m_name = ""; m_bIsConst = bIsConst; m_bIsScState = false;
		m_bIsReference = false; m_pIdent = 0; m_bNoload = false; m_bHasDefaultValue = false;
	}

    CHtfeIdent	*GetType() { return m_pType; }
    string		&GetName() { return m_name; }
    bool		GetIsConst() { return m_bIsConst; }
	bool		GetIsReference() { return m_bIsReference; }
    bool		GetIsScState() { return m_bIsScState; }
    void		SetName(const string &name) { m_name = name; }
    int			GetWidth();
    void		SetIdent(CHtfeIdent *pIdent) { m_pIdent = pIdent; }
    CHtfeIdent	*GetIdent() { return m_pIdent; }
	void		SetDefaultValue(CConstValue & value) {
					m_bHasDefaultValue = true;
					m_defaultValue = value;
				}
	bool		HasDefaultValue() { return m_bHasDefaultValue; }
	CConstValue & GetDefaultValue() { Assert(m_bHasDefaultValue); return m_defaultValue; }
	void		SetIsNoload(bool bNoload=true) { m_bNoload = bNoload; }
	bool		IsNoload() { return m_bNoload; }

private:
    CHtfeIdent * m_pType;
    string		m_name;
    bool		m_bIsConst;
	bool		m_bIsReference;
    bool		m_bIsScState;
    CHtfeIdent * m_pIdent;
	bool		m_bNoload;
	bool		m_bHasDefaultValue;
	CConstValue m_defaultValue;
};
